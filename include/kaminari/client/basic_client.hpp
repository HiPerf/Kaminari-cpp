#pragma once

#include <inttypes.h>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/intrusive_ptr.hpp>


namespace kaminari
{
    class super_packet_reader;

    class basic_client
    {
    public:
        enum class status
        {
            CONNECTED,
            DESYNCED,
            PENDING_DISCONNECTION,
            DISCONNECTING
        };

    public:
        basic_client() noexcept;
        basic_client(basic_client&& other) noexcept;
        basic_client& operator=(basic_client&& other) noexcept;

        void reset() noexcept;
        
        inline status connexion_status() const noexcept;
        inline bool pending_disconnection() const noexcept;
        inline void flag_disconnection() noexcept;
        inline void flag_disconnecting() noexcept;
        inline void flag_desync() noexcept;

        uint16_t first_super_packet_tick_id() const noexcept;
        bool has_pending_super_packets() const noexcept;
        super_packet_reader first_super_packet() noexcept;

        inline uint16_t lag() const noexcept;
        inline void lag(uint16_t value) noexcept;

    protected:
        status _status;
        boost::circular_buffer<super_packet_reader> _pending_super_packets;
        uint16_t _lag;
    };


    inline basic_client::status basic_client::connexion_status() const noexcept
    {
        return _status;
    }

    inline bool basic_client::pending_disconnection() const noexcept
    {
        return _status == status::PENDING_DISCONNECTION ||
            _status == status::DISCONNECTING;
    }

    inline void basic_client::flag_disconnection() noexcept
    {
        _status = status::PENDING_DISCONNECTION;
    }

    inline void basic_client::flag_disconnecting() noexcept
    {
        _status = status::DISCONNECTING;
    }

    inline void basic_client::flag_desync() noexcept
    {
        _status = status::DESYNCED;
    }

    inline uint16_t basic_client::lag() const noexcept
    {
        return _lag;
    }

    inline void basic_client::lag(uint16_t value) noexcept
    {
        _lag = value;
    }
}
