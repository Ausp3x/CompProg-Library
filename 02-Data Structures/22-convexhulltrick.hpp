#pragma once

#include "../01-Core/01-template.hpp"

namespace cht_detail {
    // T: O(1), M: O(1)
    template<typename T>
    bool inDomain(T v) {
        if constexpr (std::is_integral_v<T>) { return -(1LL << 62) < v && v < (1LL << 62); }
        else {
            T a = std::abs(v), e = std::ldexp(T(1), std::numeric_limits<T>::max_exponent / 4);
            return a == 0 || (1 / e <= a && a <= e);}}
} // namespace cht_detail

// T lng (|m|, |b| < 2^62, |m * x|, |m * x + b| < 2^63 - 1) or floating (|m|, |b| 0 or in [2^-E, 2^E], E = max_exponent / 4, approximate); slopes at either end; queries return {value, line id}.
// S: O(1), U: amortized O(1), Q: O(log(n)) (queryMonotone amortized O(1) for monotone x and T = lng), M: O(n)
template<bool MAX = false, typename T = lng>
struct MonotoneCHT {
    static_assert(std::is_same_v<T, lng> || std::is_floating_point_v<T>);
    using W = std::conditional_t<std::is_integral_v<T>, lll, T>;
    struct Line {
        T m, b;
        int id;

        T eval(T x) const { return m * x + b; }
    };
    static constexpr T NONE = MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    deque<Line> d;
    int cnt = 0, ptr = 0;

    // Lower hull of the stored (negated when MAX) lines, slopes strictly decreasing from front to back; lines are compared through differences.
    static bool bad(const Line &f, const Line &g, const Line &h) { return W(h.b - f.b) * W(f.m - g.m) <= W(g.b - f.b) * W(f.m - h.m); }

    static bool notWorse(const Line &f, const Line &g, T x) { return W(f.m - g.m) * W(x) <= W(g.b - f.b); }

    pair<T, int> result(const Line &f, T x) const { return {MAX ? -f.eval(x) : f.eval(x), f.id}; }

    void addLine(T m, T b) {
        assert(cht_detail::inDomain(m) && cht_detail::inDomain(b));
        Line f{MAX ? -m : m, MAX ? -b : b, cnt++};
        if (d.empty() || f.m <= d.back().m) {
            if (!d.empty() && f.m == d.back().m) {
                if (d.back().b <= f.b) { return; }
                d.pop_back();}
            while (d.size() >= 2 && bad(d[d.size() - 2], d.back(), f)) { d.pop_back(); }
            d.push_back(f);
            ptr = min(ptr, int(d.size()) - 1);
            return;}
        assert(f.m >= d.front().m);
        int k = 0;
        if (f.m == d.front().m) {
            if (d.front().b <= f.b) { return; }
            d.pop_front(); ++k;}
        while (d.size() >= 2 && bad(f, d[0], d[1])) { d.pop_front(); ++k; }
        d.push_front(f);
        ptr = min(max(ptr - k, 0) + 1, int(d.size()) - 1);}

    pair<T, int> query(T x) const {
        if (d.empty()) { return {NONE, -1}; }
        int l = 0, r = int(d.size()) - 1;
        while (l < r) {
            int md = (l + r) / 2;
            if (notWorse(d[md], d[md + 1], x)) { r = md; }
            else { l = md + 1; }}
        return result(d[l], x);}

    pair<T, int> queryMonotone(T x) {
        if (d.empty()) { return {NONE, -1}; }
        while (ptr + 1 < int(d.size()) && !notWorse(d[ptr], d[ptr + 1], x)) { ++ptr; }
        while (ptr > 0 && notWorse(d[ptr - 1], d[ptr], x)) { --ptr; }
        return result(d[ptr], x);}
};

// T lng (|m|, |b| < 2^62, |m * x|, |m * x + b| < 2^63 - 1) or floating (|m|, |b| 0 or in [2^-E, 2^E], E = max_exponent / 4, approximate); NONE when empty.
// S: O(1), U: amortized O(log(n)), Q: O(log(n)), M: O(n)
template<bool MAX = false, typename T = lng>
struct LineContainer {
    static_assert(std::is_same_v<T, lng> || std::is_floating_point_v<T>);
    struct Line {
        mutable T m, b, p;

        bool operator<(const Line &o) const { return m < o.m; }
        bool operator<(T x) const { return p < x; }
    };
    static constexpr T NONE = MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max();
    static constexpr T INF = std::numeric_limits<T>::has_infinity ? std::numeric_limits<T>::infinity() : std::numeric_limits<T>::max();
    multiset<Line, std::less<>> s;

    static T div(T a, T b) {
        if constexpr (std::is_integral_v<T>) { return a / b - ((a ^ b) < 0 && a % b != 0); }
        else { return a / b; }}

    // Upper envelope of the stored (negated when not MAX) lines; p is the last x where a line is optimal.
    bool isect(multiset<Line, std::less<>>::iterator x, multiset<Line, std::less<>>::iterator y) {
        if (y == s.end()) {
            x->p = INF;
            return false;}
        x->p = x->m == y->m ? (x->b > y->b ? INF : -INF) : div(y->b - x->b, x->m - y->m);
        return x->p >= y->p;}

    void addLine(T m, T b) {
        assert(cht_detail::inDomain(m) && cht_detail::inDomain(b));
        auto z = s.insert({MAX ? m : -m, MAX ? b : -b, 0}), y = z++, x = y;
        while (isect(y, z)) { z = s.erase(z); }
        if (x != s.begin() && isect(--x, y)) { isect(x, y = s.erase(y)); }
        while ((y = x) != s.begin() && (--x)->p >= y->p) { isect(x, s.erase(y)); }}

    T query(T x) const {
        if (s.empty()) { return NONE; }
        auto it = s.lower_bound(max(x, -INF + 1));
        T y = it->m * x + it->b;
        return MAX ? y : -y;}
};
