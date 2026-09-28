#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// T: O(1), M: O(m). Success has vertices[i] --arcs[i]/edges[i]--> vertices[i+1].
// Each logical edge appears exactly once, including undirected loops.
// Failure has exists=false and all witness vectors empty.
struct EulerTrailResult {
    bool exists = true;
    vector<int> vertices, edges, arcs;
};

// T: O(n + m), M: O(n + m), including output. Iterative Hierholzer for
// directed/undirected Graph/CsrGraph, loops/multiedges allowed, weights ignored.
// start=-1 selects any valid start; otherwise 0<=start<n is a precondition.
// A valid vertex unable to start a trail is ordinary failure. Isolates are
// ignored for connectivity. No edges: {start or 0}; n=0: empty successful trail.
// Degree conditions plus consuming all edges certify existence; g is unchanged.
template<class G>
EulerTrailResult eulerianTrail(const G &g, int start = -1) {
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
        if (start != -1 && first != -1 && start != first) { return {false, {}, {}, {}}; } }
    else {
        for (int u = 0; u < g.n; ++u) {
            degree[u] = int(g[u].size());
            if (degree[u] & 1) { if (first == -1) { first = u; } ++count; }}
        if (count != 0 && count != 2) { return {false, {}, {}, {}}; }
        if (start != -1 && count && !(degree[start] & 1)) { return {false, {}, {}, {}}; } }
    if (start == -1) {
        start = first;
        if (start == -1) { start = 0; while (g[start].empty()) { ++start; }} }
    if (g[start].empty()) { return {false, {}, {}, {}}; }
    vector<int> next(g.n); vector<uint8_t> used(g.edges.size());
    vector<pair<int, int>> path{{start, -1}}; EulerTrailResult out;
    while (!path.empty()) {
        auto [u, incoming] = path.back(); int &i = next[u];
        while (i < int(g[u].size()) && used[g.arcs[g[u][i]].id]) { ++i; }
        if (i == int(g[u].size())) {
            out.vertices.push_back(u);
            if (incoming != -1) { out.arcs.push_back(incoming); }
            path.pop_back(); }
        else {
            int a = g[u][i++]; used[g.arcs[a].id] = true; path.emplace_back(g.arcs[a].to, a); }}
    if (out.arcs.size() != g.edges.size()) { return {false, {}, {}, {}}; }
    reverse(out.vertices.begin(), out.vertices.end()); reverse(out.arcs.begin(), out.arcs.end());
    out.edges.reserve(out.arcs.size());
    for (int a : out.arcs) { out.edges.push_back(g.arcs[a].id); }
    return out; }
