#include <kaminari/client/basic_client.hpp>


namespace kaminari
{
    basic_client::basic_client() noexcept :
        _status(status::CONNECTED),
        // TODO(gpascualg): Configurable maximum pending packets
        _pending_super_packets(100),
        _lag(50)
    {}

    void basic_client::reset() noexcept
    {
        _status = status::CONNECTED;
        _pending_super_packets.clear();
        _lag = 50;
    }
}
