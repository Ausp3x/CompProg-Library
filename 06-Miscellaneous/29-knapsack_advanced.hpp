#pragma once
#include "../01-Core/01-template.hpp"
#include "../05-Mathematics/09-modinverse.hpp"
#include "../05-Mathematics/14-polynomial_algorithms.hpp"
#include "13-knapsack.hpp"

// Exact-weight table entry of a weight no selection reaches; finite entries stay above KNAPSACK_UNREACHABLE / 2.
inline constexpr lll KNAPSACK_UNREACHABLE = -(lll(1) << 126);

// T: NA, M: O(1); weights >= 0, count=-1 unlimited else 0 <= count <= INT_MAX.
struct KnapsackItem2D { lng weight1, weight2, value; int count = 1; };

namespace knapsack_advanced_detail {
    // T: O(1), M: O(1); weight >= 0, count >= -1, no unlimited zero-weight item with a positive value; zeroOne also asks count <= 1.
    inline void check([[maybe_unused]] const KnapsackItem &x, [[maybe_unused]] bool zero_one = false) {
        assert(x.weight >= 0 && x.count >= -1 && (x.weight || x.count != -1 || x.value <= 0));
        assert(!zero_one || (0 <= x.count && x.count <= 1));}
    // T: O(n), M: O(1); n = dp.size(); adds the positive value of a finite zero-weight item to every reachable entry.
    inline void addFree(vector<lll> &dp, const KnapsackItem &x) {
        if (x.value <= 0 || x.count < 0) { return; }
        for (auto &d : dp) {
            if (d != KNAPSACK_UNREACHABLE) { d += lll(x.value) * x.count; }}}
    // T: O(m), M: O(m); concave g; h[t] = max over 0 <= t - s < |g| of a[s] + g[t - s], by SMAWK on the m x m band.
    inline vector<lll> concaveMaxPlus(const vector<lll> &a, const vector<lll> &g) {
        int m = int(a.size()), len = int(g.size()) - 1;
        // Out-of-band entries rank first by their distance to the band, which keeps the matrix totally monotone.
        auto key = [&](int t, int s) { return pair(s > t ? t - s : t - s > len ? len - (t - s) : 0, a[s] + g[std::clamp(t - s, 0, len)]); };
        auto better = [&](int t, int j, int k) { return key(t, k) >= key(t, j); };
        auto solve = [&](auto &&solve, const vector<int> &rows, const vector<int> &cols) -> vector<int> {
            int n = int(rows.size());
            if (!n) { return {}; }
            vector<int> live, odd;
            for (int c : cols) {
                while (!live.empty() && better(rows[live.size() - 1], live.back(), c)) { live.pop_back(); }
                if (int(live.size()) < n) { live.push_back(c); }}
            for (int i = 1; i < n; i += 2) { odd.push_back(rows[i]); }
            auto sub = solve(solve, odd, live);
            vector<int> res(n);
            for (int i = 0; i < int(sub.size()); ++i) { res[2 * i + 1] = sub[i]; }
            for (int i = 0, j = 0; i < n; i += 2) {
                res[i] = live[j];
                int end = i + 1 == n ? live.back() : res[i + 1];
                while (live[j] != end) {
                    ++j;
                    if (better(rows[i], res[i], live[j])) { res[i] = live[j]; }}}
            return res;};
        vector<int> id(m);
        iota(id.begin(), id.end(), 0);
        auto arg = solve(solve, id, id);
        vector<lll> h(m);
        for (int t = 0; t < m; ++t) { h[t] = a[arg[t]] + g[t - arg[t]]; }
        return h;}
    // T: O(n) op calls, M: O(n); res[s] folds a[max(0, s - c) .. s] under an associative, commutative op: a two-stack queue split at mid.
    template<class S, class Op>
    vector<S> windowFold(const vector<S> &a, int c, Op op) {
        int n = int(a.size()), mid = 0;
        vector<S> res, suf = a;
        S back = a[0];
        for (int s = 0; s < n; ++s) {
            int lo = max(0, s - c);
            back = mid == s ? a[s] : op(back, a[s]);
            if (lo > mid) {
                for (int i = s - 1; i >= lo; --i) { suf[i] = op(a[i], suf[i + 1]); }
                mid = s + 1;}
            res.push_back(lo == mid ? back : mid > s ? suf[lo] : op(suf[lo], back));}
        return res;}
} // namespace knapsack_advanced_detail

// T: O((n + 1) * (W + 1)), M: O(W + 1); W = capacity; items as knapsack_advanced_detail::check; exact-weight maxima, KNAPSACK_UNREACHABLE when unreachable.
inline vector<lll> boundedKnapsackMonotoneQueue(int capacity, const vector<KnapsackItem> &items) {
    assert(0 <= capacity && capacity < INT_MAX);
    vector<lll> dp(capacity + 1, KNAPSACK_UNREACHABLE); dp[0] = 0;
    vector<pair<int, lll>> q(capacity + 1);
    for (const auto &x : items) {
        knapsack_advanced_detail::check(x);
        if (!x.count || x.weight > capacity) { continue; }
        if (!x.weight) { knapsack_advanced_detail::addFree(dp, x); continue; }
        int w = int(x.weight), c = x.count < 0 ? capacity / w : min(x.count, capacity / w);
        // Per residue class, dp[r + t * w] - t * value is a sliding-window maximum over the last c + 1 positions.
        for (int r = 0; r < w; ++r) {
            int head = 0, tail = 0;
            for (int t = 0, top = (capacity - r) / w; t <= top; ++t) {
                int j = r + t * w;
                if (dp[j] != KNAPSACK_UNREACHABLE) {
                    lll key = dp[j] - lll(t) * x.value;
                    while (tail > head && q[tail - 1].second <= key) { --tail; }
                    q[tail++] = {t, key};}
                while (head < tail && q[head].first < t - c) { ++head; }
                dp[j] = head < tail ? q[head].second + lll(t) * x.value : KNAPSACK_UNREACHABLE;}}}
    return dp;}

// T: O((n + 1) * log(n + 2) + (D + 1) * (W + 1)), M: O(n + W + 1); W = capacity, D distinct positive weights <= W; table and items as boundedKnapsackMonotoneQueue.
inline vector<lll> smallWeightKnapsack(int capacity, const vector<KnapsackItem> &items) {
    assert(0 <= capacity && capacity < INT_MAX);
    vector<lll> dp(capacity + 1, KNAPSACK_UNREACHABLE); dp[0] = 0;
    vector<KnapsackItem> sorted;
    for (const auto &x : items) {
        knapsack_advanced_detail::check(x);
        if (!x.count || x.weight > capacity) { continue; }
        if (x.weight) { sorted.push_back(x); }
        else { knapsack_advanced_detail::addFree(dp, x); }}
    sort(sorted.begin(), sorted.end(), [](const auto &x, const auto &y) { return x.weight != y.weight ? x.weight < y.weight : x.value > y.value; });
    // One weight class at a time: the best c copies have concave prefix sums g, so each residue class is a concave max-plus product.
    for (int l = 0, r = 0; l < int(sorted.size()); l = r) {
        int w = int(sorted[l].weight), top = capacity / w;
        vector<lll> g{0};
        for (; r < int(sorted.size()) && sorted[r].weight == w; ++r) {
            for (lng c = sorted[r].count < 0 ? top : sorted[r].count; c > 0 && int(g.size()) <= top; --c) { g.push_back(g.back() + sorted[r].value); }}
        for (int s = 0; s < w; ++s) {
            vector<lll> a((capacity - s) / w + 1);
            for (int t = 0; t < int(a.size()); ++t) { a[t] = dp[s + t * w]; }
            auto h = knapsack_advanced_detail::concaveMaxPlus(a, g);
            for (int t = 0; t < int(a.size()); ++t) { dp[s + t * w] = h[t] < KNAPSACK_UNREACHABLE / 2 ? KNAPSACK_UNREACHABLE : h[t]; }}}
    return dp;}

// T: O((G + k + 1) * (W + 1)), M: O(W + 1); W = capacity, G groups, k items, 0/1 items (count 0 or 1); exact-weight maxima choosing at most one item per group.
inline vector<lll> groupKnapsack(int capacity, const vector<vector<KnapsackItem>> &groups) {
    assert(0 <= capacity && capacity < INT_MAX);
    vector<lll> dp(capacity + 1, KNAPSACK_UNREACHABLE), next; dp[0] = 0;
    for (const auto &group : groups) {
        next = dp;
        for (const auto &x : group) {
            knapsack_advanced_detail::check(x, true);
            if (!x.count || x.weight > capacity) { continue; }
            for (int j = int(x.weight); j <= capacity; ++j) {
                if (dp[j - x.weight] != KNAPSACK_UNREACHABLE) { next[j] = max(next[j], dp[j - x.weight] + x.value); }}}
        swap(dp, next);}
    return dp;}

// T: O((W1 + 1) * (W2 + 1) * (n + 1)), M: O((W1 + 1) * (W2 + 1)); W1, W2 = capacities; no unlimited (0, 0)-weight positive item; exact two-resource maxima.
inline vector<vector<lll>> twoDimensionalKnapsack(int cap1, int cap2, const vector<KnapsackItem2D> &items) {
    assert(0 <= cap1 && cap1 < INT_MAX && 0 <= cap2 && cap2 < INT_MAX);
    vector<vector<lll>> dp(cap1 + 1, vector<lll>(cap2 + 1, KNAPSACK_UNREACHABLE)); dp[0][0] = 0;
    vector<pair<int, lll>> q(max(cap1, cap2) + 1);
    for (const auto &x : items) {
        assert(x.weight1 >= 0 && x.weight2 >= 0 && x.count >= -1 && (x.weight1 || x.weight2 || x.count != -1 || x.value <= 0));
        if (!x.count || x.weight1 > cap1 || x.weight2 > cap2) { continue; }
        if (!x.weight1 && !x.weight2) {
            for (auto &row : dp) { knapsack_advanced_detail::addFree(row, {0, x.value, x.count}); }
            continue;}
        int w1 = int(x.weight1), w2 = int(x.weight2);
        // Each chain (i0 + t * w1, j0 + t * w2) starts at i0 < w1 or j0 < w2; dp - t * value is a sliding-window maximum over its last c + 1 positions.
        for (int i0 = 0; i0 <= cap1; ++i0) {
            for (int j0 = 0, end = i0 < w1 ? cap2 : min(cap2, w2 - 1); j0 <= end; ++j0) {
                int top = min(w1 ? (cap1 - i0) / w1 : INT_MAX, w2 ? (cap2 - j0) / w2 : INT_MAX), c = x.count < 0 ? top : min(x.count, top), head = 0, tail = 0;
                for (int t = 0; t <= top; ++t) {
                    lll &d = dp[i0 + t * w1][j0 + t * w2];
                    if (d != KNAPSACK_UNREACHABLE) {
                        lll key = d - lll(t) * x.value;
                        while (tail > head && q[tail - 1].second <= key) { --tail; }
                        q[tail++] = {t, key};}
                    while (head < tail && q[head].first < t - c) { ++head; }
                    d = head < tail ? q[head].second + lll(t) * x.value : KNAPSACK_UNREACHABLE;}}}}
    return dp;}

// T: O((n + 1) * (W + 1)), M: O(n + (W + 1) * log(n + 2)); W = capacity, forest parent[v] (-1 for roots), 0/1 items; exact-weight maxima over sets containing each member's parent.
inline vector<lll> treeKnapsack(int capacity, const vector<int> &parent, const vector<KnapsackItem> &items) {
    int n = int(parent.size());
    assert(0 <= capacity && capacity < INT_MAX && items.size() == parent.size());
    vector<vector<int>> child(n + 1);
    for (int v = 0; v < n; ++v) {
        assert(-1 <= parent[v] && parent[v] < n);
        knapsack_advanced_detail::check(items[v], true);
        child[parent[v] < 0 ? n : parent[v]].push_back(v);}
    vector<int> size(n + 1, 1), order{n}, pre;
    for (int i = 0; i < int(order.size()); ++i) {
        for (int c : child[order[i]]) { order.push_back(c); }}
    assert(int(order.size()) == n + 1);
    for (int i = int(order.size()) - 1; i > 0; --i) { size[parent[order[i]] < 0 ? n : parent[order[i]]] += size[order[i]]; }
    // Heaviest child last: a table stays live only across a light edge, so at most log2(n) + 2 coexist.
    for (auto &c : child) {
        if (!c.empty()) { swap(*std::max_element(c.begin(), c.end(), [&](int u, int v) { return size[u] < size[v]; }), c.back()); }}
    for (vector<int> st{n}; !st.empty();) {
        int v = st.back(); st.pop_back();
        if (v < n) { pre.push_back(v); }
        for (int i = int(child[v].size()) - 1; i >= 0; --i) { st.push_back(child[v][i]); }}
    int cnt = int(pre.size());
    vector<pair<int, vector<lll>>> st;
    st.emplace_back(cnt, vector<lll>(capacity + 1, KNAPSACK_UNREACHABLE)); st[0].second[0] = 0;
    // Table i covers preorder suffix [i, cnt): take v or skip v's subtree to its end; that end table outlives v only if v is a last child.
    for (int i = cnt - 1; i >= 0; --i) {
        int v = pre[i], end = i + size[v], k = int(st.size()) - 1;
        bool last = child[parent[v] < 0 ? n : parent[v]].back() == v;
        while (st[k].first < end) { --k; }
        const auto &x = items[v];
        const auto &skip = st[k].second;
        bool use = x.count && x.weight <= capacity;
        vector<lll> cur;
        if (use && end > i + 1) { cur = std::move(st.back().second); }
        else { cur = skip; }
        for (int j = capacity; use && j >= 0; --j) {
            lll take = j >= x.weight && cur[j - x.weight] != KNAPSACK_UNREACHABLE ? cur[j - x.weight] + x.value : KNAPSACK_UNREACHABLE;
            cur[j] = max(skip[j], take);}
        while (st.back().first < end + !last) { st.pop_back(); }
        st.emplace_back(i, std::move(cur));}
    return std::move(st.back().second);}

// S: O(W + 1), U: O(W + 1), Q: NA, M: O(W + 1), in T operations; W = capacity; ways[w] counts multiplicity vectors of exact weight w, T needs +, - and construction from lng.
// weight >= 1, count=-1 unlimited else count >= 0; add multiplies by the item's series (1 - x^((c + 1) * w)) / (1 - x^w), remove divides, in any order.
template<class T>
struct RemovableKnapsackCounts {
    vector<T> ways;

    explicit RemovableKnapsackCounts(int capacity) {
        assert(0 <= capacity && capacity < INT_MAX);
        ways.assign(capacity + 1, T(0)); ways[0] = T(1);}

    void add(const KnapsackItem &x) { update(x, false); }
    void remove(const KnapsackItem &x) { update(x, true); }
    void update(const KnapsackItem &x, bool undo) {
        assert(x.weight >= 1 && x.count >= -1);
        int cap = int(ways.size()) - 1;
        lll big = x.count < 0 ? lll(cap) + 1 : (lll(x.count) + 1) * x.weight;
        auto times = [&](lll m) {
            for (int j = cap; m <= cap && j >= int(m); --j) { ways[j] = T(ways[j] - ways[j - int(m)]); }};
        auto over = [&](lll m) {
            for (int j = int(min(m, lll(cap) + 1)); j <= cap; ++j) { ways[j] = T(ways[j] + ways[j - int(m)]); }};
        if (!x.count) { return; }
        if (undo) { times(x.weight); over(big); }
        else { times(big); over(x.weight); }}
};

// T: O((n + z * log(k + 1) + 1) * (W + 1) * k), M: O((W + 1) * k); W = capacity, z zero-weight items with count -1 or >= 2; count >= -1, unlimited weight >= 1 or value <= 0; res[w]: the k largest values of selections of weight <= w, decreasing, per selection (distinct: per value).
inline vector<vector<lll>> kthBestKnapsack(int capacity, const vector<KnapsackItem> &items, int k, bool distinct = false) {
    assert(0 <= capacity && capacity < INT_MAX && k >= 1);
    vector<vector<lll>> dp(capacity + 1, vector<lll>{0});
    vector<lll> buf;
    auto merge = [&](const vector<lll> &a, const vector<lll> &b, lll v, vector<lll> &res) {
        res.clear();
        for (int x = 0, y = 0; int(res.size()) < k && (x < int(a.size()) || y < int(b.size()));) {
            lll z = y == int(b.size()) || (x < int(a.size()) && a[x] >= b[y] + v) ? a[x++] : b[y++] + v;
            if (!distinct || res.empty() || res.back() != z) { res.push_back(z); }}};
    auto join = [&](const vector<lll> &a, const vector<lll> &b) { vector<lll> res; merge(a, b, 0, res); return res; };
    auto shift = [](vector<lll> a, lll v) { for (auto &z : a) { z += v; } return a; };
    for (const auto &x : items) {
        assert(x.weight >= 0 && x.count >= -1 && (x.count != -1 || x.weight >= 1 || x.value <= 0));
        if (!x.count || x.weight > capacity) { continue; }
        int w = int(x.weight);
        if (x.count == 1) {
            for (int j = capacity; j >= w; --j) { merge(dp[j], dp[j - w], x.value, buf); dp[j].swap(buf); }}
        else if (!w) {
            // The k best of copies 0..c lie in k consecutive copies: n shifted copies of a list, unioned by doubling.
            int n = x.count < 0 ? k : int(min(lng(x.count) + 1, lng(k)));
            lll u = x.value < 0 ? -lll(x.value) : lll(x.value), base = x.value > 0 ? lll(x.count) * x.value : 0;
            for (auto &d : dp) {
                vector<lll> res, pw = d;
                lll size = 0, len = 1;
                for (int e = n; e > 0; e >>= 1, len *= 2) {
                    if (e & 1) { merge(res, pw, -size * u, buf); res.swap(buf); size += len; }
                    if (e > 1) { merge(pw, pw, -len * u, buf); pw.swap(buf); }}
                d = shift(std::move(res), base);}}
        else if (x.count < 0 || x.count >= capacity / w) {
            for (int j = w; j <= capacity; ++j) { merge(dp[j], dp[j - w], x.value, buf); dp[j].swap(buf); }}
        else {
            for (int r = 0; r < w; ++r) {
                vector<vector<lll>> a;
                for (int i = 0, top = (capacity - r) / w; i <= top; ++i) { a.push_back(shift(dp[r + i * w], -lll(i) * x.value)); }
                auto res = knapsack_advanced_detail::windowFold(a, x.count, join);
                for (int i = 0; i < int(a.size()); ++i) { dp[r + i * w] = shift(std::move(res[i]), lll(i) * x.value); }}}}
    return dp;}

// T: O((n + 1) * (W + 1)), M: O(W + 1), in T operations; W = capacity; items as kthBestKnapsack but unlimited zero-weight values < 0, T needs +, * and construction from lng; res[w] = {maximum value over selections of weight <= w, how many reach it}.
template<class T>
vector<pair<lll, T>> countOptimalSelections(int capacity, const vector<KnapsackItem> &items) {
    assert(0 <= capacity && capacity < INT_MAX);
    using S = pair<lll, T>;
    vector<S> dp(capacity + 1, {0, T(1)});
    auto best = [](const S &p, const S &q) { return p.first != q.first ? (p.first > q.first ? p : q) : S{p.first, T(p.second + q.second)}; };
    for (const auto &x : items) {
        assert(x.weight >= 0 && x.count >= -1 && (x.count != -1 || x.weight >= 1 || x.value < 0));
        if (!x.count || x.weight > capacity) { continue; }
        int w = int(x.weight);
        auto add = [&](int j) { dp[j] = best(dp[j], {dp[j - w].first + x.value, dp[j - w].second}); };
        if (x.count == 1) { for (int j = capacity; j >= w; --j) { add(j); } }
        else if (!w) {
            // All copies when the value is positive, none when negative, any number of the c + 1 when zero.
            for (auto &d : dp) {
                if (x.value > 0) { d.first += lll(x.count) * x.value; }
                if (!x.value) { d.second = T(d.second * T(lng(x.count) + 1)); }}}
        else if (x.count < 0 || x.count >= capacity / w) { for (int j = w; j <= capacity; ++j) { add(j); } }
        else {
            for (int r = 0; r < w; ++r) {
                vector<S> a;
                for (int i = 0, top = (capacity - r) / w; i <= top; ++i) { a.push_back({dp[r + i * w].first - lll(i) * x.value, dp[r + i * w].second}); }
                auto res = knapsack_advanced_detail::windowFold(a, x.count, best);
                for (int i = 0; i < int(a.size()); ++i) { dp[r + i * w] = {res[i].first + lll(i) * x.value, res[i].second}; }}}}
    return dp;}

// T: O((t + 1) * log(t + 2) + n), M: O(t + 1); prime M::mod() > t, weight >= 0, no unlimited zero weight; the multiplicity-vector counts of KnapsackCounts modulo M::mod().
template<class M>
vector<M> knapsackCountsFps(int t, const vector<KnapsackItem> &items) {
    assert(0 <= t && t < INT_MAX && ulng(t) < ulng(M::mod()));
    vector<lng> coef(t + 1);
    M scale = 1;
    // log of the product is sum over m of coef[m] * log(1 / (1 - x^m)), and log(1 / (1 - x^m)) = sum over j of x^(j * m) / j.
    for (const auto &x : items) {
        assert(x.weight >= 0 && x.count >= -1 && (x.weight || x.count != -1));
        if (!x.count) { continue; }
        if (!x.weight) { scale *= M(lng(x.count) + 1); continue; }
        if (x.weight <= t) { ++coef[x.weight]; }
        if (x.count >= 0 && (lll(x.count) + 1) * x.weight <= t) { --coef[(lng(x.count) + 1) * x.weight]; }}
    auto iv = inverseTable<M>(t);
    vector<M> lg(t + 1, M(0));
    for (int m = 1; m <= t; ++m) {
        if (!coef[m]) { continue; }
        M c(coef[m]);
        for (int j = 1; j <= t / m; ++j) { lg[j * m] += c * iv[j]; }}
    auto res = fpsExp(lg, t + 1);
    for (auto &x : res) { x *= scale; }
    return res;}

// T: O(n * 2^n) worst case, M: O(n); 0/1 items, weight >= 0, capacity >= 0; {maximum value within capacity, one optimal 0/1 vector}.
inline pair<lll, vector<int>> knapsackBranchAndBound(lng capacity, const vector<KnapsackItem> &items) {
    assert(capacity >= 0);
    int n = int(items.size());
    vector<int> take(n), id;
    lll base = 0;
    for (int i = 0; i < n; ++i) {
        const auto &x = items[i];
        knapsack_advanced_detail::check(x, true);
        if (!x.count || x.value <= 0 || x.weight > capacity) { continue; }
        if (!x.weight) { take[i] = 1; base += x.value; }
        else { id.push_back(i); }}
    sort(id.begin(), id.end(), [&](int i, int j) { return lll(items[i].value) * items[j].weight > lll(items[j].value) * items[i].weight; });
    int m = int(id.size());
    vector<lll> pw(m + 1), pv(m + 1);
    for (int i = 0; i < m; ++i) { pw[i + 1] = pw[i] + items[id[i]].weight; pv[i + 1] = pv[i] + items[id[i]].value; }
    // Dantzig bound: whole items in ratio order from i, then the fractional break item, floored since values are integers.
    auto bound = [&](int i, lng rem) {
        int k = int(upper_bound(pw.begin() + i, pw.end(), pw[i] + rem) - pw.begin()) - 1;
        lll res = pv[k] - pv[i];
        if (k < m) { res += (rem - (pw[k] - pw[i])) * items[id[k]].value / items[id[k]].weight; }
        return res;};
    lll best = 0, val = 0;
    lng rem = capacity;
    vector<int> cur(m), sol(m);
    bool stale = false;
    // Depth-first, take before skip; a dead end resumes at the skip branch of the deepest taken item, and saves cur once if best rose.
    for (int i = 0;;) {
        if (val > best) { best = val; stale = true; }
        if (i < m && val + bound(i, rem) > best) {
            const auto &x = items[id[i]];
            if (x.weight <= rem) { cur[i] = 1; rem -= x.weight; val += x.value; }
            ++i; continue;}
        if (stale) { sol = cur; stale = false; }
        while (i > 0 && !cur[i - 1]) { --i; }
        if (!i) { break; }
        cur[--i] = 0; rem += items[id[i]].weight; val -= items[id[i]].value; ++i;}
    for (int i = 0; i < m; ++i) { take[id[i]] = sol[i]; }
    return {base + best, take};}

// T: O((n + 1) * (m + 1)), M: O(n + m + 1) (trace O(n * (m + 1))); m = max weight <= INT_MAX / 2, t >= 0, weights >= 0; {largest subset sum <= t, one 0/1 vector when traced}.
inline pair<lng, vector<int>> pisingerSubsetSum(lng t, const vector<int> &w, bool trace = false) {
    assert(t >= 0);
    vector<int> id, take(trace ? w.size() : 0);
    for (int i = 0; i < int(w.size()); ++i) {
        assert(w[i] >= 0);
        if (w[i]) { id.push_back(i); }}
    int n = int(id.size()), b = 0, m = 0;
    lng a = 0;
    while (b < n && a + w[id[b]] <= t) { a += w[id[b++]]; }
    for (int i = 0; i < b && trace; ++i) { take[id[i]] = 1; }
    if (b == n) { return {a, take}; }
    for (int i : id) { m = max(m, w[i]); }
    assert(m <= INT_MAX / 2);
    // Balanced states: v[x] is the largest j keeping items [0, j) of the break prefix at sum t - m + 1 + x; add below t, remove above.
    vector<int> v(2 * m, -1), u;
    vector<vector<int>> layer;
    v[a - t + m - 1] = b;
    if (trace) { layer.push_back(v); }
    for (int i = b; i < n; ++i) {
        u = v;
        for (int x = 0; x < m; ++x) { v[x + w[id[i]]] = max(v[x + w[id[i]]], u[x]); }
        for (int x = 2 * m - 1; x >= m; --x) {
            for (int j = max(0, u[x]); j < v[x]; ++j) { v[x - w[id[j]]] = max(v[x - w[id[j]]], j); }}
        if (trace) { layer.push_back(v); }}
    int x = m - 1;
    while (v[x] < 0) { --x; }
    lng best = t - m + 1 + x;
    for (int i = n - 1; trace && i >= b;) {
        const auto &cur = layer[i - b + 1], &prev = layer[i - b];
        int j = cur[x], wi = w[id[i]];
        if (prev[x] == j) { --i; }
        else if (x >= wi && x - wi < m && prev[x - wi] == j) { take[id[i--]] = 1; x -= wi; }
        else { take[id[j]] = 0; x += w[id[j]]; }}
    return {best, take};}

// T: O(n + m^2 * log(C + 2)), M: O(m + log(C + 2)); C = capacity >= 0, m = max useful weight < INT_MAX / 2 (useful: weight <= C, value > 0); unlimited items (count -1), weight >= 1; maximum value of weight <= C.
inline lll unboundedKnapsackLargeCapacity(lng capacity, const vector<KnapsackItem> &items) {
    assert(capacity >= 0);
    int m = 0;
    for (const auto &x : items) {
        assert(x.weight >= 1 && x.count == -1);
        if (x.weight <= capacity && x.value > 0) { assert(x.weight < INT_MAX / 2); m = max(m, int(x.weight)); }}
    if (!m) { return 0; }
    vector<lng> gain(m + 1);
    for (const auto &x : items) {
        if (x.weight <= m && x.value > 0) { gain[x.weight] = max(gain[x.weight], x.value); }}
    int base = int(min(capacity, lng(2 * m)));
    vector<lll> f(base + 1);
    for (int c = 1; c <= base; ++c) {
        f[c] = f[c - 1];
        for (int w = 1; w <= min(c, m); ++w) { if (gain[w]) { f[c] = max(f[c], f[c - w] + gain[w]); } }}
    // A multiset of weight <= c > 2m splits into halves within m of each other, so f(c) = max f(a) + f(c - a) over |2a - c| <= m.
    vector<pair<lng, lng>> lv{{capacity, capacity}};
    while (lv.back().second > base) {
        auto [l, r] = lv.back();
        lv.push_back({l > m ? (l - m + 1) / 2 : 0, lng((lll(r) + m) / 2)});}
    vector<lll> below;
    for (int k = int(lv.size()) - 1; k >= 0; --k) {
        auto [l, r] = lv[k];
        vector<lll> cur(r - l + 1);
        auto get = [&](lng y) { return y <= base ? f[y] : below[y - lv[k + 1].first]; };
        for (int i = 0; i < int(cur.size()); ++i) {
            lng c = l + i;
            if (c <= base) { cur[i] = f[c]; continue; }
            lll res = 0;
            for (lng a = (c - m + 1) / 2; a <= c - a; ++a) { res = max(res, get(a) + get(c - a)); }
            cur[i] = res;}
        below = std::move(cur);}
    return below[0];}

// T: O((n + 1) * log(n + 2) + P * (n + 1 + sum(log(c_i + 1)))), M: O(P + n); vmax = max value, c_i = min(count, vmax - 1), P = sum c_i * value_i < INT_MAX; weight >= 1, value >= 0; maximum value of weight <= capacity.
inline lll boundedKnapsackSmallValue(lng capacity, const vector<KnapsackItem> &items) {
    assert(capacity >= 0);
    vector<KnapsackItem> keep;
    lng vmax = 0;
    for (const auto &x : items) {
        assert(x.weight >= 1 && x.value >= 0 && x.count >= -1);
        if (x.count && x.value && x.weight <= capacity) { keep.push_back(x); vmax = max(vmax, x.value); }}
    if (keep.empty()) { return 0; }
    sort(keep.begin(), keep.end(), [](const auto &x, const auto &y) { return lll(x.value) * y.weight > lll(y.value) * x.weight; });
    // Exchange argument: some optimum takes at most vmax - 1 copies of each item beyond a greedy ratio-order fill of the remaining copies.
    vector<KnapsackItem> small;
    lll axis = 0;
    for (const auto &x : keep) {
        lng c = x.count < 0 ? vmax - 1 : min(lng(x.count), vmax - 1);
        axis = min(axis + lll(c) * x.value, lll(INT_MAX));
        small.push_back({x.weight, x.value, int(c)});}
    assert(axis < INT_MAX);
    int top = int(min(axis, lll(INT_MAX - 1)));
    ValueKnapsackDP dp(top, small);
    lll res = 0;
    for (int u = 0; u <= top; ++u) {
        auto r = dp.minWeight(u);
        if (r.status != KnapsackStatus::finite || r.value > capacity) { continue; }
        lll val = u, cap = capacity - r.value;
        for (int i = 0; i < int(keep.size()); ++i) {
            lll more = cap / keep[i].weight;
            if (keep[i].count >= 0) { more = min(more, lll(keep[i].count - small[i].count)); }
            val += more * keep[i].value; cap -= more * keep[i].weight;}
        res = max(res, val);}
    return res;}
