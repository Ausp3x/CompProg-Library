#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), U: O(m * log(sigma + 1)) amortized, Q: O(m * log(sigma + 1)), M: O(P).
// Byte multiset: unsigned alphabet 0..255, empty/NUL keys included, m < INT_MAX.
// P is peak live prefix nodes (including root), P <= INT_MAX; sigma <= 256.
// Exact positive lng multiplicities, total <= INT64_MAX. erase is all-or-nothing.
// Node slots are reclaimed/reused; clear resets contents but retains capacities.
// Fields/helpers are internal state; node IDs/references are unstable after edits.
// A moved-from object supports clear/assignment before other operations.
struct Trie {
    struct Node {
        map<unsigned char, int> next;
        lng terminal = 0, pass = 0;
    };
    vector<Node> nodes{Node{}};
    vector<int> free;

    // T: O(P); releases edges, retains node/free-list capacity.
    void clear() { nodes.clear(); nodes.emplace_back(); free.clear(); }
    lng size() const { return nodes[0].pass; }
    bool empty() const { return !size(); }

    int newNode() {
        if (!free.empty()) { int u = free.back(); free.pop_back(); return u; }
        assert(nodes.size() < INT_MAX); int u = int(nodes.size());
        nodes.emplace_back(); return u;
    }
    int findNode(string_view s) const {
        assert(s.size() < INT_MAX); int u = 0;
        for (unsigned char c : s) {
            auto it = nodes[u].next.find(c);
            if (it == nodes[u].next.end()) { return -1; }
            u = it->second; }
        return u;
    }
    lng count(string_view s) const { int u = findNode(s); return u < 0 ? 0 : nodes[u].terminal; }
    lng countPrefix(string_view s) const { int u = findNode(s); return u < 0 ? 0 : nodes[u].pass; }
    bool search(string_view s) const { return count(s) != 0; }

    void insert(string_view s, lng k = 1) {
        assert(s.size() < INT_MAX && k > 0 && size() <= std::numeric_limits<lng>::max() - k);
        int u = 0; nodes[u].pass += k;
        for (unsigned char c : s) {
            auto it = nodes[u].next.find(c); int v;
            if (it == nodes[u].next.end()) { v = newNode(); nodes[u].next.emplace(c, v); }
            else { v = it->second; }
            u = v; nodes[u].pass += k; }
        nodes[u].terminal += k;
    }
    // Workspace O(m); false leaves the dictionary unchanged (absent/insufficient).
    bool erase(string_view s, lng k = 1) {
        assert(k > 0); int u = findNode(s);
        if (u < 0 || nodes[u].terminal < k) { return false; }
        nodes[u].terminal -= k;
        vector<int> path{0}; nodes[0].pass -= k; u = 0;
        for (unsigned char c : s) { u = nodes[u].next.find(c)->second; nodes[u].pass -= k; path.push_back(u); }
        for (int i = int(s.size()); i && !nodes[path[i]].pass; --i) {
            nodes[path[i - 1]].next.erase(static_cast<unsigned char>(s[i - 1]));
            free.push_back(path[i]); }
        return true;
    }

    // T: O(m * log(sigma + 1) + V), workspace O(h + m), excluding callback cost.
    // V visited subtree nodes, h max suffix depth. One callback per distinct key,
    // unsigned-byte lexicographic order. Borrowed word view is valid during callback.
    // Callback(word, multiplicity)->bool; false cancels and returns false, even last.
    // Empty prefix enumerates all keys; missing prefix returns true without visits.
    // Callbacks may nest reads, but never mutate the dictionary.
    template<class F> bool forEach(string_view prefix, F &&visit) const {
        int u = findNode(prefix); if (u < 0) { return true; }
        string word(prefix);
        if (nodes[u].terminal && !visit(string_view(word), nodes[u].terminal)) { return false; }
        struct Frame { int u; map<unsigned char, int>::const_iterator it; };
        vector<Frame> stack{{u, nodes[u].next.begin()}};
        while (!stack.empty()) {
            auto &f = stack.back();
            if (f.it == nodes[f.u].next.end()) {
                stack.pop_back(); if (!stack.empty()) { word.pop_back(); } continue; }
            auto [c, v] = *f.it++; word.push_back(char(c));
            if (nodes[v].terminal && !visit(string_view(word), nodes[v].terminal)) { return false; }
            stack.push_back({v, nodes[v].next.begin()}); }
        return true;
    }
    template<class F> bool forEach(F &&visit) const { return forEach({}, std::forward<F>(visit)); }
};
