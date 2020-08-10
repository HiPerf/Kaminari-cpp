add_library(kaminari STATIC 
    buffers/packet.cpp
    buffers/packet_reader.cpp
    client/basic_client.cpp
    protocol/basic_protocol.cpp
    super_packet_reader.cpp)

target_compile_features(kaminari PUBLIC cxx_std_20)
target_include_directories(kaminari PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    target_compile_options(kaminari PUBLIC -stdlib=libc++)
    target_link_options(kaminari INTERFACE -stdlib=libc++ -lc++abi -lc++)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_definitions(kaminari PUBLIC _UNICODE UNICODE)
    target_compile_options(kaminari PUBLIC /MP)
endif()
