#include <limits>
#include <vector>
#include <atomic>
#include <numeric>
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


// V prvni sade uloh 'findmin_*' hledame cislo 'n' z vektoru 'data', pro
// ktere Collatzova sekvence dosahne cisla 1 nejrychleji. Takova uloha
// se da dobre paralelizovat, protoze muzeme hodnotu funkce 'collatz(n)'
// spocitat pro kazdy prvek pole nezavisle. V nasem pripade hledame dane
// cislo v intervalu [MIN_ELEMENT .. MAX_ELEMENT]. Tyto hodnoty si muzete
// upravit ve funkci `main`.
//
// Mnoho optimalizacnich problemu jde resit efektivneji. Pokud zjistime,
// ze dany prvek garantovane neni optimalni, nemusime se snazit spocitat
// jeho presnou hodnotu (v tomto pripade pocet potrebnych kroku). Mohli
// bychom takoveto odrezavani pouzit i v tomto pripade?

uint64_t findmin_sequential(const std::vector<uint64_t>& data) {
    auto min = std::numeric_limits<uint64_t>::max();

    for (auto n : data) {
        uint64_t steps;
        for (steps = 0; steps < min && n > 1; steps++) {
            if (n % 2) n = 3 * n + 1;
            else n /= 2;
        }

        if (steps < min) {
            min = steps;
        }
    }
    return min;
}

uint64_t findmin_parallel(const std::vector<uint64_t>& data) {
    // Inicializace na maximální možnou hodnotu
    std::atomic<uint64_t> min_val{std::numeric_limits<uint64_t>::max()};

    // 1. Spustíme paralelní sekci a smyčku s dynamickým plánováním
    #pragma omp parallel for schedule(dynamic, 100)
    for (size_t i = 0; i < data.size(); ++i) {
        uint64_t n = data[i];
        uint64_t steps = 0;

        // 2. Pruning (odřezávání):
        // Počítáme jen dokud je naše 'steps' menší než zatím nejlepší nalezené minimum
        uint64_t current_min = min_val.load();
        for (; steps < current_min && n > 1; steps++) {
            if (n % 2) n = 3 * n + 1;
            else n /= 2;
        }

        // 3. Pokus o aktualizaci globálního minima (pokud jsme našli lepší)
        if (steps < current_min) {
            // Použijeme tvůj oblíbený compare_exchange_strong
            // "Pokud je v min_val stále current_min, dej tam steps"
            min_val.compare_exchange_strong(current_min, steps);
        }
    }
    return min_val.load();
}


int main() {
    constexpr uint64_t MIN_ELEMENT = 10'000'000;
    constexpr uint64_t MAX_ELEMENT = 15'000'000;

    // vytvorime si vektor testovanych vstupu
    std::vector<uint64_t> data(MAX_ELEMENT - MIN_ELEMENT);
    // a naplnime ho cisly od MIN_ELEMENT do MAX_ELEMENT
    std::iota(data.begin(), data.end(), MIN_ELEMENT);

    pdv::benchmark("findmin_sequential", 10, [&] { return findmin_sequential(pdv::launder_value(data)); });
    pdv::benchmark("findmin_parallel", 10, [&] { return findmin_parallel(pdv::launder_value(data)); });

}
