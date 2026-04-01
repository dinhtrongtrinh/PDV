#include <thread>
#include <mutex>
#include <iostream>
#include <condition_variable>

using namespace std::chrono_literals;

static bool value = false;
static bool running = true;

static std::mutex m;

// Prvni vlakno ve smycce periodicky nastavuje hodnotu promenne 'value'.
// Tuto hodnotu pak kontroluje druhe vlakno, vykonavajici 'logger'.
void setter() {
    for (size_t i = 0; i < 5; i++) {
        {
            auto lock = std::unique_lock(m);
            value = !value;
            std::cout << "Setting value = " << value << "\n";
        }
        std::this_thread::sleep_for(500ms);
    }

    auto lock = std::unique_lock(m);
    running = false;
}

// Druhe vlakno reaguje na zmenu v hodnote promenne 'value'.
// TODO: Doplnte do vlakna podminkovou promennou tak, aby nemuselo aktivne cekat na zmenu promenne (busy waiting)
void logger() {
    bool last_value = false;
    auto lock = std::unique_lock(m);
    while (running) {
        if (last_value != value) {
            std::cout << "Value changed to " << value << "\n";
            last_value = value;
        }

        lock.unlock();
        std::this_thread::sleep_for(10ms);
        lock.lock();
    }
}

int main() {
    auto logger_thread = std::jthread(logger);
    auto setter_thread = std::jthread(setter);

    return 0;
}
