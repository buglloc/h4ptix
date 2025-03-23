cmake_minimum_required(VERSION 3.20.0)
# list(APPEND BOARD_ROOT ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(h4ptix)
project(app LANGUAGES C)

target_sources(app
PRIVATE
  src/main.cxx
  src/trigger.cxx
  src/ui.cxx
  src/ui/led.cxx

)

target_sources_ifdef(
  CONFIG_DISPLAY
  app PRIVATE src/ui/display.cxx
)

include(${ZEPHYR_BASE}/cmake/cfb.cmake NO_POLICY_SCOPE)
set(gen_dir ${ZEPHYR_BINARY_DIR}/include/generated/)
generate_cfb_font_for_target(app fonts/PressStart2P-Regular.ttf ${gen_dir}/cfb_ui.h 12 16 --type font --size 12)
