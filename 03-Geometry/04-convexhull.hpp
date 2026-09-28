#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// Exact integral hulls, n <= INT_MAX; orientation/squared-distance intermediates
// must fit lll (|coordinate| <= 10^9 suffices). Input is copied, duplicates removed.
// Output starts at the lexicographic minimum, proceeds CCW, never repeats its
// start; keep_collinear retains every distinct boundary input point. A collinear
// set returns sorted endpoints, or all sorted points when retention is enabled.
namespace hull_detail {
    template<typename T> bool prepare(vector<Point2<T>> &p, bool keep_collinear) {
        static_assert(std::is_integral_v<T>); assert(p.size() <= INT_MAX);
        sort(p.begin(), p.end()); p.erase(unique(p.begin(), p.end()), p.end());
        if (p.size() <= 2) { return true; }
        for (const auto &q : p) { if (orient(p.front(), p.back(), q)) { return false; } }
        if (!keep_collinear) { p = {p.front(), p.back()}; } return true; }
} // namespace hull_detail

// T: O(n * log(n)), M: O(n), including returned hull. Andrew monotone chain.
template<typename T> vector<Point2<T>> convexHull(vector<Point2<T>> p, bool keep_collinear = false) {
    if (hull_detail::prepare(p, keep_collinear)) { return p; }
    vector<Point2<T>> h; h.reserve(p.size() + 1);
    int n = int(p.size());
    for (int pass = 0; pass < 2; ++pass) {
        int base = int(h.size());
        for (int i = 0; i < n; ++i) {
            const auto &q = p[pass ? n - 1 - i : i];
            while (h.size() - base >= 2) {
                int s = orient(h[h.size() - 2], h.back(), q);
                if (s > 0 || (keep_collinear && s == 0)) { break; } h.pop_back(); }
            h.push_back(q); }
        h.pop_back(); }
    return h; }

// T: O(n * log(n)), M: O(n), including returned hull. Graham angular scan;
// integral-only angle comparison avoids non-transitive floating predicates.
template<typename T> vector<Point2<T>> convexHullGraham(vector<Point2<T>> p, bool keep_collinear = false) {
    if (hull_detail::prepare(p, keep_collinear)) { return p; }
    auto o = p.front();
    sort(p.begin() + 1, p.end(), [o](auto a, auto b) {
        int s = orient(o, a, b); return s ? s > 0 : dist2(o, a) < dist2(o, b); });
    if (keep_collinear) {
        int i = int(p.size()) - 1;
        while (i > 1 && orient(o, p[i - 1], p.back()) == 0) { --i; }
        reverse(p.begin() + i, p.end()); }
    vector<Point2<T>> h; h.reserve(p.size());
    for (const auto &q : p) {
        while (h.size() >= 2) {
            int s = orient(h[h.size() - 2], h.back(), q);
            if (s > 0 || (keep_collinear && s == 0)) { break; } h.pop_back(); }
        h.push_back(q); }
    return h; }
