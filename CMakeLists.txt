cmake_minimum_required( VERSION 3.21 )
set( CMAKE_CXX_STANDARD 17 )
project( superbub VERSION 0.1 )

add_subdirectory( ext/raylib EXCLUDE_FROM_ALL )
add_subdirectory( ext/spdlog EXCLUDE_FROM_ALL )
add_subdirectory( ext/gflags EXCLUDE_FROM_ALL )

add_executable( superbub src/superbub/main.cpp src/superbub/qsincos.cpp src/superbub/qsincos.h src/superbub/engine.cpp src/superbub/engine.h )
target_link_libraries( superbub PUBLIC raylib spdlog )

add_executable( tim2png src/tim2png/tim2png.cpp )
target_link_libraries( tim2png PUBLIC raylib spdlog gflags::gflags )
