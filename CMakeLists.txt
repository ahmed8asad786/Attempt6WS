cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(Attempt6WS)

file(GLOB FONT_SOURCES fonts/*.c)

target_sources(app PRIVATE
    src/main.c
    src/fonts5x7.c
    src/fonts_8x12_montserrat.c
    src/my_image.c
    ${FONT_SOURCES}
)

target_include_directories(app PRIVATE
  include
)
