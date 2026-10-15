#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "02-traversal.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"

namespace treeiso_detail {
    // T: O(n), M: O(n); BFS parents (-1 at r) and top-down order of the tree g rooted at r.
    template<class G>
    pair<vector<int>, vector<int>> rooted(const G &g, int r) {
        assert(!g.directed && 0 <= r && r < g.n && g.arcs.size() == 2 * size_t(g.n - 1));
        auto t = bfs(g, {r});
        assert(int(t.order.size()) == g.n);
        return {std::move(t.parent), std::move(t.order)};}

    // T: O(n), M: O(n); rooted at the center, or at a virtual vertex n joined to both centers.
    template<class G>
    pair<vector<int>, vector<int>> centered(const G &g) {
        auto [par, order] = rooted(g, rooted(g, 0).se.back());
        vector<int> path{order.back()};
        while (par[path.back()] != -1) { path.push_back(par[path.back()]); }
        int k = int(path.size()), c = path[(k - 1) / 2];
        std::tie(par, order) = rooted(g, c);
        if (k % 2 == 0) { par.pb(-1); par[c] = par[path[k / 2]] = g.n; order.insert(order.begin(), g.n); }
        return {par, order};}

    // T: O(n), M: O(n); per-height tries order classes by height, then sorted child ids (prefix first); kids sorted by id.
    struct Classes {
        vector<int> id, start, kids;

        Classes(const vector<int> &par, const vector<int> &order) {
            int n = int(par.size()), h = 0, nodes, k = 0;
            vector<int> height(n), pos;
            start.assign(n + 1, 0);
            for (int i = int(order.size()) - 1; i >= 0; --i) {
                int v = order[i];
                if (par[v] != -1) { height[par[v]] = max(height[par[v]], height[v] + 1); ++start[par[v] + 1]; }
                h = max(h, height[v] + 1);}
            for (int v = 0; v < n; ++v) { start[v + 1] += start[v]; }
            pos.assign(start.begin(), start.end() - 1);
            kids.resize(start[n]); id.assign(n, -1);
            vector<int> at(height), link(n), mark(h + n, -1), step(h + n), head(h + n, -1), sib(h + n), ends(h + n, -1), st;
            nodes = h;
            for (int v = 0; v < n; ++v) {
                if (start[v] == start[v + 1]) { link[v] = ends[at[v]]; ends[at[v]] = v; }}
            for (int x = 0; x < h; ++x) {
                for (st.assign(1, x); !st.empty();) {
                    int t = st.back(); st.pop_back();
                    for (int v = ends[t]; v != -1; v = link[v]) {
                        id[v] = k;
                        int p = par[v];
                        if (p == -1) { continue; }
                        kids[pos[p]++] = v;
                        if (mark[at[p]] != k) { mark[at[p]] = k; sib[nodes] = head[at[p]]; head[at[p]] = nodes; step[at[p]] = nodes++; }
                        at[p] = step[at[p]];
                        if (pos[p] == start[p + 1]) { link[p] = ends[at[p]]; ends[at[p]] = p; }}
                    k += ends[t] != -1;
                    for (int u = head[t]; u != -1; u = sib[u]) { st.pb(u); }}}}
    };

    // T: O(n), M: O(n); maps the first rooted tree onto the second (shifted past it), empty when the roots' classes differ.
    inline vector<int> match(pair<vector<int>, vector<int>> a, const pair<vector<int>, vector<int>> &b, int n) {
        auto &[par, order] = a;
        int m = int(par.size()), r = order[0];
        for (int p : b.fi) { par.pb(p == -1 ? -1 : p + m); }
        for (int v : b.se) { order.pb(v + m); }
        Classes cl(par, order);
        if (cl.id[r] != cl.id[b.se[0] + m]) { return {}; }
        vector<int> res(m), st{r};
        res[r] = b.se[0] + m;
        while (!st.empty()) {
            int u = st.back(), v = res[u]; st.pop_back();
            for (int i = 0; i < cl.start[u + 1] - cl.start[u]; ++i) {
                int x = cl.kids[cl.start[u] + i];
                res[x] = cl.kids[cl.start[v] + i]; st.pb(x);}}
        res.resize(n);
        for (int &x : res) { x -= m; }
        return res;}
} // namespace treeiso_detail

// T: O(n), M: O(n); tree g, root in [0, n); ids in [0, K) equal iff subtrees are isomorphic, ordered as Classes.
template<class G>
vector<int> rootedTreeClassification(const G &g, int root) {
    auto [par, order] = treeiso_detail::rooted(g, root);
    return treeiso_detail::Classes(par, order).id;}

// T: O(n), M: O(n); Monte Carlo prod(x[height] + child) mod 2^61 - 1, x from seed; a non-isomorphic pair collides w.p. <= n / (2^61 - 1).
template<class G>
vector<ulng> rootedTreeHash(const G &g, int root, ulng seed = CustomHash::rnd) {
    constexpr ulng P = (ulng(1) << 61) - 1;
    auto [par, order] = treeiso_detail::rooted(g, root);
    vector<int> height(g.n);
    vector<ulng> res(g.n, 1), x;
    for (int i = int(order.size()) - 1; i > 0; --i) { height[par[order[i]]] = max(height[par[order[i]]], height[order[i]] + 1); }
    for (int i = 0; i <= height[root]; ++i) { x.pb(CustomHash::splitMix64(seed + ulng(i)) % P); }
    for (int i = int(order.size()) - 1; i > 0; --i) {
        int v = order[i], p = par[v];
        ulng f = x[height[p]] + res[v];
        ulll y = ulll(res[p]) * (f >= P ? f - P : f);
        ulng z = ulng(y & P) + ulng(y >> 61);
        res[p] = z >= P ? z - P : z;}
    return res;}

// T: O(n), M: O(n); tree g; ids in [0, K) equal iff g rooted at u and at v are isomorphic (automorphism orbits).
template<class G>
vector<int> allRootsClassification(const G &g) {
    auto [par, order] = treeiso_detail::centered(g);
    treeiso_detail::Classes cl(par, order);
    int m = int(par.size()), k = 1;
    vector<int> orbit(m), rep(m, -1);
    for (int v : order) {
        int r = rep[orbit[v]] == -1 ? (rep[orbit[v]] = v) : rep[orbit[v]];
        for (int i = cl.start[v], j = cl.start[r]; i < cl.start[v + 1]; ++i, ++j) {
            int u = cl.kids[i];
            bool same = i > cl.start[v] && cl.id[u] == cl.id[cl.kids[i - 1]];
            orbit[u] = r != v ? orbit[cl.kids[j]] : same ? orbit[cl.kids[i - 1]] : k++;}}
    orbit.resize(g.n);
    for (int &x : orbit) { x -= m - g.n; }
    return orbit;}

// T: O(n), M: O(n); trees g rooted at r and h rooted at s; res[v] in h for each v of g, empty when not isomorphic.
template<class G, class H>
vector<int> rootedTreeIsomorphism(const G &g, int r, const H &h, int s) {
    return treeiso_detail::match(treeiso_detail::rooted(g, r), treeiso_detail::rooted(h, s), g.n);}

// T: O(n), M: O(n); trees g and h; res[v] in h for each v of g, empty when not isomorphic.
template<class G, class H>
vector<int> unrootedTreeIsomorphism(const G &g, const H &h) {
    if (g.n != h.n) { return {}; }
    return treeiso_detail::match(treeiso_detail::centered(g), treeiso_detail::centered(h), g.n);}

// T: O(n), M: O(n); tree g; root = -1 is unrooted: the center's string, or both halves' strings for two centers.
template<class G>
string treeCanonicalString(const G &g, int root = -1) {
    auto [par, order] = root == -1 ? treeiso_detail::centered(g) : treeiso_detail::rooted(g, root);
    treeiso_detail::Classes cl(par, order);
    string res;
    for (vector<int> st{order[0]}; !st.empty();) {
        int v = st.back(); st.pop_back();
        if (v < 0) { res += ')'; continue; }
        if (v < g.n) { res += '('; st.pb(~v); }
        for (int i = cl.start[v + 1] - 1; i >= cl.start[v]; --i) { st.pb(cl.kids[i]); }}
    return res;}

// T: O(n), M: O(n); tree g; root = -1 counts all automorphisms, else those fixing root; M needs M(int) and *=.
template<class M, class G>
M automorphismCount(const G &g, int root = -1) {
    auto [par, order] = root == -1 ? treeiso_detail::centered(g) : treeiso_detail::rooted(g, root);
    treeiso_detail::Classes cl(par, order);
    M res = M(1);
    for (int i = 1, run = 1; i < int(cl.kids.size()); ++i) {
        bool same = par[cl.kids[i]] == par[cl.kids[i - 1]] && cl.id[cl.kids[i]] == cl.id[cl.kids[i - 1]];
        run = same ? run + 1 : 1;
        if (run > 1) { res *= M(run); }}
    return res;}
