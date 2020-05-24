cmake_minimum_required(VERSION 3.4)
project(chippuhachi)

enable_testing()
set(CMAKE_CXX_STANDARD 14)

include(build/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(src)
add_subdirectory(tests)

add_executable(chippuhachi src/main.cpp)

target_link_libraries(chippuhachi libchippuhachi)

install(TARGETS chippuhachi DESTINATION bin)