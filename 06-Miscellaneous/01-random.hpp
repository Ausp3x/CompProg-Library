#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), U: O(1), Q: expected O(1), M: O(1) (312-word MT state); MT19937-64 URBG, not cryptographic.
// Inclusive integer bounds, randBelow(n) in [0, n), randDouble in [0, 1) or finite [l, r); default seed is clock ticks.
struct Random {
    using result_type = ulng;
    std::mt19937_64 rng;
    Random() : Random(ulng(std::chrono::steady_clock::now().time_since_epoch().count())) {}
    explicit Random(ulng seed) : rng(seed) {}
    void seed(ulng x) { rng.seed(x); }
    static constexpr ulng min() { return 0; }
    static constexpr ulng max() { return ~0ULL; }
    ulng operator()() { return rng(); }

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
    double randDouble() { return double(rng() >> 11) * 0x1p-53; }
    double randDouble(double l, double r) {
        assert(l < r && std::isfinite(r - l));
        double x;
        do { x = l + (r - l) * randDouble(); } while (x >= r);
        return x;}

    // T: expected O(n), M: O(1); valid mutable random-access [first, last).
    template<std::random_access_iterator It>
    void shuffle(It first, It last) {
        auto n = last - first;
        assert(n >= 0);
        while (n > 1) {
            auto j = std::iter_difference_t<It>(randBelow(ulng(n)));
            std::iter_swap(first + --n, first + j);}}
};
inline Random rng;
