#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/13-getinv.hpp"

// T: O(n^2 * log(n)), M: O(n^2)
template<typename T> 
requires (std::integral<T> || requires { T::is_inf; })
T garner(const vector<pair<lng, lng>> &cong, T mod) { // coprime moduli
    if (cong.empty()) { return 0; }
    int n = cong.size();
    vector<vector<lng>> R(n, vector<lng>(n));
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            R[i][j] = getInvE(cong[i].second, cong[j].second);
            if (R[i][j] == -1) { return -1; }}}
    vector<lng> X(n);
    for (int i = 0; i < n; i++) {
        X[i] = (cong[i].first % cong[i].second + cong[i].second) % cong[i].second;
        for (int j = 0; j < i; j++) {
            X[i] = (lll(R[j][i]) * (X[i] - X[j]) % cong[i].second + cong[i].second) % cong[i].second;}}
    T res = 0, mul = 1;
    for (int i = 0; i < n; i++) { res = (res + mul * X[i]) % mod; mul = mul * cong[i].second % mod; }
    return res;}