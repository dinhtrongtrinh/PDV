#include <iostream>
#include <chrono>
#include <thread>

int main_var = 0;

void run(int count){
    main_var++;
}

int main() {
    std::thread t1 (run,10);
    t1.join();
    std::cout << main_var << std::endl;
}