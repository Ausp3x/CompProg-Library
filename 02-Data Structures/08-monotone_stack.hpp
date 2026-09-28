#pragma once

#include "../01-Core/01-template.hpp"

// All sequence/matrix dimensions fit int. less is a pure strict weak ordering;
// "smaller" means preceding in that order, with comparator-equivalent ties.
// strict=false allows equal elements. Missing previous/next indices are -1/n.
// T: O(n), M: O(n), plus O(n) returned indices; O(1)-cost comparisons.
template<typename T, typename C = std::less<T>>
vector<int> previousSmaller(const vector<T> &a, bool strict = true, C less = {}) {
    assert(a.size() <= INT_MAX);
    int n = int(a.size()); vector<int> ans(n, -1), st; st.reserve(n);
    for (int i = 0; i < n; ++i) {
        while (!st.empty() && (strict ? !less(a[st.back()], a[i]) : less(a[i], a[st.back()]))) { st.pop_back(); }
        if (!st.empty()) { ans[i] = st.back(); }
        st.push_back(i);}
    return ans;}

// T: O(n), M: O(n), plus O(n) returned indices.
template<typename T, typename C = std::less<T>>
vector<int> nextSmaller(const vector<T> &a, bool strict = true, C less = {}) {
    assert(a.size() <= INT_MAX);
    int n = int(a.size()); vector<int> ans(n, n), st; st.reserve(n);
    for (int i = n - 1; i >= 0; --i) {
        while (!st.empty() && (strict ? !less(a[st.back()], a[i]) : less(a[i], a[st.back()]))) { st.pop_back(); }
        if (!st.empty()) { ans[i] = st.back(); }
        st.push_back(i);}
    return ans;}

// T: O(n), M: O(n), plus O(n) returned indices; natural greater-order adapters.
template<typename T>
vector<int> previousGreater(const vector<T> &a, bool strict = true) {
    return previousSmaller(a, strict, std::greater<T>{});}
template<typename T>
vector<int> nextGreater(const vector<T> &a, bool strict = true) {
    return nextSmaller(a, strict, std::greater<T>{});}

// Indices of extrema in windows [i, i + k), k > 0; k > n returns no windows.
// Equivalent minima use the leftmost index unless rightmost=true.
// T: O(n), M: O(min(n, k)), plus O(max(0, n - k + 1)) returned indices.
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

// T: O(n), M: O(min(n, k)), plus O(max(0, n - k + 1)) returned indices.
template<typename T>
vector<int> slidingMaximum(const vector<T> &a, int k, bool rightmost = false) {
    return slidingMinimum(a, k, rightmost, std::greater<T>{});}

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
    // Emit O(n) sufficient positive-height candidates, reusing the stack.
    template<typename F>
    void histogram(const vector<lng> &a, vector<int> &st, F emit) {
        int n = int(a.size()); st.clear();
        for (int i = 0;; ++i) {
            lng h = i < n ? a[i] : 0;
            assert(h >= 0);
            while (!st.empty() && a[st.back()] >= h) {
                lng height = a[st.back()]; st.pop_back();
                if (height) { emit(st.empty() ? 0 : st.back() + 1, i, height); }}
            if (i == n) { break; }
            st.push_back(i);}}

    template<typename P>
    BinaryRectangle matrixRectangle(const vector<vector<int>> &a, P matches) {
        assert(a.size() <= INT_MAX);
        BinaryRectangle ans;
        if (a.empty()) { return ans; }
        assert(a[0].size() <= INT_MAX);
        int n = int(a.size()), m = int(a[0].size());
        vector<lng> h(m); vector<int> st; st.reserve(m);
        for (int i = 0; i < n; ++i) {
            assert(a[i].size() == a[0].size());
            for (int j = 0; j < m; ++j) { h[j] = matches(a[i][j]) ? h[j] + 1 : 0; }
            histogram(h, st, [&](int l, int r, lng height) {
                int bottom = i + 1, top = bottom - int(height);
                lng area = height * (r - l);
                if (area > ans.area || (area == ans.area && std::tie(top, l, bottom, r) < std::tie(ans.top, ans.left, ans.bottom, ans.right))) {
                    ans = {area, top, l, bottom, r}; }
            });}
        return ans;}
} // namespace monotone_detail

// Nonnegative lng heights, unit-width bars; lll area supports the full domain.
// Positive witness is [l, r) x [0, height), ties use smallest (l, r).
// No positive rectangle: area=height=0 and l=r=-1 (also for an empty histogram).
// T: O(n), M: O(n).
inline HistogramRectangle largestHistogramRectangle(const vector<lng> &a) {
    assert(a.size() <= INT_MAX);
    HistogramRectangle ans; vector<int> st; st.reserve(a.size());
    monotone_detail::histogram(a, st, [&](int l, int r, lng height) {
        lll area = lll(height) * (r - l);
        if (area > ans.area || (area == ans.area && std::tie(l, r) < std::tie(ans.l, ans.r))) { ans = {area, l, r, height}; }
    });
    return ans;}

// Rectangular binary matrix, value is 0 or 1. Empty dimensions are allowed.
// Witness [top, bottom) x [left, right); ties use smallest (top,left,bottom,right).
// No positive rectangle: area=0 and all coordinates=-1. int dimensions fit lng area.
// T: O(n * m + n), M: O(m); the +n checks row shapes even when m=0.
inline BinaryRectangle largestBinaryRectangle(const vector<vector<int>> &a, int value = 1) {
    assert(value == 0 || value == 1);
    return monotone_detail::matrixRectangle(a, [value](int x) { assert(x == 0 || x == 1); return x == value; });}

// Legacy area-only adapter: any nonzero int is a blocker, not just binary 1.
// T: O(n * m + n), M: O(m). Returned lng widens the old int area safely.
inline lng maxZeroSubmatrix(const vector<vector<int>> &a) {
    return monotone_detail::matrixRectangle(a, [](int x) { return x == 0; }).area;}
