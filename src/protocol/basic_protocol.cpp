#include "kaminari/cx/overflow.hpp"
#include <kaminari/protocol/basic_protocol.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/buffers/packet_reader.hpp>
#include <kaminari/client/basic_client.hpp>
#include <kaminari/super_packet.hpp>
#include <optional>


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
        _expected_tick_id = 0;
        _timestamp = 0;
        _timestamp_block_id = 0;
        _resolution_table.fill(0);
        _oldest_resolution_block_id = 0;
        _oldest_resolution_position = 0;
        _max_blocks_until_resync = 200;
        _max_blocks_until_disconnection = 300;
        _ping_interval = 20;
        _timestamps.fill(std::chrono::steady_clock::now());
        _timestamp_head_position = 0;
        _timestamp_head_id = 0;
        _last_confirmed_timestamp_id = 0;
    }

    bool basic_protocol::resolve(basic_client* client, buffers::packet_reader* packet, uint16_t block_id) noexcept
    {
        // Check if this is an older block id
        if (cx::overflow::le(block_id, _oldest_resolution_block_id))
        {
            reset_resolution_table(block_id);
            client->flag_desync();
            return false;
        }

        // Otherwise, it might be newer
        uint16_t diff = cx::overflow::sub(block_id, _oldest_resolution_block_id);
        uint16_t idx = cx::overflow::add(_oldest_resolution_position, diff) % resolution_table_size;
        if (diff >= resolution_table_size)
        {
            // We have to move oldest so that newest points to block_id
            auto move_amount = cx::overflow::sub(diff, resolution_table_diff);
            _oldest_resolution_block_id = cx::overflow::add(_oldest_resolution_block_id, move_amount);
            _oldest_resolution_position = cx::overflow::add(_oldest_resolution_position, move_amount) % resolution_table_size;

            // Fix diff so we don't overrun the new position
            idx = cx::overflow::add(_oldest_resolution_position, cx::overflow::sub(diff, move_amount)) % resolution_table_size;

            // Clean position, as it is a newer packet that hasn't been parsed yet
            _resolution_table[idx] = 0;
        }

        // Compute packet mask
        uint64_t mask = static_cast<uint64_t>(1) << packet->counter();

        // Get block_id position, bitmask, and compute
        if (_resolution_table[idx] & mask)
        {
            // The packet is already in
            return false;
        }
        
        _resolution_table[idx] |= mask;
        return true;
    }

    void basic_protocol::reset_resolution_table(uint16_t block_id) noexcept
    {
        _resolution_table.fill(0);
        _oldest_resolution_block_id = cx::overflow::sub(block_id, static_cast<uint16_t>(resolution_table_size / 2));
        _oldest_resolution_position = 0;
    }

    std::optional<std::chrono::steady_clock::time_point> basic_protocol::super_packet_timestamp(uint16_t block_id) noexcept
    {
        // TODO(gpascualg): Configurable/non-hardcoded max blocks diff for 
        if (cx::overflow::geq(_last_confirmed_timestamp_id, block_id) && cx::overflow::sub(_timestamp_head_id, _last_confirmed_timestamp_id) < 100)
        {
            return std::nullopt;
        }

        _last_confirmed_timestamp_id = block_id;
        uint16_t position = cx::overflow::sub(_timestamp_head_position, cx::overflow::sub(_timestamp_head_id, block_id)) % resolution_table_size;
        return _timestamps[position];
    }
}
