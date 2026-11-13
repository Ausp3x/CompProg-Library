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

// T: O(1) (bisect O(itr)), M: O(1); validate asserts finite inputs and nonnegative limits, done is the converged predicate, bisect splits at mid, step is Brent's least step.
namespace search_detail {
    inline void validate([[maybe_unused]] double l, [[maybe_unused]] double r, [[maybe_unused]] int itr, [[maybe_unused]] double abs_tol, [[maybe_unused]] double rel_tol) {
        assert(std::isfinite(l) && std::isfinite(r) && itr >= 0);
        assert(std::isfinite(abs_tol) && abs_tol >= 0 && std::isfinite(rel_tol) && rel_tol >= 0);}
    inline bool done(double l, double r, double abs_tol, double rel_tol) {
        return static_cast<long double>(r) - l <= static_cast<long double>(abs_tol) + static_cast<long double>(rel_tol) * max(abs(l), abs(r))
            || std::nextafter(l, r) == r;}
    template<typename F, typename M>
    RealSearchResult bisect(double ok, double ng, F f, M mid, int itr, double abs_tol, double rel_tol) {
        validate(ok, ng, itr, abs_tol, rel_tol); int used = 0;
        while (used < itr && !done(min(ok, ng), max(ok, ng), abs_tol, rel_tol)) {
            double md = mid(ok, ng);
            if (f(md)) { ok = md; }
            else { ng = md; }
            ++used;}
        double l = min(ok, ng), r = max(ok, ng);
        return {l, r, ok, used, done(l, r, abs_tol, rel_tol)};}
    inline constexpr long double CGOLD = 0.38196601125010515180L;
    inline long double step(long double x, double abs_tol, double rel_tol) {
        return max(0.25L * abs_tol + 0.25L * rel_tol * abs(x), 2 * DBL_EPSILON * abs(x) + std::numeric_limits<double>::denorm_min());}
} // namespace search_detail

// T: O(itr), M: O(1); finite ok/ng in either order, never evaluated, x is the true endpoint; geometric: nonzero, one sign, splits at the geometric mean; bits: splits at the median double, at most 64 reductions.
template<typename F>
RealSearchResult binSearchRealBracket(double ok, double ng, F f, int itr = 100,
                                     double abs_tol = 0, double rel_tol = 0) {
    return search_detail::bisect(ok, ng, f, [](double a, double b) { return std::midpoint(a, b); }, itr, abs_tol, rel_tol);}
template<typename F>
RealSearchResult binSearchRealGeometric(double ok, double ng, F f, int itr = 100,
                                       double abs_tol = 0, double rel_tol = 0) {
    assert((ok > 0 && ng > 0) || (ok < 0 && ng < 0));
    auto mid = [](double a, double b) {
        double g = std::copysign(std::sqrt(abs(a)) * std::sqrt(abs(b)), a);
        return min(a, b) < g && g < max(a, b) ? g : std::midpoint(a, b);};
    return search_detail::bisect(ok, ng, f, mid, itr, abs_tol, rel_tol);}
template<typename F>
RealSearchResult binSearchRealBits(double ok, double ng, F f, int itr = 100,
                                  double abs_tol = 0, double rel_tol = 0) {
    // Order-preserving map from doubles to lng: negative patterns are reflected below zero, both zeros map to 0 and 0 maps back to +0.
    auto flip = [](lng i) { return i < 0 ? std::numeric_limits<lng>::min() - i : i; };
    auto mid = [&](double a, double b) {
        return std::bit_cast<double>(flip(std::midpoint(flip(std::bit_cast<lng>(a)), flip(std::bit_cast<lng>(b)))));};
    return search_detail::bisect(ok, ng, f, mid, itr, abs_tol, rel_tol);}

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
            if (right) { l = m1; m1 = m2; f1 = f2; m2 = std::lerp(m1, r, 1 - RATIO); }
            else { r = m2; m2 = m1; f2 = f1; m1 = std::lerp(m2, l, 1 - RATIO); }
            ++used;
            if (used == itr || search_detail::done(l, r, abs_tol, rel_tol)
                || !(l < m1 && m1 < m2 && m2 < r)) { break; }
            if (right) { f2 = f(m2); }
            else { f1 = f(m1); }}}
    return {l, r, std::midpoint(l, r), used, search_detail::done(l, r, abs_tol, rel_tol)};}

// T: O(itr), M: O(1); finite l <= r, ternSearch shape, f real-valued; parabolic steps with golden fallback (Brent), x the best probe, at most itr + 1 evaluations.
template<typename F>
RealSearchResult brentSearchRealBracket(double l, double r, F f, int itr = 100,
                                       double abs_tol = 0, double rel_tol = 0) {
    search_detail::validate(l, r, itr, abs_tol, rel_tol); assert(l <= r); int used = 0;
    double x = std::lerp(l, r, double(search_detail::CGOLD)), w = x, v = x;
    if (!itr || search_detail::done(l, r, abs_tol, rel_tol) || !(l < x && x < r)) {
        return {l, r, std::midpoint(l, r), 0, search_detail::done(l, r, abs_tol, rel_tol)};}
    auto fx = f(x), fw = fx, fv = fx;
    long double d = 0, e = 0;
    while (used < itr && !search_detail::done(l, r, abs_tol, rel_tol)) {
        long double xx = x, xm = 0.5L * l + 0.5L * r;
        long double tol1 = search_detail::step(xx, abs_tol, rel_tol);
        bool golden = true;
        if (abs(e) > tol1) {
            long double t = (xx - w) * (fx - fv), q = (xx - v) * (fx - fw), p = (xx - v) * q - (xx - w) * t;
            q = 2 * (q - t);
            if (q > 0) { p = -p; }
            else { q = -q; }
            long double last = std::exchange(e, d);
            if (abs(p) < abs(0.5L * q * last) && p > q * (l - xx) && p < q * (r - xx)) {
                d = p / q; golden = false;
                if (xx + d - l < 2 * tol1 || r - xx - d < 2 * tol1) { d = xm >= xx ? tol1 : -tol1; }}}
        if (golden) { e = xx >= xm ? l - xx : r - xx; d = search_detail::CGOLD * e; }
        double u = double(abs(d) >= tol1 ? xx + d : d >= 0 ? xx + tol1 : xx - tol1);
        if (!(l < u && u < r) || u == x) { break; }
        auto fu = f(u); ++used;
        if (fu <= fx) {
            if (u >= x) { l = x; }
            else { r = x; }
            v = w; fv = fw; w = x; fw = fx; x = u; fx = fu;}
        else {
            if (u < x) { l = u; }
            else { r = u; }
            if (fu <= fw || w == x) { v = w; fv = fw; w = u; fw = fu; }
            else if (fu <= fv || v == x || v == w) { v = u; fv = fu; }}}
    return {l, r, x, used, search_detail::done(l, r, abs_tol, rel_tol)};}
// T: O(itr), M: O(1); as brentSearchRealBracket, df the derivative of f; secant steps on df with bisection downhill (NR dbrent), at most itr + 1 calls of f and of df.
template<typename F, typename D>
RealSearchResult brentSearchRealBracketDerivative(double l, double r, F f, D df, int itr = 100,
                                                  double abs_tol = 0, double rel_tol = 0) {
    search_detail::validate(l, r, itr, abs_tol, rel_tol); assert(l <= r); int used = 0;
    double x = std::lerp(l, r, double(search_detail::CGOLD)), w = x, v = x;
    if (!itr || search_detail::done(l, r, abs_tol, rel_tol) || !(l < x && x < r)) {
        return {l, r, std::midpoint(l, r), 0, search_detail::done(l, r, abs_tol, rel_tol)};}
    auto fx = f(x), fw = fx, fv = fx;
    auto dx = df(x), dw = dx, dv = dx;
    long double d = 0, e = 0;
    while (used < itr && !search_detail::done(l, r, abs_tol, rel_tol)) {
        long double xx = x, xm = 0.5L * l + 0.5L * r;
        long double tol1 = search_detail::step(xx, abs_tol, rel_tol);
        bool halve = true;
        if (abs(e) > tol1) {
            // Secant steps toward df = 0 through w and v; out of the bracket when undefined.
            long double d1 = std::numeric_limits<long double>::infinity(), d2 = d1;
            if (dw != dx) { d1 = (w - xx) * dx / (dx - dw); }
            if (dv != dx) { d2 = (v - xx) * dx / (dx - dv); }
            bool ok1 = l < xx + d1 && xx + d1 < r && dx * d1 <= 0, ok2 = l < xx + d2 && xx + d2 < r && dx * d2 <= 0;
            long double last = std::exchange(e, d);
            if (ok1 || ok2) {
                long double s = ok1 && ok2 ? (abs(d1) < abs(d2) ? d1 : d2) : ok1 ? d1 : d2;
                if (abs(s) <= abs(0.5L * last)) {
                    d = s; halve = false;
                    if (xx + d - l < 2 * tol1 || r - xx - d < 2 * tol1) { d = xm >= xx ? tol1 : -tol1; }}}}
        if (halve) { e = dx >= 0 ? l - xx : r - xx; d = 0.5L * e; }
        double u = double(abs(d) >= tol1 ? xx + d : d >= 0 ? xx + tol1 : xx - tol1);
        // The downhill side is within tol1: probe the other side, which closes the bracket unless strictly lower.
        bool back = !(l < u && u < r);
        if (back) { u = double(d >= 0 ? xx - tol1 : xx + tol1); }
        if (!(l < u && u < r) || u == x) { break; }
        auto fu = f(u); auto du = df(u); ++used;
        if (back ? fu < fx : fu <= fx) {
            if (u >= x) { l = x; }
            else { r = x; }
            v = w; fv = fw; dv = dw; w = x; fw = fx; dw = dx; x = u; fx = fu; dx = du;}
        else {
            if (u < x) { l = u; }
            else { r = u; }
            if (fu <= fw || w == x) { v = w; fv = fw; dv = dw; w = u; fw = fu; dw = du; }
            else if (fu < fv || v == x || v == w) { v = u; fv = fu; dv = du; }}}
    return {l, r, x, used, search_detail::done(l, r, abs_tol, rel_tol)};}
