#include <string>
#include <vector>
#include <numeric>
#include <random>
#include <algorithm>
#include "SequentialLinkedList.h"
#include "1ConcurrentLinkedList.h"
#include "2LockFreeLinkedList.h"

/** Pocet prvku, ktere budeme do listu vkladat. */
constexpr size_t INSERTION_COUNT = 30000;
/** Pocet iteraci, po ktere budeme zkouset spravnost implementace mazani. */
constexpr size_t REMOVAL_ITERATIONS = 1000000;

// Metoda, ktera zkontroluje, ze list obsahuje prvky 0..(elems-1) v serazenem poradi.
// Tato metoda funguje na libovolnem listu, ktery ma vnitrni uzly reprezentovane tridou
// 'Node', ktera obsahuje hodnotu v clenske promenne 'value' a ukazatel (neatomicky i
// atomicky) v clenske promenne 'next'. Dale potrebujeme, aby ukazatel (opet neatomicky
// i atomicky) byl v clenske promenne 'head' listu.
template<typename T>
bool check(const T& list, unsigned elems) {
    const typename T::Node* current = &list.head;

    // V nasi implementaci spojoveho seznamu je 'head' dummy prvek, takze ho preskocime.
    current = current->next;

    // stejny typ jako `value`
    decltype(current->value) i = 0;

    // Nyni prochazime spojovy seznam a kontrolujeme, zda na 'i'-te pozici je hodnota 'i'
    while (current != nullptr) {
        if (current->value != i) return false;
        else {
            current = current->next;
            i++;
        }
    }

    // Na zaver zkontrolujeme, ze jsme skutecne prosli 'elems' prvku
    return i == elems;
}


// Typovy system C++ nam umoznuje definovat specializovane tridy (podle typoveho parametru
// LLType). Zde rikame, ze kazdy typ spojoveho seznamu je konkurentni...
template<typename LLType>
class is_concurrent {
public:
    static constexpr bool value = true;
};

// ... az na jednu instanci, spojovy seznam typu Sequential.
template<>
class is_concurrent<SequentialLinkedList> {
public:
    static constexpr bool value = false;
};

// Metoda, ktera provede kontrolu spravnosti implementace spojoveho seznamu typu LLType
// (LLType je Sequential, Concurrent a nebo Lockfree).
template<typename LLType>
double test_insertion(const char* name, double ref_time = -1) {
    // Nejprve si vytvorime instanci spojoveho seznamu (bez ohledu na konkretni typ).
    LLType ll;
    try {
        // Spojovy seznam testujeme tak, ze do nej vkladame prvky 0 .. (N-1) v nahodnem
        // poradi. Proto si je nyni predpripravime.
        std::vector<uint64_t> data(INSERTION_COUNT);
        std::iota(data.begin(), data.end(), 0);
        std::random_device rd;
        std::mt19937 g(rd());
        std::shuffle(data.begin(), data.end(), g);

        auto begin = std::chrono::steady_clock::now();

        // OpenMP ma problemy, pokud dojde k vyhozeni vyjimky (tj. v situaci, kdy metoda
        // neni jeste naimplementovana). Abychom se vyhnuli problemum, vlozime prvni prvek
        // mimo paralelni region OpenMP.
        ll.insert(data[0]);

        // Nyni uz vime, ze k vyhozeni vyjimky nedoslo a muzeme do spojoveho seznamu zacit
        // paralelne vkladat data. Vsimnete si podminky if(is_concurrent<LLType>::value).
        // Ta nam zajisti, ze pokud LLType=Sequential, vkladani do spojoveho seznamu bude
        // provadet pouze jedno vlakno (viz specializace tridy is_concurrent na radku 50).
        #pragma omp parallel for if(is_concurrent<LLType>::value)
        for (size_t i = 1; i < INSERTION_COUNT; i++) {
            ll.insert(data[i]);
        }

        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration<double, std::milli>(end - begin).count();
        if (ref_time < 0) {
            ref_time = elapsed;
        }
        double speedup = ref_time / elapsed;

        // Nyni uz nam staci vypsat vysledky a zkontrolovat spravnost reseni
        printf("%s %7.2f ms (speedup %4.2fx)           result: %s\n", name, elapsed, speedup,
               check(ll, INSERTION_COUNT) ? "correct" : "incorrect");

        return elapsed;
    } catch (const pdv::not_implemented&) {
        printf("%s               ----- not implemented yet -----\n", name);
    }
    return -1;
}

template<typename LLType>
void to_vector(std::vector<uint64_t>& out, LLType& ll) {
    typename LLType::Node* current = ll.head.next;
    while (current != nullptr) {
        out.push_back(current->value);
        current = current->next;
    }
}

template<typename LLType>
double test_removal(const char* name, double ref_time) {
    try {
        double elapsed = 0;
        bool isCorrect = true;

        for (size_t i = 0; i < REMOVAL_ITERATIONS; i++) {
            // Nejprve si vytvorime instanci spojoveho seznamu (bez ohledu na konkretni typ).
            LLType ll;

            // Otestujeme zda jsou metody naimplementovane
            ll.insert(0);
            ll.remove(0);

            auto begin = std::chrono::steady_clock::now();

            // Zkontrolujeme typicky problem nastavajici pri vlozeni a mazani v jeden okamzik
            #pragma omp parallel num_threads(2) if(is_concurrent<LLType>::value)
            {
                #pragma omp single
                {
                    ll.insert(1);
                    ll.insert(2);
                    ll.insert(4);
                }

                #pragma omp barrier

                #pragma omp sections
                {
                    #pragma omp section
                    {
                        ll.remove(2);
                    }

                    #pragma omp section
                    {
                        ll.insert(3);
                    }
                }
            }

            auto end = std::chrono::steady_clock::now();

            elapsed += std::chrono::duration<double, std::milli>(end - begin).count();

            std::vector<uint64_t> content;
            to_vector(content, ll);

            std::vector<uint64_t> expected = {1, 3, 4};

            if (content != expected) {
                isCorrect = false;
            }
        }

        if (ref_time < 0) {
            ref_time = elapsed;
        }
        double speedup = ref_time / elapsed;

        // Nyni uz nam staci vypsat vysledky
        printf("%s %7.2f ms (speedup %4.2fx)           result: %s\n", name, elapsed, speedup,
               isCorrect ? "correct" : "incorrect");

        return elapsed;
    } catch (const pdv::not_implemented&) {
        printf("%s               ----- not implemented yet -----\n", name);
    }

    return -1;
}

int main() {
    // Spustime testy jednotlivych implementaci spojoveho seznamu:
    printf("INSERT:\n");
    double sequential_time = test_insertion<SequentialLinkedList>("SequentialLinkedList  ", -1);
    test_insertion<ConcurrentLinkedList>("ConcurrentLinkedList  ", sequential_time);
    test_insertion<LockFreeLinkedList>("LockFreeLinkedList    ", sequential_time);

    printf("\n");
    printf("INSERT & REMOVE:\n");
    sequential_time = test_removal<SequentialLinkedList>("SequentialLinkedList  ", -1);
    test_removal<ConcurrentLinkedList>("ConcurrentLinkedList  ", sequential_time);
    test_removal<LockFreeLinkedList>("LockFreeLinkedList    ", sequential_time);

    printf("\n\n");
    return 0;
}
