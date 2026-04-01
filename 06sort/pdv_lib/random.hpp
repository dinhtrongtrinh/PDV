#pragma once

#include <vector>
#include <algorithm>
#include <cstdint>
#include <cassert>

namespace pdv {
    // namespace for internal implementation details
    namespace _ {
        // https://artificial-mind.net/blog/2020/09/26/dont-deduce
        // a "hack" to prevent the compiler from deducing template arguments
        template<typename T>
        using dont_deduce = typename std::common_type<T>::type;

        class lcg_musl {
        private:
            // LCG parameters taken from Musl (https://github.com/bminor/musl/blob/master/src/prng/rand.c)
            // they are nice, because the modulo is 2^64, which is a no-op with 64bit arithmetic
            static constexpr uint64_t ADD_COEF = 1ull;
            static constexpr uint64_t MULTIPLY_COEF = 6'364'136'223'846'793'005ull;

            uint64_t state;

        public:
            // use `seed + 1` to avoid seeding with 0, which would return 1 as the first `state`
            explicit lcg_musl(uint64_t seed) : state{seed + 1} {}

            /**
             * The bottom bits of `state` are very periodic (e.g. the bottom bit alternates
             * between 1 and 0), only use the upper 32 bits for more randomness.
             */
            uint64_t operator()() {
                // the LCG works modulo 2^64, which is implicit for 64-bit unsigned arithmetic
                state = state * MULTIPLY_COEF + ADD_COEF;
                return state;
            }
        };

        class uniform_random_bool {
        private:
            lcg_musl lcg;

        public:
            explicit uniform_random_bool(uint64_t seed) : lcg{seed} {}

            bool operator()() {
                // use the highest bit, it should be the most unpredictable one
                // the bottom bits are quite predictable (e.g. the first bit always alternates)
                return (bool)(lcg() >> 63);
            }
        };

        template<typename T>
        class uniform_random_int {
            static_assert(std::is_integral_v<T>);
        private:
            lcg_musl lcg;
            const T min;
            const uint64_t diff;

        public:
            explicit uniform_random_int(uint64_t seed) : lcg{seed}, min{0}, diff{0} {}

            // prevent template argument deduction from min, max
            uniform_random_int(uint64_t seed, dont_deduce<T> min, dont_deduce<T> max)
                    : lcg{seed}, min{min}, diff{(uint64_t)(max - min)} {
                assert(min < max);
            }

            T operator()() {
                if constexpr (sizeof(T) <= 4) {
                    if (diff == 0) {
                        return (T)generate_u32();
                    } else {
                        return (T)(generate_u32() % diff) + min;
                    }
                } else {
                    if (diff == 0) {
                        return (T)generate_u64();
                    } else {
                        return (T)(generate_u64() % diff) + min;
                    }
                }
            }

        private:
            uint64_t generate_u64() {
                // recover an u64 pseudo-random number by combining two u32 pseudo-random numbers
                return ((uint64_t)generate_u32() << 32) | (uint64_t)generate_u32();
            }

            uint32_t generate_u32() {
                // the bottom bits of `state` are very periodic (e.g. the bottom bit alternates
                //  between 1 and 0), use the upper 32 bits for more randomness
                return lcg() >> 32;
            }
        };

        template<typename T>
        class uniform_random_float {
            static_assert(std::is_floating_point_v<T>);
        private:
            lcg_musl lcg;
            const T min;
            const T diff;

        public:
            // to simplify usage, default float range is <0.0, 1.0)
            explicit uniform_random_float(uint64_t seed)
                    : uniform_random_float(seed, (T)0.0, (T)1.0) {}

            // prevent template argument deduction from min, max
            uniform_random_float(uint64_t seed, dont_deduce<T> min, dont_deduce<T> max)
                    : lcg{seed}, min{min}, diff{max - min} {
                assert(min < max);
            }

            T operator()() {
                return generate_normalized() * diff + min;
            }

        private:
            T generate_normalized() {
                // https://prng.di.unimi.it/, section "Generating uniform doubles in the unit interval"
                if constexpr (std::is_same_v<T, double>) {
                    // double has 53 bits of mantissa
                    return (double)(lcg() >> (64 - 53)) * 0x1.0p-53;
                } else if (std::is_same_v<T, float>) {
                    // float has 24 bits of mantissa
                    return (float)(lcg() >> (64 - 24)) * 0x1.0p-24f;
                } else {
                    // unknown floating point type, let's use as much randomness as we can
                    // use 63 bits instead of 64 bits to improve performance, https://godbolt.org/z/K13nceazj
                    return (T)(lcg() >> 1) * (T)0x1.0p-63;
                }
            }
        };
    }

    /**
     * Simple LCG random number generator from a uniform distribution in <min, max).
     * The seed is fixed, but each instance receives a different one, so the generated
     * sequence is the same between repeated executions of the whole binary.
     *
     * This class exists because C++11 <random> is very slow, dunno why, and we don't need
     * much randomness for these benchmarks.
     *
     * Example usage:
     * ```cpp
     * pdv::uniform_random<uint32_t> random(0, 1000);
     * auto r1 = random();
     * auto r2 = random();
     * ...
     * ```
     */
    template<typename T>
    class uniform_random {
        static_assert(std::is_same_v<T, bool> || std::is_integral_v<T>
                || std::is_floating_point_v<T>);
        static_assert(sizeof(T) <= 8, "pdv::uniform_random supports at most 64 bit result types");
    private:
        // use either bool, int or float version based on T
        using rng_t = std::conditional_t<
                std::is_same_v<T, bool>, _::uniform_random_bool,
                std::conditional_t<std::is_integral_v<T>,
                        _::uniform_random_int<T>,
                        _::uniform_random_float<T>>>;

        // yes, use seed 0, to get reproducible results
        static inline uint64_t next_seed = 0; // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

        rng_t rng;

    public:
        // use different (but deterministic) seed for each instance
        uniform_random() : rng{next_seed++} {}

        explicit uniform_random(uint64_t seed) : rng{seed} {}

        // prevent template argument deduction from min, max
        uniform_random(_::dont_deduce<T> min, _::dont_deduce<T> max) : rng{next_seed++, min, max} {}

        // prevent template argument deduction from min, max
        uniform_random(uint64_t seed, _::dont_deduce<T> min, _::dont_deduce<T> max) : rng{seed, min, max} {}

        T operator()() {
            return rng();
        }
    };

    template<typename T, typename ...Ts>
    inline std::vector<T> generate_random_vector(size_t length, Ts&& ...args) {
        // forward all extra arguments after `length` to pdv::uniform_random constructor
        pdv::uniform_random<T> random((T)(args)...);
        std::vector<T> vec(length);
        std::generate(vec.begin(), vec.end(), random);
        return vec;
    }
}
