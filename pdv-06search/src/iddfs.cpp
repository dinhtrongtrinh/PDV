#include "iddfs.h"
#include <unordered_map>
#include <set>
#include <format>
#include <bits/stdc++.h>
#include "omp.h"
#include <limits.h>


// Naimplementujte efektivni algoritmus pro nalezeni nejkratsi (respektive nejlevnejsi) cesty v grafu.
// V teto metode mate ze ukol naimplementovat pametove efektivni algoritmus pro prohledavani velkeho stavoveho
// prostoru. Pocitejte s tim, ze Vami navrzeny algoritmus pobezi na stroji s omezenym mnozstvim pameti (radove
// nizke stovky megabytu). Vhodnym pristupem tak muze byt napr. iterative-deepening depth-first search.
//
// Metoda ma za ukol vratit ukazatel na cilovy stav, ktery je dosazitelny pomoci nejkratsi/nejlevnejsi cesty.
// Evaluacni kod muze funkci volat opakovane, dejte si pozor, abyste korektne reinicializovali globalni promenne,
// pokud je pouzivate (idealne se jim vyhnete).


bool found = false;
int modulo_num = 10;
int best_depth = INT_MAX;
state_ptr goal = nullptr;
size_t lock_num = 32;
size_t TABLE_SIZE = 1000;
void recursion(state_ptr root, std::vector<omp_lock_t>& locks, std::vector<std::pair<uint64_t, int>>& transposition_table,int recursion_depth, int global_depth) {
    if (root == nullptr || recursion_depth <= 0) return;
    int current_absolute_depth = global_depth - recursion_depth;
    if (found && current_absolute_depth >= best_depth) {
        return;
    }
    for (state_ptr child : root->next_states()) {

        if (child->goal()) {
            #pragma omp critical
            {
                found = true;
                // Dítě je o 1 krok hlouběji než já
                int child_depth = current_absolute_depth+ 1;


                if (child_depth < best_depth) {
                    best_depth = child_depth;
                    goal = child;
                }
                else if (child_depth == best_depth) {
                    // Teprve teď přichází na řadu "tie-breaker" s nejmenším ID
                    if (goal == nullptr || child->id() < goal->id()) {
                        goal = child;
                    }
                }
            }
        }
        else {
            bool was_added = false;

            // 1. Kam uzel patří ve velké tabulce?
            size_t index = child->id() % TABLE_SIZE;

            // 2. Který z 64 zámků tento index chrání?
            size_t lock_idx = index % lock_num;

            omp_set_lock(&locks[lock_idx]);

            // 3. Vezmeme REFERENCI na daný chlívek
            auto& it = transposition_table[index];

            if (it.first != child->id()) {
                // Nikdo tu není, nebo je tu někdo cizí -> přepíšeme to naším uzlem!
                it.first = child->id();
                it.second = recursion_depth;
                was_added = true;
            } else if (recursion_depth > it.second) {
                // Jsme to my a máme teď lepší hloubku!
                it.second = recursion_depth;
                was_added = true;
            }
            omp_unset_lock(&locks[lock_idx]);

            if (was_added) {
                if (!found || (current_absolute_depth + 1) < best_depth) {
                    if (current_absolute_depth < 3) {
                    #pragma omp task
                        recursion(child, locks, transposition_table, recursion_depth - 1, global_depth);
                    } else {
                        recursion(child, locks, transposition_table, recursion_depth - 1, global_depth);
                    }
                }
            }
        }
    }
    #pragma omp taskwait
}

state_ptr iddfs(state_ptr root) {
    if (root->goal()) return root;
    found = false;
    goal = nullptr;
    best_depth = INT_MAX;
    int depth = 10;
    // Pole párů (uint64_t ID, int depth). Inicializováno na ID=0 a depth=-1.
    std::vector<std::pair<uint64_t, int>> transposition_table(TABLE_SIZE, {0, -1});
    std::vector<omp_lock_t> locks(lock_num);
    for (size_t i = 0; i < lock_num; ++i) {
        omp_init_lock(&locks[i]);
    }

    while (!found) {
        auto first_id = root->id() % TABLE_SIZE;
        transposition_table[first_id] = {root->id(), depth};
        #pragma omp parallel
        {
            #pragma omp single
            recursion(root,locks, transposition_table, depth,depth);
        }

        if (!found) {

            depth += 10; // Násobení 10x by hloubku nafouklo příliš rychle
        }
    }
    for (size_t i = 0; i < lock_num; ++i) {
        omp_destroy_lock(&locks[i]);
    }
    return goal;
}
