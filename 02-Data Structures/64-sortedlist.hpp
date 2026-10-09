#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n) sorted input, O(n * log(n + 1)) otherwise; U: O(sqrt(n)) amortized; Q: O(sqrt(n)) (O(log(n)) bound queries); M: O(n)
// Sorted multiset in buckets of [load/2, 2 * load] elements, load ~ sqrt(RATIO * n); kth needs k in [0, n); searches return end() when absent.
template<typename T, typename C = std::less<T>>
struct SortedList {
    static constexpr int BMIN = 32, RATIO = 4;
    vector<vector<T>> a;
    int n = 0, built = 0, load = BMIN;
    C cmp;

    struct iterator {
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using pointer = const T *;
        using reference = const T &;
        const SortedList *s = nullptr;
        int b = 0, i = 0;
        const T &operator*() const { return s->a[b][i]; }
        const T *operator->() const { return &s->a[b][i]; }
        iterator &operator++() {
            if (++i == int(s->a[b].size())) { ++b; i = 0; }
            return *this;}
        iterator &operator--() {
            if (i-- == 0) { --b; i = int(s->a[b].size()) - 1; }
            return *this;}
        iterator operator++(int) { iterator t = *this; ++*this; return t; }
        iterator operator--(int) { iterator t = *this; --*this; return t; }
        bool operator==(const iterator &o) const { return b == o.b && i == o.i; }
    };

    explicit SortedList(vector<T> v = {}, C cmp = C()) : cmp(std::move(cmp)) { rebuild(std::move(v)); }

    int size() const { return n; }
    bool empty() const { return !n; }
    iterator begin() const { return {this, 0, 0}; }
    iterator end() const { return {this, int(a.size()), 0}; }
    std::reverse_iterator<iterator> rbegin() const { return std::reverse_iterator(end()); }
    std::reverse_iterator<iterator> rend() const { return std::reverse_iterator(begin()); }
    const T &front() const { assert(n > 0); return a[0][0]; }
    const T &back() const { assert(n > 0); return a.back().back(); }

    void rebuild(vector<T> v) {
        if (!std::is_sorted(v.begin(), v.end(), cmp)) { std::stable_sort(v.begin(), v.end(), cmp); }
        n = built = int(v.size());
        load = max(BMIN, int(std::sqrt(double(RATIO) * n)));
        int m = (n + load - 1) / load;
        a.assign(m, {});
        for (int j = 0; j < m; ++j) {
            a[j].assign(std::make_move_iterator(v.begin() + lng(n) * j / m), std::make_move_iterator(v.begin() + lng(n) * (j + 1) / m));}}
    void rebuild() {
        vector<T> v;
        v.reserve(n);
        for (auto &x : a) { v.insert(v.end(), std::make_move_iterator(x.begin()), std::make_move_iterator(x.end())); }
        rebuild(std::move(v));}
    void clear() { rebuild({}); }
    void split(int b) {
        if (int(a[b].size()) <= 2 * load) { return; }
        int h = int(a[b].size()) / 2;
        vector<T> r(std::make_move_iterator(a[b].begin() + h), std::make_move_iterator(a[b].end()));
        a[b].resize(h);
        a.insert(a.begin() + b + 1, std::move(r));}
    // Restores bucket b to [load/2, 2 * load] after removals by dropping it or merging it into a neighbour.
    void fix(int b) {
        if (a[b].empty()) { a.erase(a.begin() + b); }
        else {
            while (a.size() > 1 && int(a[b].size()) < load / 2) {
                b = min(b, int(a.size()) - 2);
                a[b].insert(a[b].end(), std::make_move_iterator(a[b + 1].begin()), std::make_move_iterator(a[b + 1].end()));
                a.erase(a.begin() + b + 1);
                split(b);}}}
    void insert(const T &x) {
        if (a.empty()) { a.push_back({x}); }
        else {
            int b = min(upperBucket(x), int(a.size()) - 1);
            a[b].insert(upper_bound(a[b].begin(), a[b].end(), x, cmp), x);
            split(b);}
        if (++n > 2 * built + BMIN) { rebuild(); }}
    T eraseAt(int b, int i) {
        T res = std::move(a[b][i]);
        a[b].erase(a[b].begin() + i);
        --n;
        fix(b);
        if (2 * n < built) { rebuild(); }
        return res;}
    bool eraseOne(const T &x) {
        iterator it = lowerBound(x);
        if (it == end() || cmp(x, *it)) { return false; }
        eraseAt(it.b, it.i);
        return true;}
    int erase(T x) {
        int b = lowerBucket(x), e = b, res = 0;
        for (; e < int(a.size()); ++e) {
            auto l = lower_bound(a[e].begin(), a[e].end(), x, cmp), r = upper_bound(l, a[e].end(), x, cmp);
            bool more = r == a[e].end();
            res += int(r - l);
            a[e].erase(l, r);
            if (!more) { break; }}
        if (!res) { return 0; }
        n -= res;
        e = min(e + 1, int(a.size()));
        a.erase(std::remove_if(a.begin() + b, a.begin() + e, [](const vector<T> &v) { return v.empty(); }), a.begin() + e);
        // Only the two partial end buckets survive, now at b and b + 1.
        if (b + 1 < int(a.size())) { fix(b + 1); }
        if (b < int(a.size())) { fix(b); }
        if (2 * n < built) { rebuild(); }
        return res;}
    T eraseKth(int k) { auto [b, i] = locateKth(k); return eraseAt(b, i); }
    T popFront() { assert(n > 0); return eraseAt(0, 0); }
    T popBack() { assert(n > 0); return eraseAt(int(a.size()) - 1, int(a.back().size()) - 1); }

    int lowerBucket(const T &x) const {
        return int(std::partition_point(a.begin(), a.end(), [&](const vector<T> &v) { return cmp(v.back(), x); }) - a.begin());}
    int upperBucket(const T &x) const {
        return int(std::partition_point(a.begin(), a.end(), [&](const vector<T> &v) { return !cmp(x, v.back()); }) - a.begin());}
    iterator lowerBound(const T &x) const {
        int b = lowerBucket(x);
        if (b == int(a.size())) { return end(); }
        return {this, b, int(lower_bound(a[b].begin(), a[b].end(), x, cmp) - a[b].begin())};}
    iterator upperBound(const T &x) const {
        int b = upperBucket(x);
        if (b == int(a.size())) { return end(); }
        return {this, b, int(upper_bound(a[b].begin(), a[b].end(), x, cmp) - a[b].begin())};}
    iterator prev(const T &x) const {
        iterator it = upperBound(x);
        return it == begin() ? end() : --it;}
    iterator next(const T &x) const { return lowerBound(x); }
    bool contains(const T &x) const {
        iterator it = lowerBound(x);
        return it != end() && !cmp(x, *it);}
    int position(iterator it) const {
        int res = it.i;
        for (int j = 0; j < it.b; ++j) { res += int(a[j].size()); }
        return res;}
    int rank(const T &x) const { return position(lowerBound(x)); }
    int upperRank(const T &x) const { return position(upperBound(x)); }
    int count(const T &x) const { return upperRank(x) - rank(x); }
    pair<int, int> locateKth(int k) const {
        assert(0 <= k && k < n);
        int b = 0;
        while (b + 1 < int(a.size()) && k >= int(a[b].size())) { k -= int(a[b++].size()); }
        return {b, k};}
    const T &kth(int k) const { auto [b, i] = locateKth(k); return a[b][i]; }
    const T &operator[](int k) const { return kth(k); }
};
