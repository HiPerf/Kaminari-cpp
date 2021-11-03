#pragma once

#include <kaminari/packers/packer.hpp>

#include <unordered_map>


namespace kaminari
{
    template <typename Detail>
    using unique_merge_packer_allocator_t = detail::pending_data<Detail>;

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator = std::allocator<detail::pending_data<Detail>>>
    class unique_merge_packer : public packer<unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>
    {
        friend class packer<unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;

    public:
        using packer_t = packer<unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>;
        using pending_vector_t = typename packer_t::pending_vector_t;

    public:
        using packer<unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>, Detail, Allocator>::packer;

        template <typename T, typename... Args>
        void add(uint16_t _unused, T&& data, Args&&... args);
        void process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block);

    protected:
        inline void on_ack(const typename pending_vector_t::iterator& part);
        inline void clear();

    protected:
        unique_merge_packer_allocator_t<Detail>* _unique;
    };


    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    template <typename T, typename... Args>
    void unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::add(uint16_t _unused, T&& data, Args&&... args)
    {
        // Opcode is ignored
        (void)_unused;

        // HACK(gpascualg): unique_merge_packer is broken in multithreaded environments without locks

        // Do we have a pending packet already?
        if (_unique)
        {
            _unique->data = data;
            _unique->internal_tick_list.clear();
            _unique->client_ack_ids.clear();
        }
        else
        {
            _unique = packer_t::_allocator.allocate(1);
            std::allocator_traits<Allocator>::construct(packer_t::_allocator, _unique, std::forward<T>(data));
            packer_t::_pending.push_back(_unique);
        }
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::process(uint16_t tick_id, uint16_t block_id, uint16_t& remaining, bool& unfitting_data, detail::packets_by_block& by_block)
    {
        // Do not do useless jobs
        if (_unique == nullptr)
        {
            return;
        }

        // TODO(gpascualg): MAGIC NUMBERS, 2 is vector size
        uint16_t size = packet_data_start + packer_t::new_tick_block_cost(tick_id, by_block) + Marshal::packet_size(_unique->data);
        if (size > remaining)
        {
            unfitting_data = true;
            return;
        }

        buffers::packet::ptr packet = buffers::packet::make(opcode);
        Marshal::pack(packet, _unique->data);
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

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::on_ack(const typename pending_vector_t::iterator& part)
    {
        // TODO(gpascualg): Is this correct?
        _unique = nullptr;
    }

    template <typename Id, typename Global, typename Detail, uint16_t opcode, class Marshal, class Allocator>
    inline void unique_merge_packer<Id, Global, Detail, opcode, Marshal, Allocator>::clear()
    {
        _unique = nullptr;
    }
}
