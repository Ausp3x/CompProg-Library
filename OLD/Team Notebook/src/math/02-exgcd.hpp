#pragma once
#include "../1-Core/01-template.hpp"

// T: O(log(n)), M: O(1)
lng exGcd(lng a, lng b, lng &x, lng &y) {
    x = 1; y = 0; lng X = 0, Y = 1;
    while (b != 0) {
        lng q = a / b;
        x = std::exchange(X, x - q * X);
        y = std::exchange(Y, y - q * Y);
        a = std::exchange(b, a % b);}
    return a;}