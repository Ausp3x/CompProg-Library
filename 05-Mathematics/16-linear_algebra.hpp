#pragma once
#include "../01-Core/01-template.hpp"
#include "01-mod_arithmetic.hpp"

template<typename T>
using Mat = vector<vector<T>>;

namespace linear_algebra_detail {
    template<typename T>
    int cols(const Mat<T> &a) { return a.empty() ? 0 : int(a[0].size()); }
    template<typename T>
    bool isZero(const T &x, T eps) {
        if constexpr (std::is_floating_point_v<T>) { return abs(x) <= eps; }
        else { return x == T(0); }}
    // Gauss-Jordan on columns [0, m); rows may be wider and are carried along. Floating T pivots on the largest |x|, others on the first nonzero.
    template<typename T>
    vector<int> echelon(Mat<T> &a, int m, bool full, T &det, T eps = T(0)) {
        int n = int(a.size());
        for (int i = 0; i < n; ++i) { assert(a[i].size() == a[0].size() && int(a[i].size()) >= m); }
        vector<int> piv;
        det = T(1);
        for (int c = 0, r = 0; c < m && r < n; ++c) {
            int p = r;
            if constexpr (std::is_floating_point_v<T>) {
                for (int i = r + 1; i < n; ++i) { if (abs(a[i][c]) > abs(a[p][c])) { p = i; } }}
            else {
                while (p + 1 < n && a[p][c] == T(0)) { ++p; }}
            if (isZero(a[p][c], eps)) { det = T(0); continue; }
            if (p != r) { swap(a[p], a[r]); det = -det; }
            auto &row = a[r];
            int w = int(row.size());
            det *= row[c];
            T inv = T(1) / row[c];
            for (int j = c; j < w; ++j) { row[j] *= inv; }
            for (int i = full ? 0 : r + 1; i < n; ++i) {
                T f = a[i][c];
                if (i == r || f == T(0)) { continue; }
                auto &cur = a[i];
                for (int j = c; j < w; ++j) { cur[j] -= f * row[j]; }}
            piv.push_back(c);
            ++r;}
        return piv;}
    template<typename T>
    Mat<T> kernel(const Mat<T> &a, const vector<int> &piv, int m) {
        Mat<T> res;
        for (int f = 0, k = 0; f < m; ++f) {
            if (k < int(piv.size()) && piv[k] == f) { ++k; continue; }
            vector<T> x(m, T(0));
            x[f] = T(1);
            for (int i = 0; i < k; ++i) { x[piv[i]] = -a[i][f]; }
            res.push_back(x);}
        return res;}
    template<typename T>
    Mat<T> solve(Mat<T> &a, const vector<T> &b, T eps) {
        int n = int(a.size()), m = cols(a);
        assert(int(b.size()) == n);
        for (int i = 0; i < n; ++i) { assert(int(a[i].size()) == m); a[i].push_back(b[i]); }
        T det;
        auto piv = echelon(a, m, true, det, eps);
        int r = int(piv.size());
        for (int i = r; i < n; ++i) { if (!isZero(a[i][m], eps)) { return {}; } }
        Mat<T> res(1, vector<T>(m, T(0)));
        for (int i = 0; i < r; ++i) { res[0][piv[i]] = a[i][m]; }
        for (auto &x : kernel(a, piv, m)) { res.push_back(x); }
        return res;}
    template<typename T>
    void addTo(Mat<T> &s, const Mat<T> &t) {
        for (int i = 0; i < int(s.size()); ++i) { for (int j = 0; j < int(s[i].size()); ++j) { s[i][j] += t[i][j]; } }}
    inline ulng mulMod(ulng a, ulng b, ulng m) { return m >> 32 ? ulng(ulll(a) * b % m) : a * b % m; }
    inline ulng addMod(ulng a, ulng b, ulng m) { a += b; return a >= m ? a - m : a; }
    // Columns [0, n) by unimodular row operations and unit scalings mod m; returns their determinant, stopping at a non-unit pivot when full.
    inline lng eliminateMod(Mat<lng> &a, lng mod, bool full) {
        int n = int(a.size());
        ulng m = mod, det = 1 % m;
        for (int c = 0; c < n; ++c) {
            int w = int(a[c].size()), p = c;
            while (p < n && gcd64(a[p][c], mod) != 1) { ++p; }
            if (p == n) {
                for (int i = c + 1; i < n; ++i) {
                    ulng x = a[c][c], y = a[i][c], p0 = 1 % m, q0 = 0, p1 = 0, q1 = 1 % m;
                    if (!y) { continue; }
                    // Euclid on the column entries, recording the 2 x 2 transform of the two rows.
                    for (; y; det = (m - det) % m) {
                        ulng t = x / y;
                        x -= t * y;
                        p0 = addMod(p0, m - mulMod(t, p1, m), m); q0 = addMod(q0, m - mulMod(t, q1, m), m);
                        swap(x, y); swap(p0, p1); swap(q0, q1);}
                    for (int j = c; j < w; ++j) {
                        ulng u = a[c][j], v = a[i][j];
                        a[c][j] = lng(addMod(mulMod(p0, u, m), mulMod(q0, v, m), m));
                        a[i][j] = lng(addMod(mulMod(p1, u, m), mulMod(q1, v, m), m));}}
                if (gcd64(a[c][c], mod) != 1) {
                    det = mulMod(det, a[c][c], m);
                    if (full || !det) { return lng(det); }
                    continue;}
                p = c;}
            if (p != c) { swap(a[p], a[c]); det = (m - det) % m; }
            auto &row = a[c];
            det = mulMod(det, row[c], m);
            ulng inv = modNorm(extendedGcd(row[c], mod).x, mod);
            for (int j = c; j < w; ++j) { row[j] = lng(mulMod(row[j], inv, m)); }
            for (int i = full ? 0 : c + 1; i < n; ++i) {
                ulng f = a[i][c];
                if (i == c || !f) { continue; }
                f = m - f;
                auto &cur = a[i];
                for (int j = c; j < w; ++j) { cur[j] = lng(addMod(cur[j], mulMod(f, row[j], m), m)); }}}
        return lng(det);}
    inline void normalizeMod(Mat<lng> &a, lng mod) {
        assert(mod >= 1);
        for (auto &row : a) {
            assert(row.size() == a.size());
            for (auto &x : row) { x = modNorm(x, mod); }}}
} // namespace linear_algebra_detail

// T: O(n^2), M: O(n^2); any ring.
template<typename T>
Mat<T> identityMatrix(int n) {
    Mat<T> res(n, vector<T>(n, T(0)));
    for (int i = 0; i < n; ++i) { res[i][i] = T(1); }
    return res;}

// T: O(n * m * k), M: O(n * k); any ring, a is n x m, b is m x k, a Mat without rows is 0 x 0.
template<typename T>
Mat<T> matrixProduct(const Mat<T> &a, const Mat<T> &b) {
    int n = int(a.size()), m = int(b.size()), k = linear_algebra_detail::cols(b);
    for (int j = 0; j < m; ++j) { assert(int(b[j].size()) == k); }
    Mat<T> res(n, vector<T>(k, T(0)));
    for (int i = 0; i < n; ++i) {
        assert(int(a[i].size()) == m);
        auto &row = res[i];
        for (int j = 0; j < m; ++j) {
            T x = a[i][j];
            if (x == T(0)) { continue; }
            for (int l = 0; l < k; ++l) { row[l] += x * b[j][l]; }}}
    return res;}

// T: O(n * m * (r + 1)), M: O(m); exact field, a is n x m of rank r; reduces a in place to reduced row echelon form, returns the pivot columns.
template<typename T>
vector<int> rref(Mat<T> &a) {
    static_assert(!std::is_floating_point_v<T>);
    T det;
    return linear_algebra_detail::echelon(a, linear_algebra_detail::cols(a), true, det);}

// T: O(n * m * min(n, m)), M: O(n * m); exact field; rank, and the determinant of a square matrix.
template<typename T>
int matrixRank(Mat<T> a) {
    static_assert(!std::is_floating_point_v<T>);
    T det;
    return int(linear_algebra_detail::echelon(a, linear_algebra_detail::cols(a), false, det).size());}
template<typename T>
T determinant(Mat<T> a) {
    static_assert(!std::is_floating_point_v<T>);
    for (int i = 0; i < int(a.size()); ++i) { assert(a[i].size() == a.size()); }
    T det;
    linear_algebra_detail::echelon(a, int(a.size()), false, det);
    return det;}

// T: O(n^3 + n^2 * log(mod)), M: O(n^2); mod in [1, 2^63), all lng entries; determinant in [0, mod).
inline lng determinantMod(Mat<lng> a, lng mod) {
    linear_algebra_detail::normalizeMod(a, mod);
    return linear_algebra_detail::eliminateMod(a, mod, false);}

// T: O(n^3) operations on T, M: O(n^2); integer T with exact division, the product of any two minors must fit T.
template<typename T>
T determinantInteger(Mat<T> a) {
    int n = int(a.size());
    T prev = T(1);
    bool neg = false;
    for (int k = 0; k < n; ++k) {
        assert(int(a[k].size()) == n);
        int p = k;
        while (p < n && a[p][k] == T(0)) { ++p; }
        if (p == n) { return T(0); }
        if (p != k) { swap(a[p], a[k]); neg = !neg; }
        // Bareiss: every entry stays a minor of the original, so the division is exact.
        for (int i = k + 1; i < n; ++i) {
            for (int j = k + 1; j < n; ++j) { a[i][j] = (a[k][k] * a[i][j] - a[i][k] * a[k][j]) / prev; }}
        prev = a[k][k];}
    return neg ? -prev : prev;}

// T: O(n^3), M: O(n^2); exact field, n x n; replaces a by its inverse, false (a unspecified) when singular.
template<typename T>
bool inverse(Mat<T> &a) {
    static_assert(!std::is_floating_point_v<T>);
    int n = int(a.size());
    for (int i = 0; i < n; ++i) {
        assert(int(a[i].size()) == n);
        a[i].resize(2 * n, T(0));
        a[i][n + i] = T(1);}
    T det;
    if (int(linear_algebra_detail::echelon(a, n, true, det).size()) < n) { return false; }
    for (auto &row : a) { row.erase(row.begin(), row.begin() + n); }
    return true;}

// T: O(n^3 + n^2 * log(mod)), M: O(n^2); mod in [1, 2^63), all lng entries; replaces a by its inverse in [0, mod), false (a unspecified) when the determinant is not a unit.
inline bool inverseMod(Mat<lng> &a, lng mod) {
    linear_algebra_detail::normalizeMod(a, mod);
    int n = int(a.size());
    for (int i = 0; i < n; ++i) {
        a[i].resize(2 * n, 0);
        a[i][n + i] = 1 % mod;}
    if (gcd64(linear_algebra_detail::eliminateMod(a, mod, true), mod) != 1) { return false; }
    for (auto &row : a) { row.erase(row.begin(), row.begin() + n); }
    return true;}

// T: O(n^3), M: O(n^2); exact field, n x n of any rank.
template<typename T>
Mat<T> adjugate(const Mat<T> &a) {
    static_assert(!std::is_floating_point_v<T>);
    int n = int(a.size());
    Mat<T> e = a, res(n, vector<T>(n, T(0)));
    for (int i = 0; i < n; ++i) {
        assert(int(e[i].size()) == n);
        e[i].resize(2 * n, T(0));
        e[i][n + i] = T(1);}
    T det;
    auto piv = linear_algebra_detail::echelon(e, n, true, det);
    int r = int(piv.size());
    if (r == n) {
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { res[i][j] = det * e[i][n + j]; } }
        return res;}
    if (r < n - 1) { return res; }
    // Rank n - 1: adj = c x y^T with A x = 0 and y^T A = 0 (the last row of the transform); one cofactor fixes c.
    auto x = linear_algebra_detail::kernel(e, piv, n)[0];
    const auto &y = e[n - 1];
    int i = 0, j = 0;
    while (y[n + i] == T(0)) { ++i; }
    while (x[j] == T(0)) { ++j; }
    Mat<T> minor;
    for (int k = 0; k < n; ++k) {
        if (k == i) { continue; }
        minor.push_back(a[k]);
        minor.back().erase(minor.back().begin() + j);}
    T c = determinant(minor) / (x[j] * y[n + i]);
    if ((i + j) % 2) { c = -c; }
    for (int k = 0; k < n; ++k) { for (int l = 0; l < n; ++l) { res[k][l] = c * x[k] * y[n + l]; } }
    return res;}

// T: O(n * m * min(n, m)), M: O(n * m + m * (m - r)); exact field, a is n x m of rank r, |b| = n; empty when A x = b has no solution, else row 0 is one solution and rows 1.. a nullspace basis.
template<typename T>
Mat<T> solveLinear(Mat<T> a, const vector<T> &b) {
    static_assert(!std::is_floating_point_v<T>);
    return linear_algebra_detail::solve(a, b, T(0));}

// T: O(n * (m + k) * min(n, m)), M: O(n * (m + k) + m * k); exact field, a is n x m, b is n x k; x (m x k) with A x = B, false (x untouched) when a column has no solution.
template<typename T>
bool solveLinearMultiRhs(Mat<T> a, const Mat<T> &b, Mat<T> &x) {
    static_assert(!std::is_floating_point_v<T>);
    int n = int(a.size()), m = linear_algebra_detail::cols(a), k = linear_algebra_detail::cols(b);
    assert(int(b.size()) == n);
    for (int i = 0; i < n; ++i) {
        assert(int(a[i].size()) == m && int(b[i].size()) == k);
        a[i].insert(a[i].end(), b[i].begin(), b[i].end());}
    T det;
    auto piv = linear_algebra_detail::echelon(a, m, true, det);
    int r = int(piv.size());
    for (int i = r; i < n; ++i) {
        for (int j = m; j < m + k; ++j) { if (a[i][j] != T(0)) { return false; } }}
    x.assign(m, vector<T>(k, T(0)));
    for (int i = 0; i < r; ++i) { std::copy(a[i].begin() + m, a[i].end(), x[piv[i]].begin()); }
    return true;}

// T: O(n * m * min(n, m)), M: O(n * m + m * (m - r)); exact field, a is n x m of rank r; a basis of {x : A x = 0}, empty when only x = 0.
template<typename T>
Mat<T> nullspace(Mat<T> a) {
    static_assert(!std::is_floating_point_v<T>);
    int m = linear_algebra_detail::cols(a);
    T det;
    auto piv = linear_algebra_detail::echelon(a, m, true, det);
    return linear_algebra_detail::kernel(a, piv, m);}

// T: O(n^3), M: O(n); floating T, n x n; Crout in place, row i of L U is row perm[i] of A (L keeps the diagonal, U is unit upper); false when a pivot has |x| <= eps.
template<typename T>
bool luDecompose(Mat<T> &a, vector<int> &perm, T eps = T(1e-9)) {
    static_assert(std::is_floating_point_v<T>);
    int n = int(a.size());
    perm.resize(n);
    iota(perm.begin(), perm.end(), 0);
    for (int j = 0; j < n; ++j) {
        assert(int(a[j].size()) == n);
        for (int i = j; i < n; ++i) {
            T s = a[i][j];
            for (int k = 0; k < j; ++k) { s -= a[i][k] * a[k][j]; }
            a[i][j] = s;}
        int p = j;
        for (int i = j + 1; i < n; ++i) { if (abs(a[i][j]) > abs(a[p][j])) { p = i; } }
        if (abs(a[p][j]) <= eps) { return false; }
        swap(a[p], a[j]); swap(perm[p], perm[j]);
        for (int i = j + 1; i < n; ++i) {
            T s = a[j][i];
            for (int k = 0; k < j; ++k) { s -= a[j][k] * a[k][i]; }
            a[j][i] = s / a[j][j];}}
    return true;}

// T: O(n^2), M: O(n); a and perm from a successful luDecompose, |b| = n; x with A x = b.
template<typename T>
vector<T> luSolve(const Mat<T> &a, const vector<int> &perm, const vector<T> &b) {
    int n = int(a.size());
    assert(int(b.size()) == n);
    vector<T> x(n);
    for (int i = 0; i < n; ++i) {
        T s = b[perm[i]];
        for (int k = 0; k < i; ++k) { s -= a[i][k] * x[k]; }
        x[i] = s / a[i][i];}
    for (int i = n - 1; i >= 0; --i) {
        for (int k = i + 1; k < n; ++k) { x[i] -= a[i][k] * x[k]; }}
    return x;}

// T: O(n^3 * log(e)), M: O(n^2); any ring, n x n; a^e with a^0 = I, and the sum of a^i over 0 <= i < e.
template<typename T>
Mat<T> matrixPower(Mat<T> a, ulng e) {
    Mat<T> res = identityMatrix<T>(int(a.size()));
    for (; e; e >>= 1) {
        if (e & 1) { res = matrixProduct(res, a); }
        if (e > 1) { a = matrixProduct(a, a); }}
    return res;}
template<typename T>
Mat<T> matrixPowerSum(const Mat<T> &a, ulng e) {
    int n = int(a.size());
    if (!e) { return Mat<T>(n, vector<T>(n, T(0))); }
    // s = sum of a^i over i < k and p = a^k, from k = 1 along the bits of e.
    Mat<T> s = identityMatrix<T>(n), p = a;
    for (int i = int(std::bit_width(e)) - 2; i >= 0; --i) {
        linear_algebra_detail::addTo(s, matrixProduct(p, s));
        p = matrixProduct(p, p);
        if (e >> i & 1) {
            linear_algebra_detail::addTo(s, p);
            p = matrixProduct(p, a);}}
    return s;}

// T: O(n), M: O(n); a[i] x[i-1] + b[i] x[i] + c[i] x[i+1] = d[i], cyclic indices for the cyclic form; d becomes x; no pivoting, false (d unspecified) on a zero pivot.
template<typename T>
bool tridiagonalSolve(vector<T> a, vector<T> b, vector<T> c, vector<T> &d) {
    int n = int(d.size());
    assert(int(a.size()) == n && int(b.size()) == n && int(c.size()) == n);
    for (int i = 1; i < n; ++i) {
        if (b[i - 1] == T(0)) { return false; }
        T f = a[i] / b[i - 1];
        b[i] -= f * c[i - 1];
        d[i] -= f * d[i - 1];}
    if (n && b[n - 1] == T(0)) { return false; }
    for (int i = n - 1; i >= 0; --i) { d[i] = (i + 1 < n ? d[i] - c[i] * d[i + 1] : d[i]) / b[i]; }
    return true;}
template<typename T>
bool cyclicTridiagonalSolve(vector<T> a, vector<T> b, vector<T> c, vector<T> &d) {
    int n = int(d.size());
    assert(int(a.size()) == n && int(b.size()) == n && int(c.size()) == n);
    if (n == 1) { b[0] += a[0] + c[0]; }
    if (n == 2) { c[0] += a[0]; a[1] += c[1]; }
    if (n <= 2) { return tridiagonalSolve(a, b, c, d); }
    // Rows 0 .. n-2 keep the last column in e; the last row's fill g walks along it.
    vector<T> e(n, T(0));
    e[0] = a[0];
    for (int i = 1; i < n - 1; ++i) {
        if (b[i - 1] == T(0)) { return false; }
        T f = a[i] / b[i - 1];
        b[i] -= f * c[i - 1];
        e[i] -= f * e[i - 1];
        d[i] -= f * d[i - 1];}
    e[n - 2] += c[n - 2];
    T g = c[n - 1];
    for (int p = 0; p < n - 1; ++p) {
        if (b[p] == T(0)) { return false; }
        T f = g / b[p];
        b[n - 1] -= f * e[p];
        d[n - 1] -= f * d[p];
        g = (p + 3 == n ? a[n - 1] : T(0)) - f * c[p];}
    if (b[n - 1] == T(0)) { return false; }
    d[n - 1] /= b[n - 1];
    d[n - 2] = (d[n - 2] - e[n - 2] * d[n - 1]) / b[n - 2];
    for (int i = n - 3; i >= 0; --i) { d[i] = (d[i] - c[i] * d[i + 1] - e[i] * d[n - 1]) / b[i]; }
    return true;}

// T: O(n * (p + 1) * (q + 1)), M: O(n * (p + q)); lower and upper bandwidths p, q, a[i][j - i + p] = A[i][j] for -p <= j - i <= q, rows of p + q + 1; d becomes x; no pivoting, false (d unspecified) on a zero pivot.
template<typename T>
bool bandedSolve(Mat<T> a, int p, int q, vector<T> &d) {
    int n = int(d.size());
    assert(int(a.size()) == n && p >= 0 && q >= 0);
    for (int i = 0; i < n; ++i) { assert(int(a[i].size()) == p + q + 1); }
    for (int k = 0; k < n; ++k) {
        if (a[k][p] == T(0)) { return false; }
        for (int i = k + 1; i <= min(n - 1, k + p); ++i) {
            T f = a[i][k - i + p] / a[k][p];
            if (f == T(0)) { continue; }
            for (int j = k; j <= min(n - 1, k + q); ++j) { a[i][j - i + p] -= f * a[k][j - k + p]; }
            d[i] -= f * d[k];}}
    for (int k = n - 1; k >= 0; --k) {
        for (int j = k + 1; j <= min(n - 1, k + q); ++j) { d[k] -= a[k][j - k + p] * d[j]; }
        d[k] /= a[k][p];}
    return true;}

// T: O(n * m * min(n, m)), M: O(n * m + m * (m - r)); floating T, numerical rank r, partial pivoting, |x| <= eps counts as zero; result shape of solveLinear.
template<typename T>
Mat<T> solveLinearReal(Mat<T> a, const vector<T> &b, T eps = T(1e-9)) {
    static_assert(std::is_floating_point_v<T>);
    return linear_algebra_detail::solve(a, b, eps);}

// T: O(n * m * min(n, m)), M: O(n * m); floating T, partial pivoting; pivots with |x| <= eps count as zero, determinant without tolerance.
template<typename T>
int matrixRankReal(Mat<T> a, T eps = T(1e-9)) {
    static_assert(std::is_floating_point_v<T>);
    T det;
    return int(linear_algebra_detail::echelon(a, linear_algebra_detail::cols(a), false, det, eps).size());}
template<typename T>
T determinantReal(Mat<T> a) {
    static_assert(std::is_floating_point_v<T>);
    for (int i = 0; i < int(a.size()); ++i) { assert(a[i].size() == a.size()); }
    T det;
    linear_algebra_detail::echelon(a, int(a.size()), false, det);
    return det;}

// T: O(n^2 * k + k^3), M: O(n^2 + k^2); exact field, ainv = A^-1 (n x n), u n x k, v k x n; ainv becomes (A + U V)^-1, false (ainv untouched) when it is singular.
template<typename T>
bool lowRankUpdateInverse(Mat<T> &ainv, const Mat<T> &u, const Mat<T> &v) {
    int n = int(ainv.size()), k = int(v.size());
    assert(int(u.size()) == n);
    for (int i = 0; i < n; ++i) { assert(int(u[i].size()) == k); }
    for (int i = 0; i < k; ++i) { assert(int(v[i].size()) == n); }
    if (!n || !k) { return true; }
    Mat<T> w = matrixProduct(v, ainv), z = matrixProduct(ainv, u), s = matrixProduct(w, u);
    for (int i = 0; i < k; ++i) { s[i][i] += T(1); }
    if (!inverse(s)) { return false; }
    auto d = matrixProduct(matrixProduct(z, s), w);
    for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { ainv[i][j] -= d[i][j]; } }
    return true;}

// S: O(1), U: O(r * (m + r)), Q: O(r * (m + r)), M: O(r * (m + r)); exact field, vectors of length m, r = size().
// insert false when v is dependent; represent false when v is outside the span, else coefficients over the inserted vectors in order.
template<typename T>
struct FieldBasis {
    int m;
    Mat<T> b, comb;
    vector<int> piv;

    FieldBasis(int m) : m(m) { static_assert(!std::is_floating_point_v<T>); }

    int size() const { return int(b.size()); }

    bool insert(vector<T> v) {
        vector<T> c = reduce(v);
        int p = 0;
        while (p < m && v[p] == T(0)) { ++p; }
        if (p == m) { return false; }
        T inv = T(1) / v[p];
        for (auto &x : v) { x *= inv; }
        for (auto &x : c) { x = -x * inv; }
        c.push_back(inv);
        b.push_back(v); comb.push_back(c); piv.push_back(p);
        return true;}
    void rollback(int r) {
        assert(0 <= r && r <= size());
        r = max(0, min(r, size()));
        b.resize(r); comb.resize(r); piv.resize(r);}

    bool represent(vector<T> v, vector<T> &coef) const {
        coef = reduce(v);
        for (auto &x : v) { if (x != T(0)) { return false; } }
        return true;}
    // Subtracts basis vectors from v until it is zero at every pivot; returns the coefficients removed.
    vector<T> reduce(vector<T> &v) const {
        assert(int(v.size()) == m);
        vector<T> c(size(), T(0));
        for (int i = 0; i < size(); ++i) {
            T f = v[piv[i]];
            if (f == T(0)) { continue; }
            for (int j = piv[i]; j < m; ++j) { v[j] -= f * b[i][j]; }
            for (int j = 0; j <= i; ++j) { c[j] += f * comb[i][j]; }}
        return c;}
};

// T: O(rounds * (n * m + m * k + n * k)), M: O(n + m + k); prime field of size p or overflow-free integers; false proves A B != C, true errs with probability <= (2 / min(p, 2^31))^rounds.
template<typename T>
bool freivaldsProductCheck(const Mat<T> &a, const Mat<T> &b, const Mat<T> &c, int rounds = 2, ulng seed = 0) {
    int k = linear_algebra_detail::cols(c);
    assert(a.size() == c.size());
    if (c.empty()) { return true; }
    std::mt19937_64 rng(seed);
    auto apply = [&](const Mat<T> &x, const vector<T> &v) {
        vector<T> res(x.size(), T(0));
        for (int i = 0; i < int(x.size()); ++i) {
            assert(x[i].size() == v.size());
            for (int j = 0; j < int(v.size()); ++j) { res[i] += x[i][j] * v[j]; }}
        return res;};
    while (rounds-- > 0) {
        vector<T> r(k);
        for (auto &x : r) { x = T(lng(rng() >> 33)); }
        if (apply(a, apply(b, r)) != apply(c, r)) { return false; }}
    return true;}

// T: O(rounds * (vars + E)), M: O(vars); E one evaluation of f: vector<T> -> T, a polynomial of total degree d over a prime field of size p; false proves f != 0, true errs with probability <= (2 d / min(p, 2^31))^rounds.
template<typename T, typename F>
bool schwartzZippelIdentityCheck(F f, int vars, int rounds = 2, ulng seed = 0) {
    std::mt19937_64 rng(seed);
    vector<T> x(vars);
    while (rounds-- > 0) {
        for (auto &v : x) { v = T(lng(rng() >> 33)); }
        if (f(x) != T(0)) { return false; }}
    return true;}
