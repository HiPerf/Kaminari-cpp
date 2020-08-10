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
    inline kaminari::super_packet<Queues>* client<Queues>::super_packet()
    {
        return &_super_packet;
    }
}
