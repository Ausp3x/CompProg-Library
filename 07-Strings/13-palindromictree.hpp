#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), U: O(log(S + 2)) amortized (map), O(1) amortized (dense), Q: O(log(n)) substringSuffixPalindrome, O(V) counts, M: O(n) (map), O(n * S) (dense).
// Integer symbols (S = 0: any int, dense: [BASE, BASE + S)); total length n < INT_MAX - 2; node 0 has len -1, node 1 len 0, missing node -1.
template<int S = 0, int BASE = 0> struct BasicPalindromicTree {
    static_assert(S >= 0);
    using Next = std::conditional_t<S == 0, map<int, int>, array<int, S>>;
    struct Node {
        int len, link, diff, series, parent, pos;
        Next next{};
    };
    vector<Node> nodes{{-1, 0, 0, 0, 0, -1}, {0, 0, 0, 0, 0, -1}};
    vector<int> s, suffix;
    int last = 1, start = 0;

    BasicPalindromicTree() = default;
    explicit BasicPalindromicTree(string_view t) { build(t); }
    explicit BasicPalindromicTree(const vector<int> &t) { build(t); }
    void clear() { nodes.resize(2); nodes[0].next = nodes[1].next = Next{}; s.clear(); suffix.clear(); last = 1; start = 0; }
    void build(string_view t) { clear(); for (char c : t) { append(uint8_t(c)); } }
    void build(const vector<int> &t) { clear(); for (int c : t) { append(c); } }
    void newString() { start = int(s.size()); last = 1; }

    int size() const { return int(nodes.size()); }
    int distinctPalindromes() const { return size() - 2; }
    int step(int v, int c) const {
        if constexpr (S == 0) {
            auto it = nodes[v].next.find(c);
            return it == nodes[v].next.end() ? -1 : it->second;}
        else { return BASE <= c && c - BASE < S && nodes[v].next[c - BASE] ? nodes[v].next[c - BASE] : -1; }}
    int longestSuffixPalindrome(int r) const { assert(0 <= r && r <= int(s.size())); return r ? suffix[r - 1] : 1; }
    pair<int, int> palindrome(int v) const { assert(2 <= v && v < size()); return {nodes[v].pos - nodes[v].len + 1, nodes[v].pos + 1}; }
    int substringSuffixPalindrome(int l, int r) const {
        assert(0 <= l && l <= r && r <= int(s.size()));
        for (int v = longestSuffixPalindrome(r); nodes[v].len > 0; v = nodes[v].series) {
            const Node &u = nodes[v];
            if (u.len <= r - l) { return u.len; }
            if (nodes[u.series].len + u.diff <= r - l) { return u.len - (u.len - (r - l) + u.diff - 1) / u.diff * u.diff; }}
        return 0;}

    int append(int c) {
        assert(s.size() < INT_MAX - 3 && (S == 0 || (BASE <= c && c - BASE < S)));
        int i = int(s.size());
        s.push_back(c);
        auto fits = [&](int v) { return i - nodes[v].len - 1 >= start && s[i - nodes[v].len - 1] == c; };
        int v = last;
        while (!fits(v)) { v = nodes[v].link; }
        int u = step(v, c);
        if (u < 0) {
            int len = nodes[v].len + 2, link = 1;
            if (len > 1) {
                int w = nodes[v].link;
                while (!fits(w)) { w = nodes[w].link; }
                link = max(1, step(w, c));}
            int diff = len - nodes[link].len;
            u = size();
            nodes.push_back({len, link, diff, diff == nodes[link].diff ? nodes[link].series : link, v, i});
            if constexpr (S == 0) { nodes[v].next.emplace(c, u); }
            else if (BASE <= c && c - BASE < S) { nodes[v].next[c - BASE] = u; }}
        suffix.push_back(u);
        return last = u;}

    // T: O(V + r - l), M: O(V); occurrences ending in [l, r) per node; occurrencesAt[i] counts palindromes ending at i.
    vector<int> occurrenceCounts(int l = 0, int r = -1) const {
        if (r == -1) { r = int(s.size()); }
        assert(0 <= l && l <= r && r <= int(s.size()));
        vector<int> res(size());
        for (int i = l; i < r; ++i) { ++res[suffix[i]]; }
        for (int v = size() - 1; v >= 2; --v) { res[nodes[v].link] += res[v]; }
        return res;}
    vector<int> occurrencesAt() const {
        vector<int> depth(size()), res(s.size());
        for (int v = 2; v < size(); ++v) { depth[v] = depth[nodes[v].link] + 1; }
        for (int i = 0; i < int(s.size()); ++i) { res[i] = depth[suffix[i]]; }
        return res;}

    // T: O(n * log(n + 1)), M: O(V + n); res[i] folds piece(res[i - len]) over palindromic suffixes of the current string's prefix i (piece distributes over plus).
    template<class T, class Plus, class Piece> vector<T> palindromicFactorization(T zero, T one, Plus plus, Piece piece, bool even = false) const {
        int n = int(s.size()) - start;
        vector<T> res(n + 1, zero), series(size(), zero);
        res[0] = one;
        for (int i = 1; i <= n; ++i) {
            for (int v = suffix[start + i - 1]; nodes[v].len > 0; v = nodes[v].series) {
                const Node &u = nodes[v];
                series[v] = res[i - nodes[u.series].len - u.diff];
                if (u.diff == nodes[u.link].diff) { series[v] = plus(series[v], series[u.link]); }
                res[i] = plus(res[i], piece(series[v]));}
            if (even && i % 2) { res[i] = zero; }}
        return res;}
    vector<int> palindromicLength() const {
        return palindromicFactorization(INT_MAX, 0, [](int a, int b) { return min(a, b); }, [](int a) { return a + 1; });}
    template<class T> vector<T> evenPalindromePartition() const {
        return palindromicFactorization(T(0), T(1), [](const T &a, const T &b) { return a + b; }, [](const T &a) { return a; }, true);}
    vector<int> minPalindromePartition() const {
        int n = int(s.size()) - start;
        vector<pair<int, int>> best(n + 1, {INT_MAX, 0}), series(size());
        best[0] = {0, 0};
        for (int i = 1; i <= n; ++i) {
            for (int v = suffix[start + i - 1]; nodes[v].len > 0; v = nodes[v].series) {
                const Node &u = nodes[v];
                int j = i - nodes[u.series].len - u.diff;
                series[v] = {best[j].first, j};
                if (u.diff == nodes[u.link].diff) { series[v] = min(series[v], series[u.link]); }
                best[i] = min(best[i], {series[v].first + 1, series[v].second});}}
        vector<int> res{n};
        while (n) { res.push_back(n = best[n].second); }
        reverse(res.begin(), res.end());
        return res;}
};
using PalindromicTree = BasicPalindromicTree<>;
template<int S = 26, int BASE = 'a'> using PalindromicTreeDense = BasicPalindromicTree<S, BASE>;
