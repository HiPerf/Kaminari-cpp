#include "kaminari/detail/detail.hpp"
#include "kaminari/super_packet.hpp"
#include <catch2/catch_all.hpp>

#include <kaminari/packers/immediate_packer.hpp>
#include <kaminari/packers/ordered_packer.hpp>
#include <kaminari/packers/vector_merge_packer.hpp>
#include <kaminari/packers/vector_merge_with_priority_packer.hpp>
#include <stdlib.h>


#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif
#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

struct test_data_1
{
    uint8_t x;

    static auto get() { return test_data_1 { .x = 0 }; }
};
PACK(struct _test_data_1_packed { uint8_t x; });

struct test_data_2
{
    uint16_t x;

    static auto get() { return test_data_2 { .x = 0 }; }
};
PACK(struct _test_data_2_packed { uint16_t x; });

struct test_data_3
{
    uint8_t x;
    uint16_t y;

    static auto get() { return test_data_3 { .x = 0, .y = 0 }; }
};
PACK(struct _test_data_3_packed { uint8_t x; uint16_t y; });

struct test_data_4
{
    uint32_t x;

    static auto get() { return test_data_4 { .x = 0 }; }
};
PACK(struct _test_data_4_packed { uint32_t x; });

struct test_data_5
{
    uint8_t x;
    uint16_t y;
    uint32_t z;

    static auto get() noexcept
    {
        return test_data_5 {
            .x = 0,
                .y = 0,
                .z = 0
        };
    }
};

PACK(struct _test_data_5_packed
        {
        uint8_t x;
        uint16_t y;
        uint32_t z;
        });


struct test_data_1_with_id
{
    uint64_t id;
    uint8_t x;
    float priority;
    float priority_multiplier;

    static auto get() { return test_data_1_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX, .priority_multiplier = 1.5f }; }
};
PACK(struct _test_data_1_with_id_packed { uint64_t id; uint8_t x; });

struct test_data_2_with_id
{
    uint64_t id;
    uint16_t x;
    float priority;
    float priority_multiplier;

    static auto get() { return test_data_2_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX, .priority_multiplier = 1.5f  }; }
};
PACK(struct _test_data_2_with_id_packed { uint64_t id; uint16_t x; });

struct test_data_3_with_id
{
    uint64_t id;
    uint8_t x;
    uint16_t y;
    float priority;
    float priority_multiplier;

    static auto get() { return test_data_3_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .y = 0, .priority = rand() / (float)RAND_MAX, .priority_multiplier = 1.5f  }; }
};
PACK(struct _test_data_3_with_id_packed { uint64_t id; uint8_t x; uint16_t y; });

struct test_data_4_with_id
{
    uint64_t id;
    uint32_t x;
    float priority;
    float priority_multiplier;

    static auto get() { return test_data_4_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX, .priority_multiplier = 1.5f }; }
};
PACK(struct _test_data_4_with_id_packed { uint64_t id; uint32_t x; });

struct test_data_5_with_id
{
    uint64_t id;
    uint8_t x;
    uint16_t y;
    uint32_t z;
    float priority;
    float priority_multiplier;

    static auto get() noexcept
    {
        return test_data_5_with_id {
            .id = (uint64_t)(rand() % 516),
                .x = 0,
                .y = 0,
                .z = 0,
                .priority = rand() / (float)RAND_MAX,
                .priority_multiplier = 1.5f
        };
    }
};

PACK(struct _test_data_5_with_id_packed
        {
        uint64_t id;
        uint8_t x;
        uint16_t y;
        uint32_t z;
        });

std::string gen_random(const int len) {
    static const char alphanum[] =
        "012356789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}

struct test_dynamic
{
    std::string x;

    static auto get() noexcept
    {
        return test_dynamic {
            .x = gen_random((rand() % 12) + 1)
        };
    }
};

struct test_dynamic_with_id
{
    uint64_t id;
    std::string x;
    float priority;
    float priority_multiplier;

    static auto get() noexcept
    {
        return test_dynamic_with_id {
            .id = (uint64_t)(rand() % 516),
            .x = gen_random((rand() % 12) + 1),
            .priority = rand() / (float)RAND_MAX,
            .priority_multiplier = 1.5f
        };
    }
};

template <typename T>
struct test_ev_sync
{
    std::vector<T> data;
};

class test_marshal
{
    public:

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_1& data)
        {
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_2& data)
        {
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_3& data)
        {
            *packet << data.x;
            *packet << data.y;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_4& data)
        {
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_5& data)
        {
            *packet << data.x;
            *packet << data.y;
            *packet << data.z;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_dynamic& data)
        {
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_1_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_2_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_3_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
            *packet << data.y;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_4_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_data_5_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
            *packet << data.y;
            *packet << data.z;
        }

        static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_dynamic_with_id& data)
        {
            *packet << data.id;
            *packet << data.x;
        }

        template <typename T>
            static void pack(const boost::intrusive_ptr<::kaminari::buffers::packet>& packet, const test_ev_sync<T>& data)
            {
                *packet << (uint8_t)data.data.size();
                for (const auto& d : data.data)
                {
                    pack(packet, d);
                }
            }

        static uint8_t sizeof_test_data_1() { return sizeof(_test_data_1_packed); }
        static uint8_t packet_size(const test_data_1& data) { return sizeof(_test_data_1_packed); }

        static uint8_t sizeof_test_data_2() { return sizeof(_test_data_2_packed); }
        static uint8_t packet_size(const test_data_2& data) { return sizeof(_test_data_2_packed); }

        static uint8_t sizeof_test_data_3() { return sizeof(_test_data_3_packed); }
        static uint8_t packet_size(const test_data_3& data) { return sizeof(_test_data_3_packed); }

        static uint8_t sizeof_test_data_4() { return sizeof(_test_data_4_packed); }
        static uint8_t packet_size(const test_data_4& data) { return sizeof(_test_data_4_packed); }

        static uint8_t sizeof_test_data_5() { return sizeof(_test_data_5_packed); }
        static uint8_t packet_size(const test_data_5& data) { return sizeof(_test_data_5_packed); }

        static uint8_t packet_size(const test_dynamic& data) { return sizeof(uint8_t) + data.x.size(); }

        static uint8_t sizeof_test_data_1_with_id() { return sizeof(_test_data_1_with_id_packed); }
        static uint8_t packet_size(const test_data_1_with_id& data) { return sizeof(_test_data_1_with_id_packed); }

        static uint8_t sizeof_test_data_2_with_id() { return sizeof(_test_data_2_with_id_packed); }
        static uint8_t packet_size(const test_data_2_with_id& data) { return sizeof(_test_data_2_with_id_packed); }

        static uint8_t sizeof_test_data_3_with_id() { return sizeof(_test_data_3_with_id_packed); }
        static uint8_t packet_size(const test_data_3_with_id& data) { return sizeof(_test_data_3_with_id_packed); }

        static uint8_t sizeof_test_data_4_with_id() { return sizeof(_test_data_4_with_id_packed); }
        static uint8_t packet_size(const test_data_4_with_id& data) { return sizeof(_test_data_4_with_id_packed); }

        static uint8_t sizeof_test_data_5_with_id() { return sizeof(_test_data_5_with_id_packed); }
        static uint8_t packet_size(const test_data_5_with_id& data) { return sizeof(_test_data_5_with_id_packed); }

        static uint8_t packet_size(const test_dynamic_with_id& data) { return sizeof(uint64_t) + sizeof(uint8_t) + data.x.size(); }
};

::kaminari::buffers::packet* get_kaminari_packet(uint16_t opcode)
{
    return new ::kaminari::buffers::packet(opcode);
}

void release_kaminari_packet(::kaminari::buffers::packet* packet)
{
    delete packet;
}

std::size_t get_packets_size(const kaminari::detail::packets_by_block& by_block)
{
    std::size_t total_size = 0;
    for (const auto& [block, data] : by_block)
    {
        for (const auto& packet : data)
        {
            total_size += packet->size();
        }
    }
    return total_size;
}

std::size_t get_total_size(const kaminari::detail::packets_by_block& by_block)
{
    return get_packets_size(by_block) + by_block.size() * kaminari::super_packet_block_size;
}

std::size_t get_super_packet_size(const kaminari::detail::packets_by_block& by_block)
{
    return kaminari::super_packet_header_size + kaminari::super_packet_ack_size + kaminari::super_packet_data_prefrace_size + get_total_size(by_block);
}

std::size_t get_total_processed(const kaminari::detail::packets_by_block& by_block)
{
    std::size_t total_processed = 0;
    for (const auto& [block, data] : by_block)
    {
        total_processed += data.size();
    }
    return total_processed;
}


template <typename Packer, typename Allocator, typename Data>
void add_and_process(uint8_t resend_threshold, int num_packets)
{
    Packer packer(resend_threshold, Allocator());

    WHEN("it processes")
    {
        uint16_t tick_id = 0;
        uint16_t block_id = 0;
        bool add_packets = true;
        while (packer.any_pending() || tick_id == 0)
        {
            if (add_packets)
            {
                add_packets = false;
                for (int i = 0; i < num_packets / (tick_id + 1); ++i)
                {
                    packer.add(1010, Data::get());
                }
            }

            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - kaminari::super_packet_data_prefrace_size;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            packer.process(tick_id, block_id++, remaining, unfitting_data, by_block);
            // Rand block processed
            if (rand() / (float)RAND_MAX > 0.9f)
            {
                packer.ack(block_id - 1);
            }

            printf("At %u / %u processed %lld in %lld blocks, size %lld and remaining %u\n", tick_id, block_id, get_total_processed(by_block), by_block.size(), get_super_packet_size(by_block), remaining);
            if (by_block.empty())
            {
                ++tick_id;
                add_packets = true;
                continue;
            }

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_packets_size(by_block) < max_data_size); // Because this is without block sizes
                REQUIRE(get_super_packet_size(by_block) <= kaminari::super_packet_max_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }
}

SCENARIO("an immediate_packer processes many accumulated data")
{
    using allocator_t = std::allocator<kaminari::immediate_packer_allocator_t>;
    using packer_t = kaminari::immediate_packer<test_marshal, allocator_t>;

    GIVEN("1235 data, 1 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(1, 1000);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(2, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(3, 200);
    }

    // 2 BYTES
    GIVEN("1235 data, 2 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(1, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(2, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(3, 200);
    }

    // 3 BYTES
    GIVEN("1235 data, 3 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(1, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(2, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(3, 200);
    }

    // 4 BYTES
    GIVEN("1235 data, 4 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(1, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(2, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(3, 200);
    }

    // 5 BYTES
    GIVEN("1235 data, 5 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(1, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(2, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(3, 200);
    }

    // DYNAMIC
    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(1, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(2, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(3, 200);
    }
}


SCENARIO("an ordered_packer processes many accumulated data")
{
    using allocator_t = std::allocator<kaminari::ordered_packer_allocator_t>;
    using packer_t = kaminari::ordered_packer<test_marshal, allocator_t>;

    GIVEN("1235 data, 1 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(1, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(2, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_1>(3, 200);
    }

    // 2 BYTES
    GIVEN("1235 data, 2 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(1, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(2, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_2>(3, 200);
    }

    // 3 BYTES
    GIVEN("1235 data, 3 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(1, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(2, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_3>(3, 200);
    }

    // 4 BYTES
    GIVEN("1235 data, 4 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(1, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(2, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_4>(3, 200);
    }

    // 5 BYTES
    GIVEN("1235 data, 5 byte each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(1, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(2, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_data_5>(3, 200);
    }

    // DYNAMIC
    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 1")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(1, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 2")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(2, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 3")
    {
        add_and_process<packer_t, allocator_t, test_dynamic>(3, 200);
    }
}


template <typename D>
using vm_allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<D>>;

template <typename D>
using vm_packer_t = kaminari::vector_merge_packer<uint64_t, test_ev_sync<D>, D, 1010, test_marshal, vm_allocator_t<D>>;

SCENARIO("an vector_merge_packer processes many accumulated data")
{
    GIVEN("1235 data, 1 byte each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_data_1_with_id>, vm_allocator_t<test_data_1_with_id>, test_data_1_with_id>(1, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_data_1_with_id>, vm_allocator_t<test_data_1_with_id>, test_data_1_with_id>(2, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_data_1_with_id>, vm_allocator_t<test_data_1_with_id>, test_data_1_with_id>(3, 200);
    }

    // 2 BYTES
    GIVEN("1235 data, 2 byte each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_data_2_with_id>, vm_allocator_t<test_data_2_with_id>, test_data_2_with_id>(1, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_data_2_with_id>, vm_allocator_t<test_data_2_with_id>, test_data_2_with_id>(2, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_data_2_with_id>, vm_allocator_t<test_data_2_with_id>, test_data_2_with_id>(3, 200);
    }

    // 3 BYTES
    GIVEN("1235 data, 3 byte each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_data_3_with_id>, vm_allocator_t<test_data_3_with_id>, test_data_3_with_id>(1, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_data_3_with_id>, vm_allocator_t<test_data_3_with_id>, test_data_3_with_id>(2, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_data_3_with_id>, vm_allocator_t<test_data_3_with_id>, test_data_3_with_id>(3, 200);
    }

    // 4 BYTES
    GIVEN("1235 data, 4 byte each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_data_4_with_id>, vm_allocator_t<test_data_4_with_id>, test_data_4_with_id>(1, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_data_4_with_id>, vm_allocator_t<test_data_4_with_id>, test_data_4_with_id>(2, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_data_4_with_id>, vm_allocator_t<test_data_4_with_id>, test_data_4_with_id>(3, 200);
    }

    // 5 BYTES
    GIVEN("1235 data, 5 byte each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_data_5_with_id>, vm_allocator_t<test_data_5_with_id>, test_data_5_with_id>(1, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_data_5_with_id>, vm_allocator_t<test_data_5_with_id>, test_data_5_with_id>(2, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_data_5_with_id>, vm_allocator_t<test_data_5_with_id>, test_data_5_with_id>(3, 200);
    }

    // DYNAMIC
    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 1")
    {
        add_and_process<vm_packer_t<test_dynamic_with_id>, vm_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(1, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 2")
    {
        add_and_process<vm_packer_t<test_dynamic_with_id>, vm_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(2, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 3")
    {
        add_and_process<vm_packer_t<test_dynamic_with_id>, vm_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(3, 200);
    }
}

template <typename D>
using vmp_allocator_t = std::allocator<kaminari::vector_merge_with_priority_packer_allocator_t<D>>;

template <typename D>
using vmp_packer_t = kaminari::vector_merge_with_priority_packer<uint64_t, test_ev_sync<D>, D, 1010, test_marshal, vmp_allocator_t<D>>;

SCENARIO("an vector_merge_with_priority_packer processes many accumulated data")
{
    GIVEN("1235 data, 1 byte each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_data_1_with_id>, vmp_allocator_t<test_data_1_with_id>, test_data_1_with_id>(1, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_data_1_with_id>, vmp_allocator_t<test_data_1_with_id>, test_data_1_with_id>(2, 200);
    }

    GIVEN("1235 data, 1 byte each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_data_1_with_id>, vmp_allocator_t<test_data_1_with_id>, test_data_1_with_id>(3, 200);
    }

    // 2 BYTES
    GIVEN("1235 data, 2 byte each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_data_2_with_id>, vmp_allocator_t<test_data_2_with_id>, test_data_2_with_id>(1, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_data_2_with_id>, vmp_allocator_t<test_data_2_with_id>, test_data_2_with_id>(2, 200);
    }

    GIVEN("1235 data, 2 byte each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_data_2_with_id>, vmp_allocator_t<test_data_2_with_id>, test_data_2_with_id>(3, 200);
    }

    // 3 BYTES
    GIVEN("1235 data, 3 byte each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_data_3_with_id>, vmp_allocator_t<test_data_3_with_id>, test_data_3_with_id>(1, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_data_3_with_id>, vmp_allocator_t<test_data_3_with_id>, test_data_3_with_id>(2, 200);
    }

    GIVEN("1235 data, 3 byte each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_data_3_with_id>, vmp_allocator_t<test_data_3_with_id>, test_data_3_with_id>(3, 200);
    }

    // 4 BYTES
    GIVEN("1235 data, 4 byte each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_data_4_with_id>, vmp_allocator_t<test_data_4_with_id>, test_data_4_with_id>(1, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_data_4_with_id>, vmp_allocator_t<test_data_4_with_id>, test_data_4_with_id>(2, 200);
    }

    GIVEN("1235 data, 4 byte each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_data_4_with_id>, vmp_allocator_t<test_data_4_with_id>, test_data_4_with_id>(3, 200);
    }

    // 5 BYTES
    GIVEN("1235 data, 5 byte each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_data_5_with_id>, vmp_allocator_t<test_data_5_with_id>, test_data_5_with_id>(1, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_data_5_with_id>, vmp_allocator_t<test_data_5_with_id>, test_data_5_with_id>(2, 200);
    }

    GIVEN("1235 data, 5 byte each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_data_5_with_id>, vmp_allocator_t<test_data_5_with_id>, test_data_5_with_id>(3, 200);
    }

    // DYNAMIC
    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 1")
    {
        add_and_process<vmp_packer_t<test_dynamic_with_id>, vmp_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(1, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 2")
    {
        add_and_process<vmp_packer_t<test_dynamic_with_id>, vmp_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(2, 200);
    }

    GIVEN("1235 data, 1-13 bytes each, with resend_threshold = 3")
    {
        add_and_process<vmp_packer_t<test_dynamic_with_id>, vmp_allocator_t<test_dynamic_with_id>, test_dynamic_with_id>(3, 200);
    }
}

