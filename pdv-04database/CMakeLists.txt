cmake_minimum_required(VERSION 3.15)
project(04database)

set(CMAKE_CXX_STANDARD 20)

# force release build
set(CMAKE_BUILD_TYPE "Release")
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")
# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# OpenMP 4.0 is required for cancellations
find_package(OpenMP 4.0 REQUIRED)
link_libraries(OpenMP::OpenMP_CXX)

add_executable(hw "src/main.cpp" "src/generator.cpp" "src/query.cpp")
