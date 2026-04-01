#pragma once

#include "vector_sum.h"

#include <random>
#include <stdexcept>

namespace data_generator {
    // Uniformni generator cisel
    // Parametry:
    //      data - datova sada
    //      solution - suma kazdeho vektoru cisel v datove sade
    inline void generate_data(OutputVector& solution, InputVectors& data) {
        if (data.size() != solution.size()) {
            throw std::invalid_argument("Solution vector and count of vectors in data lengths differ.");
        }

        std::mt19937 gen{0}; // NOLINT(*-msc51-cpp)
        std::uniform_int_distribution<int8_t> dist{std::numeric_limits<int8_t>::min(),
                                                   std::numeric_limits<int8_t>::max()};
        // Vygenerujeme data
        for (size_t i = 0; i < data.size(); i++) {
            int64_t sum = 0;
            for (auto& j : data[i]) {
                int8_t number = dist(gen);;
                j = number;
                sum += number;
            }
            solution[i] = sum;
        }
    }

    // Gaussovsky generator cisel
    // Parametry:
    //      begin, end - vektor nahodnych cisel
    //      mean - stredni hodnota normalniho rozdelen
    //      sigma - smerodatna odchylka normalniho rozdeleni
    template<typename Iter>
    inline void generate_vector_sizes(Iter begin, Iter end, float mean, float sigma) {
        // Mersenne twister PRNG, initialized with a fixed seed
        std::mt19937 gen(0); // NOLINT(*-msc51-cpp)
        // instance of class std::normal_distribution with specific mean and stddev
        std::normal_distribution<float> d(mean, sigma);

        for (auto it = begin; it != end; it++) {
            // get random number with normal distribution using gen as random source, then round to an integer
            *it = (size_t)std::max(std::round(d(gen)), 1.0f);
        }
    }
}
