cmake_minimum_required( VERSION 3.21 )
set( CMAKE_CXX_STANDARD 17 )
project( superbub VERSION 0.3 )

add_executable( superbub src/main.cpp src/NY_Debug_Prefix.h src/qsincos.c src/qsincos.h src/engine.h )
