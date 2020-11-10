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

    void basic_protocol::reset() noexcept
    {
        _buffer_size = 0;
        _since_last_send = 0;
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

    bool basic_protocol::resolve(basic_client* client, packet_reader* packet, uint16_t block_id) noexcept
    {
        auto opcode = packet->opcode();
        uint8_t id = packet->id();

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
                    info.packet_counters.clear();
                }
            }
            // Resync detection
            else if (cx::overflow::sub(_last_block_id_read, block_id) > _max_blocks_until_resync)
            {
                client->flag_desync();
                return false;
            }

            // Check if block has already been parsed
            if (auto jt = info.packet_counters.find(id); jt != info.packet_counters.end())
            {
                return false;
            }

            info.packet_counters.insert(id);
        }
        else
        {
            _already_resolved.emplace(block_id, resolved_block {
                .loop_counter = _loop_counter,
                .packet_counters = std::set<uint8_t> { id }
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
            return std::move(ts);
        }

        return std::nullopt;
    }
}
