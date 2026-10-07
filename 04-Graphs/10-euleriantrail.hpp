#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// vertices[i] --arcs[i] (logical edges[i])--> vertices[i + 1]; failure is exists=false with empty vectors.
// T: O(1), M: O(m).
struct EulerTrailResult {
    bool exists = true;
    vector<int> vertices, edges, arcs;
};

// Directed or undirected, loops/multiedges allowed; start=-1 picks the smallest valid start.
// T: O(n + m), O(n + m * log(m + 1)) lexicographic (smallest vertex sequence), M: O(n + m).
template<class G>
EulerTrailResult eulerianTrail(const G &g, int start = -1, bool lexicographic = false) {
    assert(-1 <= start && start < g.n);
    if (!g.n) { return {}; }
    if (g.edges.empty()) { return {true, {start == -1 ? 0 : start}, {}, {}}; }
    vector<int> degree(g.n); int first = -1, count = 0;
    if (g.directed) {
        for (const auto &e : g.edges) { ++degree[e.u]; --degree[e.v]; }
        int last = -1;
        for (int u = 0; u < g.n; ++u) {
            if (degree[u] == 1 && first == -1) { first = u; }
            else if (degree[u] == -1 && last == -1) { last = u; }
            else if (degree[u]) { return {false, {}, {}, {}}; }}
        if ((first == -1) != (last == -1)) { return {false, {}, {}, {}}; }
        if (start != -1 && first != -1 && start != first) { return {false, {}, {}, {}}; }}
    else {
        for (int u = 0; u < g.n; ++u) {
            degree[u] = int(g[u].size());
            if (degree[u] & 1) { if (first == -1) { first = u; } ++count; }}
        if (count != 0 && count != 2) { return {false, {}, {}, {}}; }
        if (start != -1 && count && !(degree[start] & 1)) { return {false, {}, {}, {}}; }}
    if (start == -1) {
        start = first;
        if (start == -1) { start = 0; while (g[start].empty()) { ++start; }}}
    if (g[start].empty()) { return {false, {}, {}, {}}; }
    vector<vector<int>> sorted(lexicographic ? g.n : 0);
    for (int u = 0; u < int(sorted.size()); ++u) {
        sorted[u].assign(g[u].begin(), g[u].end());
        std::stable_sort(sorted[u].begin(), sorted[u].end(), [&](int a, int b) { return g.arcs[a].to < g.arcs[b].to; });}
    auto arc = [&](int u, int i) { return lexicographic ? sorted[u][i] : g[u][i]; };
    vector<int> next(g.n); vector<uint8_t> used(g.edges.size());
    vector<pair<int, int>> path{{start, -1}}; EulerTrailResult out;
    while (!path.empty()) {
        auto [u, incoming] = path.back(); int &i = next[u];
        while (i < int(g[u].size()) && used[g.arcs[arc(u, i)].id]) { ++i; }
        if (i == int(g[u].size())) {
            out.vertices.push_back(u);
            if (incoming != -1) { out.arcs.push_back(incoming); }
            path.pop_back();}
        else {
            int a = arc(u, i++); used[g.arcs[a].id] = true; path.emplace_back(g.arcs[a].to, a);}}
    if (out.arcs.size() != g.edges.size()) { return {false, {}, {}, {}}; }
    reverse(out.vertices.begin(), out.vertices.end()); reverse(out.arcs.begin(), out.arcs.end());
    out.edges.reserve(out.arcs.size());
    for (int a : out.arcs) { out.edges.push_back(g.arcs[a].id); }
    return out;}
