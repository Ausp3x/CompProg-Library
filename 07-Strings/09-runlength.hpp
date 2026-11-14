#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n), M: O(r) returned; r = maximal runs, n <= INT_MAX, max_count >= 0; false (a run above max_count) clears runs.
template<class S> requires std::equality_comparable<typename S::value_type> && std::copy_constructible<typename S::value_type>
bool runLengthEncode(const S &s, vector<pair<typename S::value_type, lng>> &runs,
                     lng max_count = std::numeric_limits<lng>::max()) {
    assert(s.size() <= INT_MAX && max_count >= 0);
    runs.clear();
    for (const auto &x : s) {
        if (runs.empty() || runs.back().first != x) {
            if (!max_count) { runs.clear(); return false; }
            runs.emplace_back(x, 1);}
        else {
            if (runs.back().second == max_count) { runs.clear(); return false; }
            ++runs.back().second;}}
    return true;}

// T: O(r), M: O(1); r = runs.size() <= INT_MAX; false (count <= 0 or total > max_size) sets size = 0.
template<class T>
bool runLengthSize(const vector<pair<T, lng>> &runs, lng &size, lng max_size = std::numeric_limits<lng>::max()) {
    assert(runs.size() <= INT_MAX && max_size >= 0);
    lng n = 0;
    for (const auto &[x, count] : runs) {
        if (count <= 0 || count > max_size - n) { size = 0; return false; }
        n += count;}
    size = n;
    return true;}
// T: O(r), M: O(r) returned; validates as runLengthSize; false clears spans.
template<class T>
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
template<class S>
bool runLengthDecode(const vector<pair<typename S::value_type, lng>> &runs, S &s, int max_size = INT_MAX) {
    s.clear();
    lng n;
    if (!runLengthSize(runs, n, max_size) || ulng(n) > s.max_size()) { return false; }
    s.reserve(int(n));
    for (const auto &[x, count] : runs) { s.insert(s.end(), int(count), x); }
    return true;}

// S: O(1), O(n) from a sequence, U: O(1) amortized, Q: O(1), M: O(r); r = most runs held so far; ctor needs n <= INT_MAX, pushBack 0 <= count <= INT64_MAX - length(), popBack 0 <= count <= length().
template<class T> requires std::equality_comparable<T> && std::copy_constructible<T> struct RunLengthEncoder {
    vector<pair<T, lng>> runs;
    lng n = 0;

    RunLengthEncoder() = default;
    template<class S> requires std::same_as<typename S::value_type, T>
    explicit RunLengthEncoder(const S &s) : n(lng(s.size())) { runLengthEncode(s, runs); }

    lng length() const { return n; }

    void pushBack(const T &x, lng count = 1) {
        assert(0 <= count && count <= std::numeric_limits<lng>::max() - n);
        if (!count) { return; }
        if (runs.empty() || runs.back().first != x) { runs.emplace_back(x, count); }
        else { runs.back().second += count; }
        n += count;}
    void popBack(lng count = 1) {
        assert(0 <= count && count <= n);
        n -= count;
        while (count > 0 && !runs.empty()) {
            lng k = min(count, runs.back().second);
            count -= k;
            if (!(runs.back().second -= k)) { runs.pop_back(); }}}
};
template<class S> RunLengthEncoder(const S &) -> RunLengthEncoder<typename S::value_type>;
