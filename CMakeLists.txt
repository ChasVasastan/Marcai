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
  audio.cpp
  wifi.cpp
  http_client.cpp
)

add_subdirectory(lib/picomp3lib/src)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

pico_add_extra_outputs(${PROJECT_NAME})

target_include_directories(${PROJECT_NAME} PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_LIST_DIR}/include
  ${CMAKE_CURRENT_LIST_DIR}/lib/picomp3lib/src
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  pico_stdlib
  pico_multicore
  pico_audio_i2s
  picomp3lib
  pico_cyw43_arch_lwip_threadsafe_background
  pico_lwip_http
  pico_lwip_mbedtls
  pico_mbedtls
)

add_custom_target(${PROJECT_NAME}_flash
  COMMAND picotool load --force ${CMAKE_BINARY_DIR}/marcai.elf
  DEPENDS ${PROJECT_NAME}
  COMMENT "Flashing board..."
)

if (EXISTS ${CMAKE_CURRENT_LIST_DIR}/credentials.cmake)
  include(credentials.cmake)
else()
  if (DEFINED ENV{WIFI_SSID})
    target_compile_definitions(${PROJECT_NAME}
      PRIVATE WIFI_SSID=\"$ENV{WIFI_SSID}\"
    )
  else()
    message(FATAL_ERROR "WIFI_SSID not found")
  endif()

  if (DEFINED ENV{WIFI_PASS})
    target_compile_definitions(${PROJECT_NAME}
      PRIVATE WIFI_PASS=\"$ENV{WIFI_PASS}\"
    )
  else()
    message(FATAL_ERROR "WIFI_PASS not found")
  endif()
endif()
