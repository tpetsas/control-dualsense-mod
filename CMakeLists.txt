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

set(DUALSENSECPP_ROOT "${CMAKE_CURRENT_LIST_DIR}/src/dualsense-cpp")

if (NOT EXISTS "${DUALSENSECPP_ROOT}/src/dualsense.cpp")
    message(FATAL_ERROR,
            " Error: dualsense-cpp source code not found; "
            "make sure to init submodules first through: "
            "git submodule update --init --recursive"
    )
    message("${CMAKE_CURRENT_LIST_DIR}/src/dualsence-cpp/src/dualsense.cpp")
endif()
include_directories(${DUALSENSECPP_ROOT})

file(GLOB_RECURSE DUALSENSECPP_SOURCE_FILES
    ${DUALSENSECPP_ROOT}/src/*.cpp
    ${DUALSENSECPP_ROOT}/src/*.h
)

# Create the static lib
add_library(dualsense-cpp STATIC ${DUALSENSECPP_SOURCE_FILES})

target_include_directories(dualsense-cpp PUBLIC
    ${DUALSENSECPP_ROOT}/src
    ${DUALSENSECPP_ROOT}/include
)

# to avoid dllimport conflicts
target_compile_definitions(dualsense-cpp PUBLIC DS5W_BUILD_LIB)

# link necessary Windows libs
target_link_libraries(dualsense-cpp
    setupapi
    hid
    cfgmgr32
)

add_definitions(-DDSX_V3)
include_directories(${DSXPP_SRC})
include_directories(${DUALSENSECPP_ROOT})


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

target_link_libraries(dualsense-mod PRIVATE DSX++ dualsense-cpp)
