include(FetchContent)


find_package(Boost ${BOOST_VERSION} REQUIRED)
target_include_directories(kaminari PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(kaminari PUBLIC ${Boost_LIBRARY_DIRS})

