#pragma once

#include <kaminari/protocol/basic_protocol.hpp>
#include <kaminari/cx/overflow.hpp>
#include <kaminari/super_packet.hpp>
#include <kaminari/client/basic_client.hpp>
#include <kaminari/client/client.hpp>
#include <kaminari/super_packet_reader.hpp>

#include <memory>
#include <unordered_map>
#include <set>


namespace kaminari
{
    namespace buffers
    {
        class packet_reader;
    }

#if defined __has_include
    #if __has_include(<kumo/marshal.hpp>)
        constexpr inline bool use_kumo_buffers = true;
    #else
        constexpr inline bool use_kumo_buffers = false;
    #endif
#else
    #pragma warning "Your compiler does not support __has_include, define USE_KUMO_BUFFERS to override default behaviour"
    #if defined USE_KUMO_BUFFERS
        constexpr inline bool use_kumo_buffers = true;
    #else
        constexpr inline bool use_kumo_buffers = false;
    #endif
#endif


    class protocol : public basic_protocol
    {
    public:
        using basic_protocol::basic_protocol;

        template <typename Queues>
        void initiate_handshake(::kaminari::super_packet<Queues>* super_packet);

        template <typename Queues>
        bool update(uint16_t tick_id, ::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);

        template <typename TimeBase, uint64_t interval, typename Marshal, typename Queues>
        bool read(::kaminari::basic_client* client, Marshal& marshal, ::kaminari::super_packet<Queues>* super_packet);

        template <typename TimeBase, typename Queues, typename Marshal>
        void handle_acks(uint16_t tick_id, super_packet_reader& reader, ::kaminari::basic_client* client, Marshal& marshal,::kaminari::super_packet<Queues>* super_packet);

        inline bool is_out_of_order(uint16_t id);

    private:
        template <typename Queues>
        void increase_expected(::kaminari::super_packet<Queues>* super_packet);

        template <typename TimeBase, uint64_t interval, typename Marshal, typename Queues>
        void read_impl(::kaminari::basic_client* client, Marshal& marshal, ::kaminari::super_packet<Queues>* super_packet);
    };

    template <typename Queues>
    void protocol::initiate_handshake(::kaminari::super_packet<Queues>* super_packet)
    {
        super_packet->set_flag(::kaminari::super_packet_flags::handshake);
    }

    template <typename Queues>
    bool protocol::update(uint16_t tick_id, ::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        bool needs_ping = basic_protocol::update();
        if (needs_ping)
        {
            super_packet->set_flag(kaminari::super_packet_flags::ping);
            scheduled_ping();
        }

        bool first_packet = true;
        super_packet->prepare();
        while (super_packet->finish(tick_id, first_packet))
        {
            // Move head
            uint16_t packet_id_diff = cx::overflow::sub(super_packet->id(), _timestamp_head_id);
            _timestamp_head_position = cx::overflow::add(_timestamp_head_position, packet_id_diff) % resolution_table_size;
            _timestamps[_timestamp_head_position] = std::chrono::steady_clock::now();
            _timestamp_head_id = super_packet->id();

            first_packet = false;

            // If there is nothing else to send, done
            if (!super_packet->last_left_data())
            {
                break;
            }
        }

        // TODO(gpascualg): This is, at the very least, ugly. Try to simplify buffer freeing logic
        // first_packet will only be true if no superpacket has been finished
        //  that is, if there is nothing to send
        if (first_packet)
        {
            // Reaching here means there is nothing to send, but the superpacket increased
            //  its write pointer, which means we must increase read pointer to compensate
            super_packet->free(super_packet->next_buffer());
        }

        // We have updates if first_packet is false
        return !first_packet;
    }

    template <typename TimeBase, uint64_t interval, typename Marshal, typename Queues>
    bool protocol::read(::kaminari::basic_client* client, Marshal& marshal, ::kaminari::super_packet<Queues>* super_packet)
    {
        // Update timestamp
        _timestamp_block_id = _expected_tick_id;
        _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        if (!client->has_pending_super_packets())
        {
            if (++_since_last_recv > max_blocks_until_disconnection())
            {
                client->flag_disconnection();
                return false;
            }

            // Update marshal just in case
            marshal.update(client, _expected_tick_id);
            _expected_tick_id = cx::overflow::inc(_expected_tick_id);
            return false;
        }

        // There is something, whatever, so we've recv
        _since_last_recv = 0;

        // When using kumo we are not buffering here, otherwise do so
        uint16_t expected_id = _expected_tick_id;
        if constexpr (!use_kumo_buffers)
        {
            expected_id = cx::overflow::sub(_expected_tick_id, static_cast<uint16_t>(_buffer_size));
        }

        // Keep reading superpackets until we reach the currently expected
        while (client->has_pending_super_packets() &&
            !cx::overflow::ge(client->first_super_packet_tick_id(), expected_id))
        {
            read_impl<TimeBase, interval>(client, marshal, super_packet);
        }

        // Now update marshal if it has too
        marshal.update(client, _expected_tick_id);
        _expected_tick_id = cx::overflow::inc(_expected_tick_id);
        return true;
    }

    template <typename TimeBase, typename Queues, typename Marshal>
    void protocol::handle_acks(uint16_t tick_id, super_packet_reader& reader, ::kaminari::basic_client* client, Marshal& marshal, ::kaminari::super_packet<Queues>* super_packet)
    {
        // Acknowledge user acks
        reader.handle_acks<TimeBase>(super_packet, this, client);

        // Let's add it to pending acks
        bool is_handshake = reader.has_flag(kaminari::super_packet_flags::handshake);
        if (is_handshake || reader.has_data() || reader.is_ping_packet())
        {
            super_packet->schedule_ack(reader.id());
        }

        // Handle flags immediately
        if (is_handshake)
        {
            // TODO(gpascualg): Remove re-handshake max diff magic number
            if (cx::overflow::abs_diff(reader.tick_id(), tick_id) > 10)
            {
                super_packet->set_flag(super_packet_flags::handshake);
            }

            // Make sure we are ready for the next valid block
            _expected_tick_id = reader.tick_id();
            _last_tick_id_read = reader.tick_id();

            // Overwrite default
            _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            reset_resolution_table(reader.tick_id());
            marshal.reset();

            // Acks have no implication for us, but non-acks mean we have to ack
            if (!reader.has_flag(kaminari::super_packet_flags::ack))
            {
                super_packet->set_flag(kaminari::super_packet_flags::ack);
                super_packet->set_flag(kaminari::super_packet_flags::handshake);
            }
        }
    }

    template <typename Queues>
    void protocol::increase_expected(::kaminari::super_packet<Queues>* super_packet)
    {
        if (!super_packet->has_flag(kaminari::super_packet_flags::handshake))
        {
            _expected_tick_id = cx::overflow::inc(_expected_tick_id);
        }
    }
    
    template <typename TimeBase, uint64_t interval, typename Marshal, typename Queues>
    void protocol::read_impl(::kaminari::basic_client* client, Marshal& marshal, ::kaminari::super_packet<Queues>* super_packet)
    {
        super_packet_reader reader = client->first_super_packet();
		
        // Handshake process skips all procedures, including order
        // TODO(gpascualg): Can we skip this check?
        if (reader.has_flag(kaminari::super_packet_flags::handshake))
        {
            _last_tick_id_read = reader.tick_id();
            return;
        }

        // Unordered packets are not to be parsed, as they contain outdated information
        // in case that information was important, it would have already been resent and not added
        assert(!is_out_of_order(reader.tick_id()) && "An out of order packet should never reach here");

        // Check how old the packet is wrt what we expect
        if (cx::overflow::sub(_expected_tick_id, reader.tick_id()) > max_blocks_until_resync())
        {
            initiate_handshake(super_packet);
            client->flag_desync();
        }

        // Handle all inner packets
        _last_tick_id_read = reader.tick_id();
        reader.handle_packets<TimeBase, interval>(client, marshal, this);
    }

    inline bool protocol::is_out_of_order(uint16_t id)
    {
        if constexpr (use_kumo_buffers)
        {
            return cx::overflow::le(id, cx::overflow::sub(_last_tick_id_read, static_cast<uint16_t>(_buffer_size)));
        }
        else
        {
            return cx::overflow::le(id, _last_tick_id_read);
        }
    }
}
