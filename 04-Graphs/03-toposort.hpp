#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"

// T: O(1), M: O(n); on a cycle order is empty and cycle is a directed witness.
struct TopologicalResult {
    bool acyclic = true;
    vector<int> order;
    CycleWitness cycle;
    bool unique = false;
};

// T: O(1), M: O(n); layers[i] holds the vertices whose longest incoming path has i arcs, in increasing order.
struct GenerationsResult {
    bool acyclic = true;
    vector<vector<int>> layers;
    CycleWitness cycle;
};

namespace toposort_detail {
    // T: O(n + m), M: O(n); a complete order is the only one iff arcs join all consecutive vertices, else the cycle result.
    template<class G = Graph>
    TopologicalResult ordered(const G &g, vector<int> order) {
        if (int(order.size()) != g.n) { return {false, {}, dfsForest(g).cycle}; }
        vector<int> pos(g.n); vector<char> linked(g.n);
        for (int i = 0; i < g.n; ++i) { pos[order[i]] = i; }
        for (const auto &e : g.arcs) {
            if (pos[e.to] == pos[e.from] + 1) { linked[pos[e.from]] = true; }}
        bool unique = std::count(linked.begin(), linked.end(), true) + 1 >= g.n;
        return {true, std::move(order), {}, unique};}

    // T: O(m + n * log(n + 1)), M: O(n); Kahn taking the ready vertex first under the heap order of cmp.
    template<class G, class C>
    vector<int> kahnHeap(const G &g, C cmp) {
        vector<int> degree = inDegrees(g), ready, order;
        for (int u = 0; u < g.n; ++u) { if (!degree[u]) { ready.push_back(u); }}
        std::make_heap(ready.begin(), ready.end(), cmp);
        while (!ready.empty()) {
            std::pop_heap(ready.begin(), ready.end(), cmp);
            int u = ready.back(); ready.pop_back(); order.push_back(u);
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (--degree[v] == 0) { ready.push_back(v); std::push_heap(ready.begin(), ready.end(), cmp); }}}
        return order;}
} // namespace toposort_detail

// T: O(n + m), M: O(n); directed only, Kahn with a FIFO queue.
template<class G = Graph>
TopologicalResult topologicalSort(const G &g) {
    assert(g.directed);
    vector<int> degree = inDegrees(g), order;
    for (int u = 0; u < g.n; ++u) { if (!degree[u]) { order.push_back(u); }}
    for (int i = 0; i < int(order.size()); ++i) {
        for (int a : g[order[i]]) {
            int v = g.arcs[a].to;
            if (--degree[v] == 0) { order.push_back(v); }}}
    return toposort_detail::ordered(g, std::move(order));}

// T: O(m + n * log(n + 1)), M: O(n); directed only, lexicographically smallest order.
template<class G = Graph>
TopologicalResult topologicalSortSmallest(const G &g) {
    assert(g.directed);
    return toposort_detail::ordered(g, toposort_detail::kahnHeap(g, std::greater<int>{}));}

// T: O(m + n * log(n + 1)), M: O(n); directed only, lexicographically largest order.
template<class G = Graph>
TopologicalResult topologicalSortLargest(const G &g) {
    assert(g.directed);
    return toposort_detail::ordered(g, toposort_detail::kahnHeap(g, std::less<int>{}));}

// T: O(m + n * log(n + 1)), M: O(n + m); directed only, lexicographically smallest position array (vertex 0 earliest, then 1, ...).
template<class G = Graph>
TopologicalResult topologicalSortEarliest(const G &g) {
    assert(g.directed);
    auto order = toposort_detail::kahnHeap(g.reverse(), std::less<int>{});
    reverse(order.begin(), order.end());
    return toposort_detail::ordered(g, std::move(order));}

// T: O(n + m), M: O(n); directed only, reverse DFS finishing order.
template<class G = Graph>
TopologicalResult topologicalSortDfs(const G &g) {
    assert(g.directed);
    auto traversal = dfsForest(g);
    if (!traversal.cycle.arcs.empty()) { return {false, {}, std::move(traversal.cycle)}; }
    reverse(traversal.postorder.begin(), traversal.postorder.end());
    return toposort_detail::ordered(g, std::move(traversal.postorder));}

// T: O(n + m), M: O(n); directed only, Kahn layers: each layer an antichain, as many as the longest chain (Mirsky).
template<class G = Graph>
GenerationsResult topologicalGenerations(const G &g) {
    auto topo = topologicalSort(g);
    if (!topo.acyclic) { return {false, {}, std::move(topo.cycle)}; }
    vector<int> level(g.n); int depth = 0;
    for (int u : topo.order) {
        depth = max(depth, level[u] + 1);
        for (int a : g[u]) { level[g.arcs[a].to] = max(level[g.arcs[a].to], level[u] + 1); }}
    GenerationsResult res; res.layers.resize(depth);
    for (int u = 0; u < g.n; ++u) { res.layers[level[u]].push_back(u); }
    return res;}

// T: O(n + m + limit * n * log(n)), M: O(n + m + limit * n); directed only, up to limit distinct orders (Varol–Rotem), empty iff limit == 0 or g has a cycle.
template<class G = Graph>
vector<vector<int>> allTopologicalSorts(const G &g, int limit) {
    assert(limit >= 0);
    vector<vector<int>> res;
    auto t = topologicalSort(g);
    if (limit <= 0 || !t.acyclic) { return res; }
    int n = g.n;
    vector<int> label(n), a(n), pos(n), order(n);
    for (int i = 0; i < n; ++i) { label[t.order[i]] = i; }
    vector<vector<int>> pred(n);
    for (int l = 0; l < n; ++l) {
        for (int e : g[t.order[l]]) {
            auto &p = pred[label[g.arcs[e].to]];
            if (p.empty() || p.back() != l) { p.push_back(l); }}}
    iota(a.begin(), a.end(), 0); iota(pos.begin(), pos.end(), 0);
    while (true) {
        for (int i = 0; i < n; ++i) { order[i] = t.order[a[i]]; }
        res.push_back(order);
        if (int(res.size()) >= limit) { return res; }
        int k = n - 1;
        for (; k >= 0; --k) {
            int j = pos[k];
            // k may pass its left neighbour l < k unless l -> k is an arc; adjacent comparable elements always share one.
            if (j > 0 && !binary_search(pred[k].begin(), pred[k].end(), a[j - 1])) {
                int l = a[j - 1]; a[j - 1] = k; a[j] = l; pos[k] = j - 1; pos[l] = j;
                break;}
            for (; j < k; ++j) { a[j] = a[j + 1]; pos[a[j]] = j; }
            a[k] = k; pos[k] = k;}
        if (k < 0) { return res; }}}
