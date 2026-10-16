#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/04-sparsetable.hpp"
#include "07-suffixarray.hpp"
#include "14-lyndon.hpp"

namespace lexicographic_detail {
    template<class C> auto key(C c) {
        if constexpr (std::is_same_v<C, char>) { return int(uint8_t(c)); }
        else { return c; }}
    inline vector<int> bytes(string_view s) {
        assert(s.size() < INT_MAX);
        vector<int> res(s.begin(), s.end());
        for (int &c : res) { c = uint8_t(c); }
        return res;}
    // res[x] = sum over ranks y > x of min(lcp[x, y)), by next strictly smaller lcp.
    inline vector<lng> later(const vector<int> &lcp, int n) {
        vector<lng> res(n);
        vector<int> st;
        for (int x = n - 2; x >= 0; --x) {
            while (!st.empty() && lcp[st.back()] >= lcp[x]) { st.pop_back(); }
            int next = st.empty() ? n - 1 : st.back();
            res[x] = lng(lcp[x]) * (next - x) + res[next];
            st.push_back(x);}
        return res;}
    // res[x] = sum over ranks y < x of min(lcp[y, x)).
    inline vector<lng> earlier(const vector<int> &lcp, int n) {
        vector<lng> res(n), sum(max(n - 1, 0));
        vector<int> st;
        for (int e = 0; e + 1 < n; ++e) {
            while (!st.empty() && lcp[st.back()] >= lcp[e]) { st.pop_back(); }
            int prev = st.empty() ? -1 : st.back();
            sum[e] = lng(lcp[e]) * (e - prev) + (prev >= 0 ? sum[prev] : 0);
            st.push_back(e);
            res[e + 1] = sum[e];}
        return res;}
    // res[i] = length of the shortest substring starting at i that occurs once, 0 when the suffix repeats.
    template<class T> vector<int> shortestUnique(const SuffixArray<T> &sa) {
        int n = sa.size();
        vector<int> res(n);
        for (int x = 0; x < n; ++x) {
            int len = max(x ? sa.lcp[x - 1] : 0, x + 1 < n ? sa.lcp[x] : 0) + 1;
            res[sa.sa[x]] = len <= n - sa.sa[x] ? len : 0;}
        return res;}
} // namespace lexicographic_detail

// T: O(1), M: O(1); sa with RMQ; -1, 0, 1 as s[l, r) is less, equal or greater than s[a, b), a proper prefix being less.
template<class T> int compareSubstrings(const SuffixArray<T> &sa, int l, int r, int a, int b) {
    int k = sa.substringLce(l, r, a, b);
    if (k == r - l || k == b - a) { return (r - l > b - a) - (r - l < b - a); }
    return sa.text[l + k] < sa.text[a + k] ? -1 : 1;}

// S: O(n * log(n + 1)), U: NA, Q: O(1), M: O(n * log(n + 1)); integral T (string input: bytes as T = int); forward and reversed suffix arrays.
// longestCommonSuffix(l, r, a, b) is the longest common suffix of s[l, r) and s[a, b).
template<class T = int> requires std::is_integral_v<T>
struct BidirectionalSuffixArray {
    SuffixArray<T> forward, backward;

    explicit BidirectionalSuffixArray(const vector<T> &s) : forward(s), backward(vector<T>(s.rbegin(), s.rend())) {}
    explicit BidirectionalSuffixArray(string_view s) requires std::is_same_v<T, int> : forward(s), backward(string(s.rbegin(), s.rend())) {}

    int longestCommonSuffix(int l, int r, int a, int b) const {
        int n = forward.size();
        assert(0 <= l && l <= r && r <= n && 0 <= a && a <= b && b <= n);
        return backward.substringLce(n - r, n - l, n - b, n - a);}
};
BidirectionalSuffixArray(string_view) -> BidirectionalSuffixArray<int>;
BidirectionalSuffixArray(const string &) -> BidirectionalSuffixArray<int>;
BidirectionalSuffixArray(const char *) -> BidirectionalSuffixArray<int>;

// S: O(L * log(L + 1)), U: NA, Q: O(1) lce/compare, O(x + y) comparePieces, M: O(L * log(L + 1)); L = total length plus string count < INT_MAX.
// Strings share one symbol type (char as unsigned bytes); lce(i, l, j, a) of suffixes strings[i][l, ..) and strings[j][a, ..); piece {i, l, r} = strings[i][l, r).
struct StringCollection {
    SuffixArray<int> index;
    vector<int> start;

    template<class S> explicit StringCollection(const vector<S> &strings) {
        using K = decltype(lexicographic_detail::key(std::declval<typename S::value_type>()));
        int k = int(strings.size());
        vector<K> alphabet;
        for (const S &s : strings) {
            for (auto c : s) { alphabet.push_back(lexicographic_detail::key(c)); }}
        sort(alphabet.begin(), alphabet.end());
        alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
        assert(alphabet.size() < size_t(INT_MAX) - k);
        vector<int> joined;
        for (int i = 0; i < k; ++i) {
            start.push_back(int(joined.size()));
            assert(strings[i].size() < size_t(INT_MAX) - joined.size() - 1);
            for (auto c : strings[i]) { joined.push_back(k + int(lower_bound(alphabet.begin(), alphabet.end(), lexicographic_detail::key(c)) - alphabet.begin())); }
            joined.push_back(i);}
        start.push_back(int(joined.size()));
        index = SuffixArray<int>(std::move(joined));}

    int length(int i) const {
        assert(0 <= i && i + 1 < int(start.size()));
        return start[i + 1] - start[i] - 1;}
    int lce(int i, int l, int j, int a) const {
        assert(0 <= l && l <= length(i) && 0 <= a && a <= length(j));
        return min({index.lce(start[i] + l, start[j] + a), length(i) - l, length(j) - a});}
    int compare(int i, int l, int r, int j, int a, int b) const {
        assert(l <= r && r <= length(i) && a <= b && b <= length(j));
        int k = min({lce(i, l, j, a), r - l, b - a});
        if (k == r - l || k == b - a) { return (r - l > b - a) - (r - l < b - a); }
        return index.text[start[i] + l + k] < index.text[start[j] + a + k] ? -1 : 1;}
    int comparePieces(const vector<array<int, 3>> &x, const vector<array<int, 3>> &y) const {
        [[maybe_unused]] auto valid = [&](const vector<array<int, 3>> &z) {
            return z.size() < INT_MAX && std::ranges::all_of(z, [&](const array<int, 3> &e) { return 0 <= e[1] && e[1] <= e[2] && e[2] <= length(e[0]); });};
        assert(valid(x) && valid(y));
        int p = 0, q = 0, u = 0, v = 0, nx = int(x.size()), ny = int(y.size());
        auto skip = [&](const vector<array<int, 3>> &z, int &t, int &off) {
            while (t < int(z.size()) && z[t][1] + off == z[t][2]) { ++t; off = 0; }};
        for (;;) {
            skip(x, p, u); skip(y, q, v);
            if (p == nx || q == ny) { return (p < nx) - (q < ny); }
            auto [i, l, r] = x[p];
            auto [j, a, b] = y[q];
            int len = min(r - l - u, b - a - v), k = min(lce(i, l + u, j, a + v), len);
            if (k < len) { return index.text[start[i] + l + u + k] < index.text[start[j] + a + v + k] ? -1 : 1; }
            u += len; v += len;}}
};

// S: O(n * log(n + 1)), U: NA, Q: O(1) equal/compare, O(log(n + 1)) lcp, M: O(n * log(n + 1)); integral T (string input: bytes), n < INT_MAX.
// names[k][i] = dense rank of s[i, i + 2^k) among factors of length 2^k; compare as compareSubstrings; lcp of suffixes i and j.
template<class T = int> requires std::is_integral_v<T>
struct DictionaryOfBasicFactors {
    int n;
    vector<vector<int>> names;

    explicit DictionaryOfBasicFactors(const vector<T> &s) : n(int(s.size())) {
        assert(s.size() < INT_MAX);
        if (!n) { return; }
        vector<T> sorted = s;
        sort(sorted.begin(), sorted.end());
        sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
        names.emplace_back(n);
        for (int i = 0; i < n; ++i) { names[0][i] = int(lower_bound(sorted.begin(), sorted.end(), s[i]) - sorted.begin()); }
        vector<int> order(n), tmp(n), count(n + 1);
        for (int h = 1; 2 * h <= n; h *= 2) {
            const vector<int> &cur = names.back();
            int m = n - 2 * h + 1;
            for (int pass = 1; pass >= 0; --pass) {
                fill(count.begin(), count.end(), 0);
                for (int i = 0; i < m; ++i) { ++count[cur[i + pass * h] + 1]; }
                for (int c = 1; c <= n; ++c) { count[c] += count[c - 1]; }
                for (int t = 0; t < m; ++t) {
                    int i = pass ? t : tmp[t];
                    order[count[cur[i + pass * h]]++] = i;}
                if (pass) { std::copy(order.begin(), order.begin() + m, tmp.begin()); }}
            vector<int> next(m);
            for (int t = 0, rank = -1; t < m; ++t) {
                int i = order[t], j = t ? order[t - 1] : -1;
                if (j < 0 || cur[i] != cur[j] || cur[i + h] != cur[j + h]) { ++rank; }
                next[i] = rank;}
            names.push_back(std::move(next));}}
    explicit DictionaryOfBasicFactors(string_view s) requires std::is_same_v<T, int> : DictionaryOfBasicFactors(lexicographic_detail::bytes(s)) {}

    int size() const { return n; }
    int name(int k, int i) const {
        assert(0 <= k && k < int(names.size()) && 0 <= i && i < int(names[k].size()));
        return names[k][i];}
    bool equal(int l, int r, int a, int b) const {
        assert(0 <= l && l <= r && r <= n && 0 <= a && a <= b && b <= n);
        if (r - l != b - a) { return false; }
        if (l == r) { return true; }
        int k = std::bit_width(uint(r - l)) - 1;
        return names[k][l] == names[k][a] && names[k][r - (1 << k)] == names[k][b - (1 << k)];}
    int compare(int l, int r, int a, int b) const {
        assert(0 <= l && l <= r && r <= n && 0 <= a && a <= b && b <= n);
        int len = min(r - l, b - a);
        if (len) {
            int k = std::bit_width(uint(len)) - 1, d = len - (1 << k);
            auto x = std::make_pair(names[k][l], names[k][l + d]), y = std::make_pair(names[k][a], names[k][a + d]);
            if (x != y) { return x < y ? -1 : 1; }}
        return (r - l > b - a) - (r - l < b - a);}
    int lcp(int i, int j) const {
        assert(0 <= i && i <= n && 0 <= j && j <= n);
        if (i == j) { return n - i; }
        int res = 0;
        for (int k = int(names.size()) - 1; k >= 0; --k) {
            int h = 1 << k;
            if (max(i, j) + res + h <= n && names[k][i + res] == names[k][j + res]) { res += h; }}
        return res;}
};
DictionaryOfBasicFactors(string_view) -> DictionaryOfBasicFactors<int>;
DictionaryOfBasicFactors(const string &) -> DictionaryOfBasicFactors<int>;
DictionaryOfBasicFactors(const char *) -> DictionaryOfBasicFactors<int>;

// T: O(log(n + 1)), M: O(1); sa with RMQ; ranks [lo, hi) of the suffixes starting with s[l, r), [0, n) for the empty range.
template<class T> pair<int, int> occurrenceRange(const SuffixArray<T> &sa, int l, int r) {
    int n = sa.size();
    assert(sa.indexed && 0 <= l && l <= r && r <= n);
    if (l == r) { return {0, n}; }
    int p = sa.rank[l], len = r - l, a = -1, b = p;
    while (b - a > 1) {
        int mid = std::midpoint(a, b);
        (sa.rmq.query(mid, p) >= len ? b : a) = mid;}
    int lo = b;
    a = p; b = n;
    while (b - a > 1) {
        int mid = std::midpoint(a, b);
        (sa.rmq.query(p, mid) >= len ? a : b) = mid;}
    return {lo, a + 1};}
// T: O(log(n + 1)), M: O(1); occurrences of s[l, r) as a substring, overlaps included; n + 1 for the empty range.
template<class T> int occurrenceCount(const SuffixArray<T> &sa, int l, int r) {
    auto [lo, hi] = occurrenceRange(sa, l, r);
    return hi - lo + (l == r);}

// T: O(q * log(n + q + 1)), M: O(q); sa with RMQ; res[i] = dense rank of s[l_i, r_i) among the given substrings (equal strings share a rank).
template<class T> vector<int> sortSubstrings(const SuffixArray<T> &sa, const vector<pair<int, int>> &substrings) {
    int q = int(substrings.size());
    vector<pair<int, int>> keys(q);
    vector<int> order(q), res(q);
    for (int i = 0; i < q; ++i) {
        auto [l, r] = substrings[i];
        keys[i] = {occurrenceRange(sa, l, r).first, r - l};}
    iota(order.begin(), order.end(), 0);
    sort(order.begin(), order.end(), [&](int i, int j) { return keys[i] < keys[j]; });
    for (int t = 0, rank = -1; t < q; ++t) {
        if (!t || keys[order[t]] != keys[order[t - 1]]) { ++rank; }
        res[order[t]] = rank;}
    return res;}

// S: O(n * log(n + 1)), U: NA, Q: O(log(n + 1)) ranks, O(log(n + 1)^2) kth, M: O(n * log(n + 1)); integral T (string input: bytes).
// k 0-based over distinct or all n(n+1)/2 nonempty substrings: {leftmost start, length}, {-1, 0} out of range; ranks count smaller ones.
template<class T = int> requires std::is_integral_v<T>
struct SubstringOrder {
    using Minimum = typename SuffixArray<T>::Minimum;
    SuffixArray<T> index;
    SparseTable<int, Minimum> leftmost{{}, {}};
    vector<lng> distinct, total, later;

    explicit SubstringOrder(vector<T> s) : index(std::move(s)) { init(); }
    explicit SubstringOrder(string_view s) requires std::is_same_v<T, int> : index(s) { init(); }
    void init() {
        int n = index.size();
        leftmost = SparseTable<int, Minimum>(index.sa, {});
        distinct.assign(n + 1, 0); total.assign(n + 1, 0);
        for (int r = 0; r < n; ++r) {
            int len = n - index.sa[r];
            distinct[r + 1] = distinct[r] + len - (r ? index.lcp[r - 1] : 0);
            total[r + 1] = total[r] + len;}
        later = lexicographic_detail::later(index.lcp, n);}

    // Occurrences of strings at most X, where [p, q) are the ranks of X and len = |X| > 0.
    lng atMost(int p, int q, int len) const { return total[p] + lng(q - p) * len + later[q - 1]; }
    pair<int, int> kthDistinctSubstring(lng k) const {
        int n = index.size();
        if (k < 0 || k >= distinct[n]) { return {-1, 0}; }
        int r = int(upper_bound(distinct.begin(), distinct.end(), k) - distinct.begin()) - 1, s = index.sa[r];
        int len = (r ? index.lcp[r - 1] : 0) + 1 + int(k - distinct[r]);
        auto [p, q] = occurrenceRange(index, s, s + len);
        return {leftmost.query(p, q), len};}
    pair<int, int> kthSubstring(lng k) const {
        int n = index.size();
        if (k < 0 || k >= total[n]) { return {-1, 0}; }
        auto count = [&](int s, int len) { auto [p, q] = occurrenceRange(index, s, s + len); return atMost(p, q, len); };
        int lo = -1, hi = n - 1;
        while (hi - lo > 1) {
            int mid = std::midpoint(lo, hi);
            (count(index.sa[mid], n - index.sa[mid]) > k ? hi : lo) = mid;}
        int s = index.sa[hi], a = hi ? index.lcp[hi - 1] : 0, b = n - s;
        while (b - a > 1) {
            int mid = std::midpoint(a, b);
            (count(s, mid) > k ? b : a) = mid;}
        auto [p, q] = occurrenceRange(index, s, s + b);
        return {leftmost.query(p, q), b};}
    lng distinctSubstringRank(int l, int r) const {
        auto [p, q] = occurrenceRange(index, l, r);
        return l == r ? 0 : distinct[p] + (r - l) - (p ? index.lcp[p - 1] : 0) - 1;}
    lng substringRank(int l, int r) const {
        auto [p, q] = occurrenceRange(index, l, r);
        return l == r ? 0 : atMost(p, q, r - l) - (q - p);}
};
SubstringOrder(string_view) -> SubstringOrder<int>;
SubstringOrder(const string &) -> SubstringOrder<int>;
SubstringOrder(const char *) -> SubstringOrder<int>;

// T: O(n), M: O(n); res[k] = distinct nonempty substrings occurring exactly k times (overlaps counted), n + 1 entries.
template<class T> vector<lng> frequencyStatistics(const SuffixArray<T> &sa) {
    int n = sa.size();
    vector<lng> res(n + 1);
    for (int x = 0; x < n; ++x) { res[1] += n - sa.sa[x] - max(x ? sa.lcp[x - 1] : 0, x + 1 < n ? sa.lcp[x] : 0); }
    vector<pair<int, int>> st{{0, 0}};
    for (int i = 1; i <= n; ++i) {
        int h = i < n ? sa.lcp[i - 1] : 0, left = i - 1;
        while (st.back().first > h) {
            auto [d, l] = st.back();
            st.pop_back();
            res[i - l] += d - max(h, st.back().first);
            left = l;}
        if (st.back().first < h) { st.push_back({h, left}); }}
    return res;}

// T: O(n), M: O(n); res[i] = sum over all j in [0, n) of lce(i, j), j = i included.
template<class T> vector<lng> lcpSums(const SuffixArray<T> &sa) {
    int n = sa.size();
    vector<lng> right = lexicographic_detail::later(sa.lcp, n), left = lexicographic_detail::earlier(sa.lcp, n), res(n);
    for (int x = 0; x < n; ++x) { res[sa.sa[x]] = n - sa.sa[x] + left[x] + right[x]; }
    return res;}

// T: O(n + S + out), M: O(n + S + out); symbols in [0, S); {b, start, length} is the word b s[start, start + length), start leftmost.
// Every minimal absent word once (single absent symbols as {b, 0, 0}), in traversal order.
template<class T> vector<tuple<T, int, int>> minimalAbsentWords(const SuffixArray<T> &sa, int sigma) {
    int n = sa.size(), stamp = 0;
    const vector<T> &s = sa.text;
    assert(sigma >= 1 && std::ranges::all_of(s, [&](T c) { return 0 <= lll(c) && lll(c) < sigma; }));
    struct Node { int depth, rep; vector<T> left; vector<int> kids; };
    vector<Node> pool;
    vector<int> mark(sigma, -1), st;
    vector<tuple<T, int, int>> res;
    auto make = [&](int depth, int rep) { pool.push_back({depth, rep, {}, {}}); return int(pool.size()) - 1; };
    auto attach = [&](int v, int c) {
        pool[v].rep = min(pool[v].rep, pool[c].rep);
        if (pool[c].depth > pool[v].depth) { pool[v].kids.push_back(c); }
        else { pool[v].left.insert(pool[v].left.end(), pool[c].left.begin(), pool[c].left.end()); }};
    // The node's left set is the union over kids and own suffixes; b P a is absent exactly when b misses kid a's left set.
    auto finish = [&](int v) {
        vector<T> all;
        ++stamp;
        auto add = [&](const vector<T> &from) {
            for (T c : from) {
                if (mark[int(c)] != stamp) { mark[int(c)] = stamp; all.push_back(c); }}};
        add(pool[v].left);
        for (int c : pool[v].kids) { add(pool[c].left); }
        for (int c : pool[v].kids) {
            ++stamp;
            for (T b : pool[c].left) { mark[int(b)] = stamp; }
            for (T b : all) {
                if (mark[int(b)] != stamp) { res.emplace_back(b, pool[c].rep, pool[v].depth + 1); }}
            vector<T>().swap(pool[c].left);}
        pool[v].left = std::move(all);};
    st.push_back(make(0, n));
    for (int x = 0; x < n; ++x) {
        int cur = make(n - sa.sa[x], sa.sa[x]), h = x + 1 < n ? sa.lcp[x] : 0;
        if (sa.sa[x]) { pool[cur].left.push_back(s[sa.sa[x] - 1]); }
        while (pool[st.back()].depth > h) {
            int v = st.back();
            st.pop_back();
            attach(v, cur); finish(v);
            cur = v;}
        if (pool[st.back()].depth < h) {
            int v = make(h, n);
            attach(v, cur);
            st.push_back(v);}
        else { attach(st.back(), cur); }}
    if (n) { pool[st[0]].left.push_back(s[n - 1]); }
    finish(st[0]);
    ++stamp;
    for (T c : s) { mark[int(c)] = stamp; }
    for (int c = 0; c < sigma; ++c) {
        if (mark[c] != stamp) { res.emplace_back(T(c), 0, 0); }}
    return res;}

// T: O(n), M: O(n); {start, length} ascending: substrings occurring once whose proper substrings all repeat.
template<class T> vector<pair<int, int>> minimalUniqueSubstrings(const SuffixArray<T> &sa) {
    int n = sa.size();
    vector<int> len = lexicographic_detail::shortestUnique(sa);
    vector<pair<int, int>> res;
    for (int i = 0; i < n; ++i) {
        if (len[i] && (i + 1 == n || !len[i + 1] || len[i + 1] >= len[i])) { res.push_back({i, len[i]}); }}
    return res;}

// T: O(n), M: O(n); res[p] = {start, length} of the shortest substring covering p that occurs once, leftmost on ties.
template<class T> vector<pair<int, int>> shortestUniqueSubstrings(const SuffixArray<T> &sa) {
    vector<pair<int, int>> mus = minimalUniqueSubstrings(sa), res(sa.size());
    int k = int(mus.size()), add = 0, done = 0;
    deque<int> window;
    for (int p = 0; p < sa.size(); ++p) {
        for (; add < k && mus[add].first <= p; ++add) {
            while (!window.empty() && mus[window.back()].second > mus[add].second) { window.pop_back(); }
            window.push_back(add);}
        while (!window.empty() && mus[window.front()].first + mus[window.front()].second <= p) { window.pop_front(); }
        while (done < k && mus[done].first + mus[done].second <= p) { ++done; }
        pair<int, int> best{INT_MAX, INT_MAX};
        if (!window.empty()) { best = {mus[window.front()].second, mus[window.front()].first}; }
        if (done) { best = min(best, {p - mus[done - 1].first + 1, mus[done - 1].first}); }
        if (add < k) { best = min(best, {mus[add].first + mus[add].second - p, p}); }
        res[p] = {best.second, best.first};}
    return res;}

// T: O(n), M: O(n); random-access sequence (char as unsigned bytes); res[i] = start of the smallest nonempty suffix of s[0, i), res[0] = 0.
template<class S> vector<int> lexMinSuffixPerPrefix(const S &s) {
    assert(s.size() < INT_MAX);
    IncrementalLyndon<decltype(lexicographic_detail::key(s[0]))> lyndon;
    vector<int> res{0};
    for (auto c : s) {
        lyndon.add(lexicographic_detail::key(c));
        res.push_back(lyndon.size() - lyndon.minSuffix(lyndon.size()));}
    return res;}

// T: O(n), M: O(n); as lexMinSuffixPerPrefix for the largest suffix; online over the border chain of the current maximal suffix.
template<class S> vector<int> lexMaxSuffixPerPrefix(const S &s) {
    assert(s.size() < INT_MAX);
    int n = int(s.size());
    auto at = [&](int i) { return lexicographic_detail::key(s[i]); };
    vector<int> res(n + 1), pi;
    // Borders of the maximal suffix x = s[top, i) carry nondecreasing next symbols as they shorten, so one comparison decides each step.
    for (int i = 0, top = 0; i < n; ++i) {
        if (!i) { pi.push_back(0); continue; }
        int z = pi[i - top - 1];
        if (at(top + z) < at(i)) {
            while (z && at(top + pi[z - 1]) < at(i)) { z = pi[z - 1]; }
            int next = z ? pi[z - 1] : 0, old = top;
            top = i - z;
            pi.resize(z);
            pi.push_back(z && at(old + next) == at(i) ? next + 1 : 0);}
        else { pi.push_back(at(top + z) == at(i) ? z + 1 : 0); }
        res[i + 1] = top;}
    return res;}

// T: O(n), M: O(n); sa with RMQ; res[i] = distinct nonempty substrings of s[0, i), n + 1 entries.
template<class T> vector<lng> distinctSubstringsPerPrefix(const SuffixArray<T> &sa) {
    int n = sa.size();
    vector<int> seen(n), st, bucket(n + 1);
    for (int pass = 0; pass < 2; ++pass) {
        st.clear();
        for (int t = 0; t < n; ++t) {
            int x = pass ? n - 1 - t : t, j = sa.sa[x];
            while (!st.empty() && sa.sa[st.back()] > j) { st.pop_back(); }
            if (!st.empty()) { seen[j] = max(seen[j], sa.lce(sa.sa[st.back()], j)); }
            st.push_back(x);}}
    for (int j = 0; j < n; ++j) { ++bucket[j + seen[j]]; }
    vector<lng> res(n + 1);
    lng count = 0, sum = 0;
    for (int i = 0; i <= n; ++i) {
        res[i] = lng(i) * count - sum;
        count += bucket[i]; sum += lng(i) * bucket[i];}
    return res;}
