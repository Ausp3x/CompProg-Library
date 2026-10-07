#pragma once

#include "../01-Core/01-template.hpp"

// S: O(n * log(n + 1)), U: rebuild O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n)
template<typename T, typename C = std::less<T>>
struct SortedVector {
    using Iterator = typename vector<T>::const_iterator;
    vector<T> v;
    C cmp;

    explicit SortedVector(vector<T> a = {}, C cmp = C()) : cmp(std::move(cmp)) { rebuild(std::move(a)); }

    int size() const { return int(v.size()); }
    bool empty() const { return v.empty(); }
    Iterator begin() const { return v.begin(); }
    Iterator end() const { return v.end(); }

    void rebuild(vector<T> a) {
        assert(a.size() <= INT_MAX);
        sort(a.begin(), a.end(), cmp); v = std::move(a);}

    int rank(const T &x) const { return int(lower_bound(v.begin(), v.end(), x, cmp) - v.begin()); }
    int upperRank(const T &x) const { return int(upper_bound(v.begin(), v.end(), x, cmp) - v.begin()); }
    int count(const T &x) const { return upperRank(x) - rank(x); }
    int index(const T &x) const { int i = rank(x); return i < size() && !cmp(x, v[i]) ? i : -1; }
    Iterator findByOrder(int k) const { return 0 <= k && k < size() ? v.begin() + k : v.end(); }
};

// S: O(n * log(n + 1)), U: rebuild O(n * log(n + 1)), Q: O(log(n + 1)) per encoded key, M: O(n)
template<typename T, typename C = std::less<T>>
struct CoordinateCompression : SortedVector<T, C> {
    explicit CoordinateCompression(vector<T> a = {}, C cmp = C()) : SortedVector<T, C>({}, std::move(cmp)) { rebuild(std::move(a)); }

    void rebuild(vector<T> a) {
        SortedVector<T, C>::rebuild(std::move(a));
        auto &v = this->v;
        auto same = [this](const T &x, const T &y) { return !this->cmp(x, y) && !this->cmp(y, x); };
        v.erase(unique(v.begin(), v.end(), same), v.end());}

    typename vector<T>::const_reference decode(int i) const { assert(0 <= i && i < this->size()); return this->v[i]; }
    vector<int> encode(const vector<T> &a) const {
        vector<int> res; res.reserve(a.size());
        for (const T &x : a) { res.push_back(this->index(x)); }
        return res;}
};

// S: O(1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n); GNU PBDS set and map with unique keys, never less_equal as C.
template<typename T, typename C = std::less<T>>
using OrderedSet = __gnu_pbds::tree<T, __gnu_pbds::null_type, C, __gnu_pbds::rb_tree_tag, __gnu_pbds::tree_order_statistics_node_update>;
template<typename K, typename V, typename C = std::less<K>>
using OrderedMap = __gnu_pbds::tree<K, V, C, __gnu_pbds::rb_tree_tag, __gnu_pbds::tree_order_statistics_node_update>;

// At most INT_MAX live elements and LLONG_MAX - 1 lifetime insertions; insert returns an exact erase token; misses return end().
// S: O(1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n)
template<typename T, typename C = std::less<T>>
struct OrderedMultiSet {
    using Key = pair<T, lng>;
    struct Compare {
        C cmp;
        bool operator()(const Key &a, const Key &b) const {
            if (cmp(a.first, b.first)) { return true; }
            if (cmp(b.first, a.first)) { return false; }
            return a.second < b.second;}
    };
    using Tree = OrderedSet<Key, Compare>;
    using Iterator = typename Tree::const_iterator;
    Tree tree;
    lng next_id = 1;

    explicit OrderedMultiSet(C cmp = C()) : tree(Compare{std::move(cmp)}) {}
    explicit OrderedMultiSet(const vector<T> &a, C cmp = C()) : OrderedMultiSet(std::move(cmp)) { rebuild(a); }

    int size() const { return int(tree.size()); }
    bool empty() const { return tree.empty(); }
    Iterator begin() const { return tree.begin(); }
    Iterator end() const { return tree.end(); }

    Key insert(T x) {
        assert(tree.size() < INT_MAX && next_id < LLONG_MAX);
        Key key(std::move(x), next_id++); tree.insert(key); return key;}
    bool erase(const Key &key) { return tree.erase(key); }
    bool eraseOne(const T &x) {
        auto it = lowerBound(x);
        if (it == end() || tree.get_cmp_fn().cmp(x, it->first)) { return false; }
        tree.erase(it); return true;}
    // T: O(n) for clear, O(n + k * log(k + 1)) for rebuild, k = a.size().
    void clear() { tree.clear(); }
    void rebuild(const vector<T> &a) {
        assert(a.size() <= INT_MAX && a.size() <= size_t(LLONG_MAX - next_id));
        clear(); for (const T &x : a) { insert(x); }}

    Iterator lowerBound(const T &x) const { return tree.lower_bound(Key(x, 0)); }
    Iterator upperBound(const T &x) const { return tree.lower_bound(Key(x, LLONG_MAX)); }
    Iterator prev(const T &x) const { auto it = upperBound(x); return it == begin() ? end() : std::prev(it); }
    int rank(const T &x) const { return int(tree.order_of_key(Key(x, 0))); }
    int upperRank(const T &x) const { return int(tree.order_of_key(Key(x, LLONG_MAX))); }
    int count(const T &x) const { return upperRank(x) - rank(x); }
    Iterator findByOrder(int k) const { return k < 0 ? end() : tree.find_by_order(k); }
};
