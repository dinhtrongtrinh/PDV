#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// type aliases to make the function parameters more readable
using InputVectors = std::vector<std::vector<int8_t>>;
using OutputVector = std::vector<int64_t>;
using SolutionFn = void (*)(const InputVectors& data, OutputVector& solution, size_t min_vector_size);

void vector_sum_omp_per_vector(const InputVectors& data, OutputVector& solution, size_t min_vector_size);
void vector_sum_omp_static(const InputVectors& data, OutputVector& solution, size_t min_vector_size);
void vector_sum_omp_dynamic(const InputVectors& data, OutputVector& solution, size_t min_vector_size);
void vector_sum_omp_shuffle(const InputVectors& data, OutputVector& solution, size_t min_vector_size);

// sequential implementation, for comparison
inline void vector_sum_sequential(const InputVectors& data, OutputVector& solution, size_t) {
    for (size_t i = 0; i < data.size(); i++) {
        int64_t sum = 0;
        for (auto n : data[i]) {
            sum += n;
        }
        solution[i] = sum;
    }
}
