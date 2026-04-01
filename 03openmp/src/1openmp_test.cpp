#include <omp.h>
#include <cstdio>

int main() {
    // Tady se vytvori pool vlaken a kazde z nich nezavisle pusti obsah bloku
    #pragma omp parallel
    {
        // Index vlakna vykonavajiciho kod
        int thread_id = omp_get_thread_num();

        // Jenom hlavni (prvni) vlakno bude vypisovat informace
        if (thread_id == 0) {
            // Informace o prostredi
            // pocet procesoru, ktere jsou dostupne pri volani funkce
            int procs = omp_get_num_procs();

            // pocet vlaken v paralelnim bloku
            int nthreads = omp_get_num_threads();

            // maximalni pocet vlaken, ktere by mohly byt v paralelnim bloku
            int maxt = omp_get_max_threads();

            // pokud je volano v paralelnim bloku, vrati nenulovou hodnotu
            int inpar = omp_in_parallel();

            // hodnota rika, zda je povolena vnorena paralelizace
            bool nested = omp_get_max_active_levels() > 1;

            printf("Number of processors = %d\n", procs);
            printf("Number of threads = %d\n", nthreads);
            printf("Max threads = %d\n", maxt);
            printf("In parallel? = %d\n", inpar);
            printf("Nested parallelism enabled? = %d\n", nested);

        }

    }

}
