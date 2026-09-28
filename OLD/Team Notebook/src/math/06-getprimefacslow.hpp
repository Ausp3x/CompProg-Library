#pragma once
#include "../1-Core/01-template.hpp"

// T: O(n^(1/2)), M: O(log(n))
vector<lng> getPrimeFacSlow(lng n) {
    vector<lng> res;
    for (int p : {2, 3, 5}) { while (n % p == 0) { res.push_back(p); n /= p; } }
    int j = 0; static constexpr int inc[] = {4, 2, 4, 2, 4, 6, 2, 6};
    for (lng i = 7; i * i <= n; i += inc[j], j = (j + 1) & 7) {
        while (n % i == 0) { res.push_back(i); n /= i; }}
    if (n > 1) { res.push_back(n); }
    sort(res.begin(), res.end());
    return res;}