#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); value, converged, iterations (evaluations after the start), residual |f(value)| (fixedPointIterate: last step).
struct RootStatus {
    double value;
    bool converged;
    int iterations;
    double residual;
};

namespace numerical_detail {
    // T: O(1) (pairwise: O(n), O(log(n)) recursion), M: O(1); tol floors abs_tol + rel_tol |x| at 4 ulp of x.
    inline void validate(int itr, double abs_tol, double rel_tol) {
        assert(itr >= 0 && std::isfinite(abs_tol) && abs_tol >= 0 && std::isfinite(rel_tol) && rel_tol >= 0);}
    inline double tol(double x, double abs_tol, double rel_tol) {
        return max(abs_tol + rel_tol * abs(x), 4 * DBL_EPSILON * abs(x));}
    inline bool opposite(double a, double b) { return a == 0 || b == 0 || std::signbit(a) != std::signbit(b); }
    inline double pairwise(const double *p, int n) {
        if (n > 128) { return pairwise(p, n / 2) + pairwise(p + n / 2, n - n / 2); }
        double res = 0;
        for (int i = 0; i < n; ++i) { res += p[i]; }
        return res;}
} // namespace numerical_detail

// T: O(1), M: O(1); real roots of a x^2 + b x + c ascending with multiplicity, a = 0 linear; finite, not all zero; scaled by a power of 2 first.
inline vector<double> solveQuadratic(double a, double b, double c) {
    assert(std::isfinite(a) && std::isfinite(b) && std::isfinite(c) && (a != 0 || b != 0 || c != 0));
    if (a == 0) { return b == 0 ? vector<double>{} : vector<double>{-c / b}; }
    int e = std::ilogb(max({abs(a), abs(b), abs(c)}));
    a = std::ldexp(a, -e); b = std::ldexp(b, -e); c = std::ldexp(c, -e);
    double w = 4 * a * c, d = std::fma(b, b, -w) + std::fma(-4 * a, c, w);
    if (d < 0) { return {}; }
    double q = -(b + std::copysign(std::sqrt(d), b)) / 2;
    if (q == 0) { return {0.0, 0.0}; }
    double x = q / a, y = c / q;
    return x < y ? vector<double>{x, y} : vector<double>{y, x};}

// T: O(1), M: O(1); real roots of a x^3 + b x^2 + c x + d ascending: three with multiplicity when R^2 <= Q^3 up to rounding, else one; polished.
inline vector<double> solveCubic(double a, double b, double c, double d) {
    if (a == 0) { return solveQuadratic(b, c, d); }
    assert(std::isfinite(a) && std::isfinite(b) && std::isfinite(c) && std::isfinite(d));
    // x = 2^e y with monic y coefficients below 2, 4, 8, from exponents alone so that no division or scaling over- or underflows.
    int ea = std::ilogb(a), e = 0;
    double am = std::ldexp(a, -ea), co[3] = {b, c, d};
    bool any = false;
    for (int k = 1; k <= 3; ++k) {
        if (co[k - 1] == 0) { continue; }
        int v = std::ilogb(co[k - 1]) - ea, f = v >= 0 ? v / k : -((k - 1 - v) / k);
        e = any ? max(e, f) : f; any = true;}
    if (!any) { return {0.0, 0.0, 0.0}; }
    for (int k = 1; k <= 3; ++k) {
        if (co[k - 1] != 0) {
            int ex = std::ilogb(co[k - 1]);
            co[k - 1] = std::ldexp(std::ldexp(co[k - 1], -ex) / am, ex - ea - k * e);}}
    b = co[0]; c = co[1]; d = co[2];
    double q = (b * b - 3 * c) / 9, r = (2 * b * b * b - 9 * b * c + 27 * d) / 54, q3 = q * q * q;
    double eq = 4 * DBL_EPSILON * (b * b + 3 * abs(c)) / 9, er = 4 * DBL_EPSILON * (2 * abs(b * b * b) + 9 * abs(b * c) + 27 * abs(d)) / 54;
    vector<double> res;
    if (r * r - q3 <= 2 * abs(r) * er + 3 * q * q * eq + 4 * DBL_EPSILON * max(r * r, abs(q3))) {
        double t = q3 > 0 ? std::acos(std::clamp(r / std::sqrt(q3), -1.0, 1.0)) : 0, s = -2 * std::sqrt(max(q, 0.0));
        for (int k = 0; k < 3; ++k) { res.push_back(s * std::cos((t + 2 * k * std::numbers::pi) / 3) - b / 3); }
        double big = *std::max_element(res.begin(), res.end(), [](double x, double y) { return abs(x) < abs(y); });
        if (big != 0) {
            double h = (c + d / big) / big / 2;
            auto rest = solveQuadratic(1, -2 * h, -d / big);
            res = rest.size() == 2 ? vector<double>{rest[0], rest[1], big} : vector<double>{h, h, big};}}
    else {
        double u = -std::copysign(std::cbrt(abs(r) + std::sqrt(r * r - q3)), r);
        res.push_back(u + (u == 0 ? 0 : q / u) - b / 3);}
    auto p = [&](double x) { return ((x + b) * x + c) * x + d; };
    sort(res.begin(), res.end());
    double big = max(abs(res[0]), abs(res.back()));
    vector<double> cap(res.size(), std::numeric_limits<double>::infinity());
    for (int i = 0; i + 1 < int(res.size()); ++i) { cap[i] = min(cap[i], res[i + 1] - res[i]); cap[i + 1] = res[i + 1] - res[i]; }
    for (int i = 0; i < int(res.size()); ++i) {
        for (int k = 0; k < 2; ++k) {
            double x = res[i], dp = (3 * x + 2 * b) * x + c, y = dp == 0 ? x : x - p(x) / dp;
            if (!(abs(p(y)) < abs(p(x)) && abs(y - x) < max(cap[i] / 2, 1e-6 * big))) { break; }
            res[i] = y;}}
    for (double &x : res) { x = std::ldexp(x, e); }
    sort(res.begin(), res.end());
    return res;}

// T: O(n), M: O(1); {p(x), p'(x)} for p = a[0] + a[1] x + ..., any ring T; empty a gives {0, 0}.
template<typename T>
pair<T, T> hornerDerivative(const vector<T> &a, T x) {
    T v{}, d{};
    for (int i = int(a.size()) - 1; i >= 0; --i) { d = d * x + v; v = v * x + a[i]; }
    return {v, d};}

// T: O(n), M: O(1); compensated sums: Kahan error <= 2 eps sum |a_i|, Neumaier also when a term exceeds the running sum.
inline double kahanSum(const vector<double> &a) {
    double s = 0, c = 0;
    for (double x : a) {
        double y = x - c, t = s + y;
        c = (t - s) - y; s = t;}
    return s;}
inline double neumaierSum(const vector<double> &a) {
    double s = 0, c = 0;
    for (double x : a) {
        double t = s + x;
        c += abs(s) >= abs(x) ? (s - t) + x : (x - t) + s;
        s = t;}
    return s + c;}

// T: O(n), M: O(log(n)); halves down to blocks of 128, error <= (128 + log2(n)) eps sum |a_i|.
inline double pairwiseSum(const vector<double> &a) { return numerical_detail::pairwise(a.data(), int(a.size())); }

// T: O(itr) evaluations, M: O(1); finite l, r in either order, f(l) and f(r) of opposite sign or zero; value is the endpoint of smaller |f|.
template<typename F>
RootStatus bisectionRoot(double l, double r, F f, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(l) && std::isfinite(r));
    double fl = f(l), fr = f(r);
    assert(numerical_detail::opposite(fl, fr));
    auto done = [&] { return abs(r - l) <= abs_tol + rel_tol * max(abs(l), abs(r)) || std::nextafter(l, r) == r; };
    int i = 0;
    for (; i < itr && fl != 0 && fr != 0 && !done(); ++i) {
        double m = std::midpoint(l, r), fm = f(m);
        if (std::signbit(fm) == std::signbit(fl)) { l = m; fl = fm; }
        else { r = m; fr = fm; }}
    bool ok = fl == 0 || fr == 0 || done();
    return abs(fl) <= abs(fr) ? RootStatus{l, ok, i, abs(fl)} : RootStatus{r, ok, i, abs(fr)};}

// T: O(itr) evaluations of f and df, M: O(1); finite x; stops at f = 0, a step within tolerance, or a zero or non-finite derivative (not converged).
template<typename F, typename D>
RootStatus newtonRoot(double x, F f, D df, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(x));
    double fx = f(x);
    int i = 0;
    for (; i < itr && fx != 0; ++i) {
        double y = x - fx / df(x);
        if (!std::isfinite(y)) { return {x, false, i, abs(fx)}; }
        bool small = abs(y - x) <= numerical_detail::tol(y, abs_tol, rel_tol);
        x = y; fx = f(x);
        if (small) { return {x, true, i + 1, abs(fx)}; }}
    return {x, fx == 0, i, abs(fx)};}

// T: O(itr) evaluations of f and df, M: O(1); bracket as bisectionRoot; Newton steps leaving the bracket or shrinking too slowly bisect.
template<typename F, typename D>
RootStatus newtonSafeRoot(double l, double r, F f, D df, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(l) && std::isfinite(r));
    double fl = f(l), fr = f(r);
    assert(numerical_detail::opposite(fl, fr));
    if (fl == 0 || fr == 0) { return fl == 0 ? RootStatus{l, true, 0, 0} : RootStatus{r, true, 0, 0}; }
    if (fl > 0) { swap(l, r); }
    double x = std::midpoint(l, r), fx = f(x), step = abs(r - l), prev = step;
    int i = 1;
    for (; i <= itr && fx != 0; ++i) {
        (fx < 0 ? l : r) = x;
        double d = df(x), y = x - fx / d;
        if (!(min(l, r) < y && y < max(l, r)) || abs(2 * fx) > abs(prev * d)) { y = std::midpoint(l, r); }
        prev = step; step = abs(y - x);
        if (y == x) { return {x, true, i - 1, abs(fx)}; }
        x = y; fx = f(x);
        if (step <= numerical_detail::tol(x, abs_tol, rel_tol)) { return {x, true, i, abs(fx)}; }}
    return {x, fx == 0, i - 1, abs(fx)};}

// T: O(itr) evaluations, M: O(1); finite x0 != x1; stops at f = 0, a step within tolerance, or equal values f(x0) = f(x1) (not converged).
template<typename F>
RootStatus secantRoot(double x0, double x1, F f, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(x0) && std::isfinite(x1) && x0 != x1);
    double f0 = f(x0), f1 = f(x1);
    int i = 0;
    for (; i < itr && f1 != 0; ++i) {
        double y = x1 - f1 * (x1 - x0) / (f1 - f0);
        if (!std::isfinite(y)) { return {x1, false, i, abs(f1)}; }
        bool small = abs(y - x1) <= numerical_detail::tol(y, abs_tol, rel_tol);
        x0 = x1; f0 = f1; x1 = y; f1 = f(x1);
        if (small) { return {x1, true, i + 1, abs(f1)}; }}
    return {x1, f1 == 0, i, abs(f1)};}

// T: O(itr) evaluations, M: O(1); bracket as bisectionRoot; Brent's inverse quadratic interpolation, secant and bisection (zbrent).
template<typename F>
RootStatus brentRoot(double l, double r, F f, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(l) && std::isfinite(r));
    double a = l, b = r, fa = f(a), fb = f(b);
    assert(numerical_detail::opposite(fa, fb));
    if (fa == 0) { return {a, true, 0, 0}; }
    double c = b, fc = fb, d = b - a, e = d;
    for (int i = 0; i < itr; ++i) {
        if (fb == 0) { return {b, true, i, 0}; }
        if (std::signbit(fb) == std::signbit(fc)) { c = a; fc = fa; e = d = b - a; }
        if (abs(fc) < abs(fb)) { a = b; b = c; c = a; fa = fb; fb = fc; fc = fa; }
        double t = 2 * DBL_EPSILON * abs(b) + (abs_tol + rel_tol * abs(b)) / 2 + DBL_MIN, m = c / 2 - b / 2;
        if (abs(m) <= t) { return {b, true, i, abs(fb)}; }
        if (abs(e) >= t && abs(fa) > abs(fb)) {
            double s = fb / fa, p, q;
            if (a == c) { p = 2 * m * s; q = 1 - s; }
            else {
                double u = fa / fc, v = fb / fc;
                p = s * (2 * m * u * (u - v) - (b - a) * (v - 1)); q = (u - 1) * (v - 1) * (s - 1);}
            if (p > 0) { q = -q; }
            p = abs(p);
            if (2 * p < min(3 * m * q - abs(t * q), abs(e * q))) { e = d; d = p / q; }
            else { d = e = m; }}
        else { d = e = m; }
        a = b; fa = fb;
        b += abs(d) > t ? d : std::copysign(t, m);
        fb = f(b);}
    return {b, fb == 0, itr, abs(fb)};}

// T: O(itr) evaluations, M: O(1); x_{k+1} = g(x_k) from finite x; stops at a step within tolerance or a non-finite iterate (not converged).
template<typename G>
RootStatus fixedPointIterate(double x, G g, int itr = 100, double abs_tol = 0, double rel_tol = 0) {
    numerical_detail::validate(itr, abs_tol, rel_tol); assert(std::isfinite(x));
    double step = std::numeric_limits<double>::infinity();
    for (int i = 0; i < itr; ++i) {
        double y = g(x);
        if (!std::isfinite(y)) { return {x, false, i + 1, step}; }
        step = abs(y - x); x = y;
        if (step <= numerical_detail::tol(x, abs_tol, rel_tol)) { return {x, true, i + 1, step}; }}
    return {x, false, itr, step};}

// T: O(n) evaluations, M: O(1); composite Simpson on n intervals, odd n rounded up; error O((r - l) h^4 max |f''''|), h = (r - l) / n.
template<typename F>
double simpsonFixed(double l, double r, F f, int n = 1000) {
    assert(n >= 1);
    lng m = lng(n) + (n & 1);
    double h = (r - l) / double(m), res = f(l) + f(r);
    for (lng i = 1; i < m; ++i) { res += (i & 1 ? 4 : 2) * f(l + double(i) * h); }
    return res * h / 3;}

// T: O(2^depth) evaluations worst case, M: O(depth); adaptive Simpson with Richardson correction, eps halved per split; NaN or depth 0 stops a branch.
template<typename F>
double simpsonAdaptive(double l, double r, F f, double eps = 1e-9, int depth = 50) {
    assert(eps > 0 && depth >= 0);
    auto rec = [&](auto &&rec, double l, double r, double fl, double fm, double fr, double whole, double eps, int depth) -> double {
        double m = std::midpoint(l, r), lm = std::midpoint(l, m), rm = std::midpoint(m, r), flm = f(lm), frm = f(rm);
        double left = (fl + 4 * flm + fm) * (m - l) / 6, right = (fm + 4 * frm + fr) * (r - m) / 6, delta = left + right - whole;
        double mag = (abs(fl) + 4 * abs(flm) + 2 * abs(fm) + 4 * abs(frm) + abs(fr)) * abs(r - l) / 12;
        if (depth <= 0 || !(abs(delta) > 15 * max(eps, 16 * DBL_EPSILON * mag))) { return left + right + delta / 15; }
        return rec(rec, l, m, fl, flm, fm, left, eps / 2, depth - 1) + rec(rec, m, r, fm, frm, fr, right, eps / 2, depth - 1);};
    double m = std::midpoint(l, r), fl = f(l), fm = f(m), fr = f(r);
    return rec(rec, l, r, fl, fm, fr, (fl + 4 * fm + fr) * (r - l) / 6, eps, depth);}

// T: O(n^2), M: O(n); {nodes ascending, weights} on [-1, 1] for n >= 1, exact for degree 2 n - 1; Newton on P_n from cos(pi (i + 3/4) / (n + 1/2)).
inline pair<vector<double>, vector<double>> gaussLegendreNodes(int n) {
    assert(n >= 1);
    n = max(n, 1);
    vector<double> x(n), w(n);
    auto legendre = [&](double z) {
        double p = 1, q = 0;
        for (int j = 1; j <= n; ++j) {
            double s = q; q = p;
            p = ((2 * j - 1) * z * q - (j - 1) * s) / j;}
        return pair<double, double>{p, n * (z * p - q) / (z * z - 1)};};
    for (int i = 0; i < (n + 1) / 2; ++i) {
        double z = std::cos(std::numbers::pi * (i + 0.75) / (n + 0.5));
        for (int k = 0; k < 100; ++k) {
            auto [p, dp] = legendre(z);
            double dz = p / dp; z -= dz;
            if (abs(dz) <= 4 * DBL_EPSILON) { break; }}
        double dp = legendre(z).second;
        x[i] = -z; x[n - 1 - i] = z; w[i] = w[n - 1 - i] = 2 / ((1 - z * z) * dp * dp);}
    return {x, w};}

// T: O(panels * n) evaluations, M: O(1); n-point rule nw from gaussLegendreNodes on each of panels equal parts of [l, r].
template<typename F>
double gaussLegendreIntegrate(double l, double r, F f, const pair<vector<double>, vector<double>> &nw, int panels = 1) {
    auto &[x, w] = nw;
    assert(panels >= 1 && !x.empty() && x.size() == w.size());
    int n = int(min(x.size(), w.size()));
    double h = (r - l) / panels, res = 0;
    for (int k = 0; k < panels; ++k) {
        double c = l + (k + 0.5) * h;
        for (int i = 0; i < n; ++i) { res += w[i] * f(c + h / 2 * x[i]); }}
    return res * h / 2;}

// T: O(2^k) evaluations, k <= min(levels, 30) the levels used, M: O(k); Romberg on halved trapezoids; stops after 5 levels once diagonals differ by <= max(eps, rounding).
template<typename F>
double rombergIntegrate(double l, double r, F f, double eps = 1e-10, int levels = 20) {
    assert(eps >= 0 && levels >= 1 && levels <= 30);
    levels = min(levels, 30);
    double h = r - l, fl = f(l), fr = f(r), mag = abs(h) * (abs(fl) + abs(fr)) / 2;
    vector<double> row{h * (fl + fr) / 2}, cur;
    for (int k = 1; k < levels; ++k) {
        h /= 2;
        double s = 0, sa = 0, q = 1;
        for (lng i = 1; i < lng(1) << k; i += 2) {
            double y = f(l + double(i) * h);
            s += y; sa += abs(y);}
        cur.assign(1, row[0] / 2 + h * s);
        mag = mag / 2 + abs(h) * sa;
        for (int j = 1; j <= k; ++j) {
            q *= 4;
            cur.push_back(cur[j - 1] + (cur[j - 1] - row[j - 1]) / (q - 1));}
        bool done = k >= 4 && abs(cur[k] - row[k - 1]) <= max(eps, 16 * DBL_EPSILON * mag);
        swap(row, cur);
        if (done) { break; }}
    return row.back();}

// T: O(levels) evaluations of f plus O(levels^2), M: O(levels); f(0) from f(h), f(h / t), ... when f(h) - f(0) = c1 h^p + c2 h^(2 p) + ...
template<typename F>
double richardsonExtrapolate(double h, F f, int levels = 4, double t = 2, int p = 2) {
    assert(levels >= 1 && t > 1 && p >= 1);
    levels = max(levels, 1);
    vector<double> row, cur;
    double tp = std::pow(t, p);
    for (int i = 0; i < levels; ++i, h /= t) {
        cur.assign(1, f(h));
        double q = 1;
        for (int j = 1; j <= i; ++j) {
            q *= tp;
            cur.push_back(cur[j - 1] + (cur[j - 1] - row[j - 1]) / (q - 1));}
        swap(row, cur);}
    return row.back();}

// T: O(d) evaluations, M: O(1); d-th derivative by forward (error O(h)) or central (error O(h^2)) binomial differences; h = 0 picks eps^(1 / (d + 1 + central)) max(1, |x|); h must survive x + h - x.
template<typename F>
double finiteDifference(double x, F f, int d = 1, double h = 0, bool central = true) {
    assert(std::isfinite(x) && d >= 0 && d <= 60 && std::isfinite(h) && h >= 0);
    d = min(max(d, 0), 60);
    if (h == 0) { h = std::pow(DBL_EPSILON, 1.0 / (d + 1 + central)) * max(1.0, abs(x)); }
    double y = x + h; h = y - x;
    assert(h > 0 && std::isfinite(h));
    double res = 0, c = 1;
    for (int i = 0; i <= d; ++i) {
        res += ((d - i) & 1 ? -c : c) * f(x + (central ? i - d / 2.0 : i) * h);
        c = c * (d - i) / (i + 1);}
    return res / std::pow(h, d);}

// T: O(2^k) evaluations, k <= min(levels, 20) the levels used, M: O(1); tanh-sinh on finite [l, r], endpoint singularities allowed; stops once two levels differ by <= eps.
template<typename F>
double doubleExponentialIntegrate(double l, double r, F f, double eps = 1e-12, int levels = 12) {
    assert(std::isfinite(l) && std::isfinite(r) && eps >= 0 && levels >= 1 && levels <= 20);
    levels = min(levels, 20);
    if (l == r) { return 0; }
    constexpr double HP = std::numbers::pi / 2;
    double half = (r - l) / 2;
    auto sum = [&](double t0, double step) {
        double res = 0;
        for (double t = t0; t < 7; t += step) {
            double e = std::exp(-2 * HP * std::sinh(t)), g = 1 / (1 + e), dist = (r - l) * e * g, xl = l + dist, xr = r - dist;
            if (e == 0 || (xl == l && xr == r)) { break; }
            res += 4 * half * HP * std::cosh(t) * e * g * g * ((xl != l ? f(xl) : 0) + (xr != r ? f(xr) : 0));}
        return res;};
    double h = 1, res = h * (half * HP * f(std::midpoint(l, r)) + sum(h, h));
    for (int k = 1; k < levels; ++k) {
        double next = res / 2 + h / 2 * sum(h / 2, h);
        h /= 2;
        bool done = k >= 2 && abs(next - res) <= eps;
        res = next;
        if (done) { break; }}
    return res;}

// T: O(1) per operation, M: O(1); forward-mode dual numbers x + dx e with e^2 = 0 over a field T; division follows T (IEEE for double).
template<typename T = double>
struct DualNumber {
    T x, dx;

    DualNumber(T x = 0, T dx = 0) : x(x), dx(dx) {}
    // T: O(cost of f), M: O(1); f'(x) for f built from the operators below.
    template<typename F> static T derivative(F f, T x) { return f(DualNumber(x, 1)).dx; }

    DualNumber &operator+=(const DualNumber &o) { x += o.x; dx += o.dx; return *this; }
    DualNumber &operator-=(const DualNumber &o) { x -= o.x; dx -= o.dx; return *this; }
    DualNumber &operator*=(const DualNumber &o) { dx = dx * o.x + x * o.dx; x *= o.x; return *this; }
    DualNumber &operator/=(const DualNumber &o) { x /= o.x; dx = (dx - x * o.dx) / o.x; return *this; }

    friend DualNumber operator+(DualNumber a, const DualNumber &b) { return a += b; }
    friend DualNumber operator-(DualNumber a, const DualNumber &b) { return a -= b; }
    friend DualNumber operator-(const DualNumber &a) { return {-a.x, -a.dx}; }
    friend DualNumber operator*(DualNumber a, const DualNumber &b) { return a *= b; }
    friend DualNumber operator/(DualNumber a, const DualNumber &b) { return a /= b; }
};
