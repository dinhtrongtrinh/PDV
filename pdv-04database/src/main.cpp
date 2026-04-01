#include "generator.h"
#include "query.h"

#include <chrono>
#include <cstdio>
#include <omp.h>

using Data = std::vector<uint32_t>;
using Predicates = std::vector<Predicate<uint32_t>>;

struct TestAll {
    bool expected_result;
    std::pair<Data, Predicates> test_data;

    explicit TestAll(bool result) : expected_result(result), test_data(generate_all(result)) {}

    [[nodiscard]] bool run_test() const {
        return is_satisfied_for_all(test_data.second, test_data.first);
    }
};

struct TestAny {
    bool expected_result;
    std::pair<Data, Predicates> test_data;

    explicit TestAny(bool result) : expected_result(result), test_data(generate_any(result)) {}

    [[nodiscard]] bool run_test() const {
        return is_satisfied_for_any(test_data.second, test_data.first);
    }
};

void eval(const auto& test, const char* test_name) {
    try {
        auto begin = std::chrono::steady_clock::now();
        auto result = test.run_test();
        auto end = std::chrono::steady_clock::now();

        if (result != test.expected_result) {
            printf("%s       --- wrong result ---\n", test_name);
        } else {
            auto ms = duration_cast<std::chrono::milliseconds>(end - begin);
            printf("%s          %7ldms\n", test_name, ms.count());
        }
    } catch (const not_implemented_error&) {
        printf("%s      --- not implemented ---\n", test_name);
    }
}

int main() {
    if (!omp_get_cancellation()) {
        printf("-----------------------------------------------------------------------------\n");
        printf("| WARNING: OpenMP cancellations are not enabled                             |\n");
        printf("| You can enable them by setting environment variable OMP_CANCELLATION=true |\n");
        printf("-----------------------------------------------------------------------------\n");
    }

    // V tomto testovacim pripade testujeme rychlost vyhodnocovani dotazu typu:
    //  "Jsou vsechny dilci dotazy splnene?"
    // tj., existuje pro kazdy dilci dotaz alespon jeden radek v tabulce, pro
    // ktery predikat plati?
    eval(TestAll(true), "true = is_satisfied_for_all(...)");

    // Analogie predchoziho dotazu, ale v tomto pripade ma dotaz formu:
    //  "Existuje alespon jeden dilci dotaz, ktery je splneny?"
    // tj., existuje alespon jeden predikat a jeden radek v tabulce takovy, ze
    // je pro tento radek predikat pravdivy?
    eval(TestAny(true), "true = is_satisfied_for_any(...)");

    // Parametr testu udava, zda ma byt dotaz pravdivy nebo nikoliv. Vykon Vasi
    // implementace ale budeme testovat pouze na `true` dotazech.
    //
    // POZOR! Vase implementace ale samozrejme musi byt funkcni i pro `false`
    // dotazy! (tj., return true neni reseni ;-) To si muzete otestovat na techto
    // testech:
    printf("\n");
    eval(TestAll(false), "false = is_satisfied_for_all(...)");
    eval(TestAny(false), "false = is_satisfied_for_any(...)");

    // Parametry generovani dat si muzete upravit v souboru `generator.cpp`,
    // ve funkcich `generate_all` a `generate_any`.
}
