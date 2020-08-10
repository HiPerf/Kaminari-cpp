#pragma once

#include <kaminari/detail/detail.hpp>
#include <kaminari/buffers/packet.hpp>

#include <inttypes.h>
#include <mutex>
#include <set>
#include <vector>
#include <unordered_map>

#include <boost/asio.hpp>


namespace kaminari
{
    template <typename Queues>
    class super_packet : public Queues
    {
    public:
        constexpr static inline uint16_t MinHeaderSize = 5;
        constexpr static inline uint16_t MaxSize = 512;

        template <typename... Args>
        super_packet(uint8_t resend_threshold, Args&&... args);
        super_packet(const super_packet& other) = delete;
        super_packet(super_packet&& other) = default;
        super_packet& operator=(super_packet&& other) = default;

        void reset();

        // Player has acked a packet
        void ack(uint16_t block_id);

        // We are ack'ing a player packet
        void schedule_ack(uint16_t block_id);

        // Obtain buffer
        bool finish();

        inline uint16_t id() const;
        inline const boost::asio::const_buffer& buffer() const;

    private:
        uint16_t _id;

        // Needs acks from client
        std::vector<uint16_t> _pending_acks;
        
        // Data array
        uint8_t _data[MaxSize];
        boost::asio::const_buffer _buffer;
    };


    template <typename Queues>
    inline uint16_t super_packet<Queues>::id() const
    {
        return _id;
    }

    template <typename Queues>
    inline const boost::asio::const_buffer& super_packet<Queues>::buffer() const
    {
        return _buffer;    
    }

    template <typename Queues>
    template <typename... Args>
    super_packet<Queues>::super_packet(uint8_t resend_threshold, Args&&... args) :
        Queues(resend_threshold, std::forward<Args>(args)...),
        _id(0)
    {}

    template <typename Queues>
    void super_packet<Queues>::reset()
    {
        _id = 0;
        Queues::reset();
        _pending_acks.clear();
    }

    template <typename Queues>
    void super_packet<Queues>::ack(uint16_t block_id)
    {
        Queues::ack(block_id);
    }

    template <typename Queues>
    void super_packet<Queues>::schedule_ack(uint16_t block_id)
    {
        _pending_acks.push_back(block_id);
    }

    template <typename Queues>
    bool super_packet<Queues>::finish()
    {
        // Current write head
        uint8_t* ptr = _data;

        // First two bytes are size, next two id
        ptr += sizeof(uint16_t); // Will set length later!
        *reinterpret_cast<uint16_t*>(ptr) = _id;
        ptr += sizeof(uint16_t);

        // Write acks
        uint8_t ack_num = static_cast<uint8_t>(_pending_acks.size());
        *reinterpret_cast<uint8_t*>(ptr) = ack_num;
        ptr += sizeof(uint8_t);

        bool has_acks = ack_num > 0;
        if (has_acks)
        {
            uint16_t ack_size = static_cast<uint16_t>(ack_num * sizeof(uint16_t));
            memcpy(ptr, &_pending_acks[0], ack_size);
            ptr += ack_size;
        }

        // Clear all acks
        _pending_acks.clear();

        // How much packet is there left?
        //  -1 is to account for the number of blocks
        uint16_t remaining = 500 - (ptr - _data) - 1;

        // Organize pending opcodes by superpacket, oldest to newest
        detail::packets_by_block by_block;
        Queues::process(_id, remaining, by_block);

        // Write number of blocks
        *reinterpret_cast<uint8_t*>(ptr) = by_block.size();
        ptr += sizeof(uint8_t);

        // Do we have any data?
        bool has_data = !by_block.empty();

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

            // Current packet counter
            uint8_t counter = 0;

            // Write in the packet
            for (auto& [id, pending_packets] : by_block)
            {
                // Write section identifier and number of packets
                *reinterpret_cast<uint16_t*>(ptr) = static_cast<uint16_t>(id);
                ptr += sizeof(uint16_t);
                *reinterpret_cast<uint8_t*>(ptr) = static_cast<uint8_t>(pending_packets.size());
                ptr += sizeof(uint8_t);

                // Write all packets
                for (auto& pending : pending_packets)
                {
                    if (id == _id)
                    {
                        pending->finish(counter++);
                    }

                    memcpy(ptr, pending->raw(), pending->size());
                    ptr += pending->size();
                }
            }
        }

        // Done, write length
        auto size = static_cast<uint16_t>(ptr - _data);
        *reinterpret_cast<uint16_t*>(_data) = size;

        // Increment _id for next iter
        ++_id;

        // And done
        _buffer = boost::asio::buffer(const_cast<const uint8_t*>(_data), static_cast<std::size_t>(size));
        return has_acks || has_data;
    }
}
