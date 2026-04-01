cmake_minimum_required(VERSION 3.15)
project(01thrpool)

set(CMAKE_CXX_STANDARD 20)

# force release build
set(CMAKE_BUILD_TYPE "Release")
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")
# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

find_package(Threads REQUIRED)
link_libraries(Threads::Threads)

add_executable(hw "src/main.cpp")

