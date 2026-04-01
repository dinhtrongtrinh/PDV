#pragma once

#include <algorithm>
#include <utility>
#include <vector>
#include <iterator>
#include "../pdv_lib/pdv_lib.hpp"

// Timto radkem rikame, ze genericky parametr `It` musi byt neco, co se chova jako "random access iterator", tedy
// iterator, se kterym muzeme iterovat dopredu (++), dozadu (--), skakat (it + 10) i do nej indexovat (it[0], it[10]).
// Takovy iterator je treba pointer do vectoru, nebo jakehokoliv jineho bufferu. Mohli bychom zde klidne napsat jen
// `template <typename It>`, ale v pripade nespravneho parametru bychom dostali hur citelnou chybovou hlasku.
template<typename It>
void mergesort_seq_worker(It begin, It end, It tmp) {
    auto size = end - begin;
    // Pole velikosti 1 nebo 0 je serazene.
    if (size <= 1) {
        return;
    }

    // Ulozime si iterator ("pointer") na stred pole.
    auto middle = begin + size / 2;
    auto tmp_middle = tmp + size / 2;

    // Zavolame rekurzivni volani na levou a pravou polovinu.
    mergesort_seq_worker(begin, middle, tmp);
    mergesort_seq_worker(middle, end, tmp_middle);

    // Vysledky slijeme pomoci operace merge do pomocneho pole `tmp`...
    std::merge(begin, middle, middle, end, tmp);
    // ...a presuneme obsah zpet do puvodniho pole.
    std::move(tmp, tmp + size, begin);
}

template<typename It>
void mergesort_seq(It begin, It end) {
    // type that the iterator refers to (the actual type of values in the array)
    using T = typename std::iterator_traits<It>::value_type;

    // helper vector of the same size as the sorted data, which we'll use as storage for merging
    auto tmp = std::vector<T>(end - begin);

    // start the actual mergesort
    mergesort_seq_worker(begin, end, tmp.begin());
}

template<typename It>
void mergesort_par_worker(It begin, It end, It tmp) {
    auto size = end - begin;
    // Pole velikosti 1 nebo 0 je serazene.
    if (size <= 2000) {
        mergesort_seq_worker(begin, end, tmp);
        return;
    }

    // Ulozime si iterator ("pointer") na stred pole.
    auto middle = begin + size / 2;
    auto tmp_middle = tmp + size / 2;

    // Zavolame rekurzivni volani na levou a pravou polovinu.
    #pragma omp task
    mergesort_par_worker(begin, middle, tmp);
    #pragma omp task
    mergesort_par_worker(middle, end, tmp_middle);

    // Vysledky slijeme pomoci operace merge do pomocneho pole `tmp`...
    #pragma omp taskwait
    std::merge(begin, middle, middle, end, tmp);
    // ...a presuneme obsah zpet do puvodniho pole.
    std::move(tmp, tmp + size, begin);
}

template<typename It>
void mergesort_par(It begin, It end) {
    // type that the iterator refers to (the actual type of values in the array)
    using T = typename std::iterator_traits<It>::value_type;

    // Vytvorime si pomocne pole, do ktereho budeme provadet operaci `merge`.
    auto tmp = std::vector<T>(end - begin);
    #pragma omp parallel
    #pragma omp single
    mergesort_par_worker(begin, end, tmp.begin());

}
