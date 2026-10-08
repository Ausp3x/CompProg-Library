#pragma once
#include "../01-Core/01-template.hpp"

// S: O(k + L * log(S + 2)) sparse, O(k + L * log(S + 2) + V * S) dense, U: O(m * log(S + 2)) add before build, Q: O(n * log(S + 2)) sparse scan, O(n) dense scan, M: O(V + k) sparse, O(V * (S + 1) + k) dense.
// Bytes 0..255, empty and duplicate patterns allowed; L total pattern length, V states, k IDs, S used bytes; L, V, k, n < INT_MAX.
struct AhoCorasick {
    struct Node {
        map<unsigned char, int> next;
        vector<int> ids;
        int link = 0, out = -1, count = 0;
    };
    vector<Node> nodes{Node{}};
    vector<int> terminal, length, order, go;
    array<int, 256> code;
    int sigma = 0;
    bool built = false, dense = false;

    AhoCorasick() { code.fill(-1); }
    explicit AhoCorasick(const vector<string> &patterns, bool dense_ = false) : AhoCorasick() {
        for (const auto &s : patterns) { add(s); }
        build(dense_);}
    // T: O(k + min(D * log(S + 2), V * S)) sparse, D = sum of leaf depths, M: as the struct (peak O(V * S) when D > V * S); tree rooted at 0, distinct sibling labels, ends[i] gets ID i.
    static AhoCorasick fromTrie(const vector<int> &parent, string_view label, const vector<int> &ends = {}, bool dense_ = false) {
        int n = int(parent.size());
        assert(1 <= n && parent.size() < INT_MAX && label.size() == parent.size() && ends.size() < INT_MAX);
        AhoCorasick ac;
        ac.nodes.resize(n);
        for (int v = 1; v < n; ++v) {
            assert(0 <= parent[v] && parent[v] < n);
            unsigned char c = label[v];
            ac.code[c] = 0;
            [[maybe_unused]] bool fresh = ac.nodes[parent[v]].next.emplace(c, v).second;
            assert(fresh);}
        vector<int> depth(n), queue{0};
        lng leaf_depths = 0;
        for (int i = 0; i < int(queue.size()); ++i) {
            int u = queue[i];
            leaf_depths += ac.nodes[u].next.empty() ? depth[u] : 0;
            for (auto [c, v] : ac.nodes[u].next) { depth[v] = depth[u] + 1; queue.push_back(v); }}
        assert(int(queue.size()) == n);
        for (int u : ends) {
            assert(0 <= u && u < n);
            ac.nodes[u].ids.push_back(ac.patterns());
            ac.terminal.push_back(u);
            ac.length.push_back(depth[u]);}
        // Sparse failure walks cost up to D; past V * S, links come from a temporary dense table.
        ac.build(dense_ || leaf_depths > lng(n) * std::ranges::count(ac.code, 0));
        if (ac.dense && !dense_) { ac.dense = false; vector<int>().swap(ac.go); }
        return ac;}
    void clear() {
        nodes.clear();
        nodes.emplace_back();
        terminal.clear(); length.clear(); order.clear(); go.clear();
        code.fill(-1);
        sigma = 0;
        built = dense = false;}
    int size() const { return int(nodes.size()); }
    int patterns() const { return int(terminal.size()); }

    int add(string_view s) {
        assert(!built && s.size() < INT_MAX && terminal.size() < INT_MAX - 1);
        int u = 0;
        for (unsigned char c : s) {
            code[c] = 0;
            auto it = nodes[u].next.find(c);
            if (it != nodes[u].next.end()) { u = it->second; continue; }
            assert(nodes.size() < INT_MAX - 1);
            nodes[u].next.emplace(c, size());
            u = size();
            nodes.emplace_back();}
        nodes[u].ids.push_back(patterns());
        terminal.push_back(u);
        length.push_back(int(s.size()));
        return patterns() - 1;}
    int walk(int u, unsigned char c) const {
        for (;; u = nodes[u].link) {
            auto it = nodes[u].next.find(c);
            if (it != nodes[u].next.end()) { return it->second; }
            if (!u) { return 0; }}}
    int next(int u, unsigned char c) const {
        if (code[c] < 0) { return 0; }
        return dense ? go[size_t(u) * sigma + code[c]] : walk(u, c);}
    void build(bool dense_ = false) {
        dense = dense_;
        sigma = 0;
        for (int &c : code) {
            if (c >= 0) { c = sigma++; }}
        if (dense) { go.assign(size_t(size()) * sigma, 0); }
        else { vector<int>().swap(go); }
        order = {0};
        nodes[0].link = 0;
        nodes[0].out = -1;
        for (int i = 0; i < int(order.size()); ++i) {
            int u = order[i], f = nodes[u].link;
            nodes[u].count = int(nodes[u].ids.size()) + (u ? nodes[f].count : 0);
            if (u) { nodes[u].out = nodes[f].ids.empty() ? nodes[f].out : f; }
            if (dense) {
                if (u) { std::copy_n(go.begin() + size_t(f) * sigma, sigma, go.begin() + size_t(u) * sigma); }
                for (auto [c, v] : nodes[u].next) { go[size_t(u) * sigma + code[c]] = v; }}
            for (auto [c, v] : nodes[u].next) {
                nodes[v].link = !u ? 0 : dense ? go[size_t(f) * sigma + code[c]] : walk(f, c);
                order.push_back(v);}}
        built = true;}

    void checkState([[maybe_unused]] int u) const { assert(built && 0 <= u && u < size()); }
    // T: O((h + 1) * log(S + 2)) sparseStep and sparse step, O(1) dense step, M: O(1); h = depth of u, a scan amortizes to the struct Q.
    int sparseStep(int u, unsigned char c) const { checkState(u); return walk(u, c); }
    int step(int u, unsigned char c) const { checkState(u); return next(u, c); }
    int matchCount(int u) const { checkState(u); return nodes[u].count; }
    bool forbidden(int u) const { return matchCount(u) != 0; }
    int safeStep(int u, unsigned char c) const {
        if (u == -1) { assert(built); return -1; }
        if (forbidden(u)) { return -1; }
        u = next(u, c);
        return nodes[u].count ? -1 : u;}
    bool avoids(string_view text) const {
        checkState(0);
        assert(text.size() < INT_MAX);
        int u = 0;
        if (nodes[u].count) { return false; }
        for (unsigned char c : text) {
            u = next(u, c);
            if (nodes[u].count) { return false; }}
        return true;}

    // T: O(V) combine calls, M: O(V) returned; value[u] folds u, then its proper suffix states nearest first, root once.
    template<class T, class F> vector<T> suffixAggregate(vector<T> value, F &&combine) const {
        checkState(0);
        assert(value.size() == nodes.size());
        for (int u : order) {
            if (u) { value[u] = combine(value[u], value[nodes[u].link]); }}
        return value;}
    template<class F> bool outputs(int u, F &&visit) const {
        for (int v = u; v >= 0; v = nodes[v].out) {
            for (int id : nodes[v].ids) {
                if (!visit(id)) { return false; }}}
        return true;}
    // T: O(out + 1), M: O(1); IDs ending at u, longest first, duplicates in ID order; visit(id) -> bool, false cancels.
    template<class F> bool forEachOutput(int u, F &&visit) const { checkState(u); return outputs(u, visit); }
    // T: scan + O(out), M: O(1); visit(id, l, r) -> bool for every match [l,r) by increasing r, then output order.
    template<class F> bool forEachMatch(string_view text, F &&visit) const {
        checkState(0);
        assert(text.size() < INT_MAX);
        for (int u = 0, r = 0;; u = next(u, uint8_t(text[r++]))) {
            if (!outputs(u, [&](int id) { return visit(id, r - length[id], r); })) { return false; }
            if (r == int(text.size())) { return true; }}}
    // T: scan + O(V), M: O(V) returned; occurrences of every state's string, root n + 1.
    vector<lng> stateCounts(string_view text) const {
        checkState(0);
        assert(text.size() < INT_MAX);
        vector<lng> res(size());
        res[0] = 1;
        int u = 0;
        for (unsigned char c : text) { ++res[u = next(u, c)]; }
        for (int i = size() - 1; i > 0; --i) { res[nodes[order[i]].link] += res[order[i]]; }
        return res;}
    // T: scan + O(V + k), M: O(V + k); res[id] counts occurrences of each ID separately.
    vector<lng> countPatterns(string_view text) const {
        auto count = stateCounts(text);
        vector<lng> res;
        res.reserve(patterns());
        for (int u : terminal) { res.push_back(count[u]); }
        return res;}
    // T: scan, M: O(n) returned; res[r] counts IDs ending at boundary r, 0 <= r <= n.
    vector<int> countPositions(string_view text) const {
        checkState(0);
        assert(text.size() < INT_MAX);
        vector<int> res{nodes[0].count};
        res.reserve(text.size() + 1);
        int u = 0;
        for (unsigned char c : text) { res.push_back(nodes[u = next(u, c)].count); }
        return res;}
    lng countMatches(string_view text) const {
        checkState(0);
        assert(text.size() < INT_MAX);
        lng res = nodes[0].count;
        int u = 0;
        for (unsigned char c : text) { res += nodes[u = next(u, c)].count; }
        return res;}
};
