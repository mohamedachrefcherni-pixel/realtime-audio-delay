cmake_minimum_required(VERSION 4.0)
project(audio_delay)

set(CMAKE_CXX_STANDARD 20)

set(ASIO_SDK_DIR "" CACHE PATH "Path to the Steinberg ASIO SDK")
if(NOT ASIO_SDK_DIR)
    message(FATAL_ERROR "Set ASIO_SDK_DIR, e.g. -DASIO_SDK_DIR=C:/SDKs/ASIOSDK")
endif()

set(PA_USE_ASIO ON CACHE BOOL "" FORCE)
set(ASIODIR ${ASIO_SDK_DIR} CACHE PATH "" FORCE)

set(PA_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(portaudio)

add_executable(audio_delay main.cpp)
target_link_libraries(audio_delay PRIVATE portaudio)
target_include_directories(audio_delay PRIVATE portaudio/include)