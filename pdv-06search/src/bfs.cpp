#include "bfs.h"

#include <iostream>
#include <unordered_set>
#include <omp.h>
#include <cmath>
#include <string>
#include <queue>
#include <unordered_map>
// Naimplementujte efektivni algoritmus pro nalezeni nejkratsi cesty v grafu. V teto metode nemusite prilis
// optimalizovat pametove naroky, a vhodnym algoritmem tak muze byt napriklad pouziti prohledavani do sirky
// (breadth-first search).
//
// Funkce ma za ukol vratit ukazatel na cilovy stav, ktery je dosazitelny pomoci nejkratsi cesty.
// Evaluacni kod muze funkci volat opakovane, dejte si pozor, abyste korektne reinicializovali
// globalni promenne, pokud je pouzivate (idealne se jim vyhnete).
// Pomocná funkce pro výpočet heuristiky z to_string()
int get_manhattan_distance(const std::string& state_str, size_t size_puzzle) {
    int distance = 0;
    int current_index = 0;


    // Procházíme string a hledáme čísla
    for (size_t i = 0; i < state_str.size(); ++i) {
        if (isdigit(state_str[i])) {
            int val = 0;
            // Vytažení celého čísla (může mít 2 cifry, např. 15)
            while (i < state_str.size() && isdigit(state_str[i])) {
                val = val * 10 + (state_str[i] - '0');
                i++;
            }

            // Nulu (prázdné políčko) do Manhattanské vzdálenosti nepočítáme
            if (val != 0) {
                // Výpočet aktuální pozice v 4x4 mřížce
                int current_row = current_index / size_puzzle;
                int current_col = current_index % size_puzzle;


                int target_val = val;
                int target_row = target_val / size_puzzle;
                int target_col = target_val % size_puzzle;

                distance += std::abs(current_row - target_row) + std::abs(current_col - target_col);
            }
            current_index++;
        }
    }
    return distance;
}

struct AStarNode {
    state_ptr state;
    int g_cost;
    int f_cost;

    // C++ priority_queue dává na vrchol největší prvek.
    // My chceme nejmenší f_cost, proto operátor vrací true, když je naše f VĚTŠÍ.
    bool operator>(const AStarNode& other) const {
        if (f_cost == other.f_cost) {
            // Tie-breaker podle ID, jak vyžaduje zadání
            return state->id() > other.state->id();
        }
        return f_cost > other.f_cost;
    }
};




state_ptr bfs(state_ptr root) {
    if (root->goal()) return root;
    size_t lock_num = 4096;
    std:: vector<state_ptr> main_container;
    std::vector<state_ptr> side_container;
    std::vector<std::unordered_set<uint64_t>> visited_shards(lock_num);
    std::vector<omp_lock_t> locks(lock_num);
    for (size_t i = 0; i < lock_num; ++i) {
        visited_shards[i].reserve(1000000 / lock_num);
        omp_init_lock(&locks[i]);
    }
    main_container.push_back(root);
    uint64_t first_root_id = root->id() % lock_num;
    visited_shards[first_root_id].insert(root->id());
    bool found = false;
    state_ptr best_goal = nullptr;
    // fking sliding puzzle
    if (root->to_string()[0] == '[' && root->to_string().size() > 10) {
        size_t size_puzzle = 0;
        for (char c : root->to_string()) {
            if (isdigit(c)) size_puzzle++;
        }
        size_puzzle = size_t(sqrt(size_puzzle));
        std::priority_queue<AStarNode, std::vector<AStarNode>, std::greater<AStarNode>> open_set;
        std::unordered_map<uint64_t, int> g_scores; // Místo pouhého "visited" si pamatujeme nejlepší cenu k uzlu

        open_set.push({root, 0, get_manhattan_distance(root->to_string(), size_puzzle)});
        g_scores[root->id()] = 0;

        while (!open_set.empty()) {
            AStarNode current_node = open_set.top();
            open_set.pop();

            state_ptr current_state = current_node.state;

            // Pokud jsme našli cíl, A* s přípustnou heuristikou garantuje nejkratší cestu
            if (current_state->goal()) {
                return current_state;
            }

            // Pokud jsme tento stav už dříve navštívili s lepší nebo stejnou cenou z jiné cesty, ignorujeme ho
            if (current_node.g_cost > g_scores[current_state->id()]) {
                continue;
            }

            int tentative_g = current_node.g_cost + 1; // Každý tah stojí 1

            for (auto next_state : current_state->next_states()) {
                uint64_t next_id = next_state->id();

                // Pokud jsme uzel ještě neviděli, nebo jsme našli KRATŠÍ cestu k němu
                if (g_scores.find(next_id) == g_scores.end() || tentative_g < g_scores[next_id]) {

                    g_scores[next_id] = tentative_g;
                    int h = get_manhattan_distance(next_state->to_string(),size_puzzle);
                    int f = tentative_g + h;

                    open_set.push({next_state, tentative_g, f});
                }
            }
        }
        return nullptr; // Cesta neexistuje
    }
    else {
        while (!found && !main_container.empty()) {
            side_container.clear();
            #pragma omp parallel shared(found)
            {
                std::vector<state_ptr> thread_vector;
                thread_vector.reserve(1000);
                #pragma omp for schedule(dynamic,64)
                for (size_t i = 0; i < main_container.size(); ++i) {
                    state_ptr current = main_container[i];
                    for (auto next_state : current->next_states()) {
                        if (next_state->goal()) {
                            #pragma omp critical
                            {
                                found = true;
                                if (best_goal != nullptr) {
                                    if (next_state->id() < best_goal->id()) {
                                        best_goal = next_state;
                                    }
                                }
                                else {
                                    best_goal = next_state;
                                }
                            }
                        }
                        else {
                            auto next_state_id = next_state->id() % lock_num;
                            //if the id was not found
                            omp_set_lock(&locks[next_state_id]);
                            if (visited_shards[next_state_id].find(next_state->id()) == visited_shards[next_state_id].end()) {
                                thread_vector.push_back(next_state);
                                visited_shards[next_state_id].insert(next_state->id());
                            }
                            omp_unset_lock(&locks[next_state_id]);
                        }
                    }
                }
                #pragma omp critical
                if (found == false) {
                    side_container.insert(side_container.end(), thread_vector.begin(), thread_vector.end());
                }
            }
            main_container.swap(side_container);
        }
    }
    for (size_t i = 0; i < lock_num; ++i) {
        omp_destroy_lock(&locks[i]);
    }
    return best_goal;
}