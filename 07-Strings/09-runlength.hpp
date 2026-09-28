#pragma once
#include "../01-Core/01-template.hpp"

// T: O(n), M: O(r) returned; byte/integer sequence, n <= INT_MAX, r maximal runs.
// Positive lng counts; max_count>=0. False on exceeding the cap, clearing output.
// Empty input succeeds even with cap zero; source symbols are preserved exactly.
template<class S> requires std::integral<typename S::value_type>
bool runLengthEncode(const S &s, vector<pair<typename S::value_type, lng>> &runs,
                     lng max_count = std::numeric_limits<lng>::max()) {
    assert(s.size() <= INT_MAX && max_count >= 0); runs.clear();
    for (typename S::value_type x : s) {
        if (runs.empty() || runs.back().first != x) {
            if (!max_count) { runs.clear(); return false; }
            runs.emplace_back(x, 1); }
        else {
            if (runs.back().second == max_count) { runs.clear(); return false; }
            ++runs.back().second; } }
    return true;
}

// T: O(r), M: O(1); r <= INT_MAX, max_size>=0, adjacent equal runs are allowed.
// Rejects nonpositive counts or a total beyond max_size without overflowing lng.
// False sets size=0; empty succeeds with size=0. Output may alias an input count.
template<std::integral T>
bool runLengthSize(const vector<pair<T, lng>> &runs, lng &size,
                   lng max_size = std::numeric_limits<lng>::max()) {
    assert(runs.size() <= INT_MAX && max_size >= 0); lng n = 0;
    for (const auto &[x, count] : runs) {
        if (count <= 0 || count > max_size - n) { size = 0; return false; }
        n += count; }
    size = n; return true;
}

// T: O(r + n), M: O(n) returned; string/vector output, max_size>=0, n<=INT_MAX.
// Preflights all counts and size/capacity before allocation; false clears output.
// Accepts nonmaximal runs. Resource exhaustion retains standard allocation errors.
template<class S> requires std::integral<typename S::value_type>
bool runLengthDecode(const vector<pair<typename S::value_type, lng>> &runs, S &s,
                     int max_size = INT_MAX) {
    assert(max_size >= 0); s.clear(); lng n;
    if (!runLengthSize(runs, n, max_size) || ulng(n) > s.max_size()) { return false; }
    s.reserve(int(n));
    for (const auto &[x, count] : runs) { s.insert(s.end(), int(count), x); }
    return true;
}

// T: O(r), M: O(r) returned; half-open decoded witness spans in input run order.
// Same validation/cap contract as runLengthSize; no decoded symbols are allocated.
// Output may alias input when T=lng. False clears output, including aliased input.
template<std::integral T>
bool runLengthSpans(const vector<pair<T, lng>> &runs, vector<pair<lng, lng>> &spans,
                    lng max_size = std::numeric_limits<lng>::max()) {
    lng n;
    if (!runLengthSize(runs, n, max_size)) { spans.clear(); return false; }
    vector<pair<lng, lng>> out; out.reserve(runs.size()); n = 0;
    for (const auto &[x, count] : runs) { out.emplace_back(n, n + count); n += count; }
    spans = std::move(out); return true;
}
