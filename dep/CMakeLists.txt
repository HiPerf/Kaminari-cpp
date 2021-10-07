include(FetchContent)


find_package(Boost ${BOOST_VERSION} REQUIRED)
target_include_directories(kaminari PUBLIC ${Boost_INCLUDE_DIRS})
target_link_directories(kaminari PUBLIC ${Boost_LIBRARY_DIRS})


IF (NOT KAMINARI_DISABLE_ADDONS)
    FetchContent_Declare(
        function
        GIT_REPOSITORY https://github.com/Naios/function2
        GIT_TAG        master
    )

    FetchContent_GetProperties(function)
    if(NOT function_POPULATED)
        FetchContent_Populate(function)
        add_subdirectory(${function_SOURCE_DIR} ${function_BINARY_DIR})
    endif()

    target_link_libraries_system(kaminari PUBLIC function2)
endif()
