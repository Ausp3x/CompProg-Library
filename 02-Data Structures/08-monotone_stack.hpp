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
    // T: O(n), M: O(1); emits every maximal positive-height span, reusing st.
    template<typename F>
    void histogram(const vector<lng> &a, vector<int> &st, F emit) {
        int n = int(a.size()); st.clear();
        for (int i = 0;; ++i) {
            lng h = i < n ? a[i] : 0;
            while (!st.empty() && a[st.back()] >= h) {
                lng height = a[st.back()]; st.pop_back();
                if (height) { emit(st.empty() ? 0 : st.back() + 1, i, height); }}
            if (i == n) { break; }
            st.push_back(i);}}

    // T: O(n * m + n), M: O(m)
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
            auto relax = [&](int l, int r, lng height) {
                int bottom = i + 1, top = bottom - int(height);
                lng area = height * (r - l);
                if (area > ans.area || (area == ans.area && std::tie(top, l, bottom, r) < std::tie(ans.top, ans.left, ans.bottom, ans.right))) {
                    ans = {area, top, l, bottom, r};}};
            histogram(h, st, relax);}
        return ans;}
} // namespace monotone_detail

// T: O(n), M: O(n)
inline HistogramRectangle largestHistogramRectangle(const vector<lng> &a) {
    assert(a.size() <= INT_MAX && std::ranges::all_of(a, [](lng h) { return h >= 0; }));
    HistogramRectangle ans; vector<int> st; st.reserve(a.size());
    auto relax = [&](int l, int r, lng height) {
        lll area = lll(height) * (r - l);
        if (area > ans.area || (area == ans.area && std::tie(l, r) < std::tie(ans.l, ans.r))) { ans = {area, l, r, height}; }};
    monotone_detail::histogram(a, st, relax);
    return ans;}

// T: O(n * m + n), M: O(m)
inline BinaryRectangle largestBinaryRectangle(const vector<vector<int>> &a, int value = 1) {
    assert(value == 0 || value == 1);
    assert(std::ranges::all_of(a, [](const vector<int> &row) { return std::ranges::all_of(row, [](int x) { return x == 0 || x == 1; }); }));
    return monotone_detail::matrixRectangle(a, [value](int x) { return x == value; });}
inline lng maxZeroSubmatrix(const vector<vector<int>> &a) {
    return monotone_detail::matrixRectangle(a, [](int x) { return x == 0; }).area;}
