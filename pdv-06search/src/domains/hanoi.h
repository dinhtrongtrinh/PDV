#pragma once

#include <iostream>
#include <bit>
#include <sstream>
#include <limits>
#include <utility>
#include "../state.h"

// NOTE: this code is very non-idiomatic from a performance perspective; however, making the domain faster
//  would make it much harder for you to get a reasonable speedup over the serial solution

/// Domain representing a game of Towers of Hanoi. Each state corresponds to a valid configuration of discs on one or
/// more towers, each action corresponds to a taking a single disk and moving it to a different tower, if allowed
/// by the rules of the game (there must never be a larger disk on top of a smaller one).
template<size_t RODS, size_t TOWERS, size_t DISCS>
class hanoi {
    static_assert(TOWERS > 0 && DISCS > 0, "Vezi i disku musi byt vice jak 0");
    static_assert(RODS >= 2, "Tyce musi byt alespon 2");
    static_assert(RODS > TOWERS, "Tyci musi byt vic jak vezi");
    static_assert(DISCS <= 32, "Disku musi byt nejvyse 32");
    // internal consistency check to keep the error message above in sync with code
    static_assert(std::numeric_limits<uint32_t>::digits == 32);

private:
    std::vector<uint32_t> _conf = std::vector<uint32_t>(RODS);

public:
    hanoi() {
        for (size_t i = 0; i < TOWERS; i++) {
            _conf[i] = (1 << DISCS) - 1;
        }
    }

    [[nodiscard]] state_ptr root() const {
        return std::make_shared<dstate>(state_ptr(), 0, _conf);
    }

    friend std::ostream& operator<<(std::ostream& s, const hanoi&) {
        s << "Domain: Towers of Hanoi\n";
        s << "Rod count = " << RODS << ", tower height = " << DISCS << ", tower count = " << TOWERS << "\n";
        return s;
    }

private:
    class dstate final : public state, public std::enable_shared_from_this<dstate> {
    private:
        const std::vector<uint32_t> _conf;
        uint64_t _id;

    public:
        dstate(state_ptr predecessor, uint64_t cost, std::vector<uint32_t> conf)
            : state(std::move(predecessor), cost), _conf(std::move(conf)) {
            _id = 0ull;
            for (size_t disc = 0; disc < DISCS; ++disc) {
                uint32_t mask = 1u << disc;
                size_t rod = 0;
                for (size_t tower = 0; tower < TOWERS; ++tower) {
                    for (; !(_conf[rod] & mask); ++rod) {
                    }
                    _id = (_id << std::bit_width(RODS)) | rod;
                }
            }
        }

        [[nodiscard]] std::vector<state_ptr> next_states() const override {
            auto tmp_conf = _conf;
            auto succ = std::vector<state_ptr>();
            for (size_t s = 0; s < RODS; ++s) {
                auto sconf = _conf[s];
                if (!sconf) continue;

                uint32_t sdisk_ind = (1 << std::countr_zero(sconf));
                uint32_t sdisk_mask = (sdisk_ind << 1) - 1;

                for (size_t t = 0; t < RODS; ++t) {
                    if (_conf[t] & sdisk_mask) continue;

                    tmp_conf[s] ^= sdisk_ind;
                    tmp_conf[t] ^= sdisk_ind;

                    succ.push_back(std::make_shared<const dstate>(
                        this->shared_from_this(), total_cost() + 1, tmp_conf));

                    tmp_conf[s] ^= sdisk_ind;
                    tmp_conf[t] ^= sdisk_ind;
                }
            }
            return succ;
        }

        [[nodiscard]] bool goal() const override {
            uint32_t mask = (1 << DISCS) - 1;
            size_t crod = RODS - 1;
            for (size_t i = 0; i < TOWERS; i++) {
                if (_conf[crod--] != mask) return false;
            }
            return true;
        }

        [[nodiscard]] uint64_t id() const override {
            return _id;
        }

        [[nodiscard]] std::string to_string() const override {
            auto out = std::ostringstream();
            for (size_t i = 0; i < RODS; i++) {
                uint32_t rconf = _conf[i];
                while (rconf) {
                    uint32_t disc = std::countr_zero(rconf);
                    rconf ^= (1 << disc);
                    out << disc << " ";
                }
                out << "| ";
            }
            return out.str();
        }
    };
};

// explicit instantiation to get type checking
template
class hanoi<2, 1, 1>;

// verify that we implement the `domain` "interface"
static_assert(domain<hanoi<2, 1, 1>>);
