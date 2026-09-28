#pragma once
#include "../1-Core/01-template.hpp"

// T: O(n^(1/4)), M: O(1)
lng getOneFacBPR(lng n, lng x0 = 2, lng c = 1) { // Brent's Pollard's Rho
    auto f = [](lng x, lng c, lng mod) -> lng { 
        return (ulll(x) * x + c) % mod;};
    int m = 128, l = 1;
    lng x = x0, xs = -1, y = -1, g = 1, q = 1;
    while (g == 1) {
        y = x; int k = 0;
        while (g == 1 && k < l) {
            xs = x;
            for (int i = 0; i < min(m, l - k); i++) {
                x = f(x, c, n);
                q = ulll(q) * (x > y ? x - y : y - x) % n;}
            g = gcd(q, n); k += m;}
        l *= 2;}
    if (g == n) {
        do {
            xs = f(xs, c, n);
            g = gcd(xs > y ? xs - y : y - xs, n);} 
        while (g == 1);}
    return g;}