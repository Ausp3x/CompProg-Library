#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "03-toposort.hpp"
#include "19-max_flow.hpp"
#include "21-matching_bipartite.hpp"

// T: O(n + m * sqrt(n)), M: O(n + m); directed acyclic (asserted), multiedges allowed; paths by first vertex, each along original arcs.
template<class G>
vector<vector<int>> dagMinimumPathCover(const G &g) {
    assert(g.directed && topologicalSort(g).acyclic);
    vector<pair<int, int>> es;
    for (auto e : g.edges) { es.push_back({e.u, e.v}); }
    BipartiteMatching bm(g.n, g.n, es); bm.hopcroftKarp();
    vector<vector<int>> res;
    for (int v = 0; v < g.n; ++v) {
        if (bm.right[v] != -1) { continue; }
        res.emplace_back();
        for (int u = v; u != -1; u = bm.mate(u)) { res.back().push_back(u); }}
    return res;}

// T: O(1), M: O(n); chains partition the vertices, each in reachability order; antichain ascending with |antichain| = chains.size().
struct ChainCoverResult { vector<vector<int>> chains; vector<int> antichain; };

// T: O(n + m * n / w + n^2 * sqrt(n) / w), M: O(n^2 / w); directed acyclic (asserted); chains by first vertex.
template<class G>
ChainCoverResult dagMinimumChainCover(const G &g) {
    auto topo = topologicalSort(g);
    assert(g.directed && topo.acyclic);
    int n = g.n, W = (n + 63) / 64;
    vector<ulng> reach(size_t(n) * W), seen(W), masks;
    auto row = [&](int u) { return reach.data() + size_t(u) * W; };
    for (int i = int(topo.order.size()) - 1; i >= 0; --i) {
        ulng *r = row(topo.order[i]);
        for (int a : g[topo.order[i]]) {
            int v = g.arcs[a].to; const ulng *s = row(v);
            for (int k = 0; k < W; ++k) { r[k] |= s[k]; }
            r[v / 64] |= 1ULL << (v % 64);}}
    // Hopcroft-Karp on u -> v (v reachable from u); masks[d] holds the unused right vertices of layer d, free ones only at the last layer.
    vector<int> ml(n, -1), mr(n, -1), dist(n), layer(n), it(n), chosen(n), queue, st;
    while (true) {
        fill(seen.begin(), seen.end(), ~0ULL);
        if (n % 64) { seen[W - 1] = (1ULL << (n % 64)) - 1; }
        fill(dist.begin(), dist.end(), -1); queue.clear();
        for (int u = 0; u < n; ++u) {
            if (ml[u] == -1) { dist[u] = 0; queue.push_back(u); }}
        int limit = INT_MAX;
        for (int i = 0; i < int(queue.size()) && dist[queue[i]] <= limit; ++i) {
            int u = queue[i]; const ulng *r = row(u);
            for (int k = 0; k < W; ++k) {
                for (ulng c = r[k] & seen[k]; c; c &= c - 1) {
                    int v = 64 * k + std::countr_zero(c);
                    seen[k] ^= 1ULL << (v % 64); layer[v] = dist[u];
                    if (mr[v] == -1) { limit = dist[u]; }
                    else { dist[mr[v]] = dist[u] + 1; queue.push_back(mr[v]); }}}}
        if (limit == INT_MAX) { break; }
        masks.assign(size_t(limit + 1) * W, 0);
        for (int v = 0; v < n; ++v) {
            if (!(seen[v / 64] >> (v % 64) & 1) && layer[v] <= limit && (layer[v] < limit) == (mr[v] != -1)) {
                masks[size_t(layer[v]) * W + v / 64] |= 1ULL << (v % 64);}}
        fill(it.begin(), it.end(), 0);
        for (int i = 0; i < int(queue.size()) && dist[queue[i]] == 0; ++i) {
            st.assign(1, queue[i]);
            while (!st.empty()) {
                int x = st.back(); const ulng *r = row(x);
                ulng *mk = masks.data() + size_t(dist[x]) * W;
                while (it[x] < W && !(r[it[x]] & mk[it[x]])) { ++it[x]; }
                if (it[x] == W) { st.pop_back(); continue; }
                int v = 64 * it[x] + std::countr_zero(r[it[x]] & mk[it[x]]);
                mk[v / 64] ^= 1ULL << (v % 64); chosen[x] = v;
                if (mr[v] != -1) { st.push_back(mr[v]); continue; }
                for (int y : st) { ml[y] = chosen[y]; mr[chosen[y]] = y; }
                break;}}}
    ChainCoverResult res;
    for (int v = 0; v < n; ++v) {
        if (dist[v] != -1 && (seen[v / 64] >> (v % 64) & 1)) { res.antichain.push_back(v); }
        if (mr[v] != -1) { continue; }
        res.chains.emplace_back();
        for (int u = v; u != -1; u = ml[u]) { res.chains.back().push_back(u); }}
    return res;}

// T: O(1), M: O(n); antichain ascending, weight its exact total.
struct WeightedAntichainResult { lll weight = 0; vector<int> antichain; };

// T: O(n^2 * (n + m)), M: O(n + m); directed acyclic (asserted), w[v] >= 0; min cut of the split DAG, no transitive closure.
template<class G>
WeightedAntichainResult maximumWeightedAntichain(const G &g, const vector<lng> &w) {
    assert(g.directed && int(w.size()) == g.n && topologicalSort(g).acyclic);
    int n = g.n; lll total = 0;
    for (lng x : w) { assert(x >= 0); total += max(x, lng(0)); }
    // Out-copy v, in-copy n + v: s -> out (w), out_u -> in_v per arc, in -> out, in -> t (w), so finite cuts are W - w(antichain).
    MaxFlow<lll> mf(2 * n + 2);
    for (int v = 0; v < n; ++v) {
        lll c = max(w[v], lng(0));
        mf.addEdge(2 * n, v, c); mf.addEdge(n + v, 2 * n + 1, c); mf.addEdge(n + v, v, total + 1);}
    for (auto e : g.edges) { mf.addEdge(e.u, n + e.v, total + 1); }
    WeightedAntichainResult res{total - mf.flow(2 * n, 2 * n + 1), {}};
    auto side = mf.minCut(2 * n);
    for (int v = 0; v < n; ++v) {
        if (side[v] && !side[n + v]) { res.antichain.push_back(v); }}
    return res;}
