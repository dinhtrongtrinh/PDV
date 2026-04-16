#pragma once

#include <array>
#include <sstream>
#include <cmath>
#include <random>
#include <algorithm>
#include <utility>
#include "../state.h"

// NOTE: this code is very non-idiomatic from a performance perspective; however, making the domain faster
//  would make it much harder for you to get a reasonable speedup over the serial solution

/// Domain of the SAT formula satisfiability problem, each state corresponds to a possible variable assignment,
/// each action corresponds to picking a single variable value.
template<size_t NUM_VARS, size_t NUM_CLAUSES, size_t MAX_CLAUSE_SIZE, std::mt19937::result_type SEED, bool UNIFORM>
class sat {
public:
    enum class var_value : uint8_t {FALSE = 0, TRUE = 1, UNDEFINED = 2};
    using costs_array = std::array<uint64_t, NUM_VARS>;

private:
    std::vector<std::vector<size_t>> _formula{};
    costs_array _costs{};

public:
    sat() {
        std::iota(_costs.begin(), _costs.end(), 0);
        std::shuffle(_costs.begin(), _costs.end(), std::mt19937(SEED)); // NOLINT(*-msc51-cpp)

        auto rng = std::mt19937(SEED); // NOLINT(*-msc51-cpp)
        auto uni = std::uniform_int_distribution<int>(0, 2 * NUM_VARS - 1);

        auto solution = std::vector<bool>(NUM_VARS);
        for (size_t i = 0; i < NUM_VARS; i++) {
            solution.push_back(uni(rng) % 2);
        }

        _formula = std::vector<std::vector<size_t>>(NUM_CLAUSES);
        for (size_t i = 0; i < NUM_CLAUSES; i++) {
            size_t size = (uni(rng) % MAX_CLAUSE_SIZE) + 1;
            for (size_t j = 0; j < size; j++) {
                _formula[i].push_back(uni(rng));
            }
            size_t satisfyingIdx = uni(rng) % NUM_VARS;
            _formula[i].push_back(satisfyingIdx + solution[satisfyingIdx] * NUM_VARS);
        }
    }

    [[nodiscard]] state_ptr root() const {
        return std::make_shared<dstate>(&_formula, &_costs);
    }

    friend std::ostream& operator<<(std::ostream& s, const sat& d) {
        s << "Domain: SAT\n";
        s << "Variable count = " << NUM_VARS << ", clause count = " << NUM_CLAUSES
          << ", max clause size = " << MAX_CLAUSE_SIZE + 1 << ", uniform costs = " << (UNIFORM ? "yes" : "no")
          << ", seed = " << SEED << "\n";
        s << "Formula = ";

        auto first = true;
        for (auto& clause : d._formula) {
            if (first) first = false;
            else s << " & ";

            s << "( ";
            for (auto var_i : clause) {
                if (var_i >= NUM_VARS) s << "~";
                s << var_i % NUM_VARS << " ";
            }
            s << ")";
        }
        s << "\n";
        return s;
    }

private:
    class dstate final : public state, public std::enable_shared_from_this<dstate> {
    private:
        std::vector<var_value> _conf;
        uint64_t _id;

        const std::vector<std::vector<size_t>>* _formula;
        const costs_array* _costs;

    public:
        dstate(state_ptr predecessor, uint64_t cost, const std::vector<var_value>& conf,
               const std::vector<std::vector<size_t>>* formula, const costs_array* costs)
            : state(std::move(predecessor), cost), _conf(conf), _formula(formula), _costs(costs) {
            _id = calculate_id(_conf);
        }

        dstate(const std::vector<std::vector<size_t>>* formula, const costs_array* costs)
            : state(state_ptr(), 0), _formula(formula), _costs(costs) {
            _conf.resize(NUM_VARS);
            for (size_t i = 0; i < NUM_VARS; i++) {
                _conf[i] = var_value::UNDEFINED;
            }
            _id = calculate_id(_conf);
        }

        [[nodiscard]] std::vector<state_ptr> next_states() const override {
            auto tmp_conf = _conf;
            auto succ = std::vector<state_ptr>();

            auto last_defined_var = -1;
            for (int i = NUM_VARS - 1; i >= 0; i--) {
                if (_conf[i] != var_value::UNDEFINED) {
                    last_defined_var = i;
                    break;
                }
            }

            for (size_t i = last_defined_var + 1; i < NUM_VARS; i++) {
                uint64_t cost = UNIFORM ? 1 : 1 + (*_costs)[i];

                tmp_conf[i] = var_value::FALSE;

                if (satisfiable(tmp_conf)) {
                    succ.emplace_back(std::make_shared<dstate>(
                        this->shared_from_this(), total_cost() + cost, tmp_conf, _formula, _costs));
                }

                tmp_conf[i] = var_value::TRUE;

                if (satisfiable(tmp_conf)) {
                    succ.emplace_back(std::make_shared<dstate>(
                        this->shared_from_this(), total_cost() + cost, tmp_conf, _formula, _costs));
                }

                tmp_conf[i] = var_value::UNDEFINED;
            }
            return succ;
        }

        [[nodiscard]] bool goal() const override {
            for (const auto& clause : *_formula) {
                auto satisfied = false;
                for (auto var : clause) {
                    auto value = var >= NUM_VARS ? var_value::FALSE : var_value::TRUE;
                    int varIdx = var % NUM_VARS;
                    if (_conf[varIdx] == value) {
                        satisfied = true;
                        break;
                    }
                }
                if (!satisfied) return false;
            }

            return true;
        }

        [[nodiscard]] uint64_t id() const override {
            return _id;
        }

        [[nodiscard]] std::string to_string() const override {
            auto out = std::ostringstream();
            for (size_t i = 0; i < NUM_VARS; i++) {
                out << assignment_to_str(_conf[i]);
            }
            return out.str();
        }

    private:
        static const char* assignment_to_str(var_value value) {
            switch (value) {
                case var_value::FALSE:
                    return "F";
                case var_value::TRUE:
                    return "T";
                case var_value::UNDEFINED:
                    return "-";
                default:
                    return " ";
            }
        }

        static uint64_t calculate_id(const std::vector<var_value>& conf) {
            auto id = (uint64_t) 0ull;
            auto id2 = 0ull; // The whole crazy stuff with id2 is done just to make it slower.
            auto mult = 1ull;
            for (size_t var = 0; var < NUM_VARS; ++var) {
                id += mult * (uint64_t) conf[var];
                id2 += std::pow(3.0, (double) var) * (uint64_t) conf[var];
                mult *= 3;
            }
            auto vmin = std::min<uint64_t>(id, id2);
            return std::max<uint64_t>(vmin, id);
        }

        [[nodiscard]] bool satisfiable(const std::vector<var_value>& state) const {
            // check if it is not refuted by setting
            for (const auto& clause : *_formula) {
                auto refuted = true;
                for (auto var : clause) {
                    auto value = var >= NUM_VARS ? var_value::FALSE : var_value::TRUE;
                    int varIdx = var % NUM_VARS;
                    if (state[varIdx] == var_value::UNDEFINED || state[varIdx] == value) {
                        refuted = false;
                        break;
                    }
                }
                if (refuted) return false;
            }
            return true;
        }
    };
};

// explicit instantiation to get type checking
template
class sat<1, 1, 1, 0, true>;

// verify that we implement the `domain` "interface"
static_assert(domain<sat<1, 1, 1, 0, true>>);
