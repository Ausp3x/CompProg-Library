#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "04-sieve_algorithms.hpp"
#include "18-floorsum.hpp"

namespace continued_fraction_detail {
    // T: O(1) for isqrt (x < 2^126), O(n^(2/3)) setup and O(1) access for Mertens, O(sqrt(n) * log(y)) for countBelow, M: O(n^(2/3));
    // Mertens: M(v) at every v = floor(n / k); countBelow: #{reduced r/s : 1 <= s <= n, 0 <= r/s < x/y, or <= when closed}.
    inline ulll isqrt(ulll x) {
        ulll r = ulll(std::sqrt(static_cast<long double>(x)));
        while (r * r > x) { --r; }
        while ((r + 1) * (r + 1) <= x) { ++r; }
        return r;}
    struct Mertens {
        lng n, lim;
        vector<int> small;
        vector<lng> large;
        explicit Mertens(lng n) : n(n), lim(min({n, lng(std::pow(double(n), 2.0 / 3)) + 1, lng(INT_MAX) - 1})) {
            LinearSieve sieve{int(lim)}; sieve.getMu();
            small.assign(lim + 1, 0);
            for (int i = 1; i <= lim; ++i) { small[i] = small[i - 1] + sieve.mu[i]; }
            lng top = n / (lim + 1);
            large.assign(top + 1, 0);
            for (lng k = top; k >= 1; --k) {
                lng v = n / k, s = 1;
                forEachQuotientBlock(v, [&](lng w, lng l, lng r) { if (l >= 2) { s -= (r - l) * (*this)(w); }});
                large[k] = s;}}
        lng operator()(lng v) const { return v <= lim ? small[v] : large[n / v]; }
    };
    inline lll countBelow(const Mertens &mu, lng x, lng y, bool closed) {
        ulll res = 0;
        forEachQuotientBlock(mu.n, [&](lng v, lng l, lng r) {
            res += ulll(mu(r - 1) - mu(l - 1)) * ulll(floorSum(v, y, x, lll(x) + y - !closed));});
        return lll(res);}
} // namespace continued_fraction_detail

// T: O(log(q)) (convergents: O(k) for k terms; semiconvergents: O(out)), M: O(out); continuedFraction: q >= 1, [a0; a1, ..., ak] of p / q, ak >= 2 when k >= 1; a_i >= 1 for i >= 1 and results fitting lng;
// convergents: p_i / q_i of [a0; ..., ai]; semiconvergents: p_0 / q_0, then (t p_{i-1} + p_{i-2}) / (t q_{i-1} + q_{i-2}) for i >= 1 and t = 1..a_i, by denominator.
inline vector<lng> continuedFraction(lng p, lng q) {
    assert(q >= 1);
    vector<lng> res{lng(floorDiv(p, q))};
    for (lng u = q, v = modNorm(p, q); v; u = std::exchange(v, u % v)) { res.pb(u / v); }
    return res;}
inline vector<pair<lng, lng>> convergents(const vector<lng> &a) {
    vector<pair<lng, lng>> res;
    lll p = 1, q = 0, pp = 0, qq = 1;
    for (int i = 0; i < int(a.size()); ++i) {
        assert(i == 0 || a[i] >= 1);
        pp = std::exchange(p, a[i] * p + pp); qq = std::exchange(q, a[i] * q + qq);
        assert(LLONG_MIN <= p && p <= LLONG_MAX && q <= LLONG_MAX);
        res.pb({lng(p), lng(q)});}
    return res;}
inline vector<pair<lng, lng>> semiconvergents(const vector<lng> &a) {
    vector<pair<lng, lng>> res;
    lll p = 1, q = 0, pp = 0, qq = 1;
    for (int i = 0; i < int(a.size()); ++i) {
        lll lo = i == 0 ? a[0] : 1, hi = a[i];
        assert((i == 0 || hi >= 1) && LLONG_MIN <= min(lo * p, hi * p) + pp && max(lo * p, hi * p) + pp <= LLONG_MAX && hi * q + qq <= LLONG_MAX);
        for (lll t = lo; t <= hi; ++t) { res.pb({lng(t * p + pp), lng(t * q + qq)}); }
        pp = std::exchange(p, a[i] * p + pp); qq = std::exchange(q, a[i] * q + qq);}
    return res;}

// T: O(log(|P| + |Q| + 2) + l), l = period length (O(|Q| * sqrt(D) * log(D * Q^2))), M: O(out); Q != 0, 0 <= D < 2^62, |P|, |Q| < 2^31 (Sqrt: P = 0, Q = 1);
// (P + sqrt(D)) / Q = [pre; period, period, ...] with minimal preperiod and period; a square D gives the finite expansion and an empty period.
struct PeriodicContinuedFraction { vector<lng> pre, period; };
inline PeriodicContinuedFraction continuedFractionQuadratic(lng P, lng D, lng Q) {
    assert(Q != 0 && 0 <= D && D < (1LL << 62) && -(1LL << 31) < P && P < (1LL << 31) && -(1LL << 31) < Q && Q < (1LL << 31));
    lll p = P, d = max<lng>(D, 0), q = Q;
    if ((d - p * p) % q != 0) { lll s = q < 0 ? -q : q; p *= s; d *= s * s; q *= s; }
    lll r = lll(continued_fraction_detail::isqrt(ulll(d)));
    if (r * r == d) { return {q < 0 ? continuedFraction(lng(-p - r), lng(-q)) : continuedFraction(lng(p + r), lng(q)), {}}; }
    vector<lng> terms;
    lll p0 = 0, q0 = 0;
    int start = -1;
    while (true) {
        if (start < 0 && q > 0 && p <= r && p + q > r && q <= p + r) { start = int(terms.size()); p0 = p; q0 = q; }
        else if (start >= 0 && p == p0 && q == q0) { break; }
        lll a = floorDiv(p + r + (q < 0), q);
        terms.pb(lng(a));
        p = a * q - p; q = (d - p * p) / q;}
    return {vector<lng>(terms.begin(), terms.begin() + start), vector<lng>(terms.begin() + start, terms.end())};}
inline PeriodicContinuedFraction continuedFractionSqrt(lng D) { return continuedFractionQuadratic(0, D, 1); }

// T: O(log(y)) (O(log(b + d)) for rationalInInterval), M: O(1); x >= 0, y >= 1, n >= 1: {largest p/q <= x/y, smallest r/s >= x/y}, 0 <= p, q, r, s <= n, q >= 1, reduced,
// r/s = 1/0 above n, both x/y when it qualifies; 0 <= a/b < c/d, b >= 1, d >= 0 (c/0 infinite), a + c, b + d fit lng: the p/q in (a/b, c/d) with least q, then p.
inline pair<pair<lng, lng>, pair<lng, lng>> bestRationalApproximation(lng x, lng y, lng n) {
    assert(x >= 0 && y >= 1 && n >= 1);
    lng g = gcd(x, y); x /= g; y /= g;
    if (x <= n && y <= n) { return {{x, y}, {x, y}}; }
    array<lng, 2> lo{0, 1}, hi{1, 0};
    for (bool right = true;; right = !right) {
        lll below = lll(x) * lo[1] - lll(lo[0]) * y, above = lll(hi[0]) * y - lll(x) * hi[1];
        auto &u = right ? lo : hi;
        auto &v = right ? hi : lo;
        lng full = lng(right ? below / above : above / below), t = full;
        if (v[0]) { t = min(t, (n - u[0]) / v[0]); }
        if (v[1]) { t = min(t, (n - u[1]) / v[1]); }
        u[0] += t * v[0]; u[1] += t * v[1];
        if (t < full) { return {{lo[0], lo[1]}, {hi[0], hi[1]}}; }}}
inline pair<lng, lng> rationalInInterval(lng a, lng b, lng c, lng d) {
    assert(a >= 0 && b >= 1 && c >= 0 && d >= 0 && lll(a) * d < lll(c) * b);
    lng pl = 0, ql = 1, pr = 1, qr = 0;
    while (true) {
        lng t = lng((lll(a) * ql - lll(pl) * b) / (lll(pr) * b - lll(a) * qr));
        pl += t * pr; ql += t * qr;
        if (lll(pl + pr) * d < lll(c) * (ql + qr)) { return {pl + pr, ql + qr}; }
        t = lng((lll(pr) * d - lll(c) * qr) / (lll(c) * ql - lll(pl) * d));
        pr += t * pl; qr += t * ql;
        if (lll(pl + pr) * b > lll(a) * (ql + qr)) { return {pl + pr, ql + qr}; }}}

// T: O(log(n)) calls of f, M: O(1); n >= 1, f(p, q) monotone over positive p/q (false, then true); the adjacent reduced pair {lo, hi} of
// {0/1, 1/0, p/q with 1 <= p, q <= n} with f(lo) false or lo = 0/1, f(hi) true or hi = 1/0.
template<typename F> pair<pair<lng, lng>, pair<lng, lng>> fractionBinarySearch(lng n, F f) {
    assert(n >= 1);
    array<lng, 2> lo{0, 1}, hi{1, 0};
    for (bool right = true; lll(lo[0]) + hi[0] <= n && lll(lo[1]) + hi[1] <= n; right = !right) {
        auto &u = right ? lo : hi;
        auto &v = right ? hi : lo;
        lng cap = min(v[0] ? (n - u[0]) / v[0] : n, v[1] ? (n - u[1]) / v[1] : n), t = 0;
        auto ok = [&](lll s) { return t + s <= cap && f(u[0] + lng(t + s) * v[0], u[1] + lng(t + s) * v[1]) != right; };
        lll step = 1;
        for (; ok(step); step *= 2) { t += lng(step); }
        while (step /= 2) { if (ok(step)) { t += lng(step); }}
        u[0] += t * v[0]; u[1] += t * v[1];}
    return {{lo[0], lo[1]}, {hi[0], hi[1]}};}

// T: O(log(max(p, q, r, s))) (Decode: O(runs)), M: O(log(max(p, q, r, s))); p, q, r, s >= 1, p/q and r/s reduced, results fit lng; Encode: the alternating run path from 1/1
// ({'L' or 'R', count >= 1}); Decode: its node (counts >= 0); Lca; Ancestor: depth k >= 0 (1/1 at 0) or {-1, -1}; Range: the open subtree interval (0/1, 1/0 ends); Children.
inline vector<pair<char, lng>> sternBrocotEncode(lng p, lng q) {
    assert(p >= 1 && q >= 1 && gcd(p, q) == 1);
    vector<pair<char, lng>> res;
    while (p != q && min(p, q) > 0) {
        if (p > q) { lng t = (p - 1) / q; res.pb({'R', t}); p -= t * q; }
        else { lng t = (q - 1) / p; res.pb({'L', t}); q -= t * p; }}
    return res;}
inline pair<lng, lng> sternBrocotDecode(const vector<pair<char, lng>> &path) {
    lng pl = 0, ql = 1, pr = 1, qr = 0;
    for (auto [c, t] : path) {
        assert((c == 'L' || c == 'R') && t >= 0);
        if (c == 'R') { pl += t * pr; ql += t * qr; }
        else { pr += t * pl; qr += t * ql; }}
    return {pl + pr, ql + qr};}
inline pair<lng, lng> sternBrocotLca(lng p, lng q, lng r, lng s) {
    auto x = sternBrocotEncode(p, q), y = sternBrocotEncode(r, s);
    vector<pair<char, lng>> common;
    for (int i = 0; i < int(min(x.size(), y.size())) && x[i].fi == y[i].fi; ++i) {
        common.pb({x[i].fi, min(x[i].se, y[i].se)});
        if (x[i].se != y[i].se) { break; }}
    return sternBrocotDecode(common);}
inline pair<lng, lng> sternBrocotAncestor(lng p, lng q, lng k) {
    assert(k >= 0);
    vector<pair<char, lng>> path;
    for (auto [c, t] : sternBrocotEncode(p, q)) {
        if (k == 0) { break; }
        path.pb({c, min(t, k)}); k -= min(t, k);}
    return k > 0 ? pair<lng, lng>{-1, -1} : sternBrocotDecode(path);}
inline pair<pair<lng, lng>, pair<lng, lng>> sternBrocotRange(lng p, lng q) {
    assert(p >= 1 && q >= 1 && gcd(p, q) == 1);
    lng pl = 0, ql = 1, pr = 1, qr = 0;
    for (lng x = p, y = q; x != y && min(x, y) > 0;) {
        if (x > y) { lng t = (x - 1) / y; x -= t * y; pl += t * pr; ql += t * qr; }
        else { lng t = (y - 1) / x; y -= t * x; pr += t * pl; qr += t * ql; }}
    return {{pl, ql}, {pr, qr}};}
inline pair<pair<lng, lng>, pair<lng, lng>> sternBrocotChildren(lng p, lng q) {
    auto [lo, hi] = sternBrocotRange(p, q);
    return {{lo.fi + p, lo.se + q}, {p + hi.fi, q + hi.se}};}

// T: O(out) (out ~ 0.304 n^2), O(log(q)) for fareyNext, O(n^(2/3) + sqrt(n) * log(n)^2) for fareyKth and countRationalsInRange (log(n + b + d)), M: O(out), O(1), O(n^(2/3));
// n >= 1 (n <= 10^13 for fareyKth and countRationalsInRange); F_n from 0/1 to 1/1; the successor of reduced p/q, 0 <= p < q <= n; the k-th term, 0 <= k < |F_n|; #{reduced r/s, 1 <= s <= n, a/b <= r/s < c/d}, 0 <= a/b <= c/d.
inline vector<pair<int, int>> fareySequence(int n) {
    assert(n >= 1);
    vector<pair<int, int>> res{{0, 1}};
    for (lng a = 0, b = 1, c = 1, d = n; c <= n;) {
        res.pb({int(c), int(d)});
        lng k = (n + b) / d, e = k * c - a, f = k * d - b;
        a = c; b = d; c = e; d = f;}
    return res;}
inline pair<lng, lng> fareyNext(lng p, lng q, lng n) {
    assert(0 <= p && p < q && q <= n && gcd(p, q) == 1);
    lng s = modNorm(-extendedGcd(p, q).x, q);
    s += (n - s) / q * q;
    return {lng((lll(p) * s + 1) / q), s};}
inline pair<lng, lng> fareyKth(lng n, lng k) {
    assert(1 <= n && n <= 10000000000000LL && k >= 0);
    if (k == 0) { return {0, 1}; }
    continued_fraction_detail::Mertens mu(n);
    assert(k <= continued_fraction_detail::countBelow(mu, 1, 1, false));
    return fractionBinarySearch(n, [&](lng p, lng q) { return continued_fraction_detail::countBelow(mu, p, q, true) > k; }).se;}
inline lll countRationalsInRange(lng n, lng a, lng b, lng c, lng d) {
    assert(1 <= n && n <= 10000000000000LL && a >= 0 && b >= 1 && d >= 1 && lll(a) * d <= lll(c) * b);
    continued_fraction_detail::Mertens mu(n);
    return lll(ulll(continued_fraction_detail::countBelow(mu, c, d, false)) - ulll(continued_fraction_detail::countBelow(mu, a, b, false)));}
