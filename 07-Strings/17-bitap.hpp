#pragma once
#include "../01-Core/01-template.hpp"

// T: NA, M: O(S * ceil(m / w)); bit i of mask c (word i / 64) is set when pattern position i accepts symbol c.
struct PatternMasks {
    int m = 0, sigma = 0, words = 0;
    vector<ulng> bits;

    const ulng *operator[](int c) const { return bits.data() + lng(c) * words; }
    bool accepts(int i, int c) const { return (*this)[c][i >> 6] >> (i & 63) & 1; }
};

namespace bitap_detail {
    template<class C> auto key(C c) {
        if constexpr (std::is_same_v<C, char>) { return uint8_t(c); }
        else { return c; }}
    template<class C> int code(C c) { return int(key(c)); }
    template<class S> bool coded(const S &s, int sigma) {
        return std::ranges::all_of(s, [&](auto c) { return std::cmp_greater_equal(key(c), 0) && std::cmp_less(key(c), sigma); });}
    template<class S> int check([[maybe_unused]] const PatternMasks &masks, const S &text) {
        assert(text.size() < INT_MAX && masks.sigma >= 1 && coded(text, masks.sigma));
        return int(text.size());}
    inline vector<int> all(int n) {
        vector<int> res(n + 1);
        iota(res.begin(), res.end(), 0);
        return res;}
    // rows[d] holds the Shift-And state with at most d errors; edits adds insertions and deletions to substitutions.
    template<class S> vector<int> errors(const PatternMasks &masks, const S &text, int k, bool edits) {
        int n = check(masks, text), m = masks.m, words = masks.words, last = (m - 1) & 63;
        assert(k >= 0);
        if (!m) { return all(n); }
        k = min(k, m);
        vector<ulng> rows(lng(k + 1) * words), old(words), keep(words);
        for (int d = 0; d <= k && edits; ++d) {
            for (int i = 0; i < d; ++i) { rows[lng(d) * words + (i >> 6)] |= 1ULL << (i & 63); }}
        vector<int> res;
        ulng *top = rows.data() + lng(k) * words;
        if (edits && top[words - 1] >> last & 1) { res.push_back(0); }
        for (int i = 0; i < n; ++i) {
            const ulng *b = masks[code(text[i])];
            for (int e = 0; e <= k; ++e) {
                int d = edits ? e : k - e;
                ulng *row = rows.data() + lng(d) * words, *low = d ? row - words : row;
                ulng in = 1, in_old = 1, in_new = 1;
                for (int w = 0; w < words; ++w) {
                    ulng cur = row[w], next = (cur << 1 | in) & b[w];
                    in = cur >> 63;
                    if (d && edits) {
                        next |= old[w] | (old[w] << 1 | in_old) | (low[w] << 1 | in_new);
                        in_old = old[w] >> 63; in_new = low[w] >> 63;}
                    else if (d) {
                        next |= low[w] << 1 | in_old;
                        in_old = low[w] >> 63;}
                    keep[w] = cur;
                    row[w] = next;}
                swap(old, keep);}
            if (top[words - 1] >> last & 1) { res.push_back(edits ? i + 1 : i + 1 - m); }}
        return res;}
} // namespace bitap_detail

// T: O(m + S * ceil(m / w)), M: O(S * ceil(m / w)) returned; symbols in [0, S) (char as unsigned), m < INT_MAX.
template<class P> PatternMasks patternMasks(const P &pattern, int sigma = 256) {
    assert(pattern.size() < INT_MAX && sigma >= 1 && bitap_detail::coded(pattern, sigma));
    int m = int(pattern.size()), words = (m + 63) / 64;
    PatternMasks res{m, sigma, words, vector<ulng>(lng(sigma) * words)};
    for (int i = 0; i < m; ++i) { res.bits[lng(bitap_detail::code(pattern[i])) * words + (i >> 6)] |= 1ULL << (i & 63); }
    return res;}
// T: O(sum of class sizes + S * ceil(m / w)), M: as above; position i accepts exactly the symbols of classes[i] (empty accepts none).
inline PatternMasks patternMasks(const vector<vector<int>> &classes, int sigma) {
    assert(classes.size() < INT_MAX && sigma >= 1);
    int m = int(classes.size()), words = (m + 63) / 64;
    PatternMasks res{m, sigma, words, vector<ulng>(lng(sigma) * words)};
    for (int i = 0; i < m; ++i) {
        assert(bitap_detail::coded(classes[i], sigma));
        for (int c : classes[i]) { res.bits[lng(c) * words + (i >> 6)] |= 1ULL << (i & 63); }}
    return res;}

// T: O(n), M: O(out); m <= 64, text symbols in [0, S); starts ascending, overlaps included, empty pattern 0..n; pattern overloads add patternMasks.
template<class S> vector<int> shiftAnd(const PatternMasks &masks, const S &text) {
    int n = bitap_detail::check(masks, text), m = masks.m;
    assert(m <= 64);
    if (!m) { return bitap_detail::all(n); }
    vector<int> res;
    ulng d = 0, hit = 1ULL << (m - 1);
    for (int i = 0; i < n; ++i) {
        d = (d << 1 | 1) & masks.bits[bitap_detail::code(text[i])];
        if (d & hit) { res.push_back(i + 1 - m); }}
    return res;}
template<class P, class S> vector<int> shiftAnd(const P &pattern, const S &text, int sigma = 256) { return shiftAnd(patternMasks(pattern, sigma), text); }
// T: O(n + S), M: O(S + out); m <= 64; complemented masks, a zero bit marks a live prefix.
template<class S> vector<int> shiftOr(const PatternMasks &masks, const S &text) {
    int n = bitap_detail::check(masks, text), m = masks.m;
    assert(m <= 64);
    if (!m) { return bitap_detail::all(n); }
    vector<ulng> b(masks.sigma);
    for (int c = 0; c < masks.sigma; ++c) { b[c] = ~masks.bits[c]; }
    vector<int> res;
    ulng d = ~0ULL, hit = 1ULL << (m - 1);
    for (int i = 0; i < n; ++i) {
        d = d << 1 | b[bitap_detail::code(text[i])];
        if (!(d & hit)) { res.push_back(i + 1 - m); }}
    return res;}
template<class P, class S> vector<int> shiftOr(const P &pattern, const S &text, int sigma = 256) { return shiftOr(patternMasks(pattern, sigma), text); }
// T: O(n * ceil(m / w)) worst, O(n * (a + 1)) when live prefixes stay below a * w symbols, M: O(ceil(m / w) + out); any m.
template<class S> vector<int> shiftOrMultiword(const PatternMasks &masks, const S &text) {
    int n = bitap_detail::check(masks, text), m = masks.m, words = masks.words, last = (m - 1) & 63;
    if (!m) { return bitap_detail::all(n); }
    vector<ulng> d(words, ~0ULL);
    vector<int> res;
    // Words above top are all ones (no live prefix), so only words 0..top + 1 change.
    for (int i = 0, top = -1; i < n; ++i) {
        const ulng *b = masks[bitap_detail::code(text[i])];
        int hi = min(top + 1, words - 1);
        ulng in = 0;
        for (int w = 0; w <= hi; ++w) {
            ulng out = d[w] >> 63;
            d[w] = d[w] << 1 | in | ~b[w];
            in = out;}
        for (top = hi; top >= 0 && d[top] == ~0ULL; --top) {}
        if (top == words - 1 && !(d[top] >> last & 1)) { res.push_back(i + 1 - m); }}
    return res;}
template<class P, class S> vector<int> shiftOrMultiword(const P &pattern, const S &text, int sigma = 256) { return shiftOrMultiword(patternMasks(pattern, sigma), text); }

// T: O(n * (k + 1) * ceil(m / w)), M: O(k * ceil(m / w) + out); starts s with at most k mismatches in text[s, s + m), any m.
template<class S> vector<int> bitapHamming(const PatternMasks &masks, const S &text, int k) { return bitap_detail::errors(masks, text, k, false); }
template<class P, class S> vector<int> bitapHamming(const P &pattern, const S &text, int k, int sigma = 256) { return bitapHamming(patternMasks(pattern, sigma), text, k); }
// T: O(n * (k + 1) * ceil(m / w)), M: O(k * ceil(m / w) + out); ends e in [0, n] with some text[s, e) within k edits (Wu-Manber).
template<class S> vector<int> bitapEdit(const PatternMasks &masks, const S &text, int k) { return bitap_detail::errors(masks, text, k, true); }
template<class P, class S> vector<int> bitapEdit(const P &pattern, const S &text, int k, int sigma = 256) { return bitapEdit(patternMasks(pattern, sigma), text, k); }

// S: O(n + S * n / w), U: O(1), Q: O(m * ((r - l) / w + 1)), M: O(S * n / w); symbols in [0, S) (char as unsigned), n < INT_MAX.
// count and occurrences cover starts i in [l, r - m] (occurrences inside [l, r)); pattern symbols outside [0, S) never match.
struct BitsetText {
    int n = 0, sigma = 0, words = 0;
    vector<int> text;
    vector<ulng> bits;

    BitsetText() = default;
    template<class S> explicit BitsetText(const S &s, int size = 256) : n(int(s.size())), sigma(size), words((n + 63) / 64), text(n), bits(lng(size) * words) {
        assert(s.size() < INT_MAX && size >= 1 && bitap_detail::coded(s, size));
        for (int i = 0; i < n; ++i) { set(i, bitap_detail::code(s[i])); }}

    int get(int i) const { assert(0 <= i && i < n); return text[i]; }

    void set(int i, int c) {
        assert(0 <= i && i < n && 0 <= c && c < sigma);
        bits[lng(text[i]) * words + (i >> 6)] &= ~(1ULL << (i & 63));
        text[i] = c;
        bits[lng(c) * words + (i >> 6)] |= 1ULL << (i & 63);}

    // f(w, x) receives word w of the candidate set; bit b of x stands for start 64 * w + b.
    template<class P, class F> void scan(const P &pattern, int l, int r, F f) const {
        assert(pattern.size() < INT_MAX && 0 <= l && l <= r && r <= n);
        int m = int(pattern.size());
        if (!m) {
            for (int i = l; i <= r; ++i) { f(i >> 6, 1ULL << (i & 63)); }
            return;}
        if (r - l < m || !bitap_detail::coded(pattern, sigma)) { return; }
        int lo = l >> 6, hi = (r - m) >> 6;
        vector<ulng> cand(hi - lo + 1, ~0ULL);
        cand[0] &= ~0ULL << (l & 63);
        cand[hi - lo] &= ~0ULL >> (63 - ((r - m) & 63));
        for (int j = 0; j < m; ++j) {
            const ulng *b = bits.data() + lng(bitap_detail::code(pattern[j])) * words;
            int q = j >> 6, s = j & 63;
            for (int w = lo; w <= hi; ++w) {
                int at = w + q;
                ulng x = at < words ? b[at] >> s : 0;
                if (s && at + 1 < words) { x |= b[at + 1] << (64 - s); }
                cand[w - lo] &= x;}}
        for (int w = lo; w <= hi; ++w) { f(w, cand[w - lo]); }}
    template<class P> int count(const P &pattern, int l = 0, int r = -1) const {
        int res = 0;
        scan(pattern, l, r == -1 ? n : r, [&](int, ulng x) { res += std::popcount(x); });
        return res;}
    template<class P> vector<int> occurrences(const P &pattern, int l = 0, int r = -1) const {
        vector<int> res;
        scan(pattern, l, r == -1 ? n : r, [&](int w, ulng x) {
            for (; x; x &= x - 1) { res.push_back(64 * w + std::countr_zero(x)); }});
        return res;}
};
