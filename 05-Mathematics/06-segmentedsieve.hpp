#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1), M: O(1); Plain marks every integer, Odd stores odd candidates, Wheel30 only residues coprime to 30.
enum class SegmentedSieveMode { Plain, Odd, Wheel30 };

namespace segmented_sieve_detail {
    // T: O(1), M: O(1); root = floor(sqrt(n)) for n >= 0, firstMultiple = least multiple of p >= l, validate asserts the arguments.
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
            for (lng p : {2, 3, 5}) { if ((wheel || p == 2) && l <= p && p < r) { f(p); }}}
        l = max(l, lng(wheel ? 7 : odd ? 3 : 2));
        constexpr uint MASK = (1u << 1) | (1u << 7) | (1u << 11) | (1u << 13)
                           | (1u << 17) | (1u << 19) | (1u << 23) | (1u << 29);
        // NEXT[k % 30] moves k to the next residue coprime to 30, STEP[k % 30] from one such residue to the next.
        constexpr int NEXT[30] = {1, 0, 5, 4, 3, 2, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 1, 0, 3, 2, 1, 0, 5, 4, 3, 2, 1, 0};
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
                lng k = max(p, first / p + (first % p != 0));
                // Offsets p * k - first are below 2^64, so wrapping ulng arithmetic is exact.
                if (wheel) {
                    k += NEXT[k % 30];
                    for (ulng j = ulng(p) * ulng(k) - ulng(l); j < ulng(n); j = ulng(p) * ulng(k) - ulng(l)) { mark[j] = false; k += STEP[k % 30]; }}
                else {
                    k |= odd;
                    for (ulng j = (ulng(p) * ulng(k) - ulng(first)) >> odd; j < ulng(n); j += ulng(p)) { mark[j] = false; }}}
            for (int i = 0; i < n; ++i) { if (mark[i]) { f(first + lng(stride) * i); }}
            l = hi;}}
} // namespace segmented_sieve_detail

// S: O(s * log(log(s + 3)) + ceil(s / B) * pi(sqrt(s))), U: same, Q: see forEachPrime, M: O(pi(s)); s = floor(sqrt(max_value)), B = block.
struct SegmentedSieveBase {
    lng max_value = 1, limit = 1;
    vector<lng> bprms;
    explicit SegmentedSieveBase(lng maximum = 1, int block = 32768) { reset(maximum, block); }
    void reset(lng maximum, int block = 32768) {
        assert(maximum >= 0 && block > 0); max_value = maximum;
        limit = segmented_sieve_detail::root(max_value); lng t = segmented_sieve_detail::root(limit);
        vector<char> mark(size_t(t + 1), true); vector<lng> seed;
        for (lng p = 2; p <= t; ++p) {
            if (!mark[size_t(p)]) { continue; }
            seed.push_back(p);
            for (lng j = p * p; j <= t; j += p) { mark[size_t(j)] = false; }}
        bprms.clear();
        segmented_sieve_detail::visit(2, max(lng(2), limit + 1), seed,
            [&](lng p) { bprms.push_back(p); }, block, SegmentedSieveMode::Odd);}

    // T: O(n * log(log(R + 3)) + ceil(n / B) * pi(sqrt(R))), M: O(B); n = max(0, R - max(L, 2)), ascending primes in [L, R), R - 1 <= max_value.
    template<typename F>
    void forEachPrime(lng L, lng R, F f, int block = 32768,
                      SegmentedSieveMode mode = SegmentedSieveMode::Odd) const {
        segmented_sieve_detail::validate(L, R, block, mode);
        assert(L == R || R <= 2 || lll(R) - 1 <= max_value);
        segmented_sieve_detail::visit(L, R, bprms, std::move(f), block, mode);}
};

// S: base setup + forEachPrime + O(n), U: same (reset), Q: O(1), M: O(n + pi(sqrt(r))); n = r - l, HALF-OPEN [l, r), tables 0 for x <= 0.
struct SegmentedSieve {
    lng l = 0, r = 0;
    vector<char> is_prime;
    vector<lng> bprms, prms, num_div, phi, rem;
    vector<lll> sum_div;
    vector<int8_t> mu;

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
        l = L; r = R; is_prime.assign(size_t(lll(r) - l), false);
        bprms.assign(base.bprms.begin(), upper_bound(base.bprms.begin(), base.bprms.end(), segmented_sieve_detail::root(max(lng(0), r - 1))));
        prms.clear(); num_div.clear(); sum_div.clear(); phi.clear(); mu.clear(); rem.clear();
        base.forEachPrime(l, r, [&](lng p) { is_prime[size_t(lll(p) - l)] = true; prms.push_back(p); }, block, mode);}

    // T: O(n * log(R) + pi(sqrt(R))), M: O(n); R = max(2, r), f(offset, prime, exponent) per prime power of each x > 1.
    template<typename F>
    void forEachFactor(F f) {
        rem.resize(is_prime.size());
        for (lng i = 0; i < lng(rem.size()); ++i) { rem[i] = max(lng(1), lng(lll(l) + i)); }
        for (lng p : bprms) {
            if (lll(p) * p >= r) { break; }
            lll start = segmented_sieve_detail::firstMultiple(max(lng(2), l), p);
            if (start >= r) { continue; }
            for (lng x = lng(start);;) {
                lng i = lng(lll(x) - l); int e = 0;
                while (rem[i] % p == 0) { rem[i] /= p; ++e; }
                if (e) { f(i, p, e); }
                if (p >= r - x) { break; } x += p;}}
        for (lng i = 0; i < lng(rem.size()); ++i) { if (rem[i] > 1) { f(i, rem[i], 1); }}}

    // T: O(n * log(R) + pi(sqrt(R))), M: O(n + out); sorted lists for l >= 1, single x in [l, r) in O(pi(sqrt(x)) + log(x)).
    vector<vector<pair<lng, int>>> getPrimeFac() {
        assert(l >= 1 || l == r); vector<vector<pair<lng, int>>> factors(is_prime.size());
        forEachFactor([&](lng i, lng p, int e) { factors[i].push_back({p, e}); }); return factors;}
    vector<pair<lng, int>> getPrimeFac(lng x) const {
        assert(1 <= x && l <= x && x < r); vector<pair<lng, int>> factors;
        for (lng p : bprms) {
            if (p > x / p) { break; }
            int e = 0;
            while (x % p == 0) { x /= p; ++e; }
            if (e) { factors.push_back({p, e}); }}
        if (x > 1) { factors.push_back({x, 1}); }
        return factors;}

    void getNumDiv() {
        num_div.resize(is_prime.size());
        for (lng i = 0; i < lng(num_div.size()); ++i) { num_div[i] = lll(l) + i > 0; }
        forEachFactor([&](lng i, lng, int e) { num_div[i] *= e + 1; });}
    void getSumDiv() {
        sum_div.resize(is_prime.size());
        for (lng i = 0; i < lng(sum_div.size()); ++i) { sum_div[i] = lll(l) + i > 0; }
        forEachFactor([&](lng i, lng p, int e) {
            lll power_sum = 1; for (int j = 0; j < e; ++j) { power_sum = power_sum * p + 1; }
            sum_div[i] *= power_sum;});}
    void getPhi() {
        phi.resize(is_prime.size());
        for (lng i = 0; i < lng(phi.size()); ++i) { phi[i] = max(lng(0), lng(lll(l) + i)); }
        forEachFactor([&](lng i, lng p, int) { phi[i] -= phi[i] / p; });}
    void getMu() {
        mu.resize(is_prime.size());
        for (lng i = 0; i < lng(mu.size()); ++i) { mu[i] = lll(l) + i > 0; }
        forEachFactor([&](lng i, lng, int e) { mu[i] = int8_t(e > 1 ? 0 : -mu[i]); });}
    void getTables() {
        lng n = lng(is_prime.size()); num_div.resize(n); sum_div.resize(n); phi.resize(n); mu.resize(n);
        for (lng i = 0; i < n; ++i) {
            lng x = lng(lll(l) + i); num_div[i] = sum_div[i] = x > 0; mu[i] = x > 0; phi[i] = max(lng(0), x);}
        forEachFactor([&](lng i, lng p, int e) {
            num_div[i] *= e + 1; phi[i] -= phi[i] / p; mu[i] = int8_t(e > 1 ? 0 : -mu[i]);
            lll power_sum = 1; for (int j = 0; j < e; ++j) { power_sum = power_sum * p + 1; }
            sum_div[i] *= power_sum;});}
};
