#pragma once
// existing-unverified; unchanged excerpt from OLD/algorithms.cpp:576-854.
// Legacy contracts and comments await audit; see this folder's 00-index.md.
#include "01-template.hpp"

// T: O(n^2), M: O(n)
template<typename T>
struct SparseMatrix {
    int n, m;
    vector<int> rwp;
    vector<int> col;
    vector<T> val;

    SparseMatrix(int N = 0, int M = 0) : n(N), m(M) {
        assert((n == 0 && m == 0) || (n > 0 && m > 0));

        rwp.assign(n + 1, 0);
    }

    SparseMatrix(int N, int M, const vector<tuple<int, int, T>> &a) : n(N), m(M) {
        assert((n == 0 && m == 0) || (n > 0 && m > 0));

        rwp.assign(n + 1, 0);
        col.reserve(a.size());
        val.reserve(a.size());

        for (const auto &[r, c, v] : a) {
            assert(0 <= r && r < n && 0 <= c && c < m);
            rwp[r + 1]++;
        }
        
        for (int i = 0; i < n; i++) {
            rwp[i + 1] += rwp[i];}
        
        col.resize(a.size());
        val.resize(a.size());
        vector<int> cur_rwp = rwp;
        for (const auto &[r, c, v] : a) {
            int pos = cur_rwp[r]++;
            col[pos] = c;
            val[pos] = v;
        }
    }

    int size() const { 
        return n; 
    }

    // T: O(K) 
    vector<T> operator*(const vector<T> &b) const {
        assert(m == b.size());

        vector<T> res(n, T(0));
        for (int i = 0; i < n; i++) {
            T sum = 0;
            for (int j = rwp[i]; j < rwp[i + 1]; j++) {
                sum += val[j] * b[col[j]];}
            
            res[i] = sum;
        }

        return res;
    }

    static bool isNil(const T &x) {
        if constexpr (std::is_floating_point_v<T>) {
            return -1e-9 < x && x < 1e-9;
        } else {
            return x == T(0);
        }
    }

    // T: O(N^2)
    static vector<T> berlekampMassey(const vector<T> &s) {
        vector<T> c = {T(1)}, old_c = {T(1)};
        int f = -1;
        T df = 1;

        for (int i = 0; i < s.size(); i++) {
            T d = 0;
            for (int j = 0; j < c.size(); j++) {
                d += c[j] * s[i - j];}

            if (isNil(d)) {
                continue;}

            vector<T> next_c = c;
            T coef = d / df;
            
            if (next_c.size() < old_c.size() + i - f) {
                next_c.resize(old_c.size() + i - f, T(0));}
            
            for (int j = 0; j < old_c.size(); j++) {
                next_c[j + i - f] -= coef * old_c[j];}

            if (i - f + old_c.size() > c.size()) {
                old_c = c;
                f = i;
                df = d;
            }
            c = std::move(next_c);
        }
        
        return c;
    }

    // T: O(L^2)
    static vector<T> polyMulMod(const vector<T> &a, const vector<T> &b, const vector<T> &c) {
        int L = c.size() - 1;
        vector<T> res(a.size() + b.size() - 1, T(0));
        for (int i = 0; i < a.size(); i++) {
            for (int j = 0; j < b.size(); j++) {
                res[i + j] += a[i] * b[j];}}
        
        for (int i = res.size() - 1; i >= L; i--) {
            if (isNil(res[i])) {
                continue;}
            
            for (int j = 1; j <= L; j++) {
                res[i - j] -= res[i] * c[j];}
        }
        res.resize(min((int)res.size(), L));
        
        return res;
    }

    // T: O(N * K + N^2)
    friend T det(const SparseMatrix &a) {
        assert(a.n == a.m);
        static_assert(!std::is_integral_v<T>);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> D(a.n);
        T det_D = 1;
        for (int i = 0; i < a.n; i++) {
            D[i] = T((rng() % max_val) + 1);
            det_D *= D[i];
        }

        vector<T> u(a.n), v(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));
            v[i] = T(rng() % (max_val + 1));
        }

        vector<T> seq(2 * a.n);
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * v[j];}
            
            seq[i] = dot;

            vector<T> dv(a.n);
            for (int j = 0; j < a.n; j++) {
                dv[j] = D[j] * v[j];}
            
            v = a * dv;
        }

        vector<T> min_poly = berlekampMassey(seq);

        if (min_poly.size() - 1 < a.n) {
            return T(0);}

        T det_AD = min_poly.back();
        if (a.n % 2 == 1) {
            det_AD = -det_AD;}

        return det_AD / det_D;
    }

    // T: O(N * K + N^2)
    friend vector<T> solveLin(const SparseMatrix &a, vector<T> b) {
        assert(a.n == a.m && a.n == b.size());
        static_assert(!std::is_integral_v<T>);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> u(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));}
        
        vector<T> seq(2 * a.n);
        vector<T> cur = b;
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * cur[j];}
            
            seq[i] = dot;
            cur = a * cur;
        }

        vector<T> min_poly = berlekampMassey(seq);
        int L = min_poly.size() - 1;

        assert(!isNil(min_poly.back()) && "Linear system solution does not exist or matrix is singular with respect to b.");

        T inv_cL = T(1) / min_poly.back();
        vector<T> res(a.n, T(0));
        cur = b;
        for (int i = 0; i < L; i++) {
            T coef = -min_poly[L - 1 - i] * inv_cL;
            for (int j = 0; j < a.n; j++) {
                res[j] += cur[j] * coef;}
            
            if (i + 1 < L) {
                cur = a * cur;}
        }

        return res;
    }

    // T: O(N * K + N * L + L^2 * log(k)) where L <= N
    friend vector<T> pow(const SparseMatrix &a, lng k, vector<T> v) {
        assert(a.n == a.m && a.n == v.size() && k >= 0);

        std::mt19937 rng(1337);
        uint32_t max_val = 10000;
        if constexpr (requires { int(T()); } && !std::is_floating_point_v<T>) {
            max_val = uint32_t(int(T(-1)));
        }

        vector<T> u(a.n);
        for (int i = 0; i < a.n; i++) {
            u[i] = T(rng() % (max_val + 1));}
        
        vector<T> seq(2 * a.n);
        vector<T> cur = v;
        for (int i = 0; i < 2 * a.n; i++) {
            T dot = 0;
            for (int j = 0; j < a.n; j++) {
                dot += u[j] * cur[j];}
            
            seq[i] = dot;
            cur = a * cur;
        }

        vector<T> min_poly = berlekampMassey(seq);

        vector<T> res_poly = {T(1)};
        vector<T> base_poly = {T(0), T(1)};
        while (k > 0) {
            if (k & 1) {
                res_poly = polyMulMod(res_poly, base_poly, min_poly);}
            
            k >>= 1;
            if (k > 0) {
                base_poly = polyMulMod(base_poly, base_poly, min_poly);}
        }

        vector<T> res(a.n, T(0));
        cur = v;
        for (int i = 0; i < res_poly.size(); i++) {
            for (int j = 0; j < a.n; j++) {
                res[j] += cur[j] * res_poly[i];}
            
            if (i + 1 < res_poly.size()) {
                cur = a * cur;}
        }

        return res;
    }

    // T: O(k * K)
    friend vector<T> steadyState(const SparseMatrix &a, vector<T> v, int max_iter = 1000) {
        assert(a.n == a.m && a.n == v.size());

        for (int i = 0; i < max_iter; i++) {
            v = a * v;}
        
        return v;
    }
};
