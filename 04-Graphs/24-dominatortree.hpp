#pragma once
#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"

// S: O(V), U: NA, Q: O(1), M: O(V); idom[v] = -1 for the root and unreachable v; tin[v] = -1 if v is unreachable from the root.
struct DominatorTree {
    int root;
    vector<int> idom, tin, tout;

    DominatorTree(int r, vector<int> dom) : root(r), idom(std::move(dom)) {
        int n = int(idom.size());
        auto parent = [&](int v) { return 0 <= idom[v] && idom[v] < n ? idom[v] : -1; };
        assert(0 <= r && r < n && idom[r] == -1 && std::ranges::all_of(idom, [&](int p) { return -1 <= p && p < n; }));
        tin.assign(n, -1); tout.assign(n, -1);
        if (r < 0 || r >= n) { return; }
        vector<int> start(n + 1), kids(n), st{r};
        for (int v = 0; v < n; ++v) {
            if (parent(v) != -1) { ++start[parent(v) + 1]; }}
        for (int v = 0; v < n; ++v) { start[v + 1] += start[v]; }
        vector<int> pos(start.begin(), start.end() - 1);
        for (int v = 0; v < n; ++v) {
            if (parent(v) != -1) { kids[pos[parent(v)]++] = v; }}
        int timer = 0;
        while (!st.empty()) {
            int v = st.back();
            if (tin[v] == -1) {
                tin[v] = timer++;
                for (int i = start[v]; i < start[v + 1]; ++i) { st.push_back(kids[i]); }
                continue;}
            st.pop_back(); tout[v] = timer;}}

    int immediateDominator(int v) const {
        assert(0 <= v && v < int(idom.size())); return idom[v];}
    bool dominates(int u, int v) const {
        assert(0 <= u && u < int(idom.size()) && 0 <= v && v < int(idom.size()));
        return tin[v] != -1 && tin[u] != -1 && tin[u] <= tin[v] && tin[v] < tout[u];}
    // T: O(V + E + out), M: O(V + out); g is the graph the tree was built from; lists in no particular order.
    template<class G>
    vector<vector<int>> dominanceFrontier(const G &g) const {
        assert(g.directed && g.n == int(idom.size()));
        vector<vector<int>> res(g.n);
        vector<int> start(g.n + 1), pred(g.arcs.size()), stamp(g.n, -1);
        for (const auto &a : g.arcs) { ++start[a.to + 1]; }
        for (int v = 0; v < g.n; ++v) { start[v + 1] += start[v]; }
        vector<int> pos(start.begin(), start.end() - 1);
        for (const auto &a : g.arcs) { pred[pos[a.to]++] = a.from; }
        // Walks for one w run consecutively, so a stamped x means the rest of its idom chain is already recorded.
        for (int w = 0; w < g.n; ++w) {
            for (int i = start[w]; i < start[w + 1]; ++i) {
                if (tin[pred[i]] == -1) { continue; }
                for (int x = pred[i]; x != idom[w] && stamp[x] != w; x = idom[x]) { res[x].push_back(w); stamp[x] = w; }}}
        return res;}
};

// T: O((V + E) * alpha(E, V)), M: O(V + E); Lengauer-Tarjan with balanced linking on a directed Graph/CsrGraph.
template<class G>
DominatorTree dominatorTree(const G &g, int root) {
    assert(g.directed && 0 <= root && root < g.n);
    int n = g.n;
    vector<int> num(n), vert{-1, root}, par{0, 0}, st{root}, it(n);
    num[root] = 1;
    while (!st.empty()) {
        int u = st.back();
        if (it[u] == int(g[u].size())) { st.pop_back(); continue; }
        int v = g.arcs[g[u][it[u]++]].to;
        if (num[v] == 0) { num[v] = int(vert.size()); vert.push_back(v); par.push_back(num[u]); st.push_back(v); }}
    int k = int(vert.size()) - 1;
    vector<int> start(k + 2), pred;
    for (const auto &a : g.arcs) {
        if (num[a.from] && num[a.to]) { ++start[num[a.to] + 1]; }}
    for (int w = 0; w <= k; ++w) { start[w + 1] += start[w]; }
    pred.resize(start[k + 1]);
    vector<int> pos(start.begin(), start.end() - 1);
    for (const auto &a : g.arcs) {
        if (num[a.from] && num[a.to]) { pred[pos[num[a.to]]++] = num[a.from]; }}
    vector<int> semi(k + 1), label(k + 1), anc(k + 1), child(k + 1), sz(k + 1, 1), dom(k + 1), head(k + 1, 0), nxt(k + 1);
    iota(semi.begin(), semi.end(), 0); iota(label.begin(), label.end(), 0);
    sz[0] = 0; st.clear();
    // Vertex 0 is the forest sentinel: semi 0, size 0; numbers are DFS preorder from 1.
    auto eval = [&](int v) {
        if (anc[v] == 0) { return label[v]; }
        for (int x = v; anc[anc[x]] != 0; x = anc[x]) { st.push_back(x); }
        while (!st.empty()) {
            int x = st.back(); st.pop_back();
            if (semi[label[anc[x]]] < semi[label[x]]) { label[x] = label[anc[x]]; }
            anc[x] = anc[anc[x]];}
        return semi[label[anc[v]]] >= semi[label[v]] ? label[v] : label[anc[v]];};
    auto link = [&](int v, int w) {
        int s = w;
        while (semi[label[w]] < semi[label[child[s]]]) {
            if (sz[s] + sz[child[child[s]]] >= 2 * sz[child[s]]) { anc[child[s]] = s; child[s] = child[child[s]]; }
            else { sz[child[s]] = sz[s]; s = anc[s] = child[s]; }}
        label[s] = label[w]; sz[v] += sz[w];
        if (sz[v] < 2 * sz[w]) { swap(s, child[v]); }
        for (; s != 0; s = child[s]) { anc[s] = v; }};
    for (int w = k; w >= 2; --w) {
        for (int i = start[w]; i < start[w + 1]; ++i) { semi[w] = min(semi[w], semi[eval(pred[i])]); }
        nxt[w] = head[semi[w]]; head[semi[w]] = w;
        link(par[w], w);
        for (int v = head[par[w]]; v != 0; v = nxt[v]) {
            int u = eval(v);
            dom[v] = semi[u] < semi[v] ? u : par[w];}
        head[par[w]] = 0;}
    vector<int> idom(n, -1);
    for (int w = 2; w <= k; ++w) {
        if (dom[w] != semi[w]) { dom[w] = dom[dom[w]]; }
        idom[vert[w]] = vert[dom[w]];}
    return DominatorTree(root, std::move(idom));}

// T: O(V * (V + E)), M: O(V + E); vertex deletion: d strictly dominates w iff deleting d cuts w off; idom has fewest dominatees.
template<class G>
DominatorTree dominatorTreeSimple(const G &g, int root) {
    assert(g.directed && 0 <= root && root < g.n);
    int n = g.n;
    vector<int> seen(n, -1), best(n, -1), count(n), queue;
    auto bfs = [&](int skip, int mark) {
        queue.assign(1, root); seen[root] = mark;
        for (int i = 0; i < int(queue.size()); ++i) {
            for (int a : g[queue[i]]) {
                int v = g.arcs[a].to;
                if (v != skip && seen[v] != mark) { seen[v] = mark; queue.push_back(v); }}}};
    bfs(-1, n);
    vector<int> reach(queue);
    for (int d : reach) {
        if (d == root) { continue; }
        bfs(d, d);
        for (int w : reach) { count[d] += w != d && seen[w] != d; }
        for (int w : reach) {
            if (w != d && seen[w] != d && (best[w] == -1 || count[d] < count[best[w]])) { best[w] = d; }}}
    vector<int> idom(n, -1);
    for (int w : reach) {
        if (w != root) { idom[w] = best[w] == -1 ? root : best[w]; }}
    return DominatorTree(root, std::move(idom));}
