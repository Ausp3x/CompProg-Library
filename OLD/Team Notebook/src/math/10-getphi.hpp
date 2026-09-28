#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/08-getprimefacfast.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getPhi(lng n, bool opt = 0) {
    if (!opt) {
        lng res = n;
        for (lng i = 2; i * i <= n; i++) {
            if (n % i == 0) { 
                while (n % i == 0) { n /= i; }
                res -= res / i;}}
        if (n > 1) { res -= res / n; }
        return res;}
    vector<lng> prms;
    //*
    getPrimeFacFast(n, prms);//*/
    sort(prms.begin(), prms.end());
    prms.erase(unique(prms.begin(), prms.end()), prms.end());
    lng res = n;
    for (lng p : prms) { res -= res / p; }
    return res;}