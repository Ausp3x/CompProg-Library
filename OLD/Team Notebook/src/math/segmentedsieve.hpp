#pragma once
#include "../1-Core/01-template.hpp"

// S: O(r^(1/2) * loglog(r^(1/2)) + n * loglog(r)), U: NA, Q: O(1), M: O(r^(1/2) + n)
struct SegmentedSieve {
    lng l, r; vector<char> is_prime; 
    vector<lng> bprms, prms, num_div, sum_div, phi, rem;
    vector<signed char> mu;
    SegmentedSieve(lng L, lng R) : l(L), r(R) {
        assert(0 <= l && l <= r);
        lng lim = sqrt(r) + 1; 
        while (lim * lim > r) { lim--; }
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