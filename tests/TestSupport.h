#pragma once
#include <cmath>
#include <iostream>
#define CHECK(expr) do { if (!(expr)) { std::cerr << __FILE__ << ':' << __LINE__ << " CHECK failed: " #expr << '\n'; return 1; } } while (0)
#define CHECK_NEAR(a,b,t) CHECK(std::fabs((a)-(b)) <= (t))
