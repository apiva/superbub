cmake_minimum_required( VERSION 3.21 )
set( CMAKE_CXX_STANDARD 17 )
project( superbub VERSION 0.1 )

add_subdirectory( ext/raylib EXCLUDE_FROM_ALL )
add_subdirectory( ext/spdlog EXCLUDE_FROM_ALL )
add_subdirectory( ext/gflags EXCLUDE_FROM_ALL )

add_library( libtim src/libtim/tim.hpp src/libtim/tim.cpp )
target_include_directories( libtim PUBLIC src/libtim )
target_link_libraries( libtim PUBLIC raylib spdlog )

add_executable( superbub src/superbub/main.cpp src/superbub/qsincos.cpp src/superbub/qsincos.h src/superbub/engine.h )
target_link_libraries( superbub PUBLIC raylib spdlog libtim gflags::gflags )

