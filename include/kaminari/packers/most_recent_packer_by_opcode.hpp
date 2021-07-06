#pragma once

#include <kaminari/packers/packer.hpp>

#include <unordered_map>


namespace kaminari
{
    struct packet_by_opcode
    {
        packet_by_opcode(const buffers::packet::ptr& p, uint16_t o) :
            packet(p),
            opcode(o)
        {}

        buffers::packet::ptr packet;
        uint16_t opcode;
    };

    using most_recent_packer_by_opcode_allocator_t = detail::pending_data<packet_by_opcode>;

    template <class Marshal, class Allocator = std::allocator<detail::pending_data<packet_by_opcode>>>
    class most_recent_packer_by_opcode : public packer<most_recent_packer_by_opcode<Marshal, Allocator>, packet_by_opcode, Allocator>
    {
        friend class packer<most_recent_packer_by_opcode<Marshal, Allocator>, packet_by_opcode, Allocator>;

    public:
        using packer_t = packer<most_recent_packer_by_opcode<Marshal, Allocator>, packet_by_opcode, Allocator>;

    public:
        using packer<most_recent_packer_by_opcode<Marshal, Allocator>, packet_by_opcode, Allocator>::packer;
        
        template <typename T, typename... Args>
        void add(uint16_t opcode, T&& data, Args&&... args);
        void process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block);

    private:
        void add(const buffers::packet::ptr& packet, uint16_t opcode);

    protected:
        inline void on_ack(const typename packer_t::pending_vector_t::iterator& part);
        inline void clear();

    protected:
        std::unordered_map<uint16_t, typename detail::pending_data<packet_by_opcode>*> _opcode_map;
    };


    template <class Marshal, class Allocator>
    template <typename T, typename... Args>
    void most_recent_packer_by_opcode<Marshal, Allocator>::add(uint16_t opcode, T&& data, Args&&... args)
    {
        // Immediate mode means that the structure is packed right now
        buffers::packet::ptr packet = buffers::packet::make(opcode, std::forward<Args>(args)...);
        Marshal::pack(packet, data);

        // Add to pending
        add(packet, opcode);
    }

    template <class Marshal, class Allocator>
    void most_recent_packer_by_opcode<Marshal, Allocator>::add(const buffers::packet::ptr& packet, uint16_t opcode)
    {
        // Add to pending
        if (auto it = _opcode_map.find(opcode); it != _opcode_map.end())
        {
            auto pending = it->second;
            pending->data.packet = packet;
            pending->blocks.clear();
        }
        else
        {
            // Add to pending
            auto pending = packer_t::_allocator.allocate(1);
            std::allocator_traits<Allocator>::construct(packer_t::_allocator, pending, packet_by_opcode { packet, opcode });
            packer_t::_pending.push_back(pending);
            _opcode_map.emplace(opcode, pending);
        }
    }

    template <class Marshal, class Allocator>
    void most_recent_packer_by_opcode<Marshal, Allocator>::process(uint16_t block_id, uint16_t& remaining, detail::packets_by_block& by_block)
    {
        for (auto& pending : packer_t::_pending)
        {
            if (!packer_t::is_pending(pending->blocks, block_id, false))
            {
                continue;
            }

            uint16_t actual_block = packer_t::get_actual_block(pending->blocks, block_id);
            uint16_t size = pending->data.packet->size();
            if (auto it = by_block.find(actual_block); it != by_block.end())
            {
                // TODO(gpascualg): Do we want a hard-break here? packets in the vector should probably be
                //  ordered by size? But we could starve big packets that way, it requires some "agitation"
                //  factor for packets being ignored too much time
                if (size > remaining)
                {
                    break;
                }

                it->second.push_back(pending->data.packet);
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

                by_block.emplace(actual_block, std::initializer_list<buffers::packet::ptr> { pending->data.packet });
            }

            pending->blocks.push_back(block_id);
            remaining -= size;
        }
    }

    template <class Marshal, class Allocator>
    inline void most_recent_packer_by_opcode<Marshal, Allocator>::on_ack(const typename packer_t::pending_vector_t::iterator& part)
    {
        // Erased acked entities
        for (auto it = part; it != packer_t::_pending.end(); ++it)
        {
            _opcode_map.erase((*it)->data.opcode);
        }
    }

    template <class Marshal, class Allocator>
    inline void most_recent_packer_by_opcode<Marshal, Allocator>::clear()
    {
        _opcode_map.clear();
    }
}
