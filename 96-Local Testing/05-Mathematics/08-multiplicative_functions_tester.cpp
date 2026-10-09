#include "../../05-Mathematics/08-multiplicative_functions.hpp"
#include "../../01-Core/05-modint.hpp"

lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
string str(ulll x) {
    string s;
    do { s.push_back(char('0' + x % 10)); x /= 10; } while (x);
    reverse(s.begin(), s.end()); return s;}
vector<pair<ulng, int>> trialFactor(ulng n) {
    vector<pair<ulng, int>> res;
    for (ulng i = 2; i <= n / i; ++i) {
        if (n % i) { continue; }
        res.pb({i, 0});
        while (n % i == 0) { n /= i; ++res.back().se; }}
    if (n > 1) { res.pb({n, 1}); }
    return res;}
vector<ulng> trialDivisors(ulng n) {
    vector<ulng> res;
    for (ulng i = 1; i <= n; ++i) { if (n % i == 0) { res.pb(i); } }
    return res;}
ulng ipow(ulng a, int e) {
    ulng r = 1;
    for (int i = 0; i < e; ++i) { r *= a; }
    return r;}
ulng powMod(ulng a, ulng e, ulng m) {
    ulng r = 1 % m; a %= m;
    for (; e; e >>= 1) {
        if (e & 1) { r = ulng(ulll(r) * a % m); }
        a = ulng(ulll(a) * a % m);}
    return r;}
// Independent point oracles from trial factorization.
struct Brute {
    ulng phi = 1, rad = 1;
    int mu = 1, omega = 0, big = 0;
    Brute(ulng n) {
        for (auto [p, e] : trialFactor(n)) {
            phi *= (p - 1) * ipow(p, e - 1); rad *= p;
            mu = e > 1 ? 0 : -mu; ++omega; big += e;}}
};

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "phi-zero") { phi(0); }
        if (s == "lambda-zero") { carmichaelLambda(0); }
        if (s == "sigma-zero") { sigmaK(0, 1); }
        if (s == "table-negative") { phiTable(-1); }
        if (s == "table-max") { multiplicativeTable<int>(INT_MAX, [](int, int, int) { return 1; }); }
        if (s == "conv-size") { dirichletConvolutionTable(vector<lng>{0, 1}, vector<lng>{0, 1, 2}); }
        if (s == "conv-empty") { dirichletConvolutionTable(vector<lng>{}, vector<lng>{}); }
        if (s == "inverse-small") { dirichletInverseTable(vector<mint>{0}); }
        if (s == "index-zero") { DivisorArray<lng>(12).index(0); }
        if (s == "index-nondivisor") { DivisorArray<lng>(12).index(8); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        ulng n;
        while (cin >> n) {
            cout << phi(n) << ' ' << mobius(n) << ' ' << omega(n) << ' ' << bigOmega(n) << ' ' << liouville(n) << ' ' << carmichaelLambda(n);
            cout << ' ' << str(sigmaK(n, 0)) << ' ' << str(sigmaK(n, 1)) << ' ' << str(sigmaK(n, 3)) << ' ' << str(jordanTotient(n, 2));
            cout << ' ' << sigmaK<mint>(n, 1000000000000000000ULL).val() << ' ' << jordanTotient<mint>(n, 123456789).val() << '\n';}
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    int N = mode == "quick" ? 3000 : mode == "full" ? 30000 : 150000;
    std::mt19937_64 rng(seed);

    for (ulng n = 1; n <= ulng(N); ++n) {
        Brute b(n);
        string tag = " n=" + str(n);
        check(phi(n) == b.phi && mobius(n) == b.mu && omega(n) == b.omega && bigOmega(n) == b.big, "phi/mobius/omega/bigOmega" + tag);
        check(liouville(n) == (b.big % 2 ? -1 : 1), "liouville" + tag);
        if (n <= 400) {
            ulng cnt = 0;
            for (ulng a = 1; a <= n; ++a) { cnt += gcd(a, n) == 1; }
            check(cnt == b.phi, "phi gcd count" + tag);
            ulng lam = 1;
            for (ulng a = 1; a <= n; ++a) {
                if (gcd(a, n) != 1) { continue; }
                ulng ord = 1, x = a % n;
                while (x != 1 % n) { x = x * a % n; ++ord; }
                lam = lcm(lam, ord);}
            check(carmichaelLambda(n) == lam, "carmichaelLambda brute order" + tag);}
        else if (n <= 3000) {
            ulng lam = carmichaelLambda(n);
            bool all = true;
            for (ulng a = 1; a < n && all; ++a) { all = gcd(a, n) != 1 || powMod(a, lam, n) == 1; }
            check(all, "carmichaelLambda exponent" + tag);
            for (auto [q, e] : trialFactor(lam)) {
                bool some = false;
                for (ulng a = 1; a < n && !some; ++a) { some = gcd(a, n) == 1 && powMod(a, lam / q, n) != 1; }
                check(some, "carmichaelLambda minimal" + tag);}}
        if (n <= 2000) {
            auto ds = trialDivisors(n);
            for (ulng k = 0; k <= 3; ++k) {
                ulll s = 0;
                mint sm = 0;
                for (ulng d : ds) { s += ipow(d, int(k)); sm += pow(mint(d), k + 1000000007ULL); }
                check(sigmaK(n, k) == s, "sigmaK k=" + str(k) + tag);
                check(sigmaK<mint>(n, k + 1000000007ULL) == sm, "sigmaK mint" + tag);
                if (k == 0) { continue; }
                ulll js = 0;
                for (ulng d : ds) { js += jordanTotient(d, k); }
                check(js == ulll(ipow(n, int(k))), "jordanTotient inversion k=" + str(k) + tag);}
            if (n <= 60) {
                ulng cnt = 0;
                for (ulng x = 0; x < n; ++x) { for (ulng y = 0; y < n; ++y) { cnt += gcd(gcd(x, y), n) == 1; } }
                check(jordanTotient(n, 2) == cnt && jordanTotient(n, 1) == b.phi, "jordanTotient pair count" + tag);}}}
    cout << "PASS point functions through " << N << '\n';

    for (int n : {0, 1, 2, 3, N}) {
        auto ph = phiTable(n);
        auto mu = mobiusTable(n);
        auto dc = divisorCountTable(n);
        auto ds = divisorSumTable(n);
        auto pp = prefixPhiTable(n);
        auto pm = prefixMobiusTable(n);
        string tag = " table n=" + std::to_string(n);
        check(int(ph.size()) == n + 1 && int(mu.size()) == n + 1 && int(dc.size()) == n + 1 && int(ds.size()) == n + 1 && int(pp.size()) == n + 1 && int(pm.size()) == n + 1, "table sizes" + tag);
        check(ph[0] == 0 && mu[0] == 0 && dc[0] == 0 && ds[0] == 0 && pp[0] == 0 && pm[0] == 0, "table entry 0" + tag);
        vector<int> cnt(n + 1, 0);
        vector<lng> sum(n + 1, 0);
        for (int d = 1; d <= n; ++d) { for (int m = d; m <= n; m += d) { ++cnt[m]; sum[m] += d; } }
        lng sp = 0, sm = 0;
        for (int i = 1; i <= n; ++i) {
            Brute b{ulng(i)};
            sp += lng(b.phi); sm += b.mu;
            check(ph[i] == lng(b.phi) && mu[i] == b.mu && dc[i] == cnt[i] && ds[i] == sum[i], "table value i=" + std::to_string(i) + tag);
            check(pp[i] == sp && pm[i] == sm, "prefix table i=" + std::to_string(i) + tag);}}
    {
        ulng salt = rng();
        auto hashF = [&](int p, int k, int pk) { return (ulng(p) * 0x9e3779b97f4a7c15ULL) ^ (ulng(k) * salt) ^ ulng(pk); };
        auto t = multiplicativeTable<ulng>(N, hashF);
        ulng K = rng();
        auto pw = multiplicativeTable<mint>(N, [&](int, int, int pk) { return pow(mint(pk), K); });
        for (int i = 1; i <= N; ++i) {
            ulng want = 1;
            for (auto [p, e] : trialFactor(ulng(i))) { want *= hashF(int(p), e, int(ipow(p, e))); }
            check(t[i] == want, "multiplicativeTable custom i=" + std::to_string(i));
            check(pw[i] == pow(mint(i), K), "multiplicativeTable power i=" + std::to_string(i));}
        check(multiplicativeTable<ulng>(0, hashF) == vector<ulng>{0} && multiplicativeTable<ulng>(1, hashF) == vector<ulng>{0, 1}, "multiplicativeTable tiny");}
    cout << "PASS tables through " << N << '\n';

    int cn = mode == "quick" ? 300 : mode == "full" ? 3000 : 12000;
    for (int n : {0, 1, 2, 7, cn}) {
        vector<lng> a(n + 1), b(n + 1);
        vector<mint> x(n + 1);
        for (int i = 1; i <= n; ++i) { a[i] = lng(rng() % 2001) - 1000; b[i] = lng(rng() % 2001) - 1000; x[i] = mint(rng()); }
        if (n >= 1) { x[1] = mint(rng() % 998244352 + 1); }
        auto c = dirichletConvolutionTable(a, b);
        check(int(c.size()) == n + 1 && c[0] == 0, "convolution size n=" + std::to_string(n));
        for (int m = 1; m <= n; ++m) {
            lng s = 0;
            for (int d = 1; d <= m; ++d) { if (m % d == 0) { s += a[d] * b[m / d]; } }
            check(c[m] == s, "dirichletConvolutionTable m=" + std::to_string(m));}
        if (n == 0) { continue; }
        vector<ulng> u(n + 1);
        for (int i = 1; i <= n; ++i) { u[i] = i == 1 ? 1 : rng(); }
        auto ui = dirichletInverseTable(u);
        auto xi = dirichletInverseTable(x);
        auto ea = dirichletConvolutionTable(u, ui);
        auto ex = dirichletConvolutionTable(xi, x);
        for (int m = 1; m <= n; ++m) {
            check(ea[m] == ulng(m == 1) && ex[m] == mint(m == 1), "dirichletInverseTable m=" + std::to_string(m) + " n=" + std::to_string(n));}}
    {
        vector<lng> one(cn + 1, 1), mu(cn + 1);
        auto mt = mobiusTable(cn);
        for (int i = 0; i <= cn; ++i) { mu[i] = i ? mt[i] : 0; }
        one[0] = 0;
        check(dirichletInverseTable(one) == mu, "inverse of 1 is mobius");}
    cout << "PASS Dirichlet tables through " << cn << '\n';

    vector<ulng> ns{1, 2, 12, 360, 1024, 720720, 9699690, 999999999989ULL, 600851475143ULL, 897612484786617600ULL, ~0ULL};
    for (int i = 0; i < (mode == "quick" ? 20 : 200); ++i) { ns.pb(rng() % 100000 + 1); ns.pb(rng() >> (rng() % 64)); }
    for (ulng n : ns) {
        if (n == 0) { continue; }
        string tag = " n=" + str(n);
        DivisorArray<lng> da(n);
        DivisorArray<lng> db(factorize(n));
        auto sorted = da.divs;
        sort(sorted.begin(), sorted.end());
        check(sorted == divisors(n) && db.divs == da.divs && da.size() == int(da.divs.size()), "DivisorArray build" + tag);
        if (n <= 100000) { check(sorted == trialDivisors(n), "DivisorArray brute divisors" + tag); }
        int d = da.size();
        for (int i = 0; i < d; ++i) { check(da.index(da.divs[i]) == i, "DivisorArray index" + tag); }
        for (int i = 0; i < d; ++i) { da[i] = lng(rng() % 1000) - 500; }
        auto orig = da.val;
        da.get(da.divs[d - 1]) += 7; orig[d - 1] += 7;
        check(da.val == orig, "DivisorArray get" + tag);
        da.zeta();
        bool brute = d <= 3000;
        if (brute) {
            for (int i = 0; i < d; ++i) {
                lng s = 0;
                for (int j = 0; j < d; ++j) { if (da.divs[i] % da.divs[j] == 0) { s += orig[j]; } }
                check(da[i] == s, "DivisorArray zeta" + tag);}}
        da.mobius();
        check(da.val == orig, "DivisorArray mobius restores" + tag);
        da.multipleZeta();
        if (brute) {
            for (int i = 0; i < d; ++i) {
                lng s = 0;
                for (int j = 0; j < d; ++j) { if (da.divs[j] % da.divs[i] == 0) { s += orig[j]; } }
                check(da[i] == s, "DivisorArray multipleZeta" + tag);}}
        da.multipleMobius();
        check(da.val == orig, "DivisorArray multipleMobius restores" + tag);
        DivisorArray<mint> dm(n);
        dm.setMultiplicative([](ulng p, int, ulng pk) { return mint(pk - pk / p); });
        dm.zeta();
        bool ok = true;
        for (int i = 0; i < d; ++i) { ok &= dm[i] == mint(dm.divs[i]); }
        check(ok, "DivisorArray setMultiplicative phi then zeta gives identity" + tag);
        if (brute && n <= 1000000000000ULL) {
            DivisorArray<lng> dq(n);
            dq.setMultiplicative([](ulng, int k, ulng) { return lng(k == 1 ? -1 : 0); });
            for (int i = 0; i < d; ++i) { check(dq[i] == Brute(dq.divs[i]).mu, "DivisorArray setMultiplicative mobius" + tag); }}}
    DivisorArray<lng> unit;
    check(unit.size() == 1 && unit.divs[0] == 1 && unit.index(1) == 0, "DivisorArray default");
    cout << "PASS DivisorArray on " << ns.size() << " moduli, " << checks << " checks\n";}
