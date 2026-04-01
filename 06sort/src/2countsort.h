#pragma once

#include <limits>
#include <vector>
#include <atomic>
#include "../pdv_lib/pdv_lib.hpp"
#include "./3prefixsum.h"

template<typename T>
void countsort_seq(std::vector<T>& data) {
    // find the lowest and highest values in the input
    auto min = std::numeric_limits<T>::max();
    auto max = std::numeric_limits<T>::min();
    for (auto& n : data) {
        if (n < min) min = n;
        if (n > max) max = n;
    }

    // count each value in the vector in the min/max range
    auto range = max - min + 1;
    auto counts = std::vector<size_t>(range);
    for (auto& n : data) {
        counts[n - min]++;
    }

    // reconstruct the vector content from the counts
    auto it = data.begin();
    for (size_t i = 0; i < counts.size(); i++) {
        // set `counts[i]` values to `min + i`, return iterator to the end of the range
        it = std::fill_n(it, counts[i], min + i);
    }
}

template<typename T>


void countsort_par(std::vector<T>& data) {
    auto global_min = std::numeric_limits<T>::max();
    auto global_max = std::numeric_limits<T>::min();

    // 1. Min a Max (Složené do jednoho průchodu pro rychlost)
    #pragma omp parallel for reduction(min:global_min) reduction(max:global_max)
    for (size_t i = 0; i < data.size(); i++) {
        T val = data[i];
        if (val < global_min) global_min = val;
        if (val > global_max) global_max = val;
    }

    auto range = global_max - global_min + 1;
    std::vector<size_t> counts(range, 0);

    // 2. Histogram (Pro jednoduchost použijeme atomic, ale pozor - counts musí být sdílené)
    #pragma omp parallel for
    for (size_t i = 0; i < data.size(); i++) {
    #pragma omp atomic
        counts[data[i] - global_min]++;
    }

    // 3. Prefix Sum (Sekvenční část - pokud nemáš parallel_prefix_sum)
    // POZOR: Prefix sum mění pole counts na pole indexů
    std::vector<size_t> start_pos(range);
    start_pos[0] = 0;
    for (size_t i = 1; i < range; i++) {
        start_pos[i] = start_pos[i-1] + counts[i-1];
    }

    // 4. Dynamické "Vypsání" (Fill)
    // Tady nepoužíváme "for (auto n : data)", ale procházíme pole INDEXŮ (counts)
    #pragma omp parallel for schedule(dynamic, 64)
    for (size_t i = 0; i < range; i++) {
        size_t count = counts[i];
        if (count > 0) {
            size_t offset = start_pos[i];
            T value_to_write = global_min + i;

            // Každé vlákno dostane instrukci: "Zapiš hodnotu X na pozici Y, celkem Z krát"
            std::fill_n(data.begin() + offset, count, value_to_write);
        }
    }
}
