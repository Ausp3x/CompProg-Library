#pragma once
#include "../01-Core/01-template.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

// S: O(1), O(m) with reserve m; U: O(1) expected amortized, Q: O(1) expected, M: O(cap) with cap <= 4 * max(peak size, reserve)
// Linear probing, load <= 1/2, backward-shift erase; T is K (set) or pair<K, V> (map); FIXED asserts instead of rehashing.
template<typename K, typename T, typename H, bool FIXED>
struct HashTable {
    struct Slot { T x; uint t = 0; };
    vector<Slot> s;
    int n = 0;
    uint gen = 1;
    H hash;

    template<bool C>
    struct Iter {
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = ptrdiff_t;
        using reference = std::conditional_t<C, const T &, T &>;
        using pointer = std::conditional_t<C, const T *, T *>;
        std::conditional_t<C, const HashTable *, HashTable *> p = nullptr;
        int i = 0;
        reference operator*() const { return p->s[i].x; }
        pointer operator->() const { return &p->s[i].x; }
        Iter &operator++() {
            do { ++i; } while (i < int(p->s.size()) && p->s[i].t != p->gen);
            return *this;}
        Iter operator++(int) { Iter t = *this; ++*this; return t; }
        operator Iter<true>() const { return {p, i}; }
        bool operator==(const Iter &o) const { return i == o.i; }
    };
    using iterator = Iter<std::is_same_v<K, T>>;
    using const_iterator = Iter<true>;

    explicit HashTable(int m = 0, H hash = H()) : hash(std::move(hash)) { reserve(m); }

    static const K &key(const T &x) {
        if constexpr (std::is_same_v<K, T>) { return x; }
        else { return x.first; }}
    int home(const K &k) const { return int(hash(k) & (s.size() - 1)); }
    int locate(const K &k) const {
        int i = home(k), mask = int(s.size()) - 1;
        while (s[i].t == gen && !(key(s[i].x) == k)) { i = (i + 1) & mask; }
        return i;}
    int index(const K &k) const {
        if (!n) { return -1; }
        int i = locate(k);
        return s[i].t == gen ? i : -1;}
    int size() const { return n; }
    bool empty() const { return !n; }
    bool contains(const K &k) const { return index(k) >= 0; }
    iterator find(const K &k) { int i = index(k); return {this, i < 0 ? int(s.size()) : i}; }
    const_iterator find(const K &k) const { int i = index(k); return {this, i < 0 ? int(s.size()) : i}; }
    iterator begin() { return ++iterator{this, -1}; }
    iterator end() { return {this, int(s.size())}; }
    const_iterator begin() const { return ++const_iterator{this, -1}; }
    const_iterator end() const { return {this, int(s.size())}; }

    void rehash(int c) {
        vector<Slot> old(c);
        swap(old, s);
        for (auto &o : old) {
            if (o.t != gen) { continue; }
            int i = home(key(o.x));
            while (s[i].t == gen) { i = (i + 1) & (c - 1); }
            s[i] = {std::move(o.x), gen};}}
    void reserve(int m) {
        assert(0 <= m && m <= (1 << 29));
        int c = max(4, int(std::bit_ceil(2 * uint(m))));
        if (m && c > int(s.size())) { rehash(c); }}
    // Slot of k and whether it was new; make() builds the element before a rehash can free storage that k aliases.
    template<typename F>
    pair<int, bool> place(const K &k, F make) {
        int i = s.empty() ? 0 : locate(k);
        if (!s.empty() && s[i].t == gen) { return {i, false}; }
        T y = make();
        if (2 * (n + 1) > int(s.size())) {
            assert(!FIXED);
            rehash(max(4, 2 * int(s.size())));
            i = locate(key(y));}
        s[i] = {std::move(y), gen};
        ++n;
        return {i, true};}
    bool erase(const K &k) {
        int i = index(k), mask = int(s.size()) - 1;
        if (i < 0) { return false; }
        for (int j = (i + 1) & mask; s[j].t == gen; j = (j + 1) & mask) {
            // j may fill the hole i when i lies cyclically in [home(j), j).
            if (((j - home(key(s[j].x))) & mask) >= ((j - i) & mask)) {
                s[i].x = std::move(s[j].x);
                i = j;}}
        s[i].t = 0;
        --n;
        return true;}
    void clear() {
        n = 0;
        if (++gen == 0) {
            for (auto &x : s) { x.t = 0; }
            gen = 1;}}
};

// S: O(1), O(m) with reserve m; U: O(1) expected amortized, Q: O(1) expected, M: O(cap) with cap <= 4 * max(peak size, reserve)
template<typename K, typename V, typename H = CustomHash, bool FIXED = false>
struct HashMap : HashTable<K, pair<K, V>, H, FIXED> {
    using B = HashTable<K, pair<K, V>, H, FIXED>;
    using B::B, B::s, B::index, B::place;

    V &operator[](const K &k) { return s[place(k, [&] { return pair<K, V>(k, V()); }).first].x.second; }
    bool insert(const K &k, const V &v) { return place(k, [&] { return pair<K, V>(k, v); }).second; }
    V &at(const K &k) { int i = index(k); assert(i >= 0); return s[i].x.second; }
    const V &at(const K &k) const { int i = index(k); assert(i >= 0); return s[i].x.second; }
    V get(const K &k, const V &def = V()) const { int i = index(k); return i < 0 ? def : s[i].x.second; }
};

// S: O(1), O(m) with reserve m; U: O(1) expected amortized, Q: O(1) expected, M: O(cap) with cap <= 4 * max(peak size, reserve)
template<typename K, typename H = CustomHash, bool FIXED = false>
struct HashSet : HashTable<K, K, H, FIXED> {
    using B = HashTable<K, K, H, FIXED>;
    using B::B, B::s, B::place;

    bool insert(const K &k) { return place(k, [&] { return k; }).second; }
};

template<typename K, typename V, typename H = CustomHash>
using HashMapFixed = HashMap<K, V, H, true>;
template<typename K, typename H = CustomHash>
using HashSetFixed = HashSet<K, H, true>;
