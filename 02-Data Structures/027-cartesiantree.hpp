#pragma once

#include "../01-Core/01-template.hpp"

// M: O(n); absent parents and children are -1, root is -1 when empty, range[i] = [l, r) is the index interval of i's subtree.
struct CartesianTree {
    int root = -1;
    vector<int> parent, leftChild, rightChild;
    vector<pair<int, int>> range;
};

// T: O(n), M: O(n); n <= INT_MAX, comp a strict weak order (leftmost equal element is the ancestor) or its reflexive closure like less_equal (rightmost).
template<typename T, typename C = std::less<T>>
CartesianTree cartesianTree(const vector<T> &a, C comp = C()) {
    assert(a.size() <= size_t(INT_MAX));
    int n = int(a.size());
    CartesianTree t{-1, vector<int>(n, -1), vector<int>(n, -1), vector<int>(n, -1), vector<pair<int, int>>(n)};
    vector<int> st;
    for (int i = 0; i < n; ++i) {
        int last = -1;
        while (!st.empty() && comp(a[i], a[st.back()])) {
            last = st.back(); st.pop_back();
            t.range[last].second = i;}
        if (last != -1) { t.parent[last] = i; t.leftChild[i] = last; }
        if (!st.empty()) { t.parent[i] = st.back(); t.rightChild[st.back()] = i; }
        t.range[i].first = st.empty() ? 0 : st.back() + 1;
        st.push_back(i);}
    for (int i : st) { t.range[i].second = n; }
    if (n) { t.root = st[0]; }
    return t;}

// T: O(n), M: O(n); n < 2^30; Euler tour (2n - 1 vertices), their depths and first positions: lca(u, v) is the min-depth vertex between first[u] and first[v].
inline tuple<vector<int>, vector<int>, vector<int>> lcaRmqBridge(const CartesianTree &t) {
    int n = int(t.parent.size());
    assert(n < (1 << 30));
    vector<int> tour, depth, first(n, -1);
    tour.reserve(max(0, 2 * n - 1)); depth.reserve(max(0, 2 * n - 1));
    for (int v = t.root, from = -1, d = 0; v != -1 && int(tour.size()) < 2 * n;) {
        if (from == t.parent[v]) { first[v] = int(tour.size()); }
        tour.push_back(v); depth.push_back(d);
        int l = t.leftChild[v], r = t.rightChild[v];
        int to = from == t.parent[v] && l != -1 ? l : from != r && r != -1 ? r : t.parent[v];
        d += to == t.parent[v] ? -1 : 1;
        from = v; v = to;}
    return {std::move(tour), std::move(depth), std::move(first)};}
