#pragma once

#include <kaminari/detail/detail.hpp>

#include <inttypes.h>


namespace kaminari
{
    // Basically this queue does nothing, whatever the packer does
    // it just relays

    template <typename Packer, uint8_t max_retries=0>
    class unreliable_queue : public Packer
    {
    public:
        using Packer::Packer;

        inline void process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block)
        {
            // Process first
            Packer::process(tick_id, block_id, remaining, unfitting_data, by_block);

            // There are two cases, zero retries or more
            if constexpr (max_retries == 0)
            {
                // Make sure we dont have any pending packet
                for (auto pending : Packer::_pending)
                {
                    std::destroy_at(pending);
                    Packer::_allocator.deallocate(pending, 1);
                }

                Packer::_pending.clear();
            }
            else
            {
                // Reorder vec so that packets that still have retries are first
                auto end = Packer::_pending.end();
                auto part = std::partition(Packer::_pending.begin(), end, [](auto& x) { return x->internal_tick_list.size() <= max_retries; });

                // Free those that have reached max retries ar freed
                for (auto it = part; it != end; ++it)
                {
                    auto pending = *it;
                    std::destroy_at(pending);
                    Packer::_allocator.deallocate(pending, 1);
                }

                Packer::_pending.resize(std::distance(Packer::_pending.begin(), part));
            }

            Packer::_index = Packer::_pending.size();
        }

        inline void ack(uint16_t block_id)
        {
            // There are two cases, zero retries or more
            if constexpr (max_retries == 0)
            {
                // Nothing to do here, on purpouse
                (void)block_id;
            }
            else
            {
                // Call base and ack some packets
                Packer::ack(block_id);
            }
        }
    };
}
