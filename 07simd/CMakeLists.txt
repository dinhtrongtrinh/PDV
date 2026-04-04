cmake_minimum_required(VERSION 3.15)
project(pdv_lab07)

# uncomment this out to enable auto-vectorization in `1autovectorization`
#set(USE_AUTOVECTORIZATION TRUE)

set(CMAKE_CXX_STANDARD 20)

# target the instruction set of the current CPU
add_compile_options("-march=native")
# Clang seems to have an issue where it ignores the flags to disable auto-vectorization when LTO is enabled, disable it
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION FALSE)
# enable more warnings
add_compile_options("-Wall" "-Wextra" "-Wpedantic")
# enable extra float math optimizations
add_compile_options("-ffast-math")
# disable auto-vectorization, the compilers (especially Clang) are too smart for some of our benchmarks...
if (CLANG)
    add_compile_options("-fno-vectorize" "-fno-slp-vectorize" "-fno-tree-vectorize")
    add_link_options("-fno-vectorize" "-fno-slp-vectorize" "-fno-tree-vectorize")
else ()
    add_compile_options("-fno-tree-vectorize")
    add_link_options("-fno-tree-vectorize")
endif ()


find_package(OpenMP REQUIRED)


add_executable(07_0test_simd_support   src/0test_simd_support.cpp)
add_executable(07_1autovectorization   src/1autovectorization.cpp)
add_executable(07_2intrinsics_test     src/2intrinsics_test.cpp)
add_executable(07_3normal_distribution src/3normal_distribution.cpp)
add_executable(07_4conditional_swap    src/4conditional_swap.cpp)
add_executable(07_5matrix              src/5matrix.cpp)

if (USE_AUTOVECTORIZATION)
    target_compile_options(07_1autovectorization PUBLIC "-DAUTOVECTORIZATION")
    # enable autovectorization
    if (CLANG)
        target_compile_options(07_1autovectorization PUBLIC "-fvectorize" "-fslp-vectorize")
    else ()
        target_compile_options(07_1autovectorization PUBLIC "-ftree-vectorize")
    endif ()
endif ()

target_link_libraries(07_5matrix PUBLIC OpenMP::OpenMP_CXX)
