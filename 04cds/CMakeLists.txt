cmake_minimum_required(VERSION 3.15)
project(pdv_lab04)

set(CMAKE_CXX_STANDARD 20)

# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic" "-Wno-unused-parameter")


find_package(OpenMP REQUIRED)


add_executable(04_linked_list "src/main.cpp")
target_link_libraries(04_linked_list PUBLIC OpenMP::OpenMP_CXX)
