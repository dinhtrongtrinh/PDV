#include <vector>
#include "../pdv_lib/pdv_lib.hpp"
#include "simd.h"

// Nasledujici program pocita nasobeni matice vektorem, y = Ax
int main() {
    constexpr size_t MATRIX_SIZE = 1024 * 12;
    // Zkontrolujeme, ze MATRIX_SIZE je delitelne velikosti SIMD, tedy pri pouziti vektoru nemusime resit prebytecne prvky
    static_assert(MATRIX_SIZE % vec_f32::size() == 0);

    // Vygenerujeme nahodnou matici A a vektor x
    std::cout << "Generating random test data...\n";
    std::vector<float> A = pdv::generate_random_vector<float>(MATRIX_SIZE * MATRIX_SIZE, 0, 100);
    std::vector<float> x = pdv::generate_random_vector<float>(MATRIX_SIZE, 0, 100);
    std::vector<float> y(MATRIX_SIZE);

    pdv::benchmark("Scalar", 3, [&] {
        pdv::do_not_optimize_away(A);
        pdv::do_not_optimize_away(x);

        for (size_t i = 0; i < MATRIX_SIZE; i++) {
            for (size_t j = 0; j < MATRIX_SIZE; j++) {
                y[i] += A[i * MATRIX_SIZE + j] * x[j];
            }
        }

        pdv::do_not_optimize_away(y);
    });

    pdv::benchmark("SIMD", 3, [&] {
        pdv::do_not_optimize_away(A);
        pdv::do_not_optimize_away(x);

        for (size_t i = 0; i < MATRIX_SIZE; i++) {
            vec_f32 sum{0.0f};
            for (size_t j = 0; j < MATRIX_SIZE; j += vec_f32::size()) {
                sum += vec_f32{&A[i * MATRIX_SIZE + j], element_aligned} * vec_f32{&x[j], element_aligned};
            }
            // sum entries in the vector into a single float
            y[i] = std::experimental::reduce(sum);
        }

        pdv::do_not_optimize_away(y);
    });

    pdv::benchmark("SIMD + OpenMP", 3, [&] {
        pdv::do_not_optimize_away(A);
        pdv::do_not_optimize_away(x);

        #pragma omp parallel for num_threads(4)
        for (size_t i = 0; i < MATRIX_SIZE; i++) {
            vec_f32 sum{0.0f};
            for (size_t j = 0; j < MATRIX_SIZE; j += vec_f32::size()) {
                sum += vec_f32{&A[i * MATRIX_SIZE + j], element_aligned} * vec_f32{&x[j], element_aligned};
            }
            // sum entries in the vector into a single float
            y[i] = std::experimental::reduce(sum);
        }

        pdv::do_not_optimize_away(y);
    });
}
