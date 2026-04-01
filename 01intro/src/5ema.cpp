#include "../pdv_lib/pdv_lib.hpp"
#include <vector>
#include <thread>

// Vygenerujeme náhodná data ceny akcii v čase.
// Formát je následující:
// Vector jednolivých akcií obsahující ceny za jednotlivé dny.
std::vector<std::vector<double>> generate_random_stock_data(size_t num_days, size_t num_stocks) {
    auto stock_prices = std::vector<std::vector<double>>(num_stocks);
    auto random = pdv::uniform_random<double>(0, 10000);

    for (auto& day : stock_prices) {
        day.resize(num_days);
        for (auto& price : day) {
            price = random();
        }
    }
    return stock_prices;
}

// Implementace exponenciálního klouzavého průměru.
// Vstupní data jsou ceny akcií v čase.
// Alpha je váha, která určuje, jak moc se má brát v úvahu nová data.
std::vector<double> exponential_moving_average(const std::vector<double>& data, double alpha) {
    auto result = std::vector<double>();
    // Alokujeme pamet, ale neinicializujeme ji. To je spousta práce (přísutpů do paměti).
    result.reserve(data.size());

    // První prvek je stejný jako vstupní data.
    result.push_back(data[0]);
    for (size_t i = 1; i < data.size(); i++) {
        // Výpočet exponenciálního klouzavého průměru.
        result.push_back(alpha * data[i] + (1 - alpha) * result[i - 1]);
    }
    return result;
}

int main() {
    // Vygenerujeme nahodna data vyvoje ceny akcii v case.
    auto stock_prices_outer = generate_random_stock_data(100000, 500);

    pdv::benchmark("Exponential Moving Average serial", 20, [&] {
        // Zajistime, ze vypocet kompilator nezoptimalizuje pres hranici benchmarku.
        // Zaroven vsak nechceme data behem benchmarku kopirovat.
        auto& stock_prices = pdv::launder_value(stock_prices_outer);
        auto ema_prices = std::vector<std::vector<double>>(stock_prices.size());

        for (size_t i = 0; i < stock_prices.size(); i++) {
            ema_prices[i] = exponential_moving_average(stock_prices[i], 0.1);
        }
        pdv::do_not_optimize_away(ema_prices);
    });

    pdv::benchmark("Exponential Moving Average paraler", 20, [&] {
        // Zajistime, ze vypocet kompilator nezoptimalizuje pres hranici benchmarku.
        // Zaroven vsak nechceme data behem benchmarku kopirovat.
        auto& stock_prices = pdv::launder_value(stock_prices_outer);
        auto ema_prices = std::vector<std::vector<double>>(stock_prices.size());

        #pragma omp parallel for
        for (size_t i = 0; i < stock_prices.size(); i++) {
            ema_prices[i] = exponential_moving_average(stock_prices[i], 0.1);
        }
        pdv::do_not_optimize_away(ema_prices);
    });


    return 0;
}
