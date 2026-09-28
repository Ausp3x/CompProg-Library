#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-exgcdandmodpow.hpp"
#include "../5-Mathematics/03-eqnsolvers.hpp"
#include "../5-Mathematics/06-phiandinverse.hpp"

 // T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T> 
requires std::integral<T> || std::same_as<T, lll> || std::same_as<T, ulll> || requires { T::is_inf; }
pair<T, T> chiRemThm(const vector<pair<lng, lng>> &cong) { // x ≡ a_i (mod m_i)
    if (cong.empty()) { return {0, 1}; }
    assert(cong[0].second > 0);
    T a = (cong[0].first % cong[0].second + cong[0].second) % cong[0].second;
    T m = cong[0].second;
    for (auto [a_i, m_i] : cong) {
        assert(m_i > 0);
        a_i = (a_i % m_i + m_i) % m_i;
        lng d = (lll(a_i) - lng(a % m_i) + m_i) % m_i;
        lng x, y, g = exGcd(lng(m % m_i), m_i, x, y);
        if (d % g != 0) { return {-1, -1}; } 
        m_i /= g;
        x = (lll((x % m_i + m_i) % m_i) * (d / g) % m_i + m_i) % m_i;
        a += x * m; m *= m_i;}
    return {(a % m + m) % m, m};}

// T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T>
requires std::integral<T> || std::same_as<T, lll> || std::same_as<T, ulll> || requires { T::is_inf; }
pair<T, T> superChiRemThm(const vector<tuple<lng, lng, lng>> &eqs) { // a_i * x ≡ b_i (mod m_i)
    vector<pair<lng, lng>> cong;
    for (const auto &[a_i, b_i, m_i] : eqs) {
        pair<lng, lng> cur = solveModEq(a_i, b_i, m_i);
        if (cur.first == -1) { return {-1, -1}; }
        cong.push_back(cur);}
    return chiRemThm<T>(cong);}

// T: O(n^2 * log(n)), M: O(n^2)
template<typename T> 
requires std::integral<T> || std::same_as<T, lll> || std::same_as<T, ulll> || requires { T::is_inf; }
T garner(const vector<pair<lng, lng>> &cong, T mod) { // coprime moduli
    if (cong.empty()) { return 0; }
    assert(mod > 0);
    for (auto [a_i, m_i] : cong) { assert(m_i > 0); }
    int n = cong.size();
    vector<vector<lng>> R(n, vector<lng>(n));
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            R[i][j] = getInvE(cong[i].second, cong[j].second);
            if (R[i][j] == -1) { return -1; }}}
    vector<lng> X(n);
    for (int i = 0; i < n; i++) {
        lng m_i = cong[i].second;
        X[i] = (cong[i].first % m_i + m_i) % m_i;
        for (int j = 0; j < i; j++) {
            X[i] = (lll(R[j][i]) * (X[i] - X[j]) % m_i + m_i) % m_i;}}
    T res = 0, mul = 1;
    for (int i = 0; i < n; i++) { res = (res + mul * X[i]) % mod; mul = mul * cong[i].second % mod; }
    return res;}