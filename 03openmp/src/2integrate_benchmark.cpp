#include <cmath>
#include "../pdv_lib/benchmark.hpp"
#include "2integrate.hpp"

// parametry pro integraci:
// pocatek intervalu, kde se zacina integrovat
constexpr double INTEGRATION_START = 0.0;
// velikost kroku behem integrace
constexpr double STEP_SIZE = 0.05;


// Jednoducha integracni funkce - identita
// Vypocet trva velmi kratce
double identity(double x) {
    return x;
}

// Slozitejsi integracni funkce - integral ze sinu s pohyblivou horni mezi integrace
// Vypocet trva tim dele, cim vetsi je argument udavajici delku integrace
double integrated_function(double integrationRange) {
    const size_t num_steps = 200 * (size_t)(std::ceil(
        integrationRange * integrationRange * integrationRange * integrationRange));
    const double step_size = (integrationRange * integrationRange) / (double)num_steps;

    return integrate_sequential([](double x) { return std::sin(x); },
                                INTEGRATION_START, step_size, num_steps);
}


#define TEST(TESTED_FN, ITERS) pdv::benchmark(#TESTED_FN, ITERS, [&] { \
    return TESTED_FN(fn, pdv::launder_value(INTEGRATION_START), pdv::launder_value(STEP_SIZE), pdv::launder_value(step_count)); \
})

int main() {
    std::cout << "Integrace slozitejsi funkce `integrated_function(...)` - "
                 "numericky aproximovany integral sin(x) s promenlivym poctem kroku\n";
    size_t step_count = 120;
    auto fn = integrated_function;
    pdv::benchmark_group(30);

    TEST(integrate_sequential, 3);
    TEST(integrate_omp_critical, 3);
    TEST(integrate_omp_atomic, 3);
    TEST(integrate_omp_reduction, 3);
    TEST(integrate_omp_for_static, 3);
    TEST(integrate_omp_for_dynamic, 3);


    std::cout << "\n";
    // set format for the result print, a bit hacky
    std::cout << std::fixed << std::setprecision(0);
    std::cout << "Integrace `identity(...)`: identity(x)=x\n";
    // uvazujeme vice kroku, protoze funkce f(x)=x je opravdu jednoducha
    step_count = 5'000'000;
    fn = identity;
    pdv::benchmark_group(30);

    TEST(integrate_sequential, 1);
    TEST(integrate_omp_critical, 1);
    TEST(integrate_omp_atomic, 1);
    TEST(integrate_omp_reduction, 1);
    TEST(integrate_omp_for_static, 1);
    TEST(integrate_omp_for_dynamic, 1);

    return 0;
}
