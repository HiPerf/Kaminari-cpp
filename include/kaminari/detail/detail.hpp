#pragma once

#include <inttypes.h>
#include <map>
#include <vector>

#include <boost/intrusive_ptr.hpp>


namespace kaminari
{
    class packet;

    namespace detail
    {
        using packets_by_block = std::map<uint32_t, std::vector<boost::intrusive_ptr<packet>>>;
    }
}
