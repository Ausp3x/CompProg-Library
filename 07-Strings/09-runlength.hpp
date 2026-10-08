#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n), M: O(r) returned; r = maximal runs, n <= INT_MAX, max_count >= 0; false (a run above max_count) clears runs.
template<class S> requires std::integral<typename S::value_type>
bool runLengthEncode(const S &s, vector<pair<typename S::value_type, lng>> &runs,
                     lng max_count = std::numeric_limits<lng>::max()) {
    assert(s.size() <= INT_MAX && max_count >= 0);
    runs.clear();
    for (typename S::value_type x : s) {
        if (runs.empty() || runs.back().first != x) {
            if (!max_count) { runs.clear(); return false; }
            runs.emplace_back(x, 1);}
        else {
            if (runs.back().second == max_count) { runs.clear(); return false; }
            ++runs.back().second;}}
    return true;}

// T: O(r), M: O(1) for size, O(r) returned for spans; r = runs.size() <= INT_MAX; false (count <= 0 or total > max_size) zeroes size, clears spans.
template<std::integral T>
bool runLengthSize(const vector<pair<T, lng>> &runs, lng &size, lng max_size = std::numeric_limits<lng>::max()) {
    assert(runs.size() <= INT_MAX && max_size >= 0);
    lng n = 0;
    for (const auto &[x, count] : runs) {
        if (count <= 0 || count > max_size - n) { size = 0; return false; }
        n += count;}
    size = n;
    return true;}
template<std::integral T>
bool runLengthSpans(const vector<pair<T, lng>> &runs, vector<pair<lng, lng>> &spans,
                    lng max_size = std::numeric_limits<lng>::max()) {
    lng n;
    if (!runLengthSize(runs, n, max_size)) { spans.clear(); return false; }
    vector<pair<lng, lng>> res;
    res.reserve(runs.size());
    n = 0;
    for (const auto &[x, count] : runs) {
        res.emplace_back(n, n + count);
        n += count;}
    spans = std::move(res);
    return true;}

// T: O(r + n), M: O(n) returned; r = runs.size(), n = decoded length <= max_size <= INT_MAX; false clears s.
template<class S> requires std::integral<typename S::value_type>
bool runLengthDecode(const vector<pair<typename S::value_type, lng>> &runs, S &s, int max_size = INT_MAX) {
    assert(max_size >= 0);
    s.clear();
    lng n;
    if (!runLengthSize(runs, n, max_size) || ulng(n) > s.max_size()) { return false; }
    s.reserve(int(n));
    for (const auto &[x, count] : runs) { s.insert(s.end(), int(count), x); }
    return true;}
