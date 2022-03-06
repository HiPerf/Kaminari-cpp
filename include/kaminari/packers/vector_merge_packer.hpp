#pragma once

#include <kaminari/packers/packer.hpp>

#include <unordered_map>


namespace kaminari
{
    template <typename Detail>
    using vector_merge_packer_allocator_t = detail::pending_data<Detail>;

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator = std::allocator<detail::pending_data<Detail>>>
    class vector_merge_packer : public packer<vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>
    {
        friend class packer<vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;

    public:
        using packer_t = packer<vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;
        using pending_vector_t = typename packer_t::pending_vector_t;

    public:
        using packer<vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>::packer;

        template <typename T, typename... Args>
        void add(uint16_t _unused, T&& data, Args&&... args);
        void process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block);

    protected:
        inline void on_ack(const typename pending_vector_t::iterator& part);
        inline void clear();

    protected:
        std::unordered_map<Id, vector_merge_packer_allocator_t<Detail>*> _id_map;
    };


    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    template <typename T, typename... Args>
    void vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::add(uint16_t _unused, T&& data, Args&&... args)
    {
        // Opcode is ignored
        (void)_unused;

        // HACK(gpascualg): Wild assumption, no two different threads will try to write the same ID

        // Do we have this entity already?
        auto id = data.id;
        if (auto it = _id_map.find(id); it != _id_map.end())
        {
            auto pending = it->second;
            pending->data = data;
            pending->internal_tick_list.clear();
            pending->client_ack_ids.clear();
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
    inline void vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block)
    {
        // Do not do useless jobs
        if (packer_t::_pending.empty())
        {
            return;
        }

        // We can not naively pack everything into a single packet, as it might
        // go beyond its maximum size
        bool outgrows_superpacket = false;
        auto it = packer_t::_pending.begin();
        while (!outgrows_superpacket && it != packer_t::_pending.end())
        {
            // Create the global structure
            Global global;

            // TODO(gpascualg): MAGIC NUMBERS, 1 is vector size
            uint16_t size = packet_data_start + 1 + packer_t::new_tick_block_cost(tick_id, by_block);

            // Populate it as big as we can
            for (; it != packer_t::_pending.end(); ++it)
            {
                auto& pending = *it;
                if (!packer_t::is_pending(pending->internal_tick_list, tick_id, false))
                {
                    continue;
                }

                // If this one won't fit, neither will the rest
                auto next_size = size + Marshal::packet_size(pending->data);
                if (next_size > remaining)
                {
                    unfitting_data = true;
                    outgrows_superpacket = true;
                    break;
                }
                
                if (next_size > MAX_PACKET_SIZE)
                {
                    break;
                }

                size = next_size;
                global.data.push_back(pending->data);
                pending->internal_tick_list.push_back(tick_id);
                pending->client_ack_ids.push_back(block_id);
            }

            // Nothing to do here
            if (global.data.empty())
            {
                return;
            }

            buffers::packet::ptr packet = buffers::packet::make(opcode);
            Marshal::pack(packet, global);
            remaining -= size;

            if (auto it = by_block.find(tick_id); it != by_block.end())
            {
                it->second.push_back(packet);
            }
            else
            {
                by_block.emplace(tick_id, std::initializer_list<buffers::packet::ptr> { packet });
            }
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
    {
        // Erased acked entities
        for (auto it = part; it != packer_t::_pending.end(); ++it)
        {
            _id_map.erase((*it)->data.id);
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void vector_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::clear()
    {
        _id_map.clear();
    }
}
