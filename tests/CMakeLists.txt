include(../build/conanbuildinfo.cmake)
conan_basic_setup()

set(UNIT_TEST_LIST
        mem)

foreach(NAME IN LISTS UNIT_TEST_LIST)
    list(APPEND UNIT_TEST_SOURCE_LIST ${NAME}.test.cpp)
endforeach()

set(TARGET_NAME tests)

add_executable(${TARGET_NAME}
        main.cpp
        ${UNIT_TEST_SOURCE_LIST})

target_link_libraries(${TARGET_NAME}
        PUBLIC libchippuhachi)

add_test(
        NAME ${TARGET_NAME}
        COMMAND ${TARGET_NAME} -o report.xml -r junit)