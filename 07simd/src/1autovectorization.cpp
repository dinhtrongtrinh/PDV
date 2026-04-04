#include <numeric>
#include <iostream>
#include <cmath>
#include <random>
#include "../pdv_lib/pdv_lib.hpp"
#include "simd.h"

float run_scalar(const std::vector<float>& b, const std::vector<float>& c) {
    std::vector<float> a(b.size());

    #ifdef AUTOVECTORIZATION
    std::string name = "Auto-vectorized";
    #else
    std::string name = "Scalar";
    #endif

    pdv::benchmark(name, 5, [&] {
        pdv::do_not_optimize_away(a);
        for (size_t i = 0; i < a.size(); i++) {
            a[i] = std::sqrt(b[i] * b[i] + c[i] * c[i] + b[i] * c[i] + b[i] / c[i] + c[i] / b[i]
                + b[i] / (b[i] + c[i]) + c[i] / (b[i] - c[i]) + c[i] / (b[i] + c[i]));
        }
        pdv::do_not_optimize_away(a);
    });

    return std::reduce(a.begin(), a.end());
}

float run_vectorized(const std::vector<float>& b, const std::vector<float>& c) {
    std::vector<float> a(b.size());

    pdv::benchmark("SIMD", 5, [&] {
        pdv::do_not_optimize_away(a);
        for (size_t i = 0; i < a.size(); i += vec_f32::size()) {
            vec_f32 B{&b[i], element_aligned};
            vec_f32 C{&c[i], element_aligned};

            vec_f32 A = sqrt(B * B + C * C + B * C + B / C + C / B
                + B / (B + C) + C / (B - C) + C / (B + C));
            A.copy_to(&a[i], element_aligned);
        }
        pdv::do_not_optimize_away(a);
    });

    return std::reduce(a.begin(), a.end());
}

int main() {
    // Pocet prvku v poli
    constexpr size_t N = 1024 * 1024 * 128;
    // Zkontrolujeme, ze N je delitelne 8, tedy pri pouziti vektoru nemusime resit "leftovers"
    static_assert(N % 8 == 0);

    std::cout << "Generating random test data...\n";
    std::vector<float> b = pdv::generate_random_vector<float>(N, 1, 2);
    std::vector<float> c = pdv::generate_random_vector<float>(N, 0.0000000001f, 1);

    // Spocitame dostatecne slozitou funkci
    // Sekvencne
    float checksum_scalar = run_scalar(b, c);
    // Vektorove
    float checksum_vec = run_vectorized(b, c);

    std::cout << "\n";
    #ifndef AUTOVECTORIZATION
    std::cout << "Now try to enable auto-vectorization in 'CMakeLists.txt'.\n";
    #endif

    if (abs(checksum_vec - checksum_scalar) > 100) {
        std::cerr << "V 'SIMD' verzi je pravdepodobne chyba. Rozdil souctu vysledku: "
                  << checksum_vec - checksum_scalar << "\n";
    }

    return 0;
}
