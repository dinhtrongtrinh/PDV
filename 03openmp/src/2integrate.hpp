#pragma once

#include <cstddef>

/** `Integrand` is a pointer to a function taking and returning a `double`. */
using Integrand = double (*)(double);

double integrate_sequential(Integrand integrand, double a, double step_size, size_t step_count);
double integrate_omp_critical(Integrand integrand, double a, double step_size, size_t step_count);
double integrate_omp_atomic(Integrand integrand, double a, double step_size, size_t step_count);
double integrate_omp_reduction(Integrand integrand, double a, double step_size, size_t step_count);

double integrate_omp_for_static(Integrand integrand, double a, double step_size, size_t step_count);
double integrate_omp_for_dynamic(Integrand integrand, double a, double step_size, size_t step_count);
