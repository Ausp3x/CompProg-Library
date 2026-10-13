// Independent oracles for 17-polymini.hpp: schoolbook products and division, naive DFT, series recurrences, power sums, Lagrange, Horner shifts, brute-force recurrences, Kitamasa; Poly<T> agreement is supporting evidence.
#include "../../01-Core/06-modintmini.hpp"
#include "../../01-Core/17-polymini.hpp"
#include "../../01-Core/16-poly.hpp"

static ulng seed = 42;
static std::string mode = "full", context;
static lng checks = 0;
static std::mt19937_64 rng(42);

ulng otherTranslationUnit();

[[noreturn]] static void fail(const std::string &message) {
    std::cerr << "FAIL seed=" << seed << " mode=" << mode << " context=" << context << '\n' << message << '\n';
    std::exit(1);}
template<typename T> static std::string show(const vector<T> &v) {
    std::ostringstream out;
    out << "[" << v.size() << "]";
    for (size_t i = 0; i < v.size() && i < 10; ++i) { out << " " << v[i].val(); }
    if (v.size() > 10) { out << " ..."; }
    return out.str();}
template<typename A, typename B> static void same(const vector<A> &actual, const vector<B> &expected, const std::string &operation) {
    ++checks;
    bool ok = actual.size() == expected.size();
    for (size_t i = 0; ok && i < actual.size(); ++i) { ok = ulng(actual[i].val()) == ulng(expected[i].val()); }
    if (!ok) { fail(operation + "\n expected=" + show(expected) + "\n actual=" + show(actual)); }}
template<typename A, typename B> static void sameValue(const A &actual, const B &expected, const std::string &operation) {
    ++checks;
    if (ulng(actual.val()) != ulng(expected.val())) {
        fail(operation + "\n expected=" + std::to_string(ulng(expected.val())) + "\n actual=" + std::to_string(ulng(actual.val())));}}
static void require(bool ok, const std::string &message) {
    ++checks;
    if (!ok) { fail(message); }}
static int ri(int k) { return int(rng() % ulng(k)); }
static bool full() { return mode != "quick"; }
static bool stress() { return mode == "stress"; }

template<typename T> using V = vector<T>;
template<typename T> static V<T> rv(int n) { V<T> a(n); for (T &x : a) { x = T(rng()); } return a; }
template<typename T> static T pw(T a, ulng e) { T r = 1; for (; e; e >>= 1, a *= a) { if (e & 1) { r *= a; } } return r; }
template<typename T> static V<T> trimmed(V<T> a) { while (a.size() && a.back() == T(0)) { a.pop_back(); } return a; }
template<typename T> static V<T> cut(V<T> a, int n) { a.resize(n); return a; }
template<typename T> static V<T> nmul(const V<T> &a, const V<T> &b) {
    if (a.empty() || b.empty()) { return {}; }
    V<T> r(a.size() + b.size() - 1);
    for (size_t i = 0; i < a.size(); ++i) { for (size_t j = 0; j < b.size(); ++j) { r[i + j] += a[i] * b[j]; } }
    return r;}
template<typename T> static pair<V<T>, V<T>> ndiv(V<T> a, const V<T> &b) {
    V<T> bt = trimmed(b); a = trimmed(a);
    int db = int(bt.size()) - 1, da = int(a.size()) - 1;
    if (da < db) { return {{}, a}; }
    V<T> q(da - db + 1); T il = T(1) / bt[db];
    for (int i = da; i >= db; --i) {
        T c = a[i] * il; q[i - db] = c;
        for (int j = 0; j <= db; ++j) { a[i - db + j] -= c * bt[j]; }}
    a.resize(db);
    return {q, trimmed(a)};}
template<typename T> static T coef(const V<T> &a, int i) { return i < int(a.size()) ? a[i] : T(0); }
template<typename T> static V<T> ninv(const V<T> &a, int n) {
    V<T> g(n); if (!n) { return g; }
    T c = T(1) / a[0]; g[0] = c;
    for (int i = 1; i < n; ++i) { T s = 0; for (int j = 1; j <= i; ++j) { s += coef(a, j) * g[i - j]; } g[i] = -s * c; }
    return g;}
template<typename T> static V<T> nexp(const V<T> &a, int n) {
    V<T> g(n); if (!n) { return g; }
    g[0] = 1;
    for (int i = 1; i < n; ++i) { T s = 0; for (int j = 1; j <= i; ++j) { s += T(j) * coef(a, j) * g[i - j]; } g[i] = s / T(i); }
    return g;}
// i * b_i = i * a_i - sum_{j<i} j * b_j * a_{i-j} for b = log a with a_0 = 1.
template<typename T> static V<T> nlog(const V<T> &a, int n) {
    V<T> b(n);
    for (int i = 1; i < n; ++i) { T s = T(i) * coef(a, i); for (int j = 1; j < i; ++j) { s -= T(j) * b[j] * coef(a, i - j); } b[i] = s / T(i); }
    return b;}
template<typename T> static V<T> npow(V<T> a, lng k, int n) {
    if (k < 0) { a = ninv(a, n); k = -k; }
    V<T> r(n), b = cut(a, n);
    if (!n) { return r; }
    r[0] = 1;
    for (ulng e = ulng(k); e; e >>= 1, b = cut(nmul(b, b), n)) { if (e & 1) { r = cut(nmul(r, b), n); } }
    return r;}
template<typename T> static T neval(const V<T> &a, T x) { T r = 0, p = 1; for (const T &c : a) { r += c * p; p *= x; } return r; }
template<typename T> static V<T> nshift(const V<T> &a, T c) {
    V<T> r;
    for (int i = int(a.size()) - 1; i >= 0; --i) {
        V<T> s(r.size() + 1);
        for (size_t j = 0; j < r.size(); ++j) { s[j + 1] += r[j]; s[j] += r[j] * c; }
        s[0] += a[i]; r = s;}
    return r.size() < a.size() ? cut(r, int(a.size())) : r;}
template<typename T> static V<T> ninterp(const V<T> &xs, const V<T> &ys) {
    int n = int(xs.size()); V<T> p = {T(1)}, r(n);
    for (const T &x : xs) { p = nmul(p, V<T>{-x, T(1)}); }
    for (int i = 0; i < n; ++i) {
        V<T> q(n); T carry = 0, d = 1;
        for (int j = n; j >= 1; --j) { carry = p[j] + carry * xs[i]; q[j - 1] = carry; }
        for (int j = 0; j < n; ++j) { if (j != i) { d *= xs[i] - xs[j]; } }
        T s = ys[i] / d;
        for (int j = 0; j < n; ++j) { r[j] += q[j] * s; }}
    return r;}
template<typename T> static T qnr() {
    ulng p = ulng(T::mod()); T z = 2;
    while (pw(z, (p - 1) / 2) == T(1)) { z += 1; }
    return z;}
template<typename T> static V<T> ndft(const V<T> &a) {
    int n = int(a.size()), lg = std::countr_zero(uint(n));
    if (n == 1) { return a; }
    T w = pw(qnr<T>(), (ulng(T::mod()) - 1) / ulng(n));
    V<T> r(n);
    for (int j = 0; j < n; ++j) {
        int b = 0;
        for (int t = 0; t < lg; ++t) { if (j >> t & 1) { b |= 1 << (lg - 1 - t); } }
        r[j] = neval(a, pw(w, ulng(b)));}
    return r;}
// a_k from the first d terms through x^k mod (x^d - c_1 x^{d-1} - ... - c_d) with naive products.
template<typename T> static T nkth(const V<T> &a, const V<T> &c, lng k) {
    if (k < lng(a.size())) { return a[size_t(k)]; }
    int d = int(c.size());
    if (!d) { return T(0); }
    auto red = [&](V<T> p) {
        for (int i = int(p.size()) - 1; i >= d; --i) { for (int j = 1; j <= d; ++j) { p[i - j] += p[i] * c[j - 1]; } }
        return cut(p, d);};
    if (k <= 4000) {
        V<T> s(a.begin(), a.begin() + d);
        while (lng(s.size()) <= k) { T x = 0; for (int j = 1; j <= d; ++j) { x += c[j - 1] * s[s.size() - j]; } s.push_back(x); }
        return s[size_t(k)];}
    V<T> r = {T(1)}, b = {T(0), T(1)};
    for (ulng e = ulng(k); e; e >>= 1, b = red(nmul(b, b))) { if (e & 1) { r = red(nmul(r, b)); } }
    r = red(r); T res = 0;
    for (int i = 0; i < d; ++i) { res += coef(r, i) * a[i]; }
    return res;}
template<typename T> static bool validRecurrence(const V<T> &a, const V<T> &c) {
    for (size_t i = c.size(); i < a.size(); ++i) {
        T s = 0;
        for (size_t j = 1; j <= c.size(); ++j) { s += c[j - 1] * a[i - j]; }
        if (s != a[i]) { return false; }}
    return true;}
// Smallest L for which some recurrence of length L is valid, by enumerating every c in F^L.
template<typename T> static int bruteComplexity(const V<T> &a) {
    int p = int(T::mod()), n = int(a.size());
    for (int l = 0; l <= n; ++l) {
        V<T> c(l); lng total = 1;
        for (int i = 0; i < l; ++i) { total *= p; }
        for (lng code = 0; code < total; ++code) {
            lng x = code;
            for (int i = 0; i < l; ++i) { c[i] = T(x % p); x /= p; }
            if (validRecurrence(a, c)) { return l; }}}
    return n;}

template<typename F, typename T> static Poly<F> toFull(const V<T> &a) { Poly<F> r(int(a.size())); for (size_t i = 0; i < a.size(); ++i) { r.v[i] = F(a[i].val()); } return r; }
template<typename F, typename T> static V<F> toFullV(const V<T> &a) { return toFull<F>(a).v; }

template<typename T, typename F> struct Suite {
    using P = PolyMini<T>;
    std::string name; int cap, lim;

    explicit Suite(std::string n) : name(std::move(n)) {
        ulng e = ulng(T::mod()) - 1;
        lim = int(min<ulng>(e & -e, ulng(1) << 30));
        cap = int(min<ulng>(ulng(lim / 2) - 1, ulng(T::mod()) - 1));}
    vector<int> sizes(vector<int> base, int top) {
        vector<int> r;
        for (int x : base) { if (x <= min(cap, top)) { r.push_back(x); } }
        return r;}
    vector<int> small() { return sizes({0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 34, 63, 64, 65, 100, 127, 128, 129, 255, 256, 257, 400, 511, 512, 513, 1000}, stress() ? 1000 : full() ? 513 : 65); }
    vector<int> large() { return sizes({1023, 1024, 1025, 4097, 1 << 14, (1 << 16) + 3, 200003, (1 << 19) - 1}, stress() ? 1 << 19 : full() ? (1 << 16) + 3 : 1100); }
    P rp(int n, bool unit = false) { P a = rv<T>(n); if (unit && n && a[0] == T(0)) { a[0] = 1; } return a; }
    P withZeros(P a) { for (int t = ri(3); t > 0 && int(a.size()) < cap; --t) { a.push_back(T(0)); } return a; }
    V<T> distinct(int n) { T c = T(rng()), s = T(rng()); if (s == T(0)) { s = 1; } V<T> xs(n); for (int i = 0; i < n; ++i) { xs[i] = c + s * T(i); } return xs; }
    std::string at(const std::string &op, int n, int m = -1) { return name + " " + op + " n=" + std::to_string(n) + (m >= 0 ? " m=" + std::to_string(m) : ""); }

    void testNtt() {
        for (int len = 1; len <= min(lim, stress() ? 1 << 19 : full() ? 1 << 16 : 1 << 10); len *= 2) {
            context = at("ntt", len);
            V<T> a = rv<T>(len), b = a;
            V<F> f = toFullV<F>(a);
            P::ntt(b); Poly<F>::ntt(f); same(b, f, context + " vs Poly::ntt");
            if (len <= 512) { same(b, ndft(a), context + " vs naive DFT"); }
            else {
                T w = pw(qnr<T>(), (ulng(T::mod()) - 1) / ulng(len)); int lg = std::countr_zero(uint(len));
                for (int t = 0; t < 8; ++t) {
                    int j = ri(len), br = 0;
                    for (int s = 0; s < lg; ++s) { if (j >> s & 1) { br |= 1 << (lg - 1 - s); } }
                    sameValue(b[j], neval(a, pw(w, ulng(br))), context + " spot DFT");}}
            P::ntt(b, true); same(b, a, context + " inverse round trip");
            P::ntt(b, true); Poly<F>::intt(f = toFullV<F>(a)); same(b, f, context + " inverse vs Poly::intt");}}
    void testArithmetic() {
        vector<int> s = small();
        for (int n : s) {
            for (int m : s) {
                if (!full() && ri(3)) { continue; }
                context = at("convolution", n, m);
                P a = rp(n), b = rp(m), c = a * b;
                same(c, nmul<T>(a, b), context + " vs schoolbook");
                same(c, (toFull<F>(a) * toFull<F>(b)).v, context + " vs Poly");
                P sum = a + b, dif = a - b; V<T> es(max(n, m)), ed(max(n, m));
                for (int i = 0; i < max(n, m); ++i) { es[i] = coef<T>(a, i) + coef<T>(b, i); ed[i] = coef<T>(a, i) - coef<T>(b, i); }
                same(sum, es, at("+", n, m)); same(dif, ed, at("-", n, m));}
            context = at("scalar *", n);
            P a = rp(n); T c = T(rng()); V<T> e = a;
            for (T &x : e) { x *= c; }
            same(a * c, e, context); same(a * T(0), V<T>(n), context + " zero");}
        for (int n : large()) {
            context = at("convolution large", n, n / 3 + 33);
            P a = rp(n), b = rp(n / 3 + 33), c = a * b;
            require(int(c.size()) == n + n / 3 + 32, context + " size");
            for (int t = 0; t < 6; ++t) { T x = T(rng()); sameValue(neval<T>(c, x), neval<T>(a, x) * neval<T>(b, x), context + " random-point evaluation"); }
            if (n <= 1 << 16) { same(c, (toFull<F>(a) * toFull<F>(b)).v, context + " vs Poly"); }}
        context = name + " aliasing";
        int an = min(70, cap);
        P a = rp(an), a0 = a, b = a * a;
        same(b, nmul<T>(a, a), context + " square vs schoolbook");
        a = a * a; same(a, b, context + " a = a * a");
        a = a0; a = a + a; same(a, a0 * T(2), context + " a = a + a");
        a = a0; a = a - a; same(a, V<T>(an), context + " a = a - a");
        a = a0; a = divMod(a, a).first; same(a, V<T>{T(1)}, context + " a = a / a");}
    void testAccess() {
        context = name + " deg/trim/eval/deriv/integ";
        P z; require(z.deg() == -1, context + " empty deg");
        P zz(5); require(zz.deg() == -1 && zz.trim().empty(), context + " zero deg/trim");
        for (int n : small()) {
            P a = withZeros(rp(n)); V<T> t = trimmed<T>(a);
            require(a.deg() == int(t.size()) - 1, at("deg", n)); P b = a; same(b.trim(), t, at("trim", n)); same(b, t, at("trim mutates", n));
            T x = T(rng()); sameValue(eval(a, x), neval<T>(a, x), at("eval", n)); sameValue(eval(a, x), eval(toFull<F>(a), F(x.val())), at("eval vs Poly", n));
            sameValue(eval(a, T(0)), coef<T>(a, 0), at("eval at 0", n));
            V<T> d(max(int(a.size()) - 1, 0));
            for (size_t i = 1; i < a.size(); ++i) { d[i - 1] = a[i] * T(int(i)); }
            same(deriv(a), d, at("deriv", n)); same(deriv(a), deriv(toFull<F>(a)).v, at("deriv vs Poly", n));
            T c = T(rng()); V<T> g(a.size() + 1); g[0] = c;
            for (size_t i = 0; i < a.size(); ++i) { g[i + 1] = a[i] / T(int(i + 1)); }
            same(integ(a, c), g, at("integ", n)); same(integ(a), integ(toFull<F>(a)).v, at("integ vs Poly", n));
            same(deriv(integ(a, c)), a, at("deriv(integ)", n));}}
    void testDivision() {
        vector<int> s = small();
        for (int n : s) {
            for (int m : s) {
                if (!m || (!full() && ri(3))) { continue; }
                context = at("divMod", n, m);
                P a = withZeros(rp(n)), b = withZeros(rp(m));
                if (b.deg() < 0) { b[0] = 1; }
                auto [q, r] = divMod(a, b); auto [eq, er] = ndiv<T>(a, b);
                same(q, eq, context + " quotient vs long division"); same(r, er, context + " remainder vs long division");
                auto [fq, fr] = divMod(toFull<F>(a), toFull<F>(b));
                same(q, fq.v, context + " quotient vs Poly"); same(r, fr.v, context + " remainder vs Poly");}}
        context = name + " divMod by constant/zero dividend";
        P b = {T(3)}; auto [q0, r0] = divMod(P(), b); require(q0.empty() && r0.empty(), context + " zero dividend");
        auto [q1, r1] = divMod(P{T(6), T(9)}, b); same(q1, V<T>{T(2), T(3)}, context + " constant divisor"); require(r1.empty(), context + " constant remainder");
        for (int n : large()) {
            context = at("divMod large", n, n / 2 + 7);
            P a = rp(n), b2 = rp(n / 2 + 7); b2.back() = 1;
            auto [q, r] = divMod(a, b2);
            require(int(q.size()) == a.deg() - b2.deg() + 1 && r.deg() < b2.deg() && (r.empty() || r.back() != T(0)), context + " shape");
            for (int t = 0; t < 6; ++t) { T x = T(rng()); sameValue(neval<T>(a, x), neval<T>(q, x) * neval<T>(b2, x) + neval<T>(r, x), context + " a = q * b + r at a random point"); }}}
    void testSeries() {
        for (int n : small()) {
            for (int m : {0, 1, n / 2, n, n + 5, 2 * n + 1}) {
                if (!full() && ri(2)) { continue; }
                P a = rp(m, true);
                context = at("inv", n, m);
                if (m) { P g = inv(a, n); same(g, ninv<T>(a, n), context + " vs recurrence"); same(g, inv(toFull<F>(a), n).v, context + " vs Poly"); }
                same(inv(a, 0), V<T>(), context + " n = 0");
                P l = a; if (m) { l[0] = 1; }
                context = at("log", n, m);
                if (m) { same(log(l, n), nlog<T>(l, n), context + " vs recurrence"); same(log(l, n), log(toFull<F>(l), n).v, context + " vs Poly"); }
                P e = a; if (m) { e[0] = 0; }
                context = at("exp", n, m);
                same(exp(e, n), nexp<T>(e, n), context + " vs recurrence"); same(exp(e, n), exp(toFull<F>(e), n).v, context + " vs Poly");}}
        for (int n : large()) {
            context = at("series large", n);
            P a = rp(n, true), g = inv(a, n);
            V<T> one(n); one[0] = 1; same(cut<T>(a * g, n), one, context + " a * inv(a) = 1");
            P l = a; l[0] = 1; P e = a; e[0] = 0;
            same(exp(log(l, n), n), l, context + " exp(log(a)) = a");
            same(log(exp(e, n), n), e, context + " log(exp(a)) = a");
            if (n <= 1 << 16) { same(exp(e, n), exp(toFull<F>(e), n).v, context + " exp vs Poly"); same(g, inv(toFull<F>(a), n).v, context + " inv vs Poly"); }}}
    void testPow() {
        lng big = std::numeric_limits<lng>::max();
        vector<lng> ks = {0, 1, 2, 3, 5, 17, lng(T::mod()) - 1, lng(min<ulng>(ulng(T::mod()), ulng(big))), 1000000007, big, -1, -2, -7};
        for (int n : small()) {
            if (n > (full() ? 300 : 40)) { continue; }
            for (int v : {0, 1, 2, 5}) {
                for (lng k : ks) {
                    if (!full() && ri(3)) { continue; }
                    if (k < 0 && v) { continue; }
                    if (ulng(T::mod()) <= ulng(n) && k > 1) { continue; }
                    int m = ri(n + 4);
                    P a = rp(m + v, true);
                    for (int i = 0; i < min(v, int(a.size())); ++i) { a[i] = 0; }
                    if (v < int(a.size()) && a[v] == T(0)) { a[v] = 1; }
                    if (k < 0 && a.empty()) { a = {T(1)}; }
                    context = at("pow k=" + std::to_string(k) + " v=" + std::to_string(v), n, m);
                    P r = pow(a, k, n);
                    if (v == 0 || k <= 4000) { same(r, npow<T>(a, k, n), context + " vs binary powering"); }
                    else { same(r, V<T>(n), context + " v * k >= n gives zero"); }
                    same(r, pow(toFull<F>(a), k, n).v, context + " vs Poly");}}}
        context = name + " pow edges";
        same(pow(P(), 0, 3), V<T>{T(1), T(0), T(0)}, context + " 0^0"); same(pow(P(), 5, 3), V<T>(3), context + " 0^5");
        same(pow(P{T(0), T(1)}, 3, 3), V<T>(3), context + " x^3 mod x^3"); same(pow(P{T(0), T(1)}, 2, 3), V<T>{T(0), T(0), T(1)}, context + " x^2 mod x^3");
        same(pow(P{T(2)}, 0, 0), V<T>(), context + " n = 0");
        for (int n : large()) {
            if (n > (1 << 16)) { continue; }
            context = at("pow large", n);
            P a = rp(n, true); a[0] = 0; a[1] = 1;
            P r = pow(a, 3, n), r2 = pow(a, 2, n);
            same(r, cut<T>(cut<T>(a * a, n) * a, n), context + " a^3 by products");
            same(cut<T>(r * r2, n), pow(a, 5, n), context + " a^3 * a^2 = a^5");
            P u = rp(n, true); same(cut<T>(pow(u, -1, n) * u, n), cut<T>(V<T>{T(1)}, n), context + " u^-1 * u");}}
    void testEvaluation() {
        for (int n : small()) {
            for (int m : {0, 1, 2, n, 33, 70}) {
                if (m > cap || (!full() && ri(3))) { continue; }
                context = at("evalMulti", n, m);
                P a = rp(n); V<T> xs = rv<T>(m);
                if (m > 3) { xs[1] = xs[0]; xs[2] = 0; }
                V<T> ys = evalMulti(a, xs), e(m);
                for (int i = 0; i < m; ++i) { e[i] = neval<T>(a, xs[i]); }
                same(ys, e, context + " vs power sums"); same(ys, evalMulti(toFull<F>(a), toFullV<F>(xs)), context + " vs Poly");}
            if (ulng(n) > ulng(T::mod())) { continue; }
            context = at("interpolate", n);
            V<T> xs(n), ys = rv<T>(n); std::set<ulng> seen;
            for (int i = 0; i < n; ++i) { do { xs[i] = T(n < 3 * cap / 4 && ulng(T::mod()) < 4096 ? ulng(ri(int(T::mod()))) : rng()); } while (!seen.insert(ulng(xs[i].val())).second); }
            P f = P::interpolate(xs, ys);
            same(f, ninterp(xs, ys), context + " vs Lagrange"); same(f, Poly<F>::interpolate(toFullV<F>(xs), toFullV<F>(ys)).v, context + " vs Poly");
            context = at("taylorShift", n);
            for (T c : {T(0), T(1), -T(1), T(rng())}) {
                P a = withZeros(rp(n)), s = taylorShift(a, c);
                same(s, nshift<T>(a, c), context + " vs Horner expansion"); same(s, taylorShift(toFull<F>(a), F(c.val())).v, context + " vs Poly");}}
        for (int n : large()) {
            if (n > (stress() ? 1 << 17 : 1 << 14)) { continue; }
            context = at("evalMulti/interpolate large", n);
            P a = rp(n); V<T> xs = distinct(n), ys = evalMulti(a, xs);
            for (int t = 0; t < 20; ++t) { int i = ri(n); sameValue(ys[i], neval<T>(a, xs[i]), context + " spot evaluation"); }
            P f = P::interpolate(xs, ys); same(f, a, context + " interpolate(evalMulti(a)) = a");
            T c = T(rng()); P s = taylorShift(a, c);
            for (int t = 0; t < 6; ++t) { T x = T(rng()); sameValue(neval<T>(s, x), neval<T>(a, x + c), context + " taylorShift at a random point"); }}}
    void testRecurrences() {
        for (int n : small()) {
            for (int d : {0, 1, 2, n / 4, n / 2}) {
                if (2 * d > n || (!full() && ri(2))) { continue; }
                context = at("findLinearRecurrence generated d=" + std::to_string(d), n);
                V<T> c = rv<T>(d), a = rv<T>(d);
                while (int(a.size()) < n) { T x = 0; for (int j = 1; j <= d; ++j) { x += c[j - 1] * a[a.size() - j]; } a.push_back(x); }
                V<T> got = P::findLinearRecurrence(a);
                require(validRecurrence(a, got) && int(got.size()) <= d, context + " valid and no longer than the generator");
                if (int(got.size()) == d) { same(got, c, context + " unique minimal recurrence"); }
                same(got, Poly<F>::findLinearRecurrence(toFullV<F>(a)), context + " vs Poly");
                context = at("linearRecurrenceKth d=" + std::to_string(d), n);
                V<T> pre(a.begin(), a.begin() + min(n, d + ri(3)));
                for (lng k : {lng(0), lng(d), lng(n), lng(n) + 1000, lng(1e9) + 7, std::numeric_limits<lng>::max(), lng(ri(5000))}) {
                    if (!full() && k > 4000 && d > 30) { continue; }
                    T r = P::linearRecurrenceKth(pre, c, k);
                    sameValue(r, nkth(pre, c, k), context + " k=" + std::to_string(k) + " vs Kitamasa/iteration");
                    sameValue(r, Poly<F>::linearRecurrenceKth(toFullV<F>(pre), toFullV<F>(c), k), context + " k=" + std::to_string(k) + " vs Poly");}}
            context = at("findLinearRecurrence random", n);
            V<T> a = rv<T>(n);
            if (n > 2) { a[0] = 0; }
            V<T> got = P::findLinearRecurrence(a);
            require(validRecurrence(a, got), context + " valid");
            same(got, Poly<F>::findLinearRecurrence(toFullV<F>(a)), context + " vs Poly");}
        context = name + " recurrence edges";
        require(P::findLinearRecurrence({}).empty() && P::findLinearRecurrence(V<T>(9)).empty(), context + " empty/zero sequence");
        same(P::findLinearRecurrence(V<T>{T(0), T(0), T(1)}), Poly<F>::findLinearRecurrence(V<F>{F(0), F(0), F(1)}), context + " 0 0 1 vs Poly");
        sameValue(P::linearRecurrenceKth(V<T>{T(4), T(5)}, {}, 1), T(5), context + " d = 0 inside");
        sameValue(P::linearRecurrenceKth(V<T>{T(4), T(5)}, {}, 2), T(0), context + " d = 0 outside");}
    void run() {
        lng before = checks;
        testNtt(); testArithmetic(); testAccess(); testDivision(); testSeries(); testPow(); testEvaluation(); testRecurrences();
        std::cout << name << ": " << checks - before << " checks (cap " << cap << ")\n";}
};

// Exhaustive minimality of Berlekamp–Massey over GF(p) for every sequence of length <= n, plus agreement with Poly in the non-unique cases.
template<uint p> static void bruteRecurrences(int n) {
    using T = ModIntMini<p>; using F = ModInt<p>;
    lng count = 0;
    for (int len = 0; len <= n; ++len) {
        lng total = 1;
        for (int i = 0; i < len; ++i) { total *= p; }
        for (lng code = 0; code < total; ++code) {
            V<T> a(len); lng x = code;
            for (int i = 0; i < len; ++i) { a[i] = T(x % p); x /= p; }
            context = "GF(" + std::to_string(p) + ") brute recurrence " + show(a);
            V<T> c = PolyMini<T>::findLinearRecurrence(a);
            require(validRecurrence(a, c) && int(c.size()) == bruteComplexity(a), context + " minimal length");
            same(c, Poly<F>::findLinearRecurrence(toFullV<F>(a)), context + " vs Poly");
            if (len && !c.empty()) {
                lng k = lng(ri(1 << 20));
                sameValue(PolyMini<T>::linearRecurrenceKth(V<T>(a.begin(), a.begin() + lng(c.size())), c, k), nkth(V<T>(a.begin(), a.begin() + lng(c.size())), c, k), context + " kth");}
            ++count;}}
    for (int len = 1; ulng(p - 1) % ulng(len) == 0; len *= 2) {
        context = "GF(" + std::to_string(p) + ") ntt length " + std::to_string(len);
        V<T> x = rv<T>(len), y = x; V<F> fy = toFullV<F>(x);
        PolyMini<T>::ntt(y); Poly<F>::ntt(fy); same(y, fy, context + " vs Poly"); same(y, ndft(x), context + " vs naive DFT");
        PolyMini<T>::ntt(y, true); same(y, x, context + " round trip");}
    std::cout << "GF(" << p << ") brute recurrences: " << count << " sequences\n";}

// The oracles themselves on hand-computed cases.
static void oracleSelfTest() {
    using T = ModIntMini<998244353>;
    context = "oracle self-test";
    same(nmul<T>({T(1), T(1)}, {T(1), T(1)}), V<T>{T(1), T(2), T(1)}, "nmul (1 + x)^2");
    same(ninv<T>({T(1), T(-1)}, 4), V<T>{T(1), T(1), T(1), T(1)}, "ninv 1 / (1 - x)");
    same(nexp<T>({T(0), T(1)}, 4), V<T>{T(1), T(1), T(1) / T(2), T(1) / T(6)}, "nexp e^x");
    same(nlog<T>({T(1), T(-1)}, 4), V<T>{T(0), T(-1), -T(1) / T(2), -T(1) / T(3)}, "nlog 1 - x");
    auto [q, r] = ndiv<T>({T(-1), T(0), T(1)}, {T(-1), T(1)});
    same(q, V<T>{T(1), T(1)}, "ndiv quotient"); require(r.empty(), "ndiv remainder");
    same(nshift<T>({T(0), T(0), T(1)}, T(1)), V<T>{T(1), T(2), T(1)}, "nshift x^2 at x + 1");
    same(ninterp<T>({T(0), T(1), T(2)}, {T(1), T(2), T(5)}), V<T>{T(1), T(0), T(1)}, "ninterp x^2 + 1");
    same(npow<T>({T(1), T(1)}, 3, 4), V<T>{T(1), T(3), T(3), T(1)}, "npow (1 + x)^3");
    same(npow<T>({T(1), T(-1)}, -2, 3), V<T>{T(1), T(2), T(3)}, "npow (1 - x)^-2");
    same(ndft<T>({T(3), T(5)}), V<T>{T(8), T(-2)}, "ndft length 2");
    sameValue(nkth<T>({T(0), T(1)}, {T(1), T(1)}, 90), T(2880067194370816120ULL), "nkth Fibonacci 90");
    sameValue(nkth<T>({T(0), T(1)}, {T(1), T(1)}, 4000), nkth<T>({T(0), T(1)}, {T(1), T(1)}, 4001) - nkth<T>({T(0), T(1)}, {T(1), T(1)}, 3999), "nkth iteration/Kitamasa seam");
    require(bruteComplexity<ModIntMini<2>>({0, 0, 1}) == 3 && bruteComplexity<ModIntMini<3>>({1, 1, 2, 0, 2, 2}) == 2 && bruteComplexity<ModIntMini<5>>({}) == 0, "bruteComplexity");
    require(validRecurrence<T>({T(1), T(1), T(2), T(3)}, {T(1), T(1)}) && !validRecurrence<T>({T(1), T(1), T(3)}, {T(1), T(1)}), "validRecurrence");}

static void invalid(const std::string &which) {
    using T = ModIntMini<998244353>; using P = PolyMini<T>;
    if (which == "divmod-zero") { divMod(P{T(1)}, P{T(0)}); }
    else if (which == "inv-zero") { inv(P{T(0), T(1)}, 3); }
    else if (which == "inv-empty") { inv(P(), 2); }
    else if (which == "inv-negative") { inv(P{T(1)}, -1); }
    else if (which == "log-constant") { log(P{T(2), T(1)}, 3); }
    else if (which == "exp-constant") { exp(P{T(1), T(1)}, 3); }
    else if (which == "pow-negative-valuation") { pow(P{T(0), T(1)}, -1, 3); }
    else if (which == "pow-negative-n") { pow(P{T(1)}, 2, -1); }
    else if (which == "ntt-length") { V<T> a(3); P::ntt(a); }
    else if (which == "ntt-limit") { V<ModIntMini<17>> a(32); PolyMini<ModIntMini<17>>::ntt(a); }
    else if (which == "convolution-limit") { PolyMini<ModIntMini<97>> a(40), b(40); a = a * b; }
    else if (which == "interpolate-size") { P::interpolate({T(1), T(2)}, {T(1)}); }
    else if (which == "interpolate-duplicate") { P::interpolate({T(1), T(1)}, {T(1), T(2)}); }
    else if (which == "kth-negative") { P::linearRecurrenceKth({T(1)}, {T(1)}, -1); }
    else if (which == "kth-short") { P::linearRecurrenceKth({T(1)}, {T(1), T(1)}, 5); }
    else { fail("unknown --invalid case " + which); }
    std::cerr << "precondition not rejected: " << which << '\n';
    std::exit(3);}

int main(int argc, char **argv) {
    std::string bad;
    for (int i = 1; i < argc; ++i) {
        std::string a = argv[i];
        if (a == "--mode" && i + 1 < argc) { mode = argv[++i]; }
        else if (a == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid" && i + 1 < argc) { bad = argv[++i]; }}
    if (!bad.empty()) { invalid(bad); }
    rng.seed(seed);
    auto start = std::chrono::steady_clock::now();
    oracleSelfTest();
    context = "second translation unit";
    require(otherTranslationUnit() == 998244353ULL, "two-TU inline instantiation");
    Suite<ModIntMini<998244353>, ModInt<998244353>>("ModIntMini<998244353>").run();
    Suite<ModIntMini<7340033>, ModInt<7340033>>("ModIntMini<7340033>").run();
    Suite<ModIntMini<257>, ModInt<257>>("ModIntMini<257>").run();
    Suite<ModIntMini<97>, ModInt<97>>("ModIntMini<97>").run();
    Suite<ModIntMini<17>, ModInt<17>>("ModIntMini<17>").run();
    Suite<ModInt64Mini<4179340454199820289ULL>, ModInt64<4179340454199820289ULL>>("ModInt64Mini<29*2^57+1>").run();
    Suite<mint, mint>("full mint").run();
    DynModIntMini<0>::setMod(469762049); DynModInt<0>::setMod(469762049);
    Suite<DynModIntMini<0>, DynModInt<0>>("DynModIntMini 469762049").run();
    DynModIntMini<0>::setMod(65537); DynModInt<0>::setMod(65537);
    Suite<DynModIntMini<0>, DynModInt<0>>("DynModIntMini 65537 after setMod").run();
    bruteRecurrences<2>(full() ? 10 : 6);
    bruteRecurrences<3>(full() ? 7 : 4);
    if (full()) { bruteRecurrences<5>(5); }
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "PolyMini mode=" << mode << " seed=" << seed << ": " << checks << " checks in " << std::fixed << std::setprecision(2) << seconds << "s\n";}
