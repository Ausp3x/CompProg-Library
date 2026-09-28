#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/04-sparsetable.hpp"

// S: O(n * log(n + 1)), Q: O(1) LCE, M: O(n * log(n + 1)) with RMQ, O(n) without.
// Integer T (including signed/unsigned extremes), n < INT_MAX; byte strings use T=int.
// Sparse integer compression costs O(n * log(n + 1)); byte initialization is O(n+256).
// Sentinel-free: sa/rank have n entries, lcp[k]=LCP(sa[k],sa[k+1]) has max(0,n-1).
// The empty suffix n is omitted from sa, but is valid in lce. No caller sentinel.
// Fields are read-only after construction; assign a moved-from object before queries.
template<class T = int> requires std::is_integral_v<T>
struct SuffixArray {
    struct Minimum { int operator()(int a, int b) const { return min(a, b); } };
    vector<T> text;
    vector<int> sa, rank, lcp;
    SparseTable<int, Minimum> rmq{{}, {}};
    bool indexed = true;

    SuffixArray() = default;
    explicit SuffixArray(vector<T> s, bool with_rmq = true) : text(std::move(s)) { build(false, with_rmq); }
    explicit SuffixArray(string_view s, bool with_rmq = true) requires std::is_same_v<T, int> {
        assert(s.size() < INT_MAX); text.reserve(s.size());
        for (unsigned char c : s) { text.push_back(c); }
        build(true, with_rmq);
    }
    int size() const { return int(text.size()); }

    // Construction helper: bytes requires every text value in [0,256).
    void build(bool bytes, bool with_rmq) {
        assert(text.size() < INT_MAX); int n = size(); sa.resize(n); rank.resize(n);
        iota(sa.begin(), sa.end(), 0);
        if (bytes) {
            array<int, 256> count{};
            for (T c : text) { assert(0 <= c && c < 256); ++count[int(c)]; }
            for (int c = 1; c < 256; ++c) { count[c] += count[c - 1]; }
            for (int i = n - 1; i >= 0; --i) { sa[--count[int(text[i])]] = i; } }
        else { sort(sa.begin(), sa.end(), [&](int i, int j) { return text[i] < text[j]; }); }
        int classes = 0;
        for (int i = 0; i < n; ++i) {
            if (!i || text[sa[i - 1]] != text[sa[i]]) { ++classes; }
            rank[sa[i]] = classes - 1; }
        vector<int> second(n), next_rank(n), count(n);
        for (int k = 1; k < n && classes < n; k = k >= n - k ? n : k + k) {
            int at = 0;
            for (int i = n - k; i < n; ++i) { second[at++] = i; }
            for (int i : sa) { if (i >= k) { second[at++] = i - k; } }
            fill(count.begin(), count.begin() + classes, 0);
            for (int r : rank) { ++count[r]; }
            for (int i = 1; i < classes; ++i) { count[i] += count[i - 1]; }
            for (int i = n - 1; i >= 0; --i) { sa[--count[rank[second[i]]]] = second[i]; }
            classes = 1; next_rank[sa[0]] = 0;
            for (int i = 1; i < n; ++i) {
                int a = sa[i - 1], b = sa[i];
                int ra = k < n - a ? rank[a + k] : -1, rb = k < n - b ? rank[b + k] : -1;
                if (rank[a] != rank[b] || ra != rb) { ++classes; }
                next_rank[b] = classes - 1; }
            rank.swap(next_rank); }
        lcp.assign(max(0, n - 1), 0);
        for (int i = 0, k = 0; i < n; ++i) {
            if (rank[i] == n - 1) { k = 0; continue; }
            int j = sa[rank[i] + 1];
            while (k < n - i && k < n - j && text[i + k] == text[j + k]) { ++k; }
            lcp[rank[i]] = k; if (k) { --k; } }
        indexed = false; rmq = SparseTable<int, Minimum>({}, {});
        if (with_rmq) { buildRmq(); }
    }
    // S: O(n * log(n + 1)); may be called after construction with with_rmq=false.
    void buildRmq() { rmq = SparseTable<int, Minimum>(lcp, {}); indexed = true; }

    // Requires RMQ, even for trivial queries. Exact suffix LCE; 0 <= i,j <= n.
    int lce(int i, int j) const {
        int n = size(); assert(indexed && 0 <= i && i <= n && 0 <= j && j <= n);
        if (i == j) { return n - i; }
        if (i == n || j == n) { return 0; }
        int a = rank[i], b = rank[j]; if (a > b) { swap(a, b); }
        return rmq.query(a, b);
    }
    // Exact LCE of half-open substrings, capped at the shorter length.
    int substringLce(int l, int r, int a, int b) const {
        assert(0 <= l && l <= r && r <= size() && 0 <= a && a <= b && b <= size());
        return min({r - l, b - a, lce(l, a)});
    }
    // Q: O(m + log(n + 1)) with RMQ; O(m * log(n + 1)) otherwise, m=pattern.size().
    // Returns ranks [l,r) in sa; empty pattern returns [0,n), omitting boundary n.
    pair<int, int> patternRange(const vector<T> &pattern) const {
        assert(pattern.size() < INT_MAX); int n = size(), m = int(pattern.size());
        auto bound = [&](bool upper) {
            int l = 0, r = n, anchor = -1, matched = 0;
            while (l < r) {
                int mid = std::midpoint(l, r), k = 0, cmp = 0;
                if (indexed && anchor != -1) {
                    k = min(matched, lce(sa[anchor], sa[mid]));
                    if (k < matched) { cmp = mid < anchor ? -1 : 1; } }
                if (!cmp) {
                    while (k < m && k < n - sa[mid] && text[sa[mid] + k] == pattern[k]) { ++k; }
                    if (k == m) { cmp = 0; }
                    else if (k == n - sa[mid]) { cmp = -1; }
                    else { cmp = text[sa[mid] + k] < pattern[k] ? -1 : 1; }
                    if (k >= matched) { matched = k; anchor = mid; } }
                if (cmp < 0 || (upper && !cmp)) { l = mid + 1; } else { r = mid; } }
            return l;
        };
        return {bound(false), bound(true)};
    }
    // Byte overload additionally uses O(m) temporary memory; embedded NUL is valid.
    pair<int, int> patternRange(string_view pattern) const requires std::is_same_v<T, int> {
        assert(pattern.size() < INT_MAX); vector<int> p; p.reserve(pattern.size());
        for (unsigned char c : pattern) { p.push_back(c); }
        return patternRange(p);
    }
    // Q: O(n); {start,length}, overlapping occurrences allowed, {0,0} if none.
    // Ties choose the lexicographically smallest repeated substring, then its earliest start.
    pair<int, int> longestRepeated() const {
        int start = 0, length = 0;
        for (int i = 0; i < int(lcp.size()); ++i) {
            if (lcp[i] > length) { length = lcp[i]; start = min(sa[i], sa[i + 1]); } }
        if (length) {
            int at = rank[start];
            while (at && lcp[at - 1] >= length) { start = min(start, sa[--at]); }
            while (at < int(lcp.size()) && lcp[at] >= length) { start = min(start, sa[++at]); } }
        return {start, length};
    }
};
SuffixArray(string_view, bool = true) -> SuffixArray<int>;
SuffixArray(const string &, bool = true) -> SuffixArray<int>;
SuffixArray(const char *, bool = true) -> SuffixArray<int>;

// T: O((n+m) * log(n+m+1)), M: O(n+m); sparse integer compression included.
// Return {start_in_a,start_in_b,length}; no common symbol gives {0,0,0}.
// Ties choose lexicographically smallest substring, then earliest start in each input.
// n+m+1 < INT_MAX; equal integer types, no reserved input value or caller sentinel.
template<class T> requires std::is_integral_v<T>
tuple<int, int, int> longestCommonSubstring(const vector<T> &a, const vector<T> &b) {
    assert(a.size() < INT_MAX && b.size() < INT_MAX && a.size() + b.size() < INT_MAX - 1);
    if (a.empty() || b.empty()) { return {0, 0, 0}; }
    int n = int(a.size()), m = int(b.size()); vector<T> alphabet = a;
    alphabet.insert(alphabet.end(), b.begin(), b.end()); sort(alphabet.begin(), alphabet.end());
    alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
    vector<int> joined; joined.reserve(n + m + 1);
    for (T c : a) { joined.push_back(int(lower_bound(alphabet.begin(), alphabet.end(), c) - alphabet.begin()) + 1); }
    joined.push_back(0);
    for (T c : b) { joined.push_back(int(lower_bound(alphabet.begin(), alphabet.end(), c) - alphabet.begin()) + 1); }
    SuffixArray index(std::move(joined), false); int best = 0, first = 0;
    for (int i = 0; i < n + m; ++i) {
        int x = index.sa[i], y = index.sa[i + 1];
        if (x != n && y != n && (x < n) != (y < n) && index.lcp[i] > best) { best = index.lcp[i]; first = i; } }
    if (!best) { return {0, 0, 0}; }
    int x = n, y = m, last = first;
    while (first && index.lcp[first - 1] >= best) { --first; }
    while (last < n + m && index.lcp[last] >= best) { ++last; }
    for (int i = first; i <= last; ++i) {
        int p = index.sa[i]; if (p < n) { x = min(x, p); } else if (p > n) { y = min(y, p - n - 1); } }
    return {x, y, best};
}
// Same bounds/contracts, bytes ordered unsigned; temporary integer copies use O(n+m).
inline tuple<int, int, int> longestCommonSubstring(string_view a, string_view b) {
    assert(a.size() < INT_MAX && b.size() < INT_MAX && a.size() + b.size() < INT_MAX - 1);
    vector<int> x, y; x.reserve(a.size()); y.reserve(b.size());
    for (unsigned char c : a) { x.push_back(c); }
    for (unsigned char c : b) { y.push_back(c); }
    return longestCommonSubstring(x, y);
}
