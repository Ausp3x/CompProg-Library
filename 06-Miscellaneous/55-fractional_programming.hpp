#pragma once
#include "../01-Core/01-template.hpp"
#include "02-customhash.hpp"

// T: O(log(A * B + 2) * oracle), M: O(1); A >= |N|, B >= D over feasible sets; oracle(p, q) gives {N, D}, D > 0, of a feasible set maximizing q * N - p * D; q > 0, p / q <= optimum, |p|, |q|, |N|, |D| < 2^63; returns the last oracle result, an optimum.
template<class F>
pair<lng, lng> dinkelbach(F oracle, lng p, lng q) {
    assert(q > 0);
    // A maximizing oracle with |N|, D < 2^63 reaches value 0 within 4 + 63 + 3 * 63 = 256 calls.
    for (int calls = 1;; ++calls) {
        auto [n, d] = oracle(p, q);
        lll v = lll(q) * n - lll(p) * d;
        assert(d > 0 && v >= 0 && (!v || calls < 256));
        if (d <= 0 || v <= 0 || calls >= 256) { return {n, d}; }
        p = n; q = d;}}

// T: NA, M: O(k); num / den of the chosen items in increasing index order; den = 0 marks no feasible set.
struct FractionalSelection { lng num, den; vector<int> items; };

// T: O(n * log(n + 1)) expected, M: O(n); sum |a_i| < 2^62, sum b_i < 2^62, b_i > 0, 1 <= k <= n; exactly k items maximizing sum a / sum b; Las Vegas in seed.
inline FractionalSelection zeroOneFractionalSelect(const vector<lng> &a, const vector<lng> &b, int k, ulng seed = CustomHash::rnd) {
    int n = int(min(a.size(), b.size()));
    lll sa = 0, sv = 0, sw = 0;
    for (int i = 0; i < n; ++i) { sa += abs(lll(a[i])); sv += a[i]; sw += b[i]; }
    bool ok = a.size() == b.size() && 1 <= k && k <= n && sa < lll(1) << 62 && sw < lll(1) << 62 && std::ranges::all_of(b, [](lng x) { return x > 0; });
    assert(ok);
    int need = ok ? k : 0;
    auto below = [](pair<lng, lng> x, pair<lng, lng> y) { return lll(x.first) * y.second < lll(y.first) * x.second; };
    pair<lng, lng> lo{lng(sv), lng(sw)}, hi{1, 0};
    lng num = 0, den = 0;
    vector<int> live(n), taken, cls(n), z, ord;
    vector<lll> key(n);
    iota(live.begin(), live.end(), 0);
    // F(t) = max over k-sets of sum (a - t * b) is 0 exactly at the optimum: F(t) > 0 raises lo to the best set's ratio, F(t) < 0 lowers hi to t.
    auto test = [&](pair<lng, lng> t) {
        for (int j : live) { key[j] = lll(t.second) * a[j] - lll(t.first) * b[j]; }
        ord = live;
        std::nth_element(ord.begin(), ord.begin() + need, ord.end(), [&](int x, int y) { return key[x] > key[y]; });
        lll f = lll(t.second) * num - lll(t.first) * den;
        lng x = num, y = den;
        for (int j = 0; j < need; ++j) { f += key[ord[j]]; x += a[ord[j]]; y += b[ord[j]]; }
        if (f > 0) { lo = {x, y}; }
        else if (f < 0) { hi = t; }
        else {
            taken.insert(taken.end(), ord.begin(), ord.begin() + need);
            num = x; den = y; need = 0;}};
    for (int step = 0; step < 8 && 0 < need && need < n; ++step) { test(lo); }
    while (0 < need && need < int(live.size())) {
        int i = live[CustomHash::splitMix64(seed++) % live.size()];
        // Eppstein-Hirschberg with lo <= optimum < hi: cls 1 if a_j - t * b_j >= a_i - t * b_i there, -1 if <=, 2 if the lines coincide, 0 if unknown.
        auto cross = [&](int j) { lng d = b[j] - b[i]; return d > 0 ? pair(a[j] - a[i], d) : pair(a[i] - a[j], -d); };
        auto side = [&](int j) {
            int up = b[j] < b[i] ? 1 : -1;
            return b[j] == b[i] ? (a[j] > a[i] ? 1 : a[j] < a[i] ? -1 : 2) : !below(lo, cross(j)) ? up : !below(cross(j), hi) ? -up : 0;};
        z.clear();
        for (int j : live) {
            cls[j] = side(j);
            if (!cls[j]) { z.push_back(j); }}
        while (need && 32 * z.size() > live.size()) {
            auto mid = z.begin() + int(z.size()) / 2;
            std::nth_element(z.begin(), mid, z.end(), [&](int x, int y) { return below(cross(x), cross(y)); });
            test(cross(*mid));
            z.erase(std::remove_if(z.begin(), z.end(), [&](int j) { return (cls[j] = side(j)) != 0; }), z.end());}
        if (!need) { break; }
        int up = 0, down = 0, same = 0, drop = int(live.size()) - need, len = 0;
        for (int j : live) { up += cls[j] == 1; down += cls[j] == -1; same += cls[j] == 2; }
        // need items >= f_i: drop those <= f_i and spare copies of i; else drop items <= f_i: take those >= f_i and spare copies of i.
        bool out = up + same >= need, in = !out && down + same >= drop;
        int spare = out ? min(same, up + same - need) : in ? min(same, down + same - drop) : 0;
        for (int j : live) {
            bool twin = cls[j] == 2 && spare > 0;
            spare -= twin;
            if (out && (cls[j] == -1 || twin)) { continue; }
            if (in && (cls[j] == 1 || twin)) { taken.push_back(j); num += a[j]; den += b[j]; --need; }
            else { live[len++] = j; }}
        live.resize(len);}
    if (need) {
        for (int j : live) { taken.push_back(j); num += a[j]; den += b[j]; }}
    sort(taken.begin(), taken.end());
    return {num, den, taken};}

// T: O(n * (need + 1) * log(A * B + 2)), M: O(n * (need + 1)); A = sum |a_i| < 2^62, B = sum b_i < 2^62, b_i >= 1, 1 <= need < INT_MAX; maximum sum a / sum b over sets with sum b >= need.
inline FractionalSelection ratioKnapsack(const vector<lng> &a, const vector<lng> &b, int need) {
    int n = int(min(a.size(), b.size()));
    lll p = 0, q = 0, sa = 0;
    for (int i = 0; i < n; ++i) { p += a[i]; q += b[i]; sa += abs(lll(a[i])); }
    bool ok = a.size() == b.size() && 1 <= need && need < INT_MAX && sa < lll(1) << 62 && q < lll(1) << 62 && std::ranges::all_of(b, [](lng x) { return x >= 1; });
    assert(ok);
    if (!ok || q < need) { return {0, 0, {}}; }
    constexpr lll NONE = -(lll(1) << 126);
    vector<lll> dp(need + 1);
    vector<vector<int>> from(n, vector<int>(need + 1));
    vector<int> best;
    auto oracle = [&](lng s, lng t) {
        fill(dp.begin(), dp.end(), NONE); dp[0] = 0;
        for (int i = 0; i < n; ++i) {
            lll key = lll(t) * a[i] - lll(s) * b[i];
            fill(from[i].begin(), from[i].end(), -1);
            for (int c = need; c >= 0; --c) {
                int to = int(min(lng(need), c + b[i]));
                if (dp[c] != NONE && dp[c] + key > dp[to]) { dp[to] = dp[c] + key; from[i][to] = c; }}}
        best.clear(); lng x = 0, y = 0;
        for (int i = n - 1, c = need; i >= 0; --i) {
            if (from[i][c] >= 0) { best.push_back(i); x += a[i]; y += b[i]; c = from[i][c]; }}
        return pair(x, y);};
    auto [num, den] = dinkelbach(oracle, lng(p), lng(q));
    reverse(best.begin(), best.end());
    return {num, den, best};}
