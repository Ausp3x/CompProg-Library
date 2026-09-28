#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"
#include "../02-Data Structures/03-segmenttree.hpp"
#include "../02-Data Structures/05-prefix_sum.hpp"
#include "../02-Data Structures/08-monotone_stack.hpp"
#include "04-compression.hpp"

// Dimensions < INT_MAX; sum/weight inputs are integral with <=64 bits.
// lll sums cover the full domain. Inputs are unchanged; ties are deterministic
// but arbitrary. Comparators are pure strict weak orderings.
// PrefixSum[2D], DifferenceArray[2D], slidingMinimum/Maximum reuse their canonical
// included headers; their separate arithmetic and domain contracts still apply.
struct SubarraySum {
    lll sum = 0;
    int l = -1, r = -1;
    bool exists() const { return l >= 0; }
};
struct CircularSubarraySum {
    lll sum = 0;
    int start = -1, length = 0;
    bool exists() const { return start >= 0; }
};
struct SubrectangleSum {
    lll sum = 0;
    int top = -1, left = -1, bottom = -1, right = -1;
    bool exists() const { return top >= 0; }
};
struct WeightedSubsequence {
    lll sum = 0;
    bool found = false;
    vector<int> indices;
};

namespace sequence_detail {
    inline int checkedSize(size_t n) { assert(n < INT_MAX); return int(n); }
    template<typename T>
    constexpr void checkInteger() { static_assert(std::is_integral_v<T> && sizeof(T) <= 8); }
    template<typename F>
    SubarraySum maximum(int n, F get, bool empty) {
        SubarraySum ans;
        if (empty) { ans = {0, 0, 0}; }
        lll sum = 0, low = 0; int pos = 0;
        for (int r = 1; r <= n; ++r) {
            sum += get(r - 1);
            if (!ans.exists() || sum - low > ans.sum) { ans = {sum - low, pos, r}; }
            if (sum < low) { low = sum; pos = r; }}
        return ans;}
    inline vector<int> restore(const vector<int> &p, int i) {
        vector<int> ans;
        for (; i >= 0; i = p[i]) { ans.push_back(i); }
        reverse(ans.begin(), ans.end()); return ans;}
} // namespace sequence_detail

// Nonempty [l,r) by default; n=0 is absent (l=r=-1,sum=0). allow_empty adds
// [0,0), also for n=0. All-negative optima remain valid negative answers.
// T: O(n), M: O(1).
template<typename T>
SubarraySum maximumSubarray(const vector<T> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>();
    return sequence_detail::maximum(sequence_detail::checkedSize(a.size()), [&](int i) -> lll { return a[i]; }, allow_empty);}

// Legacy: empty -> 0, otherwise nonempty optimum. Exact result must fit lng;
// widened accumulation prevents overflow even for very negative prefixes.
// T: O(n), M: O(1).
template<typename T>
lng kadane(const vector<T> &a) {
    lll ans = maximumSubarray(a).sum;
    assert(std::numeric_limits<lng>::min() <= ans && ans <= std::numeric_limits<lng>::max());
    return lng(ans);}

// 0<=lo<=hi; choose lo<=length<=hi. lo>n is absent; lo=0 permits empty.
// T: O(n), M: O(min(n,hi-lo) + 1); two running prefixes and monotone deque.
template<typename T>
SubarraySum boundedMaximumSubarray(const vector<T> &a, int lo, int hi) {
    sequence_detail::checkInteger<T>(); assert(0 <= lo && lo <= hi);
    int n = sequence_detail::checkedSize(a.size()); SubarraySum ans;
    if (lo > n) { return ans; }
    lll sum = 0, prefix = 0; deque<pair<int, lll>> q;
    for (int r = 0; r <= n; ++r) {
        if (r) { sum += a[r - 1]; }
        if (r < lo) { continue; }
        int l = r - lo;
        if (l) { prefix += a[l - 1]; }
        while (!q.empty() && q.front().first < r - hi) { q.pop_front(); }
        while (!q.empty() && q.back().second > prefix) { q.pop_back(); }
        q.emplace_back(l, prefix); lll value = sum - q.front().second;
        if (!ans.exists() || value > ans.sum) { ans = {value, q.front().first, r}; }}
    return ans;}

// At most one turn: witness (start+i)%n for i<length. Empty input is absent
// unless allow_empty adds start=0,length=0. Nonempty result has length in [1,n].
// T: O(n), M: O(1); compare ordinary optimum with complement of a minimum.
template<typename T>
CircularSubarraySum circularMaximumSubarray(const vector<T> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    auto high = maximumSubarray(a, allow_empty);
    CircularSubarraySum ans{high.sum, high.l, high.exists() ? high.r - high.l : 0};
    if (!n) { return ans; }
    auto low = sequence_detail::maximum(n, [&](int i) -> lll { return -lll(a[i]); }, false);
    lll total = accumulate(a.begin(), a.end(), lll(0));
    if (low.r - low.l < n && total + low.sum > ans.sum) {
        ans = {total + low.sum, low.r % n, n - (low.r - low.l)}; }
    return ans;}

// Rectangular input; [top,bottom) x [left,right), positive dimensions by default.
// allow_empty adds the all-zero-coordinate rectangle, even for empty dimensions.
// T: O(min(n,m)^2 * max(n,m) + n), M: O(max(n,m)); +n validates row shapes.
template<typename T>
SubrectangleSum maximumSubrectangle(const vector<vector<T>> &a, bool allow_empty = false) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    int m = n ? sequence_detail::checkedSize(a[0].size()) : 0;
    for (const auto &row : a) { assert(row.size() == size_t(m)); }
    SubrectangleSum ans;
    if (allow_empty) { ans = {0, 0, 0, 0, 0}; }
    if (!n || !m) { return ans; }
    int small = min(n, m), large = max(n, m); vector<lll> sum(large);
    for (int l = 0; l < small; ++l) {
        fill(sum.begin(), sum.end(), 0);
        for (int r = l; r < small; ++r) {
            for (int i = 0; i < large; ++i) { sum[i] += n <= m ? lll(a[r][i]) : lll(a[i][r]); }
            auto cur = sequence_detail::maximum(large, [&](int i) -> lll { return sum[i]; }, false);
            if (!ans.exists() || cur.sum > ans.sum) {
                ans = n <= m ? SubrectangleSum{cur.sum, l, cur.l, r + 1, cur.r}
                             : SubrectangleSum{cur.sum, cur.l, l, cur.r, r + 1}; }}}
    return ans;}

// Indices of a longest subsequence; strict=false allows comparator equivalence.
// Empty input returns the unique empty witness. No numeric sentinels.
// T: O(n * log(n + 1)), M: O(n), including returned indices.
template<typename T, typename Compare = std::less<T>>
vector<int> longestIncreasingSubsequence(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    int n = sequence_detail::checkedSize(a.size()); vector<int> tail, p(n, -1);
    for (int i = 0; i < n; ++i) {
        int l = 0, r = int(tail.size());
        while (l < r) {
            int m = std::midpoint(l, r);
            if (strict ? cmp(a[tail[m]], a[i]) : !cmp(a[i], a[tail[m]])) { l = m + 1; }
            else { r = m; }}
        if (l) { p[i] = tail[l - 1]; }
        if (l == int(tail.size())) { tail.push_back(i); } else { tail[l] = i; }}
    return sequence_detail::restore(p, tail.empty() ? -1 : tail.back());}

// {length,count} of INDEX-distinct LIS; empty -> {0,1}. Select Count explicitly:
// additions must fit, or Count implements intentional modular arithmetic.
// Value-distinct counts are a separate problem. Distinct classes <=2^29.
// T: O(n * log(n + 1)), M: O(n), in Count additions/copies and comparisons.
template<typename Count, typename T, typename Compare = std::less<T>>
pair<int, Count> countLongestIncreasingSubsequences(const vector<T> &a, bool strict = true, Compare cmp = {}) {
    sequence_detail::checkedSize(a.size()); Compression<T, Compare> c(a, cmp);
    using State = pair<int, Count>;
    auto merge = [](const State &x, const State &y) -> State {
        if (x.first != y.first) { return x.first > y.first ? x : y; }
        return {x.first, Count(x.second + y.second)}; };
    SegmentTree tree(c.size(), State{0, Count(0)}, merge);
    for (const auto &x : a) {
        int r = c.id(x); auto cur = tree.query(0, r + !strict);
        if (!cur.first) { cur.second = Count(1); }
        ++cur.first; tree.set(r, merge(tree.get(r), cur)); }
    auto ans = tree.allQuery(); if (!ans.first) { ans.second = Count(1); } return ans;}

// Maximize total weight with LIS ordering. Empty is optional; otherwise empty
// input is absent and all-negative inputs pick one item. Weights <=64 integral
// bits; distinct comparator classes <=2^29. No longest-length tie objective.
// T: O(n * log(n + 1)), M: O(n), including returned indices.
template<typename T, typename W, typename Compare = std::less<T>>
WeightedSubsequence weightedIncreasingSubsequence(const vector<T> &a, const vector<W> &w,
                                                bool strict = true, bool allow_empty = false, Compare cmp = {}) {
    sequence_detail::checkInteger<W>(); int n = sequence_detail::checkedSize(a.size());
    assert(a.size() == w.size()); Compression<T, Compare> c(a, cmp);
    using State = pair<lll, int>;
    auto merge = [](State x, State y) -> State {
        if (x.second == -1) { return y; }
        if (y.second == -1) { return x; }
        return x.first >= y.first ? x : y; };
    SegmentTree tree(c.size(), State{0, -1}, merge); vector<int> p(n, -1);
    for (int i = 0; i < n; ++i) {
        int r = c.id(a[i]); auto old = tree.query(0, r + !strict); lll value = w[i];
        if (old.second >= 0 && old.first > 0) { value += old.first; p[i] = old.second; }
        tree.set(r, merge(tree.get(r), State{value, i})); }
    auto best = tree.allQuery();
    if (allow_empty && (best.second == -1 || best.first <= 0)) { return {0, true, {}}; }
    return {best.first, best.second >= 0, sequence_detail::restore(p, best.second)};}

// Exact count of i<j with cmp(a[j],a[i]); equivalent pairs excluded.
// T: O(n * log(n + 1)), M: O(n); shared compression/Fenwick; no mutation.
template<typename T, typename Compare = std::less<T>>
lng inversionCount(const vector<T> &a, Compare cmp = {}) {
    sequence_detail::checkedSize(a.size()); Compression<T, Compare> c(a, cmp);
    Fenwick<lng> f(c.size()); lng ans = 0, seen = 0;
    for (const auto &x : a) { int r = c.id(x); ans += seen - f.prefixSum(r + 1); f.add(r, 1); ++seen; }
    return ans;}

// Maximal valid [l,r) endpoints. Validity MUST hold for empty ranges and be
// hereditary under deleting either end. State starts empty; can_add(r) is pure
// and tests appending r; add(r)/remove(l) maintain exactly the active window.
// Invalid singletons are skipped without add/remove. These monotonicity/state
// conditions are the caller's proof obligation, not inferred from callbacks.
// T: O(n) callback calls, M: O(n) returned, O(1) workspace plus callback state.
template<typename CanAdd, typename Add, typename Remove>
vector<int> monotoneWindowEnds(int n, CanAdd can_add, Add add, Remove remove) {
    assert(0 <= n && n < INT_MAX); vector<int> ends(n); int r = 0;
    for (int l = 0; l < n; ++l) {
        while (r < n && can_add(r)) { add(r++); }
        ends[l] = r;
        if (r == l) { ++r; } else { remove(l); }}
    return ends;}

// Nonnegative values and limit>=0 make sum<=limit hereditary. Full nonnegative
// lll limit allowed; subtraction avoids overflowing sum+value in the test.
// T: O(n), M: O(n) returned, O(1) workspace.
template<typename T>
vector<int> nonnegativeSumWindowEnds(const vector<T> &a, lll limit) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size()); assert(limit >= 0);
    for (const auto &x : a) { assert(x >= 0); }
    lll sum = 0;
    return monotoneWindowEnds(n, [&](int r) { return lll(a[r]) <= limit - sum; },
        [&](int r) { sum += a[r]; }, [&](int l) { sum -= a[l]; });}

// Ascending input; distinct indices i<j with a[i]+a[j]==target, or {-1,-1}.
// T: O(n), M: O(1); too-small sums discard l, too-large sums discard r.
template<typename T>
pair<int, int> sortedPairSum(const vector<T> &a, lll target) {
    sequence_detail::checkInteger<T>(); int n = sequence_detail::checkedSize(a.size());
    assert(std::is_sorted(a.begin(), a.end())); int l = 0, r = n - 1;
    while (l < r) {
        lll sum = lll(a[l]) + a[r];
        if (sum == target) { return {l, r}; }
        if (sum < target) { ++l; } else { --r; }}
    return {-1, -1};}

// Verified Boyer-Moore: {representative index, exact frequency}, or {-1,0}.
// Equal is a pure equivalence relation; a mandatory second pass verifies >n/2.
// T: O(n), M: O(1).
template<typename T, typename Equal = std::equal_to<T>>
pair<int, int> majorityElement(const vector<T> &a, Equal eq = {}) {
    int n = sequence_detail::checkedSize(a.size()), candidate = -1, balance = 0;
    for (int i = 0; i < n; ++i) {
        if (!balance) { candidate = i; }
        balance += eq(a[candidate], a[i]) ? 1 : -1; }
    int count = 0;
    for (const auto &x : a) { count += eq(a[candidate], x); }
    return count > n / 2 ? pair{candidate, count} : pair{-1, 0};}

// Verified Misra-Gries: representatives and EXACT frequencies >floor(n/k), k>=1,
// sorted by cmp; k=1 gives none. Cancellation deletes k distinct classes.
// T: O(n * log(min(n,k) + 1)), M: O(min(n,k)), including output. Full decrement
// scans total O(n) work: each removes a counter unit created by an earlier item.
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
                if (--p->second == 0) { p = counts.erase(p); } else { ++p; } }}}
    for (auto &[x, count] : counts) { count = 0; }
    for (const auto &x : a) { auto it = counts.find(x); if (it != counts.end()) { ++it->second; } }
    vector<pair<T, int>> ans;
    for (const auto &[x, count] : counts) { if (count > n / k) { ans.emplace_back(x, count); } }
    return ans;}
