#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"
#include "07-primality_factorization.hpp"
#include "08-multiplicative_functions.hpp"

namespace discrete_log_detail {
    // T: O(1) for mul, O(|f| * log(m)) for generates, M: O(1); x * y mod m, and whether g^(t / q) != 1 mod m for each prime q of t listed in f.
    inline ulng mul(ulng x, ulng y, ulng m) { return ulng(ulll(x) * y % m); }
    inline bool generates(ulng g, ulng m, ulng t, const vector<pair<ulng, int>> &f) {
        for (auto [q, e] : f) { if (modPow64(g, t / q, m) == 1) { return false; } }
        return true;}
    // T: O(sqrt(t)) expected, M: O(sqrt(t)); unit a, t + 1 >= ord(a): least x >= 0 with k * a^x = b mod m, or -1.
    inline lng bsgs(ulng a, ulng b, ulng k, ulng m, ulng t) {
        if (k == b) { return 0; }
        ulng n = ulng(std::sqrt(double(t))) + 1;
        while (n <= t / n) { ++n; }
        ulng cap = std::bit_ceil(2 * n);
        int sh = 64 - std::countr_zero(cap);
        vector<ulng> key(cap, ~0ULL);
        vector<uint> val(cap);
        auto slot = [&](ulng x) {
            ulng i = (x * 0x9e3779b97f4a7c15ULL) >> sh;
            while (key[i] != x && key[i] != ~0ULL) { i = (i + 1) & (cap - 1); }
            return i;};
        // Keeping the largest j per key makes the first giant hit i * n - j the least solution.
        for (ulng j = 0, v = b; j < n; ++j, v = mul(v, a, m)) {
            ulng i = slot(v);
            key[i] = v; val[i] = uint(j);}
        ulng an = modPow64(a, n, m);
        for (ulng i = 1, v = mul(k, an, m); i <= n; ++i, v = mul(v, an, m)) {
            ulng s = slot(v);
            if (key[s] == v) { return lng(i * n - val[s]); }}
        return -1;}
    // T: O(s * log(p) + (s - e) * (sqrt(q) + s * log(q)) + z * log(p)) expected, s = v_q(p - 1), z = nonresidue candidates, M: O(sqrt(q)); a q^e-th root of a q^e-th power unit c.
    inline ulng rootPrimePower(ulng c, ulng q, int e, ulng p) {
        ulng r = p - 1, qe = 1, qt = 1;
        int s = 0;
        while (r % q == 0) { r /= q; ++s; }
        for (int i = 0; i < s; ++i) { (i < e ? qe : qt) *= q; }
        ulng x = modPow64(c, ulng(modNorm(extendedGcd(lng(qe), lng(r)).x, lng(r))), p);
        if (s == e) { return x; }
        ulng z = 2;
        while (modPow64(z, (p - 1) / q, p) == 1) { ++z; }
        // h generates the q-Sylow subgroup; x^qe / c = hq^j for hq = h^qe, found digit by digit (Pohlig-Hellman), and x h^-j is the root.
        ulng h = modPow64(z, r, p), hq = modPow64(h, qe, p), cur = mul(modPow64(x, qe, p), modPow64(c, p - 2, p), p);
        ulng ex = qt / q, gam = modPow64(hq, ex, p), step = modPow64(hq, qt - 1, p), j = 0;
        for (ulng qi = 1; qi < qt; qi *= q, ex /= q) {
            ulng d = ulng(bsgs(gam, modPow64(cur, ex, p), 1, p, q - 1));
            j += d * qi;
            cur = mul(cur, modPow64(step, d, p), p);
            step = modPow64(step, q, p);}
        return mul(x, modPow64(h, qt * qe - j, p), p);}
} // namespace discrete_log_detail

// T: O(m^(1/4) * log(m)) expected, M: O(log(m)); m >= 1, least t >= 1 with a^t = 1 mod m, or -1 for a nonunit.
inline lng multiplicativeOrder(lng a, lng m) {
    assert(m >= 1);
    ulng x = ulng(modNorm(a, m)), t = carmichaelLambda(ulng(m));
    if (gcd(x, ulng(m)) != 1) { return -1; }
    for (auto [q, e] : factorize(t)) {
        while (t % q == 0 && modPow64(x, t / q, ulng(m)) == 1) { t /= q; }}
    return lng(t);}

// T: O(sqrt(t) + m^(1/4) * log(m)) expected, t < m the order of a once gcd(a, m) is removed, M: O(sqrt(t)); m >= 1, 0^0 = 1, least x >= 0 with k * a^x = b mod m, or -1.
inline lng discreteLogEx(lng a, lng b, lng m, lng k = 1) {
    assert(m >= 1);
    m = max(m, lng(1));
    ulng n = ulng(m), x = ulng(modNorm(a, m)), y = ulng(modNorm(b, m)), c = ulng(modNorm(k, m));
    for (lng add = 0;; ++add) {
        if (c == y) { return add; }
        ulng g = gcd(x, n);
        if (g == 1) {
            lng r = discrete_log_detail::bsgs(x, y, c, n, ulng(multiplicativeOrder(lng(x), lng(n))) - 1);
            return r < 0 ? -1 : r + add;}
        if (y % g) { return -1; }
        n /= g; y /= g;
        c = discrete_log_detail::mul(c, x / g, n); x %= n;}}

// T: O(sqrt(ub - lb) * (act + hash) + log(ub) * mul) expected, M: O(sqrt(ub - lb)) words and O(min(sqrt(ub - lb), orbit of t)) states; 0 <= lb <= ub, least n in [lb, ub) with x^n acting on s equal to t, or -1.
template<typename X, typename S, typename Mul, typename Act, typename Hash>
lng discreteLogMonoid(const X &x, S s, const S &t, lng lb, lng ub, Mul mul, Act act, Hash hash) {
    assert(0 <= lb && lb <= ub);
    lb = max(lb, lng(0));
    if (lb >= ub) { return -1; }
    auto pw = [&](ulng e) {
        X r = x, b = x;
        for (--e; e; e >>= 1) {
            if (e & 1) { r = mul(r, b); }
            if (e > 1) { b = mul(b, b); }}
        return r;};
    if (lb) { s = act(pw(ulng(lb)), s); }
    ulng len = ulng(ub - lb), n = ulng(std::sqrt(double(len)));
    while (n * n < len) { ++n; }
    ulng cap = std::bit_ceil(2 * n);
    int sh = 64 - std::countr_zero(cap);
    vector<S> baby;
    vector<uint> tab(cap, 0);
    auto slot = [&](const S &v) {
        ulng i = (ulng(hash(v)) * 0x9e3779b97f4a7c15ULL) >> sh;
        while (tab[i] && !(baby[tab[i] - 1] == v)) { i = (i + 1) & (cap - 1); }
        return i;};
    S u = t;
    for (ulng j = 0; j < n; ++j) {
        u = act(x, u);
        ulng i = slot(u);
        if (tab[i]) { break; }
        baby.pb(u); tab[i] = uint(baby.size());}
    X y = pw(n);
    // A block holding a solution always hits; once a true solution exists, a false hit is followed only by true ones.
    for (ulng k = 0, failed = 0; k * n < len; ++k) {
        S nx = act(y, s);
        if (tab[slot(nx)]) {
            for (ulng i = 0; i < n && k * n + i < len; ++i, s = act(x, s)) {
                if (s == t) { return lb + lng(k * n + i); }}
            if (++failed == 2) { return -1; }}
        s = nx;}
    return -1;}

// T: O(log(m)^2) for exists, else O(m^(1/4) * log(m) + r * log(m)^2) expected, r = candidates tried, M: O(log(m)); m >= 1; smallest primitive root in [0, m) or -1.
inline bool primitiveRootExists(lng m) {
    assert(m >= 1);
    if (m % 4 == 0) { return m == 4; }
    ulng n = ulng(m % 2 ? m : m / 2);
    return n == 1 || isPrimePower(n) != 0;}
inline bool isPrimitiveRoot(lng g, lng m) {
    if (!primitiveRootExists(m)) { return false; }
    ulng x = ulng(modNorm(g, m)), t = phi(ulng(m));
    return gcd(x, ulng(m)) == 1 && discrete_log_detail::generates(x, ulng(m), t, factorize(t));}
inline lng primitiveRoot(lng m) {
    if (!primitiveRootExists(m)) { return -1; }
    ulng t = phi(ulng(m));
    auto f = factorize(t);
    for (ulng g = 1;; ++g) {
        if (gcd(g, ulng(m)) == 1 && discrete_log_detail::generates(g, ulng(m), t, f)) { return lng(g % ulng(m)); }}}

// T: O(log(|a| + |n| + 2)) for kronecker, O(log(p)) for legendre, M: O(1); all a and n, odd prime p; -1, 0 or 1.
inline int kroneckerSymbol(lng a, lng n) {
    if (!n) { return a == 1 || a == -1; }
    if (!(a & 1) && !(n & 1)) { return 0; }
    int v = std::countr_zero(ulng(n)), res = (v & 1) && ((a & 7) == 3 || (a & 7) == 5) ? -1 : 1;
    n >>= v;
    if (n < 0) { n = -n; res = a < 0 ? -res : res; }
    ulng x = ulng(modNorm(a, n)), y = ulng(n);
    while (x) {
        v = std::countr_zero(x); x >>= v;
        if ((v & 1) && (y % 8 == 3 || y % 8 == 5)) { res = -res; }
        if (x % 4 == 3 && y % 4 == 3) { res = -res; }
        swap(x, y); x %= y;}
    return y == 1 ? res : 0;}
inline int legendreSymbol(lng a, lng p) {
    assert(p >= 3 && p % 2);
    ulng r = modPow64(ulng(modNorm(a, p)), ulng(p - 1) / 2, ulng(p));
    return r <= 1 ? int(r) : -1;}

// T: O((1 + z) * log(p)), z = candidates t with t^2 - a tried, M: O(1); prime p, the smaller root min(x, p - x), or -1.
inline lng sqrtModPrimeCipolla(lng a, lng p) {
    assert(p >= 2);
    ulng x = ulng(modNorm(a, p)), m = ulng(p);
    if (x < 2 || m == 2) { return lng(x); }
    if (kroneckerSymbol(lng(x), p) != 1) { return -1; }
    ulng t = 0, w = m - x;
    while (kroneckerSymbol(lng(w), p) != -1) {
        if (++t == 64 && !isPrime(m)) { return -1; }
        w = (discrete_log_detail::mul(t, t, m) + m - x) % m;}
    auto times = [&](ulng &c0, ulng &c1, ulng d0, ulng d1) {
        ulng e0 = (discrete_log_detail::mul(c0, d0, m) + discrete_log_detail::mul(discrete_log_detail::mul(c1, d1, m), w, m)) % m;
        c1 = (discrete_log_detail::mul(c0, d1, m) + discrete_log_detail::mul(c1, d0, m)) % m; c0 = e0;};
    ulng u = 1, v = 0, b0 = t, b1 = 1;
    for (ulng e = (m + 1) / 2; e; e >>= 1) {
        if (e & 1) { times(u, v, b0, b1); }
        times(b0, b1, b0, b1);}
    return lng(min(u, m - u));}

// T: O(log(p)) for exists, else O(p^(1/4) * log(p) + z * log(p)^2) expected, z = nonresidue candidates, M: O(p^(1/4)); prime p, k >= 0, 0^0 = 1; some root or -1.
inline bool kthRootExists(lng a, lng k, lng p) {
    assert(k >= 0 && p >= 2);
    ulng x = ulng(modNorm(a, p)), q = ulng(p) - 1;
    if (!k) { return x == 1; }
    return !x || modPow64(x, q / gcd(ulng(k), q), ulng(p)) == 1;}
inline lng kthRootModPrime(lng a, lng k, lng p) {
    if (!kthRootExists(a, k, p)) { return -1; }
    ulng x = ulng(modNorm(a, p)), m = ulng(p);
    if (x < 2) { return lng(x); }
    ulng r = ulng(k) % (m - 1), g = gcd(r, m - 1), y = x;
    for (auto [q, e] : factorize(g)) { y = discrete_log_detail::rootPrimePower(y, q, e, m); }
    return lng(modPow64(y, ulng(modNorm(extendedGcd(lng(r / g), lng((m - 1) / g)).x, lng((m - 1) / g))), m));}

// T: O(n^(1/4) * log(n) + c * |f| * log(p)) expected, f = primes of n, c = candidates tried, M: O(log(n)); prime p, n >= 1, an element of order n, or -1 when n does not divide p - 1.
inline lng unitRootModPrime(lng n, lng p) {
    assert(n >= 1 && p >= 2);
    if ((p - 1) % n) { return -1; }
    auto f = factorize(ulng(n));
    for (ulng c = 1;; ++c) {
        ulng x = modPow64(c, ulng((p - 1) / n), ulng(p));
        if (discrete_log_detail::generates(x, ulng(p), ulng(n), f)) { return lng(x); }}}
