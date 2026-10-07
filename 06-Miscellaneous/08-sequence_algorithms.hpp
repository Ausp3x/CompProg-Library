#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "../02-Data Structures/03-segmenttree.hpp"
#include "../02-Data Structures/05-prefix_sum.hpp"
#include "../02-Data Structures/08-monotone_stack.hpp"
#include "04-compression.hpp"

// T: NA, M: O(1) (WeightedSubsequence O(out)); [l, r) witnesses, exists() and found are false for an absent optimum.
struct SubarraySum { lll sum = 0; int l = -1, r = -1; bool exists() const { return l >= 0; } };
struct CircularSubarraySum { lll sum = 0; int start = -1, length = 0; bool exists() const { return start >= 0; } };
struct SubrectangleSum { lll sum = 0; int top = -1, left = -1, bottom = -1, right = -1; bool exists() const { return top >= 0; } };
struct WeightedSubsequence { lll sum = 0; bool found = false; vector<int> indices; };

namespace sequence_detail {
    // T: O(1) checks, O(n) maximum, O(out) restore, M: O(out); sizes below INT_MAX, sums and weights integral with at most 64 bits.
    inline int checkedSize(size_t n) { assert(n < INT_MAX); return int(n); }
    template<typename T>
    constexpr void checkInteger() { static_assert(std::is_integral_v<T> && sizeof(T) <= 8); }
    template<typename F>
    SubarraySum maximum(int n, F get, bool empty) {
        SubarraySum res;
        if (empty) { res = {0, 0, 0}; }
        lll sum = 0, low = 0; int pos = 0;
        for (int r = 1; r <= n; ++r) {
            sum += get(r - 1);
            if (!res.exists() || sum - low > res.sum) { res = {sum - low, pos, r}; }
            if (sum < low) { low = sum; pos = r; }}
        return res;}
    inline vector<int> restore(const vector<int> &p, int i) {
        vector<int> res;
        for (; i >= 0; i = p[i]) { res.push_back(i); }
        reverse(res.begin(), res.end()); return res;}
} // namespace sequence_detail

// T: O(n), M: O(1); nonempty optimum, absent for n = 0 unless allow_empty adds [0, 0); kadane returns 0 for n = 0 and asserts a lng fit.
template<typename T>
SubarraySum maximumSubarray(const vector<T> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>();
    return sequence_detail::maximum(sequence_detail::checkedSize(a.size()), [&](int i) -> lll { return a[i]; }, allow_empty);}
template<typename T>
lng kadane(const vector<T> &a) {
    lll res = maximumSubarray(a).sum;
    assert(std::numeric_limits<lng>::min() <= res && res <= std::numeric_limits<lng>::max());
    return lng(res);}

// T: O(n), M: O(min(n, hi - lo) + 1); 0 <= lo <= hi bounds the length, absent when lo > n.
template<typename T>
SubarraySum boundedMaximumSubarray(const vector<T> &a, int lo, int hi) {
    sequence_detail::checkInteger<T>(); assert(0 <= lo && lo <= hi);
    int n = sequence_detail::checkedSize(a.size()); SubarraySum res;
    if (lo > n) { return res; }
    lll sum = 0, prefix = 0; deque<pair<int, lll>> q;
    for (int r = 0; r <= n; ++r) {
        if (r) { sum += a[r - 1]; }
        if (r < lo) { continue; }
        int l = r - lo;
        if (l) { prefix += a[l - 1]; }
        while (!q.empty() && q.front().first < r - hi) { q.pop_front(); }
        while (!q.empty() && q.back().second > prefix) { q.pop_back(); }
        q.emplace_back(l, prefix); lll value = sum - q.front().second;
        if (!res.exists() || value > res.sum) { res = {value, q.front().first, r}; }}
    return res;}

// T: O(n), M: O(1); witness (start + i) % n for i < length, at most one turn; absent for n = 0 unless allow_empty.
template<typename T>
CircularSubarraySum circularMaximumSubarray(const vector<T> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    auto high = maximumSubarray(a, allow_empty);
    CircularSubarraySum res{high.sum, high.l, high.exists() ? high.r - high.l : 0};
    if (!n) { return res; }
    auto low = sequence_detail::maximum(n, [&](int i) -> lll { return -lll(a[i]); }, false);
    lll total = accumulate(a.begin(), a.end(), lll(0));
    if (low.r - low.l < n && total + low.sum > res.sum) { res = {total + low.sum, low.r % n, n - (low.r - low.l)}; }
    return res;}

// T: O(min(n, m)^2 * max(n, m) + n), M: O(max(n, m)); rectangular input, [top, bottom) x [left, right), absent for an empty dimension unless allow_empty.
template<typename T>
SubrectangleSum maximumSubrectangle(const vector<vector<T>> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    int m = n ? sequence_detail::checkedSize(a[0].size()) : 0;
    for (const auto &row : a) { assert(row.size() == size_t(m)); }
    SubrectangleSum res;
    if (allow_empty) { res = {0, 0, 0, 0, 0}; }
    if (!n || !m) { return res; }
    int small = min(n, m), large = max(n, m); vector<lll> sum(large);
    for (int l = 0; l < small; ++l) {
        fill(sum.begin(), sum.end(), 0);
        for (int r = l; r < small; ++r) {
            for (int i = 0; i < large; ++i) { sum[i] += n <= m ? lll(a[r][i]) : lll(a[i][r]); }
            auto cur = sequence_detail::maximum(large, [&](int i) -> lll { return sum[i]; }, false);
            if (!res.exists() || cur.sum > res.sum) {
                res = n <= m ? SubrectangleSum{cur.sum, l, cur.l, r + 1, cur.r} : SubrectangleSum{cur.sum, cur.l, l, cur.r, r + 1};}}}
    return res;}

// T: O(n), M: O(n); length >= lo >= 1, exact average sum / (r - l), absent when lo > n.
template<typename T>
SubarraySum maximumAverageSubarray(const vector<T> &a, int lo) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size()); assert(lo >= 1);
    SubarraySum res;
    if (lo > n) { return res; }
    vector<lll> p(n + 1); vector<int> q(n + 1); int h = 0, t = 0;
    for (int i = 0; i < n; ++i) { p[i + 1] = p[i] + a[i]; }
    auto below = [&](int i, int j, int r) { return (p[r] - p[i]) * (r - j) < (p[r] - p[j]) * (r - i); };
    for (int r = lo; r <= n; ++r) {
        int j = r - lo;
        while (t - h >= 2 && (p[q[t - 1]] - p[q[t - 2]]) * (j - q[t - 1]) >= (p[j] - p[q[t - 1]]) * (q[t - 1] - q[t - 2])) { --t; }
        q[t++] = j;
        while (t - h >= 2 && !below(q[h + 1], q[h], r)) { ++h; }
        lll sum = p[r] - p[q[h]];
        if (!res.exists() || sum * (res.r - res.l) > res.sum * (r - q[h])) { res = {sum, q[h], r}; }}
    return res;}

// T: O(n * log(n + 1)), M: O(n); res[i] = longest chain ending at i, equal values form a minimum cover by non-increasing (strict) or decreasing chains.
template<typename T, typename Compare = std::less<T>>
vector<int> increasingSubsequenceLengths(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    int n = sequence_detail::checkedSize(a.size()); vector<int> tail, res(n);
    for (int i = 0; i < n; ++i) {
        int l = 0, r = int(tail.size());
        while (l < r) {
            int m = std::midpoint(l, r);
            if (strict ? cmp(a[tail[m]], a[i]) : !cmp(a[i], a[tail[m]])) { l = m + 1; }
            else { r = m; }}
        if (l == int(tail.size())) { tail.push_back(i); } else { tail[l] = i; }
        res[i] = l + 1;}
    return res;}
// T: O(n * log(n + 1)), M: O(n); indices of one longest chain, empty for n = 0.
template<typename T, typename Compare = std::less<T>>
vector<int> longestIncreasingSubsequence(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    auto len = increasingSubsequenceLengths(a, strict, cmp);
    int i = int(std::max_element(len.begin(), len.end()) - len.begin()); vector<int> res;
    if (len.empty()) { return res; }
    res.push_back(i);
    for (int j = i - 1; j >= 0; --j) {
        if (len[j] == len[i] - 1 && (strict ? cmp(a[j], a[i]) : !cmp(a[i], a[j]))) { res.push_back(i = j); }}
    reverse(res.begin(), res.end()); return res;}

// T: O(n * log(n + 1)), M: O(n) in Count operations; {length, count} of index-distinct longest chains, {0, 1} for n = 0; Count additions must fit or wrap intentionally.
template<typename Count, typename T, typename Compare = std::less<T>>
pair<int, Count> countLongestIncreasingSubsequences(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    sequence_detail::checkedSize(a.size()); Compression<T, Compare> c(a, cmp);
    using State = pair<int, Count>;
    auto merge = [](const State &x, const State &y) -> State {
        if (x.first != y.first) { return x.first > y.first ? x : y; }
        return {x.first, Count(x.second + y.second)};};
    SegmentTree tree(c.size(), State{0, Count(0)}, merge);
    for (const auto &x : a) {
        int r = c.id(x); auto cur = tree.query(0, r + !strict);
        if (!cur.first) { cur.second = Count(1); }
        ++cur.first; tree.set(r, merge(tree.get(r), cur));}
    auto res = tree.allQuery();
    if (!res.first) { res.second = Count(1); }
    return res;}
// T: O(n * log(n + 1)), M: O(n) in Count operations; number of nonempty index-distinct chains of any length.
template<typename Count, typename T, typename Compare = std::less<T>>
Count countIncreasingSubsequences(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    sequence_detail::checkedSize(a.size()); Compression<T, Compare> c(a, cmp);
    Fenwick<Count> tree(c.size()); Count res(0);
    for (const auto &x : a) {
        int r = c.id(x); Count cur = tree.prefixSum(r + !strict); cur += Count(1);
        tree.add(r, cur); res += cur;}
    return res;}

// T: O(n * log(n + 1)), M: O(n); maximum total weight with chain order, empty only when allowed, all-negative input picks one item.
template<typename T, typename W, typename Compare = std::less<T>>
WeightedSubsequence weightedIncreasingSubsequence(const vector<T> &a, const vector<W> &w, bool strict = true, bool allow_empty = false, Compare cmp = {}) {
    sequence_detail::checkInteger<W>(); int n = sequence_detail::checkedSize(a.size());
    assert(a.size() == w.size()); Compression<T, Compare> c(a, cmp);
    using State = pair<lll, int>;
    auto merge = [](State x, State y) -> State {
        if (x.second == -1) { return y; }
        if (y.second == -1) { return x; }
        return x.first >= y.first ? x : y;};
    SegmentTree tree(c.size(), State{0, -1}, merge); vector<int> p(n, -1);
    for (int i = 0; i < n; ++i) {
        int r = c.id(a[i]); auto old = tree.query(0, r + !strict); lll value = w[i];
        if (old.second >= 0 && old.first > 0) { value += old.first; p[i] = old.second; }
        tree.set(r, merge(tree.get(r), State{value, i}));}
    auto best = tree.allQuery();
    if (allow_empty && (best.second == -1 || best.first <= 0)) { return {0, true, {}}; }
    return {best.first, best.second >= 0, sequence_detail::restore(p, best.second)};}

// T: O(n * log(n + 1)), M: O(n); pairs i < j with cmp(a[j], a[i]), equivalent keys excluded.
template<typename T, typename Compare = std::less<T>>
lng inversionCount(const vector<T> &a, Compare cmp = {}) {
    sequence_detail::checkedSize(a.size()); Compression<T, Compare> c(a, cmp);
    Fenwick<lng> f(c.size()); lng res = 0, seen = 0;
    for (const auto &x : a) {
        int r = c.id(x); res += seen - f.prefixSum(r + 1);
        f.add(r, 1); ++seen;}
    return res;}
// T: O(n * log(n + 1)), M: O(n); fewest adjacent swaps turning a into b, -1 when b is not a rearrangement of a under cmp.
template<typename T, typename Compare = std::less<T>>
lng adjacentSwapDistance(const vector<T> &a, const vector<T> &b, Compare cmp = {}) {
    int n = sequence_detail::checkedSize(a.size());
    if (b.size() != a.size()) { return -1; }
    vector<int> x(n), y(n), pos(n);
    iota(x.begin(), x.end(), 0); iota(y.begin(), y.end(), 0);
    std::stable_sort(x.begin(), x.end(), [&](int i, int j) { return cmp(a[i], a[j]); });
    std::stable_sort(y.begin(), y.end(), [&](int i, int j) { return cmp(b[i], b[j]); });
    for (int i = 0; i < n; ++i) {
        if (cmp(a[x[i]], b[y[i]]) || cmp(b[y[i]], a[x[i]])) { return -1; }
        pos[x[i]] = y[i];}
    return inversionCount(pos);}

// T: O(n), M: O(n); smallest nonnegative integer absent from a, negative values ignored.
template<typename T>
int mex(const vector<T> &a) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size()), res = 0; vector<bool> seen(n + 1);
    for (const auto &x : a) {
        lll v = x;
        if (0 <= v && v <= n) { seen[int(v)] = true; }}
    while (seen[res]) { ++res; }
    return res;}

// T: O(n) callback calls, M: O(n); validity holds for empty windows and is hereditary, can_add(r) is pure, add/remove track the window exactly.
template<typename CanAdd, typename Add, typename Remove>
vector<int> monotoneWindowEnds(int n, CanAdd can_add, Add add, Remove remove) {
    assert(0 <= n && n < INT_MAX); vector<int> ends(n); int r = 0;
    for (int l = 0; l < n; ++l) {
        while (r < n && can_add(r)) { add(r++); }
        ends[l] = r;
        if (r == l) { ++r; } else { remove(l); }}
    return ends;}
// T: O(n), M: O(n); nonnegative values, 0 <= limit, ends of windows with sum <= limit.
template<typename T>
vector<int> nonnegativeSumWindowEnds(const vector<T> &a, lll limit) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size()); assert(limit >= 0);
    for (const auto &x : a) { assert(x >= 0); }
    lll sum = 0;
    return monotoneWindowEnds(n, [&](int r) { return lll(a[r]) <= limit - sum; }, [&](int r) { sum += a[r]; }, [&](int l) { sum -= a[l]; });}

// T: O(n), M: O(1); ascending input, indices i < j with a[i] + a[j] == target, {-1, -1} when none.
template<typename T>
pair<int, int> sortedPairSum(const vector<T> &a, lll target) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    assert(std::is_sorted(a.begin(), a.end())); int l = 0, r = n - 1;
    while (l < r) {
        lll sum = lll(a[l]) + a[r];
        if (sum == target) { return {l, r}; }
        if (sum < target) { ++l; } else { --r; }}
    return {-1, -1};}

// T: O(n), M: O(1); verified Boyer-Moore {index, exact frequency} of a class above n / 2, {-1, 0} when none.
template<typename T, typename Equal = std::equal_to<T>>
pair<int, int> majorityElement(const vector<T> &a, Equal eq = {}) {
    int n = sequence_detail::checkedSize(a.size()), candidate = -1, balance = 0;
    for (int i = 0; i < n; ++i) {
        if (!balance) { candidate = i; }
        balance += eq(a[candidate], a[i]) ? 1 : -1;}
    int count = 0;
    for (const auto &x : a) { count += eq(a[candidate], x); }
    return count > n / 2 ? pair{candidate, count} : pair{-1, 0};}
// T: O(n * log(min(n, k) + 1)), M: O(min(n, k)); k >= 1, verified Misra-Gries classes above n / k with exact counts, sorted by cmp.
template<typename T, typename Compare = std::less<T>>
vector<pair<T, int>> heavyHitters(const vector<T> &a, int k, Compare cmp = {}) {
    int n = sequence_detail::checkedSize(a.size()); assert(k >= 1);
    map<T, int, Compare> counts(cmp);
    for (const auto &x : a) {
        auto it = counts.find(x);
        if (it != counts.end()) { ++it->second; }
        else if (int(counts.size()) < k - 1) { counts.emplace(x, 1); }
        else {
            for (auto p = counts.begin(); p != counts.end();) {
                if (--p->second == 0) { p = counts.erase(p); } else { ++p; }}}}
    for (auto &[x, count] : counts) { count = 0; }
    for (const auto &x : a) {
        auto it = counts.find(x);
        if (it != counts.end()) { ++it->second; }}
    vector<pair<T, int>> res;
    for (const auto &[x, count] : counts) {
        if (count > n / k) { res.emplace_back(x, count); }}
    return res;}
