#pragma once
#include "../01-Core/01-template.hpp"

namespace editdistance_detail {
    template<class S> using Value = std::remove_cvref_t<decltype(std::declval<const S &>()[0])>;
    inline constexpr lng NEG = std::numeric_limits<lng>::min() / 4;

    // Byte symbols keep their unsigned value; other symbols get ranks among a's symbols, -1 for b's symbols absent from a.
    template<class S> pair<vector<int>, vector<int>> codes(const S &a, const S &b) {
        using T = Value<S>;
        vector<int> x(a.size()), y(b.size());
        if constexpr (sizeof(T) == 1 && std::is_integral_v<T>) {
            for (int i = 0; i < int(a.size()); ++i) { x[i] = uint8_t(a[i]); }
            for (int j = 0; j < int(b.size()); ++j) { y[j] = uint8_t(b[j]); }}
        else {
            vector<T> alphabet;
            for (int i = 0; i < int(a.size()); ++i) { alphabet.push_back(a[i]); }
            sort(alphabet.begin(), alphabet.end());
            alphabet.erase(unique(alphabet.begin(), alphabet.end()), alphabet.end());
            auto code = [&](const T &c) {
                auto it = lower_bound(alphabet.begin(), alphabet.end(), c);
                return it != alphabet.end() && *it == c ? int(it - alphabet.begin()) : -1;};
            for (int i = 0; i < int(a.size()); ++i) { x[i] = code(a[i]); }
            for (int j = 0; j < int(b.size()); ++j) { y[j] = code(b[j]); }}
        return {x, y};}
    // Band |i - j| <= w over the longer string's rows; cap >= 0 stops with cap + 1 once a whole row exceeds cap.
    template<class S> int band(const S &a, const S &b, int w, int cap) {
        if (a.size() < b.size()) { return band(b, a, w, cap); }
        int n = int(a.size()), m = int(b.size()), inf = INT_MAX / 2;
        w = min(w, n);
        if (n - m > w) { return -1; }
        vector<int> prev(m + 1, inf), cur(m + 1, inf);
        for (int j = 0; j <= min(m, w); ++j) { prev[j] = j; }
        for (int i = 1; i <= n; ++i) {
            int lo = max(0, i - w), hi = min(m, i + w), best = inf;
            if (!lo) { cur[0] = best = i; }
            for (int j = max(lo, 1); j <= hi; ++j) {
                cur[j] = min({prev[j - 1] + (a[i - 1] != b[j - 1]), prev[j] + 1, j > lo ? cur[j - 1] + 1 : inf});
                best = min(best, cur[j]);}
            if (hi < m) { cur[hi + 1] = inf; }
            if (cap >= 0 && best > cap) { return cap + 1; }
            swap(prev, cur);}
        return prev[m];}
    // Last row of the global alignment of a[la, ra) against b[lb, rb), or of both ranges reversed.
    template<class S, class F> vector<lng> row(const S &a, int la, int ra, const S &b, int lb, int rb, F &score, lng gap, bool reversed) {
        int m = rb - lb;
        vector<lng> d(m + 1);
        for (int j = 0; j <= m; ++j) { d[j] = gap * j; }
        for (int i = 1; i <= ra - la; ++i) {
            const auto &x = a[reversed ? ra - i : la + i - 1];
            lng diag = d[0];
            d[0] = gap * i;
            for (int j = 1; j <= m; ++j) {
                lng up = d[j];
                d[j] = max({diag + score(x, b[reversed ? rb - j : lb + j - 1]), up + gap, d[j - 1] + gap});
                diag = up;}}
        return d;}
    // Best split of b[lb, rb) for a[la, mid) | a[mid, ra): smallest k maximizing forward plus reversed scores; rows freed on return.
    template<class S, class F> int split(const S &a, int la, int mid, int ra, const S &b, int lb, int rb, F &score, lng gap) {
        int m = rb - lb, k = 0;
        vector<lng> f = row(a, la, mid, b, lb, rb, score, gap, false), g = row(a, mid, ra, b, lb, rb, score, gap, true);
        for (int j = 1; j <= m; ++j) {
            if (f[j] + g[m - j] > f[k] + g[m - k]) { k = j; }}
        return k;}
    template<class S, class F> void hirschberg(const S &a, int la, int ra, const S &b, int lb, int rb, F &score, lng gap, vector<pair<int, int>> &res) {
        if (la == ra) {
            for (int j = lb; j < rb; ++j) { res.push_back({-1, j}); }
            return;}
        if (ra - la == 1) {
            int k = -1;
            lng best = gap * (rb - lb + 1);
            for (int j = lb; j < rb; ++j) {
                if (lng v = score(a[la], b[j]) + gap * (rb - lb - 1); v > best) { best = v; k = j; }}
            if (k < 0) { res.push_back({la, -1}); }
            for (int j = lb; j < rb; ++j) { res.push_back({j == k ? la : -1, j}); }
            return;}
        int mid = std::midpoint(la, ra), k = split(a, la, mid, ra, b, lb, rb, score, gap);
        hirschberg(a, la, mid, b, lb, lb + k, score, gap, res);
        hirschberg(a, mid, ra, b, lb + k, rb, score, gap, res);}
} // namespace editdistance_detail

// T: O(n * m), M: O(min(n, m)); unit-cost Levenshtein distance; S is string, string_view, vector<T> or another indexable sequence.
template<class S> int levenshtein(const S &a, const S &b) {
    if (a.size() < b.size()) { return levenshtein(b, a); }
    int n = int(a.size()), m = int(b.size());
    vector<int> d(m + 1);
    iota(d.begin(), d.end(), 0);
    for (int i = 1; i <= n; ++i) {
        int diag = d[0];
        d[0] = i;
        for (int j = 1; j <= m; ++j) {
            int up = d[j];
            d[j] = min({up + 1, d[j - 1] + 1, diag + (a[i - 1] != b[j - 1])});
            diag = up;}}
    return d[m];}
// T: O(n * m), M: O(n * m); {distance, columns}: (i, j) aligned, (i, -1) deletes a[i], (-1, j) inserts b[j]; ties prefer diagonal, deletion, insertion from the end.
template<class S> pair<int, vector<pair<int, int>>> levenshteinWitness(const S &a, const S &b) {
    int n = int(a.size()), m = int(b.size());
    vector<vector<int>> d(n + 1, vector<int>(m + 1));
    for (int i = 0; i <= n; ++i) {
        for (int j = 0; j <= m; ++j) {
            if (!i || !j) { d[i][j] = i + j; }
            else { d[i][j] = min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + (a[i - 1] != b[j - 1])}); }}}
    vector<pair<int, int>> res;
    for (int i = n, j = m; i || j;) {
        if (i && j && d[i][j] == d[i - 1][j - 1] + (a[i - 1] != b[j - 1])) { res.push_back({--i, --j}); }
        else if (i && d[i][j] == d[i - 1][j] + 1) { res.push_back({--i, -1}); }
        else { res.push_back({-1, --j}); }}
    reverse(res.begin(), res.end());
    return {d[n][m], res};}
// T: O(n * m), M: O(n + m); maximum global alignment score(x, y) + gap per gap column, with an optimal column list (as levenshteinWitness).
template<class S, class F> pair<lng, vector<pair<int, int>>> hirschberg(const S &a, const S &b, F &&score, lng gap) {
    vector<pair<int, int>> res;
    editdistance_detail::hirschberg(a, 0, int(a.size()), b, 0, int(b.size()), score, gap, res);
    lng total = 0;
    for (auto [i, j] : res) { total += i < 0 || j < 0 ? gap : score(a[i], b[j]); }
    return {total, res};}
// T: O(n * m), M: O(n + m); Levenshtein distance with an optimal column list in linear space.
template<class S> pair<int, vector<pair<int, int>>> hirschberg(const S &a, const S &b) {
    auto [total, res] = hirschberg(a, b, [](const auto &x, const auto &y) { return -lng(x != y); }, -1);
    return {int(-total), res};}
// T: O(min(n, m) * ceil(max(n, m) / 64) + n + m), M: O(n + m + sigma); exact Levenshtein; sigma <= 256 for bytes, else add O((n + m) * log(n + 1)) ranking.
template<class S> int myersBitVector(const S &a, const S &b) {
    if (a.size() < b.size()) { return myersBitVector(b, a); }
    int n = int(a.size()), m = int(b.size()), score = n;
    if (!m) { return n; }
    auto [x, y] = editdistance_detail::codes(a, b);
    int sigma = *std::max_element(x.begin(), x.end()) + 1;
    vector<ulng> peq(sigma);
    vector<int> h(m, 1);
    // Block-major: h[j] carries the horizontal delta at the bottom row of the previous 64-row block.
    for (int lo = 0; lo < n; lo += 64) {
        int hi = min(n, lo + 64);
        for (int i = lo; i < hi; ++i) { peq[x[i]] |= 1ULL << (i - lo); }
        ulng pv = ~0ULL, mv = 0, top = 1ULL << (hi - lo - 1);
        for (int j = 0; j < m; ++j) {
            ulng eq = 0 <= y[j] && y[j] < sigma ? peq[y[j]] : 0, xv = eq | mv;
            if (h[j] < 0) { eq |= 1; }
            ulng xh = (((eq & pv) + pv) ^ pv) | eq, ph = mv | ~(xh | pv), mh = pv & xh;
            int out = ph & top ? 1 : mh & top ? -1 : 0;
            ph = ph << 1 | (h[j] > 0);
            mh = mh << 1 | (h[j] < 0);
            pv = mh | ~(xv | ph);
            mv = ph & xv;
            h[j] = out;}
        for (int i = lo; i < hi; ++i) { peq[x[i]] = 0; }}
    for (int d : h) { score += d; }
    return score;}
// T: O(max(n, m) * min(2 * w + 1, min(n, m) + 1)), M: O(min(n, m)); best alignment within |i - j| <= w (exact when the distance is <= w), -1 if |n - m| > w.
template<class S> int bandedEditDistance(const S &a, const S &b, int w) { assert(w >= 0); return editdistance_detail::band(a, b, w, -1); }
// T: O(max(n, m) * min(2 * k + 1, min(n, m) + 1)), M: O(min(n, m)); min(distance, k + 1), stops at the first band row above k.
template<class S> int thresholdEditDistance(const S &a, const S &b, int k) {
    assert(k >= 0);
    int d = editdistance_detail::band(a, b, k, k);
    return d < 0 ? k + 1 : min(d, k + 1);}
// T: O((d + 1) * (min(n, m) + 1)), M: O(n + m); d is the answer; furthest-reaching diagonals (Ukkonen/Myers O(ND)) with Ukkonen's cutoff.
template<class S> int diagonalEditDistance(const S &a, const S &b) {
    int n = int(a.size()), m = int(b.size()), top = max(n, m), unreached = INT_MIN / 2;
    vector<int> prev(n + m + 3, unreached), cur(n + m + 3, unreached);
    auto slide = [&](int k, int i) {
        while (i < n && i + k < m && a[i] == b[i + k]) { ++i; }
        return i;};
    prev[n + 1] = slide(0, 0);
    for (int e = 1;; ++e) {
        if (prev[m + 1] >= n) { return e - 1; }
        // Diagonal k at cost e can lie on an optimal path only if e + |k - (m - n)| <= max(n, m).
        int lo = max({-n, -e, m - n - (top - e)}), hi = min({m, e, m - n + (top - e)});
        cur[lo + n] = cur[hi + n + 2] = unreached;
        for (int k = lo; k <= hi; ++k) {
            int at = k + n + 1, i = max({prev[at] + 1, prev[at + 1] + 1, prev[at - 1]});
            cur[at] = i < 0 ? unreached : slide(k, min({i, n, m - k}));}
        swap(prev, cur);}}
// T: O(n * m), M: O(m); costs >= 0 to insert b[j], delete a[i] and substitute unequal symbols; matches are free.
template<class S> lng weightedEditDistance(const S &a, const S &b, lng ins, lng del, lng sub) {
    assert(ins >= 0 && del >= 0 && sub >= 0);
    int n = int(a.size()), m = int(b.size());
    vector<lng> d(m + 1);
    for (int j = 0; j <= m; ++j) { d[j] = ins * j; }
    for (int i = 1; i <= n; ++i) {
        lng diag = d[0];
        d[0] = del * i;
        for (int j = 1; j <= m; ++j) {
            lng up = d[j];
            d[j] = min({up + del, d[j - 1] + ins, diag + (a[i - 1] != b[j - 1] ? sub : 0)});
            diag = up;}}
    return d[m];}
// T: O(n * m), M: O(m); restricted Damerau-Levenshtein: adjacent transpositions of untouched symbols cost 1.
template<class S> int optimalStringAlignment(const S &a, const S &b) {
    int n = int(a.size()), m = int(b.size());
    vector<int> two(m + 1), prev(m + 1), cur(m + 1);
    iota(prev.begin(), prev.end(), 0);
    for (int i = 1; i <= n; ++i) {
        cur[0] = i;
        for (int j = 1; j <= m; ++j) {
            cur[j] = min({prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + (a[i - 1] != b[j - 1])});
            if (i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1]) { cur[j] = min(cur[j], two[j - 2] + 1); }}
        two.swap(prev);
        prev.swap(cur);}
    return prev[m];}
// T: O(n * m + (n + m) * log(n + 1)), M: O(n * m); unrestricted Damerau-Levenshtein (Lowrance-Wagner); log term only for non-byte symbols.
template<class S> int damerauLevenshtein(const S &a, const S &b) {
    int n = int(a.size()), m = int(b.size()), inf = n + m;
    auto [x, y] = editdistance_detail::codes(a, b);
    int sigma = 1;
    for (int c : x) { sigma = max(sigma, c + 1); }
    for (int c : y) { sigma = max(sigma, c + 1); }
    vector<int> last(sigma);
    auto seenRow = [&](int c) { return c < 0 ? 0 : last[c]; };
    vector<vector<int>> d(n + 2, vector<int>(m + 2, inf));
    for (int i = 0; i <= n; ++i) { d[i + 1][1] = i; }
    for (int j = 0; j <= m; ++j) { d[1][j + 1] = j; }
    for (int i = 1; i <= n; ++i) {
        for (int j = 1, seen = 0; j <= m; ++j) {
            int i1 = seenRow(y[j - 1]), j1 = seen, same = x[i - 1] == y[j - 1];
            if (same) { seen = j; }
            d[i + 1][j + 1] = min({d[i][j] + !same, d[i + 1][j] + 1, d[i][j + 1] + 1, d[i1][j1] + (i - i1 - 1) + 1 + (j - j1 - 1)});}
        last[x[i - 1]] = i;}
    return d[n + 1][m + 1];}
// T: O(n * m), M: O(m); maximum global alignment score: sum of score(x, y) over aligned pairs plus gap per gap column.
template<class S, class F> lng needlemanWunsch(const S &a, const S &b, F &&score, lng gap) {
    return editdistance_detail::row(a, 0, int(a.size()), b, 0, int(b.size()), score, gap, false).back();}
// T: O(n * m), M: O(m); gap <= 0; global alignment whose leading and trailing gaps in either string are free.
template<class S, class F> lng semiGlobalAlignment(const S &a, const S &b, F &&score, lng gap) {
    assert(gap <= 0);
    int n = int(a.size()), m = int(b.size());
    vector<lng> d(m + 1);
    lng res = 0;
    for (int i = 1; i <= n; ++i) {
        lng diag = d[0];
        for (int j = 1; j <= m; ++j) {
            lng up = d[j];
            d[j] = max({diag + score(a[i - 1], b[j - 1]), up + gap, d[j - 1] + gap});
            diag = up;}
        res = max(res, d[m]);}
    return max(res, *std::max_element(d.begin(), d.end()));}
// T: O(n * m), M: O(m); gap <= 0; {score, la, ra, lb, rb}: best local alignment of a[la, ra) with b[lb, rb), first row-major end; {0, 0, 0, 0, 0} if none is positive.
template<class S, class F> tuple<lng, int, int, int, int> smithWaterman(const S &a, const S &b, F &&score, lng gap) {
    assert(gap <= 0);
    int n = int(a.size()), m = int(b.size());
    struct Cell { lng v; int i, j; };
    vector<Cell> d(m + 1);
    for (int j = 0; j <= m; ++j) { d[j] = {0, 0, j}; }
    tuple<lng, int, int, int, int> res{0, 0, 0, 0, 0};
    for (int i = 1; i <= n; ++i) {
        Cell diag = d[0];
        d[0] = {0, i, 0};
        for (int j = 1; j <= m; ++j) {
            Cell up = d[j], here{0, i, j};
            for (Cell c : {Cell{diag.v + score(a[i - 1], b[j - 1]), diag.i, diag.j}, Cell{up.v + gap, up.i, up.j}, Cell{d[j - 1].v + gap, d[j - 1].i, d[j - 1].j}}) {
                if (c.v > here.v) { here = c; }}
            d[j] = here;
            diag = up;
            if (here.v > std::get<0>(res)) { res = {here.v, here.i, i, here.j, j}; }}}
    return res;}
// T: O(n * m), M: O(m); maximum global alignment with affine gaps: a gap run of length L adds open + extend * L.
template<class S, class F> lng gotoh(const S &a, const S &b, F &&score, lng open, lng extend) {
    using editdistance_detail::NEG;
    int n = int(a.size()), m = int(b.size());
    vector<lng> pair_end(m + 1, NEG), del_end(m + 1, NEG), ins_end(m + 1, NEG);
    pair_end[0] = 0;
    for (int j = 1; j <= m; ++j) { ins_end[j] = open + extend * j; }
    for (int i = 1; i <= n; ++i) {
        lng dp = pair_end[0], dd = del_end[0], di = ins_end[0];
        pair_end[0] = ins_end[0] = NEG;
        del_end[0] = open + extend * i;
        for (int j = 1; j <= m; ++j) {
            lng up = pair_end[j], ud = del_end[j], ui = ins_end[j];
            pair_end[j] = max({dp, dd, di}) + score(a[i - 1], b[j - 1]);
            del_end[j] = max({up + open, ud, ui + open}) + extend;
            ins_end[j] = max({pair_end[j - 1] + open, ins_end[j - 1], del_end[j - 1] + open}) + extend;
            dp = up; dd = ud; di = ui;}}
    return max({pair_end[m], del_end[m], ins_end[m]});}
