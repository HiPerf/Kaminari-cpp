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
            *reinterpret_cast<uint16_t*>(_data + opcode_position) = static_cast<uint16_t>(opcode);
            *reinterpret_cast<uint8_t*>(_data + header_unshifted_flags_position) = static_cast<uint8_t>(0);
            _ptr = &_data[0] + packet_data_start;
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
            // TODO(gpacualg): Test if having a noop instead of an if is faster
            if (packet->_on_ack)
            {
                packet->_on_ack();
            }

            release_kaminari_packet(packet);
        }

        const packet& packet::finish(uint8_t counter)
        {
#if !defined(NDEBUG)
            auto old_opcode = opcode();
#endif

            // We must take into account that OPCODE (HHL0) gets shifted in memory to L0HH
            *reinterpret_cast<uint8_t*>(_data + 0) |= (counter & 0x0F);
            *reinterpret_cast<uint8_t*>(_data + 2) |= ((counter & 0x30) << 2);

#if !defined(NDEBUG)
            assert(old_opcode == opcode() && "After writing counter, opcode is wrong");
#endif
            return *this;
        }
    }
}
