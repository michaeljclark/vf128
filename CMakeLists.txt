cmake_minimum_required(VERSION 3.12)

project(vf8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include(CheckCXXCompilerFlag)

# We need haswell for TZCNT/LZCNT
check_cxx_compiler_flag("-march=haswell" has_march_haswell "int main() { return 0; }")
if (has_march_haswell)
	list(APPEND CMAKE_C_FLAGS -march=haswell)
	list(APPEND CMAKE_CXX_FLAGS -march=haswell)
endif()

include_directories(src)

add_library(vf8 STATIC src/vf128.cc)

add_executable(bench_io test/bench_io.cc)
target_link_libraries(bench_io vf8)

enable_testing()

foreach(prog IN ITEMS t1)
	add_executable(${prog} test/${prog}.c)
	target_link_libraries(${prog} vf8)
	add_test(test_${prog} ${prog})
endforeach()
