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
    class packet_reader;


    class protocol : public basic_protocol
    {
    public:
        using basic_protocol::basic_protocol;

        template <typename Queues>
        bool update(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);


        template <typename Marshal, typename TimeBase, typename Queues>
        bool read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);

    private:
        template <typename Marshal, typename TimeBase, typename Queues>
        void read_impl(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);
    };

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

    template <typename Marshal, typename TimeBase, typename Queues>
    bool protocol::read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        // Update timestamp
        _timestamp_block_id = _expected_block_id;
        _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        if (!client->has_pending_super_packets())
        {
            // Increment expected eiter way
            _expected_block_id = cx::overflow::inc(_expected_block_id);

            if (++_since_last_recv > _max_blocks_until_disconnection)
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
            read_impl<Marshal, TimeBase>(client, super_packet);
        }

        // Flag for next block
        _expected_block_id = cx::overflow::inc(_expected_block_id);

        return true;
    }

    template <typename Marshal, typename TimeBase, typename Queues>
    void protocol::read_impl(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        super_packet_reader reader(client->first_super_packet());

        // Unordered packets are not to be parsed, as they contain outdated information
        // in case that information was important, it would have already been resent
        if (cx::overflow::le(reader.id(), _last_block_id_read))
        {
            return;
        }

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

        // Update block id
        _last_block_id_read = reader.id();

        // Acknowledge user acks
        reader.handle_acks<TimeBase>(super_packet, this, client);

        // Let's add it to pending acks
        if (reader.has_data() || reader.is_ping_packet())
        {
            super_packet->schedule_ack(_last_block_id_read);
        }

        // Actually handle inner packets
        reader.handle_packets<Marshal>(client, this);
    }
}
