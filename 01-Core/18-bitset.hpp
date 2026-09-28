#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif

// Runtime-length bits, bit 0 is least significant. Ranges are [l, r).
// Equal sizes are required for Boolean operations/subset/intersection (not equality).
// Shifts >= size yield zero; rotations reduce modulo size (empty is unchanged).
// Scans return size() on absence; next/prev are strictly after/before p <= size().
// Public n/a are representation, not mutation APIs: a.size() == ceil(n / 64), padding zero.
// Proxies/spans invalidate on resize, clear, push/pop, move, assignment and swap.
// Other mutations preserve proxies; scans have no persistent iterators. Not thread-safe.
// All mutating operations support self-aliasing; moved-from objects are empty.
// T: O(ceil(n / 64)) bulk operations/scans, O(1) bit access; M: O(ceil(n / 64)).
// Resize: worst-case O(old words + new words) on growth, O(1) on shrink.
// pushBack: amortized O(1); popBack/clear/swap/size/empty/blocks are O(1).
// Rotation/copy/nonmutating operators use O(ceil(n / 64)) extra result/workspace.
struct Bitset {
    enum class Op { Assign, And, Or, Xor, AndNot };
    size_t n = 0;
    vector<ulng> a;
    static constexpr size_t SIMD_WORDS = 16;

    static size_t wordCount(size_t bits) { return bits / 64 + (bits % 64 != 0); }
    static ulng lowMask(size_t bits) { assert(bits <= 64); return bits == 64 ? ~ulng(0) : (ulng(1) << bits) - 1; }
    void trim() { if (n % 64) { a.back() &= lowMask(n % 64); } }

    explicit Bitset(size_t bits = 0, bool value = false) : n(bits), a(wordCount(bits), value ? ~ulng(0) : 0) { trim(); }
    // T: O(n), M: O(ceil(n / 64)); binary strings are most-significant-bit first.
    explicit Bitset(string_view s) : Bitset(s.size()) {
        for (size_t i = 0; i < n; ++i) {
            assert(s[i] == '0' || s[i] == '1');
            if (s[i] == '1') { set(n - 1 - i); }}}
    Bitset(const Bitset &) = default;
    Bitset &operator=(const Bitset &b) { a = b.a; n = b.n; return *this; }
    Bitset(Bitset &&b) noexcept : n(std::exchange(b.n, 0)), a(std::move(b.a)) { b.a.clear(); }
    Bitset &operator=(Bitset &&b) noexcept {
        if (this != &b) { n = std::exchange(b.n, 0); a = std::move(b.a); b.a.clear(); }
        return *this;}
    void swap(Bitset &b) noexcept { std::swap(n, b.n); a.swap(b.a); }
    static Bitset fromWords(size_t bits, std::span<const ulng> words) {
        assert(words.size() == wordCount(bits));
        Bitset b(bits);
        std::copy(words.begin(), words.end(), b.a.begin());
        b.trim(); return b;}

    std::span<const ulng> blocks() const { return a; }
    size_t size() const { return n; }
    bool empty() const { return !n; }

    void clear() { n = 0; a.clear(); }
    void resize(size_t bits, bool value = false) {
        size_t old = n;
        a.resize(wordCount(bits), 0); n = bits;
        if (value && bits > old) { setRange(old, bits); }
        trim();}
    void pushBack(bool value) {
        assert(n < SIZE_MAX);
        size_t old = n;
        resize(n + 1); set(old, value);}
    void popBack() { assert(n); resize(n - 1); }

    bool test(size_t p) const { assert(p < n); return (a[p / 64] >> (p % 64)) & 1; }
    bool operator[](size_t p) const { return test(p); }
    struct Reference {
        ulng *word; ulng mask;

        operator bool() const { return (*word & mask) != 0; }
        Reference &operator=(bool value) { if (value) { *word |= mask; } else { *word &= ~mask; } return *this; }
        Reference &operator=(const Reference &b) { return *this = bool(b); }
        Reference &flip() { *word ^= mask; return *this; }
    };
    Reference operator[](size_t p) { assert(p < n); return {&a[p / 64], ulng(1) << (p % 64)}; }

    Bitset &set(size_t p, bool value = true) { (*this)[p] = value; return *this; }
    Bitset &reset(size_t p) { return set(p, false); }
    Bitset &flip(size_t p) { (*this)[p].flip(); return *this; }

    Bitset &set() { std::fill(a.begin(), a.end(), ~ulng(0)); trim(); return *this; }
    Bitset &reset() { std::fill(a.begin(), a.end(), 0); return *this; }
    Bitset &flip() { for (auto &x : a) { x = ~x; } trim(); return *this; }

    template<Op OP> static ulng apply(ulng x, ulng y) {
        if constexpr (OP == Op::Assign) { return y; }
        if constexpr (OP == Op::And) { return x & y; }
        if constexpr (OP == Op::Or) { return x | y; }
        if constexpr (OP == Op::Xor) { return x ^ y; }
        if constexpr (OP == Op::AndNot) { return x & ~y; }}
#ifdef __AVX2__
    template<Op OP> static __m256i applyVector(__m256i x, __m256i y) {
        if constexpr (OP == Op::Assign) { return y; }
        if constexpr (OP == Op::And) { return _mm256_and_si256(x, y); }
        if constexpr (OP == Op::Or) { return _mm256_or_si256(x, y); }
        if constexpr (OP == Op::Xor) { return _mm256_xor_si256(x, y); }
        if constexpr (OP == Op::AndNot) { return _mm256_andnot_si256(y, x); }}

    static __m256i load(const ulng *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
    static void store(ulng *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }

    static __m256i popcounts(__m256i x) {
        const __m256i lut = _mm256_setr_epi8(0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4);
        __m256i mask = _mm256_set1_epi8(15);
        __m256i lo = _mm256_shuffle_epi8(lut, _mm256_and_si256(x, mask));
        __m256i hi = _mm256_shuffle_epi8(lut, _mm256_and_si256(_mm256_srli_epi16(x, 4), mask));
        return _mm256_sad_epu8(_mm256_add_epi8(lo, hi), _mm256_setzero_si256());}
#endif

    // T: O(1 + (r - l) / 64).
    template<Op OP> Bitset &range(size_t l, size_t r) {
        assert(l <= r && r <= n);
        if (l == r) { return *this; }
        size_t first = l / 64, last = (r - 1) / 64;
        if (first == last) { a[first] = apply<OP>(a[first], lowMask((r - 1) % 64 + 1) & ~lowMask(l % 64)); }
        else {
            a[first] = apply<OP>(a[first], ~lowMask(l % 64));
            for (size_t i = first + 1; i < last; ++i) { a[i] = apply<OP>(a[i], ~ulng(0)); }
            a[last] = apply<OP>(a[last], lowMask((r - 1) % 64 + 1)); }
        return *this;}
    Bitset &setRange(size_t l, size_t r, bool value = true) { return value ? range<Op::Or>(l, r) : range<Op::AndNot>(l, r); }
    Bitset &resetRange(size_t l, size_t r) { return range<Op::AndNot>(l, r); }
    Bitset &flipRange(size_t l, size_t r) { return range<Op::Xor>(l, r); }
    static Bitset rangeMask(size_t bits, size_t l, size_t r) { Bitset b(bits); b.setRange(l, r); return b; }

    template<Op OP> Bitset &combine(const Bitset &b) {
        assert(n == b.n);
        for (size_t i = 0; i < a.size(); ++i) { a[i] = apply<OP>(a[i], b.a[i]); }
        return *this;}

    template<Op OP> static size_t countWords(const ulng *x, const ulng *y, size_t words) {
        size_t res = 0, i = 0;
#ifdef __AVX2__
        if (words >= SIMD_WORDS) {
            __m256i sum = _mm256_setzero_si256();
            for (; i + 4 <= words; i += 4) { sum = _mm256_add_epi64(sum, popcounts(applyVector<OP>(load(x + i), load(y + i)))); }
            ulng lanes[4]; store(lanes, sum);
            res = lanes[0] + lanes[1] + lanes[2] + lanes[3]; }
#endif
        for (; i < words; ++i) { res += std::popcount(apply<OP>(x[i], y[i])); }
        return res;}
    template<Op OP> size_t countWith(const Bitset &b) const {
        assert(n == b.n); return countWords<OP>(a.data(), b.a.data(), a.size());}
    size_t count() const { return countWith<Op::Assign>(*this); }
    size_t countAnd(const Bitset &b) const { return countWith<Op::And>(b); }
    size_t countOr(const Bitset &b) const { return countWith<Op::Or>(b); }
    size_t countXor(const Bitset &b) const { return countWith<Op::Xor>(b); }

    // T: O(1 + (r - l) / 64).
    size_t count(size_t l, size_t r) const {
        assert(l <= r && r <= n);
        if (l == r) { return 0; }
        size_t first = l / 64, last = (r - 1) / 64;
        if (first == last) { return std::popcount(a[first] & ~lowMask(l % 64) & lowMask((r - 1) % 64 + 1)); }
        size_t res = std::popcount(a[first] & ~lowMask(l % 64)) + std::popcount(a[last] & lowMask((r - 1) % 64 + 1));
        return res + countWords<Op::Assign>(a.data() + first + 1, a.data() + first + 1, last - first - 1);}

    bool any() const { return std::any_of(a.begin(), a.end(), [](ulng x) { return x != 0; }); }
    bool none() const { return !any(); }
    bool all() const {
        if (a.empty()) { return true; }
        for (size_t i = 0; i + 1 < a.size(); ++i) { if (a[i] != ~ulng(0)) { return false; } }
        return a.back() == lowMask(n % 64 ? n % 64 : 64);}

    bool intersects(const Bitset &b) const {
        assert(n == b.n);
        for (size_t i = 0; i < a.size(); ++i) { if (a[i] & b.a[i]) { return true; } }
        return false;}
    bool isSubsetOf(const Bitset &b) const {
        assert(n == b.n);
        for (size_t i = 0; i < a.size(); ++i) { if (a[i] & ~b.a[i]) { return false; } }
        return true;}

    size_t findFrom(size_t p) const {
        assert(p <= n);
        if (p == n) { return n; }
        size_t i = p / 64; ulng x = a[i] & ~lowMask(p % 64);
        while (!x) { if (++i == a.size()) { return n; } x = a[i]; }
        return 64 * i + std::countr_zero(x);}
    size_t findFirst() const { return findFrom(0); }
    size_t findNext(size_t p) const { assert(p <= n); return p == n ? n : findFrom(p + 1); }
    size_t findPrev(size_t p) const {
        assert(p <= n);
        if (!p) { return n; }
        size_t i = (p - 1) / 64; ulng x = a[i] & lowMask((p - 1) % 64 + 1);
        while (!x) { if (!i) { return n; } x = a[--i]; }
        return 64 * i + 63 - std::countl_zero(x);}
    size_t findLast() const { return findPrev(n); }

    ulng shiftedWord(size_t i, size_t d, size_t s, bool left) const {
        assert(i < a.size() && s < 64);
        if (left) {
            if (i < d) { return 0; }
            ulng x = a[i - d] << s;
            if (s && i > d) { x |= a[i - d - 1] >> (64 - s); }
            return x;}
        if (d >= a.size() - i) { return 0; }
        ulng x = a[i + d] >> s;
        if (s && i + d + 1 < a.size()) { x |= a[i + d + 1] << (64 - s); }
        return x;}
    // this OP= (b shifted k); b may alias this, with original-value semantics.
    // T: O(ceil(n / 64)), workspace O(1), including self-aliasing.
    template<Op OP> Bitset &combineShift(const Bitset &b, size_t k, bool left = true) {
        assert(n == b.n);
        if (k >= n) {
            if constexpr (OP == Op::Assign || OP == Op::And) { reset(); }
            return *this;}
        if (!k) { return combine<OP>(b); }
        size_t d = k / 64, i = left ? a.size() : 0;
        int s = int(k % 64);
#ifdef __AVX2__
        if (a.size() >= SIMD_WORDS) {
            __m128i shift = _mm_cvtsi64_si128(s), back = _mm_cvtsi64_si128(64 - s);
            if (left) {
                while (i >= 4 && i - 4 >= d + (s != 0)) {
                    i -= 4;
                    __m256i x = _mm256_sll_epi64(load(b.a.data() + i - d), shift);
                    if (s) { x = _mm256_or_si256(x, _mm256_srl_epi64(load(b.a.data() + i - d - 1), back)); }
                    store(a.data() + i, applyVector<OP>(load(a.data() + i), x)); }}
            else {
                while (i + 4 <= a.size() && d + (s != 0) <= a.size() - i - 4) {
                    __m256i x = _mm256_srl_epi64(load(b.a.data() + i + d), shift);
                    if (s) { x = _mm256_or_si256(x, _mm256_sll_epi64(load(b.a.data() + i + d + 1), back)); }
                    store(a.data() + i, applyVector<OP>(load(a.data() + i), x));
                    i += 4; }}}
#endif
        if (left) {
            while (i > d) { --i; a[i] = apply<OP>(a[i], b.shiftedWord(i, d, s, true)); }
            if constexpr (OP == Op::Assign || OP == Op::And) { std::fill(a.begin(), a.begin() + ptrdiff_t(i), 0); }}
        else {
            for (; i < a.size() - d; ++i) { a[i] = apply<OP>(a[i], b.shiftedWord(i, d, s, false)); }
            if constexpr (OP == Op::Assign || OP == Op::And) { std::fill(a.begin() + ptrdiff_t(i), a.end(), 0); }}
        trim(); return *this;}

    Bitset &operator<<=(size_t k) { return combineShift<Op::Assign>(*this, k); }
    Bitset &operator>>=(size_t k) { return combineShift<Op::Assign>(*this, k, false); }

    Bitset &rotateLeft(size_t k) {
        if (n && (k %= n)) {
            Bitset b(*this);
            *this <<= k; combineShift<Op::Or>(b, n - k, false); }
        return *this;}
    Bitset &rotateRight(size_t k) {
        if (n && (k %= n)) {
            Bitset b(*this);
            *this >>= k; combineShift<Op::Or>(b, n - k); }
        return *this;}

    Bitset &operator&=(const Bitset &b) { return combine<Op::And>(b); }
    Bitset &operator|=(const Bitset &b) { return combine<Op::Or>(b); }
    Bitset &operator^=(const Bitset &b) { return combine<Op::Xor>(b); }
    Bitset &operator-=(const Bitset &b) { return combine<Op::AndNot>(b); }

    friend bool operator==(const Bitset &b, const Bitset &c) { return b.n == c.n && b.a == c.a; }
    friend Bitset operator&(Bitset b, const Bitset &c) { b &= c; return b; }
    friend Bitset operator|(Bitset b, const Bitset &c) { b |= c; return b; }
    friend Bitset operator^(Bitset b, const Bitset &c) { b ^= c; return b; }
    friend Bitset operator-(Bitset b, const Bitset &c) { b -= c; return b; }
    friend Bitset operator~(Bitset b) { b.flip(); return b; }
    friend Bitset operator<<(Bitset b, size_t k) { b <<= k; return b; }
    friend Bitset operator>>(Bitset b, size_t k) { b >>= k; return b; }

    // T: O(n), returned storage O(n).
    string toString() const {
        string s(n, '0');
        for (size_t i = 0; i < n; ++i) { if (test(i)) { s[n - 1 - i] = '1'; } }
        return s;}
    friend string debugString(const Bitset &b) { return b.toString(); }
};
