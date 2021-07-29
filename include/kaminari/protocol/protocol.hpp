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


    class protocol : public basic_protocol
    {
    public:
        using basic_protocol::basic_protocol;

        template <typename Queues>
        bool initiate_handshake(::kaminari::super_packet<Queues>* super_packet);

        template <typename Queues>
        bool update(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);

        template <typename Marshal, typename TimeBase, uint64_t interval, typename Queues>
        bool read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);

        template <typename TimeBase, typename Queues>
        void handle_acks(super_packet_reader& reader, ::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);

        inline bool is_out_of_order(uint16_t id);

    private:
        template <typename Queues>
        void increase_expected(::kaminari::super_packet<Queues>* super_packet);

        template <typename Marshal, typename TimeBase, uint64_t interval, typename Queues>
        void read_impl(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);
    };

    template <typename Queues>
    bool initiate_handshake(::kaminari::super_packet<Queues>* super_packet)
    {
        super_packet->set_flag(::kaminari::super_packet_flags::handshake);
    }

    template <typename Queues>
    bool protocol::update(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        bool needs_ping = basic_protocol::update();

        if (super_packet->finish() || needs_ping)
        {
            if (needs_ping)
            {
                scheduled_ping();
            }

            _send_timestamps.emplace(super_packet->id(), std::chrono::steady_clock::now());
            return true;
        }

        // If there is no new superpacket, erase it from the map
        _send_timestamps.erase(super_packet->id());
        return false;
    }

    template <typename Marshal, typename TimeBase, uint64_t interval, typename Queues>
    bool protocol::read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        // Update timestamp
        _timestamp_block_id = _expected_block_id;
        _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        if (!client->has_pending_super_packets())
        {
            // Increment expected eiter way
            increase_expected(super_packet);

            if (++_since_last_recv > max_blocks_until_disconnection())
            {
                client->flag_disconnection();
                return false;
            }

            return false;
        }

        // There is something, whatever, so we've recv
        _since_last_recv = 0;

        // Should we buffer?
        uint16_t expected_id = cx::overflow::sub(_expected_block_id, static_cast<uint16_t>(_buffer_size));

        // Keep reading superpackets until we reach the currently expected
        while (client->has_pending_super_packets() &&
            !cx::overflow::geq(client->first_super_packet_id(), expected_id))
        {
            read_impl<Marshal, TimeBase, interval>(client, super_packet);
        }

        // Flag for next block
        increase_expected(super_packet);

        return true;
    }

    template <typename TimeBase, typename Queues>
    void protocol::handle_acks(super_packet_reader& reader, ::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        // Handle flags immediately
        bool is_handshake = reader.has_flag(kaminari::super_packet_flags::handshake);
        if (is_handshake)
        {
            // Acks have no implication for us, but non-acks mean we have to ack
            if (!reader.has_flag(kaminari::super_packet_flags::ack))
            {
                super_packet->set_flag(kaminari::super_packet_flags::ack);
                super_packet->set_flag(kaminari::super_packet_flags::handshake);
            }
        }

        // Acknowledge user acks
        reader.handle_acks<TimeBase>(super_packet, this, client);

        // Let's add it to pending acks
        if (is_handshake || reader.has_data() || reader.is_ping_packet())
        {
            super_packet->schedule_ack(reader.id());
        }
    }

    template <typename Queues>
    void protocol::increase_expected(::kaminari::super_packet<Queues>* super_packet)
    {
        if (!super_packet->has_flag(kaminari::super_packet_flags::handshake))
        {
            _expected_block_id = cx::overflow::inc(_expected_block_id);
        }
    }
    
    template <typename Marshal, typename TimeBase, uint64_t interval, typename Queues>
    void protocol::read_impl(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        super_packet_reader reader = client->first_super_packet();
		
        // Handshake process skips all procedures, including order
        if (reader.has_flag(kaminari::super_packet_flags::handshake))
        {
			// Make sure we are ready for the next valid block
            _expected_block_id = cx::overflow::inc(reader.id());

            // Reset all variables related to packet parsing
            _timestamp_block_id = _expected_block_id;
            _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
            _loop_counter = 0;
            _already_resolved.clear();
            
            // Do nothing
            _last_block_id_read = reader.id();
            return;
        }

        // Unordered packets are not to be parsed, as they contain outdated information
        // in case that information was important, it would have already been resent and not added
        assert(!is_out_of_order(reader.id()) && "An out of order packet should never reach here");

        // Check how old the packet is wrt what we expect
        if (cx::overflow::sub(_expected_block_id, reader.id()) > max_blocks_until_resync())
        {
            client->flag_desync();
        }

        // Detect loop
        if (_last_block_id_read > reader.id()) // Ie. 65536 > 0
        {
            _loop_counter = cx::overflow::inc(_loop_counter);
        }

        // Handle all inner packets
        _last_block_id_read = reader.id();
        reader.handle_packets<Marshal, TimeBase, interval>(client, this);
    }

    inline bool protocol::is_out_of_order(uint16_t id)
    {
        return cx::overflow::le(id, _last_block_id_read);
    }
}
