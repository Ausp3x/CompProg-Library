#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/00-monoids.hpp"
#include "../02-Data Structures/04-sparsetable.hpp"

namespace kbest_detail {
    // T: O(1), M: O(1); heap order: smaller sum under cmp first, ties by the lexicographically smaller array.
    template<class T, class C, int K>
    struct Later {
        C cmp;
        bool operator()(const pair<T, array<int, K>> &x, const pair<T, array<int, K>> &y) const {
            return cmp(y.first, x.first) || (!cmp(x.first, y.first) && y.second < x.second);}
    };
    // T: O(1), M: O(1); an empty min-heap of (sum, K-int state) pairs under Later.
    template<int K, class T, class C>
    auto frontier(C cmp) {
        return priority_queue<pair<T, array<int, K>>, vector<pair<T, array<int, K>>>, Later<T, C, K>>(Later<T, C, K>{cmp});}
} // namespace kbest_detail

// T: O((k + 1) * log(k + 2)), M: O(k + 1); a, b sorted under cmp; index pairs of the k smallest a[i] + b[j] (all when fewer), ordered by (sum, i, j).
template<class T, class C = std::less<>>
vector<pair<int, int>> kSmallestPairSums(const vector<T> &a, const vector<T> &b, int k, C cmp = {}) {
    assert(k >= 0);
    vector<pair<int, int>> res;
    auto pq = kbest_detail::frontier<2, T>(cmp);
    for (int i = 0; i < min(int(a.size()), k) && !b.empty(); ++i) { pq.push({a[i] + b[0], {i, 0}}); }
    while (int(res.size()) < k && !pq.empty()) {
        auto [i, j] = pq.top().second; pq.pop();
        res.emplace_back(i, j);
        if (j + 1 < int(b.size())) { pq.push({a[i] + b[j + 1], {i, j + 1}}); }}
    return res;}

// T: O((n + k + 1) * log(n + k + 2)), M: O((n + 1) * log(n + 2) + k); 1 <= min_len <= max_len; ranges [l, r), min_len <= r - l <= max_len, of the k smallest sums, ordered by (sum, l, r).
template<class T, class C = std::less<>>
vector<pair<int, int>> kSmallestSubarraySums(const vector<T> &a, int k, int min_len, int max_len, C cmp = {}) {
    int n = int(a.size());
    assert(k >= 0 && 1 <= min_len && min_len <= max_len);
    vector<T> p{T(0)};
    vector<int> id(n + 1);
    for (int i = 0; i < n; ++i) { p.push_back(T(p.back() + a[i])); }
    iota(id.begin(), id.end(), 0);
    // Leftmost index of the minimum prefix under this cmp object, so stateful comparators work.
    auto low = [&](int i, int j) { return j < 0 || (i >= 0 && !cmp(p[j], p[i])) ? i : j; };
    SparseTable st(id, Monoid{-1, low});
    auto pq = kbest_detail::frontier<4, T>(cmp);
    auto add = [&](int l, int lo, int hi) {
        if (lo <= hi) { int r = st.prod(lo, hi + 1); pq.push({T(p[r] - p[l]), {l, r, lo, hi}}); }};
    for (int l = 0; n - l >= min_len; ++l) { add(l, l + min_len, l + min(max_len, n - l)); }
    vector<pair<int, int>> res;
    while (int(res.size()) < k && !pq.empty()) {
        auto [l, r, lo, hi] = pq.top().second; pq.pop();
        res.emplace_back(l, r); add(l, lo, r - 1); add(l, r + 1, hi);}
    return res;}

// S: O(1) per output, U: NA, Q: O(n) subset, M: O(n + k); n elements, k outputs; output e has sorted positions [0, prefix[e]) and the node list from head[e], order maps them to indices, flip complements.
template<class T>
struct KSmallestSubsets {
    vector<T> sums;
    vector<int> prefix, head, order;
    vector<pair<int, int>> node;
    vector<char> flip;

    void emit(const T &s, int pre, int pos, int next) {
        sums.push_back(s); prefix.push_back(pre); head.push_back(pos < 0 ? -1 : int(node.size()));
        if (pos >= 0) { node.push_back({pos, next}); }}

    vector<int> subset(int e) const {
        assert(0 <= e && e < int(sums.size()));
        vector<char> in = flip;
        for (int i = 0; i < prefix[e]; ++i) { in[order[i]] ^= 1; }
        for (int x = head[e]; x != -1; x = node[x].second) { in[order[node[x].first]] ^= 1; }
        vector<int> res;
        for (int i = 0; i < int(in.size()); ++i) {
            if (in[i]) { res.push_back(i); }}
        return res;}
};

// T: O((n + 1) * log(n + 2) + k * log(k + 1)), M: O(n + k); T an ordered group under cmp; the k smallest of the 2^n subset sums, empty set first, ties by discovery.
template<class T, class C = std::less<>>
KSmallestSubsets<T> kSmallestSubsetSums(const vector<T> &a, int k, C cmp = {}) {
    int n = int(a.size());
    assert(k >= 0);
    KSmallestSubsets<T> res; res.flip.assign(n, 0); res.order.resize(n);
    vector<T> b(n); T base = T(0);
    // Cmp-negative elements start taken; choosing one in the frontier drops it at cost -a[i].
    for (int i = 0; i < n; ++i) {
        res.flip[i] = cmp(a[i], T(0));
        if (res.flip[i]) { base = T(base + a[i]); b[i] = T(-a[i]); }
        else { b[i] = a[i]; }}
    iota(res.order.begin(), res.order.end(), 0);
    std::stable_sort(res.order.begin(), res.order.end(), [&](int i, int j) { return cmp(b[i], b[j]); });
    if (!k) { return res; }
    res.emit(base, 0, -1, -1);
    auto pq = kbest_detail::frontier<3, T>(cmp);
    int seq = 0;
    if (n) { pq.push({T(base + b[res.order[0]]), {seq++, 0, -1}}); }
    while (int(res.sums.size()) < k && !pq.empty()) {
        auto [s, st] = pq.top(); pq.pop();
        int i = st[1], rest = st[2];
        res.emit(s, 0, i, rest);
        if (i + 1 < n) {
            const T &x = b[res.order[i]], &y = b[res.order[i + 1]];
            pq.push({T(s + y), {seq++, i + 1, res.head.back()}});
            pq.push({T(s - x + y), {seq++, i + 1, rest}});}}
    return res;}

// T: O((n + 1) * log(n + 2) + k * log(k + 1)), M: O(n + k); m >= 0; the k smallest sums of m-element subsets (none when m > n), ties by discovery.
template<class T, class C = std::less<>>
KSmallestSubsets<T> kSmallestFixedSizeSubsetSums(const vector<T> &a, int m, int k, C cmp = {}) {
    int n = int(a.size());
    assert(m >= 0 && k >= 0);
    KSmallestSubsets<T> res; res.flip.assign(n, 0); res.order.resize(n);
    iota(res.order.begin(), res.order.end(), 0);
    std::stable_sort(res.order.begin(), res.order.end(), [&](int i, int j) { return cmp(a[i], a[j]); });
    if (m > n || !k) { return res; }
    auto v = [&](int i) -> const T & { return a[res.order[i]]; };
    T base = T(0);
    for (int i = 0; i < m; ++i) { base = T(base + v(i)); }
    if (!m) { res.emit(base, 0, -1, -1); return res; }
    auto pq = kbest_detail::frontier<5, T>(cmp);
    int seq = 0;
    // State {seq, j, p, lim, list}: element j sits at p < lim, elements below j at 0..j-1, elements above j fixed in the list.
    pq.push({base, {seq++, m - 1, m - 1, n, -1}});
    while (int(res.sums.size()) < k && !pq.empty()) {
        auto [s, st] = pq.top(); pq.pop();
        int j = st[1], p = st[2], lim = st[3];
        res.emit(s, j, p, st[4]);
        if (p + 1 < lim) { pq.push({T(s - v(p) + v(p + 1)), {seq++, j, p + 1, lim, st[4]}}); }
        if (j > 0 && j < p) { pq.push({T(s - v(j - 1) + v(j)), {seq++, j - 1, j, p, res.head.back()}}); }}
    return res;}

// S: O(1) per output, U: NA, Q: O(m) choice, M: O(m + k); m lists, k outputs; choice(e)[i] is the element taken from list i: 0 unless a node (sorted list, element, next) from head[e] sets it.
template<class T>
struct KSmallestChoices {
    vector<T> sums;
    vector<int> head, order;
    vector<array<int, 3>> node;
    int m = 0;

    vector<int> choice(int e) const {
        assert(0 <= e && e < int(sums.size()));
        vector<int> res(m);
        for (int x = head[e]; x != -1; x = node[x][2]) { res[order[node[x][0]]] = node[x][1]; }
        return res;}
};

// T: O((m + 1) * log(m + 2) + k * log(k + 1)), M: O(m + k); lists sorted under cmp; the k smallest sums taking one element of every list (none if a list is empty), ties by discovery.
template<class T, class C = std::less<>>
KSmallestChoices<T> kSmallestSortedListSums(const vector<vector<T>> &lists, int k, C cmp = {}) {
    assert(k >= 0);
    KSmallestChoices<T> res; res.m = int(lists.size());
    T base = T(0);
    for (int i = 0; i < res.m; ++i) {
        if (lists[i].empty()) { return res; }
        base = T(base + lists[i][0]);
        if (lists[i].size() > 1) { res.order.push_back(i); }}
    auto gap = [&](int i) { return T(lists[i][1] - lists[i][0]); };
    std::stable_sort(res.order.begin(), res.order.end(), [&](int i, int j) { return cmp(gap(i), gap(j)); });
    if (!k) { return res; }
    res.sums.push_back(base); res.head.push_back(-1);
    int q = int(res.order.size()), seq = 0;
    auto pq = kbest_detail::frontier<4, T>(cmp);
    // State {seq, p, t, list}: sorted list p takes element t >= 1, later lists take 0; lists sorted by first gap make every step nondecreasing.
    if (q) { pq.push({T(base + gap(res.order[0])), {seq++, 0, 1, -1}}); }
    while (int(res.sums.size()) < k && !pq.empty()) {
        auto [s, st] = pq.top(); pq.pop();
        int p = st[1], t = st[2], rest = st[3], e = int(res.node.size());
        res.sums.push_back(s); res.head.push_back(e); res.node.push_back({p, t, rest});
        const auto &l = lists[res.order[p]];
        if (t + 1 < int(l.size())) { pq.push({T(s - l[t] + l[t + 1]), {seq++, p, t + 1, rest}}); }
        if (p + 1 < q) {
            T next = gap(res.order[p + 1]);
            pq.push({T(s + next), {seq++, p + 1, 1, e}});
            if (t == 1) { pq.push({T(s - gap(res.order[p]) + next), {seq++, p + 1, 1, rest}}); }}}
    return res;}

// T: O((k * (n + 1) + 1) * oracle + k * n * log(k * n + 2)), M: O(k * n); oracle(fixed), fixed[i] -1 free or a forced 0/1, returns std::optional{value, 0/1 vector} minimal under cmp or nullopt.
// The k best solutions (all when fewer) in order, ties by discovery; equal values must be reproducible, as each output is solved again.
template<class F, class C = std::less<>>
auto lawlerKBest(int n, int k, F oracle, C cmp = {}) {
    using T = std::decay_t<decltype(oracle(vector<int>())->first)>;
    assert(n >= 0 && k >= 0);
    vector<pair<T, vector<int>>> res;
    vector<vector<int>> fixed;
    auto pq = kbest_detail::frontier<3, T>(cmp);
    int seq = 0;
    if (k) {
        if (auto x = oracle(vector<int>(n, -1))) { pq.push({x->first, {seq++, -1, 0}}); }}
    while (int(res.size()) < k && !pq.empty()) {
        auto [val, st] = pq.top(); pq.pop();
        int par = st[1], t = st[2];
        vector<int> f = par < 0 ? vector<int>(n, -1) : fixed[par];
        for (int i = 0, c = 0; par >= 0 && c <= t; ++i) {
            if (f[i] == -1) { f[i] = c++ < t ? res[par].second[i] : 1 - res[par].second[i]; }}
        auto x = oracle(f);
        assert(x && int(x->second.size()) == n && !cmp(x->first, val) && !cmp(val, x->first));
        res.emplace_back(val, std::move(x->second)); fixed.push_back(f);
        int e = int(res.size()) - 1, c = 0;
        for (int i = 0; i < n; ++i) {
            if (f[i] != -1) { continue; }
            f[i] = 1 - res[e].second[i];
            if (auto y = oracle(f)) { pq.push({y->first, {seq++, e, c}}); }
            f[i] = res[e].second[i]; ++c;}}
    return res;}
