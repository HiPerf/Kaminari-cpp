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
        basic_client();
        
        inline void received_packet(const boost::intrusive_ptr<data_wrapper>& data);
        inline bool has_pending_super_packets() const;
        inline boost::intrusive_ptr<data_wrapper> first_super_packet();

    private:
        boost::circular_buffer<boost::intrusive_ptr<data_wrapper>> _pending_super_packets;
    };

    inline void basic_client::received_packet(const boost::intrusive_ptr<data_wrapper>& data)
    {
        _pending_super_packets.push_back(data);
    }

    inline bool basic_client::has_pending_super_packets() const
    {
        return !_pending_super_packets.empty();
    }

    inline boost::intrusive_ptr<data_wrapper> basic_client::first_super_packet()
    {
        auto data = _pending_super_packets.front();
        _pending_super_packets.pop_front();
        return data;
    }
}
