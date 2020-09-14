#pragma once

#include <kaminari/buffers/packet_reader.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/super_packet.hpp>
#include <kaminari/cx/overflow.hpp>
#include <kaminari/protocol/basic_protocol.hpp>
#include <kaminari/types/data_wrapper.hpp>

#include <boost/intrusive_ptr.hpp>

#include <inttypes.h>


namespace kaminari
{
    class basic_client;
    class protocol;

    class super_packet_reader
    {
    public:
        explicit super_packet_reader(const boost::intrusive_ptr<data_wrapper>& data);
        super_packet_reader(const super_packet_reader&) = delete; // Do not copy!
        super_packet_reader(super_packet_reader&&) = default;
        ~super_packet_reader() = default;

        inline uint16_t length() const;
        inline uint16_t id() const;

        template <typename Queues>
        void handle_acks(super_packet<Queues>* super_packet);

        inline uint8_t* data();
        inline bool has_data();

        template <typename Marshal>
        void handle_packets(basic_client* client, basic_protocol* protocol);
    private:
        boost::intrusive_ptr<data_wrapper> _data;
        const uint8_t* _ack_end;
    };


    inline uint16_t super_packet_reader::length() const
    {
        return *reinterpret_cast<const uint16_t*>(_data->data);
    }

    inline uint16_t super_packet_reader::id() const
    {
        return *reinterpret_cast<const uint16_t*>(_data->data + sizeof(uint16_t));
    }

    template <typename Queues>
    void super_packet_reader::handle_acks(super_packet<Queues>* super_packet)
    {
        _ack_end = _data->data + sizeof(uint16_t) * 2;
        uint8_t num_acks = *reinterpret_cast<const uint8_t*>(_ack_end);
        _ack_end += sizeof(uint8_t);

        for (uint8_t i = 0; i < num_acks; ++i)
        {
            uint16_t ack = *reinterpret_cast<const uint16_t*>(_ack_end);
            super_packet->ack(ack);
            _ack_end += sizeof(uint16_t);
        }
    }

    inline uint8_t* super_packet_reader::data()
    {
        return _data->data;
    }

    inline bool super_packet_reader::has_data()
    {
        return *reinterpret_cast<const uint8_t*>(_ack_end) != 0x0;
    }

    template <typename Marshal>
    void super_packet_reader::handle_packets(basic_client* client, basic_protocol* protocol)
    {
        // Start reading old blocks
        uint8_t num_blocks = *reinterpret_cast<const uint8_t*>(_ack_end);
        const uint8_t* block_pos = _ack_end + sizeof(uint8_t);

        // Set some upper limit to avoid exploits
        int remaining = _data->size - (block_pos - _data->data); // Keep it signed on purpouse
        for (uint8_t i = 0; i < num_blocks; ++i)
        {
            uint16_t block_id = *reinterpret_cast<const uint16_t*>(block_pos);
            uint8_t num_packets = *reinterpret_cast<const uint8_t*>(block_pos + sizeof(uint16_t));
            if (num_packets == 0)
            {
                // TODO(gpascualg): Should we kick the player for packet forging?
                return;
            }

            const uint64_t block_timestamp = protocol->block_timestamp(block_id);
            block_pos += sizeof(uint16_t) + sizeof(uint8_t);
            remaining -= sizeof(uint16_t) + sizeof(uint8_t);

            for (uint8_t j = 0; j < num_packets && remaining > 0; ++j)
            {
                packet_reader reader(block_pos, block_timestamp, remaining);
                uint16_t length = reader.length();
                block_pos += length;
                remaining -= length;

                if (length < packet::DataStart || remaining < 0)
                {
                    // TODO(gpascualg): Should we kick the player for packet forging?
                    return;
                }   

                if (protocol->resolve(&reader, block_id))
                {
                    Marshal::handle_packet(&reader, client);
                }
            }
        }
    }
}
