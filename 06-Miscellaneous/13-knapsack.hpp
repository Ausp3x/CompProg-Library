#pragma once
#include "../01-Core/01-template.hpp"

// T: NA, M: O(1); weight >= 0, count=-1 unlimited else 0 <= count <= INT_MAX; status unreachable/unbounded has value 0.
struct KnapsackItem { lng weight; lng value; int count = 1; };
enum class KnapsackStatus { finite, unreachable, unbounded };
struct KnapsackResult {
    KnapsackStatus status = KnapsackStatus::unreachable;
    lll value = 0;
    int target = -1;
};
template<class T>
struct KnapsackCount { KnapsackStatus status; T count; };

namespace knapsack_detail {
    // T: O(n), M: O(1); 0 <= limit < INT_MAX, value >= 0 on the value axis; returns limit clamped into [0, INT_MAX) for NDEBUG.
    inline int validate(int limit, const vector<KnapsackItem> &items, bool by_value = false) {
        assert(0 <= limit && limit < INT_MAX && items.size() <= INT_MAX);
        for ([[maybe_unused]] const auto &x : items) {
            assert(x.weight >= 0 && x.count >= -1);
            if (by_value) { assert(x.value >= 0); }}
        return min(max(limit, 0), INT_MAX - 1);}

    // T: O(1), M: O(1); no copies or a stride off [0, limit], including precondition violations under NDEBUG.
    inline bool idle(const KnapsackItem &x, lng stride, int limit) { return x.count == 0 || x.count < -1 || stride < 0 || stride > limit; }

    // S: O((W + 1) * (n + 1 + sum(log(c_i + 1)))), Q: O(1), M: O(W + 1) (trace O(n * (W + 1))); W=limit, c_i as in KnapsackDP.
    struct Optimizer {
        static constexpr lll NONE = -(lll(1) << 126);
        vector<lll> dp;
        vector<int> step;
        vector<vector<int>> take;
        bool infinite = false, tracing;

        Optimizer(int axis, const vector<KnapsackItem> &items, bool by_value, bool trace) : tracing(trace) {
            int limit = validate(axis, items, by_value);
            int n = int(items.size()); dp.assign(limit + 1, NONE); dp[0] = 0;
            if (trace) { step.resize(n); take.assign(n, vector<int>(limit + 1)); }
            for (int i = 0; i < n; ++i) {
                const auto &x = items[i]; lng stride = by_value ? x.value : x.weight;
                lll score = by_value ? -lll(x.weight) : lll(x.value);
                if (idle(x, stride, limit)) { continue; }
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
        : state(capacity, items, false, trace), best(state.dp.size()) {
        for (int w = 1; w < int(best.size()); ++w) {
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
    int top = knapsack_detail::validate(capacity, items);
    vector<int> left(top + 1, -1); left[0] = 0;
    for (const auto &x : items) {
        if (!x.weight || knapsack_detail::idle(x, x.weight, top)) { continue; }
        int step = int(x.weight), count = x.count == -1 ? top / step : min(x.count, top / step);
        for (int w = 0; w <= top; ++w) {
            if (left[w] >= 0) { left[w] = count; }
            else if (w < step || left[w - step] <= 0) { left[w] = -1; }
            else { left[w] = left[w - step] - 1; }}}
    vector<char> reachable(top + 1);
    for (int w = 0; w <= top; ++w) { reachable[w] = left[w] >= 0; }
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
        int top = int(reachable.size()) - 1;
        ways.assign(top + 1, T(0)); prefix.assign(top + 1, T(0));
        for (const auto &x : items) { infinite |= !x.weight && x.count == -1; }
        if (infinite) { return; }
        ways[0] = T(1);
        for (const auto &x : items) {
            if (knapsack_detail::idle(x, x.weight, top)) { continue; }
            if (!x.weight) {
                for (auto &z : ways) { z = T(z * T(lng(x.count) + 1)); }
                continue;}
            int step = int(x.weight), bound = top / step;
            if (x.count == -1 || x.count >= bound) {
                for (int w = step; w <= top; ++w) { ways[w] = T(ways[w] + ways[w - step]); }
                continue;}
            if (x.count == 1) {
                for (int w = top; w >= step; --w) { ways[w] = T(ways[w] + ways[w - step]); }
                continue;}
            // Window of count + 1 chain terms: new[w] = new[w - step] - old[w - span] + old[w], span <= top.
            int span = (x.count + 1) * step;
            vector<T> old(ways.begin(), ways.begin() + (top - span + 1));
            for (int w = step; w < span; ++w) { ways[w] = T(ways[w - step] + ways[w]); }
            for (int w = span; w <= top; ++w) { ways[w] = T(T(ways[w - step] - old[w - span]) + ways[w]); }}
        T sum(0);
        for (int w = 0; w <= top; ++w) { sum = T(sum + ways[w]); prefix[w] = sum; }}

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

// T: O((n + 1) * (H + 1)), M: O(H + n), in T operations; H=S/2-max(values), S=sum(values), values >= 0, H < INT_MAX, T as in KnapsackCounts.
// Unordered splits of the items into two equal-sum halves (1 for no items); the largest item is fixed in the first half.
template<class T>
T equalPartitionCount(const vector<int> &values) {
    if (values.empty()) { return T(1); }
    lng sum = 0; int big = 0;
    for (int i = 0; i < int(values.size()); ++i) {
        assert(values[i] >= 0);
        sum += values[i];
        if (values[i] > values[big]) { big = i; }}
    lng target = sum / 2 - values[big];
    if (sum % 2 || target < 0) { return T(0); }
    assert(target < INT_MAX);
    vector<KnapsackItem> items;
    for (int i = 0; i < int(values.size()); ++i) {
        if (i != big) { items.push_back({values[i], 0, 1}); }}
    return KnapsackCounts<T>(int(target), items).ways.back();}

// T: O((n + 1) * (min(P, S - P) + 1)), M: O(min(P, S - P) + n), in T operations; P=(S + target) / 2, S=sum(values), values >= 0.
// Sign vectors with sum(+-values[i]) = target, 0 when parity or |target| > S rules it out; min(P, S - P) < INT_MAX, T as in KnapsackCounts.
template<class T>
T signedSumCount(const vector<int> &values, lng target) {
    lng sum = 0;
    for (int x : values) { assert(x >= 0); sum += x; }
    if (target < -sum || target > sum || (sum + target) % 2) { return T(0); }
    lng half = min((sum + target) / 2, (sum - target) / 2);
    assert(half < INT_MAX);
    vector<KnapsackItem> items;
    for (int x : values) { items.push_back({x, 0, 1}); }
    return KnapsackCounts<T>(int(half), items).ways.back();}

// T: O((n + 1) * (W + 1)), M: O(W + n); W=total, 0 <= W < INT_MAX; unlimited coins >= 0 as KnapsackDP items of value -1.
// Fewest coins summing to each total 0..W, -1 if none; KnapsackDP.restore gives a witness.
inline vector<int> minimumCoins(int total, const vector<int> &coins) {
    vector<KnapsackItem> items;
    for (int c : coins) { items.push_back({c, -1, -1}); }
    KnapsackDP dp(total, items);
    vector<int> fewest(dp.best.size());
    for (int w = 0; w < int(fewest.size()); ++w) {
        auto r = dp.exact(w);
        fewest[w] = r.status == KnapsackStatus::finite ? int(-r.value) : -1;}
    return fewest;}

// T: O(n * log(n) + (n + 1) * (W + 1)), M: O(W + n), in T operations; W=total, 0 <= W < INT_MAX, T as in KnapsackCounts.
// Ordered sequences of parts summing to each total 0..W; parts >= 1, each listed part a distinct type.
template<class T>
vector<T> compositionCountTable(int total, vector<int> parts) {
    int top = knapsack_detail::validate(total, {});
    sort(parts.begin(), parts.end());
    assert(parts.empty() || parts[0] >= 1);
    parts.erase(parts.begin(), lower_bound(parts.begin(), parts.end(), 1));
    vector<T> ways(top + 1, T(0)); ways[0] = T(1);
    for (int w = 1; w <= top; ++w) {
        T sum(0);
        for (int p : parts) {
            if (p > w) { break; }
            sum = T(sum + ways[w - p]);}
        ways[w] = sum;}
    return ways;}
