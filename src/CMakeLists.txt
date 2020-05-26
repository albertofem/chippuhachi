set(TARGET_NAME libchippuhachi)

add_library(${TARGET_NAME} chippuhachi.cpp chippuhachi.h cpu.cpp cpu.h mem.cpp mem.h gpu.cpp gpu.h
        backend/videobackend.h backend/glfwvulkan.cpp backend/glfwvulkan.h backend/imgui_impl_vulkan.cpp
        backend/imgui_impl_vulkan.h backend/imgui_impl_glfw.h backend/imgui_impl_glfw.cpp
        emulator.h emulator.cpp system.h)

target_include_directories(${TARGET_NAME} INTERFACE ./)

find_package(vulkan REQUIRED)
set(CMAKE_MACOSX_RPATH TRUE)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(../vendor/glfw/ ../bin/)

target_link_libraries(libchippuhachi ${CONAN_LIBS_SPDLOG} ${CONAN_LIBS_FMT} ${CONAN_LIBS_IMGUI} Vulkan::Vulkan glfw)

if (APPLE)
    set(LIB_VULKAN ${CMAKE_SOURCE_DIR}/ext/vulkan/macos/lib/libvulkan.dylib)
    INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/ext/vulkan/macos/include)
endif ()