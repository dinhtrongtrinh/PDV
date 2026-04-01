cmake_minimum_required(VERSION 3.15)
project(pdv_lab01)

set(CMAKE_CXX_STANDARD 20)

# target the instruction set of the current CPU
add_compile_options("-march=native")
# enable LTO
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")


find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)


add_executable(01_0hello src/0hello.cpp)
add_executable(01_0cpp20 src/0cpp20.cpp)
add_executable(01_1memory src/1memory.cpp)
add_executable(01_2matrix src/2matrix.cpp)
add_executable(01_3false_sharing src/3false_sharing.cpp)
add_executable(01_4magic src/4magic.cpp)
add_executable(01_5ema src/5ema.cpp)

target_link_libraries(01_0hello OpenMP::OpenMP_CXX)
target_link_libraries(01_0cpp20 OpenMP::OpenMP_CXX)
target_link_libraries(01_3false_sharing Threads::Threads)
target_link_libraries(01_4magic OpenMP::OpenMP_CXX)
target_link_libraries(01_5ema OpenMP::OpenMP_CXX)
