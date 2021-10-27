#pragma once

#include <kaminari/super_packet.hpp>
#include <kaminari/protocol/protocol.hpp>
#include <kaminari/client/basic_client.hpp>

#include <type_traits>
#include <concepts>


namespace kaminari
{
    template <typename T>
    concept has_receive_callback = requires(T t)
    {
        { t.on_receive_packet((void*)nullptr, std::declval<uint16_t>()) };
    };

    template <typename T>
    concept stateful_callback = std::is_constructible_v<T> && std::is_move_constructible_v<T> &&
        has_receive_callback<T>;

    template <typename Queues, stateful_callback... StatefulCallbacks>
    class client : public basic_client, public StatefulCallbacks...
    {
    public:
        template <typename... Args>
        client(uint8_t resend_threshold, Args&&... allocator_args);

        inline void reset() noexcept;

        template <typename TimeBase>
        inline void received_packet(const boost::intrusive_ptr<data_wrapper>& data);

        inline kaminari::super_packet<Queues>* super_packet();
        inline kaminari::protocol* protocol();

        template <typename T>
        static constexpr bool has_stateful_callback();

    protected:
        kaminari::super_packet<Queues> _super_packet;
        kaminari::protocol _protocol;
    };


    template <typename Queues, stateful_callback... StatefulCallbacks>
    template <typename... Args>
    client<Queues, StatefulCallbacks...>::client(uint8_t resend_threshold, Args&&... allocator_args) :
        basic_client(),
        StatefulCallbacks()...,
        _super_packet(resend_threshold, std::forward<Args>(allocator_args)...)
    {}

    template <typename Queues, stateful_callback... StatefulCallbacks>
    inline void client<Queues, StatefulCallbacks...>::reset() noexcept
    {
        basic_client::reset();
        _super_packet.reset();
        _protocol.reset();
    }

    template <typename Queues, stateful_callback... StatefulCallbacks>
    template <typename TimeBase>
    inline void client<Queues, StatefulCallbacks...>::received_packet(const boost::intrusive_ptr<data_wrapper>& data)
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
        while (it != _pending_super_packets.end())
        {
            if (cx::overflow::ge(it->id(), reader.id()))
            {
                break;
            }

            ++it;
        }

        _pending_super_packets.insert(it, reader);

        // Call all callbacks, if any
        (..., StatefulCallbacks::on_receive_packet(this, _pending_super_packets.back().id()));
    }

    template <typename Queues, stateful_callback... StatefulCallbacks>
    inline kaminari::super_packet<Queues>* client<Queues, StatefulCallbacks...>::super_packet()
    {
        return &_super_packet;
    }

    template <typename Queues, stateful_callback... StatefulCallbacks>
    inline kaminari::protocol* client<Queues, StatefulCallbacks...>::protocol()
    {
        return &_protocol;
    }

    template <typename Queues, stateful_callback... StatefulCallbacks>
    template <typename T>
    constexpr bool client<Queues, StatefulCallbacks...>::has_stateful_callback()
    {
        return (std::same_as<T, StatefulCallbacks> || ...);
    }
}
