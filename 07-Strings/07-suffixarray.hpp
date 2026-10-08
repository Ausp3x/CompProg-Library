#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/04-sparsetable.hpp"

// S: O(n * log(n + 1)), U: NA, Q: O(1) lce, O(m + log(n + 1)) patternRange (O(m * log(n + 1)) without RMQ), M: O(n * log(n + 1)) with RMQ, O(n) without.
// Integral T, n < INT_MAX, sentinel-free (sa omits suffix n, lce accepts it); string input is T = int with unsigned byte order.
template<class T = int> requires std::is_integral_v<T>
struct SuffixArray {
    struct Minimum { int operator()(int a, int b) const { return min(a, b); } };
    vector<T> text;
    vector<int> sa, rank, lcp;
    SparseTable<int, Minimum> rmq{{}, {}};
    bool indexed = true;

    SuffixArray() = default;
    explicit SuffixArray(vector<T> s, bool with_rmq = true) : text(std::move(s)) { build(false, with_rmq); }
    explicit SuffixArray(std::initializer_list<T> s, bool with_rmq = true) : SuffixArray(vector<T>(s), with_rmq) {}
    explicit SuffixArray(string_view s, bool with_rmq = true) requires std::is_same_v<T, int> {
        assert(s.size() < INT_MAX);
        text.assign(s.begin(), s.end());
        for (int &c : text) { c = uint8_t(c); }
        build(true, with_rmq);}
    int size() const { return int(text.size()); }

    void build(bool bytes, bool with_rmq) {
        assert(text.size() < INT_MAX);
        int n = size();
        sa.resize(n);
        rank.resize(n);
        iota(sa.begin(), sa.end(), 0);
        if (bytes) {
            array<int, 256> count{};
            for (T c : text) {
                assert(0 <= lll(c) && lll(c) < 256);
                ++count[int(c)];}
            for (int c = 1; c < 256; ++c) { count[c] += count[c - 1]; }
            for (int i = n - 1; i >= 0; --i) { sa[--count[int(text[i])]] = i; }}
        else { sort(sa.begin(), sa.end(), [&](int i, int j) { return text[i] < text[j]; }); }
        int classes = 0;
        for (int i = 0; i < n; ++i) {
            if (!i || text[sa[i - 1]] != text[sa[i]]) { ++classes; }
            rank[sa[i]] = classes - 1;}
        vector<int> second(n), next_rank(n), count(n);
        for (int k = 1; k < n && classes < n; k = k >= n - k ? n : 2 * k) {
            int at = 0;
            for (int i = n - k; i < n; ++i) { second[at++] = i; }
            for (int i : sa) {
                if (i >= k) { second[at++] = i - k; }}
            fill(count.begin(), count.begin() + classes, 0);
            for (int r : rank) { ++count[r]; }
            for (int i = 1; i < classes; ++i) { count[i] += count[i - 1]; }
            for (int i = n - 1; i >= 0; --i) { sa[--count[rank[second[i]]]] = second[i]; }
            classes = 1;
            next_rank[sa[0]] = 0;
            for (int i = 1; i < n; ++i) {
                int a = sa[i - 1], b = sa[i];
                int ra = k < n - a ? rank[a + k] : -1, rb = k < n - b ? rank[b + k] : -1;
                if (rank[a] != rank[b] || ra != rb) { ++classes; }
                next_rank[b] = classes - 1;}
            rank.swap(next_rank);}
        lcp.assign(max(0, n - 1), 0);
        for (int i = 0, k = 0; i < n; ++i) {
            if (rank[i] == n - 1) { k = 0; continue; }
            int j = sa[rank[i] + 1];
            while (k < n - i && k < n - j && text[i + k] == text[j + k]) { ++k; }
            lcp[rank[i]] = k;
            k -= k > 0;}
        indexed = false;
        rmq = SparseTable<int, Minimum>({}, {});
        if (with_rmq) { buildRmq(); }}
    void buildRmq() { rmq = SparseTable<int, Minimum>(lcp, {}); indexed = true; }

    int lce(int i, int j) const {
        int n = size();
        assert(indexed && 0 <= i && i <= n && 0 <= j && j <= n);
        if (i == j) { return n - i; }
        if (i == n || j == n) { return 0; }
        auto [a, b] = std::minmax(rank[i], rank[j]);
        return rmq.query(a, b);}
    int substringLce(int l, int r, int a, int b) const {
        assert(0 <= l && l <= r && r <= size() && 0 <= a && a <= b && b <= size());
        return min({r - l, b - a, lce(l, a)});}
    pair<int, int> patternRange(const vector<T> &pattern) const {
        assert(pattern.size() < INT_MAX);
        int n = size(), m = int(pattern.size());
        auto bound = [&](bool upper) {
            int l = 0, r = n, anchor = -1, matched = 0;
            while (l < r) {
                int mid = std::midpoint(l, r), k = 0, cmp = 0;
                if (indexed && anchor != -1) {
                    k = min(matched, lce(sa[anchor], sa[mid]));
                    if (k < matched) { cmp = mid < anchor ? -1 : 1; }}
                if (!cmp) {
                    while (k < m && k < n - sa[mid] && text[sa[mid] + k] == pattern[k]) { ++k; }
                    if (k == m) { cmp = 0; }
                    else if (k == n - sa[mid]) { cmp = -1; }
                    else { cmp = text[sa[mid] + k] < pattern[k] ? -1 : 1; }
                    if (k >= matched) { matched = k; anchor = mid; }}
                if (cmp < 0 || (upper && !cmp)) { l = mid + 1; }
                else { r = mid; }}
            return l;};
        return {bound(false), bound(true)};}
    pair<int, int> patternRange(std::initializer_list<T> pattern) const { return patternRange(vector<T>(pattern)); }
    pair<int, int> patternRange(string_view pattern) const requires std::is_same_v<T, int> {
        assert(pattern.size() < INT_MAX);
        vector<int> p(pattern.begin(), pattern.end());
        for (int &c : p) { c = uint8_t(c); }
        return patternRange(p);}

    // T: O(n), M: O(min(k, n)); {start, length}: smallest longest substring with >= k (overlapping) occurrences, earliest start; {0, 0} if none.
    pair<int, int> longestRepeated(int k = 2) const {
        assert(k >= 1);
        int n = size(), start = 0, length = 0;
        if (k == 1 || k > n) { return {0, k == 1 ? n : 0}; }
        deque<int> window;
        for (int i = 0; i < n - 1; ++i) {
            while (!window.empty() && lcp[window.back()] >= lcp[i]) { window.pop_back(); }
            window.push_back(i);
            if (window.front() <= i - (k - 1)) { window.pop_front(); }
            if (i >= k - 2 && lcp[window.front()] > length) { length = lcp[window.front()]; start = sa[i]; }}
        if (length) {
            int at = rank[start];
            while (at && lcp[at - 1] >= length) { start = min(start, sa[--at]); }
            while (at < n - 1 && lcp[at] >= length) { start = min(start, sa[++at]); }}
        return {start, length};}
    // T: O(n * log(n + 1)), M: O(1); {first, last, length}: smallest longest substring at two disjoint starts, extreme starts; {0, 0, 0} if none.
    tuple<int, int, int> longestRepeatedDisjoint() const {
        int n = size(), x = 0, y = 0;
        auto find = [&](int len) {
            for (int i = 0, j = 0; i < n; i = ++j) {
                x = y = sa[i];
                for (; j < n - 1 && lcp[j] >= len; ++j) { x = min(x, sa[j + 1]); y = max(y, sa[j + 1]); }
                if (y - x >= len) { return true; }}
            return false;};
        int lo = 0, hi = n / 2 + 1;
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            (find(mid) ? lo : hi) = mid;}
        if (!lo) { return {0, 0, 0}; }
        find(lo);
        return {x, y, lo};}
};
SuffixArray(string_view, bool = true) -> SuffixArray<int>;
SuffixArray(const string &, bool = true) -> SuffixArray<int>;
SuffixArray(const char *, bool = true) -> SuffixArray<int>;

// T: O((n + m) * log(n + m + 1)), M: O(n + m); n + m + 1 < INT_MAX.
// {start in a, start in b, length} of the smallest longest common substring, earliest starts; {0, 0, 0} if none.
template<class T> requires std::is_integral_v<T>
tuple<int, int, int> longestCommonSubstring(const vector<T> &a, const vector<T> &b) {
    assert(a.size() < INT_MAX && b.size() < INT_MAX && a.size() + b.size() < INT_MAX - 1);
    if (a.empty() || b.empty()) { return {0, 0, 0}; }
    int n = int(a.size()), m = int(b.size());
    vector<T> alphabet = a;
    alphabet.insert(alphabet.end(), b.begin(), b.end());
    sort(alphabet.begin(), alphabet.end());
    alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
    auto code = [&](T c) { return int(lower_bound(alphabet.begin(), alphabet.end(), c) - alphabet.begin()) + 1; };
    vector<int> joined;
    joined.reserve(n + m + 1);
    for (T c : a) { joined.push_back(code(c)); }
    joined.push_back(0);
    for (T c : b) { joined.push_back(code(c)); }
    SuffixArray index(std::move(joined), false);
    int best = 0, first = 0;
    for (int i = 0; i < n + m; ++i) {
        int x = index.sa[i], y = index.sa[i + 1];
        if (x != n && y != n && (x < n) != (y < n) && index.lcp[i] > best) { best = index.lcp[i]; first = i; }}
    if (!best) { return {0, 0, 0}; }
    int x = n, y = m, last = first;
    while (first && index.lcp[first - 1] >= best) { --first; }
    while (last < n + m && index.lcp[last] >= best) { ++last; }
    for (int i = first; i <= last; ++i) {
        int p = index.sa[i];
        if (p < n) { x = min(x, p); }
        else if (p > n) { y = min(y, p - n - 1); }}
    return {x, y, best};}
// T: O((n + m) * log(n + m + 1)), M: O(n + m); bytes ordered unsigned.
inline tuple<int, int, int> longestCommonSubstring(string_view a, string_view b) {
    vector<int> x(a.begin(), a.end()), y(b.begin(), b.end());
    for (int &c : x) { c = uint8_t(c); }
    for (int &c : y) { c = uint8_t(c); }
    return longestCommonSubstring(x, y);}
