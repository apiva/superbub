cmake_minimum_required( VERSION 3.21 )
set( CMAKE_CXX_STANDARD 17 )
project( superbub VERSION 0.1 )

add_subdirectory( ext/raylib EXCLUDE_FROM_ALL )
add_subdirectory( ext/spdlog EXCLUDE_FROM_ALL )

add_executable( superbub src/main.cpp src/qsincos.cpp src/qsincos.h src/engine.cpp src/engine.h )
target_link_libraries( superbub PUBLIC raylib spdlog )
