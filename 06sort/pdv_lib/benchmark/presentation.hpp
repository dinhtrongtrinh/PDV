/** Functions for pretty-printing benchmark results. */
#pragma once

#include <iostream>
#include <iomanip>
#include <chrono>
#include <sstream>
#include <optional>
#include <type_traits>
#include "result.hpp"

namespace pdv {
    namespace _ {
        template<typename Rep, typename Period>
        inline std::string duration_to_string(std::chrono::duration<Rep, Period> d) {
            using namespace std::chrono;
            using namespace std::chrono_literals;

            // write to an intermediate sstream instead, otherwise formatting flags behave weirdly
            //  (e.g. std::setw only setting width for the number and not for the unit,...)
            std::stringstream s{};
            if (d < 1ms) {
                s << duration_cast<microseconds>(d).count() << " μs";
            } else if (d < 100ms) {
                s << std::setprecision(2) << std::fixed;
                s << duration<double, std::milli>(d).count() << " ms";
            } else {
                s << duration_cast<milliseconds>(d).count() << " ms";
            }

            return s.str();
        }

        inline std::string frequency_to_string(double freq_hz) {
            std::stringstream s{};
            if (freq_hz < 100) {
                s << std::setprecision(2) << std::fixed;
                s << freq_hz << " Hz";
            } else if (freq_hz < 1000) {
                s << (size_t) freq_hz << " Hz";
            } else if (freq_hz < 1e6) {
                s << (size_t) (freq_hz / 1e3) << " kHz";
            } else {
                s << (size_t) (freq_hz / 1e6) << " MHz";
            }
            return s.str();
        }

        /** RAII class for saving and restoring stream flags. */
        class StreamFlags {
        private:
            std::ios& stream;
            std::ios::fmtflags flags{stream.flags()};

        public:
            explicit StreamFlags(std::ios& stream) : stream{stream} {}

            ~StreamFlags() {
                stream.flags(flags);
            }

            StreamFlags(const StreamFlags&) = delete;

            StreamFlags& operator=(const StreamFlags&) = delete;
        };

        template<typename T, typename = void>
        struct is_ostreamable : std::false_type {};

        template<typename T>
        struct is_ostreamable<T, std::void_t<decltype(std::declval<std::ostream&>() << std::declval<T>())>>
            : std::true_type {
        };

        /** Indicates if the type can be outputted to an ostream using `os << val`. */
        template<typename T>
        constexpr bool is_ostreamable_v = is_ostreamable<T>::value;
    }


    enum class benchmark_presentation {
        /** Default options. */
        DEFAULT = 0,
        /**
         * If passed, benchmarks show mean instead of minimum. Typically, minimum gives a more
         * reasonable estimate on desktop, as the threads tend to contend with other processes
         * and minimum represents the "ideal" scenario with minimal interference.
         */
        SHOW_MEAN = 1,
        SHOW_FREQUENCY = 2,
        /** If passed, speedup is not calculated and shown for benchmarks in this group. */
        HIDE_SPEEDUP = 4,
        SHOW_INDIVIDUAL_TIMES = 8,
    };

    inline benchmark_presentation operator|(benchmark_presentation o1, benchmark_presentation o2) {
        using ut = std::underlying_type_t<benchmark_presentation>;
        return benchmark_presentation{(ut) o1 | (ut) o2};
    }

    inline benchmark_presentation operator&(benchmark_presentation o1, benchmark_presentation o2) {
        using ut = std::underlying_type_t<benchmark_presentation>;
        return benchmark_presentation{(ut) o1 & (ut) o2};
    }


    namespace _ {
        inline bool show_mean = false;
        inline bool show_frequency = false;

        inline bool show_speedup = true;
        inline bool show_individual_times = false;
        inline std::optional<benchmark_duration> speedup_base{};
        inline size_t benchmark_name_width = 25;
    }

    inline void benchmark_group(benchmark_presentation options = benchmark_presentation::DEFAULT,
                                size_t default_benchmark_name_width = 25) {
        _::speedup_base = std::nullopt;
        _::benchmark_name_width = default_benchmark_name_width;
        _::show_speedup = !(bool) (options & benchmark_presentation::HIDE_SPEEDUP);
        _::show_individual_times = (bool) (options & benchmark_presentation::SHOW_INDIVIDUAL_TIMES);
        _::show_mean = (bool) (options & benchmark_presentation::SHOW_MEAN);
        _::show_frequency = (bool) (options & benchmark_presentation::SHOW_FREQUENCY);
    }

    inline void benchmark_group(size_t default_benchmark_name_width) {
        benchmark_group(benchmark_presentation::DEFAULT, default_benchmark_name_width);
    }

    namespace _ {
        namespace ansi {
            const std::string RESET = "\033[0m";
            const std::string YELLOW = "\033[33m";
            const std::string RED = "\033[91m";
            const std::string GREEN = "\033[92m";
            const std::string MAGENTA = "\033[95m";
        }

        inline void print_test_heading(std::string_view description) {
            // keep the length of the longest benchmark name
            if (_::benchmark_name_width < description.size()) {
                _::benchmark_name_width = description.size();
            }
            std::cout << std::setw((int) _::benchmark_name_width) << description << ": " << std::flush;
        }

        inline void print_benchmark_result(const benchmark_result& result) {
            auto duration = _::show_mean ? result.mean() : result.min();

            // store the duration as reference if this is the first benchmark
            if (!_::speedup_base.has_value()) {
                _::speedup_base = duration;
            }

            // print the results
            _::StreamFlags flags(std::cout);
            std::cout << std::left << ansi::GREEN << std::setw(10);
            if (_::show_frequency) {
                std::cout << frequency_to_string(
                    _::show_mean ? result.mean_frequency() : result.max_frequency());
            } else {
                std::cout << duration_to_string(duration);
            }
            std::cout << ansi::RESET;

            // compose contents of the parenthesized info block after the duration
            {
                std::stringstream ss{};

                if (_::show_speedup) {
                    auto speedup = (double) _::speedup_base.value().count() / (double) duration.count();
                    ss << "speedup: " << ansi::MAGENTA << std::setprecision(2) << std::fixed
                       << speedup << "x" << ansi::RESET;
                }

                if (result.iteration_count() > 1 || result.warmup_iteration_count() > 0) {
                    ss << (ss.tellp() > 0 ? ", " : "");
                    if (result.warmup_iteration_count() > 0) {
                        ss << result.warmup_iteration_count() << "+";
                    }
                    ss << result.iteration_count() << " iterations";
                }

                if (_::show_individual_times) {
                    ss << (ss.tellp() > 0 ? ", " : "");
                    ss << "[";
                    ss << duration_to_string(result.iterations()[0]);
                    for (auto d : result.iterations()) {
                        ss << ", " << duration_to_string(d);
                    }
                    ss << "]";
                }

                // if anything was written, render the block
                if (ss.tellp() > 0) {
                    std::cout << " (" << ss.str() << ")";
                }
            }
        }

        template<typename T>
        inline void print_benchmark_return_value(const T& return_value) {
            if constexpr (std::is_floating_point_v<T>) {
                _::StreamFlags flags(std::cout);
                // print floats with high precision, the default formatting drops quite a lot of decimal digits
                std::cout << "       result: " << std::setprecision(15) << return_value;
            } else if constexpr (std::is_same_v<T, bool>) {
                std::cout << "       result: " << (return_value ? "true" : "false");
            } else if constexpr (_::is_ostreamable_v<T>) {
                std::cout << "       result: " << return_value;
            } else {
                // not streamable, do not print any representation
            }
        }

        /** Specialization for `std::vector<T>`, prints a sum of all items. */
        template<typename T, typename = std::enable_if_t<_::is_ostreamable_v<T>>>
        inline void print_benchmark_return_value(const std::vector<T>& vec) {
            std::cout << "     checksum: " << std::reduce(vec.begin(), vec.end());
        }

        /** Specialization for `std::vector<T>*` (to avoid copying), prints a sum of all items. */
        template<typename T, typename = std::enable_if_t<_::is_ostreamable_v<T>>>
        inline void print_benchmark_return_value(const std::vector<T>* vec) {
            std::cout << "     checksum: " << std::reduce(vec->begin(), vec->end());
        }
    }

    /// Specialize this function if you want to define a custom validator for a specific type of values
    /// returned from the benchmark function.
    template <typename T>
    void validate_result(const T& bench_result) {
        // by default, just print the result and do not validate it
        _::print_benchmark_return_value(bench_result);
    }
}
