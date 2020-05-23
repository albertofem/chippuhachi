cmake_minimum_required(VERSION 2.8.12)
project(chippuhachi)

add_definitions("-std=c++11")

include(build/conanbuildinfo.cmake)
conan_basic_setup()

add_executable(chippuhachi src/main.cpp)