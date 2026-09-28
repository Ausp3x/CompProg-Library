#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-modpow.hpp"
#include "../5-Mathematics/06-getprimefacslow.hpp"
#include "../5-Mathematics/08-getprimefacfast.hpp"
#include "../5-Mathematics/10-getphi.hpp"

// T: O(n^(1/4) * log(n)) or O(n^(1/2) * log(n)), M: O(log(n))
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
    prms.erase(unique(prms.begin(), prms.end()), prms.end());
    for (lng res = 2; res < mod; res++) {
        if (gcd(res, mod) != 1) { continue; }
        bool chk = true;
        for (int i = 0; i < prms.size() && chk; i++) {
            if (modPow(res, phi / prms[i], mod) == 1) { chk = false; }}
        if (chk) { return res; }}
    return -1;}