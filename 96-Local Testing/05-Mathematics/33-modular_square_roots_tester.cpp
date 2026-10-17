#include "../../05-Mathematics/33-modular_square_roots.hpp"

lng checks = 0;
ulng seed = 1;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << " seed=" << seed << std::endl; std::exit(1); }}
void expect(lng got, lng want, const string &what) {
    ++checks;
    if (got != want) { cout << "FAIL " << what << " expected=" << want << " actual=" << got << " seed=" << seed << std::endl; std::exit(1); }}
string show(std::initializer_list<lng> v) {
    string s;
    for (lng x : v) { s += ' ' + std::to_string(x); }
    return s;}
lng nrm(lll a, lng m) { lng r = lng(a % m); return r < 0 ? r + m : r; }
lng mulm(lng a, lng b, lng m) { return lng(lll(a) * b % m); }
// Square-and-multiply with 0^0 = 1, written independently of the header.
lng powm(lng a, ulng e, lng m) {
    lng r = 1 % m; a = nrm(a, m);
    for (; e; e >>= 1, a = mulm(a, a, m)) { if (e & 1) { r = mulm(r, a, m); } }
    return r;}
lng evalm(const vector<lng> &f, lng x, lng m) {
    lll v = 0;
    for (int i = int(f.size()) - 1; i >= 0; --i) { v = (v * x + f[i]) % m; }
    return nrm(v, m);}
// (p, e) when m = p^e with e >= 1 by trial division, else (0, 0).
pair<lng, int> primePower(lng m) {
    for (lng p = 2; p * p <= m; ++p) {
        if (m % p) { continue; }
        int e = 0;
        while (m % p == 0) { m /= p; ++e; }
        return m == 1 ? pair<lng, int>{p, e} : pair<lng, int>{0, 0};}
    return m > 1 ? pair<lng, int>{m, 1} : pair<lng, int>{0, 0};}
// Members of residue classes in [0, m), after checking shape (M | m, 0 <= r < M), sorted order and disjointness.
vector<lng> expand(const vector<pair<lng, lng>> &cls, lng m, const string &what) {
    vector<lng> res;
    for (auto [r, M] : cls) {
        check(M >= 1 && m % M == 0 && 0 <= r && r < M, what + " class shape");
        for (lng x = r; x < m; x += M) { res.pb(x); }}
    sort(res.begin(), res.end());
    check(std::is_sorted(cls.begin(), cls.end()) && std::adjacent_find(res.begin(), res.end()) == res.end(), what + " sorted disjoint");
    return res;}
void printClasses(const vector<pair<lng, lng>> &cls) {
    cout << cls.size();
    for (auto [r, M] : cls) { cout << ' ' << r << ' ' << M; }}

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "sqrtpp-overflow") { sqrtModPrimePower(1, 2, 63); }
        if (s == "sqrtpp-exponent") { sqrtModPrimePower(1, 3, 0); }
        if (s == "sqrtpp-prime") { sqrtModPrimePower(1, 1, 3); }
        if (s == "sqrt2k-range") { sqrtMod2k(1, 65); }
        if (s == "sqrt2k-negative") { sqrtMod2k(1, -1); }
        if (s == "sqrtcomp-mod") { sqrtModComposite(1, 0); }
        if (s == "sqrtall-mod") { sqrtModAll(1, -3); }
        if (s == "kthpp-negative") { kthRootModPrimePower(1, -1, 3, 2); }
        if (s == "kthpp-overflow") { kthRootModPrimePower(1, 2, 3, 40); }
        if (s == "kthcomp-mod") { kthRootModComposite(1, 2, 0); }
        if (s == "kthcomp-negative") { kthRootModComposite(1, -2, 10); }
        if (s == "count-mod") { rootCount(1, 2, 0); }
        if (s == "count-negative") { rootCount(1, -1, 10); }
        if (s == "poly-mod") { polynomialCongruenceRoots({1, 1}, 0); }
        if (s == "quad-mod") { quadraticCongruenceMod(1, 0, -1, 0); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op;
        while (cin >> op) {
            if (op == "S") { lng a, m; cin >> a >> m; cout << sqrtModComposite(a, m) << ' '; printClasses(sqrtModAll(a, m)); }
            if (op == "P") { lng a, p; int e; cin >> a >> p >> e; cout << sqrtModPrimePower(a, p, e); }
            if (op == "T") { lng a; int k; cin >> a >> k; cout << sqrtMod2k(a, k); }
            if (op == "K") { lng a, k, m; cin >> a >> k >> m; cout << kthRootModComposite(a, k, m) << ' ' << rootCount(a, k, m); }
            if (op == "R") { lng a, k, p; int e; cin >> a >> k >> p >> e; cout << kthRootModPrimePower(a, k, p, e); }
            if (op == "Q") { lng a, b, c, m; cin >> a >> b >> c >> m; printClasses(quadraticCongruenceMod(a, b, c, m)); }
            if (op == "F") {
                lng m; ulng sd; int n; cin >> m >> sd >> n;
                vector<lng> f(n);
                for (lng &c : f) { cin >> c; }
                printClasses(polynomialCongruenceRoots(f, m, sd));}
            cout << '\n';}
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

    // sqrtModAll, sqrtModComposite, sqrtModPrimePower, rootCount(a, 2, m): every residue of every m up to the limit against enumerated square roots.
    lng limS = array<lng, 3>{120, 400, 700}[lvl];
    for (lng m = 1; m <= limS; ++m) {
        vector<vector<lng>> roots(m);
        for (lng x = 0; x < m; ++x) { roots[mulm(x, x, m)].pb(x); }
        auto [p, e] = primePower(m);
        vector<lng> as;
        for (lng a = 0; a < m; ++a) { as.pb(a); }
        for (int i = 0; i < 6; ++i) { as.pb(rnd(MIN, MAX)); }
        as.pb(MIN); as.pb(MAX); as.pb(-m); as.pb(-1);
        for (lng a : as) {
            auto &want = roots[nrm(a, m)];
            string tag = show({a, m});
            check(expand(sqrtModAll(a, m), m, "sqrtModAll" + tag) == want, "sqrtModAll" + tag);
            lng r = sqrtModComposite(a, m);
            check(want.empty() ? r == -1 : 0 <= r && r < m && mulm(r, r, m) == nrm(a, m), "sqrtModComposite" + tag);
            expect(rootCount(a, 2, m), lng(want.size()), "rootCount square" + tag);
            if (p) { expect(sqrtModPrimePower(a, p, e), want.empty() ? -1 : want[0], "sqrtModPrimePower" + tag); }}}

    // sqrtMod2k: exhaustive least roots for k <= 14, then planted and random residues up to k = 64 (2-adic wraparound).
    for (int k = 0; k <= 14; ++k) {
        lng P = 1LL << k;
        vector<lng> least(P, -1);
        for (lng x = P - 1; x >= 0; --x) { least[mulm(x, x, P)] = x; }
        for (lng a = 0; a < P; ++a) { expect(sqrtMod2k(a, k), least[a], "sqrtMod2k" + show({a, k})); }
        for (int i = 0; i < 40; ++i) {
            lng a = rnd(MIN, MAX);
            expect(sqrtMod2k(a, k), least[ulng(a) & ulng(P - 1)], "sqrtMod2k shifted" + show({a, k}));}}
    int cases2 = array<int, 3>{400, 3000, 12000}[lvl];
    for (int it = 0; it < cases2; ++it) {
        int k = int(rnd(15, 64));
        ulng mask = k == 64 ? ~0ULL : (1ULL << k) - 1, y = ulng(rnd(MIN, MAX)) & mask;
        if (it % 4 == 1) { y &= ~0ULL << rnd(0, k - 1); }
        ulng a = it % 3 == 2 ? ulng(rnd(MIN, MAX)) & mask : y * y & mask;
        lng r = sqrtMod2k(lng(a), k);
        string tag = show({lng(a), k});
        if (it % 3 != 2) {
            ulng h = k == 64 ? 1ULL << 63 : 1ULL << (k - 1);
            ulng best = min({y, (0 - y) & mask, (y + h) & mask, (h - y) & mask});
            check(r >= 0 && (ulng(r) * ulng(r) & mask) == a && ulng(r) <= best, "sqrtMod2k planted" + tag);}
        else {
            // Squares mod 2^k: 0, or 4^t u with u = 1 mod 8 (mod 4 or 2 when only two or one bit of u remains).
            int v = a ? std::countr_zero(a) : k, f = k - v;
            ulng u = a >> v;
            bool sq = !a || (v % 2 == 0 && (f == 1 || (f == 2 && u % 4 == 1) || (f >= 3 && u % 8 == 1)));
            check(sq ? r >= 0 && (ulng(r) * ulng(r) & mask) == a : r == -1, "sqrtMod2k random" + tag);}
        if (k <= 62) { expect(sqrtModPrimePower(lng(a & mask), 2, k), r, "sqrtModPrimePower 2^k" + tag); }}

    // kthRootModComposite, kthRootModPrimePower, rootCount: every (a, k) for m up to the limit against power tables, including huge k.
    lng limK = array<lng, 3>{60, 150, 230}[lvl];
    int kmax = array<int, 3>{10, 18, 24}[lvl];
    for (lng m = 1; m <= limK; ++m) {
        auto [p, e] = primePower(m);
        vector<lng> ks;
        for (lng k = 0; k <= kmax; ++k) { ks.pb(k); }
        for (lng k : vector<lng>{MAX, MAX - 1, lng(1) << 62, lng(720720) * 1000003, lng(3) << 60, lng(6) * 1000000007}) { ks.pb(k); }
        for (lng k : ks) {
            vector<lng> cnt(m, 0);
            for (lng x = 0; x < m; ++x) { ++cnt[powm(x, ulng(k), m)]; }
            for (lng a = -1; a < m; ++a) {
                lng b = a < 0 ? rnd(MIN, MAX) : a, want = cnt[nrm(b, m)];
                string tag = show({b, k, m});
                lng r = kthRootModComposite(b, k, m);
                check(want ? 0 <= r && r < m && powm(r, ulng(k), m) == nrm(b, m) : r == -1, "kthRootModComposite" + tag);
                expect(rootCount(b, k, m), want, "rootCount" + tag);
                if (p) {
                    r = kthRootModPrimePower(b, k, p, e);
                    check(want ? 0 <= r && r < m && powm(r, ulng(k), m) == nrm(b, m) : r == -1, "kthRootModPrimePower" + tag);}}}}

    // Large prime powers with p | k (digit lifting), planted roots including non-units, and composite moduli near 2^63.
    vector<pair<lng, int>> pps{{2, 62}, {2, 61}, {2, 33}, {3, 39}, {3, 20}, {5, 27}, {7, 22}, {11, 18}, {13, 17}, {1000003, 3}, {2147483647, 2}, {2305843009213693951LL, 1}, {1000000007, 2}};
    int casesK = array<int, 3>{300, 2500, 10000}[lvl];
    for (int it = 0; it < casesK; ++it) {
        auto [p, e] = pps[it % pps.size()];
        lng P = 1;
        for (int i = 0; i < e; ++i) { P *= p; }
        lng x = rnd(0, P - 1), k = it % 5 == 0 ? rnd(0, MAX) : it % 5 == 1 ? p * rnd(1, min(lng(40), MAX / p)) : it % 5 == 2 ? rnd(1, 64) : 1;
        if (it % 5 >= 3) { for (int j = int(rnd(1, 6)); j > 0; --j) { if (k <= MAX / p) { k *= p; } } }
        if (it % 7 == 0) { x = x / p * p; }
        lng a = powm(x, ulng(k), P);
        string tag = show({a, k, p, e});
        lng r = kthRootModPrimePower(a, k, p, e);
        check(0 <= r && r < P && powm(r, ulng(k), P) == a, "kthRootModPrimePower planted" + tag);
        lng sq = mulm(x, x, P), s = sqrtModPrimePower(sq, p, e);
        check(0 <= s && s <= min(x, P - x) && mulm(s, s, P) == sq, "sqrtModPrimePower planted" + tag);
        // quadraticCongruenceMod on the prime power: lead (t - x)(t - z), every class checked by its Taylor terms, planted roots covered, total against the polynomial lifter.
        lng z = rnd(0, P - 1), lead = it % 4 == 0 ? mulm(rnd(1, P - 1), p, P) : rnd(1, P - 1);
        lng qa = lead, qb = nrm(-lll(lead) * ((lll(x) + z) % P), P), qc = mulm(mulm(lead, x, P), z, P);
        auto qcls = quadraticCongruenceMod(qa, qb, qc, P);
        lng qtotal = 0;
        bool hx = false, hz = false;
        for (auto [c, M] : qcls) {
            check(M >= 1 && P % M == 0 && 0 <= c && c < M && nrm(lll(mulm(qa, mulm(c, c, P), P)) + mulm(qb, c, P) + qc, P) == 0 && mulm(nrm(lll(mulm(mulm(2, qa, P), c, P)) + qb, P), M, P) == 0 && mulm(qa, mulm(M, M, P), P) == 0, "quadraticCongruenceMod prime power class" + tag);
            qtotal += P / M; hx |= x % M == c; hz |= z % M == c;}
        lng ptotal = 0;
        for (auto [c, M] : polynomialCongruenceRoots({qc, qb, qa}, P, ulng(it))) { ptotal += P / M; }
        check(hx && hz && qtotal == ptotal, "quadraticCongruenceMod prime power planted" + tag);
        if (lead % p && (x - z) % p) { expect(qtotal, 2, "quadraticCongruenceMod two simple roots" + tag); }
        // A composite modulus: P times a coprime cofactor, the root found must reduce correctly.
        lng q = it % 2 ? 3 : 1000000007;
        if (p != q && P <= MAX / q) {
            lng y = rnd(0, P * q - 1), b = powm(y, ulng(k), P * q);
            r = kthRootModComposite(b, k, P * q);
            check(0 <= r && r < P * q && powm(r, ulng(k), P * q) == b, "kthRootModComposite planted" + tag);
            check(rootCount(b, k, P * q) == rootCount(b, k, P) * rootCount(b, k, q) && rootCount(b, k, P * q) >= 1, "rootCount multiplicative" + tag);
            lng b2 = mulm(y, y, P * q), total = 0;
            auto cls = sqrtModAll(b2, P * q);
            for (auto [c, M] : cls) {
                check(M >= 1 && (P * q) % M == 0 && 0 <= c && c < M && mulm(c, c, P * q) == b2 && mulm(mulm(2, c, P * q), M, P * q) == 0 && mulm(M, M, P * q) == 0, "sqrtModAll planted class" + tag);
                total += P * q / M;}
            expect(total, rootCount(b2, 2, P * q), "sqrtModAll planted count" + tag);}}

    // quadraticCongruenceMod: every m up to the limit, a grid of coefficients with prime-power multiples, and random 64-bit coefficients.
    lng limQ = array<lng, 3>{50, 120, 180}[lvl];
    vector<lng> grid{0, 1, -1, 2, 3, 4, 6, 8, 9, 12, 25, 27};
    if (lvl == 0) { grid.resize(8); }
    for (lng m = 1; m <= limQ; ++m) {
        auto test = [&](lng a, lng b, lng c) {
            vector<lng> want;
            for (lng x = 0; x < m; ++x) { if (nrm(lll(nrm(a, m)) * x * x + lll(nrm(b, m)) * x + nrm(c, m), m) == 0) { want.pb(x); } }
            string tag = show({a, b, c, m});
            auto cls = quadraticCongruenceMod(a, b, c, m);
            check(expand(cls, m, "quadraticCongruenceMod" + tag) == want, "quadraticCongruenceMod" + tag);
            for (auto [r, M] : cls) { check(M == cls[0].se, "quadraticCongruenceMod common modulus" + tag); }};
        for (lng a : grid) {
            for (lng b : grid) {
                for (lng c : grid) { test(a, b, c); }}}
        for (int i = 0; i < 20; ++i) { test(rnd(MIN, MAX), rnd(MIN, MAX), rnd(MIN, MAX)); }}

    // polynomialCongruenceRoots: structured products of (x - r)^k, random and degenerate polynomials, against evaluation at every residue.
    int casesP = array<int, 3>{400, 4000, 16000}[lvl];
    for (int it = 0; it < casesP; ++it) {
        lng m = it % 10 == 0 ? vector<lng>{64, 81, 128, 243, 625, 1024, 2187, 3125, 512 * 9, 2048}[rnd(0, 9)] : rnd(1, it % 3 ? 300 : 60);
        vector<lng> f;
        int kind = it % 6;
        if (kind == 0) { f = {}; }
        if (kind == 1) { int n = int(rnd(1, 7)); for (int i = 0; i < n; ++i) { f.pb(rnd(-5, 5) * (rnd(0, 3) ? 1 : m)); } }
        if (kind >= 2) {
            f = {lng(kind == 5 ? rnd(1, 4) : 1)};
            for (int j = int(rnd(1, 4)); j > 0; --j) {
                lng r = rnd(0, m), mult = rnd(1, 3);
                for (lng t = 0; t < mult; ++t) {
                    vector<lng> g(f.size() + 1, 0);
                    for (int i = 0; i < int(f.size()); ++i) { g[i + 1] = (g[i + 1] + f[i]) % m; g[i] = nrm(g[i] - lll(f[i]) * r, m); }
                    f = g;}}
            if (kind == 4) { for (lng &c : f) { c *= vector<lng>{2, 3, 4, 8, 9}[rnd(0, 4)]; } }
            if (kind == 5) { f.pb(rnd(-3, 3)); }}
        vector<lng> want;
        for (lng x = 0; x < m; ++x) { if (evalm(f, x, m) == 0) { want.pb(x); } }
        string tag = " m=" + std::to_string(m) + " f=";
        for (lng c : f) { tag += std::to_string(c) + ","; }
        check(expand(polynomialCongruenceRoots(f, m, ulng(it)), m, "polynomialCongruenceRoots" + tag) == want, "polynomialCongruenceRoots" + tag);}

    // Large-prime polynomial roots (Cantor-Zassenhaus and Newton lifting): planted roots, classes checked by every Taylor term.
    int casesL = array<int, 3>{40, 300, 1200}[lvl];
    vector<lng> bigP{2305843009213693951LL, 1000000007, 998244353, 1000003, 4611686018427387847LL};
    for (int it = 0; it < casesL; ++it) {
        lng p = bigP[it % bigP.size()], m = it % 3 == 0 && p < 3037000499LL ? p * p : p;
        vector<lng> f{1}, rs;
        for (int j = int(rnd(1, 5)); j > 0; --j) {
            lng r = rnd(0, p - 1), mult = rnd(1, 2);
            rs.pb(r);
            for (lng t = 0; t < mult; ++t) {
                vector<lng> g(f.size() + 1, 0);
                for (int i = 0; i < int(f.size()); ++i) { g[i + 1] = nrm(lll(g[i + 1]) + f[i], m); g[i] = nrm(lll(g[i]) - lll(f[i]) * r, m); }
                f = g;}}
        auto cls = polynomialCongruenceRoots(f, m, ulng(it));
        string tag = show({p, m, it});
        for (auto [r, M] : cls) {
            check(m % M == 0 && 0 <= r && r < M, "polynomialCongruenceRoots large shape" + tag);
            // f(r + t M) = sum_j f^(j)(r) / j! (t M)^j vanishes for every t when each Taylor term does.
            vector<lng> h = f;
            int n = int(h.size());
            for (int i = 0; i < n; ++i) { for (int j = n - 2; j >= i; --j) { h[j] = nrm(lll(h[j]) + lll(r) * h[j + 1], m); } }
            for (int j = 0; j < n; ++j) { check(mulm(h[j], powm(M, ulng(j), m), m) == 0, "polynomialCongruenceRoots large class" + tag); }}
        for (lng r : rs) {
            bool hit = false;
            for (auto [c, M] : cls) { hit |= r % M == c; }
            check(hit, "polynomialCongruenceRoots large planted" + tag);}}

    cout << "PASS 33-modular_square_roots " << checks << " checks seed=" << seed << std::endl;
    return 0;}
