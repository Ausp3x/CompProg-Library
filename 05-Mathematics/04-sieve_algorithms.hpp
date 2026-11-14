#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n * log(log(3 + n))), U: same (reset), Q: O(1), M: O(n); tables on [0, n], 0 <= n < INT_MAX, n the largest size used.
struct SieveOfErath {
    int n;
    vector<char> is_prime;
    vector<int> prms;
    SieveOfErath(int N) { reset(N); }
    void reset(int N) {
        assert(0 <= N && N < std::numeric_limits<int>::max()); n = N;
        is_prime.assign(n + 1, true); prms.clear(); is_prime[0] = false;
        if (n >= 1) { is_prime[1] = false; }
        for (lng i = 4; i <= n; i += 2) { is_prime[i] = false; }
        int root = int(std::sqrt(double(n)));
        // The prefix [0, root] is sieved first; its primes strike the rest.
        for (int p = 3; p * p <= root; p += 2) { if (is_prime[p]) { for (int k = p * p; k <= root; k += 2 * p) { is_prime[k] = false; } } }
        vector<lng> base, next;
        for (int p = 3; p <= root; p += 2) { if (is_prime[p]) { base.pb(p); next.pb(lng(p) * p); } }
        // Striking odd multiples per 128 KB segment keeps the table in L2.
        constexpr lng SEGMENT = lng(1) << 17;
        for (lng lo = 0; lo <= n; lo += SEGMENT) {
            lng hi = min(lng(n) + 1, lo + SEGMENT);
            for (int i = 0; i < int(base.size()) && base[i] * base[i] < hi; ++i) {
                lng k = next[i], step = 2 * base[i];
                for (; k < hi; k += step) { is_prime[k] = false; }
                next[i] = k;}}
        if (n >= 2) { prms.pb(2); }
        for (lng i = 3; i <= n; i += 2) { if (is_prime[i]) { prms.pb(int(i)); } }}

    // Q: O(log(n)); x <= n, pi(x) = 0 below 2.
    int primeCount(int x) const { assert(x <= n); return int(upper_bound(prms.begin(), prms.end(), x) - prms.begin()); }
};

// S: O(n * log(log(3 + n))), Q: O(1), M: O(n / w); 0 <= n < 2^32, odd numbers as bits, no prime list.
struct OddBitSieve {
    lng n;
    vector<ulng> bits;
    vector<int> block_count;
    OddBitSieve(lng N) { reset(N); }
    void reset(lng N) {
        assert(0 <= N && N < (lng(1) << 32)); n = N;
        lng odd = (n + 1) / 2;
        int words = int((odd + 63) / 64);
        bits.assign(words, ~0ULL);
        if (words) { bits[0] &= ~1ULL; }
        vector<lng> base, next;
        for (int p : SieveOfErath(int(std::sqrt(double(n)))).prms) { if (p > 2) { base.pb(p); next.pb((lng(p) * p - 1) / 2); } }
        // Bit k is 2k + 1; striking multiples per 32 KB segment keeps the words in L1.
        constexpr lng SEGMENT = lng(1) << 18;
        for (lng lo = 0; lo < odd; lo += SEGMENT) {
            lng hi = min(odd, lo + SEGMENT);
            for (int i = 0; i < int(base.size()) && (base[i] * base[i] - 1) / 2 < hi; ++i) {
                lng k = next[i], step = base[i];
                for (; k < hi; k += step) { bits[k >> 6] &= ~(1ULL << (k & 63)); }
                next[i] = k;}}
        block_count.assign((words + 7) / 8 + 1, 0);
        for (int b = 0; 8 * b < words; ++b) {
            int c = 0;
            for (int w = 8 * b; w < min(words, 8 * b + 8); ++w) { c += std::popcount(bits[w]); }
            block_count[b + 1] = block_count[b] + c;}}

    bool isPrime(lng x) const { assert(0 <= x && x <= n); return x == 2 || ((x & 1) && (bits[x >> 7] >> ((x >> 1) & 63) & 1)); }
    int primeCount(lng x) const {
        assert(x <= n);
        if (x < 2) { return 0; }
        lng k = (x - 1) / 2;
        int w = int(k >> 6), b = w / 8, r = block_count[b] + 1;
        for (int i = 8 * b; i < w; ++i) { r += std::popcount(bits[i]); }
        return r + std::popcount(bits[w] & ((2ULL << (k & 63)) - 1));}
};

// S: O(n * log(log(3 + n))), M: O(n); [0, n] as SieveOfErath, spf -1 at 0 and 1 as LinearSieve, without its multiplicative tables.
struct SieveOfErathSpf {
    int n;
    vector<int> spf, prms;
    SieveOfErathSpf(int N) { reset(N); }
    void reset(int N) {
        assert(0 <= N && N < std::numeric_limits<int>::max()); n = N;
        spf.assign(n + 1, -1); prms.clear();
        for (lng j = 2; j <= n; j += 2) { spf[j] = 2; }
        vector<lng> base, next;
        for (int p : SieveOfErath(int(std::sqrt(double(n)))).prms) { if (p > 2) { base.pb(p); next.pb(lng(p) * p); } }
        // Primes in increasing order per 128 KB segment, so the first write is the smallest factor.
        constexpr lng SEGMENT = lng(1) << 15;
        for (lng lo = 0; lo <= n; lo += SEGMENT) {
            lng hi = min(lng(n) + 1, lo + SEGMENT);
            for (int i = 0; i < int(base.size()) && base[i] * base[i] < hi; ++i) {
                lng k = next[i], step = 2 * base[i];
                for (int p = int(base[i]); k < hi; k += step) { if (spf[k] == -1) { spf[k] = p; } }
                next[i] = k;}}
        if (n >= 2) { prms.pb(2); }
        for (lng i = 3; i <= n; i += 2) { if (spf[i] == -1) { spf[i] = int(i); prms.pb(int(i)); } }}
};

// S: O(n), U: O(n) per get table or reset, Q: O(1), M: O(n); [0, n] as above, spf/lpf -1 at 0 and 1, optional tables empty until built.
struct LinearSieve {
    int n;
    vector<int> prms, spf, lpf, num_div, phi;
    vector<lng> sum_div;
    vector<int8_t> mu;
    LinearSieve(int N) { reset(N); }
    void reset(int N) {
        assert(0 <= N && N < std::numeric_limits<int>::max()); n = N;
        spf.assign(n + 1, -1); prms.clear(); lpf.clear(); num_div.clear();
        phi.clear(); sum_div.clear(); mu.clear();
        for (int i = 2; i <= n; ++i) {
            if (spf[i] == -1) { prms.pb(i); spf[i] = i; }
            for (int p : prms) {
                if (lng(i) * p > n) { break; }
                spf[i * p] = p;
                if (p == spf[i]) { break; }}}}

    void lpfTable() {
        lpf.assign(n + 1, -1);
        for (int i = 2; i <= n; ++i) { lpf[i] = max(spf[i], lpf[i / spf[i]]); }}
    void divisorCountTable() {
        num_div.assign(n + 1, 0); vector<int> exponent(n + 1, 0);
        if (n >= 1) { num_div[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                exponent[i] = exponent[j] + 1;
                num_div[i] = num_div[j] / (exponent[j] + 1) * (exponent[i] + 1);}
            else { exponent[i] = 1; num_div[i] = 2 * num_div[j]; }}}
    void divisorSumTable() {
        sum_div.assign(n + 1, 0); vector<lng> prime_sum(n + 1, 0);
        if (n >= 1) { sum_div[1] = prime_sum[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                prime_sum[i] = prime_sum[j] * p + 1;
                sum_div[i] = sum_div[j] / prime_sum[j] * prime_sum[i];}
            else { prime_sum[i] = lng(p) + 1; sum_div[i] = sum_div[j] * prime_sum[i]; }}}
    void phiTable() {
        phi.assign(n + 1, 0);
        if (n >= 1) { phi[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            phi[i] = phi[j] * (j % p == 0 ? p : p - 1);}}
    void mobiusTable() {
        mu.assign(n + 1, 0);
        if (n >= 1) { mu[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            mu[i] = j % p == 0 ? 0 : -mu[j];}}

    // Q: O(log(n)); x <= n, pi(x) = 0 below 2.
    int primeCount(int x) const { assert(x <= n); return int(upper_bound(prms.begin(), prms.end(), x) - prms.begin()); }
    // Q: O(log(a)), M: O(log(a)) returned; 1 <= a <= n, ascending prime powers.
    vector<pair<int, int>> factorize(int a) const {
        assert(1 <= a && a <= n);
        vector<pair<int, int>> res;
        while (a > 1) {
            int p = spf[a], count = 0;
            do { ++count; a /= p; } while (a > 1 && spf[a] == p);
            res.pb({p, count});}
        return res;}
    // Q: O(log(a) + out), M: O(out); 1 <= a <= n, unsorted positive divisors.
    vector<int> divisors(int a) const {
        auto factors = factorize(a);
        vector<int> res{1};
        for (auto [p, count] : factors) {
            int len = int(res.size()), power = 1;
            for (int e = 1; e <= count; ++e) {
                power *= p;
                for (int i = 0; i < len; ++i) { res.pb(res[i] * power); }}}
        return res;}
};
