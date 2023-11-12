add_library(ui INTERFACE)

target_sources(ui INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}/ui.c
  ${CMAKE_CURRENT_SOURCE_DIR}/screensaver.c
)

target_include_directories(ui INTERFACE
  ${CMAKE_CURRENT_SOURCE_DIR}
)