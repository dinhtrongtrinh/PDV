#pragma once

#include "vector_sum.h"

#include <optional>
#include <chrono>
#include <omp.h>

/**
 *  Datova struktura, ktera obsahuje vysledky behu ruznych implementaci
 */
struct Results {
    using Result = std::optional<std::chrono::nanoseconds>;

    Result time_ref;         // rychlost sekvencni implementace
    Result time_per_vector;  // rychlost implementace pri paralelizaci na urovni jednotlivych vektoru
    Result time_shuffle;     // rychlost implementace s "michanim" a statickym rozvrhovanim
    Result time_omp_dynamic; // rychlost implementace s dynamickym rozvrhovanim
    Result time_omp_static;  // rychlost implementace s statickym rozvrhovanim
};

namespace executor {
    [[nodiscard]] inline size_t size_of_smallest_vector(const InputVectors& data) {
        size_t min_size = std::numeric_limits<size_t>::max();

        // Redukce pomaha najit nejkratsi vektor
        // V pripade ze je datova sada mensi nez (pocet vlaken x 1000), vykonavame hledani sekvencne
        #pragma omp parallel for reduction(min : min_size) \
                default(none) shared(data) if(data.size() > (size_t)omp_get_max_threads() * 1000)
        for (auto& vec : data) {
            if (vec.size() < min_size) {
                min_size = vec.size();
            }
        }
        return min_size;
    }

    // Spoustec jednotlivych implementaci
    // Kontroluje spravnost vysledku. Pokud je vysledek spravny, vraci dobu behu, jinak vraci std::nullopt.
    inline Results::Result
    execute_method(SolutionFn functionPtr, const OutputVector& correct_solution, const InputVectors& data,
                   size_t minVectorSize) {
        OutputVector result(data.size());

        // Cas zacatku behu metody
        auto begin = std::chrono::steady_clock::now();
        // Beh metody
        try {
            (*functionPtr)(data, result, minVectorSize);
        } catch (...) {
            //ignored
        }
        // Cas konce behu metody
        auto end = std::chrono::steady_clock::now();

        // Kontrola spravnosti
        if (result == correct_solution) {
            // V pripade spravneho vysledku vraci cas behu
            return end - begin;
        }

        return std::nullopt;
    }

    inline Results::Result
    execute_method(SolutionFn functionPtr, const OutputVector& correct_solution, const InputVectors& data) {
        return execute_method(functionPtr, correct_solution, data, size_of_smallest_vector(data));
    }

    inline Results execute_methods(const OutputVector& solution, const InputVectors& data) {
        size_t shortestVectorLength = size_of_smallest_vector(data);
        auto referenceTime = execute_method(&vector_sum_sequential, solution, data, shortestVectorLength);
        auto perVectorTime = execute_method(&vector_sum_omp_per_vector, solution, data, shortestVectorLength);
        auto withShuffleTime = execute_method(&vector_sum_omp_shuffle, solution, data, shortestVectorLength);
        auto dynamicSchedTime = execute_method(&vector_sum_omp_dynamic, solution, data, shortestVectorLength);
        auto staticSchedulingTime = execute_method(&vector_sum_omp_static, solution, data, shortestVectorLength);
        return {referenceTime, perVectorTime, withShuffleTime, dynamicSchedTime, staticSchedulingTime};
    }
}
