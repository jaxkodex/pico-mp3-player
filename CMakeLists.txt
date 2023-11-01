cmake_minimum_required(VERSION 3.13)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(sd_reader)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# rest of your project
add_executable(mp3player
    src/main.c
    src/drivers/sdmm/diskio.c
    src/drivers/sdmm/sdmm.c
)

# Add fatfs lib
add_library(fatfs INTERFACE)
target_sources(fatfs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libs/fatfs/ff.c
    )
target_include_directories(fatfs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libs/fatfs
    )

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(mp3player pico_stdlib hardware_spi fatfs)

target_include_directories(mp3player PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# enable usb output, disable uart output
pico_enable_stdio_usb(mp3player 1)
pico_enable_stdio_uart(mp3player 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(mp3player)