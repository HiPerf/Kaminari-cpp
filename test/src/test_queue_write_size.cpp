#include "kaminari/super_packet.hpp"
#include <catch2/catch_all.hpp>

#include <kaminari/packers/immediate_packer.hpp>
#include <kaminari/packers/ordered_packer.hpp>
#include <kaminari/packers/vector_merge_packer.hpp>
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

    static auto get() { return test_data_1_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX  }; }
};
PACK(struct _test_data_1_with_id_packed { uint64_t id; uint8_t x; });

struct test_data_2_with_id
{
    uint64_t id;
    uint16_t x;
    float priority;

    static auto get() { return test_data_2_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX  }; }
};
PACK(struct _test_data_2_with_id_packed { uint64_t id; uint16_t x; });

struct test_data_3_with_id
{
    uint64_t id;
    uint8_t x;
    uint16_t y;
    float priority;

    static auto get() { return test_data_3_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .y = 0, .priority = rand() / (float)RAND_MAX  }; }
};
PACK(struct _test_data_3_with_id_packed { uint64_t id; uint8_t x; uint16_t y; });

struct test_data_4_with_id
{
    uint64_t id;
    uint32_t x;
    float priority;

    static auto get() { return test_data_4_with_id { .id = (uint64_t)(rand() % 516), .x = 0, .priority = rand() / (float)RAND_MAX }; }
};
PACK(struct _test_data_4_with_id_packed { uint64_t id; uint32_t x; });

struct test_data_5_with_id
{
    uint64_t id;
    uint8_t x;
    uint16_t y;
    uint32_t z;
    float priority;

    static auto get() noexcept
    {
        return test_data_5_with_id {
            .id = (uint64_t)(rand() % 516),
            .x = 0,
            .y = 0,
            .z = 0,
            .priority = rand() / (float)RAND_MAX
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
        "0123456789"
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

    static auto get() noexcept
    {
        return test_dynamic_with_id {
            .id = (uint64_t)(rand() % 516),
            .x = gen_random((rand() % 12) + 1),
            .priority = rand() / (float)RAND_MAX
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

uint16_t get_total_size(const kaminari::detail::packets_by_block& by_block)
{
    uint16_t total_size = 0;
    for (const auto& [block, data] : by_block)
    {
        for (const auto& packet : data)
        {
            total_size += packet->size();
        }
    }
    return total_size;
}

SCENARIO("an immediate_packer processes many accumulated data")
{
    using allocator_t = std::allocator<kaminari::immediate_packer_allocator_t>;
    kaminari::immediate_packer<test_marshal, allocator_t> ip(2, allocator_t());

    GIVEN("1000 data, 1 byte each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_1::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 2 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_2::get());
        }

        WHEN("it processes")
        {
            while (true)
            {
                uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
                uint16_t max_data_size = remaining;
                bool unfitting_data = true;
                kaminari::detail::packets_by_block by_block;
                ip.process(0, 0, remaining, unfitting_data, by_block);

                if (by_block.empty())
                {
                    break;
                }
                // printf("PROCESSED, remaining: %u / %u, total: %lld\n", remaining, max_data_size, by_block.size());

                THEN("remaining is whithin constraints")
                {
                    REQUIRE(remaining >= 0);
                    REQUIRE(remaining < max_data_size);
                }

                THEN("size is whithin constraints")
                {
                    REQUIRE(get_total_size(by_block) < max_data_size);
                }

                THEN("not all data fit")
                {
                    REQUIRE(unfitting_data);
                }
            }
        }
    }

    GIVEN("1000 data, 3 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_3::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 4 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_4::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 5 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_5::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 1-13 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_dynamic::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }
}


SCENARIO("a ordered_packer processes many accumulated data")
{
    using allocator_t = std::allocator<kaminari::ordered_packer_allocator_t>;
    kaminari::ordered_packer<test_marshal, allocator_t> ip(2, allocator_t());

    GIVEN("1000 data, 1 byte each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_1::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 2 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_2::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 3 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_3::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 4 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_4::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 5 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_5::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 1-13 bytes each")
    {
        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_dynamic::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }
}


SCENARIO("a vector_merge_packer processes many accumulated data")
{
    GIVEN("1000 data, 1 byte each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_data_1_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_data_1_with_id>, test_data_1_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_1_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 2 bytes each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_data_2_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_data_2_with_id>, test_data_2_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_2_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 3 bytes each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_data_3_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_data_3_with_id>, test_data_3_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_3_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 4 bytes each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_data_4_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_data_4_with_id>, test_data_4_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_4_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 5 bytes each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_data_5_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_data_5_with_id>, test_data_5_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_data_5_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }

    GIVEN("1000 data, 1-13 bytes each")
    {
        using allocator_t = std::allocator<kaminari::vector_merge_packer_allocator_t<test_dynamic_with_id>>;
        kaminari::vector_merge_packer<uint64_t, test_ev_sync<test_dynamic_with_id>, test_dynamic_with_id, 1010, test_marshal, allocator_t> ip(2, allocator_t());

        for (int i = 0; i < 1000; ++i)
        {
            ip.add(1010, test_dynamic_with_id::get());
        }

        WHEN("it processes")
        {
            uint16_t remaining = kaminari::super_packet_max_size - kaminari::super_packet_header_size - kaminari::super_packet_ack_size - 2;
            uint16_t max_data_size = remaining;
            bool unfitting_data = false;
            kaminari::detail::packets_by_block by_block;

            ip.process(0, 0, remaining, unfitting_data, by_block);

            THEN("remaining is whithin constraints")
            {
                REQUIRE(remaining >= 0);
                REQUIRE(remaining < max_data_size);
            }

            THEN("size is whithin constraints")
            {
                REQUIRE(get_total_size(by_block) < max_data_size);
            }

            THEN("not all data fit")
            {
                REQUIRE(unfitting_data);
            }
        }
    }
}

