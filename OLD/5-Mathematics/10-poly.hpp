#pragma once
#include "../1-Core/01-template.hpp"

#ifdef __AVX2__
#include <immintrin.h>
namespace FastPoly {
    static constexpr double pi = 3.14159265358979323846;
    
    inline void butterfly(__m256d &r0, __m256d &i0, __m256d &r1, __m256d &i1, const __m256d &wr, const __m256d &wi) {
        __m256d tr = _mm256_sub_pd(_mm256_mul_pd(r1, wr), _mm256_mul_pd(i1, wi));
        __m256d ti = _mm256_add_pd(_mm256_mul_pd(r1, wi), _mm256_mul_pd(i1, wr));
        r1 = _mm256_sub_pd(r0, tr); i1 = _mm256_sub_pd(i0, ti);
        r0 = _mm256_add_pd(r0, tr); i0 = _mm256_add_pd(i0, ti);}
    inline void fft(vector<double> &re, vector<double> &im, bool is_inv) {
        int n = re.size();
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) { j ^= bit; }
            j ^= bit;
            if (i < j) { std::swap(re[i], re[j]); std::swap(im[i], im[j]); }}
        static vector<double> Wr = {0, 1}, Wi = {0, 0};
        if (Wr.size() < n) {
            int m = Wr.size(); Wr.resize(n); Wi.resize(n);
            for (int len = m; len < n; len <<= 1) {
                for (int i = 0; i < len; i++) {
                    Wr[len + i] = std::cos(pi * i / len);
                    Wi[len + i] = std::sin(pi * i / len);}}}
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                int j = 0;
                for (; j + 3 < len; j += 4) {
                    __m256d r0 = _mm256_loadu_pd(&re[i + j]),       i0 = _mm256_loadu_pd(&im[i + j]);
                    __m256d r1 = _mm256_loadu_pd(&re[i + j + len]), i1 = _mm256_loadu_pd(&im[i + j + len]);
                    __m256d wr = _mm256_loadu_pd(&Wr[len + j]),     wi = _mm256_loadu_pd(&Wi[len + j]);
                    butterfly(r0, i0, r1, i1, wr, wi);
                    _mm256_storeu_pd(&re[i + j], r0);       _mm256_storeu_pd(&im[i + j], i0);
                    _mm256_storeu_pd(&re[i + j + len], r1); _mm256_storeu_pd(&im[i + j + len], i1);}
                for (; j < len; j++) {
                    double tr = re[i + j + len] * Wr[len + j] - im[i + j + len] * Wi[len + j];
                    double ti = re[i + j + len] * Wi[len + j] + im[i + j + len] * Wr[len + j];
                    re[i + j + len] = re[i + j] - tr; im[i + j + len] = im[i + j] - ti;
                    re[i + j] += tr;                  im[i + j] += ti;}}}
        if (is_inv) {
            reverse(re.begin() + 1, re.end()); reverse(im.begin() + 1, im.end());
            int i = 0; double n_inv = 1.0 / n; __m256d v_inv = _mm256_set1_pd(n_inv);
            for (; i + 3 < n; i += 4) {
                _mm256_storeu_pd(&re[i], _mm256_mul_pd(_mm256_loadu_pd(&re[i]), v_inv));
                _mm256_storeu_pd(&im[i], _mm256_mul_pd(_mm256_loadu_pd(&im[i]), v_inv));}
            for (; i < n; i++) { re[i] *= n_inv; im[i] *= n_inv; }}}}
#endif

// T: O(n) or O(n * log(n)), M: O(n)
struct Poly {
    using cld = std::complex<long double>;
    static constexpr long double pi = 3.14159265358979323846;

    vector<lng> v; // ascending powers
    Poly(int n = 1) : v(max(n, 1), 0) {}
    Poly(const vector<lng> &a) : v(a) { trim(); }
    Poly(vector<lng> &&a) noexcept : v(std::move(a)) { trim(); }
    
    lng &operator[](int i) { return v[i]; }
    lng operator[](int i) const { return v[i]; }
    int deg() const { return v.size() - 1; }
    lng lead() const { return v.empty() ? 0 : v.back(); }    
    int size() const { return v.size(); }
    void trim() {
        while (v.size() > 1 && v.back() == 0) { v.pop_back(); }
        if (v.empty()) { v.push_back(0); }}

    Poly &operator+=(const Poly &o) {
        if (o.size() > size()) { v.resize(o.size(), 0); }
        for (int i = 0; i < o.size(); i++) { v[i] += o[i]; }
        trim();
        return *this;}
    Poly &operator-=(const Poly &o) {
        if (o.size() > size()) { v.resize(o.size(), 0); }
        for (int i = 0; i < o.size(); i++) { v[i] -= o[i]; }
        trim();
        return *this;}
    Poly &operator*=(lng o) {
        if (o == 0) { v = {0}; return *this; }
        for (lng &x : v) { x *= o; } 
        trim(); 
        return *this;}
    static void fft(vector<cld> &a, bool is_inv) {
        int n = a.size();
        for (int i = 1, j = 0; i < n; i++) {
            int bit = n >> 1;
            for (; j & bit; bit >>= 1) { j ^= bit; }
            j ^= bit;
            if (i < j) { std::swap(a[i], a[j]); }}
        static vector<cld> w = {cld(0, 0), cld(1, 0)};
        if (w.size() < n) {
            int m = w.size(); w.resize(n);
            for (int len = m; len < n; len <<= 1) {
                for (int i = 0; i < len; i++) {
                    w[len + i] = cld(std::cos(pi * i / len), std::sin(pi * i / len));}}}
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    cld u = a[i + j];
                    cld v = a[i + j + len] * w[len + j];
                    a[i + j]       = u + v;
                    a[i + j + len] = u - v;}}}
        if (is_inv) {
            reverse(a.begin() + 1, a.end());
            long double n_inv = 1.0L / n;
            for (cld &x : a) { x *= n_inv; }}}
    Poly &operator*=(const Poly &o) {
        if (isNil() || o.isNil()) { v = {0}; return *this; }
        if (size() == 1) {
            lng v0 = v[0]; v = o.v;
            for (auto &x : v) { x *= v0; }
            trim();
            return *this;}
        if (o.size() == 1) {
            for (lng &x : v) { x *= o[0]; }
            trim();
            return *this;}
        int n = 1;
        while (n < size() + o.size() - 1) { n <<= 1; }
        #ifdef __AVX2__
        static vector<double> Ra, Ia, Rb, Ib; 
        Ra.assign(v.begin(), v.end());     Ra.resize(n, 0); Ia.assign(n, 0);
        Rb.assign(o.v.begin(), o.v.end()); Rb.resize(n, 0); Ib.assign(n, 0);
        FastPoly::fft(Ra, Ia, false); FastPoly::fft(Rb, Ib, false);
        int i = 0;
        for (; i + 3 < n; i += 4) {
            __m256d ra = _mm256_loadu_pd(&Ra[i]), ia = _mm256_loadu_pd(&Ia[i]);
            __m256d rb = _mm256_loadu_pd(&Rb[i]), ib = _mm256_loadu_pd(&Ib[i]);
            __m256d tr = _mm256_sub_pd(_mm256_mul_pd(ra, rb), _mm256_mul_pd(ia, ib));
            __m256d ti = _mm256_add_pd(_mm256_mul_pd(ra, ib), _mm256_mul_pd(ia, rb));
            _mm256_storeu_pd(&Ra[i], tr); _mm256_storeu_pd(&Ia[i], ti);}
        for (; i < n; i++) {
            double tr = Ra[i] * Rb[i] - Ia[i] * Ib[i];
            double ti = Ra[i] * Ib[i] + Ia[i] * Rb[i];
            Ra[i] = tr; Ia[i] = ti;}
        FastPoly::fft(Ra, Ia, true); v.resize(n);
        for (int i = 0; i < n; i++) { v[i] = std::llround(Ra[i]); }
        #else
        static vector<cld> fa, fb; 
        fa.assign(v.begin(), v.end());     fa.resize(n);
        fb.assign(o.v.begin(), o.v.end()); fb.resize(n);
        fft(fa, false); fft(fb, false);
        for (int i = 0; i < n; i++) { fa[i] *= fb[i]; }
        fft(fa, true); v.resize(n);
        for (int i = 0; i < n; i++) { v[i] = std::llround(fa[i].real()); }
        #endif
        trim();
        return *this;}

    Poly operator+() const { return *this; }
    Poly operator-() const { Poly res = *this; for (lng &x : res.v) { x = -x; } return res; }
    friend Poly operator+(Poly a, const Poly &b) { a += b; return a; }
    friend Poly operator-(Poly a, const Poly &b) { a -= b; return a; }
    friend Poly operator*(Poly a, lng b) { a *= b; return a; }
    friend Poly operator*(lng b, Poly a) { a *= b; return a; }
    friend Poly operator*(Poly a, const Poly &b) { a *= b; return a; }

    friend lng eval(const Poly &a, lng x) {
        lng res = 0;
        for (int i = a.size() - 1; i >= 0; i--) { res = res * x + a[i]; }
        return res;}
    // T: O(n * log(n)^2)
    friend Poly pow(Poly a, lng b, int deg) {
        assert(b >= 0);
        Poly res({1});
        while (b > 0) {
            if (b & 1) {
                res *= a;
                if (res.size() > deg + 1) { res.v.resize(deg + 1); }
                res.trim();}
            b >>= 1;
            if (b == 0) { break; }
            a *= a;
            if (a.size() > deg + 1) { a.v.resize(deg + 1); }
            a.trim();}
        return res;}
        
    // T: O(1)
    bool isNil() const { return v.empty() || (v.size() == 1 && v[0] == 0); }    
    bool operator==(const Poly &o) const { return v == o.v; }
    bool operator!=(const Poly &o) const { return !(*this == o); }

    friend ostream &operator<<(ostream &os, const Poly &a) {
        if (a.size() == 0) { return os << "[]"; }
        os << "[";
        for (int i = 0; i < a.size(); i++) { os << a[i] << (i < a.size() - 1 ? ", " : ""); }
        return os << "]";}
};