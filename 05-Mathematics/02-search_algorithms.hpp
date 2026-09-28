#pragma once
#include "../01-Core/01-template.hpp"

// All integer endpoints support the full lng range. Predicates are deterministic.
// Known true ok / known false ng bracket one transition; endpoints are not queried.
// Equal endpoints represent an already converged answer. At most 64 evaluations.
// T: O(log(n)), M: O(1); n = mathematical |ok - ng| + 1.
template<typename F>
lng binSearch(lng ok, lng ng, F f) {
    for (;;) {
        lng md = std::midpoint(ok, ng);
        if (md == ok) { return ok; }
        if (f(md)) { ok = md; }
        else { ng = md; }}}

// First true in half-open [l, r), false then true. Returns {found, position};
// position is r when absent. Empty ranges are valid; neither endpoint is a sentinel.
// T: O(log(n + 1)), M: O(1); n = mathematical r - l.
template<typename F>
pair<bool, lng> firstTrue(lng l, lng r, F f) {
    assert(l <= r); lng end = r;
    while (l < r) {
        lng md = std::midpoint(l, r);
        if (f(md)) { r = md; }
        else { l = md + 1; }}
    return {l != end, l};}

// Last true in [l, r), true then false. Returns {false, r} when absent.
// T: O(log(n + 1)), M: O(1); n = mathematical r - l.
template<typename F>
pair<bool, lng> lastTrue(lng l, lng r, F f) {
    lng p = firstTrue(l, r, [&](lng x) { return !f(x); }).second;
    return p == l ? pair<bool, lng>{false, r} : pair<bool, lng>{true, p - 1};}

// Leftmost minimum in CLOSED [l, r], preserving the legacy interface. The function
// strictly decreases, is optionally flat at its minimum, then strictly increases.
// Adjacent-value bisection improves the legacy ternary reduction; values need only <.
// T: O(log(n)), M: O(1); n = mathematical r - l + 1; at most 128 evaluations.
template<typename F>
lng ternSearch(lng l, lng r, F f) {
    assert(l <= r);
    while (l < r) {
        lng md = std::midpoint(l, r);
        if (f(md + 1) < f(md)) { l = md + 1; }
        else { r = md; }}
    return l;}

// Closed sorted bracket [l, r], recommended point x, completed reductions.
// converged means width <= abs_tol + rel_tol * max(|l|, |r|), or no representable
// double lies strictly inside. False means iteration cap or interpolation stagnation.
// Width/point guarantees assume reliable comparisons of f; this is not interval math.
struct RealSearchResult {
    double l, r, x;
    int iterations;
    bool converged;
};

namespace search_detail {
    inline void validate(double l, double r, int itr, double abs_tol, double rel_tol) {
        assert(std::isfinite(l) && std::isfinite(r) && itr >= 0);
        assert(std::isfinite(abs_tol) && abs_tol >= 0 && std::isfinite(rel_tol) && rel_tol >= 0);}
    inline bool done(double l, double r, double abs_tol, double rel_tol) {
        return static_cast<long double>(r) - l <= static_cast<long double>(abs_tol) + static_cast<long double>(rel_tol) * max(abs(l), abs(r))
            || std::nextafter(l, r) == r;}
} // namespace search_detail

// Finite true/false endpoints in either order, never evaluated; equal is converged.
// x is the true endpoint. In exact arithmetic width is initial_width / 2^iterations.
// T: O(itr), M: O(1); at most itr predicate evaluations.
template<typename F>
RealSearchResult binSearchRealBracket(double ok, double ng, F f, int itr = 100,
                                     double abs_tol = 0, double rel_tol = 0) {
    search_detail::validate(ok, ng, itr, abs_tol, rel_tol); int used = 0;
    while (used < itr && !search_detail::done(min(ok, ng), max(ok, ng), abs_tol, rel_tol)) {
        double md = std::midpoint(ok, ng);
        if (md == ok || md == ng) { break; }
        if (f(md)) { ok = md; }
        else { ng = md; }
        ++used;}
    double l = min(ok, ng), r = max(ok, ng);
    return {l, r, ok, used, search_detail::done(l, r, abs_tol, rel_tol)};}

// Minimum in CLOSED [l, r], finite l <= r; strictly decreasing then optional flat
// minimum then strictly increasing. Comparisons must preserve this shape (no NaN).
// In exact arithmetic width shrinks by 2/3 per reduction; x is the final midpoint.
// T: O(itr), M: O(1); at most 2 * itr evaluations.
template<typename F>
RealSearchResult ternSearchRealBracket(double l, double r, F f, int itr = 200,
                                      double abs_tol = 0, double rel_tol = 0) {
    search_detail::validate(l, r, itr, abs_tol, rel_tol); assert(l <= r); int used = 0;
    while (used < itr && !search_detail::done(l, r, abs_tol, rel_tol)) {
        double m1 = std::lerp(l, r, 1.0 / 3), m2 = std::lerp(l, r, 2.0 / 3);
        if (!(l < m1 && m1 < m2 && m2 < r)) { break; }
        if (f(m2) < f(m1)) { l = m1; }
        else { r = m2; }
        ++used;}
    return {l, r, std::midpoint(l, r), used, search_detail::done(l, r, abs_tol, rel_tol)};}

// Same contract as ternSearchRealBracket; cached values suit expensive functions.
// In exact arithmetic width shrinks by (sqrt(5) - 1) / 2 per reduction.
// T: O(itr), M: O(1); zero evaluations if done/itr=0, otherwise <= itr + 1.
template<typename F>
RealSearchResult goldenSearchRealBracket(double l, double r, F f, int itr = 100,
                                        double abs_tol = 0, double rel_tol = 0) {
    search_detail::validate(l, r, itr, abs_tol, rel_tol); assert(l <= r); int used = 0;
    constexpr double RATIO = 0.6180339887498948482;
    double m1 = std::lerp(l, r, 1 - RATIO), m2 = std::lerp(l, r, RATIO);
    if (itr && !search_detail::done(l, r, abs_tol, rel_tol) && l < m1 && m1 < m2 && m2 < r) {
        auto f1 = f(m1), f2 = f(m2);
        while (used < itr) {
            bool right = f2 < f1;
            if (right) { l = m1; m1 = m2; f1 = f2; m2 = std::lerp(l, r, RATIO); }
            else { r = m2; m2 = m1; f2 = f1; m1 = std::lerp(l, r, 1 - RATIO); }
            ++used;
            if (used == itr || search_detail::done(l, r, abs_tol, rel_tol)
                || !(l < m1 && m1 < m2 && m2 < r)) { break; }
            if (right) { f2 = f(m2); }
            else { f1 = f(m1); }}}
    return {l, r, std::midpoint(l, r), used, search_detail::done(l, r, abs_tol, rel_tol)};}

// Legacy point interfaces: same domains as the corresponding bracket API.
// T: O(itr), M: O(1).
template<typename F>
double binSearchReal(double ok, double ng, F f, int itr = 100) {
    return binSearchRealBracket(ok, ng, std::move(f), itr).x;}
// T: O(itr), M: O(1).
template<typename F>
double ternSearchReal(double l, double r, F f, int itr = 200) {
    return ternSearchRealBracket(l, r, std::move(f), itr).x;}
// T: O(itr), M: O(1).
template<typename F>
double goldenSearchReal(double l, double r, F f, int itr = 100) {
    return goldenSearchRealBracket(l, r, std::move(f), itr).x;}
