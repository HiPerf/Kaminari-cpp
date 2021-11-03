#pragma once

#include <kaminari/packers/packer.hpp>


namespace kaminari
{
    template <typename Detail>
    using merge_packer_allocator_t = detail::pending_data<Detail>;

    template <typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator = std::allocator<detail::pending_data<Detail>>>
    class merge_packer : public packer<merge_packer<Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>
    {
        friend class packer<merge_packer<Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;

    public:
        using packer_t = packer<merge_packer<Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;
        using pending_vector_t = typename packer_t::pending_vector_t;

    public:
        using packer<merge_packer<Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>::packer;
        
        template <typename... Args>
        void add(uint16_t _unused, Detail&& data, Args&&... args);
        void process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block);

    protected:
        inline void on_ack(const typename pending_vector_t::iterator& part);
        inline void clear();
    };


    template <typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    template <typename... Args>
    void merge_packer<Global, Detail, opcode, Marshal, Allocator>::add(uint16_t _unused, Detail&& data, Args&&... args)
    {
        // Opcode is ignored
        (void)_unused;

        // In merge mode we add the detailed structure
        packer_t::get_pending_data(std::forward<Detail>(data));
    }

    template <typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void merge_packer<Global, Detail, opcode, Marshal, Allocator>::process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block)
    {
        // Do not do useless jobs
        if (packer_t::_pending.empty())
        {
            return;
        }
        
        // Create the global structure
        Global global;

        // TODO(gpascualg): MAGIC NUMBERS, 2 is vector size
        uint16_t size = packet_data_start + 2 + packer_t::new_tick_block_cost(tick_id, by_block);

        // Populate it as big as we can
        for (auto& pending : packer_t::_pending)
        {
            if (!packer_t::is_pending(pending->internal_tick_list, tick_id, false))
            {
                continue;
            }

            // If this one won't fit, neither will the rest
            auto next_size = size + Marshal::packet_size(pending->data);
            if (next_size > remaining)
            {
                unfitting_data = true;
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

    template <typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void merge_packer<Global, Detail, opcode, Marshal, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
    {
        // Nothing to do here
        (void)part;
    }

    template <typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void merge_packer<Global, Detail, opcode, Marshal, Allocator>::clear()
    {}
}
