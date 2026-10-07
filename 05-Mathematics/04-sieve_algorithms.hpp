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
        for (int i = 3; lng(i) * i <= n; i += 2) {
            if (!is_prime[i]) { continue; }
            for (lng j = lng(i) * i; j <= n; j += 2 * i) { is_prime[j] = false; }}
        if (n >= 2) { prms.pb(2); }
        for (lng i = 3; i <= n; i += 2) { if (is_prime[i]) { prms.pb(int(i)); } }}
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

    void getLpf() {
        lpf.assign(n + 1, -1);
        for (int i = 2; i <= n; ++i) { lpf[i] = max(spf[i], lpf[i / spf[i]]); }}
    void getNumDiv() {
        num_div.assign(n + 1, 0); vector<int> exponent(n + 1, 0);
        if (n >= 1) { num_div[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                exponent[i] = exponent[j] + 1;
                num_div[i] = num_div[j] / (exponent[j] + 1) * (exponent[i] + 1);}
            else { exponent[i] = 1; num_div[i] = 2 * num_div[j]; }}}
    void getSumDiv() {
        sum_div.assign(n + 1, 0); vector<lng> prime_sum(n + 1, 0);
        if (n >= 1) { sum_div[1] = prime_sum[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                prime_sum[i] = prime_sum[j] * p + 1;
                sum_div[i] = sum_div[j] / prime_sum[j] * prime_sum[i];}
            else { prime_sum[i] = lng(p) + 1; sum_div[i] = sum_div[j] * prime_sum[i]; }}}
    void getPhi() {
        phi.assign(n + 1, 0);
        if (n >= 1) { phi[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            phi[i] = phi[j] * (j % p == 0 ? p : p - 1);}}
    void getMu() {
        mu.assign(n + 1, 0);
        if (n >= 1) { mu[1] = 1; }
        for (int i = 2; i <= n; ++i) {
            int p = spf[i], j = i / p;
            mu[i] = j % p == 0 ? 0 : -mu[j];}}

    // Q: O(log(a)), M: O(log(a)) returned; 1 <= a <= n, ascending prime powers.
    vector<pair<int, int>> getPrimeFac(int a) const {
        assert(1 <= a && a <= n);
        vector<pair<int, int>> res;
        while (a > 1) {
            int p = spf[a], count = 0;
            do { ++count; a /= p; } while (a > 1 && spf[a] == p);
            res.pb({p, count});}
        return res;}
    // Q: O(log(a) + out), M: O(out); 1 <= a <= n, unsorted positive divisors.
    vector<int> getAllFac(int a) const {
        auto factors = getPrimeFac(a);
        vector<int> res{1};
        for (auto [p, count] : factors) {
            int len = int(res.size()), power = 1;
            for (int e = 1; e <= count; ++e) {
                power *= p;
                for (int i = 0; i < len; ++i) { res.pb(res[i] * power); }}}
        return res;}
};
