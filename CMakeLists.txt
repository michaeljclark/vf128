cmake_minimum_required(VERSION 3.12)

project(vf128)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(src)

add_library(vf8 STATIC src/vf128.cc)

add_executable(bench_io test/bench_io.cc)
target_link_libraries(bench_io vf8)

add_executable(rand_io test/rand_io.cc)
target_link_libraries(rand_io vf8)

enable_testing()

add_executable(t1 test/t1.c)
target_link_libraries(t1 vf8)
add_test(test_t1 t1)
