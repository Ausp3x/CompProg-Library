#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

namespace cycle_basis_detail {
    // T: O(n + m), M: O(n); BFS forest from each unreached vertex in increasing order, parent_edge -1 at roots; returns the BFS order.
    template<class G>
    vector<int> bfsForest(const G &g, vector<int> &parent_edge, vector<int> &depth) {
        assert(!g.directed);
        parent_edge.assign(g.n, -1); depth.assign(g.n, -1);
        vector<int> order;
        for (int s = 0; s < g.n; ++s) {
            if (depth[s] != -1) { continue; }
            depth[s] = 0; order.push_back(s);
            for (int i = int(order.size()) - 1; i < int(order.size()); ++i) {
                for (int a : g[order[i]]) {
                    int v = g.arcs[a].to;
                    if (depth[v] == -1) { depth[v] = depth[order[i]] + 1; parent_edge[v] = g.arcs[a].id; order.push_back(v); }}}}
        return order;}

    // T: O(r), M: O(1); reduces x by a fully reduced basis b sorted by decreasing value, keeping the smaller of x and x ^ y.
    inline ulng reduce(const vector<ulng> &b, ulng x) {
        for (ulng y : b) { x = min(x, x ^ y); }
        return x;}

    // T: O(r), M: O(1); inserts x keeping b fully reduced and sorted decreasing; false if x is already spanned.
    inline bool insert(vector<ulng> &b, ulng x) {
        x = reduce(b, x);
        if (!x) { return false; }
        for (ulng &y : b) { y = min(y, y ^ x); }
        b.insert(std::ranges::upper_bound(b, x, std::greater<>()), x);
        return true;}

    // T: O(path), M: O(1); calls f(e, side) for each forest edge on the u-v path, climbing from u (side 0) and v (side 1).
    template<class G, class F>
    void forestPath(const G &g, const vector<int> &parent_edge, const vector<int> &depth, int u, int v, F f) {
        while (u != v) {
            bool side = depth[v] > depth[u];
            int &x = side ? v : u;
            int e = parent_edge[x];
            f(e, side); x ^= g.edges[e].u ^ g.edges[e].v;}}
} // namespace cycle_basis_detail

// T: O(1), M: O(n + out); cycles[i] is the edge walk of chords[i] from its u to its v, then back along the forest.
struct CycleBasis {
    vector<int> parent_edge, depth, chords;
    vector<vector<int>> cycles;
};

// T: O(n + m + out), M: O(n + out); undirected, out = total cycle length.
template<class G>
CycleBasis fundamentalCycleBasis(const G &g) {
    CycleBasis res;
    cycle_basis_detail::bfsForest(g, res.parent_edge, res.depth);
    vector<char> tree(g.edges.size());
    for (int e : res.parent_edge) {
        if (e != -1) { tree[e] = 1; }}
    vector<int> down;
    for (int e = 0; e < int(g.edges.size()); ++e) {
        if (tree[e]) { continue; }
        int u = g.edges[e].u, v = g.edges[e].v;
        res.chords.push_back(e); down.clear();
        auto &cycle = res.cycles.emplace_back(1, e);
        cycle_basis_detail::forestPath(g, res.parent_edge, res.depth, v, u, [&](int f, bool side) { (side ? down : cycle).push_back(f); });
        cycle.insert(cycle.end(), down.rbegin(), down.rend());}
    return res;}

// T: O(1), M: O(n + out); cuts[i] lists, ascending, the edges crossing the split of tree[i]'s component by removing it.
struct CutBasis {
    vector<int> parent_edge, depth, tree;
    vector<vector<int>> cuts;
};

// T: O(n + m + out), M: O(n + m + out); undirected, same forest as fundamentalCycleBasis, out = total cut size.
template<class G>
CutBasis fundamentalCuts(const G &g) {
    CutBasis res;
    cycle_basis_detail::bfsForest(g, res.parent_edge, res.depth);
    vector<int> index(g.edges.size(), -1);
    for (int e : res.parent_edge) {
        if (e != -1) { index[e] = 0; }}
    for (int e = 0; e < int(g.edges.size()); ++e) {
        if (index[e] == 0) { index[e] = int(res.tree.size()); res.tree.push_back(e); }}
    res.cuts.resize(res.tree.size());
    for (int e = 0; e < int(g.edges.size()); ++e) {
        if (index[e] != -1) { res.cuts[index[e]].push_back(e); continue; }
        cycle_basis_detail::forestPath(g, res.parent_edge, res.depth, g.edges[e].u, g.edges[e].v,
                                       [&](int f, bool) { res.cuts[index[f]].push_back(e); });}
    return res;}

// T: O(n + m), M: O(n); undirected, m - n + (number of components).
template<class G>
int cycleSpaceDimension(const G &g) {
    return int(g.edges.size()) - g.n + connectedComponents(g).count;}

// T: O(k * r * m / 64 + s), M: O(r * m / 64); k sets of total size s, r = min(k, m); repeated IDs cancel.
template<class G>
bool isIndependentF2(const G &g, const vector<vector<int>> &sets) {
    int m = int(g.edges.size()), words = (m + 63) / 64;
    if (sets.size() > size_t(m)) { return false; }
    vector<vector<ulng>> rows; vector<int> pivot;
    for (const auto &set : sets) {
        vector<ulng> x(words);
        for (int e : set) {
            assert(0 <= e && e < m); x[e >> 6] ^= ulng(1) << (e & 63);}
        for (int i = 0; i < int(rows.size()); ++i) {
            if (x[pivot[i] >> 6] >> (pivot[i] & 63) & 1) {
                for (int j = 0; j < words; ++j) { x[j] ^= rows[i][j]; }}}
        int j = 0;
        while (j < words && !x[j]) { ++j; }
        if (j == words) { return false; }
        pivot.push_back(64 * j + std::countr_zero(x[j])); rows.push_back(std::move(x));}
    return true;}

// S: O(n + m * 64), Q: O(64), M: O(n + m); undirected, walk values XOR edge weight bit patterns; dist from each component's minimum vertex.
struct XorWalk {
    vector<int> component;
    vector<ulng> dist;
    vector<vector<ulng>> basis;

    template<class G>
    explicit XorWalk(const G &g) : component(g.n), dist(g.n) {
        vector<int> parent_edge, depth;
        for (int v : cycle_basis_detail::bfsForest(g, parent_edge, depth)) {
            if (parent_edge[v] == -1) { component[v] = int(basis.size()); basis.emplace_back(); continue; }
            auto e = g.edges[parent_edge[v]];
            int p = e.u ^ e.v ^ v;
            component[v] = component[p]; dist[v] = dist[p] ^ ulng(e.w);}
        for (int e = 0; e < int(g.edges.size()); ++e) {
            auto [u, v, w] = g.edges[e];
            if (parent_edge[u] != e && parent_edge[v] != e) { cycle_basis_detail::insert(basis[component[u]], dist[u] ^ dist[v] ^ ulng(w)); }}}

    ulll count(int u, int v) const {
        assert(0 <= u && u < int(dist.size()) && 0 <= v && v < int(dist.size()));
        return component[u] != component[v] ? 0 : ulll(1) << basis[component[u]].size();}
    bool contains(int u, int v, ulng x) const {
        return count(u, v) && !cycle_basis_detail::reduce(basis[component[u]], x ^ dist[u] ^ dist[v]);}
    ulng minXorWalk(int u, int v) const {
        assert(count(u, v)); return cycle_basis_detail::reduce(basis[component[u]], dist[u] ^ dist[v]);}
    ulng maxXorWalk(int u, int v) const {
        assert(count(u, v));
        ulng x = dist[u] ^ dist[v];
        for (ulng y : basis[component[u]]) { x = max(x, x ^ y); }
        return x;}
    ulng kth(int u, int v, ulng k) const {
        assert(k < count(u, v));
        const auto &b = basis[component[u]];
        ulng x = cycle_basis_detail::reduce(b, dist[u] ^ dist[v]);
        for (int i = 0; i < int(b.size()); ++i) {
            if (k >> i & 1) { x ^= b[b.size() - 1 - i]; }}
        return x;}
};

// S: O(n + m), Q: O(min(k, 64)^2), M: O(n + m); undirected, Monte Carlo: a false true has probability below 2^(k - 64).
struct CutSpaceHash {
    vector<ulng> label;

    template<class G>
    explicit CutSpaceHash(const G &g, ulng seed = CustomHash::rnd) : label(g.edges.size()) {
        vector<int> parent_edge, depth;
        auto order = cycle_basis_detail::bfsForest(g, parent_edge, depth);
        vector<ulng> acc(g.n);
        for (int e = 0; e < int(g.edges.size()); ++e) {
            int u = g.edges[e].u, v = g.edges[e].v;
            if (parent_edge[u] != e && parent_edge[v] != e) { label[e] = CustomHash::splitMix64(CustomHash::splitMix64(seed) + ulng(e)); acc[u] ^= label[e]; acc[v] ^= label[e]; }}
        // A forest edge carries the XOR of the chords leaving its subtree, so every cut XORs to zero.
        for (int i = g.n - 1; i >= 0; --i) {
            int v = order[i], e = parent_edge[v];
            if (e != -1) { label[e] = acc[v]; acc[g.edges[e].u ^ g.edges[e].v ^ v] ^= acc[v]; }}}

    bool disconnects(const vector<int> &edges) const {
        if (edges.size() > 64) { return true; }
        assert([&] { auto sorted = edges; sort(sorted.begin(), sorted.end()); return std::ranges::adjacent_find(sorted) == sorted.end(); }());
        vector<ulng> b;
        for (int e : edges) {
            assert(0 <= e && e < int(label.size()));
            if (!cycle_basis_detail::insert(b, label[e])) { return true; }}
        return false;}
};
