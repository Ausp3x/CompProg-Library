#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"

// T: O(log(2 + max(|a|, |b|))), M: O(1); all lng a * x + b * y = c; dimension -1 none, 1 line (x, y) + t * (dx, dy), 2 all Z^2.
struct DioSolution {
    int dimension = -1;
    lll x = 0, y = 0, dx = 0, dy = 0, g = 0;
};
inline DioSolution solveDioEq(lng a, lng b, lng c) {
    if (a == 0 && b == 0) { return {c == 0 ? 2 : -1}; }
    auto [g, x, y] = extendedGcd(a, b);
    if (c % g != 0) { return {-1, 0, 0, 0, 0, g}; }
    x *= c / g;
    if (b != 0) {
        lll m = b / g; if (m < 0) { m = -m; }
        x %= m; if (x < 0) { x += m; }
        y = (c - lll(a) * x) / b;}
    return {1, x, y, b / g, -lll(a) / g, g};}

// T: O(log(2 + max(|a|, |b|))), M: O(1); half-open lng box [xl, xr) * [yl, yr); count points, t in [l, r) for dimension 1.
struct DioBox {
    DioSolution solution;
    lll l = 0, r = 0;
    ulll count = 0;
};
inline DioBox solveDioBox(lng a, lng b, lng c, lng xl, lng xr, lng yl, lng yr) {
    assert(xl <= xr && yl <= yr);
    DioBox box{solveDioEq(a, b, c)};
    if (xl == xr || yl == yr || box.solution.dimension < 0) { return box; }
    if (box.solution.dimension == 2) {
        box.count = ulll(lll(xr) - xl) * ulll(lll(yr) - yl); return box;}
    bool initialized = false;
    auto restrict = [&](lll p, lll d, lng lo, lng hi) -> bool {
        if (d == 0) { return lo <= p && p < hi; }
        lll u = lll(lo) - p, v = lll(hi) - 1 - p;
        if (d < 0) { swap(u, v); }
        u = ceilDiv(u, d); v = floorDiv(v, d) + 1;
        if (!initialized) { box.l = u; box.r = v; initialized = true; }
        else { box.l = max(box.l, u); box.r = min(box.r, v); }
        return box.l < box.r;};
    auto &s = box.solution;
    if (!restrict(s.x, s.dx, xl, xr) || !restrict(s.y, s.dy, yl, yr)) {
        box.l = box.r = 0; return box;}
    box.count = ulll(box.r - box.l); return box;}

// T: O(log(2 + m0)), M: O(1); a * x = b mod m0 with m0 > 0, all lng a, b; {x, m} means x + k * m, {-1, -1} none.
inline pair<lng, lng> solveModEq(lng a, lng b, lng m0) {
    assert(m0 > 0);
    auto [g, x, y] = extendedGcd(a, m0);
    if (b % g != 0) { return {-1, -1}; }
    lng m = lng(m0 / g);
    return {modNorm(x * (b / g), m), m};}

// T: O(log(2 + max(|a|, |b|))), M: O(1); box as solveDioBox, minimizes p * x + q * y over its points; {false, 0, 0} when none.
inline tuple<bool, lng, lng> solveDioMinLinear(lng a, lng b, lng c, lng p, lng q, lng xl, lng xr, lng yl, lng yr) {
    auto box = solveDioBox(a, b, c, xl, xr, yl, yr);
    if (!box.count) { return {false, 0, 0}; }
    auto &s = box.solution;
    if (s.dimension == 2) { return {true, p < 0 ? xr - 1 : xl, q < 0 ? yr - 1 : yl}; }
    lll t = lll(p) * s.dx + lll(q) * s.dy < 0 ? box.r - 1 : box.l;
    return {true, lng(s.x + t * s.dx), lng(s.y + t * s.dy)};}

// T: O(k * m), M: O(m); m = min(a), k = |a|, a_i >= 1, m * max(a) < 2^63; d[r] least nonnegative combination = r mod m, -1 none.
inline vector<lng> residueShortestPath(const vector<lng> &a) {
    assert(!a.empty());
    lng m = *std::min_element(a.begin(), a.end());
    assert(m >= 1 && lll(m) * *std::max_element(a.begin(), a.end()) <= std::numeric_limits<lng>::max());
    constexpr lng INF = std::numeric_limits<lng>::max();
    vector<lng> d(m, INF); d[0] = 0;
    for (lng b : a) {
        lng s = b % m, len = m / gcd(m, s);
        for (lng r = 0; s && r < m / len; ++r) {
            lng u = r, best = r;
            for (lng i = 1; i < len; ++i) { u = u + s < m ? u + s : u + s - m; if (d[u] < d[best]) { best = u; } }
            if (d[best] == INF) { continue; }
            u = best;
            for (lng i = 1; i < len; ++i) {
                lng v = u + s < m ? u + s : u + s - m;
                d[v] = min(d[v], d[u] + b); u = v;}}}
    for (lng &x : d) { if (x == INF) { x = -1; } }
    return d;}
// T: O(k * m * log(k * m)), M: O(k * m); as residueShortestPath, by Dijkstra on the residues: lighter, slower.
inline vector<lng> residueShortestPathDijkstra(const vector<lng> &a) {
    assert(!a.empty());
    lng m = *std::min_element(a.begin(), a.end());
    assert(m >= 1 && lll(m) * *std::max_element(a.begin(), a.end()) <= std::numeric_limits<lng>::max());
    vector<lng> d(m, -1); d[0] = 0;
    vector<char> done(m);
    priority_queue<pair<lng, lng>, vector<pair<lng, lng>>, std::greater<>> pq; pq.push({0, 0});
    while (!pq.empty()) {
        auto [du, u] = pq.top(); pq.pop();
        if (std::exchange(done[u], 1)) { continue; }
        for (lng b : a) {
            lng v = (u + b) % m;
            if (d[v] < 0 || du + b < d[v]) { d[v] = du + b; pq.push({d[v], v}); }}}
    return d;}

// T: O(m), M: O(1); d from residueShortestPath; frobeniusNumber -1 when all are representable; it and countNonRepresentable INT64_MAX when gcd(a) > 1; count of t in [0, x].
inline lng frobeniusNumber(const vector<lng> &d) {
    lng mx = 0;
    for (lng v : d) {
        if (v < 0) { return std::numeric_limits<lng>::max(); }
        mx = max(mx, v);}
    return mx - lng(d.size());}
inline ulng countRepresentable(const vector<lng> &d, lng x) {
    ulng res = 0; lng m = lng(d.size());
    for (lng v : d) { if (v >= 0 && v <= x) { res += ulng((x - v) / m) + 1; } }
    return res;}
inline lng countNonRepresentable(const vector<lng> &d) {
    lng res = 0, m = lng(d.size());
    for (lng v : d) {
        if (v < 0) { return std::numeric_limits<lng>::max(); }
        res += v / m;}
    return res;}

// T: O(log(max(a, b))), M: O(1); two coins a, b >= 1; frobeniusNumberSylvester -1 when all are representable; both lll max when gcd(a, b) > 1.
inline lll frobeniusNumberSylvester(lng a, lng b) {
    assert(a >= 1 && b >= 1);
    return gcd(a, b) > 1 ? std::numeric_limits<lll>::max() : lll(a) * b - a - b;}
inline lll countNonRepresentableSylvester(lng a, lng b) {
    assert(a >= 1 && b >= 1);
    return gcd(a, b) > 1 ? std::numeric_limits<lll>::max() : lll(a - 1) * (b - 1) / 2;}
