#pragma once
#include "../1-Core/01-template.hpp"

// T: O(log(n)), M: O(1)
lll getFloorSum(lng n, lng a, lng b, lng mod) { // \sum_{i = 0}^{n - 1} floor((a * x + b) / m)
    lll res = 0;
    if (a >= mod) { res += lll(n - 1) * n / 2 * (a / mod); a %= mod; }
    if (b >= mod) { res += lll(n) * (b / mod); b %= mod; }
    lng y = (lll(a) * n + b) / mod;
    if (y == 0) { return res; }
    lll x = lll(y) * mod - b;
    res += lll(n - (x + a - 1) / a) * y;
    res += getFloorSum(y, mod, (a - x % a) % a, a);
    return res;}