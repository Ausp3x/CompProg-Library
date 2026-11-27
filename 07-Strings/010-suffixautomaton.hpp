#pragma once
#include "../01-Core/01-template.hpp"

// S: O(n * log(S + 1)) map, O(n * S) dense, U: extend O(log(S + 1)) amortized map, O(S) dense, Q: O(m * log(S + 1)) map, O(m) dense pattern walks, M: O(n) map, O(n * S) dense; S = 0 means 256.
// Byte text, n < 2^30; dense keys in [BASE, BASE + S); counting queries need build(); absent pattern gives -1, empty result {-1, 0}.
template<int S = 0, unsigned char BASE = 0> struct BasicSuffixAutomaton {
    static_assert(S >= 0 && BASE + S <= 256 && (S || !BASE));
    static constexpr int A = S ? S : 256;
    using Next = std::conditional_t<S == 0, map<unsigned char, int>, array<int, S>>;
    struct Node {
        int len = 0, link = -1, end = -1;
        Next next{};
    };
    vector<Node> nodes{Node{}};
    int last = 0;
    lng distinct = 0;
    lll total = 0;
    vector<int> order, cnt, last_end, child_start, child;
    vector<lng> paths, weighted;
    bool built = false;

    BasicSuffixAutomaton() = default;
    explicit BasicSuffixAutomaton(string_view s) { build(s); }
    int size() const { return nodes[last].len; }
    int step(int u, int x) const {
        if constexpr (S == 0) {
            auto it = 0 <= x && x < A ? nodes[u].next.find(uint8_t(x)) : nodes[u].next.end();
            return it == nodes[u].next.end() ? -1 : it->second;}
        else { return 0 <= x && x < S && nodes[u].next[x] ? nodes[u].next[x] : -1; }}
    int step(int u, char c) const { return step(u, uint8_t(c) - BASE); }
    static int target(const auto &e) {
        if constexpr (S == 0) { return e.second; }
        else { return e; }}
    pair<int, int> firstFrom(int u, int x) const {
        if constexpr (S == 0) {
            auto it = x < A ? nodes[u].next.lower_bound(uint8_t(x)) : nodes[u].next.end();
            return it == nodes[u].next.end() ? pair<int, int>{A, -1} : pair<int, int>{it->first, it->second};}
        else {
            for (; x < A && !nodes[u].next[x]; ++x) {}
            return {x, x < A ? nodes[u].next[x] : -1};}}

    void extend(char c) {
        int x = uint8_t(c) - BASE, cur = int(nodes.size()), p = last;
        assert(0 <= x && x < A && size() < (1 << 30) - 1);
        nodes.push_back({nodes[last].len + 1, 0, nodes[last].len, {}});
        for (; p != -1 && step(p, x) < 0; p = nodes[p].link) { nodes[p].next[uint8_t(x)] = cur; }
        if (p != -1) {
            int q = step(p, x);
            if (nodes[p].len + 1 == nodes[q].len) { nodes[cur].link = q; }
            else {
                int clone = int(nodes.size());
                nodes.push_back(nodes[q]);
                nodes[clone].len = nodes[p].len + 1;
                for (; p != -1 && step(p, x) == q; p = nodes[p].link) { nodes[p].next[uint8_t(x)] = clone; }
                nodes[q].link = nodes[cur].link = clone;}}
        last = cur;
        lll lo = nodes[nodes[cur].link].len, hi = nodes[cur].len;
        distinct += lng(hi - lo);
        total += (lo + 1 + hi) * (hi - lo) / 2;
        built = false;}
    // T: O(n) map, O(n * S) dense, M: O(n); fills order, cnt, last_end, the link-tree CSR, paths and weighted.
    void build() {
        int states = int(nodes.size()), n = size();
        vector<int> bucket(n + 2);
        for (auto &v : nodes) { ++bucket[v.len + 1]; }
        for (int i = 0; i <= n; ++i) { bucket[i + 1] += bucket[i]; }
        order.assign(states, 0);
        for (int v = 0; v < states; ++v) { order[bucket[nodes[v].len]++] = v; }
        cnt.assign(states, 0);
        last_end.assign(states, -1);
        child_start.assign(states + 1, 0);
        for (int v = 1; v < states; ++v) {
            cnt[v] = nodes[v].end + 1 == nodes[v].len;
            last_end[v] = nodes[v].end;
            ++child_start[nodes[v].link + 1];}
        for (int v = 0; v < states; ++v) { child_start[v + 1] += child_start[v]; }
        child.assign(max(states - 1, 0), 0);
        vector<int> at(child_start.begin(), child_start.end() - 1);
        for (int v = 1; v < states; ++v) { child[at[nodes[v].link]++] = v; }
        paths.assign(states, 1);
        weighted.assign(states, 0);
        for (int k = states - 1; k >= 0; --k) {
            int v = order[k];
            if (v) {
                cnt[nodes[v].link] += cnt[v];
                last_end[nodes[v].link] = max(last_end[nodes[v].link], last_end[v]);
                weighted[v] = cnt[v];}
            for (auto &e : nodes[v].next) {
                if (int u = target(e); u > 0) { paths[v] += paths[u]; weighted[v] += weighted[u]; }}}
        cnt[0] = n + 1;
        built = true;}
    void build(string_view s) {
        assert(s.size() < (1 << 30));
        *this = BasicSuffixAutomaton();
        nodes.reserve(2 * s.size() + 1);
        for (char c : s) { extend(c); }
        build();}

    int findNode(string_view p) const {
        int u = 0;
        for (char c : p) {
            if ((u = step(u, c)) < 0) { return -1; }}
        return u;}
    bool isSubstring(string_view p) const { return findNode(p) >= 0; }
    int occurrenceCount(string_view p) const { assert(built); int u = findNode(p); return u < 0 ? 0 : cnt[u]; }
    int firstOccurrence(string_view p) const { int u = findNode(p); return u < 0 ? -1 : nodes[u].end - int(p.size()) + 1; }
    int lastOccurrence(string_view p) const { assert(built); int u = findNode(p); return u < 0 ? -1 : last_end[u] - int(p.size()) + 1; }
    // T: O(m * log(S + 1) + out * log(out + 1)), M: O(out); ascending starts, empty pattern gives 0..n.
    vector<int> occurrences(string_view p) const {
        assert(built);
        int u = findNode(p), m = int(p.size());
        vector<int> res, stack;
        if (u >= 0) { stack.push_back(u); }
        while (!stack.empty()) {
            int v = stack.back();
            stack.pop_back();
            if (v && nodes[v].end + 1 == nodes[v].len) { res.push_back(nodes[v].end - m + 1); }
            for (int k = child_start[v]; k < child_start[v + 1]; ++k) { stack.push_back(child[k]); }}
        if (u == 0) { res.push_back(0); }
        sort(res.begin(), res.end());
        return res;}
    int endposSize(int v) const { assert(built); return cnt[v]; }
    int minimalLength(int v) const { return v ? nodes[nodes[v].link].len + 1 : 0; }
    lng distinctSubstrings() const { return distinct; }
    lll totalSubstringLength() const { return total; }

    // T: O(L * S) dense, O(L * log(S + 1) + visited transitions) map; L answer length; {start, length} of the k-th (0-based) substring, {-1, 0} if k is out of range.
    pair<int, int> kthSubstringDistinct(lng k) const { return kth(k, false); }
    pair<int, int> kthSubstring(lng k) const { return kth(k, true); }
    pair<int, int> kth(lng k, bool multi) const {
        assert(built);
        const vector<lng> &w = multi ? weighted : paths;
        if (k < 0 || k >= w[0] - !multi) { return {-1, 0}; }
        for (int u = 0, len = 1;; ++len) {
            for (auto e = firstFrom(u, 0); e.second >= 0; e = firstFrom(u, e.first + 1)) {
                int v = e.second;
                if (k >= w[v]) { k -= w[v]; continue; }
                lng own = multi ? cnt[v] : 1;
                if (k < own) { return {nodes[v].end - len + 1, len}; }
                k -= own;
                u = v;
                break;}}}
    // T: O(out * S) dense, O(out * log(S + 1)) map, M: O(n); visit(word, state) -> bool in lexicographic order of distinct nonempty substrings; false stops.
    template<class F> bool lexicographicWalk(F &&visit) const {
        string word;
        vector<pair<int, int>> stack{{0, 0}};
        while (!stack.empty()) {
            auto &[u, x] = stack.back();
            auto [y, v] = firstFrom(u, x);
            x = y + 1;
            if (v < 0) {
                stack.pop_back();
                if (!word.empty()) { word.pop_back(); }
                continue;}
            word.push_back(char(y + BASE));
            if (!visit(string_view(word), v)) { return false; }
            stack.push_back({v, 0});}
        return true;}
    // T: O(m * log(S + 1)), M: O(m); res[j] = longest suffix of t[0, j] that occurs in the text.
    vector<int> matchingStatistics(string_view t) const {
        vector<int> res(t.size());
        int u = 0, len = 0;
        for (int j = 0; j < int(t.size()); ++j) {
            while (u && step(u, t[j]) < 0) { u = nodes[u].link; len = nodes[u].len; }
            if (int v = step(u, t[j]); v >= 0) { u = v; ++len; }
            res[j] = len;}
        return res;}
    // T: O(m * log(S + 1)), M: O(m); {start in text (first occurrence), start in t (leftmost), length}, {0, 0, 0} if none.
    tuple<int, int, int> longestCommonSubstring(string_view t) const {
        int u = 0, len = 0, best = 0, at = 0, state = 0;
        for (int j = 0; j < int(t.size()); ++j) {
            while (u && step(u, t[j]) < 0) { u = nodes[u].link; len = nodes[u].len; }
            if (int v = step(u, t[j]); v >= 0) { u = v; ++len; }
            if (len > best) { best = len; at = j - len + 1; state = u; }}
        if (!best) { return {0, 0, 0}; }
        return {nodes[state].end - best + 1, at, best};}
    // T: O(m * log(S + 1) + m * log(m + 1)), M: O(m); total occurrences of the distinct rotations of t; empty t gives n + 1.
    lng cyclicShiftOccurrences(string_view t) const {
        assert(built && t.size() < (1u << 30));
        int m = int(t.size()), u = 0, len = 0;
        if (!m) { return cnt[0]; }
        vector<int> hit;
        for (int j = 0; j < 2 * m - 1; ++j) {
            char c = t[j % m];
            while (u && step(u, c) < 0) { u = nodes[u].link; len = nodes[u].len; }
            if (int v = step(u, c); v >= 0) { u = v; ++len; }
            if (len >= m) {
                while (nodes[nodes[u].link].len >= m) { u = nodes[u].link; }
                len = m;
                hit.push_back(u);}}
        sort(hit.begin(), hit.end());
        hit.erase(unique(hit.begin(), hit.end()), hit.end());
        lng res = 0;
        for (int v : hit) { res += cnt[v]; }
        return res;}
    // T: O(V * S) dense, O(V * log(S + 1)) map, M: O(V); lexicographically smallest shortest string over the alphabet that is not a substring.
    string shortestAbsentString() const {
        assert(built);
        int states = int(nodes.size());
        vector<int> d(states);
        auto best = [&](int u) {
            pair<int, int> res{INT_MAX, 0};
            for (int y = 0; y < A; ++y) {
                auto [x, v] = firstFrom(u, y);
                if (x != y) { return pair<int, int>{1, y}; }
                res = min(res, {d[v] + 1, y});}
            return res;};
        for (int k = states - 1; k >= 0; --k) { d[order[k]] = best(order[k]).first; }
        string res;
        for (int u = 0;;) {
            auto [len, x] = best(u);
            res.push_back(char(x + BASE));
            if (len == 1) { return res; }
            u = step(u, x);}}
    vector<vector<int>> suffixLinkTree() const {
        vector<vector<int>> res(nodes.size());
        for (int v = 1; v < int(nodes.size()); ++v) { res[nodes[v].link].push_back(v); }
        return res;}
};
using SuffixAutomaton = BasicSuffixAutomaton<>;
template<int S = 26, unsigned char BASE = 'a'> using SuffixAutomatonDense = BasicSuffixAutomaton<S, BASE>;

// T: O(n * log(S + 1)), M: O(n); res[i] = distinct nonempty substrings of s[0, i), n + 1 entries.
inline vector<lng> distinctSubstringsOnline(string_view s) {
    SuffixAutomaton sam;
    vector<lng> res{0};
    for (char c : s) { sam.extend(c); res.push_back(sam.distinctSubstrings()); }
    return res;}
