#pragma once
#include "../01-Core/01-template.hpp"
#include "07-suffixarray.hpp"

// S: O(n) from a moved SuffixArray, O(n * log(n + 1)) from a text, U: NA, Q: O(1) access, O(log(S + 1)) child/step, O(m * log(S + 1)) locate/isSubstring, O(log(n + 1)) weightedAncestor/suffixLink, M: O(n) (O(n * log(n + 1)) if index keeps RMQ).
// Integral T (string input: bytes as T = int), n < 2^30, sentinel-free; ids in lexicographic pre-order, root 0; locus {node, length}, {-1, 0} when step fails.
template<class T = int> requires std::is_integral_v<T>
struct SuffixTree {
    struct Node { int l, r, depth, parent, end; };
    SuffixArray<T> index;
    vector<Node> nodes;
    vector<int> start, kids, leaf, jump;

    SuffixTree() : SuffixTree(SuffixArray<T>()) {}
    explicit SuffixTree(SuffixArray<T> sa) { build(std::move(sa)); }
    explicit SuffixTree(vector<T> s) : SuffixTree(SuffixArray<T>(std::move(s), false)) {}
    explicit SuffixTree(string_view s) requires std::is_same_v<T, int> : SuffixTree(SuffixArray<int>(s, false)) {}

    void build(SuffixArray<T> from) {
        index = std::move(from);
        int n = index.size();
        assert(n < (1 << 30));
        const vector<int> &sa = index.sa, &lcp = index.lcp;
        vector<Node> raw{{0, n, 0, -1, 0}};
        vector<int> st{0};
        leaf.assign(n, 0);
        // Bottom-up LCP-interval scan; a suffix that is a prefix of the next one ends on the internal node of depth h.
        for (int k = 0; k < n; ++k) {
            int d = n - sa[k], h = k + 1 < n ? lcp[k] : 0, last = -1;
            if (d > h) {
                last = int(raw.size());
                leaf[sa[k]] = last;
                raw.push_back({k, k + 1, d, -1, 0});}
            while (raw[st.back()].depth > h) {
                int v = st.back();
                st.pop_back();
                raw[v].r = k + 1;
                raw[last].parent = v;
                last = v;}
            if (raw[st.back()].depth < h) {
                int v = int(raw.size());
                raw.push_back({last < 0 ? k : raw[last].l, 0, h, -1, 0});
                if (last < 0) { leaf[sa[k]] = v; }
                else { raw[last].parent = v; }
                st.push_back(v);}
            else { raw[last].parent = st.back(); }}
        int cnt = int(raw.size());
        vector<int> id(cnt), pos(n + 1);
        // Non-root nodes sharing l are created deepest first, so a reversed counting sort by l yields (l, depth) order.
        pos[0] = 1;
        for (int v = 1; v < cnt; ++v) { ++pos[raw[v].l + 1]; }
        for (int l = 1; l <= n; ++l) { pos[l] += pos[l - 1]; }
        for (int v = cnt - 1; v > 0; --v) { id[v] = pos[raw[v].l]++; }
        nodes.resize(cnt);
        for (int v = 0; v < cnt; ++v) {
            nodes[id[v]] = raw[v];
            if (v) { nodes[id[v]].parent = id[raw[v].parent]; }}
        for (int &u : leaf) { u = id[u]; }
        start.assign(cnt + 1, 0);
        kids.resize(cnt - 1);
        for (int v = 1; v < cnt; ++v) { ++start[nodes[v].parent]; }
        for (int v = 1; v <= cnt; ++v) { start[v] += start[v - 1]; }
        for (int v = cnt - 1; v > 0; --v) { kids[--start[nodes[v].parent]] = v; }
        for (int v = cnt - 1; v >= 0; --v) { nodes[v].end = start[v] == start[v + 1] ? v + 1 : nodes[kids[start[v + 1] - 1]].end; }
        jump.assign(cnt, 0);
        vector<int> level(cnt);
        for (int v = 1; v < cnt; ++v) {
            int p = nodes[v].parent, j = jump[p];
            level[v] = level[p] + 1;
            jump[v] = level[p] - level[j] == level[j] - level[jump[j]] ? jump[j] : p;}}

    int root() const { return 0; }
    int depth(int v) const {
        assert(0 <= v && v < int(nodes.size()));
        return nodes[v].depth;}
    std::span<const int> children(int v) const {
        assert(0 <= v && v < int(nodes.size()));
        return {kids.data() + start[v], kids.data() + start[v + 1]};}
    int child(int v, T c) const {
        assert(0 <= v && v < int(nodes.size()));
        int d = nodes[v].depth;
        auto at = [&](int u) { return index.text[index.sa[nodes[u].l] + d]; };
        auto it = lower_bound(kids.begin() + start[v], kids.begin() + start[v + 1], c, [&](int u, T x) { return at(u) < x; });
        return it != kids.begin() + start[v + 1] && at(*it) == c ? *it : -1;}
    pair<int, int> edgeLabel(int v) const {
        assert(0 < v && v < int(nodes.size()));
        int i = index.sa[nodes[v].l];
        return {i + nodes[nodes[v].parent].depth, i + nodes[v].depth};}
    int leafSuffix(int v) const {
        assert(0 <= v && v < int(nodes.size()));
        if (!v) { return -1; }
        int i = index.sa[nodes[v].l];
        return leaf[i] == v ? i : -1;}

    pair<int, int> step(pair<int, int> at, T c) const {
        auto [v, k] = at;
        assert(0 <= v && v < int(nodes.size()) && k <= nodes[v].depth && (v ? k > nodes[nodes[v].parent].depth : !k));
        if (k < nodes[v].depth) { return index.text[index.sa[nodes[v].l] + k] == c ? pair{v, k + 1} : pair{-1, 0}; }
        int u = child(v, c);
        return u < 0 ? pair{-1, 0} : pair{u, k + 1};}
    pair<int, int> locate(const vector<T> &p) const {
        assert(p.size() < INT_MAX);
        pair<int, int> at{0, 0};
        for (T c : p) {
            auto nx = step(at, c);
            if (nx.first < 0) { break; }
            at = nx;}
        return at;}
    pair<int, int> locate(string_view p) const requires std::is_same_v<T, int> {
        vector<int> q(p.begin(), p.end());
        for (int &c : q) { c = uint8_t(c); }
        return locate(q);}
    bool isSubstring(const vector<T> &p) const { return locate(p).second == int(p.size()); }
    bool isSubstring(string_view p) const requires std::is_same_v<T, int> { return locate(p).second == int(p.size()); }
    // T: O(out * log(out + 1)), M: O(out); starts of the node's string, ascending.
    vector<int> occurrences(int v) const {
        assert(0 <= v && v < int(nodes.size()));
        vector<int> res(index.sa.begin() + nodes[v].l, index.sa.begin() + nodes[v].r);
        sort(res.begin(), res.end());
        return res;}
    int weightedAncestor(int v, int len) const {
        assert(0 <= v && v < int(nodes.size()) && 0 <= len && len <= nodes[v].depth);
        while (v && nodes[nodes[v].parent].depth >= len) { v = nodes[jump[v]].depth >= len ? jump[v] : nodes[v].parent; }
        return v;}
    int suffixLink(int v) const {
        assert(0 < v && v < int(nodes.size()));
        int d = nodes[v].depth;
        return d == 1 ? 0 : weightedAncestor(leaf[index.sa[nodes[v].l] + 1], d - 1);}

    // T: O(n), M: O(n); left- and right-maximal repeats (Gusfield), ascending ids.
    vector<int> maximalRepeats() const {
        int n = index.size();
        const vector<int> &sa = index.sa;
        const vector<T> &s = index.text;
        vector<int> diverse(n);
        for (int k = 0; k + 1 < n; ++k) { diverse[k + 1] = diverse[k] + (!sa[k] || !sa[k + 1] || s[sa[k] - 1] != s[sa[k + 1] - 1]); }
        vector<int> res;
        for (int v = 1; v < int(nodes.size()); ++v) {
            if (diverse[nodes[v].r - 1] > diverse[nodes[v].l]) { res.push_back(v); }}
        return res;}
    // T: O(n), M: O(n); repeats none of whose one-symbol extensions repeats, ascending ids.
    vector<int> supermaximalRepeats() const {
        int n = index.size();
        const vector<int> &sa = index.sa, &rank = index.rank, &lcp = index.lcp;
        vector<int> res;
        for (int v = 1; v < int(nodes.size()); ++v) {
            bool ok = nodes[v].r - nodes[v].l >= 2;
            for (int u : children(v)) { ok = ok && nodes[u].r - nodes[u].l == 1; }
            // Leaf-only nodes have disjoint intervals; b w repeats exactly when the suffix starting with it shares more than depth symbols with a neighbour.
            for (int k = nodes[v].l; ok && k < nodes[v].r; ++k) {
                int x = sa[k] ? rank[sa[k] - 1] : -1;
                ok = x < 0 || max(x ? lcp[x - 1] : 0, x + 1 < n ? lcp[x] : 0) <= nodes[v].depth;}
            if (ok) { res.push_back(v); }}
        return res;}
    // T: O(n), M: O(1).
    lng distinctSubstrings() const {
        lng res = 0;
        for (int v = 1; v < int(nodes.size()); ++v) { res += nodes[v].depth - nodes[nodes[v].parent].depth; }
        return res;}
};
SuffixTree(string_view) -> SuffixTree<int>;
SuffixTree(const string &) -> SuffixTree<int>;
SuffixTree(const char *) -> SuffixTree<int>;
