#include "../pdv_lib/pdv_lib.hpp"
#include <cstdio>

// Pocet iteraci v ramci jednoho mereni
constexpr size_t ITERS = 50'000'000;
// Pocet mereni
constexpr size_t TRIALS = 5;

// Konstanty pro prevod z bytu na kilobyty a megabyty (a opacne)
constexpr size_t KB = 1024;
constexpr size_t MB = 1024 * 1024;


double benchmark(size_t size, size_t jump_size, std::optional<double> previous_freq) {
    // Nejprve si naalokujeme blok pameti pozadovane velikosti
    std::vector<char> memory(size);
    // A spocteme masku, ktera nam umozni rychle pocitat modulo (pro size=2^n, x % size == x & mask)
    size_t mask = size - 1;

    // Provedeme TRIALS mereni
    auto result = pdv::benchmark_raw(TRIALS, [&] {
        size_t index = 0;
        // A v prubehu mereni provedeme ITERS pristupu do pameti
        for (size_t i = 0; i < ITERS; i++) {
            memory[index] ^= 1;
            index = (index + jump_size) & mask;
            // Pokud je velikost cache-line procesoru 64B, pricteni jump_size=67 k indexu zajisti, ze
            // nasledujici pristup do pameti bude pristupem do jine cache-line. Procesor tak bude nucen
            // ji nacist z pameti (resp. nejrychlejsi pameti cache, ve ktere je uz tento blok pameti
            // nacteny). Kod nam proto ilustruje "rychlost" pameti cache (merenou v poctu operaci za
            // vterinu), do ktere se blok pameti o velikosti size vejde.
        }

        // Abychom kompilatoru zabranili pri optimalizaci odstranit for smycku, pouzijeme nasledujici
        //  funkci, ktera compileru "nakuka", ze pouziva `memory`
        pdv::do_not_optimize_away(memory);
    });

    // Vypiseme frekvenci pristupu do pameti, ktere jsme dosahli
    double freq_mhz = result.max_frequency() * ITERS / 1'000'000;
    if (size < KB) {
        printf(" %4zuB   %8.2f MHz", size, freq_mhz);
    } else if (size < MB) {
        printf("%4zuKB   %8.2f MHz", size / KB, freq_mhz);
    } else {
        printf("%4zuMB   %8.2f MHz", size / MB, freq_mhz);
    }

    if (previous_freq && freq_mhz / *previous_freq < 0.8) {
        // looks like we spilt out to a slower cache level, this run is significantly slower than the previous one
        printf(" !!!");
    }
    printf("\n");

    return freq_mhz;
}

int main() {
    std::optional<double> previous_freq{};
    // Provedeme benchmark pro ruzne velikosti pametoveho bloku od 256B do 256MB
    for (size_t i = 8; i < 29; i++) {
        previous_freq = benchmark((size_t)1 << i,  67, previous_freq);
    }

    return 0;
}
