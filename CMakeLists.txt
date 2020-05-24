cmake_minimum_required(VERSION 3.4)
project(chippuhachi)

add_definitions("-std=c++11")

include(build/conanbuildinfo.cmake)
conan_basic_setup()

enable_testing()
set(CMAKE_CXX_STANDARD 14)

add_subdirectory(src)
add_subdirectory(tests)

add_executable(chippuhachi main.cpp)