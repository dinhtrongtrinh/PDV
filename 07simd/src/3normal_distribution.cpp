#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numbers>
#include "../pdv_lib/pdv_lib.hpp"
#include "simd.h"

static constexpr float PI = std::numbers::pi_v<float>;

// Pro ucely porovnani v nasem kodu pouzivame jednoduchou aproximaci
// funkce exp(x) - jak pro skalarni tak vektorizovanou verzi.
static float exp_scalar(float x) {
    float x_plus_3 = x + 3.0f;
    float x_minus_3 = x - 3.0f;
    return (x_plus_3 * x_plus_3 + 3.0f) / (x_minus_3 * x_minus_3 + 3.0f);
}

// Vektorova implementace funkce 'exp_scalar(...)'
static vec_f32 exp_vec_cpp(vec_f32 x) {
    vec_f32 three{3.0f};
    vec_f32 x_plus_3 = x + three;
    vec_f32 x_minus_3 = x - three;
    return (x_plus_3 * x_plus_3 + three) / (x_minus_3 * x_minus_3 + three);
}


// Skalarni implementace funkce, ktera vypocte hodnotu hustotni funkce
// normalniho rozdeleni (s parametry mu a sigma) nad polem dat 'data'.
static void normaldist_scalar(float mu, float sigma, std::vector<float>& data) {
    float expdiv = -2 * sigma * sigma;
    float normalizer = std::sqrt(2 * PI * sigma * sigma);

    for (size_t i = 0; i < data.size(); i++) { // NOLINT(modernize-loop-convert)
        float sc_data = data[i] - mu;
        sc_data = sc_data * sc_data;
        sc_data = sc_data / expdiv;
        sc_data = exp_scalar(sc_data);
        sc_data = sc_data / normalizer;
        data[i] = sc_data;
    }
}

// Vektorova implementace funkce 'normaldist_scalar(...)'.
static void normaldist_vec(float mu, float sigma, std::vector<float>& data) {
    // TODO: Pouzijte `vec_f32` pro rychlejsi vypocet funkce hustoty
    throw pdv::not_implemented{};
}

int main() {
    constexpr size_t N = 16 * 10000000;
    // Zkontrolujeme, ze N je delitelne velikosti SIMD, tedy pri pouziti vektoru nemusime resit prebytecne prvky
    static_assert(N % vec_f32::size() == 0);

    // Vygenerujeme testovaci data
    std::cout << "Generating random test data...\n";
    std::vector<float> data = pdv::generate_random_vector<float>(N, 0.0, 3.0);

    // Vytvorime kopie dat
    auto data_scalar = data;
    auto data_vec = data;

    // Otestujeme implementace
    pdv::benchmark("Scalar", 10, [&] {
        pdv::do_not_optimize_away(data_scalar);
        normaldist_scalar(0.0, 1.0, data_scalar);
        pdv::do_not_optimize_away(data_scalar);
    });

    pdv::benchmark("SIMD", 10, [&] {
        pdv::do_not_optimize_away(data_vec);
        normaldist_vec(0.0, 1.0, data_vec);
        pdv::do_not_optimize_away(data_vec);

    });

    // Spocitame rozdily v approximaci
    double diff_vec = 0.0;
    for (size_t i = 0; i < N; i++) {
        diff_vec += std::abs(data_scalar[i] - data_vec[i]);
    }

    // Pokud je odchylka SIMD metody vypoctu moc velka, vypiseme varovani
    constexpr double MAX_APPROXIMATION_ERROR = 10;
    if (diff_vec > MAX_APPROXIMATION_ERROR) {
        std::cout << "\n";
        std::cout << std::fixed << std::setprecision(2);
        std::cerr << "V 'SIMD C++' je pravdepodobne chyba. Absolutni chyba vypoctu: "
                  << diff_vec << "\n";
    }

    return 0;
}
