#pragma once

#include "../01-Core/01-template.hpp"

// 0 <= n <= INT_MAX; M is a monoid (S, op, e); B = 0 picks block width k = max(1, floor(sqrt(n))).
// S: O(n), U: O(k), Q: O(k + n / k), M: O(n + 1)
template<typename M>
struct SqrtDecomp {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int n = 0, width = 1;
    vector<S> v, blks;

    explicit SqrtDecomp(int N = 0, int B = 0, M mon = {}) : m(std::move(mon)) { assert(N >= 0); rebuild(vector<S>(max(N, 0), m.e()), B); }
    explicit SqrtDecomp(vector<S> a, int B = 0, M mon = {}) : m(std::move(mon)) { rebuild(std::move(a), B); }
    SqrtDecomp(std::initializer_list<S> a, int B = 0, M mon = {}) : SqrtDecomp(vector<S>(a), B, std::move(mon)) {}
    void rebuild(vector<S> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); width = B ? B : max(1, int(std::sqrt(n))); v = std::move(a);
        blks.assign(n / width + (n % width != 0), m.e());
        for (int bi = 0; bi < int(blks.size()); ++bi) { pull(bi); }}

    // Q: O(1) for get, O(n) for values.
    typename vector<S>::const_reference get(int i) const { assert(0 <= i && i < n); return v[i]; }
    vector<S> values() const { return v; }

    void set(int i, S x) { assert(0 <= i && i < n); v[i] = std::move(x); pull(i / width); }
    void apply(int i, S x) { assert(0 <= i && i < n); v[i] = m.op(v[i], x); pull(i / width); }

    S prod(int l, int r) const {
        assert(0 <= l && l <= r && r <= n);
        S res = m.e();
        while (l < min(r, n)) {
            int bi = l / width, start = bi * width, end = start + min(width, n - start), stop = min(r, end);
            if (l == start && stop == end) { res = m.op(res, blks[bi]); }
            else {
                for (int i = l; i < stop; ++i) { res = m.op(res, v[i]); }}
            l = stop;}
        return res;}

    void pull(int bi) {
        int l = bi * width, r = l + min(width, n - l); blks[bi] = m.e();
        for (int i = l; i < r; ++i) { blks[bi] = m.op(blks[bi], v[i]); }}
};

// T is a commutative ring with T(0), T(1), T(length) and ==; affine(l, r, a, b) maps x to a * x + b after pending tags.
// S: O(n), U: O(m + n / m), Q: O(m + n / m), M: O(n + 1)
template<typename T>
struct SqrtRangeSum {
    int n = 0, m = 1;
    vector<T> v, blks, lazy_mul, lazy_add;

    explicit SqrtRangeSum(int N = 0, int B = 0) : SqrtRangeSum(vector<T>(max(N, 0), T(0)), B) { assert(N >= 0); }
    explicit SqrtRangeSum(vector<T> a, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); m = B ? B : max(1, int(std::sqrt(n))); v = std::move(a);
        int count = n / m + (n % m != 0);
        blks.assign(count, T(0)); lazy_mul.assign(count, T(1)); lazy_add.assign(count, T(0));
        for (int bi = 0; bi < count; ++bi) { pull(bi); }}
    SqrtRangeSum(std::initializer_list<T> a, int B = 0) : SqrtRangeSum(vector<T>(a), B) {}

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
        while (l < min(r, n)) {
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
        while (l < min(r, n)) {
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

// T has T(0), + and a total order < that + preserves; B = 0 picks width k = max(1, floor(sqrt(n * log2(n + 1)))); maxLess returns false when nothing qualifies.
// S: O(n * log(n + 1)), U: O(k + n / k), Q: O(k + (n / k) * log(k + 1)), M: O(n + 1)
template<typename T>
struct SortedBlocks {
    int n = 0, width = 1;
    vector<T> v, s, add_tag, set_val;
    vector<int> ord;
    vector<bool> assigned;

    explicit SortedBlocks(vector<T> a = {}, int B = 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); width = B ? B : max(1, int(std::sqrt(n * std::log2(n + 1.0)))); v = std::move(a);
        int count = n / width + (n % width != 0);
        add_tag.assign(count, T(0)); set_val.assign(count, T(0)); assigned.assign(count, false);
        ord.resize(n); s.assign(n, T(0));
        for (int bi = 0; bi < count; ++bi) {
            auto b = ord.begin() + bi * width, e = b + min(width, n - bi * width);
            std::iota(b, e, bi * width);
            sort(b, e, [&](int i, int j) { return v[i] < v[j]; });
            for (auto it = b; it != e; ++it) { s[it - ord.begin()] = v[*it]; }}}

    // Q: O(1) for get, O(n) for values.
    T get(int i) const { assert(0 <= i && i < n); int bi = i / width; return assigned[bi] ? set_val[bi] : v[i] + add_tag[bi]; }
    vector<T> values() const {
        vector<T> res; res.reserve(n);
        for (int i = 0; i < n; ++i) { res.push_back(get(i)); }
        return res;}

    void add(int l, int r, T x) {
        blocks(l, r, [&](int bi) { T &tag = assigned[bi] ? set_val[bi] : add_tag[bi]; tag = tag + x; },
            [&](int bi, int lo, int hi) { reorder(bi, lo, hi, [&](T &y) { y = y + x; }); });}
    void assign(int l, int r, T x) {
        blocks(l, r, [&](int bi) { assigned[bi] = true; set_val[bi] = x; },
            [&](int bi, int lo, int hi) { reorder(bi, lo, hi, [&](T &y) { y = x; }); });}

    int countLess(int l, int r, const T &x) const { return countBelow(l, r, [&](const T &y) { return y < x; }); }
    int countLeq(int l, int r, const T &x) const { return countBelow(l, r, [&](const T &y) { return !(x < y); }); }
    int countGeq(int l, int r, const T &x) const { return r - l - countLess(l, r, x); }
    int countGreater(int l, int r, const T &x) const { return r - l - countLeq(l, r, x); }
    int countEqual(int l, int r, const T &x) const { return countLeq(l, r, x) - countLess(l, r, x); }
    bool maxLess(int l, int r, T x, T &res) const {
        bool found = false;
        auto relax = [&](const T &y) { if (y < x && (!found || res < y)) { res = y; found = true; }};
        blocks(l, r, [&](int bi) {
            if (assigned[bi]) { relax(set_val[bi]); return; }
            int j = first(bi, [&](const T &y) { return y < x; });
            if (j > bi * width) { relax(s[j - 1] + add_tag[bi]); }},
            [&](int, int lo, int hi) { for (int i = lo; i < hi; ++i) { relax(get(i)); }});
        return found;}

    template<typename Full, typename Part>
    void blocks(int l, int r, Full full, Part part) const {
        assert(0 <= l && l <= r && r <= n);
        while (l < min(r, n)) {
            int bi = l / width, start = bi * width, end = start + min(width, n - start), stop = min(r, end);
            if (l == start && stop == end) { full(bi); }
            else { part(bi, l, stop); }
            l = stop;}}
    template<typename P>
    int first(int bi, P below) const {
        int start = bi * width, end = start + min(width, n - start);
        return int(std::partition_point(s.begin() + start, s.begin() + end, [&](const T &y) { return below(y + add_tag[bi]); }) - s.begin());}
    template<typename P>
    int countBelow(int l, int r, P below) const {
        int res = 0;
        blocks(l, r, [&](int bi) { res += assigned[bi] ? (below(set_val[bi]) ? min(width, n - bi * width) : 0) : first(bi, below) - bi * width; },
            [&](int, int lo, int hi) { for (int i = lo; i < hi; ++i) { res += below(get(i)); }});
        return res;}
    // T: O(k); applies f to v[lo, hi) after the block's tags and merges the changed and unchanged sorted runs.
    template<typename F>
    void reorder(int bi, int lo, int hi, F f) {
        int start = bi * width, end = start + min(width, n - start);
        for (int i = start; i < end; ++i) { v[i] = get(i); }
        add_tag[bi] = T(0); assigned[bi] = false;
        for (int i = lo; i < hi; ++i) { f(v[i]); }
        auto b = ord.begin() + start, e = ord.begin() + end;
        auto mid = std::stable_partition(b, e, [&](int i) { return i < lo || i >= hi; });
        std::inplace_merge(b, mid, e, [&](int i, int j) { return v[i] < v[j]; });
        for (int j = start; j < end; ++j) { s[j] = v[ord[j]]; }}
};

// Block: build(l, r), updateAll(x), updatePart(l, r, x), queryAll(), queryPart(l, r) over global [l, r) in the block; M folds results; B = 0 picks k = max(1, floor(sqrt(n))).
// S: O(n / k + 1) build calls, U: O(n / k + 1) updateAll and 2 updatePart calls, Q: O(n / k + 1) queryAll and 2 queryPart calls, M: O(n / k + 1)
template<typename Block, typename M>
struct BlockDecomp {
    using S = typename M::S;
    [[no_unique_address]] M m;
    int n = 0, width = 1;
    vector<Block> blks;

    explicit BlockDecomp(int N = 0, int B = 0, Block proto = {}, M mon = {}) : m(std::move(mon)) {
        assert(N >= 0 && B >= 0);
        n = N; width = B ? B : max(1, int(std::sqrt(n)));
        blks.assign(n / width + (n % width != 0), proto);
        for (int bi = 0; bi < int(blks.size()); ++bi) { blks[bi].build(bi * width, bi * width + min(width, n - bi * width)); }}

    template<typename F>
    void apply(int l, int r, const F &x) {
        assert(0 <= l && l <= r && r <= n);
        while (l < min(r, n)) {
            int bi = l / width, start = bi * width, end = start + min(width, n - start), stop = min(r, end);
            if (l == start && stop == end) { blks[bi].updateAll(x); }
            else { blks[bi].updatePart(l, stop, x); }
            l = stop;}}
    S prod(int l, int r) {
        assert(0 <= l && l <= r && r <= n);
        S res = m.e();
        while (l < min(r, n)) {
            int bi = l / width, start = bi * width, end = start + min(width, n - start), stop = min(r, end);
            res = m.op(res, l == start && stop == end ? blks[bi].queryAll() : blks[bi].queryPart(l, stop));
            l = stop;}
        return res;}
};

// B = 0 picks width k = max(1, floor(sqrt(n))) and refreshes it when n leaves [k^2 / 4, 4 * k^2]; blocks hold at most 2 * k and adjacent pairs at least k.
// S: O(n), U: O(k + n / k) (amortized O(sqrt(n)) when B = 0), Q: O(n / k), M: O(n + 1)
template<typename T>
struct BlockList {
    int n = 0, width = 1;
    bool adaptive = true;
    vector<vector<T>> blks;

    explicit BlockList(vector<T> a = {}, int B = 0) : adaptive(B == 0) {
        assert(a.size() <= size_t(INT_MAX) && B >= 0);
        n = int(a.size()); width = B ? B : max(1, int(std::sqrt(n)));
        reshape(a);}

    int size() const { return n; }
    typename vector<T>::reference at(int i) { auto [bi, j] = locate(i); return blks[bi][j]; }
    typename vector<T>::const_reference at(int i) const { auto [bi, j] = locate(i); return blks[bi][j]; }

    void insert(int i, T x) {
        assert(0 <= i && i <= n && n < INT_MAX);
        if (blks.empty()) { blks.emplace_back(); }
        int bi = 0;
        for (; bi + 1 < int(blks.size()) && i > int(blks[bi].size()); ++bi) { i -= int(blks[bi].size()); }
        auto &b = blks[bi];
        b.insert(b.begin() + i, std::move(x)); ++n;
        if (b.size() > 2 * size_t(width)) {
            vector<T> half(std::make_move_iterator(b.begin() + width), std::make_move_iterator(b.end()));
            b.erase(b.begin() + width, b.end()); blks.insert(blks.begin() + bi + 1, std::move(half));}
        refresh();}
    void erase(int i) {
        auto [bi, j] = locate(i);
        auto &b = blks[bi];
        b.erase(b.begin() + j); --n;
        if (b.empty()) { blks.erase(blks.begin() + bi); }
        else if (bi > 0 && blks[bi - 1].size() + b.size() < size_t(width)) { merge(bi - 1); }
        else if (bi + 1 < int(blks.size()) && b.size() + blks[bi + 1].size() < size_t(width)) { merge(bi); }
        refresh();}

    pair<int, int> locate(int i) const {
        assert(0 <= i && i < n);
        int bi = 0;
        for (; bi + 1 < int(blks.size()) && i >= int(blks[bi].size()); ++bi) { i -= int(blks[bi].size()); }
        return {bi, i};}
    void merge(int bi) {
        auto &b = blks[bi];
        b.insert(b.end(), std::make_move_iterator(blks[bi + 1].begin()), std::make_move_iterator(blks[bi + 1].end()));
        blks.erase(blks.begin() + bi + 1);}
    void refresh() {
        if (!adaptive || (4 * lng(n) >= lng(width) * width && lng(n) <= 4 * lng(width) * width)) { return; }
        vector<T> a; a.reserve(n);
        for (auto &b : blks) { std::ranges::move(b, std::back_inserter(a)); }
        width = max(1, int(std::sqrt(n))); reshape(a);}
    void reshape(vector<T> &a) {
        blks.clear();
        for (int i = 0, len = 0; i < n; i += len) {
            len = min(width, n - i);
            blks.emplace_back(std::make_move_iterator(a.begin() + i), std::make_move_iterator(a.begin() + i + len));}}
};
