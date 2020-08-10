#include <kaminari/buffers/packet_reader.hpp>
#include <kaminari/buffers/packet.hpp>


namespace kaminari
{
    packet_reader::packet_reader(const uint8_t* data, uint64_t block_timestamp, uint16_t buffer_size) :
        _data(data),
        _block_timestamp(block_timestamp),
        _buffer_size(buffer_size)
    {
        _ptr = data + packet::DataStart;
    }
}
