#pragma once

#include "../01-Core/01-template.hpp"

namespace xorbasis_detail {
    // T: O(w), M: O(1); inserts x from position pos into a w-bit basis indexed by leading bit, keeping at each pivot the latest position.
    inline void insertTimed(ulng *v, int *p, int w, ulng x, int pos) {
        for (int i = w - 1; i >= 0 && x; --i) {
            if (!(x >> i & 1)) { continue; }
            if (!v[i]) { v[i] = x; p[i] = pos; return; }
            if (p[i] < pos) { std::swap(v[i], x); std::swap(p[i], pos); }
            x ^= v[i];}}
} // namespace xorbasis_detail

// GF(2) span of 64-bit words; b is fully reduced (each leading bit appears in no other vector) and sorted decreasing, m[i] the successful inserts (numbered from 0) whose xor is b[i]; kth asserts k < 2^size().
// S: O(1), U: O(r) insert, Q: O(r), M: O(r) with r = size() <= 64
struct XorBasis {
    vector<ulng> b, m;

    int size() const { return int(b.size()); }

    bool insert(ulng x) {
        ulng mx = 1ull << (size() & 63);
        for (int i = 0; i < size(); ++i) {
            ulng t = -ulng((x ^ b[i]) < x);
            x ^= b[i] & t; mx ^= m[i] & t;}
        if (!x) { return false; }
        for (int i = 0; i < size(); ++i) {
            ulng t = -ulng((b[i] ^ x) < b[i]);
            b[i] ^= x & t; m[i] ^= mx & t;}
        int j = int(std::ranges::upper_bound(b, x, std::greater<>()) - b.begin());
        b.insert(b.begin() + j, x); m.insert(m.begin() + j, mx);
        return true;}
    // T: O(r * r_o), M: O(1) with r_o = o.size(); o's vectors become successful inserts of this basis in decreasing order.
    void merge(const XorBasis &o) {
        for (ulng y : o.b) { insert(y); }}

    ulng reduce(ulng x) const {
        for (ulng y : b) { x = std::min(x, x ^ y); }
        return x;}
    bool contains(ulng x) const { return !reduce(x); }
    bool represent(ulng x, ulng &mask) const {
        mask = 0;
        for (int i = 0; i < size(); ++i) {
            ulng t = -ulng((x ^ b[i]) < x);
            x ^= b[i] & t; mask ^= m[i] & t;}
        return !x;}
    ulng max(ulng x = 0) const {
        for (ulng y : b) { x = std::max(x, x ^ y); }
        return x;}
    ulng kth(ulng k) const {
        int r = size();
        assert(r == 64 || k >> r == 0);
        ulng res = 0;
        for (int i = 0; i < r; ++i) {
            if (k >> (r - 1 - i) & 1) { res ^= b[i]; }}
        return res;}
    ulng rank(ulng x) const {
        int r = size();
        ulng k = 0, cur = 0;
        for (int i = 0; i < r; ++i) {
            if ((cur ^ b[i]) < x) { cur ^= b[i]; k |= 1ull << (r - 1 - i); }}
        return x ? k + 1 : 0;}

    // T: O(64 * r_b), M: O(64) with r_b = b.size()
    friend XorBasis intersection(const XorBasis &a, const XorBasis &b) {
        array<ulng, 64> v{}, t{};
        for (ulng y : a.b) { v[63 - std::countl_zero(y)] = y; }
        XorBasis res;
        // Invariant: t[i] lies in span(b) and v[i] ^ t[i] in span(a), so a vector of b reducing to 0 leaves its tag in both.
        for (ulng y : b.b) {
            ulng x = y, s = y;
            for (int i = 63; i >= 0 && x; --i) {
                if (!(x >> i & 1)) { continue; }
                if (!v[i]) { v[i] = x; t[i] = s; break; }
                x ^= v[i]; s ^= t[i];}
            if (!x) { res.insert(s); }}
        return res;}

    // T: O((n + q) * 64), M: O(n + q); calls f(i, e) for query i = [l, r) of a, e an echelon basis of a[l, r) (distinct leading bits, decreasing, not reduced).
    template<class F>
    static void rangeXorBasisQuery(const vector<ulng> &a, const vector<pair<int, int>> &qs, F f) {
        int n = int(a.size()), q = int(qs.size());
        vector<int> start(n + 2), order(q);
        for (auto [l, r] : qs) { assert(0 <= l && l <= r && r <= n); ++start[r + 1]; }
        for (int i = 0; i <= n; ++i) { start[i + 1] += start[i]; }
        for (int i = 0; i < q; ++i) { order[start[qs[i].second]++] = i; }
        array<ulng, 64> v{};
        array<int, 64> p;
        p.fill(-1);
        vector<ulng> e;
        for (int r = 0, j = 0; r <= n; ++r) {
            if (r) { xorbasis_detail::insertTimed(v.data(), p.data(), 64, a[r - 1], r - 1); }
            for (; j < q && qs[order[j]].second == r; ++j) {
                e.clear();
                for (int i = 63; i >= 0; --i) {
                    if (p[i] >= qs[order[j]].first) { e.push_back(v[i]); }}
                f(order[j], std::as_const(e));}}}
};

// Prefix bases of a over B-bit words (a[i] < 2^B), built by push: pivot i of prefix r keeps the latest position p; [l, r) spans the pivots with p >= l.
// S: O(n * B), U: O(B) amortized push, Q: O(B), M: O(n * B)
template<int B = 64>
struct PrefixXorBasis {
    static_assert(1 <= B && B <= 64);
    vector<array<ulng, B>> v;
    vector<array<int, B>> p;

    explicit PrefixXorBasis(const vector<ulng> &a = {}) : v(1), p(1) {
        p[0].fill(-1);
        v.reserve(a.size() + 1); p.reserve(a.size() + 1);
        for (ulng x : a) { push(x); }}

    int n() const { return int(v.size()) - 1; }

    void push(ulng x) {
        assert(B == 64 || x >> (B & 63) == 0);
        v.push_back(v.back()); p.push_back(p.back());
        xorbasis_detail::insertTimed(v.back().data(), p.back().data(), B, x, n() - 1);}

    ulng rangeMax(int l, int r, ulng x = 0) const {
        assert(0 <= l && l <= r && r <= n());
        for (int i = B - 1; i >= 0; --i) {
            if (p[r][i] >= l) { x = std::max(x, x ^ v[r][i]); }}
        return x;}
    bool rangeContains(int l, int r, ulng x) const {
        assert(0 <= l && l <= r && r <= n());
        if (B < 64 && x >> (B & 63)) { return false; }
        for (int i = B - 1; i >= 0; --i) {
            if (!(x >> i & 1)) { continue; }
            if (p[r][i] < l) { return false; }
            x ^= v[r][i];}
        return true;}
};
