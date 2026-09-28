#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-exgcdandmodpow.hpp"
#include "../5-Mathematics/03-eqnsolvers.hpp"
#include "../5-Mathematics/05-primesandfactors.hpp"
#include "../5-Mathematics/06-phiandinverse.hpp"
#include "../6-Miscellaneous/customhash.hpp"

// T: O(n^(1/2)), M: O(n^(1/2))
lng modLog(lng a, lng b, lng mod, lng k = 1) { // a^x ≡ b mod m
    assert(mod > 0);
    a = (a % mod + mod) % mod; 
    b = (b % mod + mod) % mod;
    k = (k % mod + mod) % mod;
    if (mod == 1 || b == k) { return 0; }
    lng add = 0, g;
    while ((g = gcd(a, mod)) > 1) {
        if (b == k) { return add; }
        if (b % g != 0) { return -1; }
        b /= g; mod /= g;
        k = lll(k) * a / g % mod;
        add++;}
    lng n = std::sqrt(mod) + 1, an = modPow(a, n, mod);
    safe_unordered_map<lng, int> Q;
    Q.max_load_factor(0.7); Q.reserve(n + 1);
    for (lng q = 0, r = b; q <= n; q++) { Q[r] = q; r = lll(r) * a % mod; }
    for (lng p = 1, l = k; p <= n; p++) {
        l = lll(l) * an % mod;
        auto it = Q.find(l);
        if (it != Q.end()) { return n * p - it->second + add; }}
    return -1;}

// T: O(n * log(n)), M: O(log(n))
lng getPrimRoot(lng mod, bool opt = 0) {
    if (mod <= 0) { return -1; }
    if (mod == 1 || mod == 2 || mod == 4) { return mod - 1; }
    if (mod % 4 == 0) { return -1; }
    vector<lng> prms;
    //*
    if (!opt) { prms = getPrimeFacSlow(mod >> (!(mod & 1))); }//*/
    //*
    if (opt) { getPrimeFacFast(mod >> (!(mod & 1)), prms); }//*/
    sort(prms.begin(), prms.end());
    if (!prms.empty()) { if (prms[0] != prms.back()) { return -1; } }
    lng phi = getPhi(mod, opt); prms.clear();
    //*
    if (!opt) { prms = getPrimeFacSlow(phi); }//*/
    //*
    if (opt) { getPrimeFacFast(phi, prms); }//*/
    sort(prms.begin(), prms.end());
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    for (lng res = 2; res < mod; res++) {
        if (gcd(res, mod) != 1) { continue; }
        bool chk = true;
        for (int i = 0; i < prms.size() && chk; i++) {
            if (modPow(res, phi / prms[i], mod) == 1) { chk = false; }}
        if (chk) { return res; }}
    return -1;}

// T: O(n * log(n)), M: O(n)
vector<lng> modRoot(lng a, lng b, lng mod, bool opt = 0) { // x^a ≡ b mod m
    // b = 0 for arbitrary m or gcd(b, m) = 1 for m with primitive root
    assert(a > 0 && mod > 0);
    b = (b % mod + mod) % mod;
    if (mod == 1) { return {0}; }
    if (a == 1) { return {b}; }
    if (b == 0) { 
        vector<lng> prms;
        //*
        if (!opt) { prms = getPrimeFacSlow(mod); }//*/
        //*
        if (opt) { getPrimeFacFast(mod, prms); }//*/
        sort(prms.begin(), prms.end());
        lng cur = 1;
        for (int i = 0; i < prms.size();) {
            int cnt = 0; lng p = prms[i];
            while (i < prms.size() && prms[i] == p) { cnt++; i++; }
            for (int j = 0; j < (cnt - 1) / a + 1; j++) { cur *= p; }}
        vector<lng> res;
        for (lng i = 0; i < mod; i += cur) { res.push_back(i); }
        return res;}
    assert(gcd(b, mod) == 1);
    lng g = getPrimRoot(mod, opt);
    if (g == -1) { return {}; }
    lng k = modLog(g, b, mod);
    if (k == -1) { return {}; } 
    lng phi = getPhi(mod, opt), x, y, d = exGcd(a, phi, x, y);
    if (k % d != 0) { return {}; }
    phi /= d; x = lll((x % phi + phi) % phi) * (k / d) % phi;
    vector<lng> res;
    for (lng i = 0; i < d; i++) {
        res.push_back(modPow(g, x + i * phi, mod));}
    sort(res.begin(), res.end());
    res.erase(std::unique(res.begin(), res.end()), res.end());
    return res;}