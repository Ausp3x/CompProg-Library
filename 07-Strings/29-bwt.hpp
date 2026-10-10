#pragma once
#include "../01-Core/01-template.hpp"
#include "01-prefixfunction.hpp"
#include "12-sais.hpp"
#include "14-lyndon.hpp"

namespace bwt_detail {
    template<class S> pair<vector<int>, int> ranks(const S &s) {
        assert(s.size() < INT_MAX);
        if constexpr (std::is_same_v<std::decay_t<decltype(s[0])>, char>) {
            vector<int> res(s.size());
            for (int i = 0; i < int(s.size()); ++i) { res[i] = uint8_t(s[i]); }
            return {res, 256};}
        else { return compressAlphabet(vector<std::decay_t<decltype(s[0])>>(s.begin(), s.end())); }}
    template<class S> vector<int> psi(const S &last, int d) {
        auto [r, k] = ranks(last);
        int n = int(r.size());
        vector<int> start(k + 2), res(n);
        for (int i = 0; i < n; ++i) { r[i] = i == d ? 0 : r[i] + 1; ++start[r[i] + 1]; }
        for (int c = 0; c <= k; ++c) { start[c + 1] += start[c]; }
        for (int i = 0; i < n; ++i) { res[start[r[i]]++] = i; }
        return res;}
    template<class S> int find(const S &last, auto sentinel) {
        int d = int(std::find(last.begin(), last.end(), sentinel) - last.begin());
        assert(d < int(last.size()) && std::count(last.begin(), last.end(), sentinel) == 1);
        return d;}
    template<class S> S walk(const S &last, const vector<int> &order, int r, int n) {
        S res(last.begin(), last.begin() + n);
        for (int i = 0; i < n; ++i) { r = order[r]; res[i] = last[r]; }
        return res;}
} // namespace bwt_detail

// T: O(n) for bytes, O(n * log(n + 1)) otherwise, M: O(n); starts of the sorted rotations of s, equal rotations by ascending start.
template<class S> vector<int> sortCyclicRotations(const S &s) {
    auto [r, k] = bwt_detail::ranks(s);
    int n = int(r.size());
    if (!n) { return {}; }
    auto pi = prefixFunction(r);
    int p = n % (n - pi[n - 1]) ? n : n - pi[n - 1];
    // Rotations of the primitive root u are distinct and ordered like the suffixes of u + u starting in u.
    vector<int> uu(r.begin(), r.begin() + p), res;
    uu.insert(uu.end(), r.begin(), r.begin() + p);
    for (int i : sais(uu, k - 1)) {
        if (i >= p) { continue; }
        for (int j = i; j < n; j += p) { res.push_back(j); }}
    return res;}

// T: O(n), M: O(n) returned; sa = sentinel-free suffix array of s (n entries); n + 1 symbols with the sentinel ranked lowest.
template<class S> S bwtFromSuffixArray(const S &s, const vector<int> &sa, typename S::value_type sentinel) {
    assert(s.size() < INT_MAX && sa.size() == s.size());
    int n = int(s.size());
    S res(n + 1, sentinel);
    for (int r = 0; r < n; ++r) {
        if (sa[r]) { res[r + 1] = s[sa[r] - 1]; }}
    if (n) { res[0] = s[n - 1]; }
    return res;}
// T: O(n) for bytes, O(n * log(n + 1)) otherwise, M: O(n); BWT of s + sentinel (ranked lowest, absent from s), n + 1 symbols.
template<class S> S bwt(const S &s, typename S::value_type sentinel) {
    assert(std::find(s.begin(), s.end(), sentinel) == s.end());
    auto [r, k] = bwt_detail::ranks(s);
    return bwtFromSuffixArray(s, sais(r, max(k - 1, 0)), sentinel);}
// T: as bwt, M: O(n); last column of the sorted rotation matrix and the smallest row holding s itself (0 when empty).
template<class S> pair<S, int> bwtPrimary(const S &s) {
    auto order = sortCyclicRotations(s);
    int n = int(order.size()), primary = 0;
    S res = s;
    for (int r = 0; r < n; ++r) {
        res[r] = s[order[r] ? order[r] - 1 : n - 1];
        if (!order[r]) { primary = r; }}
    return {res, primary};}

// T: O(n) for bytes, O(n * log(n + 1)) otherwise, M: O(n); psi[r] = row of the r-th smallest symbol of L (stable); LF is its inverse.
// The sentinel overloads rank the unique sentinel of L lowest; the others order rows by symbol only (rotation matrices).
template<class S> vector<int> psiMapping(const S &last) { return bwt_detail::psi(last, -1); }
template<class S> vector<int> psiMapping(const S &last, typename S::value_type sentinel) { return bwt_detail::psi(last, bwt_detail::find(last, sentinel)); }
template<class S> vector<int> lfMapping(const S &last) {
    auto psi = psiMapping(last);
    vector<int> res(psi.size());
    for (int r = 0; r < int(psi.size()); ++r) { res[psi[r]] = r; }
    return res;}
template<class S> vector<int> lfMapping(const S &last, typename S::value_type sentinel) {
    auto psi = psiMapping(last, sentinel);
    vector<int> res(psi.size());
    for (int r = 0; r < int(psi.size()); ++r) { res[psi[r]] = r; }
    return res;}
// T: O(n) for bytes, O(n * log(n + 1)) otherwise, M: O(n); inverts bwt (unique sentinel in L) and bwtPrimary (primary in [0, n)).
template<class S> S inverseBwt(const S &last, typename S::value_type sentinel) {
    int d = bwt_detail::find(last, sentinel);
    return bwt_detail::walk(last, bwt_detail::psi(last, d), d, int(last.size()) - 1);}
template<class S> S inverseBwtPrimary(const S &last, int primary) {
    assert(last.size() < INT_MAX && (last.empty() ? primary == 0 : 0 <= primary && primary < int(last.size())));
    return bwt_detail::walk(last, psiMapping(last), primary, int(last.size()));}

// T: O(n * log(n + 1)), M: O(n); Gil-Scott bijective BWT: last symbols of all rotations of the Lyndon factors in omega order.
template<class S> S bijectiveBwt(const S &s) {
    auto [r, k] = bwt_detail::ranks(s);
    int n = int(r.size()), classes = k, prev = int(std::set<int>(r.begin(), r.end()).size());
    if (!n) { return s; }
    auto cuts = duval(r);
    vector<int> begin(n), len(n), order(n), tmp(n), rank = r, cnt;
    for (int f = 0; f + 1 < int(cuts.size()); ++f) {
        for (int i = cuts[f]; i < cuts[f + 1]; ++i) { begin[i] = cuts[f]; len[i] = cuts[f + 1] - cuts[f]; }}
    auto shift = [&](int i, lng h) { return begin[i] + int((i - begin[i] + h % len[i]) % len[i]); };
    iota(order.begin(), order.end(), 0);
    // Prefix doubling on omega-words; a partition equal to the previous one is final.
    for (lng h = 1;; h *= 2) {
        auto sortBy = [&](auto key) {
            cnt.assign(classes + 1, 0);
            for (int i : order) { ++cnt[key(i) + 1]; }
            for (int c = 0; c < classes; ++c) { cnt[c + 1] += cnt[c]; }
            for (int i : order) { tmp[cnt[key(i)]++] = i; }
            swap(order, tmp);};
        sortBy([&](int i) { return rank[shift(i, h)]; });
        sortBy([&](int i) { return rank[i]; });
        int fresh = 0;
        for (int t = 0; t < n; ++t) {
            int i = order[t], j = t ? order[t - 1] : i;
            fresh += t && (rank[i] != rank[j] || rank[shift(i, h)] != rank[shift(j, h)]);
            tmp[i] = fresh;}
        bool done = fresh + 1 == prev || fresh + 1 == n || h >= n;
        rank.swap(tmp);
        classes = prev = fresh + 1;
        if (done) { break; }}
    S res = s;
    for (int t = 0; t < n; ++t) { res[t] = s[shift(order[t], len[order[t]] - 1)]; }
    return res;}
// T: O(n) for bytes, O(n * log(n + 1)) otherwise, M: O(n); cycles of the stable permutation are the Lyndon factors, emitted in nonincreasing order.
template<class S> S inverseBijectiveBwt(const S &last) {
    auto order = psiMapping(last);
    int n = int(order.size());
    vector<int> seq, cuts{0};
    vector<char> seen(n);
    for (int r0 = 0; r0 < n; ++r0) {
        if (seen[r0]) { continue; }
        for (int r = order[r0];; r = order[r]) {
            seen[r] = 1;
            seq.push_back(r);
            if (r == r0) { break; }}
        cuts.push_back(int(seq.size()));}
    S res = last;
    for (int f = int(cuts.size()) - 2, t = 0; f >= 0; --f) {
        for (int i = cuts[f]; i < cuts[f + 1]; ++i) { res[t++] = last[seq[i]]; }}
    return res;}
