#pragma once
#include "../1-Core/01-template.hpp"

// S: O(n * loglog(n)), U: NA, Q: O(1), M: O(n)
struct SieveOfErath {
    int n; vector<char> is_prime;
    SieveOfErath(int N) : n(N) {
        assert(n >= 0);
        is_prime.assign(n + 1, true);
        if (n >= 0) { is_prime[0] = false; }
        if (n >= 1) { is_prime[1] = false; }
        for (int i = 4; i <= n; i += 2) { is_prime[i] = false; }
        for (int i = 3; 1LL * i * i <= n; i += 2) {
            if (!is_prime[i]) { continue; }
            for (lng j = 1LL * i * i; j <= n; j += 2 * i) { is_prime[j] = false; }}}
};

// S: O(n), U: O(n), Q: O(1) or O(log(n)), M: O(n)
struct LinearSieve {
    int n;
    vector<int> prms, spf, num_div, phi;
    vector<lng> sum_div; vector<signed char> mu;
    LinearSieve(int N) : n(N) {
        assert(n >= 0);
        spf.assign(n + 1, -1);
        for (int i = 2; i <= n; i++) {
            if (spf[i] == -1) { prms.push_back(i); spf[i] = i; }
            for (int p : prms) {
                if (1LL * i * p > n) { break; }
                spf[i * p] = p;
                if (i % p == 0) { break; }}}}

    void getNumDiv() {
        num_div.assign(n + 1, 0); vector<int> spf_exp(n + 1, 0);
        if (n >= 1) { num_div[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                spf_exp[i] = spf_exp[j] + 1; 
                num_div[i] = num_div[j] / (spf_exp[j] + 1) * (spf_exp[i] + 1);} 
            else {
                spf_exp[i] = 1;
                num_div[i] = num_div[j] * 2;}}}
    void getSumDiv() {
        sum_div.assign(n + 1, 0); vector<lng> spf_exp_sum(n + 1, 0);
        if (n >= 1) { sum_div[1] = 1; spf_exp_sum[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) {
                spf_exp_sum[i] = spf_exp_sum[j] * p + 1;
                sum_div[i] = sum_div[j] / spf_exp_sum[j] * spf_exp_sum[i];} 
            else {
                spf_exp_sum[i] = p + 1;
                sum_div[i] = sum_div[j] * (p + 1);}}}
    void getPhi() {
        phi.assign(n + 1, 0);
        if (n >= 1) { phi[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { phi[i] = phi[j] * p; } 
            else { phi[i] = phi[j] * (p - 1); }}}
    void getMu() {
        mu.assign(n + 1, 0);
        if (n >= 1) { mu[1] = 1; }
        for (int i = 2; i <= n; i++) {
            int p = spf[i], j = i / p;
            if (j % p == 0) { mu[i] = 0; } 
            else { mu[i] = -mu[j]; }}}
    vector<pair<int, int>> getPrimeFac(int a) {
        assert(1 <= a && a <= n);
        vector<pair<int, int>> res;
        while (a > 1) { 
            int p = spf[a], cnt = 0;
            while (a > 1 && spf[a] == p) { cnt++; a /= p; }
            res.push_back({p, cnt});}
        return res;}
    // Q: O(d(n))
    vector<int> getAllFac(int a) { // unsorted
        auto prms = getPrimeFac(a); 
        vector<int> res = {1};
        for (auto [p, cnt] : prms) {
            int len = res.size();
            for (int i = 0; i < len; i++) {
                lng cur = res[i]; 
                for (int j = 0; j < cnt; j++) { cur *= p; res.push_back(cur); }}}
        return res;}
};

// S: O(n * loglog(n)), U: O(n * loglog(n)), Q: O(1), M: O(n)
struct SegmentedSieve {
    lng l, r; vector<char> is_prime; 
    vector<lng> bprms, prms, num_div, sum_div, phi, rem;
    vector<signed char> mu;
    SegmentedSieve(lng L, lng R) : l(L), r(R) {
        assert(0 <= l && l <= r);
        lng lim = std::sqrtl(r);
        while (lll(lim + 1) * (lim + 1) <= r) { lim++; }
        while (lll(lim) * lim > r) { lim--; }
        vector<char> is_bprime(lim + 1, true);
        if (lim >= 0) { is_bprime[0] = false; }
        if (lim >= 1) { is_bprime[1] = false; }
        if (lim >= 2) { bprms.push_back(2); }
        for (int i = 4; i <= lim; i += 2) { is_bprime[i] = false; }
        for (lng i = 3; i <= lim; i += 2) {
            if (!is_bprime[i]) { continue; }
            bprms.push_back(i);
            for (lng j = i * i; j <= lim; j += 2 * i) { is_bprime[j] = false; }}
        is_prime.assign(r - l + 1, true);
        if (l <= 1) { is_prime[0] = false; if (l == 0 && r >= 1) { is_prime[1] = false; } }
        for (lng p : bprms) {
            lng j0 = max(p * p, (l + p - 1) / p * p);
            for (lng j = j0; j <= r; j += p) { is_prime[j - l] = false; }}
        for (lng i = 0; i <= r - l; i++) {
            if (is_prime[i]) { prms.push_back(i + l); }}}
    
    void getNumDiv() {
        num_div.assign(r - l + 1, 1); rem.resize(r - l + 1);
        for (lng i = 0; i <= r - l; i++) { rem[i] = i + l; }
        if (l == 0) { num_div[0] = 0; rem[0] = 1; }
        for (lng p : bprms) {
            lng j0 = (l + p - 1) / p * p; 
            if (j0 == 0) { j0 = p; }
            for (lng j = j0; j <= r; j += p) {
                lng cnt = 0;
                while (rem[j - l] % p == 0) { rem[j - l] /= p; cnt++; }
                num_div[j - l] *= (cnt + 1); }}
        for (lng i = 0; i <= r - l; i++) {
            if (rem[i] > 1) { num_div[i] *= 2; }}}
    void getSumDiv() {
        sum_div.assign(r - l + 1, 1); rem.resize(r - l + 1);
        for (lng i = 0; i <= r - l; i++) { rem[i] = i + l; }
        if (l == 0) { sum_div[0] = 0; rem[0] = 1; }
        for (lng p : bprms) {
            lng j0 = (l + p - 1) / p * p; 
            if (j0 == 0) { j0 = p; }
            for (lng j = j0; j <= r; j += p) {
                lng cur = 1, sum = 1;
                while (rem[j - l] % p == 0) { rem[j - l] /= p; cur *= p; sum += cur; }
                sum_div[j - l] *= sum; }}
        for (lng i = 0; i <= r - l; i++) {
            if (rem[i] > 1) { sum_div[i] *= (rem[i] + 1); }}}
    void getPhi() {
        phi.assign(r - l + 1, 0); rem.resize(r - l + 1);
        for (lng i = 0; i <= r - l; i++) { phi[i] = rem[i] = i + l; }
        if (l == 0) { phi[0] = 0; rem[0] = 1; }
        for (lng p : bprms) {
            lng j0 = (l + p - 1) / p * p; 
            if (j0 == 0) { j0 = p; }
            for (lng j = j0; j <= r; j += p) {
                phi[j - l] -= phi[j - l] / p;
                while (rem[j - l] % p == 0) { rem[j - l] /= p; }}}
        for (lng i = 0; i <= r - l; i++) {
            if (rem[i] > 1) { phi[i] -= phi[i] / rem[i]; }}}
    void getMu() {
        mu.assign(r - l + 1, 1); rem.resize(r - l + 1);
        for (lng i = 0; i <= r - l; i++) { rem[i] = i + l; }
        if (l == 0) { mu[0] = 0; rem[0] = 1; }
        for (lng p : bprms) {
            lng j0 = (l + p - 1) / p * p; 
            if (j0 == 0) { j0 = p; }
            for (lng j = j0; j <= r; j += p) {
                lng cnt = 0;
                while (rem[j - l] % p == 0) { rem[j - l] /= p; cnt++; }
                if (cnt > 1) { mu[j - l] = 0; } 
                else { mu[j - l] = -mu[j - l]; }}}
        for (lng i = 0; i <= r - l; i++) {
            if (rem[i] > 1) { mu[i] = -mu[i]; }}}
};