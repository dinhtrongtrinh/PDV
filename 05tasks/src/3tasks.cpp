#include <omp.h>
#include <map>
#include "../pdv_lib/pdv_lib.hpp"

// V posledni sade uloh se zamerime na vypocet Fibonacciho cisla pomoci rekurze a budete si moci vyzkouset
// `#pragma omp task` pro rekurzivni paralelizaci. V testech pocitame cislo FIB_QUERY, ktere si muzete
// upravit ve funkci `main`.


// sekvencni implementace; je tu jednak pro porovnani rychlosti, a druhak proto, ze i v paralelni
//  implementaci na ni prepneme, kdyz uz mame dost vytvorenych tasku, abychom saturovali vsechna jadra CPU
uint64_t fibonacci_seq(uint64_t n) {
    if (n <= 1) return n;
    return fibonacci_seq(n - 1) + fibonacci_seq(n - 2);
}


uint64_t fibonacci_par(uint64_t n) {
    if (n <= 1) return n;
    uint64_t first,second;
    #pragma omp task shared(first)
    first = fibonacci_seq(n - 1);
    #pragma omp task shared(second)
    second = fibonacci_seq(n - 2);
    #pragma omp taskwait
    return first + second;
}
uint64_t fibonacci_parallel_dp(uint64_t n) {
    if (n <= 1) return n;

    // 1. Příprava sdílené tabulky pro Dynamické Programování (memoizace)
    // Inicializujeme 0, což značí "ještě nespočítáno"
    std::vector<uint64_t> memo(n + 1, 0);
    memo[0] = 0;
    memo[1] = 1;

    uint64_t final_result = 0;

    // 2. Definice rekurzivní logiky pomocí lambdy
    // Musíme ji předat jako std::function, aby se mohla volat rekurzivně
    std::function<uint64_t(uint64_t)> compute_fib = [&](uint64_t k) -> uint64_t {
        // Kontrola, zda už výsledek v tabulce máme
        if (k <= 1) return k;
        if (memo[k] != 0) return memo[k];

        // Mezní hodnota (cut-off): Pro malá čísla nemá smysl vytvářet task
        if (k < 20) {
            return compute_fib(k - 1) + compute_fib(k - 2);
        }

        uint64_t x, y;

        // Vytvoření úkolů pro paralelní zpracování větví
#pragma omp task shared(x, memo)
        x = compute_fib(k - 1);

#pragma omp task shared(y, memo)
        y = compute_fib(k - 2);

        // Počkáme, až se obě větve vypočítají
#pragma omp taskwait

        // Uložíme do tabulky a vrátíme
        memo[k] = x + y;
        return memo[k];
    };

    // 3. Spuštění paralelní sekce
#pragma omp parallel
    {
        // Pouze jedno vlákno odstartuje hlavní rekurzi,
        // ostatní si budou rozebírat vygenerované tasky z fronty.
#pragma omp single
        {
            final_result = compute_fib(n);
        }
    }

    return final_result;
}

int main() {
    constexpr uint64_t FIB_QUERY = 38;

    // pdv::benchmark("fibonacci_seq", 100, [&] { return fibonacci_seq(pdv::launder_value(FIB_QUERY)); });
    // pdv::benchmark("fibonacci_par", 100, [&] { return fibonacci_par(pdv::launder_value(FIB_QUERY)); });
    pdv::benchmark("fibonacci_DP_par", 10000, [&] { return fibonacci_parallel_dp(pdv::launder_value(FIB_QUERY)); });

    return 0;
}
