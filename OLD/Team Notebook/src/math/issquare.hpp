#pragma once
#include "../1-Core/01-template.hpp"

// T: O(1), M: O(1)
bool isSquare(lng x) {
    static constexpr ulng msk = []() {
        ulng res = 0;
        for (int i = 0; i < 64; i++) { res |= (1ULL << (63 - (i * i) % 64)); }
        return res;}();
    
    if (x < 0) { return false; }
    if (x == 0) { return true; }
    if ((lng(msk << (x & 63))) >= 0) { return false; }
    int cnt = __builtin_ctzll(x);
    if (cnt & 1) { return false; }
    x >>= cnt;
    if ((x & 7) != 1) { return false; }
    lng xrt = std::round(std::sqrtl(x));
    return xrt * xrt == x;}