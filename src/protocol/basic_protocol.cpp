#include <kaminari/protocol/basic_protocol.hpp>
#include <kaminari/buffers/packet.hpp>
#include <kaminari/buffers/packet_reader.hpp>
#include <kaminari/super_packet.hpp>


namespace kaminari
{
    basic_protocol::basic_protocol()
    {
        reset();
    }

    void basic_protocol::reset()
    {
        _buffer_mode = BufferMode::NO_BUFFER;
        _since_last_send = 0;
        _since_last_recv = 0;
        _last_block_id_read = 0;
        _expected_block_id = 0;
        _timestamp = 0;
        _timestamp_block_id = 0;
        _already_resolved.clear();
    }

    bool basic_protocol::resolve(packet_reader* packet, uint16_t block_id)
    {
        auto opcode = packet->opcode();
        uint8_t id = packet->id();

        if (auto it = _already_resolved.find(block_id); it != _already_resolved.end())
        {
            // That block has already been parsed
            auto& parsed_deps = it->second;
            if (auto jt = parsed_deps.find(id); jt != parsed_deps.end())
            {
                return false;
            }

            parsed_deps.insert(id);
        }
        else
        {
            _already_resolved.emplace(block_id, std::set<uint8_t> { id });
        }
        
        return true;
    }
}
