#pragma once

#include <kaminari/packers/packer.hpp>


namespace kaminari
{
    using immediate_packer_allocator_t = detail::pending_data<buffers::packet::ptr>;

    template <class Marshal, class Allocator = std::allocator<detail::pending_data<buffers::packet::ptr>>>
    class immediate_packer : public packer<immediate_packer<Marshal, Allocator>, buffers::packet::ptr, Allocator>
    {
        friend class packer<immediate_packer<Marshal, Allocator>, buffers::packet::ptr, Allocator>;

    public:
        using packer_t = packer<immediate_packer<Marshal, Allocator>, buffers::packet::ptr, Allocator>;

    public:
        using packer<immediate_packer<Marshal, Allocator>, buffers::packet::ptr, Allocator>::packer;

        template <typename T, typename... Args>
        void add(uint16_t opcode, T&& data, Args&&... args);
        void add(const buffers::packet::ptr& packet);
        void process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block);

    protected:
        inline void on_ack(const typename packer_t::pending_vector_t::iterator& part);
        inline void clear();
    };


    template <class Marshal, class Allocator>
    template <typename T, typename... Args>
    void immediate_packer<Marshal, Allocator>::add(uint16_t opcode, T&& data, Args&&... args)
    {
        // Immediate mode means that the structure is packed right now
        buffers::packet::ptr packet = buffers::packet::make(opcode, std::forward<Args>(args)...);
        Marshal::pack(packet, data);

        // Add to pending
        add(packet);
    }

    template <class Marshal, class Allocator>
    void immediate_packer<Marshal, Allocator>::add(const buffers::packet::ptr& packet)
    {
        // Add to pending
        packer_t::get_pending_data(packet);
    }

    template <class Marshal, class Allocator>
    void immediate_packer<Marshal, Allocator>::process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block)
    {
        for (auto& pending : packer_t::_pending)
        {
            if (!packer_t::is_pending(pending->internal_tick_list, tick_id, false))
            {
                continue;
            }

            uint16_t actual_block = packer_t::get_actual_tick_id(pending->internal_tick_list, tick_id);
            uint16_t size = pending->data->size();
            if (auto it = by_block.find(actual_block); it != by_block.end())
            {
                // TODO(gpascualg): Do we want a hard-break here? packets in the vector should probably be
                //  ordered by size? But we could starve big packets that way, it requires some "agitation"
                //  factor for packets being ignored too much time
                if (size > remaining)
                {
                    unfitting_data = true;
                    break;
                }

                it->second.push_back(pending->data);
            }
            else
            {
                // Block header size
                size += super_packet_block_size;

                // TODO(gpascualg): Same as above, do we want to hard-break?
                if (size > remaining)
                {
                    unfitting_data = true;
                    break;
                }

                by_block.emplace(actual_block, std::initializer_list<buffers::packet::ptr> { pending->data });
            }

            pending->internal_tick_list.push_back(tick_id);
            pending->client_ack_ids.push_back(block_id);
            remaining -= size;
        }
    }

    template <class Marshal, class Allocator>
    inline void immediate_packer<Marshal, Allocator>::on_ack(const typename packer_t::pending_vector_t::iterator& part)
    {
        // Nothing to do here
        (void)part;
    }

    template <class Marshal, class Allocator>
    inline void immediate_packer<Marshal, Allocator>::clear()
    {}
}
