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

    // TODO(gpascualg): Make this configurable via protocol
    inline constexpr uint16_t WorldHeartBeat = 50;

    class basic_protocol
    {
    public:
        basic_protocol();

        void reset();
        bool resolve(packet_reader* packet, uint16_t block_id);

        inline uint16_t last_block_id_read() const;
        inline uint16_t expected_block_id() const;
        inline bool is_expected(uint16_t id) const;

        inline void set_timestamp(uint64_t timestamp, uint16_t block_id);
        inline uint64_t block_timestamp(uint16_t block_id);

    protected:
        BufferMode _buffer_mode;
        uint16_t _since_last_send;
        uint16_t _since_last_recv;
        uint16_t _last_block_id_read;
        uint16_t _expected_block_id;
        uint64_t _timestamp;
        uint16_t _timestamp_block_id;
        std::unordered_map<uint16_t, std::set<uint8_t>> _already_resolved;
    };


    inline uint16_t basic_protocol::last_block_id_read() const 
    { 
        return _last_block_id_read; 
    }

    inline uint16_t basic_protocol::expected_block_id() const
    { 
        return _expected_block_id; 
    }

    inline bool basic_protocol::is_expected(uint16_t id) const
    {
        return _expected_block_id == 0 || !cx::overflow::le(id, _expected_block_id); 
    }

    inline void basic_protocol::set_timestamp(uint64_t timestamp, uint16_t block_id)
    {
        _timestamp = timestamp;
        _timestamp_block_id = block_id;
    }

    inline uint64_t basic_protocol::block_timestamp(uint16_t block_id)
    {
        if (block_id >= _timestamp_block_id)
        {
            return _timestamp + (block_id - _timestamp_block_id) * WorldHeartBeat;
        }

        return _timestamp - (_timestamp_block_id - block_id) * WorldHeartBeat;
    }
}