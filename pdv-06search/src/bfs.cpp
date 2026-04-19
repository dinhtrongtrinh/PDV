#include "bfs.h"
#include <unordered_set>
#include <omp.h>
// Naimplementujte efektivni algoritmus pro nalezeni nejkratsi cesty v grafu. V teto metode nemusite prilis
// optimalizovat pametove naroky, a vhodnym algoritmem tak muze byt napriklad pouziti prohledavani do sirky
// (breadth-first search).
//
// Funkce ma za ukol vratit ukazatel na cilovy stav, ktery je dosazitelny pomoci nejkratsi cesty.
// Evaluacni kod muze funkci volat opakovane, dejte si pozor, abyste korektne reinicializovali
// globalni promenne, pokud je pouzivate (idealne se jim vyhnete).
state_ptr bfs(state_ptr root) { // NOLINT(*-unnecessary-value-param)
    size_t lock_num = 256;
    std:: vector<state_ptr> main_container;
    std::vector<state_ptr> side_container;
    std::vector<std::unordered_set<uint64_t>> visited_shards(lock_num);
    std::vector<omp_lock_t> locks(lock_num);
    for (size_t i = 0; i < lock_num; ++i) {
        omp_init_lock(&locks[i]);
    }
    if (root->goal()) return root;
    main_container.push_back(root);
    uint64_t first_root_id = root->id() % lock_num;
    visited_shards[first_root_id].insert(root->id());
    bool found = false;
    state_ptr best_goal = nullptr;
    while (!found && !main_container.empty()) {
        side_container.clear();
        #pragma omp parallel shared(found)
        {
            std::vector<state_ptr> thread_vector;
            thread_vector.reserve(1000);
            #pragma omp for schedule(dynamic, 64)
            for (size_t i = 0; i < main_container.size(); ++i) {
                if (found) continue;
                state_ptr current = main_container[i];
                for (auto next_state : current->next_states()) {
                    if (found) break;
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
                    auto next_state_id = next_state->id() % lock_num;
                    //if the id was not found
                    if (visited_shards[next_state_id].find(next_state->id()) == visited_shards[next_state_id].end()) {
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
            side_container.insert(side_container.end(), thread_vector.begin(), thread_vector.end());

        }

        if (found) {
            break;
        }
        main_container.swap(side_container);


    }
    for (size_t i = 0; i < lock_num; ++i) {
        omp_destroy_lock(&locks[i]);
    }
    return best_goal;
}