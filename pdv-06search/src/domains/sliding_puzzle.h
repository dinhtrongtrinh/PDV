#pragma once

#include <sstream>
#include <cmath>
#include <random>
#include <utility>
#include "../state.h"

// NOTE: this code is very non-idiomatic from a performance perspective; however, making the domain faster
//  would make it much harder for you to get a reasonable speedup over the serial solution

/// Domain of the sliding puzzle game, where you have a square board with a single empty square, and your goal is to
/// get the numbered boxes in the remaining squares into the correct order. Each action corresponds to moving all
/// squares that are able to move in a selected direction.
template<size_t SIZE, size_t SOLUTION_DEPTH, std::mt19937::result_type SEED>
class sliding_puzzle {
    std::vector<size_t> _starting_state{};

public:
    sliding_puzzle() {
        auto starting_state = std::vector<size_t>(SIZE * SIZE);
        for (size_t i = 0; i < SIZE; i++) {
            for (size_t k = 0; k < SIZE; k++) {
                starting_state[i * SIZE + k] = i * SIZE + k;
            }
        }

        // do SOLUTION_DEPTH random moves from the solved position
        auto rng = std::mt19937(SEED); // NOLINT(*-msc51-cpp)
        auto uni = std::uniform_int_distribution<int>(0, 3);
        state_ptr s = std::make_shared<const dstate>(state_ptr(), 0, starting_state);
        for (size_t i = 0; i < SOLUTION_DEPTH; i++) {
            auto next_states = s->next_states();
            s = next_states[uni(rng) % next_states.size()];
        }

        auto ss = std::static_pointer_cast<const dstate>(s);
        _starting_state = ss->config();
    }

    [[nodiscard]] state_ptr root() const {
        return std::make_shared<const dstate>(state_ptr(), 0, _starting_state);
    }

    friend std::ostream& operator<<(std::ostream& s, const sliding_puzzle& d) {
        s << "Domain: Sliding puzzle\n";
        s << "Size = " << SIZE << "*" << SIZE << ", seed = " << SEED << "\n";
        // not very efficient, but whatever
        s << "Initial layout = " << d.root()->to_string() << "\n";
        return s;
    }

private:
    class dstate final : public state, public std::enable_shared_from_this<dstate> {
    private:
        std::vector<size_t> _conf;
        uint64_t _id;

        const size_t BLANK = SIZE * SIZE - 1;

    public:
        dstate(state_ptr predecessor, uint64_t cost, std::vector<size_t> conf)
            : state(std::move(predecessor), cost), _conf(std::move(conf)) {
            _id = 0;
            uint64_t id2 = 0;
            uint64_t mult = 1;
            for (size_t i = 0; i < SIZE; i++) {
                for (size_t j = 0; j < SIZE; j++) {
                    _id += mult * _conf[i * SIZE + j];
                    id2 += pow(BLANK + 1, i * SIZE + j) * _conf[i * SIZE + j];
                    mult *= BLANK + 1;
                }
            }

            auto vmin = std::min<uint64_t>(_id, id2);
            _id = std::max<uint64_t>(vmin, _id);
        }

        [[nodiscard]] std::vector<state_ptr> next_states() const override {
            auto tmp_conf = _conf;
            auto succ = std::vector<state_ptr>();

            auto blank_x = 0;
            auto blank_y = 0;

            // find blank
            for (size_t i = 0; i < SIZE; i++) {
                for (size_t k = 0; k < SIZE; k++) {
                    if (_conf[i * SIZE + k] == BLANK) {
                        blank_x = (int) i;
                        blank_y = (int) k;
                    }
                }
            }

            // 4 possibilities
            if (blank_x - 1 >= 0) {
                tmp_conf[blank_x * SIZE + blank_y] = _conf[(blank_x - 1) * SIZE + blank_y];
                tmp_conf[(blank_x - 1) * SIZE + blank_y] = BLANK;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + 1, tmp_conf));
                tmp_conf[blank_x * SIZE + blank_y] = _conf[blank_x * SIZE + blank_y];
                tmp_conf[(blank_x - 1) * SIZE + blank_y] = _conf[(blank_x - 1) * SIZE + blank_y];
            }

            if (blank_x + 1 < (int) SIZE) {
                tmp_conf[blank_x * SIZE + blank_y] = _conf[(blank_x + 1) * SIZE + blank_y];
                tmp_conf[(blank_x + 1) * SIZE + blank_y] = BLANK;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + 1, tmp_conf));
                tmp_conf[blank_x * SIZE + blank_y] = _conf[blank_x * SIZE + blank_y];
                tmp_conf[(blank_x + 1) * SIZE + blank_y] = _conf[(blank_x + 1) * SIZE + blank_y];
            }

            if (blank_y - 1 >= 0) {
                tmp_conf[blank_x * SIZE + blank_y] = _conf[blank_x * SIZE + blank_y - 1];
                tmp_conf[blank_x * SIZE + blank_y - 1] = BLANK;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + 1, tmp_conf));
                tmp_conf[blank_x * SIZE + blank_y] = _conf[blank_x * SIZE + blank_y];
                tmp_conf[blank_x * SIZE + blank_y - 1] = _conf[blank_x * SIZE + blank_y - 1];
            }

            if (blank_y + 1 < (int) SIZE) {
                tmp_conf[blank_x * SIZE + blank_y] = _conf[blank_x * SIZE + blank_y + 1];
                tmp_conf[blank_x * SIZE + blank_y + 1] = BLANK;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + 1, tmp_conf));
            }
            return succ;
        }

        [[nodiscard]] bool goal() const override {
            for (size_t i = 0; i < SIZE; i++) {
                for (size_t k = 0; k < SIZE; k++) {
                    if (i == SIZE - 1 && k == SIZE - 1) return true;
                    if (_conf[i * SIZE + k] != i * SIZE + k) return false;
                }
            }
            return false;
        }

        [[nodiscard]] uint64_t id() const override {
            return _id;
        }

        [[nodiscard]] std::string to_string() const override {
            auto out = std::ostringstream();
            out << "[ ";
            for (size_t i = 0; i < SIZE; i++) {
                for (size_t j = 0; j < SIZE; j++) {
                    out << _conf[i * SIZE + j] << " ";
                }
                if (i != SIZE - 1) out << "| ";
                else out << "]";
            }
            return out.str();
        }

        [[nodiscard]] std::vector<size_t> config() const {
            return _conf;
        }
    };
};

// explicit instantiation to get type checking
template
class sliding_puzzle<1, 1, 0>;

// verify that we implement the `domain` "interface"
static_assert(domain<sliding_puzzle<1, 1, 0>>);
