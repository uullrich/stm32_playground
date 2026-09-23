cmake_minimum_required(VERSION 3.22)

# Standalone host-compiled test project. Configure separately from the
# firmware build so the cross-compile toolchain isn't applied:
#
#   cmake -S tests -B build/tests
#   cmake --build build/tests
#   ctest --test-dir build/tests --output-on-failure

project(stm32_playground_tests C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(deps)

enable_testing()

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../src)

add_executable(unit_tests
    # --- test sources (mirror ../src/ tree) ---
    util/RingBufferTest.cpp
    drivers/can_bus/CanMessageTest.cpp
    drivers/logger/ILoggerTest.cpp
    devices/distance_sensor/Vl53l1xTest.cpp
    protocols/custom_can/CustomCanTest.cpp
    io/CanDispatcherTest.cpp

    # --- production sources under test ---
    ${SRC_DIR}/drivers/logger/ILogger.cpp
    ${SRC_DIR}/devices/distance_sensor/IDistanceSensor.cpp
    ${SRC_DIR}/devices/distance_sensor/Vl53l1x.cpp
    ${SRC_DIR}/devices/distance_sensor/Vl53l1xPlatform.cpp
    ${SRC_DIR}/protocols/custom_can/CustomCan.cpp
    ${SRC_DIR}/io/CanDispatcher.cpp
    ${SRC_DIR}/io/IoRepository.cpp
    ${SRC_DIR}/io/VirtualDigitalOutput.cpp
    ${SRC_DIR}/io/VirtualDigitalInput.cpp
    ${SRC_DIR}/io/VirtualPwmOutput.cpp
    ${SRC_DIR}/io/VirtualAdcInput.cpp
    ${SRC_DIR}/../third_party/vl53l1x_uld/VL53L1X_api.c

    # --- host stubs ---
    support/HalStub.cpp
    support/SimulatedVl53l1xBus.cpp
)

target_include_directories(unit_tests PRIVATE
    # support/ comes first so the HAL shim shadows any system header search.
    support
    ${SRC_DIR}/util
    ${SRC_DIR}/drivers/can_bus
    ${SRC_DIR}/drivers/logger
    ${SRC_DIR}/drivers/i2c_bus
    ${SRC_DIR}/devices/distance_sensor
    ${SRC_DIR}/protocols/custom_can
    ${SRC_DIR}/io
    ${SRC_DIR}/drivers/digital_output
    ${SRC_DIR}/drivers/digital_input
    ${SRC_DIR}/drivers/pwm_output
    ${SRC_DIR}/drivers/adc_input
    ${SRC_DIR}/../third_party/vl53l1x_uld
)

target_compile_options(unit_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

target_link_libraries(unit_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
)

include(GoogleTest)
gtest_discover_tests(unit_tests)
