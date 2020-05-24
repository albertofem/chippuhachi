set(TARGET_NAME libchippuhachi)

add_library(${TARGET_NAME} chippuhachi.cpp chippuhachi.h cpu.cpp cpu.h mem.cpp mem.h gpu.cpp gpu.h)

target_include_directories(${TARGET_NAME} INTERFACE ./)

target_link_libraries(libchippuhachi ${CONAN_LIBS_SPDLOG} ${CONAN_LIBS_FMT})