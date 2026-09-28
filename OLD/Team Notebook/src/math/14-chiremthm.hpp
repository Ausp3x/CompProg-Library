#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/02-exgcd.hpp"
#include "../5-Mathematics/solvemodeq.hpp"

 // T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T> 
requires (std::integral<T> || requires { T::is_inf; })
pair<T, T> chiRemThm(const vector<pair<lng, lng>> &cong) { // x ≡ a_i (mod m_i)
    if (cong.empty()) { return {0, 1}; }
    T a = cong[0].first, m = cong[0].second;
    for (int i = 1; i < cong.size(); i++) {
        lng a_i = cong[i].first, m_i = cong[i].second;
        lng d = a_i - lng(a % m_i), x, y, g = exGcd(lng(m % m_i), m_i, x, y);
        if (d % g != 0) { return {-1, -1}; } 
        m_i /= g;
        x = (lll((x % m_i + m_i) % m_i) * (d / g) % m_i + m_i) % m_i;
        a += x * m; m *= m_i;}
    return {(a % m + m) % m, m};}

 // T: O(n * log(n)) or O(n^2), M: O(n)
template<typename T>
requires (std::integral<T> || requires { T::is_inf; })
pair<T, T> superChiRemThm(const vector<tuple<lng, lng, lng>> &eqs) { // a_i * x ≡ b_i (mod m_i)
    vector<pair<lng, lng>> cong;
    for (const auto &[a_i, b_i, m_i] : eqs) {
        pair<lng, lng> cur = solveModEq(a_i, b_i, m_i);
        if (cur.first == -1) { return {-1, -1}; }
        cong.push_back(cur);}
    return chiRemThm<T>(cong);}