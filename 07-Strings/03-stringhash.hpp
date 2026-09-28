#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n), Q: O(1), M: O(n); three arrays of eight-byte entries, n < INT_MAX.
// Monte Carlo fingerprints, never proof of equality. Fixed default bases; no global state.
// Bytes encode unsigned+1; vector<uint> symbols encode x+1, with x<1000000006
// for double primes, or any uint for WRAP64. See 91-stringhash.md for collision bounds.
template<bool WRAP64 = false> struct StringHash {
    using Word = std::conditional_t<WRAP64, ulng, array<uint, 2>>;
    static constexpr array<uint, 2> MOD{1000000007, 1000000009};
    struct Digest {
        Word value;
        int length;
        Word base;
        bool operator==(const Digest &) const = default;
    };
    Word base;
    vector<Word> powers, prefix, reversed;

    static Word defaultBase() {
        if constexpr (WRAP64) { return 11400714819323198485ULL; }
        else { return {911382323, 972663749}; }
    }
    static Word randomBase(ulng seed) {
        std::mt19937_64 rng(seed);
        if constexpr (WRAP64) {
            ulng b; do { b = rng() | 1; } while (b < 257); return b; }
        else {
            return {uint(std::uniform_int_distribution<ulng>(257, MOD[0] - 1)(rng)),
                    uint(std::uniform_int_distribution<ulng>(257, MOD[1] - 1)(rng))}; }
    }
    explicit StringHash(string_view s = {}, Word b = defaultBase()) : base(b) {
        assert(s.size() < INT_MAX);
        build(int(s.size()), [&](int i) { return ulng(uint8_t(s[i])) + 1; });
    }
    explicit StringHash(const vector<uint> &s, Word b = defaultBase()) : base(b) {
        assert(s.size() < INT_MAX);
        build(int(s.size()), [&](int i) {
            if constexpr (!WRAP64) { assert(s[i] < MOD[0] - 1); }
            return ulng(s[i]) + 1; });
    }

    static Word encode(ulng x) {
        if constexpr (WRAP64) { return x; }
        else { return {uint(x), uint(x)}; }
    }
    static Word add(Word a, Word b) {
        if constexpr (WRAP64) { return a + b; }
        else {
            for (int i = 0; i < 2; ++i) { a[i] += b[i]; if (a[i] >= MOD[i]) { a[i] -= MOD[i]; } }
            return a; }
    }
    static Word subtract(Word a, Word b) {
        if constexpr (WRAP64) { return a - b; }
        else {
            for (int i = 0; i < 2; ++i) { a[i] = a[i] >= b[i] ? a[i] - b[i] : a[i] + MOD[i] - b[i]; }
            return a; }
    }
    static Word multiply(Word a, Word b) {
        if constexpr (WRAP64) { return a * b; }
        else {
            for (int i = 0; i < 2; ++i) { a[i] = uint(ulng(a[i]) * b[i] % MOD[i]); }
            return a; }
    }
    template<class F> void build(int n, F code) {
        if constexpr (WRAP64) { assert(base >= 257 && (base & 1)); }
        else { for (int i = 0; i < 2; ++i) { assert(257 <= base[i] && base[i] < MOD[i]); } }
        powers.resize(n + 1); prefix.assign(n + 1, {}); reversed.assign(n + 1, {});
        powers[0] = encode(1);
        for (int i = 0; i < n; ++i) {
            powers[i + 1] = multiply(powers[i], base);
            prefix[i + 1] = add(multiply(prefix[i], base), encode(code(i)));
            reversed[i + 1] = add(multiply(reversed[i], base), encode(code(n - 1 - i))); }
    }

    int size() const { return int(prefix.size()) - 1; }
    void checkRange(int l, int r) const { assert(0 <= l && l <= r && r <= size()); }
    Digest get(int l, int r) const {
        checkRange(l, r); return {subtract(prefix[r], multiply(prefix[l], powers[r - l])), r - l, base};
    }
    Digest reverseGet(int l, int r) const {
        checkRange(l, r); int n = size();
        return {subtract(reversed[n - l], multiply(reversed[n - r], powers[r - l])), r - l, base};
    }
    // T: O(1) if k<=size(), otherwise O(log(k)); M: O(1).
    Word power(int k) const {
        assert(k >= 0); if (k < int(powers.size())) { return powers[k]; }
        Word ans = encode(1), x = base;
        while (k) { if (k & 1) { ans = multiply(ans, x); } k /= 2; if (k) { x = multiply(x, x); } }
        return ans;
    }
    // T: O(1) if b.length<=size(), otherwise O(log(b.length)); no cache growth.
    Digest concat(const Digest &a, const Digest &b) const {
        assert(a.base == base && b.base == base && a.length >= 0 && b.length >= 0);
        assert(a.length <= INT_MAX - b.length);
        return {add(multiply(a.value, power(b.length)), b.value), a.length + b.length, base};
    }

    // T: O(log(k + 1)), k=min(r-l,b-a); equal bases required, answers Monte Carlo.
    int common(const StringHash &other, int l, int r, int a, int b, bool suffix) const {
        checkRange(l, r); other.checkRange(a, b); assert(base == other.base);
        int lo = 0, hi = min(r - l, b - a);
        while (lo < hi) {
            int mid = std::midpoint(lo, hi) + 1;
            bool eq = suffix ? get(r - mid, r) == other.get(b - mid, b) : get(l, l + mid) == other.get(a, a + mid);
            if (eq) { lo = mid; } else { hi = mid - 1; } }
        return lo;
    }
    int lcp(const StringHash &other, int l, int r, int a, int b) const { return common(other, l, r, a, b, false); }
    int lcs(const StringHash &other, int l, int r, int a, int b) const { return common(other, l, r, a, b, true); }
};
using StringHash64 = StringHash<true>;
