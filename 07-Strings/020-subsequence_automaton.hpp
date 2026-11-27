#pragma once
#include "../01-Core/01-template.hpp"

namespace subsequence_detail {
    template<class C> auto key(C c) {
        if constexpr (std::is_same_v<C, char>) { return uint8_t(c); }
        else { return c; }}
    template<class C> bool fits(C c, int sigma) { return std::cmp_greater_equal(key(c), 0) && std::cmp_less(key(c), sigma); }
    template<class T> T add(T a, T b) {
        if constexpr (std::is_integral_v<T>) { return a > std::numeric_limits<T>::max() - b ? std::numeric_limits<T>::max() : T(a + b); }
        else { return a + b; }}
} // namespace subsequence_detail

// S: O(n * S) dense, O(n + S) sparse, Q: next/prev/step O(1) dense, O(log(n)) sparse, M: O(n * S) dense, O(n + S) sparse.
// Text symbols in [0, S) (char as unsigned), n < INT_MAX; next gives n and prev -1 when absent; state n + 1 is dead.
template<bool DENSE> struct BasicSubsequenceAutomaton {
    int n = 0, sigma = 0;
    vector<int> text, same, nxt, prv;
    vector<vector<int>> pos;

    BasicSubsequenceAutomaton() = default;
    template<class S> explicit BasicSubsequenceAutomaton(const S &s, int size = 256) { build(s, size); }
    template<class S> void build(const S &s, int size = 256) {
        assert(s.size() < INT_MAX && size >= 1);
        assert(std::ranges::all_of(s, [&](auto c) { return subsequence_detail::fits(c, size); }));
        n = int(s.size()); sigma = size;
        text.resize(n); same.resize(n);
        for (int i = 0; i < n; ++i) { text[i] = int(subsequence_detail::key(s[i])); }
        vector<int> last(sigma, n);
        for (int i = n - 1; i >= 0; --i) { same[i] = std::exchange(last[text[i]], i); }
        if constexpr (DENSE) {
            nxt.assign(lng(n + 1) * sigma, n); prv.assign(lng(n + 1) * sigma, -1);
            for (int i = n - 1; i >= 0; --i) {
                std::copy_n(nxt.begin() + lng(i + 1) * sigma, sigma, nxt.begin() + lng(i) * sigma);
                nxt[lng(i) * sigma + text[i]] = i;}
            for (int i = 0; i < n; ++i) {
                std::copy_n(prv.begin() + lng(i) * sigma, sigma, prv.begin() + lng(i + 1) * sigma);
                prv[lng(i + 1) * sigma + text[i]] = i;}}
        else {
            pos.assign(sigma, {});
            for (int i = 0; i < n; ++i) { pos[text[i]].push_back(i); }}}

    // Positions of c below i; a hand-written search avoids the O(n) partition check of debug-mode lower_bound.
    int rank(int i, int c) const {
        int lo = 0, hi = int(pos[c].size());
        while (lo < hi) {
            int mid = (lo + hi) / 2;
            if (pos[c][mid] < i) { lo = mid + 1; }
            else { hi = mid; }}
        return lo;}
    int next(int i, int c) const {
        assert(0 <= i && i <= n && 0 <= c && c < sigma);
        if constexpr (DENSE) { return nxt[lng(i) * sigma + c]; }
        else {
            int r = rank(i, c);
            return r < int(pos[c].size()) ? pos[c][r] : n;}}
    int prev(int i, int c) const {
        assert(0 <= i && i <= n && 0 <= c && c < sigma);
        if constexpr (DENSE) { return prv[lng(i) * sigma + c]; }
        else {
            int r = rank(i, c);
            return r ? pos[c][r - 1] : -1;}}
    template<class C> int step(int state, C c) const {
        assert(0 <= state && state <= n + 1);
        if (state > n || !subsequence_detail::fits(c, sigma)) { return n + 1; }
        return next(state, int(subsequence_detail::key(c))) + 1;}
    // T: O(m) dense, O(m * log(n)) sparse; t is a subsequence of text[l, r), r = -1 means n.
    template<class P> bool isSubsequence(const P &t, int l = 0, int r = -1) const {
        if (r == -1) { r = n; }
        assert(0 <= l && l <= r && r <= n);
        int state = l;
        for (int i = 0; i < int(t.size()) && state <= r; ++i) { state = step(state, t[i]); }
        return state <= r;}

    // T: O(n), M: O(n); res[i] = distinct subsequences of text[i, n), empty included; integral T saturates, other T is a ring.
    template<class T = lng> vector<T> countDistinctSubsequences() const {
        vector<T> res(n + 2, T(0));
        res[n] = T(1);
        for (int i = n - 1; i >= 0; --i) {
            T x = res[i + 1], y = res[same[i] + 1];
            if constexpr (std::is_integral_v<T>) { res[i] = x == std::numeric_limits<T>::max() ? x : subsequence_detail::add(x, T(x - y)); }
            else { res[i] = x + x - y; }}
        res.pop_back();
        return res;}
    // T: O(n + |res| * S) dense, O(n + |res| * S * log(n)) sparse; 0-based in lexicographic order (k = 0 is empty); false clears res.
    bool kthDistinctSubsequence(lng k, vector<int> &res) const {
        auto cnt = countDistinctSubsequences<lng>();
        res.clear();
        if (k < 0 || k >= cnt[0]) { return false; }
        for (int i = 0; k--;) {
            for (int c = 0; c < sigma; ++c) {
                int j = next(i, c);
                if (j == n) { continue; }
                if (k < cnt[j + 1]) { res.push_back(c); i = j + 1; break; }
                k -= cnt[j + 1];}}
        return true;}
    // T: O(n + S) dense, O(n + S + |res| * log(n)) sparse; lexicographically smallest among the shortest absent sequences over [0, S).
    vector<int> shortestAbsentSubsequence() const {
        vector<int> len(n + 2), cnt(sigma), res;
        len[n] = 1;
        // Window [i, hi) is the shortest prefix of text[i, n) holding every symbol that text[i, n) holds.
        for (int i = n - 1, hi = n, distinct = 0; i >= 0; --i) {
            distinct += !cnt[text[i]]++;
            while (hi - 1 > i && cnt[text[hi - 1]] > 1) { --cnt[text[--hi]]; }
            len[i] = distinct == sigma ? len[hi] + 1 : 1;}
        vector<char> seen(sigma);
        for (int i = 0, q = 1, need = len[0]; i <= n; --need) {
            q = max(q, i + 1);
            while (len[q] >= need) { ++q; }
            for (int j = i; j < q - 1; ++j) { seen[text[j]] = 1; }
            int c = int(std::find(seen.begin(), seen.end(), 0) - seen.begin());
            for (int j = i; j < q - 1; ++j) { seen[text[j]] = 0; }
            res.push_back(c);
            i = next(i, c) + 1;}
        return res;}
    // T: O(n + m + S + r), r = pairs (i, j) with text[i] = t[j], M: O(m + S); index tuples, integral T saturates, other T is a ring.
    template<class T = lng, class P> T countSubsequenceOccurrences(const P &t) const {
        assert(t.size() < INT_MAX);
        int m = int(t.size());
        vector<T> dp(m + 1, T(0));
        vector<int> head(sigma, -1), link(m);
        dp[0] = T(1);
        for (int j = 0; j < m; ++j) {
            if (!subsequence_detail::fits(t[j], sigma)) { return T(0); }
            int c = int(subsequence_detail::key(t[j]));
            link[j] = std::exchange(head[c], j);}
        for (int i = 0; i < n; ++i) {
            for (int j = head[text[i]]; j >= 0; j = link[j]) { dp[j + 1] = subsequence_detail::add(dp[j + 1], dp[j]); }}
        return dp[m];}
};
using SubsequenceAutomaton = BasicSubsequenceAutomaton<false>;
using SubsequenceAutomatonDense = BasicSubsequenceAutomaton<true>;
