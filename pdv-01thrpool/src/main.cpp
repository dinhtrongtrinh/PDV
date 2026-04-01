#include "ThreadPool.h"

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <random>
#include <mutex>
#include <thread>

// Nastaveni poctu konzumentu a producentu
constexpr size_t NWORKERS = 2;
constexpr size_t NPRODUCERS = 2;

// Nastaveni poctu uloh pro kazdeho producenta a rozsah,
// ze ktereho se budou ulohy generovat
constexpr size_t NDATA = 30;
constexpr uint64_t MIN_DATA = 1000;
constexpr uint64_t MAX_DATA = 10000;

// Delka sleepu pro producenty a konzumenty, ovlivnujici
// jak rychle se ulohy vytvari a zpracovavaji
const uint64_t SLEEP_PER_STEP_WORKER = 2;
const uint64_t SLEEP_PER_FASTEST_PRODUCER = 20;

// Povoleni kontrolnich vypisu
constexpr bool VERBOSE = true;

// Mutex pro vypisy na stdout
std::mutex cout_mutex{};

/*
 * Tato funkce pocita ulohu zadanou konzumentovi.
 * V tomto pripade jde o problem 3n+1, tzv. Colatzuv problem
 * https://cs.wikipedia.org/wiki/Collatzův_problém
 *
 * !! >>>> Na testovani Vasi odevzdane ulohy budeme pouzivat i jine funkce <<<< !!
 */
void worker(uint64_t data) {
    uint64_t origdata = data;
    std::thread::id this_id = std::this_thread::get_id();

    uint64_t steps = 0;
    while (data > 1) {
        steps++;
        if (data % 2) data = 3 * data + 1;
        else data /= 2;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_PER_STEP_WORKER * steps));
    if (VERBOSE) {
        std::unique_lock<std::mutex> lock{cout_mutex};
        std::cout << "Thread " << this_id << " working on: " << origdata << " " << steps << std::endl;
    }
}

int main() {
    // Inicializace poolu vlaken, ktere zpracovavaji ulohy vytvarene producenty
    ThreadPool<uint64_t, void (*)(uint64_t)> pool(NWORKERS, &worker);

    // Vytvareni vlaken producentu, produkujicich ulohy
    std::vector<std::thread> producer_threads;
    for (unsigned int i = 1; i < NPRODUCERS + 1; i++) {
        producer_threads.push_back(std::thread([&pool, i]() {
            // ziskani id vlakna
            std::thread::id this_id = std::this_thread::get_id();
            // inicializace generatoru nahodnych cisel
            std::mt19937 RND(i);
            std::uniform_int_distribution<uint64_t> dist(MIN_DATA, MAX_DATA);

            for (unsigned int j = 1; j < NDATA; j++) {
                // vytvorime nahodnou ulohu
                auto data = dist(RND);
                auto sleepTime = i * SLEEP_PER_FASTEST_PRODUCER;

                if (VERBOSE) {
                    std::unique_lock<std::mutex> lock{cout_mutex};
                    std::cout << "Thread " << this_id << " pushing to queue: " << data << ", sleeping for: "
                              << sleepTime << "\n";
                }

                // vkladani ulohy do fronty uloh
                pool.process(data);

                // a pockame chvili, nez zadame dalsi ulohu ke zpracovani
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
            }
        }));
    }

    // Join pro producenty
    for (size_t i = 0; i < NPRODUCERS; i++) producer_threads[i].join();

    if (VERBOSE) {
        std::unique_lock<std::mutex> lock{cout_mutex};
        std::cout << "Producers finished\n";
    }

    // Ukoncovani cinnosti konzumentu
    for (size_t i = 0; i < NWORKERS; i++) pool.process(0);
    // Join pro konzumenty
    pool.join();

    return 0;
}
