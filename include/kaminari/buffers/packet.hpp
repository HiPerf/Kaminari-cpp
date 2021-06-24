#pragma once


#include <inttypes.h>
#include <string>

#include <boost/asio.hpp>
#include <boost/pool/singleton_pool.hpp>
#include <boost/intrusive_ptr.hpp>


namespace kaminari
{
    constexpr uint8_t MAX_PACKET_SIZE = 255;

    template <typename Q> class super_packet;

    namespace buffers
    {
        class packet
        {
            template <typename Q> friend class kaminari::super_packet;
            friend inline void intrusive_ptr_add_ref(packet* x);
            friend inline void intrusive_ptr_release(packet* x);

        public:
            packet(uint16_t opcode);
            
            using ptr = boost::intrusive_ptr<packet>;
            static const uint8_t DataStart = sizeof(uint8_t) * 2 + sizeof(uint16_t) + sizeof(uint8_t);
            
            static boost::intrusive_ptr<packet> make(uint16_t opcode);

            template <typename C>
            static boost::intrusive_ptr<packet> make(uint16_t opcode, C&& on_ack);

            packet& operator<<(const char& v) = delete;

            template <typename T>
            packet& operator<<(const T& v)
            {
                return write(_ptr, v);
            }

            template <typename T>
            packet& write_at(uint8_t position, const T& v)
            {
                auto tmp = _data + position;
                return write(tmp, v);
            }

            template <typename T>
            T peek(uint8_t offset) const
            {
                return peek_ptr<T>(&_data[offset]);
            }
            
            boost::asio::const_buffer buffer() const
            {
                return boost::asio::buffer(const_cast<const uint8_t*>(_data), static_cast<std::size_t>(size()));
            }
            
            inline boost::asio::mutable_buffer header(uint8_t size)
            {
                return boost::asio::buffer(_data, size);
            }
            
            inline boost::asio::mutable_buffer buffer(uint8_t size)
            {
                return boost::asio::buffer(_ptr, size);
            }

            inline uint8_t length() const { return peek<uint8_t>(0); }
            inline uint16_t opcode() const { return peek<uint16_t>(2); }
            inline uint8_t id() const { return peek<uint8_t>(4); }
            inline uint8_t size() const { return static_cast<uint8_t>(_ptr - &_data[0]); }

        private:
            explicit packet(const packet& other);

            static void free(packet* packet);

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

            template <typename T>
            inline packet& write(uint8_t*& ptr, const T& v)
            {
                if constexpr (std::is_base_of_v<packet::ptr, T>)
                {
                    uint8_t size = v->length() - DataStart;
                    memcpy(ptr, v->_data + DataStart, size);
                    ptr += size;
                }
                else if constexpr (std::is_same_v<float, T>)
                {
                    memcpy(ptr, &v, sizeof(float));
                    ptr += sizeof(float);
                }
                else if constexpr (std::is_same_v<std::string, T> || std::is_same_v<std::string_view, T>)
                {
                    *this << static_cast<uint8_t>(v.size());

                    for (uint8_t i = 0; i < v.size(); ++i)
                    {
                        *this << static_cast<uint8_t>(v[i]);
                    }
                }
                else
                {
                    *reinterpret_cast<T*>(ptr) = v;
                    ptr += sizeof(T);
                }
                
                return *this;
            }

            inline uint8_t* raw()
            {
                return _data;
            }
            
            const packet& finish(uint8_t counter);

        private:
            uint8_t _data[MAX_PACKET_SIZE];
            uint8_t* _ptr;
            uint8_t _references;
            std::function<void()> _on_ack;
        };

        inline void intrusive_ptr_add_ref(packet* x)
        {
            ++x->_references;
        }

        inline void intrusive_ptr_release(packet* x)
        {
            if (--x->_references == 0) 
            {
                packet::free(x);
            }
        }
        
        template <typename C>
        boost::intrusive_ptr<packet> packet::make(uint16_t opcode, C&& on_ack)
        {
            auto packet = make(opcode);
            packet->_on_ack = std::move(on_ack);
            return packet;
        }
    }
}
