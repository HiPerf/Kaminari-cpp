#pragma once

#include <kaminari/packers/packer.hpp>


namespace kaminari
{
    using ordered_packer_allocator_t = detail::pending_data<packet::ptr>;

    template <class Marshal, class Allocator = std::allocator<detail::pending_data<packet::ptr>>>
    class ordered_packer : public packer<ordered_packer<Marshal, Allocator>, packet::ptr, Allocator>
    {
        friend class packer<ordered_packer<Marshal, Allocator>, packet::ptr, Allocator>;

    public:
        using packer_t = packer<ordered_packer<Marshal, Allocator>, packet::ptr>;

    public:
        using packer<ordered_packer<Marshal, Allocator>, packet::ptr, Allocator>::packer;

        template <typename T, typename... Args>
        void add(uint16_t opcode, T&& data, Args&&... args);
        void add(const packet::ptr& packet);
        void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

    protected:
        bool is_pending(uint16_t block_id);
        inline void on_ack(const typename packer_t::pending_vector_t::iterator& part);
        inline void clear();

    protected:
        uint16_t _last_block;
        bool _has_new_packet;
    };


    template <class Marshal, class Allocator>
    template <typename T, typename... Args>
    void ordered_packer<Marshal, Allocator>::add(uint16_t opcode, T&& data, Args&&... args)
    {
        // Immediate mode means that the structure is packed right now
        packet::ptr packet = packet::make(opcode, std::forward<Args>(args)...);
        Marshal::pack(packet, data);

        // Add to pending
        add(packet);
    }

    template <class Marshal, class Allocator>
    void ordered_packer<Marshal, Allocator>::add(const packet::ptr& packet)
    {
        // Add to pending
        auto pending = packer_t::_allocator.construct(packer_t::_allocator.allocate(1), packet);
        packer_t::_pending.push_back(pending);
        _has_new_packet = true;
    }

    template <class Marshal, class Allocator>
    void ordered_packer<Marshal, Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
    {
        // Pending is global in case of ordered packets
        if (!is_pending(block_id))
        {
            return;
        }

        // Insert from older to newer, all of them
        auto num_inserted = 0;
        for (auto it = packer_t::_pending.rbegin(); it != packer_t::_pending.rend(); ++it, ++num_inserted)
        {
            auto& pending = *it;
            uint16_t actual_block = get_actual_block(pending->blocks, block_id);
            uint16_t size = pending->data->size();
            if (auto it = by_block.find(actual_block); it != by_block.end())
            {
                // TODO(gpascualg): Do we want a hard-break here? packets in the vector should probably be
                //  ordered by size? But we could starve big packets that way, it requires some "agitation"
                //  factor for packets being ignored too much time
                if (size > remaining)
                {
                    break;
                }

                it->second.push_back(pending->data);
            }
            else
            {
                // TODO(gpascualg): Magic numbers, 4 is block header + block size
                // TODO(gpascualg): This can be brought down to 3, block header + packet count
                size += 4;

                // TODO(gpascualg): Same as above, do we want to hard-break?
                if (size > remaining)
                {
                    break;
                }

                by_block.emplace(actual_block, std::initializer_list<packet::ptr> { pending->data });
            }

            pending->blocks.push_back(block_id);
            remaining -= size;
        }

        // Only if something was pushed will we actually stop pushing in further iterations
        if (num_inserted > 0)
        {
            _has_new_packet = false;
            _last_block = block_id;
        }
    }

    template <class Marshal, class Allocator>
    bool ordered_packer<Marshal, Allocator>::is_pending(uint16_t block_id)
    {
        if (packer_t::_pending.empty())
        {
            return false;
        }

        // Pending inclusions are those forced, not yet included in any block or
        // which have expired without an ack
        return _has_new_packet ||
            cx::overflow::sub(block_id, _last_block) >= packer_t::resend_threshold; // We do want 0s here
    }

    template <class Marshal, class Allocator>
    void ordered_packer<Marshal, Allocator>::on_ack(const typename packer_t::pending_vector_t::iterator& part)
    {
        // TODO: A lot to do here
        (void)part;
    }

    template <class Marshal, class Allocator>
    inline void ordered_packer<Marshal, Allocator>::clear()
    {}
}
