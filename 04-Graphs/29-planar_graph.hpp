#pragma once

#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/01-dsu.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "05-shortest_path.hpp"

// T: O(1), M: O(m); value is the cut capacity, arcs its arcs from the s side ascending, flow[e] the net flow along edge e's u->v.
struct PlanarCut {
    lll value = 0;
    vector<int> arcs;
    vector<lng> flow;
};

// T: O(1), M: O(m); edge IDs ascending: a primal spanning forest, a dual spanning forest of the rest, and 2 * sum(genus) leftovers.
struct TreeCotree {
    vector<int> tree, cotree, leftover;
};

// S: O(n + m), Q: O(1), M: O(n + m); undirected, g[v] lists v's arcs counter-clockwise, an arc's face lies to its left.
struct PlanarEmbedding {
    int n, m, components = 0;
    vector<GraphArc> arcs;
    vector<int> first, next, prev, face, component, outer, genus;
    vector<vector<int>> faces;

    template<class G>
    explicit PlanarEmbedding(const G &g) : n(g.n), m(int(g.edges.size())), arcs(g.arcs), first(g.n, -1), next(g.arcs.size()), prev(g.arcs.size()), face(g.arcs.size(), -1) {
        assert(!g.directed);
        auto comps = connectedComponents(g);
        components = comps.count; component = std::move(comps.id); outer.assign(components, -1); genus.assign(components, 0);
        if (g.directed) { return; }
        for (int v = 0; v < n; ++v) {
            int d = int(g[v].size());
            if (d) { first[v] = g[v][0]; }
            for (int i = 0; i < d; ++i) {
                int a = g[v][i], b = g[v][(i + d - 1) % d];
                next[arcs[a].rev] = b; prev[b] = arcs[a].rev;}}
        vector<int> chi(components);
        for (const auto &e : g.edges) { --chi[component[e.u]]; }
        for (int v = 0; v < n; ++v) {
            ++chi[component[v]];
            if (g[v].size() == 0) { outer[component[v]] = int(faces.size()); faces.emplace_back(); ++chi[component[v]]; }
            for (int a : g[v]) {
                if (face[a] != -1) { continue; }
                auto &f = faces.emplace_back(); ++chi[component[v]];
                for (int b = a; face[b] == -1; b = next[b]) { face[b] = int(faces.size()) - 1; f.push_back(b); }}}
        for (int c = 0; c < components; ++c) { genus[c] = (2 - chi[c]) / 2; }}
    // T: O(n + m * log(m)), M: O(n + m); straight-line drawing of a simple graph, |x|, |y| <= 2^61; outer[c] has nonpositive area.
    static PlanarEmbedding fromDrawing(Graph g, const vector<pair<lng, lng>> &xy) {
        assert(int(xy.size()) == g.n);
        assert(std::ranges::all_of(xy, [](pair<lng, lng> p) { lng l = lng(1) << 61; return -l <= p.first && p.first <= l && -l <= p.second && p.second <= l; }));
        auto half = [](pair<lng, lng> d) { return d.second < 0 || (d.second == 0 && d.first < 0) ? 1 : d.first || d.second ? 0 : 2; };
        auto cross = [](pair<lng, lng> p, pair<lng, lng> q) { return lll(p.first) * q.second - lll(p.second) * q.first; };
        for (int v = 0; v < g.n; ++v) {
            auto dir = [&](int a) { return pair(xy[g.arcs[a].to].first - xy[v].first, xy[g.arcs[a].to].second - xy[v].second); };
            sort(g.adj[v].begin(), g.adj[v].end(), [&](int a, int b) {
                auto p = dir(a), q = dir(b);
                return half(p) != half(q) ? half(p) < half(q) : cross(p, q) > 0;});
            assert(std::ranges::all_of(std::views::iota(0, int(g[v].size())), [&](int i) {
                auto p = dir(g[v][i]), q = dir(g[v][(i + 1) % g[v].size()]);
                return half(p) < 2 && (g[v].size() == 1 || half(p) != half(q) || cross(p, q) != 0);}));}
        PlanarEmbedding res(g);
        for (int f = 0; f < int(res.faces.size()); ++f) {
            ulll area = 0;
            for (int a : res.faces[f]) {
                auto [x1, y1] = xy[res.arcs[a].from]; auto [x2, y2] = xy[res.arcs[a].to];
                area += ulll(lll(x1) * y2 - lll(y1) * x2);}
            if (!res.faces[f].empty() && lll(area) <= 0) { res.outer[res.component[res.arcs[res.faces[f][0]].from]] = f; }}
        return res;}

    int eulerCharacteristic() const { return n - m + int(faces.size()); }
    // T: O((deg(u) + deg(v)) * log(deg(u) + 2)), M: O(deg(u) + deg(v)); smallest face bounded by both u != v, -1 if none.
    int commonFace(int u, int v) const {
        assert(0 <= u && u < n && 0 <= v && v < n && u != v);
        auto around = [&](int x) {
            vector<int> fs;
            for (int a = first[x], b = a; b != -1; b = next[arcs[b].rev] == a ? -1 : next[arcs[b].rev]) { fs.push_back(face[b]); }
            return fs;};
        auto fu = around(u); sort(fu.begin(), fu.end());
        int res = -1;
        for (int f : around(v)) {
            if (binary_search(fu.begin(), fu.end(), f) && (res == -1 || f < res)) { res = f; }}
        return res;}

    // T: O(F + m), M: O(F + m), F = faces.size(); dual vertex f is face f, dual edge e joins the faces left and right of edge e's first arc.
    Graph dualGraph() const {
        Graph res(int(faces.size()));
        for (int a = 0; a < int(arcs.size()); ++a) {
            if (a < arcs[a].rev) { res.addEdge(face[a], face[arcs[a].rev], arcs[a].w); }}
        return res;}
    // T: O((n + m + F) * alpha(n + F)), M: O(n + m + F), F = faces.size(); forests take edges greedily in ID order.
    TreeCotree treeCotree() const {
        TreeCotree res;
        DSU primal(n), dual(int(faces.size()));
        for (int a = 0; a < int(arcs.size()); ++a) {
            if (a > arcs[a].rev) { continue; }
            if (primal.uniteSets(arcs[a].from, arcs[a].to)) { res.tree.push_back(arcs[a].id); }}
        vector<char> in_tree(m);
        for (int e : res.tree) { in_tree[e] = 1; }
        for (int a = 0; a < int(arcs.size()); ++a) {
            if (a > arcs[a].rev || in_tree[arcs[a].id]) { continue; }
            (dual.uniteSets(face[a], face[arcs[a].rev]) ? res.cotree : res.leftover).push_back(arcs[a].id);}
        return res;}

    // T: O((n + m) * log(n + m)), M: O(n + m); s's component has genus 0, s != t share a face unless disconnected; cap per arc >= 0, empty uses w.
    PlanarCut planarMinCutDual(int s, int t, const vector<lng> &cap = {}) const {
        assert(0 <= s && s < n && 0 <= t && t < n && s != t);
        assert(cap.empty() || cap.size() == arcs.size());
        PlanarCut res; res.flow.assign(m, 0);
        if (component[s] != component[t]) { return res; }
        assert(genus[component[s]] == 0);
        int f = commonFace(s, t), k = int(faces.size()), start = -1;
        assert(f != -1);
        if (f == -1) { return res; }
        for (int a : faces[f]) {
            if (arcs[a].from == t) { start = a; break; }}
        // A virtual s-t chord splits face f: its arcs from t back to s move to the new dual vertex k.
        vector<int> side(face);
        for (int a = start; arcs[a].from != s; a = next[a]) { side[a] = k; }
        Graph dual(k + 1, true);
        for (int a = 0; a < int(arcs.size()); ++a) {
            lng c = cap.size() == arcs.size() ? cap[a] : arcs[a].w;
            assert(c >= 0); dual.addEdge(side[a], side[arcs[a].rev], max<lng>(c, 0));}
        auto sp = dijkstra(dual, {f});
        res.value = sp.dist[k]; res.arcs = sp.path(dual, k).arcs;
        sort(res.arcs.begin(), res.arcs.end());
        for (int a = 0; a < int(arcs.size()); ++a) {
            if (a < arcs[a].rev && sp.reachable[side[a]]) { res.flow[arcs[a].id] = lng(sp.dist[side[arcs[a].rev]] - sp.dist[side[a]]); }}
        return res;}
};
