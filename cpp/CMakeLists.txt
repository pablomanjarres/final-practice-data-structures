cmake_minimum_required(VERSION 3.16)
project(dialsort_bench CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_executable(bench
    src/dial_sort.cpp
    src/radix_sort.cpp
    src/merge_sort.cpp
    src/distributions.cpp
    src/stats.cpp
    src/runner.cpp
    src/main.cpp
)

target_include_directories(bench PRIVATE include)

if(MSVC)
    target_compile_options(bench PRIVATE /W4)
else()
    target_compile_options(bench PRIVATE -Wall -Wextra -Wpedantic)
endif()
