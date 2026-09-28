#pragma once

#include "../01-Core/01-template.hpp"
#include "01-graph.hpp"
#include "../02-Data Structures/01-dsu.hpp"

namespace lca_detail {
    // Iterative DFS metadata; inclusive legacy [t_in,t_out] subtree intervals.
    struct Forest {
        int n, root, timer = 0;
        vector<int> parent, parent_edge, component, dep, t_in, t_out, order, postorder;
        vector<lll> weight;

        template<class G>
        explicit Forest(const G &g, int r) : n(g.n), root(r), parent(n, -1), parent_edge(n, -1),
            component(n, -1), dep(n), t_in(n), t_out(n), weight(n) {
            assert(!g.directed && -1 <= r && r < n);
            if (r == -1 && n) { root = 0; }
            vector<pair<int, int>> stack;
            auto visit = [&](int u, int p, int id, int c, lng w) {
                parent[u] = p; parent_edge[u] = id; component[u] = c;
                if (p != -1) { dep[u] = dep[p] + 1; weight[u] = weight[p] + lll(w); }
                t_in[u] = timer++; order.push_back(u); stack.push_back({u, 0}); };
            auto traverse = [&](int s) {
                visit(s, -1, -1, s, 0);
                while (!stack.empty()) {
                    auto &[u, i] = stack.back();
                    if (i == int(g[u].size())) {
                        t_out[u] = timer - 1; postorder.push_back(u); stack.pop_back(); continue; }
                    auto e = g.arcs[g[u][i++]];
                    if (e.id == parent_edge[u]) { continue; }
                    assert(component[e.to] == -1);
                    visit(e.to, u, e.id, s, e.w); } };
            if (root != -1) { traverse(root); }
            for (int u = 0; u < n; ++u) { if (component[u] == -1) { traverse(u); } } }

        void check(int u) const { assert(0 <= u && u < n); (void)u; }
        bool isAncestor(int u, int v) const {
            check(u); check(v);
            return component[u] == component[v] && t_in[u] <= t_in[v] && t_out[v] <= t_out[u]; }
    };

    // Legacy adjacency lists are simple symmetric forests, sized n or n+1.
    inline Graph fromAdjacency(int n, const vector<vector<int>> &adj) {
        assert(n >= 0 && adj.size() <= size_t(INT_MAX));
        assert(adj.size() == size_t(n) || adj.size() == size_t(n) + 1);
        Graph g(int(adj.size())); vector<pair<int, int>> arcs;
        for (int u = 0; u < g.n; ++u) { for (int v : adj[u]) {
            assert(0 <= v && v < g.n && u != v); arcs.push_back({u, v}); } }
        sort(arcs.begin(), arcs.end());
        assert(std::adjacent_find(arcs.begin(), arcs.end()) == arcs.end());
        for (auto [u, v] : arcs) {
            assert(binary_search(arcs.begin(), arcs.end(), pair{v, u}));
            if (u < v) { g.addEdge(u, v); } }
        return g; }
} // namespace lca_detail

// Immutable owning preprocessing of a Graph/CsrGraph undirected forest. No loops,
// parallel edges or cycles. Vertices [0,n); weights span lng. root=-1 selects each
// component's minimum vertex; an explicit root roots its component first.
// Missing LCA/ancestor/jump/distance is -1; distance counts edges. k is nonnegative.
// weightedDistance writes the exact signed path sum to out only if connected.
// Every self path has zero edges/weight; rerootedLCA requires all 3 vertices connected.
// Public state is read-only; input changes do not affect this snapshot.
// S: O(n * log(n)), Q: O(log(n)), M: O(n * log(n)); isAncestor is O(1).
struct LCA : lca_detail::Forest {
    int l;
    vector<vector<int>> up;

    template<class G>
    explicit LCA(const G &g, int root = -1) : Forest(g, root), l(int(std::bit_width(uint(max(1, n)))) - 1),
        up(n, vector<int>(l + 1)) {
        for (int u : order) {
            up[u][0] = parent[u] == -1 ? u : parent[u];
            for (int j = 1; j <= l; ++j) { up[u][j] = up[up[u][j - 1]][j - 1]; } } }
    // Compatibility: n+1 lists retain one-based vertex labels (0 also exists).
    // n metadata becomes the actual adjacency size. Historical init is superseded
    // by assigning a newly constructed LCA. Nonpositive getKthAncestor is identity.
    LCA(int n, int root, const vector<vector<int>> &adj) : LCA(lca_detail::fromAdjacency(n, adj), root) {}

    int kthAncestor(int u, lng k) const {
        check(u); assert(k >= 0);
        if (k > dep[u]) { return -1; }
        for (int j = 0; k; ++j, k >>= 1) { if (k & 1) { u = up[u][j]; } }
        return u; }
    int getKthAncestor(int u, int k) const { return kthAncestor(u, max(0, k)); }
    int getLCA(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        if (isAncestor(u, v)) { return u; }
        if (isAncestor(v, u)) { return v; }
        for (int j = l; j >= 0; --j) { if (!isAncestor(up[u][j], v)) { u = up[u][j]; } }
        return parent[u]; }
    int distance(int u, int v) const {
        int a = getLCA(u, v); return a == -1 ? -1 : (dep[u] - dep[a]) + (dep[v] - dep[a]); }
    bool weightedDistance(int u, int v, lll &out) const {
        int a = getLCA(u, v);
        if (a == -1) { return false; }
        out = weight[u] + weight[v] - 2 * weight[a]; return true; }
    int jump(int u, int v, lng k) const {
        assert(k >= 0); int a = getLCA(u, v);
        if (a == -1) { return -1; }
        int left = dep[u] - dep[a], length = left + dep[v] - dep[a];
        return k > length ? -1 : k <= left ? kthAncestor(u, k) : kthAncestor(v, length - k); }
    int rerootedLCA(int u, int v, int r) const {
        check(r); int a = getLCA(u, v);
        if (a == -1 || component[u] != component[r]) { return -1; }
        int b = getLCA(u, r), c = getLCA(v, r);
        return dep[a] >= dep[b] && dep[a] >= dep[c] ? a : dep[b] >= dep[c] ? b : c; }
};

// Same forest/query contract as LCA; 2*n+1 <= INT_MAX. Farach-Colton--Bender
// Euler RMQ uses a virtual root (-1) to keep adjacent tour depths differing by 1.
// Binary lifting owns kthAncestor/path jump; use LCA when these are required.
// S: O(n), Q: O(1), M: O(n).
struct EulerLCA : lca_detail::Forest {
    int block_size;
    vector<int> first, tour, mask;
    vector<vector<int>> table;
    vector<vector<uint8_t>> micro;

    template<class G>
    explicit EulerLCA(const G &g, int root = -1) : Forest(g, root), first(n) {
        assert(n <= (INT_MAX - 1) / 2); tour.reserve(2 * n + 1); tour.push_back(-1);
        int cur = -1;
        for (int u : order) {
            while (cur != parent[u]) { cur = parent[cur]; tour.push_back(cur); }
            first[u] = int(tour.size()); tour.push_back(u); cur = u; }
        while (cur != -1) { cur = parent[cur]; tour.push_back(cur); }
        int m = int(tour.size()); block_size = max(1, (int(std::bit_width(uint(m))) - 1) / 2);
        int count = (m - 1) / block_size + 1;
        mask.assign(count, 0); table.push_back(vector<int>(count));
        micro.resize(1 << (block_size - 1));
        for (int b = 0; b < count; ++b) {
            int start = b * block_size, length = min(block_size, m - start), best = start;
            for (int j = 1; j < length; ++j) {
                if (height(start + j) > height(start + j - 1)) { mask[b] |= 1 << (j - 1); }
                best = better(best, start + j); }
            table[0][b] = best; auto &local = micro[mask[b]];
            if (!local.empty()) { continue; }
            local.resize(block_size * block_size); vector<int> h(block_size);
            for (int j = 1; j < block_size; ++j) { h[j] = h[j - 1] + ((mask[b] >> (j - 1) & 1) ? 1 : -1); }
            for (int l = 0; l < block_size; ++l) {
                int pos = l;
                for (int r = l; r < block_size; ++r) {
                    if (h[r] < h[pos]) { pos = r; }
                    local[l * block_size + r] = uint8_t(pos); } } }
        for (int j = 1; (1 << j) <= count; ++j) {
            table.push_back(vector<int>(count - (1 << j) + 1));
            for (int i = 0; i < int(table[j].size()); ++i) {
                table[j][i] = better(table[j - 1][i], table[j - 1][i + (1 << (j - 1))]); } } }

    int height(int i) const { return tour[i] == -1 ? -1 : dep[tour[i]]; }
    int better(int i, int j) const { return height(i) <= height(j) ? i : j; }
    int inside(int b, int l, int r) const { return b * block_size + micro[mask[b]][l * block_size + r]; }
    int getLCA(int u, int v) const {
        check(u); check(v);
        if (component[u] != component[v]) { return -1; }
        int l = first[u], r = first[v]; if (l > r) { swap(l, r); }
        int a = l / block_size, b = r / block_size;
        if (a == b) { return tour[inside(a, l % block_size, r % block_size)]; }
        int best = better(inside(a, l % block_size, block_size - 1), inside(b, 0, r % block_size));
        if (a + 1 < b) {
            int j = int(std::bit_width(uint(b - a - 1))) - 1;
            best = better(best, better(table[j][a + 1], table[j][b - (1 << j)])); }
        return tour[best]; }
    int distance(int u, int v) const {
        int a = getLCA(u, v); return a == -1 ? -1 : (dep[u] - dep[a]) + (dep[v] - dep[a]); }
    bool weightedDistance(int u, int v, lll &out) const {
        int a = getLCA(u, v);
        if (a == -1) { return false; }
        out = weight[u] + weight[v] - 2 * weight[a]; return true; }
    int rerootedLCA(int u, int v, int r) const {
        check(r); int a = getLCA(u, v);
        if (a == -1 || component[u] != component[r]) { return -1; }
        int b = getLCA(u, r), c = getLCA(v, r);
        return dep[a] >= dep[b] && dep[a] >= dep[c] ? a : dep[b] >= dep[c] ? b : c; }
};

// Tarjan's offline LCA, same forest/root contract. Answers preserve query order,
// duplicates and self queries; disconnected pairs return -1. q fits int.
// T: O((n + q) * alpha(n)) amortized, M: O(n + q); returned storage O(q).
// alpha is inverse Ackermann. Explicit DFS postorder avoids recursion.
template<class G>
vector<int> offlineLCA(const G &g, const vector<pair<int, int>> &queries, int root = -1) {
    assert(queries.size() <= size_t(INT_MAX)); lca_detail::Forest f(g, root);
    vector<vector<pair<int, int>>> adj(g.n); vector<int> result(queries.size(), -1), ancestor(g.n);
    vector<bool> done(g.n); DSU dsu(g.n); iota(ancestor.begin(), ancestor.end(), 0);
    for (int i = 0; i < int(queries.size()); ++i) {
        auto [u, v] = queries[i]; f.check(u); f.check(v);
        adj[u].push_back({v, i}); adj[v].push_back({u, i}); }
    for (int u : f.postorder) {
        done[u] = true;
        for (auto [v, i] : adj[u]) {
            if (done[v] && f.component[u] == f.component[v]) { result[i] = ancestor[dsu.findSet(v)]; } }
        int p = f.parent[u];
        if (p != -1) { dsu.uniteSets(u, p); ancestor[dsu.findSet(p)] = p; } }
    return result;
}
