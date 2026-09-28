#pragma once
#include "../01-Core/01-template.hpp"

// S: O(k + L * log(sigma + 2)) sparse, O(k + L * log(sigma + 2) + V * sigma) dense;
// Q: O(n * log(sigma + 2)) sparse scan, O(n) dense scan; M: O(V + k) sparse,
// O(V * (sigma + 1) + k) dense. L total inserted length, V trie states, k IDs,
// sigma distinct pattern bytes <= 256. Map insertion and sparse build are included.
// Bytes 0..255, embedded NUL/empty patterns allowed. Each add gets its own ID;
// empty IDs match at all n+1 boundaries. Lengths, IDs and V are < INT_MAX.
// add before build; build may repeat/change representation; clear restarts IDs.
// clear retains vector capacities (memory bounded by prior peaks); build(false)
// releases the dense table. Other rebuilding work never materializes outputs.
// Root is state 0. State IDs survive build, not clear. Pattern/text views are borrowed
// only during a call. Fields/helpers are internal except nodes' link/out and terminal.
// A moved-from object supports clear/assignment before other operations.
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
        for (const auto &s : patterns) { add(s); } build(dense_);
    }
    void clear() {
        nodes.clear(); nodes.emplace_back(); terminal.clear(); length.clear(); order.clear(); go.clear();
        code.fill(-1); sigma = 0; built = dense = false;
    }
    int size() const { return int(nodes.size()); }
    int patterns() const { return int(terminal.size()); }
    int add(string_view s) {
        assert(!built && s.size() < INT_MAX && terminal.size() < INT_MAX - 1); int u = 0;
        for (unsigned char c : s) {
            code[c] = 0; auto it = nodes[u].next.find(c);
            if (it == nodes[u].next.end()) {
                assert(nodes.size() < INT_MAX - 1); int v = size();
                nodes.emplace_back(); nodes[u].next.emplace(c, v); u = v; }
            else { u = it->second; } }
        int id = patterns(); nodes[u].ids.push_back(id); terminal.push_back(u); length.push_back(int(s.size()));
        return id;
    }

    int sparseStep(int u, unsigned char c) const {
        while (true) {
            auto it = nodes[u].next.find(c);
            if (it != nodes[u].next.end()) { return it->second; }
            if (!u) { return 0; } u = nodes[u].link; }
    }
    void build(bool dense_ = false) {
        dense = dense_; sigma = 0;
        for (int &c : code) { if (c >= 0) { c = sigma++; } }
        if (dense) { go.assign(size_t(size()) * sigma, 0); }
        else { vector<int>().swap(go); } order = {0};
        nodes[0].link = 0; nodes[0].out = -1;
        for (int i = 0; i < int(order.size()); ++i) {
            int u = order[i], f = nodes[u].link;
            nodes[u].count = int(nodes[u].ids.size()) + (u ? nodes[f].count : 0);
            if (u) { nodes[u].out = !nodes[f].ids.empty() ? f : nodes[f].out; }
            if (dense) {
                if (u) { std::copy_n(go.begin() + size_t(f) * sigma, sigma, go.begin() + size_t(u) * sigma); }
                for (auto [c, v] : nodes[u].next) { go[size_t(u) * sigma + code[c]] = v; } }
            for (auto [c, v] : nodes[u].next) {
                nodes[v].link = !u ? 0 : dense ? go[size_t(f) * sigma + code[c]] : sparseStep(f, c);
                order.push_back(v); } }
        built = true;
    }
    void checkState(int u) const { assert(built && 0 <= u && u < size()); }
    // Sparse individual step: O((h + 1) * log(sigma + 2)), h max pattern length;
    // a scan from root is amortized O(log(sigma + 2)) per byte. Dense step: O(1).
    int step(int u, unsigned char c) const {
        checkState(u); if (code[c] < 0) { return 0; }
        return dense ? go[size_t(u) * sigma + code[c]] : sparseStep(u, c);
    }
    int matchCount(int u) const { checkState(u); return nodes[u].count; }
    bool forbidden(int u) const { return matchCount(u) != 0; }
    // -1 is a rejecting sink; rejects both already-forbidden and newly-bad states.
    int safeStep(int u, unsigned char c) const {
        if (u == -1) { assert(built); return -1; }
        if (forbidden(u)) { return -1; } u = step(u, c); return forbidden(u) ? -1 : u;
    }
    bool avoids(string_view text) const {
        checkState(0); assert(text.size() < INT_MAX); if (forbidden(0)) { return false; }
        int u = 0; for (unsigned char c : text) { u = safeStep(u, c); if (u < 0) { return false; } }
        return true;
    }

    // T: O(V) combine calls, M: O(V) returned values. value[u] folds its own value
    // then nearest-to-farthest proper suffix states, including root once. combine
    // must be associative; its arithmetic/lifetime constraints belong to the caller.
    template<class T, class F> vector<T> suffixAggregate(vector<T> value, F &&combine) const {
        checkState(0); assert(value.size() == nodes.size());
        for (int u : order) { if (u) { value[u] = combine(value[u], value[nodes[u].link]); } }
        return value;
    }
    // O(number of reported IDs + 1), longest pattern first, duplicates in ID order.
    // Callback(id)->bool; false cancels, even on the last match. Do not mutate this
    // automaton from callbacks; nested const queries are allowed.
    template<class F> bool forEachOutput(int u, F &&visit) const {
        checkState(u);
        for (int v = u; v >= 0; v = nodes[v].out) {
            for (int id : nodes[v].ids) { if (!visit(id)) { return false; } } }
        return true;
    }
    // Adds O(z) callback time to the scan, O(1) workspace, no stored occurrence list.
    // Callback(id,l,r)->bool, half-open match [l,r), increasing end boundary r.
    template<class F> bool forEachMatch(string_view text, F &&visit) const {
        checkState(0); assert(text.size() < INT_MAX); int u = 0, r = 0;
        while (true) {
            if (!forEachOutput(u, [&](int id) { return visit(id, r - length[id], r); })) { return false; }
            if (r == int(text.size())) { return true; } u = step(u, static_cast<unsigned char>(text[r++])); }
    }
    // Scan + O(V), M: O(V) returned counts. Counts every trie-prefix occurrence;
    // root occurs n+1 times. Counts are exact lng; no outputs are materialized.
    vector<lng> stateCounts(string_view text) const {
        checkState(0); assert(text.size() < INT_MAX); vector<lng> count(size()); count[0] = 1; int u = 0;
        for (unsigned char c : text) { u = step(u, c); ++count[u]; }
        for (int i = size() - 1; i > 0; --i) { int v = order[i]; count[nodes[v].link] += count[v]; }
        return count;
    }
    // Scan + O(V + k), M: O(V + k) workspace/result; duplicate IDs remain separate.
    vector<lng> countPatterns(string_view text) const {
        auto count = stateCounts(text); vector<lng> answer; answer.reserve(patterns());
        for (int u : terminal) { answer.push_back(count[u]); } return answer;
    }
    // M: O(n) result; answer[r] counts IDs ending at boundary r (0 <= r <= n).
    vector<int> countPositions(string_view text) const {
        checkState(0); assert(text.size() < INT_MAX); vector<int> answer{matchCount(0)}; answer.reserve(text.size() + 1);
        int u = 0; for (unsigned char c : text) { u = step(u, c); answer.push_back(matchCount(u)); } return answer;
    }
    // O(1) workspace; (n+1) * k < INT64_MAX under the documented length/ID bounds.
    lng countMatches(string_view text) const {
        checkState(0); assert(text.size() < INT_MAX); int u = 0; lng answer = matchCount(0);
        for (unsigned char c : text) { u = step(u, c); answer += matchCount(u); } return answer;
    }
};
