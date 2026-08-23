cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED ENV{PICO_SDK_PATH})
    message(FATAL_ERROR "PICO_SDK_PATH is not set. Point it to your pico-sdk checkout.")
endif()

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(jamin_controller C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(jamin_controller
    src/main.c
)

target_link_libraries(jamin_controller
    pico_stdlib
    hardware_gpio
    hardware_sync
)

pico_enable_stdio_usb(jamin_controller 1)
pico_enable_stdio_uart(jamin_controller 0)

pico_add_extra_outputs(jamin_controller)
