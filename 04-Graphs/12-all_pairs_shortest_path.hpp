#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "05-shortest_path.hpp"
#include "08-scc.hpp"
#include "13-shortest_path_advanced.hpp"

// S: O(n^2), Q: O(1) status, O(n) path, M: O(n^2); dist PATH_INF unreachable, -PATH_INF negative-infinite, otherwise exact.
struct AllPairsResult {
    vector<vector<lll>> dist;
    vector<vector<int>> parent_arc;

    explicit AllPairsResult(int n = 0) : dist(n, vector<lll>(n, PATH_INF)), parent_arc(n, vector<int>(n, -1)) {
        for (int u = 0; u < n; ++u) { dist[u][u] = 0; }}

    bool finite(int u, int v) const {
        assert(0 <= u && u < int(dist.size()) && 0 <= v && v < int(dist.size()));
        return dist[u][v] != PATH_INF && dist[u][v] != -PATH_INF;}
    template<class G = Graph>
    PathWitness path(const G &g, int u, int v) const {
        assert(g.n == int(dist.size()));
        if (!finite(u, v)) { return {}; }
        PathWitness out; out.exists = true; out.vertices.push_back(v);
        for (int i = 0; v != u && i < g.n; ++i) {
            int a = parent_arc[u][v];
            out.arcs.push_back(a); v = g.arcs[a].from; out.vertices.push_back(v);}
        reverse(out.vertices.begin(), out.vertices.end()); reverse(out.arcs.begin(), out.arcs.end());
        return out;}
};

// T: O(1), M: O(n); ecc PATH_INF where some vertex is unreachable, center and periphery ascending.
struct Eccentricities {
    vector<lll> ecc;
    lll radius, diameter;
    vector<int> center, periphery;
};

namespace all_pairs_detail {
    // T: O(n), M: O(1); copies one single-source result into row u.
    inline void fillRow(AllPairsResult &res, int u, const ShortestPathResult &sp, const vector<char> &negative) {
        for (int v = 0; v < int(sp.dist.size()); ++v) {
            if (!negative.empty() && negative[v]) { res.dist[u][v] = -PATH_INF; res.parent_arc[u][v] = -1; }
            else if (sp.reachable[v]) { res.dist[u][v] = sp.dist[v]; res.parent_arc[u][v] = sp.parent_arc[v]; }}}
} // namespace all_pairs_detail

// T: O(n^3 + m), M: O(n^2); signed weights, pairs joined through a negative cycle are -PATH_INF.
template<class G = Graph>
AllPairsResult floydWarshall(const G &g) {
    int n = g.n; AllPairsResult res(n); auto &d = res.dist; auto &par = res.parent_arc;
    for (int a = 0; a < int(g.arcs.size()); ++a) {
        const auto &e = g.arcs[a];
        if (e.w < d[e.from][e.to]) { d[e.from][e.to] = e.w; par[e.from][e.to] = a; }}
    for (int k = 0; k < n; ++k) {
        const auto &dk = d[k]; const auto &pk = par[k];
        for (int i = 0; i < n; ++i) {
            lll dik = d[i][k];
            if (dik == PATH_INF) { continue; }
            auto &di = d[i]; auto &pi = par[i];
            for (int j = 0; j < n; ++j) {
                // Clamping keeps values bounded; only pairs through a negative cycle reach -PATH_INF.
                if (dk[j] == PATH_INF) { continue; }
                lll c = max(dik + dk[j], -PATH_INF);
                if (c < di[j]) { di[j] = c; pi[j] = pk[j]; }}}}
    vector<int> cyc;
    for (int k = 0; k < n; ++k) { if (d[k][k] < 0) { cyc.push_back(k); }}
    for (int i = 0; i < n; ++i) {
        for (int k : cyc) {
            if (d[i][k] == PATH_INF) { continue; }
            for (int j = 0; j < n; ++j) {
                if (d[k][j] != PATH_INF) { d[i][j] = -PATH_INF; par[i][j] = -1; }}}}
    return res;}

// T: O(n * m + n * (n + m) * log(n + m + 1)), M: O(n^2); signed weights, the floydWarshall contract.
template<class G = Graph>
AllPairsResult johnson(const G &g) {
    int n = g.n; AllPairsResult res(n);
    Graph dg(g.directed ? 0 : n, true);
    if (!g.directed) { for (const auto &e : g.arcs) { dg.addEdge(e.from, e.to, e.w); }}
    auto comp = g.directed ? tarjanScc(g).component : tarjanScc(dg).component;
    // Bellman-Ford on arcs inside components: a change in pass n marks a component holding a negative cycle.
    vector<lll> h(n); vector<char> cyc(n), comp_cyc(n); bool any = false;
    for (int pass = 0, changed = 1; pass < n && changed; ++pass) {
        changed = 0;
        for (const auto &e : g.arcs) {
            if (comp[e.from] == comp[e.to] && h[e.from] + e.w < h[e.to]) {
                h[e.to] = h[e.from] + e.w; changed = 1;
                if (pass == n - 1) { comp_cyc[comp[e.to]] = true; }}}}
    for (int v = 0; v < n; ++v) { cyc[v] = comp_cyc[comp[v]]; any |= bool(cyc[v]); }
    fill(h.begin(), h.end(), lll(0));
    for (int pass = 0, changed = 1; pass < n && changed; ++pass) {
        changed = 0;
        for (const auto &e : g.arcs) {
            if (!cyc[e.from] && !cyc[e.to] && h[e.from] + e.w < h[e.to]) { h[e.to] = h[e.from] + e.w; changed = 1; }}}
    vector<char> negative(any ? n : 0);
    for (int u = 0; u < n; ++u) {
        if (any) {
            auto seen = bfs(g, {u}); vector<int> seeds;
            for (int v : seen.order) { if (cyc[v]) { seeds.push_back(v); }}
            fill(negative.begin(), negative.end(), char(0));
            if (!seeds.empty()) { for (int v : bfs(g, seeds).order) { negative[v] = true; }}}
        if (any && cyc[u]) { all_pairs_detail::fillRow(res, u, ShortestPathResult(n), negative); }
        else { all_pairs_detail::fillRow(res, u, shortest_path_advanced_detail::dijkstraPotential(g, {u}, h, cyc), negative); }}
    return res;}

// T: O(n^3 / w + n^2 + m * log(m + 1)), M: O(n^2); least maximum arc weight (greatest minimum when maximum), unreachable worst and diagonal best sentinel.
template<class G = Graph>
vector<vector<lll>> minimaxFloydWarshall(const G &g, bool maximum = false) {
    int n = g.n, words = (n + 63) / 64;
    vector<vector<lll>> res(n, vector<lll>(n, maximum ? -PATH_INF : PATH_INF));
    vector<vector<ulng>> to(n, vector<ulng>(words)), from(n, vector<ulng>(words));
    for (int u = 0; u < n; ++u) {
        res[u][u] = maximum ? PATH_INF : -PATH_INF;
        to[u][u / 64] |= ulng(1) << (u % 64); from[u][u / 64] |= ulng(1) << (u % 64);}
    vector<int> order(g.arcs.size());
    iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(), [&](int a, int b) { return maximum ? g.arcs[a].w > g.arcs[b].w : g.arcs[a].w < g.arcs[b].w; });
    vector<pair<int, int>> todo;
    for (int a : order) {
        lll w = g.arcs[a].w;
        auto add = [&](int u, int v) {
            if (to[u][v / 64] >> (v % 64) & 1) { return; }
            res[u][v] = w; to[u][v / 64] |= ulng(1) << (v % 64); from[v][u / 64] |= ulng(1) << (u % 64);
            todo.push_back({u, v});};
        add(g.arcs[a].from, g.arcs[a].to);
        // Each new pair (u, v) closes u -> to[v] and from[u] -> v once, so every pair costs O(n / w).
        while (!todo.empty()) {
            auto [u, v] = todo.back(); todo.pop_back();
            for (int i = 0; i < words; ++i) {
                for (ulng s = to[v][i] & ~to[u][i]; s; s &= s - 1) { add(u, 64 * i + std::countr_zero(s)); }
                for (ulng s = from[u][i] & ~from[v][i]; s; s &= s - 1) { add(64 * i + std::countr_zero(s), v); }}}}
    return res;}

// T: O(n^3 * log(k + 1) + m), M: O(n^2); least weight of a walk with exactly k arcs, PATH_INF when none, k * max |w| < 2^100.
template<class G = Graph>
vector<vector<lll>> fixedLengthShortestPath(const G &g, lng k) {
    int n = g.n; lll big = 0;
    assert(k >= 0);
    for (const auto &e : g.arcs) { big = max(big, abs(lll(e.w))); }
    assert(lll(k) * big < lll(1) << 100);
    vector<vector<lll>> res(n, vector<lll>(n, PATH_INF)), base = res;
    for (int u = 0; u < n; ++u) { res[u][u] = 0; }
    for (const auto &e : g.arcs) { base[e.from][e.to] = min(base[e.from][e.to], lll(e.w)); }
    auto mul = [&](const vector<vector<lll>> &a, const vector<vector<lll>> &b) {
        vector<vector<lll>> c(n, vector<lll>(n, 2 * PATH_INF));
        for (int i = 0; i < n; ++i) {
            for (int l = 0; l < n; ++l) {
                if (a[i][l] == PATH_INF) { continue; }
                // Sums with a PATH_INF entry stay above every finite total and are normalized below.
                for (int j = 0; j < n; ++j) { c[i][j] = min(c[i][j], a[i][l] + b[l][j]); }}
            for (int j = 0; j < n; ++j) { if (c[i][j] > PATH_INF / 2) { c[i][j] = PATH_INF; }}}
        return c;};
    for (; k > 0; k >>= 1) {
        if (k & 1) { res = mul(res, base); }
        if (k > 1) { base = mul(base, base); }}
    return res;}

// T: O(n^2), M: O(n); dist square with no -PATH_INF entry and n >= 1.
inline Eccentricities eccentricities(const vector<vector<lll>> &dist) {
    int n = int(dist.size());
    assert(n >= 1);
    for (const auto &row : dist) { assert(int(row.size()) == n && std::find(row.begin(), row.end(), -PATH_INF) == row.end()); (void)row; }
    Eccentricities out{vector<lll>(n, -PATH_INF), PATH_INF, -PATH_INF, {}, {}};
    for (int u = 0; u < n; ++u) {
        for (lll x : dist[u]) { out.ecc[u] = max(out.ecc[u], x); }
        out.radius = min(out.radius, out.ecc[u]); out.diameter = max(out.diameter, out.ecc[u]);}
    for (int u = 0; u < n; ++u) {
        if (out.ecc[u] == out.radius) { out.center.push_back(u); }
        if (out.ecc[u] == out.diameter) { out.periphery.push_back(u); }}
    return out;}
