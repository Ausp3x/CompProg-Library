#pragma once
#include "../1-Core/01-template.hpp"
#include "../1-Core/03-modint.hpp"

struct ModFac {
    int n;
    vector<mint> fac, inv_fac;
    ModFac(int n): n(n) {
        fac.resize(n + 1); fac[0] = 1;
        for (int i = 1; i <= n; i++) {
            fac[i] = fac[i - 1] * i;}
        inv_fac.resize(n + 1);
        inv_fac[n] = inv(fac[n]);
        for (int i = n - 1; i >= 0; i--) {
            inv_fac[i] = inv_fac[i + 1] * (i + 1);}}

    mint combiNR(int a, int b) const { // aCb
        if (b > a || b < 0) { return mint(0); }
        assert(a <= n);
        return fac[a] * inv_fac[b] * inv_fac[a - b];}
    mint combiWR(int a, int b) const {
        if (a < 1 || b < 0) { return mint(0); }
        assert(a + b - 1 <= n);
        return fac[a + b - 1] * inv_fac[b] * inv_fac[a - 1];}
    mint permuNR(int a, int b) const { // aPb
        if (b > a || b < 0) { return mint(0); }
        assert(a <= n);   
        return fac[a] * inv_fac[a - b]; }
    mint permuWR(int a, int b) const {
        if (b < 0) { return mint(0); }
        return pow(mint(a), b);}
};