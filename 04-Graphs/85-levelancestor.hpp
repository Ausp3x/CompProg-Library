#pragma once

#include "../01-Core/01-template.hpp"
#include "07-lca.hpp"

// Same forest contract as LCA; query(u, k) is the k-th ancestor, -1 when k > depth; l is the leaf count.
// S: O(n + l * log(n)), Q: O(1), M: O(n + l * log(n)).
struct LevelAncestor : lca_detail::Forest {
    vector<int> bottom, pos, jump_start, ladder, jumps;

    template<class G>
    explicit LevelAncestor(const G &g, int root = -1) : Forest(g, root), bottom(n), pos(n), jump_start(n) {
        vector<int> deep(n, -1), height(n, 1), path(n);
        for (int u : postorder) {
            int p = parent[u];
            if (p != -1 && height[u] + 1 > height[p]) { height[p] = height[u] + 1; deep[p] = u; }}
        ladder.reserve(2 * n);
        for (int u : order) {
            int d = dep[u]; path[d] = u;
            if (deep[u] != -1) { continue; }
            int len = 1, s = int(ladder.size());
            while (len <= d && deep[path[d - len]] == path[d - len + 1]) { ++len; }
            for (int i = 0; i < len + min(len, d - len + 1); ++i) {
                int x = path[d - i]; ladder.push_back(x);
                if (i < len) { pos[x] = s + i; bottom[x] = u; }}
            jump_start[u] = int(jumps.size());
            for (int h = 0; h < int(std::bit_width(uint(d))); ++h) { jumps.push_back(path[d - (1 << h)]); }}
        for (int &x : jumps) { x = pos[x]; }}

    int query(int u, lng k) const {
        check(u); assert(k >= 0);
        if (k < 0 || k > dep[u]) { return -1; }
        int b = bottom[u], d = int(k) + dep[b] - dep[u];
        if (d == 0) { return u; }
        int h = int(std::bit_width(uint(d))) - 1;
        return ladder[jumps[jump_start[b] + h] + d - (1 << h)];}
};

// T: O(n + q), M: O(n + q); queries (u, k) as LevelAncestor::query, answers in query order.
template<class G>
vector<int> offlineLevelAncestor(const G &g, const vector<pair<int, lng>> &queries, int root = -1) {
    assert(queries.size() <= size_t(INT_MAX)); lca_detail::Forest f(g, root);
    int q = int(queries.size());
    vector<int> res(q, -1), head(g.n + 1), ids(q), path(g.n);
    for (auto [u, k] : queries) { f.check(u); assert(k >= 0); ++head[u]; }
    for (int u = 0; u < g.n; ++u) { head[u + 1] += head[u]; }
    for (int i = q - 1; i >= 0; --i) { ids[--head[queries[i].first]] = i; }
    for (int u : f.order) {
        path[f.dep[u]] = u;
        for (int j = head[u]; j < head[u + 1]; ++j) {
            lng k = queries[ids[j]].second;
            if (0 <= k && k <= f.dep[u]) { res[ids[j]] = path[f.dep[u] - k]; }}}
    return res;}
