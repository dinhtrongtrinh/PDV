#include <iostream>
#include "simd.h"

/**
 * Tato binarka testuje, ktera vektorova rozsireni vas pocitac podporuje.
 */
int main() {
    std::cout << "vec_f32: " << vec_f32::size() << " elements\n";
    std::cout << "vec_i32: " << vec_i32::size() << " elements\n";

    return 0;
}
