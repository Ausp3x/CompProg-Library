#pragma once

#include "../01-Core/01-template.hpp"

namespace binary_trie_detail {
    // S: O(1), U: NA, Q: O(B), M: O(m); B key bits, m nodes; node 0 is empty and queries on roots (u, v) count v minus u.
    template<typename T, int B>
    struct Nodes {
        using U = std::make_unsigned_t<T>;
        static_assert(std::is_integral_v<T> && 1 <= B && B <= std::numeric_limits<U>::digits);
        static constexpr U MASK = U(~U(0)) >> (std::numeric_limits<U>::digits - B);
        static constexpr U SIGN = std::is_signed_v<T> ? U(U(1) << (B - 1)) : U(0);
        vector<array<int, 2>> ch{{0, 0}};
        vector<int> cnt{0};

        static U enc(T a) { return U((U(a) ^ SIGN) & MASK); }
        static T dec(U s) { return T(U(s - SIGN)); }
        static bool valid(T a) { return dec(enc(a)) == a; }

        int count(int u, int v, U s) const {
            for (int b = B - 1; b >= 0; --b) {
                int d = int(s >> b & 1);
                u = ch[u][d]; v = ch[v][d];}
            return cnt[v] - cnt[u];}
        U kth(int u, int v, int k, U x) const {
            U res = 0;
            for (int b = B - 1; b >= 0; --b) {
                int d = int(x >> b & 1), c = cnt[ch[v][d]] - cnt[ch[u][d]];
                if (k >= c) { k -= c; d ^= 1; res = U(res | U(1) << b); }
                u = ch[u][d]; v = ch[v][d];}
            return res;}
        int countLess(int u, int v, U t, U x) const {
            int res = 0;
            for (int b = B - 1; b >= 0; --b) {
                int d = int(x >> b & 1);
                if (t >> b & 1) { res += cnt[ch[v][d]] - cnt[ch[u][d]]; d ^= 1; }
                u = ch[u][d]; v = ch[v][d];}
            return res;}
    };
} // namespace binary_trie_detail

// Keys in [0, 2^B) for unsigned T, [-2^(B-1), 2^(B-1)) for signed T; multiset of at most INT_MAX keys; lowerBound returns false when absent.
// S: O(1), U: O(B), Q: O(B), M: O(n * B); B key bits, n insert calls
template<typename T, int B = std::numeric_limits<std::make_unsigned_t<T>>::digits>
struct BinaryTrie {
    using N = binary_trie_detail::Nodes<T, B>;
    using U = typename N::U;
    N t;
    U lz = 0;

    BinaryTrie() { t.ch.push_back({0, 0}); t.cnt.push_back(0); }

    int size() const { return t.cnt[1]; }
    int count(T a) const { assert(N::valid(a)); return t.count(0, 1, N::enc(a) ^ lz); }

    void add(U s, int c) {
        int v = 1; t.cnt[v] += c;
        for (int b = B - 1; b >= 0; --b) {
            int d = int(s >> b & 1);
            if (!t.ch[v][d]) { t.ch[v][d] = int(t.ch.size()); t.ch.push_back({0, 0}); t.cnt.push_back(0); }
            v = t.ch[v][d]; t.cnt[v] += c;}}
    void insert(T a, int c = 1) { assert(N::valid(a) && 0 <= c && c <= INT_MAX - size()); add(N::enc(a) ^ lz, c); }
    int erase(T a, int c = 1) {
        assert(c >= 0);
        int k = min(c, count(a));
        if (k) { add(N::enc(a) ^ lz, -k); }
        return k;}
    void applyXor(T x) { assert(N::valid(x)); lz ^= U(x) & N::MASK; }

    T xorMin(T x) const { assert(N::valid(x) && size() > 0); return N::dec(t.kth(0, 1, 0, lz ^ (U(x) & N::MASK))); }
    T xorMax(T x) const { assert(N::valid(x) && size() > 0); return N::dec(t.kth(0, 1, size() - 1, lz ^ (U(x) & N::MASK))); }
    T kth(int k) const { assert(0 <= k && k < size()); return N::dec(t.kth(0, 1, k, lz)); }
    int countLess(T y) const { assert(N::valid(y)); return t.countLess(0, 1, N::enc(y), lz); }
    bool lowerBound(T y, T &res) const {
        int k = countLess(y);
        if (k == size()) { return false; }
        res = kth(k); return true;}
};

// Versions are node ids (0 is empty); queries on (u, v) see the multiset v minus u, which needs u contained in v.
// S: O(1), U: O(B), Q: O(B), M: O(n * B); B key bits, n insert and erase calls
template<typename T, int B = std::numeric_limits<std::make_unsigned_t<T>>::digits>
struct PersistentBinaryTrie {
    using N = binary_trie_detail::Nodes<T, B>;
    using U = typename N::U;
    N t;

    int add(int v, U s, int c) {
        int root = int(t.ch.size());
        for (int b = B; b >= 0; --b) {
            array<int, 2> nx = t.ch[v];
            int k = t.cnt[v] + c;
            if (b) {
                int d = int(s >> (b - 1) & 1);
                v = nx[d]; nx[d] = int(t.ch.size()) + 1;}
            t.ch.push_back(nx); t.cnt.push_back(k);}
        return root;}
    int insert(int v, T a, int c = 1) {
        assert(0 <= v && v < int(t.ch.size()) && N::valid(a) && 0 <= c && c <= INT_MAX - t.cnt[v]);
        return add(v, N::enc(a), c);}
    int erase(int v, T a, int c = 1) {
        assert(c >= 0);
        int k = min(c, count(0, v, a));
        return k ? add(v, N::enc(a), -k) : v;}

    int size(int u, int v) const { assert(0 <= u && u < int(t.ch.size()) && 0 <= v && v < int(t.ch.size())); return t.cnt[v] - t.cnt[u]; }
    int count(int u, int v, T a) const { assert(N::valid(a) && size(u, v) >= 0); return t.count(u, v, N::enc(a)); }
    T xorMin(int u, int v, T x) const { assert(N::valid(x) && size(u, v) > 0); return N::dec(t.kth(u, v, 0, U(x) & N::MASK)); }
    T xorMax(int u, int v, T x) const { assert(N::valid(x) && size(u, v) > 0); return N::dec(t.kth(u, v, size(u, v) - 1, U(x) & N::MASK)); }
    T kth(int u, int v, int k, T x = 0) const { assert(N::valid(x) && 0 <= k && k < size(u, v)); return N::dec(t.kth(u, v, k, U(x) & N::MASK)); }
    int countLess(int u, int v, T y, T x = 0) const {
        assert(N::valid(x) && N::valid(y) && size(u, v) >= 0);
        return t.countLess(u, v, N::enc(y), U(x) & N::MASK);}
};
