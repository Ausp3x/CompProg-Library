#include "../../05-Mathematics/16-linear_algebra.hpp"
#include "../../01-Core/05-modint.hpp"

using F2 = ModInt<2>;
using F3 = ModInt<3>;
using F5 = ModInt<5>;
using F7 = ModInt<7>;
using Z12 = ModInt<12>;
using MP = ModInt<1000000007>;
using P61 = ModInt64<2305843009213693951ULL>;
using P62 = ModInt64<4179340454199820289ULL>;
using Dyn = DynModInt<1>;

lng checks = 0;
ulng seed = 1;
std::mt19937_64 rng;
bool quick = false, stress = false;

int reps(int full) { return quick ? max(1, full / 10) : stress ? 4 * full : full; }
int rnd(int lo, int hi) { return lo + int(rng() % ulng(hi - lo + 1)); }
template<typename F> void check(bool ok, F &&msg) {
    ++checks;
    if (!ok) { cout << "FAIL " << msg() << " seed=" << seed << endl; exit(1); }}
string str(lll x) {
    if (x == 0) { return "0"; }
    ulll u = x < 0 ? -ulll(x) : ulll(x);
    string s;
    for (; u; u /= 10) { s += char('0' + int(u % 10)); }
    if (x < 0) { s += '-'; }
    reverse(s.begin(), s.end());
    return s;}
template<typename T> string show(const T &x) {
    if constexpr (std::is_same_v<T, lll>) { return str(x); }
    else {
        std::ostringstream os;
        os << std::setprecision(17) << x;
        return os.str();}}
template<typename T> string show(const vector<T> &v) {
    string s = "[";
    for (int i = 0; i < int(v.size()); ++i) { s += (i ? ", " : "") + show(v[i]); }
    return s + "]";}

// Generators.
template<typename T> T relem(int zero = 0) {
    if (rnd(0, 99) < zero) { return T(0); }
    if constexpr (std::is_floating_point_v<T>) { return T(rnd(-1000000, 1000000)) / T(1000000); }
    else { return T(rng()); }}
template<typename T> vector<T> rvec(int n, int zero = 0) {
    vector<T> v(n);
    for (auto &x : v) { x = relem<T>(zero); }
    return v;}
template<typename T> Mat<T> randMat(int n, int m, int zero = 0) {
    Mat<T> a(n);
    for (auto &row : a) { row = rvec<T>(m, zero); }
    return a;}
template<typename T> Mat<T> eye(int n) {
    Mat<T> a(n, vector<T>(n, T(0)));
    for (int i = 0; i < n; ++i) { a[i][i] = T(1); }
    return a;}
// Own product: a has a.size() rows and b.size() columns, the result has k columns.
template<typename T> Mat<T> mul(const Mat<T> &a, const Mat<T> &b, int k) {
    Mat<T> c(a.size(), vector<T>(k, T(0)));
    for (int i = 0; i < int(a.size()); ++i) {
        for (int j = 0; j < k; ++j) {
            for (int t = 0; t < int(b.size()); ++t) { c[i][j] += a[i][t] * b[t][j]; }}}
    return c;}
template<typename T> vector<T> mulVec(const Mat<T> &a, const vector<T> &x) {
    vector<T> y(a.size(), T(0));
    for (int i = 0; i < int(a.size()); ++i) {
        for (int j = 0; j < int(x.size()); ++j) { y[i] += a[i][j] * x[j]; }}
    return y;}
template<typename T> Mat<T> lowRank(int n, int m, int r) { return mul(randMat<T>(n, r), randMat<T>(r, m), m); }
template<typename T> Mat<T> genMat(int n, int m) {
    int kind = rnd(0, 3);
    if (kind == 0) { return lowRank<T>(n, m, rnd(0, min(n, m))); }
    Mat<T> a = randMat<T>(n, m, std::array{0, 30, 60, 90}[rnd(0, 3)]);
    for (int i = 1; kind == 1 && i < n; ++i) {
        if (rnd(0, 2)) { continue; }
        T f = relem<T>();
        int s = rnd(0, i - 1);
        for (int j = 0; j < m; ++j) { a[i][j] = f * a[s][j]; }}
    return a;}

// Oracles: permutation expansion, own Gauss-Jordan, brute force over GF(p).
template<typename T> T leibniz(const Mat<T> &a) {
    int n = int(a.size());
    vector<int> p(n);
    iota(p.begin(), p.end(), 0);
    T res = T(0);
    for (bool more = true; more; more = std::next_permutation(p.begin(), p.end())) {
        T t = T(1);
        int inv = 0;
        for (int i = 0; i < n; ++i) {
            t = t * a[i][p[i]];
            for (int j = i + 1; j < n; ++j) { inv += p[i] > p[j]; }}
        res = inv % 2 ? res - t : res + t;}
    return res;}
// Product of the rows' absolute sums: bounds |det| and scales the rounding error of floating elimination.
template<typename T> T rowNormProduct(const Mat<T> &a) {
    T res = T(1);
    for (auto &row : a) {
        T s = T(0);
        for (auto x : row) { s += abs(x); }
        res *= s;}
    return res;}
template<typename T> int gauss(Mat<T> &a, int m, T &det) {
    int n = int(a.size()), r = 0;
    det = T(1);
    for (int c = 0; c < m; ++c) {
        int p = r;
        while (p < n && a[p][c] == T(0)) { ++p; }
        if (p == n) { det = T(0); continue; }
        if (p != r) { swap(a[p], a[r]); det = T(0) - det; }
        det = det * a[r][c];
        T inv = T(1) / a[r][c];
        for (auto &x : a[r]) { x = x * inv; }
        for (int i = 0; i < n; ++i) {
            T f = a[i][c];
            if (i == r || f == T(0)) { continue; }
            for (int j = 0; j < int(a[i].size()); ++j) { a[i][j] = a[i][j] - f * a[r][j]; }}
        ++r;}
    return r;}
template<typename T> int ownRank(Mat<T> a, int m) {
    T det;
    return gauss(a, m, det);}
template<typename T> T ownDet(Mat<T> a) {
    T det;
    gauss(a, int(a.size()), det);
    return det;}
template<typename T> bool ownInverse(const Mat<T> &a, Mat<T> &out) {
    int n = int(a.size());
    Mat<T> e = a;
    for (int i = 0; i < n; ++i) {
        e[i].resize(2 * n, T(0));
        e[i][n + i] = T(1);}
    T det;
    if (gauss(e, n, det) < n) { return false; }
    out.assign(n, vector<T>());
    for (int i = 0; i < n; ++i) { out[i].assign(e[i].begin() + n, e[i].end()); }
    return true;}
template<typename T> Mat<T> minorOf(const Mat<T> &a, int r, int c) {
    Mat<T> res;
    for (int i = 0; i < int(a.size()); ++i) {
        if (i == r) { continue; }
        res.push_back(a[i]);
        res.back().erase(res.back().begin() + c);}
    return res;}
template<typename T> Mat<T> cofactorAdjugate(const Mat<T> &a) {
    int n = int(a.size());
    Mat<T> res(n, vector<T>(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            T d = ownDet(minorOf(a, i, j));
            res[j][i] = (i + j) % 2 ? T(0) - d : d;}}
    return res;}
// Every leading principal minor of order 1..n is nonzero iff elimination without pivoting never meets a zero pivot.
template<typename T> bool leadingNonzero(Mat<T> a) {
    int n = int(a.size());
    for (int k = 0; k < n; ++k) {
        if (a[k][k] == T(0)) { return false; }
        for (int i = k + 1; i < n; ++i) {
            T f = a[i][k] / a[k][k];
            for (int j = k; j < n; ++j) { a[i][j] -= f * a[k][j]; }}}
    return true;}
lng ipow(lng b, int e) {
    lng r = 1;
    while (e-- > 0) { r *= b; }
    return r;}
template<typename T> lng code(const vector<T> &v) {
    lng c = 0;
    for (int i = int(v.size()) - 1; i >= 0; --i) { c = c * lng(T::mod()) + lng(v[i].val()); }
    return c;}
template<typename T> vector<T> decode(lng c, int m) {
    vector<T> v(m);
    for (auto &x : v) { x = T(c % lng(T::mod())); c /= lng(T::mod()); }
    return v;}
// Codes of base + span(rows) over GF(p).
template<typename T> std::set<lng> spanOf(const Mat<T> &rows, vector<T> base) {
    std::set<lng> s;
    lng p = lng(T::mod()), total = ipow(p, int(rows.size()));
    for (lng t = 0; t < total; ++t) {
        vector<T> v = base;
        lng u = t;
        for (int i = 0; i < int(rows.size()); ++i, u /= p) {
            T c = T(u % p);
            for (int j = 0; j < int(v.size()); ++j) { v[j] += c * rows[i][j]; }}
        s.insert(code(v));}
    return s;}
template<typename T> std::set<lng> bruteSolutions(const Mat<T> &a, const vector<T> &b, int m) {
    std::set<lng> s;
    lng total = ipow(lng(T::mod()), m);
    for (lng c = 0; c < total; ++c) {
        if (mulVec(a, decode<T>(c, m)) == b) { s.insert(c); }}
    return s;}
template<typename T> bool isRref(const Mat<T> &e, const vector<int> &piv, int m) {
    int n = int(e.size()), r = int(piv.size());
    if (r > n) { return false; }
    for (int i = 0; i < n; ++i) {
        if (int(e[i].size()) != m) { return false; }
        for (int j = 0; j < m; ++j) {
            bool lead = i < r && j == piv[i];
            if (lead && e[i][j] != T(1)) { return false; }
            if (!lead && (i >= r || j < piv[i]) && e[i][j] != T(0)) { return false; }}}
    for (int i = 0; i < r; ++i) {
        if (piv[i] < 0 || piv[i] >= m || (i && piv[i] <= piv[i - 1])) { return false; }
        for (int k = 0; k < n; ++k) {
            if (k != i && e[k][piv[i]] != T(0)) { return false; }}}
    return true;}
// res must be a particular solution plus an independent kernel basis of size m - rank.
template<typename T> void checkSolution(const Mat<T> &a, const vector<T> &b, const Mat<T> &res, int m, int rk, bool consistent, bool brute, const string &what) {
    auto msg = [&] { return what + " a=" + show(a) + " b=" + show(b) + " got=" + show(res) + " rank=" + std::to_string(rk); };
    check(res.empty() != consistent, msg);
    if (res.empty()) { return; }
    check(int(res.size()) == m - rk + 1, msg);
    for (auto &x : res) { check(int(x.size()) == m, msg); }
    Mat<T> basis(res.begin() + 1, res.end());
    check(mulVec(a, res[0]) == b, msg);
    for (auto &x : basis) { check(mulVec(a, x) == vector<T>(a.size(), T(0)), msg); }
    check(ownRank(basis, m) == m - rk, msg);
    if (brute) { check(spanOf(basis, res[0]) == bruteSolutions(a, b, m), msg); }}

void validateOracles() {
    check(leibniz(Mat<lll>{{1, 2}, {3, 4}}) == -2 && leibniz(Mat<lll>{}) == 1 && leibniz(Mat<lll>{{0, 1, 0}, {0, 0, 1}, {1, 0, 0}}) == 1, [] { return string("oracle leibniz fixtures"); });
    for (int it = 0; it < reps(300); ++it) {
        int n = rnd(0, 5);
        auto a = genMat<F3>(n, n);
        auto b = genMat<mint>(n, n);
        check(ownDet(a) == leibniz(a) && ownDet(b) == leibniz(b), [&] { return "oracle ownDet vs leibniz a=" + show(a) + " b=" + show(b); });
        check(lng(spanOf(a, vector<F3>(n)).size()) == ipow(3, ownRank(a, n)), [&] { return "oracle ownRank vs span a=" + show(a); });
        bool lead = true;
        for (int k = 1; k <= n; ++k) {
            Mat<F3> s(k);
            for (int i = 0; i < k; ++i) { s[i].assign(a[i].begin(), a[i].begin() + k); }
            lead = lead && leibniz(s) != F3(0);}
        check(leadingNonzero(a) == lead, [&] { return "oracle leadingNonzero a=" + show(a); });}
    cout << "PASS oracle validation: Leibniz fixtures, own Gauss vs Leibniz and GF(3) spans, no-pivot elimination vs minors\n";}

void productSuite() {
    auto run = [&]<typename T>(T, const string &name) {
        for (int it = 0; it < reps(200); ++it) {
            int n = rnd(0, 6), m = rnd(0, 6), k = m ? rnd(0, 6) : 0;
            auto a = randMat<T>(n, m, rnd(0, 60)), b = randMat<T>(m, k, rnd(0, 60));
            check(matrixProduct(a, b) == mul(a, b, k), [&] { return "matrixProduct " + name + " a=" + show(a) + " b=" + show(b); });
            check(identityMatrix<T>(n) == eye<T>(n) && matrixProduct(identityMatrix<T>(n), a) == a, [&] { return "identityMatrix " + name + " n=" + std::to_string(n); });}};
    run(mint(), "998244353"); run(Z12(), "Z/12"); run(ulng(), "ulng mod 2^64"); run(double(), "double");
    check(matrixProduct(Mat<mint>(3, vector<mint>()), Mat<mint>{}) == Mat<mint>(3, vector<mint>()), [] { return string("matrixProduct 3 x 0 times 0 x 0"); });
    cout << "PASS identityMatrix, matrixProduct: mint, Z/12, ulng, double, empty shapes\n";}

// rref, matrixRank, solveLinear, nullspace, solveLinearMultiRhs, determinant, inverse, adjugate.
template<typename T> void exactSuite(const string &name, int maxd, int rounds, bool brute) {
    for (int it = 0; it < rounds; ++it) {
        int n = rnd(0, maxd), m = n ? (rnd(0, 2) ? rnd(0, maxd) : n) : 0;
        auto a = genMat<T>(n, m);
        string in = name + " a=" + show(a);
        int rk = ownRank(a, m);
        auto e = a;
        auto piv = rref(e);
        check(isRref(e, piv, m) && int(piv.size()) == rk, [&] { return "rref " + in + " got=" + show(e) + " piv=" + show(piv) + " rank=" + std::to_string(rk); });
        bool same = brute ? spanOf(a, vector<T>(m)) == spanOf(e, vector<T>(m)) : ownRank(a, m) == ownRank(e, m) && [&] { auto s = a; s.insert(s.end(), e.begin(), e.end()); return ownRank(s, m) == rk; }();
        check(same, [&] { return "rref row space " + in + " got=" + show(e); });
        check(matrixRank(a) == rk, [&] { return "matrixRank " + in + " expected=" + std::to_string(rk); });
        vector<T> b = rnd(0, 1) ? mulVec(a, rvec<T>(m)) : rvec<T>(n, rnd(0, 50));
        auto ab = a;
        for (int i = 0; i < n; ++i) { ab[i].push_back(b[i]); }
        bool consistent = ownRank(ab, m + 1) == rk;
        checkSolution(a, b, solveLinear(a, b), m, rk, consistent, brute, "solveLinear " + name);
        auto ns = nullspace(a);
        check(ns.empty() == (rk == m), [&] { return "nullspace emptiness " + in; });
        ns.insert(ns.begin(), vector<T>(m, T(0)));
        checkSolution(a, vector<T>(n, T(0)), ns, m, rk, true, brute, "nullspace " + name);
        int k = rnd(0, 3);
        Mat<T> bs(n, vector<T>(k)), x{{T(1)}};
        bool all = true;
        for (int j = 0; j < k; ++j) {
            auto col = rnd(0, 2) ? mulVec(a, rvec<T>(m)) : rvec<T>(n, rnd(0, 50));
            auto t = a;
            for (int i = 0; i < n; ++i) { bs[i][j] = col[i]; t[i].push_back(col[i]); }
            all = all && ownRank(t, m + 1) == rk;}
        bool ok = solveLinearMultiRhs(a, bs, x);
        check(ok == all && (ok ? int(x.size()) == m && mul(a, x, k) == bs : x == Mat<T>{{T(1)}}), [&] { return "solveLinearMultiRhs " + in + " b=" + show(bs) + " got=" + show(x) + " ok=" + std::to_string(ok); });
        if (n != m) { continue; }
        T d = n <= 7 ? leibniz(a) : ownDet(a);
        check(determinant(a) == d, [&] { return "determinant " + in + " expected=" + show(d) + " got=" + show(determinant(a)); });
        auto inv = a;
        ok = inverse(inv);
        check(ok == (d != T(0)) && (!ok || (mul(a, inv, n) == eye<T>(n) && mul(inv, a, n) == eye<T>(n))), [&] { return "inverse " + in + " got=" + show(inv) + " ok=" + std::to_string(ok); });
        for (auto &s : {a, lowRank<T>(n, n, max(0, n - 1)), lowRank<T>(n, n, rnd(0, max(0, n - 2)))}) {
            check(adjugate(s) == cofactorAdjugate(s), [&] { return "adjugate " + name + " a=" + show(s) + " expected=" + show(cofactorAdjugate(s)) + " got=" + show(adjugate(s)); });}}
    cout << "PASS rref, matrixRank, solveLinear, nullspace, solveLinearMultiRhs, determinant, inverse, adjugate over " << name << (brute ? " (brute-force spans and solution sets)" : " (own Gauss ranks, exact residuals)") << ", dims <= " << maxd << ", " << rounds << " cases\n";}

// determinantMod and inverseMod.
lng nrm(lll x, lng m) {
    lll r = x % m;
    return lng(r < 0 ? r + m : r);}
lng mulm(lng a, lng b, lng m) { return lng(ulll(nrm(a, m)) * ulll(nrm(b, m)) % ulll(m)); }
lng leibnizMod(const Mat<lng> &a, lng m) {
    int n = int(a.size());
    vector<int> p(n);
    iota(p.begin(), p.end(), 0);
    lng res = 0;
    for (bool more = true; more; more = std::next_permutation(p.begin(), p.end())) {
        lng t = 1 % m;
        int inv = 0;
        for (int i = 0; i < n; ++i) {
            t = mulm(t, a[i][p[i]], m);
            for (int j = i + 1; j < n; ++j) { inv += p[i] > p[j]; }}
        res = nrm(inv % 2 ? lll(res) - t : lll(res) + t, m);}
    return res;}
Mat<lng> mulMod(const Mat<lng> &a, const Mat<lng> &b, lng m) {
    int n = int(a.size());
    Mat<lng> c(n, vector<lng>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int t = 0; t < n; ++t) { c[i][j] = nrm(lll(c[i][j]) + mulm(a[i][t], b[t][j], m), m); }}}
    return c;}
vector<lng> smallDivisors(lng m) {
    vector<lng> d;
    for (lng x = 1; x <= 1000 && x <= m; ++x) {
        if (m % x == 0) { d.push_back(x); }}
    d.push_back(m);
    return d;}
lng rmod(lng m) { return lng(rng() % ulng(m)); }
// Any lng representative of x mod m.
lng spread(lng x, lng m) { return rnd(0, 1) && x >= INT64_MIN + m ? x - m : x; }
Mat<lng> genModMat(int n, lng m) {
    auto divs = smallDivisors(m);
    int kind = rnd(0, 3);
    Mat<lng> a(n, vector<lng>(n));
    vector<lng> col(n);
    for (auto &d : col) { d = divs[rnd(0, int(divs.size()) - 1)]; }
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            lng &x = a[i][j];
            if (kind == 0) { x = rnd(0, 9) ? lng(rng()) : std::array{INT64_MIN, INT64_MAX, lng(0), lng(-1)}[rnd(0, 3)]; }
            if (kind == 1) { x = lng(lll(col[j]) * rnd(-50, 50) % m); }
            if (kind == 2) { x = rnd(-3, 3); }
            if (kind == 3) { x = i && rnd(0, 1) ? lng(lll(a[i - 1][j]) * 2 % m) : spread(rmod(m), m); }}}
    return a;}
// A = P L U mod m with L unit lower and U upper: det = sign(P) * prod U[i][i].
Mat<lng> knownDet(int n, lng m, lng &det) {
    auto divs = smallDivisors(m);
    Mat<lng> l(n, vector<lng>(n, 0)), u = l;
    det = 1 % m;
    for (int i = 0; i < n; ++i) {
        l[i][i] = 1 % m;
        for (int j = 0; j < i; ++j) { l[i][j] = rnd(0, 2) ? rmod(m) : 0; }
        int c = rnd(0, 9);
        u[i][i] = c < 5 ? rmod(m) : c < 9 ? divs[rnd(0, int(divs.size()) - 1)] % m : 0;
        for (int j = i + 1; j < n; ++j) { u[i][j] = rmod(m); }
        det = mulm(det, u[i][i], m);}
    auto a = mulMod(l, u, m);
    for (int i = n - 1; i > 0; --i) {
        int j = rnd(0, i);
        if (j != i) { swap(a[i], a[j]); det = nrm(-lll(det), m); }}
    for (auto &row : a) { for (auto &x : row) { x = spread(x, m); } }
    return a;}
template<typename T> void primeCross(lng m) {
    for (int it = 0; it < reps(30); ++it) {
        int n = rnd(0, quick ? 8 : 16);
        auto a = genModMat(n, m);
        Mat<T> b(n, vector<T>(n));
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { b[i][j] = T(nrm(a[i][j], m)); } }
        check(determinantMod(a, m) == lng(determinant(b).val()), [&] { return "determinantMod vs modint determinant mod=" + std::to_string(m) + " a=" + show(a); });}}
void modSuite() {
    const lng p1 = 998244353, p2 = 1000000007;
    vector<lng> mods = {1, 2, 3, 4, 6, 8, 12, 30, 36, 64, 210, 720720, 1 << 30, p1, p2, 4294967291LL, 1LL << 32, (1LL << 32) + 1, 3 * p2,
                        1000000000000000000LL, 1LL << 62, (1LL << 62) + 135, 2305843009213693951LL, 4179340454199820289LL, INT64_MAX, 3 * p1 * p2, 6 * p2 * p2};
    for (lng m : mods) {
        auto ms = std::to_string(m);
        for (int it = 0; it < reps(40); ++it) {
            int n = rnd(0, quick ? 5 : 6);
            auto a = genModMat(n, m);
            lng want = leibnizMod(a, m), got = determinantMod(a, m);
            check(got == want, [&] { return "determinantMod mod=" + ms + " a=" + show(a) + " expected=" + show(want) + " got=" + show(got); });
            auto x = a;
            bool ok = inverseMod(x, m);
            bool unit = gcd64(want, m) == 1;
            auto msg = [&] { return "inverseMod mod=" + ms + " a=" + show(a) + " det=" + show(want) + " ok=" + std::to_string(ok) + " got=" + show(x); };
            check(ok == unit, msg);
            if (!ok) { continue; }
            for (auto &row : x) { for (lng v : row) { check(int(row.size()) == n && 0 <= v && v < m, msg); } }
            check(mulMod(a, x, m) == mulMod(eye<lng>(n), eye<lng>(n), m) && mulMod(x, a, m) == mulMod(eye<lng>(n), eye<lng>(n), m), msg);}
        for (int it = 0; it < reps(15); ++it) {
            int n = rnd(7, quick ? 10 : 24);
            lng want;
            auto a = knownDet(n, m, want);
            check(determinantMod(a, m) == want, [&] { return "determinantMod (P L U) mod=" + ms + " a=" + show(a) + " expected=" + show(want) + " got=" + show(determinantMod(a, m)); });
            auto x = a;
            bool ok = inverseMod(x, m);
            check(ok == (gcd64(want, m) == 1) && (!ok || mulMod(a, x, m) == mulMod(eye<lng>(n), eye<lng>(n), m)), [&] { return "inverseMod (P L U) mod=" + ms + " a=" + show(a) + " det=" + show(want); });}}
    for (auto [m, d] : vector<pair<lng, lng>>{{12, 4}, {12, 3}, {720720, 16}, {3 * p2, p2}, {INT64_MAX, 7}, {1000000000000000000LL, 1 << 18}, {6 * p2 * p2, p2 * p2}}) {
        for (int it = 0; it < reps(10); ++it) {
            auto a = genModMat(rnd(0, 12), m);
            check(determinantMod(a, m) % d == determinantMod(a, d), [&] { return "determinantMod divisor mod=" + std::to_string(m) + " d=" + std::to_string(d) + " a=" + show(a); });}}
    primeCross<F2>(2); primeCross<F3>(3); primeCross<mint>(p1); primeCross<MP>(p2);
    primeCross<P61>(2305843009213693951LL); primeCross<P62>(4179340454199820289LL);
    cout << "PASS determinantMod, inverseMod: " << mods.size() << " moduli in [1, 2^63) against Leibniz (n <= 6, all lng entries, unit-free columns) and P L U (n <= 24); divisor and prime-field cross-checks\n";}

// determinantInteger (Bareiss) against Leibniz in lll.
template<typename T> void integerSuite(const string &name, int maxn, int bound) {
    for (int it = 0; it < reps(200); ++it) {
        int n = rnd(0, maxn), kind = rnd(0, 3);
        Mat<T> a(n, vector<T>(n));
        for (auto &row : a) { for (auto &x : row) { x = rnd(0, 2) ? T(rnd(-bound, bound)) : T(0); } }
        if (kind == 1 && n) {
            int c = rnd(0, n - 1);
            for (int i = 0; i < n; ++i) { (rnd(0, 1) ? a[c][i] : a[i][c]) = T(0); }}
        if (kind == 2) {
            for (int i = 0; i < n; ++i) { for (int j = 0; j < i; ++j) { a[i][j] = T(0); } }
            std::shuffle(a.begin(), a.end(), rng);}
        if (kind == 3 && n > 1) { a[0][0] = T(0); a[1] = a[0]; a[1][0] = T(1); }
        Mat<lll> w(n, vector<lll>(n));
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { w[i][j] = lll(a[i][j]); } }
        lll want = leibniz(w), got = lll(determinantInteger(a));
        check(got == want, [&] { return "determinantInteger " + name + " a=" + show(w) + " expected=" + str(want) + " got=" + str(got); });}
    cout << "PASS determinantInteger<" << name << ">: n <= " << maxn << ", |entries| <= " << bound << ", zero rows/columns, row swaps\n";}
// determinantInteger at the documented limit: the product of any two minors fits T.
template<typename T> void integerBoundary(const string &name, int n, lll bound) {
    for (int it = 0; it < reps(400); ++it) {
        Mat<T> a(n, vector<T>(n));
        for (auto &row : a) { for (auto &x : row) { x = rnd(0, 3) ? T(lll(rng() % ulng(2 * bound + 1)) - bound) : T(rnd(0, 1) ? bound : -bound); } }
        if (rnd(0, 3) == 0) { a[0][0] = T(0); }
        Mat<lll> w(n, vector<lll>(n));
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { w[i][j] = lll(a[i][j]); } }
        lll want = leibniz(w), got = lll(determinantInteger(a));
        check(got == want, [&] { return "determinantInteger boundary " + name + " a=" + show(w) + " expected=" + str(want) + " got=" + str(got); });}
    cout << "PASS determinantInteger<" << name << "> limit: n = " << n << ", |entries| <= " << str(bound) << "\n";}
// Outputs that share storage with inputs.
void aliasSuite() {
    using M = mint;
    for (int it = 0; it < reps(200); ++it) {
        int n = rnd(1, 8), k = rnd(0, 3);
        Mat<M> a = genMat<M>(n, n), b = genMat<M>(n, k), x, bb = b;
        bool ok1 = solveLinearMultiRhs(a, b, x), ok2 = solveLinearMultiRhs(a, bb, bb);
        check(ok1 == ok2 && (!ok1 || x == bb) && (ok1 || bb == b), [&] { return "solveLinearMultiRhs aliasing x = b a=" + show(a) + " b=" + show(b); });
        vector<M> v(n), d, w;
        for (auto &e : v) { e = M(rnd(0, 1000)); }
        for (int cyc = 0; cyc < 2; ++cyc) {
            d = v; w = v;
            bool r1 = cyc ? cyclicTridiagonalSolve(v, v, v, d) : tridiagonalSolve(v, v, v, d);
            bool r2 = cyc ? cyclicTridiagonalSolve(w, w, w, w) : tridiagonalSolve(w, w, w, w);
            check(r1 == r2 && (!r1 || d == w), [&] { return string(cyc ? "cyclicTridiagonalSolve" : "tridiagonalSolve") + " aliasing a = b = c = d v=" + show(Mat<M>{v}); });}
        Mat<M> ai = a, u = a, vv = a, aj = a;
        bool q1 = lowRankUpdateInverse(ai, u, vv), q2 = lowRankUpdateInverse(aj, aj, aj);
        check(q1 == q2 && ai == aj, [&] { return "lowRankUpdateInverse aliasing ainv = u = v a=" + show(a); });}
    cout << "PASS aliasing: solveLinearMultiRhs(a, b, b), tridiagonal and cyclic with a = b = c = d, lowRankUpdateInverse(a, a, a)\n";}
void integerKnown() {
    for (int it = 0; it < reps(100); ++it) {
        int n = rnd(0, 10);
        Mat<lll> l(n, vector<lll>(n, 0)), u = l;
        lll want = 1;
        for (int i = 0; i < n; ++i) {
            l[i][i] = 1;
            u[i][i] = rnd(-2, 2);
            want *= u[i][i];
            for (int j = 0; j < i; ++j) { l[i][j] = rnd(-1, 1); u[j][i] = rnd(-1, 1); }}
        auto a = mul(l, u, n);
        for (int i = n - 1; i > 0; --i) {
            int j = rnd(0, i);
            if (j != i) { swap(a[i], a[j]); want = -want; }}
        check(determinantInteger(a) == want, [&] { return "determinantInteger<lll> (P L U) a=" + show(a) + " expected=" + str(want); });}
    cout << "PASS determinantInteger<lll>: P L U products n <= 10 with known determinant\n";}

// matrixPower and matrixPowerSum against repeated products and the block matrix [[A, I], [0, I]].
template<typename T> pair<Mat<T>, Mat<T>> blockPower(const Mat<T> &a, ulng e) {
    int n = int(a.size());
    Mat<T> b(2 * n, vector<T>(2 * n, T(0))), r = eye<T>(2 * n), p(n), s(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) { b[i][j] = a[i][j]; }
        b[i][n + i] = b[n + i][n + i] = T(1);}
    for (; e; e >>= 1) {
        if (e & 1) { r = mul(r, b, 2 * n); }
        b = mul(b, b, 2 * n);}
    for (int i = 0; i < n; ++i) {
        p[i].assign(r[i].begin(), r[i].begin() + n);
        s[i].assign(r[i].begin() + n, r[i].end());}
    return {p, s};}
template<typename T> void powerSuite(const string &name) {
    for (int it = 0; it < reps(60); ++it) {
        int n = rnd(0, 5), kind = rnd(0, 4);
        Mat<T> a = kind == 0 ? eye<T>(n) : kind == 1 ? Mat<T>(n, vector<T>(n, T(0))) : genMat<T>(n, n);
        for (int i = 0; kind == 2 && i < n; ++i) { for (int j = 0; j <= i; ++j) { a[i][j] = T(0); } }
        Mat<T> p = eye<T>(n), s(n, vector<T>(n, T(0)));
        for (ulng e = 0; e <= 20; ++e) {
            check(matrixPower(a, e) == p && matrixPowerSum(a, e) == s, [&] { return "matrixPower/Sum " + name + " a=" + show(a) + " e=" + std::to_string(e); });
            for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { s[i][j] += p[i][j]; } }
            p = mul(p, a, n);}
        int k = rnd(5, 63);
        for (ulng e : {ulng(1) << k, (ulng(1) << k) - 1, (ulng(1) << k) + 1, ulng(1000000000000000000ULL), ~ulng(0), ulng(rng())}) {
            auto [q, t] = blockPower(a, e);
            check(matrixPower(a, e) == q && matrixPowerSum(a, e) == t, [&] { return "matrixPower/Sum (block) " + name + " a=" + show(a) + " e=" + std::to_string(e); });}}
    cout << "PASS matrixPower, matrixPowerSum over " << name << ": e <= 20 against repeated products, e up to 2^64 - 1 against [[A, I], [0, I]]^e\n";}

// tridiagonalSolve, cyclicTridiagonalSolve, bandedSolve against dense matrices.
template<typename T> bool residualOk(const Mat<T> &a, const vector<T> &x, const vector<T> &d) {
    for (int i = 0; i < int(a.size()); ++i) {
        T s = T(0), scale = T(0);
        for (int j = 0; j < int(x.size()); ++j) { s += a[i][j] * x[j]; scale += abs(a[i][j] * x[j]); }
        if (!(abs(s - d[i]) <= 1e-11 * (scale + abs(d[i])) + 1e-300)) { return false; }}
    return true;}
// Makes a strictly diagonally dominant with margin >= 1; diag[i] receives the amount added to a[i][i].
void dominate(Mat<double> &a, vector<double> &diag) {
    for (int i = 0; i < int(a.size()); ++i) {
        double s = 0;
        for (int j = 0; j < int(a.size()); ++j) { s += j == i ? 0 : abs(a[i][j]); }
        double dd = (s + 1 + abs(relem<double>())) * (rnd(0, 1) ? 1 : -1);
        diag[i] = dd - a[i][i];
        a[i][i] = dd;}}
template<typename T> void bandSuite(const string &name, int rounds) {
    constexpr bool real = std::is_floating_point_v<T>;
    for (int it = 0; it < rounds; ++it) {
        int n = rnd(0, quick ? 12 : 30), zero = rnd(0, 50);
        auto a = rvec<T>(n, zero), b = rvec<T>(n, zero), c = rvec<T>(n, zero), d = rvec<T>(n);
        Mat<T> t(n, vector<T>(n, T(0))), cy = t;
        for (int i = 0; i < n; ++i) {
            if (i) { t[i][i - 1] = a[i]; }
            if (i + 1 < n) { t[i][i + 1] = c[i]; }
            cy[i][(i + n - 1) % n] += a[i];
            cy[i][(i + 1) % n] += c[i];}
        auto bt = b, bc = b;
        if constexpr (real) { dominate(t, bt); dominate(cy, bc); }
        else {
            for (int i = 0; i < n; ++i) { t[i][i] = b[i]; cy[i][i] += b[i]; }}
        auto x = d;
        bool ok = tridiagonalSolve(a, bt, c, x);
        auto in = name + " a=" + show(a) + " b=" + show(bt) + " c=" + show(c) + " d=" + show(d);
        if constexpr (real) { check(ok && residualOk(t, x, d), [&] { return "tridiagonalSolve " + in + " got=" + show(x); }); }
        else { check(ok == leadingNonzero(t) && (!ok || mulVec(t, x) == d), [&] { return "tridiagonalSolve " + in + " got=" + show(x) + " ok=" + std::to_string(ok); }); }
        x = d;
        ok = cyclicTridiagonalSolve(a, bc, c, x);
        in = name + " cyclic a=" + show(a) + " b=" + show(bc) + " c=" + show(c) + " d=" + show(d);
        if constexpr (real) { check(ok && residualOk(cy, x, d), [&] { return "cyclicTridiagonalSolve " + in + " got=" + show(x); }); }
        else { check(ok == leadingNonzero(cy) && (!ok || mulVec(cy, x) == d), [&] { return "cyclicTridiagonalSolve " + in + " got=" + show(x) + " ok=" + std::to_string(ok); }); }
        int p = rnd(0, 3), q = rnd(0, 3);
        Mat<T> band = randMat<T>(n, p + q + 1, zero), dense(n, vector<T>(n, T(0)));
        for (int i = 0; i < n; ++i) {
            for (int j = max(0, i - p); j <= min(n - 1, i + q); ++j) { dense[i][j] = band[i][j - i + p]; }}
        if constexpr (real) {
            vector<T> dg(n);
            for (int i = 0; i < n; ++i) { dense[i][i] = T(0); }
            dominate(dense, dg);
            for (int i = 0; i < n; ++i) { band[i][p] = dg[i]; }}
        x = d;
        ok = bandedSolve(band, p, q, x);
        in = name + " p=" + std::to_string(p) + " q=" + std::to_string(q) + " band=" + show(band) + " d=" + show(d);
        if constexpr (real) { check(ok && residualOk(dense, x, d), [&] { return "bandedSolve " + in + " got=" + show(x); }); }
        else { check(ok == leadingNonzero(dense) && (!ok || mulVec(dense, x) == d), [&] { return "bandedSolve " + in + " got=" + show(x) + " ok=" + std::to_string(ok); }); }}
    cout << "PASS tridiagonalSolve, cyclicTridiagonalSolve, bandedSolve (p, q <= 3) over " << name << ", n <= " << (quick ? 12 : 30) << (real ? ", diagonally dominant residuals" : ", success iff all leading minors nonzero, exact A x = d") << "\n";}

// Floating solvers.
int realRank(Mat<double> a, int m) {
    int n = int(a.size()), r = 0;
    for (int c = 0; c < m && r < n; ++c) {
        int p = r;
        for (int i = r; i < n; ++i) { if (abs(a[i][c]) > abs(a[p][c])) { p = i; } }
        if (abs(a[p][c]) <= 1e-7) { continue; }
        swap(a[p], a[r]);
        for (int i = r + 1; i < n; ++i) {
            double f = a[i][c] / a[r][c];
            for (int j = c; j < m; ++j) { a[i][j] -= f * a[r][j]; }}
        ++r;}
    return r;}
void realSuite() {
    using R = double;
    for (int it = 0; it < reps(200); ++it) {
        int n = rnd(0, 7);
        auto a = genMat<R>(n, n);
        R want = leibniz(a), got = determinantReal(a);
        check(abs(got - want) <= 1e-11 * rowNormProduct(a), [&] { return "determinantReal a=" + show(a) + " expected=" + show(want) + " got=" + show(got); });
        n = rnd(0, 10);
        int m = n ? rnd(0, 10) : 0, r = rnd(0, min(n, m));
        a = lowRank<R>(n, m, r);
        int rk = matrixRankReal(a);
        check(rk == r, [&] { return "matrixRankReal a=" + show(a) + " expected=" + std::to_string(r) + " got=" + std::to_string(rk); });
        bool consistent = r == n || rnd(0, 1);
        auto b = consistent ? mulVec(a, rvec<R>(m)) : rvec<R>(n);
        auto res = solveLinearReal(a, b);
        auto msg = [&] { return "solveLinearReal a=" + show(a) + " b=" + show(b) + " got=" + show(res) + " rank=" + std::to_string(r); };
        check(res.empty() != consistent, msg);
        if (!res.empty()) {
            check(int(res.size()) == m - r + 1, msg);
            for (auto &x : res) { check(int(x.size()) == m, msg); }
            check(residualOk(a, res[0], b), msg);
            for (int i = 1; i < int(res.size()); ++i) { check(residualOk(a, res[i], vector<R>(n, 0)), msg); }
            check(realRank(Mat<R>(res.begin() + 1, res.end()), m) == m - r, msg);}
        n = rnd(0, quick ? 12 : 30);
        a = randMat<R>(n, n);
        if (rnd(0, 1)) {
            for (int i = 0; i < n; ++i) { a[i][i] = R(n + 1) * (rnd(0, 1) ? 1 : -1); }}
        auto lu = a;
        vector<int> perm;
        bool ok = luDecompose(lu, perm);
        auto lmsg = [&] { return "luDecompose a=" + show(a) + " got=" + show(lu) + " perm=" + show(perm); };
        auto sorted = perm;
        sort(sorted.begin(), sorted.end());
        check(ok && int(perm.size()) == n && [&] { for (int i = 0; i < n; ++i) { if (sorted[i] != i) { return false; } } return true; }(), lmsg);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                R s = 0, scale = 0;
                for (int k = 0; k <= min(i, j); ++k) {
                    R u = k == j ? 1 : lu[k][j];
                    s += lu[i][k] * u;
                    scale += abs(lu[i][k] * u);}
                check(abs(s - a[perm[i]][j]) <= 1e-13 * n * scale + 1e-300, lmsg);}}
        b = rvec<R>(n);
        auto x = luSolve(lu, perm, b);
        check(residualOk(a, x, b), [&] { return "luSolve a=" + show(a) + " b=" + show(b) + " got=" + show(x); });
        if (n) {
            int i = rnd(0, n - 1), j = rnd(0, n - 1), kind = rnd(0, 2);
            for (int k = 0; k < n; ++k) {
                if (kind == 0) { a[i][k] = 0; }
                if (kind == 1) { a[k][j] = 0; }}
            if (kind == 2 && n > 1) { a[i] = a[(i + 1) % n]; }
            if (kind < 2 || n > 1) {
                lu = a;
                check(!luDecompose(lu, perm), [&] { return "luDecompose singular a=" + show(a); });}
            lu = randMat<R>(n, n);
            check(!luDecompose(lu, perm, 2.0), [&] { return string("luDecompose eps = 2"); });}}
    check(matrixRankReal(Mat<R>{{1e-12}}) == 0 && matrixRankReal(Mat<R>{{1e-12}}, 1e-15) == 1 && matrixRankReal(Mat<R>{{1, 0}, {0, 1e-6}}) == 2 && matrixRankReal(Mat<R>{}) == 0, [] { return string("matrixRankReal eps fixtures"); });
    check(determinantReal(Mat<R>{}) == 1 && determinantReal(Mat<R>{{0, 2}, {3, 0}}) == -6, [] { return string("determinantReal fixtures"); });
    check(solveLinearReal(Mat<R>{}, vector<R>{}) == Mat<R>{{}} && solveLinearReal(Mat<R>{{1e-12}}, vector<R>{1e-12}) == Mat<R>{{0}, {1}}, [] { return string("solveLinearReal empty and eps fixtures"); });
    Mat<long double> ld{{2, 1, 0}, {1, 3, 1}, {0, 1, 4}};
    vector<int> perm;
    check(abs(determinantReal(ld) - 18) < 1e-15L && luDecompose(ld, perm), [] { return string("long double instantiation"); });
    cout << "PASS determinantReal (Leibniz, tolerance 1e-11 * product of row sums), matrixRankReal (rank-r products), solveLinearReal (residuals, kernel), luDecompose/luSolve (reconstruction, residuals, singular and eps), n <= " << (quick ? 12 : 30) << "\n";}

// lowRankUpdateInverse against the own inverse of A + U V.
template<typename T> void lowRankSuite(const string &name) {
    for (int it = 0; it < reps(150); ++it) {
        int n = rnd(0, 8), k = rnd(0, 4);
        Mat<T> a, ainv;
        do { a = randMat<T>(n, n, rnd(0, 40)); } while (!ownInverse(a, ainv));
        Mat<T> u = genMat<T>(n, k), v = genMat<T>(k, n), w = a, want;
        auto uv = mul(u, v, n);
        for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { w[i][j] += uv[i][j]; } }
        bool expect = ownInverse(w, want);
        auto got = ainv;
        bool ok = lowRankUpdateInverse(got, u, v);
        check(ok == expect && got == (ok ? want : ainv), [&] { return "lowRankUpdateInverse " + name + " a=" + show(a) + " u=" + show(u) + " v=" + show(v) + " ok=" + std::to_string(ok) + " got=" + show(got) + " expected=" + show(want); });}
    cout << "PASS lowRankUpdateInverse over " << name << ": n <= 8, k <= 4 (k = 0 included), singular updates leave ainv untouched\n";}

// FieldBasis against a model list and brute spans or own ranks.
template<typename T> void basisSuite(const string &name, int maxm, bool brute) {
    for (int it = 0; it < reps(150); ++it) {
        int m = rnd(0, maxm);
        FieldBasis<T> fb(m);
        Mat<T> model;
        for (int op = rnd(1, 40); op-- > 0;) {
            int kind = rnd(0, 9);
            vector<T> v = rvec<T>(m, rnd(0, 80));
            if (rnd(0, 1) && !model.empty()) {
                v.assign(m, T(0));
                for (auto &row : model) {
                    T f = relem<T>(30);
                    for (int j = 0; j < m; ++j) { v[j] += f * row[j]; }}}
            bool in = brute ? spanOf(model, vector<T>(m)).count(code(v)) > 0 : [&] { auto s = model; s.push_back(v); return ownRank(s, m) == int(model.size()); }();
            string ctx = name + " m=" + std::to_string(m) + " model=" + show(model) + " v=" + show(v);
            if (kind < 5) {
                bool ok = fb.insert(v);
                check(ok == !in, [&] { return "FieldBasis::insert " + ctx + " got=" + std::to_string(ok); });
                if (ok) { model.push_back(v); }}
            else if (kind < 8) {
                vector<T> coef{T(7)};
                bool ok = fb.represent(v, coef);
                vector<T> sum(m, T(0));
                for (int i = 0; ok && i < int(coef.size()) && i < int(model.size()); ++i) {
                    for (int j = 0; j < m; ++j) { sum[j] += coef[i] * model[i][j]; }}
                check(ok == in && (!ok || (coef.size() == model.size() && sum == v)), [&] { return "FieldBasis::represent " + ctx + " got=" + std::to_string(ok) + " coef=" + show(coef); });}
            else {
                int r = rnd(0, int(model.size()));
                fb.rollback(r);
                model.resize(r);}
            check(fb.size() == int(model.size()), [&] { return "FieldBasis::size " + ctx; });}}
    cout << "PASS FieldBasis insert, represent, rollback, size over " << name << ", m <= " << maxm << (brute ? " (brute spans)" : " (own ranks)") << "\n";}

template<typename T> void freivaldsSuite(const string &name, int rounds) {
    int caught = 0, cases = 0;
    for (int it = 0; it < reps(200); ++it) {
        int n = rnd(0, 8), m = n ? rnd(0, 8) : 0, k = m ? rnd(0, 8) : 0;
        if (n && !m) { k = rnd(0, 8); }
        auto a = randMat<T>(n, m, rnd(0, 50)), b = randMat<T>(m, m ? k : 0, rnd(0, 50));
        auto c = mul(a, b, k);
        ulng s = rng();
        check(freivaldsProductCheck(a, b, c, rnd(0, 3), s) && freivaldsProductCheck(a, b, c), [&] { return "freivaldsProductCheck equal " + name + " a=" + show(a) + " b=" + show(b); });
        if (!n || !k) { continue; }
        c[rnd(0, n - 1)][rnd(0, k - 1)] += T(rnd(1, int(min<ulng>(T::mod() - 1, 1000))));
        bool got = freivaldsProductCheck(a, b, c, rounds, s);
        check(!got, [&] { return "freivaldsProductCheck perturbed " + name + " rounds=" + std::to_string(rounds) + " seed=" + std::to_string(s) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c); });
        bool once = freivaldsProductCheck(a, b, c, 1, s);
        check(once == freivaldsProductCheck(a, b, c, 1, s), [&] { return "freivaldsProductCheck seed reproducibility " + name; });
        caught += !once;
        ++cases;}
    if (T::mod() == 2 && cases >= 20) { check(0 < caught && caught < cases, [&] { return "freivaldsProductCheck GF(2) single-round detection rate " + std::to_string(caught) + "/" + std::to_string(cases); }); }
    cout << "PASS freivaldsProductCheck over " << name << ": true on products (any rounds, empty shapes, inner size 0), single-entry perturbations caught with " << rounds << " rounds, seed reproducible\n";}

void szSuite() {
    using T = mint;
    for (int n = 1; n <= 5; ++n) {
        auto build = [&](const vector<T> &x, int off, bool tr) {
            Mat<T> a(n, vector<T>(n));
            for (int i = 0; i < n; ++i) { for (int j = 0; j < n; ++j) { a[i][j] = tr ? x[off + j * n + i] : x[off + i * n + j]; } }
            return a;};
        auto zero1 = [&](const vector<T> &x) { return determinant(build(x, 0, false)) - determinant(build(x, 0, true)); };
        auto zero2 = [&](const vector<T> &x) {
            auto a = build(x, 0, false), b = build(x, n * n, false);
            return determinant(matrixProduct(a, b)) - determinant(a) * determinant(b);};
        auto nonzero = [&](const vector<T> &x) {
            auto a = build(x, 0, false);
            T d = T(1);
            for (int i = 0; i < n; ++i) { d *= a[i][i]; }
            return n == 1 ? a[0][0] : determinant(a) - d;};
        for (int r = 1; r <= 3; ++r) {
            ulng s = rng();
            check(schwartzZippelIdentityCheck<T>(zero1, n * n, r, s) && schwartzZippelIdentityCheck<T>(zero2, 2 * n * n, r, s) && !schwartzZippelIdentityCheck<T>(nonzero, n * n, r, s), [&] { return "schwartzZippelIdentityCheck determinant identities n=" + std::to_string(n) + " seed=" + std::to_string(s); });}}
    auto sq = [](const vector<T> &x) { return (x[0] + x[1]) * (x[0] + x[1]) - x[0] * x[0] - T(2) * x[0] * x[1] - x[1] * x[1]; };
    auto lin = [](const vector<T> &x) { return x[0] - x[1]; };
    vector<vector<T>> log1, log2;
    auto rec1 = [&](const vector<T> &x) { log1.push_back(x); return T(0); };
    auto rec2 = [&](const vector<T> &x) { log2.push_back(x); return T(0); };
    bool calls = schwartzZippelIdentityCheck<T>(rec1, 3, 0, 5);
    check(calls && log1.empty() && schwartzZippelIdentityCheck<T>(sq, 2) && !schwartzZippelIdentityCheck<T>(lin, 2) && schwartzZippelIdentityCheck<T>([](const vector<T> &) { return T(0); }, 0) && !schwartzZippelIdentityCheck<T>([](const vector<T> &) { return T(5); }, 0, 1), [] { return string("schwartzZippelIdentityCheck fixtures"); });
    schwartzZippelIdentityCheck<T>(rec1, 3, 4, 77);
    schwartzZippelIdentityCheck<T>(rec2, 3, 4, 77);
    check(log1 == log2 && log1.size() == 4, [] { return string("schwartzZippelIdentityCheck same seed same points"); });
    log2.clear();
    schwartzZippelIdentityCheck<T>(rec2, 3, 4, 78);
    check(log1 != log2, [] { return string("schwartzZippelIdentityCheck different seed different points"); });
    cout << "PASS schwartzZippelIdentityCheck: det(M) = det(M^T), det(AB) = det(A) det(B), (x + y)^2 expansion true; nonzero polynomials false; rounds 0, vars 0, seed reproducibility\n";}

void dynSuite() {
    Dyn::setMod(11);
    exactSuite<Dyn>("DynModInt mod 11", 3, reps(150), true);}

int main(int argc, char **argv) {
    using M = mint;
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        vector<int> perm;
        Mat<M> x;
        Mat<lng> l{{1, 2}};
        Mat<double> r{{1, 2}};
        vector<double> dv(2);
        if (s == "det-nonsquare") { determinant(Mat<M>{{1, 2}}); }
        if (s == "product-shape") { matrixProduct(Mat<M>{{1, 2, 3}}, Mat<M>{{1}, {2}}); }
        if (s == "power-nonsquare") { matrixPower(Mat<M>{{1, 2, 3}, {4, 5, 6}}, 3); }
        if (s == "detmod-zero") { determinantMod(Mat<lng>{{1}}, 0); }
        if (s == "detmod-negative") { determinantMod(Mat<lng>{{1}}, -6); }
        if (s == "detmod-nonsquare") { determinantMod(l, 6); }
        if (s == "inversemod-zero") { Mat<lng> a{{1}}; inverseMod(a, 0); }
        if (s == "inversemod-nonsquare") { inverseMod(l, 6); }
        if (s == "integer-nonsquare") { determinantInteger(l); }
        if (s == "inverse-nonsquare") { Mat<M> a{{1, 2}}; inverse(a); }
        if (s == "adjugate-nonsquare") { adjugate(Mat<M>{{1, 2}}); }
        if (s == "solve-size") { solveLinear(Mat<M>{{1, 2}, {3, 4}}, vector<M>(3)); }
        if (s == "solve-ragged") { solveLinear(Mat<M>{{1, 2}, {3}}, vector<M>(2)); }
        if (s == "multirhs-size") { solveLinearMultiRhs(Mat<M>{{1, 2}, {3, 4}}, Mat<M>{{1}}, x); }
        if (s == "multirhs-ragged") { solveLinearMultiRhs(Mat<M>{{1, 2}, {3, 4}}, Mat<M>{{1}, {1, 2}}, x); }
        if (s == "lu-nonsquare") { luDecompose(r, perm); }
        if (s == "lusolve-size") { Mat<double> a{{1}}; luDecompose(a, perm); luSolve(a, perm, dv); }
        if (s == "tridiagonal-size") { vector<M> d(3); tridiagonalSolve(vector<M>(2), vector<M>(3), vector<M>(3), d); }
        if (s == "cyclic-size") { vector<M> d(3); cyclicTridiagonalSolve(vector<M>(3), vector<M>(3), vector<M>(4), d); }
        if (s == "banded-width") { vector<M> d(2); bandedSolve(Mat<M>(2, vector<M>(3)), 1, 0, d); }
        if (s == "banded-negative") { vector<M> d(2); bandedSolve(Mat<M>(2, vector<M>(1)), -1, 1, d); }
        if (s == "banded-rows") { vector<M> d(3); bandedSolve(Mat<M>(2, vector<M>(1)), 0, 0, d); }
        if (s == "solvereal-size") { solveLinearReal(Mat<double>{{1}}, dv); }
        if (s == "detreal-nonsquare") { determinantReal(r); }
        if (s == "lowrank-shape") { Mat<M> a{{1}}; lowRankUpdateInverse(a, Mat<M>{{1}, {1}}, Mat<M>{{1}}); }
        if (s == "basis-length") { FieldBasis<M> b(2); b.insert(vector<M>(3)); }
        if (s == "basis-represent-length") { FieldBasis<M> b(2); vector<M> c; b.represent(vector<M>(1), c); }
        if (s == "basis-rollback") { FieldBasis<M> b(2); b.insert(vector<M>{1, 0}); b.rollback(2); }
        if (s == "freivalds-shape") { freivaldsProductCheck(Mat<M>{{1}}, Mat<M>{{1}}, Mat<M>{{1}, {1}}); }
        if (s == "rank-ragged") { matrixRank(Mat<M>{{1, 2}, {3}}); }
        if (s == "rref-ragged") { Mat<M> a{{1}, {1, 2}}; rref(a); }
        if (s == "nullspace-ragged") { nullspace(Mat<M>{{1, 2}, {3}}); }
        if (s == "product-ragged") { matrixProduct(Mat<M>{{1, 2}}, Mat<M>{{1}, {1, 2}}); }
        if (s == "basis-rollback-negative") { FieldBasis<M> b(2); b.rollback(-1); }
        if (s == "freivalds-inner") { freivaldsProductCheck(Mat<M>{{1, 2}}, Mat<M>{{1}}, Mat<M>{{1}}); }
        return 0;}
    string mode = "full";
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    quick = mode == "quick"; stress = mode == "stress";
    validateOracles();
    productSuite();
    exactSuite<F2>("GF(2)", 7, reps(300), true);
    exactSuite<F3>("GF(3)", 5, reps(300), true);
    exactSuite<F5>("GF(5)", 4, reps(300), true);
    exactSuite<F7>("GF(7)", 3, reps(300), true);
    dynSuite();
    exactSuite<M>("998244353", 12, reps(200), false);
    exactSuite<MP>("1e9+7", 9, reps(60), false);
    modSuite();
    integerSuite<int>("int", 4, 5);
    integerSuite<lng>("lng", 6, 10);
    integerSuite<lng>("lng", 8, 3);
    integerSuite<lll>("lll", 5, 1000);
    integerKnown();
    integerBoundary<lng>("lng", 2, (lll(1) << 31) - 1);
    integerBoundary<lng>("lng", 3, lll(1) << 15);
    integerBoundary<lll>("lll", 2, (lll(1) << 62) - 1);
    integerBoundary<lll>("lll", 3, lll(1) << 20);
    aliasSuite();
    powerSuite<M>("998244353"); powerSuite<F2>("GF(2)"); powerSuite<Z12>("Z/12"); powerSuite<ulng>("ulng mod 2^64");
    bandSuite<F2>("GF(2)", reps(200)); bandSuite<F3>("GF(3)", reps(200)); bandSuite<F5>("GF(5)", reps(200));
    bandSuite<F7>("GF(7)", reps(100)); bandSuite<M>("998244353", reps(100)); bandSuite<double>("double", reps(200));
    realSuite();
    lowRankSuite<M>("998244353"); lowRankSuite<F2>("GF(2)"); lowRankSuite<F3>("GF(3)"); lowRankSuite<F5>("GF(5)");
    basisSuite<F2>("GF(2)", 8, true); basisSuite<F3>("GF(3)", 5, true); basisSuite<F5>("GF(5)", 4, true); basisSuite<M>("998244353", 8, false);
    freivaldsSuite<M>("998244353", 2); freivaldsSuite<F2>("GF(2)", 40);
    szSuite();
    cout << "PASS all, " << checks << " checks\n";}
