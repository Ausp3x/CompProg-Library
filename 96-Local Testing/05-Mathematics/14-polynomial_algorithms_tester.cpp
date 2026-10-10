#include "../../05-Mathematics/14-polynomial_algorithms.hpp"

lng checks = 0;
std::mt19937_64 rng;
bool quick = false, stress = false, fast = false;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
string str(lng x) { return std::to_string(x); }
template<typename M> vector<M> rnd(int n) {
    vector<M> v(n);
    for (auto &x : v) { x = M(rng()); }
    return v;}
template<typename M> vector<M> rndNz(int n) {
    auto v = rnd<M>(n);
    if (n && v.back() == M(0)) { v.back() = 1; }
    return v;}
template<typename M> vector<M> trimmed(vector<M> a) {
    while (!a.empty() && a.back() == M(0)) { a.pop_back(); }
    return a;}
template<typename M> int dg(const vector<M> &a) { return int(trimmed(a).size()) - 1; }
template<typename M> vector<M> bmul(const vector<M> &a, const vector<M> &b) {
    if (a.empty() || b.empty()) { return {}; }
    vector<M> c(a.size() + b.size() - 1);
    for (size_t i = 0; i < a.size(); ++i) { for (size_t j = 0; j < b.size(); ++j) { c[i + j] += a[i] * b[j]; } }
    return c;}
template<typename M> vector<M> bmulT(const vector<M> &a, const vector<M> &b, int n) {
    vector<M> c(n);
    for (int i = 0; i < min(n, int(a.size())); ++i) { for (int j = 0; i + j < n && j < int(b.size()); ++j) { c[i + j] += a[i] * b[j]; } }
    return c;}
template<typename M> vector<M> badd(vector<M> a, const vector<M> &b, M s = M(1)) {
    if (a.size() < b.size()) { a.resize(b.size()); }
    for (size_t i = 0; i < b.size(); ++i) { a[i] += s * b[i]; }
    return trimmed(a);}
template<typename M> pair<vector<M>, vector<M>> bdivmod(vector<M> a, vector<M> b) {
    a = trimmed(a); b = trimmed(b);
    if (a.size() < b.size()) { return {{}, a}; }
    vector<M> q(a.size() - b.size() + 1);
    M ic = M(1) / b.back();
    for (int i = int(q.size()) - 1; i >= 0; --i) {
        q[i] = a[i + b.size() - 1] * ic;
        for (size_t j = 0; j < b.size(); ++j) { a[i + j] -= q[i] * b[j]; }}
    return {trimmed(q), trimmed(a)};}
template<typename M> vector<M> bmod(const vector<M> &a, const vector<M> &b) { return bdivmod(a, b).second; }
template<typename M> vector<M> bder(const vector<M> &a) {
    vector<M> r;
    for (size_t i = 1; i < a.size(); ++i) { r.pb(a[i] * M(lng(i))); }
    return r;}
template<typename M> M horner(const vector<M> &a, M x) {
    M r = 0;
    for (int i = int(a.size()) - 1; i >= 0; --i) { r = r * x + a[i]; }
    return r;}
template<typename M> M bpow(M a, ulng e) {
    M r = 1;
    for (; e; e >>= 1, a = a * a) { if (e & 1) { r = r * a; } }
    return r;}
template<typename M> M det(vector<vector<M>> m) {
    int n = int(m.size());
    M r = 1;
    for (int c = 0; c < n; ++c) {
        int p = c;
        while (p < n && m[p][c] == M(0)) { ++p; }
        if (p == n) { return 0; }
        if (p != c) { swap(m[p], m[c]); r = -r; }
        r *= m[c][c];
        M ic = M(1) / m[c][c];
        for (int i = c + 1; i < n; ++i) {
            M f = m[i][c] * ic;
            for (int j = c; j < n; ++j) { m[i][j] -= f * m[c][j]; }}}
    return r;}
// Sylvester determinant with formal degrees n >= deg a and m >= deg b (leading entries may vanish).
template<typename M> M sylvesterFormal(vector<M> a, int n, vector<M> b, int m) {
    a.resize(n + 1); b.resize(m + 1);
    vector<vector<M>> s(n + m, vector<M>(n + m));
    for (int i = 0; i < m; ++i) { for (int j = 0; j <= n; ++j) { s[i][i + j] = a[n - j]; } }
    for (int i = 0; i < n; ++i) { for (int j = 0; j <= m; ++j) { s[m + i][i + j] = b[m - j]; } }
    return det(s);}
template<typename M> M sylvester(vector<M> a, vector<M> b) {
    a = trimmed(a); b = trimmed(b);
    if (a.empty() || b.empty()) { return 0; }
    return sylvesterFormal(a, int(a.size()) - 1, b, int(b.size()) - 1);}
template<typename M> vector<M> monic(vector<M> a) {
    a = trimmed(a);
    if (a.empty()) { return a; }
    M c = M(1) / a.back();
    for (auto &x : a) { x *= c; }
    return a;}
// Extended Euclid by repeated long division: (g, s, t) with s a + t b = g monic.
template<typename M> array<vector<M>, 3> bxgcd(vector<M> a, vector<M> b) {
    a = trimmed(a); b = trimmed(b);
    vector<M> s0{1}, t0{}, s1{}, t1{1};
    if (a.empty() && b.empty()) { return {}; }
    while (!b.empty()) {
        auto [q, r] = bdivmod(a, b);
        auto s2 = badd(s0, bmul(q, s1), M(-1)), t2 = badd(t0, bmul(q, t1), M(-1));
        a = b; b = r; s0 = s1; t0 = t1; s1 = s2; t1 = t2;}
    M c = M(1) / a.back();
    for (auto *p : {&a, &s0, &t0}) { for (auto &x : *p) { x *= c; } }
    return {a, s0, t0};}
template<typename M> vector<M> bpowT(vector<M> a, ulng k, int n) {
    vector<M> r(n);
    if (n) { r[0] = 1; }
    a.resize(n);
    for (; k; k >>= 1, a = bmulT(a, a, n)) { if (k & 1) { r = bmulT(r, a, n); } }
    return r;}
template<typename M> vector<M> binvT(const vector<M> &a, int n) {
    vector<M> g(n);
    M c = M(1) / a[0];
    for (int i = 0; i < n; ++i) {
        M s = i ? M(0) : M(1);
        for (int j = 1; j <= i && j < int(a.size()); ++j) { s -= a[j] * g[i - j]; }
        g[i] = s * c;}
    return g;}
using namespace polynomial_detail;
template<typename M> int cutOf(Cut c) { return cut<M>(c); }
template<typename M> vector<M> unit(int n) {
    vector<M> r(n);
    if (n) { r[0] = 1; }
    return r;}

// Each FPS result is checked by the identity that determines it uniquely.
template<typename M> bool isInverse(const vector<M> &a, const vector<M> &g, int n, bool fast) {
    if (int(g.size()) != n) { return false; }
    return (fast ? truncatedConvolution(a, g, n) : bmulT(a, g, n)) == unit<M>(n);}
template<typename M> bool isLog(const vector<M> &a, const vector<M> &b, int n, bool fast) {
    if (int(b.size()) != n || (n && b[0] != M(0))) { return false; }
    if (n <= 1) { return true; }
    auto l = fast ? truncatedConvolution(a, bder(b), n - 1) : bmulT(a, bder(b), n - 1), r = bder(vector<M>(a.begin(), a.begin() + min(int(a.size()), n)));
    r.resize(n - 1);
    return l == r;}
template<typename M> bool isExp(const vector<M> &a, const vector<M> &f, int n, bool fast) {
    if (int(f.size()) != n || (n && f[0] != M(1))) { return false; }
    if (n <= 1) { return true; }
    auto da = bder(vector<M>(a.begin(), a.begin() + min(int(a.size()), n)));
    auto l = fast ? truncatedConvolution(f, da, n - 1) : bmulT(f, da, n - 1), r = bder(f);
    r.resize(n - 1);
    return l == r;}

template<typename M>
void basicSuite(const string &name) {
    const int cap = int(min<ulng>(12, M::mod() - 4));
    for (int rep = 0; rep < (quick ? 50 : 400); ++rep) {
        int n = int(rng() % cap), m = int(rng() % 12);
        auto a = rnd<M>(n), b = rnd<M>(m);
        if (rng() % 3 == 0) { a.resize(n + 3); }
        check(polyAdd(a, b) == badd(a, b) && polySub(a, b) == badd(a, b, M(-1)), name + " add/sub");
        check(polyMul(a, b) == trimmed(bmul(a, b)), name + " mul");
        check(polyDerivative(a) == trimmed(bder(a)), name + " derivative");
        auto in = polyIntegral(a);
        check(trimmed(bder(in)) == trimmed(a) && (in.empty() || in[0] == M(0)), name + " integral");
        M x = M(rng());
        check(polyEval(a, x) == horner(a, x), name + " eval");
        int k = int(rng() % 9) - 4;
        auto s = polyShift(a, k), e = trimmed(a);
        if (k >= 0 && !e.empty()) { e.insert(e.begin(), k, M(0)); }
        if (k < 0) { e = int(e.size()) > -k ? vector<M>(e.begin() - k, e.end()) : vector<M>(); }
        check(s == e, name + " shift k=" + str(k));
        auto t = polyTruncate(a, m);
        check(int(t.size()) == m && trimmed(t) == trimmed(vector<M>(a.begin(), a.begin() + min(n, m))), name + " truncate");
        check(polyNormalize(a) == trimmed(a), name + " normalize");
        auto [q, r] = polyDivLinear(a, x);
        check(badd(bmul(q, vector<M>{-x, 1}), vector<M>{r}) == trimmed(a) && q == trimmed(q) && r == horner(a, x), name + " polyDivLinear");}
    check(polyDivLinear(vector<M>{}, M(3)).first.empty() && polyDivLinear(vector<M>{0, 0}, M(3)).second == M(0), name + " polyDivLinear zero");
    cout << "PASS " << name << ": basic operations, polyDivLinear\n";}

template<typename M>
void divSuite(const string &name, bool big) {
    vector<pair<int, int>> sizes{{0, 1}, {1, 1}, {5, 1}, {5, 6}, {10, 4}, {127, 2}, {129, 64}, {300, 129}, {300, 150}, {257, 128}, {600, 300}, {1000, 3}};
    if (!quick) { sizes.insert(sizes.end(), {{2000, 1000}, {3000, 129}, {2500, 2400}, {4000, 1500}}); }
    for (int c : {cutOf<M>(DIV), cutOf<M>(DIV) + 1}) { sizes.pb({2 * c, c + 1}); sizes.pb({2 * c - 1, c}); }
    for (auto [n, m] : sizes) {
        auto a = rnd<M>(n), b = rndNz<M>(m);
        auto [q, r] = polyDivmod(a, b);
        check(q == trimmed(q) && r == trimmed(r) && int(r.size()) < m && badd(bmul(q, b), r) == trimmed(a), name + " divmod identity n=" + str(n) + " m=" + str(m));
        check(polyMod(a, b) == r, name + " mod");
        if (lng(n) * m <= 4000000) { check(bdivmod(a, b) == pair{q, r}, name + " divmod brute"); }}
    if (big) {
        int n = quick ? 20000 : fast ? 400000 : 50000, m = n * 3 / 8;
        auto a = rnd<M>(n), b = rndNz<M>(m);
        auto [q, r] = polyDivmod(a, b);
        auto c = convolution(b, q);
        for (size_t i = 0; i < r.size(); ++i) { c[i] += r[i]; }
        check(trimmed(c) == trimmed(a) && int(r.size()) < m, name + " divmod large identity");}
    auto [q, r] = polyDivmod(vector<M>{1, 2, 0, 0}, vector<M>{3, 0});
    check(q == vector<M>{M(1) / M(3), M(2) / M(3)} && r.empty(), name + " divmod trailing zeros");
    cout << "PASS " << name << ": polyDivmod, polyMod\n";}

template<typename M>
void fpsSuite(const string &name, int bigN) {
    vector<int> ns{0, 1, 2, 3, 17, 64, 65, 127, 128, 129, 130, 255, 256, 257, 500};
    if (!quick) { ns.insert(ns.end(), {1000, 2049}); }
    if (fast) { ns.insert(ns.end(), {1500, 2047}); }
    for (Cut c : {INV, LOG, EXP}) { for (int d : {0, 1, 2}) { if (fast || cutOf<M>(c) < 1000) { ns.pb(cutOf<M>(c) + d); } } }
    for (int n : ns) {
        if (ulng(n) > M::mod()) { continue; }
        int len = int(rng() % (n + 3));
        auto a = rnd<M>(len);
        if (a.empty() || a[0] == M(0)) { a.resize(max(len, 1)); a[0] = 1; }
        auto g = fpsInverse(a, n);
        check(isInverse(a, g, n, false), name + " fpsInverse n=" + str(n));
        check(fpsSparseInverse(a, n) == g, name + " fpsSparseInverse n=" + str(n));
        auto f = rnd<M>(int(rng() % (n + 3)));
        auto d = fpsDiv(f, a, n);
        check(int(d.size()) == n && bmulT(d, a, n) == bmulT(f, unit<M>(1), n), name + " fpsDiv n=" + str(n));
        auto u = a;
        u[0] = 1;
        auto l = fpsLog(u, n);
        check(isLog(u, l, n, false), name + " fpsLog n=" + str(n));
        check(fpsSparseLog(u, n) == l, name + " fpsSparseLog n=" + str(n));
        auto z = rnd<M>(int(rng() % (n + 3)));
        if (!z.empty()) { z[0] = 0; }
        auto e = fpsExp(z, n);
        check(isExp(z, e, n, false), name + " fpsExp n=" + str(n));
        check(fpsSparseExp(z, n) == e, name + " fpsSparseExp n=" + str(n));
        check(fpsLog(e.empty() ? vector<M>{1} : e, n) == polyTruncate(z, n), name + " log(exp) n=" + str(n));}
    for (int n : {bigN / 3, bigN}) {
        auto a = rnd<M>(n);
        a[0] = 1;
        check(isInverse(a, fpsInverse(a, n), n, true), name + " fpsInverse large n=" + str(n));
        check(isLog(a, fpsLog(a, n), n, true), name + " fpsLog large n=" + str(n));
        a[0] = 0;
        check(isExp(a, fpsExp(a, n), n, true), name + " fpsExp large n=" + str(n));}
    {
        // Sparse inputs with few terms against the dense versions at sizes above the naive thresholds.
        int n = quick ? 600 : 3000;
        vector<M> a(n);
        a[0] = 1;
        for (int t = 0; t < 6; ++t) { a[rng() % n] = M(rng()); }
        a[0] = 1;
        check(fpsSparseInverse(a, n) == fpsInverse(a, n) && fpsSparseLog(a, n) == fpsLog(a, n), name + " sparse inverse/log dense agreement");
        a[0] = 0;
        check(fpsSparseExp(a, n) == fpsExp(a, n), name + " sparse exp dense agreement");}
    cout << "PASS " << name << ": fpsInverse, fpsDiv, fpsLog, fpsExp and sparse inverse/log/exp\n";}

template<typename M>
void powSuite(const string &name, int bigN) {
    for (int rep = 0; rep < (quick ? 60 : 400); ++rep) {
        int n = int(rng() % 140) + (rep % 4 == 0 ? 0 : 1);
        if (ulng(n) > M::mod()) { n = int(M::mod()); }
        int len = int(rng() % 12), v = int(rng() % 4) * (rng() % 2);
        auto a = rnd<M>(len);
        if (rng() % 3 == 0) { for (auto &x : a) { if (rng() % 2) { x = 0; } } }
        a.insert(a.begin(), v, M(0));
        int vv = 0;
        while (vv < int(a.size()) && a[vv] == M(0)) { ++vv; }
        lng k = rep % 5 == 0 ? lng(rng() >> 2) : lng(rng() % 20);
        auto want = bpowT(a, ulng(k), n);
        bool fits = vv == int(a.size()) || k == 0 || lll(vv) * k >= n || n - vv * k <= lng(M::mod());
        if (!fits) { continue; }
        check(fpsPow(a, k, n) == want, name + " fpsPow n=" + str(n) + " k=" + str(k) + " v=" + str(vv));
        check(fpsSparsePow(a, k, n) == want, name + " fpsSparsePow n=" + str(n) + " k=" + str(k));
        if (vv == 0 && !a.empty()) {
            lng kn = -(lng(rng() % 30) + 1);
            check(fpsPow(a, kn, n) == bpowT(binvT(a, n), ulng(-kn), n), name + " fpsPow negative k=" + str(kn));}}
    for (int d : {0, 1, 2}) {
        int n = cutOf<M>(POW) + d;
        if (ulng(n) > M::mod()) { continue; }
        auto a = rnd<M>(n);
        for (lng k : {lng(3), lng(rng() % 1000)}) { check(fpsPow(a, k, n) == bpowT(a, ulng(k), n) && fpsSparsePow(a, k, n) == bpowT(a, ulng(k), n), name + " fpsPow cut neighbour n=" + str(n)); }}
    check(fpsPow(vector<M>{}, 0, 3) == unit<M>(3) && fpsPow(vector<M>{0, 0}, 5, 3) == vector<M>(3) && fpsPow(vector<M>{1}, 7, 0).empty(), name + " fpsPow zero cases");
    check(fpsPow(vector<M>{2, 3}, LLONG_MIN, 2) == vector<M>{bpow(M(1) / M(2), ulng(1) << 63), M(3) * M(LLONG_MIN) * bpow(M(1) / M(2), (ulng(1) << 63) + 1)}, name + " fpsPow LLONG_MIN");
    {
        int n = bigN;
        auto a = rnd<M>(n);
        a[0] = 0; a[1] = 0; a[2] = 5;
        auto r = fpsPow(a, 3, n);
        auto u = vector<M>(a.begin() + 2, a.end());
        auto c = truncatedConvolution(truncatedConvolution(u, u, n), u, n);
        bool ok = r[0] == M(0) && r[5] == M(0);
        for (int i = 6; i < n && ok; ++i) { ok = r[i] == c[i - 6]; }
        check(ok, name + " fpsPow large cube");}
    // Rational exponents p / q: result^q = a^p; no answer when the valuation does not divide.
    for (int rep = 0; rep < (quick ? 40 : 300); ++rep) {
        int n = int(rng() % 90) + 1, q = int(rng() % 4) + 1, p = int(rng() % 9) - 3, v = q * int(rng() % 3);
        if (ulng(n) > M::mod()) { n = int(M::mod()); }
        auto a = rnd<M>(int(rng() % 10) + 1);
        a[0] = 1;
        a.insert(a.begin(), v, M(0));
        if (p < 0) { a.erase(a.begin(), a.begin() + v); v = 0; }
        auto r = fpsPow(a, p, q, n);
        check(int(r.size()) == n, name + " fpsPow rational size");
        auto lhs = bpowT(r, ulng(q), n), rhs = p >= 0 ? bpowT(a, ulng(p), n) : bpowT(binvT(a, n), ulng(-p), n);
        check(lhs == rhs, name + " fpsPow rational p=" + str(p) + " q=" + str(q) + " n=" + str(n));
        int s = v * max(p, 0) / q;
        bool lead = true;
        for (int i = 0; i < min(s, n); ++i) { lead = lead && r[i] == M(0); }
        check(lead && (s >= n || r[s] == M(1)), name + " fpsPow rational branch");}
    check(fpsPow(vector<M>{0, 1}, 1, 2, 4).empty() && fpsPow(vector<M>{0, 1}, -2, 1, 4).empty() && fpsPow(vector<M>{}, -1, 2, 3).empty(), name + " fpsPow rational no answer");
    check(fpsPow(vector<M>{}, 0, 3, 3) == unit<M>(3) && fpsPow(vector<M>{0}, 2, 3, 2) == vector<M>(2) && fpsPow(vector<M>{0, 0, 1}, 3, 2, 3) == vector<M>(3), name + " fpsPow rational zero cases");
    cout << "PASS " << name << ": fpsPow (integer, negative, rational), fpsSparsePow\n";}

template<typename M>
void sqrtSuite(const string &name, int bigN) {
    for (int rep = 0; rep < (quick ? 80 : 600); ++rep) {
        int n = int(rng() % 300), len = int(rng() % 8), v = int(rng() % 5);
        auto a = rnd<M>(len);
        a.insert(a.begin(), v, M(0));
        if (rep % 4 == 0 && len) { a[v] = bpow(M(rng() % 1000 + 1), 2); }
        auto r = fpsSqrt(a, n);
        check(fpsSparseSqrt(a, n) == r, name + " fpsSparseSqrt n=" + str(n));
        int vv = 0;
        while (vv < int(a.size()) && a[vv] == M(0)) { ++vv; }
        bool zero = vv == int(a.size());
        bool exists = zero || (vv % 2 == 0 && bpow(a[vv], (M::mod() - 1) / 2) == M(1));
        if (!exists) { check(r.empty(), name + " fpsSqrt no root n=" + str(n)); continue; }
        check(int(r.size()) == n && bmulT(r, r, n) == polyTruncate(a, n), name + " fpsSqrt square n=" + str(n) + " v=" + str(vv));
        if (!zero && vv / 2 < n) { check(r[vv / 2].val() <= (-r[vv / 2]).val() && r[vv / 2] != M(0), name + " fpsSqrt branch"); }}
    for (int d : {0, 1, 2}) {
        int n = cutOf<M>(SQRT) + d;
        auto a = rnd<M>(n);
        a[0] = 9;
        auto r = fpsSqrt(a, n);
        check(r == fpsSparseSqrt(a, n) && bmulT(r, r, n) == a && r[0] == M(3), name + " fpsSqrt cut neighbour n=" + str(n));}
    {
        auto a = rnd<M>(bigN);
        a[0] = 4;
        auto r = fpsSqrt(a, bigN);
        check(truncatedConvolution(r, r, bigN) == a && r[0] == M(2), name + " fpsSqrt large");}
    cout << "PASS " << name << ": fpsSqrt, fpsSparseSqrt\n";}

template<typename M>
void gcdSuite(const string &name, bool big) {
    vector<array<int, 3>> shapes{{0, 0, 0}, {3, 0, 0}, {0, 4, 0}, {1, 1, 0}, {5, 5, 0}, {6, 3, 2}, {40, 30, 5}, {130, 100, 0}, {200, 199, 20}, {300, 140, 60}, {260, 255, 1}};
    if (!quick) { shapes.insert(shapes.end(), {{700, 650, 0}, {900, 500, 300}, {1200, 1100, 7}}); }
    if (!quick && fast) { for (int d : {-1, 0, 1}) { shapes.pb({cutOf<M>(GCD) + d - 2, cutOf<M>(GCD) - 5, 2}); } }
    for (auto [n, m, gd] : shapes) {
        auto g = rndNz<M>(gd + 1);
        auto a = n ? bmul(g, rndNz<M>(n)) : vector<M>(), b = m ? bmul(g, rndNz<M>(m)) : vector<M>();
        if (rng() % 2) { a.resize(a.size() + 2); }
        auto want = bxgcd(a, b);
        check(polyGcd(a, b) == want[0], name + " polyGcd brute n=" + str(n) + " m=" + str(m));
        auto x = polyXgcd(a, b);
        check(x.g == want[0] && x.x == trimmed(want[1]) && x.y == trimmed(want[2]), name + " polyXgcd brute n=" + str(n) + " m=" + str(m));
        check(badd(bmul(x.x, a), bmul(x.y, b)) == x.g, name + " polyXgcd identity");
        if (dg(a) + dg(b) <= (quick ? 260 : fast ? 600 : 300) && !trimmed(a).empty() && !trimmed(b).empty()) {
            check(polyResultant(a, b) == sylvester(a, b), name + " polyResultant Sylvester n=" + str(n) + " m=" + str(m));
            check(polyResultant(b, a) == sylvester(b, a), name + " polyResultant swapped");}
        if (dg(a) > dg(b)) {
            auto t = polyHalfGcd(a, b);
            auto c = badd(bmul(t[0], a), bmul(t[1], b)), d = badd(bmul(t[2], a), bmul(t[3], b));
            int h = (dg(a) + 1) / 2;
            vector<M> r0 = trimmed(a), r1 = trimmed(b), s0{1}, t0{}, s1{}, t1{1};
            while (dg(r1) >= h) {
                auto [q, r] = bdivmod(r0, r1);
                auto s2 = badd(s0, bmul(q, s1), M(-1)), t2 = badd(t0, bmul(q, t1), M(-1));
                r0 = r1; r1 = r; s0 = s1; t0 = t1; s1 = s2; t1 = t2;}
            check(c == r0 && d == r1 && t[0] == s0 && t[1] == t0 && t[2] == s1 && t[3] == t1, name + " polyHalfGcd brute n=" + str(n) + " m=" + str(m));}}
    for (int rep = 0; rep < (quick ? 100 : 1000); ++rep) {
        int n = int(rng() % 7), m = int(rng() % 7);
        auto a = rnd<M>(n), b = rnd<M>(m);
        if (rng() % 2) { for (auto &x : a) { x = M(rng() % 3); } for (auto &x : b) { x = M(rng() % 3); } }
        check(polyResultant(a, b) == sylvester(a, b), name + " polyResultant small");
        auto w = bxgcd(a, b);
        auto x = polyXgcd(a, b);
        check(polyGcd(a, b) == w[0] && x.g == w[0] && x.x == trimmed(w[1]) && x.y == trimmed(w[2]), name + " polyXgcd small");
        if (dg(a) >= 1) {
            int deg = dg(a);
            M disc = sylvesterFormal(trimmed(a), deg, bder(trimmed(a)), deg - 1) / trimmed(a).back();
            if (lng(deg) * (deg - 1) / 2 % 2) { disc = -disc; }
            check(polyDiscriminant(a) == disc, name + " polyDiscriminant small");}}
    {
        // Roots formulas: Res(a, b) = prod b(r_i), disc = prod_{i < j} (r_i - r_j)^2 for monic a.
        int n = quick ? 300 : 1500, m = quick ? 250 : 1400;
        auto rs = rnd<M>(n);
        if (n > 5) { rs[3] = rs[1]; }
        auto a = polyFromRoots(rs), b = rndNz<M>(m + 1);
        M res = 1, disc = 1;
        for (auto r : rs) { res *= horner(b, r); }
        for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { disc *= (rs[i] - rs[j]) * (rs[i] - rs[j]); } }
        check(polyResultant(a, b) == res, name + " polyResultant roots formula n=" + str(n));
        check(polyDiscriminant(a) == disc && disc == M(0), name + " polyDiscriminant repeated root");
        rs[3] = rs[3] + M(1);
        a = polyFromRoots(rs);
        disc = 1;
        for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { disc *= (rs[i] - rs[j]) * (rs[i] - rs[j]); } }
        check(polyDiscriminant(a) == disc, name + " polyDiscriminant roots formula");}
    if (!quick && (fast || cutOf<M>(GCD) < 4000)) {
        // Above cut(GCD) the resultant reads the quotient log through the half-GCD recursion.
        int n = cutOf<M>(GCD) + 400, m = cutOf<M>(GCD) + 100;
        auto rs = rnd<M>(n);
        auto a = polyFromRoots(rs), b = rndNz<M>(m + 1);
        M res = 1, disc = 1, sw = lng(n) * m % 2 ? M(-1) : M(1);
        for (auto r : rs) { res *= horner(b, r); }
        for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { disc *= (rs[i] - rs[j]) * (rs[i] - rs[j]); } }
        check(polyResultant(a, b) == res && polyResultant(b, a) == sw * res && polyDiscriminant(a) == disc, name + " polyResultant/polyDiscriminant above cut(GCD) n=" + str(n));
        auto c = polyMul(a, vector<M>{-rs[5], 1});
        check(polyResultant(c, polyMul(b, vector<M>{-rs[7], 1})) == M(0) && polyDiscriminant(c) == M(0), name + " polyResultant common root above cut(GCD)");}
    if (big) {
        int n = quick ? 3000 : fast ? 60000 : 8000;
        auto g = rndNz<M>(n / 3), a = polyMul(g, rndNz<M>(n)), b = polyMul(g, rndNz<M>(n - 7));
        auto x = polyXgcd(a, b);
        auto c = polyAdd(polyMul(x.x, a), polyMul(x.y, b));
        check(c == x.g && polyMod(a, x.g).empty() && polyMod(b, x.g).empty() && x.g.size() >= g.size() && polyGcd(a, b) == x.g, name + " polyXgcd large identity n=" + str(n));
        check(int(x.x.size()) <= dg(b) - dg(x.g) && int(x.y.size()) <= dg(a) - dg(x.g), name + " polyXgcd cofactor degrees");
        auto t = polyHalfGcd(a, b);
        auto r0 = polyAdd(polyMul(t[0], a), polyMul(t[1], b)), r1 = polyAdd(polyMul(t[2], a), polyMul(t[3], b));
        int h = (dg(a) + 1) / 2;
        check(dg(r0) >= h && dg(r1) < h && polyMod(r0, x.g).empty() && polyMod(r1, x.g).empty(), name + " polyHalfGcd large degrees");
        check(polySub(polyMul(t[0], t[3]), polyMul(t[1], t[2])).size() == 1, name + " polyHalfGcd unimodular");}
    cout << "PASS " << name << ": polyGcd, polyXgcd, polyHalfGcd, polyResultant, polyDiscriminant\n";}

template<typename M>
void modSuite(const string &name) {
    for (int rep = 0; rep < (quick ? 30 : fast ? 150 : 50); ++rep) {
        int d = int(rng() % 60) + (rep % 10 == 0 ? 0 : 1);
        if (rep % 7 == 0) { d = 126 + int(rng() % 6); }
        if (rep % 25 == 1) { d = fast ? 400 : 200; }
        auto m = rndNz<M>(d + 1), a = rnd<M>(int(rng() % (2 * d + 5)));
        ulng e = rep % 3 ? rng() : rng() % 10;
        vector<M> want = bmod(vector<M>{1}, m), x = bmod(a, m);
        for (ulng t = e; t; t >>= 1, x = bmod(bmul(x, x), m)) { if (t & 1) { want = bmod(bmul(want, x), m); } }
        check(polyPowMod(a, e, m) == want, name + " polyPowMod d=" + str(d) + " e=" + str(lng(e % 1000)));
        auto f = rnd<M>(int(rng() % 60)), g = rnd<M>(int(rng() % (d + 3)));
        vector<M> h;
        for (int i = int(f.size()) - 1; i >= 0; --i) { h = bmod(badd(bmul(h, g), vector<M>{f[i]}), m); }
        check(polyComposeMod(f, g, m) == h, name + " polyComposeMod d=" + str(d));
        if (d >= 1) {
            auto iv = polyInverseMod(a, m);
            bool unit = bxgcd(a, m)[0] == vector<M>{1};
            check(unit ? bmod(bmul(iv, a), m) == vector<M>{1} && int(iv.size()) <= d : iv.empty(), name + " polyInverseMod d=" + str(d));
            auto c = bmul(m, rndNz<M>(3));
            check(polyInverseMod(c, m).empty() && polyInverseMod(vector<M>{}, m).empty(), name + " polyInverseMod no inverse");}}
    check(polyPowMod(vector<M>{0, 1}, 0, vector<M>{5}).empty() && polyPowMod(vector<M>{}, 0, vector<M>{1, 1}) == vector<M>{1} && polyPowMod(vector<M>{}, 3, vector<M>{1, 1}).empty(), name + " polyPowMod edges");
    cout << "PASS " << name << ": polyPowMod, polyInverseMod, polyComposeMod\n";}

template<typename M>
void compositionSuite(const string &name) {
    for (int rep = 0; rep < (quick ? 25 : 40); ++rep) {
        int n = int(rng() % 200), k = int(rng() % 220);
        auto f = rnd<M>(k), g = rnd<M>(int(rng() % (n + 3)));
        if (rep % 2 && !g.empty()) { g[0] = 0; }
        vector<M> h(n);
        for (int i = k - 1; i >= 0; --i) { h = bmulT(h, g, n); if (n) { h[0] += f[i]; } }
        check(fpsComposition(f, g, n) == h, name + " fpsComposition n=" + str(n) + " k=" + str(k));
        if (n >= 1) {
            auto F = rnd<M>(int(rng() % (n + 3)) + 2);
            F[0] = 0;
            if (F[1] == M(0)) { F[1] = 1; }
            auto G = fpsCompositionalInverse(F, n);
            vector<M> c(n), x(n);
            for (int i = int(F.size()) - 1; i >= 0; --i) { c = bmulT(c, G, n); c[0] += F[i]; }
            if (n > 1) { x[1] = 1; }
            check(c == x && int(G.size()) == n && G[0] == M(0), name + " fpsCompositionalInverse n=" + str(n));}}
    check(fpsComposition(vector<M>{}, vector<M>{1}, 3) == vector<M>(3) && fpsComposition(vector<M>{2, 1}, vector<M>{}, 2) == vector<M>{2, 0}, name + " fpsComposition edges");
    check(fpsCompositionalInverse(vector<M>{0, 2}, 0).empty() && fpsCompositionalInverse(vector<M>{0, 2}, 1) == vector<M>{0}, name + " fpsCompositionalInverse edges");
    cout << "PASS " << name << ": fpsComposition, fpsCompositionalInverse\n";}

template<typename M>
void miscSuite(const string &name, int bigN) {
    vector<int> ns{0, 1, 2, 7, 128, 129, 300, 1000, cutOf<M>(SHIFT), cutOf<M>(SHIFT) + 1};
    for (int n : ns) {
        auto a = rnd<M>(n);
        M c = M(rng());
        vector<M> want;
        for (int i = n - 1; i >= 0; --i) { want = badd(bmul(want, vector<M>{c, 1}), vector<M>{a[i]}); }
        want.resize(n);
        check(taylorShift(a, c) == want, name + " taylorShift n=" + str(n));}
    {
        auto a = rnd<M>(bigN);
        M c = M(rng()), x = M(rng());
        check(polyEval(taylorShift(a, c), x) == polyEval(a, x + c), name + " taylorShift large evaluation");}
    for (int rep = 0; rep < (quick ? 20 : 100); ++rep) {
        int k = int(rng() % 12);
        vector<vector<M>> ps;
        vector<M> want{1};
        for (int i = 0; i < k; ++i) { ps.pb(rnd<M>(int(rng() % 40))); want = trimmed(bmul(want, ps.back())); }
        check(productTree(ps) == (k ? want : vector<M>{1}), name + " productTree k=" + str(k));
        auto rs = rnd<M>(int(rng() % 300));
        vector<M> w{1};
        for (auto r : rs) { w = bmul(w, vector<M>{-r, 1}); }
        check(polyFromRoots(rs) == w, name + " polyFromRoots");
        auto a = rnd<M>(int(rng() % 200)), b = rnd<M>(int(rng() % 200));
        int n = a.empty() || b.empty() ? 0 : int(a.size() + b.size()) - 1;
        vector<vector<M>> C(max(n, 1), vector<M>(max(n, 1)));
        for (int i = 0; i < n; ++i) { C[i][0] = 1; for (int j = 1; j <= i; ++j) { C[i][j] = C[i - 1][j - 1] + (j < i ? C[i - 1][j] : M(0)); } }
        vector<M> e(n);
        for (int i = 0; i < int(a.size()); ++i) { for (int j = 0; j < int(b.size()); ++j) { e[i + j] += C[i + j][i] * a[i] * b[j]; } }
        check(egfConvolution(a, b) == e, name + " egfConvolution");}
    {
        auto big = productTree(vector<vector<M>>(quick ? 500 : 20000, vector<M>{1, 1}));
        int n = int(big.size()) - 1;
        bool ok = big.back() == M(1);
        M c = 1;
        for (int i = 0; i <= n && ok; ++i) { ok = big[i] == c; c = c * M(n - i) / M(i + 1); }
        check(ok, name + " productTree binomial");}
    // Power sums by Newton's identities for monic a = x^d + c_{d-1} x^{d-1} + ... + c_0.
    for (int rep = 0; rep < (quick ? 30 : 200); ++rep) {
        int d = int(rng() % 40), n = int(rng() % 80);
        auto a = rndNz<M>(d + 1);
        if (rep % 3 == 0) { a.insert(a.begin(), int(rng() % 3), M(0)); d = int(a.size()) - 1; }
        auto m = monic(a);
        vector<M> p(n);
        for (int k = 0; k < n; ++k) {
            if (k == 0) { p[k] = M(d); continue; }
            M s = k <= d ? M(k) * m[d - k] : M(0);
            for (int i = 1; i < k && i <= d; ++i) { s += m[d - i] * p[k - i]; }
            p[k] = -s;}
        check(powerSumsOfRoots(a, n) == p, name + " powerSumsOfRoots d=" + str(d) + " n=" + str(n));
        if (ulng(d) < M::mod()) { check(polyFromPowerSums(powerSumsOfRoots(a, d + 1)) == m, name + " polyFromPowerSums round trip d=" + str(d)); }}
    {
        auto rs = rnd<M>(quick ? 300 : 3000);
        auto p = powerSumsOfRoots(polyFromRoots(rs), 50);
        bool ok = true;
        for (int k = 0; k < 50 && ok; ++k) {
            M s = 0;
            for (auto r : rs) { s += bpow(r, ulng(k)); }
            ok = p[k] == s;}
        check(ok && polyFromPowerSums(powerSumsOfRoots(polyFromRoots(rs), int(rs.size()) + 1)) == polyFromRoots(rs), name + " power sums from roots");}
    for (int d : {0, 1, 2, 5, 60, 200, 700}) {
        auto f = rnd<M>(d);
        auto g = prefixSumOfPolynomial(f);
        M s = 0;
        bool ok = int(g.size()) <= d + 1 && (d == 0 || int(g.size()) == d + 1 || f.back() == M(0));
        for (int x = 0; x <= d + 2 && ok; ++x) { ok = polyEval(g, M(x)) == s; s += horner(f, M(x)); }
        check(ok, name + " prefixSumOfPolynomial d=" + str(d));}
    cout << "PASS " << name << ": taylorShift, productTree, polyFromRoots, egfConvolution, powerSumsOfRoots, polyFromPowerSums, prefixSumOfPolynomial\n";}

// Phi_{pm} = Phi_m(x^p) when p | m, Phi_m(x^p) / Phi_m(x) otherwise, by exact integer division.
vector<lll> phiBrute(int n, map<int, vector<lll>> &memo) {
    if (memo.count(n)) { return memo[n]; }
    if (n == 1) { return memo[1] = {-1, 1}; }
    int p = n, m = n;
    for (int d = 2; d <= m / d; ++d) { while (m % d == 0) { p = d; m /= d; } }
    if (m > 1) { p = m; }
    m = n / p;
    auto f = phiBrute(m, memo);
    vector<lll> g((f.size() - 1) * p + 1);
    for (size_t i = 0; i < f.size(); ++i) { g[i * p] = f[i]; }
    if (m % p == 0) { return memo[n] = g; }
    int dq = int(g.size() - f.size());
    vector<lll> q(dq + 1);
    for (int i = dq; i >= 0; --i) {
        q[i] = g[i + f.size() - 1];
        for (size_t j = 0; j < f.size(); ++j) { g[i + j] -= q[i] * f[j]; }}
    for (auto x : g) { check(x == 0, "cyclotomic brute exact division"); }
    return memo[n] = q;}
void cyclotomicSuite() {
    map<int, vector<lll>> memo;
    using P = ModInt<1000000007>;
    using Q = ModInt<7>;
    int lim = quick ? 400 : 3000;
    for (int n = 1; n <= lim; ++n) {
        auto want = phiBrute(n, memo);
        auto got = cyclotomicPolynomial(n);
        bool ok = got.size() == want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) { ok = lll(got[i]) == want[i]; }
        check(ok, "cyclotomicPolynomial n=" + str(n));
        auto gp = cyclotomicPolynomialMod<P>(n);
        auto gq = cyclotomicPolynomialMod<Q>(n);
        for (size_t i = 0; ok && i < want.size(); ++i) { ok = gp[i] == P(lng(want[i] % 1000000007)) && gq[i] == Q(lng(want[i] % 7)); }
        check(ok, "cyclotomicPolynomialMod n=" + str(n));}
    vector<int> big{15015, 30030, 90090, 65536, 65537};
    if (stress) { big.pb(255255); }
    for (int n : big) {
        auto want = phiBrute(n, memo);
        auto got = cyclotomicPolynomial(n);
        bool ok = got.size() == want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) { ok = lll(got[i]) == want[i]; }
        check(ok, "cyclotomicPolynomial large n=" + str(n));}
    cout << "PASS cyclotomicPolynomial, cyclotomicPolynomialMod\n";}

void compositeSuite() {
    using D = DynModInt<6>;
    D::setMod(1000000000);
    // Ring operations over Z / 10^9 with unit leading or constant coefficients, on both sides of the cutoffs.
    for (int n : fast ? vector<int>{5, 200, 1700, 4000} : vector<int>{5, 200, 1700}) {
        auto a = rnd<D>(n), b = rnd<D>(n / 2 + 1);
        b.back() = D(3);
        a[0] = D(7);
        auto [qq, r] = polyDivmod(a, b);
        check(badd(bmul(qq, b), r) == trimmed(a) && int(r.size()) < int(b.size()), "composite polyDivmod n=" + str(n));
        check(isInverse(a, fpsInverse(a, n), n, false), "composite fpsInverse n=" + str(n));
        auto m = rnd<D>(min(n, 300));
        m.back() = 1;
        vector<D> want = bmod(vector<D>{1}, m), x = bmod(a, m);
        ulng e = rng();
        for (ulng t = e; t; t >>= 1, x = bmod(bmul(x, x), m)) { if (t & 1) { want = bmod(bmul(want, x), m); } }
        check(polyPowMod(a, e, m) == want, "composite polyPowMod n=" + str(n));
        auto f = rnd<D>(30), g = rnd<D>(n / 3);
        vector<D> h;
        for (int i = int(f.size()) - 1; i >= 0; --i) { h = bmod(badd(bmul(h, g), vector<D>{f[i]}), m); }
        check(polyComposeMod(f, g, m) == h, "composite polyComposeMod n=" + str(n));
        if (n <= 1700) {
            D c = D(rng());
            vector<D> w;
            for (int i = n - 1; i >= 0; --i) { w = badd(bmul(w, vector<D>{c, 1}), vector<D>{a[i]}); }
            w.resize(n);
            check(taylorShift(a, c) == w, "composite taylorShift n=" + str(n));}}
    map<int, vector<lll>> memo;
    for (int n : {1, 2, 12, 105, 210, 1155}) {
        auto want = phiBrute(n, memo);
        auto got = cyclotomicPolynomialMod<D>(n);
        bool ok = got.size() == want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) { ok = got[i] == D(lng(want[i] % 1000000000)); }
        check(ok, "composite cyclotomicPolynomialMod n=" + str(n));}
    cout << "PASS composite modulus 1e9: polyDivmod, fpsInverse, polyPowMod, polyComposeMod, taylorShift, cyclotomicPolynomialMod\n";}

void smallPrimeSuite() {
    using S = ModInt<7>;
    // Domain edges at n = mod: log/exp/pow need n <= p, taylorShift falls back to O(n^2) for n > p.
    auto a = vector<S>{1, 3, 2, 6, 5, 1, 4};
    check(isLog(a, fpsLog(a, 7), 7, false) && fpsSparseLog(a, 7) == fpsLog(a, 7), "p=7 fpsLog n=p");
    a[0] = 0;
    check(isExp(a, fpsExp(a, 7), 7, false), "p=7 fpsExp n=p");
    a[0] = 1;
    check(fpsPow(a, 1000000000000LL, 7) == bpowT(a, 1000000000000ULL, 7), "p=7 fpsPow n=p");
    check(fpsPow(vector<S>{0, 3, 1}, 3, 9) == bpowT(vector<S>{0, 3, 1}, 3, 9), "p=7 fpsPow shifted len=p");
    auto b = vector<S>{1, 2, 3, 4, 5, 6, 0, 1, 2, 3, 4};
    vector<S> want;
    for (int i = int(b.size()) - 1; i >= 0; --i) { want = badd(bmul(want, vector<S>{3, 1}), vector<S>{b[i]}); }
    want.resize(b.size());
    check(taylorShift(b, S(3)) == want, "p=7 taylorShift n>p");
    check(isInverse(b, fpsInverse(b, 40), 40, false), "p=7 fpsInverse n>p");
    check(polyIntegral(vector<S>{1, 2, 3, 4, 5, 6}) == vector<S>{0, 1, 1, 1, 1, 1, 1}, "p=7 polyIntegral |a| = p - 1");
    auto f = vector<S>{1, 1, 1, 1, 1, 1};
    auto g = prefixSumOfPolynomial(f);
    S s = 0;
    for (int x = 0; x < 7; ++x) { check(polyEval(g, S(x)) == s, "p=7 prefixSumOfPolynomial"); s += horner(f, S(x)); }
    {
        vector<vector<S>> C(7, vector<S>(7));
        vector<S> x{1, 2, 3}, y{4, 5, 6, 0, 1}, e(7);
        for (int i = 0; i < 7; ++i) { C[i][0] = 1; for (int j = 1; j <= i; ++j) { C[i][j] = C[i - 1][j - 1] + (j < i ? C[i - 1][j] : S(0)); } }
        for (int i = 0; i < 3; ++i) { for (int j = 0; j < 5; ++j) { e[i + j] += C[i + j][i] * x[i] * y[j]; } }
        check(egfConvolution(x, y) == e, "p=7 egfConvolution n=p");}
    check(fpsSqrt(vector<S>{2, 1}, 3).size() == 3 && fpsSqrt(vector<S>{3, 1}, 3).empty(), "p=7 fpsSqrt residue test");
    auto r = fpsSqrt(vector<S>{2, 1}, 7);
    check(bmulT(r, r, 7) == vector<S>{2, 1, 0, 0, 0, 0, 0} && r[0] == S(3), "p=7 fpsSqrt n=p branch 3 not 4");
    for (int n : {300, 1000}) {
        auto u = rnd<S>(n);
        u[0] = 2;
        auto w = fpsSqrt(u, n);
        check(bmulT(w, w, n) == u && w[0] == S(3), "p=7 fpsSqrt Newton path n=" + str(n) + " > p");}
    {
        using T = ModInt<113>;
        // Not an NTT prime, so cut(SHIFT) = 112: n = 113 = p takes the factorial path with (p - 1)!.
        for (int n : {112, 113, 114}) {
            auto a = rnd<T>(n);
            T c = T(rng());
            vector<T> w;
            for (int i = n - 1; i >= 0; --i) { w = badd(bmul(w, vector<T>{c, 1}), vector<T>{a[i]}); }
            w.resize(n);
            check(taylorShift(a, c) == w, "p=113 taylorShift n=" + str(n));}}
    cout << "PASS p=7 and p=113 domain edges\n";}

int main(int argc, char **argv) {
    using M = mint;
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        using S = ModInt<7>;
        if (s == "inverse-zero") { fpsInverse(vector<M>{0, 1}, 3); }
        if (s == "sparse-inverse-zero") { fpsSparseInverse(vector<M>{0, 1}, 3); }
        if (s == "log-constant") { fpsLog(vector<M>{2, 1}, 3); }
        if (s == "sparse-log-constant") { fpsSparseLog(vector<M>{2, 1}, 3); }
        if (s == "log-long") { fpsLog(vector<S>{1, 1}, 8); }
        if (s == "exp-constant") { fpsExp(vector<M>{1}, 3); }
        if (s == "sparse-exp-constant") { fpsSparseExp(vector<M>{1}, 3); }
        if (s == "pow-negative-valuation") { fpsPow(vector<M>{0, 1}, -1, 3); }
        if (s == "pow-long") { fpsPow(vector<S>{1, 1}, 3, 9); }
        if (s == "pow-rational-constant") { fpsPow(vector<M>{2, 1}, 1, 2, 3); }
        if (s == "pow-rational-q") { fpsPow(vector<M>{1, 1}, 1, 0, 3); }
        if (s == "sparse-pow-negative") { fpsSparsePow(vector<M>{1, 1}, -1, 3); }
        if (s == "sqrt-even-modulus") { fpsSqrt(vector<ModInt<2>>{1, 1}, 3); }
        if (s == "divmod-zero") { polyDivmod(vector<M>{1}, vector<M>{0}); }
        if (s == "halfgcd-degree") { polyHalfGcd(vector<M>{1, 1}, vector<M>{1, 1}); }
        if (s == "discriminant-constant") { polyDiscriminant(vector<M>{5}); }
        if (s == "inversemod-constant") { polyInverseMod(vector<M>{1, 1}, vector<M>{3}); }
        if (s == "powmod-zero") { polyPowMod(vector<M>{1, 1}, 3, vector<M>{}); }
        if (s == "composemod-zero") { polyComposeMod(vector<M>{1, 1}, vector<M>{1}, vector<M>{0}); }
        if (s == "compinv-linear") { fpsCompositionalInverse(vector<M>{0, 0, 1}, 3); }
        if (s == "composition-negative") { fpsComposition(vector<M>{1}, vector<M>{1}, -1); }
        if (s == "egf-long") { egfConvolution(vector<S>(5, 1), vector<S>(4, 1)); }
        if (s == "integral-long") { polyIntegral(vector<S>(7, 1)); }
        if (s == "cyclotomic-zero") { cyclotomicPolynomial(0); }
        if (s == "powersums-zero") { powerSumsOfRoots(vector<M>{0}, 3); }
        if (s == "frompowersums-empty") { polyFromPowerSums(vector<M>{}); }
        if (s == "truncate-negative") { polyTruncate(vector<M>{1}, -1); }
        if (s == "prefix-sum-long") { prefixSumOfPolynomial(vector<S>(7, 1)); }
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    quick = mode == "quick"; stress = mode == "stress";
#ifdef NDEBUG
    fast = true;
#endif
    int big = quick ? 3000 : fast ? (stress ? 1 << 20 : 1 << 18) : 1 << 14;
    using P = ModInt<1000000007>;
    using D = DynModInt<3>;
    using L = ModInt64<4179340454199820289ULL>;
    D::setMod(1000003);
    basicSuite<M>("998244353"); basicSuite<P>("1e9+7"); basicSuite<ModInt<7>>("7");
    divSuite<M>("998244353", true); divSuite<P>("1e9+7", true); divSuite<D>("dynamic 1000003", false); divSuite<L>("64-bit", false);
    fpsSuite<M>("998244353", big); fpsSuite<P>("1e9+7", big / 8); fpsSuite<D>("dynamic 1000003", big / 16); fpsSuite<L>("64-bit", big / 16);
    powSuite<M>("998244353", big); powSuite<P>("1e9+7", big / 8); powSuite<ModInt<97>>("97", 90);
    sqrtSuite<M>("998244353", big); sqrtSuite<P>("1e9+7", big / 8); sqrtSuite<D>("dynamic 1000003", big / 16);
    gcdSuite<M>("998244353", true); gcdSuite<P>("1e9+7", fast); gcdSuite<ModInt<5>>("5", false);
    modSuite<M>("998244353"); modSuite<P>("1e9+7"); modSuite<ModInt<3>>("3");
    compositionSuite<M>("998244353"); compositionSuite<P>("1e9+7");
    miscSuite<M>("998244353", big); miscSuite<P>("1e9+7", big / 8);
    cyclotomicSuite();
    compositeSuite();
    smallPrimeSuite();
    cout << "PASS all, " << checks << " checks\n";}
