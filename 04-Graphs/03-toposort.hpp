#pragma once
#include "../01-Core/01-template.hpp"
#include "02-traversal.hpp"

// T: O(1), M: O(n); on a cycle order is empty and cycle is a directed witness.
struct TopologicalResult {
    bool acyclic = true;
    vector<int> order;
    CycleWitness cycle;
    bool unique = false;
};

namespace toposort_detail {
    // T: O(n + m), M: O(n); a complete order is the only one iff arcs join all consecutive vertices.
    template<class G>
    TopologicalResult ordered(const G &g, vector<int> order) {
        vector<int> pos(g.n); vector<char> linked(g.n);
        for (int i = 0; i < g.n; ++i) { pos[order[i]] = i; }
        for (const auto &e : g.arcs) {
            if (pos[e.to] == pos[e.from] + 1) { linked[pos[e.from]] = true; }}
        bool unique = std::count(linked.begin(), linked.end(), true) + 1 >= g.n;
        return {true, std::move(order), {}, unique};}
} // namespace toposort_detail

// T: O(n + m), M: O(n); directed only, reverse DFS finishing order.
template<class G>
TopologicalResult topologicalSortDfs(const G &g) {
    assert(g.directed);
    auto traversal = dfsForest(g);
    if (!traversal.cycle.arcs.empty()) { return {false, {}, std::move(traversal.cycle)}; }
    reverse(traversal.postorder.begin(), traversal.postorder.end());
    return toposort_detail::ordered(g, std::move(traversal.postorder));}

// T: O(n + m) FIFO, O(m + n * log(n + 1)) lexicographic, M: O(n); directed only, Kahn.
template<class G>
TopologicalResult topologicalSort(const G &g, bool lexicographic = false) {
    assert(g.directed);
    vector<int> degree(g.n), ready, order;
    for (const auto &a : g.arcs) { ++degree[a.to]; }
    for (int u = 0; u < g.n; ++u) { if (!degree[u]) { ready.push_back(u); }}
    if (lexicographic) { std::make_heap(ready.begin(), ready.end(), std::greater<int>{}); }
    int head = 0;
    while (lexicographic ? !ready.empty() : head < int(ready.size())) {
        int u;
        if (lexicographic) {
            std::pop_heap(ready.begin(), ready.end(), std::greater<int>{});
            u = ready.back(); ready.pop_back();}
        else { u = ready[head++]; }
        order.push_back(u);
        for (int a : g[u]) {
            int v = g.arcs[a].to;
            if (--degree[v] == 0) {
                ready.push_back(v);
                if (lexicographic) { std::push_heap(ready.begin(), ready.end(), std::greater<int>{}); }}}}
    if (int(order.size()) != g.n) { return {false, {}, findCycle(g)}; }
    return toposort_detail::ordered(g, std::move(order));}
