#include "../../05-Mathematics/17-numerical_methods.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
static std::mt19937_64 rng;
using ld = long double;
constexpr double EPS = DBL_EPSILON;

void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
string num(ld x) { std::ostringstream out; out << std::setprecision(21) << x; return out.str(); }
string reals(const vector<double> &v) {
    string s = "{";
    for (double x : v) { s += num(x) + ","; }
    return s + "}";}
void near(ld got, ld want, ld tol, const string &op) {
    require(abs(got - want) <= tol, op, num(want) + " tol=" + num(tol), num(got));}
void same(ld got, ld want, const string &op) { require(got == want, op, num(want), num(got)); }
void expect(lng got, lng want, const string &op) { require(got == want, op, std::to_string(want), std::to_string(got)); }
lng randint(lng l, lng r) { return std::uniform_int_distribution<lng>(l, r)(rng); }
double uniform(double l, double r) { return std::uniform_real_distribution<double>(l, r)(rng); }
string status(const RootStatus &s) {
    return "{value=" + num(s.value) + " converged=" + std::to_string(s.converged) + " iterations=" + std::to_string(s.iterations)
        + " residual=" + num(s.residual) + "}";}
void sameStatus(const RootStatus &s, double value, bool converged, int iterations, double residual, const string &op) {
    RootStatus want{value, converged, iterations, residual};
    require(s.value == value && s.converged == converged && s.iterations == iterations && s.residual == residual, op, status(want), status(s));}

// Exact prime field, an independent oracle for the ring and field operations of hornerDerivative and DualNumber.
struct Mod {
    static constexpr lng P = 998244353;
    lng v;
    Mod(lng x = 0) : v((x % P + P) % P) {}
    Mod &operator+=(Mod o) { v = (v + o.v) % P; return *this; }
    Mod &operator-=(Mod o) { v = (v - o.v + P) % P; return *this; }
    Mod &operator*=(Mod o) { v = v * o.v % P; return *this; }
    Mod &operator/=(Mod o) { return *this *= o.inv(); }
    Mod inv() const {
        Mod r = 1, b = *this;
        for (lng e = P - 2; e > 0; e >>= 1, b *= b) { if (e & 1) { r *= b; }}
        return r;}
    friend Mod operator+(Mod a, Mod b) { return a += b; }
    friend Mod operator-(Mod a, Mod b) { return a -= b; }
    friend Mod operator*(Mod a, Mod b) { return a *= b; }
    friend Mod operator/(Mod a, Mod b) { return a /= b; }
    friend Mod operator-(Mod a) { return Mod(-a.v); }
    friend bool operator==(Mod a, Mod b) { return a.v == b.v; }
};
void sameMod(Mod got, Mod want, const string &op) { require(got == want, op, std::to_string(want.v), std::to_string(got.v)); }

// Rational roots p / q from integer coefficients: the discriminant and 4 a c are exact, so each root has relative error <= 3.1 u.
void quadraticRational(lng p1, lng q1, lng p2, lng q2, lng k) {
    double a = double(k * q1 * q2), b = double(-k * (p1 * q2 + p2 * q1)), c = double(k * p1 * p2);
    context = "solveQuadratic a=" + num(a) + " b=" + num(b) + " c=" + num(c);
    auto got = solveQuadratic(a, b, c);
    require(got.size() == 2 && got[0] <= got[1], "two ascending roots with multiplicity", "2 ascending", reals(got));
    ld r1 = ld(p1) / ld(q1), r2 = ld(p2) / ld(q2);
    if (r1 > r2) { swap(r1, r2); }
    if (p1 * q2 == p2 * q1) { same(got[0], double(p1) / double(q1), "double root correctly rounded"); same(got[1], got[0], "double root twice"); }
    else {
        near(got[0], r1, 8 * EPS * abs(r1), "smaller rational root, exact discriminant");
        near(got[1], r2, 8 * EPS * abs(r2), "larger rational root, exact discriminant");}
    double s = std::ldexp(1.0, 300);
    auto neg = solveQuadratic(-a, -b, -c), up = solveQuadratic(a * s, b * s, c * s), mirror = solveQuadratic(a, -b, c);
    require(neg == got && up == got, "negation and power-of-two scaling leave roots bitwise", reals(got), reals(neg) + reals(up));
    require(mirror.size() == 2 && mirror[0] == -got[1] && mirror[1] == -got[0], "b -> -b mirrors the roots", reals(got), reals(mirror));}

void quadraticCases(int count) {
    context = "solveQuadratic fixed cases";
    require(solveQuadratic(0, 4, -2) == vector<double>{0.5}, "linear a = 0", "{0.5}", reals(solveQuadratic(0, 4, -2)));
    same(solveQuadratic(-0.0, 3, 1).at(0), -1.0 / 3, "linear a = -0 gives correctly rounded -c / b");
    require(solveQuadratic(0, 0, 5).empty(), "nonzero constant has no root");
    require(solveQuadratic(1, 0, 1).empty() && solveQuadratic(-3, 2, -1).empty(), "negative discriminant has no real root");
    require(solveQuadratic(5, 0, 0) == vector<double>{0, 0}, "x^2 has the double root 0");
    require(solveQuadratic(2, 6, 0) == vector<double>{-3, 0}, "c = 0 gives -b / a and 0");
    require(solveQuadratic(-1, 0, 4) == vector<double>{-2, 2}, "b = 0 gives symmetric roots");
    double t = std::ldexp(1.0, -29);
    auto k = solveQuadratic(1, -(2 + t), 1 + t);
    require(k == vector<double>{1, 1 + t}, "fma discriminant 2^-58 separates roots 1 and 1 + 2^-29", "{1,1+2^-29}", reals(k));
    auto s = solveQuadratic(1, -1e8, 1);
    ld small = 2 / (1e8L + std::sqrt(1e16L - 4));
    require(s.size() == 2, "cancellation case has two roots", "2", reals(s));
    near(s[0], small, 8 * EPS * small, "small root without cancellation when b^2 >> 4 a c");
    near(s[1], 1 / small, 8 * EPS / small, "large root when b^2 >> 4 a c");
    double h = std::ldexp(1.0, 500), e = std::ldexp(1.0, -500);
    auto big = solveQuadratic(h, -3 * h, 2 * h), tiny = solveQuadratic(e, -3 * e, 2 * e);
    require(big == vector<double>{1, 2} && tiny == vector<double>{1, 2}, "extreme power-of-two scales", "{1,2}", reals(big) + reals(tiny));
    for (int p = -4; p <= 4; ++p) {
        for (int q = 1; q <= 3; ++q) {
            for (int p2 = -4; p2 <= 4; ++p2) { quadraticRational(p, q, p2, 1, q == 2 ? -3 : 1); }}}
    for (int i = 0; i < count; ++i) {
        lng p1 = randint(-60, 60), p2 = i % 5 == 0 ? p1 : randint(-60, 60), q1 = randint(1, 60), q2 = i % 5 == 0 ? q1 : randint(1, 60);
        lng k = randint(1, 16) * (rng() & 1 ? 1 : -1);
        quadraticRational(p1, q1, p2, q2, k);
        lng m = randint(1, 30);
        double a = double(k * q1 * q1), b = double(-2 * k * p1 * q1), c = double(k * (p1 * p1 + m * m));
        context = "solveQuadratic a=" + num(a) + " b=" + num(b) + " c=" + num(c);
        require(solveQuadratic(a, b, c).empty(), "exact negative discriminant -4 k^2 q^2 m^2 has no real root", "{}", reals(solveQuadratic(a, b, c)));
        b = double(randint(1, 1000) * (rng() & 1 ? 1 : -1)); c = double(randint(-1000, 1000));
        same(solveQuadratic(0, b, c).at(0), -c / b, "linear branch is correctly rounded -c / b");}
    // Regression: b^2 and 4 a c under- or overflowed before the power-of-2 scaling.
    for (int e : {-1070, -1000, -600, -200, 200, 600, 1000, 1020}) {
        context = "solveQuadratic scaled by 2^" + std::to_string(e);
        auto sc = [&](double x) { return std::ldexp(x, e); };
        require(solveQuadratic(sc(1), sc(1), sc(-2)) == vector<double>{-2, 1}, "roots -2, 1 survive extreme coefficients", "{-2,1,}", reals(solveQuadratic(sc(1), sc(1), sc(-2))));
        require(solveQuadratic(sc(1), 0, sc(-1)) == vector<double>{-1, 1}, "roots -1, 1 survive extreme coefficients", "{-1,1,}", reals(solveQuadratic(sc(1), 0, sc(-1))));
        require(solveQuadratic(sc(1), 0, sc(1)).empty(), "x^2 + 1 has no real root at any scale", "{}", reals(solveQuadratic(sc(1), 0, sc(1))));}
    std::cout << "PASS solveQuadratic rational, double, linear, Kahan, cancellation and scaling cases=" << count << '\n';}

// Normwise backward error: coefficient k perturbed by eps |a| S^(3-k), S the root-magnitude sum, so M(x) = |a| (|x| + S)^3; A(r) = |p^(m)(r) / m!|.
template<typename Mf, typename Af>
void cubicCheck(double a, double b, double c, double d, const vector<ld> &roots, Mf M, Af A) {
    context = "solveCubic a=" + num(a) + " b=" + num(b) + " c=" + num(c) + " d=" + num(d);
    auto got = solveCubic(a, b, c, d);
    require(got.size() == roots.size() && std::is_sorted(got.begin(), got.end()), "real roots ascending with multiplicity",
        std::to_string(roots.size()) + " roots", reals(got));
    ld scale = 1;
    for (ld r : roots) { scale = max(scale, abs(r)); }
    for (int i = 0; i < int(roots.size()); ++i) {
        int m = int(std::count(roots.begin(), roots.end(), roots[i]));
        ld tol = max(std::pow(64 * EPS * M(roots[i]) / A(roots[i]), 1.0L / m), 4096 * EPS * EPS * scale);
        near(got[i], roots[i], tol, "root " + std::to_string(i) + " of multiplicity " + std::to_string(m) + " within its conditioning");}
    double s = std::ldexp(1.0, -200);
    auto neg = solveCubic(-a, -b, -c, -d), down = solveCubic(a * s, b * s, c * s, d * s);
    require(neg == got && down == got, "negation and power-of-two scaling leave roots bitwise", reals(got), reals(neg) + reals(down));}

// k prod (q_j x - p_j), times x^2 + u x + v with u^2 < 4 v when exactly one linear factor is given.
void cubicExact(const vector<pair<lng, lng>> &lin, lng k, lng u = 0, lng v = 0) {
    vector<lng> c{k};
    auto mul = [&](const vector<lng> &g) {
        vector<lng> h(c.size() + g.size() - 1);
        for (int i = 0; i < int(c.size()); ++i) { for (int j = 0; j < int(g.size()); ++j) { h[i + j] += c[i] * g[j]; }}
        c = h;};
    for (auto [p, q] : lin) { mul({-p, q}); }
    if (lin.size() == 1) { mul({v, u, 1}); }
    vector<ld> roots;
    for (auto [p, q] : lin) { roots.push_back(ld(p) / ld(q)); }
    std::sort(roots.begin(), roots.end());
    ld lead = abs(ld(c[3]));
    auto quad = [&](ld x) { return lin.size() == 1 ? x * x + ld(u) * x + ld(v) : 1.0L; };
    ld S = lin.size() == 1 ? abs(ld(u)) + 2 * std::sqrt(ld(v)) : 0;
    for (ld r : roots) { S += abs(r); }
    auto M = [&](ld x) { return lead * std::pow(abs(x) + S, 3); };
    auto A = [&](ld x) {
        ld m = lead * abs(quad(x));
        for (ld r : roots) { m *= r == x ? 1 : abs(x - r); }
        return m;};
    cubicCheck(double(c[3]), double(c[2]), double(c[1]), double(c[0]), roots, M, A);}

void cubicRandom(double S, bool pair) {
    double a = (rng() & 1 ? 1 : -1) * uniform(0.5, 2);
    if (!pair) {
        vector<double> r(3);
        do {
            for (double &x : r) { x = uniform(-S, S); }
            std::sort(r.begin(), r.end());} while (r[1] - r[0] < S / 16 || r[2] - r[1] < S / 16);
        double b = -a * (r[0] + r[1] + r[2]), c = a * (r[0] * r[1] + r[0] * r[2] + r[1] * r[2]), d = -a * (r[0] * r[1] * r[2]);
        auto M = [&](ld x) { return abs(ld(a)) * std::pow(abs(x) + abs(r[0]) + abs(r[1]) + abs(r[2]), 3); };
        auto A = [&](ld x) {
            ld m = abs(ld(a));
            for (double y : r) { m *= y == x ? 1 : abs(x - y); }
            return m;};
        cubicCheck(a, b, c, d, vector<ld>(r.begin(), r.end()), M, A);
        return;}
    double s = uniform(-S, S), al = uniform(-S, S), be = uniform(S / 8, S), n = al * al + be * be;
    double b = a * (-2 * al - s), c = a * (n + 2 * al * s), d = -a * s * n;
    auto M = [&](ld x) { return abs(ld(a)) * std::pow(abs(x) + abs(s) + 2 * std::sqrt(ld(n)), 3); };
    auto A = [&](ld x) { return abs(ld(a)) * ((x - al) * (x - al) + ld(be) * be); };
    cubicCheck(a, b, c, d, {ld(s)}, M, A);}

void cubicCases(int count) {
    context = "solveCubic a = 0";
    for (auto [b, c, d] : vector<std::array<double, 3>>{{1, -3, 2}, {0, 2, -1}, {2, 0, 3}, {1, 0, 0}, {-3, 1e8, 3}}) {
        require(solveCubic(0, b, c, d) == solveQuadratic(b, c, d), "a = 0 falls back to solveQuadratic", reals(solveQuadratic(b, c, d)),
            reals(solveCubic(0, b, c, d)));}
    cubicExact({{0, 1}, {0, 1}, {0, 1}}, 1);
    cubicExact({{0, 1}, {1, 1}, {2, 1}}, 1);
    cubicExact({{1, 2}, {-3, 1}, {5, 4}}, -3);
    cubicExact({{2, 1}}, 1, 0, 1);
    for (lng r = -6; r <= 6; ++r) { for (lng k : {1, 3, -7}) { cubicExact({{r, 1}, {r, 1}, {r, 1}}, k); }}
    for (lng r = -5; r <= 5; ++r) { for (lng s = -5; s <= 5; ++s) { if (r != s) { cubicExact({{r, 1}, {r, 1}, {s, 1}}, 1); }}}
    // Regressions: rounding put R^2 above Q^3 (first two); an unrestricted polish step moved a copy of 19 to 11; a blocked polish left -55 inexact.
    cubicExact({{-5, 1}, {-5, 1}, {0, 1}}, 1); cubicExact({{-2, 1}, {-2, 1}, {3, 1}}, 1); cubicExact({{11, 1}, {19, 1}, {19, 1}}, 1);
    cubicExact({{-55, 1}, {-55, 1}, {223, 1}}, -10); cubicExact({{-147, 1}, {52, 1}, {52, 1}}, 5);
    // Regression: Q^3 and R^2 over- or underflowed for extreme monic coefficients before the substitution x = 2^e y.
    for (int e : {-340, -250, -100, 100, 250, 340}) {
        context = "solveCubic roots 2^" + std::to_string(e) + " {1, 2, 3} and one root -cbrt(4) 2^" + std::to_string(e);
        auto got = solveCubic(1, std::ldexp(-6, e), std::ldexp(11, 2 * e), std::ldexp(-6, 3 * e));
        require(got.size() == 3, "three roots", "3", reals(got));
        for (int i = 0; i < 3; ++i) { near(std::ldexp(got[i], -e), i + 1, 64 * EPS, "scaled simple root"); }
        got = solveCubic(1, 0, 0, std::ldexp(4, 3 * e));
        require(got.size() == 1, "one real root", "1", reals(got));
        near(std::ldexp(got[0], -e), -std::cbrt(4.0L), 64 * EPS, "scaled cube root");}
    context = "solveCubic reviewer cases";
    require(solveCubic(1, 0, 3e-110, 0) == vector<double>{0}, "x^3 + 3e-110 x has the single real root 0", "{0,}", reals(solveCubic(1, 0, 3e-110, 0)));
    auto tiny = solveCubic(1, 0, -3e-110, 0);
    require(tiny.size() == 3 && tiny[1] == 0, "x^3 - 3e-110 x has roots 0 and +-sqrt(3e-110)", "3 roots", reals(tiny));
    near(tiny[2], std::sqrt(3e-110L), 8 * EPS * 2e-55, "tiny simple root");
    near(solveCubic(1e-100, 0, 0, 1e60).at(0), -std::cbrt(1e160L), 8 * EPS * 5e53, "huge single root");
    // Regression: the monic division over- or underflowed before scaling; non-monic extremes.
    near(solveCubic(1e-300, 0, 0, 1e300).at(0), -1e200L, 8 * EPS * 1e200L, "1e-300 x^3 + 1e300 has the one root -1e200");
    near(solveCubic(1e300, 0, 0, -1e-300).at(0), 1e-200L, 8 * EPS * 1e-200L, "1e300 x^3 - 1e-300 has the one root 1e-200");
    auto wide = solveCubic(1e-170, 0, -1e170, 0);
    require(wide.size() == 3 && wide[1] == 0, "1e-170 x^3 - 1e170 x has roots 0 and +-1e170", "3 roots", reals(wide));
    near(wide[2], 1e170L, 8 * EPS * 1e170L, "huge simple root of a non-monic cubic");
    // Regression: small roots collapsed when the largest root dominated; each root is checked against its own size.
    for (lng big : {lng(1e4), lng(1e6), lng(1e8), lng(1e10), lng(1e12)}) {
        for (auto [u, v] : vector<pair<lng, lng>>{{1, 2}, {-3, 5}, {-7, -7}, {1, 1}}) {
            context = "solveCubic spread roots " + std::to_string(u) + ", " + std::to_string(v) + ", " + std::to_string(big);
            ld s1 = ld(u) + ld(v) + ld(big), s2 = ld(u) * ld(v) + ld(u + v) * ld(big), s3 = ld(u) * ld(v) * ld(big);
            auto got = solveCubic(1, double(-s1), double(s2), double(-s3));
            require(got.size() == 3, "three real roots", "3", reals(got));
            vector<ld> want{ld(min(u, v)), ld(max(u, v)), ld(big)};
            for (int i = 0; i < 3; ++i) { near(got[i], want[i], (u == v && i < 2 ? 1e-7L : 64 * EPS) * max(abs(want[i]), 1.0L), "root within its own magnitude"); }}}
    // Regression: a rounded near-double pair beside a dominant root fell back to trigonometric values off by sqrt(eps) of the large root.
    context = "solveCubic rounded (221, 221, 1e14)";
    auto nd = solveCubic(1, -100000000000442, 44200000000048840, -4.8841e18);
    require(nd.size() == 3, "three real roots", "3", reals(nd));
    near(nd[0], 221, 1e-4, "near-double root"); near(nd[1], 221, 1e-4, "near-double root"); near(nd[2], 1e14, 1e14 * 64 * EPS, "dominant root");
    for (int i = 0; i < count; ++i) {
        double big = (rng() & 1 ? 1 : -1) * std::pow(10.0, uniform(6, 13)), u = uniform(-100, 100), v = u * (1 + uniform(-1e-9, 1e-9));
        context = "solveCubic near-double " + num(u) + ", " + num(v) + " beside " + num(big);
        ld s1 = ld(u) + v + big, s2 = ld(u) * v + (ld(u) + v) * big, s3 = ld(u) * v * big;
        auto got = solveCubic(1, double(-s1), double(s2), double(-s3));
        require(got.size() == 3, "three real roots", "3", reals(got));
        ld scale = max(ld(abs(u)), 1.0L), tol = 64 * std::sqrt(EPS * abs(big) * max(scale, 1.0L));
        vector<ld> want{ld(min(u, v)), ld(max(u, v)), ld(big)};
        std::sort(want.begin(), want.end());
        for (int k = 0; k < 3; ++k) { near(got[k], want[k], abs(want[k]) == abs(ld(big)) ? 64 * EPS * abs(ld(big)) : tol, "root within its conditioning"); }}
    context = "solveCubic reviewer cases";
    auto complexPair = solveCubic(1, 0, -3 + 2.5e-13, 2 + 5e-13);
    require(complexPair.size() == 1, "roots -2 and 1 +- 5e-7 i: one real root, discriminant above rounding", "1 root", reals(complexPair));
    for (int i = 0; i < count; ++i) {
        lng k = randint(1, 16) * (rng() & 1 ? 1 : -1), R = 300;
        vector<lng> p(3);
        do {
            for (lng &x : p) { x = randint(-R, R); }} while (p[0] == p[1] || p[1] == p[2] || p[0] == p[2]);
        cubicExact({{p[0], 1}, {p[1], 1}, {p[2], 1}}, k);
        vector<pair<lng, lng>> q(3);
        do {
            for (auto &[x, y] : q) { y = randint(1, 4); x = randint(-40, 40); }} while (q[0].first * q[1].second == q[1].first * q[0].second
            || q[1].first * q[2].second == q[2].first * q[1].second || q[0].first * q[2].second == q[2].first * q[0].second);
        cubicExact(q, k);
        lng u = randint(-20, 20), v = u * u / 4 + randint(1, 100);
        cubicExact({{randint(-R, R), randint(1, 4)}}, k, u, v);
        cubicExact({{p[0], 1}, {p[0], 1}, {p[1], 1}}, k);
        cubicExact({{p[2], 1}, {p[2], 1}, {p[2], 1}}, k);
        double S = std::pow(10.0, double(randint(-3, 3)));
        cubicRandom(S, false); cubicRandom(S, true);}
    std::cout << "PASS solveCubic integer, rational, double, triple, complex-pair and random-root cases=" << count << '\n';}

template<typename D, typename C> D evalRatio(const vector<C> &P, const vector<C> &Q, D x) {
    D p = D(P.back()), q = D(Q.back());
    for (int i = int(P.size()) - 2; i >= 0; --i) { p = p * x + D(P[i]); }
    for (int i = int(Q.size()) - 2; i >= 0; --i) { q = q * x + D(Q[i]); }
    return p / q;}

void hornerCases(int count) {
    context = "hornerDerivative empty";
    require(hornerDerivative(vector<lng>{}, lng(5)) == pair<lng, lng>{0, 0}, "empty integer polynomial is {0, 0}");
    auto e = hornerDerivative(vector<double>{}, 2.5);
    require(e.first == 0 && e.second == 0, "empty double polynomial is {0, 0}");
    for (int i = 0; i < count; ++i) {
        int n = int(randint(1, 13));
        vector<lng> a(n);
        for (lng &x : a) { x = randint(-50, 50); }
        lng x = randint(-6, 6);
        context = "hornerDerivative degree=" + std::to_string(n - 1) + " x=" + std::to_string(x);
        lll v = 0, d = 0, dd = 0, pw = 1;
        for (int j = 0; j < n; ++j, pw *= x) {
            v += a[j] * pw;
            if (j + 1 < n) { d += lll(j + 1) * a[j + 1] * pw; }
            if (j + 2 < n) { dd += lll(j + 2) * (j + 1) * a[j + 2] * pw; }}
        auto [hv, hd] = hornerDerivative(a, x);
        expect(hv, lng(v), "integer value against the power sum"); expect(hd, lng(d), "integer derivative against the power sum");
        vector<DualNumber<double>> ad(a.begin(), a.end());
        auto [dv, ddv] = hornerDerivative(ad, DualNumber<double>(double(x), 1));
        same(dv.x, ld(v), "DualNumber ring: value"); same(dv.dx, ld(d), "DualNumber ring: dual part is p'");
        same(ddv.x, ld(d), "DualNumber ring: derivative"); same(ddv.dx, ld(dd), "DualNumber ring: derivative dual part is p''");
        // p(x + e1 + e2) = p + p' (e1 + e2) + p'' e1 e2 over nested duals, which needs T{} rather than T = 0.
        using DD = DualNumber<DualNumber<double>>;
        auto [nv, nd] = hornerDerivative(vector<DD>(a.begin(), a.end()), DD(DualNumber<double>(double(x), 1), 1));
        same(nv.x.x, ld(v), "nested DualNumber: value"); same(nv.dx.dx, ld(dd), "nested DualNumber: e1 e2 part is p''");
        same(nd.x.x, ld(d), "nested DualNumber: derivative");
        Mod xm = Mod(randint(0, Mod::P - 1)), vm = 0, dm = 0, pm = 1;
        vector<Mod> am(a.begin(), a.end());
        for (int j = 0; j < n; ++j, pm *= xm) {
            vm += am[j] * pm;
            if (j + 1 < n) { dm += Mod(j + 1) * am[j + 1] * pm; }}
        auto [mv, md] = hornerDerivative(am, xm);
        sameMod(mv, vm, "prime field value"); sameMod(md, dm, "prime field derivative");
        if (n <= 11) {
            lng k = randint(-24, 24);
            // 4^(n-1) p(k / 4) = sum a_j k^j 4^(n-1-j) and 4^(n-2) p'(k / 4) = sum j a_j k^(j-1) 4^(n-1-j), exact in 64-bit mantissas.
            lll num = 0, den = 0;
            for (int j = 0; j < n; ++j) {
                lll pk = 1, p4 = 1;
                for (int t = 1; t < j; ++t) { pk *= k; }
                for (int t = j; t < n - 1; ++t) { p4 *= 4; }
                num += a[j] * (j == 0 ? 1 : pk * k) * p4;
                if (j > 0) { den += lll(j) * a[j] * pk * p4; }}
            auto [lv, ldv] = hornerDerivative(vector<ld>(a.begin(), a.end()), ld(k) / 4);
            same(std::ldexp(lv, 2 * (n - 1)), ld(num), "long double value at a dyadic point (exact)");
            same(n == 1 ? ldv : std::ldexp(ldv, 2 * (n - 2)), ld(den), "long double derivative at a dyadic point (exact)");}}
    std::cout << "PASS hornerDerivative over lng, double DualNumber, prime field and long double cases=" << count << '\n';}

void dualCases(int count) {
    using DM = DualNumber<Mod>;
    for (int i = 0; i < count; ++i) {
        DM a(Mod(randint(0, Mod::P - 1)), Mod(randint(0, Mod::P - 1))), b(Mod(randint(1, Mod::P - 1)), Mod(randint(0, Mod::P - 1)));
        context = "DualNumber<Mod> a=(" + std::to_string(a.x.v) + "," + std::to_string(a.dx.v) + ") b=(" + std::to_string(b.x.v) + ","
            + std::to_string(b.dx.v) + ")";
        DM s = a + b, d = a - b, n = -a, m = a * b, q = a / b;
        sameMod(s.x, a.x + b.x, "sum value"); sameMod(s.dx, a.dx + b.dx, "sum dual part");
        sameMod(d.x, a.x - b.x, "difference value"); sameMod(d.dx, a.dx - b.dx, "difference dual part");
        sameMod(n.x, -a.x, "negation value"); sameMod(n.dx, -a.dx, "negation dual part");
        sameMod(m.x, a.x * b.x, "product value"); sameMod(m.dx, a.dx * b.x + a.x * b.dx, "product rule");
        sameMod(q.x, a.x / b.x, "quotient value"); sameMod(q.dx, (a.dx * b.x - a.x * b.dx) / (b.x * b.x), "quotient rule");
        DM c = a;
        c += b; sameMod(c.x, s.x, "+= value"); sameMod(c.dx, s.dx, "+= dual part");
        c = a; c -= b; sameMod(c.x, d.x, "-= value"); sameMod(c.dx, d.dx, "-= dual part");
        c = a; c *= b; sameMod(c.x, m.x, "*= value"); sameMod(c.dx, m.dx, "*= dual part");
        c = a; c /= b; sameMod(c.x, q.x, "/= value"); sameMod(c.dx, q.dx, "/= dual part");
        c = a; c *= c; sameMod(c.x, a.x * a.x, "aliased *= value"); sameMod(c.dx, Mod(2) * a.x * a.dx, "aliased *= dual part");
        c = b; c /= c; sameMod(c.x, 1, "aliased /= value"); sameMod(c.dx, 0, "aliased /= dual part");
        c = a; c -= c; sameMod(c.x, 0, "aliased -= value"); sameMod(c.dx, 0, "aliased -= dual part");
        c = a; c += c; sameMod(c.x, Mod(2) * a.x, "aliased += value"); sameMod(c.dx, Mod(2) * a.dx, "aliased += dual part");
        vector<Mod> P(randint(1, 8)), Q(randint(1, 8));
        for (Mod &x : P) { x = Mod(randint(-9, 9)); }
        for (Mod &x : Q) { x = Mod(randint(-9, 9)); }
        Q.back() = Mod(randint(1, 9));
        Mod x = Mod(randint(0, Mod::P - 1)), pv = 0, pd = 0, qv = 0, qd = 0, pw = 1;
        for (int j = 0; j < int(max(P.size(), Q.size())); ++j, pw *= x) {
            if (j < int(P.size())) { pv += P[j] * pw; }
            if (j + 1 < int(P.size())) { pd += Mod(j + 1) * P[j + 1] * pw; }
            if (j < int(Q.size())) { qv += Q[j] * pw; }
            if (j + 1 < int(Q.size())) { qd += Mod(j + 1) * Q[j + 1] * pw; }}
        if (qv == 0) { continue; }
        auto f = [&](DM y) { return evalRatio(P, Q, y); };
        sameMod(DM::derivative(f, x), (pd * qv - pv * qd) / (qv * qv), "static derivative of P / Q against the quotient rule");}
    context = "DualNumber<double>";
    for (int i = 0; i < count; ++i) {
        double x = uniform(-3, 3);
        auto f = [](DualNumber<double> y) { return (y * y * y - DualNumber<double>(2) * y + DualNumber<double>(1)) / (y * y + DualNumber<double>(1)); };
        ld X = x, w = X * X + 1, want = ((3 * X * X - 2) * w - (X * X * X - 2 * X + 1) * 2 * X) / (w * w);
        ld bound = ((3 * X * X + 2) * w + (abs(X * X * X) + 2 * abs(X) + 1) * 2 * abs(X)) / (w * w);
        near(DualNumber<double>::derivative(f, x), want, 64 * EPS * bound, "double derivative of a rational function x=" + num(x));
        lng k = randint(-9, 9);
        auto cube = [](DualNumber<DualNumber<double>> y) { return y * y * y - y; };
        auto second = DualNumber<DualNumber<double>>::derivative(cube, DualNumber<double>(double(k), 1));
        same(second.x, ld(3 * k * k - 1), "nested DualNumber first derivative x=" + std::to_string(k));
        same(second.dx, ld(6 * k), "nested DualNumber second derivative x=" + std::to_string(k));}
    DualNumber<double> z;
    require(z.x == 0 && z.dx == 0, "default DualNumber is zero");
    auto inf = DualNumber<double>(1, 1) / DualNumber<double>(0, 0);
    require(std::isinf(inf.x), "division by zero follows IEEE", "inf", num(inf.x));
    std::cout << "PASS DualNumber operators, compound and aliased forms, quotient rule and nested derivatives cases=" << count << '\n';}

// Strictly increasing with the exact double root `root`: sign(f(x)) = sign(x - root) and f'(x) >= 1.
struct Monotone {
    double root, k;
    double operator()(double x) const { double t = x - root; return t * (1 + k * t * t); }
    double slope(double x) const { double t = x - root; return 1 + 3 * k * t * t; }
};
Monotone randomMonotone() { return {uniform(-100, 100), std::exp(uniform(std::log(1e-3), std::log(10.0)))}; }
string describe(const Monotone &g) { return " root=" + num(g.root) + " k=" + num(g.k); }
// Newton steps on Monotone contract by 2 / 3, so a step within tol leaves at most 2 tol to the root.
ld newtonBound(double value, double root, double at, double rt) {
    return 2 * max(ld(at) + ld(rt) * abs(value), 4 * EPS * abs(ld(value))) + 4 * EPS * abs(ld(root)) + 4 * DBL_MIN;}

void bisectionCase(const Monotone &g, double l, double r, int itr, double at, double rt) {
    context = "bisectionRoot" + describe(g) + " l=" + num(l) + " r=" + num(r) + " itr=" + std::to_string(itr) + " abs=" + num(at) + " rel=" + num(rt);
    double lo = min(l, r), hi = max(l, r);
    int calls = 0;
    auto f = [&](double x) {
        ++calls; require(lo <= x && x <= hi, "bisection evaluates inside the bracket", "[" + num(lo) + "," + num(hi) + "]", num(x));
        return g(x);};
    auto s = bisectionRoot(l, r, f, itr, at, rt);
    same(s.residual, abs(g(s.value)), "residual is |f(value)|");
    expect(s.iterations, calls - 2, "iterations are the evaluations after both endpoints");
    require(s.iterations <= itr && (s.converged || s.iterations == itr), "stops early only when converged", "<= itr", status(s));
    ld err = abs(ld(s.value) - g.root), tol = at + rt * max(abs(lo), abs(hi));
    if (s.converged) { require(s.residual == 0 || err <= tol, "converged value meets the tolerance", num(tol), num(err)); }
    ld width = std::ldexp(ld(hi) - lo, -s.iterations) + 2 * (s.iterations + 1) * EPS * max(abs(lo), abs(hi));
    require(err <= width, "the root stays inside the halved bracket", num(width), num(err));
    if (itr >= 2200 && at == 0 && rt == 0) { sameStatus(s, g.root, true, s.iterations, 0, "zero tolerance finds the exact double root"); }
    if (at > 0 && rt == 0 && s.residual != 0) {
        require(s.iterations <= std::ceil(std::log2((ld(hi) - lo) / at)) + 1, "stops once the bracket fits the tolerance", "<= log2(w / tol) + 1", status(s));}}

void newtonCase(const Monotone &g, double x0, double at, double rt) {
    context = "newtonRoot" + describe(g) + " x0=" + num(x0) + " abs=" + num(at) + " rel=" + num(rt);
    int fc = 0, dc = 0;
    auto s = newtonRoot(x0, [&](double x) { ++fc; return g(x); }, [&](double x) { ++dc; return g.slope(x); }, 300, at, rt);
    same(s.residual, abs(g(s.value)), "residual is |f(value)|");
    require(s.converged, "monotone Newton converges", "converged", status(s));
    expect(s.iterations, fc - 1, "iterations are the evaluations after the start");
    expect(dc, s.iterations, "one derivative per step");
    near(s.value, g.root, newtonBound(s.value, g.root, at, rt), "converged value within twice the step tolerance");}

void newtonSafeCase(const Monotone &g, double l, double r, double at, double rt) {
    context = "newtonSafeRoot" + describe(g) + " l=" + num(l) + " r=" + num(r) + " abs=" + num(at) + " rel=" + num(rt);
    double lo = min(l, r), hi = max(l, r);
    int fc = 0;
    auto inside = [&](double x, const string &what) { require(lo <= x && x <= hi, what + " evaluated inside the bracket", "[" + num(lo) + "," + num(hi) + "]", num(x)); };
    auto f = [&](double x) { ++fc; inside(x, "f"); return g(x); };
    auto df = [&](double x) { inside(x, "f'"); return g.slope(x); };
    auto s = newtonSafeRoot(l, r, f, df, 300, at, rt);
    same(s.residual, abs(g(s.value)), "residual is |f(value)|");
    require(s.converged, "safeguarded Newton converges", "converged", status(s));
    expect(s.iterations, fc == 2 ? 0 : fc - 3, "iterations are the steps after both endpoints and the midpoint");
    near(s.value, g.root, newtonBound(s.value, g.root, at, rt), "converged value within twice the step tolerance");}

void secantCase(const Monotone &g, double x0, double x1, double at, double rt) {
    context = "secantRoot" + describe(g) + " x0=" + num(x0) + " x1=" + num(x1) + " abs=" + num(at) + " rel=" + num(rt);
    int fc = 0;
    auto s = secantRoot(x0, x1, [&](double x) { ++fc; return g(x); }, 300, at, rt);
    same(s.residual, abs(g(s.value)), "residual is |f(value)|");
    require(s.converged, "secant converges where k t^2 <= 1/4", "converged", status(s));
    expect(s.iterations, fc - 2, "iterations are the evaluations after both starts");
    near(s.value, g.root, newtonBound(s.value, g.root, at, rt), "converged value within twice the step tolerance");}

// zbrent returns b once |c - b| / 2 <= t with the sign change in [b, c], so |b - root| <= 2 t.
template<typename F>
void brentCase(F g, double root, double l, double r, double at, double rt, const string &name) {
    context = "brentRoot " + name + " root=" + num(root) + " l=" + num(l) + " r=" + num(r) + " abs=" + num(at) + " rel=" + num(rt);
    double lo = min(l, r), hi = max(l, r);
    int fc = 0;
    auto f = [&](double x) { ++fc; require(lo <= x && x <= hi, "brent evaluates inside the bracket", "[" + num(lo) + "," + num(hi) + "]", num(x)); return g(x); };
    auto s = brentRoot(l, r, f, 2000, at, rt);
    same(s.residual, abs(g(s.value)), "residual is |f(value)|");
    require(s.converged, "brent converges within 2000 evaluations", "converged", status(s));
    expect(s.iterations, fc - 2, "iterations are the evaluations after both endpoints");
    ld v = abs(ld(s.value)), bound = (4 * EPS * v + at + rt * v + 2 * DBL_MIN) * (1 + 4 * EPS) + 4 * EPS * v;
    near(s.value, root, bound, "value within 2 t of the sign change");}

void rootCases(int count) {
    auto line = [](double c) { return [c](double x) { return x - c; }; };
    auto one = [](double) { return 1.0; };
    auto sq = [](double x) { return x * x - 2; };
    auto dsq = [](double x) { return 2 * x; };
    double r2 = std::sqrt(2.0);
    context = "root finders fixed cases";
    auto b = bisectionRoot(0, 2, sq);
    require(b.converged && abs(b.value - r2) <= 2.3e-16, "bisection default arguments find sqrt(2) to adjacent doubles", num(r2), status(b));
    sameStatus(bisectionRoot(3, 7, line(3)), 3, true, 0, 0, "bisection zero at l");
    sameStatus(bisectionRoot(7, 3, line(3)), 3, true, 0, 0, "bisection zero at reversed r");
    sameStatus(bisectionRoot(-0.0, 1, line(0)), -0.0, true, 0, 0, "bisection treats -0 as a root");
    sameStatus(bisectionRoot(0, 3, line(1), 0), 0, false, 0, 1, "bisection itr = 0 returns the endpoint of smaller |f|");
    sameStatus(newtonRoot(1, sq, dsq, 100, 0.5), 1.5, true, 1, 0.25, "newton absolute tolerance stops after the first small step");
    sameStatus(newtonRoot(0, sq, dsq), 0, false, 0, 2, "newton zero derivative is not converged");
    sameStatus(newtonRoot(1, sq, [](double) { return std::nan(""); }), 1, false, 0, 1, "newton NaN derivative is not converged");
    sameStatus(newtonRoot(1, line(2), one, 0), 1, false, 0, 1, "newton itr = 0");
    sameStatus(newtonRoot(2, line(2), one), 2, true, 0, 0, "newton start at the root");
    sameStatus(newtonRoot(0, [](double x) { return x * x * x - 2 * x + 2; }, [](double x) { return 3 * x * x - 2; }, 7), 1, false, 7, 1,
        "newton 0 -> 1 -> 0 cycle runs out of iterations");
    auto n = newtonRoot(1, sq, dsq);
    require(n.converged && abs(n.value - r2) <= 2.3e-16 && n.iterations <= 8, "newton default arguments find sqrt(2)", num(r2), status(n));
    sameStatus(secantRoot(-2, 2, sq), 2, false, 0, 2, "secant equal values f(x0) = f(x1) is not converged");
    sameStatus(secantRoot(0, 1, line(2), 0), 1, false, 0, 1, "secant itr = 0");
    sameStatus(secantRoot(0, 2, line(2)), 2, true, 0, 0, "secant start at the root");
    auto e = secantRoot(0, 2, [](double x) { return std::exp(x) - 3; });
    require(e.converged && abs(e.value - std::log(3.0)) <= 2.3e-16, "secant finds log 3", num(std::log(3.0)), status(e));
    sameStatus(newtonSafeRoot(3, 7, line(3), one), 3, true, 0, 0, "newtonSafe zero at l");
    sameStatus(newtonSafeRoot(3, 7, line(7), one), 7, true, 0, 0, "newtonSafe zero at r");
    auto at = newtonSafeRoot(-10, 20, [](double x) { return std::atan(x); }, [](double x) { return 1 / (1 + x * x); });
    require(at.converged && abs(at.value) <= 1e-300, "newtonSafe converges on atan where plain Newton diverges", "0", status(at));
    auto cube = [](double x) { return x * x * x - 1; };
    auto w = newtonSafeRoot(0, 3, cube, [](double) { return 1e-300; });
    require(w.converged && abs(w.value - 1) <= 12 * EPS, "newtonSafe bisects when Newton steps leave the bracket", "1", status(w));
    auto z = newtonSafeRoot(-2, 2, cube, [](double x) { return 3 * x * x; });
    require(z.converged && abs(z.value - 1) <= 12 * EPS, "newtonSafe survives a zero derivative at the midpoint", "1", status(z));
    sameStatus(brentRoot(3, 7, line(3)), 3, true, 0, 0, "brent zero at l");
    sameStatus(brentRoot(3, 7, line(7)), 7, true, 0, 0, "brent zero at r");
    sameStatus(brentRoot(0, 3, line(1), 0), 3, false, 0, 2, "brent itr = 0 returns r");
    auto bc = brentRoot(0, 2, [](double x) { return x * x * x - 2; });
    require(bc.converged && abs(bc.value - std::cbrt(2.0)) <= 8 * EPS && bc.iterations <= 20, "brent finds cbrt(2) superlinearly", num(std::cbrt(2.0)), status(bc));
    // Regression: c - b overflowed on brackets wider than DBL_MAX and stepped outside them.
    bool finite = true;
    auto bw = brentRoot(-DBL_MAX, DBL_MAX, [&](double x) { finite &= std::isfinite(x); return std::atan(x - 3); }, 3000);
    require(finite && bw.converged && abs(bw.value - 3) <= 12 * EPS, "brent on [-DBL_MAX, DBL_MAX] stays finite", "3", status(bw));
    auto bz = brentRoot(-1e308, 1e308, [&](double x) { finite &= std::isfinite(x); return x; });
    require(finite && bz.converged && bz.value == 0, "brent on [-1e308, 1e308] finds 0", "0", status(bz));
    // Contract: iterations count the evaluations after the start, also when an iterate hits f = 0 exactly.
    sameStatus(newtonRoot(1, line(2), one), 2, true, 1, 0, "newton step landing on f = 0 reports one iteration");
    sameStatus(secantRoot(0, 1, line(2)), 2, true, 1, 0, "secant step landing on f = 0 reports one iteration");
    sameStatus(newtonSafeRoot(-1, 3, line(1), one), 1, true, 0, 0, "newtonSafe midpoint f = 0 reports zero steps");
    for (int i = 0; i < count; ++i) {
        Monotone g = randomMonotone();
        double l = g.root - uniform(1e-3, 100), r = g.root + uniform(1e-3, 100), atol = i % 3 == 0 ? std::ldexp(1.0, -int(randint(1, 40))) : 0;
        double rtol = i % 4 == 1 ? std::ldexp(1.0, -int(randint(10, 50))) : 0;
        if (rng() & 1) { swap(l, r); }
        bisectionCase(g, l, r, 2200, 0, 0);
        bisectionCase(g, l, r, int(randint(0, 70)), atol, rtol);
        bisectionCase(g, g.root, r, 10, 0, 0);
        double x0 = g.root + uniform(-200, 200);
        newtonCase(g, x0, atol, rtol);
        newtonSafeCase(g, l, r, atol, rtol);
        double span = 0.5 / std::sqrt(g.k);
        secantCase(g, g.root + uniform(-span, span), g.root + uniform(-span, span), atol, rtol);
        brentCase(g, g.root, l, r, atol, rtol, "monotone k=" + num(g.k));
        double s = uniform(-1, 1);
        brentCase([s](double x) { return x < s ? -1.0 : 1.0; }, s, -1, 1, atol, rtol, "step");
        brentCase([](double x) { double t = x - 1; return t * t * t * t * t * t * t * t * t; }, 1, uniform(-2, 0.9), uniform(1.1, 4), atol + 1e-30, rtol, "(x-1)^9");}
    std::cout << "PASS bisection, newton, newtonSafe, secant and brent status, bracket, tolerance and count cases=" << count << '\n';}

void fixedPointCases(int count) {
    context = "fixedPointIterate";
    int calls = 0;
    double prev = 0;
    auto cosine = [&](double x) { ++calls; prev = x; return std::cos(x); };
    auto s = fixedPointIterate(1, cosine, 1000);
    ld dottie = 0.739085133215160641655312087673873404L, L = std::sin(dottie);
    require(s.converged, "cos iteration converges", "converged", status(s));
    expect(s.iterations, calls, "iterations are the evaluations of g");
    same(s.residual, abs(s.value - prev), "residual is the last step");
    near(s.value, dottie, L / (1 - L) * 4 * EPS + 4 * EPS, "contraction bound L / (1 - L) step around the Dottie number");
    calls = 0;
    auto sqr = [&](double x) { ++calls; prev = x; return x * x; };
    auto d = fixedPointIterate(2, sqr, 100);
    require(!d.converged && std::isfinite(d.value) && d.value == prev && d.iterations == calls && calls == 10,
        "x^2 from 2 stops at the first non-finite iterate", "10 calls, value = last finite", status(d));
    sameStatus(fixedPointIterate(1, [](double x) { return 2 * x + 1; }, 3), 15, false, 3, 8, "divergent 2 x + 1 runs out of iterations");
    sameStatus(fixedPointIterate(5, [](double x) { return x; }), 5, true, 1, 0, "identity converges after one step");
    sameStatus(fixedPointIterate(5, [](double x) { return x; }, 0), 5, false, 0, std::numeric_limits<double>::infinity(), "itr = 0");
    auto h = fixedPointIterate(1, [](double x) { return (x + 2 / x) / 2; }, 100, 1e-3);
    require(h.converged && h.residual <= 1e-3 && abs(h.value - std::sqrt(2.0)) <= 1e-6, "Heron with absolute tolerance", num(std::sqrt(2.0)), status(h));
    for (int i = 0; i < count; ++i) {
        double a = uniform(-0.9, 0.9), b = uniform(-100, 100), at = i % 2 ? std::ldexp(1.0, -int(randint(1, 40))) : 0, rt = i % 3 ? 0 : 1e-9;
        context = "fixedPointIterate g = " + num(a) + " x + " + num(b) + " abs=" + num(at) + " rel=" + num(rt);
        calls = 0;
        auto lin = [&](double x) { ++calls; prev = x; return a * x + b; };
        auto r = fixedPointIterate(uniform(-1000, 1000), lin, 5000, at, rt);
        ld fp = ld(b) / (1 - ld(a)), c = abs(ld(a)), tol = max(ld(at) + rt * abs(ld(r.value)), 4 * EPS * abs(ld(r.value)));
        // With zero tolerance, rounding may leave a cycle of amplitude 4 eps (|x*| + |b|) / (1 - |a|) above the 4-ulp floor.
        ld cycle = 8 * EPS * (abs(fp) + abs(ld(b))) / (1 - c);
        require(r.converged || (at == 0 && rt == 0 && r.iterations == 5000 && r.residual <= cycle), "contraction converges or cycles at rounding level",
            "converged", status(r));
        expect(r.iterations, calls, "iterations are the evaluations of g");
        same(r.residual, abs(r.value - prev), "residual is the last step");
        require(!r.converged || r.residual <= tol, "last step within tolerance", num(tol), num(r.residual));
        near(r.value, fp, (c * r.residual + 4 * EPS * (abs(fp) + abs(ld(b)))) / (1 - c), "contraction error bound");}
    std::cout << "PASS fixedPointIterate convergence, divergence, overflow, status and contraction cases=" << count << '\n';}

// Polynomial with integer coefficients: value, antiderivative and a rounding scale sum (i + 1) |c_i| X^i.
struct Poly {
    vector<lng> c;
    double operator()(double x) const {
        double v = 0;
        for (int i = int(c.size()) - 1; i >= 0; --i) { v = v * x + double(c[i]); }
        return v;}
    ld integral(ld l, ld r) const {
        ld v = 0;
        for (int i = int(c.size()) - 1; i >= 0; --i) { v = v * r + ld(c[i]) / (i + 1); }
        ld w = 0;
        for (int i = int(c.size()) - 1; i >= 0; --i) { w = w * l + ld(c[i]) / (i + 1); }
        return v * r - w * l;}
    ld scale(double l, double r) const {
        ld X = max({1.0, abs(l), abs(r)}), s = 0, p = 1;
        for (int i = 0; i < int(c.size()); ++i, p *= X) { s += (i + 1) * abs(ld(c[i])) * p; }
        return s;}
};
Poly randomPoly(int degree) {
    Poly p{vector<lng>(degree + 1)};
    for (lng &x : p.c) { x = randint(-9, 9); }
    p.c.back() = randint(1, 9);
    return p;}
double dyadic(double lo, double hi) { return std::ldexp(double(randint(lng(lo * 64), lng(hi * 64))), -6); }

struct Closed {
    string name;
    double (*f)(double);
    double l, r;
    ld value;
};
vector<Closed> smooth() {
    return {{"sin", [](double x) { return std::sin(x); }, 0, std::numbers::pi, 2.0L},
            {"exp", [](double x) { return std::exp(x); }, 0, 1, std::numbers::e_v<ld> - 1},
            {"1/(1+x^2)", [](double x) { return 1 / (1 + x * x); }, 0, 1, std::numbers::pi_v<ld> / 4},
            {"cos", [](double x) { return std::cos(x); }, -1, 2, std::sin(2.0L) + std::sin(1.0L)},
            {"log", [](double x) { return std::log(x); }, 1, std::numbers::e, 1.0L},
            {"exp reversed", [](double x) { return std::exp(x); }, 1, 0, 1 - std::numbers::e_v<ld>}};}

void quadratureCases(int count) {
    for (int i = 0; i < count; ++i) {
        double l = dyadic(-4, 4), r = dyadic(-4, 4);
        Poly p = randomPoly(3), q = randomPoly(4);
        int n = int(randint(1, 60)), m = n + (n & 1), calls = 0;
        context = "simpsonFixed l=" + num(l) + " r=" + num(r) + " n=" + std::to_string(n);
        double s = simpsonFixed(l, r, [&](double x) { ++calls; return p(x); }, n);
        expect(calls, m + 1, "odd n rounds up: n + (n & 1) + 1 evaluations");
        ld tol = 8 * (m + 8) * EPS * abs(ld(r) - l) * p.scale(l, r);
        near(s, p.integral(l, r), tol, "exact for cubics");
        if (n & 1) { same(simpsonFixed(l, r, p, n + 1), s, "odd n equals n + 1"); }
        ld h = (ld(r) - l) / m;
        near(simpsonFixed(l, r, q, n), q.integral(l, r) + ld(q.c[4]) * 2 / 15 * (ld(r) - l) * h * h * h * h, 8 * (m + 8) * EPS * abs(ld(r) - l) * q.scale(l, r),
            "quartic error (2 / 15) c4 (r - l) h^4");
        context = "simpsonAdaptive depth 0 quintic l=" + num(l) + " r=" + num(r);
        Poly q5 = randomPoly(5);
        calls = 0;
        double a = simpsonAdaptive(l, r, [&](double x) { ++calls; return q5(x); }, 1, 0);
        expect(calls, 5, "depth 0 evaluates five points");
        near(a, q5.integral(l, r), 64 * EPS * abs(ld(r) - l) * q5.scale(l, r), "Richardson-corrected Simpson (Boole) is exact for quintics");
        int n2 = int(randint(1, 12)), panels = int(randint(1, 6));
        Poly g = randomPoly(int(randint(0, 2 * n2 - 1)));
        context = "gaussLegendreIntegrate n=" + std::to_string(n2) + " panels=" + std::to_string(panels) + " l=" + num(l) + " r=" + num(r);
        calls = 0;
        double gi = gaussLegendreIntegrate(l, r, [&](double x) { ++calls; return g(x); }, gaussLegendreNodes(n2), panels);
        expect(calls, n2 * panels, "panels * n evaluations");
        near(gi, g.integral(l, r), 16 * (n2 * panels + 24) * EPS * abs(ld(r) - l) * g.scale(l, r), "exact up to degree 2 n - 1");
        context = "rombergIntegrate quintic l=" + num(l) + " r=" + num(r);
        near(rombergIntegrate(l, r, q5, 0, 3), q5.integral(l, r), 64 * EPS * abs(ld(r) - l) * q5.scale(l, r), "three levels are exact for quintics");
        int levels = int(randint(1, 5));
        double t = double(randint(2, 4)), h0 = uniform(0.1, 1), L = uniform(-10, 10);
        int pw = int(randint(1, 3));
        vector<double> c(levels);
        for (double &x : c) { x = uniform(-10, 10); }
        context = "richardsonExtrapolate levels=" + std::to_string(levels) + " t=" + num(t) + " p=" + std::to_string(pw) + " h=" + num(h0);
        calls = 0;
        double hh = h0;
        auto A = [&](double y) {
            same(y, hh, "A is called at h, h / t, ...");
            ++calls; hh /= t;
            ld v = L, z = 1, step = std::pow(ld(y), pw);
            for (int j = 1; j < levels; ++j) { z *= step; v += c[j] * z; }
            return double(v);};
        ld mag = abs(L);
        for (double x : c) { mag += abs(x); }
        near(richardsonExtrapolate(h0, A, levels, t, pw), L, 1e-12 * mag, "levels - 1 error terms c_j h^(j p) eliminated");
        expect(calls, levels, "one evaluation per level");}
    context = "quadrature fixed cases";
    same(simpsonFixed(2, 2, [](double) { return 1.0; }), 0, "simpsonFixed l = r");
    same(simpsonFixed(0, 1, [](double x) { return x; }, 1), 0.5, "simpsonFixed n = 1 uses two intervals");
    // Regression: n + 1 overflowed int at n = INT_MAX; the sum 3 * 2^31 is exact.
    if (count >= 1000) { same(simpsonFixed(0, 1, [](double) { return 1.0; }, INT_MAX), 1, "simpsonFixed n = INT_MAX rounds up to 2^31 intervals"); }
    for (int n = 1; n <= 8; ++n) {
        for (int panels = 1; panels <= 3; ++panels) {
            for (auto [l, r] : vector<pair<double, double>>{{-1, 1}, {0, 2}, {1, 0}}) {
                context = "gaussLegendreIntegrate x^2n n=" + std::to_string(n) + " panels=" + std::to_string(panels) + " l=" + num(l) + " r=" + num(r);
                ld fact = 1, f2 = 1, H = (ld(r) - l) / panels;
                for (int j = 1; j <= n; ++j) { fact *= j; }
                for (int j = 1; j <= 2 * n; ++j) { f2 *= j; }
                ld E = panels * std::pow(H, 2 * n + 1) * std::pow(fact, 4) / ((2 * n + 1) * f2 * f2);
                ld I = (std::pow(ld(r), 2 * n + 1) - std::pow(ld(l), 2 * n + 1)) / (2 * n + 1);
                double g = gaussLegendreIntegrate(l, r, [n](double x) { return std::pow(x, 2 * n); }, gaussLegendreNodes(n), panels);
                if (abs(E) >= 1e-9 * abs(I)) { near(I - g, E, 1e-6 * abs(E) + 64 * n * panels * EPS * 2 * std::pow(2.0L, 2 * n), "error 2^(2n+1) (n!)^4 / ((2n+1) ((2n)!)^3) f^(2n)"); }}}}
    for (auto &[name, f, l, r, value] : smooth()) {
        context = "smooth integral " + name;
        near(simpsonAdaptive(l, r, f, 1e-10), value, 1e-9, "simpsonAdaptive within 10 eps");
        near(rombergIntegrate(l, r, f), value, 1e-9, "rombergIntegrate within 10 eps");
        near(gaussLegendreIntegrate(l, r, f, gaussLegendreNodes(10), 4), value, 1e-14, "10-point Gauss on 4 panels of an analytic function");
        near(doubleExponentialIntegrate(l, r, f), value, 1e-11, "doubleExponentialIntegrate within 10 eps");
        near(simpsonFixed(l, r, f, 2000), value, 1e-12, "simpsonFixed with h^4 error");}
    context = "simpsonAdaptive";
    near(simpsonAdaptive(0, 1, [](double x) { return std::sqrt(x); }), 2.0L / 3, 1e-8, "sqrt with a singular derivative");
    int calls = 0;
    double sa = simpsonAdaptive(0, 1, [&](double x) { ++calls; return x < 1.0 / 3 ? 0.0 : 1.0; }, 1e-9, 12);
    near(sa, 2.0L / 3, std::ldexp(1.0, -11), "jump resolved to the depth-12 interval width");
    require(calls <= 3 + 2 * ((1 << 13) - 1), "depth bounds the evaluations", "<= 2^14 + 1", std::to_string(calls));
    calls = 0;
    double nan = simpsonAdaptive(0, 1, [&](double) { ++calls; return std::nan(""); });
    require(std::isnan(nan) && calls == 5, "NaN stops the branch at once", "NaN after 5 calls", num(nan) + " calls=" + std::to_string(calls));
    same(simpsonAdaptive(3, 3, [](double) { return 1.0; }), 0, "l = r");
    // Regression: an absolute eps below the rounding noise of a large smooth integrand forced a near-full tree.
    calls = 0;
    near(simpsonAdaptive(0, 1, [&](double x) { ++calls; return 1e8 * std::exp(x); }), 1e8 * (std::exp(1.0L) - 1), 1e8 * 64 * EPS, "1e8 exp at the default eps");
    require(calls <= 1 << 13, "rounding floor bounds the evaluations for 1e8 exp", "<= 8192", std::to_string(calls));
    calls = 0;
    near(simpsonAdaptive(0, 10, [&](double x) { ++calls; return 100 * std::sin(x); }, 1e-15), 100 * (1 - std::cos(10.0L)), 1e-10, "100 sin at eps 1e-15");
    require(calls <= 1 << 16, "rounding floor bounds the evaluations for 100 sin", "<= 65536", std::to_string(calls));
    context = "rombergIntegrate";
    same(rombergIntegrate(0, 2, [](double x) { return x * x; }, 0, 1), 4, "one level is the trapezoid");
    // Regression: an absolute eps below the rounding noise of 1e6 exp ran all 20 levels.
    calls = 0;
    near(rombergIntegrate(0, 1, [&](double x) { ++calls; return 1e6 * std::exp(x); }), 1e6 * (std::exp(1.0L) - 1), 1e6 * 64 * EPS, "1e6 exp at the default eps");
    require(calls <= 1 + (1 << 8), "rounding floor bounds the levels for 1e6 exp", "<= 257", std::to_string(calls));
    near(rombergIntegrate(0, 2, [](double x) { return x * x * x * x; }, 0, 2), 2.0L / 6 * (0 + 4 + 16), 8 * EPS * 16, "two levels are Simpson");
    for (int levels = 1; levels <= 20; ++levels) {
        calls = 0;
        double v = rombergIntegrate(0, 1, [&](double x) { ++calls; return std::sqrt(x); }, 0, levels);
        expect(calls, (1 << (levels - 1)) + 1, "eps = 0 uses every level: 2^(levels - 1) + 1 evaluations");
        near(v, 2.0L / 3, std::pow(2.0L, -1.5L * (levels - 1)), "sqrt converges at the trapezoid rate h^1.5");}
    std::cout << "PASS simpsonFixed, simpsonAdaptive, gaussLegendreIntegrate, rombergIntegrate, richardsonExtrapolate cases=" << count << '\n';}

void gaussNodeCases(int maxN) {
    ld s35 = std::sqrt(3.0L / 5), s65 = std::sqrt(6.0L / 5), s107 = std::sqrt(10.0L / 7), s30 = std::sqrt(30.0L), s70 = std::sqrt(70.0L);
    vector<vector<pair<ld, ld>>> table{
        {{0, 2}},
        {{-1 / std::sqrt(3.0L), 1}, {1 / std::sqrt(3.0L), 1}},
        {{-s35, 5.0L / 9}, {0, 8.0L / 9}, {s35, 5.0L / 9}},
        {{-std::sqrt(3.0L / 7 + 2.0L / 7 * s65), (18 - s30) / 36}, {-std::sqrt(3.0L / 7 - 2.0L / 7 * s65), (18 + s30) / 36},
         {std::sqrt(3.0L / 7 - 2.0L / 7 * s65), (18 + s30) / 36}, {std::sqrt(3.0L / 7 + 2.0L / 7 * s65), (18 - s30) / 36}},
        {{-std::sqrt(5 + 2 * s107) / 3, (322 - 13 * s70) / 900}, {-std::sqrt(5 - 2 * s107) / 3, (322 + 13 * s70) / 900}, {0, 128.0L / 225},
         {std::sqrt(5 - 2 * s107) / 3, (322 + 13 * s70) / 900}, {std::sqrt(5 + 2 * s107) / 3, (322 - 13 * s70) / 900}}};
    for (int n = 1; n <= 5; ++n) {
        context = "gaussLegendreNodes table n=" + std::to_string(n);
        auto [x, w] = gaussLegendreNodes(n);
        for (int i = 0; i < n; ++i) {
            near(x[i], table[n - 1][i].first, 8 * EPS, "node against the closed form");
            near(w[i], table[n - 1][i].second, 8 * EPS, "weight against the closed form");}}
    for (int n = 1; n <= maxN; ++n) {
        context = "gaussLegendreNodes n=" + std::to_string(n);
        auto [x, w] = gaussLegendreNodes(n);
        require(int(x.size()) == n && int(w.size()) == n, "n nodes and weights");
        for (int i = 0; i < n; ++i) {
            require(-1 < x[i] && x[i] < 1 && w[i] > 0 && (i == 0 || x[i - 1] < x[i]), "ascending interior nodes with positive weights", "", num(x[i]));
            require(x[i] == -x[n - 1 - i] || 2 * i + 1 == n, "symmetric nodes", num(-x[n - 1 - i]), num(x[i]));
            same(w[i], w[n - 1 - i], "symmetric weights");}
        if (n & 1) { near(x[n / 2], 0, 1e-15, "odd n has the middle node 0"); }
        vector<int> ks{0, 1, 2, 3, 2 * n - 2, 2 * n - 1};
        for (int t = 0; t < 4; ++t) { ks.push_back(int(randint(0, 2 * n - 1))); }
        for (int k : ks) {
            if (k > 2 * n - 1) { continue; }
            ld m = 0;
            for (int i = 0; i < n; ++i) { m += w[i] * std::pow(ld(x[i]), k); }
            near(m, k & 1 ? 0 : 2.0L / (k + 1), 8 * (n + k) * EPS, "moment sum w x^k = int_{-1}^{1} x^k, k=" + std::to_string(k));}}
    std::cout << "PASS gaussLegendreNodes tables n<=5, symmetry, order and moments through degree 2n-1 for n<=" << maxN << '\n';}

void differenceCases(int count) {
    for (int i = 0; i < count; ++i) {
        int d = int(randint(0, 6));
        double x = double(randint(-3, 3)), h = rng() & 1 ? 1 : 0.5;
        Poly p = randomPoly(d + 1);
        ld fd = 1;
        for (int j = 2; j <= d; ++j) { fd *= j; }
        context = "finiteDifference d=" + std::to_string(d) + " x=" + num(x) + " h=" + num(h);
        int calls = 0;
        double c = finiteDifference(x, [&](double y) { ++calls; return p(y); }, d, h, true);
        expect(calls, d + 1, "d + 1 evaluations");
        same(c, fd * p.c[d] + fd * (d + 1) * p.c[d + 1] * x, "central difference is exact for degree d + 1");
        double f = finiteDifference(x, p, d, h, false);
        same(f, fd * p.c[d] + fd * (d + 1) * p.c[d + 1] * (x + d * h / 2), "forward difference is p^(d)(x) + (d h / 2) p^(d+1) for degree d + 1");
        double y = uniform(-2, 2);
        auto ex = [](double z) { return std::exp(z); };
        for (bool central : {false, true}) {
            int o = central ? 2 : 1;
            ld tol = 8 * (std::ldexp(1.0, d) + d) * std::pow(EPS, double(o) / (d + o)) * std::exp(ld(y));
            near(finiteDifference(y, ex, d, 0, central), std::exp(ld(y)), tol, "automatic h: error 2^d eps^(o / (d + o)) for exp, central=" + std::to_string(central));}}
    context = "finiteDifference fixed cases";
    for (int d : {8, 10, 12}) {
        ld fd = 1;
        for (int j = 2; j <= d; ++j) { fd *= j; }
        same(finiteDifference(0, [d](double y) { return std::pow(y, d); }, d, 1, false), fd, "forward difference of x^d is d! for d=" + std::to_string(d));}
    int calls = 0;
    same(finiteDifference(0.3, [&](double y) { ++calls; return std::exp(y); }, 0), std::exp(0.3), "d = 0 returns f(x)");
    expect(calls, 1, "d = 0 evaluates once");
    near(finiteDifference(1e6, [](double y) { return y * y; }), 2e6, 1e6 * 1e-9, "automatic h scales with |x|");
    std::cout << "PASS finiteDifference exact polynomial, forward bias, automatic step and order cases=" << count << '\n';}

void doubleExponentialCases() {
    struct Singular { string name; double (*f)(double); double l, r; ld value, tol; };
    // Nonzero endpoints round the abscissa: the 1/sqrt mass within one ulp of the endpoint, 2 sqrt(ulp) <= 4.3e-8, bounds the error.
    vector<Singular> cases{
        {"1/sqrt(x)", [](double x) { return 1 / std::sqrt(x); }, 0, 1, 2, 1e-11},
        {"log x", [](double x) { return std::log(x); }, 0, 1, -1, 1e-11},
        {"log^2 x", [](double x) { return std::log(x) * std::log(x); }, 0, 1, 2, 1e-11},
        {"x^-0.75", [](double x) { return std::pow(x, -0.75); }, 0, 1, 4, 1e-10},
        {"sqrt(1-x^2)", [](double x) { return std::sqrt((1 - x) * (1 + x)); }, -1, 1, std::numbers::pi_v<ld> / 2, 1e-11},
        {"1/sqrt(1-x^2)", [](double x) { return 1 / std::sqrt((1 - x) * (1 + x)); }, -1, 1, std::numbers::pi_v<ld>, 1e-7},
        {"1/sqrt(x) reversed", [](double x) { return 1 / std::sqrt(x); }, 1, 0, -2, 1e-11},
        {"1/sqrt(x-3)", [](double x) { return 1 / std::sqrt(x - 3); }, 3, 4, 2, 1e-7},
        {"exp", [](double x) { return std::exp(x); }, -1, 1, std::numbers::e_v<ld> - 1 / std::numbers::e_v<ld>, 1e-11}};
    for (auto &[name, f, l, r, value, tol] : cases) {
        for (int levels : {12, 20}) {
            context = "doubleExponentialIntegrate " + name + " levels=" + std::to_string(levels);
            int calls = 0;
            double lo = min(l, r), hi = max(l, r);
            double v = doubleExponentialIntegrate(l, r, [&](double x) {
                ++calls; require(lo < x && x < hi, "never evaluates an endpoint", "(" + num(lo) + "," + num(hi) + ")", num(x));
                return f(x);}, 1e-12, levels);
            near(v, value, tol, "closed form with endpoint singularity");
            require(calls <= 1 + 2 * (7 * (1 << levels) + levels), "levels bound the evaluations", "<= 14 2^levels", std::to_string(calls));}}
    context = "doubleExponentialIntegrate fixed cases";
    same(doubleExponentialIntegrate(2, 2, [](double) { return 1.0; }), 0, "l = r");
    same(doubleExponentialIntegrate(0, 0, [](double x) { return 1 / std::sqrt(x); }), 0, "l = r never evaluates a singular endpoint");
    near(doubleExponentialIntegrate(0, 1, [](double x) { return 1 / std::sqrt(x); }, 1e-12, 1), 2, 0.05, "one level is a coarse estimate");
    near(doubleExponentialIntegrate(-2, 3, [](double x) { return x * x; }, 0, 3), 35.0L / 3, 1e-6, "eps = 0 runs the given levels");
    std::cout << "PASS doubleExponentialIntegrate endpoint singularities, reversed and degenerate intervals, evaluation bounds\n";}

void invalid(const string &probe) {
    double inf = std::numeric_limits<double>::infinity(), nan = std::numeric_limits<double>::quiet_NaN();
    auto f = [](double x) { return x; };
    auto one = [](double) { return 1.0; };
    pair<vector<double>, vector<double>> empty, mismatch{{0.0}, {1.0, 1.0}};
    if (probe == "quadratic-zero") { solveQuadratic(0, 0, 0); }
    else if (probe == "quadratic-nan") { solveQuadratic(1, nan, 0); }
    else if (probe == "cubic-infinite") { solveCubic(1, 0, 0, inf); }
    else if (probe == "bisection-sign") { bisectionRoot(1, 2, f); }
    else if (probe == "bisection-iterations") { bisectionRoot(-1, 1, f, -1); }
    else if (probe == "bisection-infinite") { bisectionRoot(-inf, 1, f); }
    else if (probe == "newton-tolerance") { newtonRoot(1, f, one, 10, -1); }
    else if (probe == "newton-relative-nan") { newtonRoot(1, f, one, 10, 0, nan); }
    else if (probe == "newton-start") { newtonRoot(inf, f, one); }
    else if (probe == "safe-sign") { newtonSafeRoot(1, 2, f, one); }
    else if (probe == "safe-iterations") { newtonSafeRoot(-1, 1, f, one, -1); }
    else if (probe == "secant-infinite") { secantRoot(0, inf, f); }
    else if (probe == "secant-tolerance") { secantRoot(0, 1, f, 10, 0, -1); }
    else if (probe == "secant-equal") { secantRoot(1, 1, f); }
    else if (probe == "brent-sign") { brentRoot(1, 2, f); }
    else if (probe == "brent-tolerance") { brentRoot(-1, 1, f, 10, inf); }
    else if (probe == "fixed-iterations") { fixedPointIterate(1, f, -1); }
    else if (probe == "fixed-start") { fixedPointIterate(nan, f); }
    else if (probe == "simpson-intervals") { simpsonFixed(0, 1, f, 0); }
    else if (probe == "adaptive-eps") { simpsonAdaptive(0, 1, f, 0); }
    else if (probe == "adaptive-depth") { simpsonAdaptive(0, 1, f, 1e-9, -1); }
    else if (probe == "gauss-nodes") { gaussLegendreNodes(0); }
    else if (probe == "gauss-panels") { gaussLegendreIntegrate(0, 1, f, gaussLegendreNodes(2), 0); }
    else if (probe == "gauss-empty") { gaussLegendreIntegrate(0, 1, f, empty); }
    else if (probe == "gauss-mismatch") { gaussLegendreIntegrate(0, 1, f, mismatch); }
    else if (probe == "romberg-levels") { rombergIntegrate(0, 1, f, 1e-10, 0); }
    else if (probe == "romberg-levels-high") { rombergIntegrate(0, 1, f, 1e-10, 31); }
    else if (probe == "romberg-eps") { rombergIntegrate(0, 1, f, -1); }
    else if (probe == "richardson-levels") { richardsonExtrapolate(1, f, 0); }
    else if (probe == "richardson-ratio") { richardsonExtrapolate(1, f, 4, 1); }
    else if (probe == "richardson-order") { richardsonExtrapolate(1, f, 4, 2, 0); }
    else if (probe == "difference-order") { finiteDifference(0, f, -1); }
    else if (probe == "difference-order-high") { finiteDifference(0, f, 61); }
    else if (probe == "difference-step") { finiteDifference(0, f, 1, -1); }
    else if (probe == "difference-infinite") { finiteDifference(inf, f); }
    else if (probe == "difference-lost-step") { finiteDifference(1e20, f, 1, 1e-10); }
    else if (probe == "de-levels") { doubleExponentialIntegrate(0, 1, f, 1e-12, 0); }
    else if (probe == "de-levels-high") { doubleExponentialIntegrate(0, 1, f, 1e-12, 21); }
    else if (probe == "de-infinite") { doubleExponentialIntegrate(0, inf, f); }
    else if (probe == "de-eps") { doubleExponentialIntegrate(0, 1, f, -1); }
    else { throw std::runtime_error("unknown assertion probe=" + probe); }}

// Reads "q a b c" and "s n x_1 ... x_n" lines of hexadecimal doubles for the Python exact-arithmetic oracle.
void oracle() {
    string kind;
    auto read = [] { string s; std::cin >> s; return std::strtod(s.c_str(), nullptr); };
    while (std::cin >> kind) {
        if (kind == "q") {
            double a = read(), b = read(), c = read();
            auto r = solveQuadratic(a, b, c);
            std::printf("%zu", r.size());
            for (double x : r) { std::printf(" %a", x); }
            std::printf("\n");}
        else {
            int n;
            std::cin >> n;
            vector<double> v(n);
            for (double &x : v) { x = read(); }
            std::printf("%a %a %a\n", kahanSum(v), neumaierSum(v), pairwiseSum(v));}}}

int main(int argc, char **argv) {
    string mode = "full";
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 1; }
            else if (arg == "--oracle") { oracle(); return 0; }
            else { throw std::runtime_error("unknown argument=" + arg); }}
        rng.seed(seed);
        int R = mode == "quick" ? 1 : mode == "stress" ? 100 : 10;
        quadraticCases(300 * R); cubicCases(100 * R); hornerCases(300 * R); dualCases(300 * R);
        gaussNodeCases(mode == "quick" ? 40 : mode == "stress" ? 400 : 200);
        quadratureCases(100 * R); differenceCases(200 * R); doubleExponentialCases(); fixedPointCases(100 * R); rootCases(50 * R);
        std::cout << "PASS 17-numerical_methods mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        std::cerr << "FAIL 17-numerical_methods mode=" << mode << " seed=" << seed << " " << e.what() << '\n'; return 1;}}
