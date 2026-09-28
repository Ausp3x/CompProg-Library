#pragma once
#include "../1-Core/01-template.hpp"

// S: O(n), U: NA, Q: O(1), M: O(n)
struct LinearSieve {
    int n;
    vector<int> prms, spf, num_div, phi;
    vector<lng> sum_div; vector<signed char> mu;
    LinearSieve(int N) : n(N) {
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
    // Q: O(log(n))
    vector<pair<int, int>> getPrimeFac(int a) {
        assert(a <= n);
        vector<pair<int, int>> res;
        while (a > 1) { 
            int p = spf[a], cnt = 0;
            while (spf[a] == p) { cnt++; a /= p; }
            res.push_back({p, cnt});}
        return res;}
};