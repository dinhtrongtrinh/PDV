#include <cstdio>

int main() {
    // Pomoci #pragma omp parallel num_threads(64) rekneme kompilatoru,
    // ze ma nasledujici blok kodu spustit 16x na 16 vlaknech,
    // Vice o OpenMP se dozvite v prubehu semestru.
    #pragma omp parallel num_threads(16)
    {
        printf("Hello ");
        printf("parallel");
        printf(" world!\n");

        // Vlakna bezi soucasne, a jejich operace se tak mohou prokladat.
    }

    return 0;
}
