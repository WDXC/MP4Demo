# 设置最低CMake版本
cmake_minimum_required(VERSION 3.10)

# 设置项目名称和版�?
project(MP4Demo VERSION 1.0)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14) # �?以修改为C++11, C++17�?
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Source directory: ${CMAKE_SOURCE_DIR} ")

include_directories(
    ${CMAKE_SOURCE_DIR}/10-include
    ${CMAKE_SOURCE_DIR}/30-thirdparty/SDL2/include
    ${CMAKE_SOURCE_DIR}/30-thirdparty/FFmpeg/include
)

link_directories(
    ${CMAKE_SOURCE_DIR}/30-thirdparty/SDL2/lib
    ${CMAKE_SOURCE_DIR}/30-thirdparty/FFmpeg/lib
)


set(SOURCES
    ${CMAKE_SOURCE_DIR}/20-source/integrate.cpp
)


add_executable(${PROJECT_NAME} ${SOURCES})

target_link_libraries(
    ${PROJECT_NAME}
    avformat
    avcodec
    avdevice
    avfilter
    avutil
    swscale
    SDL2
    SDL2main
    lzma
)

