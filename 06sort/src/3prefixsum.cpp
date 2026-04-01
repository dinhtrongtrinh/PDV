#include "3prefixsum.h"
#include <string>
#include <vector>
#include <chrono>
#include <execution>
#include "../pdv_lib/pdv_lib.hpp"

using ElemT = uint32_t;
constexpr size_t N = 1 << 28;

PDV_DEFINE_RESULT_COMPARER(std::vector<ElemT>, ref_solution)

int main() {
    // Nagenerujeme si nahodna data z intervalu [0 .. 3]. Pro ty budeme pocitat prefix-sum.
    auto data = pdv::generate_random_vector<ElemT>(N, 0, 4);

    pdv::benchmark("prefix_sum_seq", [data]() mutable {
        pdv::do_not_optimize_away(data);
        prefix_sum_seq(data.begin(), data.end());
        return std::move(data);
    });

    pdv::benchmark("prefix_sum_par", [data]() mutable {
        pdv::do_not_optimize_away(data);
        prefix_sum_par(data.begin(), data.end());
        return std::move(data);
    });

    return 0;
}
