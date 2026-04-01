#include "../pdv_lib/pdv_lib.hpp"
#include <thread>
#include <vector>

constexpr size_t NTHREADS = 8;

// Tuto funkci vykonava `NTHREADS` vlaken soucasne. Kazde vlakno zapisuje pouze do pametove bunky
// odkazovane pomoci ukazatele `x`.
void inc(volatile int32_t* x) {
    for (size_t i = 0L; i < 1'000'000'000; ++i) {
        if (i & 1) *x = *x + 1;
        else *x = (*x) * (*x);
    }
}

void run_test(size_t step_size) {
    // array, do ktere vlakna za behu opakovane zapisuji; pomoci `Step` lze menit vzdalenost
    //  mezi misty, kam jednotliva vlakna zapisuji
    auto data = std::vector<int32_t>(NTHREADS * step_size, 0);

    pdv::benchmark("False sharing (STEP=" + std::to_string(step_size) + ")", [&] {
        // array na jednotlive objekty reprezentujici bezici vlakna
        auto threads = std::vector<std::jthread>{};
        for (size_t i = 0; i < NTHREADS; i++) {
            // Nyni spustime vlakna. Kazde vlakno bude cist a zapisovat do promenne na pozici
            // `i * Step` v poli data. Pokud zvolime hodnotu `Step=1`, promenne se nachazi ve stejne
            // cache line (a proto hrozi, ze dojde k false-sharingu, protoze vic jader bude mit
            // stejnou cache line ve sve privatni cache). Pokud hodnotu navysime, napriklad na
            // `Step=16`, vzdalenost mezi dvemi sousednimi promennymi bude `16 * sizeof(int32_t)`.
            // Pokud je velikost cache line 64 bytu, promenne se nikdy nebudou nachazet ve stejne
            // cache line, a k false-sharingu tak nedojde.
            threads.push_back(std::jthread(inc, &data[i * step_size]));
        }

        // Na zaver pockame na dokonceni vsech vlaken (to se stane automaticky pri volani destructoru).
    });
}

int main() {
    // pomale, casto dochazi k false sharingu, data jsou hned u sebe
    run_test(1);

    // rychle, k false sharingu nedochazi vubec, kazda upravovana hodnota je ve vlastni radce cache
    run_test(16);

    return 0;
}
