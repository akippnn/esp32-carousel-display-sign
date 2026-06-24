cmake_minimum_required(VERSION 3.10)
project(u8g2_emulator C CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find SDL2
find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

# U8g2 source directories (project root is parent)
set(U8G2_CSRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../u8g2/csrc")
set(U8G2_SYSSDL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../u8g2/sys/sdl")
set(U8G2_SYSSDL_COMMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../u8g2/sys/sdl/common")

# Collect all U8g2 C source files
file(GLOB U8G2_C_SOURCES
    "${U8G2_CSRC_DIR}/*.c"
    "${U8G2_SYSSDL_COMMON_DIR}/*.c"
)

# Create executable
add_executable(emulator src/emulator.cpp ${U8G2_C_SOURCES})

# Include directories
target_include_directories(emulator PRIVATE
    ${U8G2_CSRC_DIR}
    ${U8G2_SYSSDL_DIR}
)

# Link SDL2
target_link_libraries(emulator ${SDL2_LIBRARIES})

# Platform-specific settings
if(APPLE)
    # macOS-specific settings if needed
    target_link_libraries(emulator "-framework Cocoa")
elseif(UNIX)
    # Linux-specific settings
    # Nothing special needed for Linux typically
elseif(WIN32)
    # Windows-specific settings
    # May need additional libraries
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(emulator PRIVATE /W4)
else()
    target_compile_options(emulator PRIVATE -Wall -Wextra -pedantic)
endif()
