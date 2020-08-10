#include <kaminari/client/basic_client.hpp>


namespace kaminari
{
    basic_client::basic_client() :
        // TODO(gpascualg): Configurable maximum pending packets
        _pending_super_packets(100)
    {}
}
