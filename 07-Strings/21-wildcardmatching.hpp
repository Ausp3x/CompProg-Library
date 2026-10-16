#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "../05-Mathematics/13-convolution.hpp"
#include "../06-Miscellaneous/02-customhash.hpp"
#include "01-prefixfunction.hpp"
#include "17-bitap.hpp"

namespace wildcard_detail {
    constexpr int BUDGET = 16, SPREAD = 8, LIMIT = 1 << 23;
    template<class C> ulng key(C c) {
        if constexpr (std::is_same_v<C, char>) { return uint8_t(c); }
        else { return ulng(c); }}
    // Codes: pattern symbols ranked in sorted order, other text symbols get rank sigma, wildcards -1.
    template<class S, class P, class C> tuple<vector<lng>, vector<lng>, int> ranks(const S &text, const P &pattern, C wildcard) {
        assert(text.size() < INT_MAX && pattern.size() <= LIMIT / 2);
        vector<ulng> sorted;
        for (auto c : pattern) {
            if (c != wildcard) { sorted.push_back(key(c)); }}
        sort(sorted.begin(), sorted.end());
        sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
        auto code = [&](auto c) -> lng {
            if (c == wildcard) { return -1; }
            auto it = lower_bound(sorted.begin(), sorted.end(), key(c));
            return it != sorted.end() && *it == key(c) ? it - sorted.begin() : lng(sorted.size());};
        vector<lng> tc, pc;
        tc.reserve(text.size()); pc.reserve(pattern.size());
        for (auto c : text) { tc.push_back(code(c)); }
        for (auto c : pattern) { pc.push_back(code(c)); }
        return {std::move(tc), std::move(pc), int(sorted.size())};}
    // Exact scan of starts from from with rate * (m + scanned starts) comparisons at most; a true hit(i) stops it; returns the first unexamined start.
    template<class F, class H> int scan(int from, int end, int m, lng rate, F same, H hit) {
        lng budget = rate * m;
        int i = from;
        for (; i + m <= end; ++i) {
            int j = 0;
            while (j < m && same(i + j, j)) { ++j; }
            if (j == m && hit(i)) { return i + 1; }
            if ((budget += rate - j - 1) < 0) { return i + 1; }}
        return i;}
    // One block when n <= SPREAD * m, else blocks of bit_ceil(SPREAD * m) symbols overlapping by m - 1.
    inline int blockLength(int n, int m) { return min(int(std::bit_ceil(uint(min(n, SPREAD * m)))), LIMIT); }
    // Pattern rows a[r] pair with text rows r; the block of len symbols from l leaves sum_r sum_j a[r][j] * text_r[l + i + j] at sum[m - 1 + i].
    struct Correlator {
        int m, len;
        vector<vector<mint>> rows;
        vector<mint> sum, buf;

        Correlator(const vector<vector<mint>> &a, int size) : m(int(a[0].size())), len(size) {
            for (const auto &x : a) {
                vector<mint> y(len);
                for (int j = 0; j < m; ++j) { y[j] = x[m - 1 - j]; }
                ntt(y);
                rows.push_back(std::move(y));}
            sum.resize(len); buf.resize(len);}
        int stride() const { return len - m + 1; }
        template<class G> void block(int l, int n, G value) {
            int cnt = min(len, n - l);
            fill(sum.begin(), sum.end(), mint(0));
            for (int r = 0; r < int(rows.size()); ++r) {
                for (int i = 0; i < cnt; ++i) { buf[i] = value(r, l + i); }
                fill(buf.begin() + cnt, buf.end(), mint(0));
                ntt(buf);
                for (int i = 0; i < len; ++i) { sum[i] += rows[r][i] * buf[i]; }}
            intt(sum);}
        // Calls hit(start) for each zero sum, starts ascending from from, until hit returns true.
        template<class G, class H> void scan(int from, int n, G value, H hit) {
            for (int l = from; l + m <= n; l += stride()) {
                block(l, n, value);
                for (int i = 0; i < stride() && l + i + m <= n; ++i) {
                    if (sum[m - 1 + i] == mint(0) && hit(l + i)) { return; }}}}
    };
    // Exact rows: the sum over solid pairs of popcount(p xor t) = popP * Wt + Wp * popT - 2 * sum_b P_b * T_b stays below the prime.
    inline vector<vector<mint>> exactRows(const vector<lng> &pc, int bits) {
        int m = int(pc.size());
        vector<vector<mint>> a(2 + bits, vector<mint>(m));
        for (int j = 0; j < m; ++j) {
            if (pc[j] < 0) { continue; }
            a[0][j] = std::popcount(ulng(pc[j])); a[1][j] = 1;
            for (int b = 0; b < bits; ++b) { a[2 + b][j] = pc[j] >> b & 1 ? -2 : 0; }}
        return a;}
    inline mint exactValue(int r, lng c) {
        if (c < 0) { return 0; }
        return r == 0 ? 1 : r == 1 ? std::popcount(ulng(c)) : int(c >> (r - 2) & 1);}
    inline bool fits(string_view text, int at, string_view seg, char any) {
        for (int j = 0; j < int(seg.size()); ++j) {
            if (seg[j] != any && seg[j] != text[at + j]) { return false; }}
        return true;}
    // Leftmost start in [from, end - |seg|] where seg fits, or -1: KMP without any, else a budgeted scan, then exact blocks.
    inline int leftmost(string_view text, int from, int end, string_view seg, char any) {
        int m = int(seg.size()), res = -1;
        if (end - from < m) { return -1; }
        if (seg.find(any) == string_view::npos) {
            KmpMatcher kmp(seg);
            if (kmp.matched()) { return from; }
            for (int i = from; i < end; ++i) {
                if (kmp.step(text[i])) { return i + 1 - m; }}
            return -1;}
        assert(m <= LIMIT / 2);
        if (m > LIMIT / 2) { return -1; }
        from = scan(from, end, m, BUDGET, [&](int i, int j) { return seg[j] == any || seg[j] == text[i]; }, [&](int i) { res = i; return true; });
        if (res >= 0 || from + m > end) { return res; }
        vector<int> symbols;
        for (char c : seg) {
            if (c != any) { symbols.push_back(uint8_t(c)); }}
        sort(symbols.begin(), symbols.end());
        symbols.erase(unique(symbols.begin(), symbols.end()), symbols.end());
        int sigma = int(symbols.size());
        array<lng, 256> table;
        table.fill(sigma);
        for (int k = 0; k < sigma; ++k) { table[symbols[k]] = k; }
        vector<lng> pc(m);
        for (int j = 0; j < m; ++j) { pc[j] = seg[j] == any ? -1 : table[uint8_t(seg[j])]; }
        Correlator cor(exactRows(pc, std::bit_width(uint(sigma))), blockLength(end - from, m));
        cor.scan(from, end, [&](int r, int i) { return exactValue(r, table[uint8_t(text[i])]); }, [&](int i) { res = i; return true; });
        return res;}
} // namespace wildcard_detail

// T: O(n * log(m + 1)), M: O(n + m); m <= 2^22; Monte Carlo: a non-matching start is reported with probability <= 4 / 998244352.
// Sequences (string, string_view, vector) of char or integral symbols; wildcard in text and pattern; starts ascending, empty pattern 0..n.
template<class S, class P> vector<int> wildcardMatch(const S &text, const P &pattern, typename S::value_type wildcard, ulng seed = CustomHash::rnd) {
    int n = int(text.size()), m = int(pattern.size());
    assert(text.size() < INT_MAX && pattern.size() <= wildcard_detail::LIMIT / 2);
    vector<int> res;
    if (pattern.size() > wildcard_detail::LIMIT / 2) { return res; }
    auto collect = [&](int i) { res.push_back(i); return false; };
    auto same = [&](int i, int j) { return pattern[j] == wildcard || text[i] == wildcard || pattern[j] == text[i]; };
    int from = wildcard_detail::scan(0, n, m, wildcard_detail::BUDGET, same, collect);
    if (from + m > n) { return res; }
    auto value = [&](auto c) { return c == wildcard ? mint(0) : mint(1 + CustomHash::splitMix64(seed ^ wildcard_detail::key(c)) % (mint::mod() - 1)); };
    vector<mint> tv(n);
    vector<vector<mint>> a(3, vector<mint>(m));
    for (int i = from; i < n; ++i) { tv[i] = value(text[i]); }
    for (int j = 0; j < m; ++j) {
        mint p = value(pattern[j]);
        a[0][j] = p * p * p; a[1][j] = -2 * p * p; a[2][j] = p;}
    wildcard_detail::Correlator cor(a, wildcard_detail::blockLength(n - from, m));
    cor.scan(from, n, [&](int r, int i) { return r == 0 ? tv[i] : r == 1 ? tv[i] * tv[i] : tv[i] * tv[i] * tv[i]; }, collect);
    return res;}

// T: O(n * log(m + 1) * log(S + 1)), M: O(n + m * log(S + 1)); S distinct pattern symbols, m <= 2^22 (so m * bit_width(S) < 998244353); exact.
// Same contract as wildcardMatch without randomness.
template<class S, class P> vector<int> wildcardMatchDeterministic(const S &text, const P &pattern, typename S::value_type wildcard) {
    int n = int(text.size()), m = int(pattern.size());
    assert(text.size() < INT_MAX && pattern.size() <= wildcard_detail::LIMIT / 2);
    vector<int> res;
    if (pattern.size() > wildcard_detail::LIMIT / 2) { return res; }
    auto collect = [&](int i) { res.push_back(i); return false; };
    auto same = [&](int i, int j) { return pattern[j] == wildcard || text[i] == wildcard || pattern[j] == text[i]; };
    int from = wildcard_detail::scan(0, n, m, wildcard_detail::BUDGET, same, collect);
    if (from + m > n) { return res; }
    auto [tc, pc, sigma] = wildcard_detail::ranks(text, pattern, wildcard);
    wildcard_detail::Correlator cor(wildcard_detail::exactRows(pc, std::bit_width(uint(sigma))), wildcard_detail::blockLength(n - from, m));
    cor.scan(from, n, [&](int r, int i) { return wildcard_detail::exactValue(r, tc[i]); }, collect);
    return res;}

// T: O(n + m) without any, O((n + m) * log(m + 1) * log(S + 1)) otherwise; M: O(m); S distinct symbols of a segment.
// Whole-text glob over bytes: star matches any run, any one symbol, no escapes; a searched middle segment holding any has at most 2^22 symbols.
inline bool globMatch(string_view text, string_view pattern, char any = '?', char star = '*') {
    assert(text.size() < INT_MAX && pattern.size() < INT_MAX);
    int n = int(text.size());
    vector<string_view> seg;
    for (size_t at = 0;;) {
        size_t next = pattern.find(star, at);
        seg.push_back(pattern.substr(at, next == string_view::npos ? next : next - at));
        if (next == string_view::npos) { break; }
        at = next + 1;}
    if (seg.size() == 1) { return pattern.size() == text.size() && wildcard_detail::fits(text, 0, pattern, any); }
    int lo = int(seg[0].size()), hi = n - int(seg.back().size());
    if (lo > hi || !wildcard_detail::fits(text, 0, seg[0], any) || !wildcard_detail::fits(text, hi, seg.back(), any)) { return false; }
    for (int i = 1; i + 1 < int(seg.size()); ++i) {
        int at = wildcard_detail::leftmost(text, lo, hi, seg[i], any);
        if (at < 0) { return false; }
        lo = at + int(seg[i].size());}
    return true;}

// S: O(m + S * k / w), U: O(k / w + 1), Q: O(1), M: O(S * k / w); bytes, k non-star pattern symbols, S = 256.
// Bit i of state: the text read so far matches the pattern through its i-th non-star symbol; star bits loop on every symbol.
struct GlobAutomaton {
    PatternMasks masks;
    vector<ulng> loops, state;

    explicit GlobAutomaton(string_view pattern, char any = '?', char star = '*') {
        assert(pattern.size() < INT_MAX);
        int k = int(pattern.size() - std::ranges::count(pattern, star)), words = k / 64 + 1;
        masks = PatternMasks{k, 256, words, vector<ulng>(256 * words)};
        loops.assign(words, 0);
        vector<ulng> wild(words);
        int i = 0;
        for (char c : pattern) {
            ulng bit = 1ULL << (i & 63);
            if (c == star) { loops[i >> 6] |= bit; continue; }
            (c == any ? wild[i >> 6] : masks.bits[uint8_t(c) * words + (i >> 6)]) |= bit;
            ++i;}
        for (int x = 0; x < 256; ++x) {
            for (int w = 0; w < words; ++w) { masks.bits[x * words + w] |= wild[w]; }}
        reset();}

    void reset() { state.assign(masks.words, 0); state[0] = 1; }
    bool step(char c) {
        const ulng *b = masks[uint8_t(c)];
        ulng carry = 0;
        for (int w = 0; w < masks.words; ++w) {
            ulng x = state[w] & b[w];
            state[w] = x << 1 | carry | (state[w] & loops[w]);
            carry = x >> 63;}
        return accepted();}

    bool accepted() const { return state[masks.m >> 6] >> (masks.m & 63) & 1; }
};
