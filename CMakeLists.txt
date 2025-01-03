cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

project(control-dualsense-mod)

# Set the DLL output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(DSXPP_SRC "${CMAKE_CURRENT_LIST_DIR}/src/DSXpp/src")

if (NOT EXISTS "${DSXPP_SRC}/UDPClient.cpp" OR
    NOT EXISTS "${DSXPP_SRC}/DSX++.cpp"
)
    message(FATAL_ERROR,
            "Error: DSX++ source code not found"
            "make sure to init submodules first through:"
            "git submodule update --init --recursive"
    )
endif()


add_definitions(-DDSX_V3)
include_directories(${DSXPP_SRC})

# Add the source files, including dllmain.cpp
add_library(dualsense-mod SHARED
    src/dllmain.cpp
    src/Logger.cpp
    src/Utils.cpp
    src/Config.cpp
    src/DualsenseMod.cpp
    src/minhook/src/buffer.c
    src/minhook/src/hook.c
    src/minhook/src/trampoline.c
    src/minhook/src/hde/hde32.c
    src/minhook/src/hde/hde64.c
    src/rva/sscan/Pattern.cpp
)

add_library(DSX++ STATIC
    ${DSXPP_SRC}/UDPClient.cpp
    ${DSXPP_SRC}/DSX++.cpp
)

target_link_libraries(dualsense-mod PRIVATE DSX++)
