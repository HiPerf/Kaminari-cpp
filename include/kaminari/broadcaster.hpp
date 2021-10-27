#pragma once


namespace kaminari
{
    template <class Derived>
    class broadcaster
    {
    public:
        template <typename C>
        inline void broadcast(C&& callback)
        {
            static_cast<Derived&>(*this).broadcast(std::move(callback));
        }

        template <typename C>
        inline void broadcast_single(C&& callback)
        {
            static_cast<Derived&>(*this).broadcast_single(std::move(callback));
        }

        template <typename C>
        inline void broadcast_with_callback(C&& callback)
        {
            static_cast<Derived&>(*this).broadcast_with_callback(std::move(callback));
        }

        template <typename C>
        inline void broadcast_single_with_callback(C&& callback)
        {
            static_cast<Derived&>(*this).broadcast_single_with_callback(std::move(callback));
        }
    };
}
