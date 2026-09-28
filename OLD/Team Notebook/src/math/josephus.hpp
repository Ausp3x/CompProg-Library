#pragma once
#include "../1-Core/01-template.hpp"

// T: O(n * log(n)), M: O(log(n))
lng josephus(lng n, lng k) {
    if (n == 1) { return 0; }
    if (k == 1) { return n - 1; }
    if (n < k) {
        lng res = 0;
        for (lng i = 2; i <= n; i++) { res = (res + k) % i; }
        return res;}
    lng nk = n - n / k;
    return k * ((josephus(nk, k) + nk - n % k % nk) % nk) / (k - 1);}