#pragma once

#include "../01-Core/01-template.hpp"
#include "01-dsu.hpp"

// Keys [0, n); next(x) is the smallest key >= x or n, prev(x) the largest key <= x or -1, for any int x.
// S: O(n / 64) (build O(n)), U: O(log(n) / log(64)), Q: O(log(n) / log(64)), M: O(n / 64)
struct FastSet {
    int n, cnt = 0;
    vector<vector<ulng>> seg;

    explicit FastSet(int N = 0) : n(N) {
        assert(n >= 0);
        int m = n;
        do { m = m / 64 + (m % 64 != 0); seg.emplace_back(m); } while (m > 1);}
    template<typename F>
    FastSet(int N, F f) : FastSet(N) {
        for (int x = 0; x < n; ++x) {
            if (f(x)) { seg[0][x >> 6] |= 1ULL << (x & 63); ++cnt; }}
        for (int h = 1; h < int(seg.size()); ++h) {
            for (int i = 0; i < int(seg[h - 1].size()); ++i) {
                if (seg[h - 1][i]) { seg[h][i >> 6] |= 1ULL << (i & 63); }}}}

    bool insert(int x) {
        if (contains(x)) { return false; }
        ++cnt;
        for (auto &s : seg) {
            ulng old = s[x >> 6];
            s[x >> 6] |= 1ULL << (x & 63);
            if (old) { break; }
            x >>= 6;}
        return true;}
    bool erase(int x) {
        if (!contains(x)) { return false; }
        --cnt;
        for (auto &s : seg) {
            s[x >> 6] &= ~(1ULL << (x & 63));
            if (s[x >> 6]) { break; }
            x >>= 6;}
        return true;}

    bool contains(int x) const { assert(0 <= x && x < n); return seg[0][x >> 6] >> (x & 63) & 1; }
    int size() const { return cnt; }
    bool empty() const { return !cnt; }
    int next(int x) const {
        x = std::max(x, 0);
        if (x >= n) { return n; }
        for (int h = 0; h < int(seg.size()) && (x >> 6) < int(seg[h].size()); ++h) {
            ulng d = seg[h][x >> 6] >> (x & 63);
            if (!d) { x = (x >> 6) + 1; continue; }
            x += std::countr_zero(d);
            for (int g = h - 1; g >= 0; --g) { x = x << 6 | std::countr_zero(seg[g][x]); }
            return x;}
        return n;}
    int prev(int x) const {
        x = std::min(x, n - 1);
        for (int h = 0; h < int(seg.size()) && x >= 0; ++h) {
            ulng d = seg[h][x >> 6] << (63 - (x & 63));
            if (!d) { x = (x >> 6) - 1; continue; }
            x -= std::countl_zero(d);
            for (int g = h - 1; g >= 0; --g) { x = x << 6 | (63 - std::countl_zero(seg[g][x])); }
            return x;}
        return -1;}
    int min() const { return next(0); }
    int max() const { return prev(n - 1); }
    // T: O(k + 1) next calls for k visited keys; f may erase the key it visits.
    template<typename F>
    void enumerate(int l, int r, F f) const {
        assert(0 <= l && l <= r && r <= n);
        for (int x = next(l); x < std::min(r, n); x = next(x + 1)) { f(x); }}
};

// Keys [0, n), all present at construction; erase only; next and prev as in FastSet. A DSU joins runs of empty 64-bit words.
// S: O(n / 64), U: O(alpha(n)) amortized, Q: O(alpha(n)) amortized, M: O(n / 64)
struct DecrementalSet {
    int n;
    vector<ulng> w;
    vector<int> lo, hi;
    DSU d;

    explicit DecrementalSet(int N = 0) : n(N) {
        assert(n >= 0);
        int m = n / 64 + (n % 64 != 0);
        w.assign(m, ~0ULL); lo.resize(m); iota(lo.begin(), lo.end(), 0); hi = lo; d = DSU(m);
        if (n % 64 > 0) { w.back() >>= 64 - n % 64; }}

    bool erase(int x) {
        if (!contains(x)) { return false; }
        int b = x >> 6;
        w[b] &= ~(1ULL << (x & 63));
        if (w[b]) { return true; }
        int l = b, h = b;
        if (b > 0 && !w[b - 1]) { l = lo[d.findSet(b - 1)]; d.uniteSets(b, b - 1); }
        if (b + 1 < int(w.size()) && !w[b + 1]) { h = hi[d.findSet(b + 1)]; d.uniteSets(b, b + 1); }
        int r = d.findSet(b);
        lo[r] = l; hi[r] = h;
        return true;}

    bool contains(int x) const { assert(0 <= x && x < n); return w[x >> 6] >> (x & 63) & 1; }
    int next(int x) {
        x = max(x, 0);
        if (x >= n) { return n; }
        int b = x >> 6;
        if (ulng m = w[b] >> (x & 63)) { return x + std::countr_zero(m); }
        if (++b < int(w.size()) && !w[b]) { b = hi[d.findSet(b)] + 1; }
        return b < int(w.size()) ? b << 6 | std::countr_zero(w[b]) : n;}
    int prev(int x) {
        x = min(x, n - 1);
        if (x < 0) { return -1; }
        int b = x >> 6;
        if (ulng m = w[b] << (63 - (x & 63))) { return x - std::countl_zero(m); }
        if (--b >= 0 && !w[b]) { b = lo[d.findSet(b)] - 1; }
        return b >= 0 ? b << 6 | (63 - std::countl_zero(w[b])) : -1;}
    // T: O(k + 1) next calls for k visited keys; f may erase the key it visits.
    template<typename F>
    void enumerate(int l, int r, F f) {
        assert(0 <= l && l <= r && r <= n);
        for (int x = next(l); x < min(r, n); x = next(x + 1)) { f(x); }}
};
