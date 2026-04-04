#pragma once

// define everything in `std::experimental::`
#define VIR_SIMD_TS_DROPIN

#include "vir/simd.h"
#include <ostream>
#include <iterator>

using vec_f32 = std::experimental::native_simd<float>;
using vec_i32 = std::experimental::native_simd<int32_t>;
using std::experimental::element_aligned;

template<typename ElemT>
inline auto vec(std::initializer_list<ElemT> init) {
    std::experimental::native_simd<ElemT> vec{};
    size_t i = 0;
    for (auto& item : init) {
        vec[i++] = item;
    }
    return vec;
}

/** Operator overload to print masks to ostream. */
template<typename MaskT>
inline std::ostream& operator<<(std::ostream& os, MaskT v) requires(std::experimental::is_simd_mask_v<MaskT>) {
    os << "mask(";
    for (size_t i = 0; i < MaskT::size(); i++) {
        os << v[i];
    }
    os << ")";
    return os;
}

/** Operator overload to print vectors to ostream. */
template<typename VecT>
inline std::ostream& operator<<(std::ostream& os, VecT v) requires(std::experimental::is_simd_v<VecT>) {
    using scalar = typename VecT::value_type;

    std::array<scalar, VecT::size()> arr;
    v.copy_to(arr.data(), element_aligned);

    os << "(";
    std::copy(arr.begin(), arr.end() - 1, std::ostream_iterator<scalar>(os, ", "));
    os << *(arr.end() - 1);
    os << ")";
    return os;
}
