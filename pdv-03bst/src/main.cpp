#include "tests.h"

#include <cstddef>
#include <cstdio>
#include <chrono>

constexpr size_t N1 = 1000000; // Kolik prvku budeme do BVS vkladat "napreskacku"
constexpr size_t N2 = 40000; // Kolik prvku budeme do BVS vkladat "poporade"

// Tato metoda spousti test definovany ve tride Test (implementaci testu si muzete prohlednout ve tride tests.h).
template<typename Test>
void run_test(const char* test_name) {
    // Nejprve si vytvorime instanci testu
    Test test{};

    try {
        auto begin = std::chrono::steady_clock::now();
        test.run_test();
        auto end = std::chrono::steady_clock::now();

        // Kontrola spravnosti vysledku
        if (!test.verify()) {
            printf("%s       --- wrong result ---\n", test_name);
        } else {
            auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - begin);
            printf("%s          %7ld ms\n", test_name, duration_ms.count());
        }
    } catch (...) {
        printf("%s      --- not implemented ---\n", test_name);
    }
}

int main() {
    // Test vkladani do BST "napreskacku"
    run_test<shuffled_data<N1>>("Shuffled data");
    // Test vkladani do BST "poporade"
    run_test<sorted_data<N2>>("Sorted data  ");

    return 0;
}
