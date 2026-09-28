#pragma once

#include "../01-Core/01-template.hpp"

// Immutable sorted sequence; comparator is a stable strict weak order. Duplicates
// remain, with unspecified order inside an equivalence class. State is read-only.
// n <= INT_MAX. Ranks count elements before / before-or-equivalent to x.
// All bounds count key copies/comparisons as O(1); size/empty are O(1).
// S: O(n * log(n + 1)), U: rebuild O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n)
template<typename T, typename C = std::less<T>>
struct SortedVector {
    using Iterator = typename vector<T>::const_iterator;
    vector<T> v;
    C cmp;

    explicit SortedVector(vector<T> a = {}, C cmp = C()) : cmp(std::move(cmp)) { rebuild(std::move(a)); }
    void rebuild(vector<T> a) {
        assert(a.size() <= INT_MAX);
        sort(a.begin(), a.end(), cmp); v = std::move(a);}
    int size() const { return int(v.size()); }
    bool empty() const { return v.empty(); }
    Iterator begin() const { return v.begin(); }
    Iterator end() const { return v.end(); }
    int rank(const T &x) const { return int(lower_bound(v.begin(), v.end(), x, cmp) - v.begin()); }
    int upperRank(const T &x) const { return int(upper_bound(v.begin(), v.end(), x, cmp) - v.begin()); }
    int count(const T &x) const { return upperRank(x) - rank(x); }
    // First equivalent rank, or -1 if absent. No operator== is required.
    int index(const T &x) const {
        int i = rank(x); return i < size() && !cmp(x, v[i]) ? i : -1;}
    // Q: O(1). end() for any out-of-range rank; iterator lasts until mutation.
    Iterator findByOrder(int k) const { return 0 <= k && k < size() ? v.begin() + k : v.end(); }
};

// Offline compression into [0,size()); comparator-equivalent keys share a rank.
// The representative of an equivalence class is unspecified. Distances between
// coordinates are not preserved. Rebuild invalidates old ranks and iterators.
// S: O(n * log(n + 1)), U: rebuild O(n * log(n + 1)), Q: O(log(n + 1)), M: O(n)
template<typename T, typename C = std::less<T>>
struct CoordinateCompression : SortedVector<T, C> {
    explicit CoordinateCompression(vector<T> a = {}, C cmp = C()) : SortedVector<T, C>({}, std::move(cmp)) {
        rebuild(std::move(a));}
    void rebuild(vector<T> a) {
        SortedVector<T, C>::rebuild(std::move(a));
        auto &v = this->v;
        v.erase(unique(v.begin(), v.end(), [this](const T &a, const T &b) {
            return !this->cmp(a, b) && !this->cmp(b, a); }), v.end());}
    // Q: O(k * log(n + 1)), output O(k); unknown keys map to -1.
    vector<int> encode(const vector<T> &a) const {
        vector<int> res; res.reserve(a.size());
        for (const T &x : a) { res.push_back(this->index(x)); }
        return res;}
};

// GNU PBDS unique-key adapter. Native insert/erase/find/order_of_key/find_by_order
// APIs are retained; kth >= size() returns end(). Never use less_equal as C.
// Stateful comparators use _GLIBCXX_ASSERTIONS for checked builds: GNU PBDS's
// _GLIBCXX_DEBUG equality checker default-constructs a separate comparator.
// S: O(1), U: O(log(n + 1)), Q: O(log(n + 1)), M: O(n)
template<typename T, typename C = std::less<T>>
using OrderedSet = __gnu_pbds::tree<T, __gnu_pbds::null_type, C, __gnu_pbds::rb_tree_tag,
                                   __gnu_pbds::tree_order_statistics_node_update>;

// GNU PBDS multiset. Strict (key, insertion-ID) order keeps equivalent keys in
// insertion order. IDs [1,LLONG_MAX) are never reused, even after clear/rebuild;
// at most LLONG_MAX-1 lifetime insertions and INT_MAX live elements.
// Within one history, insert tokens erase exactly that occurrence; erased tokens
// return false. Copies preserve existing tokens; subsequent histories are local.
// Assignment replaces the destination history and invalidates its old tokens.
// Tokens from unrelated/replaced histories must not be passed to erase.
// Public state must not be edited directly. C is a stable strict weak order.
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
    // S: O(n * log(n + 1)).
    explicit OrderedMultiSet(const vector<T> &a, C cmp = C()) : OrderedMultiSet(std::move(cmp)) { rebuild(a); }
    int size() const { return int(tree.size()); }
    bool empty() const { return tree.empty(); }
    Iterator begin() const { return tree.begin(); }
    Iterator end() const { return tree.end(); }
    Iterator lowerBound(const T &x) const { return tree.lower_bound(Key(x, 0)); }
    Iterator upperBound(const T &x) const { return tree.lower_bound(Key(x, LLONG_MAX)); }
    int rank(const T &x) const { return int(tree.order_of_key(Key(x, 0))); }
    int upperRank(const T &x) const { return int(tree.order_of_key(Key(x, LLONG_MAX))); }
    int count(const T &x) const { return upperRank(x) - rank(x); }
    Iterator findByOrder(int k) const { return k < 0 ? end() : tree.find_by_order(k); }

    Key insert(T x) {
        assert(tree.size() < INT_MAX && next_id < LLONG_MAX);
        Key key(std::move(x), next_id++); tree.insert(key); return key;}
    bool erase(const Key &key) { return tree.erase(key); }
    // Erases the earliest surviving equivalent occurrence, or returns false.
    bool eraseOne(const T &x) {
        auto it = lowerBound(x);
        if (it == end() || tree.get_cmp_fn().cmp(x, it->first)) { return false; }
        tree.erase(it); return true;}
    // T: O(n). Preserving next_id prevents stale tokens matching later insertions.
    void clear() { tree.clear(); }
    // T: O(n + k * log(k + 1)); invalidates existing iterators/tokens, retains ID history.
    void rebuild(const vector<T> &a) {
        assert(a.size() <= INT_MAX && a.size() <= size_t(LLONG_MAX - next_id));
        clear(); for (const T &x : a) { insert(x); }}
};
