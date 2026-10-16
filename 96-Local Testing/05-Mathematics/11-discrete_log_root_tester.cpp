#include "../../05-Mathematics/11-discrete_log_root.hpp"

lng checks = 0;
ulng seed = 1;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << " seed=" << seed << endl; exit(1); }}
void expect(lng got, lng want, const string &what) {
    ++checks;
    if (got != want) { cout << "FAIL " << what << " expected=" << want << " actual=" << got << " seed=" << seed << endl; exit(1); }}
string show(std::initializer_list<lng> v) {
    string s;
    for (lng x : v) { s += ' ' + std::to_string(x); }
    return s;}
lng nrm(lng a, lng m) { lng r = a % m; return r < 0 ? r + m : r; }
lng mulm(lng a, lng b, lng m) { return lng(lll(a) * b % m); }
// Square-and-multiply with 0^0 = 1, written independently of the header.
lng powm(lng a, ulng e, lng m) {
    lng r = 1 % m; a = nrm(a, m);
    for (; e; e >>= 1, a = mulm(a, a, m)) { if (e & 1) { r = mulm(r, a, m); } }
    return r;}
bool isPrimeSmall(lng n) {
    if (n < 2) { return false; }
    for (lng d = 2; d * d <= n; ++d) { if (n % d == 0) { return false; } }
    return true;}
// Least x in [0, m) with k * a^x = b mod m by iteration; the sequence is eventually periodic within m terms.
lng bruteLog(lng a, lng b, lng m, lng k) {
    a = nrm(a, m); b = nrm(b, m);
    lng v = nrm(k, m);
    for (lng x = 0; x < m; ++x, v = mulm(v, a, m)) { if (v == b) { return x; } }
    return -1;}
vector<lng> orderTable(lng m) {
    vector<lng> ord(m, -1);
    for (lng a = 0; a < m; ++a) {
        if (gcd(a, m) != 1) { continue; }
        lng v = a % m;
        for (lng t = 1; t <= m; ++t, v = mulm(v, a, m)) { if (v == 1 % m) { ord[a] = t; break; } }}
    return ord;}
lng brutePhi(lng m) {
    lng c = 0;
    for (lng a = 0; a < m; ++a) { c += gcd(a, m) == 1; }
    return c;}
int bruteLegendre(lng a, lng p) {
    lng r = nrm(a, p);
    if (!r) { return 0; }
    for (lng x = 1; x < p; ++x) { if (mulm(x, x, p) == r) { return 1; } }
    return -1;}
// Kronecker symbol from its definition: sign rule, (a / 2) table and square enumeration per odd prime factor.
int bruteKronecker(lng a, lng n) {
    if (!n) { return a == 1 || a == -1; }
    int res = n < 0 && a < 0 ? -1 : 1;
    ulng u = n < 0 ? -ulng(n) : ulng(n);
    while (u % 2 == 0) {
        u /= 2;
        if (a % 2 == 0) { return 0; }
        lng r = nrm(a, 8); res *= r == 1 || r == 7 ? 1 : -1;}
    for (ulng p = 3; p <= u; p += 2) {
        while (u % p == 0) { u /= p; res *= bruteLegendre(a, lng(p)); }}
    return res;}

// Functional graph (x = map), modular scalars and 2 x 2 matrices acting on vectors for discreteLogMonoid.
using Map = vector<int>;
Map compose(const Map &f, const Map &g) {
    Map h(f.size());
    for (size_t i = 0; i < f.size(); ++i) { h[i] = f[g[i]]; }
    return h;}
// Least n in [lb, ub) with f^n(s) = t: jump to lb through the tail-cycle decomposition, then walk at most |S| + 1 steps.
lng bruteMonoid(const Map &f, int s, int t, lng lb, lng ub) {
    int N = int(f.size());
    vector<lng> seen(N, -1);
    vector<int> seq;
    int v = s;
    while (seen[v] < 0) { seen[v] = lng(seq.size()); seq.pb(v); v = f[v]; }
    lng mu = seen[v], lam = lng(seq.size()) - mu;
    v = lb < lng(seq.size()) ? seq[lb] : seq[mu + (lb - mu) % lam];
    for (lng n = lb; n < ub && n <= lb + N; ++n, v = f[v]) { if (v == t) { return n; } }
    return -1;}
using Mat = array<lng, 4>;

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        auto mulI = [](lng x, lng y) { return x * y; };
        auto actI = [](lng x, lng y) { return x * y; };
        auto hashI = [](lng x) { return ulng(x); };
        if (s == "order-mod") { multiplicativeOrder(2, 0); }
        if (s == "log-mod") { discreteLogEx(2, 3, 0); }
        if (s == "monoid-range") { discreteLogMonoid(lng(1), lng(1), lng(1), 5, 4, mulI, actI, hashI); }
        if (s == "monoid-negative") { discreteLogMonoid(lng(1), lng(1), lng(1), -1, 4, mulI, actI, hashI); }
        if (s == "exists-mod") { primitiveRootExists(0); }
        if (s == "isroot-mod") { isPrimitiveRoot(1, 0); }
        if (s == "root-mod") { primitiveRoot(-5); }
        if (s == "legendre-two") { legendreSymbol(1, 2); }
        if (s == "legendre-even") { legendreSymbol(1, 4); }
        if (s == "cipolla-mod") { sqrtModPrimeCipolla(1, 1); }
        if (s == "kth-negative") { kthRootModPrime(1, -1, 7); }
        if (s == "kthexists-mod") { kthRootExists(1, 1, 1); }
        if (s == "unit-order") { unitRootModPrime(0, 7); }
        if (s == "unit-mod") { unitRootModPrime(1, 1); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op;
        while (cin >> op) {
            if (op == "L") { lng a, b, m, k; cin >> a >> b >> m >> k; cout << discreteLogEx(a, b, m, k) << '\n'; }
            if (op == "O") { lng a, m; cin >> a >> m; cout << multiplicativeOrder(a, m) << '\n'; }
            if (op == "P") { lng m, g; cin >> m >> g; cout << primitiveRootExists(m) << ' ' << primitiveRoot(m) << ' ' << isPrimitiveRoot(g, m) << '\n'; }
            if (op == "K") { lng a, n; cin >> a >> n; cout << kroneckerSymbol(a, n) << '\n'; }
            if (op == "S") { lng a, p; cin >> a >> p; cout << legendreSymbol(a, p) << ' ' << sqrtModPrimeCipolla(a, p) << '\n'; }
            if (op == "R") { lng a, k, p; cin >> a >> k >> p; cout << kthRootExists(a, k, p) << ' ' << kthRootModPrime(a, k, p) << '\n'; }
            if (op == "U") { lng n, p; cin >> n >> p; cout << unitRootModPrime(n, p) << '\n'; }}
        return 0;}
    string mode = "full";
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    std::mt19937_64 rng(seed);
    auto rnd = [&](lng l, lng r) {
        ulng span = ulng(r) - ulng(l) + 1;
        return lng(ulng(l) + (span ? rng() % span : rng()));};
    int lvl = mode == "quick" ? 0 : mode == "full" ? 1 : 2;
    const lng MIN = std::numeric_limits<lng>::min(), MAX = std::numeric_limits<lng>::max();

    // multiplicativeOrder, isPrimitiveRoot, primitiveRootExists, primitiveRoot: every m up to the limit against order tables.
    lng limM = array<lng, 3>{120, 320, 520}[lvl];
    for (lng m = 1; m <= limM; ++m) {
        auto ord = orderTable(m);
        lng ph = brutePhi(m), small = -1;
        for (lng a = 0; a < m && small < 0; ++a) { if (ord[a] == ph) { small = a; } }
        string tag = show({m});
        check(primitiveRootExists(m) == (small >= 0), "primitiveRootExists" + tag);
        expect(primitiveRoot(m), small, "primitiveRoot" + tag);
        for (lng a = -m; a < 2 * m; ++a) {
            lng r = nrm(a, m);
            expect(multiplicativeOrder(a, m), ord[r], "multiplicativeOrder" + show({a, m}));
            check(isPrimitiveRoot(a, m) == (ord[r] == ph), "isPrimitiveRoot" + show({a, m}));}
        for (lng a : vector<lng>{MIN, MIN + 1, MAX, MAX - 1}) {
            expect(multiplicativeOrder(a, m), ord[nrm(a, m)], "multiplicativeOrder extreme" + show({a, m}));
            check(isPrimitiveRoot(a, m) == (ord[nrm(a, m)] == ph), "isPrimitiveRoot extreme" + show({a, m}));}}
    for (auto [p, g] : vector<pair<lng, lng>>{{998244353, 3}, {1000000007, 5}, {2305843009213693951LL, 37}, {754974721, 11}, {167772161, 3}, {469762049, 3}}) {
        check(primitiveRoot(p) == g && isPrimitiveRoot(g, p) && !isPrimitiveRoot(g - 1, p), "primitiveRoot known" + show({p, g}));}
    check(!primitiveRootExists(MAX) && primitiveRoot(MAX) == -1 && !isPrimitiveRoot(3, MAX), "primitiveRoot 2^63 - 1");
    check(multiplicativeOrder(2, MAX) == 63 && multiplicativeOrder(-1, MAX) == 2 && multiplicativeOrder(7, MAX) == -1, "multiplicativeOrder 2^63 - 1");
    check(multiplicativeOrder(3, 998244353) == 998244352 && multiplicativeOrder(MIN, 1) == 1, "multiplicativeOrder known");

    // discreteLogEx: every (a, b, k) for small m, random moduli and planted solutions against iteration.
    lng limL = array<lng, 3>{12, 20, 26}[lvl];
    for (lng m = 1; m <= limL; ++m) {
        for (lng a = 0; a < m; ++a) {
            for (lng b = 0; b < m; ++b) {
                for (lng k = 0; k < m; ++k) {
                    expect(discreteLogEx(a, b, m, k), bruteLog(a, b, m, k), "discreteLogEx" + show({a, b, m, k}));}
                expect(discreteLogEx(a - m, b + 3 * m, m), bruteLog(a, b, m, 1), "discreteLogEx default k" + show({a, b, m}));}}}
    int cnt = array<int, 3>{400, 4000, 20000}[lvl], solved = 0;
    for (int it = 0; it < cnt; ++it) {
        lng m = it % 4 == 0 ? rnd(1, 30000) : lng(std::exp2(double(rnd(0, 1400)) / 100.0)) + rnd(0, 3);
        if (it % 7 == 0) { m = array<lng, 8>{720720, 65536, 59049, 2 * 15625, 30011, 2 * 30011, 9 * 25 * 49, 1 << 14}[it / 7 % 8]; }
        lng a = rnd(MIN, MAX), k = it % 3 ? 1 : rnd(MIN, MAX), b = it % 2 ? rnd(MIN, MAX) : mulm(nrm(k, m), powm(a, ulng(rnd(0, 2 * m)), m), m);
        lng want = bruteLog(a, b, m, k);
        solved += want >= 0;
        expect(discreteLogEx(a, b, m, k), want, "discreteLogEx random" + show({a, b, m, k}));}
    check(discreteLogEx(0, 0, 1, 5) == 0 && discreteLogEx(0, 0, 7) == 1 && discreteLogEx(0, 1, 7) == 0 && discreteLogEx(0, 3, 7, 3) == 0 && discreteLogEx(0, 3, 7) == -1, "discreteLogEx zero cases");
    check(discreteLogEx(3, 1, 998244353) == 0 && discreteLogEx(3, 998244352, 998244353) == 499122176, "discreteLogEx known prime");

    // discreteLogMonoid: random functional graphs (rho shapes, false hits), all (s, t), several ranges and three hashes.
    int graphs = array<int, 3>{150, 900, 4000}[lvl], falseProne = 0;
    for (int it = 0; it < graphs; ++it) {
        int N = int(rnd(1, 10));
        Map f(N);
        for (int &v : f) { v = int(rnd(0, N - 1)); }
        int hk = it % 3;
        auto hash = [hk](int v) { return hk == 0 ? ulng(v) : hk == 1 ? ulng(0) : ulng(v % 2); };
        auto act = [](const Map &g, int v) { return g[v]; };
        for (int s = 0; s < N; ++s) {
            for (int t = 0; t < N; ++t) {
                falseProne += bruteMonoid(f, s, t, 0, N + 1) < 0 && bruteMonoid(f, s, f[t], 0, N + 1) >= 0;
                for (int r = 0; r < 4; ++r) {
                    lng lb = r == 0 ? 0 : r == 1 ? rnd(0, 3 * N) : r == 2 ? rnd(0, 2) : lng(1e18) + rnd(0, 100);
                    lng ub = lb + rnd(0, 4 * N);
                    lng want = bruteMonoid(f, s, t, lb, ub);
                    expect(discreteLogMonoid(f, s, t, lb, ub, compose, act, hash), want, "discreteLogMonoid graph" + show({it, s, t, lb, ub}));}}}}
    // Modular scalars: x^n s = t mod m on [lb, ub) against iteration from s * x^lb.
    for (int it = 0; it < cnt / 4; ++it) {
        lng m = rnd(1, 300), x = rnd(0, m - 1), s = rnd(0, m - 1), t = it % 2 ? rnd(0, m - 1) : mulm(s, powm(x, ulng(rnd(0, 3 * m)), m), m);
        lng lb = it % 5 ? rnd(0, 2 * m) : rnd(0, lng(1e18)), ub = lb + rnd(0, 3 * m), want = -1, v = mulm(s, powm(x, ulng(lb), m), m);
        for (lng n = lb; n < ub && n <= lb + m; ++n, v = mulm(v, x, m)) { if (v == t) { want = n; break; } }
        auto mul = [m](lng p, lng q) { return mulm(p, q, m); };
        expect(discreteLogMonoid(x, s, t, lb, ub, mul, mul, [](lng v2) { return ulng(v2); }), want, "discreteLogMonoid modular" + show({x, s, t, m, lb, ub}));
        if (lb == 0 && ub > m) { check(want == discreteLogEx(x, t, m, s), "discreteLogMonoid vs discreteLogEx" + show({x, s, t, m})); }}
    // 2 x 2 matrices mod q acting on vectors.
    for (int it = 0; it < cnt / 8; ++it) {
        lng q = rnd(2, 7);
        Mat x{rnd(0, q - 1), rnd(0, q - 1), rnd(0, q - 1), rnd(0, q - 1)};
        pair<lng, lng> s{rnd(0, q - 1), rnd(0, q - 1)}, t{rnd(0, q - 1), rnd(0, q - 1)};
        auto mul = [q](const Mat &A, const Mat &B) {
            return Mat{(A[0] * B[0] + A[1] * B[2]) % q, (A[0] * B[1] + A[1] * B[3]) % q, (A[2] * B[0] + A[3] * B[2]) % q, (A[2] * B[1] + A[3] * B[3]) % q};};
        auto act = [q](const Mat &A, pair<lng, lng> v) { return pair<lng, lng>{(A[0] * v.fi + A[1] * v.se) % q, (A[2] * v.fi + A[3] * v.se) % q}; };
        lng lb = rnd(0, 60), ub = lb + rnd(0, 120), want = -1;
        auto v = s;
        for (lng n = 0; n < ub; ++n, v = act(x, v)) { if (n >= lb && v == t) { want = n; break; } }
        expect(discreteLogMonoid(x, s, t, lb, ub, mul, act, [q](pair<lng, lng> w) { return ulng(w.fi * q + w.se); }), want, "discreteLogMonoid matrix" + show({it, lb, ub}));}

    // kroneckerSymbol, legendreSymbol: exhaustive window, extremes with smooth n, odd primes.
    lng limK = array<lng, 3>{40, 120, 200}[lvl];
    for (lng a = -limK; a <= limK; ++a) {
        for (lng n = -limK; n <= limK; ++n) { expect(kroneckerSymbol(a, n), bruteKronecker(a, n), "kroneckerSymbol" + show({a, n})); }}
    vector<lng> ext{MIN, MIN + 1, MAX, MAX - 1, -(1LL << 62), 1LL << 62, (1LL << 62) + 1, -3, -1, 0, 1, 2, 3, 5, 7};
    for (lng a : ext) {
        for (lng n : vector<lng>{MIN, MIN + 1, MAX, -(1LL << 62), 1LL << 62, 3LL << 61, -(5LL << 60), 45LL << 55, -1, 1, 0, 2, 9, -15}) {
            expect(kroneckerSymbol(a, n), bruteKronecker(a, n), "kroneckerSymbol extreme" + show({a, n}));}}
    lng limP = array<lng, 3>{150, 600, 1500}[lvl];
    for (lng p = 3; p <= limP; p += 2) {
        if (!isPrimeSmall(p)) { continue; }
        for (lng a = -p; a <= 2 * p; ++a) { check(legendreSymbol(a, p) == bruteLegendre(a, p) && kroneckerSymbol(a, p) == bruteLegendre(a, p), "legendreSymbol" + show({a, p})); }
        for (lng a : vector<lng>{MIN, MAX}) { check(legendreSymbol(a, p) == bruteLegendre(a, p), "legendreSymbol extreme" + show({a, p})); }}

    // sqrtModPrimeCipolla: every residue of every prime up to the limit, smaller root.
    for (lng p = 2; p <= limP; ++p) {
        if (!isPrimeSmall(p)) { continue; }
        vector<lng> root(p, -1);
        for (lng x = p - 1; x >= 0; --x) { root[mulm(x, x, p)] = x; }
        for (lng a = -p; a < 2 * p; ++a) { expect(sqrtModPrimeCipolla(a, p), root[nrm(a, p)], "sqrtModPrimeCipolla" + show({a, p})); }
        for (lng a : vector<lng>{MIN, MAX}) { expect(sqrtModPrimeCipolla(a, p), root[nrm(a, p)], "sqrtModPrimeCipolla extreme" + show({a, p})); }}

    // kthRootModPrime, kthRootExists: every (a, k) for small primes, then valuation-heavy primes with sampled residues.
    lng limR = array<lng, 3>{50, 100, 160}[lvl];
    auto kthCase = [&](lng p, lng k, const vector<lng> &as) {
        vector<lng> img(p, -1);
        for (lng x = 0; x < p; ++x) { img[powm(x, ulng(k), p)] = x; }
        for (lng a : as) {
            lng r = kthRootModPrime(a, k, p), want = img[nrm(a, p)];
            string tag = show({a, k, p});
            check(kthRootExists(a, k, p) == (want >= 0), "kthRootExists" + tag);
            check(want < 0 ? r == -1 : 0 <= r && r < p && powm(r, ulng(k), p) == nrm(a, p), "kthRootModPrime" + tag);}};
    for (lng p = 2; p <= limR; ++p) {
        if (!isPrimeSmall(p)) { continue; }
        vector<lng> as;
        for (lng a = -p; a < 2 * p; ++a) { as.pb(a); }
        as.pb(MIN); as.pb(MAX);
        for (lng k = 0; k <= p + 1; ++k) { kthCase(p, k, as); }
        for (lng k : vector<lng>{2 * (p - 1), 3 * (p - 1) + 2, (p - 1) * 1000003 + 5, MAX, MAX - 1, lng(1e18)}) { kthCase(p, k, as); }}
    vector<lng> heavy{17, 41, 73, 97, 193, 257, 109, 163, 487, 1459, 251, 751, 1373, 2663, 12289, 40961, 65537, 39367, 62501, 14407, 13183};
    for (lng p : heavy) {
        if (lvl == 0 && p > 2000) { continue; }
        vector<lng> as;
        if (p < 2000) { for (lng a = 0; a < p; ++a) { as.pb(a); } }
        else { for (int i = 0; i < 300; ++i) { as.pb(rnd(MIN, MAX)); as.pb(powm(rnd(0, p - 1), ulng(rnd(0, p)), p)); } }
        vector<lng> ks;
        for (lng q : vector<lng>{2, 3, 5, 7, 11, 13}) {
            lng s = 0, r = p - 1;
            while (r % q == 0) { r /= q; ++s; }
            for (lng e = 1, qe = q; e <= s; ++e, qe *= q) {
                ks.pb(qe); ks.pb(qe * rnd(1, 50));
                if (r > 1) { ks.pb(qe * r); }}}
        ks.pb(p - 1); ks.pb(rnd(0, MAX));
        if (p > 2000) { ks.resize(min(ks.size(), size_t(lvl == 1 ? 14 : 40))); }
        for (lng k : ks) { kthCase(p, k, as); }}

    // unitRootModPrime: order exactly n for n | p - 1, -1 otherwise.
    lng limU = array<lng, 3>{150, 500, 1000}[lvl];
    for (lng p = 2; p <= limU; ++p) {
        if (!isPrimeSmall(p)) { continue; }
        for (lng n = 1; n <= p + 1; ++n) {
            lng x = unitRootModPrime(n, p), ord = 0;
            if ((p - 1) % n) { check(x == -1, "unitRootModPrime nondivisor" + show({n, p})); continue; }
            for (lng v = x, t = 1; t <= n && x > 0; ++t, v = mulm(v, x, p)) { if (v == 1) { ord = t; break; } }
            check(0 < x && x < p && ord == n, "unitRootModPrime" + show({n, p}));}}
    for (lng p : vector<lng>{998244353, 1000000007, 2305843009213693951}) {
        lng x = unitRootModPrime(p - 1, p);
        check(isPrimitiveRoot(x, p) && unitRootModPrime(1, p) == 1 && unitRootModPrime(2, p) == p - 1, "unitRootModPrime known" + show({p}));}
    lng w = unitRootModPrime(1 << 23, 998244353);
    check(powm(w, 1 << 23, 998244353) == 1 && powm(w, 1 << 22, 998244353) == 998244352, "unitRootModPrime 2^23");

    cout << "PASS 11-discrete_log_root " << checks << " checks seed=" << seed << " (" << solved << " solvable random logs, " << falseProne << " false-hit-prone monoid cases)" << endl;
    return 0;}
