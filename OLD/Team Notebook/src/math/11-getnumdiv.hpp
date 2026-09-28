#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/08-getprimefacfast.hpp"

// T: O(n^(1/4)) or O(n^(1/2)), M: O(1) or O(log(n))
lng getNumDiv(lng n, bool opt = 0) {
    if (!opt) {
        lng res = 1;
        for (lng i = 2; i * i <= n; i++) {
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