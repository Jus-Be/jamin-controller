cmake_minimum_required(VERSION 3.13)

# Pull in Pico SDK (must happen before project())
include(pico_sdk_import.cmake)

project(jamin_controller C CXX ASM)
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

# Enable USB and UART stdio (UART stdio for debug, USB disabled to free resources)
pico_enable_stdio_usb(jamin_controller 1)
pico_enable_stdio_uart(jamin_controller 0)

# Create map/bin/hex/uf2 output in addition to ELF
pico_add_extra_outputs(jamin_controller)
