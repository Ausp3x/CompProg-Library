#pragma once
#include "../01-Core/01-template.hpp"
#include "02-z.hpp"
#include "03-stringhash.hpp"

namespace matching_detail {
    template<class C> auto key(C c) {
        if constexpr (std::is_same_v<C, char>) { return uint8_t(c); }
        else { return c; }}
    template<class C> int code(C c) { return int(key(c)); }
    template<class S> bool coded(const S &s, int sigma) {
        return std::ranges::all_of(s, [&](auto c) { return std::cmp_greater_equal(key(c), 0) && std::cmp_less(key(c), sigma); });}
    template<class P, class S> bool same(const P &p, const S &t, int i, int l, int r) {
        while (l < r && p[l] == t[i + l]) { ++l; }
        return l == r;}
    template<class P, class S, class F, class G> void run(const P &p, const S &t, F &f, G scan) {
        assert(p.size() < INT_MAX && t.size() < INT_MAX);
        int m = int(p.size()), n = int(t.size());
        if (!m) {
            for (int i = 0; i <= n && f(i); ++i) {}
            return;}
        if (m <= n) { scan(m, n); }}
    template<class P> vector<int> last(const P &p, int sigma, int len) {
        vector<int> res(sigma, -1);
        for (int i = 0; i < len; ++i) { res[code(p[i])] = i; }
        return res;}
    template<class P> vector<int> goodSuffix(const P &p) {
        int m = int(p.size());
        vector<std::decay_t<decltype(p[0])>> rev(p.rbegin(), p.rend());
        auto z = zFunction(rev);
        vector<int> gs(m, m);
        for (int i = m - 1, j = 0; i >= 0; --i) {
            if (z[m - 1 - i] != i + 1) { continue; }
            for (; j < m - 1 - i; ++j) {
                if (gs[j] == m) { gs[j] = m - 1 - i; }}}
        for (int i = 0; i + 1 < m; ++i) { gs[m - 1 - z[m - 1 - i]] = m - 1 - i; }
        return gs;}
    template<class P, class S, class F> void naive(const P &p, const S &t, F f) {
        run(p, t, f, [&](int m, int n) {
            for (int i = 0; i <= n - m; ++i) {
                if (same(p, t, i, 0, m) && !f(i)) { return; }}});}
    template<class P, class S, class F> void rabinKarp(const P &p, const S &t, ulng seed, F f) {
        using H = StringHash61;
        auto enc = [](auto c) { return H::encode(ulng(key(c))); };
        ulng base = std::mt19937_64(seed)() % (H::P61 - 512) + 256;
        run(p, t, f, [&](int m, int n) {
            ulng hp = 0, ht = 0, top = 1;
            for (int i = 0; i < m; ++i) {
                hp = H::add(H::multiply(hp, base), enc(p[i]));
                ht = H::add(H::multiply(ht, base), enc(t[i]));
                if (i) { top = H::multiply(top, base); }}
            for (int i = 0;; ++i) {
                if (hp == ht && same(p, t, i, 0, m) && !f(i)) { return; }
                if (i == n - m) { return; }
                ht = H::add(H::multiply(H::subtract(ht, H::multiply(enc(t[i]), top)), base), enc(t[i + m]));}});}
    template<class P, class S, class F> void boyerMoore(const P &p, const S &t, int sigma, F f) {
        run(p, t, f, [&](int m, int n) {
            auto bad = last(p, sigma, m);
            auto gs = goodSuffix(p);
            // Galil: after a match the window moves by the period and its first m - period symbols are known.
            for (int i = 0, bound = 0; i <= n - m;) {
                int j = m - 1;
                while (j >= bound && p[j] == t[i + j]) { --j; }
                if (j < bound) {
                    if (!f(i)) { return; }
                    i += gs[0];
                    bound = m - gs[0];}
                else {
                    i += max(gs[j], j - bad[code(t[i + j])]);
                    bound = 0;}}});}
    template<class P, class S, class F> void horspool(const P &p, const S &t, int sigma, F f) {
        run(p, t, f, [&](int m, int n) {
            auto bad = last(p, sigma, m - 1);
            for (int i = 0; i <= n - m; i += m - 1 - bad[code(t[i + m - 1])]) {
                if (p[m - 1] == t[i + m - 1] && same(p, t, i, 0, m - 1) && !f(i)) { return; }}});}
    template<class P, class S, class F> void sunday(const P &p, const S &t, int sigma, F f) {
        run(p, t, f, [&](int m, int n) {
            auto bad = last(p, sigma, m);
            for (int i = 0; i <= n - m; i += m - bad[code(t[i + m])]) {
                if (same(p, t, i, 0, m) && !f(i)) { return; }
                if (i == n - m) { return; }}});}
    template<class P> pair<int, int> maxSuffix(const P &p, bool tilde) {
        int m = int(p.size()), ms = -1, j = 0, k = 1, per = 1;
        while (j + k < m) {
            auto a = key(p[j + k]), b = key(p[ms + k]);
            if (tilde ? b < a : a < b) { j += k; k = 1; per = j - ms; }
            else if (a == b) {
                if (k != per) { ++k; }
                else { j += per; k = 1; }}
            else { ms = j++; k = per = 1; }}
        return {ms, per};}
    template<class P, class S, class F> void twoWay(const P &p, const S &t, F f) {
        run(p, t, f, [&](int m, int n) {
            auto [a, pa] = maxSuffix(p, false);
            auto [b, pb] = maxSuffix(p, true);
            int ell = max(a, b), per = a > b ? pa : pb;
            if (same(p, p, per, 0, ell + 1)) {
                for (int j = 0, memory = -1; j <= n - m;) {
                    int i = max(ell, memory) + 1;
                    while (i < m && p[i] == t[i + j]) { ++i; }
                    if (i < m) { j += i - ell; memory = -1; continue; }
                    for (i = ell; i > memory && p[i] == t[i + j]; --i) {}
                    if (i <= memory && !f(j)) { return; }
                    j += per;
                    memory = m - per - 1;}
                return;}
            per = max(ell + 1, m - ell - 1) + 1;
            for (int j = 0; j <= n - m;) {
                int i = ell + 1;
                while (i < m && p[i] == t[i + j]) { ++i; }
                if (i < m) { j += i - ell; continue; }
                for (i = ell; i >= 0 && p[i] == t[i + j]; --i) {}
                if (i < 0 && !f(j)) { return; }
                j += per;}});}
    template<class S, class F> void dfa(const vector<vector<int>> &aut, const S &t, F f) {
        assert(!aut.empty() && t.size() < INT_MAX && coded(t, int(aut[0].size())));
        int m = int(aut.size()) - 1, n = int(t.size()), q = 0;
        if (!m && !f(0)) { return; }
        for (int i = 0; i < n; ++i) {
            q = aut[q][code(t[i])];
            if (q == m && !f(i + 1 - m)) { return; }}}
    template<class P, class S, class F> void best(const P &p, const S &t, F f) {
        if constexpr (std::is_same_v<std::decay_t<decltype(p[0])>, char> && std::is_same_v<std::decay_t<decltype(t[0])>, char>) { boyerMoore(p, t, 256, f); }
        else { twoWay(p, t, f); }}
    template<class G> vector<int> collect(G scan) {
        vector<int> res;
        scan([&](int i) { res.push_back(i); return true; });
        return res;}
} // namespace matching_detail

// Every search returns the starts of pattern in text ascending, overlaps included; the empty pattern gives 0..n; n, m < INT_MAX.
// T: O(n * m), M: O(out); equality-comparable symbols.
template<class P, class S> vector<int> naiveSearch(const P &pattern, const S &text) {
    return matching_detail::collect([&](auto f) { matching_detail::naive(pattern, text, f); });}
// T: O(n + m) expected plus O(m) per hash hit (O(n * m) worst), M: O(out); integral or char symbols; exact (hits verified), seeded base.
template<class P, class S> vector<int> rabinKarp(const P &pattern, const S &text, ulng seed = 20261010) {
    return matching_detail::collect([&](auto f) { matching_detail::rabinKarp(pattern, text, seed, f); });}
// T: O(m + S), M: O(S) returned; res[c] = last i < len with pattern[i] = c, -1 if none; symbols in [0, S) (char as unsigned), len = -1 means m.
template<class P> vector<int> badCharacterTable(const P &pattern, int sigma = 256, int len = -1) {
    assert(pattern.size() < INT_MAX && sigma >= 1 && matching_detail::coded(pattern, sigma));
    if (len == -1) { len = int(pattern.size()); }
    assert(0 <= len && len <= int(pattern.size()));
    return matching_detail::last(pattern, sigma, len);}
// T: O(m), M: O(m) returned; strong good-suffix shift for a mismatch at j after pattern[j + 1, m) matched; res[0] is the smallest period.
template<class P> vector<int> goodSuffixTable(const P &pattern) {
    assert(pattern.size() < INT_MAX);
    return matching_detail::goodSuffix(pattern);}
// T: O(n + m + S) (strong good suffix plus Galil), M: O(m + S + out); symbols of both in [0, S) (char as unsigned).
template<class P, class S> vector<int> boyerMoore(const P &pattern, const S &text, int sigma = 256) {
    assert(sigma >= 1 && matching_detail::coded(pattern, sigma) && matching_detail::coded(text, sigma));
    return matching_detail::collect([&](auto f) { matching_detail::boyerMoore(pattern, text, sigma, f); });}
// T: O(n * m + S) worst, O(n / m + m + S) when every window shifts by m, M: O(S + out); symbols of both in [0, S) (char as unsigned).
template<class P, class S> vector<int> horspool(const P &pattern, const S &text, int sigma = 256) {
    assert(sigma >= 1 && matching_detail::coded(pattern, sigma) && matching_detail::coded(text, sigma));
    return matching_detail::collect([&](auto f) { matching_detail::horspool(pattern, text, sigma, f); });}
// T: O(n * m + S) worst, O(n / m + m + S) when every window shifts by m + 1, M: O(S + out); Quick Search, symbols of both in [0, S).
template<class P, class S> vector<int> sunday(const P &pattern, const S &text, int sigma = 256) {
    assert(sigma >= 1 && matching_detail::coded(pattern, sigma) && matching_detail::coded(text, sigma));
    return matching_detail::collect([&](auto f) { matching_detail::sunday(pattern, text, sigma, f); });}
// T: O(n + m), M: O(out) (O(1) workspace); totally ordered symbols (char as unsigned) with == consistent with <.
template<class P, class S> vector<int> twoWay(const P &pattern, const S &text) {
    return matching_detail::collect([&](auto f) { matching_detail::twoWay(pattern, text, f); });}
// T: O(n), M: O(out); aut = prefixAutomaton(pattern, S) with m + 1 rows, text symbols in [0, S) (char as unsigned).
template<class S> vector<int> dfaSearch(const vector<vector<int>> &aut, const S &text) {
    return matching_detail::collect([&](auto f) { matching_detail::dfa(aut, text, f); });}
// T: O((m + 1) * S + n), M: O((m + 1) * S + out); symbols of both in [0, S) (char as unsigned).
template<class P, class S> vector<int> dfaSearch(const P &pattern, const S &text, int sigma = 256) {
    assert(pattern.size() < INT_MAX && sigma >= 1 && matching_detail::coded(pattern, sigma));
    vector<int> codes(pattern.size());
    for (int i = 0; i < int(codes.size()); ++i) { codes[i] = matching_detail::code(pattern[i]); }
    return dfaSearch(prefixAutomaton(codes, sigma), text);}

// T: O(n + m), M: O(m + S) for char (Boyer-Moore with Galil), O(1) otherwise (two-way, ordered symbols); smallest start, -1 when absent; isSubstring is firstOccurrence >= 0.
template<class P, class S> int firstOccurrence(const P &pattern, const S &text) {
    int res = -1;
    matching_detail::best(pattern, text, [&](int i) { res = i; return false; });
    return res;}
template<class P, class S> bool isSubstring(const P &pattern, const S &text) { return firstOccurrence(pattern, text) >= 0; }
// T: O(n + m), M: O(m + S) for char, O(1) otherwise (two-way, ordered symbols); overlapping, or the greedy maximum of disjoint occurrences; empty pattern n + 1.
template<class P, class S> lng countOccurrences(const P &pattern, const S &text, bool overlapping = true) {
    lng res = 0;
    lng m = lng(pattern.size()), next = 0;
    matching_detail::best(pattern, text, [&](int i) {
        if (overlapping || i >= next) { ++res; next = i + m; }
        return true;});
    return res;}
