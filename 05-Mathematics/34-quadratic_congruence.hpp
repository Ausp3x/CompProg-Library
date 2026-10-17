#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "07-primality_factorization.hpp"
#include "11-discrete_log_root.hpp"
#include "33-modular_square_roots.hpp"

namespace quadratic_detail {
    // T: O(1) for isqrt, O(log(m)) for descent, O(log(p) + z * log(p)) expected for gaussPrime, M: O(1); floor(sqrt(n)) for every ulng;
    // descent: Cornacchia's Euclidean descent from a root r of -d mod m >= 1, (x, y) with x^2 + d y^2 = m or (-1, -1); gaussPrime: a^2 + b^2 = p for p = 2 or prime p = 1 mod 4.
    inline ulng isqrt(ulng n) {
        ulng r = ulng(std::sqrt(double(n)));
        while (r && r > n / r) { --r; }
        while (r + 1 <= n / (r + 1)) { ++r; }
        return r;}
    inline pair<lng, lng> descent(lng d, lng m, lng r) {
        for (lng prev = m; lll(r) * r >= m;) { prev = std::exchange(r, prev % r); }
        lng t = m - r * r, y = lng(isqrt(ulng(t / d)));
        return t % d || y * y != t / d ? pair<lng, lng>{-1, -1} : pair<lng, lng>{r, y};}
    inline pair<lng, lng> gaussPrime(lng p) { return p == 2 ? pair<lng, lng>{1, 1} : descent(1, p, sqrtModPrimeCipolla(p - 1, p)); }

    // T: O(l) ring operations, l = period of the continued fraction of sqrt(D), M: O(1); D >= 2 not a square; least (x, y), y > 0, with x^2 - D y^2 = (-1)^l, and whether l is odd.
    template<typename T> tuple<T, T, bool> pellUnit(lng D) {
        lng s = lng(isqrt(ulng(D))), num = 0, den = 1, a = s;
        T x0 = T(1), x1 = T(a), y0 = T(0), y1 = T(1);
        for (bool odd = true;; odd = !odd) {
            num = a * den - num; den = (D - num * num) / den; a = (s + num) / den;
            if (den == 1) { return {x1, y1, odd}; }
            x0 = std::exchange(x1, T(a) * x1 + x0); y0 = std::exchange(y1, T(a) * y1 + y0);}}

    // T: O(1 + steps) ring operations, M: O(1); ordered T, (ux, uy) the fundamental unit; the least y >= 0 (then larger x) among (+-)(x + y sqrt(D)) (ux + uy sqrt(D))^k.
    template<typename T> pair<T, T> normalize(T x, T y, const T &ux, const T &uy, lng D) {
        auto mag = [](const T &v) { return v < T(0) ? T(0) - v : v; };
        auto step = [&](const T &px, const T &py, bool up) {
            T v = up ? uy : T(0) - uy;
            return pair<T, T>{px * ux + T(D) * py * v, px * v + py * ux};};
        for (bool up : {true, false}) {
            for (auto nx = step(x, y, up); mag(nx.se) < mag(y); nx = step(x, y, up)) { x = nx.fi; y = nx.se; }}
        pair<T, T> best{T(0), T(-1)};
        for (auto c : {pair<T, T>{x, y}, step(x, y, true), step(x, y, false)}) {
            if (mag(c.se) != mag(y)) { continue; }
            if (c.se < T(0) || (c.se == T(0) && c.fi < T(0))) { c = {T(0) - c.fi, T(0) - c.se}; }
            if (best.se < T(0) || best.fi < c.fi) { best = c; }}
        return best;}

    // T: O(1) for definite, O(log(max(|b|, c) / a) + 1) steps for reduce, M: O(1); definite: a > 0, b^2 < 4 a c, coefficients and |D| below 2^62; reduce: in place, right-multiplying t (row-major 2 x 2) by each SL2(Z) step, so the result is f(t (x, y)).
    inline bool definite(lng a, lng b, lng c) {
        lng lim = lng(1) << 62;
        return 0 < a && a < lim && 0 < c && c < lim && -lim < b && b < lim && lll(b) * b < 4 * lll(a) * c && 4 * lll(a) * c - lll(b) * b < lll(lim);}
    inline void reduce(lll &a, lll &b, lll &c, array<lll, 4> &t) {
        auto shift = [&]() {
            lll q = floorDiv(a - b, 2 * a);
            c += (a * q + b) * q; b += 2 * a * q;
            t[1] += q * t[0]; t[3] += q * t[2];};
        auto flip = [&]() {
            std::swap(a, c); b = -b;
            t = {t[1], -t[0], t[3], -t[2]};};
        for (shift(); a > c; shift()) { flip(); }
        if (a == c && b < 0) { flip(); }}
} // namespace quadratic_detail

// T: O(l) ring operations, l = period of the continued fraction of sqrt(D) (O(sqrt(D) * log(D))), M: O(1); T from lng with + and *; least positive (x, y) with x^2 - D y^2 = 1, or (0, 0) when D < 2 or D is a square.
template<typename T> pair<T, T> pellFundamental(lng D) {
    ulng s = quadratic_detail::isqrt(ulng(max(D, lng(0))));
    if (D < 2 || lll(s) * s == D) { return {T(0), T(0)}; }
    auto [x, y, odd] = quadratic_detail::pellUnit<T>(D);
    if (odd) { return {x * x + T(D) * y * y, T(2) * x * y}; }
    return {x, y};}

// T: O(log(|k| + 2)) ring operations, M: O(1); x1^2 - D y1^2 = 1 (asserted, x1^2 fits T), T from lng with +, -, *, ==; (x + y sqrt(D)) (x1 + y1 sqrt(D))^k, k < 0 through the conjugate unit.
template<typename T> pair<T, T> pellSolutions(T x, T y, T x1, T y1, lng D, lng k) {
    assert(x1 * x1 - T(D) * y1 * y1 == T(1));
    if (k < 0) { y1 = T(0) - y1; }
    for (ulng e = k < 0 ? -ulng(k) : ulng(k); e; e >>= 1) {
        if (e & 1) { std::tie(x, y) = pair<T, T>{x * x1 + T(D) * y * y1, x * y1 + y * x1}; }
        if (e > 1) { std::tie(x1, y1) = pair<T, T>{x1 * x1 + T(D) * y1 * y1, T(2) * x1 * y1}; }}
    return {x, y};}

// T: O(q * |N|^(1/4) * log(|N|) + l + sum over (f, z) of the continued-fraction length of (z + sqrt(D)) / |m|) ring operations, q = square divisors of N, M: O(out); D >= 2 not a square,
// N > LLONG_MIN, T ordered, exact, from lll: iint, or lll when 64 x1^2 max(|N|, 1) < 2^127 for x1 = pellFundamental(D); one fundamental solution per class (least y >= 0, then x >= 0), sorted by (y, x); N = 0 gives (0, 0).
template<typename T> vector<pair<T, T>> pellGeneralized(lng D, lng N) {
    lll s = quadratic_detail::isqrt(ulng(max(D, lng(0))));
    assert(D >= 2 && s * s != D && N != std::numeric_limits<lng>::min());
    if (D < 2 || s * s == D || N == std::numeric_limits<lng>::min()) { return {}; }
    if (!N) { return {{T(0), T(0)}}; }
    auto [t, u, odd] = quadratic_detail::pellUnit<T>(D);
    T ux = odd ? t * t + T(D) * u * u : t, uy = odd ? T(2) * t * u : u;
    vector<lng> fs{1};
    for (auto [p, e] : factorize(ulng(N < 0 ? -N : N))) {
        int len = int(fs.size());
        for (lng i = 2, q = lng(p); i <= e; i += 2, q *= lng(p)) {
            for (int j = 0; j < len; ++j) { fs.pb(fs[j] * q); }}}
    vector<pair<T, T>> res;
    for (lng f : fs) {
        lng m = N / (f * f), am = m < 0 ? -m : m;
        for (auto [r, md] : sqrtModAll(D, am)) {
            for (lll z0 = r; z0 < am; z0 += md) {
                // Lagrange-Matthews-Mollin: expand (z + sqrt(D)) / |m| until den = +-1 or a reduced state repeats.
                lll z = 2 * z0 > am ? z0 - am : z0, num = z, den = am, rn = 0, rd = 0;
                T g0 = T(-z), g1 = T(am), b0 = T(1), b1 = T(0);
                for (bool sign = true;; sign = !sign) {
                    lll a = floorDiv(den > 0 ? num + s : num + s + 1, den);
                    g0 = std::exchange(g1, T(a) * g1 + g0); b0 = std::exchange(b1, T(a) * b1 + b0);
                    num = a * den - num; den = (D - num * num) / den;
                    if (den == 1 || den == -1) {
                        T x = T(f) * g1, y = T(f) * b1;
                        if ((sign ? -den : den) * am != m) {
                            if (!odd) { break; }
                            std::tie(x, y) = pair<T, T>{x * t + T(D) * y * u, x * u + y * t};}
                        res.pb(quadratic_detail::normalize(x, y, ux, uy, D));
                        break;}
                    bool reduced = den > 0 && num > 0 && num <= s && den > s - num && den <= s + num;
                    if (reduced && num == rn && den == rd) { break; }
                    if (reduced && !rd) { rn = num; rd = den; }}}}}
    sort(res.begin(), res.end(), [](const pair<T, T> &x, const pair<T, T> &y) { return x.se < y.se || (x.se == y.se && x.fi < y.fi); });
    return res;}

// T: O(m^(1/4) * log(m) + 2^w * log(m)) expected, w = distinct primes of m, M: O(2^w); d >= 1, m >= 1, gcd(d, m) = 1; all (x, y), x, y >= 0, gcd(x, y) = 1, x^2 + d y^2 = m, sorted.
inline vector<pair<lng, lng>> cornacchia(lng d, lng m) {
    assert(d >= 1 && m >= 1 && gcd(d, m) == 1);
    if (d < 1 || m < 1 || gcd(d, m) != 1) { return {}; }
    if (m == 1) { return d == 1 ? vector<pair<lng, lng>>{{0, 1}, {1, 0}} : vector<pair<lng, lng>>{{1, 0}}; }
    vector<pair<lng, lng>> res;
    for (auto [r, md] : sqrtModAll(-d, m)) {
        for (lll x = r; 2 * x <= m; x += md) {
            auto [u, v] = quadratic_detail::descent(d, m, lng(x));
            if (u < 0 || gcd(u, v) != 1) { continue; }
            res.pb({u, v});
            if (d == 1) { res.pb({v, u}); }}}
    sort(res.begin(), res.end());
    res.erase(unique(res.begin(), res.end()), res.end());
    return res;}

// T: O(n^(1/4) * log(n) + log(n)^2) expected (twoSquaresAll plus O(out * log(out))), M: O(log(n)) (O(out) for all); n >= 0;
// twoSquares: (x, y), x >= y >= 0, or (-1, -1); twoSquaresAll: every (x, y) with x, y >= 0, sorted; twoSquaresCount: r2(n), signs and order counted.
inline pair<lng, lng> twoSquares(lng n) {
    assert(n >= 0);
    if (n <= 0) { return n ? pair<lng, lng>{-1, -1} : pair<lng, lng>{0, 0}; }
    lll x = 1, y = 0;
    for (auto [p, e] : factorize(ulng(n))) {
        if (p % 4 == 3 && e % 2) { return {-1, -1}; }
        auto [a, b] = p % 4 == 3 ? pair<lng, lng>{lng(p), 0} : quadratic_detail::gaussPrime(lng(p));
        for (int i = 0; i < (p % 4 == 3 ? e / 2 : e); ++i) { std::tie(x, y) = pair<lll, lll>{x * a - y * b, x * b + y * a}; }}
    x = x < 0 ? -x : x; y = y < 0 ? -y : y;
    return {lng(max(x, y)), lng(min(x, y))};}
inline vector<pair<lng, lng>> twoSquaresAll(lng n) {
    assert(n >= 0);
    if (n <= 0) { return n ? vector<pair<lng, lng>>{} : vector<pair<lng, lng>>{{0, 0}}; }
    vector<pair<lll, lll>> zs{{1, 0}};
    for (auto [p, e] : factorize(ulng(n))) {
        if (p % 4 == 3 && e % 2) { return {}; }
        auto [a, b] = p % 4 == 3 ? pair<lng, lng>{lng(p), 0} : quadratic_detail::gaussPrime(lng(p));
        int c = p % 4 == 1 ? e : 0, f = p % 4 == 3 ? e / 2 : e;
        vector<pair<lll, lll>> opts, nxt;
        for (int k = 0; k <= c; ++k) {
            lll x = 1, y = 0;
            for (int i = 0; i < f; ++i) { std::tie(x, y) = pair<lll, lll>{x * a - y * (i < k || !c ? b : -b), x * (i < k || !c ? b : -b) + y * a}; }
            opts.pb({x, y});}
        for (auto [x, y] : zs) {
            for (auto [u, v] : opts) { nxt.pb({x * u - y * v, x * v + y * u}); }}
        zs = nxt;}
    vector<pair<lng, lng>> res;
    for (auto [x, y] : zs) {
        while (x <= 0 || y < 0) { std::tie(x, y) = pair<lll, lll>{-y, x}; }
        res.pb({lng(x), lng(y)});}
    ulng s = quadratic_detail::isqrt(ulng(n));
    if (lll(s) * s == n) { res.pb({0, lng(s)}); }
    sort(res.begin(), res.end());
    return res;}
inline lng twoSquaresCount(lng n) {
    assert(n >= 0);
    if (n <= 0) { return n ? 0 : 1; }
    lng res = 4;
    for (auto [p, e] : factorize(ulng(n))) {
        if (p % 4 == 3 && e % 2) { return 0; }
        res *= p % 4 == 1 ? e + 1 : 1;}
    return res;}

// T: O(c * log(n)) expected heuristic (Rabin-Shallit scan, c = candidates, O(log(n)) observed), worst case O(sqrt(n) * n^(1/4) * log(n)) by the factoring fallback, M: O(log(n)); n >= 0;
// threeSquares: descending (x, y, z) with x^2 + y^2 + z^2 = n, or (-1, -1, -1) for n = 4^a (8 b + 7); fourSquares: every n.
inline array<lng, 3> threeSquares(lng n) {
    assert(n >= 0);
    if (n <= 0) { return n ? array<lng, 3>{-1, -1, -1} : array<lng, 3>{0, 0, 0}; }
    lng sc = 1;
    for (; n % 4 == 0; n /= 4) { sc *= 2; }
    if (n % 8 == 7) { return {-1, -1, -1}; }
    auto done = [&](lng x, lng y, lng z) {
        array<lng, 3> r{x * sc, y * sc, z * sc};
        sort(r.rbegin(), r.rend());
        return r;};
    // Scan z downward with the parity making n - z^2 (halved when n = 3 mod 8) = 1 mod 4, accepting a square or a prime.
    lng z = lng(quadratic_detail::isqrt(ulng(n))), half = n % 8 == 3;
    if ((z % 2 == 0) != (n % 4 == 1)) { --z; }
    for (; z >= 0; z -= 2) {
        lng r = (n - z * z) >> half, s = lng(quadratic_detail::isqrt(ulng(r)));
        auto [x, y] = s * s == r ? pair<lng, lng>{s, 0} : isPrime(ulng(r)) ? quadratic_detail::gaussPrime(r) : pair<lng, lng>{-1, -1};
        if (x >= 0) { return half ? done(x + y, x > y ? x - y : y - x, z) : done(x, y, z); }}
    for (z = lng(quadratic_detail::isqrt(ulng(n)));; --z) {
        auto [x, y] = twoSquares(n - z * z);
        if (x >= 0) { return done(x, y, z); }}}
inline array<lng, 4> fourSquares(lng n) {
    assert(n >= 0);
    if (n <= 0) { return {0, 0, 0, 0}; }
    lng sc = 1;
    for (; n % 4 == 0; n /= 4) { sc *= 2; }
    lng w = n % 8 == 7;
    auto t = threeSquares(n - w);
    array<lng, 4> r{t[0] * sc, t[1] * sc, t[2] * sc, w * sc};
    sort(r.rbegin(), r.rend());
    return r;}

// T: O(threeSquares(8 n + 3)), M: O(log(n)); 0 <= n <= (2^63 - 4) / 8; descending (a, b, c) with a(a + 1)/2 + b(b + 1)/2 + c(c + 1)/2 = n.
inline array<lng, 3> threeTriangular(lng n) {
    assert(0 <= n && n <= (std::numeric_limits<lng>::max() - 3) / 8);
    auto r = threeSquares(8 * min(max(n, lng(0)), (std::numeric_limits<lng>::max() - 3) / 8) + 3);
    return {(r[0] - 1) / 2, (r[1] - 1) / 2, (r[2] - 1) / 2};}

// T: O(out), M: O(out); 0 <= N <= 2^63 / 8; every primitive (a, b, c), a odd, b even, a^2 + b^2 = c^2 <= N^2 with c <= N, in Berggren-tree order.
inline vector<array<lng, 3>> pythagoreanTriples(lng N) {
    assert(0 <= N && N <= std::numeric_limits<lng>::max() / 8);
    vector<array<lng, 3>> res, todo;
    if (N >= 5 && N <= std::numeric_limits<lng>::max() / 8) { todo.pb({3, 4, 5}); }
    while (!todo.empty()) {
        auto [a, b, c] = todo.back();
        todo.pop_back();
        res.pb({a, b, c});
        for (array<lng, 3> x : {array<lng, 3>{a - 2 * b + 2 * c, 2 * a - b + 2 * c, 2 * a - 2 * b + 3 * c}, array<lng, 3>{a + 2 * b + 2 * c, 2 * a + b + 2 * c, 2 * a + 2 * b + 3 * c},
                array<lng, 3>{-a + 2 * b + 2 * c, -2 * a + b + 2 * c, -2 * a + 2 * b + 3 * c}}) {
            if (x[2] <= N) { todo.pb(x); }}}
    return res;}

// T: O(sqrt(n)), M: O(1); n >= 0; the number of (x, y) in Z^2 with x^2 + y^2 <= n.
inline lll gaussCircleCount(lng n) {
    assert(n >= 0);
    if (n < 0) { return 0; }
    lng r = lng(quadratic_detail::isqrt(ulng(n))), y = r;
    lll res = 2 * r + 1;
    for (lng x = 1; x <= r; ++x) {
        while (lll(x) * x + lll(y) * y > n) { --y; }
        res += 2 * (2 * y + 1);}
    return res;}

// T: O(log(max(|b|, c) / a) + 1) for reduce, O(log(|D|)) for compose, M: O(1); positive definite a x^2 + b x y + c y^2 (a > 0, D = b^2 - 4 a c < 0), coefficients and |D| below 2^62;
// reduce: the unique equivalent form with |b| <= a <= c, b >= 0 when |b| = a or a = c; compose: Gauss composition of primitive forms of one discriminant, reduced.
struct BinaryQuadraticForm {
    lng a, b, c;

    // T: O(|D|^(3/4)) expected, M: O(h(D)); -2^62 < D < 0, D = 0 or 1 mod 4; every primitive reduced form of discriminant D, sorted by (a, b, c).
    static vector<BinaryQuadraticForm> reducedForms(lng D) {
        bool ok = -(lng(1) << 62) < D && D < 0 && (D % 4 == 0 || D % 4 == -3);
        assert(ok);
        vector<BinaryQuadraticForm> res;
        for (lng b = D & 1; ok && 3 * b * b <= -D; b += 2) {
            lng prod = (b * b - D) / 4;
            for (ulng x : divisors(ulng(prod))) {
                lng a = lng(x), c = prod / a;
                if (a < max(b, lng(1)) || gcd(gcd(a, b), c) != 1) { continue; }
                if (a > c) { break; }
                res.pb({a, b, c});
                if (b && b != a && a != c) { res.pb({a, -b, c}); }}}
        sort(res.begin(), res.end(), [](const BinaryQuadraticForm &x, const BinaryQuadraticForm &y) { return std::tie(x.a, x.b, x.c) < std::tie(y.a, y.b, y.c); });
        return res;}

    BinaryQuadraticForm reduce() const {
        assert(quadratic_detail::definite(a, b, c));
        lll x = a, y = b, z = c;
        array<lll, 4> t{1, 0, 0, 1};
        if (quadratic_detail::definite(a, b, c)) { quadratic_detail::reduce(x, y, z, t); }
        return {lng(x), lng(y), lng(z)};}

    BinaryQuadraticForm compose(const BinaryQuadraticForm &o) const {
        bool ok = quadratic_detail::definite(a, b, c) && quadratic_detail::definite(o.a, o.b, o.c);
        lll disc = ok ? lll(b) * b - 4 * lll(a) * c : 0;
        ok = ok && disc == lll(o.b) * o.b - 4 * lll(o.a) * o.c && gcd(gcd(a, b), c) == 1 && gcd(gcd(o.a, o.b), o.c) == 1;
        assert(ok);
        if (!ok) { return *this; }
        BinaryQuadraticForm f = reduce(), g = o.reduce();
        if (f.a > g.a) { std::swap(f, g); }
        lng s = (f.b + g.b) / 2, n = g.b - s, y1 = 0, d = f.a, x2 = 0, y2 = -1, d1 = f.a;
        if (g.a % f.a) { auto e = extendedGcd(g.a, f.a); y1 = lng(e.x); d = d1 = lng(e.g); }
        if (s % d) { auto e = extendedGcd(s, d); x2 = lng(e.x); y2 = lng(-e.y); d1 = lng(e.g); }
        lng v1 = f.a / d1, v2 = g.a / d1, r = modNorm(lll(modMul(y1, y2, v1)) * (n % v1) - lll(x2 % v1) * (g.c % v1), v1);
        lll a3 = lll(v1) * v2, b3 = g.b + 2 * lll(v2) * r, c3 = (b3 * b3 - disc) / (4 * a3);
        array<lll, 4> t{1, 0, 0, 1};
        quadratic_detail::reduce(a3, b3, c3, t);
        return {lng(a3), lng(b3), lng(c3)};}

    // T: O(n^(1/4) * log(n) * q + R * log(n)) expected, q = square divisors of n, R = roots t enumerated, M: O(out + R); n >= 0; every (x, y) in Z^2 with a x^2 + b x y + c y^2 = n, sorted.
    vector<pair<lng, lng>> represent(lng n) const {
        assert(quadratic_detail::definite(a, b, c));
        if (n < 0 || !quadratic_detail::definite(a, b, c)) { return {}; }
        if (!n) { return {{0, 0}}; }
        lng k = gcd(gcd(a, b), c);
        if (n % k) { return {}; }
        lll fa = a / k, fb = b / k, fc = c / k, disc = fb * fb - 4 * fa * fc;
        array<lll, 4> tf{1, 0, 0, 1};
        quadratic_detail::reduce(fa, fb, fc, tf);
        array<lll, 4> sg = fa == 1 && fb == 0 && fc == 1 ? array<lll, 4>{0, -1, 1, 0} : fa == 1 && fb == 1 && fc == 1 ? array<lll, 4>{0, -1, 1, 1} : array<lll, 4>{-1, 0, 0, -1};
        int order = fa == 1 && fc == 1 && fb <= 1 ? int(4 + 2 * fb) : 2;
        n /= k;
        vector<lng> gs{1};
        for (auto [p, e] : factorize(ulng(n))) {
            int len = int(gs.size());
            for (lng i = 2, q = lng(p); i <= e; i += 2, q *= lng(p)) {
                for (int j = 0; j < len; ++j) { gs.pb(gs[j] * q); }}}
        vector<pair<lng, lng>> res;
        lng del = lng(disc & 1);
        for (lng g : gs) {
            lng nn = n / g / g;
            for (auto [r0, md] : quadraticCongruenceMod(1, del, lng((del - disc) / 4), nn)) {
                for (lll t0 = r0; t0 < nn; t0 += md) {
                    lll hb = 2 * t0 + del, ha = nn;
                    hb -= hb > nn ? 2 * ha : 0;
                    lll hc = (hb * hb - disc) / (4 * ha);
                    if (gcd(gcd(lng(ha), lng(hb < 0 ? -hb : hb)), lng(hc)) != 1) { continue; }
                    array<lll, 4> th{1, 0, 0, 1};
                    quadratic_detail::reduce(ha, hb, hc, th);
                    if (ha != fa || hb != fb || hc != fc) { continue; }
                    lll u = th[3], v = -th[2];
                    for (int i = 0; i < order; ++i) {
                        res.pb({lng(g * (tf[0] * u + tf[1] * v)), lng(g * (tf[2] * u + tf[3] * v))});
                        std::tie(u, v) = pair<lll, lll>{sg[0] * u + sg[1] * v, sg[2] * u + sg[3] * v};}}}}
        sort(res.begin(), res.end());
        return res;}
};
