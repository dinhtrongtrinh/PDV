#include <iostream>
#include <vector>
#include "../pdv_lib/pdv_lib.hpp"
#include "simd.h"

void conditional_swap_scalar(float* data, size_t N) {
    size_t half = N / 2;
    for (size_t i = 0; i < half; i++) {
        if (data[i] > data[i + half]) {
            std::swap(data[i], data[i + half]);
        }
    }
}

void conditional_swap_vec(float* data, size_t N) {
    // TODO: Pouzijte `vec_f32` pro rychlejsi vypocet prohazovani prvku
    throw pdv::not_implemented{};
}

int main() {
    constexpr size_t N = 1 << 27;
    // Zkontrolujeme, ze N je delitelne velikosti SIMD, tedy pri pouziti vektoru nemusime resit prebytecne prvky
    static_assert(N % vec_f32::size() == 0);

    // Vygenerujeme data
    std::cout << "Generating random test data...\n";
    std::vector<float> data = pdv::generate_random_vector<float>(N, -100, 100);

    // Vytvorime kopie dat
    auto data_scalar = data;
    auto data_vec = data;

    // Vypocet provedeme skalarne
    pdv::benchmark("Scalar", [&] {
        conditional_swap_scalar(data_scalar.data(), data_scalar.size());
    });

    // Vypocet provedeme s vektory
    pdv::benchmark("SIMD", [&] {
        conditional_swap_vec(data_vec.data(), data_vec.size());
    });


    // Zkontrolujeme, zda vypocet probehl spravne
    size_t mistake_count = 0;
    for (size_t i = 0; i < N; i++) {
        if (0.001 < std::abs(data_scalar[i] - data_vec[i])) {
            mistake_count++;
        }
    }

    if (mistake_count > 0) {
        std::cerr << "\nV 'SIMD' verzi je pravdepodobne chyba. Pocet chyb ve vypoctu: "
                  << mistake_count << "\n";
    }

    return 0;
}
