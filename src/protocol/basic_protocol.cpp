#include <kaminari/protocol/basic_protocol.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/buffers/packet_reader.hpp>
#include <kaminari/client/basic_client.hpp>
#include <kaminari/super_packet.hpp>


namespace kaminari
{
    basic_protocol::basic_protocol() noexcept
    {
        reset();
    }

    bool basic_protocol::update() noexcept
    {
        _since_last_ping += 1;
        return needs_ping();
    }

    void basic_protocol::reset() noexcept
    {
        _buffer_size = 0;
        _since_last_ping = 0;
        _since_last_recv = 0;
        _last_block_id_read = 0;
        _expected_block_id = 0;
        _timestamp = 0;
        _timestamp_block_id = 0;
        _already_resolved.clear();
        _loop_counter = 0;
        _max_blocks_until_resync = 200;
        _max_blocks_until_disconnection = 300;
        _ping_interval = 20;
    }

    bool basic_protocol::resolve(basic_client* client, buffers::packet_reader* packet, uint16_t block_id) noexcept
    {
        auto opcode = packet->opcode();
        uint32_t extended_id = packet->extended_id();

        if (auto it = _already_resolved.find(block_id); it != _already_resolved.end())
        {
            auto& info = it->second;

            // Clear structure
            if (info.loop_counter != _loop_counter)
            {
                // Maybe we are still in the turning point
                if (cx::overflow::sub(_last_block_id_read, block_id) > _max_blocks_until_resync)
                {
                    info.loop_counter = _loop_counter;
                    info.extended_counter.clear();
                }
            }
            // Resync detection
            else if (cx::overflow::sub(_last_block_id_read, block_id) > _max_blocks_until_resync)
            {
                client->flag_desync();
                return false;
            }

            // Check if block has already been parsed
            if (auto jt = info.extended_counter.find(extended_id); jt != info.extended_counter.end())
            {
                return false;
            }

            info.extended_counter.insert(extended_id);
        }
        else
        {
            _already_resolved.emplace(block_id, resolved_block {
                .loop_counter = _loop_counter,
                .extended_counter = std::set<uint32_t> { extended_id }
            });
        }
        
        return true;
    }


    std::optional<std::chrono::steady_clock::time_point> basic_protocol::super_packet_timestamp(uint16_t block_id) noexcept
    {
        if (auto it = _send_timestamps.find(block_id); it != _send_timestamps.end())
        {
            auto ts = it->second;
            _send_timestamps.erase(block_id);
            return ts;
        }

        return std::nullopt;
    }
}
