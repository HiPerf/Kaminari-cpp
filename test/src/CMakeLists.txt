add_executable(kaminari_test 
    test_queue_write_size.cpp)

target_link_libraries(kaminari_test PRIVATE kaminari)
target_compile_features(kaminari_test PRIVATE cxx_std_20)
target_include_directories(kaminari_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

