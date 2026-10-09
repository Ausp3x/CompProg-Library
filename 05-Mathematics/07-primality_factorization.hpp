#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/04-montgomery.hpp"
#include "04-sieve_algorithms.hpp"

namespace primality_detail {
    inline constexpr ulng SMALL[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61};
    // T: O(1), M: O(1); splitmix64 step, x + y mod n for x, y < n, products of Montgomery forms or plain residues (Compact).
    inline ulng splitmix(ulng &s) {
        ulng z = (s += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL; z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);}
    inline ulng addMod(ulng x, ulng y, ulng n) {
        ulng s = x + y; return s - ((s < x || s >= n) ? n : 0);}
    inline auto montMul(const Montgomery64 &mg) { return [&mg](ulng x, ulng y) { return mg.redc(ulll(x) * y, false); }; }
    inline auto montForm(const Montgomery64 &mg) { return [&mg](ulng a) { return mg.init(a); }; }
    inline auto plainMul(ulng n) { return [n](ulng x, ulng y) { return ulng(ulll(x) * y % n); }; }
    // T: O(k), M: O(1); floor(n^(1/k)) for k >= 1.
    inline ulng kthRoot(ulng n, int k) {
        if (k == 1 || n < 2) { return n; }
        auto above = [&](ulng x) {
            ulng p = 1;
            for (int i = 0; i < k; ++i) { if (__builtin_mul_overflow(p, x, &p) || p > n) { return true; } }
            return false;};
        ulng r = ulng(std::pow(double(n), 1.0 / k));
        while (above(r)) { --r; }
        while (!above(r + 1)) { ++r; }
        return r;}
    // T: O(log(n)) products (7 bases above 2^32, 3 below), M: O(1); odd n >= 5, mul and form share one residue representation.
    template<typename Mul, typename Form>
    bool strongProbablePrime(ulng n, Mul mul, Form form) {
        int s = std::countr_zero(n - 1);
        ulng d = (n - 1) >> s, one = form(1), neg = form(n - 1);
        auto witness = [&](ulng a) {
            a %= n;
            if (a == 0) { return false; }
            ulng x = one, b = form(a);
            for (ulng e = d; e; e >>= 1) {
                if (e & 1) { x = mul(x, b); }
                b = mul(b, b);}
            if (x == one || x == neg) { return false; }
            for (int i = 1; i < s; ++i) {
                x = mul(x, x);
                if (x == neg) { return false; }}
            return true;};
        if (n < (1ULL << 32)) {
            for (ulng a : {2ULL, 7ULL, 61ULL}) { if (witness(a)) { return false; } }
            return true;}
        for (ulng a : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) { if (witness(a)) { return false; } }
        return true;}
    // T: O(1) plus test(n), M: O(1); trial division by SMALL decides n < 67^2.
    template<typename Test>
    bool isPrimeWith(ulng n, Test test) {
        for (ulng p : SMALL) { if (n % p == 0) { return n == p; } }
        return n >= 67 && (n < 67 * 67 || test(n));}
    // T: O(budget * n^(1/4)) expected products, M: O(1); odd composite n, mul and form share one representation; proper divisor or 0.
    template<typename Mul, typename Form>
    ulng rho(ulng n, ulng &seed, int budget, Mul mul, Form form) {
        constexpr ulng BATCH = 128;
        for (int attempt = 0; attempt < budget; ++attempt) {
            ulng c = form(splitmix(seed) % (n - 1) + 1), y = form(splitmix(seed) % n), x = y, ys = y, q = form(1), g = 1;
            auto f = [&](ulng v) { return addMod(mul(v, v), c, n); };
            for (ulng r = 1; g == 1; r <<= 1) {
                x = y;
                for (ulng i = 0; i < r; ++i) { y = f(y); }
                for (ulng k = 0; k < r && g == 1; k += BATCH) {
                    ys = y;
                    for (ulng i = 0; i < min(BATCH, r - k); ++i) { y = f(y); q = mul(q, x > y ? x - y : y - x); }
                    g = gcd(q, n);}}
            if (g == n) {
                do { ys = f(ys); g = gcd(x > ys ? x - ys : ys - x, n); } while (g == 1);}
            if (g != n) { return g; }}
        return 0;}
    // T: O(n^(1/4) * log(n)) expected, M: O(log(n)); n >= 1, prime(m) decides cofactors, split(m, seed) splits odd composites.
    template<typename Prime, typename Split>
    vector<pair<ulng, int>> factorizeWith(ulng n, Prime prime, Split split) {
        assert(n >= 1);
        vector<ulng> ps, todo;
        for (ulng p : SMALL) { while (n > 1 && n % p == 0) { ps.pb(p); n /= p; } }
        ulng seed = 0;
        if (n > 1) { todo.pb(n); }
        while (!todo.empty()) {
            ulng m = todo.back(); todo.pop_back();
            if (prime(m)) { ps.pb(m); continue; }
            ulng d = split(m, seed);
            todo.pb(d); todo.pb(m / d);}
        sort(ps.begin(), ps.end());
        vector<pair<ulng, int>> res;
        for (ulng p : ps) {
            if (res.empty() || res.back().fi != p) { res.pb({p, 0}); }
            ++res.back().se;}
        return res;}
} // namespace primality_detail

// T: O(sqrt(n)), M: O(1); all ulng, deterministic trial division.
inline bool isPrimeTrial(ulng n) {
    if (n < 4) { return n >= 2; }
    if (n % 2 == 0 || n % 3 == 0) { return false; }
    for (ulng i = 5; i <= n / i; i += 6) { if (n % i == 0 || n % (i + 2) == 0) { return false; } }
    return true;}

// T: O(log(n)) products, M: O(1); all ulng, deterministic; Compact twins use 128-bit remainders and no Core reduction.
inline bool millerRabin64(ulng n) {
    if (n < 4 || n % 2 == 0) { return n == 2 || n == 3; }
    Montgomery64 mg(n);
    return primality_detail::strongProbablePrime(n, primality_detail::montMul(mg), primality_detail::montForm(mg));}
inline bool millerRabin64Compact(ulng n) {
    if (n < 4 || n % 2 == 0) { return n == 2 || n == 3; }
    return primality_detail::strongProbablePrime(n, primality_detail::plainMul(n), [](ulng a) { return a; });}
inline bool isPrime(ulng n) { return primality_detail::isPrimeWith(n, millerRabin64); }
inline bool isPrimeCompact(ulng n) { return primality_detail::isPrimeWith(n, millerRabin64Compact); }

// T: O(1) when n <= s.n, else O(log(n)), M: O(1); table lookup with isPrime fallback.
inline bool isPrimeSpf(ulng n, const LinearSieve &s) { return n <= ulng(s.n) ? s.spf[n] == lng(n) : isPrime(n); }

// T: O(budget * n^(1/4)) expected, M: O(1); seeded Las Vegas, 0 when n < 4, n prime or budget attempts fail.
inline ulng pollardRhoBrent(ulng n, ulng seed = 0, int budget = 64) {
    if (n < 4 || isPrime(n)) { return 0; }
    if (n % 2 == 0) { return 2; }
    Montgomery64 mg(n);
    return primality_detail::rho(n, seed, budget, primality_detail::montMul(mg), primality_detail::montForm(mg));}
inline ulng pollardRhoBrentCompact(ulng n, ulng seed = 0, int budget = 64) {
    if (n < 4 || isPrimeCompact(n)) { return 0; }
    if (n % 2 == 0) { return 2; }
    return primality_detail::rho(n, seed, budget, primality_detail::plainMul(n), [](ulng a) { return a; });}

// T: O(n^(1/4) * log(n)) expected, M: O(log(n)); n >= 1, ascending (prime, exponent) or primes with multiplicity, exact d(n) and sigma(n).
inline vector<pair<ulng, int>> factorize(ulng n) {
    return primality_detail::factorizeWith(n, isPrime, [](ulng m, ulng &seed) {
        Montgomery64 mg(m);
        return primality_detail::rho(m, seed, std::numeric_limits<int>::max(), primality_detail::montMul(mg), primality_detail::montForm(mg));});}
inline vector<pair<ulng, int>> factorizeCompact(ulng n) {
    return primality_detail::factorizeWith(n, isPrimeCompact, [](ulng m, ulng &seed) {
        return primality_detail::rho(m, seed, std::numeric_limits<int>::max(), primality_detail::plainMul(m), [](ulng a) { return a; });});}
inline vector<ulng> primeFactors(ulng n) {
    vector<ulng> res;
    for (auto [p, e] : factorize(n)) { res.insert(res.end(), e, p); }
    return res;}
inline lng divisorCount(ulng n) {
    lng res = 1;
    for (auto [p, e] : factorize(n)) { res *= e + 1; }
    return res;}
inline ulll divisorSum(ulng n) {
    ulll res = 1;
    for (auto [p, e] : factorize(n)) {
        ulll s = 1, pk = 1;
        for (int k = 1; k <= e; ++k) { pk *= p; s += pk; }
        res *= s;}
    return res;}
inline bool isSquarefree(ulng n) {
    for (auto [p, e] : factorize(n)) { if (e > 1) { return false; } }
    return true;}
inline ulng radical(ulng n) {
    ulng res = 1;
    for (auto [p, e] : factorize(n)) { res *= p; }
    return res;}

// T: O(sqrt(n)), M: O(log(n)); n >= 1, deterministic, same output as factorize.
inline vector<pair<ulng, int>> factorizeTrial(ulng n) {
    assert(n >= 1);
    vector<pair<ulng, int>> res;
    auto take = [&](ulng p) {
        if (n < 2 || n % p) { return; }
        res.pb({p, 0});
        while (n % p == 0) { n /= p; ++res.back().se; }};
    take(2); take(3);
    for (ulng i = 5; i <= n / i; i += 6) { take(i); take(i + 2); }
    if (n > 1) { res.pb({n, 1}); }
    return res;}

// T: O(out * log(out)) plus factorization, M: O(out); ascending positive divisors.
inline vector<ulng> divisors(const vector<pair<ulng, int>> &f) {
    vector<ulng> res{1};
    for (auto [p, e] : f) {
        int len = int(res.size());
        ulng pk = 1;
        for (int k = 1; k <= e; ++k) {
            pk *= p;
            for (int i = 0; i < len; ++i) { res.pb(res[i] * pk); }}}
    sort(res.begin(), res.end());
    return res;}
inline vector<ulng> divisors(ulng n) { return divisors(factorize(n)); }

// T: O(log(n)^2), M: O(1); all ulng, returns p when n = p^k with k >= 1, else 0.
inline ulng isPrimePower(ulng n) {
    for (ulng k : primality_detail::SMALL) {
        while (n > 1) {
            ulng r = primality_detail::kthRoot(n, int(k)), pk = 1;
            for (ulng i = 0; i < k; ++i) { pk *= r; }
            if (pk != n) { break; }
            n = r;}}
    return isPrime(n) ? n : 0;}

// T: O(k^2 * w^3) (gcd refinement, k = a.size()), M: O(k * w); a_i >= 1, ascending coarsest coprime base, each a_i a product of its powers.
inline vector<ulng> coprimeBase(const vector<ulng> &a) {
    vector<ulng> res, todo;
    for (ulng x : a) { assert(x >= 1); todo.pb(x); }
    while (!todo.empty()) {
        ulng x = todo.back(); todo.pop_back();
        if (x < 2) { continue; }
        int i = 0;
        while (i < int(res.size()) && gcd(x, res[i]) == 1) { ++i; }
        if (i == int(res.size())) { res.pb(x); continue; }
        ulng b = res[i], g = gcd(x, b);
        res[i] = res.back(); res.pop_back();
        todo.pb(g); todo.pb(b / g); todo.pb(x / g);}
    sort(res.begin(), res.end());
    return res;}

// T: O(c), c = exponent-nonincreasing prime products <= N (47616 at 2^64), M: O(log(N)); N >= 1, smallest n <= N with maximal d(n), as (n, d(n)).
inline pair<ulng, lng> maxDivisorCount(ulng N) {
    assert(N >= 1);
    pair<ulng, lng> best{1, 1};
    auto dfs = [&](auto &&dfs, int i, ulng n, lng d, int cap) -> void {
        if (d > best.se || (d == best.se && n < best.fi)) { best = {n, d}; }
        if (i == int(std::size(primality_detail::SMALL))) { return; }
        ulng p = primality_detail::SMALL[i];
        for (int e = 1; e <= cap && n <= N / p; ++e) {
            n *= p; dfs(dfs, i + 1, n, d * (e + 1), e);}};
    dfs(dfs, 0, 1, 1, 64);
    return best;}
