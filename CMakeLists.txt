cmake_minimum_required(VERSION 3.22)
include(pico_sdk_import.cmake)
include(pico_extras_import.cmake)

set(ENV{PICO_BOARD} pico_w)

project(marcai VERSION 0.1.0)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
pico_sdk_init()

add_executable(${PROJECT_NAME}
  marcai.cpp
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_multicore
)
