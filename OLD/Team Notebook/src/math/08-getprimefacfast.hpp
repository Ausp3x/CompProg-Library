#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/05-isprimemr.hpp"
#include "../5-Mathematics/07-getonefacbpr.hpp"
#include "../6-Miscellaneous/random.hpp"

// T: O(n^(1/4)), M: O(log(n))
Random rnd;
void getPrimeFacFast(lng n, vector<lng> &res) { 
    if (n <= 1) { return; }
    if (isPrimeMR(n)) { res.push_back(n); return; } 
    static constexpr int prms[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29};
    for (int p : prms) {
        if (n % p == 0) {
            res.push_back(p);
            getPrimeFacFast(n / p, res);
            return;}}
    lng div = n, x0 = rnd.randLng(2, n - 1), c = rnd.randLng(1, n - 1);
    while (div == n) {
        div = getOneFacBPR(n, x0, c);
        if (div == n) {
            x0 = rnd.randLng(2, n - 1);
            c = rnd.randLng(1, n - 1);}} 
    getPrimeFacFast(div, res);
    getPrimeFacFast(n / div, res);}