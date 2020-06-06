cmake_minimum_required(VERSION 3.10)
project(chippuhachi)

enable_testing()
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.15)

set(CMAKE_MACOSX_RPATH TRUE)
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)

include(build/conanbuildinfo.cmake)
conan_basic_setup()

add_subdirectory(src)
add_subdirectory(tests)

add_executable(chippuhachi src/main.cpp)

find_package(vulkan REQUIRED)

target_link_libraries(chippuhachi libchippuhachi Vulkan::Vulkan)

install(TARGETS chippuhachi DESTINATION bin)

if (APPLE)
    set(LIB_VULKAN ${CMAKE_SOURCE_DIR}/ext/vulkan/macos/lib/libvulkan.dylib)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/ext/vulkan/macos/include)
endif ()