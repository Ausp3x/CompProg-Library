#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"

// T: NA, M: O(1) plus K; a strict threshold accepts cmp(value, key), an inclusive one also accepts equivalent keys.
template<typename K>
struct OfflineThreshold { K key; bool inclusive = true; };

// T: O(n * log(n + 1) + q * log(q + 1)), M: O(n + q) plus callbacks; answer(i) sees exactly the eligible updates, equal keys keep input order.
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
        return i < j;});
    sort(v.begin(), v.end(), [&](int i, int j) {
        if (cmp(queries[i].key, queries[j].key)) { return true; }
        if (cmp(queries[j].key, queries[i].key)) { return false; }
        if (queries[i].inclusive != queries[j].inclusive) { return !queries[i].inclusive; }
        return i < j;});
    int p = 0;
    for (int i : v) {
        const auto &query = queries[i];
        while (p < n && (query.inclusive ? !cmp(query.key, updates[u[p]]) : cmp(updates[u[p]], query.key))) {
            apply(u[p++]);}
        answer(i);}}

// T: O((n + q) * log(n + q + 1)), M: O(n + q); each query counts a[l, r) values passing its threshold, 0 <= l <= r <= n, answers in query order.
template<typename K>
struct OfflineRangeThreshold { int l, r; K key; bool inclusive = true; };
template<typename K, typename Compare = std::less<K>>
vector<int> offlineRangeCount(const vector<K> &a, const vector<OfflineRangeThreshold<K>> &queries,
                              Compare cmp = {}) {
    assert(a.size() <= size_t(INT_MAX) && queries.size() <= size_t(INT_MAX));
    int n = int(a.size()), q = int(queries.size());
    if (!q) { return {}; }
    vector<OfflineThreshold<K>> keys; keys.reserve(q);
    for (const auto &query : queries) {
        assert(0 <= query.l && query.l <= query.r && query.r <= n);
        keys.push_back({query.key, query.inclusive});}
    Fenwick<int> tree(n); vector<int> res(q);
    offlineSweep(a, keys, [&](int i) { tree.add(i, 1); }, [&](int i) {
        res[i] = tree.sum(queries[i].l, queries[i].r);}, cmp);
    return res;}
