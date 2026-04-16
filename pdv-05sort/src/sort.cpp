#include "sort.h"
#include <vector>
#include <omp.h>
#include <algorithm>

void radix_par(std::vector<std::string*>& vector_to_sort, MappingFunction mapping_function,
               size_t alphabet_size, size_t str_size) {

    size_t n = vector_to_sort.size();
    if (n == 0 || str_size == 0) return;

    int num_threads = omp_get_max_threads();

    std::vector<std::string*> temp_vector(n);

    std::vector<size_t> local_counts(num_threads * alphabet_size, 0);

    for (int i = str_size - 1; i >= 0; --i) {

        std::fill(local_counts.begin(), local_counts.end(), 0);

        #pragma omp parallel
        {
            int tid = omp_get_thread_num();
            #pragma omp for schedule(static)
            for (size_t j = 0; j < n; ++j) {
                size_t char_idx = mapping_function(vector_to_sort[j]->at(i));
                local_counts[tid * alphabet_size + char_idx]++;
            }
            #pragma omp single
            {
                size_t current_offset = 0;
                for (size_t c = 0; c < alphabet_size; ++c) {
                    for (int t = 0; t < num_threads; ++t) {
                        size_t count = local_counts[t * alphabet_size + c];
                        local_counts[t * alphabet_size + c] = current_offset;
                        current_offset += count;
                    }
                }
            }
            #pragma omp for schedule(static)
            for (size_t j = 0; j < n; ++j) {
                size_t char_idx = mapping_function(vector_to_sort[j]->at(i));
                size_t dest_idx = local_counts[tid * alphabet_size + char_idx]++;
                temp_vector[dest_idx] = vector_to_sort[j];
            }
        }
        vector_to_sort.swap(temp_vector);
    }
}