cmake_minimum_required(VERSION 3.16)
project(order_book VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(order_book_lib
    src/MatchingEngine.cpp
    src/Order.cpp
        src/Trade.cpp
)

target_include_directories(order_book_lib
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(order_book
    src/main.cpp
)

target_link_libraries(order_book
    PRIVATE
        order_book_lib
)

include(CTest)

if(BUILD_TESTING)
    include(FetchContent)

    FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

#    add_executable(matching_engine_tests
#        tests/MatchingEngineTest.cpp
#    )
#
#    target_link_libraries(matching_engine_tests
#        PRIVATE
#            order_book_lib
#            GTest::gtest_main
#    )

    include(GoogleTest)
#    gtest_discover_tests(matching_engine_tests)

    add_executable(matching_engine_v5_tests
        tests/MatchingEngineV5Test.cpp
    )

    target_link_libraries(matching_engine_v5_tests
        PRIVATE
            order_book_lib
            GTest::gtest_main
    )

    gtest_discover_tests(matching_engine_v5_tests)
endif()
