#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; f is associative with two-sided identity id; B = 0 picks block width m = max(1, floor(sqrt(n))).
// S: O(n), U: O(m), Q: O(m + n / m), M: O(n + 1)
template<typename T, typename F = std::plus<T>>
struct SqrtDecomp {
    int n = 0, m = 1;
    vector<T> v, blks;
    T id;
    F f;

    explicit SqrtDecomp(int N = 0, int B = 0, T id = T{}, F f = F{}) : id(std::move(id)), f(std::move(f)) {
        assert(N >= 0); rebuild(vector<T>(N, this->id), B);}
    explicit SqrtDecomp(vector<T> a, int B = 0, T id = T{}, F f = F{}) : id(std::move(id)), f(std::move(f)) { rebuild(std::move(a), B); }
    SqrtDecomp(std::initializer_list<T> a, int B = 0, T id = T{}, F f = F{}) : SqrtDecomp(vector<T>(a), B, std::move(id), std::move(f)) {}
    void rebuild(vector<T> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); m = B ? B : max(1, int(std::sqrt(n))); v = std::move(a);
        blks.assign(n / m + (n % m != 0), id);
        for (int bi = 0; bi < int(blks.size()); ++bi) { pull(bi); }}

    // Q: O(1) for get, O(n) for values.
    typename vector<T>::const_reference get(int i) const { assert(0 <= i && i < n); return v[i]; }
    vector<T> values() const { return v; }

    void set(int i, T x) { assert(0 <= i && i < n); v[i] = std::move(x); pull(i / m); }
    void setUpdate(int i, T x) { set(i, std::move(x)); }
    void opeUpdate(int i, T x) { assert(0 <= i && i < n); v[i] = f(v[i], x); pull(i / m); }

    T query(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        T res = id;
        while (l < r) {
            int bi = l / m, start = bi * m, end = start + min(m, n - start), stop = min(r, end);
            if (l == start && stop == end) { res = f(res, blks[bi]); }
            else {
                for (int i = l; i < stop; ++i) { res = f(res, v[i]); }}
            l = stop;}
        return res;}

    void pull(int bi) {
        int l = bi * m, r = l + min(m, n - l); blks[bi] = id;
        for (int i = l; i < r; ++i) { blks[bi] = f(blks[bi], v[i]); }}
};

// T is a commutative ring with T(0), T(1), T(length) and ==; affine(l, r, a, b) maps x to a * x + b after pending tags.
// S: O(n), U: O(m + n / m), Q: O(m + n / m), M: O(n + 1)
template<typename T>
struct SqrtRangeSum {
    int n = 0, m = 1;
    vector<T> v, blks, lazy_mul, lazy_add;

    explicit SqrtRangeSum(int N = 0, int B = 0) { assert(N >= 0); rebuild(vector<T>(N, T(0)), B); }
    explicit SqrtRangeSum(vector<T> a, int B = 0) { rebuild(std::move(a), B); }
    SqrtRangeSum(std::initializer_list<T> a, int B = 0) : SqrtRangeSum(vector<T>(a), B) {}
    void rebuild(vector<T> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); m = B ? B : max(1, int(std::sqrt(n))); v = std::move(a);
        int count = n / m + (n % m != 0);
        blks.assign(count, T(0)); lazy_mul.assign(count, T(1)); lazy_add.assign(count, T(0));
        for (int bi = 0; bi < count; ++bi) { pull(bi); }}

    // Q: O(1) for get, O(n) for values.
    T get(int i) const { assert(0 <= i && i < n); return lazy_mul[i / m] * v[i] + lazy_add[i / m]; }
    vector<T> values() const {
        vector<T> res = v;
        for (int bi = 0; bi < int(blks.size()); ++bi) {
            int l = bi * m, r = l + min(m, n - l);
            for (int i = l; i < r; ++i) { res[i] = lazy_mul[bi] * v[i] + lazy_add[bi]; }}
        return res;}

    void affine(int l, int r, T a, T b) {
        assert(0 <= l && l <= r && r <= n);
        while (l < r) {
            int bi = l / m, start = bi * m, end = start + min(m, n - start), stop = min(r, end);
            if (l == start && stop == end) { apply(bi, a, b); }
            else {
                push(bi);
                for (int i = l; i < stop; ++i) { v[i] = a * v[i] + b; }
                pull(bi);}
            l = stop;}}
    void add(int l, int r, T x) { affine(l, r, T(1), x); }
    void assign(int l, int r, T x) { affine(l, r, T(0), x); }
    void multiply(int l, int r, T x) { affine(l, r, x, T(0)); }
    void set(int i, T x) { assert(0 <= i && i < n); assign(i, i + 1, x); }

    T sum(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        T res = T(0);
        while (l < r) {
            int bi = l / m, start = bi * m, end = start + min(m, n - start), stop = min(r, end);
            if (l == start && stop == end) { res = res + blks[bi]; }
            else {
                for (int i = l; i < stop; ++i) { res = res + (lazy_mul[bi] * v[i] + lazy_add[bi]); }}
            l = stop;}
        return res;}

    void apply(int bi, T a, T b) {
        blks[bi] = a * blks[bi] + b * T(min(m, n - bi * m));
        lazy_mul[bi] = a * lazy_mul[bi]; lazy_add[bi] = a * lazy_add[bi] + b;}
    void push(int bi) {
        if (lazy_mul[bi] == T(1) && lazy_add[bi] == T(0)) { return; }
        int l = bi * m, r = l + min(m, n - l);
        for (int i = l; i < r; ++i) { v[i] = lazy_mul[bi] * v[i] + lazy_add[bi]; }
        lazy_mul[bi] = T(1); lazy_add[bi] = T(0);}
    void pull(int bi) {
        int l = bi * m, r = l + min(m, n - l); blks[bi] = T(0);
        for (int i = l; i < r; ++i) { blks[bi] = blks[bi] + v[i]; }}
};
