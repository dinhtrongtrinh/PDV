#include <iostream>
#include "simd.h"

#define SHOW(op) std::cout << #op << " = " << (op) << "\n"
#define BLANK std::cout << "\n"

/** Examples of using vec_i32, a SIMD vector of 32-bit ints. */
void show_vec_i32() {
    std::cout << "=== vec_i32 ===================================================\n\n";

    // initialize a vector with 0 in all slots
    vec_i32 v1{0};
    SHOW(v1);

    // create a vector with items 0, 1, 2, 3,... and add it to `v1`
    v1 += vec_i32{[](auto i) { return i; }};
    SHOW(v1);

    // vector of ones
    vec_i32 v2(1);
    SHOW(v2);

    std::array<int32_t, 2 * vec_i32::size()> arr{};
    // store the vector of ones to an array
    v2.copy_to(arr.data(), element_aligned);
    // add 1 to all members
    v2 += 1;
    // store the vector of twos to the second half of the array
    v2.copy_to(&arr[vec_i32::size()], element_aligned);

    // now load the vector shifted by one
    v2.copy_from(&arr[1], element_aligned);
    SHOW(v2);

    BLANK;
    SHOW(v1 + v2);
    SHOW(v1 - v2);
    SHOW(v1 * v2);

    // blends don't seem to be supported in std::experimental::simd
    //BLANK;
    //SHOW(blend<0b10101010>(v1, v2));
    //SHOW(blend<0b00001111>(v1, v2));

    BLANK;
    SHOW(v1 == v2);
    SHOW(v1 > v2);
    where(v1 != v2, v1) = 0;
    SHOW(v1);

    BLANK;
    SHOW(v1 & v2);
    SHOW(v1 ^ v2);
    SHOW(v1 | v2);
}

/** Examples of using simd::vec_f32, a SIMD vector of 32-bit floats. */
void show_vec_f32() {
    vec_f32 v1{0.0f};
    v1 += vec<float>({0, 1.1, 2.2, 3, 4.4, 5, 6, 7});

    float arr[8] = {1.1, 1.1, 1.1, 1.1, 1.1, 1.1, 1.1, 1.1};
    vec_f32 v2;
    v2.copy_from(arr, element_aligned);

    std::cout << "\n=== vec_f32 ===================================================\n\n";

    SHOW(v1);
    SHOW(v2);

    BLANK;
    SHOW(v1 + v2);
    SHOW(v1 - v2);
    SHOW(v1 * v2);

    BLANK;
    SHOW(min(v1, v2));
    SHOW(max(v1, v2));

    // blends don't seem to be supported in std::experimental::simd
    //BLANK;
    //SHOW(blend<0b10101010>(v1, v2));
    //SHOW(blend<0b00001111>(v1, v2));

    BLANK;
    SHOW(v1 == v2);
    SHOW(v1 > v2);
    where(v1 != v2, v1) = 0;
    SHOW(v1);
}

int main() {
    BLANK;
    show_vec_i32();

    BLANK;
    show_vec_f32();

    return 0;
}
