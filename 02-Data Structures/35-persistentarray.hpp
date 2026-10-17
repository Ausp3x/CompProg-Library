#pragma once

#include "../01-Core/01-template.hpp"

// 1 <= B <= 8, n <= INT_MAX; versions are dense ids from 0; a reference from get lives until the next set.
// k = 2^B, s sets: S: O(n) from a vector or O(k + log_k(n)) from a fill value, U: O(k * log_k(n)), Q: O(log_k(n)), M: O(n + k * s * log_k(n)) from a vector or O(k * (s * log_k(n) + 1)) from a fill value
template<typename T, int B = 3>
struct PersistentArray {
    static_assert(1 <= B && B <= 8);
    static constexpr int K = 1 << B;
    int n, h = 0;
    vector<int> ch, roots;
    vector<T> val;

    explicit PersistentArray(const vector<T> &a) : n(int(a.size())), val(a) {
        assert(a.size() <= size_t(INT_MAX));
        while ((lng(K) << (B * h)) < n) { ++h; }
        val.resize(max<size_t>((val.size() + K - 1) / K, 1) * K);
        int b = 0, m = int(val.size() / K);
        for (int i = 0; i < h; ++i) {
            int nb = int(ch.size() / K), nm = (m + K - 1) / K;
            for (int j = 0; j < nm * K; ++j) { ch.push_back(b + min(j, m - 1)); }
            b = nb; m = nm;}
        roots.push_back(b);}
    // Node 0 serves every level: its children are node 0 again, which is also the leaf chunk holding k copies of x.
    PersistentArray(int N, const T &x) : n(N), ch(K, 0), roots{0}, val(K, x) {
        assert(n >= 0);
        while ((lng(K) << (B * h)) < n) { ++h; }}

    typename vector<T>::const_reference get(int v, int p) const {
        assert(0 <= v && v < versionCount() && 0 <= p && p < n);
        int t = roots[v];
        for (int s = B * h; s > 0; s -= B) { t = ch[t * K + ((p >> s) & (K - 1))]; }
        return val[t * K + (p & (K - 1))];}
    int versionCount() const { return int(roots.size()); }

    int set(int v, int p, T x) {
        assert(0 <= v && v < versionCount() && 0 <= p && p < n);
        int res = clone(roots[v], h == 0), t = res;
        for (int s = B * h; s > 0; s -= B) {
            int c = t * K + ((p >> s) & (K - 1));
            t = ch[c] = clone(ch[c], s == B);}
        val[t * K + (p & (K - 1))] = std::move(x);
        roots.push_back(res);
        return versionCount() - 1;}
    int clone(int t, bool leaf) {
        if (leaf) {
            val.resize(val.size() + K); std::copy_n(val.begin() + t * K, K, val.end() - K);
            return int(val.size() / K) - 1;}
        ch.resize(ch.size() + K); std::copy_n(ch.begin() + t * K, K, ch.end() - K);
        return int(ch.size() / K) - 1;}

    // T: O(n), M: O(n)
    vector<T> toVector(int v) const {
        assert(0 <= v && v < versionCount());
        vector<T> res;
        res.reserve(n);
        auto dfs = [&](auto &&dfs, int t, int s) -> void {
            for (int c = 0; c < K && int(res.size()) < n; ++c) {
                if (s) { dfs(dfs, ch[t * K + c], s - B); }
                else { res.push_back(val[t * K + c]); }}};
        if (n) { dfs(dfs, roots[v], B * h); }
        return res;}
};
