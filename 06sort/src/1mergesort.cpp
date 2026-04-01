#include <cstdint>
#include "../pdv_lib/pdv_lib.hpp"
#include "./1mergesort.h"

using ElemT = uint32_t; // Typ prvku, ktere budeme radit
constexpr size_t N_ELEMS = 15'000'000; // Pocet prvku k serazeni
constexpr ElemT MIN_VALUE = 0; // Minimalni prvek v neserazenem poli
constexpr ElemT MAX_VALUE = 8191; // Maximalni prvek v neserazenem poli

// Macro, ktere vytvori promennou, do ktere se automaticky ulozi prvni vystup testu, a specializaci interni funkce,
// ktera se zavola pro kazdy vystup testu a porovna ho s ulozenym referencnim resenim. Diky tomuto radku se u kazdeho
// testu vypise, zda vratil spravny vysledek.
PDV_DEFINE_RESULT_COMPARER(std::vector<ElemT>, ref_solution)

// Evaluacni funkce
void eval(std::string_view name, const std::vector<ElemT>& data, void (* sorter)(std::vector<ElemT>&)) {
    auto copy = data;
    pdv::benchmark(name, [&] {
        pdv::do_not_optimize_away(copy);
        sorter(copy);
        return std::move(copy);
    });
}

int main() {
    // Nagenerujeme si pole velikosti N_ELEMS a ulozime do nej nahodne prvky z rozsahu [MIN_VALUE .. MAX_VALUE].
    auto unsorted = pdv::generate_random_vector<ElemT>(N_ELEMS, MIN_VALUE, MAX_VALUE);

    eval("mergesort_seq", unsorted, [](auto& vec) { mergesort_seq(vec.begin(), vec.end()); });
    eval("mergesort_par", unsorted, [](auto& vec) { mergesort_par(vec.begin(), vec.end()); });
    eval("std::sort", unsorted, [](auto& vec) { std::sort(vec.begin(), vec.end()); });
}
