cmake_minimum_required(VERSION 3.15)
project(pdv_lab03)

set(CMAKE_CXX_STANDARD 20)

# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic" "-Wno-unused-parameter")


find_package(OpenMP REQUIRED)


add_executable(03_1openmp_test src/1openmp_test.cpp)
add_executable(03_2integrate src/2integrate_benchmark.cpp src/2integrate.cpp)

target_link_libraries(03_1openmp_test PUBLIC OpenMP::OpenMP_CXX)
target_link_libraries(03_2integrate PUBLIC OpenMP::OpenMP_CXX)
