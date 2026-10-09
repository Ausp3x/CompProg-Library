#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "08-scc.hpp"

// T: O(1), M: O(out); ascending left and right vertex lists.
struct BipartiteVertices { vector<int> left, right; };

// T: O(1), M: O(V); blocks [0, k): 0 holds the free-right side, k - 1 the free-left side, every edge (u, v) has left[u] <= right[v].
struct DulmageMendelsohn { int k; vector<int> left, right; };

// S: O(V + E), U: addEdge O(1) amortized, eraseEdge O(d) for d edges at u, augment O(V + E), Q: O(V + E), M: O(V + E); V = nl + nr.
// left[u], right[v] are matched edge IDs or -1; queries other than match assume a maximum matching (asserted).
struct BipartiteMatching {
    int nl, nr, size = 0;
    vector<pair<int, int>> edges;
    vector<vector<pair<int, int>>> adj;
    vector<int> left, right;

    BipartiteMatching(int NL, int NR, const vector<pair<int, int>> &es = {}) : nl(NL), nr(NR) {
        assert(nl >= 0 && nr >= 0);
        adj.resize(nl); left.assign(nl, -1); right.assign(nr, -1); edges.reserve(es.size());
        vector<int> deg(nl);
        for (auto [u, v] : es) {
            if (0 <= u && u < nl) { ++deg[u]; }}
        for (int u = 0; u < nl; ++u) { adj[u].reserve(deg[u]); }
        for (auto [u, v] : es) { addEdge(u, v); }}

    int mate(int u) const { return left[u] == -1 ? -1 : edges[left[u]].second; }

    int addEdge(int u, int v) {
        assert(0 <= u && u < nl && 0 <= v && v < nr && edges.size() < size_t(INT_MAX));
        edges.emplace_back(u, v); adj[u].emplace_back(v, int(edges.size()) - 1); return int(edges.size()) - 1;}
    void eraseEdge(int id) {
        assert(0 <= id && id < int(edges.size()));
        auto [u, v] = edges[id];
        auto it = std::find(adj[u].begin(), adj[u].end(), pair(v, id));
        assert(it != adj[u].end());
        if (it == adj[u].end()) { return; }
        *it = adj[u].back(); adj[u].pop_back();
        if (left[u] == id) { left[u] = right[v] = -1; --size; }}
    // Grows the matching on a CSR snapshot with slot mates; layered runs Hopcroft-Karp phases, otherwise Kuhn passes.
    void grow(bool layered) {
        vector<int> start(nl + 1), ml(nl, -1), mr(nr, -1), level(nl), it(nl), queue, st;
        for (int u = 0; u < nl; ++u) { start[u + 1] = start[u] + int(adj[u].size()); }
        vector<int> to(start[nl]), id(start[nl]);
        for (int u = 0; u < nl; ++u) {
            for (int k = start[u]; k < start[u + 1]; ++k) {
                std::tie(to[k], id[k]) = adj[u][k - start[u]];
                if (id[k] == left[u]) { ml[u] = k; mr[to[k]] = u; }}}
        // Level 0 marks an unvisited vertex in Kuhn passes; layered DFS follows level + 1 and retires exhausted vertices.
        auto dfs = [&](int s) {
            st.assign(1, s);
            if (!layered) { level[s] = -1; }
            while (!st.empty()) {
                int x = st.back();
                if (it[x] == start[x + 1]) {
                    level[x] = -1; st.pop_back();
                    if (!st.empty()) { ++it[st.back()]; }
                    continue;}
                int w = mr[to[it[x]]];
                if (w == -1) {
                    for (int y : st) { ml[y] = it[y]; mr[to[it[y]]] = y; level[y] = -1; }
                    ++size; return true;}
                if (layered ? level[w] == level[x] + 1 : level[w] == 0) {
                    if (!layered) { level[w] = -1; }
                    st.push_back(w);}
                else { ++it[x]; }}
            return false;};
        for (bool grown = true; grown;) {
            grown = false; fill(level.begin(), level.end(), layered ? -1 : 0); queue.clear();
            for (int u = 0; u < nl; ++u) {
                if (layered && ml[u] == -1) { level[u] = 0; queue.push_back(u); }}
            for (int i = 0; i < int(queue.size()); ++i) {
                int x = queue[i];
                for (int k = start[x]; k < start[x + 1]; ++k) {
                    int w = mr[to[k]];
                    if (w != -1 && level[w] == -1) { level[w] = level[x] + 1; queue.push_back(w); }}}
            std::copy(start.begin(), start.end() - 1, it.begin());
            for (int u = 0; u < nl; ++u) {
                if (ml[u] == -1 && level[u] == 0) { grown |= dfs(u); }}}
        fill(right.begin(), right.end(), -1);
        for (int u = 0; u < nl; ++u) {
            left[u] = ml[u] == -1 ? -1 : id[ml[u]];
            if (ml[u] != -1) { right[to[ml[u]]] = left[u]; }}}
    // T: O((V + E) * sqrt(V)), M: O(V + E); grows the current matching to a maximum one (phases accept any free endpoint).
    void hopcroftKarp() { grow(true); }
    // T: O(V * (V + E)), M: O(V + E); grows the current matching to a maximum one.
    void kuhn() { grow(false); }
    bool augment() {
        vector<int> from(nl + nr, -2), queue;
        for (int u = 0; u < nl; ++u) {
            if (left[u] == -1) { from[u] = -1; queue.push_back(u); }}
        for (int i = 0; i < int(queue.size()); ++i) {
            int x = queue[i];
            for (auto [v, e] : adj[x]) {
                if (from[nl + v] != -2) { continue; }
                from[nl + v] = e;
                if (right[v] == -1) {
                    for (int f = e; f != -1;) {
                        int y = edges[f].first, g = left[y];
                        left[y] = right[edges[f].second] = f; f = g == -1 ? -1 : from[nl + edges[g].second];}
                    ++size; return true;}
                int w = edges[right[v]].first;
                if (from[w] == -2) { from[w] = e; queue.push_back(w); }}}
        return false;}

    vector<int> match() const {
        vector<int> res;
        for (int u = 0; u < nl; ++u) {
            if (left[u] != -1) { res.push_back(left[u]); }}
        return res;}
    // Marks of vertices reachable by alternating paths from free left (fromLeft) or free right vertices; index nl + v for right v.
    vector<uint8_t> alternating(bool fromLeft) const {
        vector<uint8_t> mark(nl + nr);
        vector<int> queue;
        if (fromLeft) {
            for (int u = 0; u < nl; ++u) {
                if (left[u] == -1) { mark[u] = 1; queue.push_back(u); }}
            for (int i = 0; i < int(queue.size()); ++i) {
                for (auto [v, e] : adj[queue[i]]) {
                    if (mark[nl + v]) { continue; }
                    mark[nl + v] = 1;
                    if (right[v] != -1 && !mark[edges[right[v]].first]) { mark[edges[right[v]].first] = 1; queue.push_back(edges[right[v]].first); }}}
            return mark;}
        vector<vector<int>> radj(nr);
        for (int u = 0; u < nl; ++u) {
            for (auto [v, e] : adj[u]) { radj[v].push_back(e); }}
        for (int v = 0; v < nr; ++v) {
            if (right[v] == -1) { mark[nl + v] = 1; queue.push_back(v); }}
        for (int i = 0; i < int(queue.size()); ++i) {
            for (int e : radj[queue[i]]) {
                int u = edges[e].first;
                if (mark[u]) { continue; }
                mark[u] = 1;
                if (left[u] != -1 && !mark[nl + mate(u)]) { mark[nl + mate(u)] = 1; queue.push_back(mate(u)); }}}
        return mark;}
    vector<uint8_t> reachLeft() const {
        auto z = alternating(true);
        for (int v = 0; v < nr; ++v) { assert(!(z[nl + v] && right[v] == -1)); }
        return z;}
    BipartiteVertices konigVertexCover() const {
        auto z = reachLeft(); BipartiteVertices res;
        for (int u = 0; u < nl; ++u) {
            if (!z[u]) { res.left.push_back(u); }}
        for (int v = 0; v < nr; ++v) {
            if (z[nl + v]) { res.right.push_back(v); }}
        return res;}
    BipartiteVertices maximumIndependentSet() const {
        auto z = reachLeft(); BipartiteVertices res;
        for (int u = 0; u < nl; ++u) {
            if (z[u]) { res.left.push_back(u); }}
        for (int v = 0; v < nr; ++v) {
            if (!z[nl + v]) { res.right.push_back(v); }}
        return res;}
    bool minimumEdgeCover(vector<int> &res) const {
        assert((reachLeft(), true));
        res = match();
        vector<int> any(nr, -1);
        for (int u = 0; u < nl; ++u) {
            for (auto [v, e] : adj[u]) { any[v] = e; }}
        for (int u = 0; u < nl; ++u) {
            if (adj[u].empty()) { res.clear(); return false; }
            if (left[u] == -1) { res.push_back(adj[u][0].second); }}
        for (int v = 0; v < nr; ++v) {
            if (any[v] == -1) { res.clear(); return false; }
            if (right[v] == -1) { res.push_back(any[v]); }}
        return true;}
    vector<int> hallViolator() const {
        auto z = reachLeft(); vector<int> res;
        for (int u = 0; u < nl; ++u) {
            if (z[u]) { res.push_back(u); }}
        return res;}
    // Alternating-cycle structure: SCCs of left vertices with arc x -> mate-left of v for each unmatched edge (x, v).
    pair<SccResult, vector<uint8_t>> pairGraph() const {
        Graph h(nl, true); vector<uint8_t> loop(nl);
        for (int x = 0; x < nl; ++x) {
            for (auto [v, e] : adj[x]) {
                if (e == left[x] || right[v] == -1) { continue; }
                int y = edges[right[v]].first;
                if (y == x) { loop[x] = 1; }
                h.addEdge(x, y);}}
        auto scc = tarjanScc(h);
        for (auto &g : scc.groups) {
            if (g.size() > 1) { for (int x : g) { loop[x] = 1; } }}
        return {std::move(scc), loop};}
    vector<int> essentialEdges() const {
        auto zl = reachLeft(), zr = alternating(false);
        auto [scc, cyclic] = pairGraph();
        vector<int> res(edges.size());
        for (int u = 0; u < nl; ++u) {
            for (auto [v, e] : adj[u]) {
                if (e == left[u]) { res[e] = zl[u] || zr[nl + v] || cyclic[u] ? 1 : 2; }
                else { res[e] = zl[u] || zr[nl + v] || scc.component[u] == scc.component[edges[right[v]].first]; }}}
        return res;}
    DulmageMendelsohn dulmageMendelsohn() const {
        auto zl = reachLeft(), zr = alternating(false);
        auto [scc, cyclic] = pairGraph();
        vector<int> id(scc.groups.size(), -1);
        int k = 1;
        for (int c = 0; c < int(scc.groups.size()); ++c) {
            int x = scc.groups[c][0];
            if (!zl[x] && !zr[x]) { id[c] = k++; }}
        DulmageMendelsohn res{k + 1, vector<int>(nl), vector<int>(nr)};
        for (int u = 0; u < nl; ++u) { res.left[u] = zr[u] ? 0 : zl[u] ? k : id[scc.component[u]]; }
        for (int v = 0; v < nr; ++v) { res.right[v] = right[v] == -1 ? 0 : res.left[edges[right[v]].first]; }
        return res;}
};

// T: O(min(nl, nr) * nl * nr / w + E), M: O(nl * nr / w + V + E); bitset Kuhn, persistent marks until each augmentation.
inline BipartiteMatching bipartiteMatchingDense(int nl, int nr, const vector<pair<int, int>> &edges) {
    BipartiteMatching res(nl, nr, edges);
    int words = (nr + 63) / 64;
    vector<ulng> g(size_t(nl) * words), unseen(words);
    for (auto [u, v] : edges) { g[size_t(u) * words + v / 64] |= 1ULL << (v % 64); }
    vector<int> mateL(nl, -1), mateR(nr, -1), pos(nl), chosen(nl), st;
    auto reset = [&]() {
        fill(unseen.begin(), unseen.end(), ~0ULL);
        if (nr % 64) { unseen[words - 1] = (1ULL << (nr % 64)) - 1; }};
    reset();
    for (int s = 0; s < nl; ++s) {
        st.assign(1, s); pos[s] = 0;
        while (!st.empty()) {
            int x = st.back(); const ulng *row = g.data() + size_t(x) * words;
            while (pos[x] < words && !(row[pos[x]] & unseen[pos[x]])) { ++pos[x]; }
            if (pos[x] == words) { st.pop_back(); continue; }
            int v = 64 * pos[x] + std::countr_zero(row[pos[x]] & unseen[pos[x]]);
            unseen[v / 64] &= ~(1ULL << (v % 64)); chosen[x] = v;
            if (mateR[v] == -1) {
                for (int y : st) { mateL[y] = chosen[y]; mateR[chosen[y]] = y; }
                reset(); break;}
            st.push_back(mateR[v]); pos[mateR[v]] = 0;}}
    for (int e = 0; e < int(edges.size()); ++e) {
        auto [u, v] = edges[e];
        if (mateL[u] == v && res.left[u] == -1) { res.left[u] = res.right[v] = e; ++res.size; }}
    return res;}

// T: O((V + E) * sqrt(E)), M: O(V + E); each edge used at most once, vertex u (v) in at most bl[u] (br[v]) chosen edges; ascending IDs.
inline vector<int> bMatching(int nl, int nr, const vector<pair<int, int>> &edges, const vector<int> &bl, const vector<int> &br) {
    assert(int(bl.size()) == nl && int(br.size()) == nr);
    int n = nl + nr + 2, s = n - 2, t = n - 1, m = int(edges.size());
    vector<int> to, cap;
    vector<vector<int>> g(n);
    auto arc = [&](int u, int v, int c) {
        assert(c >= 0);
        g[u].push_back(int(to.size())); to.push_back(v); cap.push_back(c);
        g[v].push_back(int(to.size())); to.push_back(u); cap.push_back(0);};
    for (auto [u, v] : edges) { assert(0 <= u && u < nl && 0 <= v && v < nr); arc(u, nl + v, 1); }
    for (int u = 0; u < nl; ++u) { arc(s, u, bl[u]); }
    for (int v = 0; v < nr; ++v) { arc(nl + v, t, br[v]); }
    vector<int> level(n), it(n), queue, path;
    while (true) {
        fill(level.begin(), level.end(), -1); level[s] = 0; queue.assign(1, s);
        for (int i = 0; i < int(queue.size()); ++i) {
            for (int a : g[queue[i]]) {
                if (cap[a] > 0 && level[to[a]] == -1) { level[to[a]] = level[queue[i]] + 1; queue.push_back(to[a]); }}}
        if (level[t] == -1) { break; }
        fill(it.begin(), it.end(), 0);
        for (int x = s; true;) {
            if (x == t) {
                for (int a : path) { --cap[a]; ++cap[a ^ 1]; }
                path.clear(); x = s; continue;}
            while (it[x] < int(g[x].size()) && !(cap[g[x][it[x]]] > 0 && level[to[g[x][it[x]]]] == level[x] + 1)) { ++it[x]; }
            if (it[x] < int(g[x].size())) { path.push_back(g[x][it[x]]); x = to[path.back()]; continue; }
            if (x == s) { break; }
            level[x] = -1; x = to[path.back() ^ 1]; path.pop_back(); ++it[x];}}
    vector<int> res;
    for (int e = 0; e < m; ++e) {
        if (cap[2 * e] == 0) { res.push_back(e); }}
    return res;}
