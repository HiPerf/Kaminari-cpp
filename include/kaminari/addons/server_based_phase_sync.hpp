#pragma once

#include <inttypes.h>


namespace kaminari
{
    class server_based_sync
    {
    public:
        server_based_sync();

        template <typename C>
        void on_receive_packet(C* client, uint16_t last_server_id);

        inline int16_t superpackets_id_diff() const;

    private:
        int16_t _superpackets_id_diff;
    };
    
    server_based_sync::server_based_sync() :
        _superpackets_id_diff(0)
    {}

    template <typename C>
    void server_based_sync::on_receive_packet(C* client, uint16_t last_server_id)
    {
        _superpackets_id_diff = (int16_t)((int)(client->super_packet()->id()) - (int)(last_server_id));
        client->protocol()->override_expected_block_id(last_server_id);
    }

    inline int16_t server_based_sync::superpackets_id_diff() const
    {
        return _superpackets_id_diff;
    }
}
