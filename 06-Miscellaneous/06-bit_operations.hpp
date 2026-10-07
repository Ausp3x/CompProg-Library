#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1) (grayDecode O(log(w))), M: O(1); constexpr, unsigned words through 128 bits, positions [0, W), counts [0, W].
// Zero: width/count/floor/lowBit 0, zeros W, first/last -1; failed ceil and stepping leave the output unchanged.
template<std::unsigned_integral U = ulng>
requires (std::is_same_v<U, std::remove_cv_t<U>> && requires(U x) { std::popcount(x); })
struct BitOps {
    static constexpr int W = std::numeric_limits<U>::digits;
    static_assert(W <= 128);

    static constexpr int count(U x) { return std::popcount(x); }
    static constexpr int width(U x) { return std::bit_width(x); }
    static constexpr int leadingZeros(U x) { return std::countl_zero(x); }
    static constexpr int trailingZeros(U x) { return std::countr_zero(x); }
    static constexpr int first(U x) { return x ? trailingZeros(x) : -1; }
    static constexpr int last(U x) { return width(x) - 1; }
    static constexpr bool single(U x) { return std::has_single_bit(x); }
    static constexpr bool parity(U x) { return std::popcount(x) & 1; }
    static constexpr U lowBit(U x) { return U(x & U(U(0) - x)); }
    static constexpr U floor(U x) { return std::bit_floor(x); }
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
    static constexpr U shiftLeft(U x, int k) {
        assert(0 <= k && k <= W); return k == W ? U(0) : U(x << k);}
    static constexpr U shiftRight(U x, int k) {
        assert(0 <= k && k <= W); return k == W ? U(0) : U(x >> k);}
    static constexpr U rotateLeft(U x, int k) { return std::rotl(x, k); }
    static constexpr U rotateRight(U x, int k) { return std::rotr(x, k); }
    static constexpr U grayCode(U x) { return U(x ^ (x >> 1)); }
    static constexpr U grayDecode(U x) {
        for (int k = 1; k < W; k *= 2) { x = U(x ^ (x >> k)); }
        return x;}

    static constexpr bool prevSubmask(U &sub, U mask) {
        assert((sub & U(~mask)) == 0);
        if (!sub) { return false; }
        sub = U((sub - 1) & mask); return true;}
    static constexpr bool nextSupermask(U &sup, U mask, U full = U(~U(0))) {
        assert((mask & U(~full)) == 0 && (sup & mask) == mask && (sup & U(~full)) == 0);
        if (sup == full) { return false; }
        U rest = U(full & U(~mask));
        sup = U((U(U(sup | U(~rest)) + 1) & rest) | mask); return true;}
    // Carry moves the lowest run's top bit left, then packs the remaining ones low.
    static constexpr bool nextCombination(U &x) {
        if (!x) { return false; }
        U low = lowBit(x), next = U(x + low);
        if (!next) { return false; }
        x = U(next | (U((next ^ x) >> 2) >> trailingZeros(low))); return true;}
};
