#pragma once
#include "../1-Core/01-template.hpp"
#include "../6-Miscellaneous/random.hpp"

// T: O(log(n)), M: O(1)
bool isPrimeMR(lng n) { // Miller-Rabin
    if (n < 2) { return false; }
    if (n == 2 || n == 3) { return true; }
    if (!(n & 1)) { return false; }
    int s = __builtin_ctzll(n - 1); lng d = (n - 1) >> s;
    static constexpr lng bsm[] = {2LL, 7LL, 61LL};
    static constexpr lng blg[] = {2LL, 325LL, 9375LL, 28178LL, 450775LL, 9780504LL, 1795265022LL};
    const lng *B = n < 4759123141LL ? bsm : blg;
    for (int i = 0; i < (n < 4759123141LL ? 3 : 7); i++) {
        lng a = B[i];
        if (a % n == 0) { continue; }
        a %= n; lng x = 1;
        for (lng b = d; b > 0; b >>= 1) {
            if (b & 1) { x = ulll(x) * a % n; }
            a = ulll(a) * a % n;}
        if (x == 1 || x == n - 1) { continue; }
        bool is_comp = true;
        for (int j = 1; j < s; j++) {
            x = ulll(x) * x % n;
            if (x == n - 1) { is_comp = false; break; }
            if (x == 1) { return false; }}
        if (is_comp) { return false; }}
    return true;}

// T: O(n^(1/2)), M: O(log(n))
vector<lng> getPrimeFacSlow(lng n) {
    assert(n > 0);
    vector<lng> res;
    for (int p : {2, 3, 5}) { while (n % p == 0) { res.push_back(p); n /= p; } }
    int j = 0; static constexpr int inc[] = {4, 2, 4, 2, 4, 6, 2, 6};
    for (lng i = 7; i <= n / i; i += inc[j], j = (j + 1) & 7) {
        while (n % i == 0) { res.push_back(i); n /= i; }}
    if (n > 1) { res.push_back(n); }
    sort(res.begin(), res.end());
    return res;}

// T: O(n^(1/4)), M: O(1)
lng getOneFacBPR(lng n, lng x0 = 2, lng c = 1) { // Brent's Pollard's Rho
    assert(n > 1);
    if (n % 2 == 0) { return 2; }
    auto f = [](lng x, lng c, lng mod) -> lng { 
        return (ulll(x) * x + c) % mod;};
    int m = 128; lng l = 1;
    lng x = x0, xs = -1, y = -1, g = 1, q = 1;
    while (g == 1) {
        y = x; lng k = 0;
        while (g == 1 && k < l) {
            xs = x;
            for (int i = 0; i < min(lng(m), l - k); i++) {
                x = f(x, c, n);
                q = ulll(q) * (x > y ? x - y : y - x) % n;}
            g = gcd(q, n); k += m;}
        l *= 2;}
    if (g == n) {
        do {
            xs = f(xs, c, n);
            g = gcd(xs > y ? xs - y : y - xs, n);} 
        while (g == 1);}
    return g;}

Random rnd;
// T: O(n^(1/4)), M: O(log(n))
void getPrimeFacFast(lng n, vector<lng> &res) { 
    if (n <= 1) { return; }
    static constexpr int prms[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    for (int p : prms) {
        if (n % p == 0) {
            res.push_back(p);
            getPrimeFacFast(n / p, res);
            return;}}
    if (isPrimeMR(n)) { res.push_back(n); return; } 
    lng div = n, x0 = rnd.randLng(2, n - 1), c = rnd.randLng(1, n - 1);
    while (div == n) {
        div = getOneFacBPR(n, x0, c);
        if (div == n) {
            x0 = rnd.randLng(2, n - 1);
            c = rnd.randLng(1, n - 1);}} 
    getPrimeFacFast(div, res);
    getPrimeFacFast(n / div, res);}

// T: O(n^(1/4)) or O(n^(1/2)), M: O(d(n))
vector<lng> getAllFac(lng n, bool opt = 0) {
    if (n <= 0) { return {}; }
    if (n == 1) { return {1}; }
    vector<lng> prms; 
    //* 
    if (!opt) { prms = getPrimeFacSlow(n); }//*/  
    //*
    if (opt) { getPrimeFacFast(n, prms); }//*/
    sort(prms.begin(), prms.end());
    vector<lng> res = {1};
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i];
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        int len = res.size();
        for (int j = 0; j < len; j++) {
            lng cur = res[j];
            for (int k = 0; k < cnt; k++) { cur *= p; res.push_back(cur); }}}
    sort(res.begin(), res.end());
    return res;}

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getNumDiv(lng n, bool opt = 0) {
    assert(n > 0);
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i <= n / i; i++) {
            if (n % i == 0) {
                int exp = 0;
                do { exp++; n /= i; } while (n % i == 0);
                res *= exp + 1;}}
        if (n > 1) { res *= 2; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    lng res = 1;
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i];
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        res *= cnt + 1;}
    return res;}

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lll getSumDiv(lng n, bool opt = 0) {
    assert(n > 0);
    if (!opt) {
        lll res = 1;
        for (lng i = 2; i <= n / i; i++) {
            if (n % i == 0) {
                lng pow = 1; lll sum = 1;
                do { pow *= i; sum += pow; n /= i; } while (n % i == 0);
                res *= sum;}}
        if (n > 1) { res *= n + 1; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    lll res = 1;
    for (int i = 0; i < prms.size();) {
        int cnt = 0; lng p = prms[i]; 
        while (i < prms.size() && prms[i] == p) { cnt++; i++; }
        lng pow = 1; lll sum = 1;
        for (int j = 0; j < cnt; j++) { pow *= p; sum += pow; }
        res *= sum;}
    return res;}