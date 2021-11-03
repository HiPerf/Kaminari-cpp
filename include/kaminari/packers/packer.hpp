#pragma once

#include <kaminari/detail/detail.hpp>
#include <kaminari/cx/overflow.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/super_packet.hpp>

#include <inttypes.h>
#include <memory>
#include <vector>


namespace kaminari
{
    namespace detail
    {
        template <typename Pending>
        struct pending_data
        {
            pending_data(const Pending& d);

            Pending data;
            std::vector<uint16_t> internal_tick_list;
            std::vector<uint16_t> client_ack_ids;
        };
    }
        
    template <typename Derived, typename Pending, class Allocator = std::allocator<detail::pending_data<Pending>>>
    class packer
    {
    public:
        using pending_vector_t = std::vector<detail::pending_data<Pending>*>;
        using pending_t = Pending;

    public:
        packer(uint8_t resend_threshold, const Allocator& alloc = Allocator());
        packer(packer<Derived, Pending, Allocator>&& other) noexcept;
        packer<Derived, Pending, Allocator>& operator=(packer<Derived, Pending, Allocator>&& other) noexcept;

        inline void ack(uint16_t block_id);
        inline void clear();

        inline void reset();

        bool is_pending(const std::vector<uint16_t>& internal_ticks, uint16_t tick_id, bool force);
        inline uint16_t get_actual_tick_id(const std::vector<uint16_t>& internal_ticks, uint16_t tick_id);
        inline uint16_t new_tick_block_cost(uint16_t tick_id, detail::packets_by_block& by_block);

    protected:
        // TODO(gpascualg): Revisit packer busy loop
        detail::pending_data<Pending>* get_pending_data(const Pending& d)
        {
            auto index = _index++;
            while (index != _pending.size())
            {
#if defined(KAMINARY_YIELD_IN_BUSY_LOOP)
                std::this_thread::yield();
#endif
            }

            auto pending = _allocator.allocate(1);
            std::allocator_traits<Allocator>::construct(_allocator, pending, d);
            return _pending.emplace_back(pending);
        }

    protected:
        uint8_t _resend_threshold;
        pending_vector_t _pending;
        std::atomic<uint16_t> _index;
        Allocator _allocator;
    };


    template <typename Pending>
    detail::pending_data<Pending>::pending_data(const Pending& d) :
        data(d),
        internal_tick_list(),
        client_ack_ids()
    {}

    template <typename Derived, typename Pending, class Allocator>
    packer<Derived, Pending, Allocator>::packer(uint8_t resend_threshold, const Allocator& alloc) :
        _resend_threshold(resend_threshold),
        _pending(),
        _index(0),
        _allocator(alloc)
    {
        assert(_resend_threshold > 0 && "Use a threshold of 1 instead, which effectively does the same");
    }

    template <typename Derived, typename Pending, class Allocator>
    packer<Derived, Pending, Allocator>::packer(packer<Derived, Pending, Allocator>&& other) noexcept :
        _resend_threshold(std::move(other._resend_threshold)),
        _pending(std::move(other._pending)),
        _index(static_cast<uint16_t>(other._index)),
        _allocator(std::move(other._allocator))
    {}

    template <typename Derived, typename Pending, class Allocator>
    packer<Derived, Pending, Allocator>& packer<Derived, Pending, Allocator>::operator=(packer<Derived, Pending, Allocator>&& other) noexcept
    {
        _resend_threshold = std::move(other._resend_threshold);
        _pending = std::move(other._pending);
        _index = static_cast<uint16_t>(other._index);
        _allocator = std::move(other._allocator);

        return *this;
    }

    template <typename Derived, typename Pending, class Allocator>
    inline void packer<Derived, Pending, Allocator>::ack(uint16_t block_id)
    {
        // Partition data
        auto part = std::partition(_pending.begin(), _pending.end(), [block_id](const auto& pending) {
            // TODO(gpascualg): Client ack ids are always ASC ordered, might be worth to consider binary search
            return std::find(pending->client_ack_ids.begin(), pending->client_ack_ids.end(), block_id) == pending->client_ack_ids.end();
        });

        // Call any callback
        static_cast<Derived&>(*this).on_ack(part);

        // Free memory
        for (auto it = part; it != _pending.end(); ++it)
        {
            auto ptr = *it;
            std::destroy_at(ptr);
            _allocator.deallocate(ptr, 1);
        }

        // Erase from vector
        _pending.erase(part, _pending.end());
        _index = _pending.size();
    }

    template <typename Derived, typename Pending, class Allocator>
    inline void packer<Derived, Pending, Allocator>::clear()
    {
        static_cast<Derived&>(*this).clear();

        // Free memory
        for (auto pending : _pending)
        {
            std::destroy_at(pending);
            _allocator.deallocate(pending, 1);
        }

        // Clear
        _pending.clear();
        _index = 0;
    }

    template <typename Derived, typename Pending, class Allocator>
    inline void packer<Derived, Pending, Allocator>::reset()
    {
        static_cast<packer<Derived, Pending, Allocator>&>(*this).clear();
    }

    template <typename Derived, typename Pending, class Allocator>
    bool packer<Derived, Pending, Allocator>::is_pending(const std::vector<uint16_t>& internal_ticks, uint16_t tick_id, bool force)
    {
        // Do not add the same packet two times, which would probably be due to dependencies
        if (!internal_ticks.empty() && internal_ticks.back() == tick_id)
        {
            return false;
        }

        // Pending inclusions are those forced, not yet included in any block or
        // which have expired without an ack
        return force ||
            internal_ticks.empty() ||
            cx::overflow::sub(tick_id, internal_ticks.back()) >= _resend_threshold;
    }

    template <typename Derived, typename Pending, class Allocator>
    inline uint16_t packer<Derived, Pending, Allocator>::get_actual_tick_id(const std::vector<uint16_t>& internal_ticks, uint16_t tick_id)
    {
        if (!internal_ticks.empty())
        {
            tick_id = internal_ticks.front();
        }
        return tick_id;
    }

    template <typename Derived, typename Pending, class Allocator>
    inline uint16_t packer<Derived, Pending, Allocator>::new_tick_block_cost(uint16_t tick_id, detail::packets_by_block& by_block)
    {
        // Returns 'super_packet_block_size' if tick_id is not in by_block, otherwise 0
        return static_cast<uint16_t>(by_block.find(tick_id) == by_block.end()) * super_packet_block_size;
    }
}
