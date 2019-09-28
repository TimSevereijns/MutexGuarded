cmake_minimum_required(VERSION 3.8)

enable_language(CXX)

project(mutex-guarded)

include(conan/conanbuildinfo.cmake)
conan_basic_setup()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -Werror -Wpedantic --coverage")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall -Wextra -Werror -Wpedantic")
set(CMAKE_CXX_OUTPUT_EXTENSION_REPLACE ON)

set(SOURCES
    tests/unit_tests.cpp
    source/mutex_guarded.h)

set(SOURCE_DIR
    source)

set(THIRD_PARTY
    third-party/catch2/single_include)

include_directories(${SOURCE_DIR} ${THIRD_PARTY})

add_executable(mutex-guarded ${SOURCES})

target_link_libraries(mutex-guarded stdc++ ${CONAN_LIBS})
