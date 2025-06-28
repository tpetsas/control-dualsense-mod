cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 20)

project(control-dualsense-mod)

# Set the DLL output directory
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

set(DUALSENSITIVE_ROOT "${CMAKE_CURRENT_LIST_DIR}/src/dualsensitive")

if (NOT EXISTS "${DUALSENSITIVE_ROOT}/src/dualsensitive.cpp")
    message(FATAL_ERROR,
            " Error: dualsensitive source code not found; "
            "make sure to init submodules first through: "
            "git submodule update --init --recursive"
    )
    message("${CMAKE_CURRENT_LIST_DIR}/src/dualsence-cpp/src/dualsensitive.cpp")
endif()
include_directories(${DUALSENSITIVE_ROOT})

file(GLOB_RECURSE DUALSENSITIVE_SOURCE_FILES
    ${DUALSENSITIVE_ROOT}/src/core/*.cpp
    ${DUALSENSITIVE_ROOT}/src/core/udp/*.cpp
    ${DUALSENSITIVE_ROOT}/src/dualsensitive.cpp
)

# Create the static lib
add_library(dualsensitive STATIC ${DUALSENSITIVE_SOURCE_FILES})

target_include_directories(dualsensitive PUBLIC
    ${DUALSENSITIVE_ROOT}/src/core
    ${DUALSENSITIVE_ROOT}/src/core/udp
    ${DUALSENSITIVE_ROOT}/include
)

# to avoid dllimport conflicts
target_compile_definitions(dualsensitive PUBLIC DS5W_BUILD_LIB)

# link necessary Windows libs
target_link_libraries(dualsensitive
    setupapi
    hid
    cfgmgr32
)

include_directories(${DUALSENSITIVE_ROOT})


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


target_link_libraries(dualsense-mod PRIVATE dualsensitive)

add_executable(dualsensitive-service ${DUALSENSITIVE_ROOT}/src/service/main.cpp)

# Mark as Windows GUI app to suppress console window
set_target_properties(dualsensitive-service PROPERTIES
    WIN32_EXECUTABLE TRUE
)

target_include_directories(dualsensitive-service PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(dualsensitive-service PRIVATE dualsensitive)

