#pragma once

#include <inttypes.h>


namespace kaminari
{
    constexpr uint8_t MAX_PACKET_SIZE = 255;

    constexpr uint8_t opcode_position = 0;
    constexpr uint16_t opcode_mask = 0xFFFF & (~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3)));
    
    constexpr uint8_t counter_position = 1;
    constexpr uint16_t counter_mask = 0xFFFF 
        & (~((1 << 15) | (1 << 14) | (1 << 13) | (1 << 12))) // Higher-most 4 bits are from opcode
        & (~((1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4) | (1 << 5))); // Lowest-most 6 bits are from diff
    constexpr uint16_t counter_shift = 6;

    constexpr uint8_t header_unshifted_flags_position = 2;

    constexpr uint8_t packet_data_start = (header_unshifted_flags_position + 1) * sizeof(uint8_t);
}
