#pragma once
#include "../01-Core/01-template.hpp"

// T: O(log(n)), M: O(1); n = |ok - ng| + 1, full lng endpoints; known ok/ng are never queried.
template<typename F>
lng binSearch(lng ok, lng ng, F f) {
    for (;;) {
        lng md = std::midpoint(ok, ng);
        if (md == ok) { return ok; }
        if (f(md)) { ok = md; }
        else { ng = md; }}}

// T: O(log(n + 1)), M: O(1); n = r - l; firstTrue false-then-true, lastTrue true-then-false on [l, r); absent gives {false, r}.
template<typename F>
pair<bool, lng> firstTrue(lng l, lng r, F f) {
    assert(l <= r); lng end = r;
    while (l < r) {
        lng md = std::midpoint(l, r);
        if (f(md)) { r = md; }
        else { l = md + 1; }}
    return {l != end, l};}
template<typename F>
pair<bool, lng> lastTrue(lng l, lng r, F f) {
    lng p = firstTrue(l, r, [&](lng x) { return !f(x); }).second;
    return p == l ? pair<bool, lng>{false, r} : pair<bool, lng>{true, p - 1};}

// T: O(log(n)), M: O(1); n = r - l + 1, CLOSED [l, r], strict decrease, flat minimum, strict increase; leftmost minimum; fibSearch <= log_phi(n + 1) + 1 evaluations.
template<typename F>
lng ternSearch(lng l, lng r, F f) {
    assert(l <= r);
    while (l < r) {
        lng md = std::midpoint(l, r);
        if (f(md + 1) < f(md)) { l = md + 1; }
        else { r = md; }}
    return l;}
template<typename F>
lng fibSearch(lng l, lng r, F f) {
    assert(l <= r);
    if (l == r) { return l; }
    lll a = lll(l) - 1, x = 2, y = 3;
    while (y <= lll(r) - a) { y += x; x = y - x; }
    auto g = [&](lll p) { return f(lng(min(p, lll(r)))); };
    lll c = a + y - x, d = a + x;
    auto fc = g(c), fd = g(d);
    while (y > 3) {
        lll z = y - x; y = x; x = z;
        if (fd < fc) { a = c; c = d; fc = fd; d = a + x; fd = g(d); }
        else { d = c; fd = fc; c = a + y - x; fc = g(c); }}
    return lng(fd < fc ? d : c);}

// T: O(log(2 + x - ok)), M: O(1); f true then false on [ok, INT64_MAX], f(ok) known true; returns the last true x.
template<typename F>
lng expSearch(lng ok, F f) {
    for (ulng d = 1; ok < std::numeric_limits<lng>::max(); d *= 2) {
        lng ng = ulng(std::numeric_limits<lng>::max()) - ulng(ok) > d ? lng(ulng(ok) + d) : std::numeric_limits<lng>::max();
        if (!f(ng)) { return binSearch(ok, ng, f); }
        ok = ng;}
    return ok;}

// T: O(1), M: O(1); sorted bracket [l, r], point x, reductions done; converged = tolerance met or adjacent doubles.
struct RealSearchResult {
    double l, r, x;
    int iterations;
    bool converged;
};

// T: O(1), M: O(1); validate asserts finite inputs and nonnegative limits, done is the converged predicate.
namespace search_detail {
    inline void validate(double l, double r, int itr, double abs_tol, double rel_tol) {
        assert(std::isfinite(l) && std::isfinite(r) && itr >= 0);
        assert(std::isfinite(abs_tol) && abs_tol >= 0 && std::isfinite(rel_tol) && rel_tol >= 0);}
    inline bool done(double l, double r, double abs_tol, double rel_tol) {
        return static_cast<long double>(r) - l <= static_cast<long double>(abs_tol) + static_cast<long double>(rel_tol) * max(abs(l), abs(r))
            || std::nextafter(l, r) == r;}
} // namespace search_detail

// T: O(itr), M: O(1); finite ok/ng in either order, never evaluated; x is the true endpoint.
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

// T: O(itr), M: O(1); finite l <= r, ternSearch shape, x is the bracket midpoint; at most 2 * itr (ternary) or itr + 1 (golden) evaluations.
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

// T: O(itr), M: O(1); legacy point interfaces with the domains of the bracket functions.
template<typename F>
double binSearchReal(double ok, double ng, F f, int itr = 100) {
    return binSearchRealBracket(ok, ng, std::move(f), itr).x;}
template<typename F>
double ternSearchReal(double l, double r, F f, int itr = 200) {
    return ternSearchRealBracket(l, r, std::move(f), itr).x;}
template<typename F>
double goldenSearchReal(double l, double r, F f, int itr = 100) {
    return goldenSearchRealBracket(l, r, std::move(f), itr).x;}
