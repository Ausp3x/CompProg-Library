#pragma once
#include "01-template.hpp"

// T: O(n * log(n)) per series operation, O(n) for +, -, scalar *, eval, deriv, integ; M: O(n).
// T a prime-field residue with 2 * bit_ceil(n + 1) dividing mod - 1 and mod > n; inv needs a[0] != 0, log a[0] = 1, exp a[0] = 0.
template<typename T>
struct PolyMini : vector<T> {
    using vector<T>::vector;
    PolyMini() = default;
    PolyMini(vector<T> a) : vector<T>(std::move(a)) {}

    int deg() const { int d = int(this->size()) - 1; while (d >= 0 && (*this)[d] == T(0)) { --d; } return d; }
    PolyMini &trim() { this->resize(deg() + 1); return *this; }

    static pair<vector<T>, vector<T>> facts(int n) {
        vector<T> f(n + 1, T(1)), g(n + 1);
        for (int i = 1; i <= n; ++i) { f[i] = f[i - 1] * T(i); }
        g[n] = T(1) / f[n];
        for (int i = n; i > 0; --i) { g[i - 1] = g[i] * T(i); }
        return {f, g};}
    // n a power of two dividing mod - 1; bit-reversed output at roots z^((mod - 1) / n), z the least quadratic non-residue; inverse scales by 1/n.
    static void ntt(vector<T> &a, bool inverse = false) {
        int n = int(a.size()); ulng e = ulng(T::mod()) - 1;
        assert(std::has_single_bit(uint(n)) && e % ulng(n) == 0);
        T z = 2;
        for (int t = 0; t < 1024 && pow(z, e / 2) == T(1); ++t) { z += 1; }
        vector<T> w(n);
        for (int len = 1; len <= n / 2; len *= 2) {
            T s = pow(z, inverse ? e - e / ulng(2 * len) : e / ulng(2 * len)); w[len] = 1;
            for (int j = 1; j < len; ++j) { w[len + j] = w[len + j - 1] * s; }}
        for (int k = 1; k <= n / 2; k *= 2) {
            int len = inverse ? k : n / 2 / k;
            for (int i = 0; i + 2 * len <= n; i += 2 * len) {
                for (int j = 0; j < len; ++j) {
                    T &x = a[i + j], &y = a[i + j + len];
                    if (inverse) { T u = y * w[len + j]; y = x - u; x += u; }
                    else { T u = x - y; x += y; y = u * w[len + j]; }}}}
        if (inverse) { T s = T(1) / T(n); for (T &x : a) { x *= s; } }}

    friend PolyMini operator+(PolyMini a, const PolyMini &b) {
        if (a.size() < b.size()) { a.resize(b.size()); }
        for (int i = 0; i < int(b.size()); ++i) { a[i] += b[i]; }
        return a;}
    friend PolyMini operator-(PolyMini a, const PolyMini &b) {
        if (a.size() < b.size()) { a.resize(b.size()); }
        for (int i = 0; i < int(b.size()); ++i) { a[i] -= b[i]; }
        return a;}
    friend PolyMini operator*(PolyMini a, PolyMini b) {
        int n = int(a.size()), m = int(b.size());
        if (!n || !m) { return {}; }
        if (min(n, m) <= 32) {
            PolyMini r(n + m - 1);
            for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) { r[i + j] += a[i] * b[j]; } }
            return r;}
        int len = int(std::bit_ceil(uint(n + m - 1)));
        a.resize(len); b.resize(len); ntt(a); ntt(b);
        for (int i = 0; i < len; ++i) { a[i] *= b[i]; }
        ntt(a, true); a.resize(n + m - 1);
        return a;}
    friend PolyMini operator*(PolyMini a, const T &c) { for (T &x : a) { x *= c; } return a; }

    friend T eval(const PolyMini &a, const T &x) {
        T r = 0;
        for (int i = int(a.size()) - 1; i >= 0; --i) { r = r * x + a[i]; }
        return r;}
    friend PolyMini deriv(const PolyMini &a) {
        PolyMini r(max(int(a.size()) - 1, 0));
        for (int i = 1; i < int(a.size()); ++i) { r[i - 1] = a[i] * T(i); }
        return r;}
    friend PolyMini integ(const PolyMini &a, const T &c = T(0)) {
        int n = int(a.size()); auto [f, g] = facts(n);
        PolyMini r(n + 1); r[0] = c;
        for (int i = 0; i < n; ++i) { r[i + 1] = a[i] * f[i] * g[i + 1]; }
        return r;}

    friend pair<PolyMini, PolyMini> divMod(const PolyMini &a, const PolyMini &b) {
        int da = a.deg(), db = b.deg(); assert(db >= 0);
        PolyMini r(a.begin(), a.begin() + (da + 1));
        if (db < 0 || da < db) { return {PolyMini(), r}; }
        int n = da - db + 1;
        PolyMini q = PolyMini(r.rbegin(), r.rbegin() + n) * inv(PolyMini(b.rend() - (db + 1), b.rend()), n);
        q.resize(n); std::reverse(q.begin(), q.end());
        PolyMini t = q * b; r.resize(db);
        for (int i = 0; i < db; ++i) { r[i] -= t[i]; }
        return {q, r.trim()};}

    friend PolyMini inv(const PolyMini &a, int n) {
        assert(n >= 0);
        if (!n) { return {}; }
        assert(a.size() && a[0] != T(0));
        PolyMini g = {T(1) / (a.empty() ? T(0) : a[0])};
        for (int k = 1; k < n; k *= 2) {
            vector<T> f(a.begin(), a.begin() + min(int(a.size()), 2 * k));
            f.resize(4 * k); g.resize(4 * k); ntt(f); ntt(g);
            for (int i = 0; i < 4 * k; ++i) { g[i] *= T(2) - f[i] * g[i]; }
            ntt(g, true); g.resize(2 * k);}
        g.resize(n); return g;}
    friend PolyMini log(const PolyMini &a, int n) {
        assert(n >= 0 && a.size() && a[0] == T(1));
        if (n <= 1) { return PolyMini(n); }
        PolyMini r = deriv(PolyMini(a.begin(), a.begin() + min(int(a.size()), n))) * inv(a, n - 1);
        r.resize(n - 1); return integ(r);}
    friend PolyMini exp(const PolyMini &a, int n) {
        assert(n >= 0 && (a.empty() || a[0] == T(0)));
        PolyMini f = {T(1)};
        for (int k = 1; k < n; k *= 2) {
            PolyMini h = PolyMini(a.begin(), a.begin() + min(int(a.size()), 2 * k)) - log(f, 2 * k);
            h[0] += T(1); f = f * h; f.resize(2 * k);}
        f.resize(n); return f;}
    friend PolyMini pow(const PolyMini &a, lng k, int n) {
        int v = 0;
        while (v < int(a.size()) && a[v] == T(0)) { ++v; }
        assert(n >= 0 && (k >= 0 || (v == 0 && a.size())));
        PolyMini r(n);
        if (!k || !n) { if (n) { r[0] = 1; } return r; }
        if (v == int(a.size()) || (v && (k < 0 || k >= (n + v - 1) / v))) { return r; }
        int s = int(v * k);
        PolyMini u = PolyMini(a.begin() + v, a.begin() + min(int(a.size()), v + n - s)) * (T(1) / a[v]);
        u = exp(log(u, n - s) * T(k), n - s) * pow(a[v], k);
        std::copy(u.begin(), u.end(), r.begin() + s);
        return r;}

    static vector<PolyMini> tree(const vector<T> &xs) {
        int n = int(xs.size()); vector<PolyMini> t(4 * n);
        auto go = [&](auto &&f, int k, int l, int r) -> void {
            if (r - l == 1) { t[k] = {-xs[l], T(1)}; return; }
            int m = std::midpoint(l, r); f(f, 2 * k, l, m); f(f, 2 * k + 1, m, r);
            t[k] = t[2 * k] * t[2 * k + 1];};
        if (n) { go(go, 1, 0, n); }
        return t;}
    static vector<T> evalTree(const vector<PolyMini> &t, const vector<T> &xs, const PolyMini &a) {
        vector<T> res(xs.size());
        auto go = [&](auto &&f, PolyMini p, int k, int l, int r) -> void {
            p = divMod(p, t[k]).second;
            if (r - l <= 32) { for (int i = l; i < r; ++i) { res[i] = eval(p, xs[i]); } return; }
            int m = std::midpoint(l, r); f(f, p, 2 * k, l, m); f(f, p, 2 * k + 1, m, r);};
        if (xs.size()) { go(go, a, 1, 0, int(xs.size())); }
        return res;}
    // T: O(n * log(n)^2 + |a| * log(|a|)), M: O(n * log(n)).
    friend vector<T> evalMulti(const PolyMini &a, const vector<T> &xs) { return evalTree(tree(xs), xs, a); }
    // T: O(n * log(n)^2), M: O(n * log(n)); distinct points (asserted).
    static PolyMini interpolate(const vector<T> &xs, vector<T> ys) {
        int n = int(xs.size()); assert(int(ys.size()) == n); ys.resize(n);
        if (!n) { return {}; }
        vector<PolyMini> t = tree(xs);
        vector<T> w = evalTree(t, xs, deriv(t[1]));
        for (int i = 0; i < n; ++i) { assert(w[i] != T(0)); w[i] = ys[i] / w[i]; }
        auto go = [&](auto &&f, int k, int l, int r) -> PolyMini {
            if (r - l == 1) { return {w[l]}; }
            int m = std::midpoint(l, r);
            return f(f, 2 * k, l, m) * t[2 * k + 1] + f(f, 2 * k + 1, m, r) * t[2 * k];};
        return go(go, 1, 0, n);}
    friend PolyMini taylorShift(const PolyMini &a, const T &c) {
        int n = int(a.size()); auto [f, g] = facts(n);
        PolyMini x(n), y(n); T p = 1;
        for (int j = 0; j < n; ++j, p *= c) { x[n - 1 - j] = a[j] * f[j]; y[j] = p * g[j]; }
        x = x * y;
        for (int k = 0; k < n; ++k) { y[k] = x[n - 1 - k] * g[k]; }
        return y;}

    // T: O(n^2), M: O(n); shortest c with a_i = sum_{j=1}^{|c|} c[j-1] * a_{i-j} for |c| <= i < n (Berlekamp–Massey).
    static vector<T> findLinearRecurrence(const vector<T> &a) {
        vector<T> c = {T(1)}, b = {T(1)};
        T bd = 1; int l = 0, m = 1;
        for (int i = 0; i < int(a.size()); ++i, ++m) {
            T d = 0;
            for (int j = 0; j <= l && j < int(c.size()); ++j) { d += c[j] * a[i - j]; }
            if (d == T(0)) { continue; }
            vector<T> t = c; T s = d / bd;
            if (c.size() < b.size() + m) { c.resize(b.size() + m); }
            for (int j = 0; j < int(b.size()); ++j) { c[j + m] -= s * b[j]; }
            if (2 * l <= i) { l = i + 1 - l; b = t; bd = d; m = 0; }}
        c.resize(l + 1); c.erase(c.begin());
        for (T &x : c) { x = -x; }
        return c;}
    // T: O(d * log(d) * log(k)), M: O(d); a_k for a_i = sum_{j=1}^{d} c[j-1] * a_{i-j} with d = |c| <= |a| (Bostan–Mori).
    static T linearRecurrenceKth(const vector<T> &a, const vector<T> &c, lng k) {
        int d = int(c.size()); assert(k >= 0 && int(a.size()) >= d);
        if (k < lng(a.size())) { return k < 0 ? T(0) : a[k]; }
        if (!d) { return T(0); }
        PolyMini p(a.begin(), a.begin() + min(d, int(a.size()))), q(d + 1); q[0] = 1;
        for (int j = 1; j <= d; ++j) { q[j] = -c[j - 1]; }
        p = p * q; p.resize(d);
        for (; k; k >>= 1) {
            PolyMini r = q;
            for (int i = 1; i <= d; i += 2) { r[i] = -r[i]; }
            PolyMini u = p * r, v = q * r;
            for (int i = 0; i < d; ++i) { p[i] = u[2 * i + (k & 1)]; }
            for (int i = 0; i <= d; ++i) { q[i] = v[2 * i]; }}
        return p[0];}
};
