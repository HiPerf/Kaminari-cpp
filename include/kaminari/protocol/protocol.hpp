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
        template <typename Marshal, typename Queues>
        bool read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet);
    };

    template <typename Queues>
    bool protocol::update(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        if (super_packet->finish())
        {
            _send_timestamps.emplace(super_packet->id(), std::chrono::steady_clock::now());
            return true;
        }
        
        // If there is no new superpacket, erase it from the map
        _send_timestamps.erase(super_packet->id());
        return false;
    }

    template <typename Marshal, typename Queues>
    bool protocol::read(::kaminari::basic_client* client, ::kaminari::super_packet<Queues>* super_packet)
    {
        if (_buffer_mode == BufferMode::BUFFERING)
        {
            // TODO(gpascualg): Implement packet buffering
            //assert_true(false, "BufferMode::BUFFERING not yet supported");
            return false;
        }

        if (!client->has_pending_super_packets())
        {
            if (++_since_last_recv > _max_blocks_until_disconnection)
            {
                return false;
            }

            return false;
        }

        if (_buffer_mode == BufferMode::READY)
        {
            // TODO(gpascualg): Reading from the buffered packets, respecting order
            // assert_true(false, "BufferMode::READY not yet supported");
            return false;
        }

        super_packet_reader reader(client->first_super_packet());

        // Unordered packets are not to be parsed, as they contain outdated information
        // in case that information was important, it would have already been resent
        if (!is_expected(reader.id()))
        {
            // It is still a recv, though
            _since_last_recv = 0;
            return true;
        }

        uint16_t previous_id = _last_block_id_read;
        _last_block_id_read = reader.id();

        // Detect loop
        if (previous_id > _last_block_id_read)
        {
            _loop_counter = cx::overflow::inc(_loop_counter);
        }

        // Acknowledge user acks
        reader.handle_acks(super_packet, this, client);

        // Let's add it to pending acks
        if (reader.has_data())
        {
            super_packet->schedule_ack(_last_block_id_read);
        }

        // Update timestamp
        _timestamp_block_id = _last_block_id_read;
        _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        // Actually handle inner packets
        reader.handle_packets<Marshal>(client, this);

        _last_block_id_read = current_id;
        _expected_block_id = cx::overflow::inc(current_id);
        _since_last_recv = 0;
        return true;
    }
}
