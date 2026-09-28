#pragma once

#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"

// A strict threshold accepts cmp(value,key); inclusive also accepts equivalence.
template<typename K>
struct OfflineThreshold { K key; bool inclusive = true; };

// n updates, q queries, each <= INT_MAX. cmp is a strict weak order.
// Calls apply(update index), then answer(query index) with exactly the eligible
// updates applied. Equal-key updates retain input order; queries at one key run
// strict before inclusive, retaining input order within each group. Answers must
// not change the accumulated update state. Callbacks must not mutate input keys
// or cmp's ordering; captured state belongs to the caller and is not reset here.
// All updates are applied at most once; updates beyond the last query are unused.
// Inputs are preserved. Callback exceptions propagate without rolling back state.
// T: O(n * log(n + 1) + q * log(q + 1) + n + q), M: O(n + q), plus callback costs.
template<typename K, typename Apply, typename Answer, typename Compare = std::less<K>>
void offlineSweep(const vector<K> &updates, const vector<OfflineThreshold<K>> &queries,
                  Apply apply, Answer answer, Compare cmp = {}) {
    assert(updates.size() <= size_t(INT_MAX) && queries.size() <= size_t(INT_MAX));
    int n = int(updates.size()), q = int(queries.size());
    if (!q) { return; }
    vector<int> u(n), v(q); iota(u.begin(), u.end(), 0); iota(v.begin(), v.end(), 0);
    sort(u.begin(), u.end(), [&](int i, int j) {
        if (cmp(updates[i], updates[j])) { return true; }
        if (cmp(updates[j], updates[i])) { return false; }
        return i < j;
    });
    sort(v.begin(), v.end(), [&](int i, int j) {
        if (cmp(queries[i].key, queries[j].key)) { return true; }
        if (cmp(queries[j].key, queries[i].key)) { return false; }
        if (queries[i].inclusive != queries[j].inclusive) { return !queries[i].inclusive; }
        return i < j;
    });
    int p = 0;
    for (int i : v) {
        const auto &query = queries[i];
        while (p < n && (query.inclusive ? !cmp(query.key, updates[u[p]]) : cmp(updates[u[p]], query.key))) {
            apply(u[p++]); }
        answer(i); }}

template<typename K>
struct OfflineRangeThreshold { int l, r; K key; bool inclusive = true; };

// Count values below each threshold in [l,r), in original query order; empty
// ranges are valid. 0 <= l <= r <= n <= INT_MAX. No arithmetic on key values.
// T: O(n * log(n + 1) + q * log(q + 1) + q * log(n + 1)), M: O(n + q), output O(q).
template<typename K, typename Compare = std::less<K>>
vector<int> offlineRangeCount(const vector<K> &a, const vector<OfflineRangeThreshold<K>> &queries,
                              Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX) && queries.size() <= size_t(INT_MAX));
    int n = int(a.size()), q = int(queries.size());
    if (!q) { return {}; }
    vector<OfflineThreshold<K>> keys; keys.reserve(q);
    for (const auto &query : queries) {
        assert(0 <= query.l && query.l <= query.r && query.r <= n);
        keys.push_back({query.key, query.inclusive}); }
    Fenwick<int> tree(n); vector<int> result(q);
    offlineSweep(a, keys, [&](int i) { tree.add(i, 1); }, [&](int i) {
        result[i] = tree.sum(queries[i].l, queries[i].r);
    }, cmp);
    return result; }
