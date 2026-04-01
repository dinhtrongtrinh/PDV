#include "sort.h"
#include "test.h"

#include <cstdint>
#include <cstddef>
#include <random>
#include <algorithm>
#include <chrono>

// seed pro nahodny generator. pouzivame konstantu - kvuli replikovatelnosti vysledku
constexpr uint64_t SEED = 1;

// generator nahodnych cisel (Mersenne-Twister)
std::mt19937 rng(SEED); // NOLINT(*-msc51-cpp)

// celkove ve vectoru bude tolik prvku
constexpr size_t STR_COUNT = 10000000;
// vsechny stringy maji prave tuto delku. nemusite osetrovat zadne specialni pripady, ze by retezce byli jinak dlouhe.
// toto vam usetri osetrovani specialnich pripadu
constexpr size_t STR_LENGTH = 3;

// znaky retezcu jsou tvoreny touto abecedou
// pouzivame nekolika prvnich velkych pismen z anglicke abecedy
const std::string ALPHABET = "ABCDE";

// zjisteni offsetu pro nasi abecedu z ASCII - podle prvniho znaku. Napr. A zacina v ASCII na pozici 65.
// chceme, aby prvni znak nasi abecedy byl mapovan na 0
const char alphabet_offset = ALPHABET.at(0);

// rychla mapovaci funkce, ktera vam pro znak v abecede vrati jeho poradi
size_t get_bucket(char character_of_alphabet) {
    // poradi nastavujeme od 0. odecitame od cisla offset
    return character_of_alphabet - alphabet_offset;
}

// metoda ktera generuje retezce podle parametru vyse. retezce maji uniformni delku a jsou tvoreny znaky z abecedy
std::vector<std::string> generate_strings(size_t str_count, size_t str_length) {
    // distribuce moznych zacatku abecedy
    std::uniform_int_distribution<size_t> alphabet_i_dist(0, ALPHABET.size() - 1);
    std::vector<std::string> strings(str_count);
    // nageneruj data
    for (size_t i = 0; i < str_count; ++i) {
        std::string str{};
        str.reserve(str_length);
        // retezce maji uniformni delku
        for (size_t j = 0; j < str_length; ++j) {
            // pridame na pozici v retezci nahodny znak z abecedy
            str.push_back(ALPHABET[alphabet_i_dist(rng)]);
        }
        strings[i] = str;
    }
    return strings;
}

// instance radiciho algoritmu - instance vasho radiciho algoritmu
// volani implementace vaseho radiciho algoritmu. vsimnete si, ze promena je funkce, kterou inicializujeme lambdou.
// lambda ma jako vstup vektor odkazu na retezce, ktere maji byt serazeny. do vaseho radiciho algoritmu je vlozen tento
// vektor spolecne s mapovaci funkci, ktera vam pro znak z abecedy vrati jeho poradi. toto se hodi pro urceni spravneho
// bucketu, kam by mel tento znak patrit. vstupem pro vas algoritmus je take delka retezcu. vsechny retezce jsou stejne
// dlouhe
auto radix_sort = [](std::vector<std::string*>& vector_to_sort) {
    // metoda, kterou budete implementovat. viz soubor "sort.h"
    radix_par(vector_to_sort, get_bucket, ALPHABET.size(), STR_LENGTH);
};

// instance radiciho algoritmu
// pouziti radiciho algoritmu ze standardni knihovny
auto std_sort = [](std::vector<std::string*>& vector_to_sort) {
    // pracujeme s pointers. aby byly retezce spravne setridene je treba vlozit lambda funkcy, ktera porovnava retezce
    // na adresach, kam pointery ukazuji. bez lambda funkce bychom porovnavali adresy
    std::sort(vector_to_sort.begin(), vector_to_sort.end(), [&](std::string* first, std::string* second) {
        return (*first) < (*second);
    });
};

// evaluacni skript. v prvnim kroku preda data vasemu algoritmu. algoritmus spusti a zmeri cas.
// pokud vas algoritmus retezce spravne seradil, vypise cas. v opacnem pripade evaluace vypise chybovou hlasku
void eval(const char* test_name, std::vector<std::string>& data_to_sort, auto sorting_algorithm) {
    // Nejprve si vytvorime instanci testu
    SortingTest test{data_to_sort, sorting_algorithm};

    try {
        // Cas zacatku behu testu
        auto begin = std::chrono::steady_clock::now();
        // Beh testu
        test.run_sort();
        // Konec behu testu
        auto end = std::chrono::steady_clock::now();

        // Kontrola spravnosti vysledku
        if (!test.verify(data_to_sort, std_sort)) {
            printf("%s       --- wrong result ---\n", test_name);
        } else {
            auto ms = duration_cast<std::chrono::milliseconds>(end - begin);
            printf("%s          %7ldms\n", test_name, ms.count());
        }
    } catch (...) {
        printf("%s      --- not implemented ---\n", test_name);
    }
}

// hlavni metoda, ve ktere se vygeneruji data. data jsou serazena vasim algoritmem, nasledne je zkontrolovana
// spravnost razeni
int main() {
    // pripraveni data setu. data set obsahuje STR_COUNT retezcu
    // retezce jsou vsechny uniformne dlouhe. delka je velice male cislo. retezce se skladaji jen z nekolika
    // znaku abecedy
    // podrobnejsi informace viz metoda "sample_elements_from_distribution"
    auto data_to_sort = generate_strings(STR_COUNT, STR_LENGTH);

    // spusti evaluaci vaseho radiciho algoritmu s vygenerovanymi daty. informace se zadanim viz "sort.h"
    eval("student's radix sort", data_to_sort, radix_sort);

    // razeni za pouziti std::sort. na vygenerovane datove sade by mel byt znatelne pomalejsi nez vase reseni. radek
    // pro urychleni testovani muzete zakomentovat
    eval("std::sort", data_to_sort, std_sort);

    return 0;
}
