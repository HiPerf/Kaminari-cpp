#pragma once

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

            inline uint8_t length() const { return peek<uint8_t>(0); }
            inline uint8_t id() const { return peek<uint8_t>(1); }
            inline uint16_t bytes_read() const { return static_cast<uint16_t>(_ptr - &_data[0]); }
            inline uint16_t opcode() const { return peek<uint16_t>(2); }
            inline uint8_t offset() const { return peek<uint8_t>(5); }
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
