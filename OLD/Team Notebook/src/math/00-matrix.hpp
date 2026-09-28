#pragma once
#include "../1-Core/01-template.hpp"

// T: O(n^2), M: O(n^2)
template<typename T>
struct Matrix {
    int n, m;
    vector<T> v;
    Matrix(int N = 0, int M = 0) : n(N * (M > 0)), m(M * (N > 0)), v(n * m, T(0)) {}
    Matrix(int N, int M, vector<T> a) : n(N * (M > 0)), m(M * (N > 0)), v(std::move(a)) {
        assert(v.size() == n * m);}
    // T: O(n)
    Matrix(vector<T> a, int axis = 0) : v(std::move(a)) {
        if (v.empty()) { n = 0; m = 0; return; }
        if (axis == 0) { n = 1; m = v.size(); } 
        else if (axis == 1) { n = v.size(); m = 1; }}
    Matrix(const vector<vector<T>> &a) {
        if (a.empty()) { n = 0; m = 0; return; }
        n = a.size(); m = a[0].size(); v.resize(n * m);
        for (int i = 0; i < n; i++) { std::copy(a[i].begin(), a[i].end(), v.begin() + i * m); }}
    static Matrix eye(int N, int M) {
        Matrix res(N, M);
        for (int i = 0; i < min(N, M); i++) { res[i][i] = T(1); }      
        return res;}
    
    // T: O(1)
    T *operator[](int i) { return v.data() + i * m; }
    // T: O(1)
    const T *operator[](int i) const { return v.data() + i * m; }
    // T: O(1)
    int size() const { return n; }

    Matrix &operator+=(const Matrix &o) {
        assert(n == o.n && m == o.m);
        for (int i = 0; i < n * m; i++) { v[i] += o.v[i]; }
        return *this;}
    Matrix &operator-=(const Matrix &o) {
        assert(n == o.n && m == o.m);
        for (int i = 0; i < n * m; i++) { v[i] -= o.v[i]; }
        return *this;}
    Matrix &operator*=(T o) {
        for (int i = 0; i < n * m; i++) { v[i] *= o; }
        return *this;}
    Matrix &operator*=(const vector<T> &o) { return *this = *this * o; }
    // T: O(n^3)
    Matrix &operator*=(const Matrix &o) { return *this = *this * o; }
    Matrix &operator/=(T o) {
        for (int i = 0; i < n * m; i++) { v[i] /= o; }
        return *this;}
    // T: O(n^3)
    Matrix &operator/=(const Matrix &o) { return *this = *this / o; }
    Matrix &operator%=(T o) {
        for (int i = 0; i < n * m; i++) { v[i] %= o; }
        return *this;}

    Matrix operator+() const { return *this; }
    Matrix operator-() const {
        Matrix res(n, m);
        for (int i = 0; i < n * m; i++) { res.v[i] = -v[i]; }
        return res;}
    friend Matrix operator+(Matrix a, const Matrix &b) { a += b; return a; }
    friend Matrix operator+(const Matrix &a, Matrix &&b) { b += a; return std::move(b); }
    friend Matrix operator-(Matrix a, const Matrix &b) { a -= b; return a; }
    friend Matrix operator-(const Matrix &a, Matrix &&b) {
        assert(a.n == b.n && a.m == b.m);
        for (int i = 0; i < a.n * a.m; i++) { b.v[i] = a.v[i] - b.v[i]; }
        return std::move(b);}
    friend Matrix operator*(Matrix a, T b) { a *= b; return a; }
    friend Matrix operator*(T b, Matrix a) { a *= b; return a; }
    friend Matrix operator*(const vector<T> &b, const Matrix &a) { return Matrix(b, 0) * a; }
    friend Matrix operator*(Matrix a, const vector<T> &b) { a *= Matrix(b, 1); return a; }
    // T: O(n^3)
    friend Matrix operator*(const Matrix &a, const Matrix &b) {
        assert(a.m == b.n);
        Matrix res(a.n, b.m);
        #ifdef __AVX2__
        if constexpr (requires { T::is_prime; } && sizeof(T) == 4) {
            if (max({a.n, a.m, b.m}) >= FastMatMul::BASE && min({a.n, a.m, b.m}) >= 16) {
                constexpr uint MOD = uint(int(T(-1))) + 1;
                int mx = max({a.n, a.m, b.m}), sz = FastMatMul::BASE;
                while (sz < mx) { sz <<= 1; }
                uint *a_rm = (uint*)(_mm_malloc(sz * sz * sizeof(uint), 32));
                std::memset(a_rm, 0, sz * sz * sizeof(uint));
                uint *b_rm = (uint*)(_mm_malloc(sz * sz * sizeof(uint), 32));
                std::memset(b_rm, 0, sz * sz * sizeof(uint));
                uint *c_rm = (uint*)(_mm_malloc(sz * sz * sizeof(uint), 32));
                const uint *a_pt = reinterpret_cast<const uint*>(a.v.data());
                for (int i = 0; i < a.n; ++i) { std::memcpy(a_rm + i * sz, a_pt + i * a.m, a.m * sizeof(uint)); }
                const uint *b_pt = reinterpret_cast<const uint*>(b.v.data());
                for (int i = 0; i < b.n; ++i) { std::memcpy(b_rm + i * sz, b_pt + i * b.m, b.m * sizeof(uint)); }
                FastMatMul::Pool pool(sz);
                uint *a_rc = pool.alloc(sz * sz), *b_rc = pool.alloc(sz * sz), *c_rc = pool.alloc(sz * sz);
                FastMatMul::packRec(a_rc, a_rm, sz, sz);
                FastMatMul::packRec(b_rc, b_rm, sz, sz);
                FastMatMul::strassen(a_rc, b_rc, c_rc, sz, MOD, pool);
                FastMatMul::unpackRec(c_rm, c_rc, sz, sz);
                uint *res_pt = reinterpret_cast<uint*>(res.v.data());
                for (int i = 0; i < res.n; ++i) { std::memcpy(res_pt + i * res.m, c_rm + i * sz, res.m * sizeof(uint)); }
                _mm_free(a_rm); _mm_free(b_rm); _mm_free(c_rm);
                return res;}}
        #endif
        constexpr int BASE = 64; 
        for (int i0 = 0; i0 < a.n; i0 += BASE) {
            for (int k0 = 0; k0 < a.m; k0 += BASE) {
                for (int j0 = 0; j0 < b.m; j0 += BASE) {
        int i_mx = min(i0 + BASE, a.n);
        int k_mx = min(k0 + BASE, a.m);
        int j_mx = min(j0 + BASE, b.m);
        for (int i = i0; i < i_mx; i++) {
            T *res_i = res[i];
            for (int k = k0; k < k_mx; k++) {
                if (isNil(a[i][k])) { continue; } 
                T a_ik = a[i][k]; const T *b_k = b[k];
                #pragma GCC ivdep
                for (int j = j0; j < j_mx; j++) { res_i[j] += a_ik * b_k[j]; }}}}}}
        return res;}
    friend Matrix operator/(Matrix a, T b) { a /= b; return a; }
    // T: O(n^3)
    friend Matrix operator/(Matrix a, const Matrix &b) { assert(a.m == b.n); a *= inv(b); return a; }
    friend Matrix operator%(Matrix a, T b) { a %= b; return a; }
    
    friend Matrix concat(const Matrix &a, const Matrix &b, int axis = 0) {
        Matrix res;
        if (axis == 0) {
            assert(a.n == b.n);
            res = Matrix(a.n, a.m + b.m);
            for (int i = 0; i < a.n; i++) {
                std::copy_n(a[i], a.m, res[i]);
                std::copy_n(b[i], b.m, res[i] + a.m);}}
        else if (axis == 1) {
            assert(a.m == b.m);
            res = Matrix(a.n + b.n, a.m);
            std::copy(a.v.begin(), a.v.end(), res.v.begin());
            std::copy(b.v.begin(), b.v.end(), res.v.begin() + a.v.size());}
        return res;}
    // T: O(1)
    friend Matrix cross(const Matrix &a, const Matrix &b, int axis = 0) {
        assert((a.n == 1 || a.m == 1) && max(a.n, a.m) == 3);
        assert((b.n == 1 || b.m == 1) && max(b.n, b.m) == 3);
        T x = a.v[1] * b.v[2] - a.v[2] * b.v[1];
        T y = a.v[2] * b.v[0] - a.v[0] * b.v[2];
        T z = a.v[0] * b.v[1] - a.v[1] * b.v[0];
        return Matrix(vector<T>{x, y, z}, axis);}
    // T: O(n^3) or O(n^3 * log(n))
    friend T det(Matrix a) {
        assert(a.n == a.m);
        bool chk = !std::is_floating_point_v<T>;
        if constexpr (requires { T::is_prime; }) { chk = !T::is_prime; }
        if constexpr (requires { std::declval<T>().is_inf; }) { chk = true; }
        if (chk) {
            T res = 1;
            for (int i = 0; i < a.n; i++) {
                for (int k = i + 1; k < a.n; k++) {
                    while (!isNil(a[k][i])) {
                        T q; T *a_i = a[i]; T *a_k = a[k];
                        if constexpr (requires { T::is_prime; }) { q = T(a[i][i].n / a[k][i].n); } 
                        else { q = a[i][i] / a[k][i]; }
                        #ifdef __AVX2__
                        if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                            FastGaussian::elimRow(a_i + i, a_k + i, q, a.n - i);} 
                        else 
                        #endif 
                        {   for (int j = i; j < a.n; j++) { a_i[j] -= a_k[j] * q; } }
                        std::swap_ranges(a_i + i, a_i + a.n, a_k + i);
                        res = -res;}}
                if (isNil(a[i][i])) { return T(0); }
                res *= a[i][i];}
            return res;}

        T res = 1;
        for (int i = 0; i < a.n; i++) {
            int piv = i;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = i + 1; p < a.n; p++) {
                    if (abs(a[p][i]) > abs(a[piv][i])) { piv = p; }}
                if (isNil(a[piv][i])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][i])) { piv++; } }
            if (piv == a.n) { return T(0); }
            if (i != piv) { std::swap_ranges(a[i], a[i] + a.m, a[piv]); res = -res; }
            res *= a[i][i]; T d = T(1) / a[i][i];
            for (int k = i + 1; k < a.n; k++) {
                if (isNil(a[k][i])) { continue; }
                T a_ki = a[k][i] * d;
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[k] + i + 1, a[i] + i + 1, a_ki, a.n - i - 1);} 
                else
                #endif 
                {   for (int j = i + 1; j < a.n; j++) { a[k][j] -= a[i][j] * a_ki; } }}}
        return res;}
    // T: O(n)
    friend T dot(const Matrix &a, const Matrix &b) {
        assert((a.n == 1 || a.m == 1) && (b.n == 1 || b.m == 1));
        assert(max(a.n, a.m) == max(b.n, b.m));
        T res = 0;
        for (int i = 0; i < max(a.n, a.m); i++) { res += a.v[i] * b.v[i]; }
        return res;}
    // T: O(n^3)
    friend Matrix inv(Matrix a) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) { 
        assert(a.n == a.m);
        return solveLin(std::move(a), eye(a.n, a.n));}
    // T: O(n^3 * log(n))
    friend Matrix pow(Matrix a, lng b) {
        assert(a.n == a.m && b >= 0);
        Matrix res = eye(a.n, a.n), tmp(a.n, a.n);
        while (b > 0) {
            if (b & 1) { res *= a; }
            b >>= 1;
            if (b > 0) { a *= a; }}
        return res;}
    // T: O(n^3)
    friend Matrix solveLin(Matrix a, Matrix b) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        assert(a.n == b.n);
        int r = 0; vector<int> pivs(a.m, -1);
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = r + 1; p < a.n; p++) {
                    if (abs(a[p][c]) > abs(a[piv][c])) { piv = p; }}
                if (isNil(a[piv][c])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][c])) { piv++; } }
            if (piv == a.n) { continue; }
            if (r != piv) {
                std::swap_ranges(a[r], a[r] + a.m, a[piv]);
                std::swap_ranges(b[r], b[r] + b.m, b[piv]);}
            T d = T(1) / a[r][c]; a[r][c] = T(1);
            for (int j = c + 1; j < a.m; j++) { a[r][j] *= d; }
            for (int j = 0; j < b.m; j++) { b[r][j] *= d; }
            for (int i = 0; i < a.n; i++) {
                if (i == r || isNil(a[i][c])) { continue; }
                T a_ic = a[i][c]; a[i][c] = T(0);
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[i] + c + 1, a[r] + c + 1, a_ic, a.m - c - 1);
                    FastGaussian::elimRow(b[i], b[r], a_ic, b.m);} 
                else
                #endif 
                {   for (int j = c + 1; j < a.m; j++) { a[i][j] -= a[r][j] * a_ic; }
                    for (int j = 0; j < b.m; j++) { b[i][j] -= b[r][j] * a_ic; }}}
            pivs[c] = r++;}
        for (int i = r; i < a.n; i++) {
            for (int j = 0; j < b.m; j++) {
                if (!isNil(b[i][j])) { return Matrix(); }}}
        Matrix res(a.m, b.m);
        for (int c = 0; c < a.m; c++) {
            if (pivs[c] != -1) { std::copy_n(b[pivs[c]], b.m, res[c]); }}
        return res;}
    // T: O(n)
    friend T trc(const Matrix &a) {
        assert(a.n == a.m);
        T res = 0;
        for (int i = 0; i < a.n; i++) { res += a[i][i]; }
        return res;}
    friend Matrix trp(const Matrix &a) { 
        Matrix res(a.m, a.n);
        for (int i = 0; i < a.m; i++) {
            for (int j = 0; j < a.n; j++) { res[i][j] = a[j][i]; }}
        return res;}

    // T: O(n^3)
    friend Matrix adj(const Matrix a0) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        assert(a0.n == a0.m);
        int n = a0.n;
        if (n == 0) { return Matrix(); }
        if (n == 1) { return Matrix(vector<T>{T(1)}, 0); }
        Matrix a = a0, b = eye(n, n);
        int r = 0; T det_a = 1; vector<int> pivs(n, -1); 
        for (int c = 0; c < n; c++) {
            int piv = r;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = r + 1; p < a.n; p++) {
                    if (abs(a[p][c]) > abs(a[piv][c])) { piv = p; }}
                if (isNil(a[piv][c])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][c])) { piv++; } }
            if (piv == n) { det_a = T(0); continue; }
            if (r != piv) {
                std::swap_ranges(a[r], a[r] + n, a[piv]);
                std::swap_ranges(b[r], b[r] + n, b[piv]);
                det_a = -det_a;}
            det_a *= a[r][c]; T d = T(1) / a[r][c]; a[r][c] = T(1);
            for (int j = c + 1; j < n; j++) { a[r][j] *= d; }
            for (int j = 0; j < n; j++) { b[r][j] *= d; }
            for (int i = 0; i < n; i++) {
                if (i == r || isNil(a[i][c])) { continue; }
                T a_ic = a[i][c]; a[i][c] = T(0);
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[i] + c + 1, a[r] + c + 1, a_ic, n - c - 1);
                    FastGaussian::elimRow(b[i], b[r], a_ic, n);} 
                else
                #endif 
                {   for (int j = c + 1; j < n; j++) { a[i][j] -= a[r][j] * a_ic; }
                    for (int j = 0; j < n; j++) { b[i][j] -= b[r][j] * a_ic; }}}
            pivs[c] = r++;}
        if (!isNil(det_a)) {
            Matrix res(n, n);
            for (int c = 0; c < n; c++) { std::copy_n(b[pivs[c]], n, res[c]); }
            res *= det_a;
            return res;}
        if (r < n - 1) { return Matrix(n, n); }
        Matrix u = ker(a0), v = ker(trp(a0)); 
        int nz_i = -1, nz_j = -1;
        for (int i = 0; i < n; i++) { if (!isNil(u[0][i])) { nz_i = i; break; } }
        for (int j = 0; j < n; j++) { if (!isNil(v[0][j])) { nz_j = j; break; } }
        Matrix sub(n - 1, n - 1);
        for (int i = 0, sr = 0; i < n; i++) {
            if (i == nz_j) { continue; }
            for (int j = 0, sc = 0; j < n; j++) {
                if (j == nz_i) { continue; }
                sub[sr][sc++] = a0[i][j];}
            sr++;}
        T cof = det(sub);
        if ((nz_i + nz_j) & 1) { cof = -cof; }
        T q = cof / (u[0][nz_i] * v[0][nz_j]); 
        Matrix res(n, n);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) { res[i][j] = u[0][i] * v[0][j] * q; }}
        return res;}
    // T: O(n^3)
    friend vector<T> charPoly(Matrix a) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        assert(a.n == a.m);
        for (int i = 0; i < a.n - 2; i++) {
            int piv = i + 1;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = i + 2; p < a.n; p++) {
                    if (abs(a[p][i]) > abs(a[piv][i])) { piv = p; }}
                if (isNil(a[piv][i])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][i])) { piv++; } }
            if (piv == a.n) { continue; }
            if (piv != i + 1) {
                std::swap_ranges(a[i + 1], a[i + 1] + a.n, a[piv]);
                for (int j = 0; j < a.n; j++) { std::swap(a[j][i + 1], a[j][piv]); }}
            T d = T(1) / a[i + 1][i];
            for (int j = i + 2; j < a.n; j++) {
                if (isNil(a[j][i])) { continue; }
                T a_ji = a[j][i] * d;
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[j] + i, a[i + 1] + i, a_ji, a.n - i);} 
                else
                #endif 
                {   for (int k = i; k < a.n; k++) { a[j][k] -= a_ji * a[i + 1][k]; } }
                for (int k = 0; k < a.n; k++) { a[k][i + 1] += a_ji * a[k][j]; }}}
        vector<vector<T>> res(a.n + 1); res[0] = {T(1)};
        for (int i = 1; i <= a.n; i++) {
            res[i].assign(i + 1, T(0));
            for (int j = 0; j < i; j++) { res[i][j + 1] = res[i - 1][j]; }
            #ifdef __AVX2__
            if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                FastGaussian::elimRow(res[i].data(), res[i - 1].data(), a[i - 1][i - 1], i);} 
            else
            #endif 
            {   for (int j = 0; j < i; j++) { res[i][j] -= a[i - 1][i - 1] * res[i - 1][j]; } }
            T p = 1;
            for (int j = i - 2; j >= 0; j--) {
                p *= a[j + 1][j]; T a_ji = a[j][i - 1] * p;
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(res[i].data(), res[j].data(), a_ji, j + 1);} 
                else
                #endif 
                {   for (int k = 0; k <= j; k++) { res[i][k] -= a_ji * res[j][k]; } }}}
        return res[a.n];}
    // T: O(n^3 * 2^(n/2)), M: O(n^4)
    friend T hafnian(const Matrix &a) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        assert(a.n == a.m && !(a.n & 1));
        if (a.n == 0) { return T(1); }
        int n = a.n / 2; ulll mod = 0;
        if constexpr (requires { T::is_prime; }) { mod = int(T(-1)) + 1; }
        int sj = n + 1, si = a.n * sj, sd = a.n * si;
        vector<T> v((n + 1) * sd, T(0));
        auto getPtr = [&](int d, int i, int j) { return v.data() + d * sd + i * si + j * sj; };
        for (int i = 0; i < a.n; i++) {
            for (int j = 0; j < i; j++) { getPtr(n, i, j)[0] = a[i][j]; }}
        auto solve = [&](auto &&solve, int m) -> vector<T> {
            if (m == 0) { vector<T> res(n + 1, T(0)); res[0] = T(1); return res; }
            int d = m / 2;
            for (int i = 0; i < m - 2; i++) {
                for (int j = 0; j < i; j++) { 
                    T *cur = getPtr(d, i, j); T *nxt = getPtr(d - 1, i, j); std::copy_n(cur, n + 1, nxt);}}
            vector<T> res = solve(solve, m - 2);
            for (int i = 0; i < m - 2; i++) {
                for (int j = 0; j < i; j++) {
                    T *nxt = getPtr(d - 1, i, j);
                    T *a1 = getPtr(d, m - 2, i); T *a2 = getPtr(d, m - 1, i);
                    T *b1 = getPtr(d, m - 1, j); T *b2 = getPtr(d, m - 2, j);
                    for (int k = 0; k < n; k++) {
                        if constexpr (requires { T::is_prime; }) {
                            ulll sum = 0;
                            for (int l = 0; l <= k; l++) {
                                sum += ulll(int(a1[l])) * int(b1[k - l]) + ulll(int(a2[l])) * int(b2[k - l]);}
                            nxt[k + 1] += T(sum % mod);} 
                        else {
                            T sum = 0;
                            for (int l = 0; l <= k; l++) { sum += a1[l] * b1[k - l] + a2[l] * b2[k - l]; }
                            nxt[k + 1] += sum;}}}}
            T *p = getPtr(d, m - 1, m - 2);
            vector<T> tmp = solve(solve, m - 2);
            for (int i = 0; i <= n; i++) {
                if constexpr (requires { T::is_prime; }) {
                    ulll sum = int(tmp[i]);
                    for (int j = 0; j < i; j++) { sum += ulll(int(tmp[j])) * int(p[i - j - 1]); }
                    res[i] = T(sum % mod) - res[i];} 
                else {
                    T sum = tmp[i];
                    for (int j = 0; j < i; j++) { sum += tmp[j] * p[i - j - 1]; }
                    res[i] = sum - res[i];}}
            return res;};
        return solve(solve, a.n)[n];}
    // T: O(n^3)
    friend Matrix intscSpace(const Matrix &a, const Matrix &b) { // Row-major
        assert(a.m == b.m);
        Matrix c(a.n + b.n, 2 * a.m);
        for (int i = 0; i < a.n; i++) {
            std::copy_n(a[i], a.m, c[i]);
            std::copy_n(a[i], a.m, c[i] + a.m);}
        for (int i = 0; i < b.n; i++) { std::copy_n(b[i], a.m, c[a.n + i]); }
        c = rref(std::move(c));
        vector<vector<T>> res;
        for (int i = 0; i < c.n; i++) {
            bool chk = true;
            for (int j = 0; j < a.m; j++) {
                if (!isNil(c[i][j])) { chk = false; break; }}
            if (!chk) { continue; }
            chk = true; vector<T> cur(a.m);
            for (int j = 0; j < a.m; j++) {
                cur[j] = c[i][a.m + j];
                if (!isNil(cur[j])) { chk = false; }}
            if (!chk) { res.push_back(std::move(cur)); }}
        return Matrix(res);}
    // T: O(n^3)
    friend Matrix ker(Matrix a) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        int r = 0; vector<int> pivs(a.m, -1);
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = r + 1; p < a.n; p++) {
                    if (abs(a[p][c]) > abs(a[piv][c])) { piv = p; }}
                if (isNil(a[piv][c])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][c])) { piv++; } }
            if (piv == a.n) { continue; }
            if (r != piv) { std::swap_ranges(a[r], a[r] + a.m, a[piv]); }
            T d = T(1) / a[r][c]; a[r][c] = T(1);
            for (int j = c + 1; j < a.m; j++) { a[r][j] *= d; }
            for (int i = 0; i < a.n; i++) {
                if (i == r || isNil(a[i][c])) { continue; }
                T a_ic = a[i][c]; a[i][c] = T(0);
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[i] + c + 1, a[r] + c + 1, a_ic, a.m - c - 1);} 
                else
                #endif 
                {   for (int j = c + 1; j < a.m; j++) { a[i][j] -= a[r][j] * a_ic; } }}
            pivs[c] = r++;}
        vector<vector<T>> res;
        for (int c = 0; c < a.m; c++) {
            if (pivs[c] == -1) {
                vector<T> cur(a.m, T(0)); cur[c] = T(1);
                for (int j = 0; j < c; j++) { if (pivs[j] != -1) { cur[j] = -a[pivs[j]][c]; } }
                res.push_back(std::move(cur));}}
        return Matrix(res);}
    // T: O(n^3)
    friend int rnk(Matrix a) {
        a = rref<1>(std::move(a));
        int res = 0;
        for (int i = 0; i < a.n; i++) {
            bool chk = false;
            for (int j = 0; j < a.m; j++) { if (!isNil(a[i][j])) { chk = true; break; } }
            if (chk) { res++; }
            else { break; }}
        return res;}
    // T: O(n^3)
    template<bool opt = 0>
    friend Matrix rref(Matrix a) 
    requires (!std::is_integral_v<T>) && (requires { T::is_prime; } || std::is_floating_point_v<T>) {
        int r = 0;
        for (int c = 0; r < a.n && c < a.m; c++) {
            int piv = r;
            if constexpr (std::is_floating_point_v<T>) {
                for (int p = r + 1; p < a.n; p++) {
                    if (abs(a[p][c]) > abs(a[piv][c])) { piv = p; }}
                if (isNil(a[piv][c])) { piv = a.n; }} 
            else { while (piv < a.n && isNil(a[piv][c])) { piv++; } }
            if (piv == a.n) { continue; }
            if (r != piv) { std::swap_ranges(a[r], a[r] + a.m, a[piv]); }
            T d = T(1) / a[r][c]; a[r][c] = T(1);
            for (int j = c + 1; j < a.m; j++) { a[r][j] *= d; }
            for (int i = opt ? r + 1 : 0; i < a.n; i++) {
                if constexpr (opt) { if (isNil(a[i][c])) { continue; } }
                else { if (i == r || isNil(a[i][c])) { continue; } }
                T a_ic = a[i][c]; a[i][c] = T(0);
                #ifdef __AVX2__
                if constexpr ((requires { T::is_prime; } && sizeof(T) == 4) || std::is_floating_point_v<T>) {
                    FastGaussian::elimRow(a[i] + c + 1, a[r] + c + 1, a_ic, a.m - c - 1);} 
                else
                #endif 
                {   for (int j = c + 1; j < a.m; j++) { a[i][j] -= a[r][j] * a_ic; } }}
            r++;}
        return a;}
    
    // T: O(1)
    static bool isNil(const T &x) {
        if constexpr (std::is_floating_point_v<T>) { return abs(x) < 1e-9; }
        else { return x == T(0); }}
    friend bool operator==(const Matrix &a, const Matrix &b) = default;
   
    friend ostream &operator<<(ostream &os, const Matrix &a) {
        if (a.n == 0 || a.m == 0) { return os << "[]"; }
        os << "[";
        for (int i = 0; i < a.n; i++) {
            os << "[";
            for (int j = 0; j < a.m; j++) { os << a[i][j] << (j < a.m - 1 ? ", " : ""); }
            os << "]" << (i < a.n - 1 ? ", " : "");}
        return os << "]";}
};