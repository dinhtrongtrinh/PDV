#include "../pdv_lib/pdv_lib.hpp"
#include <cmath>
#include <vector>

void magic_operation(std::vector<double>& array, bool use_openmp) {
    // Vypocet hodnoty array[i] nezavisi na vypoctu hodnoty array[j] (pro i != j).
    // Muzeme proto vypocty ruznych array[i] rozdelit mezi vice vlaken pomoci:
    #pragma omp parallel for if(use_openmp)
    for (ptrdiff_t i = 0; i < (ptrdiff_t)array.size(); i++) { // NOLINT(modernize-loop-convert)
        for (size_t k = 0; k < 500; k++) {
            array[i] = exp(log(array[i]));
        }
    }
}

int main() {
    std::vector<double> vec(100'000);

    pdv::benchmark("Magic operation (sequential)", [&] {
        magic_operation(pdv::launder_value(vec), false);
        pdv::do_not_optimize_away(vec);
    });

    pdv::benchmark("Magic operation (parallel)", [&] {
        magic_operation(pdv::launder_value(vec), true);
        pdv::do_not_optimize_away(vec);
    });

    return 0;
}
