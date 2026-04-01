// Zkompilujte tento soubor, aby jste zkontrolovali, že máte dostatecne novy compiler podporujici C++ 20 a OpenMP.
//
// Pokud se vam tento soubor zkompiluje a nedostanete pri kompilaci zadnou chybu, je vse spravne nastaveno.
// Pokud ne, prihlaste se cvicicimu a ten vam poradi, co potrebujete nainstalovat nebo nastavit.

#include <iostream>
#include <experimental/simd>
#include <thread>
#include <syncstream>
#include <omp.h>

int main() {
    (void)std::experimental::native_simd<float>();

    std::jthread t([] {
        std::cout << "Hello from jthread!\n";
    });

    #pragma omp parallel for
    for (size_t i = 0; i < 8; i++) {
        std::osyncstream(std::cout) << "Hello from OpenMP thread " << omp_get_thread_num() << "!\n";
    }

    return 0;
}
