cmake_minimum_required(VERSION 3.13)

set(BTSTACK_3RD_PARTY_PATH $ENV{BTSTACK_ROOT}/3rd-party)
set(MP3LIB_PATH $ENV{MP3LIB_PATH})

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(mp3player)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_subdirectory(src/modules/logger)
add_subdirectory(src/modules/ui)

# rest of your project
add_executable(mp3player
    src/main.c
    src/drivers/sdmm/diskio.c
    src/drivers/sdmm/sdmm.c
    src/drivers/sdmm/f_util.c
    src/drivers/bt/blue.c
    src/drivers/bt/blue_utils.c
    src/player.c
    src/events.c
    src/drivers/video/video.c
    src/drivers/video/button.c
)

# Add minimp3 library
add_library(mp3lib INTERFACE)
target_sources(mp3lib INTERFACE
    ${MP3LIB_PATH}/interface/music_file.c
    ${MP3LIB_PATH}/src/bitstream.c
    ${MP3LIB_PATH}/src/buffers.c
    ${MP3LIB_PATH}/src/dct32.c
    ${MP3LIB_PATH}/src/dequant.c
    ${MP3LIB_PATH}/src/dqchan.c
    ${MP3LIB_PATH}/src/huffman.c
    ${MP3LIB_PATH}/src/hufftabs.c
    ${MP3LIB_PATH}/src/imdct.c
    ${MP3LIB_PATH}/src/mp3dec.c
    ${MP3LIB_PATH}/src/mp3tabs.c
    ${MP3LIB_PATH}/src/polyphase.c
    ${MP3LIB_PATH}/src/scalfact.c
    ${MP3LIB_PATH}/src/stproc.c
    ${MP3LIB_PATH}/src/subband.c
    ${MP3LIB_PATH}/src/trigtabs.c
    )
target_include_directories(mp3lib INTERFACE
    ${MP3LIB_PATH}/interface
    ${MP3LIB_PATH}/src
    )

# Add BTstack library
add_library(btstack_hxcmod_player INTERFACE)
target_sources(btstack_hxcmod_player INTERFACE
    ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/hxcmod.c
    ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/mods/nao-deceased_by_disease.c
    )
target_include_directories(btstack_hxcmod_player INTERFACE 
    ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player
    ${BTSTACK_3RD_PARTY_PATH}/hxcmod-player/mods
    )

# Add fatfs lib
add_library(fatfs INTERFACE)
target_sources(fatfs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libs/fatfs/ff.c
    )
target_include_directories(fatfs INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/libs/fatfs
    )

# Add video lib
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/lcd)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(mp3player 
    pico_stdlib hardware_spi fatfs
    # pico_cyw43_arch_none
    pico_cyw43_arch_lwip_threadsafe_background
    pico_btstack_classic pico_btstack_ble 
    pico_btstack_cyw43 pico_btstack_sbc_encoder pico_btstack_sbc_decoder 
    # btstack_hxcmod_player
    mp3lib
    logger ui
    videolib
    )

target_include_directories(mp3player PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

# enable usb output, disable uart output
pico_enable_stdio_usb(mp3player 1)
pico_enable_stdio_uart(mp3player 0)

# create map/bin/hex/uf2 file in addition to ELF.
pico_add_extra_outputs(mp3player)