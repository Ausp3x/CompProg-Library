#pragma once
#include "../01-Core/01-template.hpp"

namespace bit_operations_detail {
    // T: O(1) (2048 constexpr entries), M: O(1); at[k][v] is the position of the k-th set bit of byte v.
    struct SelectTable {
        unsigned char at[8][256]{};
        constexpr SelectTable() {
            for (int v = 0; v < 256; ++v) {
                for (int i = 0, c = 0; i < 8; ++i) {
                    if (v >> i & 1) { at[c++][v] = static_cast<unsigned char>(i); }}}}
    };
    inline constexpr SelectTable SELECT_TABLE{};

    // T: O(1), M: O(1); k-th set bit of x or -1. Byte counts summed by multiplication pick the byte, the table the bit.
    constexpr int select64(ulng x, int k) {
        constexpr ulng L = 0x0101010101010101ULL, H = 0x8080808080808080ULL;
        ulng s = x - ((x >> 1) & 0x5555555555555555ULL);
        s = (s & 0x3333333333333333ULL) + ((s >> 2) & 0x3333333333333333ULL);
        s = ((s + (s >> 4)) & 0x0f0f0f0f0f0f0f0fULL) * L;
        if (ulng(k) >= s >> 56) { return -1; }
        ulng le = ((ulng(k) * L | H) - s) & H;
        int b = int((le >> 7) * L >> 53) & ~7;
        k -= int((s << 8) >> b & 0xff);
        return b + SELECT_TABLE.at[k][(x >> b) & 0xff];}

    template<typename U>
    inline constexpr int LOG_W = std::countr_zero(unsigned(std::numeric_limits<U>::digits));
    // BLOCK_MASK<U>[k]: the low 2^k bits of every 2^(k+1)-bit block, for k < LOG_W.
    template<typename U>
    inline constexpr array<U, 7> BLOCK_MASK = [] {
        array<U, 7> res{};
        for (int k = 0; k < LOG_W<U>; ++k) { res[k] = U(U(~U(0)) / U(U(U(1) << (1 << k)) + 1)); }
        return res;}();

    // T: O(log(w)), M: O(1); spread moves bit i of the low half to bit 2i, pack inverts it on the even bits.
    template<typename U>
    constexpr U spread(U v) {
        for (int k = LOG_W<U> - 2; k >= 0; --k) {
            v = U(U(v | U(v << (1 << k))) & BLOCK_MASK<U>[k]);}
        return v;}
    template<typename U>
    constexpr U pack(U v) {
        v &= BLOCK_MASK<U>[0];
        for (int k = 0; k + 1 < LOG_W<U>; ++k) { v = U(U(v | U(v >> (1 << k))) & BLOCK_MASK<U>[k + 1]); }
        return v;}
} // namespace bit_operations_detail

// T: O(1) (bitReverse, grayDecode, morton O(log(w))), M: O(1); constexpr, unsigned words through 128 bits, positions [0, W), counts [0, W].
// Zero: lowBit 0, first/last -1, select -1 past the last bit; failed ceil, stepping and lower bound leave the output unchanged.
template<std::unsigned_integral U = ulng>
requires (std::is_same_v<U, std::remove_cv_t<U>> && requires(U x) { std::popcount(x); })
struct BitOps {
    static constexpr int W = std::numeric_limits<U>::digits;
    static constexpr U BYTES = U(U(~U(0)) / 255), HIGHS = U(BYTES << 7);
    static_assert(W <= 128);

    static constexpr int first(U x) { return x ? std::countr_zero(x) : -1; }
    static constexpr int last(U x) { return std::bit_width(x) - 1; }
    static constexpr bool parity(U x) { return std::popcount(x) & 1; }
    static constexpr int countLeadingSignBits(U x) { return (x >> (W - 1) ? std::countl_one(x) : std::countl_zero(x)) - 1; }
    static constexpr int rank(U x, int i) { return std::popcount(U(x & lowMask(i))); }
    static constexpr int select(U x, int k) {
        assert(k >= 0); int base = 0;
        if constexpr (W > 64) {
            if (int c = std::popcount(ulng(x)); k >= c) { k -= c; x = U(x >> 64); base = 64; }}
        int res = bit_operations_detail::select64(ulng(x), k);
        return res < 0 ? -1 : base + res;}
    // T: O(w), M: O(1); popcounts of 0..n summed, for W <= 64 (the total needs up to W + 6 bits).
    static constexpr ulll countOnesUpTo(U n) requires (W <= 64) {
        ulll m = ulll(n) + 1, res = 0;
        for (int k = 0; k < W; ++k) {
            ulll half = ulll(1) << k, rest = m % (2 * half);
            res += m / (2 * half) * half + (rest > half ? rest - half : 0);}
        return res;}
    static constexpr U lowBit(U x) { return U(x & U(U(0) - x)); }
    static constexpr bool ceil(U x, U &out) {
        if (x > U(U(1) << (W - 1))) { return false; }
        out = std::bit_ceil(x); return true;}

    static constexpr U lowMask(int k) {
        assert(0 <= k && k <= W);
        return k == W ? U(~U(0)) : U((U(1) << k) - 1);}
    static constexpr bool test(U x, int i) {
        assert(0 <= i && i < W); return (x >> i) & 1;}
    static constexpr U set(U x, int i, bool on = true) {
        assert(0 <= i && i < W);
        U bit = U(U(1) << i); return on ? U(x | bit) : U(x & U(~bit));}
    static constexpr U flip(U x, int i) {
        assert(0 <= i && i < W); return U(x ^ (U(1) << i));}
    static constexpr U clearLowBit(U x) { return U(x & U(x - 1)); }
    static constexpr U clearTrailingOnes(U x) { return U(x & U(x + 1)); }
    static constexpr U setLowestZero(U x) { return U(x | U(x + 1)); }
    static constexpr U mergeBits(U a, U b, U mask) { return U(a ^ U(U(a ^ b) & mask)); }
    static constexpr U shiftLeft(U x, int k) {
        assert(0 <= k && k <= W); return k == W ? U(0) : U(x << k);}
    static constexpr U shiftRight(U x, int k) {
        assert(0 <= k && k <= W); return k == W ? U(0) : U(x >> k);}
    static constexpr U signExtend(U x, int b) {
        assert(1 <= b && b <= W); U top = U(U(1) << (b - 1));
        return U(U(U(x & lowMask(b)) ^ top) - top);}
    static constexpr U swapBitRanges(U x, int i, int j, int n) {
        assert(0 <= n && 0 <= i && 0 <= j && i + n <= W && j + n <= W && (i + n <= j || j + n <= i));
        U t = U(U(shiftRight(x, i) ^ shiftRight(x, j)) & lowMask(n));
        return U(x ^ shiftLeft(t, i) ^ shiftLeft(t, j));}
    static constexpr U bitReverse(U x) {
        for (int k = 0; k < bit_operations_detail::LOG_W<U>; ++k) {
            U m = bit_operations_detail::BLOCK_MASK<U>[k]; int s = 1 << k;
            x = U(U(U(x >> s) & m) | U(U(x & m) << s));}
        return x;}
    static constexpr U grayCode(U x) { return U(x ^ (x >> 1)); }
    static constexpr U grayDecode(U x) {
        for (int k = 1; k < W; k *= 2) { x = U(x ^ (x >> k)); }
        return x;}
    static constexpr U mortonEncode(U x, U y) {
        assert(((x | y) >> (W / 2)) == 0);
        return U(bit_operations_detail::spread(x) | U(bit_operations_detail::spread(y) << 1));}
    static constexpr pair<U, U> mortonDecode(U z) {
        return {bit_operations_detail::pack(z), bit_operations_detail::pack(U(z >> 1))};}

    static constexpr bool hasZeroByte(U x) { return U(U(x - BYTES) & U(~x) & HIGHS) != 0; }
    static constexpr bool hasByteEqual(U x, int n) {
        assert(0 <= n && n <= 255); return hasZeroByte(U(x ^ U(BYTES * U(n))));}
    static constexpr bool hasByteLess(U x, int n) {
        assert(0 <= n && n <= 128); return U(U(x - U(BYTES * U(n))) & U(~x) & HIGHS) != 0;}
    static constexpr bool hasByteGreater(U x, int n) {
        assert(0 <= n && n <= 127); return U(U(U(x + U(BYTES * U(127 - n))) | x) & HIGHS) != 0;}
    static constexpr bool hasByteBetween(U x, int m, int n) {
        assert(0 <= m && m <= 127 && 0 <= n && n <= 128);
        U low = U(x & U(BYTES * 127));
        return U(U(U(BYTES * U(127 + n)) - low) & U(~x) & U(low + U(BYTES * U(127 - m))) & HIGHS) != 0;}

    static constexpr bool prevSubmask(U &sub, U mask) {
        assert((sub & U(~mask)) == 0);
        if (!sub) { return false; }
        sub = U((sub - 1) & mask); return true;}
    static constexpr bool nextSubmask(U &sub, U mask) {
        assert((sub & U(~mask)) == 0);
        if (sub == mask) { return false; }
        sub = U(U(U(sub | U(~mask)) + 1) & mask); return true;}
    static constexpr bool nextSupermask(U &sup, U mask, U full = U(~U(0))) {
        assert((sup & mask) == mask && (sup & U(~full)) == 0);
        if (sup == full) { return false; }
        U rest = U(full & U(~mask));
        sup = U((U(U(sup | U(~rest)) + 1) & rest) | mask); return true;}
    static constexpr bool prevSupermask(U &sup, U mask, U full = U(~U(0))) {
        assert((sup & mask) == mask && (sup & U(~full)) == 0);
        if (sup == mask) { return false; }
        U rest = U(full & U(~mask));
        sup = U((U(U(sup & rest) - 1) & rest) | mask); return true;}
    // Carry moves the lowest run's top bit left, then packs the remaining ones low.
    static constexpr bool nextCombination(U &x) {
        if (!x) { return false; }
        U low = lowBit(x), next = U(x + low);
        if (!next) { return false; }
        x = U(next | (U((next ^ x) >> 2) >> std::countr_zero(low))); return true;}
    static constexpr bool prevCombination(U &x) {
        U y = U(~x);
        if (!nextCombination(y)) { return false; }
        x = U(~y); return true;}
    // Raise lo at its highest fixed bit that differs from b, else at the lowest free zero above it.
    static constexpr bool xorSubmaskLowerBound(U b, U s, U lo, U &out) {
        U fixed = U(~s), d = U(U(lo ^ b) & fixed);
        if (!d) { out = lo; return true; }
        int h = last(d);
        if (test(b, h)) { out = U(U(lo & U(~lowMask(h + 1))) | U(b & fixed & lowMask(h + 1))); return true; }
        U up = U(s & U(~lo) & U(~lowMask(h + 1)));
        if (!up) { return false; }
        int p = std::countr_zero(up);
        out = U(U(lo & U(~lowMask(p + 1))) | U(U(1) << p) | U(b & fixed & lowMask(p))); return true;}

    // T: O(w), M: O(1); f(base, k) for disjoint blocks [base, base + 2^k) covering {y ^ x : l <= y < r}, at most 2w calls.
    template<typename F>
    static constexpr void xorRangeBlocks(U l, U r, U x, F &&f) {
        assert(l <= r);
        for (int k = 0; l < r; ++k) {
            U bit = U(U(1) << k), keep = U(~lowMask(k));
            if (l & bit) { f(U((l ^ x) & keep), k); l = U(l + bit); }
            if (r & bit) { r = U(r - bit); f(U((r ^ x) & keep), k); }}}
};
