#pragma once
#include "../01-Core/01-template.hpp"

// T: NA, M: O(1); weight >= 0, count=-1 unlimited else 0 <= count <= INT_MAX; status unreachable/unbounded has value 0.
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
    // T: O(n), M: O(1); 0 <= limit < INT_MAX, value >= 0 on the value axis.
    inline void validate(int limit, const vector<KnapsackItem> &items, bool by_value = false) {
        assert(0 <= limit && limit < INT_MAX && items.size() <= INT_MAX);
        for (const auto &x : items) {
            assert(x.weight >= 0 && x.count >= -1);
            if (by_value) { assert(x.value >= 0); }}}

    // S: O((W + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(W + 1) (trace O(n * (W + 1))); W=limit, c_i as in KnapsackDP.
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
                    if (!count) { break; }}}}

        KnapsackResult result(int target, bool negate = false) const {
            assert(0 <= target && target < int(dp.size()));
            if (dp[target] == NONE) { return {}; }
            if (infinite) { return {KnapsackStatus::unbounded, 0, target}; }
            return {KnapsackStatus::finite, negate ? -dp[target] : dp[target], target};}
        vector<int> restore(int target) const {
            assert(tracing && result(target).status == KnapsackStatus::finite);
            vector<int> copies(step.size());
            for (int i = int(step.size()) - 1; i >= 0; --i) {
                copies[i] = take[i][target]; target -= copies[i] * step[i];}
            assert(target == 0); return copies;}
    };
} // namespace knapsack_detail

// S: O((W + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(W + 1) (trace O(n * (W + 1))); W=capacity, c_i=min(count, W/weight) for finite positive weights, else 0.
// exact/atMost(w): max value at weight =w / <=w, any lng values; restore O(n) needs trace and a finite target.
struct KnapsackDP {
    knapsack_detail::Optimizer state;
    vector<int> best;

    KnapsackDP(int capacity, const vector<KnapsackItem> &items, bool trace = false)
        : state(capacity, items, false, trace), best(capacity + 1) {
        for (int w = 1; w <= capacity; ++w) {
            best[w] = state.dp[w] > state.dp[best[w - 1]] ? w : best[w - 1];}}
    KnapsackResult exact(int weight) const { return state.result(weight); }
    KnapsackResult atMost(int capacity) const {
        assert(0 <= capacity && capacity < int(best.size()));
        return exact(state.infinite ? 0 : best[capacity]);}
    vector<int> restore(int weight) const { return state.restore(weight); }
};

// S: O((V + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(V + 1) (trace O(n * (V + 1))); V=max_value, c_i=min(count, V/value) for finite positive values, else 0.
// values >= 0; minWeight(v) for exact value v; bestWithin(budget) O(V + 1), largest value <= V within budget.
struct ValueKnapsackDP {
    knapsack_detail::Optimizer state;

    ValueKnapsackDP(int max_value, const vector<KnapsackItem> &items, bool trace = false)
        : state(max_value, items, true, trace) {}
    KnapsackResult minWeight(int value) const { return state.result(value, true); }
    int bestWithin(lng budget) const {
        assert(budget >= 0);
        for (int v = int(state.dp.size()) - 1; v >= 0; --v) {
            if (state.dp[v] != state.NONE && -state.dp[v] <= budget) { return v; }}
        return 0;}
    vector<int> restore(int value) const { return state.restore(value); }
};

// T: O((n + 1) * (W + 1)), M: O(W + 1); exact-weight reachability, values ignored, weight 0 always reachable.
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

// S: O((n + 1) * (W + 1)), Q: O(1), M: O(W + 1), in T operations; counts multiplicity vectors, values ignored.
// T needs +,-,* and construction from lng (exact or intentional mod); unlimited zero weight makes reachable counts unbounded.
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
                    w += x.weight;}}}
        T sum(0);
        for (int w = 0; w <= capacity; ++w) { sum = T(sum + ways[w]); prefix[w] = sum; }}

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
