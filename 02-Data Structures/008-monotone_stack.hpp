#pragma once

#include "../01-Core/01-template.hpp"

// T: O(n), M: O(n)
template<typename T, typename C = std::less<T>>
vector<int> previousSmaller(const vector<T> &a, bool strict = true, C less = {}) {
    assert(a.size() <= INT_MAX);
    int n = int(a.size()); vector<int> ans(n, -1), st; st.reserve(n);
    for (int i = 0; i < n; ++i) {
        while (!st.empty() && (strict ? !less(a[st.back()], a[i]) : less(a[i], a[st.back()]))) { st.pop_back(); }
        if (!st.empty()) { ans[i] = st.back(); }
        st.push_back(i);}
    return ans;}
template<typename T, typename C = std::less<T>>
vector<int> nextSmaller(const vector<T> &a, bool strict = true, C less = {}) {
    assert(a.size() <= INT_MAX);
    int n = int(a.size()); vector<int> ans(n, n), st; st.reserve(n);
    for (int i = n - 1; i >= 0; --i) {
        while (!st.empty() && (strict ? !less(a[st.back()], a[i]) : less(a[i], a[st.back()]))) { st.pop_back(); }
        if (!st.empty()) { ans[i] = st.back(); }
        st.push_back(i);}
    return ans;}

// T: O(n), M: O(n)
template<typename T>
vector<int> previousGreater(const vector<T> &a, bool strict = true) {
    return previousSmaller(a, strict, std::greater<T>{});}
template<typename T>
vector<int> nextGreater(const vector<T> &a, bool strict = true) {
    return nextSmaller(a, strict, std::greater<T>{});}
template<typename T>
lng nextGreaterDistanceSum(const vector<T> &a, bool strict = true) {
    vector<int> nxt = nextGreater(a, strict); lng res = 0;
    for (int i = 0; i < int(nxt.size()); ++i) { res += nxt[i] - i - 1; }
    return res;}

// T: O(n), M: O(n)
template<typename T, typename C = std::less<T>>
vector<int> slidingMinimum(const vector<T> &a, int k, bool rightmost = false, C less = {}) {
    assert(a.size() <= INT_MAX && k > 0);
    int n = int(a.size()); vector<int> ans;
    if (k > n) { return ans; }
    ans.reserve(n - k + 1); deque<int> q;
    for (int i = 0; i < n; ++i) {
        if (!q.empty() && q.front() <= i - k) { q.pop_front(); }
        while (!q.empty() && (rightmost ? !less(a[q.back()], a[i]) : less(a[i], a[q.back()]))) { q.pop_back(); }
        q.push_back(i);
        if (i >= k - 1) { ans.push_back(q.front()); }}
    return ans;}
template<typename T>
vector<int> slidingMaximum(const vector<T> &a, int k, bool rightmost = false) {
    return slidingMinimum(a, k, rightmost, std::greater<T>{});}

// T: NA, M: O(1); no positive rectangle gives area 0 and every coordinate -1.
struct HistogramRectangle {
    lll area = 0;
    int l = -1, r = -1;
    lng height = 0;
};
struct BinaryRectangle {
    lng area = 0;
    int top = -1, left = -1, bottom = -1, right = -1;
};

namespace monotone_detail {
    // T: O(n), M: O(1); for every popped positive bar emits a span with a[l, r) >= height and below = max(a[l - 1], a[r]), reusing st.
    template<typename F>
    void histogram(const vector<lng> &a, vector<int> &st, F emit) {
        int n = int(a.size()); st.clear();
        for (int i = 0;; ++i) {
            lng h = i < n ? a[i] : 0;
            while (!st.empty() && a[st.back()] >= h) {
                lng height = a[st.back()]; st.pop_back();
                if (height) { emit(st.empty() ? 0 : st.back() + 1, i, height, max(h, st.empty() ? 0 : a[st.back()])); }}
            if (i == n) { break; }
            st.push_back(i);}}

    // T: O(n * m + n), M: O(1); a rectangular matrix of 0/1 cells.
    inline bool isBinaryMatrix(const vector<vector<int>> &a) {
        return std::ranges::all_of(a, [&](const vector<int> &row) {
            return row.size() == a[0].size() && std::ranges::all_of(row, [](int x) { return x == 0 || x == 1; });});}

    // T: O(n * m + n), M: O(m); calls row(i, h) with the heights of the matching runs that end in row i.
    template<typename P, typename F>
    void columnHeights(const vector<vector<int>> &a, P matches, F row) {
        assert(a.size() <= INT_MAX);
        if (a.empty()) { return; }
        assert(a[0].size() <= INT_MAX);
        int n = int(a.size()), m = int(a[0].size());
        vector<lng> h(m);
        for (int i = 0; i < n; ++i) {
            assert(a[i].size() == a[0].size());
            for (int j = 0; j < m; ++j) { h[j] = matches(a[i][j]) ? h[j] + 1 : 0; }
            row(i, h);}}

    // T: O(n * m + n), M: O(m); calls cell(i, j, side) with the largest square of value cells whose bottom-right cell is (i, j).
    template<typename F>
    void squareSides(const vector<vector<int>> &a, int value, F cell) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        vector<int> dp(m + 1);
        for (int i = 0; i < n; ++i) {
            int diag = 0;
            for (int j = 0; j < m; ++j) {
                int up = dp[j + 1];
                dp[j + 1] = a[i][j] == value ? min({up, dp[j], diag}) + 1 : 0;
                diag = up; cell(i, j, dp[j + 1]);}}}

    // T: O(n), M: O(n); max over [l, r) of min h[l, r) * sum w[l, r) with h, w >= 0 and w[j] > 0 where h[j] > 0, ties taking the smallest (l, r).
    inline HistogramRectangle weightedRectangle(const vector<lng> &h, const vector<lng> &w) {
        int n = int(h.size());
        vector<lll> p(n + 1);
        for (int i = 0; i < n; ++i) { p[i + 1] = p[i] + w[i]; }
        vector<int> lo = previousSmaller(h), hi = nextSmaller(h);
        HistogramRectangle ans;
        for (int i = 0; i < n; ++i) {
            int l = lo[i] + 1, r = hi[i];
            lll v = h[i] * (p[r] - p[l]);
            if (v > 0 && (v > ans.area || (v == ans.area && std::tie(l, r) < std::tie(ans.l, ans.r)))) { ans = {v, l, r, h[i]}; }}
        return ans;}

    // T: O(n * m + n) plus largest, M: O(n * m); the better of largest(b, 0) and largest(b, 1) on b = a xor the checkerboard pattern.
    template<typename F>
    BinaryRectangle checkerboard(const vector<vector<int>> &a, F largest) {
        auto b = a;
        for (int i = 0; i < int(b.size()); ++i) {
            for (int j = 0; j < int(b[i].size()); ++j) { b[i][j] ^= (i + j) & 1; }}
        BinaryRectangle x = largest(b, 0), y = largest(b, 1);
        return y.area > x.area || (y.area == x.area && std::tie(y.top, y.left, y.bottom, y.right) < std::tie(x.top, x.left, x.bottom, x.right)) ? y : x;}

    // T: O(n * m + n), M: O(m)
    template<typename P>
    BinaryRectangle matrixRectangle(const vector<vector<int>> &a, P matches) {
        BinaryRectangle ans; vector<int> st;
        columnHeights(a, matches, [&](int i, const vector<lng> &h) {
            histogram(h, st, [&](int l, int r, lng height, lng) {
                int bottom = i + 1, top = bottom - int(height);
                lng area = height * (r - l);
                if (area > ans.area || (area == ans.area && std::tie(top, l, bottom, r) < std::tie(ans.top, ans.left, ans.bottom, ans.right))) {
                    ans = {area, top, l, bottom, r};}});});
        return ans;}
} // namespace monotone_detail

// T: O(n), M: O(n); maximalHistogramRectangles lists each maximal [l, r) x [0, height), height > 0, once, by r, then by decreasing height.
inline HistogramRectangle largestHistogramRectangle(const vector<lng> &a) {
    assert(a.size() <= INT_MAX && std::ranges::all_of(a, [](lng h) { return h >= 0; }));
    HistogramRectangle ans; vector<int> st; st.reserve(a.size());
    auto relax = [&](int l, int r, lng height, lng) {
        lll area = lll(height) * (r - l);
        if (area > ans.area || (area == ans.area && std::tie(l, r) < std::tie(ans.l, ans.r))) { ans = {area, l, r, height}; }};
    monotone_detail::histogram(a, st, relax);
    return ans;}
inline vector<HistogramRectangle> maximalHistogramRectangles(const vector<lng> &a) {
    assert(a.size() <= INT_MAX && std::ranges::all_of(a, [](lng h) { return h >= 0; }));
    vector<HistogramRectangle> res; vector<int> st; st.reserve(a.size());
    monotone_detail::histogram(a, st, [&](int l, int r, lng height, lng below) {
        if (height > below) { res.push_back({lll(height) * (r - l), l, r, height}); }});
    return res;}
// T: O(n), M: O(n) (2D: T: O(n^2 * m + n), M: O(m)); a[i] >= 0 with sum * max < 2^127 (2D: < 2^63); ties take the smallest (l, r) (2D: (top, left, bottom, right)).
inline HistogramRectangle maxMinTimesSum(const vector<lng> &a) {
    assert(a.size() <= INT_MAX && std::ranges::all_of(a, [](lng x) { return x >= 0; }));
    assert(a.empty() || lll(*std::ranges::max_element(a)) <= std::numeric_limits<lll>::max() / std::max<lll>(1, std::accumulate(a.begin(), a.end(), lll(0))));
    return monotone_detail::weightedRectangle(a, a);}
inline BinaryRectangle maxMinTimesSum2D(const vector<vector<int>> &a) {
    assert(a.size() <= INT_MAX && std::ranges::all_of(a, [&](const vector<int> &row) {
        return row.size() == a[0].size() && std::ranges::all_of(row, [](int x) { return x >= 0; });}));
    [[maybe_unused]] lll total = 0; [[maybe_unused]] int top_cell = 0;
    for (const auto &row : a) { for (int x : row) { total += x; top_cell = max(top_cell, x); }}
    assert(lll(top_cell) * total <= LLONG_MAX);
    int n = int(a.size()), m = n ? int(a[0].size()) : 0;
    BinaryRectangle ans; vector<lng> lo(m), sum(m);
    for (int top = 0; top < n; ++top) {
        fill(lo.begin(), lo.end(), LLONG_MAX); fill(sum.begin(), sum.end(), 0);
        for (int bottom = top + 1; bottom <= n; ++bottom) {
            for (int j = 0; j < m; ++j) { lo[j] = min(lo[j], lng(a[bottom - 1][j])); sum[j] += a[bottom - 1][j]; }
            HistogramRectangle best = monotone_detail::weightedRectangle(lo, sum);
            lng area = lng(best.area);
            if (area > ans.area || (area > 0 && area == ans.area && std::tie(top, best.l, bottom, best.r) < std::tie(ans.top, ans.left, ans.bottom, ans.right))) {
                ans = {area, top, best.l, bottom, best.r};}}}
    return ans;}

// T: O(n * m + n), M: O(m)
inline BinaryRectangle largestBinaryRectangle(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    return monotone_detail::matrixRectangle(a, [value](int x) { return x == value; });}
inline lng maxZeroSubmatrix(const vector<vector<int>> &a) {
    return monotone_detail::matrixRectangle(a, [](int x) { return x == 0; }).area;}
// T: O(n * m + n), M: O(m) plus the output; maximal rectangles of value cells once each, by bottom, right, decreasing height; res[h - 1][w - 1] counts h x w ones.
inline vector<BinaryRectangle> maximalBinaryRectangles(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    int n = int(a.size());
    vector<BinaryRectangle> res; vector<int> st, next;
    monotone_detail::columnHeights(a, [value](int x) { return x == value; }, [&](int i, const vector<lng> &h) {
        int m = int(h.size()); next.assign(m + 1, 0);
        for (int j = 0; i + 1 < n && j < m; ++j) { next[j + 1] = next[j] + (a[i + 1][j] == value); }
        monotone_detail::histogram(h, st, [&](int l, int r, lng height, lng below) {
            if (height > below && next[r] - next[l] < r - l) { res.push_back({height * (r - l), i + 1 - int(height), l, i + 1, r}); }});});
    return res;}
inline vector<vector<lng>> countBinaryRectangles(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    int n = int(a.size()), m = n ? int(a[0].size()) : 0;
    vector<vector<lng>> res(n, vector<lng>(m)); vector<int> st;
    monotone_detail::columnHeights(a, [value](int x) { return x == value; }, [&](int, const vector<lng> &h) {
        monotone_detail::histogram(h, st, [&](int l, int r, lng height, lng below) {
            if (height == below) { return; }
            ++res[below][r - l - 1];
            if (height < n) { --res[height][r - l - 1]; }});});
    for (int k = 1; k < n; ++k) {
        for (int w = 0; w < m; ++w) { res[k][w] += res[k - 1][w]; }}
    for (auto &row : res) {
        lng runs = 0, total = 0;
        for (int w = m - 1; w >= 0; --w) { runs += row[w]; total += runs; row[w] = total; }}
    return res;}

// T: O(n * m + n), M: O(m); square ties take the smallest (top, left); counts need n * m < 2^42 (squares) and (n + 1) * (m + 1) <= 2^32 (rectangles).
inline BinaryRectangle largestBinarySquare(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    BinaryRectangle ans;
    monotone_detail::squareSides(a, value, [&](int i, int j, int side) {
        if (lng(side) * side > ans.area) { ans = {lng(side) * side, i + 1 - side, j + 1 - side, i + 1, j + 1}; }});
    return ans;}
inline lng countBinarySquares(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    lng res = 0;
    monotone_detail::squareSides(a, value, [&](int, int, int side) { res += side; });
    return res;}
inline lng countBinaryRectanglesTotal(const vector<vector<int>> &a, int value = 1) {
    assert((value == 0 || value == 1) && monotone_detail::isBinaryMatrix(a));
    assert(a.empty() || (lll(a.size()) + 1) * (lll(a[0].size()) + 1) <= lll(1) << 32);
    lng res = 0; vector<int> st;
    monotone_detail::columnHeights(a, [value](int x) { return x == value; }, [&](int, const vector<lng> &h) {
        monotone_detail::histogram(h, st, [&](int l, int r, lng height, lng below) {
            lng w = r - l; res += w * (w + 1) / 2 * (height - below);});});
    return res;}
// T: O(n * m + n), M: O(n * m); checkerboards are regions whose side-adjacent cells differ; the area bound is that of countBinaryRectangles.
inline BinaryRectangle largestCheckerboardSquare(const vector<vector<int>> &a) {
    assert(monotone_detail::isBinaryMatrix(a));
    return monotone_detail::checkerboard(a, largestBinarySquare);}
inline BinaryRectangle largestCheckerboardRectangle(const vector<vector<int>> &a) {
    assert(monotone_detail::isBinaryMatrix(a));
    return monotone_detail::checkerboard(a, largestBinaryRectangle);}
inline lng countBinaryRectanglesAreaAtMost(const vector<vector<int>> &a, lng k, int value = 1) {
    assert(a.empty() || (lll(a.size()) + 1) * (lll(a[0].size()) + 1) <= lll(1) << 32);
    vector<vector<lng>> cnt = countBinaryRectangles(a, value);
    lng res = 0;
    for (int h = 1; h <= int(cnt.size()); ++h) {
        for (int w = 1; w <= int(cnt[h - 1].size()) && lng(h) * w <= k; ++w) { res += cnt[h - 1][w - 1]; }}
    return res;}
