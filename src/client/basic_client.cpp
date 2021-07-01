#include <kaminari/client/basic_client.hpp>
#include <kaminari/super_packet_reader.hpp>


namespace kaminari
{
    basic_client::basic_client() noexcept :
        _status(status::CONNECTED),
        // TODO(gpascualg): Configurable maximum pending packets
        _pending_super_packets(100),
        _lag(50)
    {}

    basic_client::basic_client(basic_client&& other) noexcept :
        _status(other._status),
        _lag(other._lag)
    {
        _pending_super_packets.swap(other._pending_super_packets);
    }

    basic_client& basic_client::operator=(basic_client&& other) noexcept
    {
        _status = other._status;
        _pending_super_packets.swap(other._pending_super_packets);
        _lag = other._lag;
        return *this;
    }

    void basic_client::reset() noexcept
    {
        _status = status::CONNECTED;
        _pending_super_packets.clear();
        _lag = 50;
    }

    uint16_t basic_client::first_super_packet_id() const noexcept
    {
        return _pending_super_packets.front().id();
    }

    bool basic_client::has_pending_super_packets() const noexcept
    {
        return !_pending_super_packets.empty();
    }

    super_packet_reader basic_client::first_super_packet() noexcept
    {
        auto data = _pending_super_packets.front();
        _pending_super_packets.pop_front();
        return data;
    }
}
