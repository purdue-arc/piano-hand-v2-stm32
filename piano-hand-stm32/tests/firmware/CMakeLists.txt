cmake_minimum_required(VERSION 3.16)
project(piano_hand_tests C)
set(CMAKE_C_STANDARD 11)

# Define UNIT_TEST so BSP headers use mocks
add_definitions(-DUNIT_TEST=1)

include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/App/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../firmware/BSP/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/mocks
)

set(FIRMWARE_SOURCES
    ../../firmware/App/Src/protocol.c
    ../../firmware/BSP/Src/stepper.c
)

add_executable(piano_hand_tests
    test_runner.c
    test_protocol.c
    test_stepper.c
    test_checksum.c
    mocks/hal_mock.c
    ${FIRMWARE_SOURCES}
)

# Enable compiler warnings across platforms
if(MSVC)
    target_compile_options(piano_hand_tests PRIVATE /W4)
else()
    target_compile_options(piano_hand_tests PRIVATE -Wall -Wextra -Wpedantic)
endif()
