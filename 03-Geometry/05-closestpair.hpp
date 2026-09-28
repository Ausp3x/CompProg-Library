#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// ids are original zero-based indices, increasing and lexicographically smallest
// among all minimum-distance pairs. Fewer than two points: {-1, -1}, distance2=0.
struct ClosestPair {
    pair<int, int> ids{-1, -1};
    lll distance2 = 0;
};

// T: O(n * log(n)), M: O(n). Input is preserved; deterministic exact integers.
// Signed integral coordinates of at most 64 bits, n <= INT_MAX. The bounding-box
// diagonal squared must fit lll (asserted without overflowing). Thus all pair
// distances fit; arbitrary translated clusters and extreme singletons are valid.
// Initial (x,y,id) order equals a stable (x,y) sort; y merges preserve stable ties.
template<typename T> ClosestPair closestPair(const vector<Point2<T>> &p) {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T> && sizeof(T) <= sizeof(lng));
    assert(p.size() <= std::numeric_limits<int>::max());
    int n = int(p.size());
    if (n < 2) { return {}; }
    T x0 = p[0].x, x1 = x0, y0 = p[0].y, y1 = y0;
    for (auto q : p) { x0 = min(x0, q.x); x1 = max(x1, q.x); y0 = min(y0, q.y); y1 = max(y1, q.y); }
    ulll dx = ulll(lll(x1) - x0), dy = ulll(lll(y1) - y0);
    constexpr ulll LIMIT = (ulll(1) << 127) - 1;
    assert(dx * dx <= LIMIT && dy * dy <= LIMIT - dx * dx);

    vector<int> ids(n); iota(ids.begin(), ids.end(), 0);
    sort(ids.begin(), ids.end(), [&](int i, int j) { return p[i] != p[j] ? p[i] < p[j] : i < j; });
    ClosestPair ans{{0, 1}, dist2(p[0], p[1])};
    auto update = [&](int i, int j) {
        pair<int, int> ij = std::minmax(i, j); lll d = dist2(p[i], p[j]);
        if (d < ans.distance2 || (d == ans.distance2 && ij < ans.ids)) { ans = {ij, d}; }
    };
    // Handle all duplicate groups before recursion: zero-width strips otherwise
    // lose their positive-separation packing bound when including distance ties.
    // On an axis-parallel line, a closest pair is adjacent in this sorted order.
    bool line = x0 == x1 || y0 == y1;
    for (int i = 1; i < n; ++i) { if (line || p[ids[i - 1]] == p[ids[i]]) { update(ids[i - 1], ids[i]); } }
    if (!ans.distance2 || line) { return ans; }
    vector<int> tmp(n);
    auto by_y = [&](int i, int j) { return p[i].y != p[j].y ? p[i].y < p[j].y : p[i] < p[j]; };
    auto square = [](lll x) { return x * x; };
    auto solve = [&](auto &&solve, int l, int r) -> void {
        if (r - l <= 3) {
            for (int i = l; i < r; ++i) { for (int j = i + 1; j < r; ++j) { update(ids[i], ids[j]); } }
            sort(ids.begin() + l, ids.begin() + r, by_y); return; }
        int m = std::midpoint(l, r); T x = p[ids[m]].x;
        solve(solve, l, m); solve(solve, m, r);
        std::merge(ids.begin() + l, ids.begin() + m, ids.begin() + m, ids.begin() + r, tmp.begin() + l, by_y);
        std::copy(tmp.begin() + l, tmp.begin() + r, ids.begin() + l);
        int k = l;
        for (int i = l; i < r; ++i) {
            int a = ids[i];
            if (square(lll(p[a].x) - x) > ans.distance2) { continue; }
            for (int j = k - 1; j >= l && square(lll(p[a].y) - p[tmp[j]].y) <= ans.distance2; --j) { update(a, tmp[j]); }
            tmp[k++] = a; }
    };
    solve(solve, 0, n);
    return ans; }
