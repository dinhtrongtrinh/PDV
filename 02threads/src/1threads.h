#pragma once

#include <vector>

/** `MapFn` is a pointer to a function taking and returning a `float`. */
using MapFn = float (*)(float);

void map_sequential(std::vector<float>& data, MapFn map_fn);
//void map_std_seq(std::vector<float>& data, MapFn map_fn);
//void map_std_par(std::vector<float>& data, MapFn map_fn);
void map_openmp(std::vector<float>& data, MapFn map_fn);

void map_manual(std::vector<float>& data, MapFn map_fn);
void map_manual_locking(std::vector<float>& data, MapFn map_fn);
void map_manual_atomic(std::vector<float>& data, MapFn map_fn);
void map_manual_ranges(std::vector<float>& data, MapFn map_fn);
