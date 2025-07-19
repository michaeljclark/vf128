cmake_minimum_required(VERSION 3.12)

project(vf128)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include_directories(src)

add_library(vf128 STATIC src/vf128.cc)

add_executable(bench_io test/bench_io.cc)
target_link_libraries(bench_io vf128)

add_executable(rand_io test/rand_io.cc)
target_link_libraries(rand_io vf128)

enable_testing()

add_executable(t1 test/t1.c)
add_executable(t2 test/t2.c)
target_link_libraries(t1 vf128)
target_link_libraries(t2 vf128)
add_test(test_t1 t1)
add_test(test_t2 t2)
