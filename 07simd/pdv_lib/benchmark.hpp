#pragma once

#include <chrono>
#include <iostream>
#include <iomanip>
#include <string>
#include <sstream>
#include <optional>
#include <atomic>
#include <utility>
#include <cassert>
#include <vector>
#include <algorithm>
#include <numeric>
#include <functional>
#include "benchmark/result.hpp"
#include "benchmark/core.hpp"
#include "benchmark/presentation.hpp"
#include "util.hpp"


#ifndef NDEBUG
// write a warning message when compiling in debug mode
PDV_STATIC_INIT(debug_mode_check, {
    std::cerr << pdv::_::ansi::YELLOW
              << "WARNING: Running in DEBUG mode. If you're benchmarking your code, you should use RELEASE mode. "
                 "If you're not sure how to switch, ask your tutor."
              << pdv::_::ansi::RESET << "\n";
})
#endif


/// Macro that creates a variable `VAR_NAME` where the first test output is stored as a reference solution,
/// and a specialization of `pdv::validate_result` for `TYPE` that stores the argument on first call and on
/// subsequent invocations, compares the passed value against the stored reference. This is very ugly.
#define PDV_DEFINE_RESULT_COMPARER(TYPE, VAR_NAME) \
    static std::optional<TYPE> VAR_NAME{}; \
    \
    template<> \
    void pdv::validate_result(const TYPE& pdv_ret_value) { \
        if (!VAR_NAME) { \
            VAR_NAME = pdv_ret_value; \
            std::cout << pdv::_::ansi::GREEN << "       REFERENCE" << pdv::_::ansi::RESET; \
        } else if (pdv_ret_value == *VAR_NAME) { \
            std::cout << pdv::_::ansi::GREEN << "       CORRECT" << pdv::_::ansi::RESET; \
        } else { \
            std::cout << pdv::_::ansi::RED << "       INCORRECT" << pdv::_::ansi::RESET; \
        } \
    }


namespace pdv {
    /**
     * An exception class, which is thrown from tests that are supposed to be implemented
     * by students. `pdv::benchmark` handles this exception and shows an appropriate message.
     *
     * MK: It would be nice if this would work correctly from inside OpenMP `parallel` blocks,
     * where we shouldn't throw exceptions, e.g. by setting a flag and then somehow stopping
     * the loop, but I couldn't figure out any nice way of implementing that.
     */
    class not_implemented : public std::runtime_error {
    public:
        not_implemented() : runtime_error("Not yet implemented") {}
    };

    template<typename BenchmarkFn>
    inline void benchmark(std::string_view description, const benchmark_options& options, BenchmarkFn fn) {
        _::print_test_heading(description);
        try {
            // run the benchmark
            auto benchmark_result = benchmark_raw(options, fn);

            // print the results
            _::print_benchmark_result(benchmark_result);

            // render and print the return value, if any was returned
            if constexpr (requires { benchmark_result.return_value; }) {
                pdv::validate_result(benchmark_result.return_value);
            }
        } catch (const pdv::not_implemented&) {
            std::cout << "--- not implemented ---";
        }

        // normally, stdout should be line-buffered, but in CLion, if you turn off TTY, it will be fully buffered
        std::cout << std::endl;

        // I would like to return either the `fn` return value or the whole result struct, but we would need to figure
        //  out what to return when `pdv::not_implemented` is thrown
    }

    template<typename BenchmarkFn>
    inline void benchmark(std::string_view description, size_t warmup_iteration_count,
                          size_t iteration_count, BenchmarkFn fn) {
        benchmark(description, benchmark_options{iteration_count, warmup_iteration_count}, fn);
    }

    template<typename BenchmarkFn>
    inline void benchmark(std::string_view description, size_t iteration_count, BenchmarkFn fn) {
        benchmark(description, benchmark_options{iteration_count}, fn);
    }

    template<typename BenchmarkFn>
    inline void benchmark(std::string_view description, BenchmarkFn fn) {
        benchmark(description, benchmark_options{}, fn);
    }
}
