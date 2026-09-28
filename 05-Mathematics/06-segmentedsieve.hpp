#pragma once
#include "../01-Core/01-template.hpp"

// Block is an integer span, not a prime count. Odd stores only odd candidates;
// Wheel30 skips multiples of 2/3/5 and marks only coprime cofactors of each prime.
enum class SegmentedSieveMode { Plain, Odd, Wheel30 };

namespace segmented_sieve_detail {
    inline lng root(lng n) {
        assert(n >= 0); lng s = lng(std::sqrt(static_cast<long double>(n)));
        while (lll(s + 1) * (s + 1) <= n) { ++s; }
        while (lll(s) * s > n) { --s; }
        return s;}
    inline lll firstMultiple(lng l, lng p) { return lll(l / p + (l % p != 0)) * p; }
    inline void validate(lng l, lng r, int block, SegmentedSieveMode mode) {
        assert(l <= r && block > 0);
        assert(mode == SegmentedSieveMode::Plain || mode == SegmentedSieveMode::Odd || mode == SegmentedSieveMode::Wheel30);}
    template<typename F>
    void visit(lng l, lng r, const vector<lng> &bases, F f, int block, SegmentedSieveMode mode) {
        validate(l, r, block, mode);
        bool odd = mode == SegmentedSieveMode::Odd, wheel = mode == SegmentedSieveMode::Wheel30;
        if (odd || wheel) {
            for (lng p : {2, 3, 5}) { if ((wheel || p == 2) && l <= p && p < r) { f(p); }} }
        l = max(l, lng(wheel ? 7 : odd ? 3 : 2));
        constexpr uint MASK = (1u << 1) | (1u << 7) | (1u << 11) | (1u << 13)
                           | (1u << 17) | (1u << 19) | (1u << 23) | (1u << 29);
        constexpr int STEP[30] = {0, 6, 0, 0, 0, 0, 0, 4, 0, 0, 0, 2, 0, 4, 0,
                                 0, 0, 2, 0, 4, 0, 0, 0, 6, 0, 0, 0, 0, 0, 2};
        vector<char> mark;
        while (l < r) {
            lng hi = lng(min(lll(r), lll(l) + block)), first = odd ? l | 1 : l;
            int stride = odd ? 2 : 1, n = int((hi - first + stride - 1) / stride);
            mark.assign(n, true);
            if (wheel) { for (int i = 0; i < n; ++i) { mark[i] = (MASK >> ((l + i) % 30)) & 1; }}
            for (lng p : bases) {
                if (p * p >= hi) { break; }
                if ((odd && p == 2) || (wheel && p <= 5)) { continue; }
                if (wheel) {
                    lng k = max(p, l / p + (l % p != 0));
                    while (!(MASK >> (k % 30) & 1)) { ++k; }
                    for (lll j = lll(p) * k; j < hi; j = lll(p) * k) {
                        mark[size_t(j - l)] = false; k += STEP[k % 30]; }}
                else {
                    lll start = max(lll(p) * p, firstMultiple(first, p));
                    if (odd && !(start & 1)) { start += p; }
                    if (start >= hi) { continue; }
                    lng step = stride * p;
                    for (lng j = lng(start);;) {
                        mark[size_t((j - first) / stride)] = false;
                        if (step >= hi - j) { break; } j += step; }} }
            for (int i = 0; i < n; ++i) { if (mark[i]) { f(first + lng(stride) * i); }}
            l = hi;}}
} // namespace segmented_sieve_detail

// Reusable bases for all prime queries with R - 1 <= max_value. All lng maxima
// are supported arithmetically; sqrt(max_value) bases can require substantial time
// and memory (about 1.2 GB of prime storage near INT64_MAX). No dense sqrt-sized array.
// S: O(s * loglog(s) + ceil(s / B) * pi(sqrt(s))), U: same, Q: see forEachPrime,
// M: O(pi(s)) (setup workspace O(B + sqrt(s))); s = floor(sqrt(max_value)), B = block; loglog(s) means log(log(s + 3)).
struct SegmentedSieveBase {
    lng max_value = 1, limit = 1;
    vector<lng> bprms;
    explicit SegmentedSieveBase(lng maximum = 1, int block = 32768) { reset(maximum, block); }
    void reset(lng maximum, int block = 32768) {
        assert(maximum >= 0 && block > 0); max_value = maximum;
        limit = segmented_sieve_detail::root(max_value); lng t = segmented_sieve_detail::root(limit);
        vector<char> mark(size_t(t + 1), true); vector<lng> seed;
        for (lng p = 2; p <= t; ++p) {
            if (!mark[size_t(p)]) { continue; } seed.push_back(p);
            for (lng j = p * p; j <= t; j += p) { mark[size_t(j)] = false; }}
        bprms.clear();
        segmented_sieve_detail::visit(2, max(lng(2), limit + 1), seed,
            [&](lng p) { bprms.push_back(p); }, block, SegmentedSieveMode::Odd);}

    // Ascending primes in [L,R); signed L/R, values below 2 are ignored. Callback
    // must not mutate this base. Repeated queries do not change/cache interval state.
    // T: O(n * loglog(R) + ceil(n / B) * pi(sqrt(R))), M: O(B);
    // n = max(0, R - max(L,2)); sqrt(R) means sqrt(max(2,R)).
    template<typename F>
    void forEachPrime(lng L, lng R, F f, int block = 32768,
                      SegmentedSieveMode mode = SegmentedSieveMode::Odd) const {
        segmented_sieve_detail::validate(L, R, block, mode);
        assert(L == R || R <= 2 || lll(R) - 1 <= max_value);
        segmented_sieve_detail::visit(L, R, bprms, std::move(f), block, mode);}
};

// Canonical interval is HALF-OPEN [l,r), unlike the archived closed constructor.
// fromClosed explicitly migrates [L,R]; R == INT64_MAX is rejected before R + 1.
// All signed endpoints are valid if r-l fits vector::max_size and memory is available.
// Table entries for x<=0 are 0 (outside the positive number-theory domain), x=1 is 1.
// S: base setup + streamed query + O(n), U: reset has same cost, Q: O(1) stored lookup,
// M: O(n + pi(sqrt(R))); optional tables/factors add O(n) / O(n * log(R)) storage.
struct SegmentedSieve {
    lng l = 0, r = 0;
    vector<char> is_prime;
    vector<lng> bprms, prms, num_div, phi, rem;
    vector<lll> sum_div;
    vector<signed char> mu;

    SegmentedSieve(lng L, lng R, int block = 32768, SegmentedSieveMode mode = SegmentedSieveMode::Odd) {
        assert(L <= R);
        SegmentedSieveBase base(L == R || R <= 2 ? 1 : R - 1, block); reset(L, R, base, block, mode);}
    SegmentedSieve(lng L, lng R, const SegmentedSieveBase &base, int block = 32768,
                   SegmentedSieveMode mode = SegmentedSieveMode::Odd) { reset(L, R, base, block, mode); }
    static SegmentedSieve fromClosed(lng L, lng R, int block = 32768,
                                     SegmentedSieveMode mode = SegmentedSieveMode::Odd) {
        assert(L <= R && R < std::numeric_limits<lng>::max()); return SegmentedSieve(L, R + 1, block, mode);}
    static SegmentedSieve fromClosed(lng L, lng R, const SegmentedSieveBase &base, int block = 32768,
                                     SegmentedSieveMode mode = SegmentedSieveMode::Odd) {
        assert(L <= R && R < std::numeric_limits<lng>::max()); return SegmentedSieve(L, R + 1, base, block, mode);}
    void reset(lng L, lng R, const SegmentedSieveBase &base, int block = 32768,
               SegmentedSieveMode mode = SegmentedSieveMode::Odd) {
        segmented_sieve_detail::validate(L, R, block, mode);
        assert(lll(R) - L <= lll(is_prime.max_size()));
        l = L; r = R; is_prime.assign(size_t(lll(r) - l), false); bprms = base.bprms;
        prms.clear(); num_div.clear(); sum_div.clear(); phi.clear(); mu.clear(); rem.clear();
        base.forEachPrime(l, r, [&](lng p) { is_prime[size_t(lll(p) - l)] = true; prms.push_back(p); }, block, mode);}

    // Visits (offset, prime, exponent) for every prime power of every positive x>1.
    // Nonpositive entries/1 emit nothing; rem keeps the residual after small factors.
    // In factorization bounds R = max(2,r), so signed/empty intervals are covered.
    // T: O(n * log(R) + pi(sqrt(R))), M: O(n); n = r-l, callback cost excluded.
    template<typename F>
    void forEachFactor(F f) {
        rem.resize(is_prime.size());
        for (size_t i = 0; i < rem.size(); ++i) { rem[i] = max(lng(1), lng(lll(l) + i)); }
        for (lng p : bprms) {
            if (lll(p) * p >= r) { break; }
            lll start = segmented_sieve_detail::firstMultiple(max(lng(2), l), p);
            if (start >= r) { continue; }
            for (lng x = lng(start);;) {
                size_t i = size_t(lll(x) - l); int e = 0;
                while (rem[i] % p == 0) { rem[i] /= p; ++e; }
                if (e) { f(i, p, e); }
                if (p >= r - x) { break; } x += p; }}
        for (size_t i = 0; i < rem.size(); ++i) { if (rem[i] > 1) { f(i, rem[i], 1); }}}

    // Sorted factor lists for the positive interval. 1 has an empty factorization.
    // T: O(n * log(R) + pi(sqrt(R))), M: O(n + k); k = total returned prime powers.
    vector<vector<pair<lng, int>>> getPrimeFac() {
        assert(l >= 1 || l == r); vector<vector<pair<lng, int>>> factors(is_prime.size());
        forEachFactor([&](size_t i, lng p, int e) { factors[i].push_back({p, e}); }); return factors;}
    // T: O(pi(sqrt(x)) + log(x)), M: O(log(x)) returned storage.
    vector<pair<lng, int>> getPrimeFac(lng x) const {
        assert(1 <= x && l <= x && x < r); vector<pair<lng, int>> factors;
        for (lng p : bprms) {
            if (p > x / p) { break; } int e = 0;
            while (x % p == 0) { x /= p; ++e; } if (e) { factors.push_back({p, e}); }}
        if (x > 1) { factors.push_back({x, 1}); } return factors;}

    // Each individual table: T: O(n * log(R) + pi(sqrt(R))), M: O(n).
    void getNumDiv() {
        num_div.resize(is_prime.size());
        for (size_t i = 0; i < num_div.size(); ++i) { num_div[i] = lll(l) + i > 0; }
        forEachFactor([&](size_t i, lng, int e) { num_div[i] *= e + 1; });}
    void getSumDiv() {
        sum_div.resize(is_prime.size());
        for (size_t i = 0; i < sum_div.size(); ++i) { sum_div[i] = lll(l) + i > 0; }
        forEachFactor([&](size_t i, lng p, int e) {
            lll power_sum = 1; for (int j = 0; j < e; ++j) { power_sum = power_sum * p + 1; }
            sum_div[i] *= power_sum; });}
    void getPhi() {
        phi.resize(is_prime.size());
        for (size_t i = 0; i < phi.size(); ++i) { phi[i] = max(lng(0), lng(lll(l) + i)); }
        forEachFactor([&](size_t i, lng p, int) { phi[i] -= phi[i] / p; });}
    void getMu() {
        mu.resize(is_prime.size());
        for (size_t i = 0; i < mu.size(); ++i) { mu[i] = lll(l) + i > 0; }
        forEachFactor([&](size_t i, lng, int e) { mu[i] = e > 1 ? 0 : -mu[i]; });}
    // Combined traversal when all tables are needed. Same time/space bounds.
    void getTables() {
        size_t n = is_prime.size(); num_div.resize(n); sum_div.resize(n); phi.resize(n); mu.resize(n);
        for (size_t i = 0; i < n; ++i) {
            lng x = lng(lll(l) + i); num_div[i] = sum_div[i] = mu[i] = x > 0; phi[i] = max(lng(0), x); }
        forEachFactor([&](size_t i, lng p, int e) {
            num_div[i] *= e + 1; phi[i] -= phi[i] / p; mu[i] = e > 1 ? 0 : -mu[i];
            lll power_sum = 1; for (int j = 0; j < e; ++j) { power_sum = power_sum * p + 1; }
            sum_div[i] *= power_sum; });}
};
