cmake_minimum_required(VERSION 3.22)

# Standalone host-compiled test project. Configure separately from the
# firmware build so the cross-compile toolchain isn't applied:
#
#   cmake -S tests -B build/tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure

project(stm32_playground_tests CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)
# Force shared CRT off / static gtest libs - simplest portable default
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(unit_tests
    # --- test sources (mirror ../src/ tree) ---
    util/ring_buffer_test.cpp
    drivers/can_bus/can_message_test.cpp
    drivers/logger/ilogger_test.cpp

    # --- production sources under test ---
    ${SRC_DIR}/drivers/logger/logger.cpp

    # --- host stubs ---
    support/hal_stub.cpp
)

target_include_directories(unit_tests PRIVATE
    # support/ comes first so the HAL shim shadows any system header search.
    support
    ${SRC_DIR}/util
    ${SRC_DIR}/drivers/can_bus
    ${SRC_DIR}/drivers/logger
)

target_compile_options(unit_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
