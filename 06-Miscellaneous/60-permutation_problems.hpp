#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n), M: O(n); lexicographically largest permutation of [0, n) with exactly k inversions, 0 <= k <= n(n - 1)/2 (CSES 2214).
inline vector<int> permutationWithInversions(int n, lng k) {
    assert(n >= 0 && 0 <= k && k <= lng(n) * (n - 1) / 2); vector<int> p; p.reserve(n);
    int m = n;
    for (; m > 0 && k >= m - 1; --m) { p.push_back(m - 1); k -= m - 1; }
    if (m > 0) { p.push_back(int(k)); }
    for (int v = 0; v < m; ++v) {
        if (v != k) { p.push_back(v); }}
    return p;}
