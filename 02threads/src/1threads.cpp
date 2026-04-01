#include "1threads.h"
#include "../pdv_lib/pdv_lib.hpp"
#include <cstddef>
#include <execution>
#include <thread>
#include <mutex>
#include <atomic>

const size_t THREAD_COUNT = std::thread::hardware_concurrency();
std::mutex m;

void map_sequential(std::vector<float>& data, MapFn map_fn) {
    for (float& f : data) {
        f = map_fn(f);
    }
    // C ekvivalent:
    //for (size_t i = 0; i < data.size(); i++) {
    //    data[i] = map_fn(data[i]);
    //}
}

void map_openmp(std::vector<float>& data, MapFn map_fn) {
    #pragma omp parallel for
    for (float& f : data) {
        f = map_fn(f);
    }
}


/** Naivni implementace paralelizace se sdilenym indexem. */
void map_manual(std::vector<float>& data, MapFn map_fn) {
    size_t i = 0;
    auto process = [&] {
        while (i < data.size()) {
            // kazde vlakno si ve smycce vezme a incrementuje dalsi index ke zpracovani...
            auto& entry = data[i++];
            // ...a prvek zpracuje
            entry = map_fn(entry);
        }
    };

    auto worker_threads = std::vector<std::jthread>();
    for (size_t j = 0; j < THREAD_COUNT; j++) {
        worker_threads.push_back(std::jthread(process));
    }
}

void map_manual_locking(std::vector<float>& data, MapFn map_fn) {
    size_t i = 0;
    auto process = [&] {
        while (true) {
            size_t j;
            {
                auto lock = std::unique_lock<std::mutex>(m);
                j = i++;
            }
            if (j >= data.size()) {
                break;
            }
            if (i >= data.size()) {
                break;
            }
            auto& entry = data[i++];
            // ...a prvek zpracuje
            entry = map_fn(entry);
        }
    };

    auto worker_threads = std::vector<std::jthread>();
    for (size_t j = 0; j < THREAD_COUNT; j++) {
        worker_threads.push_back(std::jthread(process));
    }
}

void map_manual_atomic(std::vector<float>& data, MapFn map_fn) {
    std::atomic<size_t> i = 0;
    auto process = [&] {
        while (true) {
            size_t j;
            {
                auto lock = std::unique_lock<std::mutex>(m);
                j = i++;
            }
            if (j >= data.size()) {
                break;
            }
            if (i >= data.size()) {
                break;
            }
            auto& entry = data[i++];
            // ...a prvek zpracuje
            entry = map_fn(entry);
        }
    };

    auto worker_threads = std::vector<std::jthread>();
    for (size_t j = 0; j < THREAD_COUNT; j++) {
        worker_threads.push_back(std::jthread(process));
    }
}

void map_manual_ranges(std::vector<float>& data, MapFn map_fn) {
    auto process = [&](size_t start, size_t end) {
        while (true) {
            size_t j;
            {
                auto lock = std::unique_lock<std::mutex>(m);
                j = start++;
            }
            if (j >= end) {
                break;
            }
            auto& entry = data[start++];
            // ...a prvek zpracuje
            entry = map_fn(entry);
        }
    };

    auto worker_threads = std::vector<std::jthread>();
    for (size_t j = 0; j < THREAD_COUNT; j++) {
        worker_threads.push_back(std::jthread(process));
    }
}
