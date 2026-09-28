#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/06-getprimefacslow.hpp"
#include "../5-Mathematics/08-getprimefacfast.hpp"

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