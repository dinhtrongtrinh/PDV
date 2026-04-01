#pragma once

#include <cstdint>
#include <vector>
#include <functional>
#include <stdexcept>

/// Predikat je funkce, ktera bere hodnotu typu `T` a vraci bool.
template<typename T>
using Predicate = std::function<bool(T)>;

struct not_implemented_error : std::runtime_error {
    not_implemented_error() : runtime_error("Not yet implemented") {}
};

bool is_satisfied_for_all(const std::vector<Predicate<uint32_t>>& predicates, const std::vector<uint32_t>& data);
bool is_satisfied_for_any(const std::vector<Predicate<uint32_t>>& predicates, const std::vector<uint32_t>& data);
