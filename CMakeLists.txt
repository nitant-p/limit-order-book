cmake_minimum_required(VERSION 3.16)
project(order_book VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
option(ENABLE_COVERAGE "Enable gcov coverage instrumentation" OFF)
option(ENABLE_PHASE2_ORDER_BOOK_SIDE_TESTS "Enable Phase 2 OrderBookSide API contract tests" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
        add_compile_options(-O0 -g --coverage)
        add_link_options(--coverage)
    else()
        message(WARNING "Coverage is only configured for GNU/Clang/AppleClang compilers.")
    endif()
endif()

add_library(order_book_lib
        src/MatchingEngine.cpp
        src/Order.cpp
        src/Trade.cpp
        src/OrderBookSide.cpp
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

    include(GoogleTest)
    add_executable(matching_engine_tests
        tests/BestMatchingBySideTest.cpp
        tests/OrderTypeBehaviorTest.cpp
        tests/CancelOrderTest.cpp
        tests/ModifyOrderTest.cpp
        tests/OrderIdBehaviorTest.cpp
    )

    target_link_libraries(matching_engine_tests
        PRIVATE
            order_book_lib
            GTest::gtest_main
    )

    gtest_discover_tests(matching_engine_tests)

    if(ENABLE_PHASE2_ORDER_BOOK_SIDE_TESTS)
        add_executable(order_book_side_tests
            tests/OrderBookSideTest.cpp
        )

        target_link_libraries(order_book_side_tests
            PRIVATE
                order_book_lib
                GTest::gtest_main
        )

        gtest_discover_tests(order_book_side_tests)
    endif()

    if(ENABLE_COVERAGE)
        add_custom_target(coverage
            COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
            COMMAND ${CMAKE_SOURCE_DIR}/scripts/generate_coverage.sh ${CMAKE_BINARY_DIR} ${CMAKE_SOURCE_DIR}
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
            DEPENDS matching_engine_tests
            COMMENT "Run tests and generate gcov coverage artifacts under build/coverage"
        )
    endif()
endif()
