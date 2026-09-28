#pragma once

#include "../01-Core/01-template.hpp"

// Associative f with two-sided identity id, preserving left-to-right operand order.
// 0 <= n <= INT_MAX; zero-based points, half-open ranges, empty query returns id.
// B=0 chooses max(1,floor(sqrt(n))); positive B is an explicit block width m.
// Public state/helpers are implementation storage; mutate through operations only.
// Costs count T/f operations as O(1), whose arithmetic must be valid.
// S: O(n), U: O(m), Q: O(m + n / m), M: O(n + 1)
template<typename T, typename F = std::plus<T>>
struct SqrtDecomp {
    int n = 0, m = 1;
    vector<T> v, blks;
    T id;
    F f;

    SqrtDecomp(int N = 0, int B = 0, T ID = T{}, F f_ = F{}) : id(std::move(ID)), f(std::move(f_)) {
        assert(N >= 0); rebuild(vector<T>(N,id),B);
    }
    SqrtDecomp(vector<T> a, int B = 0, T ID = T{}, F f_ = F{}) : id(std::move(ID)), f(std::move(f_)) {
        rebuild(std::move(a),B);
    }
    // O(n), replaces all points; B selects a fresh block width as in construction.
    void rebuild(vector<T> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); m = B ? B : max(1,int(std::sqrt(n))); v = std::move(a);
        blks.assign(n / m + (n % m != 0),id);
        for (int bi = 0; bi < int(blks.size()); ++bi) { pull(bi); }
    }
    void pull(int bi) {
        assert(0 <= bi && bi < int(blks.size()));
        int l = bi*m, r = l+min(m,n-l); blks[bi] = id;
        for (int i = l; i < r; ++i) { blks[bi] = f(blks[bi],v[i]); }
    }

    const T &get(int i) const { assert(0 <= i && i < n); return v[i]; } // O(1).
    vector<T> values() const { return v; } // O(n) time and returned storage.
    void set(int i, T x) { assert(0 <= i && i < n); v[i] = std::move(x); pull(i/m); }
    void setUpdate(int i, T x) { set(i,std::move(x)); }
    void opeUpdate(int i, T x) { assert(0 <= i && i < n); v[i] = f(v[i],x); pull(i/m); }
    T query(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        T res = id;
        while (l < r) {
            int bi = l/m, start = bi*m, end = start+min(m,n-start), stop = min(r,end);
            if (l == start && stop == end) { res = f(res,blks[bi]); }
            else { for (int i = l; i < stop; ++i) { res = f(res,v[i]); }}
            l = stop;}
        return res;
    }
};

// Scalar sum specialization: T is a commutative ring with T(0), T(1), T(length),
// +, *, and ==. Every intermediate (including pending tags) must fit T, unless T
// is explicitly modular. Points/ranges/block widths match SqrtDecomp above.
// affine(l,r,a,b) applies x <- a*x+b. New tags compose AFTER pending tags:
// (a,b) after (c,d) is (a*c,a*d+b). Assignment uses a=0, with no inverse needed.
// State/helpers are implementation storage; pull requires already-pushed tags.
// S: O(n), U: O(m + n / m), Q: O(m + n / m), M: O(n + 1), in T operations.
template<typename T>
struct SqrtRangeSum {
    int n = 0, m = 1;
    vector<T> v, blks, lazy_mul, lazy_add;

    explicit SqrtRangeSum(int N = 0, int B = 0) {
        assert(N >= 0); rebuild(vector<T>(N,T(0)),B);
    }
    explicit SqrtRangeSum(vector<T> a, int B = 0) { rebuild(std::move(a),B); }
    // O(n), replaces points and clears pending actions; B selects a fresh width.
    void rebuild(vector<T> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); m = B ? B : max(1,int(std::sqrt(n))); v = std::move(a);
        int count = n/m + (n%m != 0);
        blks.assign(count,T(0)); lazy_mul.assign(count,T(1)); lazy_add.assign(count,T(0));
        for (int bi = 0; bi < count; ++bi) { pull(bi); }
    }
    void apply(int bi, T a, T b) {
        assert(0 <= bi && bi < int(blks.size()));
        blks[bi] = a*blks[bi]+b*T(min(m,n-bi*m));
        lazy_mul[bi] = a*lazy_mul[bi]; lazy_add[bi] = a*lazy_add[bi]+b;
    }
    void push(int bi) {
        assert(0 <= bi && bi < int(blks.size()));
        if (lazy_mul[bi] == T(1) && lazy_add[bi] == T(0)) { return; }
        int l = bi*m, r = l+min(m,n-l);
        for (int i = l; i < r; ++i) { v[i] = lazy_mul[bi]*v[i]+lazy_add[bi]; }
        lazy_mul[bi] = T(1); lazy_add[bi] = T(0);
    }
    void pull(int bi) {
        assert(0 <= bi && bi < int(blks.size()));
        int l = bi*m, r = l+min(m,n-l); blks[bi] = T(0);
        for (int i = l; i < r; ++i) { blks[bi] = blks[bi]+v[i]; }
    }

    T get(int i) const {
        assert(0 <= i && i < n); return lazy_mul[i/m]*v[i]+lazy_add[i/m];
    } // O(1).
    vector<T> values() const { // O(n) time and returned storage; leaves tags intact.
        vector<T> res = v;
        for (int bi = 0; bi < int(blks.size()); ++bi) {
            int l = bi*m, r = l+min(m,n-l);
            for (int i = l; i < r; ++i) { res[i] = lazy_mul[bi]*v[i]+lazy_add[bi]; }}
        return res;
    }
    T sum(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        T res = T(0);
        while (l < r) {
            int bi = l/m, start = bi*m, end = start+min(m,n-start), stop = min(r,end);
            if (l == start && stop == end) { res = res+blks[bi]; }
            else {
                for (int i = l; i < stop; ++i) { res = res+(lazy_mul[bi]*v[i]+lazy_add[bi]); }}
            l = stop;}
        return res;
    }

    void affine(int l, int r, T a, T b) {
        assert(0 <= l && l <= r && r <= n);
        while (l < r) {
            int bi = l/m, start = bi*m, end = start+min(m,n-start), stop = min(r,end);
            if (l == start && stop == end) { apply(bi,a,b); }
            else {
                push(bi);
                for (int i = l; i < stop; ++i) { v[i] = a*v[i]+b; }
                pull(bi);}
            l = stop;}
    }
    void add(int l, int r, T x) { affine(l,r,T(1),x); }
    void assign(int l, int r, T x) { affine(l,r,T(0),x); }
    void multiply(int l, int r, T x) { affine(l,r,x,T(0)); }
    void set(int i, T x) { assert(0 <= i && i < n); assign(i,i+1,x); } // O(m).
};
