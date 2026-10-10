// Independent oracles for 16-poly.hpp: schoolbook products, Horner, naive series recurrences, Euclid, Sylvester determinants, brute force over small fields.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wconversion"
#include "../../01-Core/07-infint.hpp"
#pragma GCC diagnostic pop
#include "../../01-Core/16-poly.hpp"
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <set>
#include <sstream>
#include <string>
#include <limits>
#include <vector>

using M = mint;
using Pd = std::complex<double>;
static ulng seed = 42;
static std::string mode = "full";
static std::string context;
static lng checks = 0;
static std::mt19937_64 rng(42);

ulng otherUnitConvolution(int n);
int otherUnitMaxLength();

[[noreturn]] static void fail(const std::string &message) {
    std::cerr << "FAIL seed=" << seed << " mode=" << mode << " context=" << context << '\n' << message << '\n';
    std::exit(1);}
template<typename T> static std::string show(const vector<T> &v) {
    std::ostringstream out;
    for (size_t i = 0; i < v.size() && i < 12; ++i) { out << (i ? " " : "") << v[i]; }
    if (v.size() > 12) { out << " ..."; }
    return out.str();}
template<typename T> static std::ostream &operator<<(std::ostream &os, const vector<T> &v) { return os << show(v); }
template<typename A, typename B> static std::ostream &operator<<(std::ostream &os, const pair<A, B> &p) { return os << "(" << p.first << ", " << p.second << ")"; }
static M toM(const InfInt &x) { M r = 0; for (char c : abs(x).toString()) { r = r * 10 + M(c - '0'); } return x.sgn < 0 ? -r : r; }
static void require(bool ok, const std::string &message) {
    ++checks;
    if (!ok) { fail(message); }}
template<class A, class B> static void expect(const A &actual, const B &expected, const std::string &operation) {
    ++checks;
    if (!(actual == expected)) {
        std::ostringstream out;
        out << operation << "\n expected=" << expected << "\n actual=" << actual;
        fail(out.str());}}
static int ri(int k) { return int(rng() % ulng(k)); }
static int pick(const vector<int> &v) { return v[static_cast<size_t>(ri(int(v.size())))]; }
static bool full() { return mode != "quick"; }
static bool stress() { return mode == "stress"; }

template<typename T> static Poly<T> nmul(const Poly<T> &a, const Poly<T> &b) {
    if (!a.size() || !b.size()) { return {}; }
    Poly<T> r(a.size() + b.size() - 1);
    for (int i = 0; i < a.size(); ++i) { for (int j = 0; j < b.size(); ++j) { r.v[i + j] += a.v[i] * b.v[j]; } }
    return r;}
template<typename T> static Poly<T> cut(Poly<T> a, int n) { a.resize(n); return a; }
template<typename T> static Poly<T> ninv(const Poly<T> &a, int n) {
    Poly<T> g(n); T c = T(1) / a.v[0];
    if (n) { g.v[0] = c; }
    for (int i = 1; i < n; ++i) { T s = 0; for (int j = 1; j <= i; ++j) { s += a.coef(j) * g.v[i - j]; } g.v[i] = -s * c; }
    return g;}
template<typename T> static Poly<T> nexp(const Poly<T> &a, int n) {
    Poly<T> g(n);
    if (n) { g.v[0] = 1; }
    for (int i = 1; i < n; ++i) { T s = 0; for (int j = 1; j <= i; ++j) { s += T(j) * a.coef(j) * g.v[i - j]; } g.v[i] = s / T(i); }
    return g;}
template<typename T> static Poly<T> nlog(const Poly<T> &a, int n) {
    if (n <= 1) { return Poly<T>(n); }
    Poly<T> q = nmul(deriv(a), ninv(a, n)); q.resize(n - 1);
    return integ(q).resize(n);}
template<typename T> static Poly<T> nsqrt(const Poly<T> &a, int n, T g0) {
    Poly<T> g(n);
    if (n) { g.v[0] = g0; }
    T i2 = T(1) / (T(2) * g0);
    for (int i = 1; i < n; ++i) { T s = a.coef(i); for (int j = 1; j < i; ++j) { s -= g.v[j] * g.v[i - j]; } g.v[i] = s * i2; }
    return g;}
template<typename T> static Poly<T> nshift(const Poly<T> &a, T c) {
    Poly<T> r, p = {T(1)};
    for (int i = 0; i < a.size(); ++i) { r += p * a.v[i]; p = nmul(p, Poly<T>{c, T(1)}); }
    return r.resize(a.size());}
template<typename T> static Poly<T> egcd(Poly<T> a, Poly<T> b) {
    a.trim(); b.trim();
    while (!b.isNil()) { Poly<T> r = a % b; a = b; b = r; }
    if (!a.isNil()) { a.normalize(); }
    return a;}
template<typename T> static vector<Poly<T>> remainderChain(Poly<T> a, Poly<T> b) {
    vector<Poly<T>> r; a.trim(); b.trim(); r.push_back(a); r.push_back(b);
    while (!b.isNil()) { Poly<T> t = a % b; a = b; b = t; r.push_back(b); }
    return r;}
template<typename T> static T sylvester(const Poly<T> &a, const Poly<T> &b) {
    int m = a.deg(), n = b.deg(), N = m + n;
    if (N == 0) { return T(1); }
    vector<vector<T>> S(static_cast<size_t>(N), vector<T>(static_cast<size_t>(N)));
    for (int i = 0; i < n; ++i) { for (int j = 0; j <= m; ++j) { S[static_cast<size_t>(i)][static_cast<size_t>(i + j)] = a.v[m - j]; } }
    for (int i = 0; i < m; ++i) { for (int j = 0; j <= n; ++j) { S[static_cast<size_t>(n + i)][static_cast<size_t>(i + j)] = b.v[n - j]; } }
    T det = 1;
    for (int c = 0; c < N; ++c) {
        int p = -1;
        for (int r = c; r < N; ++r) { if (S[static_cast<size_t>(r)][static_cast<size_t>(c)] != T(0)) { p = r; break; } }
        if (p < 0) { return T(0); }
        if (p != c) { swap(S[static_cast<size_t>(p)], S[static_cast<size_t>(c)]); det = -det; }
        det *= S[static_cast<size_t>(c)][static_cast<size_t>(c)];
        T inv = T(1) / S[static_cast<size_t>(c)][static_cast<size_t>(c)];
        for (int r = c + 1; r < N; ++r) { T f = S[static_cast<size_t>(r)][static_cast<size_t>(c)] * inv; for (int k = c; k < N; ++k) { S[static_cast<size_t>(r)][static_cast<size_t>(k)] -= f * S[static_cast<size_t>(c)][static_cast<size_t>(k)]; } }}
    return det;}
static Poly<M> rnd(int n) { Poly<M> a(n); for (M &x : a.v) { x = M(rng()); } return a; }
static Poly<M> rndz(int n, int zeroPercent) { Poly<M> a(n); for (M &x : a.v) { x = ri(100) < zeroPercent ? M(0) : M(rng()); } return a; }
static vector<M> distinctPoints(int n) {
    std::set<uint> seen; vector<M> xs;
    while (int(xs.size()) < n) { M x = M(rng()); if (seen.insert(x.val()).second) { xs.push_back(x); } }
    return xs;}

static void testBasics() {
    context = "basics";
    Poly<M> p = {1, 2, 3};
    expect(p.deg(), 2, "deg"); expect(p.lead(), M(3), "lead"); expect(p.coef(5), M(0), "coef beyond"); expect(p.coef(-1), M(0), "coef negative");
    expect(Poly<M>().deg(), -1, "deg zero"); expect(Poly<M>({0, 0}).deg(), -1, "deg trailing"); expect(Poly<M>({0, 0}).lead(), M(0), "lead zero");
    require(Poly<M>({0, 0}).isNil() && !p.isNil() && Poly<M>().isNil(), "isNil");
    expect(Poly<M>({1, 2, 0, 0}).trim().size(), 2, "trim"); expect(Poly<M>(p).resize(5).size(), 5, "resize"); expect(Poly<M>(p).truncate(2), Poly<M>({1, 2}), "truncate"); expect(Poly<M>(p).truncate(9), p, "truncate longer");
    expect(Poly<M>({2, 4, 6}).normalize(), Poly<M>({M(1) / 3, M(2) / 3, 1}), "normalize"); expect(Poly<M>(p).reverse(), Poly<M>({3, 2, 1}), "reverse"); expect(Poly<M>(p).reverse(5), Poly<M>({0, 0, 3, 2, 1}), "reverse n");
    expect(p + Poly<M>({1, 1, 1, 1}), Poly<M>({2, 3, 4, 1}), "+"); expect(p - Poly<M>({1, 1, 1, 1}), Poly<M>({0, 1, 2, -1}), "-"); expect(-p, Poly<M>({-1, -2, -3}), "unary -");
    expect(p * M(2), Poly<M>({2, 4, 6}), "* scalar"); expect(M(2) * p, Poly<M>({2, 4, 6}), "scalar *"); expect(p / M(2), Poly<M>({M(1) / 2, 1, M(3) / 2}), "/ scalar");
    expect(p << 2, Poly<M>({0, 0, 1, 2, 3}), "<<"); expect(p >> 1, Poly<M>({2, 3}), ">>"); expect(p >> 7, Poly<M>(), ">> beyond"); expect(p << 0, p, "<< 0");
    require(Poly<M>({1, 2, 3, 0}) == p && !(Poly<M>({1, 2}) == p) && Poly<M>() == Poly<M>({0}), "==");
    expect(eval(p, M(2)), M(17), "eval"); expect(eval(Poly<M>(), M(5)), M(0), "eval empty"); expect(deriv(p), Poly<M>({2, 6}), "deriv"); expect(deriv(Poly<M>()), Poly<M>(), "deriv empty");
    expect(integ(deriv(p), M(1)), p, "integ"); expect(integ(Poly<M>({1, 2, 3})), Poly<M>({0, 1, 1, 1}), "integ field"); expect(integ(Poly<lng>({1, 2, 3})), Poly<lng>({0, 1, 1, 1}), "integ ring");
    { std::ostringstream os; os << p; expect(os.str(), std::string("1 2 3"), "stream"); std::ostringstream o2; o2 << Poly<M>(); expect(o2.str(), std::string(""), "stream empty"); }
    Poly<M> q = p; q *= Poly<M>{1, 1}; expect(q, Poly<M>({1, 3, 5, 3}), "*="); q /= Poly<M>{1, 1}; expect(q, p, "/="); q = Poly<M>{1, 3, 5, 4}; q %= Poly<M>{1, 1}; expect(q, Poly<M>({-1}), "%=");
    Poly<M> s = p; s += Poly<M>{0, 0, 0, 1}; expect(s.size(), 4, "+= growth"); s -= Poly<M>{0, 0, 0, 1}; expect(s, p, "-=");
    expect(p[1], M(2), "operator[]"); Poly<M> w = p; w[0] = 9; expect(w.coef(0), M(9), "operator[] write");
    FPS<M> f = p; expect(f, p, "FPS alias");
    expect(Poly<M>(3, M(7)), Poly<M>({7, 7, 7}), "ctor n value");
    expect(poly_detail::ceilPow2(1), 1, "ceilPow2 1"); expect(poly_detail::ceilPow2(5), 8, "ceilPow2 5"); expect(poly_detail::ceilPow2(8), 8, "ceilPow2 8");
    expect(Poly<M>::nttMaxLength(), 1 << 23, "maxLength 998244353"); expect(Poly<ModInt<1000000007>>::nttMaxLength(), 2, "maxLength 1e9+7"); expect(otherUnitMaxLength(), 1 << 23, "maxLength other unit");
    require(otherUnitConvolution(100) == otherUnitConvolution(100), "other unit deterministic");
    { Poly<M> a(100), b(100); for (int i = 0; i < 100; ++i) { a.v[i] = M(i + 1); b.v[i] = M(2 * i + 3); } Poly<M> c = nmul(a, b); ulng h = 0; for (int i = 0; i < c.size(); ++i) { h = h * 1000003 + c.v[i].val(); } expect(otherUnitConvolution(100), h, "other unit convolution"); }
    context = "regressions";
    expect(pow(Poly<lng>{1, 1}, 2, 3), Poly<lng>({1, 2, 1}), "ring pow"); expect(pow(Poly<lng>{1, 1}, 5, 4), Poly<lng>({1, 5, 10, 10}), "ring pow truncated"); expect(pow(Poly<lng>{0, 0, 2}, 3, 9), Poly<lng>({0, 0, 0, 0, 0, 0, 8, 0, 0}), "ring pow valuation"); expect(pow(Poly<InfInt>{InfInt(1), InfInt(1)}, 3, 4), Poly<InfInt>({InfInt(1), InfInt(3), InfInt(3), InfInt(1)}), "InfInt pow");
    { Poly<M> a = {M(3), M(5), M(7)}; expect(inv(a, 0), Poly<M>(), "inv n=0"); Poly<M> z = {M(0), M(2)}; require(sinh(z, 0).isNil() && sinh(z, 0).size() == 0 && cosh(z, 0).size() == 0 && tan(z, 0).size() == 0 && tanh(z, 0).size() == 0 && divSeries(a, a, 0).size() == 0, "hyperbolic and tangent series with n = 0");
      Poly<M> out; require(trySqrt(Poly<M>{0, 1}, 0, out) && out.size() == 0, "trySqrt n=0"); expect(sqrtSparse(Poly<M>{0, 1}, 0), Poly<M>(), "sqrtSparse n=0"); expect(sqrt(Poly<M>{0, 1}, 0), Poly<M>(), "sqrt n=0");
      lng mn = std::numeric_limits<lng>::min(), mx = std::numeric_limits<lng>::max(); expect(cut(nmul(cut(nmul(pow(a, mn, 6), pow(a, mx, 6)), 6), a), 6), cut(Poly<M>{1}, 6), "pow LLONG_MIN"); Poly<M> m = {M(1), M(2), M(1), M(5)}; expect(mulMod(mulMod(powMod(a, mn, m), powMod(a, mx, m), m), a, m), Poly<M>{1} % m, "powMod LLONG_MIN");
      Poly<M> u = {M(1), M(4), M(9)}; expect(powUnit(u, M(3), 6), pow(u, 3, 6), "powUnit integer"); expect(powUnit(u, M(1) / M(2), 6), sqrt(u, 6), "powUnit half"); vector<lng> la = {5, -3, 7}, lb = {2, 9, -4, 1}; expect(PolyCompanion::convolutionLng(la, lb), nmul(Poly<lng>(la), Poly<lng>(lb)).v, "convolutionLng direct"); { lng big = lng(1) << 31; Poly<lng> ea = {big, -big, -big}, eb = {big, -big, big}, er = {big * big, -2 * big * big, big * big, 0, -big * big}; expect(ea * eb, er, "lng extreme dispatch"); expect(schoolbook(ea, eb), er, "lng extreme schoolbook"); expect(karatsuba(ea, eb), er, "lng extreme karatsuba"); Poly<lng> ec = {LLONG_MAX, LLONG_MIN}, ed = {1, 1}; expect(ec * ed, Poly<lng>({LLONG_MAX, -1, LLONG_MIN}), "lng extreme ends"); expect(schoolbook(ec, ed), Poly<lng>({LLONG_MAX, -1, LLONG_MIN}), "lng extreme ends schoolbook"); Poly<lng> ex(40), ey(40), ez(79); ex.v[0] = LLONG_MAX; ex.v[39] = LLONG_MIN; ey.v[0] = 1; ey.v[39] = 1; ez.v[0] = LLONG_MAX; ez.v[39] = -1; ez.v[78] = LLONG_MIN; expect(ex * ey, ez, "lng extreme ends three-prime"); fill(ex.v.begin(), ex.v.end(), big); ey = Poly<lng>(40); ey.v[0] = big; ey.v[1] = -big; ez = Poly<lng>(79); ez.v[0] = big * big; ez.v[40] = -big * big; expect(ex * ey, ez, "lng extreme three-prime"); expect(karatsuba(ex, ey), ez, "lng extreme karatsuba 40"); }
      { Poly<M> ka(3000), kb(45); for (M &x : ka.v) { x = M(rng()); } for (M &x : kb.v) { x = M(rng()); } expect(karatsuba(ka, kb), nmul(ka, kb), "karatsuba unbalanced"); expect(karatsuba(kb, ka), nmul(kb, ka), "karatsuba unbalanced swapped"); Poly<lng> kl(1001), km(7); for (lng &x : kl.v) { x = lng(rng() % 2001) - 1000; } for (lng &x : km.v) { x = lng(rng() % 2001) - 1000; } expect(karatsuba(kl, km), nmul(kl, km), "karatsuba unbalanced lng"); }
      { int ib = 1 << 15; Poly<int> ia = {ib, -ib, -ib}, ic = {ib, -ib, ib}; expect(schoolbook(ia, ic), Poly<int>({ib * ib, -2 * ib * ib, ib * ib, 0, -ib * ib}), "int extreme schoolbook"); expect(ia * ic, Poly<int>({ib * ib, -2 * ib * ib, ib * ib, 0, -ib * ib}), "int extreme dispatch"); }
      expect(powRational(Poly<M>{0, 0, 1, 5}, lng(1) << 62, 2, 5), Poly<M>(5), "powRational huge exponent zero"); { Poly<M> h = powRational(Poly<M>{0, 0, 1, 5}, 1, 2, 5); expect(cut(nmul(h, h), 5), Poly<M>({0, 0, 1, 5, 0}), "powRational half squares back"); } }
    { vector<M> one = {M(5)}; Poly<M>::ntt(one); Poly<M>::nttDoubling(one); vector<M> two = {M(5), M(0)}; Poly<M>::ntt(two); expect(one, two, "nttDoubling n=1"); }}

template<typename T, typename G> static void convolutionCase(const char *name, int n, int m, G gen, bool karatsubaToo) {
    Poly<T> a(n), b(m);
    for (T &x : a.v) { x = gen(); }
    for (T &x : b.v) { x = gen(); }
    Poly<T> ref = nmul(a, b);
    expect(a * b, ref, std::string(name) + " product"); expect(schoolbook(a, b), ref, std::string(name) + " schoolbook");
    if (karatsubaToo) { expect(karatsuba(a, b), ref, std::string(name) + " karatsuba"); }
    expect(a * a, nmul(a, a), std::string(name) + " square via *"); expect(square(a), nmul(a, a), std::string(name) + " square");
    Poly<T> empty; expect(a * empty, Poly<T>(), std::string(name) + " empty factor"); expect(empty * empty, Poly<T>(), std::string(name) + " both empty");}
static void testConvolution() {
    context = "convolution";
    vector<int> sizes = full() ? vector<int>{1, 2, 3, 7, 16, 31, 32, 33, 40, 41, 63, 64, 65, 100, 255, 256, 257, 1000, 1024, 2049} : vector<int>{1, 2, 3, 31, 32, 33, 64, 65, 200};
    if (stress()) { sizes.push_back(5000); sizes.push_back(8192); }
    for (int n : sizes) {
        for (int m : {1, 2, 33, int(n)}) {
            if (lng(n) * m > 30000000) { continue; }
            convolutionCase<M>("mint", n, m, [] { return M(rng()); }, true);
            convolutionCase<ModInt<1000000007>>("1e9+7", n, m, [] { return ModInt<1000000007>(rng()); }, n <= 1024);
            if (n * m <= 1000000) { convolutionCase<ModInt64<(1ULL << 63) + 123>>("mod64", n, m, [] { return ModInt64<(1ULL << 63) + 123>(rng()); }, false); }
            convolutionCase<ModInt64<4179340454199820289ULL>>("mod64 ntt", n, m, [] { return ModInt64<4179340454199820289ULL>(rng()); }, false);
            convolutionCase<ModInt<3221225473u>>("generic ntt 3*2^30+1", n, m, [] { return ModInt<3221225473u>(rng()); }, false);
            { using D = DynModInt<1>; D::setMod(1000000006); convolutionCase<D>("dyn composite", n, m, [] { return D(rng()); }, false); }
            { using D = DynModInt<1>; D::setMod(786433, 1); convolutionCase<D>("dyn 786433", n, m, [] { return D(rng()); }, false); }
            convolutionCase<lng>("lng", n, m, [] { return lng(rng() % 2000001) - 1000000; }, n <= 1024);
            convolutionCase<int>("int", n, m, [] { return int(rng() % 2001) - 1000; }, false);
            { Poly<ulng> a(n), b(m); for (ulng &x : a.v) { x = rng(); } for (ulng &x : b.v) { x = rng(); } Poly<ulng> c = a * b; bool ok = c.size() == n + m - 1; for (int k = 0; ok && k < n + m - 1; k += 1 + (n + m) / 23) { ulng s = 0; for (int i = max(0, k - m + 1); i <= min(k, n - 1); ++i) { s += a.v[i] * b.v[k - i]; } ok = s == c.v[k]; } require(ok, "ulng wraparound product n=" + std::to_string(n)); expect(PolyCompanion::convolution2p64(a.v, b.v), c.v, "convolution2p64 equals Poly<ulng>"); }
            { vector<ulng> a(static_cast<size_t>(n)), b(static_cast<size_t>(m)); for (ulng &x : a) { x = rng(); } for (ulng &x : b) { x = rng(); } for (ulng md : {ulng(1), ulng(2), ulng(998244353), (ulng(1) << 32) - 1, ulng(1) << 32, (ulng(1) << 63) + 123, ~ulng(0)}) { vector<ulng> e = PolyCompanion::convolutionArbitraryMod(a, b, md); bool ok = e.size() == static_cast<size_t>(n + m - 1); for (int k = 0; ok && k < n + m - 1; k += 1 + (n + m) / 23) { ulll s = 0; for (int i = max(0, k - m + 1); i <= min(k, n - 1); ++i) { s = (s + ulll(a[static_cast<size_t>(i)] % md) * (b[static_cast<size_t>(k - i)] % md)) % md; } ok = ulng(s) == e[static_cast<size_t>(k)]; } require(ok, "arbitrary mod n=" + std::to_string(n) + " mod=" + std::to_string(md)); } }
            if (n <= 1024) { Poly<double> a(n), b(m); for (double &x : a.v) { x = double(ri(2001)) - 1000; } for (double &x : b.v) { x = double(ri(2001)) - 1000; } Poly<double> c = a * b, d = nmul(a, b); double err = 0; for (int i = 0; i < c.size(); ++i) { err = max(err, std::abs(c.v[i] - d.v[i])); } require(c.size() == d.size() && err < 1e-6 * n, "double product err=" + std::to_string(err)); expect(convolutionFft(a, b).size(), d.size(), "convolutionFft size");
                Poly<Pd> x(n), y(m); for (Pd &t : x.v) { t = Pd(double(ri(2001)) - 1000, double(ri(2001)) - 1000); } for (Pd &t : y.v) { t = Pd(double(ri(2001)) - 1000, double(ri(2001)) - 1000); } Poly<Pd> z = x * y, w = nmul(x, y); err = 0; for (int i = 0; i < z.size(); ++i) { err = max(err, std::abs(z.v[i] - w.v[i])); } require(z.size() == w.size() && err < 1e-6 * n, "complex product err=" + std::to_string(err));}
            { vector<uint> a(static_cast<size_t>(n)), b(static_cast<size_t>(m)); uint md = 1000000007; for (uint &x : a) { x = uint(rng() % md); } for (uint &x : b) { x = uint(rng() % md); } vector<ulng> ua(a.begin(), a.end()), ub(b.begin(), b.end()); vector<ulng> d = PolyCompanion::convolutionArbitraryMod(ua, ub, md); vector<uint> c = PolyCompanion::convolutionFftMod(a, b, md); expect(vector<ulng>(c.begin(), c.end()), d, "convolutionFftMod"); }
            if (n <= 100) { Poly<InfInt> a(n), b(m); for (InfInt &x : a.v) { x = InfInt(lng(rng() % 2000001) - 1000000); x *= InfInt(lng(rng())); if (rng() & 1) { x *= InfInt(lng(rng())); } } for (InfInt &x : b.v) { x = InfInt(lng(rng() % 200001) - 100000); if (rng() & 1) { x *= InfInt(lng(rng())); } } expect(a * b, nmul(a, b), "InfInt product"); }
            // cyclic family
            { Poly<M> a = rnd(n), b = rnd(m); int N = poly_detail::ceilPow2(max(n, m)), L = max(n, m) + 1; Poly<M> fu = nmul(a, b), cy(N), ne(N); for (int i = 0; i < fu.size(); ++i) { cy.v[i % N] += fu.v[i]; if ((i / N) & 1) { ne.v[i % N] -= fu.v[i]; } else { ne.v[i % N] += fu.v[i]; } } expect(cyclic(a, b, N), cy, "cyclic"); expect(negacyclic(a, b, N), ne, "negacyclic"); Poly<M> cy2(L); for (int i = 0; i < fu.size(); ++i) { cy2.v[i % L] += fu.v[i]; } expect(cyclic(a, b, L), cy2, "cyclic odd length"); Poly<M> ne2(L); for (int i = 0; i < fu.size(); ++i) { if ((i / L) & 1) { ne2.v[i % L] -= fu.v[i]; } else { ne2.v[i % L] += fu.v[i]; } } expect(negacyclic(a, b, L), ne2, "negacyclic odd length");
              expect(truncatedMul(a, b, n / 2 + 1), cut(fu, n / 2 + 1), "truncatedMul"); expect(truncatedMul(a, b, 0), Poly<M>(), "truncatedMul 0"); expect(mulHigh(a, b, n / 2), fu >> (n / 2), "mulHigh");
              if (m <= n) { Poly<M> mp = middleProduct(a, b); expect(mp, Poly<M>(vector<M>(fu.v.begin() + m - 1, fu.v.begin() + n)), "middleProduct"); }
              vector<pair<int, M>> sp = {{0, M(3)}, {2, M(5)}, {7, M(11)}}; Poly<M> spd(8); spd.v[0] = 3; spd.v[2] = 5; spd.v[7] = 11; expect(sparseMul(a, sp), nmul(a, spd), "sparseMul"); expect(sparseMul(a, {}), Poly<M>(), "sparseMul empty");
              Poly<lng> al(n); for (lng &x : al.v) { x = lng(ri(21)) - 10; } Poly<lng> bl(m); for (lng &x : bl.v) { x = lng(ri(21)) - 10; } Poly<lng> fl = nmul(al, bl), cl(N), nl(N); for (int i = 0; i < fl.size(); ++i) { cl.v[i % N] += fl.v[i]; if ((i / N) & 1) { nl.v[i % N] -= fl.v[i]; } else { nl.v[i % N] += fl.v[i]; } } expect(cyclic(al, bl, N), cl, "cyclic ring"); expect(negacyclic(al, bl, N), nl, "negacyclic ring"); }}}
    // convolutionLarge against the direct product (same modulus) and a modulus with a small transform limit
    for (int n : {1, 5, 100, 3000}) { for (int m : {1, 7, 2500}) { Poly<M> a = rnd(n), b = rnd(m); expect(convolutionLarge(a, b), nmul(a, b), "convolutionLarge"); expect(convolutionLarge(a, a), nmul(a, a), "convolutionLarge square"); } }
    { using D = DynModInt<2>; D::setMod(786433, 1); int n = full() ? 300000 : 70000, m = full() ? 250000 : 70000; Poly<D> a(n), b(m); for (D &x : a.v) { x = D(rng()); } for (D &x : b.v) { x = D(rng()); } Poly<D> c = convolutionLarge(a, b), d = convolutionArbitraryMod(a, b); expect(c == d, true, "convolutionLarge vs arbitrary mod beyond the 2^18 limit"); expect(a * b == d, true, "dispatch beyond the transform limit"); expect(Poly<D>::nttMaxLength(), 1 << 18, "maxLength 786433"); }
    if (full()) { for (uint md : {1073741789u, 998244353u, (1u << 30) - 1}) { int n = 1 << 19; vector<uint> a(static_cast<size_t>(n), md - 1), b(static_cast<size_t>(n + 1), md - 1); for (int i = 0; i < n; i += 7) { a[static_cast<size_t>(i)] = uint(rng() % md); } vector<ulng> ua(a.begin(), a.end()), ub(b.begin(), b.end()); vector<ulng> d = PolyCompanion::convolutionArbitraryMod(ua, ub, md); vector<uint> c = PolyCompanion::convolutionFftMod(a, b, md); expect(vector<ulng>(c.begin(), c.end()), d, "convolutionFftMod at the 2^20 bound mod=" + std::to_string(md)); } }
    // GF(2^k)
    for (int k : {1, 3, 8, 31, 32, 63, 64}) {
        ulng red = k == 1 ? 1 : k == 3 ? 3 : k == 8 ? 0x1b : k == 31 ? 9 : k == 32 ? 0x8d : k == 63 ? 3 : 0x1b, mask = k == 64 ? ~ulng(0) : (ulng(1) << k) - 1;
        auto gmul = [&](ulng a, ulng b) { ulng r = 0; for (int i = 0; i < k; ++i) { if (b >> i & 1) { r ^= a; } bool hi = k == 64 ? (a >> 63 & 1) : (a >> (k - 1) & 1); a = (a << 1) & mask; if (hi) { a ^= red; } } return r; };
        for (int n : {1, 2, 10, 24, 25, 48, 49, 100, 700, 1200, -9}) { int m = n == 1200 ? 50 : n / 2 + 1; if (n < 0) { n = 11; m = 900; } vector<ulng> a(static_cast<size_t>(n)), b(static_cast<size_t>(m)); for (ulng &x : a) { x = rng() & mask; } for (ulng &x : b) { x = rng() & mask; } vector<ulng> c = PolyCompanion::convolutionGF2k(a, b, k, red); bool ok = c.size() == static_cast<size_t>(n + m - 1); for (int t = 0; ok && t < n + m - 1; ++t) { ulng s = 0; for (int i = max(0, t - m + 1); i <= min(t, n - 1); ++i) { s ^= gmul(a[static_cast<size_t>(i)], b[static_cast<size_t>(t - i)]); } ok = s == c[static_cast<size_t>(t)]; } require(ok, "convolutionGF2k k=" + std::to_string(k) + " n=" + std::to_string(n)); }
        expect(PolyCompanion::convolutionGF2k({}, {1}, k, red).size(), static_cast<size_t>(0), "convolutionGF2k empty");}
    // 2-D and multivariate
    for (int it = 0; it < (full() ? 40 : 10); ++it) {
        vector<int> shape(static_cast<size_t>(1 + ri(3))); int N = 1; for (int &s : shape) { s = 1 + ri(5); N *= s; }
        vector<M> a(static_cast<size_t>(N)), b(static_cast<size_t>(N)), c(static_cast<size_t>(N)); for (M &x : a) { x = M(rng()); } for (M &x : b) { x = M(rng()); }
        auto idx = [&](int i) { vector<int> r; for (int s : shape) { r.push_back(i % s); i /= s; } return r; };
        for (int i = 0; i < N; ++i) { for (int j = 0; j < N; ++j) { vector<int> u = idx(i), v = idx(j); bool ok = true; int t = 0, mul = 1; for (size_t k = 0; k < shape.size(); ++k) { int s = u[k] + v[k]; if (s >= shape[k]) { ok = false; } t += s * mul; mul *= shape[k]; } if (ok) { c[static_cast<size_t>(t)] += a[static_cast<size_t>(i)] * b[static_cast<size_t>(j)]; } } }
        expect(Poly<M>::multivariate(a, b, shape), c, "multivariate");
        if (it < 20) { vector<int> sh2(static_cast<size_t>(1 + ri(3))); int N2 = 1; for (int &s : sh2) { s = pick({1, 2, 4, 7, 8, 14, 16, 17, 34}); N2 *= s; } vector<M> x(static_cast<size_t>(N2)), y(static_cast<size_t>(N2)), z(static_cast<size_t>(N2)); for (M &t : x) { t = M(rng()); } for (M &t : y) { t = M(rng()); } auto idx2 = [&](int i) { vector<int> r; for (int s : sh2) { r.push_back(i % s); i /= s; } return r; }; for (int i = 0; i < N2; ++i) { for (int j = 0; j < N2; ++j) { vector<int> u = idx2(i), v = idx2(j); int t = 0, mul = 1; for (size_t k = 0; k < sh2.size(); ++k) { t += ((u[k] + v[k]) % sh2[k]) * mul; mul *= sh2[k]; } z[static_cast<size_t>(t)] += x[static_cast<size_t>(i)] * y[static_cast<size_t>(j)]; } } expect(Poly<M>::multivariateCyclic(x, y, sh2), z, "multivariateCyclic"); }
        int r1 = 1 + ri(4), c1 = 1 + ri(4), r2 = 1 + ri(4), c2 = 1 + ri(4); vector<M> p(static_cast<size_t>(r1 * c1)), q(static_cast<size_t>(r2 * c2)), s(static_cast<size_t>((r1 + r2 - 1) * (c1 + c2 - 1))); for (M &x : p) { x = M(rng()); } for (M &x : q) { x = M(rng()); } for (int i = 0; i < r1; ++i) { for (int j = 0; j < c1; ++j) { for (int k = 0; k < r2; ++k) { for (int l = 0; l < c2; ++l) { s[static_cast<size_t>((i + k) * (c1 + c2 - 1) + j + l)] += p[static_cast<size_t>(i * c1 + j)] * q[static_cast<size_t>(k * c2 + l)]; } } } } expect(Poly<M>::convolution2d(p, r1, c1, q, r2, c2), s, "convolution2d");}
    // large multivariate through the NTT path
    { vector<int> shape = {40, 50}; int N = 2000; vector<M> a(static_cast<size_t>(N)), b(static_cast<size_t>(N)), c(static_cast<size_t>(N)); for (M &x : a) { x = M(rng()); } for (M &x : b) { x = M(rng()); } for (int i = 0; i < N; ++i) { for (int j = 0; j < N; ++j) { int x1 = i % 40 + j % 40, y1 = i / 40 + j / 40; if (x1 < 40 && y1 < 50) { c[static_cast<size_t>(y1 * 40 + x1)] += a[static_cast<size_t>(i)] * b[static_cast<size_t>(j)]; } } } expect(Poly<M>::multivariate(a, b, shape), c, "multivariate large"); }}

static void testDft() {
    context = "dft";
    for (int n : {1, 2, 4, 8, 64, 1024}) { vector<M> a(static_cast<size_t>(n)); for (M &x : a) { x = M(rng()); } vector<M> A = a; Poly<M>::ntt(A); Poly<M>::nttDoubling(A); vector<M> B = a; B.resize(static_cast<size_t>(2 * n)); Poly<M>::ntt(B); expect(A, B, "nttDoubling n=" + std::to_string(n)); }
    for (int n : {1, 2, 4, 32, 256, 4096}) { vector<M> a(static_cast<size_t>(n)); for (M &x : a) { x = M(rng()); } vector<M> A = a; Poly<M>::ntt(A); vector<M> B = A; Poly<M>::intt(B); expect(B, a, "ntt/intt roundtrip"); M w = pow(M(3), (M::mod() - 1) / uint(n)); vector<M> nat(static_cast<size_t>(n)); for (int k = 0; k < n; ++k) { nat[static_cast<size_t>(k)] = eval(Poly<M>(a), pow(w, k)); } vector<M> C = a; Poly<M>::transposedNtt(C); vector<M> bitrev(static_cast<size_t>(n)); int lg = std::countr_zero(uint(n)); for (int i = 0; i < n; ++i) { int r = 0; for (int b = 0; b < lg; ++b) { if (i >> b & 1) { r |= 1 << (lg - 1 - b); } } bitrev[static_cast<size_t>(i)] = A[static_cast<size_t>(r)]; } expect(bitrev, nat, "ntt is the bit-reversed natural DFT"); vector<M> D = C; Poly<M>::transposedIntt(D); expect(D, a, "transposedNtt/transposedIntt roundtrip");
        // transposed map: <Poly<M>::transposedNtt(x), y> == <x, Poly<M>::ntt(y)>
        vector<M> y(static_cast<size_t>(n)); for (M &t : y) { t = M(rng()); } vector<M> Y = y; Poly<M>::ntt(Y); M l = 0, r = 0; for (int i = 0; i < n; ++i) { l += C[static_cast<size_t>(i)] * y[static_cast<size_t>(i)]; r += a[static_cast<size_t>(i)] * Y[static_cast<size_t>(i)]; } expect(l, r, "transposedNtt is the transpose of ntt");
        Poly<M> pa(a); ntt(pa); expect(pa.v, A, "ntt Poly overload"); intt(pa); expect(pa.v, a, "intt Poly overload");
        vector<Pd> z(static_cast<size_t>(n)); for (Pd &t : z) { t = Pd(double(ri(201)) - 100, double(ri(201)) - 100); } vector<Pd> Z = z; PolyCompanion::fft(Z); vector<Pd> back = Z; PolyCompanion::ifft(back); double err = 0; for (int i = 0; i < n; ++i) { err = max(err, std::abs(back[static_cast<size_t>(i)] - z[static_cast<size_t>(i)])); } require(err < 1e-7 * n, "fft/ifft roundtrip err=" + std::to_string(err)); if (n <= 256) { Pd w2 = std::polar(1.0, 2 * std::numbers::pi / n); for (int i = 0; i < n; ++i) { int r2 = 0; for (int b = 0; b < lg; ++b) { if (i >> b & 1) { r2 |= 1 << (lg - 1 - b); } } Pd s = 0; for (int j = 0; j < n; ++j) { s += z[static_cast<size_t>(j)] * std::pow(w2, double(r2) * j); } err = max(err, std::abs(s - Z[static_cast<size_t>(i)])); } require(err < 1e-6 * n, "fft bit-reversed DFT err=" + std::to_string(err));}}
    vector<int> lengths = {1, 2, 3, 4, 6, 7, 12, 31, 32, 37, 97, 128, 210, 1009, 2048};
    if (full()) { lengths.push_back(4096); lengths.push_back(9973); }
    for (int n : lengths) { if ((M::mod() - 1) % uint(n)) { continue; } M w = pow(M(3), (M::mod() - 1) / uint(n)); vector<M> a(static_cast<size_t>(n)); for (M &x : a) { x = M(rng()); } vector<M> X = Poly<M>::mixedRadix(a, w), Y(static_cast<size_t>(n)); for (int k = 0; k < n; ++k) { M s = 0, p = 1, wk = pow(w, k); for (int j = 0; j < n; ++j, p *= wk) { s += a[static_cast<size_t>(j)] * p; } Y[static_cast<size_t>(k)] = s; } expect(X, Y, "mixedRadix n=" + std::to_string(n)); expect(Poly<M>::mixedRadix(X, w, true), a, "mixedRadix inverse"); expect(Poly<M>::bluestein(a, w, n), Y, "bluestein"); expect(Poly<M>::bluestein(a, w, n / 2), vector<M>(Y.begin(), Y.begin() + n / 2), "bluestein prefix"); if (n > 2 && n % 2) { expect(Poly<M>::rader(a, w), Y, "rader"); expect(Poly<M>::rader(X, w, true), a, "rader inverse"); } }
    { using D = DynModInt<4>; vector<int> rl = {12, 37, 74, 97, 210, 1009, 1155, 3127}; if (full()) { rl.push_back(9973); }
      for (int n : rl) { uint p = 0; for (ulng k = 1;; ++k) { ulng c = k * ulng(n) + 1; bool prime = c > 1; for (ulng d = 2; d * d <= c; ++d) { if (c % d == 0) { prime = false; break; } } if (prime) { p = uint(c); break; } } D::setMod(p, 1); D w = pow(D(poly_detail::primitiveRootMod(p)), (p - 1) / uint(n)); vector<D> a(static_cast<size_t>(n)); for (D &x : a) { x = D(rng()); } vector<D> X = Poly<D>::mixedRadix(a, w), Y(static_cast<size_t>(n)); for (int k = 0; k < n; ++k) { D s = 0, pw = 1, wk = pow(w, k); for (int j = 0; j < n; ++j, pw *= wk) { s += a[static_cast<size_t>(j)] * pw; } Y[static_cast<size_t>(k)] = s; } expect(X, Y, "mixedRadix p=" + std::to_string(p) + " n=" + std::to_string(n)); expect(Poly<D>::mixedRadix(X, w, true), a, "mixedRadix inverse n=" + std::to_string(n)); expect(Poly<D>::bluestein(a, w, n), Y, "bluestein n=" + std::to_string(n)); bool prime = true; for (int d = 2; d * d <= n; ++d) { if (n % d == 0) { prime = false; } } if (prime) { expect(Poly<D>::rader(a, w), Y, "rader n=" + std::to_string(n)); expect(Poly<D>::rader(X, w, true), a, "rader inverse n=" + std::to_string(n)); } } }
    { using G = ModInt<3221225473u>; using W = ModInt64<4179340454199820289ULL>; for (int n : {1, 2, 8, 64, 1024}) { vector<G> a(static_cast<size_t>(n)); for (G &x : a) { x = G(rng()); } vector<G> A = a; Poly<G>::ntt(A); vector<G> B = A; Poly<G>::intt(B); expect(B, a, "generic ntt roundtrip"); Poly<G>::nttDoubling(A); vector<G> C = a; C.resize(static_cast<size_t>(2 * n)); Poly<G>::ntt(C); expect(A, C, "generic nttDoubling"); vector<W> x(static_cast<size_t>(n)); for (W &t : x) { t = W(rng()); } vector<W> X = x; Poly<W>::ntt(X); vector<W> Z = X; Poly<W>::intt(Z); expect(Z, x, "montgomery64 ntt roundtrip"); Poly<W>::nttDoubling(X); vector<W> Y = x; Y.resize(static_cast<size_t>(2 * n)); Poly<W>::ntt(Y); expect(X, Y, "montgomery64 nttDoubling"); } expect(Poly<G>::nttMaxLength(), 1 << 30, "maxLength 3*2^30+1"); expect(Poly<W>::nttMaxLength(), 1 << 30, "maxLength goldilocks-like 64-bit"); }
    { uint p = poly_detail::primitiveRootMod(998244353); expect(p, 3u, "primitive root 998244353"); expect(poly_detail::primitiveRootMod(2), 1u, "primitive root 2"); expect(poly_detail::primitiveRootMod(7), 3u, "primitive root 7"); expect(poly_detail::primitiveRootMod(1000000007), 5u, "primitive root 1e9+7"); }}

static void testSeries() {
    context = "series";
    vector<int> sizes = full() ? vector<int>{1, 2, 3, 7, 16, 17, 31, 32, 33, 63, 64, 65, 200, 513, 1100} : vector<int>{1, 2, 3, 17, 33, 64, 65, 200};
    if (stress()) { sizes.push_back(3000); }
    for (int n : sizes) {
        Poly<M> a = rnd(n); a.v[0] = M(ri(1000) + 1);
        expect(inv(a, n), ninv(a, n), "inv"); expect(inv(a, 2 * n + 1), ninv(a, 2 * n + 1), "inv longer"); expect(inv(a, 0), Poly<M>(), "inv 0"); expect(invSparse(a, n), ninv(a, n), "invSparse");
        Poly<M> u = a; u.v[0] = 1; expect(log(u, n), nlog(u, n), "log"); expect(logSparse(u, n), nlog(u, n), "logSparse"); expect(log(u, 0), Poly<M>(), "log 0"); expect(log(u, 1), Poly<M>(1), "log 1");
        Poly<M> z = a; z.v[0] = 0; expect(exp(z, n), nexp(z, n), "exp"); expect(exp(z, 2 * n + 3), nexp(z, 2 * n + 3), "exp longer"); expect(expSparse(z, n), nexp(z, n), "expSparse"); expect(exp(Poly<M>(), n), cut(Poly<M>{1}, n), "exp empty"); expect(exp(z, 0), Poly<M>(), "exp 0");
        Poly<M> sq = a; sq.v[0] = M(49); { Poly<M> s = sqrt(sq, n); require(s.size() == n && (s == nsqrt(sq, n, M(7)) || s == nsqrt(sq, n, M(-7))), "sqrt"); expect(cut(nmul(s, s), n), cut(sq, n), "sqrt squares back"); expect(sqrtSparse(sq, n), s, "sqrtSparse"); Poly<M> out; require(trySqrt(sq, n, out) && out == s, "trySqrt"); expect(s.coef(0), min(M(7), M(-7), [](M x, M y) { return x.val() < y.val(); }), "sqrt branch is the smaller residue");
            Poly<M> nr = sq; nr.v[0] = M(998244353 - 1); M dummy; bool residue = trySqrt(M(-1), dummy); require(trySqrt(nr, n, out) == residue, "trySqrt nonresidue"); expect(sqrt(Poly<M>(), n), Poly<M>(n), "sqrt zero"); expect(sqrt(Poly<M>{0, 1}, n), Poly<M>(), "sqrt odd valuation"); expect(sqrtSparse(Poly<M>{0, 1}, n), Poly<M>(), "sqrtSparse odd valuation"); Poly<M> sh = sq << 4; expect(sqrt(sh, n + 2), cut(s << 2, n + 2), "sqrt valuation"); expect(sqrtSparse(sh, n + 2), cut(s << 2, n + 2), "sqrtSparse valuation");}
        for (lng k : {0, 1, 2, 3, 5}) { Poly<M> pw(n); pw.v[0] = 1; for (int t = 0; t < k; ++t) { pw = cut(nmul(pw, a), n); } expect(pow(a, k, n), pw, "pow k=" + std::to_string(k)); expect(powSparse(a, k, n), pw, "powSparse k=" + std::to_string(k)); }
        expect(pow(a, 1000000000000000000LL, n), powSparse(a, 1000000000000000000LL, n), "pow huge exponent");
        { Poly<M> sh = a << 3; expect(pow(sh, 2, n + 10), cut(nmul(sh, sh), n + 10), "pow with valuation"); expect(pow(sh, 5, 14), Poly<M>(14), "pow valuation exceeds"); expect(pow(a, -2, n), ninv(cut(nmul(a, a), n), n), "pow negative"); expect(pow(Poly<M>(), 3, n), Poly<M>(n), "pow zero series"); expect(pow(Poly<M>(), 0, n), cut(Poly<M>{1}, n), "pow zero exponent"); expect(powSparse(sh, 2, n + 10), cut(nmul(sh, sh), n + 10), "powSparse valuation"); expect(pow(a, 1, n), cut(a, n), "pow one"); }
        if (n >= 2) { Poly<M> r3 = kthRoot(u, 3, n); expect(cut(nmul(r3, nmul(r3, r3)), n), cut(u, n), "kthRoot"); expect(powRational(u, 2, 3, n), cut(nmul(r3, r3), n), "powRational"); expect(powRational(u << 3, 2, 3, n), cut(powRational(u, 2, 3, n) << 2, n), "powRational valuation"); expect(powRational(Poly<M>(), 1, 2, n), Poly<M>(n), "powRational zero"); expect(powRational(u << 3, 2, 3, 1), Poly<M>(1), "powRational shift beyond"); }
        { auto [c, s] = circular(z, n); Poly<M> da = deriv(z); require(c.size() == n && s.size() == n, "circular size"); expect(deriv(s), cut(nmul(c, da), n - 1), "sin'"); expect(deriv(c), cut(-nmul(s, da), n - 1), "cos'"); expect(c.coef(0), M(1), "cos(0)"); expect(s.coef(0), M(0), "sin(0)"); expect(sin(z, n), s, "sin"); expect(cos(z, n), c, "cos");
          M i = sqrt(M(-1)); Poly<M> E = exp(z * i, n), iE = inv(E, n); expect(c, (E + iE) * (M(1) / M(2)), "cos via exp"); expect(s, (E - iE) * (M(1) / (M(2) * i)), "sin via exp");
          expect(tan(z, n), cut(nmul(s, ninv(c, n)), n), "tan"); expect(asin(s, n), cut(z, n), "asin"); expect(atan(tan(z, n), n), cut(z, n), "atan");
          Poly<M> sh = sinh(z, n), ch = cosh(z, n); expect(cut(nmul(ch, ch), n) - cut(nmul(sh, sh), n), cut(Poly<M>{1}, n), "cosh^2 - sinh^2"); expect(tanh(z, n), cut(nmul(sh, ninv(ch, n)), n), "tanh"); expect(asinh(sh, n), cut(z, n), "asinh"); expect(atanh(tanh(z, n), n), cut(z, n), "atanh"); expect(sinh(z, n), (exp(z, n) - exp(-z, n)) * (M(1) / 2), "sinh via exp"); }
        { Poly<M> b = rnd(n + 5); expect(divSeries(b, a, n), cut(nmul(b, ninv(a, n)), n), "divSeries"); expect(fromLogDerivative(b, n), exp(integ(cut(b, n - 1)), n), "fromLogDerivative"); Poly<M> fl = fromLogDerivative(b, n); if (n >= 2) { expect(cut(deriv(fl), n - 1), cut(nmul(b, fl), n - 1), "fromLogDerivative identity"); } expect(egfToOgf(ogfToEgf(b)), b, "ogf/egf roundtrip"); Poly<M> e = ogfToEgf(b); bool ok = true; M f = 1; for (int i = 0; i < b.size(); ++i) { ok &= e.v[i] * f == b.v[i]; f *= M(i + 1); } require(ok, "ogfToEgf divides by factorials");
          expect(linearOde(deriv(z), Poly<M>(), M(1), n), exp(z, n), "linearOde reproduces exp"); Poly<M> sol = linearOde(a, b, M(5), n); require(sol.size() == n && sol.coef(0) == M(5), "linearOde initial"); if (n >= 2) { expect(cut(deriv(sol), n - 1), cut(nmul(a, sol), n - 1) + cut(b, n - 1), "linearOde equation"); } }
        if (n >= 2) { Poly<M> r = rnd(n / 2 + 1); if (rng() & 1) { r.v[0] = 0; } Poly<M> s = nmul(r, r), out; require(trySqrtExact(s, out) && nmul(out, out) == s, "trySqrtExact"); Poly<M> bad = s; bad.v[static_cast<size_t>(ri(bad.size()))] += 1; Poly<M> o2; if (trySqrtExact(bad, o2)) { expect(nmul(o2, o2), bad, "trySqrtExact positive is a square"); } require(trySqrtExact(Poly<M>{}, out) && out.isNil(), "trySqrtExact zero"); require(!trySqrtExact(Poly<M>{0, 1}, out), "trySqrtExact x"); }}
    // generic field (non-NTT modulus) and double
    { using D = ModInt<1000000007>; int n = full() ? 300 : 100; Poly<D> a(n); for (D &x : a.v) { x = D(rng()); } a.v[0] = 1; expect(inv(a, n), ninv(a, n), "inv 1e9+7"); expect(log(a, n), nlog(a, n), "log 1e9+7"); Poly<D> z = a; z.v[0] = 0; expect(exp(z, n), nexp(z, n), "exp 1e9+7"); Poly<D> sq = a; sq.v[0] = 4; require(sqrt(sq, n) == nsqrt(sq, n, D(2)) || sqrt(sq, n) == nsqrt(sq, n, -D(2)), "sqrt 1e9+7"); auto [c, s] = circular(z, n); expect(deriv(s), cut(nmul(c, deriv(z)), n - 1), "sin 1e9+7"); expect(pow(a, 7, n), powSparse(a, 7, n), "pow 1e9+7"); }
    { Poly<double> a(60); for (double &x : a.v) { x = (double(ri(2001)) - 1000) / 1000; } a.v[0] = 1; Poly<double> g = inv(a, 60), h = ninv(a, 60); double err = 0; for (int i = 0; i < 60; ++i) { err = max(err, std::abs(g.v[i] - h.v[i]) / (1 + std::abs(h.v[i]))); } require(err < 1e-9, "inv double err=" + std::to_string(err)); Poly<double> z = a; z.v[0] = 0; z = z * 0.1; Poly<double> e = exp(z, 60), ne = nexp(z, 60); err = 0; for (int i = 0; i < 60; ++i) { err = max(err, std::abs(e.v[i] - ne.v[i]) / (1 + std::abs(ne.v[i]))); } require(err < 1e-9, "exp double err=" + std::to_string(err)); Poly<double> b = a * 0.1; b.v[0] = 1; Poly<double> s = sqrt(b * b, 60); err = 0; for (int i = 0; i < 60; ++i) { err = max(err, std::abs(s.v[i] - b.v[i])); } require(err < 1e-9, "sqrt double err=" + std::to_string(err)); }}

static void testDivision() {
    context = "division";
    vector<int> sizes = full() ? vector<int>{1, 2, 3, 7, 16, 31, 32, 33, 64, 65, 200, 513, 1100} : vector<int>{1, 2, 3, 33, 64, 200};
    for (int n : sizes) {
        Poly<M> a = rnd(n); a.v[0] = M(ri(1000) + 1);
        Poly<M> b = rnd(max(1, n / 3)); b.v.back() = M(ri(100) + 1); Poly<M> aa = rnd(n + 5);
        auto [q, r] = divMod(aa, b); require(nmul(q, b) + r == aa && r.deg() < b.deg(), "divMod identity"); expect(aa / b, q, "operator/"); expect(aa % b, r, "operator%"); auto [q0, r0] = divMod(b, aa); require(q0.isNil() && r0 == b, "divMod smaller dividend"); auto [q1, r1] = divMod(Poly<M>(), b); require(q1.isNil() && r1.isNil(), "divMod zero dividend");
        Poly<M> mb = b; mb.normalize(); expect(monicDiv(aa, mb), divMod(aa, mb).first, "monicDiv"); Poly<M> qq; require(tryDivide(nmul(aa, b), b, qq) && qq == aa, "tryDivide exact"); require(b.deg() == 0 || !tryDivide(nmul(aa, b) + Poly<M>{1}, b, qq), "tryDivide inexact"); require(!tryDivide(aa, Poly<M>(), qq), "tryDivide zero divisor");
        M c = M(rng()); auto [dq, dr] = divRoot(aa, c); expect(nmul(dq, Poly<M>{-c, 1}) + Poly<M>{dr}, aa, "divRoot"); expect(dr, eval(aa, c), "divRoot remainder"); expect(divRoot(Poly<M>{5}, c).second, M(5), "divRoot constant"); expect(divRoot(Poly<M>(), c).second, M(0), "divRoot zero");
        expect(divSeries(aa, a, n), cut(nmul(aa, ninv(a, n)), n), "divSeries");
        if (n <= 12) { Poly<lng> x(n), y(max(1, n / 2)); for (lng &t : x.v) { t = lng(ri(21)) - 10; } for (lng &t : y.v) { t = lng(ri(21)) - 10; } y.v.back() = 3; auto [pq, pr] = pseudoDiv(x, y); lng l = 1; for (int t = 0; t < max(0, x.deg() - y.deg() + 1); ++t) { l *= 3; } Poly<lng> lhs = x * l; require((x.deg() < y.deg() && pq.isNil() && pr == x) || (lhs == nmul(pq, y) + pr && pr.deg() < y.deg()), "pseudoDiv"); expect(pseudoRem(x, y), pr, "pseudoRem");
            Poly<lng> my = y; my.v.back() = 1; Poly<lng> prod = nmul(x, my); expect(monicDiv(prod, my), x, "monicDiv ring"); expect(divMod(prod, my).first, x, "divMod ring"); Poly<lng> rq; require(tryDivide(prod, my, rq) && rq == x, "tryDivide ring"); require(!tryDivide(prod + Poly<lng>{1}, my, rq) || my.deg() == 0, "tryDivide ring inexact"); require(!tryDivide(Poly<lng>{3}, Poly<lng>{2}, rq), "tryDivide ring inexact constant");}
        { Poly<InfInt> x(5), y = {InfInt(1), InfInt(2), InfInt(1)}; for (InfInt &t : x.v) { t = InfInt(lng(rng() % 2001) - 1000); } Poly<InfInt> prod = nmul(x, y); expect(divMod(prod, y).first, x, "divMod InfInt"); expect(divMod(prod, y).second, Poly<InfInt>(), "divMod InfInt remainder"); }}}

static void testEvalInterp() {
    context = "evaluation/interpolation";
    vector<int> sizes = full() ? vector<int>{0, 1, 2, 3, 5, 31, 32, 33, 64, 100, 257, 1000} : vector<int>{0, 1, 2, 5, 33, 64, 257};
    if (stress()) { sizes.push_back(3000); }
    for (int n : sizes) {
        Poly<M> a = rnd(n); vector<M> xs(static_cast<size_t>(n + 3)); for (M &x : xs) { x = M(rng()); }
        vector<M> ev = evalMulti(a, xs), ref(xs.size()); for (size_t i = 0; i < xs.size(); ++i) { ref[i] = eval(a, xs[i]); } expect(ev, ref, "evalMulti"); expect(evalMulti(a, {}), vector<M>(), "evalMulti no points"); expect(evalMulti(Poly<M>(), xs), vector<M>(xs.size()), "evalMulti zero polynomial");
        { vector<M> few(xs.begin(), xs.begin() + min<size_t>(xs.size(), 2)); vector<M> e2 = evalMulti(a, few), r2(few.size()); for (size_t i = 0; i < few.size(); ++i) { r2[i] = eval(a, few[i]); } expect(e2, r2, "evalMulti few points"); }
        if (n) { vector<M> px = distinctPoints(n), py(static_cast<size_t>(n)); for (M &y : py) { y = M(rng()); } Poly<M> f = Poly<M>::interpolate(px, py); require(f.size() == n, "interpolate size"); expect(evalMulti(f, px), py, "interpolate values"); vector<M> dup = px; if (n >= 2) { dup[1] = dup[0]; }
            vector<M> b(static_cast<size_t>(n)); for (M &x : b) { x = M(rng()); } vector<M> ti = Poly<M>::transposedInterpolate(b, px), ref2(static_cast<size_t>(n)); if (n <= 64) { for (int i = 0; i < n; ++i) { vector<M> unit(static_cast<size_t>(n)); unit[static_cast<size_t>(i)] = 1; Poly<M> L = Poly<M>::interpolate(px, unit); M s = 0; for (int j = 0; j < n; ++j) { s += b[static_cast<size_t>(j)] * L.coef(j); } ref2[static_cast<size_t>(i)] = s; } expect(ti, ref2, "transposedInterpolate"); }
            { vector<M> y2(static_cast<size_t>(n)); for (M &t : y2) { t = M(rng()); } M l = 0, r = 0; vector<M> g = Poly<M>::interpolate(px, y2).v; g.resize(static_cast<size_t>(n)); for (int i = 0; i < n; ++i) { l += ti[static_cast<size_t>(i)] * y2[static_cast<size_t>(i)]; r += b[static_cast<size_t>(i)] * g[static_cast<size_t>(i)]; } expect(l, r, "transposedInterpolate is the transpose"); }
            int d = n + 2; vector<M> te = Poly<M>::transposedEvalMulti(b, px, d), ref3(static_cast<size_t>(d)); for (int j = 0; j < d; ++j) { M s = 0; for (int i = 0; i < n; ++i) { s += b[static_cast<size_t>(i)] * pow(px[static_cast<size_t>(i)], j); } ref3[static_cast<size_t>(j)] = s; } expect(te, ref3, "transposedEvalMulti"); expect(Poly<M>::transposedEvalMulti(b, px, 0), vector<M>(), "transposedEvalMulti d=0");}
        M c = M(rng()), r = M(rng()); int m = n + 2; vector<M> g = evalMultiGeometric(a, c, r, m), gr(static_cast<size_t>(m)); { M x = c; for (int i = 0; i < m; ++i, x *= r) { gr[static_cast<size_t>(i)] = eval(a, x); } } expect(g, gr, "evalMultiGeometric"); expect(evalMultiGeometric(a, c, r, 0), vector<M>(), "evalMultiGeometric m=0");
        { vector<M> z = evalMultiGeometric(a, c, M(0), m); require(z[0] == eval(a, c) && z[1] == a.coef(0), "evalMultiGeometric r=0"); vector<M> z2 = evalMultiGeometric(a, M(0), r, m); require(z2[0] == a.coef(0) && z2[1] == a.coef(0), "evalMultiGeometric c=0"); }
        { Poly<M> pg = Poly<M>::productOfGeometric(c, r, n), pr = {1}; M x = c; for (int i = 0; i < n; ++i, x *= r) { pr = nmul(pr, Poly<M>{-x, 1}); } expect(pg, pr, "productOfGeometric"); Poly<M> q1 = {1}; for (int i = 0; i < n; ++i) { q1 = nmul(q1, Poly<M>{-c, 1}); } expect(Poly<M>::productOfGeometric(c, M(1), n), q1, "productOfGeometric r=1"); M rt = pow(M(3), (M::mod() - 1) / 8); Poly<M> q2 = {1}; x = c; for (int i = 0; i < n; ++i, x *= rt) { q2 = nmul(q2, Poly<M>{-x, 1}); } expect(Poly<M>::productOfGeometric(c, rt, n), q2, "productOfGeometric root of unity"); }
        if (n) { vector<M> ys(static_cast<size_t>(n)); for (M &y : ys) { y = M(rng()); } Poly<M> f = Poly<M>::interpolateGeometric(c, r, ys); require(f.size() == n, "interpolateGeometric size"); expect(evalMultiGeometric(f, c, r, n), ys, "interpolateGeometric values"); }
        { Poly<M> sh = taylorShift(a, c); expect(sh, nshift(a, c), "taylorShift"); expect(taylorShift(a, M(0)), a, "taylorShift 0"); Poly<lng> al(n); for (lng &x : al.v) { x = lng(ri(7)) - 3; } lng cl = lng(ri(5)) - 2; if (n <= 40) { expect(taylorShift(al, cl), nshift(al, cl), "taylorShift ring"); } Poly<InfInt> ai(n); for (InfInt &x : ai.v) { x = InfInt(lng(ri(7)) - 3); } if (n <= 40) { expect(taylorShift(ai, InfInt(2)), nshift(ai, InfInt(2)), "taylorShift InfInt"); } }
        { vector<M> xs2(static_cast<size_t>(max(n - 1, 0))); for (M &x : xs2) { x = M(rng()); } vector<M> nc = monomialToNewton(a, xs2); expect(int(nc.size()), n, "monomialToNewton size"); expect(Poly<M>::newtonToMonomial(nc, xs2), a, "newton roundtrip"); Poly<M> acc, basis = {1}; for (int k = 0; k < n; ++k) { acc += basis * nc[static_cast<size_t>(k)]; if (k < n - 1) { basis = nmul(basis, Poly<M>{-xs2[static_cast<size_t>(k)], 1}); } } expect(acc.resize(n), a, "monomialToNewton semantics"); vector<M> fc = monomialToFactorial(a); expect(Poly<M>::factorialToMonomial(fc), a, "factorial roundtrip"); Poly<M> acc2, b2 = {1}; for (int k = 0; k < n; ++k) { acc2 += b2 * fc[static_cast<size_t>(k)]; b2 = nmul(b2, Poly<M>{M(-k), 1}); } expect(acc2.resize(n), a, "monomialToFactorial semantics"); }
        if (n) { vector<M> ys(static_cast<size_t>(n)); for (M &y : ys) { y = M(rng()); } Poly<M> f = Poly<M>::interpolateIota(ys); require(f.size() == n, "interpolateIota size"); expect(evalMulti(f, Poly<M>::iotaPoints(n)), ys, "interpolateIota values"); for (int m2 : {0, 1, n, n + 5}) { vector<M> sh = Poly<M>::shiftSamplingPoints(ys, c, m2), ref4(static_cast<size_t>(m2)); M x = c; for (int i = 0; i < m2; ++i, x += 1) { ref4[static_cast<size_t>(i)] = eval(f, x); } expect(sh, ref4, "shiftSamplingPoints m=" + std::to_string(m2)); } expect(Poly<M>::shiftSamplingPoints(ys, M(1), n - 1), vector<M>(ys.begin() + 1, ys.end()), "shiftSamplingPoints by one"); }
        if (n >= 1 && n <= 300) { int k = 1 + ri(4); vector<M> hx; vector<vector<M>> hc; std::set<uint> seen; int tot = 0; for (int i = 0; i < k; ++i) { M x = M(rng()); if (!seen.insert(x.val()).second) { continue; } int mi = 1 + ri(4); vector<M> ci(static_cast<size_t>(mi)); for (M &t : ci) { t = M(rng()); } hx.push_back(x); hc.push_back(ci); tot += mi; } Poly<M> f = Poly<M>::hermiteInterpolate(hx, hc); require(f.size() <= tot, "hermiteInterpolate degree bound"); bool ok = true; for (size_t i = 0; i < hx.size(); ++i) { Poly<M> t = taylorShift(f, hx[i]); for (size_t j = 0; j < hc[i].size(); ++j) { ok &= t.coef(int(j)) == hc[i][j]; } } require(ok, "hermiteInterpolate values"); if (hx.size() == 1 && hc[0].size() == 1) { expect(f, Poly<M>{hc[0][0]}, "hermiteInterpolate single"); } }}
    if (full()) { vector<M> hx = distinctPoints(300); vector<vector<M>> hc(300); vector<pair<M, int>> rts; int tot = 0; for (int i = 0; i < 300; ++i) { int mi = 1 + ri(5); hc[static_cast<size_t>(i)].resize(static_cast<size_t>(mi)); for (M &t : hc[static_cast<size_t>(i)]) { t = M(rng()); } rts.emplace_back(hx[static_cast<size_t>(i)], mi); tot += mi; } Poly<M> f = Poly<M>::hermiteInterpolate(hx, hc); require(f.size() <= tot, "large hermite degree"); bool ok = true; for (int i = 0; i < 300; ++i) { Poly<M> t = taylorShift(f, hx[static_cast<size_t>(i)]); for (size_t j = 0; j < hc[static_cast<size_t>(i)].size(); ++j) { ok &= t.coef(int(j)) == hc[static_cast<size_t>(i)][j]; } } require(ok, "large hermite values");
      Poly<M> P = rnd(tot + 20); auto [u, c] = partialFractions(P, rts); vector<Poly<M>> lin(300); for (int i = 0; i < 300; ++i) { lin[static_cast<size_t>(i)] = poly_detail::polyPow(Poly<M>{-rts[static_cast<size_t>(i)].first, 1}, rts[static_cast<size_t>(i)].second); } Poly<M> Q = productOfSequence(lin), acc = u * Q; for (int i = 0; i < 300; ++i) { Poly<M> base = Q; for (int t = 0; t < rts[static_cast<size_t>(i)].second; ++t) { base = divRoot(base, rts[static_cast<size_t>(i)].first).first; } for (int j = rts[static_cast<size_t>(i)].second; j >= 1; --j) { acc += base * c[static_cast<size_t>(i)][static_cast<size_t>(j - 1)]; base = base * Poly<M>{-rts[static_cast<size_t>(i)].first, 1}; } } expect(acc, P, "large partialFractions");}
    { vector<Poly<M>> fs; Poly<M> ref = {1}; for (int i = 0; i < 50; ++i) { Poly<M> f = rnd(1 + ri(20)); fs.push_back(f); ref = nmul(ref, f); } expect(productOfSequence(fs), ref, "productOfSequence"); expect(productOfSequence(vector<Poly<M>>{}), Poly<M>{1}, "productOfSequence empty"); }
    { Poly<double> a(12); for (double &x : a.v) { x = (double(ri(2001)) - 1000) / 1000; } vector<double> xs(15); for (double &x : xs) { x = (double(ri(2001)) - 1000) / 1000; } vector<double> e = evalMulti(a, xs); double err = 0; for (int i = 0; i < 15; ++i) { err = max(err, std::abs(e[static_cast<size_t>(i)] - eval(a, xs[static_cast<size_t>(i)]))); } require(err < 1e-9, "evalMulti double err=" + std::to_string(err)); vector<double> px(12), py(12); for (int i = 0; i < 12; ++i) { px[static_cast<size_t>(i)] = i * 0.5 - 3; py[static_cast<size_t>(i)] = eval(a, px[static_cast<size_t>(i)]); } Poly<double> f = Poly<double>::interpolate(px, py); err = 0; for (int i = 0; i < 12; ++i) { err = max(err, std::abs(f.v[i] - a.v[i])); } require(err < 1e-6, "interpolate double err=" + std::to_string(err)); }}

static void testAlgebra() {
    context = "algebra";
    int iterations = full() ? 300 : 80;
    for (int it = 0; it < iterations; ++it) {
        int cap = it < iterations * 3 / 4 ? 40 : (stress() ? 1500 : 700), da = 1 + ri(cap), db = 1 + ri(cap), z = ri(4) * 30;
        Poly<M> g = rndz(1 + ri(5), z); if (g.isNil()) { g = {1}; }
        Poly<M> a = nmul(g, rndz(da, z)), b = nmul(g, rndz(db, z));
        if (a.isNil() && b.isNil()) { continue; }
        Poly<M> E = egcd(a, b); expect(gcd(a, b), E, "gcd"); expect(gcd(b, a), E, "gcd symmetric"); expect(gcd(a, Poly<M>()), a.isNil() ? Poly<M>() : Poly<M>(a).normalize(), "gcd with zero");
        Poly<M> x, y; expect(exGcd(a, b, x, y), E, "exGcd value"); expect(nmul(x, a) + nmul(y, b), E, "exGcd identity"); if (!a.isNil() && !b.isNil() && E.deg() < b.deg() && E.deg() < a.deg()) { require(x.deg() < b.deg() - E.deg() && y.deg() < a.deg() - E.deg(), "exGcd cofactor degrees"); }
        if (a.deg() > b.deg() && b.deg() >= 0) { std::array<Poly<M>, 4> H = halfGcd(a, b); Poly<M> ap = (H[0] * a + H[1] * b).trim(), bp = (H[2] * a + H[3] * b).trim(); int n = a.deg(); require(ap.deg() >= (n + 1) / 2 && bp.deg() < (n + 1) / 2, "halfGcd degrees"); vector<Poly<M>> R = remainderChain(a, b); bool found = false; for (size_t i = 0; i + 1 < R.size(); ++i) { found |= R[i] == ap && R[i + 1] == bp; } require(found, "halfGcd reaches a remainder pair"); }
        if (b.deg() >= 1) { Poly<M> inv; bool ok = tryInvMod(a, b, inv); Poly<M> gg = egcd(a, b); expect(ok, gg.deg() == 0, "tryInvMod status"); if (ok) { expect(nmul(inv, a) % b, Poly<M>{1}, "tryInvMod product"); require(inv.deg() < b.deg(), "tryInvMod degree"); expect(invMod(a, b), inv, "invMod"); } }
        if (a.deg() >= 0 && b.deg() >= 0 && a.deg() <= 30 && b.deg() <= 30) { expect(resultant(a, b), sylvester(Poly<M>(a).trim(), Poly<M>(b).trim()), "resultant vs Sylvester"); if (a.deg() >= 1) { Poly<M> ta = Poly<M>(a).trim(), dd = deriv(ta); dd.trim(); M ref = dd.isNil() ? M(0) : sylvester(ta, dd) * pow(ta.lead(), ta.deg() - 1 - dd.deg()) / ta.lead(); if ((ta.deg() / 2) & 1) { ref = -ref; } expect(discriminant(a), ref, "discriminant vs Sylvester"); } }
        else if (a.deg() >= 1 && b.deg() >= 1) { Poly<M> p = Poly<M>(a).trim(), q = Poly<M>(b).trim(); M ref = 1; if (p.deg() < q.deg()) { swap(p, q); if (p.deg() & q.deg() & 1) { ref = -ref; } } for (;;) { Poly<M> r = p % q; if (r.isNil()) { ref = q.deg() == 0 ? ref * pow(q.v[0], p.deg()) : M(0); break; } ref *= pow(q.lead(), p.deg() - r.deg()) * ((p.deg() & q.deg() & 1) ? M(-1) : M(1)); p = q; q = r; } expect(resultant(a, b), ref, "resultant vs Euclid chain"); }
        expect(lcm(a, b), a.isNil() || b.isNil() ? Poly<M>() : Poly<M>(nmul(a, b) / E).normalize(), "lcm");
        if (a.deg() >= 0 && b.deg() >= 0) { vector<Poly<M>> seq; auto [res, last] = subresultants(a, b, &seq); require(!seq.empty() && seq.back() == last, "subresultants sequence ends with the last remainder"); require(Poly<M>(last).normalize() == E, "subresultants last is the gcd"); expect(res.coef(0), resultant(a, b), "subresultants resultant"); }
        if (it < 20) { expect(resultant(Poly<M>{3}, Poly<M>{1, 2, 5}), M(9), "resultant constant left"); expect(resultant(Poly<M>{1, 2, 5}, Poly<M>{3}), M(9), "resultant constant right"); expect(resultant(Poly<M>{3}, Poly<M>{4}), M(1), "resultant constants"); expect(resultant(Poly<M>{1, 2}, Poly<M>()), M(0), "resultant zero"); expect(resultant(Poly<M>{-1, 1}, Poly<M>{-2, 1}), M(-1), "resultant linear"); expect(resultant(Poly<M>{-2, 1}, Poly<M>{-1, 1}), M(1), "resultant linear swapped"); expect(discriminant(Poly<M>{1, 0, 1}), M(-4), "discriminant x^2+1"); expect(discriminant(Poly<M>{1, 1}), M(1), "discriminant linear"); expect(discriminant(Poly<M>{0, 0, 1}), M(0), "discriminant x^2"); }}
    for (int n : full() ? vector<int>{300, 1100, 2500, 4200} : vector<int>{300, 1100}) { for (int it = 0; it < 2; ++it) { Poly<M> g = rndz(1 + ri(40), 30); if (g.isNil()) { g = {1}; } Poly<M> a = nmul(g, rndz(n, it * 60)), b = nmul(g, rndz(n - 1 - ri(n / 2), it * 60)); if (b.isNil()) { b = {1}; } Poly<M> E = egcd(a, b); expect(gcd(a, b), E, "deep gcd n=" + std::to_string(n)); Poly<M> x, y; expect(exGcd(a, b, x, y), E, "deep exGcd n=" + std::to_string(n)); expect(nmul(x, a) + nmul(y, b), E, "deep exGcd identity n=" + std::to_string(n)); if (a.deg() > b.deg()) { std::array<Poly<M>, 4> H = halfGcd(a, b); Poly<M> ap = (H[0] * a + H[1] * b).trim(), bp = (H[2] * a + H[3] * b).trim(); require(ap.deg() >= (a.deg() + 1) / 2 && bp.deg() < (a.deg() + 1) / 2, "deep halfGcd degrees n=" + std::to_string(n)); vector<Poly<M>> R = remainderChain(a, b); bool found = false; for (size_t i = 0; i + 1 < R.size(); ++i) { found |= R[i] == ap && R[i + 1] == bp; } require(found, "deep halfGcd chain n=" + std::to_string(n)); } Poly<M> p = Poly<M>(a).trim(), q = Poly<M>(b).trim(); M ref = 1; if (p.deg() < q.deg()) { swap(p, q); if (p.deg() & q.deg() & 1) { ref = -ref; } } for (;;) { Poly<M> r = p % q; if (r.isNil()) { ref = q.deg() == 0 ? ref * pow(q.v[0], p.deg()) : M(0); break; } ref *= pow(q.lead(), p.deg() - r.deg()) * ((p.deg() & q.deg() & 1) ? M(-1) : M(1)); p = q; q = r; } expect(resultant(a, b), ref, "deep resultant n=" + std::to_string(n)); } }
    // rings: lng with small coefficients and InfInt
    for (int it = 0; it < (full() ? 150 : 40); ++it) {
        auto rl = [&](int n) { Poly<lng> a(n); for (lng &x : a.v) { x = lng(ri(7)) - 3; } return a; };
        Poly<lng> g = rl(1 + ri(3)); if (g.isNil()) { g = {1}; } Poly<lng> a = nmul(g, rl(1 + ri(5))), b = nmul(g, rl(1 + ri(5))); if (a.isNil() || b.isNil()) { continue; }
        bool small = a.deg() <= 3 && b.deg() <= 3;
        Poly<InfInt> ai(a.size()), bi(b.size()), gi0(g.size()); for (int i = 0; i < a.size(); ++i) { ai.v[i] = InfInt(a.v[i]); } for (int i = 0; i < b.size(); ++i) { bi.v[i] = InfInt(b.v[i]); } for (int i = 0; i < g.size(); ++i) { gi0.v[i] = InfInt(g.v[i]); }
        Poly<InfInt> G = gcd(ai, bi), q; require(tryDivide(G, primitivePart(gi0), q), "ring gcd divisible by the common factor"); require(G.lead() > InfInt(0) && content(G) == InfInt(1), "ring gcd primitive with positive lead"); Poly<InfInt> q1, q2; require(tryDivide(ai, G, q1) && tryDivide(bi, G, q2), "ring gcd divides both");
        Poly<M> am(vector<M>(a.v.begin(), a.v.end())), bm(vector<M>(b.v.begin(), b.v.end())), gm(G.size()); for (int i = 0; i < G.size(); ++i) { gm.v[i] = toM(G.v[i]); } expect(egcd(am, bm), Poly<M>(gm).normalize(), "ring gcd matches the field gcd"); expect(gcd(ai, Poly<InfInt>()), primitivePart(ai), "ring gcd with zero"); expect(gcd(Poly<lng>(), Poly<lng>()), Poly<lng>(), "ring gcd zeros");
        if (small) { Poly<lng> Gl = gcd(a, b); expect(Poly<InfInt>(vector<InfInt>(Gl.v.begin(), Gl.v.end())), G, "lng gcd small degrees"); expect(lcm(a, b), primitivePart(nmul(a, b) / Gl), "ring lcm"); }
        if (am.deg() == a.deg() && bm.deg() == b.deg()) { InfInt r = resultant(ai, bi); expect(toM(r), resultant(am, bm), "InfInt resultant matches the field"); if (small) { expect(InfInt(resultant(a, b)), r, "lng resultant small degrees"); } if (a.deg() >= 1) { InfInt d = discriminant(ai); expect(toM(d), discriminant(am), "InfInt discriminant"); if (a.deg() <= 3) { expect(InfInt(discriminant(a)), d, "lng discriminant small degrees"); } } }
        Poly<lng> c = a * 6; expect(content(c), 6 * content(a), "content scaled"); expect(primitivePart(c), primitivePart(a), "primitivePart scaled"); expect(content(-a), -content(a), "content sign"); expect(primitivePart(-a), primitivePart(a), "primitivePart sign"); expect(content(Poly<lng>()), lng(0), "content zero"); expect(primitivePart(Poly<lng>()), Poly<lng>(), "primitivePart zero"); expect(content(am), am.lead(), "content field"); expect(primitivePart(am), Poly<M>(am).normalize(), "primitivePart field");
        expect(lcm(ai, bi), primitivePart(nmul(ai, bi) / G), "InfInt lcm");}
    // cyclotomic
    for (int n = 1; n <= (full() ? 60 : 24); ++n) { Poly<lng> prod = {1}; for (int d = 1; d <= n; ++d) { if (n % d == 0) { prod = nmul(prod, Poly<lng>::cyclotomic(d)); } } Poly<lng> xn(n + 1); xn.v[0] = -1; xn.v[n] = 1; expect(prod, xn, "cyclotomic product n=" + std::to_string(n)); Poly<M> cm = Poly<M>::cyclotomic(n); Poly<lng> cl = Poly<lng>::cyclotomic(n); expect(cm, Poly<M>(vector<M>(cl.v.begin(), cl.v.end())), "cyclotomic mint"); require(cl.lead() == 1, "cyclotomic monic"); }
    expect(Poly<lng>::cyclotomic(105).coef(7), lng(-2), "cyclotomic 105 coefficient"); expect(Poly<lng>::cyclotomic(1), Poly<lng>({-1, 1}), "cyclotomic 1"); expect(Poly<lng>::cyclotomic(2), Poly<lng>({1, 1}), "cyclotomic 2"); expect(Poly<lng>::cyclotomic(6), Poly<lng>({1, -1, 1}), "cyclotomic 6");
    // modular arithmetic
    for (int it = 0; it < (full() ? 150 : 40); ++it) {
        int d = 1 + ri(it < 100 ? 20 : 300); Poly<M> m = rnd(d + 1); m.v[d] = M(ri(1000) + 1); Poly<M> a = rnd(ri(2 * d + 3)), b = rnd(ri(2 * d + 3));
        expect(mulMod(a, b, m), nmul(a, b) % m, "mulMod"); PolyModulus<M> pm(m); expect(pm.reduce(a), a % m, "PolyModulus reduce"); expect(pm.reduce(Poly<M>()), Poly<M>(), "PolyModulus reduce empty"); { Poly<M> big = rnd(5 * d + 3); expect(pm.reduce(big), big % m, "PolyModulus reduce long"); }
        lng e = ri(50); Poly<M> pw = {1}; for (int i = 0; i < e; ++i) { pw = nmul(pw, a) % m; } expect(powMod(a, e, m), pw, "powMod"); expect(powMod(a, 0, m), Poly<M>{1} % m, "powMod 0");
        if (egcd(a, m).deg() == 0 && !a.isNil()) { Poly<M> inv = invMod(a, m); expect(powMod(a, -3, m), nmul(nmul(inv, inv), inv) % m, "powMod negative"); }
        vector<Poly<M>> P = powersMod(a, 5, m); require(P.size() == 6, "powersMod size"); Poly<M> cur = Poly<M>{1} % m; for (int i = 0; i <= 5; ++i) { expect(P[static_cast<size_t>(i)], cur, "powersMod entry"); cur = nmul(cur, a) % m; }
        Poly<M> f = rnd(ri(40)), ref; for (int i = f.size() - 1; i >= 0; --i) { ref = (nmul(ref, a) + Poly<M>{f.v[i]}) % m; } expect(composeMod(f, a, m), ref, "composeMod"); expect(composeMod(Poly<M>(), a, m), Poly<M>(), "composeMod empty");}
    // factorization over small primes by brute force
    { using D = DynModInt<3>;
      for (uint p : {2u, 3u, 7u, 101u}) { D::setMod(p, 1);
        auto all = [&](int d) { vector<Poly<D>> r; lng cnt = 1; for (int i = 0; i < d; ++i) { cnt *= p; } for (lng c = 0; c < cnt; ++c) { Poly<D> f(d + 1); lng t = c; for (int i = 0; i < d; ++i) { f.v[i] = D(t % p); t /= p; } f.v[d] = 1; r.push_back(f); } return r; };
        int maxd = p == 101 ? 2 : (p == 7 ? 3 : 4); vector<Poly<D>> irr; for (int d = 1; d <= maxd; ++d) { for (const Poly<D> &f : all(d)) { bool red = false; for (const Poly<D> &g : irr) { if (g.deg() <= d / 2 && (f % g).isNil()) { red = true; break; } } if (!red) { irr.push_back(f); } } }
        for (int d = 1; d <= maxd; ++d) { for (const Poly<D> &f : all(d)) { bool brute = false; for (const Poly<D> &g : irr) { brute |= g == f; } expect(isIrreducible(f), brute, "isIrreducible p=" + std::to_string(p)); vector<D> rts; for (uint x = 0; x < p; ++x) { if (eval(f, D(x)) == D(0)) { rts.push_back(D(x)); } } expect(roots(f), rts, "roots p=" + std::to_string(p)); vector<pair<Poly<D>, int>> fac = factor(f); Poly<D> prod = {1}; bool okf = true; for (auto &[g, e] : fac) { okf &= isIrreducible(g) && e >= 1; for (int i = 0; i < e; ++i) { prod = nmul(prod, g); } } for (size_t i = 1; i < fac.size(); ++i) { okf &= poly_detail::lessPoly(fac[i - 1].first, fac[i].first); } require(okf && prod == f, "factor p=" + std::to_string(p)); vector<pair<Poly<D>, int>> sq = squareFree(f); Poly<D> prod2 = {1}; for (auto &[g, e] : sq) { for (int i = 0; i < e; ++i) { prod2 = nmul(prod2, g); } okf &= egcd(g, deriv(g)).deg() <= 0 || deriv(g).isNil(); } require(okf && prod2 == f, "squareFree p=" + std::to_string(p)); } }
        for (int it = 0; it < (full() ? 40 : 12); ++it) { Poly<D> f = {D(1 + ri(int(p) - 1 ? int(p) - 1 : 1))}; vector<pair<Poly<D>, int>> truth; int k = 1 + ri(3); for (int i = 0; i < k; ++i) { Poly<D> g = irr[static_cast<size_t>(ri(int(irr.size())))]; int e = 1 + ri(3); bool dup = false; for (auto &[h, m] : truth) { if (h == g) { m += e; dup = true; } } if (!dup) { truth.emplace_back(g, e); } for (int t = 0; t < e; ++t) { f = nmul(f, g); } } sort(truth.begin(), truth.end(), [](const auto &u, const auto &v) { return poly_detail::lessPoly(u.first, v.first); }); expect(factor(f) == truth, true, "factor with multiplicities p=" + std::to_string(p)); expect(factor(f, 99) == truth, true, "factor other seed p=" + std::to_string(p));}}}
    // factorization over 998244353 from known factors
    for (int it = 0; it < (full() ? 30 : 8); ++it) { Poly<M> f = {M(rng())}; vector<pair<Poly<M>, int>> truth; int k = 1 + ri(4); for (int i = 0; i < k; ++i) { Poly<M> g; if (rng() & 1) { g = {M(rng()), 1}; } else { do { g = {M(rng()), M(rng()), 1}; } while (!roots(g).empty()); } int e = 1 + ri(3); bool dup = false; for (auto &[h, m] : truth) { if (h == g) { m += e; dup = true; } } if (!dup) { truth.emplace_back(g, e); } for (int t = 0; t < e; ++t) { f = nmul(f, g); } } sort(truth.begin(), truth.end(), [](const auto &u, const auto &v) { return poly_detail::lessPoly(u.first, v.first); }); expect(factor(f, 7) == truth, true, "factor mint"); vector<M> rt; for (auto &[g, e] : truth) { if (g.deg() == 1) { rt.push_back(-g.v[0]); } } sort(rt.begin(), rt.end(), [](M u, M v) { return u.val() < v.val(); }); expect(roots(f), rt, "roots mint"); }
    { Poly<M> big = rnd(full() ? 300 : 120); big.v.back() = 1; vector<pair<Poly<M>, int>> fac = factor(big); Poly<M> prod = {1}; bool ok = true; for (auto &[g, e] : fac) { ok &= isIrreducible(g); for (int i = 0; i < e; ++i) { prod = prod * g; } } require(ok && prod == big, "factor random monic"); expect(roots(Poly<M>{5}), vector<M>(), "roots constant"); expect(factor(Poly<M>{5}).size(), static_cast<size_t>(0), "factor constant"); require(!isIrreducible(Poly<M>{5}) && isIrreducible(Poly<M>{3, 1}), "isIrreducible degree 0 and 1"); expect(squareFree(Poly<M>{5}).size(), static_cast<size_t>(0), "squareFree constant"); }
    { using D = DynModInt<3>; D::setMod(3, 1); Poly<D> f = {1, 0, 0, 1}; vector<pair<Poly<D>, int>> sq = squareFree(f); require(sq.size() == 1 && sq[0].first == Poly<D>({1, 1}) && sq[0].second == 3, "squareFree p-th power"); Poly<D> f2 = nmul(nmul(Poly<D>{1, 0, 0, 1}, Poly<D>{1, 0, 0, 1}), Poly<D>{2, 1}); sq = squareFree(f2); require(sq.size() == 2 && sq[0].first == Poly<D>({2, 1}) && sq[0].second == 1 && sq[1].first == Poly<D>({1, 1}) && sq[1].second == 6, "squareFree mixed p-th power"); }}

static void testAdvanced() {
    context = "advanced";
    vector<int> sizes = full() ? vector<int>{1, 2, 3, 4, 5, 8, 9, 17, 50, 64, 100, 300, 1025} : vector<int>{1, 2, 3, 5, 9, 17, 64, 100};
    if (stress()) { sizes.push_back(3000); }
    for (int n : sizes) {
        Poly<M> f = rnd(n), g = rnd(n), w = rnd(n);
        Poly<M> ref; { Poly<M> acc; for (int k = n - 1; k >= 0; --k) { acc = cut(nmul(acc, g), n) + Poly<M>{f.v[k]}; } ref = cut(acc, n); }
        expect(compose(f, g, n), ref, "compose"); expect(composeBrentKung(f, g, n), ref, "composeBrentKung"); expect(compose(f, g, 0), Poly<M>(), "compose 0"); expect(compose(Poly<M>(), g, n), Poly<M>(n), "compose empty f"); expect(compose(f, Poly<M>(), n), cut(Poly<M>{f.v[0]}, n), "compose empty g");
        { Poly<M> g0 = g; g0.v[0] = 0; Poly<M> acc; for (int k = n - 1; k >= 0; --k) { acc = cut(nmul(acc, g0), n) + Poly<M>{f.v[k]}; } expect(compose(f, g0, n), cut(acc, n), "compose g(0)=0"); Poly<M> fs = rnd(n / 2 + 1); acc = Poly<M>(); for (int k = fs.size() - 1; k >= 0; --k) { acc = cut(nmul(acc, g), n) + Poly<M>{fs.v[k]}; } expect(compose(fs, g, n), cut(acc, n), "compose shorter f"); }
        { vector<M> pp = powerProjection(w, g, n), br(static_cast<size_t>(n)); Poly<M> gk = {1}; for (int k = 0; k < n; ++k) { M s = 0; for (int i = 0; i < n; ++i) { s += w.coef(i) * gk.coef(i); } br[static_cast<size_t>(k)] = s; gk = cut(nmul(gk, g), n); } expect(pp, br, "powerProjection"); expect(powerProjection(w, g, 0), vector<M>(), "powerProjection 0");
          vector<M> pe = polynomialPowerEnumerate(g, w, n), br2(static_cast<size_t>(n)); gk = {1}; for (int k = 0; k < n; ++k) { br2[static_cast<size_t>(k)] = cut(nmul(w, gk), n).coef(n - 1); gk = cut(nmul(gk, g), n); } expect(pe, br2, "polynomialPowerEnumerate"); }
        if (n >= 2) { Poly<M> h = g; h.v[0] = 0; h.v[1] = M(ri(1000) + 1); Poly<M> hi = compositionalInverse(h, n); Poly<M> x(n); x.v[1] = 1; require(hi.size() == n, "compositionalInverse size"); expect(compose(hi, h, n), x, "compositionalInverse left"); expect(compose(h, hi, n), x, "compositionalInverse right"); expect(compositionalInverse(h, 1), Poly<M>(1), "compositionalInverse n=1"); Poly<M> hi1 = compositionalInverse(h, n + 1); for (int k : {1, 2, 3, n / 2 + 1, n}) { expect(lagrangeInversionCoefficient(h, k, n), pow(hi1, k, n + 1).coef(n), "lagrangeInversionCoefficient k=" + std::to_string(k)); } expect(lagrangeInversionCoefficient(h, n + 1, n), M(0), "lagrangeInversionCoefficient k>n"); expect(lagrangeInversionCoefficient(h, 0, 0), M(1), "lagrangeInversionCoefficient 0,0"); expect(lagrangeInversionCoefficient(h, 0, 3), M(0), "lagrangeInversionCoefficient 0,n"); }
        if (n <= 100) { int m = 1 + ri(8); vector<M> F(static_cast<size_t>(n * m)); for (M &x : F) { x = M(rng()); } F[0] = M(ri(100) + 1); vector<M> G = Poly<M>::inv2d(F, n, m); vector<M> P(static_cast<size_t>(n * m)); for (int a = 0; a < n; ++a) { for (int b = 0; b < m; ++b) { for (int c = 0; a + c < n; ++c) { for (int d = 0; b + d < m; ++d) { P[static_cast<size_t>((a + c) * m + b + d)] += F[static_cast<size_t>(a * m + b)] * G[static_cast<size_t>(c * m + d)]; } } } } bool ok = P[0] == 1; for (int i = 1; i < n * m; ++i) { ok &= P[static_cast<size_t>(i)] == 0; } require(ok, "inv2d"); }
        { Poly<M> p = rnd(ri(n + 1)), q = rnd(n + 1); q.v[0] = M(ri(100) + 1); int L = 3 * n + 5; vector<M> ser = divSeries(p, q, L).v; for (lng k : {lng(0), lng(1), lng(2), lng(n), lng(2 * n + 1), lng(L - 1)}) { expect(coefOfRationalFps(p, q, k), ser[static_cast<size_t>(k)], "coefOfRationalFps k=" + std::to_string(k)); } expect(coefOfRationalFps(Poly<M>{1}, Poly<M>{1, -1}, 1000000000000000000LL), M(1), "coefOfRationalFps geometric huge");
          for (int l : {0, 1, n, 2 * n}) { for (int m : {0, 1, 5, n}) { expect(sliceRationalFps(p, q, l, l + m), vector<M>(ser.begin() + l, ser.begin() + l + m), "sliceRationalFps"); } } expect(sliceRationalFps(p, Poly<M>{2}, 1, 4), vector<M>({p.coef(1) / 2, p.coef(2) / 2, p.coef(3) / 2}), "sliceRationalFps constant denominator");
          vector<M> c(static_cast<size_t>(n)); for (M &x : c) { x = M(rng()); } vector<M> a(static_cast<size_t>(n)); for (M &x : a) { x = M(rng()); } vector<M> seq = a; for (int i = n; i < 4 * n + 5; ++i) { M s = 0; for (int j = 1; j <= n; ++j) { s += c[static_cast<size_t>(j - 1)] * seq[static_cast<size_t>(i - j)]; } seq.push_back(s); } for (lng k : {lng(0), lng(n - 1), lng(n), lng(3 * n + 1)}) { expect(Poly<M>::linearRecurrenceKth(a, c, k), seq[static_cast<size_t>(k)], "linearRecurrenceKth"); } expect(Poly<M>::consecutiveTerms(a, c, n + 1, n + 2), vector<M>(seq.begin() + n + 1, seq.begin() + 2 * n + 3), "consecutiveTerms"); expect(Poly<M>::consecutiveTerms(a, c, 0, 2), vector<M>(seq.begin(), seq.begin() + 2), "consecutiveTerms prefix"); expect(Poly<M>::linearRecurrenceKth(seq, c, 2), seq[2], "linearRecurrenceKth direct"); expect(Poly<M>::linearRecurrenceKth(a, {}, 5), a.size() > 5 ? a[5] : M(0), "linearRecurrenceKth empty recurrence"); expect(Poly<M>::consecutiveTerms(a, {}, 1, 2), vector<M>({a.size() > 1 ? a[1] : M(0), a.size() > 2 ? a[2] : M(0)}), "consecutiveTerms empty recurrence");
          vector<M> rec = Poly<M>::findLinearRecurrence(seq); require(int(rec.size()) <= n, "findLinearRecurrence length"); bool ok = true; for (size_t i = rec.size(); i < seq.size(); ++i) { M s = 0; for (size_t j = 1; j <= rec.size(); ++j) { s += rec[j - 1] * seq[i - j]; } ok &= s == seq[i]; } require(ok, "findLinearRecurrence reproduces"); expect(Poly<M>::findLinearRecurrence(vector<M>{1, 1, 2, 3, 5, 8, 13}), vector<M>({1, 1}), "findLinearRecurrence Fibonacci"); expect(Poly<M>::findLinearRecurrence(vector<M>{0, 0, 0}), vector<M>(), "findLinearRecurrence zeros"); expect(Poly<M>::findLinearRecurrence(vector<M>{1, 2, 4, 8}), vector<M>({2}), "findLinearRecurrence geometric"); }
        { int m = ri(n + 1), k = n - m; Poly<M> p, q, F = rnd(n + 1); if (pade(F, m, k, p, q)) { require(p.deg() <= m && q.deg() <= k && q.coef(0) == M(1), "pade degrees"); expect(cut(nmul(q, F), n + 1), cut(p, n + 1), "pade congruence"); } Poly<M> p0 = rnd(m + 1), q0 = rnd(k + 1); q0.v[0] = 1; Poly<M> F2 = divSeries(p0, q0, n + 1); require(pade(F2, m, k, p, q), "pade exact status"); expect(cut(nmul(q, F2), n + 1), cut(p, n + 1), "pade exact congruence"); }
        if (n >= 1) { int m = ri(n), k = n - 1 - m; Poly<M> p0 = rnd(m + 1), q0 = rnd(k + 1); q0.v[k] = 1; vector<M> xs, ys; std::set<uint> seen; while (int(xs.size()) < n) { M x = M(rng()); if (!seen.insert(x.val()).second || eval(q0, x) == M(0)) { continue; } xs.push_back(x); ys.push_back(eval(p0, x) / eval(q0, x)); } Poly<M> p, q; require(rationalInterpolate(xs, ys, m, k, p, q), "rationalInterpolate status"); bool good = p.deg() <= m && q.deg() <= k; for (int i = 0; i < n; ++i) { good &= eval(p, xs[static_cast<size_t>(i)]) == ys[static_cast<size_t>(i)] * eval(q, xs[static_cast<size_t>(i)]); } require(good, "rationalInterpolate values"); }
        if (n <= 100) { int k = 1 + ri(4); vector<pair<M, int>> rts; std::set<uint> seen; for (int i = 0; i < k; ++i) { M r = M(rng()); if (seen.insert(r.val()).second) { rts.emplace_back(r, 1 + ri(3)); } } Poly<M> P = rnd(ri(n + 3)); auto [u, c] = partialFractions(P, rts); Poly<M> Q = {1}; for (auto &[r, m] : rts) { for (int t = 0; t < m; ++t) { Q = nmul(Q, Poly<M>{-r, 1}); } } Poly<M> acc = nmul(u, Q); for (size_t i = 0; i < rts.size(); ++i) { Poly<M> base = Q; for (int t = 0; t < rts[i].second; ++t) { base = base / Poly<M>{-rts[i].first, 1}; } for (int j = rts[i].second; j >= 1; --j) { acc += base * c[i][static_cast<size_t>(j - 1)]; base = nmul(base, Poly<M>{-rts[i].first, 1}); } } expect(acc, P, "partialFractions"); }
        { Poly<M> a = rnd(n), b = rnd(n); expect(relaxedMul(a, b, n), cut(nmul(a, b), n), "relaxedMul"); expect(semiRelaxedMul(a, b, n), cut(nmul(a, b), n), "semiRelaxedMul"); expect(relaxedMul(a, b, 2 * n + 1), cut(nmul(a, b), 2 * n + 1), "relaxedMul beyond"); expect(semiRelaxedMul(a, b, 2 * n + 1), cut(nmul(a, b), 2 * n + 1), "semiRelaxedMul beyond");
          RelaxedMul<M> R; bool ok = true; for (int i = 0; i < n; ++i) { M expectPartial = 0; for (int j = 1; j < i; ++j) { expectPartial += a.v[j] * b.v[i - j]; } ok &= R.partial() == expectPartial; ok &= R.push(a.v[i], b.v[i]) == nmul(a, b).coef(i); } require(ok, "RelaxedMul partial and push");
          SemiRelaxedMul<M> S(b); ok = true; for (int i = 0; i < n; ++i) { M expectPartial = 0; for (int j = 0; j < i; ++j) { expectPartial += a.v[j] * b.v[i - j]; } ok &= S.partial() == expectPartial; ok &= S.push(a.v[i]) == nmul(a, b).coef(i); } require(ok, "SemiRelaxedMul partial and push"); }
        { M a = M(rng()), d = M(rng()); Poly<M> ref2 = {1}; for (int i = 0; i < n; ++i) { ref2 = nmul(ref2, Poly<M>{a + d * i, 1}); } expect(Poly<M>::productOfArithmeticProgression(a, d, n), ref2, "productOfArithmeticProgression"); Poly<M> ref3 = {1}; for (int i = 0; i < n; ++i) { ref3 = nmul(ref3, Poly<M>{a, 1}); } expect(Poly<M>::productOfArithmeticProgression(a, M(0), n), ref3, "productOfArithmeticProgression d=0"); Poly<lng> rl = {1}; for (int i = 0; i < min(n, 10); ++i) { rl = nmul(rl, Poly<lng>{2 + 3 * i, 1}); } expect(Poly<lng>::productOfArithmeticProgression(2, 3, min(n, 10)), rl, "productOfArithmeticProgression ring"); expect(Poly<M>::productOfArithmeticProgression(a, d, 0), Poly<M>{1}, "productOfArithmeticProgression 0");
          vector<pair<Poly<M>, Poly<M>>> fr; Poly<M> num, den = {1}; for (int i = 0; i < min(n, 20); ++i) { Poly<M> p = rnd(1 + ri(4)), q = rnd(1 + ri(4)); q.v[0] = 1; fr.emplace_back(p, q); num = nmul(num, q) + nmul(p, den); den = nmul(den, q); } auto [sn, sd] = sumOfRationals(fr); expect(nmul(sn, den), nmul(num, sd), "sumOfRationals"); auto [en, ed] = sumOfRationals(vector<pair<Poly<M>, Poly<M>>>{}); require(en.isNil() && ed == Poly<M>{1}, "sumOfRationals empty"); }}
    { using D = ModInt<1000000007>; int n = 70; Poly<D> f(n), g(n); for (D &x : f.v) { x = D(rng()); } for (D &x : g.v) { x = D(rng()); } Poly<D> acc; for (int k = n - 1; k >= 0; --k) { acc = cut(nmul(acc, g), n) + Poly<D>{f.v[k]}; } expect(compose(f, g, n), cut(acc, n), "compose 1e9+7"); Poly<D> h = g; h.v[0] = 0; h.v[1] = 3; Poly<D> x(n); x.v[1] = 1; expect(compose(compositionalInverse(h, n), h, n), x, "compositionalInverse 1e9+7"); }}

static void testFamous() {
    context = "famous series";
    vector<M> B = Poly<M>::bernoulliNumbers(8); expect(B, vector<M>({1, M(-1) / 2, M(1) / 6, 0, M(-1) / 30, 0, M(1) / 42, 0}), "bernoulliNumbers"); expect(Poly<M>::bernoulliNumbers(0), vector<M>(), "bernoulliNumbers 0"); expect(Poly<M>::bernoulliNumbers(1), vector<M>({1}), "bernoulliNumbers 1");
    vector<M> P = Poly<M>::partitionNumbers(12); expect(P, vector<M>({1, 1, 2, 3, 5, 7, 11, 15, 22, 30, 42, 56}), "partitionNumbers"); expect(Poly<M>::partitionNumbers(0), vector<M>(), "partitionNumbers 0"); { vector<M> big = Poly<M>::partitionNumbers(201); expect(big[100], M(190569292), "partition 100"); expect(big[200], M(ulng(3972999029388ULL) % M::mod()), "partition 200"); vector<M> brute(201); brute[0] = 1; for (int k = 1; k <= 200; ++k) { for (int i = k; i <= 200; ++i) { brute[static_cast<size_t>(i)] += brute[static_cast<size_t>(i - k)]; } } expect(big, brute, "partitionNumbers vs coin DP"); }
    expect(Poly<M>::stirling1Row(4), vector<M>({0, -6, 11, -6, 1}), "stirling1Row 4"); expect(Poly<M>::stirling1Row(0), vector<M>({1}), "stirling1Row 0"); expect(Poly<M>::stirling1Row(1), vector<M>({0, 1}), "stirling1Row 1"); { int n = 40; vector<M> row = Poly<M>::stirling1Row(n); vector<vector<M>> s(static_cast<size_t>(n + 1), vector<M>(static_cast<size_t>(n + 1))); s[0][0] = 1; for (int i = 1; i <= n; ++i) { for (int k = 1; k <= i; ++k) { s[static_cast<size_t>(i)][static_cast<size_t>(k)] = s[static_cast<size_t>(i - 1)][static_cast<size_t>(k - 1)] - M(i - 1) * s[static_cast<size_t>(i - 1)][static_cast<size_t>(k)]; } } expect(row, s[static_cast<size_t>(n)], "stirling1Row vs recurrence"); vector<M> row2 = Poly<M>::stirling2Row(n); vector<vector<M>> t(static_cast<size_t>(n + 1), vector<M>(static_cast<size_t>(n + 1))); t[0][0] = 1; for (int i = 1; i <= n; ++i) { for (int k = 1; k <= i; ++k) { t[static_cast<size_t>(i)][static_cast<size_t>(k)] = t[static_cast<size_t>(i - 1)][static_cast<size_t>(k - 1)] + M(k) * t[static_cast<size_t>(i - 1)][static_cast<size_t>(k)]; } } expect(row2, t[static_cast<size_t>(n)], "stirling2Row vs recurrence"); vector<M> row3 = Poly<M>::eulerianRow(n); vector<vector<M>> e(static_cast<size_t>(n + 1), vector<M>(static_cast<size_t>(n + 1))); e[0][0] = 1; for (int i = 1; i <= n; ++i) { for (int k = 0; k < i; ++k) { e[static_cast<size_t>(i)][static_cast<size_t>(k)] = M(k + 1) * e[static_cast<size_t>(i - 1)][static_cast<size_t>(k)] + (k ? M(i - k) * e[static_cast<size_t>(i - 1)][static_cast<size_t>(k - 1)] : M(0)); } } expect(row3, vector<M>(e[static_cast<size_t>(n)].begin(), e[static_cast<size_t>(n)].begin() + n), "eulerianRow vs recurrence"); }
    expect(Poly<M>::stirling2Row(4), vector<M>({0, 1, 7, 6, 1}), "stirling2Row 4"); expect(Poly<M>::stirling2Row(0), vector<M>({1}), "stirling2Row 0"); expect(Poly<M>::eulerianRow(4), vector<M>({1, 11, 11, 1}), "eulerianRow 4"); expect(Poly<M>::eulerianRow(1), vector<M>({1}), "eulerianRow 1"); expect(Poly<M>::eulerianRow(0), vector<M>({1}), "eulerianRow 0");
    for (int n : {1, 2, 3, 7, 30}) { Poly<M> f = rnd(n), g = prefixSumOfPolynomial(f); M acc = 0; bool ok = g.size() == n + 1; for (int k = 0; k <= 12; ++k) { ok &= eval(g, M(k)) == acc; acc += eval(f, M(k)); } require(ok, "prefixSumOfPolynomial n=" + std::to_string(n)); } expect(prefixSumOfPolynomial(Poly<M>()), Poly<M>(), "prefixSumOfPolynomial empty"); expect(prefixSumOfPolynomial(Poly<M>{0, 1}), Poly<M>({0, M(-1) / 2, M(1) / 2}), "prefixSumOfPolynomial x");}

static void invalid(const std::string &name) {
    Poly<M> a = {1, 2, 3}, zero, x = {0, 1}, m = {1, 1, 1};
    vector<M> pts = {1, 2, 3};
    volatile int minus = -1;
    if (name == "index") { (void)a[3]; }
    if (name == "resize") { a.resize(minus); }
    if (name == "truncate") { a.truncate(minus); }
    if (name == "normalize") { zero.normalize(); }
    if (name == "shift-left") { a <<= minus; }
    if (name == "shift-right") { a >>= minus; }
    if (name == "divmod-zero") { (void)divMod(a, zero); }
    if (name == "monicdiv") { (void)monicDiv(a, Poly<M>{1, 2}); }
    if (name == "pseudodiv") { (void)pseudoDiv(a, zero); }
    if (name == "inv-zero") { (void)inv(Poly<M>{0, 1}, 3); }
    if (name == "inv-negative") { (void)inv(a, minus); }
    if (name == "log-constant") { (void)log(Poly<M>{2, 1}, 3); }
    if (name == "exp-constant") { (void)exp(a, 3); }
    if (name == "pow-negative-valuation") { (void)pow(x, -1, 3); }
    if (name == "powrational") { (void)powRational(x, 1, 2, 4); }
    if (name == "sparsemul") { (void)sparseMul(a, {{-1, M(1)}}); }
    if (name == "cyclic-size") { (void)cyclic(a, a, 2); }
    if (name == "negacyclic-size") { (void)negacyclic(a, a, 2); }
    if (name == "middleproduct") { (void)middleProduct(Poly<M>{1}, a); }
    if (name == "ntt-length") { vector<M> v(3); Poly<M>::ntt(v); }
    if (name == "ntt-doubling") { vector<M> v(1 << 23); Poly<M>::nttDoubling(v); }
    if (name == "multivariate-shape") { (void)Poly<M>::multivariate(pts, pts, {2, 2}); }
    if (name == "multivariate-cyclic") { (void)Poly<M>::multivariateCyclic(pts, pts, {3}); }
    if (name == "gf2k") { (void)PolyCompanion::convolutionGF2k({1}, {1}, 65, 0); }
    if (name == "interpolate-duplicate") { vector<M> xs(40, M(1)), ys(40); for (int i = 0; i < 40; ++i) { xs[static_cast<size_t>(i)] = M(i / 2); } (void)Poly<M>::interpolate(xs, ys); }
    if (name == "interpolate-size") { (void)Poly<M>::interpolate(pts, {1}); }
    if (name == "geometric-duplicate") { (void)Poly<M>::interpolateGeometric(M(1), M(1), pts); }
    if (name == "newton-nodes") { (void)monomialToNewton(a, {1}); }
    if (name == "hermite-empty") { (void)Poly<M>::hermiteInterpolate(vector<M>{1}, vector<vector<M>>{{}}); }
    if (name == "halfgcd-degree") { (void)halfGcd(a, a); }
    if (name == "invmod") { (void)invMod(Poly<M>{1, 1}, Poly<M>{1, 2, 1}); }
    if (name == "modulus-constant") { (void)PolyModulus<M>(Poly<M>{5}); }
    if (name == "powmod-noninvertible") { (void)powMod(Poly<M>{1, 1}, -1, Poly<M>{1, 2, 1}); }
    if (name == "discriminant-constant") { (void)discriminant(Poly<M>{5}); }
    if (name == "cyclotomic") { (void)Poly<M>::cyclotomic(0); }
    if (name == "lagrange") { (void)lagrangeInversionCoefficient(a, 1, 3); }
    if (name == "inv2d") { (void)Poly<M>::inv2d(pts, 2, 2); }
    if (name == "bostan-mori") { (void)coefOfRationalFps(a, x, 3); }
    if (name == "slice-order") { (void)sliceRationalFps(a, m, 3, 2); }
    if (name == "recurrence-short") { (void)Poly<M>::linearRecurrenceKth(vector<M>{1}, vector<M>{1, 1}, 5); }
    if (name == "pade") { Poly<M> p, q; (void)pade(a, minus, 0, p, q); }
    if (name == "rational-points") { Poly<M> p, q; (void)rationalInterpolate(pts, pts, 1, 2, p, q); }
    if (name == "partial-multiplicity") { (void)partialFractions(a, {{M(1), 0}}); }
    if (name == "transposed-size") { (void)Poly<M>::transposedEvalMulti({1}, pts, 2); }
    if (name == "shift-sampling") { (void)Poly<M>::shiftSamplingPoints(pts, M(1), minus); }
    if (name == "powersmod-negative") { (void)powersMod(a, minus, m); }
    if (name == "arbitrary-mod-zero") { (void)PolyCompanion::convolutionArbitraryMod(vector<ulng>{1}, vector<ulng>{1}, 0); }
    std::cerr << "invalid case did not abort: " << name << '\n';
    std::exit(2);}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--mode") && i + 1 < argc) { mode = argv[++i]; }
        else if (!std::strcmp(argv[i], "--seed") && i + 1 < argc) { seed = std::stoull(argv[++i]); }
        else if (!std::strcmp(argv[i], "--invalid") && i + 1 < argc) { invalid(argv[++i]); }
        else { std::cerr << "usage: --mode quick|full|stress --seed N | --invalid CASE\n"; return 2; }}
    rng.seed(seed);
    auto start = std::chrono::steady_clock::now();
    testBasics(); testConvolution(); testDft(); testSeries(); testDivision(); testEvalInterp(); testAlgebra(); testAdvanced(); testFamous();
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "PASS mode=" << mode << " seed=" << seed << " checks=" << checks << " seconds=" << seconds << '\n';
    return 0;}
