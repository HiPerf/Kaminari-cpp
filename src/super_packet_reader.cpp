#include <kaminari/super_packet_reader.hpp>


namespace kaminari
{
    super_packet_reader::super_packet_reader(const boost::intrusive_ptr<data_wrapper>& data) :
        _data(data),
        _ack_end(nullptr)
    {}
}
