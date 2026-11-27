#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), U: O(L * log(A + 1)) amortized (map) or O(L) (dense), Q: same, M: O(P) or O(P * S); A <= 256 fanout, P live nodes.
// Byte-key multiset with lng counts; S = 0 maps every byte, dense keys use bytes in [BASE, BASE + S); missing node is -1.
template<int S = 0, unsigned char BASE = 0> struct BasicTrie {
    static_assert(S >= 0 && BASE + S <= 256 && (S || !BASE));
    using Next = std::conditional_t<S == 0, map<unsigned char, int>, array<int, S>>;
    struct Node {
        Next next{};
        lng terminal = 0, pass = 0;
    };
    vector<Node> nodes{Node{}};
    vector<int> free;

    // T: O(P), M: O(1); keeps node and free-list capacity.
    void clear() { nodes.clear(); nodes.emplace_back(); free.clear(); }
    lng size() const { return nodes[0].pass; }
    bool empty() const { return !size(); }

    int newNode() {
        if (!free.empty()) {
            int u = free.back();
            free.pop_back();
            return u;}
        assert(nodes.size() < INT_MAX);
        nodes.emplace_back();
        return int(nodes.size()) - 1;}
    int step(int u, char c) const {
        int x = uint8_t(c) - BASE;
        if constexpr (S == 0) {
            auto it = nodes[u].next.find(uint8_t(x));
            return it == nodes[u].next.end() ? -1 : it->second;}
        else { return 0 <= x && x < S && nodes[u].next[x] ? nodes[u].next[x] : -1; }}
    int findNode(string_view s) const {
        assert(s.size() < INT_MAX);
        int u = 0;
        for (char c : s) {
            if ((u = step(u, c)) < 0) { return -1; }}
        return u;}
    lng count(string_view s) const { int u = findNode(s); return u < 0 ? 0 : nodes[u].terminal; }
    lng countPrefix(string_view s) const { int u = findNode(s); return u < 0 ? 0 : nodes[u].pass; }
    bool search(string_view s) const { return count(s) != 0; }

    void insert(string_view s, lng k = 1) {
        assert(s.size() < INT_MAX && k > 0 && size() <= std::numeric_limits<lng>::max() - k);
        if constexpr (S > 0) { assert(std::ranges::all_of(s, [](char c) { return BASE <= uint8_t(c) && uint8_t(c) < BASE + S; })); }
        int u = 0;
        nodes[u].pass += k;
        for (char c : s) {
            int v = step(u, c);
            if (v < 0) {
                v = newNode();
                nodes[u].next[uint8_t(uint8_t(c) - BASE)] = v;}
            u = v;
            nodes[u].pass += k;}
        nodes[u].terminal += k;}
    // T: as U, M: O(L); false (absent or fewer than k copies) leaves the trie unchanged.
    bool erase(string_view s, lng k = 1) {
        assert(k > 0);
        int u = findNode(s);
        if (u < 0 || nodes[u].terminal < k) { return false; }
        nodes[u].terminal -= k;
        vector<int> path{0};
        for (char c : s) { path.push_back(step(path.back(), c)); }
        for (int v : path) { nodes[v].pass -= k; }
        for (int i = int(s.size()); i && !nodes[path[i]].pass; --i) {
            if constexpr (S == 0) { nodes[path[i - 1]].next.erase(uint8_t(s[i - 1])); }
            else { nodes[path[i - 1]].next[uint8_t(s[i - 1]) - BASE] = 0; }
            free.push_back(path[i]);}
        return true;}

    // T: O(L * log(A + 1) + V) (map) or O(L + V * S) (dense), M: O(L + h); V visited nodes, h their depth; visit(word, count) -> bool.
    template<class F> bool forEach(string_view prefix, F &&visit) const {
        int u = findNode(prefix);
        if (u < 0) { return true; }
        string word(prefix);
        if (nodes[u].terminal && !visit(string_view(word), nodes[u].terminal)) { return false; }
        using Cursor = std::conditional_t<S == 0, typename Next::const_iterator, int>;
        auto first = [&](int v) -> Cursor {
            if constexpr (S == 0) { return nodes[v].next.begin(); }
            else { return 0; }};
        vector<pair<int, Cursor>> stack{{u, first(u)}};
        while (!stack.empty()) {
            auto &[w, it] = stack.back();
            int x = -1, v = -1;
            if constexpr (S == 0) {
                if (it != nodes[w].next.end()) { x = it->first; v = it->second; ++it; }}
            else {
                while (it < S && !nodes[w].next[it]) { ++it; }
                if (it < S) { x = it; v = nodes[w].next[it++]; }}
            if (v < 0) {
                stack.pop_back();
                if (!stack.empty()) { word.pop_back(); }
                continue;}
            word.push_back(char(x + BASE));
            if (nodes[v].terminal && !visit(string_view(word), nodes[v].terminal)) { return false; }
            stack.push_back({v, first(v)});}
        return true;}
    template<class F> bool forEach(F &&visit) const { return forEach({}, std::forward<F>(visit)); }
    template<class F> bool forEachPrefixOf(string_view q, F &&visit) const {
        assert(q.size() < INT_MAX);
        for (int i = 0, u = 0;; ++i) {
            if (nodes[u].terminal && !visit(i, nodes[u].terminal)) { return false; }
            if (i == int(q.size()) || (u = step(u, q[i])) < 0) { return true; }}}
    int longestPrefix(string_view q) const {
        int res = -1;
        forEachPrefixOf(q, [&](int len, lng) { res = len; return true; });
        return res;}
};
using Trie = BasicTrie<>;
template<int S = 26, unsigned char BASE = 'a'> using TrieDense = BasicTrie<S, BASE>;
