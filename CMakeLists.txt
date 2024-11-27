cmake_minimum_required(VERSION 3.10)

project(control-dualsense-mod)

# Set the DLL output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Add the source files, including dllmain.cpp
add_library(dualsense-mod SHARED
    src/dllmain.cpp
    src/Logger.cpp
    src/Utils.cpp
    src/DualsenseMod.cpp
    src/minhook/src/buffer.c
    src/minhook/src/hook.c
    src/minhook/src/trampoline.c
    src/minhook/src/hde/hde32.c
    src/minhook/src/hde/hde64.c
    src/rva/sscan/Pattern.cpp
)
