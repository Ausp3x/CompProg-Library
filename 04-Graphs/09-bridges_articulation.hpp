#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "../02-Data Structures/01-dsu.hpp"

// T: O(1), M: O(n + m); blocks are vertex-biconnected: bridges dyads, loops alone, isolates empty.
struct LowlinkResult {
    vector<int> tin, low, parent_arc, roots, bridges, component, edge_block, orientation, cut_components;
    vector<bool> is_bridge, is_art;
    vector<vector<int>> components, block_edges, block_vertices;
};

// T: O(n + m), M: O(n + m) including output; undirected Graph/CsrGraph, loops and multiedges allowed.
template<class G = Graph>
LowlinkResult lowlink(const G &g) {
    assert(!g.directed);
    LowlinkResult out; int n = g.n, m = int(g.edges.size()), timer = 0;
    out.tin.assign(n, -1); out.low.resize(n); out.parent_arc.assign(n, -1);
    out.is_art.assign(n, false); out.is_bridge.assign(m, false);
    out.edge_block.assign(m, -1); out.orientation.assign(m, -1);
    vector<int> pos(n), cuts(n), path, edges, marked(n, -1);
    auto block = [&](int last) {
        assert(out.block_edges.size() < size_t(INT_MAX));
        int b = int(out.block_edges.size()); out.block_edges.emplace_back(); out.block_vertices.emplace_back();
        for (int e = -1; e != last;) {
            e = edges.back(); edges.pop_back(); out.edge_block[e] = b; out.block_edges.back().push_back(e);
            for (int v : {g.edges[e].u, g.edges[e].v}) {
                if (marked[v] != b) { marked[v] = b; out.block_vertices.back().push_back(v); }}}};
    for (int s = 0; s < n; ++s) {
        if (out.tin[s] != -1) { continue; }
        out.roots.push_back(s); out.tin[s] = out.low[s] = timer++; path.push_back(s);
        if (g[s].empty()) {
            assert(out.block_edges.size() < size_t(INT_MAX));
            out.block_edges.emplace_back(); out.block_vertices.push_back({s});}
        while (!path.empty()) {
            int u = path.back(), p = out.parent_arc[u];
            if (pos[u] == int(g[u].size())) {
                path.pop_back();
                if (p == -1) { continue; }
                int v = g.arcs[p].from, e = g.arcs[p].id;
                out.low[v] = min(out.low[v], out.low[u]);
                if (out.low[u] > out.tin[v]) { out.is_bridge[e] = true; out.bridges.push_back(e); }
                if (out.low[u] >= out.tin[v]) { ++cuts[v]; block(e); }
                continue;}
            int a = g[u][pos[u]++], v = g.arcs[a].to, e = g.arcs[a].id;
            if (p != -1 && a == g.arcs[p].rev) { continue; }
            if (u == v) {
                if (out.orientation[e] == -1) { out.orientation[e] = a; edges.push_back(e); block(e); }
                continue;}
            if (out.tin[v] == -1) {
                out.orientation[e] = a; edges.push_back(e);
                out.parent_arc[v] = a; out.tin[v] = out.low[v] = timer++; path.push_back(v);}
            else if (out.tin[v] < out.tin[u]) {
                out.orientation[e] = a; edges.push_back(e); out.low[u] = min(out.low[u], out.tin[v]);}}}
    int c = int(out.roots.size()); out.cut_components.resize(n);
    for (int u = 0; u < n; ++u) {
        bool root = out.parent_arc[u] == -1;
        out.is_art[u] = cuts[u] > root; out.cut_components[u] = c - root + cuts[u];}
    out.component.assign(n, -1);
    for (int s = 0; s < n; ++s) {
        if (out.component[s] != -1) { continue; }
        out.component[s] = int(out.components.size()); out.components.push_back({s});
        auto &group = out.components.back();
        for (int i = 0; i < int(group.size()); ++i) {
            for (int a : g[group[i]]) {
                int v = g.arcs[a].to;
                if (!out.is_bridge[g.arcs[a].id] && out.component[v] == -1) {
                    out.component[v] = out.component[s]; group.push_back(v);}}}}
    return out;}

// T: O(1), M: O(n + m); arcs[e] orients edge e with minimum SCC count; ok iff connected and bridgeless.
struct StrongOrientationResult {
    bool ok = true;
    int count = 0, bridge = -1;
    pair<int, int> disconnected = {-1, -1};
    vector<int> arcs, component;
    vector<vector<int>> groups;
};

// T: O(n + m), M: O(n + m) including output and the transient lowlink result.
template<class G = Graph>
StrongOrientationResult strongOrientation(const G &g) {
    auto low = lowlink(g); StrongOrientationResult out;
    if (!low.bridges.empty()) { out.ok = false; out.bridge = low.bridges[0]; }
    if (low.roots.size() > 1) { out.ok = false; out.disconnected = {low.roots[0], low.roots[1]}; }
    out.count = int(low.components.size()); out.arcs = std::move(low.orientation);
    out.component = std::move(low.component); out.groups = std::move(low.components);
    return out;}

// S: O(n), U: O(alpha(n)) amortized, Q: O(n), M: O(n); an edge stream on n vertices (loops, multiedges allowed), indices from 0.
// Keeps a spanning forest and a spanning forest of the rest, which hold every bridge of the stream; bridges() lists their indices, increasing.
struct BridgesStreaming {
    int n, count = 0;
    DSU first, second;
    vector<array<int, 3>> kept;

    explicit BridgesStreaming(int N) : n(N), first(N), second(N) {}

    int add(int u, int v) {
        assert(0 <= u && u < n && 0 <= v && v < n && count < INT_MAX);
        if (first.uniteSets(u, v) || second.uniteSets(u, v)) { kept.push_back({u, v, count}); }
        return count++;}
    vector<int> bridges() const {
        Graph g(n, false);
        for (auto [u, v, i] : kept) { g.addEdge(u, v); }
        auto low = lowlink(g); vector<int> res;
        for (int e = 0; e < int(kept.size()); ++e) { if (low.is_bridge[e]) { res.push_back(kept[e][2]); }}
        return res;}
};

// T: O(n + m), M: O(n + m); arcs[e] orients edge e; true iff all out-degrees are even, else only each odd-edge component's smallest vertex is odd.
template<class G = Graph>
bool evenOutdegreeOrientation(const G &g, vector<int> &arcs) {
    assert(!g.directed);
    int m = int(g.edges.size()); auto forest = dfsForest(g);
    vector<uint8_t> odd(g.n), tree(m);
    for (int a : forest.parent_arc) { if (a != -1) { tree[g.arcs[a].id] = 1; }}
    arcs.assign(m, -1);
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        int e = g.arcs[a].id;
        if (!tree[e] && arcs[e] == -1) { arcs[e] = a; odd[g.arcs[a].from] ^= 1; }}
    // Children before parents: the edge to the parent fixes the child's parity.
    for (int i = g.n - 1; i >= 0; --i) {
        int v = forest.order[i], a = forest.parent_arc[v];
        if (a == -1) { continue; }
        if (odd[v] && g.arcs[a].rev != -1) { a = g.arcs[a].rev; }
        arcs[g.arcs[a].id] = a; odd[g.arcs[a].from] ^= 1;}
    return std::find(odd.begin(), odd.end(), 1) == odd.end();}

// T: O(n + m * sqrt(m) + b * (n + m)), weighted O(n + m * sqrt(m) + b * (n + m) * log(n + m)), b the vertex pairs searched (pairs of local bridges that are not bridges; unweighted, only single edges); M: O(n + m).
// {e, span} for each non-loop edge e = (u, v) in no triangle, increasing: span is the fewest edges (weighted: least weight, w >= 0) from u to v without e, -1 if none.
template<class G = Graph>
vector<pair<int, lll>> localBridges(const G &g, bool weighted = false) {
    assert(!g.directed);
    int n = g.n, m = int(g.edges.size()); auto low = lowlink(g);
    for (int e = 0; weighted && e < m; ++e) { assert(g.edges[e].w >= 0); }
    vector<int> mark(n, -1), scanned(n, -1); vector<uint8_t> tri(m), hit(n);
    auto rank = [&](int u) { return pair(g[u].size(), u); };
    for (int x = 0; x < n; ++x) {
        for (int a : g[x]) { mark[g.arcs[a].to] = x; }
        // Each distinct pair scans its lower-ranked endpoint once: O(min degree) per pair.
        for (int a : g[x]) {
            int y = g.arcs[a].to;
            if (rank(y) >= rank(x)) { continue; }
            if (scanned[y] != x) {
                scanned[y] = x; hit[y] = 0;
                for (int b : g[y]) {
                    int z = g.arcs[b].to;
                    if (z != x && z != y && mark[z] == x) { hit[y] = 1; break; }}}
            tri[g.arcs[a].id] = hit[y];}}
    vector<lll> dist(n, -1), span(m, -2); vector<int> seen, copies; vector<uint8_t> done(n);
    auto reach = [&](int v, lll d) {
        if (dist[v] != -1 && dist[v] <= d) { return false; }
        if (dist[v] == -1) { seen.push_back(v); }
        dist[v] = d; return true;};
    vector<pair<int, lll>> res;
    for (int e = 0; e < m; ++e) {
        int s = g.edges[e].u, t = g.edges[e].v;
        if (s == t || tri[e]) { continue; }
        if (low.is_bridge[e]) { span[e] = -1; }
        if (span[e] == -2) {
            // The pair's copies, listed from its lower-degree end, share one search that skips them all.
            int x = g[s].size() <= g[t].size() ? s : t; copies.clear();
            for (int a : g[x]) { if (g.arcs[a].to == s + t - x) { copies.push_back(g.arcs[a].id); }}
            reach(s, 0);
            if (weighted) {
                priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<>> pq; pq.push({0, s});
                while (!pq.empty()) {
                    auto [d, u] = pq.top(); pq.pop();
                    if (u == t) { break; }
                    if (done[u]) { continue; }
                    done[u] = 1;
                    for (int a : g[u]) {
                        const auto &arc = g.arcs[a];
                        if ((u != s || arc.to != t) && !done[arc.to] && reach(arc.to, d + arc.w)) { pq.push({dist[arc.to], arc.to}); }}}}
            else if (copies.size() == 1) {
                for (int i = 0; i < int(seen.size()) && dist[t] == -1; ++i) {
                    int u = seen[i];
                    for (int a : g[u]) { if (u != s || g.arcs[a].to != t) { reach(g.arcs[a].to, dist[u] + 1); }}}}
            lll lo = -1, hi = -1, d = dist[t];
            for (int c : copies) {
                lll w = weighted ? g.edges[c].w : 1;
                if (lo == -1 || w < lo) { hi = lo; lo = w; }
                else if (hi == -1 || w < hi) { hi = w; }}
            for (int c : copies) {
                lll other = (weighted ? g.edges[c].w : 1) == lo ? hi : lo;
                span[c] = d == -1 ? other : other == -1 ? d : min(d, other);}
            for (int v : seen) { dist[v] = -1; done[v] = 0; }
            seen.clear();}
        res.push_back({e, span[e]});}
    return res;}
