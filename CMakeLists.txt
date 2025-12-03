cmake_minimum_required(VERSION 3.10)
project(png2jpg VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -pedantic)
endif()

# Release build by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Main executable
add_executable(png2jpg
    src/main.cpp
    src/png_decoder.cpp
    src/jpeg_encoder.cpp
    src/deflate.cpp
    src/image.cpp
)

target_include_directories(png2jpg PRIVATE ${CMAKE_SOURCE_DIR}/include)

# Installation
install(TARGETS png2jpg DESTINATION bin)