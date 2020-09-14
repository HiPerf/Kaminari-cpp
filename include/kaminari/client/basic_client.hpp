#pragma once

#include <kaminari/types/data_wrapper.hpp>

#include <inttypes.h>
#include <vector>

#include <boost/circular_buffer.hpp>
#include <boost/intrusive_ptr.hpp>


namespace kaminari
{
    class basic_client
    {
    public:
        basic_client() noexcept;
        
        inline bool has_pending_super_packets() const noexcept;
        inline boost::intrusive_ptr<data_wrapper> first_super_packet() noexcept;

        inline uint16_t lag() const noexcept;
        inline void lag(uint16_t value) noexcept;

    protected:
        boost::circular_buffer<boost::intrusive_ptr<data_wrapper>> _pending_super_packets;
        uint16_t _lag;
    };

    inline bool basic_client::has_pending_super_packets() const noexcept
    {
        return !_pending_super_packets.empty();
    }

    inline boost::intrusive_ptr<data_wrapper> basic_client::first_super_packet() noexcept
    {
        auto data = _pending_super_packets.front();
        _pending_super_packets.pop_front();
        return data;
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
