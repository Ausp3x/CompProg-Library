#pragma once
#include "../1-Core/01-template.hpp"

// T: O(log(n)), M: O(1)
bool isPrimeMR(lng n) { // Miller-Rabin
    if (n < 2) { return false; }
    if (n == 2 || n == 3) { return true; }
    if (n % 2 == 0) { return false; }
    int s = __builtin_ctzll(n - 1); lng d = (n - 1) >> s;
    static constexpr lng bsm[] = {2LL, 7LL, 61LL};
    static constexpr lng blg[] = {2LL, 325LL, 9375LL, 28178LL, 450775LL, 9780504LL, 1795265022LL};
    const lng *B = n < 4759123141LL ? bsm : blg;
    for (int i = 0; i < (n < 4759123141LL ? 3 : 7); i++) {
        lng a = B[i];
        if (a % n == 0) { continue; }
        a %= n; lng b = d, x = 1;
        while (b > 0) {
            if (b & 1) { x = ulll(x) * a % n; }
            a = ulll(a) * a % n;
            b >>= 1;}
        if (x == 1 || x == n - 1) { continue; }
        bool is_comp = true;
        for (int j = 1; j < s; j++) {
            x = ulll(x) * x % n;
            if (x == n - 1) { is_comp = false; break; }
            if (x == 1) { return false; }}
        if (is_comp) { return false; }}
    return true;}