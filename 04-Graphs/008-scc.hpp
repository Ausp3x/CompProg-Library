#pragma once
#include "../01-Core/01-template.hpp"
#include "001-graph.hpp"
#include "004-dsu.hpp"

// T: O(1), M: O(n); owning partition, IDs source-to-sink: every arc between components goes to a larger ID.
struct SccResult {
    vector<int> component;
    vector<vector<int>> groups;
};

// Directed Graph/CsrGraph, loops and multiedges allowed; iterative Tarjan.
// T: O(n + m), M: O(n) including output.
template<class G = Graph>
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
                    ord[v] = low[v] = timer++; path.push_back(v); active.push_back(v);}
                else if (out.component[v] == -1) { low[u] = min(low[u], ord[v]); }
                continue;}
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
    reverse(out.groups.begin(), out.groups.end()); return out;}

// T: O(n + m), M: O(n + m) including transpose; same contract as tarjanScc, iterative two-pass Kosaraju.
template<class G = Graph>
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
    return out;}

// scc is a valid SCC result for g; returns the simple unit-weight DAG on component IDs.
// T: O(n + m), M: O(k + d) for k components and d returned edges.
template<class G = Graph>
Graph condensationDag(const G &g, const SccResult &scc) {
    assert(g.directed && scc.component.size() == size_t(g.n));
    int k = int(scc.groups.size()); Graph dag(k, true); vector<int> seen(k, -1);
    for (int c = 0; c < k; ++c) {
        for (int u : scc.groups[c]) {
            for (int a : g[u]) {
                int d = scc.component[g.arcs[a].to];
                if (d != c && seen[d] != c) { seen[d] = c; dag.addEdge(c, d); }}}}
    return dag;}

// scc is a valid SCC result for g; IDs of the sink components (no arc leaves them), increasing.
// T: O(n + m), M: O(k) for k components.
template<class G = Graph>
vector<int> attractingComponents(const G &g, const SccResult &scc) {
    assert(g.directed && scc.component.size() == size_t(g.n));
    int k = int(scc.groups.size()); vector<uint8_t> left(k);
    for (const auto &a : g.arcs) {
        int c = scc.component[a.from];
        if (scc.component[a.to] != c) { left[c] = 1; }}
    vector<int> res;
    for (int c = 0; c < k; ++c) { if (!left[c]) { res.push_back(c); }}
    return res;}

// scc is a valid SCC result for g; true when every pair has a path one way (true on the empty graph).
// T: O(n + m), M: O(k) for k components.
template<class G = Graph>
bool isSemiconnected(const G &g, const SccResult &scc) {
    assert(g.directed && scc.component.size() == size_t(g.n));
    int k = int(scc.groups.size()), linked = 0; vector<uint8_t> next(k);
    for (const auto &a : g.arcs) {
        int c = scc.component[a.from];
        if (scc.component[a.to] == c + 1 && !next[c]) { next[c] = 1; ++linked; }}
    return linked >= k - 1;}

// scc is a valid SCC result for g; a minimum arc set making g strongly connected, between representatives groups[c][0].
// T: O(n + m), M: O(k + d) for k components and d condensation edges.
template<class G = Graph>
vector<pair<int, int>> strongConnectivityAugmentation(const G &g, const SccResult &scc) {
    Graph dag = condensationDag(g, scc); int k = dag.n;
    vector<pair<int, int>> res;
    if (k <= 1) { return res; }
    vector<int> indeg(k), next(k), path, heads, tails, sources, sinks;
    vector<uint8_t> seen(k);
    for (const auto &a : dag.arcs) { ++indeg[a.to]; }
    for (int s = 0; s < k; ++s) {
        if (indeg[s]) { continue; }
        // Greedy DFS over unseen vertices: every seen vertex reaches a matched sink, every unmatched sink is reached from a matched source.
        int found = -1; seen[s] = 1; path = {s};
        while (!path.empty() && found == -1) {
            int u = path.back();
            if (dag[u].empty()) { found = u; }
            else if (next[u] < int(dag[u].size())) {
                int v = dag.arcs[dag[u][next[u]++]].to;
                if (!seen[v]) { seen[v] = 1; path.push_back(v); }}
            else { path.pop_back(); }}
        if (found == -1) { sources.push_back(s); }
        else { heads.push_back(s); tails.push_back(found); }}
    for (int u = 0; u < k; ++u) { if (dag[u].empty() && !seen[u]) { sinks.push_back(u); }}
    int p = int(heads.size()), both = min(int(sources.size()), int(sinks.size()));
    auto add = [&](int c, int d) { res.push_back({scc.groups[c][0], scc.groups[d][0]}); };
    for (int i = 0; i < p; ++i) { add(tails[i], heads[(i + 1) % p]); }
    for (int i = 0; i < both; ++i) { add(sinks[i], sources[i]); }
    for (int i = both; i < int(sources.size()); ++i) { add(tails[0], sources[i]); }
    for (int i = both; i < int(sinks.size()); ++i) { add(sinks[i], heads[0]); }
    return res;}

// scc is a valid SCC result for g, w[u] >= 0; {largest weight of the distinct vertices of a walk, its components in walk order, ties to smaller IDs}, {0, {}} iff n == 0.
// T: O(n + m), M: O(k + d) for k components and d condensation edges.
template<class G = Graph>
pair<lll, vector<int>> maximumWeightWalk(const G &g, const SccResult &scc, const vector<lng> &w) {
    assert(w.size() == size_t(g.n) && std::all_of(w.begin(), w.end(), [](lng x) { return x >= 0; }));
    Graph dag = condensationDag(g, scc); int k = dag.n;
    vector<lll> best(k); vector<int> from(k, -1);
    for (int c = 0; c < k; ++c) {
        for (int u : scc.groups[c]) { best[c] += w[u]; }
        for (int a : dag[c]) {
            int d = dag.arcs[a].to;
            if (best[c] > best[d]) { best[d] = best[c]; from[d] = c; }}}
    if (!k) { return {0, {}}; }
    vector<int> path;
    for (int c = int(std::max_element(best.begin(), best.end()) - best.begin()); c != -1; c = from[c]) { path.push_back(c); }
    reverse(path.begin(), path.end());
    return {best[path.back()], path};}

// Requests are the arcs of g (loops need nothing); a minimum arc set with a path a -> b for every request (a, b), chain arcs then closing arcs by head.
// T: O((n + m) * alpha(n)), M: O(n).
template<class G = Graph>
vector<pair<int, int>> reachabilityRealization(const G &g) {
    auto scc = tarjanScc(g); auto dsu = graphComponents(g);
    vector<int> first(g.n, -1), last(g.n, -1); vector<uint8_t> cyclic(g.n);
    vector<pair<int, int>> res;
    // A topological chain through each weak component, closed into a cycle when it holds a nontrivial SCC.
    for (const auto &group : scc.groups) {
        for (int u : group) {
            int r = dsu.findSet(u);
            if (last[r] == -1) { first[r] = u; }
            else { res.push_back({last[r], u}); }
            last[r] = u; cyclic[r] |= group.size() > 1;}}
    for (int u = 0; u < g.n; ++u) {
        int r = dsu.findSet(u);
        if (cyclic[r] && first[r] == u) { res.push_back({last[r], u}); }}
    return res;}
