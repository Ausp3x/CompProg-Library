#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "../02-Data Structures/19-treap.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"
#include "03-stringhash.hpp"

namespace dynamic_hash_detail {
    template<int KIND> using H = StringHash<KIND>;
    template<int KIND> using Word = typename StringHash<KIND>::Word;
    template<int KIND> void checkBase([[maybe_unused]] const Word<KIND> &b) {
        if constexpr (KIND == 0) { assert(257 <= b[0] && b[0] < H<0>::MOD[0] && 257 <= b[1] && b[1] < H<0>::MOD[1]); }
        else if constexpr (KIND == 1) { assert(b >= 257 && (b & 1)); }
        else { assert(257 <= b && b < H<2>::P61); }}
    inline vector<uint> bytes(string_view s) {
        assert(s.size() < INT_MAX);
        vector<uint> res(s.size());
        for (int i = 0; i < int(s.size()); ++i) { res[i] = uint8_t(s[i]); }
        return res;}
    template<int KIND> Word<KIND> code(uint x) {
        if constexpr (KIND == 0) { assert(x < H<0>::MOD[0] - 1); }
        return H<KIND>::encode(ulng(x) + 1);}
    template<int KIND> Word<KIND> power(Word<KIND> x, ulng e) {
        Word<KIND> res = H<KIND>::encode(1);
        for (; e; e >>= 1, x = H<KIND>::multiply(x, x)) {
            if (e & 1) { res = H<KIND>::multiply(res, x); }}
        return res;}
    // Inverse of a unit: Fermat per prime, Newton iteration modulo 2^64 for odd words.
    template<int KIND> Word<KIND> inverse(const Word<KIND> &x) {
        if constexpr (KIND == 0) {
            Word<0> res;
            for (int i = 0; i < 2; ++i) {
                ulng a = x[i], r = 1, m = H<0>::MOD[i];
                for (ulng e = m - 2; e; e >>= 1, a = a * a % m) {
                    if (e & 1) { r = r * a % m; }}
                res[i] = uint(r);}
            return res;}
        else if constexpr (KIND == 1) {
            ulng y = x;
            for (int i = 0; i < 5; ++i) { y *= 2 - x * y; }
            return y;}
        else { return power<2>(x, H<2>::P61 - 2); }}
    // Field element as a commutative group for Fenwick.
    template<int KIND> struct Value {
        Word<KIND> w{};

        Value(int = 0) {}
        Value(const Word<KIND> &x) : w(x) {}
        Value &operator+=(const Value &o) { w = H<KIND>::add(w, o.w); return *this; }
        Value &operator-=(const Value &o) { w = H<KIND>::subtract(w, o.w); return *this; }
        friend Value operator+(Value a, const Value &b) { return a += b; }
        friend Value operator-(Value a, const Value &b) { return a -= b; }
    };
} // namespace dynamic_hash_detail

// T: O(1) per operation, M: O(1); S = (forward hash, reversed hash, B^length) under StringHash<KIND> arithmetic.
// op concatenates, reverse reads backwards, F assigns one symbol to a whole range (KIND != 1); for ImplicitTreap and LazySegmentTree.
template<int KIND = 0> struct HashMonoid {
    using H = StringHash<KIND>;
    using Word = typename H::Word;
    struct S { Word h, r, p; };
    struct F { Word k; bool assign; };

    static S e() { return {Word{}, Word{}, H::encode(1)}; }
    static S op(const S &a, const S &b) { return {H::add(H::multiply(a.h, b.p), b.h), H::add(H::multiply(b.r, a.p), a.r), H::multiply(a.p, b.p)}; }
    static S reverse(const S &a) { return {a.r, a.h, a.p}; }
    // k = code / (B - 1), so a range of length L gets code * (B^L - 1) / (B - 1).
    static S mapping(const F &f, const S &x) {
        if (!f.assign) { return x; }
        Word v = H::multiply(f.k, H::subtract(x.p, H::encode(1)));
        return {v, v, x.p};}
    static F composition(const F &f, const F &g) { return f.assign ? f : g; }
    static F id() { return {Word{}, false}; }
};

// S: O(n), U: O(log(n)), Q: O(log(n)) get/reverseGet, O(log(n)^2) lcp/lcs/compare, M: O(n); symbols and base as StringHash<KIND>.
// Digests equal StringHash<KIND> digests under the same base; lcp/lcs/compare of s[l, r) and s[a, b) are Monte Carlo.
template<int KIND = 0> struct PointUpdateHash {
    using H = StringHash<KIND>;
    using Word = typename H::Word;
    using Digest = typename H::Digest;
    using Value = dynamic_hash_detail::Value<KIND>;
    Word base;
    vector<uint> symbols;
    vector<Word> powers, inverses;
    Fenwick<Value> forward, backward;

    explicit PointUpdateHash(vector<uint> s, Word b = H::defaultBase()) : base(b), symbols(std::move(s)) {
        assert(symbols.size() < INT_MAX);
        dynamic_hash_detail::checkBase<KIND>(base);
        int n = size();
        powers.assign(n + 1, H::encode(1)); inverses.assign(n + 1, H::encode(1));
        Word inv = dynamic_hash_detail::inverse<KIND>(base);
        for (int k = 0; k < n; ++k) {
            powers[k + 1] = H::multiply(powers[k], base);
            inverses[k + 1] = H::multiply(inverses[k], inv);}
        vector<Value> f(n), g(n);
        for (int i = 0; i < n; ++i) {
            Word c = dynamic_hash_detail::code<KIND>(symbols[i]);
            f[i] = H::multiply(c, powers[n - 1 - i]); g[i] = H::multiply(c, powers[i]);}
        forward = Fenwick<Value>(std::move(f)); backward = Fenwick<Value>(std::move(g));}
    explicit PointUpdateHash(string_view s, Word b = H::defaultBase()) : PointUpdateHash(dynamic_hash_detail::bytes(s), b) {}

    int size() const { return int(symbols.size()); }
    void checkRange([[maybe_unused]] int l, [[maybe_unused]] int r) const { assert(0 <= l && l <= r && r <= size()); }
    void set(int i, uint x) {
        int n = size();
        assert(0 <= i && i < n);
        Word d = H::subtract(dynamic_hash_detail::code<KIND>(x), dynamic_hash_detail::code<KIND>(symbols[i]));
        forward.add(i, H::multiply(d, powers[n - 1 - i])); backward.add(i, H::multiply(d, powers[i]));
        symbols[i] = x;}

    Digest get(int l, int r) const {
        checkRange(l, r);
        return {H::multiply(forward.sum(l, r).w, inverses[size() - r]), r - l, base};}
    Digest reverseGet(int l, int r) const {
        checkRange(l, r);
        return {H::multiply(backward.sum(l, r).w, inverses[l]), r - l, base};}
    int common(int l, int r, int a, int b, bool suffix) const {
        checkRange(l, r); checkRange(a, b);
        int lo = 0, hi = min(r - l, b - a);
        while (lo < hi) {
            int mid = std::midpoint(lo, hi) + 1;
            bool eq = suffix ? get(r - mid, r) == get(b - mid, b) : get(l, l + mid) == get(a, a + mid);
            if (eq) { lo = mid; }
            else { hi = mid - 1; }}
        return lo;}
    int lcp(int l, int r, int a, int b) const { return common(l, r, a, b, false); }
    int lcs(int l, int r, int a, int b) const { return common(l, r, a, b, true); }
    int compare(int l, int r, int a, int b) const {
        int k = lcp(l, r, a, b);
        if (k == r - l || k == b - a) { return (r - l > b - a) - (r - l < b - a); }
        return symbols[l + k] < symbols[a + k] ? -1 : 1;}
};

// S: O(1), U: O(log(n)) expected, Q: O(log(n)) expected get/reverseGet, O(log(n)^2) expected lcp/lcs/compare, M: O(m) for m nodes.
// ImplicitTreap over HashMonoid<KIND> and one base: its insert, erase, set, split, merge, reverse, apply take leaf and assignment values.
template<int KIND = 0> struct SequenceHash : ImplicitTreap<HashMonoid<KIND>> {
    using Tree = ImplicitTreap<HashMonoid<KIND>>;
    using S = typename HashMonoid<KIND>::S;
    using F = typename HashMonoid<KIND>::F;
    using H = StringHash<KIND>;
    using Word = typename H::Word;
    using Digest = typename H::Digest;
    Word base, ratio{};

    explicit SequenceHash(Word b = H::defaultBase(), ulng seed = CustomHash::rnd) : Tree(seed), base(b) {
        dynamic_hash_detail::checkBase<KIND>(base);
        if constexpr (KIND != 1) { ratio = dynamic_hash_detail::inverse<KIND>(H::subtract(base, H::encode(1))); }}
    S leaf(uint x) const {
        Word c = dynamic_hash_detail::code<KIND>(x);
        return {c, c, base};}
    // T: O(n), M: O(n).
    vector<S> leaves(const vector<uint> &s) const {
        vector<S> res;
        for (uint x : s) { res.push_back(leaf(x)); }
        return res;}
    vector<S> leaves(string_view s) const { return leaves(dynamic_hash_detail::bytes(s)); }
    F assignment(uint x) const {
        static_assert(KIND != 1, "B - 1 is not invertible modulo 2^64");
        return {H::multiply(dynamic_hash_detail::code<KIND>(x), ratio), true};}

    Digest get(int &v, int l, int r) { return {this->prod(v, l, r).h, r - l, base}; }
    Digest reverseGet(int &v, int l, int r) { return {this->prod(v, l, r).r, r - l, base}; }
    int common(int &v, int l, int r, int a, int b, bool suffix) {
        assert(0 <= l && l <= r && r <= this->size(v) && 0 <= a && a <= b && b <= this->size(v));
        int lo = 0, hi = min(r - l, b - a);
        while (lo < hi) {
            int mid = std::midpoint(lo, hi) + 1;
            bool eq = suffix ? get(v, r - mid, r) == get(v, b - mid, b) : get(v, l, l + mid) == get(v, a, a + mid);
            if (eq) { lo = mid; }
            else { hi = mid - 1; }}
        return lo;}
    int lcp(int &v, int l, int r, int a, int b) { return common(v, l, r, a, b, false); }
    int lcs(int &v, int l, int r, int a, int b) { return common(v, l, r, a, b, true); }
    int compare(int &v, int l, int r, int a, int b) {
        int k = lcp(v, l, r, a, b);
        if (k == r - l || k == b - a) { return (r - l > b - a) - (r - l < b - a); }
        auto first = [](const Word &w) { if constexpr (KIND == 0) { return w[0]; } else { return w; } };
        return first(this->at(v, l + k).h) < first(this->at(v, a + k).h) ? -1 : 1;}
};

// S: O(1), U: O(1) amortized, Q: O(1), M: O(n) for the largest size reached; symbols and base as StringHash<KIND>, front pushes divide by the base.
// prefix[i] - prefix[j] * B^(i - j) is the digest of positions [j, i) counted from the current front.
template<int KIND = 0> struct DequeHash {
    using H = StringHash<KIND>;
    using Word = typename H::Word;
    using Digest = typename H::Digest;
    Word base, inv;
    deque<Word> prefix{Word{}};
    vector<Word> powers{H::encode(1)};

    explicit DequeHash(Word b = H::defaultBase()) : base(b), inv(dynamic_hash_detail::inverse<KIND>(b)) { dynamic_hash_detail::checkBase<KIND>(b); }

    int size() const { return int(prefix.size()) - 1; }

    void grow() {
        assert(prefix.size() < size_t(INT_MAX));
        if (powers.size() < prefix.size()) { powers.push_back(H::multiply(powers.back(), base)); }}
    void pushBack(uint x) { prefix.push_back(H::add(H::multiply(prefix.back(), base), dynamic_hash_detail::code<KIND>(x))); grow(); }
    void pushFront(uint x) { prefix.push_front(H::multiply(H::subtract(prefix.front(), dynamic_hash_detail::code<KIND>(x)), inv)); grow(); }
    void popBack() { assert(size() > 0); prefix.pop_back(); }
    void popFront() { assert(size() > 0); prefix.pop_front(); }

    Digest get(int l, int r) const {
        assert(0 <= l && l <= r && r <= size());
        return {H::subtract(prefix[r], H::multiply(prefix[l], powers[r - l])), r - l, base};}
};
