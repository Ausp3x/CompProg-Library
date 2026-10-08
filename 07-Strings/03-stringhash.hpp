#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n), U: NA, Q: O(1), M: O(n); n < INT_MAX; KIND 0 primes 1e9+7 and 1e9+9, 1 wraps mod 2^64, 2 is mod 2^61-1.
// Monte Carlo fingerprints; codes reduce mod the field; byte/uint symbols encode x + 1, KIND 0 needs x < 1000000006.
template<int KIND = 0> struct StringHash {
    using Word = std::conditional_t<KIND != 0, ulng, array<uint, 2>>;
    static constexpr array<uint, 2> MOD{1000000007, 1000000009};
    static constexpr ulng P61 = (1ULL << 61) - 1;
    struct Digest {
        Word value;
        int length;
        Word base;
        auto operator<=>(const Digest &) const = default;
    };
    Word base;
    vector<Word> powers, prefix, reversed;

    static Word defaultBase() {
        if constexpr (KIND == 0) { return {911382323, 972663749}; }
        else if constexpr (KIND == 1) { return 11400714819323198485ULL; }
        else { return 2054820241022868036ULL; }}
    static Word randomBase(ulng seed) {
        std::mt19937_64 rng(seed);
        auto pick = [&](ulng hi) { return std::uniform_int_distribution<ulng>(257, hi)(rng); };
        if constexpr (KIND == 0) { return {uint(pick(MOD[0] - 1)), uint(pick(MOD[1] - 1))}; }
        else if constexpr (KIND == 1) {
            ulng b;
            do { b = rng() | 1; } while (b < 257);
            return b;}
        else { return pick(P61 - 2); }}
    static void checkSymbols([[maybe_unused]] const vector<uint> &s) {
        if constexpr (KIND == 0) { assert(std::ranges::all_of(s, [](uint x) { return x < MOD[0] - 1; })); }}
    explicit StringHash(string_view s = {}, Word b = defaultBase()) : base(b) {
        assert(s.size() < INT_MAX);
        build(int(s.size()), [&](int i) { return ulng(uint8_t(s[i])) + 1; });}
    explicit StringHash(const vector<uint> &s, Word b = defaultBase()) : base(b) {
        assert(s.size() < INT_MAX);
        checkSymbols(s);
        build(int(s.size()), [&](int i) { return ulng(s[i]) + 1; });}

    static Word encode(ulng x) {
        if constexpr (KIND == 0) { return {uint(x % MOD[0]), uint(x % MOD[1])}; }
        else if constexpr (KIND == 1) { return x; }
        else { return x % P61; }}
    static Word add(Word a, Word b) {
        if constexpr (KIND == 0) {
            for (int i = 0; i < 2; ++i) {
                a[i] += b[i];
                if (a[i] >= MOD[i]) { a[i] -= MOD[i]; }}
            return a;}
        else if constexpr (KIND == 1) { return a + b; }
        else { a += b; return a >= P61 ? a - P61 : a; }}
    static Word subtract(Word a, Word b) {
        if constexpr (KIND == 0) {
            for (int i = 0; i < 2; ++i) { a[i] = a[i] >= b[i] ? a[i] - b[i] : a[i] + MOD[i] - b[i]; }
            return a;}
        else if constexpr (KIND == 1) { return a - b; }
        else { return a >= b ? a - b : a + P61 - b; }}
    static Word multiply(Word a, Word b) {
        if constexpr (KIND == 0) {
            for (int i = 0; i < 2; ++i) { a[i] = uint(ulng(a[i]) * b[i] % MOD[i]); }
            return a;}
        else if constexpr (KIND == 1) { return a * b; }
        else {
            ulll t = ulll(a) * b;
            ulng x = (ulng(t) & P61) + ulng(t >> 61);
            return x >= P61 ? x - P61 : x;}}
    template<class F> void build(int n, F code) {
        assert(0 <= n && n < INT_MAX);
        if constexpr (KIND == 0) {
            for (int i = 0; i < 2; ++i) { assert(257 <= base[i] && base[i] < MOD[i]); }}
        else if constexpr (KIND == 1) { assert(base >= 257 && (base & 1)); }
        else { assert(257 <= base && base < P61); }
        powers.resize(n + 1); prefix.assign(n + 1, {}); reversed.assign(n + 1, {});
        powers[0] = encode(1);
        for (int i = 0; i < n; ++i) {
            powers[i + 1] = multiply(powers[i], base);
            prefix[i + 1] = add(multiply(prefix[i], base), encode(code(i)));
            reversed[i + 1] = add(multiply(reversed[i], base), encode(code(n - 1 - i)));}}

    int size() const { return int(prefix.size()) - 1; }
    void checkRange([[maybe_unused]] int l, [[maybe_unused]] int r) const { assert(0 <= l && l <= r && r <= size()); }
    Digest get(int l, int r) const {
        checkRange(l, r);
        return {subtract(prefix[r], multiply(prefix[l], powers[r - l])), r - l, base};}
    Digest reverseGet(int l, int r) const {
        checkRange(l, r);
        int n = size();
        return {subtract(reversed[n - l], multiply(reversed[n - r], powers[r - l])), r - l, base};}
    // T: O(L), M: O(1); digest of an outside sequence under this base, without storing it.
    Digest hashOf(string_view s) const {
        assert(s.size() < INT_MAX);
        Word h{};
        for (char c : s) { h = add(multiply(h, base), encode(ulng(uint8_t(c)) + 1)); }
        return {h, int(s.size()), base};}
    Digest hashOf(const vector<uint> &s) const {
        assert(s.size() < INT_MAX);
        checkSymbols(s);
        Word h{};
        for (uint x : s) { h = add(multiply(h, base), encode(ulng(x) + 1)); }
        return {h, int(s.size()), base};}
    // T: O(1) if k <= size(), otherwise O(log(k)); M: O(1).
    Word power(int k) const {
        assert(k >= 0);
        if (k < int(powers.size())) { return powers[k]; }
        Word res = encode(1), x = base;
        for (; k; k /= 2, x = multiply(x, x)) {
            if (k & 1) { res = multiply(res, x); }}
        return res;}
    // T: O(1) if b.length <= size(), otherwise O(log(b.length)); M: O(1); no cache growth.
    Digest concat(const Digest &a, const Digest &b) const {
        assert(a.base == base && b.base == base && a.length >= 0 && b.length >= 0);
        assert(a.length <= INT_MAX - b.length);
        return {add(multiply(a.value, power(b.length)), b.value), a.length + b.length, base};}

    // T: O(log(k + 1)), M: O(1); k = min(r - l, b - a); equal bases, Monte Carlo common prefix/suffix length.
    int common(const StringHash &other, int l, int r, int a, int b, bool suffix) const {
        checkRange(l, r); other.checkRange(a, b);
        assert(base == other.base);
        int lo = 0, hi = min(r - l, b - a);
        while (lo < hi) {
            int mid = std::midpoint(lo, hi) + 1;
            bool eq = suffix ? get(r - mid, r) == other.get(b - mid, b) : get(l, l + mid) == other.get(a, a + mid);
            if (eq) { lo = mid; }
            else { hi = mid - 1; }}
        return lo;}
    int lcp(const StringHash &other, int l, int r, int a, int b) const { return common(other, l, r, a, b, false); }
    int lcs(const StringHash &other, int l, int r, int a, int b) const { return common(other, l, r, a, b, true); }
};
using StringHash64 = StringHash<1>;
using StringHash61 = StringHash<2>;
