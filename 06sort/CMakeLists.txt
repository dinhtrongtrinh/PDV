cmake_minimum_required(VERSION 3.15)
project(pdv_lab06)

set(CMAKE_CXX_STANDARD 20)

# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")


# OpenMP 4.0 is required for tasks and min/max reductions
find_package(OpenMP 4.0 REQUIRED)
link_libraries(OpenMP::OpenMP_CXX)

add_executable(06_1mergesort src/1mergesort.cpp)
add_executable(06_2countsort src/2countsort.cpp)
add_executable(06_3prefixsum src/3prefixsum.cpp)
