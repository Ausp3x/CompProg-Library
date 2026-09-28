#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), U: O(1), Q: expected O(1), M: O(1) (312-word MT state).
// Inclusive bounds; randBelow(n) is [0,n). Explicit seeds replay across libraries.
// Default seed uses clock ticks only: no entropy/uniqueness/security guarantee.
// Rejection is unbiased for uniform independent words; MT is not cryptographic.
struct Random {
    std::mt19937_64 rng;
    Random() : Random(ulng(std::chrono::steady_clock::now().time_since_epoch().count())) {}
    explicit Random(ulng seed) : rng(seed) {}
    void seed(ulng x) { rng.seed(x); }

    // Lemire multiply/reject; expected <2 words, no finite worst-case bound.
    ulng randBelow(ulng n) {
        assert(n > 0);
        ulll p = ulll(rng()) * n;
        if (ulng(p) < n) {
            ulng t = -n % n;
            while (ulng(p) < t) { p = ulll(rng()) * n; }}
        return ulng(p >> 64);}
    ulng randUlng(ulng l, ulng r) {
        assert(l <= r);
        ulng n = r - l + 1;
        return l + (n ? randBelow(n) : rng());}
    lng randLng(lng l, lng r) {
        assert(l <= r);
        ulng n = ulng(r) - ulng(l) + 1;
        return lng(lll(l) + (n ? randBelow(n) : rng()));}
    int randInt(int l, int r) { return int(randLng(l, r)); }

    // T: expected O(n), M: O(1); valid mutable random-access [first,last).
    template<std::random_access_iterator It>
    void shuffle(It first, It last) {
        auto n = last - first;
        assert(n >= 0);
        while (n > 1) {
            auto j = std::iter_difference_t<It>(randBelow(ulng(n)));
            std::iter_swap(first + --n, first + j); }}
};
inline Random rng;
