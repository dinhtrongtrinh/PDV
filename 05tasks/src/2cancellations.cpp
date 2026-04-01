#include <iostream>
#include <atomic>
#include <omp.h>
#include "../pdv_lib/benchmark.hpp"

// Collatzuv problem (znamy take jako uloha 3n+1) je definovany jako posloupnost cisel generovana podle
// nasledujicich pravidel:
//  1) Pokud je n liche, dalsi prvek posloupnosti je 3n+1
//  2) Pokud je n sude, dalsi prvek posloupnosti je n/2
// Neni znamo, zda pro libovolne prirozene cislo n posloupnost cisel "dosahne" cisla 1, ale je pravdepodobne,
// ze ano. Na dnesnim cviceni nas bude zajimat, kolik operaci (1) a (2) je pro to potreba. To muzeme zjistit
// pomoci nasledujici jednoduche funkce:
uint64_t collatz(uint64_t n) {
    uint64_t steps;
    for (steps = 0; n > 1; steps++) {
        if (n % 2) n = 3 * n + 1;
        else n /= 2;
    }
    return steps;
}

// V druhe sade uloh `findn_*` se zamerime na opacny problem. Dostaneme delku sekvence `criteria` a nasim ukolem
// bude nalezt cislo `n`, pro ktere je hodnota `collatz(n) >= criteria`. Hodnotu parametru 'criteria' si muzete
// upravit ve funkci `main`.
//
// Vsimnete si, ze v tomto pripade nevime predem kolik prvku budeme muset zpracovat, nez narazime na potrebne 'n'.
// Nevime proto predem, jak mame data rozdelit mezi jednotliva vlakna.

uint64_t findn_sequential(uint64_t criteria) {
    for (uint64_t i = 1;; i++) {
        if (collatz(i) >= criteria) return i;
    }
}

uint64_t findn_parallel(uint64_t criteria) {
    std::atomic<uint64_t> result{0}; // 0 znamená "nenalezeno"
    uint64_t current_start = 1;
    uint64_t block_size = 10000;

    // Musíme mít zapnuté OMP_CANCELLATION=true v systému
    while (result.load() == 0) {
        uint64_t current_end = current_start + block_size;

        #pragma omp parallel
        {
            // Každé vlákno si bere čísla z aktuálního rozsahu [current_start, current_end]
            #pragma omp for schedule(dynamic, 100)
            for (uint64_t n = current_start; n < current_end; n++) {

                // Kontrolní bod pro rychlé ukončení, pokud už někdo jiný našel výsledek
                #pragma omp cancellation point for

                if (collatz(n) >= criteria) {
                    uint64_t expected = 0;
                    // ATOMICKÝ ZÁPIS: "Pokud je v result stále 0, dej tam n"
                    if (result.compare_exchange_strong(expected, n)) {
                        // Pokud jsme uspěli, vyhlásíme poplach pro ostatní
                        #pragma omp cancel for
                    }
                }
            }
        }

        // Posuneme rozsah pro další kolo while cyklu
        current_start = current_end;
        block_size *= 2; // Exponenciální růst bloků (volitelné)
    }

    return result.load();
}


int main() {
    if (!omp_get_cancellation()) {
        std::cout << "-----------------------------------------------------------------------------\n";
        std::cout << "| WARNING: OpenMP cancellations are not enabled                             |\n";
        std::cout << "| You can enable them by setting environment variable OMP_CANCELLATION=true |\n";
        std::cout << "-----------------------------------------------------------------------------\n\n";
    }

    constexpr uint64_t CRITERIA = 450;

    pdv::benchmark("findn_sequential", 15, [&] { return findn_sequential(pdv::launder_value(CRITERIA)); });
    pdv::benchmark("findn_parallel", 15, [&] { return findn_parallel(pdv::launder_value(CRITERIA)); });
}
