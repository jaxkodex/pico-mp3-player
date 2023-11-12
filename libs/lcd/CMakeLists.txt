add_library(videolib INTERFACE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Config)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/GUI)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Fonts)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Infrared)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/LCD)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Icm20948)

target_link_libraries(videolib INTERFACE
        Config
        Fonts
        GUI
        Icm20948
        Infrared
        LCD
        )