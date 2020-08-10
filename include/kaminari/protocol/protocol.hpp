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

    // TODO(gpascualg): Make this configurable via protocol
    inline constexpr uint16_t MaxBlocksUntilDisconnection = 300;
    inline constexpr uint16_t MaximumBlocksUntilResync = 200;


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
        // bool needs_ping = ++_since_last_send >= PingInterval;
        // if (needs_ping)
        // {
        //     _since_last_send = 0;
        //     auto timestamp = ::now();
        //     rpc::ping(client, { .timestamp = static_cast<uint64_t>(timestamp.time_since_epoch().count()) }, [timestamp, ticket = client->ticket()]() {
        //         if (!ticket->valid())
        //         {
        //             return;
        //         }

        //         // Now update the client lag as a running mean, divide by two to count only half round trip
        //         auto cl = ticket->get();
        //         auto elapsed = std::chrono::duration_cast<TimeBase>(::now() - timestamp);
        //         auto ms = elapsed.count();
        //         cl->lag(static_cast<uint64_t>(
        //             static_cast<float>(cl->lag()) * 0.9 +
        //             static_cast<float>(ms) / 2 * 0.1)
        //         );
        //         log<LOG_LEVEL_DEBUG, LOG_CLIENT>(std::forward_as_tuple("Client updated lag estimation is ", cl->lag()));
        //     });
        // }

        return super_packet->finish();
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
            if (++_since_last_recv > MaxBlocksUntilDisconnection)
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
        uint16_t current_id = reader.id();

        // Unordered packets are not to be parsed, as they contain outdated information
        // in case that information was important, it would have already been resent
        if (!is_expected(current_id))
        {
            // It is still a recv, though
            _since_last_recv = 0;
            return true;
        }

        // Acknowledge user acks
        reader.iterate_acks([super_packet](auto ack)
        {
            super_packet->ack(ack);
        });

        // Let's add it to pending acks
        if (reader.has_data())
        {
            super_packet->schedule_ack(current_id);
        }

        // Update timestamp
        // TODO(gpascualg): Enforce steady clock?
        _timestamp_block_id = current_id;
        _timestamp = std::chrono::steady_clock::now().time_since_epoch().count();

        // Actually handle inner packets
        reader.handle_packets<Marshal>(client, this);

        // TODO(gpascualg): This can still be improved by much
        // Clear all resolved packets ranging from last received ID to current minus Resync threshold
        for (auto id = _last_block_id_read; cx::overflow::le(id, current_id);)
        {
            _already_resolved.erase(cx::overflow::sub(id, MaximumBlocksUntilResync));
            id = cx::overflow::inc(id);
        }

        _last_block_id_read = current_id;
        _expected_block_id = cx::overflow::inc(current_id);
        _since_last_recv = 0;
        return true;
    }
}
