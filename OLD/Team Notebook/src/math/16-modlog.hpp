#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-modpow.hpp"

// T: O(n^(1/2)), M: O(n^(1/2))
lng modLog(lng a, lng b, lng mod, lng k = 1) { // a^x ≡ b mod m
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
    lng n = sqrt(mod) + 1, an = modPow(a, n, mod);
    vector<pair<lng, int>> Q0(n + 1);
    for (lng q = 0, r = b; q <= n; q++) { Q0[q] = {r, q}; r = lll(r) * a % mod; }
    sort(Q0.begin(), Q0.end());
    vector<pair<lng, int>> Q;
    for (auto &[r, q] : Q0) {
        if (!Q.empty() && Q.back().first == r) { Q.back().second = q; }
        else { Q.push_back({r, q}); }}
    for (lng p = 1, l = k; p <= n; p++) {
        l = lll(l) * an % mod;
        auto it = lower_bound(Q.begin(), Q.end(), std::make_pair(l, -1));
        if (it != Q.end() && it->first == l) { return n * p - it->second + add; }}
    return -1;}