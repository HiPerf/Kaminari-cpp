#pragma once

#include <kaminari/buffers/common.hpp>

#include <inttypes.h>
#include <string>
#include <type_traits>

#include <boost/asio.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/intrusive_ptr.hpp>


namespace kaminari
{
    class super_packet_reader;
    
    namespace buffers
    {
        class packet_reader
        {
            friend class kaminari::super_packet_reader;

        public:
            template <typename T>
            T peek() const
            {
                return peek_ptr<T>(_ptr);
            }

            template <typename T>
            T peek(uint8_t offset) const
            {
                return peek_ptr<T>(&_data[offset]);
            }

            template <typename T>
            T read()
            {
                if constexpr (std::is_same_v<std::string, T>)
                {
                    T v = peek_ptr<T>(_ptr);
                    _ptr += v.length() + sizeof(uint8_t);
                    return v;
                }
                else
                {
                    T v = peek_ptr<T>(_ptr);
                    _ptr += sizeof(T);
                    return v;
                }
            }

            inline uint16_t opcode() const { return peek<uint16_t>(opcode_position) & opcode_mask; }
            inline uint8_t counter() const
            {
                // We must take into account that OPCODE gets shifted in memory
                //  LLHH, which leaves us 0LHHc
                uint8_t low = peek<uint8_t>(0) & 0x0F;
                uint8_t high = (peek<uint8_t>(2) & 0xC0) >> 2;
                return high | low;
            }

            inline uint32_t extended_id() const
            {
                return peek<uint32_t>(opcode_position) & 0x00C0FFFF;
            }

            inline uint16_t bytes_read() const { return static_cast<uint16_t>(_ptr - &_data[0]); }
            inline uint8_t offset() const { return peek<uint8_t>(header_unshifted_flags_position) & (~((1 << 7) | (1 << 6))); }
            inline uint64_t timestamp() const;
            inline uint16_t buffer_size() const;

        private:
            packet_reader(const uint8_t* data, uint64_t block_timestamp, uint16_t buffer_size);

            template <typename T>
            T peek_ptr(const uint8_t* ptr) const
            {
                if constexpr (std::is_same_v<float, T>)
                {
                    float v;
                    memcpy(&v, ptr, sizeof(float));
                    return v;
                }
                else if constexpr (std::is_same_v<std::string, T>)
                {
                    const uint8_t size = peek_ptr<uint8_t>(ptr);
                    return { reinterpret_cast<const char*>(ptr + sizeof(uint8_t)), static_cast<std::size_t>(size) };
                }
                else
                {
                    return *reinterpret_cast<const T*>(ptr);
                }
            }

        private:
            const uint8_t* _data;
            const uint8_t* _ptr;
            const uint64_t _block_timestamp;
            const uint16_t _buffer_size;
        };


        inline uint64_t packet_reader::timestamp() const
        {
            return _block_timestamp - offset();
        }


        inline uint16_t packet_reader::buffer_size() const
        {
            return _buffer_size;
        }
    }
}
