#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "01-mod_arithmetic.hpp"
#include "05-combinatorics.hpp"
#include "07-primality_factorization.hpp"
#include "09-modinverse.hpp"
#include "13-convolution.hpp"
#include "14-polynomial_algorithms.hpp"

namespace combinatorics_advanced_detail {
    // T: O(n * log(n)) egf, euler O(n), O(n * log(n)^2) fractionSum and evaluate, M: O(n); prime M::mod() > n where factorials are used.
    template<typename M>
    vector<M> egfTimesExp(const vector<M> &a, bool negate) {
        int n = int(a.size());
        if (n == 0) { return {}; }
        PrimeCombinatorics<M> pc(n - 1);
        vector<M> x(n), y(n);
        for (int i = 0; i < n; ++i) { x[i] = a[i] * pc.inverseFactorial(i); y[i] = negate && i % 2 ? -pc.inverseFactorial(i) : pc.inverseFactorial(i); }
        x = truncatedConvolution(x, y, n);
        for (int i = 0; i < n; ++i) { x[i] *= pc.factorial(i); }
        return x;}
    template<typename M>
    vector<M> euler(int n) {
        vector<M> e(n + 1, 0);
        e[0] = 1;
        for (lng k = 1; k * (3 * k - 1) / 2 <= n; ++k) {
            lng g = k * (3 * k - 1) / 2;
            e[g] += k % 2 ? -1 : 1;
            if (g + k <= n) { e[g + k] += k % 2 ? -1 : 1; }}
        return e;}
    template<typename M>
    pair<vector<M>, vector<M>> fractionSum(const vector<M> &d, int l, int r) {
        if (r - l == 1) { return {{d[l]}, {M(1), -M(l)}}; }
        int m = (l + r) / 2;
        auto [p1, q1] = fractionSum(d, l, m);
        auto [p2, q2] = fractionSum(d, m, r);
        return {polyAdd(polyMul(p1, q2), polyMul(p2, q1)), polyMul(q1, q2)};}
    template<typename M>
    vector<M> evaluate(const vector<M> &f, const vector<M> &xs) {
        int n = int(xs.size());
        vector<vector<M>> t(4 * max(n, 1));
        vector<M> res(n);
        auto build = [&](auto &&self, int k, int l, int r) -> void {
            if (r - l == 1) { t[k] = {-xs[l], M(1)}; return; }
            int m = (l + r) / 2;
            self(self, 2 * k, l, m); self(self, 2 * k + 1, m, r);
            t[k] = polyMul(t[2 * k], t[2 * k + 1]);};
        auto down = [&](auto &&self, int k, int l, int r, vector<M> g) -> void {
            g = polyDivmod(g, t[k]).second;
            if (r - l == 1) { res[l] = g.empty() ? M(0) : g[0]; return; }
            int m = (l + r) / 2;
            self(self, 2 * k, l, m, g); self(self, 2 * k + 1, m, r, g);};
        if (n) { build(build, 1, 0, n); down(down, 1, 0, n, f); }
        return res;}
} // namespace combinatorics_advanced_detail

// T: O(log_p(n)), M: O(1); p >= 2, 0 <= k <= n; v_p(n!) and the carries of k + (n - k) in base p, which equal v_p(C(n, k)).
inline ulng factorialValuation(ulng n, ulng p) {
    assert(p >= 2); ulng res = 0;
    while (n && p > 1) { n /= p; res += n; }
    return res;}
inline int kummerCarries(ulng n, ulng k, ulng p) {
    assert(p >= 2 && k <= n);
    int res = 0; ulng m = n - k;
    for (bool carry = false; p > 1 && (k || m); k /= p, m /= p) {
        carry = ulll(k % p) + m % p + carry >= p;
        res += carry;}
    return res;}

// T: O(log_p(n)), M: O(1); prime p = M::mod(), any 64-bit n, k with every base-p digit of n at most pc.n; Lucas' theorem.
template<typename M>
M binomialLucas(ulng n, ulng k, const PrimeCombinatorics<M> &pc) {
    pc.check();
    ulng p = M::mod(); M res = 1;
    for (; p > 1 && (n || k); n /= p, k /= p) {
        ulng a = n % p, b = k % p;
        if (b > a) { return 0; }
        assert(a <= ulng(pc.n)); res *= pc.combiNR(int(a), int(b));}
    return res;}

// S: O(N), Q: O(1), M: O(N); prime p = M::mod() of any size, 0 <= n <= N < INT_MAX; u = p-free part of i!, v = v_p(i!), 0 outside 0 <= k <= n.
template<typename M = mint>
struct BinomialSmallPrimeTable {
    vector<M> u, iu;
    vector<int> v;

    explicit BinomialSmallPrimeTable(int N = 0) {
        assert(0 <= N && N < std::numeric_limits<int>::max() && M::is_prime);
        ulng p = M::mod(); u.assign(N + 1, 1); iu.resize(N + 1); v.assign(N + 1, 0);
        auto strip = [&](ulng x, int &e) { for (e = 0; p > 1 && x % p == 0; x /= p) { ++e; } return x; };
        for (int i = 1, e; i <= N; ++i) { u[i] = u[i - 1] * M(strip(ulng(i), e)); v[i] = v[i - 1] + e; }
        iu[N] = inv(u[N]);
        for (int i = N, e; i > 0; --i) { iu[i - 1] = iu[i] * M(strip(ulng(i), e)); }}
    M operator()(int n, int k) const {
        assert(0 <= n && n < int(u.size()));
        if (k < 0 || k > n) { return 0; }
        return v[n] > v[k] + v[n - k] ? M(0) : u[n] * iu[k] * iu[n - k];}
};

// S: O(q), Q: O(log_p(n) + log(q)), M: O(q); prime power q = p^e < 2^31, any 64-bit n, k; Granville: p-free factorials with period q.
struct BinomialPrimePower {
    uint p, e, q;
    vector<uint> f;

    BinomialPrimePower(uint p, uint e) : p(p), e(e), q(1) {
        assert(p >= 2 && e >= 1 && isPrime(p));
        for (uint i = 0; i < e; ++i) { assert(q <= uint(std::numeric_limits<int>::max()) / p); q *= p; }
        f.assign(q, 1);
        for (uint i = 1; i < q; ++i) { f[i] = i % p ? uint(ulng(f[i - 1]) * i % q) : f[i - 1]; }}
    // The units of [1, q] multiply to f[q - 1] = +-1.
    ulng unitFactorial(ulng n) const {
        ulng res = 1;
        for (; n && p > 1; n /= p) { res = res * f[n % q] % q * ((n / q) % 2 ? f[q - 1] : 1) % q; }
        return res;}
    uint operator()(ulng n, ulng k) const {
        if (k > n) { return 0; }
        ulng c = factorialValuation(n, p) - factorialValuation(k, p) - factorialValuation(n - k, p);
        if (c >= e) { return 0; }
        ulng d = unitFactorial(k) * unitFactorial(n - k) % q;
        ulng res = unitFactorial(n) * ulng(modNorm(extendedGcd(lng(d), lng(q)).x, lng(q))) % q;
        for (; c; --c) { res = res * p % q; }
        return uint(res);}
};

// S: O(s + m^(1/4) * log(m)), Q: O(w * (log(n) + log(m))), M: O(s); 1 <= m < 2^31, s = sum and w = count of the prime powers of m; CRT over parts.
struct BinomialCompositeMod {
    uint m;
    vector<BinomialPrimePower> parts;
    vector<ulng> coef;

    explicit BinomialCompositeMod(uint m) : m(m) {
        assert(m >= 1 && m <= uint(std::numeric_limits<int>::max()));
        for (auto [p, e] : factorize(m)) {
            parts.emplace_back(uint(p), uint(e));
            ulng q = parts.back().q, r = m / q;
            coef.pb(r * ulng(modNorm(extendedGcd(lng(r % q), lng(q)).x, lng(q))) % m);}}
    uint operator()(ulng n, ulng k) const {
        ulng res = 0;
        for (int i = 0; i < int(parts.size()); ++i) { res = (res + parts[i](n, k) * coef[i]) % m; }
        return uint(res);}
};

// T: O(N * w + m^(1/4) * log(m)) expected, M: O(N); 0 <= N < INT_MAX, 1 <= m < 2^63, w = distinct primes of m up to N; C(N, k) mod m for k in [0, N].
inline vector<ulng> binomialRowCompositeMod(int N, ulng m) {
    assert(0 <= N && N < std::numeric_limits<int>::max() && m >= 1 && m >> 63 == 0);
    vector<ulng> ps, res(N + 1), unit(N + 1, 1), pre(N + 1, 1 % m);
    for (auto [p, e] : factorize(m)) { if (p <= ulng(N)) { ps.pb(p); } }
    int w = int(ps.size());
    vector<int> ex(w, 0);
    vector<vector<ulng>> pw(w, vector<ulng>(64, 1 % m));
    for (int j = 0; j < w; ++j) { for (int t = 1; t < 64; ++t) { pw[j][t] = modMul64(pw[j][t - 1], ps[j], m); } }
    // Strip the primes of m; sgn adds or removes their exponents.
    auto strip = [&](ulng x, int sgn) {
        for (int j = 0; j < w; ++j) { for (; x % ps[j] == 0; x /= ps[j]) { ex[j] += sgn; } }
        return x;};
    for (int i = 1; i <= N; ++i) { unit[i] = strip(ulng(i), 0); pre[i] = modMul64(pre[i - 1], unit[i] % m, m); }
    ulng iv = ulng(modNorm(extendedGcd(lng(pre[N] % m), lng(m)).x, lng(m))), cur = 1 % m;
    vector<ulng> inverse(N + 1, 0);
    for (int i = N; i >= 1; --i) { inverse[i] = modMul64(iv, pre[i - 1], m); iv = modMul64(iv, unit[i] % m, m); }
    res[0] = 1 % m;
    for (int k = 1; k <= N; ++k) {
        cur = modMul64(cur, strip(ulng(N - k + 1), 1) % m, m);
        strip(ulng(k), -1); cur = modMul64(cur, inverse[k], m);
        res[k] = cur;
        for (int j = 0; j < w; ++j) { res[k] = modMul64(res[k], pw[j][ex[j]], m); }}
    return res;}

// T: O(min(k, n - k)) for n <= pc.n, else O(k), M: O(1); prime M::mod(), any 64-bit n, k <= pc.n unless k >= n; sum over i <= k of r^i C(n, i).
template<typename M>
M binomialPrefixSum(ulng n, lng k, const PrimeCombinatorics<M> &pc, M r = 1) {
    pc.check();
    if (k < 0) { return 0; }
    if (ulng(k) >= n) { return pow(r + 1, n); }
    M res = 0, c = 1, rp = 1;
    if (n <= ulng(pc.n) && n - ulng(k) < ulng(k)) {
        rp = pow(r, k + 1);
        for (lng i = k + 1; ulng(i) <= n; ++i, rp *= r) { res += rp * pc.combiNR(int(n), int(i)); }
        return pow(r + 1, n) - res;}
    assert(k <= pc.n);
    for (int i = 0; i <= k; ++i, rp *= r) {
        res += rp * c;
        if (i < k) { c *= M(n - ulng(i)) * pc.inverseFactorial(i + 1) * pc.factorial(i); }}
    return res;}

// T: O(N * sqrt(q) + q * log(q)), M: O(N + q); N = max n <= pc.n, prime M::mod(), r + 1 a unit; Mo's order over (n, k) with S(n + 1, k) = (1 + r) S(n, k) - r^(k + 1) C(n, k).
template<typename M>
vector<M> binomialPrefixSumQueries(const vector<pair<int, int>> &qs, const PrimeCombinatorics<M> &pc, M r = 1) {
    pc.check();
    int q = int(qs.size()), N = 0;
    for (auto [n, k] : qs) { assert(0 <= n && n <= pc.n); N = max(N, n); }
    vector<M> pw(N + 2, 1), res(q);
    for (int i = 1; i <= N + 1; ++i) { pw[i] = pw[i - 1] * r; }
    M ir = inv(r + 1), s = 1;
    int b = max(1, int(N / std::sqrt(max(q, 1)))), n = 0, k = 0;
    vector<int> ord(q);
    iota(ord.begin(), ord.end(), 0);
    auto key = [&](int i) { int bl = qs[i].first / b; return pair<int, int>(bl, bl % 2 ? -qs[i].second : qs[i].second); };
    sort(ord.begin(), ord.end(), [&](int i, int j) { return key(i) < key(j); });
    for (int i : ord) {
        int tn = qs[i].first, tk = min(max(qs[i].second, -1), tn);
        while (n < tn) { s = (r + 1) * s - pw[k + 1] * pc.combiNR(n, k); ++n; }
        while (n > tn) { --n; s = (s + pw[k + 1] * pc.combiNR(n, k)) * ir; }
        while (k < tk) { ++k; s += pw[k] * pc.combiNR(n, k); }
        while (k > tk) { s -= pw[k] * pc.combiNR(n, k); --k; }
        res[i] = s;}
    return res;}

// S: O(N^2 / B), Q: O(B), M: O(N^2 / B); prime M::mod() > N, block B (default sqrt(N)); sum over i <= k of C(n, i) for 0 <= n <= N, any k.
template<typename M = mint>
struct BinomialPrefixSumTable {
    int b;
    PrimeCombinatorics<M> pc;
    vector<vector<M>> col;

    explicit BinomialPrefixSumTable(int N = 0, int B = 0) : b(B > 0 ? B : max(1, int(std::sqrt(N)))), pc(N) {
        for (int K = b; K <= N; K += b) {
            vector<M> c(N + 1, 1);
            for (int n = 0; n < N; ++n) { c[n + 1] = c[n] + c[n] - pc.combiNR(n, K); }
            col.pb(std::move(c));}}
    M sum(int n, lng k) const {
        assert(0 <= n && n <= pc.n);
        if (k < 0) { return 0; }
        int t = int(min<lng>(k, n)), j = t / b;
        M res = j ? col[j - 1][n] : M(0);
        for (int i = j ? j * b + 1 : 0; i <= t; ++i) { res += pc.combiNR(n, i); }
        return res;}
};

// T: O(n), M: O(1); any commutative ring T, n >= 0; x (x + 1) ... (x + n - 1) and x (x - 1) ... (x - n + 1), both 1 for n = 0.
template<typename T>
T risingFactorial(T x, lng n) {
    assert(n >= 0); T res = T(1);
    for (lng i = 0; i < n; ++i) { res *= x + T(i); }
    return res;}
template<typename T>
T fallingFactorial(T x, lng n) {
    assert(n >= 0); T res = T(1);
    for (lng i = 0; i < n; ++i) { res *= x - T(i); }
    return res;}

// T: O(n * log(n)), M: O(n); prime M::mod() > n (stirling1Row falls back to O(n^2) Taylor shifts otherwise); signed s(n, k) and S(n, k) for k in [0, n].
template<typename M = mint>
vector<M> stirling1Row(int n) {
    assert(n >= 0);
    vector<M> f{1};
    // x^(2m falling) = x^(m falling) (x - m)^(m falling), plus one linear factor per set bit.
    for (int bit = int(std::bit_width(uint(n))), m = 0; bit-- > 0;) {
        f = polyMul(f, taylorShift(f, M(-m))); m *= 2;
        if ((n >> bit) & 1) { f = polyMul(f, vector<M>{M(-m), M(1)}); ++m; }}
    return f;}
template<typename M = mint>
vector<M> stirling2Row(int n) {
    assert(n >= 0);
    PrimeCombinatorics<M> pc(n);
    auto pw = powerTable<M>(n, ulng(n));
    vector<M> a(n + 1), b(n + 1);
    for (int i = 0; i <= n; ++i) { a[i] = pw[i] * pc.inverseFactorial(i); b[i] = i % 2 ? -pc.inverseFactorial(i) : pc.inverseFactorial(i); }
    return truncatedConvolution(a, b, n + 1);}

// T: O(n * log(n)), M: O(n); prime M::mod() > n, any k >= 0; s(i, k) and S(i, k) for i in [0, n] from (log(1 + x))^k / k! and (e^x - 1)^k / k!.
template<typename M = mint>
vector<M> stirling1Column(int n, int k) {
    assert(n >= 0 && k >= 0);
    vector<M> res(n + 1, 0);
    if (k == 0 || k > n) { res[0] = k == 0; return res; }
    PrimeCombinatorics<M> pc(n);
    vector<M> h(n - k + 1);
    for (int i = 0; i <= n - k; ++i) { h[i] = (i % 2 ? -pc.inverseFactorial(i + 1) : pc.inverseFactorial(i + 1)) * pc.factorial(i); }
    h = fpsPow(h, k, n - k + 1);
    for (int i = k; i <= n; ++i) { res[i] = h[i - k] * pc.factorial(i) * pc.inverseFactorial(k); }
    return res;}
template<typename M = mint>
vector<M> stirling2Column(int n, int k) {
    assert(n >= 0 && k >= 0);
    vector<M> res(n + 1, 0);
    if (k == 0 || k > n) { res[0] = k == 0; return res; }
    PrimeCombinatorics<M> pc(n);
    vector<M> g(n - k + 1);
    for (int i = 0; i <= n - k; ++i) { g[i] = pc.inverseFactorial(i + 1); }
    g = fpsPow(g, k, n - k + 1);
    for (int i = k; i <= n; ++i) { res[i] = g[i - k] * pc.factorial(i) * pc.inverseFactorial(k); }
    return res;}

// S: O(p^2), Q: O(log(n)), M: O(p^2); prime p < 2^16, any 64-bit n, k; signed s(n, k) and S(n, k) mod p, 0 for k > n.
struct StirlingSmallPrime {
    uint p;
    vector<uint16_t> s1, c2;
    vector<uint> fac, ifac;

    explicit StirlingSmallPrime(uint p) : p(p), s1(ulng(p) * (p + 1) / 2), c2(s1.size()), fac(p, 1), ifac(p, 1) {
        assert(2 <= p && p < 65536 && isPrime(p));
        for (uint i = 1; i < p; ++i) { fac[i] = fac[i - 1] * i % p; }
        ifac[p - 1] = p - 1;
        for (uint i = p - 1; i > 1; --i) { ifac[i - 1] = ifac[i] * i % p; }
        s1[0] = 1;
        for (uint b = 1; b < p; ++b) {
            for (uint j = 1; j <= b; ++j) { s1[at1(b, j)] = uint16_t((s1[at1(b - 1, j - 1)] + (p - b + 1) % p * (j < b ? s1[at1(b - 1, j)] : 0u)) % p); }}
        // Column b of S has generating function x^(a p) c_b(x) / (1 - x^(p - 1))^(a + 1), c_b = x^b prod_(i > b) (1 - i x).
        c2[at2(p - 1, p - 1)] = 1;
        for (uint b = p - 1; b-- > 0;) {
            for (uint j = b; j < p; ++j) {
                uint hi = j + 1 < p ? c2[at2(b + 1, j + 1)] : 0, lo = j > b ? c2[at2(b + 1, j)] : 0;
                c2[at2(b, j)] = uint16_t((hi + (p - (b + 1)) * lo) % p);}}}
    ulng at1(uint b, uint j) const { return ulng(b) * (b + 1) / 2 + j; }
    ulng at2(uint b, uint j) const { return at1(p - 1 - b, j - b); }
    uint binom(ulng n, ulng k) const {
        uint res = 1;
        for (; n || k; n /= p, k /= p) {
            uint a = uint(n % p), b = uint(k % p);
            if (b > a) { return 0; }
            res = res * fac[a] % p * ifac[b] % p * ifac[a - b] % p;}
        return res;}
    uint stirling1(ulng n, ulng k) const {
        if (k > n || k < n / p) { return 0; }
        ulng a = n / p, t = k - a, res = 0;
        uint b = uint(n % p);
        // x^(n falling) = (x^p - x)^a x^(b falling) mod p.
        for (ulng j = t % (p - 1); j <= b && j <= t; j += p - 1) {
            ulng i = (t - j) / (p - 1);
            if (i <= a) { res += ulng(s1[at1(b, uint(j))]) * binom(a, i) % p * ((a - i) % 2 ? p - 1 : 1); }}
        return uint(res % p);}
    uint stirling2(ulng n, ulng k) const {
        if (k > n) { return 0; }
        ulng a = k / p, t = n - a * p, res = 0;
        uint b = uint(k % p);
        for (ulng j = b + (t % (p - 1) + (p - 1) - b % (p - 1)) % (p - 1); j < p && j <= t; j += p - 1) {
            res += ulng(c2[at2(b, uint(j))]) * binom(a + (t - j) / (p - 1), (t - j) / (p - 1)) % p;}
        return uint(res % p);}
};

// T: O(n * log(n)) table, O(p * log(p) * log(n)) large, M: O(n), O(p); prime M::mod() > n for the table, any 64-bit n for B_n mod a prime p = M::mod() < 2^31.
template<typename M = mint>
vector<M> bellTable(int n) {
    assert(n >= 0);
    PrimeCombinatorics<M> pc(n);
    vector<M> f(n + 1, 0);
    for (int i = 1; i <= n; ++i) { f[i] = pc.inverseFactorial(i); }
    f = fpsExp(f, n + 1);
    for (int i = 0; i <= n; ++i) { f[i] *= pc.factorial(i); }
    return f;}
template<typename M = mint>
M bellNumberLarge(ulng n) {
    assert(M::is_prime && M::mod() < ulng(std::numeric_limits<int>::max()));
    int p = int(M::mod());
    auto b = bellTable<M>(p - 1);
    if (n < ulng(p)) { return b[n]; }
    // Touchard: B_(m + p) = B_m + B_(m + 1), so B_n = sum r_i B_i for x^n = sum r_i x^i mod x^p - x - 1.
    auto reduce = [&](vector<M> a) {
        for (int d = int(a.size()) - 1; d >= p; --d) { a[d - p + 1] += a[d]; a[d - p] += a[d]; }
        a.resize(p); return a;};
    vector<M> r{1};
    for (int bit = int(std::bit_width(n)); bit-- > 0;) {
        r = reduce(convolution(r, r));
        if ((n >> bit) & 1) { r.insert(r.begin(), M(0)); r = reduce(r); }}
    M res = 0;
    for (int i = 0; i < p; ++i) { res += r[i] * b[i]; }
    return res;}

// T: O(n * log(n)) row, O(k + (k / log(k)) * log(n)) number, M: O(n), O(k); prime M::mod() > n + 1 (row), > k + 1 (number, any 64-bit n); A(n, k) for k < n, A(0, 0) = 1.
template<typename M = mint>
vector<M> eulerianRow(int n) {
    assert(n >= 0);
    if (n == 0) { return {M(1)}; }
    PrimeCombinatorics<M> pc(n + 1);
    vector<M> a(n + 1), pw = powerTable<M>(n, ulng(n));
    for (int j = 0; j <= n; ++j) { a[j] = j % 2 ? -pc.combiNR(n + 1, j) : pc.combiNR(n + 1, j); }
    auto c = truncatedConvolution(a, pw, n + 1);
    return vector<M>(c.begin() + 1, c.end());}
template<typename M = mint>
M eulerianNumber(ulng n, lng k) {
    if (n == 0) { return M(k == 0); }
    if (k < 0 || ulng(k) >= n) { return 0; }
    k = lng(min(ulng(k), n - 1 - ulng(k)));
    int t = int(k);
    auto pw = powerTable<M>(t + 1, n);
    auto iv = inverseTable<M>(t + 1);
    M c = 1, res = 0;
    for (int j = 0; j <= t; ++j) {
        res += (j % 2 ? -c : c) * pw[t + 1 - j];
        c *= (M(n) + 1 - M(j)) * iv[j + 1];}
    return res;}

// T: O(n^2), M: O(n^2) entringer, O(1) lah; any ring T, E(i, j) for 0 <= j <= i <= n; unsigned L(n, k) from pc, 0 outside 1 <= k <= n (L(0, 0) = 1).
template<typename T>
vector<vector<T>> entringerTable(int n) {
    assert(n >= 0); vector<vector<T>> e(max(n, 0) + 1);
    e[0] = {T(1)};
    for (int i = 1; i <= n; ++i) {
        e[i].assign(i + 1, T(0));
        for (int j = 1; j <= i; ++j) { e[i][j] = e[i][j - 1] + e[i - 1][i - j]; }}
    return e;}
template<typename M>
M lahNumber(int n, int k, const PrimeCombinatorics<M> &pc) {
    assert(n >= 0);
    if (n == 0 || k <= 0 || k > n) { return M(n == 0 && k == 0); }
    return pc.combiNR(n - 1, k - 1) * pc.factorial(n) * pc.inverseFactorial(k);}

// T: O(n * sqrt(n)) table, O(n * log(n)) others, M: O(n); table any ring T, count and distinct any modint, bounded prime M::mod() > n; p(i), parts <= k, distinct parts, i in [0, n].
template<typename T>
vector<T> partitionTable(int n) {
    assert(n >= 0);
    vector<T> res(max(n, 0) + 1, T(0));
    res[0] = T(1);
    for (int i = 1; i <= n; ++i) {
        for (lng k = 1; k * (3 * k - 1) / 2 <= i; ++k) {
            lng g = k * (3 * k - 1) / 2;
            T t = res[i - g];
            if (g + k <= i) { t += res[i - g - k]; }
            res[i] += k % 2 ? t : -t;}}
    return res;}
template<typename M = mint>
vector<M> partitionCount(int n) {
    assert(n >= 0);
    return fpsInverse(combinatorics_advanced_detail::euler<M>(n), n + 1);}
template<typename M = mint>
vector<M> partitionsBoundedParts(int n, int k) {
    assert(n >= 0 && k >= 0);
    auto iv = inverseTable<M>(n);
    vector<M> lg(n + 1, 0);
    for (int j = 1; j <= min(k, n); ++j) { for (int m = 1; m <= n / j; ++m) { lg[j * m] += iv[m]; } }
    return fpsExp(lg, n + 1);}
template<typename M = mint>
vector<M> partitionsDistinct(int n) {
    assert(n >= 0);
    auto e = combinatorics_advanced_detail::euler<M>(n);
    vector<M> e2(n + 1, 0);
    for (int i = 0; 2 * i <= n; ++i) { e2[2 * i] = e[i]; }
    return truncatedConvolution(fpsInverse(e, n + 1), e2, n + 1);}

// T: O(p(n)) amortized plus the callbacks, M: O(n); n >= 0, f(span) once per partition, nonincreasing parts, reverse lexicographic from {n}; n = 0 gives one empty span.
template<typename F>
void enumeratePartitions(int n, F f) {
    assert(n >= 0);
    vector<int> x(max(n, 1), 1);
    x[0] = n;
    int m = n > 0, h = n > 1 ? 0 : -1;
    f(std::span<const int>(x.data(), m));
    // Zoghbi-Stojmenovic ZS1: parts after h are 1; split x[h] - 1 copies greedily.
    while (n > 1 && x[0] != 1) {
        if (x[h] == 2) { x[h--] = 1; ++m; }
        else {
            int r = x[h] - 1, t = m - h;
            x[h] = r;
            while (t >= r) { x[++h] = r; t -= r; }
            if (t == 0) { m = h + 1; }
            else {
                m = h + 2;
                if (t > 1) { x[++h] = t; }}}
        f(std::span<const int>(x.data(), m));}}

// T: O(k + (k / log(k)) * log(n)) surjections, O(min(k, n / (c + 1))) bounded, M: O(k), O(1); pc covers k and n + k - 1; onto maps [n] -> [k], k-tuples in [0, c] with sum n.
template<typename M>
M surjectionCount(ulng n, int k, const PrimeCombinatorics<M> &pc) {
    assert(0 <= k && k <= pc.n);
    auto pw = powerTable<M>(k, n);
    M res = 0;
    for (int i = 0; i <= k; ++i) { res += (k - i) % 2 ? -pc.combiNR(k, i) * pw[i] : pc.combiNR(k, i) * pw[i]; }
    return res;}
template<typename M>
M boundedStarsBars(int n, int k, int c, const PrimeCombinatorics<M> &pc) {
    assert(n >= 0 && k >= 0 && c >= 0);
    if (k == 0) { return M(n == 0); }
    M res = 0;
    for (lng j = 0; j <= k && j * (lng(c) + 1) <= n; ++j) {
        M t = pc.combiNR(k, int(j)) * pc.combiNR(int(n - j * (lng(c) + 1) + k - 1), k - 1);
        res += j % 2 ? -t : t;}
    return res;}

// S: O(N + log(mod)), Q: O(1), M: O(N); prime M::mod() > N, any q; [n]_q! and [n, k]_q for 0 <= n <= N, q-Lucas at d = ord(q) when [d]_q = 0 with d <= N.
template<typename M = mint>
struct QCombinatorics {
    int n, d;
    vector<M> f, ifac;
    PrimeCombinatorics<M> pc;

    QCombinatorics(M q = 0, int N = 0) { reset(q, N); }
    void reset(M q, int N) {
        assert(0 <= N && N < std::numeric_limits<int>::max() && ulng(N) < M::mod() && M::is_prime);
        n = N; f = {M(1)};
        vector<M> s{M(0)};
        for (M qi = 1; int(f.size()) <= N; qi *= q) {
            s.pb(s.back() + qi);
            if (s.back() == 0) { break; }
            f.pb(f.back() * s.back());}
        d = int(f.size()); ifac.resize(d);
        ifac[d - 1] = inv(f[d - 1]);
        for (int i = d - 1; i > 0; --i) { ifac[i - 1] = ifac[i] * s[i]; }
        pc.reset(d <= N ? N / d : 0);}
    M qFactorial(int k) const { assert(0 <= k && k <= n); return k < d ? f[k] : M(0); }
    M qBinomial(int a, int b) const {
        assert(0 <= a && a <= n);
        if (b < 0 || b > a) { return 0; }
        int a0 = a % d, b0 = b % d;
        if (b0 > a0) { return 0; }
        return pc.combiNR(a / d, b / d) * f[a0] * ifac[b0] * ifac[a0 - b0];}
};

// T: O(n * log(n)) binomial, O(n * log(n)^2) Stirling, M: O(n); prime M::mod() > n = |a| - 1; b_i = sum_j C(i, j) a_j and sum_j S(i, j) a_j, the inverses use (-1)^(i-j) C(i, j) and s(i, j).
template<typename M = mint>
vector<M> binomialTransform(const vector<M> &a) { return combinatorics_advanced_detail::egfTimesExp(a, false); }
template<typename M = mint>
vector<M> inverseBinomialTransform(const vector<M> &b) { return combinatorics_advanced_detail::egfTimesExp(b, true); }
template<typename M = mint>
vector<M> stirlingTransform(const vector<M> &a) {
    int n = int(a.size()) - 1;
    if (n <= 0) { return a; }
    PrimeCombinatorics<M> pc(n);
    // b_i = sum_j d_j j^i with j! d_j = sum_k a_k (-1)^(k-j) / (k - j)!, read off sum_j d_j / (1 - j x).
    vector<M> ar(a.rbegin(), a.rend()), u(n + 1), d(n + 1);
    for (int t = 0; t <= n; ++t) { u[t] = t % 2 ? -pc.inverseFactorial(t) : pc.inverseFactorial(t); }
    auto e = truncatedConvolution(ar, u, n + 1);
    for (int j = 0; j <= n; ++j) { d[j] = e[n - j] * pc.inverseFactorial(j); }
    auto [num, den] = combinatorics_advanced_detail::fractionSum(d, 0, n + 1);
    return fpsDiv(num, den, n + 1);}
template<typename M = mint>
vector<M> inverseStirlingTransform(const vector<M> &b) {
    int n = int(b.size()) - 1;
    if (n <= 0) { return b; }
    PrimeCombinatorics<M> pc(n);
    // Solve b_i = sum_j d_j j^i: with Q = prod (1 - j x), P = B Q mod x^(n + 1), d_j = rev(P)(j) (-1)^(n - j) / (j! (n - j)!).
    vector<vector<M>> lin;
    vector<M> xs(n);
    for (int j = 1; j <= n; ++j) { lin.pb({M(1), -M(j)}); xs[j - 1] = j; }
    auto num = truncatedConvolution(b, productTree(lin), n + 1);
    reverse(num.begin(), num.end());
    auto val = combinatorics_advanced_detail::evaluate(num, xs);
    vector<M> e(n + 1), v(n + 1);
    e[0] = b[0];
    for (int j = 1; j <= n; ++j) {
        M dj = val[j - 1] * pc.inverseFactorial(j) * pc.inverseFactorial(n - j);
        if ((n - j) % 2) { dj = -dj; }
        e[0] -= dj; e[j] = dj * pc.factorial(j);}
    for (int t = 0; t <= n; ++t) { v[t] = pc.inverseFactorial(t); }
    reverse(e.begin(), e.end());
    auto res = truncatedConvolution(e, v, n + 1);
    reverse(res.begin(), res.end());
    return res;}

// T: O(s * c), M: O(n) recursion; s = visited subsets, c = cost of extend and f; sum over subsets S of [0, n) of (-1)^|S| f(fold(S)), extend(acc, i, next) false only when f vanishes on every superset.
template<typename R, typename T, typename E, typename F>
R inclusionExclusion(int n, const T &init, E extend, F f) {
    R res = R(0);
    auto dfs = [&](auto &&self, int i, const T &acc, bool neg) -> void {
        R v = f(acc);
        res = neg ? res - v : res + v;
        T next;
        for (int j = i; j < n; ++j) { if (extend(acc, j, next)) { self(self, j + 1, next, !neg); } }};
    dfs(dfs, 0, init, false);
    return res;}

// T: O(t * log(t) + |s|), M: O(t); prime M::mod() > t, every s_i >= 0; the number of subsets of s with each sum in [0, t], from exp(sum log(1 + x^s_i)).
template<typename M = mint>
vector<M> subsetSumCount(const vector<int> &s, int t) {
    assert(t >= 0);
    vector<int> cnt(t + 1, 0);
    int zeros = 0;
    for (int x : s) {
        assert(x >= 0);
        if (x == 0) { ++zeros; }
        else if (0 < x && x <= t) { ++cnt[x]; }}
    auto iv = inverseTable<M>(t);
    vector<M> lg(t + 1, 0);
    for (int x = 1; x <= t; ++x) {
        if (!cnt[x]) { continue; }
        for (int k = 1; k <= t / x; ++k) { lg[x * k] += k % 2 ? M(cnt[x]) * iv[k] : -M(cnt[x]) * iv[k]; }}
    auto res = fpsExp(lg, t + 1);
    M z = pow(M(2), zeros);
    for (auto &x : res) { x *= z; }
    return res;}

// T: O(m^(1/4) * log(m)^2) expected, M: O(log(m)); 1 <= m <= 3 * 10^18 (pi(m) <= 6 m < 2^64); least period of the Fibonacci numbers modulo m.
inline ulng pisanoPeriod(ulng m) {
    assert(m >= 1 && m <= 3000000000000000000ULL);
    auto isPeriod = [](ulng n, ulng mod) {
        ulng a = 0, b = 1 % mod;
        for (int i = int(std::bit_width(n)); i-- > 0;) {
            ulng c = modMul64(a, (2 * b + mod - a) % mod, mod), d = (modMul64(a, a, mod) + modMul64(b, b, mod)) % mod;
            if ((n >> i) & 1) { a = d; b = (c + d) % mod; }
            else { a = c; b = d; }}
        return a == 0 && b == 1 % mod;};
    ulng res = 1;
    for (auto [p, e] : factorize(m)) {
        // pi(p) divides p - 1 or 2 (p + 1) by Legendre's symbol (5 / p); pi(p^e) = pi(p) p^j for the least such period.
        ulng l = p == 2 ? 3 : p == 5 ? 20 : p % 5 == 1 || p % 5 == 4 ? p - 1 : 2 * (p + 1), pe = 1;
        for (auto [q, f] : factorize(l)) { while (l % q == 0 && isPeriod(l / q, p)) { l /= q; } }
        for (int i = 0; i < e; ++i) { pe *= p; }
        for (int i = 1; i < e && !isPeriod(l, pe); ++i) { l *= p; }
        res = lcm(res, l);}
    return res;}

// T: O(log(n)) zeckendorf, O(n * log(n)) indecomposable, M: O(log(n)), O(n); any 64-bit n, descending indices k >= 2 of F_k (F_1 = F_2 = 1), none consecutive; any modint, sum c_i x^i = 1 - 1 / sum i! x^i.
inline vector<int> zeckendorf(ulng n) {
    array<ulng, 94> f{0, 1};
    for (int i = 2; i < 94; ++i) { f[i] = f[i - 1] + f[i - 2]; }
    vector<int> res;
    for (int k = 93; k >= 2 && n; --k) {
        if (f[k] <= n) { res.pb(k); n -= f[k]; --k; }}
    return res;}
template<typename M = mint>
vector<M> indecomposablePermutations(int n) {
    assert(n >= 0);
    vector<M> f(n + 1, 1);
    for (int i = 1; i <= n; ++i) { f[i] = f[i - 1] * i; }
    f = fpsInverse(f, n + 1);
    for (auto &x : f) { x = -x; }
    f[0] += 1;
    return f;}
