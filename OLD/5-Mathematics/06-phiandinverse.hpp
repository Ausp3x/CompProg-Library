#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-exgcdandmodpow.hpp"
#include "../5-Mathematics/05-primesandfactors.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getPhi(lng n, bool opt = 0) {
    assert(n > 0);
    if (!opt) {
        lng res = n;
        for (lng i = 2; i <= n / i; i++) {
            if (n % i == 0) { 
                while (n % i == 0) { n /= i; }
                res -= res / i;}}
        if (n > 1) { res -= res / n; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    prms.erase(std::unique(prms.begin(), prms.end()), prms.end());
    lng res = n;
    for (lng p : prms) { res -= res / p; }
    return res;}

// T: O(log(n)), M: O(1)
lng getInvE(lng n, lng mod) {
    assert(mod > 0);
    if (mod == 1) { return 0; }
    n = (n % mod + mod) % mod;
    if (n == 0) { return -1; }
    lng x, y, g = exGcd(n, mod, x, y);
    if (g != 1) { return -1; }
    return (x % mod + mod) % mod;}

// T: O(n^(1/3)), M: O(log(n))
lng getInvP(lng n, lng mod) { // Pierce Expansion, mod must be prime
    assert(mod > 1);
    n = (n % mod + mod) % mod;
    assert(n > 0);
    return n == 1 ? 1 : mod - lll(mod) / n * getInvP(mod % n, mod) % mod;}