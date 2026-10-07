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
    x *= c / g; y *= c / g;
    if (b != 0) {
        lll m = b / g; if (m < 0) { m = -m; }
        x %= m; if (x < 0) { x += m; }
        y = (c - lll(a) * x) / b;}
    return {1, x, y, b / g, -lll(a) / g, g};}
inline bool solveDioEq(lng a, lng b, lng c, lng &x, lng &y, lng &g) {
    assert(&x != &y && &x != &g && &y != &g);
    auto s = solveDioEq(a, b, c);
    if (s.dimension < 0) { return false; }
    constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
    assert(LO <= s.x && s.x <= HI && LO <= s.y && s.y <= HI && s.g <= HI);
    x = lng(s.x); y = lng(s.y); g = lng(s.g); return true;}

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
