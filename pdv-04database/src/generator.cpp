#include "generator.h"

#include <algorithm>
#include <chrono>
#include <limits>
#include <random>

using Predicate = std::function<bool(uint32_t)>;

// generator nahodnych cisel (Mersenne-Twister)
auto rng = std::mt19937();

// Funkce simulujici kontrolu predikatu. V realnych podminkach neni kontrola platnosti predikatu okamzita,
// napriklad je potreba provest operaci s daty ulozenymi na disku.
auto predicate_checking_simulation_function() {
    auto begin = std::chrono::steady_clock::now();
    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (duration_cast<std::chrono::microseconds>(now - begin).count() >= 2L) break; // Pockame alespon 2us na data
    }
}

template<typename NumT>
auto generate_shuffled_sequence(size_t count) -> std::vector<NumT> {
    // vytvorime data v tabulce. v tabulce jsou hodnoty od 0 az (count - 1).
    auto data = std::vector<NumT>(count);
    std::iota(data.begin(), data.end(), 0);
    std::shuffle(data.begin(), data.end(), rng);
    return data;
}

/// Naplni vektor boolovskych hodnot truth/false hodnotami a zajisti, ze aspon jedna hodnota bude true/false.
///
/// ppt_of_true - pravdepodobnost, ze libovolny prvek vektoru bude true
/// to_fill - vektor, ktery se ma naplnit
/// bool_value_to_ensure - zajisti ze ve vektoru bude aspon jedno true/false
auto generate_random_bool_vec(size_t size, double ppt_of_true, bool bool_value_to_ensure) -> std::vector<uint8_t> {
    // don't use vector<bool> due to specialization
    auto to_fill = std::vector<uint8_t>(size);

    // nase distribuce cisel
    auto dis = std::uniform_real_distribution(0.0, 1.0);

    // chceme zarucit, ze se vyskytne aspon jednou bool_value_to_ensure v datech
    auto is_at_least_one_predicate_satisfied = false;

    // nagenerujeme, ktere prvky jsou pravdive
    for (size_t i = 0; i < size; ++i) {
        // bude tento prvek nastaveny na true
        to_fill[i] = dis(rng) < ppt_of_true;

        // kontrola, zda jsme jiz vygnerovali aspon jednu bool hodnotu, kterou ma vektor aspon jednou obsahovat
        if (to_fill[i] == bool_value_to_ensure) {
            is_at_least_one_predicate_satisfied = true;
        }
    }

    // vygenerovali jsme aspon jeden prvek, ktery odpovida bool_value_to_ensure?
    if (!is_at_least_one_predicate_satisfied) {
        auto pos = std::uniform_int_distribution<size_t>(0, to_fill.size() - 1);
        to_fill[pos(rng)] = bool_value_to_ensure;
    }
    return to_fill;
}

// generuje data pro disjunkci/konjunkci. evaluace dotazu je true v pripade disjunkce a false u konjunkce
auto generate_instance_with_query(const std::vector<uint32_t>& data, size_t query_count,
                                  bool bool_value_to_ensure, double t_probability_predicate) {
    if (query_count > data.size()) {
        throw std::invalid_argument("there should be more rows than queries");
    }

    // ktere query maji byt splnene
    auto is_predicate_true =
        generate_random_bool_vec(query_count, t_probability_predicate, bool_value_to_ensure);

    // nahodne urcime, ktere hodnoty se maji brat pro predikat - na jakem radku tabulky bude predikat splnen,
    // pokud bude splnen
    auto indices = generate_shuffled_sequence<size_t>(data.size());

    // vytvorime predikaty pro nasi tabulku
    auto predicates = std::vector<Predicate>(query_count);
    for (size_t i = 0; i < query_count; ++i) {
        // pokud ma byt predikat splnen, vybereme hodnotu v tabulce, kde ma byt splneny. V opacnem pripade pouzijeme
        // hodnotu, ktera v tabulce urcite neni.
        auto value_t = is_predicate_true[i] ? data[indices[i]] : std::numeric_limits<uint32_t>::max();

        // nas predikat ma podobu lambda funkce
        predicates[i] = [value_t](uint32_t value) {
            // FIXME: tohle by potencialne optimizer mohl vyhodit
            predicate_checking_simulation_function();
            // pouze pokud je hodnota rovna ocekavane hodnote, je predikat splnen
            return value == value_t;
        };
    }
    return predicates;
}

// generuje data pro konjunkci, kdy evaluace dotazu je true
auto generate_instance_with_query_conjunction_t(const std::vector<uint32_t>& data, size_t query_count) {
    // vytvorime predikaty pro nasi tabulku
    auto predicates = std::vector<Predicate>(query_count);
    for (size_t i = 0; i < query_count; ++i) {
        // pro ktere hodnoty je predikat platny.
        auto dis_index = std::uniform_int_distribution<size_t>(0, (data.size() - 1) / 20);
        // zajima nas rozpeti hodnot - od - kam
        auto start_index_number = dis_index(rng);
        auto end_index_number = data.size() - dis_index(rng);
        // ale i jejich delitelnost nejakym cislem
        auto dis_space = std::uniform_int_distribution<uint32_t>(2, 5);
        auto space = dis_space(rng);

        // nas predikat ma podobu lambda funkce
        predicates[i] = [start_index_number, end_index_number, space](uint32_t value) {
            // FIXME: tohle by potencialne optimizer mohl vyhodit
            predicate_checking_simulation_function();
            // pokud hodnota odpovida pozadovane hodnote, vratime true. vetsina hodnot by mela byt true
            return value >= start_index_number && value <= end_index_number && value % space == 0;
        };
    }
    return predicates;
}

// generuje data pro dotaz s disjunkcemi - evaluace je false
// zbyle parametry odpovidaji parametrum metody generate_instance_with_query v hlavickovem souboru
auto generate_instance_with_query_disjunction_f(const std::vector<uint32_t>&, size_t query_count) {
    // vytvorime predikaty pro nasi tabulku. vsechny se budou evaluovat jako false
    auto predicates = std::vector<Predicate>(query_count);
    for (size_t i = 0; i < query_count; ++i) {
        // nas predikat ma podobu lambda funkce
        predicates[i] = [](uint32_t value) {
            predicate_checking_simulation_function();

            // zadny predikat neni platny. -1 se v tabulce nevyskytuje
            return value == std::numeric_limits<uint32_t>::max();
        };
    }
    return predicates;
}

auto generate_all(bool expected) -> std::pair<std::vector<uint32_t>, std::vector<Predicate>> {
    // reset naseho generatoru nahodnych cisel. pouzivame konstantu - kvuli replikovatelnosti vysledku
    rng.seed(15); // NOLINT(*-msc51-cpp)

    auto row_count = expected ? 100'000 : 30'000;
    auto query_count = expected ? 100'000 : 30;
    auto true_probability = 0.99;

    auto data = generate_shuffled_sequence<uint32_t>(row_count);
    auto predicates = expected
                      ? generate_instance_with_query_conjunction_t(data, query_count)
                      : generate_instance_with_query(data, query_count, false, true_probability);
    return {data, predicates};
}

auto generate_any(bool expected) -> std::pair<std::vector<uint32_t>, std::vector<Predicate>> {
    // reset naseho generatoru nahodnych cisel. pouzivame konstantu - kvuli replikovatelnosti vysledku
    rng.seed(15); // NOLINT(*-msc51-cpp)

    auto row_count = expected ? 500'000 : 2000;
    auto query_count = expected ? 500 : 300;
    auto true_probability = 0.8;

    auto data = generate_shuffled_sequence<uint32_t>(row_count);
    auto predicates = expected
                      ? generate_instance_with_query(data, query_count, true, true_probability)
                      : generate_instance_with_query_disjunction_f(data, query_count);
    return {data, predicates};
}
