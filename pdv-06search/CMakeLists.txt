cmake_minimum_required(VERSION 3.15)
project(06search)

set(CMAKE_CXX_STANDARD 20)

# force release build
set(CMAKE_BUILD_TYPE "Release")
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")
# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)

# use address and undefined behavior sanitizers in debug build to help catch memory issues and race conditions
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    add_compile_options("-fsanitize=address" "-fsanitize=undefined")
    add_link_options("-fsanitize=address" "-fsanitize=undefined")
endif()

find_package(OpenMP REQUIRED)
link_libraries(OpenMP::OpenMP_CXX)

add_executable(hw "src/main.cpp" "src/bfs.cpp" "src/iddfs.cpp" "src/iddfs_weighted.cpp")
