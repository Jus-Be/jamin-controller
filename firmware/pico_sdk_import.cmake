# pico_sdk_import.cmake
# This file is a helper wrapper that locates the Pico SDK.
# It is copied from the pico-sdk repository's external/pico_sdk_import.cmake.
# The GitHub Actions workflow sets PICO_SDK_PATH before running cmake.

if (DEFINED ENV{PICO_SDK_PATH} AND (NOT PICO_SDK_PATH))
    set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})
    message("Using PICO_SDK_PATH from environment ('${PICO_SDK_PATH}')")
endif ()

if (NOT PICO_SDK_PATH)
    message(FATAL_ERROR "PICO_SDK_PATH is not defined. "
            "Set the PICO_SDK_PATH environment variable or pass it on the CMake command line.")
endif ()

set(PICO_SDK_PATH "${PICO_SDK_PATH}" CACHE PATH "Path to the Raspberry Pi Pico SDK")

include("${PICO_SDK_PATH}/external/pico_sdk_import.cmake")
