#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/02-exgcd.hpp"
#include "../5-Mathematics/10-getphi.hpp"
#include "../5-Mathematics/16-modlog.hpp"
#include "../5-Mathematics/17-getprimroot.hpp"

// T: O(n^(1/2)), M: O(n^(1/2))
vector<lng> modRoot(lng a, lng b, lng mod, bool opt = 0) { // x^a ≡ b mod m
    if (b == 0) { return {0}; }
    lng g = getPrimRoot(mod, opt);
    if (g == -1) { return {}; }
    lng k = modLog(g, b, mod);
    if (k == -1) { return {}; } 
    lng phi = getPhi(mod, opt), x, y, d = exGcd(a, phi, y, x);
    if (k % d != 0) { return {}; }
    y = (lll(y) % phi + phi) % phi * (k / d) % phi;
    vector<lng> res;
    for (lng i = 0; i < d; i++) {
        res.push_back(modPow(g, y + i * (phi / d), mod));}
    sort(res.begin(), res.end());
    res.erase(unique(res.begin(), res.end()), res.end());
    return res;}