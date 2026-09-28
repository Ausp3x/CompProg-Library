#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// T: O(1), M: O(n). Owning partition; component[u] indexes groups.
// IDs are source-to-sink: every intercomponent arc u->v has id[u] < id[v].
// Incomparable component IDs and vertex order within groups are unspecified.
struct SccResult {
    vector<int> component;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n), including output. Directed Graph/CsrGraph only;
// loops/multiedges allowed, weights ignored. Iterative Tarjan, no graph mutation.
template<class G>
SccResult tarjanScc(const G &g) {
    assert(g.directed);
    SccResult out{vector<int>(g.n, -1), {}};
    vector<int> ord(g.n, -1), low(g.n), next(g.n), path, active;
    int timer = 0;
    for (int root = 0; root < g.n; ++root) {
        if (ord[root] != -1) { continue; }
        ord[root] = low[root] = timer++; path.push_back(root); active.push_back(root);
        while (!path.empty()) {
            int u = path.back();
            if (next[u] < int(g[u].size())) {
                int v = g.arcs[g[u][next[u]++]].to;
                if (ord[v] == -1) {
                    ord[v] = low[v] = timer++; path.push_back(v); active.push_back(v); }
                else if (out.component[v] == -1) { low[u] = min(low[u], ord[v]); }
                continue; }
            path.pop_back();
            if (!path.empty()) { int p = path.back(); low[p] = min(low[p], low[u]); }
            if (low[u] == ord[u]) {
                int id = int(out.groups.size()); out.groups.emplace_back();
                while (true) {
                    int v = active.back(); active.pop_back();
                    out.component[v] = id; out.groups.back().push_back(v);
                    if (v == u) { break; }}}}}
    int k = int(out.groups.size());
    for (int &id : out.component) { id = k - 1 - id; }
    reverse(out.groups.begin(), out.groups.end()); return out; }

// T: O(n + m), M: O(n + m), including output and transpose adjacency.
// Same contract as tarjanScc; iterative two-pass Kosaraju alternative.
template<class G>
SccResult kosarajuScc(const G &g) {
    assert(g.directed);
    vector<uint8_t> seen(g.n);
    vector<int> next(g.n), path, order;
    for (int root = 0; root < g.n; ++root) {
        if (seen[root]) { continue; }
        seen[root] = true; path.push_back(root);
        while (!path.empty()) {
            int u = path.back();
            if (next[u] < int(g[u].size())) {
                int v = g.arcs[g[u][next[u]++]].to;
                if (!seen[v]) { seen[v] = true; path.push_back(v); }}
            else { order.push_back(u); path.pop_back(); }}}
    vector<vector<int>> rev(g.n);
    for (const auto &a : g.arcs) { rev[a.to].push_back(a.from); }
    SccResult out{vector<int>(g.n, -1), {}};
    for (auto it = order.rbegin(); it != order.rend(); ++it) {
        int root = *it;
        if (out.component[root] != -1) { continue; }
        int id = int(out.groups.size()); out.groups.emplace_back();
        out.component[root] = id; path.push_back(root);
        while (!path.empty()) {
            int u = path.back(); path.pop_back(); out.groups.back().push_back(u);
            for (int v : rev[u]) {
                if (out.component[v] == -1) { out.component[v] = id; path.push_back(v); }}}}
    return out; }

// T: O(n + m), M: O(k + d), including returned k-vertex/d-edge Graph.
// scc must be an unchanged valid SCC result for g. Produces a directed simple
// DAG with unit weights; no original edge IDs/weights or multiplicity retained.
// Component IDs are preserved; adjacency/edge order is unspecified.
template<class G>
Graph condensationDag(const G &g, const SccResult &scc) {
    assert(g.directed && scc.component.size() == size_t(g.n));
    int k = int(scc.groups.size()); Graph dag(k, true); vector<int> seen(k, -1);
    for (int c = 0; c < k; ++c) {
        for (int u : scc.groups[c]) {
            for (int a : g[u]) {
                int d = scc.component[g.arcs[a].to];
                if (d != c && seen[d] != c) { seen[d] = c; dag.addEdge(c, d); }}}}
    return dag; }
