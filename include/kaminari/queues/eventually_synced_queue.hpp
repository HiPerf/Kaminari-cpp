#pragma once

namespace kaminari
{
    // Basically this queue does nothing, whatever the packer does
    // it just relays

    template <typename Packer>
    class eventually_synced_queue : public Packer
    {
    public:
        using Packer::Packer;
    };
}
