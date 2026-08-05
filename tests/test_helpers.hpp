#pragma once
#include <cmath>
#include <string>
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

static constexpr double EPSILON = 1e-6;
static bool approx_equal(double a, double b) { return std::fabs(a - b) < EPSILON; }
static bool approx_ge(double a, double b, double eps = EPSILON) { return a >= b - eps; }

using Catch::Matchers::ContainsSubstring;
