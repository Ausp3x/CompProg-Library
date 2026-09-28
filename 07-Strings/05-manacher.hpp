#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n), Q: O(1), M: O(n); random-access/equality sequence, n <= INT_MAX.
// Sentinel-free: bytes (including NUL), integer alphabets and empty input work.
// odd[i] includes the center: [i-odd[i]+1, i+odd[i]); n entries.
// even[i] is the radius at gap i: [i-even[i], i+even[i]); n+1 entries.
// Interval helpers accept -1 for maximum, or any valid smaller radius.
// Inclusive empty even intervals are [i,i-1]. Fields are read-only after building.
// Assign a moved-from object before invoking radius queries.
struct Manacher {
    vector<int> odd, even;

    Manacher() : even(1) {}
    template<class S> explicit Manacher(const S &s) {
        assert(s.size() <= INT_MAX); int n = int(s.size());
        odd.resize(n); even.resize(size_t(n) + 1);
        for (int i = 0, l = 0, r = -1; i < n; ++i) {
            int k = i > r ? 1 : min(odd[l + (r - i)], r - i + 1);
            while (k <= i && k < n - i && s[i - k] == s[i + k]) { ++k; }
            odd[i] = k;
            if (k - 1 > r - i) { l = i - k + 1; r = i + k - 1; } }
        for (int i = 0, l = 0, r = -1; i < n; ++i) {
            int k = i > r ? 0 : min(even[l + (r - i) + 1], r - i + 1);
            while (k < i && k < n - i && s[i - k - 1] == s[i + k]) { ++k; }
            even[i] = k;
            if (k - 1 > r - i) { l = i - k; r = i + k - 1; } }
    }
    int size() const { return int(odd.size()); }

    pair<int, int> oddInterval(int i, int k = -1) const {
        assert(0 <= i && i < size()); if (k == -1) { k = odd[i]; }
        assert(1 <= k && k <= odd[i]); return {i - k + 1, i + k};
    }
    pair<int, int> evenInterval(int i, int k = -1) const {
        assert(0 <= i && i <= size()); if (k == -1) { k = even[i]; }
        assert(0 <= k && k <= even[i]); return {i - k, i + k};
    }
    pair<int, int> oddInclusive(int i, int k = -1) const {
        auto [l, r] = oddInterval(i, k); return {l, r - 1};
    }
    pair<int, int> evenInclusive(int i, int k = -1) const {
        auto [l, r] = evenInterval(i, k); return {l, r - 1};
    }
};
