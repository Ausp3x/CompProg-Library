#pragma once
#include "../01-Core/01-template.hpp"

namespace lcs_detail {
    template<class S> using Value = std::decay_t<decltype(std::declval<const S &>()[0])>;
    template<class S> auto at(const S &s, int i) {
        if constexpr (std::is_same_v<Value<S>, char>) { return uint8_t(s[i]); }
        else { return s[i]; }}
    // y gets ids in [0, sigma), x the same ids or -1 for symbols absent from b; returns sigma.
    template<class A, class B> int compress(const A &a, const B &b, vector<int> &x, vector<int> &y) {
        static_assert(std::is_same_v<Value<A>, Value<B>>);
        assert(a.size() < INT_MAX && b.size() < INT_MAX);
        int n = int(a.size()), m = int(b.size());
        x.resize(n);
        y.resize(m);
        if constexpr (std::is_same_v<Value<A>, char>) {
            for (int i = 0; i < n; ++i) { x[i] = uint8_t(a[i]); }
            for (int j = 0; j < m; ++j) { y[j] = uint8_t(b[j]); }
            return 256;}
        else {
            vector<Value<B>> v(b.begin(), b.end());
            sort(v.begin(), v.end());
            v.erase(unique(v.begin(), v.end()), v.end());
            for (int j = 0; j < m; ++j) { y[j] = int(lower_bound(v.begin(), v.end(), b[j]) - v.begin()); }
            for (int i = 0; i < n; ++i) {
                int t = int(lower_bound(v.begin(), v.end(), a[i]) - v.begin());
                x[i] = t < int(v.size()) && !(a[i] < v[t]) ? t : -1;}
            return int(v.size());}}
    template<class S> vector<int> ids(const S &s) { vector<int> x, y; compress(s, s, x, y); return y; }

    struct BitRows {
        vector<int> cnt, head, slot, pos;
        vector<ulng> pool, scratch, v;

        explicit BitRows(int sigma) : cnt(sigma), head(sigma), slot(sigma, -1) {}
        // res[j] = LCS(x[0, n), y[0, j)); a symbol with at least ceil(m / 64) occurrences keeps a dense mask, rarer ones use scratch.
        vector<int> row(const int *x, int n, const int *y, int m) {
            int w = (m + 63) / 64, at = 0;
            pos.resize(m);
            scratch.assign(w, 0);
            v.assign(w, ~0ULL);
            for (int j = 0; j < m; ++j) { ++cnt[y[j]]; }
            for (int j = 0; j < m; ++j) {
                if (cnt[y[j]] > 0) { head[y[j]] = at; at += cnt[y[j]]; cnt[y[j]] = -cnt[y[j]]; }}
            for (int j = 0; j < m; ++j) { pos[head[y[j]]++] = j; }
            for (int i = 0; i < n; ++i) {
                int c = x[i];
                if (c < 0 || !cnt[c]) { continue; }
                int k = -cnt[c];
                const int *p = pos.data() + head[c] - k;
                const ulng *mask = scratch.data();
                if (k >= w) {
                    if (slot[c] < 0) {
                        slot[c] = int(pool.size());
                        pool.resize(pool.size() + w);
                        for (int t = 0; t < k; ++t) { pool[slot[c] + p[t] / 64] |= 1ULL << (p[t] % 64); }}
                    mask = pool.data() + slot[c];}
                else {
                    for (int t = 0; t < k; ++t) { scratch[p[t] / 64] |= 1ULL << (p[t] % 64); }}
                bool carry = false;
                for (int t = 0; t < w; ++t) {
                    ulng sum;
                    bool high = __builtin_add_overflow(v[t], v[t] & mask[t], &sum);
                    carry = __builtin_add_overflow(sum, ulng(carry), &sum) || high;
                    v[t] = sum | (v[t] & ~mask[t]);}
                if (k < w) {
                    for (int t = 0; t < k; ++t) { scratch[p[t] / 64] = 0; }}}
            vector<int> res(m + 1);
            for (int j = 0; j < m; ++j) { res[j + 1] = res[j] + !(v[j / 64] >> (j % 64) & 1); }
            for (int j = 0; j < m; ++j) { cnt[y[j]] = 0; slot[y[j]] = -1; }
            pool.clear();
            return res;}
    };

    template<class A, class B> vector<int> scalarRow(const A &a, const B &b, int xl, int xr, int yl, int yr, bool rev) {
        vector<int> f(yr - yl + 1);
        for (int t = 0; t < xr - xl; ++t) {
            int i = rev ? xr - 1 - t : xl + t;
            for (int q = 1, diag = 0; q <= yr - yl; ++q) {
                int j = rev ? yr - q : yl + q - 1, up = f[q];
                f[q] = at(a, i) == at(b, j) ? diag + 1 : max(f[q], f[q - 1]);
                diag = up;}}
        return f;}
    // Appends one LCS of x[xl, xr) and y[yl, yr) as ascending pairs; row(.., true) is the LCS row of the reversed halves.
    template<class Row, class Eq> void hirschberg(int xl, int xr, int yl, int yr, Row &row, Eq &eq, vector<pair<int, int>> &res) {
        if (xl == xr || yl == yr) { return; }
        if (xr - xl == 1) {
            for (int j = yl; j < yr; ++j) {
                if (eq(xl, j)) { res.emplace_back(xl, j); return; }}
            return;}
        int mid = std::midpoint(xl, xr), split = yl, best = -1;
        {
            vector<int> f = row(xl, mid, yl, yr, false), g = row(mid, xr, yl, yr, true);
            for (int j = yl; j <= yr; ++j) {
                if (f[j - yl] + g[yr - j] > best) { best = f[j - yl] + g[yr - j]; split = j; }}}
        if (!best) { return; }
        hirschberg(xl, mid, yl, split, row, eq, res);
        hirschberg(mid, xr, split, yr, row, eq, res);}
} // namespace lcs_detail

// T: O(n * ceil(m / w) + (n + m) * log(n + m)) (no log term for bytes), M: O(n + m + S); totally ordered symbols, n, m < INT_MAX.
template<class A, class B> int bitsetLcs(const A &a, const B &b) {
    vector<int> x, y;
    int sigma = lcs_detail::compress(a, b, x, y);
    return lcs_detail::BitRows(sigma).row(x.data(), int(x.size()), y.data(), int(y.size())).back();}
// T: O(n * m), M: O(n + m) (scalar rows, equality only); lcsWitness uses bit rows for ordered symbols with bitsetLcs time; ascending index pairs.
template<class A, class B> vector<pair<int, int>> hirschbergLcs(const A &a, const B &b) {
    assert(a.size() < INT_MAX && b.size() < INT_MAX);
    vector<pair<int, int>> res;
    auto row = [&](int xl, int xr, int yl, int yr, bool rev) { return lcs_detail::scalarRow(a, b, xl, xr, yl, yr, rev); };
    auto eq = [&](int i, int j) { return lcs_detail::at(a, i) == lcs_detail::at(b, j); };
    lcs_detail::hirschberg(0, int(a.size()), 0, int(b.size()), row, eq, res);
    return res;}
template<class A, class B> vector<pair<int, int>> lcsWitness(const A &a, const B &b) {
    if constexpr (!std::totally_ordered<lcs_detail::Value<A>>) { return hirschbergLcs(a, b); }
    else {
        vector<int> x, y;
        lcs_detail::BitRows bits(lcs_detail::compress(a, b, x, y));
        int n = int(x.size()), m = int(y.size());
        vector<int> rx(x.rbegin(), x.rend()), ry(y.rbegin(), y.rend());
        vector<pair<int, int>> res;
        auto row = [&](int xl, int xr, int yl, int yr, bool rev) {
            return rev ? bits.row(rx.data() + n - xr, xr - xl, ry.data() + m - yr, yr - yl) : bits.row(x.data() + xl, xr - xl, y.data() + yl, yr - yl);};
        auto eq = [&](int i, int j) { return x[i] == y[j]; };
        lcs_detail::hirschberg(0, n, 0, m, row, eq, res);
        return res;}}
template<class A, class B> int lcs(const A &a, const B &b) {
    if constexpr (std::totally_ordered<lcs_detail::Value<A>>) { return bitsetLcs(a, b); }
    else { return lcs_detail::scalarRow(a, b, 0, int(a.size()), 0, int(b.size()), false).back(); }}

// T: O((n + m) * log(n + m) + r * log(min(n, m) + 1)), r = matching pairs, M: O(n + m + S); totally ordered symbols.
template<class A, class B> int huntSzymanski(const A &a, const B &b) {
    vector<int> x, y, tails;
    int sigma = lcs_detail::compress(a, b, x, y), m = int(y.size());
    vector<int> start(sigma + 1), pos(m);
    for (int c : y) { ++start[c + 1]; }
    for (int c = 0; c < sigma; ++c) { start[c + 1] += start[c]; }
    for (int j = m - 1; j >= 0; --j) { pos[start[y[j]]++] = j; }
    for (int c : x) {
        if (c < 0) { continue; }
        for (int t = c ? start[c - 1] : 0; t < start[c]; ++t) {
            auto it = lower_bound(tails.begin(), tails.end(), pos[t]);
            if (it == tails.end()) { tails.push_back(pos[t]); }
            else { *it = pos[t]; }}}
    return int(tails.size());}

// T: NA, M: O(1); one edit-script step: op is '=', '-' or '+', i and j are the cursors in a and b.
struct DiffOp {
    char op;
    int i, j;
};
// T: O((n + m) * D), D = edit distance, M: O(n + m + D); ops '=' (a[i] == b[j]), '-' (delete a[i]), '+' (insert b[j]), with i, j the cursors.
template<class A, class B> vector<DiffOp> myersDiff(const A &a, const B &b) {
    assert(a.size() < INT_MAX / 2 && b.size() < INT_MAX / 2);
    int n = int(a.size()), m = int(b.size());
    vector<int> vf(n + m + 5), vb(n + m + 5);
    vector<DiffOp> res;
    auto eq = [&](int i, int j) { return lcs_detail::at(a, i) == lcs_detail::at(b, j); };
    auto snake = [&](int x0, int x1, int y0, int y1) -> array<int, 5> {
        int sn = x1 - x0, sm = y1 - y0, delta = sn - sm, off = (sn + sm + 1) / 2 + 1;
        vf[off + 1] = vb[off + 1] = 0;
        for (int d = 0;; ++d) {
            for (int k = -d; k <= d; k += 2) {
                int x = k == -d || (k != d && vf[off + k - 1] < vf[off + k + 1]) ? vf[off + k + 1] : vf[off + k - 1] + 1, y = x - k, sx = x, sy = y;
                while (x < sn && y < sm && eq(x0 + x, y0 + y)) { ++x; ++y; }
                vf[off + k] = x;
                if (delta % 2 && abs(delta - k) <= d - 1 && x + vb[off + delta - k] >= sn) { return {2 * d - 1, x0 + sx, y0 + sy, x0 + x, y0 + y}; }}
            for (int k = -d; k <= d; k += 2) {
                int x = k == -d || (k != d && vb[off + k - 1] < vb[off + k + 1]) ? vb[off + k + 1] : vb[off + k - 1] + 1, y = x - k, sx = x, sy = y;
                while (x < sn && y < sm && eq(x1 - 1 - x, y1 - 1 - y)) { ++x; ++y; }
                vb[off + k] = x;
                if (delta % 2 == 0 && abs(delta - k) <= d && x + vf[off + delta - k] >= sn) { return {2 * d, x1 - x, y1 - y, x1 - sx, y1 - sy}; }}}};
    auto rec = [&](auto &&self, int x0, int x1, int y0, int y1) -> void {
        array<int, 5> mid{};
        if (x0 < x1 && y0 < y1) { mid = snake(x0, x1, y0, y1); }
        if (x0 == x1 || y0 == y1 || mid[0] <= 1) {
            while (x0 < x1 && y0 < y1 && eq(x0, y0)) { res.push_back({'=', x0++, y0++}); }
            if (x1 - x0 > y1 - y0) { res.push_back({'-', x0++, y0}); }
            else if (y1 - y0 > x1 - x0) { res.push_back({'+', x0, y0++}); }
            while (x0 < x1 && y0 < y1) { res.push_back({'=', x0++, y0++}); }
            while (x0 < x1) { res.push_back({'-', x0++, y0}); }
            while (y0 < y1) { res.push_back({'+', x0, y0++}); }
            return;}
        self(self, x0, mid[1], y0, mid[2]);
        for (int x = mid[1], y = mid[2]; x < mid[3]; ++x, ++y) { res.push_back({'=', x, y}); }
        self(self, mid[3], x1, mid[4], y1);};
    rec(rec, 0, n, 0, m);
    return res;}

// T: lcsWitness + O(n + m), M: O(n + m); a shortest common supersequence (string for char input, vector otherwise).
template<class A, class B> auto shortestCommonSupersequence(const A &a, const B &b) {
    using V = lcs_detail::Value<A>;
    std::conditional_t<std::is_same_v<V, char>, string, vector<V>> res;
    int i = 0, j = 0;
    for (auto [p, q] : lcsWitness(a, b)) {
        while (i < p) { res.push_back(a[i++]); }
        while (j < q) { res.push_back(b[j++]); }
        res.push_back(a[i]);
        ++i; ++j;}
    while (i < int(a.size())) { res.push_back(a[i++]); }
    while (j < int(b.size())) { res.push_back(b[j++]); }
    return res;}
// T: O(n * m * k), M: O(m * k); length of a longest common subsequence of three sequences.
template<class A, class B, class C> int lcs3(const A &a, const B &b, const C &c) {
    int n = int(a.size()), m = int(b.size()), k = int(c.size());
    assert(lll(m + 1) * (k + 1) < INT_MAX);
    vector<int> prev((m + 1) * (k + 1)), cur(prev.size());
    for (int i = 0; i < n; ++i) {
        for (int j = 1; j <= m; ++j) {
            for (int l = 1; l <= k; ++l) {
                int at = j * (k + 1) + l;
                bool same = lcs_detail::at(a, i) == lcs_detail::at(b, j - 1) && lcs_detail::at(b, j - 1) == lcs_detail::at(c, l - 1);
                cur[at] = same ? prev[at - k - 2] + 1 : max({prev[at], cur[at - k - 1], cur[at - 1]});}}
        prev.swap(cur);}
    return prev.back();}
template<class S> int longestPalindromicSubsequence(const S &s) { return lcs(s, vector<lcs_detail::Value<S>>(s.rbegin(), s.rend())); }

// T: O(n * log(n + 1) + S) (bytes: O(n + S)), M: O(n + S); counts in the ring T, the empty subsequence included.
template<class T, class S> T countDistinctSubsequences(const S &s) {
    vector<int> id = lcs_detail::ids(s);
    vector<T> last(id.empty() ? 0 : *std::max_element(id.begin(), id.end()) + 1, T(0));
    T res(1);
    for (int c : id) {
        T add = res - last[c];
        last[c] = res;
        res += add;}
    return res;}
// T: O(n^2 + n * log(n + 1)), M: O(n * min(n, S)) distinct, O(n) multiset; nonempty palindromic subsequences, distinct strings or index sets, in the ring T.
template<class T, class S> T countPalindromicSubsequences(const S &s, bool distinct) {
    vector<int> id = lcs_detail::ids(s);
    int n = int(id.size());
    if (!n) { return T(0); }
    if (!distinct) {
        vector<T> prev(n + 1, T(0)), cur(n + 1, T(0));
        for (int i = n - 1; i >= 0; --i) {
            cur[i] = T(0);
            cur[i + 1] = T(1);
            for (int j = i + 1; j < n; ++j) {
                cur[j + 1] = prev[j + 1] + cur[j] - prev[j];
                if (id[i] == id[j]) { cur[j + 1] += prev[j] + T(1); }}
            prev.swap(cur);}
        return prev[n];}
    vector<int> nxt(n, n), prv(n, -1), seen(*std::max_element(id.begin(), id.end()) + 1, -1);
    for (int i = 0; i < n; ++i) {
        if (seen[id[i]] >= 0) { prv[i] = seen[id[i]]; nxt[seen[id[i]]] = i; }
        seen[id[i]] = i;}
    vector<vector<T>> rows(n + 1);
    auto get = [&](int i, int j) { return j < i ? T(0) : rows[i][j - i]; };
    for (int i = n - 1; i >= 0; --i) {
        rows[i].assign(n - i, T(0));
        rows[i][0] = T(1);
        for (int j = i + 1; j < n; ++j) {
            T inner = get(i + 1, j - 1), &d = rows[i][j - i];
            if (id[i] != id[j]) { d = get(i + 1, j) + rows[i][j - i - 1] - inner; }
            else if (nxt[i] > prv[j]) { d = inner + inner + T(2); }
            else if (nxt[i] == prv[j]) { d = inner + inner + T(1); }
            else { d = inner + inner - get(nxt[i] + 1, prv[j] - 1); }}
        if (nxt[i] < n) { vector<T>().swap(rows[nxt[i] + 1]); }
        if (prv[i] < 0 && i + 1 < n) { vector<T>().swap(rows[i + 1]); }}
    return rows[0][n - 1];}
