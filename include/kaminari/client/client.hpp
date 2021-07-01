#pragma once

#include <kaminari/super_packet.hpp>
#include <kaminari/protocol/protocol.hpp>
#include <kaminari/client/basic_client.hpp>


namespace kaminari
{
    template <typename Queues>
    class client : public basic_client
    {
    public:
        template <typename... Args>
        client(uint8_t resend_threshold, Args&&... allocator_args);

        inline void reset() noexcept;

        template <typename TimeBase>
        inline void received_packet(const boost::intrusive_ptr<data_wrapper>& data);

        inline kaminari::super_packet<Queues>* super_packet();

    protected:
        kaminari::super_packet<Queues> _super_packet;
        kaminari::protocol _protocol;
    };
    

    template <typename Queues>
    template <typename... Args>
    client<Queues>::client(uint8_t resend_threshold, Args&&... allocator_args) :
        basic_client(),
        _super_packet(resend_threshold, std::forward<Args>(allocator_args)...)
    {}

    template <typename Queues>
    inline void client<Queues>::reset() noexcept
    {
        basic_client::reset();
        _super_packet.reset();
        _protocol.reset();
    }

    template <typename Queues>
    template <typename TimeBase>
    inline void client<Queues>::received_packet(const boost::intrusive_ptr<data_wrapper> & data)
    {
        super_packet_reader reader(data);

        // Make sure to ignore old packets
        if (_protocol.is_out_of_order(reader.id()))
        {
            return;
        }

        // Handle all acks now
        _protocol.template handle_acks<TimeBase>(reader, this, super_packet());

        // TODO(gpascualg): Ideally, we want to start searching from rbegin(), but then we can't insert
        auto it = _pending_super_packets.begin();
        while(it != _pending_super_packets.end())
        {
            if (it->id() > reader.id())
            {
                break;
            }

            ++it;
        }

        _pending_super_packets.insert(it, reader);
    }

    template <typename Queues>
    inline kaminari::super_packet<Queues>* client<Queues>::super_packet()
    {
        return &_super_packet;
    }
}
