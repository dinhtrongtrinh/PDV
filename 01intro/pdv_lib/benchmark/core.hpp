/** Implementation of core benchmarking functions. */
#pragma once

#include <atomic>
#include <chrono>
#include <utility>
#include <cassert>
#include "result.hpp"

namespace pdv {
    // do_not_optimize_away is a function which convinces the compiler that it's using the argument,
    // but actually is a no-op; useful to prevent the compiler from optimizing away our benchmarks
    // clang-format off
    #if defined(_MSC_VER)
        namespace _ {
            #pragma optimize("", off)
            inline void do_not_optimize_away_sink(const void*) {}
            #pragma optimize("", on)
        }

        template<typename T>
        inline void do_not_optimize_away(const T& arg) {
            _::do_not_optimize_away_sink(&arg);
        }
    #else
        template<typename T>
        inline void do_not_optimize_away(const T& val) {
            #if defined(__clang__)
            asm volatile("" : : "r,m"(val) : "memory");
            #else
            asm volatile("" : : "mr"(val) : "memory");
            #endif
        }

        template<typename T>
        inline void do_not_optimize_away(T& val) {
            #if defined(__clang__)
            asm volatile("" : "+r,m"(val) : : "memory");
            #else
            // note that Google Benchmark, nanobench and a few others use "+m,r"; from my investigation
            //  and discussion with GCC maintainers, that is wrong and "+mr" should be used instead
            //  see https://gcc.gnu.org/bugzilla/show_bug.cgi?id=114437
            asm volatile("" : "+mr"(val) : : "memory");
            #endif
        }
    #endif
    // clang-format on

    // optimization to avoid needing to store pointer-to-pointer in memory
    template<typename T>
    inline void do_not_optimize_away(T* val) {
        return do_not_optimize_away(*val);
    }

    template<typename T>
    inline void do_not_optimize_away(const T* val) {
        return do_not_optimize_away(*val);
    }

    /**
     * Use this function on all benchmark inputs to prevent the compiler from inlining the inputs
     * and partially evaluating the benchmarked functions.
     */
    template<typename T>
    inline T& launder_value(T& val) {
        do_not_optimize_away(val);
        return val;
    }

    template<typename T>
    inline T launder_value(const T& val) {
        // do not allow copying non-fundamental types, to avoid surprising performance hits
        static_assert(std::is_fundamental_v<T>, "Refusing to copy non-fundamental type value in pdv::launder_value");

        T val_copy = val;
        do_not_optimize_away(val_copy);
        return val_copy;
    }

    namespace _ {
        template<typename Callable>
        constexpr bool DefaultReturnsVoid = std::is_same_v<void, std::invoke_result_t<Callable>>;

        // this implementation still leaves the compiler some room to screw us over; however, as long as
        // `std::chrono::steady_clock::now()` is implemented in a dynamic library (and thus compiler only
        // sees an external call), this should be safe against reordering; see https://stackoverflow.com/a/69288151
        //
        // ideally, we would want to ensure that
        //  1) nothing from outside is moved inside the timed region,
        //  2) `fn` is not moved outside
        // we can prevent moving `fn` before the range if `fn` takes an argument, and we run `do_not_optimize_away(std::make_pair(start_time, input))`
        // not sure how to prevent moving anything inside, quite sure that's not generally possible
        // also quite sure that you cannot sequence `fn` with the end timer in a reliable way

        template<typename BenchmarkFn>
        [[nodiscard]] inline benchmark_duration run_iteration(BenchmarkFn fn) {
            auto begin = std::chrono::steady_clock::now();
            if constexpr (_::DefaultReturnsVoid<BenchmarkFn>) {
                fn();
            } else {
                // fn returns something, use it to ensure the value is not optimized away
                do_not_optimize_away(fn());
            }
            auto end = std::chrono::steady_clock::now();
            return end - begin;
        }

        template<typename BenchmarkFn>
        [[nodiscard]] inline auto run_iteration_with_return_value(BenchmarkFn fn, benchmark_duration& out_duration) {
            auto begin = std::chrono::steady_clock::now();
            auto return_value = fn();
            // with some prayers, this prevents the compiler from moving `fn()` outside of the clock measurements
            // see https://theunixzoo.co.uk/blog/2021-10-14-preventing-optimisations.html
            do_not_optimize_away(return_value);
            auto end = std::chrono::steady_clock::now();

            // we return the duration through an out parameter so that NRVO works for `return_value` and we avoid a move
            out_duration = benchmark_duration(end - begin);
            return return_value;
        }
    }

    struct benchmark_options {
        size_t iteration_count;
        size_t warmup_iteration_count;

        constexpr explicit benchmark_options(size_t iteration_count = 1, size_t warmup_iteration_count = 0)
            : iteration_count(iteration_count), warmup_iteration_count(warmup_iteration_count) {}
    };

    // overload used when `fn()` does not return anything
    template<typename BenchmarkFn, typename = std::enable_if_t<_::DefaultReturnsVoid<BenchmarkFn>>>
    [[nodiscard]] inline benchmark_result benchmark_raw(size_t iteration_count, BenchmarkFn fn) {
        assert(iteration_count > 0);

        std::vector<benchmark_duration> iter_durations{};
        iter_durations.reserve(iteration_count);

        for (size_t i = 0; i < iteration_count; i++) {
            iter_durations.push_back(_::run_iteration(fn));
        }

        return benchmark_result{iter_durations};
    }

    // overload used when `fn()` has a return value
    template<typename BenchmarkFn, typename = std::enable_if_t<!_::DefaultReturnsVoid<BenchmarkFn>>>
    [[nodiscard]] inline benchmark_result_with_value<std::invoke_result_t<BenchmarkFn>>
    benchmark_raw(size_t iteration_count, BenchmarkFn fn) {
        assert(iteration_count > 0);

        std::vector<benchmark_duration> iter_durations{};
        iter_durations.reserve(iteration_count);

        // record the result of the first iteration, duration is passed through a ref to enable NRVO on `return_value`
        benchmark_duration d;
        auto return_value = _::run_iteration_with_return_value(fn, d);
        iter_durations.push_back(d);

        for (size_t i = 1; i < iteration_count; i++) {
            // ignore return values for the last `n - 1` iterations
            iter_durations.push_back(_::run_iteration(fn));
        }

        // TODO: figure out how to get rid of this move of `return_value`
        return benchmark_result_with_value{std::move(iter_durations), std::move(return_value)};
    }

    /**
     * Invokes `fn()` `iteration_count` times and returns the average duration of a single
     * iteration. If `warmup_iteration_count` is non-zero, `fn()` is executed before the measurement
     * is started to warmup caches, page in memory pages,...
     */
    template<typename BenchmarkFn>
    [[nodiscard]] inline auto benchmark_raw(const benchmark_options& options, BenchmarkFn fn) {
        assert(options.iteration_count > 0);
        if (options.warmup_iteration_count > 0) {
            // run the warmup iterations, ignore the measurement
            (void)benchmark_raw(options.warmup_iteration_count, fn);
        }
        // run the actual benchmark
        auto result = benchmark_raw(options.iteration_count, fn);
        result._set_warmup_iterations(options.warmup_iteration_count);
        return result;
    }
}
