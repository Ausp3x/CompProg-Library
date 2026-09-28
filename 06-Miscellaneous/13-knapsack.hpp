#pragma once
#include "../01-Core/01-template.hpp"

// Independent item TYPES: weight >= 0, count=-1 for unlimited copies, otherwise
// 0 <= count <= INT_MAX. Equal types remain distinct for counting/witnesses.
// Axes are [0,limit], 0 <= limit < INT_MAX; item count n <= INT_MAX.
// Capacity optimization accepts every signed lng value; value DP needs value>=0.
// All finite optimization scores/weights fit lll on these domains.
struct KnapsackItem { int weight; lng value; int count = 1; };
enum class KnapsackStatus { finite, unreachable, unbounded };
struct KnapsackResult {
    KnapsackStatus status = KnapsackStatus::unreachable;
    lll value = 0;
    int target = -1;
};
template<class T>
struct KnapsackCount { KnapsackStatus status; T count; };

namespace knapsack_detail {
    inline void validate(int limit, const vector<KnapsackItem> &items, bool by_value = false) {
        assert(0 <= limit && limit < INT_MAX && items.size() <= INT_MAX);
        for (const auto &x : items) {
            assert(x.weight >= 0 && x.count >= -1);
            if (by_value) { assert(x.value >= 0); }}}

    // Shared max-plus axis DP; min weight uses value as step and -weight as score.
    // Trace row i stores the number of copies of type i in that stage's optimum.
    struct Optimizer {
        static constexpr lll NONE = -(lll(1) << 126);
        vector<lll> dp;
        vector<int> step;
        vector<vector<int>> take;
        bool infinite = false, tracing;

        Optimizer(int limit, const vector<KnapsackItem> &items, bool by_value, bool trace) : tracing(trace) {
            validate(limit, items, by_value);
            int n = int(items.size()); dp.assign(limit + 1, NONE); dp[0] = 0;
            if (trace) { step.resize(n); take.assign(n, vector<int>(limit + 1)); }
            for (int i = 0; i < n; ++i) {
                const auto &x = items[i]; lng stride = by_value ? x.value : x.weight;
                lll score = by_value ? -lll(x.weight) : lll(x.value);
                if (!x.count || stride > limit) { continue; }
                int w = int(stride);
                if (trace) { step[i] = w; }
                if (!w) {
                    if (score <= 0) { continue; }
                    if (x.count == -1) { infinite = true; continue; }
                    for (int j = 0; j <= limit; ++j) {
                        if (dp[j] != NONE) {
                            dp[j] += score * x.count;
                            if (trace) { take[i][j] = x.count; }}}
                    continue;}
                int bound = limit / w, count = x.count == -1 ? bound : min(x.count, bound);
                auto relax = [&](int j, int copies) {
                    int from = j - copies * w;
                    if (dp[from] != NONE && dp[from] + score * copies > dp[j]) {
                        dp[j] = dp[from] + score * copies;
                        if (trace) { take[i][j] = take[i][from] + copies; }}};
                // A finite bound covering the whole axis is equivalent to unlimited.
                if (count == bound) {
                    for (int j = w; j <= limit; ++j) { relax(j, 1); }
                    continue;}
                for (int block = 1; count; block *= 2) {
                    int copies = min(block, count); count -= copies;
                    for (int j = limit; j >= copies * w; --j) { relax(j, copies); }
                    if (!count) { break; }}}
        }

        KnapsackResult result(int target, bool negate = false) const {
            assert(0 <= target && target < int(dp.size()));
            if (dp[target] == NONE) { return {}; }
            if (infinite) { return {KnapsackStatus::unbounded, 0, target}; }
            return {KnapsackStatus::finite, negate ? -dp[target] : dp[target], target};}
        vector<int> restore(int target) const {
            assert(tracing && result(target).status == KnapsackStatus::finite);
            vector<int> copies(step.size());
            for (int i = int(step.size()) - 1; i >= 0; --i) {
                copies[i] = take[i][target]; target -= copies[i] * step[i]; }
            assert(target == 0); return copies;}
    };
} // namespace knapsack_detail

// S: O((W + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(W + 1) without trace;
// W=capacity; c_i=min(count,W/weight) for finite positive-weight types, else 0.
// Unlimited/axis-covering bounds use O(W + 1) each. Other positive bounded weights
// use binary grouping. Zero weights cost O(W + 1).
// trace=true adds O(n * (W + 1)) storage/initialization; restore takes O(n).
// exact(w) maximizes value at weight w; atMost(w) allows any weight <=w, including
// the empty choice, and breaks ties toward smaller weight. Negative exact optima
// are valid. Unlimited zero-weight positive value is unbounded iff reachable.
// Result.value is the finite optimum, target its weight; otherwise value=0.
struct KnapsackDP {
    knapsack_detail::Optimizer state;
    vector<int> best;

    KnapsackDP(int capacity, const vector<KnapsackItem> &items, bool trace = false)
        : state(capacity, items, false, trace), best(capacity + 1) {
        for (int w = 1; w <= capacity; ++w) {
            best[w] = state.dp[w] > state.dp[best[w - 1]] ? w : best[w - 1]; }}
    KnapsackResult exact(int weight) const { return state.result(weight); }
    KnapsackResult atMost(int capacity) const {
        assert(0 <= capacity && capacity < int(best.size()));
        return exact(state.infinite ? 0 : best[capacity]);}
    // T: O(n), M: O(n) returned; requires trace=true and a finite reachable target.
    vector<int> restore(int weight) const { return state.restore(weight); }
};

// S: O((V + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(V + 1) without trace;
// V=max_value, c_i=min(finite count,V/value) for positive values. Unlimited or
// axis-covering bounds cost O(V + 1) each; trace adds O(n * (V + 1)) setup/storage.
// Zero values may be omitted in a min-weight optimum.
// minWeight(v).value is the minimum weight for EXACT value v (or unreachable).
// bestWithin(budget) returns the largest reachable VALUE <=max_value whose minimum
// weight is <=budget; budget>=0. This TRUNCATED answer is a global maximum only
// when the caller's value bound covers a global optimum. It is always finite,
// including unlimited zero-weight positive-value items. minWeight(0)=0.
struct ValueKnapsackDP {
    knapsack_detail::Optimizer state;

    ValueKnapsackDP(int max_value, const vector<KnapsackItem> &items, bool trace = false)
        : state(max_value, items, true, trace) {}
    KnapsackResult minWeight(int value) const { return state.result(value, true); }
    // T: O(V + 1), M: O(1).
    int bestWithin(lng budget) const {
        assert(budget >= 0);
        for (int v = int(state.dp.size()) - 1; v >= 0; --v) {
            if (state.dp[v] != state.NONE && -state.dp[v] <= budget) { return v; }}
        return 0;}
    // T: O(n), M: O(n) returned; requires trace=true and a reachable exact value.
    vector<int> restore(int value) const { return state.restore(value); }
};

// T: O((n + 1) * (W + 1)), M: O(W + 1), including returned bytes; values ignored.
// Exact-weight reachability using remaining copies in each residue class.
// Empty choice reaches zero; at-most nonnegative-capacity feasibility is true.
inline vector<char> knapsackFeasible(int capacity, const vector<KnapsackItem> &items) {
    knapsack_detail::validate(capacity, items);
    vector<int> left(capacity + 1, -1); left[0] = 0;
    for (const auto &x : items) {
        if (!x.weight || !x.count || x.weight > capacity) { continue; }
        int count = x.count == -1 ? capacity / x.weight : min(x.count, capacity / x.weight);
        for (int w = 0; w <= capacity; ++w) {
            if (left[w] >= 0) { left[w] = count; }
            else if (w < x.weight || left[w - x.weight] <= 0) { left[w] = -1; }
            else { left[w] = left[w - x.weight] - 1; }}}
    vector<char> reachable(capacity + 1);
    for (int w = 0; w <= capacity; ++w) { reachable[w] = left[w] >= 0; }
    return reachable;}

// S: O((n + 1) * (W + 1)), Q: O(1), M: O(W + 1), in T arithmetic/copies; values ignored.
// Counts all feasible multiplicity vectors, NOT optimal selections or orders.
// T supports construction from lng, +,-,* and assignment; arithmetic and all
// prefix counts must be representable, arbitrary precision, or intentional mod.
// Reachability is independent of T, so a count congruent to zero stays finite.
// Unlimited zero weight gives infinitely many vectors iff the target is reachable.
// Finite zero weight multiplies counts by count+1. No binary grouping for counts.
template<class T>
struct KnapsackCounts {
    vector<T> ways, prefix;
    vector<char> reachable;
    bool infinite = false;

    KnapsackCounts(int capacity, const vector<KnapsackItem> &items)
        : reachable(knapsackFeasible(capacity, items)) {
        ways.assign(capacity + 1, T(0)); prefix.assign(capacity + 1, T(0));
        for (const auto &x : items) { infinite |= !x.weight && x.count == -1; }
        if (infinite) { return; }
        ways[0] = T(1);
        for (const auto &x : items) {
            if (!x.count || x.weight > capacity) { continue; }
            if (!x.weight) {
                for (auto &z : ways) { z = T(z * T(lng(x.count) + 1)); }
                continue;}
            int bound = capacity / x.weight;
            if (x.count == -1 || x.count >= bound) {
                for (int w = x.weight; w <= capacity; ++w) { ways[w] = T(ways[w] + ways[w - x.weight]); }
                continue;}
            vector<T> old = ways;
            for (int r = 0; r < x.weight; ++r) {
                T sum(0); int leaving = -1;
                for (int w = r; w <= capacity;) {
                    if (leaving >= 0) { sum = T(sum - old[leaving]); leaving += x.weight; }
                    sum = T(sum + old[w]); ways[w] = sum;
                    if (w / x.weight == x.count) { leaving = r; }
                    if (capacity - w < x.weight) { break; }
                    w += x.weight; }}}
        T sum(0);
        for (int w = 0; w <= capacity; ++w) { sum = T(sum + ways[w]); prefix[w] = sum; }
    }

    KnapsackCount<T> exact(int weight) const {
        assert(0 <= weight && weight < int(ways.size()));
        if (!reachable[weight]) { return {KnapsackStatus::unreachable, T(0)}; }
        if (infinite) { return {KnapsackStatus::unbounded, T(0)}; }
        return {KnapsackStatus::finite, ways[weight]};}
    KnapsackCount<T> atMost(int capacity) const {
        assert(0 <= capacity && capacity < int(prefix.size()));
        if (infinite) { return {KnapsackStatus::unbounded, T(0)}; }
        return {KnapsackStatus::finite, prefix[capacity]};}
};
