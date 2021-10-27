set(KAMINARI_SOURCES
    ../include/kaminari/buffers/packet.hpp
    ../include/kaminari/buffers/packet_reader.hpp
    buffers/packet.cpp
    buffers/packet_reader.cpp
    ../include/kaminari/client/basic_client.hpp
    ../include/kaminari/client/client.hpp
    client/basic_client.cpp
    ../include/kaminari/cx/maths.hpp
    ../include/kaminari/cx/overflow.hpp
    ../include/kaminari/detail/detail.hpp
    ../include/kaminari/packers/immediate_packer.hpp
    ../include/kaminari/packers/merge_packer.hpp
    ../include/kaminari/packers/most_recent_packer_by_opcode.hpp
    ../include/kaminari/packers/most_recent_packer_with_id.hpp
    ../include/kaminari/packers/ordered_packer.hpp
    ../include/kaminari/packers/packer.hpp
    ../include/kaminari/packers/unique_merge_packer.hpp
    ../include/kaminari/packers/vector_merge_packer.hpp
    ../include/kaminari/packers/vector_merge_with_priority_packer.hpp
    ../include/kaminari/protocol/basic_protocol.hpp
    ../include/kaminari/protocol/protocol.hpp
    protocol/basic_protocol.cpp
    ../include/kaminari/queues/eventually_synced_queue.hpp
    ../include/kaminari/queues/reliable_queue.hpp
    ../include/kaminari/queues/unreliable_queue.hpp
    ../include/kaminari/types/data_wrapper.hpp
    ../include/kaminari/broadcaster.hpp
    ../include/kaminari/super_packet.hpp
    ../include/kaminari/super_packet_reader.hpp
    super_packet_reader.cpp
)

if (NOT KAMINARI_DISABLE_ADDONS)
    set(KAMINARI_SOURCES
        ${KAMINARI_SOURCES}
        ../include/kaminari/addons/phase_sync.hpp
        ../include/kaminari/addons/peer_based_phase_sync.hpp
        addons/peer_based_phase_sync.cpp
    )
endif()

add_library(kaminari STATIC ${KAMINARI_SOURCES})

target_compile_features(kaminari PUBLIC cxx_std_20)
target_include_directories(kaminari PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kaminari PUBLIC -stdlib=libc++)
    target_link_options(kaminari INTERFACE -stdlib=libc++ -lc++abi -lc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(kaminari PUBLIC _UNICODE UNICODE)
    target_compile_options(kaminari PUBLIC /MP)
endif()
