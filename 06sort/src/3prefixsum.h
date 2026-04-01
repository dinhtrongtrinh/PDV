#pragma once

#include <numeric>
#include <vector>
#include <iterator>
#include <omp.h>
#include "../pdv_lib/pdv_lib.hpp"

template<typename It>
void prefix_sum_seq(It begin, It end) {
    // Standardni knihovna umi prefixni sumu spocitat za nas.
    std::inclusive_scan(begin, end, begin);
}


template<typename It>
void prefix_sum_par(It begin, It end) {
    using T = typename std::iterator_traits<It>::value_type;
    auto size = end - begin;
    if (size <= 0) return;

    int num_threads;
    std::vector<T> block_sums;

#pragma omp parallel
    {
        int tid = omp_get_thread_num();
#pragma omp single
        {
            num_threads = omp_get_num_threads();
            block_sums.resize(num_threads);
        }

        // 1. Rozdělení práce: Každé vlákno si určí svůj rozsah
        auto my_start_idx = (tid * size) / num_threads;
        auto my_end_idx = ((tid + 1) * size) / num_threads;
        auto my_begin = begin + my_start_idx;
        auto my_end = begin + my_end_idx;

        // 2. Lokální scan: Každé vlákno si posčítá svůj kousek
        if (my_begin < my_end) {
            std::inclusive_scan(my_begin, my_end, my_begin);
            // Uložíme celkový součet tohoto bloku (poslední prvek)
            block_sums[tid] = *(my_end - 1);
        }

        // --- BARIÉRA: Musíme mít všechny lokální součty ---
#pragma omp barrier

        // 3. Master vypočítá prefixovou sumu nad součty bloků (offsety)
#pragma omp single
        {
            for (int i = 1; i < num_threads; i++) {
                block_sums[i] += block_sums[i - 1];
            }
        }

        // --- BARIÉRA: Offsety jsou připraveny ---
#pragma omp barrier

        // 4. Finální oprava: Každé vlákno (kromě nultého) přičte offset předchozího bloku
        if (tid > 0 && my_begin < my_end) {
            T offset = block_sums[tid - 1];
            for (auto it = my_begin; it < my_end; it++) {
                *it += offset;
            }
        }
    }
}
