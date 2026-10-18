#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/05-modint.hpp"
#include "01-mod_arithmetic.hpp"
#include "05-combinatorics.hpp"
#include "09-modinverse.hpp"
#include "13-convolution.hpp"
#include "14-polynomial_algorithms.hpp"

namespace lattice_paths_detail {
    // T: O(n), M: O(n); (i + 1) x_i = 3 (2i - 1) x_(i-1) - (i - 2) x_(i-2) from x_0 = 1 and x_1, prime M::mod() > n + 1.
    template<typename M>
    vector<M> schroder(int n, M x1) {
        assert(n >= 0);
        auto iv = inverseTable<M>(n + 1);
        vector<M> res(n + 1, 1);
        if (n >= 1) { res[1] = x1; }
        for (int i = 2; i <= n; ++i) { res[i] = (M(6 * lng(i) - 3) * res[i - 1] - M(i - 2) * res[i - 2]) * iv[i + 1]; }
        return res;}
} // namespace lattice_paths_detail

// T: O(1), M: O(1); right/up unit steps, pc covers every binomial index, impossible endpoints give 0, ballot conventions of PrimeCombinatorics::ballot.
template<typename M>
M gridPaths(int dx, int dy, const PrimeCombinatorics<M> &pc) {
    if (dx < 0 || dy < 0) { return 0; }
    assert(lng(dx) + dy <= pc.n); return pc.combiNR(dx + dy, dx);}
template<typename M>
M dyckPaths(int n, int h1, int h2, const PrimeCombinatorics<M> &pc) {
    assert(n >= 0 && h1 >= 0 && h2 >= 0);
    if ((lng(n) + h1 + h2) % 2 || std::abs(lng(h1) - h2) > n) { return 0; }
    int u = int((lng(n) + h2 - h1) / 2);
    return pc.combiNR(n, u) - (lng(u) + h1 + 1 > n ? M(0) : pc.combiNR(n, u + h1 + 1));}
template<typename M>
M pathsBelowDiagonal(int x1, int y1, int x2, int y2, const PrimeCombinatorics<M> &pc) {
    if (y1 > x1 || y2 > x2) { return 0; }
    return gridPaths(x2 - x1, y2 - y1, pc) - gridPaths(x2 - y1 + 1, y2 - x1 - 1, pc);}
template<typename M>
M ballotGeneral(int c, int d, int mu, bool strict, const PrimeCombinatorics<M> &pc) {
    assert(c >= 0 && d >= 0 && mu >= 0 && lng(c) + d <= pc.n);
    if (c == 0 && d == 0) { return 1; }
    if (strict) {
        if (c == 0 || lng(c) <= lng(mu) * d) { return 0; }
        --c;}
    else if (lng(c) < lng(mu) * d) { return 0; }
    return pc.combiNR(c + d, d) - M(mu) * pc.combiNR(c + d, d - 1);}
template<typename M>
M catalanConvolution(int n, int k, const PrimeCombinatorics<M> &pc) {
    assert(n >= 0 && k >= 0 && 2 * lng(n) + k - 1 <= pc.n);
    if (k == 0) { return M(n == 0); }
    return pc.combiNR(2 * n + k - 1, n) - pc.combiNR(2 * n + k - 1, n - 1);}
template<typename M>
M narayana(int n, int k, const PrimeCombinatorics<M> &pc) {
    assert(n >= 0);
    if (n == 0) { return M(k == 0); }
    if (k < 1 || k > n) { return 0; }
    M a = pc.combiNR(n - 1, k - 1);
    return a * a - pc.combiNR(n - 1, k) * pc.combiNR(n - 1, k - 2);}

// T: O(x + y) rectangle, O(1 + d - b) below line, O(1 + n / (hi - lo + 2)) between lines, O(n) motzkinPaths, O(min(m, n)) delannoy, M: O(1); pc covers every index.
template<typename M>
M gridPathsAvoidingRectangle(int x, int y, int x1, int y1, int x2, int y2, const PrimeCombinatorics<M> &pc) {
    if (x < 0 || y < 0) { return 0; }
    x1 = max(x1, 0); y1 = max(y1, 0); x2 = min(x2, x + 1); y2 = min(y2, y + 1);
    M res = gridPaths(x, y, pc);
    if (x1 >= x2 || y1 >= y2) { return res; }
    if (x1 == 0 && y1 == 0) { return 0; }
    // Every path into the rectangle has a unique first point, entered from below or from the left.
    for (int i = x1; i < x2; ++i) { res -= gridPaths(i, y1 - 1, pc) * gridPaths(x - i, y - y1, pc); }
    for (int j = y1; j < y2; ++j) { res -= gridPaths(x1 - 1, j, pc) * gridPaths(x - x1, y - j, pc); }
    return res;}
template<typename M>
M pathsBelowLine(int a, int b, int c, int d, int mu, const PrimeCombinatorics<M> &pc) {
    assert(mu >= 0 && b >= 0);
    if (lng(a) < lng(mu) * b || lng(c) < lng(mu) * d) { return 0; }
    M res = gridPaths(c - a, d - b, pc);
    if (mu == 0) { return res; }
    // A bad path is free up to its last point on x = mu y - 1, then steps right and stays good.
    for (int y = int(max<lng>(b, (lng(a) + mu) / mu)); y <= d && lng(mu) * y <= c; ++y) {
        int q = int(lng(mu) * y);
        res -= gridPaths(q - 1 - a, y - b, pc) * ballotGeneral(c - q, d - y, mu, false, pc);}
    return res;}
template<typename M>
M pathsBetweenLines(int x1, int y1, int x2, int y2, int lo, int hi, const PrimeCombinatorics<M> &pc) {
    assert(lo <= hi);
    lng n = lng(x2) - x1 + y2 - y1, s = lng(y1) - x1, t = lng(y2) - x2, w = 2 * (lng(hi) - lo + 2);
    if (x2 < x1 || y2 < y1 || s < lo || s > hi || t < lo || t > hi) { return 0; }
    assert(n <= pc.n);
    M res = 0;
    // Alternating reflections across y - x = lo - 1 and hi + 1; each target is reachable only within distance n.
    for (lng base : {t, 2 * (lng(hi) + 1) - t}) {
        M sum = 0;
        for (lng k = lng(floorDiv(s - n - base, w)); base + k * w <= s + n; ++k) {
            lng u = n + base + k * w - s;
            if (u >= 0 && u % 2 == 0) { sum += pc.combiNR(int(n), int(u / 2)); }}
        res += base == t ? sum : -sum;}
    return res;}
template<typename M>
M motzkinPaths(int n, int h1, int h2, const PrimeCombinatorics<M> &pc) {
    assert(0 <= n && n <= pc.n && h1 >= 0 && h2 >= 0);
    auto trinomial = [&](lng j) {
        M s = 0;
        for (lng u = j; 2 * u - j <= n; ++u) {
            s += pc.inverseFactorial(int(u)) * pc.inverseFactorial(int(u - j)) * pc.inverseFactorial(int(n - 2 * u + j));}
        return s * pc.factorial(n);};
    return trinomial(std::abs(lng(h2) - h1)) - trinomial(lng(h1) + h2 + 2);}
template<typename M>
M delannoy(int m, int n, const PrimeCombinatorics<M> &pc) {
    assert(m >= 0 && n >= 0);
    M res = 0, p = 1;
    for (int k = 0; k <= min(m, n); ++k, p += p) { res += pc.combiNR(m, k) * pc.combiNR(n, k) * p; }
    return res;}

// T: O(k^2 + k * log(k)), M: O(k); k = |pts|, paths (0, 0) -> (x, y) avoiding pts; points outside the box are ignored.
template<typename M>
M gridPathsAvoidingPoints(int x, int y, vector<pair<int, int>> pts, const PrimeCombinatorics<M> &pc) {
    std::erase_if(pts, [&](pair<int, int> p) { return p.first < 0 || p.second < 0 || p.first > x || p.second > y; });
    sort(pts.begin(), pts.end());
    pts.erase(unique(pts.begin(), pts.end()), pts.end());
    pts.pb({x, y});
    int k = int(pts.size());
    vector<M> dp(k);
    for (int i = 0; i < k; ++i) {
        dp[i] = gridPaths(pts[i].first, pts[i].second, pc);
        for (int j = 0; j < i; ++j) { dp[i] -= dp[j] * gridPaths(pts[i].first - pts[j].first, pts[i].second - pts[j].second, pc); }}
    return dp[k - 1];}

// T: O((h + w) * log(h + w)), M: O(h + w); h = |left|, w = |bottom|, pc.n >= h + w; flows into an h x w all-open rectangle, out per right row and top column.
template<typename M>
pair<vector<M>, vector<M>> gridPathsBoundaryTransfer(const vector<M> &left, const vector<M> &bottom, const PrimeCombinatorics<M> &pc) {
    int h = int(left.size()), w = int(bottom.size());
    if (h == 0 || w == 0) { return {left, bottom}; }
    assert(lng(h) + w <= pc.n);
    vector<M> k1(h), k2(w), f1(h), f2(w), g(h + w - 1);
    for (int t = 0; t < h; ++t) { k1[t] = pc.combiNR(w - 1 + t, t); f1[t] = left[t] * pc.inverseFactorial(h - 1 - t); }
    for (int t = 0; t < w; ++t) { k2[t] = pc.combiNR(h - 1 + t, t); f2[t] = bottom[t] * pc.inverseFactorial(w - 1 - t); }
    for (int t = 0; t < h + w - 1; ++t) { g[t] = pc.factorial(t); }
    // Straight transfers are convolutions; the corner ones are C(i + j, i) = (i + j)! / (i! j!) correlations.
    auto rr = convolution(left, k1), tt = convolution(bottom, k2), rb = convolution(f2, g), tl = convolution(f1, g);
    vector<M> right(h), top(w);
    for (int j = 0; j < h; ++j) { right[j] = rr[j] + rb[j + w - 1] * pc.inverseFactorial(j); }
    for (int i = 0; i < w; ++i) { top[i] = tt[i] + tl[i + h - 1] * pc.inverseFactorial(i); }
    return {right, top};}

// T: O((n + h) * log(n + h)^2), M: O(n + h); h = max(b) - a[0]; prime M::mod() > n + h + 1; nondecreasing x with a_i <= x_i < b_i (a, b any), empty n gives 1.
template<typename M = mint>
M increasingSequencesBetweenBounds(vector<int> a, vector<int> b) {
    int n = int(a.size());
    assert(int(b.size()) == n);
    if (n == 0) { return 1; }
    for (int i = 1; i < n; ++i) { a[i] = max(a[i], a[i - 1]); }
    for (int i = n - 1; i-- > 0;) { b[i] = min(b[i], b[i + 1]); }
    for (int i = 0; i < n; ++i) {
        assert(a[i] >= 0);
        if (a[i] >= b[i]) { return 0; }}
    // Points of column x lie in [lo[x], hi[x]]; the i-th right step is at height x_i.
    int h = b[n - 1] - a[0];
    vector<int> lo(n + 1), hi(n + 1);
    for (int x = 0; x <= n; ++x) { lo[x] = a[max(x - 1, 0)] - a[0]; hi[x] = b[min(x, n - 1)] - 1 - a[0]; }
    PrimeCombinatorics<M> pc(n + 1 + h);
    auto solve = [&](auto &&self, int x1, int x2, int y1, int y2, vector<M> l, vector<M> d) -> pair<vector<M>, vector<M>> {
        if (lo[x2 - 1] <= y1 && hi[x1] >= y2 - 1) { return gridPathsBoundaryTransfer(l, d, pc); }
        if (hi[x2 - 1] < y1 || lo[x1] >= y2) { return {vector<M>(y2 - y1), vector<M>(x2 - x1)}; }
        if (x2 - x1 >= y2 - y1) {
            int xm = (x1 + x2) / 2;
            auto [r1, t1] = self(self, x1, xm, y1, y2, l, vector<M>(d.begin(), d.begin() + (xm - x1)));
            auto [r2, t2] = self(self, xm, x2, y1, y2, r1, vector<M>(d.begin() + (xm - x1), d.end()));
            t1.insert(t1.end(), t2.begin(), t2.end());
            return {r2, t1};}
        int ym = (y1 + y2) / 2;
        auto [r1, t1] = self(self, x1, x2, y1, ym, vector<M>(l.begin(), l.begin() + (ym - y1)), d);
        auto [r2, t2] = self(self, x1, x2, ym, y2, vector<M>(l.begin() + (ym - y1), l.end()), t1);
        r1.insert(r1.end(), r2.begin(), r2.end());
        return {r1, t2};};
    vector<M> l(h), d(n + 1);
    l[0] = 1;
    return solve(solve, 0, n + 1, 0, h, l, d).first[h - 1];}

// T: O((a + b) * n + g * n * log(g * n)), M: O((a + b) * n); g = gcd(a, b), prime M::mod() > (a + b) * n, a, b >= 1; paths (0, 0) -> (a i, b i) weakly below the line, i in [0, n].
template<typename M = mint>
vector<M> bizleyPaths(int a, int b, int n) {
    assert(a >= 1 && b >= 1 && n >= 0);
    int g = gcd(a, b), s = (a + b) / g, m = g * n;
    assert(lng(s) * m < std::numeric_limits<int>::max());
    PrimeCombinatorics<M> pc(s * m);
    vector<M> f(m + 1, 0), res(n + 1);
    for (int j = 1; j <= m; ++j) { f[j] = pc.combiNR(s * j, a / g * j) * pc.inverseFactorial(s * j) * pc.factorial(s * j - 1); }
    f = fpsExp(f, m + 1);
    for (int i = 0; i <= n; ++i) { res[i] = f[g * i]; }
    return res;}

// T: O(k^3), M: O(k^2); k = |s| = |t|, prime M::mod(); det[paths s_i -> t_j], the count of vertex-disjoint path systems when only the identity matching is possible.
template<typename M>
M nonIntersectingPathsDeterminant(const vector<pair<int, int>> &s, const vector<pair<int, int>> &t, const PrimeCombinatorics<M> &pc) {
    int k = int(s.size());
    assert(int(t.size()) == k);
    vector<vector<M>> a(k, vector<M>(k));
    for (int i = 0; i < k; ++i) { for (int j = 0; j < k; ++j) { a[i][j] = gridPaths(t[j].first - s[i].first, t[j].second - s[i].second, pc); } }
    M res = 1;
    for (int c = 0; c < k; ++c) {
        int r = c;
        while (r < k && a[r][c] == 0) { ++r; }
        if (r == k) { return 0; }
        if (r != c) { swap(a[r], a[c]); res = -res; }
        res *= a[c][c];
        M iv = inv(a[c][c]);
        for (int i = c + 1; i < k; ++i) {
            M f = a[i][c] * iv;
            for (int j = c; j < k; ++j) { a[i][j] -= f * a[c][j]; }}}
    return res;}

// T: O(n^2) triangle, O(n) others, M: O(n^2) triangle, O(n) others; triangle any ring T, others prime M::mod() > n + 2.
template<typename T>
vector<vector<T>> catalanTriangle(int n) {
    assert(n >= 0); vector<vector<T>> c(n + 1);
    for (int i = 0; i <= n; ++i) {
        c[i].assign(i + 1, T(1));
        for (int j = 1; j <= i; ++j) { c[i][j] = c[i][j - 1] + (j < i ? c[i - 1][j] : T(0)); }}
    return c;}
template<typename M = mint>
vector<M> motzkinTable(int n) {
    assert(n >= 0);
    auto iv = inverseTable<M>(n + 2);
    vector<M> res(n + 1, 1);
    for (int i = 2; i <= n; ++i) { res[i] = (M(2 * lng(i) + 1) * res[i - 1] + M(3 * lng(i) - 3) * res[i - 2]) * iv[i + 2]; }
    return res;}
template<typename M = mint> vector<M> schroderLarge(int n) { return lattice_paths_detail::schroder<M>(n, 2); }
template<typename M = mint> vector<M> schroderSmall(int n) { return lattice_paths_detail::schroder<M>(n, 1); }
template<typename M = mint>
vector<M> delannoyCentral(int n) {
    assert(n >= 0);
    auto iv = inverseTable<M>(n);
    vector<M> res(n + 1, 1);
    if (n >= 1) { res[1] = 3; }
    for (int i = 2; i <= n; ++i) { res[i] = (M(6 * lng(i) - 3) * res[i - 1] - M(i - 1) * res[i - 2]) * iv[i]; }
    return res;}
