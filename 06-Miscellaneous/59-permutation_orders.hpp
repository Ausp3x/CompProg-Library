#pragma once
#include "../01-Core/01-template.hpp"
#include "06-bit_operations.hpp"
#include "07-permutation.hpp"

// T: O(n), M: O(n); Trotter-Johnson (Steinhaus-Johnson-Trotter) order of adjacent transpositions starting at the identity: rank for n <= 20.
inline ulng trotterJohnsonRank(const vector<int> &p) {
    assert(p.size() <= 20 && isPermutation(p)); int n = int(p.size()); vector<int> pos = permutationInverse(p);
    uint seen = 0; ulng rank = 0;
    for (int j = 1; j <= n; ++j) {
        int k = std::popcount(seen & ((1u << pos[j - 1]) - 1));
        rank = rank * ulng(j) + ulng(rank % 2 ? k : j - 1 - k);
        seen |= 1u << pos[j - 1];}
    return rank;}
// T: O(n), M: O(n); 0 <= n <= 20, false iff rank >= n! leaving out unchanged.
inline bool trotterJohnsonUnrank(int n, ulng rank, vector<int> &out) {
    if (n < 0 || n > 20) { assert(0 <= n && n <= 20); return false; }
    vector<ulng> r(n + 1); r[n] = rank;
    for (int j = n; j > 0; --j) { r[j - 1] = r[j] / ulng(j); }
    if (r[0]) { return false; }
    vector<int> p(n); uint free = BitOps<uint>::lowMask(n);
    for (int j = n; j > 0; --j) {
        int k = int(r[j] - ulng(j) * r[j - 1]), slot = BitOps<uint>::select(free, r[j - 1] % 2 ? k : j - 1 - k);
        p[slot] = j - 1; free ^= 1u << slot;}
    out = std::move(p); return true;}
// T: O(n), M: O(n); steps p to its successor, or wraps to the identity and returns false after the last.
inline bool nextTrotterJohnson(vector<int> &p) {
    assert(isPermutation(p)); int n = int(p.size()), st = 0; vector<int> pos = permutationInverse(p);
    bool odd = permutationSign(p) < 0;
    for (int m = n; m > 1; --m) {
        int d = pos[m - 1] - st;
        odd ^= (m - 1 - d) & 1;
        if (odd && d < m - 1) { std::swap(p[st + d], p[st + d + 1]); return true; }
        if (!odd && d > 0) { std::swap(p[st + d - 1], p[st + d]); return true; }
        st += !odd;}
    iota(p.begin(), p.end(), 0); return false;}
// T: O(n + out) amortized, M: O(n); visit(p, i) on every permutation in order, i = -1 first, else p was reached by swapping positions i and i + 1; false iff visit stopped it.
template<class F>
bool forEachTrotterJohnson(int n, F &&visit) {
    assert(n >= 0); vector<int> p(n), c(size_t(n) + 1), o(size_t(n) + 1, 1); iota(p.begin(), p.end(), 0);
    if (!visit(std::as_const(p), -1)) { return false; }
    // Even's plain changes: value j - 1 sits c[j] steps into its sweep, o[j] its direction; s counts levels parked at the left.
    for (int j = n, s = 0; j > 1;) {
        int q = c[j] + o[j];
        if (q < 0 || q == j) {
            s += q == j; o[j] = -o[j]; --j;
            continue;}
        int i = min(j - c[j], j - q) + s - 1;
        std::swap(p[i], p[i + 1]); c[j] = q;
        if (!visit(std::as_const(p), i)) { return false; }
        j = n; s = 0;}
    return true;}
