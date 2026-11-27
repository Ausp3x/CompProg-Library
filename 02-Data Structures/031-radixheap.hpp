#pragma once

#include "../01-Core/01-template.hpp"

namespace radixheap_detail {
    template<typename K>
    concept Key = std::integral<K> && std::same_as<K, std::remove_cv_t<K>> && (sizeof(K) == 4 || sizeof(K) == 8) && !std::same_as<K, wchar_t> && !std::same_as<K, char32_t>;

    // T: O(1), M: O(1); order-preserving map onto the unsigned type (signed keys flip the sign bit).
    template<Key K>
    constexpr std::make_unsigned_t<K> encode(K x) {
        using U = std::make_unsigned_t<K>;
        return U(x) ^ U(U(std::is_signed_v<K>) << (std::numeric_limits<U>::digits - 1));}

    // T: O(n), M: O(1); empties a pulled bucket of n moved-from elements, freeing it above 4096 slots so cascades cannot retain O(n * b) capacity.
    template<typename T>
    void release(vector<T> &a) {
        if (a.capacity() > 4096) { std::exchange(a, {}); }
        else { a.clear(); }}
} // namespace radixheap_detail

// Min-heap of (key, payload) pairs, K one of int, long, long long or their unsigned types; push asserts key >= last (the minimum at the latest top or pop, initially the least K); top and pop assert a nonempty heap.
// S: O(b), U: O(b) amortized push and pop, Q: O(b) amortized top, M: O(n + b)
template<radixheap_detail::Key K, typename V>
struct RadixHeap {
    using U = std::make_unsigned_t<K>;
    static constexpr int B = std::numeric_limits<U>::digits;
    array<vector<pair<K, V>>, B + 1> v;
    U last = 0;
    int cnt = 0;

    int size() const { return cnt; }
    bool empty() const { return !cnt; }
    const pair<K, V> &top() { pull(); return v[0].back(); }

    void push(K key, V val) {
        assert(radixheap_detail::encode(key) >= last);
        v[std::bit_width(radixheap_detail::encode(key) ^ last)].emplace_back(key, std::move(val));
        ++cnt;}
    void pop() {
        pull();
        if (!v[0].empty()) { v[0].pop_back(); --cnt; }}
    void pull() {
        assert(cnt > 0);
        if (!cnt || !v[0].empty()) { return; }
        int b = 1;
        while (v[b].empty()) { ++b; }
        last = radixheap_detail::encode(std::ranges::min_element(v[b], {}, &pair<K, V>::first)->first);
        for (auto &e : v[b]) { v[std::bit_width(radixheap_detail::encode(e.first) ^ last)].push_back(std::move(e)); }
        radixheap_detail::release(v[b]);}
};

// RadixHeap over ids [0, n), each holding at most one key; push asserts an absent id, key and decreaseKey a present one, and new keys must be >= last; top() is the id of a minimum key and pop removes it.
// S: O(n + b), U: O(b) amortized push and pop, O(1) amortized decreaseKey, Q: O(b) amortized top, O(1) contains and key, M: O(n + b + k) with k decreaseKey calls
template<radixheap_detail::Key K>
struct RadixHeapArray {
    using U = std::make_unsigned_t<K>;
    static constexpr int B = std::numeric_limits<U>::digits;
    array<vector<int>, B + 1> v;
    vector<K> k;
    vector<int> bk, pos;
    U last = 0;
    int cnt = 0;

    explicit RadixHeapArray(int n = 0) : k(max(n, 0)), bk(max(n, 0), -1), pos(max(n, 0)) { assert(n >= 0); }

    int size() const { return cnt; }
    bool empty() const { return !cnt; }
    bool contains(int i) const { assert(0 <= i && i < int(bk.size())); return bk[i] >= 0; }
    const K &key(int i) const { assert(contains(i)); return k[i]; }
    int top() { pull(); return v[0].back(); }

    void place(int i) {
        int b = std::bit_width(radixheap_detail::encode(k[i]) ^ last);
        bk[i] = b; pos[i] = int(v[b].size());
        v[b].push_back(i);}
    void push(int i, K x) {
        assert(!contains(i) && radixheap_detail::encode(x) >= last);
        if (bk[i] >= 0) { return; }
        k[i] = x; place(i);
        ++cnt;}
    void decreaseKey(int i, K x) {
        assert(contains(i) && last <= radixheap_detail::encode(x) && x <= k[i]);
        if (bk[i] < 0) { return; }
        auto &w = v[bk[i]];
        pos[w.back()] = pos[i]; w[pos[i]] = w.back();
        w.pop_back();
        k[i] = x; place(i);}
    void pop() {
        pull();
        if (!v[0].empty()) { bk[v[0].back()] = -1; v[0].pop_back(); --cnt; }}
    void pull() {
        assert(cnt > 0);
        if (!cnt || !v[0].empty()) { return; }
        int b = 1;
        while (v[b].empty()) { ++b; }
        last = radixheap_detail::encode(k[*std::ranges::min_element(v[b], {}, [&](int i) { return k[i]; })]);
        for (int i : v[b]) { place(i); }
        radixheap_detail::release(v[b]);}
};
