/** Structs representing benchmark results. */
#pragma once

#include <chrono>
#include <vector>
#include <algorithm>
#include <numeric>
#include <optional>
#include <utility>
#include <cmath>
#include <cassert>

namespace pdv {
    // use picoseconds for benchmark durations, in case the benchmark runs in nanosecond range
    using benchmark_duration = std::chrono::duration<long long, std::pico>;

    struct benchmark_result {
    private:
        size_t warmup_iters_ = 0;
        std::vector<benchmark_duration> iters_;
        mutable std::optional<benchmark_duration> min_{};
        mutable std::optional<benchmark_duration> max_{};
        mutable std::optional<benchmark_duration> mean_{};
        mutable std::optional<benchmark_duration> deviation_{};

    public:
        explicit benchmark_result(std::vector<benchmark_duration> iter_durations)
                : iters_{std::move(iter_durations)} {
            assert(!iters_.empty());
        }

        // internal, called by `benchmark_raw`
        void _set_warmup_iterations(size_t warmup_iteration_count) {
            warmup_iters_ = warmup_iteration_count;
        }

        [[nodiscard]] size_t iteration_count() const {
            return iters_.size();
        }

        [[nodiscard]] size_t warmup_iteration_count() const {
            return warmup_iters_;
        }

        [[nodiscard]] const decltype(iters_)& iterations() const {
            return iters_;
        }

        [[nodiscard]] benchmark_duration min() const {
            return *(min_ ? min_ : min_ = *std::min_element(iters_.begin(), iters_.end()));
        }

        [[nodiscard]] benchmark_duration max() const {
            return *(max_ ? max_ : max_ = *std::max_element(iters_.begin(), iters_.end()));
        }

        [[nodiscard]] benchmark_duration mean() const {
            return *(mean_ ? mean_ : mean_ = std::reduce(iters_.begin(), iters_.end()) / iters_.size());
        }

        [[nodiscard]] benchmark_duration deviation() const {
            return *(deviation_ ? deviation_ : deviation_ = calc_deviation());
        }

        [[nodiscard]] double mean_frequency() const {
            return calc_frequency(mean());
        }

        [[nodiscard]] double max_frequency() const {
            return calc_frequency(min());
        }

    private:
        [[nodiscard]] static double calc_frequency(benchmark_duration d) {
            return 1.0 / std::chrono::duration<double>(d).count();
        }

        [[nodiscard]] benchmark_duration calc_deviation() const {
            double sum = 0;
            for (auto d : iters_) {
                sum += (double)((d - mean()).count() * (d - mean()).count());
            }
            // http://duramecho.com/Misc/WhyMinusOneInSd.html
            auto variance = sum / (double)(iters_.size() - 1);
            return benchmark_duration{(long)std::sqrt(variance)};
        }
    };

    /** Struct representing a benchmark result which includes the return value of the benchmarked function. */
    template<typename T>
    struct benchmark_result_with_value : public benchmark_result {
        T return_value;

        benchmark_result_with_value(std::vector<benchmark_duration> iter_durations, T&& return_value)
            : benchmark_result{std::move(iter_durations)}, return_value{std::forward<T>(return_value)} {}
    };
}
