#pragma once

#include "bst_tree.h"

#include <vector>
#include <numeric>
#include <algorithm>
#include <functional>
#include <cstdlib>
#include <cstdint>
#include <random>

// Oba nase testy se skladaji ze vkladani sekvence cisel 0..N-1 do Vaseho binarniho vyhledavaciho
// stromu (at uz v serazenem nebo neserazenem poradi). Oba tyto testy jsou v zakladu velice podob-
// ne, a implementujeme proto na zaklade stejne tridy 'base_test'.
template<size_t N>
class base_test {
public:
    std::vector<int64_t> data; // Data, ktera budeme do stromu vkladat
    bst_tree tree{}; // Instance stromu

    base_test() : data(N) {
        // Vytvorime si sekvenci cisel 0..N-1
        std::iota(data.begin(), data.end(), 0);
    }

    void run_test() {
        // Test se sklada ze vkladani prvku vektoru 'data' do binarniho vyhledavaciho stromu paralelne
        // vice vlakny. Vsimnete si, ze pouzivame dynamicky scheduling, protoze chceme alespon castecne
        // dodrzet poradi vkladani prvku do stromu. (Pokud bychom pouzili staticky scheduling, vlakna
        // s vyssim indexem by zacinala se vkladanim prvku z prostredku pole, presneji na indexech
        // k*N/omp_get_num_threads()).
        #pragma omp parallel for schedule(dynamic)
        for (auto n : data) {
            tree.insert(n);
        }
    }

    bool verify() {
        // Strom prochazime v poradi inorder a jeho prvky si vkladame do pole. Diky vlastnostem binarniho
        // vyhledavaciho stromu by vysledkem melo byt serazene pole.
        std::vector<int64_t> content;
        std::function<void(bst_tree::node*)> inorder = [&](bst_tree::node* node) {
            if (node != nullptr) {
                inorder(node->left);
                content.push_back(node->data);
                inorder(node->right);
            }
        };
        inorder(tree.root);

        // Nyni zkontrolujeme, ze pole 'content' skutecne obsahuje serazena cisla 0..N-1
        if (content.size() != N) return false;
        for (size_t i = 0; i < N; i++) {
            if (content[i] != (int64_t)i) return false;
        }
        return true;
    }
};

// Test 'shuffled_data' se od zakladniho testu lisi pouze tim, ze vstupni data, ktera se vkladaji do
// binarniho vyhledavaciho stromu jsou zprehazena.
template<size_t N>
class shuffled_data : public base_test<N> {
public:
    shuffled_data() {
        std::shuffle(this->data.begin(), this->data.end(), std::mt19937(0)); // NOLINT(*-msc51-cpp)
    }
};

// Test se serazenymi daty je identicky jako zakladni test.
template<size_t N>
class sorted_data : public base_test<N> {
public:
    // without this explicitly defined constructor, GCC 10.2 throws a nonsense error message
    //  about a missing move constructor, although it should be equivalent
    sorted_data() = default;
};
