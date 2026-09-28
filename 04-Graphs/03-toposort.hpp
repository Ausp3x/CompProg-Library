#pragma once
#include "../01-Core/01-template.hpp"
#include "02-traversal.hpp"

// T: O(1), M: O(n). Empty graphs are acyclic with empty order. On a cycle,
// order is empty and cycle contains an oriented closed vertex/arc witness.
struct TopologicalResult {
    bool acyclic = true;
    vector<int> order;
    CycleWitness cycle;
};

// T: O(n + m), M: O(n), including output. Requires a directed Graph/CsrGraph.
// Iterative DFS reverse finishing order; weights are ignored.
template<class G>
TopologicalResult topologicalSortDfs(const G &g) {
    assert(g.directed);
    auto traversal = dfsForest(g);
    if (!traversal.cycle.arcs.empty()) { return {false, {}, std::move(traversal.cycle)}; }
    reverse(traversal.postorder.begin(), traversal.postorder.end());
    return {true, std::move(traversal.postorder), {}}; }

// T: O(n + m), or O(m + n * log(n + 1)) with lexicographic=true; M: O(n),
// including output. Requires directed Graph/CsrGraph. Kahn's algorithm with
// FIFO or minimum available vertex; parallel arcs count separately in degrees.
// The min-heap choice gives the lexicographically smallest topological order.
template<class G>
TopologicalResult topologicalSort(const G &g, bool lexicographic = false) {
    assert(g.directed);
    TopologicalResult out; vector<int> degree(g.n), ready;
    for (const auto &a : g.arcs) { ++degree[a.to]; }
    for (int u = 0; u < g.n; ++u) { if (!degree[u]) { ready.push_back(u); }}
    if (lexicographic) { std::make_heap(ready.begin(), ready.end(), std::greater<int>{}); }
    int head = 0;
    while (lexicographic ? !ready.empty() : head < int(ready.size())) {
        int u;
        if (lexicographic) {
            std::pop_heap(ready.begin(), ready.end(), std::greater<int>{});
            u = ready.back(); ready.pop_back(); }
        else { u = ready[head++]; }
        out.order.push_back(u);
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (--degree[v] == 0) {
                ready.push_back(v);
                if (lexicographic) { std::push_heap(ready.begin(), ready.end(), std::greater<int>{}); }}}}
    if (int(out.order.size()) != g.n) {
        out.acyclic = false; out.order.clear(); out.cycle = findCycle(g); }
    return out; }
