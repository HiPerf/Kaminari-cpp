#include <kaminari/buffers/packet.hpp>


extern ::kaminari::buffers::packet* get_kaminari_packet(uint16_t opcode);
extern void release_kaminari_packet(::kaminari::buffers::packet* packet);

namespace kaminari
{
    namespace buffers
    {
        packet::packet(uint16_t opcode) :
            _references(0),
            _on_ack(nullptr)
        {
            *reinterpret_cast<uint16_t*>(_data + 2) = static_cast<uint16_t>(opcode);
            _ptr = &_data[0] + DataStart;
        }

        packet::packet(const packet& other) :
            _references(0),
            _on_ack(nullptr)
        {
            std::memcpy(&_data[0], &other._data[0], MAX_PACKET_SIZE);
            _ptr = &_data[0] + other.size();
        }

        boost::intrusive_ptr<packet> packet::make(uint16_t opcode)
        {
            auto packet = get_kaminari_packet(opcode);
            return boost::intrusive_ptr<class packet>(packet);
        }

        void packet::free(packet* packet)
        {
            if (packet->_on_ack)
            {
                packet->_on_ack();
            }

            release_kaminari_packet(packet);
        }

        const packet& packet::finish(uint8_t counter)
        {
            *reinterpret_cast<uint8_t*>(_data) = size();
            *reinterpret_cast<uint8_t*>(_data + 1) = counter;
            return *this;
        }
    }
}
