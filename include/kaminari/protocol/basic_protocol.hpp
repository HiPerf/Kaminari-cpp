#pragma once

#include <kaminari/cx/overflow.hpp>

#include <inttypes.h>
#include <set>
#include <unordered_map>


namespace kaminari
{
    class packet_reader;

    enum class BufferMode
    {
        NO_BUFFER,
        BUFFERING,
        READY
    };
    

    class basic_protocol
    {
    public:
        static inline uint16_t superpacket_interval = 50;

    public:
        basic_protocol() noexcept;

        void reset() noexcept;
        bool resolve(packet_reader* packet, uint16_t block_id) noexcept;

        inline uint16_t last_block_id_read() const noexcept;
        inline uint16_t expected_block_id() const noexcept;
        inline bool is_expected(uint16_t id) const noexcept;

        inline void set_timestamp(uint64_t timestamp, uint16_t block_id) noexcept;
        inline uint64_t block_timestamp(uint16_t block_id) noexcept;

        inline uint16_t max_blocks_until_resync() const noexcept;
        inline void max_blocks_until_resync(uint16_t value) noexcept;

        inline uint16_t max_blocks_until_disconnection() const noexcept;
        inline void max_blocks_until_disconnection(uint16_t value) noexcept;

    protected:
        BufferMode _buffer_mode;
        uint16_t _since_last_send;
        uint16_t _since_last_recv;
        uint16_t _last_block_id_read;
        uint16_t _expected_block_id;
        uint64_t _timestamp;
        uint16_t _timestamp_block_id;

        // Conflict resolution
        struct resolved_block
        {
            uint8_t loop_counter;
            std::set<uint8_t> packet_counters;
        };
        std::unordered_map<uint16_t, resolved_block> _already_resolved;
        uint8_t _loop_counter;

        // Configuration
        uint16_t _max_blocks_until_resync;
        uint16_t _max_blocks_until_disconnection;
    };


    inline uint16_t basic_protocol::last_block_id_read() const noexcept 
    { 
        return _last_block_id_read; 
    }

    inline uint16_t basic_protocol::expected_block_id() const noexcept
    { 
        return _expected_block_id; 
    }

    inline bool basic_protocol::is_expected(uint16_t id) const noexcept
    {
        return _expected_block_id == 0 || !cx::overflow::le(id, _expected_block_id); 
    }

    inline void basic_protocol::set_timestamp(uint64_t timestamp, uint16_t block_id) noexcept
    {
        _timestamp = timestamp;
        _timestamp_block_id = block_id;
    }

    inline uint64_t basic_protocol::block_timestamp(uint16_t block_id) noexcept
    {
        if (block_id >= _timestamp_block_id)
        {
            return _timestamp + (block_id - _timestamp_block_id) * superpacket_interval;
        }

        return _timestamp - (_timestamp_block_id - block_id) * superpacket_interval;
    }

    inline uint16_t basic_protocol::max_blocks_until_resync() const noexcept
    {
        return _max_blocks_until_resync;
    }

    inline void basic_protocol::max_blocks_until_resync(uint16_t value) noexcept
    {
        _max_blocks_until_resync = value;
    }

    inline uint16_t basic_protocol::max_blocks_until_disconnection() const noexcept
    {
        return _max_blocks_until_disconnection;
    }

    inline void basic_protocol::max_blocks_until_disconnection(uint16_t value) noexcept
    {
        _max_blocks_until_disconnection = value;
    }
}