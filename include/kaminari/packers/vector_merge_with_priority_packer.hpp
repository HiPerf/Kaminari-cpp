#pragma once

#include <kaminari/packers/packer.hpp>

#include <algorithm>
#include <unordered_map>


namespace kaminari
{
    template <typename Detail>
    using vector_merge_with_priority_packer_allocator_t = detail::pending_data<Detail>;

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator = std::allocator<detail::pending_data<Detail>>>
    class vector_merge_with_priority_packer : public packer<vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>
    {
        friend class packer<vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;

    public:
        using packer_t = packer<vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;
        using pending_vector_t = typename packer_t::pending_vector_t;

    public:
        using packer<vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>::packer;

        template <typename T, typename... Args>
        void add(uint16_t _unused, T&& data, Args&&... args);
        void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

    protected:
        inline void on_ack(const typename pending_vector_t::iterator& part);
        inline void clear();

    protected:
        std::unordered_map<Id, vector_merge_with_priority_packer_allocator_t<Detail>*> _id_map;
    };


    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    template <typename T, typename... Args>
    void vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>::add(uint16_t _unused, T&& data, Args&&... args)
    {
        // Opcode is ignored
        (void)_unused;
        assert(data.priority_multiplier > 1e-6f && "Multiplier must be higher than one");

        // HACK(gpascualg): Wild assumption, no two different threads will try to write the same ID

        // Do we have this entity already?
        auto id = data.id;
        if (auto it = _id_map.find(id); it != _id_map.end())
        {
            auto pending = it->second;
            auto old_prio = pending->data.priority;
            pending->data = data;
            pending->data.priority = (pending->data.priority + old_prio) / 2.0f / pending->data.priority_multiplier;
            pending->blocks.clear();
        }
        else
        {
            // TODO(gpascualg): Code duplication w.r.t. packer.hpp
            auto index = packer_t::_index++;
            while (index != packer_t::_pending.size())
            {
#if defined(KAMINARY_YIELD_IN_BUSY_LOOP)
                std::this_thread::yield();
#endif
            }

            auto pending = packer_t::_allocator.allocate(1);
            std::allocator_traits<Allocator>::construct(packer_t::_allocator, pending, std::forward<T>(data));
            // Emplace in map before doing so in pending, so that we don't increase size before it's ready
            _id_map.emplace(id, pending);
            packer_t::_pending.emplace_back(pending);
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
    {
        // Do not do useless jobs
        if (packer_t::_pending.empty())
        {
            return;
        }

        // TODO(gpacualg): We might want to insert ordered instead, so we don't have to sort, but we can't right now
        //  Inserting ordered would break multithreading
        std::make_heap(packer_t::_pending.begin(), packer_t::_pending.end(), [](const auto& lhs, const auto& rhs) {
                // Invert, as we want from lowest to greatest
                return lhs->data.priority > rhs->data.priority;
            });

        // We can not naively pack everything into a single packet, as it might
        // go beyond its maximum size
        bool outgrows_superpacket = false;
        auto it = packer_t::_pending.begin();
        while (!outgrows_superpacket && it != packer_t::_pending.end())
        {
            // Create the global structure
            Global global;

            // TODO(gpascualg): MAGIC NUMBERS, 2 is vector size
            uint16_t size = buffers::packet::DataStart + 2 + packer_t::new_block_cost(block_id, by_block);

            // Populate it as big as we can
            for (; it != packer_t::_pending.end(); ++it)
            {
                auto& pending = *it;
                if (!packer_t::is_pending(pending->blocks, block_id, false))
                {
                    continue;
                }

                // If this one won't fit, neither will the rest
                auto next_size = size + Marshal::packet_size(pending->data);
                if (next_size > remaining)
                {
                    outgrows_superpacket = true;
                    break;
                }
                
                if (next_size > MAX_PACKET_SIZE)
                {
                    break;
                }

                size = next_size;
                global.data.push_back(pending->data);
                pending->blocks.push_back(block_id);
            }

            // Nothing to do here
            if (global.data.empty())
            {
                return;
            }

            buffers::packet::ptr packet = buffers::packet::make(opcode);
            Marshal::pack(packet, global);
            remaining -= size;

            if (auto it = by_block.find(block_id); it != by_block.end())
            {
                it->second.push_back(packet);
            }
            else
            {
                by_block.emplace(block_id, std::initializer_list<buffers::packet::ptr> { packet });
            }
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
    {
        // Erased acked entities
        for (auto it = part; it != packer_t::_pending.end(); ++it)
        {
            _id_map.erase((*it)->data.id);
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void vector_merge_with_priority_packer<Id, Global, Detail, opcode, Marshal, Allocator>::clear()
    {
        _id_map.clear();
    }
}
