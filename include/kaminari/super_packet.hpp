#pragma once

#include <kaminari/detail/detail.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/cx/overflow.hpp>

#include <inttypes.h>
#include <mutex>
#include <set>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>


namespace kaminari
{
    enum class super_packet_flags
    {
        none =          0x00,
        handshake =     0x01,
        ping =          0x02,
        ack =           0x80,
        all =           0xFF
    };

    enum class super_packet_internal_flags
    {
        none =          0x00,
        wait_first =    0x01,
        all =           0xFF
    };

    inline constexpr uint32_t super_packet_header_size = sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint8_t);
    inline constexpr uint32_t super_packet_ack_size = sizeof(uint16_t) + sizeof(uint32_t);
    inline constexpr uint32_t super_packet_block_size = sizeof(uint16_t) + sizeof(uint8_t);

    inline constexpr uint16_t super_packet_min_header_size = 5;
    inline constexpr uint16_t super_packet_max_size = 512;

    template <typename Queues>
    class super_packet : public Queues
    {
        struct data_buffer
        {
            bool in_use;
            bool sent;
            uint8_t data[super_packet_max_size];
            uint16_t size;
        };

    public:
        template <typename... Args>
        super_packet(uint8_t resend_threshold, Args&&... args);
        super_packet(const super_packet& other) = delete;
        super_packet(super_packet&& other) = default;
        super_packet& operator=(super_packet&& other) = default;

        void clean();
        void reset();

        void set_flag(super_packet_flags flag);
        bool has_flag(super_packet_flags flag) const;

        void set_internal_flag(super_packet_internal_flags flag);
        bool has_internal_flag(super_packet_internal_flags flag) const;
        void clear_internal_flag(super_packet_internal_flags flag);

        // Player has acked a packet
        void ack(uint16_t block_id);

        // We are ack'ing a player packet
        void schedule_ack(uint16_t block_id);

        // Obtain buffer
        inline void prepare() noexcept;
        bool finish(uint16_t tick_id, bool is_first);
        void free(data_buffer* buffer);

        inline uint16_t id() const;
        inline bool has_buffer() const;
        inline data_buffer* next_buffer();
        inline const data_buffer* peek_last_buffer() const;

        inline bool last_left_data() const;

    private:
        uint16_t _id;
        uint8_t _flags;
        uint8_t _internal_flags;
        bool _last_left_data;
        uint8_t _counter;

        // Needs acks from client
        uint16_t _ack_base;
        uint32_t _pending_acks;
        bool _must_ack;
        std::unordered_map<uint16_t, uint8_t> _clear_flags_on_ack;

        // Data arrays
        uint8_t _write_pointer;
        uint8_t _read_pointer;
        std::vector<data_buffer*> _data_buffers;
    };


    template <typename Queues>
    template <typename... Args>
    super_packet<Queues>::super_packet(uint8_t resend_threshold, Args&&... args) :
        Queues(resend_threshold, std::forward<Args>(args)...),
        _id(0),
        _flags(0),
        _ack_base(0),
        _pending_acks(0),
        _must_ack(false)
    {}

    template <typename Queues>
    void super_packet<Queues>::clean()
    {
        for (auto buffer : _data_buffers)
        {
            delete buffer;
        }
        _data_buffers.clear();
    }

    template <typename Queues>
    void super_packet<Queues>::reset()
    {
        _id = 0;
        _flags = 0;
        Queues::reset();
        _ack_base = 0;
        _pending_acks = 0;
        _must_ack = 0;
        _write_pointer = 0;
        _read_pointer = 0;
        _counter = 0;

        // Have one value ready
        _data_buffers.push_back(new data_buffer{
            .in_use = false
        });
    }

    template <typename Queues>
    void super_packet<Queues>::ack(uint16_t block_id)
    {
        Queues::ack(block_id);

        // Clear flags
        if (auto it = _clear_flags_on_ack.find(block_id); it != _clear_flags_on_ack.end())
        {
            _flags = _flags & (~it->second);
            _clear_flags_on_ack.erase(it);
        }
    }

    template <typename Queues>
    void super_packet<Queues>::set_flag(super_packet_flags flag)
    {
        _flags = _flags | (uint8_t)flag;

        // Clear flags
        if (auto it = _clear_flags_on_ack.find(_id); it != _clear_flags_on_ack.end())
        {
            it->second = it->second | (uint8_t)flag;
        }
        else
        {
            _clear_flags_on_ack.emplace(_id, (uint8_t)flag);
        }
    }

    template <typename Queues>
    bool super_packet<Queues>::has_flag(super_packet_flags flag) const
    {
        return _flags & (uint8_t)flag;
    }

    template <typename Queues>
    void super_packet<Queues>::set_internal_flag(super_packet_internal_flags flag)
    {
        _internal_flags = _internal_flags | (uint8_t)flag;
    }

    template <typename Queues>
    bool super_packet<Queues>::has_internal_flag(super_packet_internal_flags flag) const
    {
        return _internal_flags & (uint8_t)flag;
    }

    template <typename Queues>
    void super_packet<Queues>::clear_internal_flag(super_packet_internal_flags flag)
    {
        _internal_flags = _internal_flags & (~(uint8_t)flag);
    }

    template <typename Queues>
    void super_packet<Queues>::schedule_ack(uint16_t block_id)
    {
        // Move pending acks
        if (cx::overflow::ge(block_id, _ack_base))
        {
            uint16_t ack_diff = cx::overflow::sub(block_id, _ack_base);
            _pending_acks = (_pending_acks << ack_diff) | 1; // | 1 because we are acking the base
            _ack_base = block_id;
        }
        else
        {
            uint16_t ack_diff = cx::overflow::sub(_ack_base, block_id);
            _pending_acks = _pending_acks | (1 << ack_diff);
        }

        _must_ack = true;
    }

    template <typename Queues>
    void super_packet<Queues>::prepare() noexcept
    {
        _counter = 0;
    }

    template <typename Queues>
    bool super_packet<Queues>::finish(uint16_t tick_id, bool is_first)
    {
        // Check if we have any available slots
        if (_data_buffers[_write_pointer]->in_use)
        {
            _data_buffers.insert(_data_buffers.cbegin() + _write_pointer, new data_buffer{});
        }

        // Current write head
        _data_buffers[_write_pointer]->in_use = true;
        _data_buffers[_write_pointer]->sent = false;
        auto* data = _data_buffers[_write_pointer]->data;
        auto& size = _data_buffers[_write_pointer]->size;
        _write_pointer = (_write_pointer + 1) % _data_buffers.size();
        uint8_t* ptr = data;

        // Superpacket header is [TICK_ID, PACKET_ID, FLAGS, ACK_BASE, ACKS]
        *reinterpret_cast<uint16_t*>(ptr) = tick_id;
        ptr += sizeof(uint16_t);
        *reinterpret_cast<uint16_t*>(ptr) = _id;
        ptr += sizeof(uint16_t);
        *reinterpret_cast<uint8_t*>(ptr) = _flags;
        ptr += sizeof(uint8_t);

        // Make sure
        assert((ptr - data) == super_packet_header_size && "Header size does not match");

        // Reset if there is something we must ack
        bool has_acks = _must_ack;
        _must_ack = false;

        // Write acks and move for next id
        // Moving acks bitset only happens if no doing handshake (ie. if incrementing id)
        *reinterpret_cast<uint16_t*>(ptr) = _ack_base;
        ptr += sizeof(uint16_t);
        *reinterpret_cast<uint32_t*>(ptr) = _pending_acks;
        ptr += sizeof(uint32_t);

        assert((ptr - data) == super_packet_header_size + super_packet_ack_size && "Ack size does not match");

        // TODO(gpascualg): Do not use magic numbers in super packet
        // How much packet is there left?
        //  -1 is to account for the number of blocks
        //  -1 more to account for start at 0
        uint16_t remaining = super_packet_max_size - (ptr - data) - 1 - 1;

        // Set that we didn't run short of space
        _last_left_data = false;
        bool has_data = false;
        if (!has_flag(super_packet_flags::handshake))
        {
            // Organize pending opcodes by superpacket, oldest to newest
            detail::packets_by_block by_block;
            Queues::process(tick_id, _id, remaining, _last_left_data, by_block);

            // Write number of blocks
            *reinterpret_cast<uint8_t*>(ptr) = by_block.size();
            ptr += sizeof(uint8_t);

            // Do we have any data?
            has_data = !by_block.empty();

            // Has there been any overflow? That can be detected by, for example
            //  checking max-min>thr
            if (has_data)
            {
                auto max = by_block.rbegin()->first;
                constexpr auto threshold = std::numeric_limits<uint16_t>::max() / 2;
            
                auto it = by_block.begin();
                while (max - it->first >= threshold)
                {
                    // Overflows are masked higher, so that they get pushed to the end
                    //  of the map
                    // Not calling et.empty() makes GCC complain about a null pointer
                    //  deference... but it can't be null, we've got it from an iterator
                    auto et = by_block.extract(it);
                    et.key() = (uint32_t(1) << 16) | et.key(); 
                    by_block.insert(std::move(et));

                    it = by_block.begin();
                }

                // Write in the packet
                for (auto& [id, pending_packets] : by_block)
                {
                    static_assert(super_packet_block_size == sizeof(uint16_t) + sizeof(uint8_t), "Packet block size does not match");

                    // Write section identifier and number of packets
                    *reinterpret_cast<uint16_t*>(ptr) = static_cast<uint16_t>(id);
                    ptr += sizeof(uint16_t);
                    *reinterpret_cast<uint8_t*>(ptr) = static_cast<uint8_t>(pending_packets.size());
                    ptr += sizeof(uint8_t);

                    // Write all packets
                    for (auto& pending : pending_packets)
                    {
                        if (id == tick_id)
                        {
                            pending->finish(_counter++);
                        }

                        memcpy(ptr, pending->raw(), pending->size());
                        ptr += pending->size();
                    }
                }
            }
        }

        // Increment _id for next iter and acks
        _id = cx::overflow::inc(_id);

        // And done
        size = ptr - data;
#ifndef NDEBUG
        if (size >= super_packet_max_size)
        {
            printf("PACKET OVERFLOW WITH %u > %u, remaining was: %u", size, super_packet_max_size, remaining);
        }
#endif
        assert(size < super_packet_max_size && "Superpacket size exceeds maximum");
        return has_acks || has_data || (is_first && _flags != 0);
    }

    template <typename Queues>
    void super_packet<Queues>::free(data_buffer* buffer)
    {
        buffer->in_use = false;
    }

    template <typename Queues>
    inline uint16_t super_packet<Queues>::id() const
    {
        return _id;
    }

    template <typename Queues>
    inline bool super_packet<Queues>::has_buffer() const
    {
        return !_data_buffers[_read_pointer]->sent && _data_buffers[_read_pointer]->in_use;
    }

    template <typename Queues>
    inline typename super_packet<Queues>::data_buffer* super_packet<Queues>::next_buffer()
    {
        auto buffer = _data_buffers[_read_pointer];
        buffer->sent = true;
        _read_pointer = (_read_pointer + 1) % _data_buffers.size();
        return buffer;
    }

    template <typename Queues>
    inline const typename super_packet<Queues>::data_buffer* super_packet<Queues>::peek_last_buffer() const
    {
        auto pointer = _read_pointer;
        while (true)
        {
            auto next_pointer = (pointer + 1) % _data_buffers.size();
            if (!_data_buffers[next_pointer]->in_use || next_pointer == _read_pointer)
            {
                return _data_buffers[pointer];
            }
            pointer = next_pointer;
        }

        return nullptr;
    }

    template <typename Queues>
    inline bool super_packet<Queues>::last_left_data() const
    {
        return _last_left_data;
    }
}
