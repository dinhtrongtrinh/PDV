#pragma once

#include <sstream>
#include <cmath>
#include <random>
#include <iostream>
#include <utility>
#include "../state.h"

// NOTE: this code is very non-idiomatic from a performance perspective; however, making the domain faster
//  would make it much harder for you to get a reasonable speedup over the serial solution

/// A domain of WIDTH*HEIGHT 2D maze with a fixed starting and goal position. The state corresponds to a position
/// in the maze, each action corresponds to moving in the selected direction, unless blocked by a wall.
template<size_t WIDTH, size_t HEIGHT, std::mt19937::result_type SEED, bool UNIFORM>
class maze {
    static_assert(WIDTH % 2 == 1, "Sirka bludiste musi byt licha");
    static_assert(HEIGHT % 2 == 1, "Vyska bludiste musi byt licha");

    static constexpr float COMPLEXITY = 0.9;
    static constexpr float DENSITY = 0.75;

private:
    std::vector<bool> _maze;
    std::vector<size_t> _start;
    std::vector<size_t> _goal;

public:
    maze() {
        _start = std::vector<size_t>{1, 1};
        _goal = std::vector<size_t>{HEIGHT - 2, WIDTH - 2};

        generate_maze(SEED);

        for (int i = 0; i < 10; i++) {
            if (!_maze[_start[0] * WIDTH + _start[1]] && !_maze[_goal[0] * WIDTH + _goal[1]]) {
                break;
            }
            std::cerr << "Spatne vygenerovane bludiste #" << i << ". Generuji znovu...\n";
            _goal[0]--;
            _goal[1]--;
            generate_maze(SEED + 1 + i);
        }

        if (_maze[_start[0] * WIDTH + _start[1]] || _maze[_goal[0] * WIDTH + _goal[1]]) {
            throw std::runtime_error("Could not generate a valid maze after 10 attempts. "
                                     "Change the domain parameters and try again.\n");
        }
    }

    [[nodiscard]] state_ptr root() const {
        return std::make_shared<dstate>(state_ptr(), 0, _start, &_maze, &_goal);
    }

    friend std::ostream& operator<<(std::ostream& s, const maze& d) {
        s << "Domain: 2D Maze\n";
        s << "Width = " << WIDTH << ", height = " << HEIGHT << ", uniform costs = " << (UNIFORM ? "yes" : "no")
          << ", seed = " << SEED << "\n";
        s << "Map = ";

        if (WIDTH >= 64 && HEIGHT >= 64) {
            s << "<skipped, maze is too large to render>\n";
            return s;
        }

        s << "\n";

        for (size_t i = 0; i < HEIGHT; i++) {
            for (size_t j = 0; j < WIDTH; j++) {
                if (i == d._start[0] && j == d._start[1]) {
                    s << "S";
                    continue;
                }
                if (i == d._goal[0] && j == d._goal[1]) {
                    s << "G";
                    continue;
                }
                s << (d._maze[i * WIDTH + j] ? "X" : " ");
            }
            s << "\n";
        }
        return s;
    }

private:
    void generate_maze(std::mt19937::result_type seed) {
        auto rng = std::mt19937(seed);

        size_t shape[2] = {(WIDTH / 2) * 2 + 1, (HEIGHT / 2) * 2 + 1};
        // Adjust complexity and density relative to maze size
        auto complexity = (size_t) (COMPLEXITY * (float) (5 * (shape[0] + shape[1])));
        auto density = (size_t) (DENSITY * (float) ((shape[0] / 2) * (shape[1] / 2)));
        // Build actual maze
        _maze = std::vector<bool>(WIDTH * HEIGHT);
        // Fill borders
        for (size_t i = 0; i < WIDTH; i++) {
            _maze[i] = true;
            _maze[WIDTH * (HEIGHT - 1) + i] = true;
        }
        for (size_t j = 0; j < HEIGHT; j++) {
            _maze[j * WIDTH] = true;
            _maze[j * WIDTH + WIDTH - 1] = true;
        }

        // Make aisles
        auto uniX = std::uniform_int_distribution<size_t>(0, shape[1] / 2);
        auto uniY = std::uniform_int_distribution<size_t>(0, shape[0] / 2);
        for (size_t i = 0; i < density; i++) {
            size_t x = 2 * uniX(rng);
            size_t y = 2 * uniY(rng);
            _maze[x * WIDTH + y] = true;
            for (size_t j = 0; j < complexity; j++) {
                auto neighbours = std::vector<size_t>();
                if (x > 1) {
                    neighbours.push_back(y);
                    neighbours.push_back(x - 2);
                }
                if (x < shape[1] - 2) {
                    neighbours.push_back(y);
                    neighbours.push_back(x + 2);
                }
                if (y > 1) {
                    neighbours.push_back(y - 2);
                    neighbours.push_back(x);
                }
                if (y < shape[0] - 2) {
                    neighbours.push_back(y + 2);
                    neighbours.push_back(x);
                }
                if (!neighbours.empty()) {
                    auto uni = std::uniform_int_distribution<int>(0, (int) (neighbours.size() / 2) - 1);
                    int ridx = uni(rng);
                    auto y_ = neighbours[2 * ridx];
                    auto x_ = neighbours[2 * ridx + 1];
                    if (!_maze[x_ * WIDTH + y_]) {
                        _maze[x_ * WIDTH + y_] = true;
                        _maze[(x_ + (int) ((x - x_) / 2)) * WIDTH + y_ + (int) ((y - y_) / 2)] = true;
                        x = x_;
                        y = y_;
                    }
                }
            }
        }
    }

private:
    class dstate final : public state, public std::enable_shared_from_this<dstate> {
    private:
        std::vector<size_t> _position;
        uint64_t _id;

        const std::vector<bool>* _maze;
        const std::vector<size_t>* _goal;

    public:
        dstate(state_ptr predecessor, uint64_t cost, std::vector<size_t> conf,
               const std::vector<bool>* maze, const std::vector<size_t>* goal)
            : state(std::move(predecessor), cost), _position(conf), _maze(maze), _goal(goal) {
            _id = conf[0] * WIDTH + conf[1];
        }

        [[nodiscard]] std::vector<state_ptr> next_states() const override {
            auto new_position = _position;
            auto succ = std::vector<state_ptr>();

            uint64_t cost = UNIFORM ? 1 : (_id % 5);
            if (!((*_maze)[(_position[0] - 1) * WIDTH + _position[1]])) {
                new_position[0] = _position[0] - 1;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + cost, new_position, _maze, _goal));
                new_position[0] = _position[0];
            }

            if (!((*_maze)[(_position[0] + 1) * WIDTH + _position[1]])) {
                new_position[0] = _position[0] + 1;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + cost, new_position, _maze, _goal));
                new_position[0] = _position[0];
            }

            if (!((*_maze)[(_position[0]) * WIDTH + _position[1] - 1])) {
                new_position[1] = _position[1] - 1;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + cost, new_position, _maze, _goal));
                new_position[1] = _position[1];
            }

            if (!((*_maze)[(_position[0]) * WIDTH + _position[1] + 1])) {
                new_position[1] = _position[1] + 1;
                succ.emplace_back(std::make_shared<dstate>(
                    this->shared_from_this(), total_cost() + cost, new_position, _maze, _goal));
                new_position[1] = _position[1];
            }

            return succ;
        }

        [[nodiscard]] bool goal() const override {
            return _position[0] == (*_goal)[0] && _position[1] == (*_goal)[1];
        }

        [[nodiscard]] uint64_t id() const override {
            return _id;
        }

        [[nodiscard]] std::string to_string() const override {
            auto out = std::ostringstream{};
            out << "[ " << _position[0] << ", " << _position[1] << " ]";
            return out.str();
        }
    };
};

// explicit instantiation to get type checking
template
class maze<1, 1, 0, true>;

// verify that we implement the `domain` "interface"
static_assert(domain<maze<1, 1, 0, true>>);
