cmake_minimum_required(VERSION 3.20)

add_executable(ulwai_tests test_main.cpp)

target_link_libraries(ulwai_tests
    PRIVATE
        ulwai_core
)

add_test(NAME ulwai_core_tests COMMAND ulwai_tests)
