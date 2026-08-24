cmake_minimum_required(VERSION 3.12)

# Set Pico Board and Pico Platform (Correct for Raspberry Pi Pico 2 W)
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_BOARD pico2_w)

# REMOVED: set(FAMILY rp2040) <- This was conflicting with the RP2350 settings above!

# Pull in Pico SDK (must happen before project())
include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)
string(TIMESTAMP DATEVER "%Y.%m.%d" UTC)
project(jamin_controller VERSION ${DATEVER} LANGUAGES C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(jamin_controller
    src/main.c
    src/bt_hid.c
    src/midi.c
    src/wav_index.c
    src/keyboard.c
)

target_include_directories(jamin_controller PRIVATE src)

# Link your libraries (Ensure pico_stdlib is at the top)
target_link_libraries(jamin_controller
    pico_stdlib
    pico_btstack_classic
    pico_btstack_ble
    pico_btstack_cyw43
    pico_cyw43_arch_none
    hardware_uart
    hardware_gpio
    hardware_flash
    hardware_sync
)

# Enable USB and disable UART stdio
pico_enable_stdio_usb(jamin_controller 1)
pico_enable_stdio_uart(jamin_controller 0)

# Create map/bin/hex/uf2 output in addition to ELF
pico_add_extra_outputs(jamin_controller)

# CMake flags
set(CMAKE_C_FLAGS  "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -ffunction-sections -fdata-sections")
