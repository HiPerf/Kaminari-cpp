#pragma once


namespace kaminari
{
    template <class Derived>
    class broadcaster
    {
    public:
        template <typename C>
        void broadcast(C&& callback)
        {}

        template <typename C>
        void broadcast_single(C&& callback)
        {}
    };
}
