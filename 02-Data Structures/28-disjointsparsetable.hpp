#pragma once

#include "../01-Core/01-template.hpp"

// n < 2^30; M is a monoid (S, op, e), op associative; query takes nonempty [l, r), fold returns e() on l == r.
// S: O(n * log(n + 1)), U: NA, Q: O(1), M: O(n * log(n + 1))
template<typename M>
struct DisjointSparseTable {
    using S = typename M::S;
    int n, h;
    vector<S> t;

    explicit DisjointSparseTable(const vector<S> &a) : n(int(a.size())), h(n > 1 ? int(std::bit_width(uint(n - 1))) : n) {
        assert(a.size() < size_t(1 << 30));
        t.reserve(size_t(h) * n);
        for (int k = 0; k < h; ++k) { t.insert(t.end(), a.begin(), a.end()); }
        for (int k = 1; k < h; ++k) {
            S *row = t.data() + size_t(k) * n;
            for (int mid = 1 << k; mid < n; mid += 2 << k) {
                row[mid - 1] = a[mid - 1]; row[mid] = a[mid];
                for (int i = mid - 2; i >= mid - (1 << k); --i) { row[i] = M::op(a[i], row[i + 1]); }
                for (int i = mid + 1; i < min(n, mid + (1 << k)); ++i) { row[i] = M::op(row[i - 1], a[i]); }}}}

    S query(int l, int r) const {
        assert(0 <= l && l < r && r <= n);
        if (--r == l) { return t[l]; }
        size_t k = std::bit_width(uint(l ^ r)) - 1;
        return M::op(t[k * n + l], t[k * n + r]);}
    S fold(int l, int r) const { assert(0 <= l && l <= r && r <= n); return l < r ? query(l, r) : M::e(); }

    // Offline; queries 0 <= l <= r <= n, empty queries give e().
    // T: O(n * log(log(n + 2)) + q), M: O(n + q)
    static vector<S> staticRangeProducts(const vector<S> &a, const vector<pair<int, int>> &qs) {
        assert(a.size() < size_t(1 << 30));
        int n = int(a.size()), q = int(qs.size()), s = int(std::bit_ceil(uint(std::bit_width(uint(n)))));
        vector<S> res(q, M::e()), mid(q, M::e()), pre(a), suf(a), blocks;
        for (int i = 1; i < n; ++i) { if (i % s) { pre[i] = M::op(pre[i - 1], a[i]); }}
        for (int i = n - 2; i >= 0; --i) { if ((i + 1) % s) { suf[i] = M::op(a[i], suf[i + 1]); }}
        for (int i = 0; i < n; i += s) { blocks.push_back(suf[i]); }
        vector<array<int, 3>> inner, outer;
        for (int j = 0; j < q; ++j) {
            auto [l, r] = qs[j];
            assert(0 <= l && l <= r && r <= n);
            if (l == r) { continue; }
            if (l / s == (r - 1) / s) { inner.push_back({l, r - 1, j}); }
            else if (l / s + 1 < (r - 1) / s) { outer.push_back({l / s + 1, (r - 1) / s - 1, j}); }}
        // out[id] = product of v[l..r] for inclusive (l, r, id): queries grouped by split level, then by block in l order.
        auto dyadic = [](const vector<S> &v, const vector<array<int, 3>> &qv, vector<S> &out) {
            int m = int(v.size());
            vector<int> start(m + 1, 0), ord(qv.size());
            for (auto &x : qv) { ++start[x[0] + 1]; }
            for (int i = 0; i < m; ++i) { start[i + 1] += start[i]; }
            for (int j = 0; j < int(qv.size()); ++j) { ord[start[qv[j][0]]++] = j; }
            vector<vector<int>> level(32);
            for (int j : ord) {
                auto [l, r, id] = qv[j];
                if (l == r) { out[id] = v[l]; }
                else { level[std::bit_width(uint(l ^ r)) - 1].push_back(j); }}
            vector<S> buf(m, M::e());
            for (int k = 0; k < 32; ++k) {
                const vector<int> &g = level[k];
                for (int from = 0, to = 0; from < int(g.size()); from = to) {
                    int b = qv[g[from]][0] >> (k + 1), md = (2 * b + 1) << k, hi = md;
                    for (to = from; to < int(g.size()) && qv[g[to]][0] >> (k + 1) == b; ++to) { hi = max(hi, qv[g[to]][1]); }
                    buf[md - 1] = v[md - 1]; buf[md] = v[md];
                    for (int i = md - 2; i >= qv[g[from]][0]; --i) { buf[i] = M::op(v[i], buf[i + 1]); }
                    for (int i = md + 1; i <= hi; ++i) { buf[i] = M::op(buf[i - 1], v[i]); }
                    for (int j = from; j < to; ++j) {
                        auto [l, r, id] = qv[g[j]];
                        out[id] = M::op(buf[l], buf[r]);}}}};
        dyadic(a, inner, res); dyadic(blocks, outer, mid);
        for (int j = 0; j < q; ++j) {
            auto [l, r] = qs[j];
            if (r - l < 1 || l / s == (r - 1) / s) { continue; }
            res[j] = l / s + 1 < (r - 1) / s ? M::op(M::op(suf[l], mid[j]), pre[r - 1]) : M::op(suf[l], pre[r - 1]);}
        return res;}
};
