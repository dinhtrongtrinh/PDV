#include <iostream>
#include <memory>
#include <random>
#include <chrono>
#include <algorithm>

#include "state.h"
#include "domains/hanoi.h"
#include "domains/maze.h"
#include "domains/sat.h"
#include "domains/sliding_puzzle.h"

#include "bfs.h"
#include "iddfs.h"
#include "iddfs_weighted.h"


// Vsechny funkce, ktere budete implementovat by mely implementovat nasledujici rozhrani.
// Mely by to tedy byt funkce s nasledujici hlavickou:
//     std::shared_ptr<const state> jmeno_funkce(std::shared_ptr<const state> root)
//
// Funkce dostane na vstupu ukazatel na pocatecni stav problemu (zdrojovy vrchol prohledavani,
// a mely by vratit ukazatel na cilovy stav, ktery je dosazitelny pomoci nejkratsi/nejlevnejsi
// cesty.
//
// Vsimnete si, ze ukazatele jsou typu 'std::shared_ptr' - coz Vam doufame usnadni hodne prace
// pri sprave pameti:
//   1) Na rozdil od 'const state *', 'std::shared_ptr<const state>' se stara o spravu pameti
//      automaticky (alokovana pamet pro stav zanikne automaticky po tom, co zanikne posledni
//      std::shared_ptr, ktery na ni ukazuje.
//   2) Na rozdil od 'std::unique_ptr<const state>' vlastnictvi ukazatele neni unikatni. To
//      znamena, ze pointer muzete predavat do ostatnich funkci/datovych struktur bez omezeni.
// Cenou za to je mirne zvysena rezie pri pristupech do pameti pres 'std::shared_ptr<...>'.
//
// Pro zjednoduseni zapisu typu pouzivame typovy alias `state_ptr`, definovany ve `state.h`.
// Pokud chcete byt radeji explicitni, muzete ve svem kodu pouzivat cely typ.
using search_fn = state_ptr (*)(state_ptr);


namespace ansi {
    const std::string RESET = "\033[0m";
    const std::string BRIGHT_RED = "\033[91m";
    const std::string BRIGHT_YELLOW = "\033[93m";
    const std::string BRIGHT_GREEN = "\033[92m";

    std::string green(const std::string& str) { return BRIGHT_GREEN + str + RESET; }

    std::string yellow(const std::string& str) { return BRIGHT_YELLOW + str + RESET; }

    std::string red(const std::string& str) { return BRIGHT_RED + str + RESET; }
}

/// Pomocne macro, ktere vytiskne nazev testovane funkce a pak zavola funkci <tt>evaluate</tt>.
#define EVALUATE(test_iterations, domain, search_fn) [&]{ \
    std::cout << "\n" << ansi::green("=== " #search_fn " ===") << "\n"; \
    evaluate(test_iterations, domain, search_fn); \
}()

/// Evaluacni funkce, ktera spusti prohledavaci algoritmus <tt>search</tt> z pocatecniho stavu <tt>root</tt>.
void evaluate(size_t test_iterations, const domain auto& domain, search_fn search) {
    using namespace std::chrono;

    if (test_iterations != 1) {
        std::cout << "Running " << test_iterations << " test iterations...\n";
    }

    state_ptr result;
    for (size_t i = 0; i < test_iterations; i++) {
        auto root = domain.root();
        auto begin = steady_clock::now();
        result = search(root);
        auto end = steady_clock::now();

        std::cout << "Time = " << duration_cast<milliseconds>(end - begin).count() << " ms, ";
        if (result == nullptr) {
            std::cout << ansi::yellow("no solution") << " (returned nullptr)\n";
        } else if (result->goal()) {
            std::cout << ansi::green("valid solution") << " (_cost = " << result->total_cost() << ")\n";
        } else {
            std::cout << ansi::red("invalid solution") << " (not a goal)\n";
        }
    }

    // Pro snazsi ladeni zrekonstruujeme a vypiseme nalezenou cestu
    auto path = std::vector<state_ptr>();
    while (result) {
        path.push_back(result);
        result = result->previous_state();
    }
    std::reverse(path.begin(), path.end());
    for (auto& s : path) {
        std::cout << s->to_string() << "\n";
    }
}

int main() {
    // Vytvoreni instance Hanojskych vezi s 3 koliky, 1 vezi (umistenou na prvnim koliku) a 4 kotouci ve vezi.
    //auto domain = hanoi<3, 1, 4>();

    // Vytvorit domenu "splnovani booleovskych funkci" muzete vytvorit nasledovne:
    // Tato domena pak ma:
    // - 30 booleovskych promennych
    // - 7 termu (ktere jsou spojeny disjunkci)
    // - kazdy term obsahuje maximalne 3 literaly
    // - seed nahodneho generatoru je 1
    // - cena za prirazeni hodnoty jednomu literalu je uniformni (1)
    //   (v pripade 'false' je cena za prirazeni hodnoty i-te promenne i)
    auto domain = sat<30, 7, 3, 1, true>();

    // Vytvorit domenu sliding-puzzle hranou na hraci plose 4x4 (15-puzzle) muzete takto:
    // Inicialni pozice je generovana provedenim 70 nahodnych tahu (nahodny generator je inicializovany seedem 0).
    //auto domain = sliding_puzzle<5, 70, 0>();

    // Posledni domenou jsou bludiste. Rozmery bludiste musi byt licha cisla. Bludiste je generovano nahodne za pouziti
    // predaneho seedu. Pokud nastavite posledni parametr na 'true', cena za jeden pohyb v bludisti nebude uniformni.
    //auto domain = maze<31, 21, 0, false>();


    // Vypiseme informace o vybrane domene
    std::cout << domain;

    // Zavolame 3x implementaci BFS, pro kazdou iteraci vypiseme cas a vysledek
    EVALUATE(5, domain, bfs);

    // Napodobne pro obe verze IDDFS (take doporucujeme testovat s vice iteracemi)
    EVALUATE(1, domain, iddfs);
    EVALUATE(1, domain, iddfs_weighted);

    return 0;
}
