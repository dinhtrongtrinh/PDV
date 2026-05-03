#include "iddfs_weighted.h"
#include <vector>
#include <algorithm>
#include <cstdint>
#include "omp.h"

uint64_t recursion_sequential(state_ptr node,
                              uint64_t cost_limit,
                              bool& found,
                              state_ptr& goal,
                              uint64_t& best_cost) {
    if (node == nullptr) return UINT64_MAX;

    // Rychlé atomické čtení pro včasné ukončení
    bool current_found;
    uint64_t current_best;
    #pragma omp atomic read
    current_found = found;
    #pragma omp atomic read
    current_best = best_cost;

    if (current_found && node->total_cost() >= current_best) return UINT64_MAX;

    auto children = node->next_states();

    // Heuristika
    std::sort(children.begin(), children.end(), [](const state_ptr& a, const state_ptr& b) {
        return a->total_cost() < b->total_cost();
    });

    uint64_t local_next_limit = UINT64_MAX;

    for (const auto& child : children) {
        uint64_t child_cost = child->total_cost();

        if (child_cost > cost_limit) {
            if (child_cost < local_next_limit) {
                local_next_limit = child_cost;
            }
            continue;
        }

        if (child->goal()) {
            // Nalezení cíle je vzácné, zde je critical sekce v pořádku
            #pragma omp critical(goal_update)
            {
                if (child_cost < best_cost) {
                    best_cost = child_cost;
                    goal = child;
                    found = true;
                }
                else if (child_cost == best_cost) {
                    if (goal == nullptr || child->id() < goal->id()) {
                        goal = child;
                    }
                }
            }
            continue;
        }

        if (!found || child_cost < best_cost) {
            uint64_t child_limit = recursion_sequential(child, cost_limit, found, goal, best_cost);
            if (child_limit < local_next_limit) {
                local_next_limit = child_limit;
            }
        }
    }

    return local_next_limit;
}

void recursion_parallel(state_ptr node,
                        uint64_t cost_limit,
                        uint64_t& global_next_cost_limit,
                        bool& found,
                        state_ptr& goal,
                        uint64_t& best_cost,
                        int depth) {
    if (node == nullptr) return;

    bool current_found;
    uint64_t current_best;
    #pragma omp atomic read
    current_found = found;
    #pragma omp atomic read
    current_best = best_cost;

    if (current_found && node->total_cost() >= current_best) return;

    auto children = node->next_states();

    std::sort(children.begin(), children.end(), [](const state_ptr& a, const state_ptr& b) {
        return a->total_cost() < b->total_cost();
    });

    for (const auto& child : children) {
        uint64_t child_cost = child->total_cost();

        if (found && (child_cost * 100 > best_cost * 45)) {
            continue;
        }

        if (child_cost > cost_limit) {
            #pragma omp critical(limit_update)
            {
                if (child_cost < global_next_cost_limit) {
                    global_next_cost_limit = child_cost;
                }
            }
            continue;
        }

        if (child->goal()) {
            #pragma omp critical(goal_update)
            {
                if (child_cost < best_cost) {
                    best_cost = child_cost;
                    goal = child;
                    found = true;
                }
                else if (child_cost == best_cost) {
                    if (goal == nullptr || child->id() < goal->id()) {
                        goal = child;
                    }
                }
            }
            continue;
        }

        if (!found || child_cost < best_cost) {
            bool enough_budget = (child_cost * 100) < (cost_limit * 45);
            if (depth < 5 && enough_budget) {
                // explicitní firstprivate(child) pro bezpečné předání do tasku
                #pragma omp task shared(global_next_cost_limit, found, goal, best_cost) firstprivate(child, child_cost, depth)
                recursion_parallel(child, cost_limit, global_next_cost_limit, found, goal, best_cost, depth + 1);
            } else {
                // Přepnutí do ultra-rychlé sekvenční větve
                uint64_t seq_limit = recursion_sequential(child, cost_limit, found, goal, best_cost);

                // Zápis do globálního limitu jen JEDNOU za celou pod-větev!
                if (seq_limit < UINT64_MAX) {
                    #pragma omp critical(limit_update)
                    {
                        if (seq_limit < global_next_cost_limit) {
                            global_next_cost_limit = seq_limit;
                        }
                    }
                }
            }
        }
    }
    #pragma omp taskwait
}

state_ptr iddfs_weighted(state_ptr root) {
    if (root->goal()) return root;

    bool found = false;
    state_ptr goal = nullptr;
    uint64_t best_cost = UINT64_MAX;

    uint64_t cost_limit = root->total_cost();

    while (!found) {
        uint64_t next_cost_limit = UINT64_MAX;

        #pragma omp parallel shared(next_cost_limit, found, goal, best_cost)
        {
            #pragma omp single
            recursion_parallel(root, cost_limit, next_cost_limit, found, goal, best_cost, 0);
        }

        if (!found) {
            if (next_cost_limit == UINT64_MAX) break;
            cost_limit = next_cost_limit;
        }
    }

    return goal;
}