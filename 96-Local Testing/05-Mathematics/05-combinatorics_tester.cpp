#include "../../05-Mathematics/05-combinatorics.hpp"
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/08-infintmini.hpp"
#include "../../01-Core/09-rational.hpp"

void need(bool ok, const string &message) {
    if (!ok) { cerr << "FAIL " << message << '\n'; std::exit(1); }}
template<typename M>
void checkTable(int n) {
    PrimeCombinatorics<M> f(n);
    vector<M> row(n + 1); row[0] = 1; M factorial = 1;
    for (int a = 0; a <= n; ++a) {
        if (a) { factorial *= a; }
        need(f.factorial(a) == factorial && f.factorial(a) * f.inverseFactorial(a) == 1, "factorial/inverse n=" + std::to_string(a));
        for (int b = -1; b <= a + 1; ++b) {
            M expected = 0 <= b && b <= a ? row[b] : M(0);
            need(f.combiNR(a,b) == expected, "Pascal n,k=" + std::to_string(a) + "," + std::to_string(b));}
        for (int b = a + 1; b > 0 && b <= n; --b) { row[b] += row[b - 1]; }}
    vector<M> dfac(n + 1, 1);
    for (int k = 2; k <= n; ++k) { dfac[k] = dfac[k - 2] * k; }
    for (int a = 0; a <= n; ++a) {
        need(f.doubleFactorial(a) == dfac[a], "double factorial running product n=" + std::to_string(a));
        for (int b = 0; b <= a; ++b) {
            string at = " n,k=" + std::to_string(a) + "," + std::to_string(b);
            need(f.combiInverse(a,b) * f.combiNR(a,b) == 1, "inverse binomial times binomial" + at);
            need(f.circularPermutation(a,b) * M(max(b,1)) == f.permuNR(a,b), "circular times length is permutation" + at);
            if (2 * b <= a + 1 && a - b + 1 <= n) { need(f.combiNonAdjacent(a,b) == f.combiNR(a - b + 1,b), "non-adjacent shift identity" + at); }}
        for (int b = 1; a + b <= n; ++b) {
            need(f.combiNegative(a,b) == f.combiNegative(a + 1,b) + f.combiNegative(a + 1,b - 1), "negative binomial Pascal rule n,k=" + std::to_string(a) + "," + std::to_string(b));}}
    auto copy = f; auto moved = std::move(copy);
    need(moved.factorial(n) == f.fac[n] && moved.inv_fac == f.inv_fac && moved.der == f.der, "copy/move table");
    copy.reset(0); need(copy.combiWR(0,0) == 1 && copy.permuWR(0,0) == 1, "reset/zero choice empty count");
    f.reset(n); need(f.factorial(n) == factorial, "repeat reset");
    for (int k = 0; k <= min(n, 8); ++k) {
        vector<int> p(k); iota(p.begin(), p.end(), 0); int count = 0;
        do { bool ok = true; for (int i = 0; i < k; ++i) { ok &= p[i] != i; } count += ok; }
        while (std::next_permutation(p.begin(), p.end()));
        need(f.derangement(k) == M(count), "enumerated derangements n=" + std::to_string(k));}
    FieldCombinatorics<M> g(n);
    for (int a = 0; a <= n; ++a) {
        need(g.factorial(a) == f.factorial(a) && g.inverseFactorial(a) * g.factorial(a) == 1, "field table factorials n=" + std::to_string(a));
        for (int b = -1; b <= a + 1; ++b) {
            need(g.combiNR(a,b) == f.combiNR(a,b) && g.permuNR(a,b) == f.permuNR(a,b), "field table selections n,k=" + std::to_string(a) + "," + std::to_string(b));
            if (b <= 0 || a == 0 || a + b - 1 <= n) { need(g.combiWR(a,b) == f.combiWR(a,b), "field table replacement n,k=" + std::to_string(a) + "," + std::to_string(b)); }}}
    auto gcopy = g; g.reset(0); need(gcopy.n == n && g.n == 0 && g.combiWR(0,0) == 1 && gcopy.multinomial({n}) == 1, "field table copy and reset");
    for (int k = 0; k <= min(n, 40); k += 3) { need(multiFactorial<M>(ulng(k), 2) == f.doubleFactorial(k), "multifactorial step 2 is the double factorial k=" + std::to_string(k)); }}
void smoke() {
    checkTable<ModInt<2>>(1); checkTable<ModInt<97>>(96); checkTable<mint>(250);
    checkTable<ModInt64<18446744073709551557ULL>>(80);
    using D = DynModInt<1205>;
    D::setMod(97); checkTable<D>(96); PrimeCombinatorics<D> table(20);
    D::setMod(97); table.reset(20); need(table.combiNR(20,2) == 93, "same-modulus reset restores table");
    D::setMod(2); table.reset(1); need(table.factorial(1) == 1, "changed-modulus rebuild");
    using E = DynModInt64<1205>;
    E::setMod(18446744073709551557ULL); checkTable<E>(80);
    ModFac legacy(20); need(legacy.combiWR(0,0) == 1 && legacy.permuNR(5,2) == 20, "legacy API");
    need(legacy.combiWR(0,INT_MAX) == 0 && legacy.combiNegative(0,INT_MAX) == 0, "no parts, any count beyond the table");
    need(legacy.ballot(3,1) == 3 && legacy.ballot(3,1,true) == 2 && legacy.starsBars(3,2) == 4 && legacy.starsBars(3,2,true) == 2, "ballot and stars-bars defaults");
    need(legacy.circularPermutation(100,101) == 0 && legacy.combiNonAdjacent(100,51) == 0 && legacy.combiNonAdjacent(INT_MAX,INT_MAX) == 0, "circular and non-adjacent early returns past the table");
    need(legacy.combiNonAdjacent(19,0) == 1 && legacy.combiNonAdjacent(39,20) == 1, "non-adjacent at the table bound");
    FieldCombinatorics<mint> field(5);
    need(field.combiWR(100,0) == 1 && field.combiWR(0,INT_MAX) == 0 && field.combiWR(INT_MAX,-1) == 0, "field replacement early returns past the table");
    LogCombinatorics small(5);
    for (long double v : {small.logBinomial(10,11), small.logPermutation(10,11)}) { need(std::isinf(v) && v < 0, "log early returns past the table"); }
    ulng def = 0; need(ballotExact(3,1,def) && def == 3 && starsBarsExact(3,2,def) && def == 4, "exact ballot and stars-bars defaults");
    need(fibonacciPair<int>(20) == pair<int,int>{6765,10946} && lucas<int>(20) == 15127, "signed32 safe doubling domain");
    ulng alias = 20; need(factorialExact(alias,alias) && alias == 2432902008176640000ULL, "exact output aliases an input");
    alias = 21; need(!factorialExact(alias,alias) && alias == 21, "aliased exact overflow preserves input/output");
    LogCombinatorics lc(30), lc_copy = lc; lc.reset(5); lc.reset(40);
    need(lc.n == 40 && lc.logFactorial(30) == lc_copy.logFactorial(30) && lc.logFactorial(0) == 0 && lc.logFactorial(1) == 0, "log table copy and grow/shrink reset");
    auto moved_lc = std::move(lc_copy); need(moved_lc.logBinomial(30,0) == 0 && std::isinf(moved_lc.logPermutation(2,3)), "log table move");
    lc_copy.reset(2); need(lc_copy.logFactorial(2) == lc.logFactorial(2), "reset moved-from log table");
    ulng dalias = 9; need(doubleFactorialExact(dalias,dalias) && dalias == 945, "double factorial output aliases input");
    need(primorial<InfIntMini>(1) == InfIntMini(1) && primorial<InfInt>(0) == InfInt(1) && primorial<lng>(30) == 6469693230LL, "primorial empty and lng");
    ulng out = 123; need(catalanExact(36,out) && out == 11959798385860453492ULL, "Catalan fits despite overflowing binomial");
    need(!catalanExact(37,out) && out == 11959798385860453492ULL, "Catalan overflow preserves output");
    cout << "PASS static/dynamic tables, copy/move/reset, Pascal and enumerated derangements\n";}
void invalid(const string &s) {
    using D = DynModInt64<1205>;
    if (s == "negative-n") { PrimeCombinatorics<D> f(-1); return; }
    if (s == "intmax-n") { PrimeCombinatorics<D> f(INT_MAX); return; }
    if (s == "modulus-one") { D::setMod(1); PrimeCombinatorics<D> f(0); return; }
    if (s == "composite") { D::setMod(8); PrimeCombinatorics<D> f(3); return; }
    if (s == "prime-flag") { D::setMod(7,0); PrimeCombinatorics<D> f(3); return; }
    if (s == "n-equals-mod") { D::setMod(7); PrimeCombinatorics<D> f(7); return; }
    if (s == "n-above-mod") { D::setMod(7); PrimeCombinatorics<D> f(8); return; }
    PrimeCombinatorics<D> f(20);
    if (s == "stale-modulus") { D::setMod(97); f.combiNR(3,1); }
    else if (s == "stale-modulus-larger") { D::setMod(18446744073709551557ULL); f.combiNR(3,1); }
    else if (s.starts_with("stale-")) {
        D::setMod(97); string op = s.substr(6);
        if (op == "factorial") { f.factorial(3); }
        else if (op == "inverse-factorial") { f.inverseFactorial(3); }
        else if (op == "derangement") { f.derangement(3); }
        else if (op == "double-factorial") { f.doubleFactorial(3); }
        else if (op == "replacement") { f.combiWR(3,0); }
        else if (op == "negative-binomial") { f.combiNegative(3,0); }
        else if (op == "combi-inverse") { f.combiInverse(3,1); }
        else if (op == "permutation") { f.permuNR(3,-1); }
        else if (op == "circular") { f.circularPermutation(3,0); }
        else if (op == "permutation-rep") { f.permuWR(3,2); }
        else if (op == "stars") { f.starsBars(0,0); }
        else if (op == "catalan") { f.catalan(1); }
        else if (op == "ballot") { f.ballot(0,0); }
        else if (op == "multinomial") { f.multinomial({}); }
        else if (op == "large") { f.combiLarge(5,6); }
        else if (op == "nonadjacent") { f.combiNonAdjacent(3,4); }
        else { std::exit(2); }}
    else if (s == "negative-choice") { f.combiNR(-1,0); }
    else if (s == "negative-choice-rep") { f.combiWR(-1,0); }
    else if (s == "negative-permutation") { f.permuNR(-1,0); }
    else if (s == "negative-permutation-rep") { f.permuWR(-1,0); }
    else if (s == "factorial-negative") { f.factorial(-1); }
    else if (s == "factorial-bound") { f.factorial(21); }
    else if (s == "inverse-bound") { f.inverseFactorial(21); }
    else if (s == "derangement-bound") { f.derangement(-1); }
    else if (s == "combination-bound") { f.combiNR(21,1); }
    else if (s == "replacement-bound") { f.combiWR(INT_MAX,INT_MAX); }
    else if (s == "permutation-bound") { f.permuNR(21,1); }
    else if (s == "stars-total") { f.starsBars(-1,2); }
    else if (s == "stars-parts") { f.starsBars(2,-1); }
    else if (s == "stars-bound") { f.starsBars(20,3); }
    else if (s == "catalan-negative") { f.catalan(-1); }
    else if (s == "catalan-bound") { f.catalan(INT_MAX); }
    else if (s == "ballot-negative") { f.ballot(1,-1); }
    else if (s == "ballot-bound") { f.ballot(INT_MAX,1); }
    else if (s == "multinomial-negative") { f.multinomial({1,-1}); }
    else if (s == "multinomial-bound") { f.multinomial({INT_MAX,INT_MAX}); }
    else if (s == "large-bound") { f.combiLarge(100,21); }
    else if (s == "negative-binomial-negative") { f.combiNegative(-1,2); }
    else if (s == "negative-binomial-bound") { f.combiNegative(20,2); }
    else if (s == "inverse-negative") { f.combiInverse(3,-1); }
    else if (s == "inverse-outside") { f.combiInverse(3,4); }
    else if (s == "combi-inverse-bound") { f.combiInverse(21,1); }
    else if (s == "circular-negative") { f.circularPermutation(-1,0); }
    else if (s == "circular-bound") { f.circularPermutation(21,1); }
    else if (s == "double-factorial-negative") { f.doubleFactorial(-1); }
    else if (s == "double-factorial-bound") { f.doubleFactorial(21); }
    else if (s == "primorial-negative") { primorial<D>(-1); }
    else if (s == "log-negative-n") { LogCombinatorics l(-1); }
    else if (s == "log-intmax-n") { LogCombinatorics l(INT_MAX); }
    else if (s == "log-factorial-negative") { LogCombinatorics(5).logFactorial(-1); }
    else if (s == "log-factorial-bound") { LogCombinatorics(5).logFactorial(6); }
    else if (s == "log-binomial-negative") { LogCombinatorics(5).logBinomial(-1,0); }
    else if (s == "log-binomial-bound") { LogCombinatorics(5).logBinomial(6,1); }
    else if (s == "log-permutation-negative") { LogCombinatorics(5).logPermutation(-1,0); }
    else if (s == "log-permutation-bound") { LogCombinatorics(5).logPermutation(6,1); }
    else if (s == "circular-zero-bound") { f.circularPermutation(21,0); }
    else if (s == "nonadjacent-negative") { f.combiNonAdjacent(-1,0); }
    else if (s == "nonadjacent-bound") { f.combiNonAdjacent(21,0); }
    else if (s == "multifactorial-zero-step") { multiFactorial<D>(5,0); }
    else if (s == "multifactorial-exact-zero-step") { ulng out; multiFactorialExact(5,0,out); }
    else if (s == "log-large-factorial-negative") { logFactorialLarge(-1); }
    else if (s == "log-large-binomial-negative") { logBinomialLarge(-1,0); }
    else if (s == "log-large-permutation-negative") { logPermutationLarge(-1,0); }
    else if (s.starts_with("field-")) {
        using F = Rational<lll>; string op = s.substr(6);
        if (op == "negative-n") { FieldCombinatorics<F> g(-1); return; }
        if (op == "intmax-n") { FieldCombinatorics<F> g(INT_MAX); return; }
        FieldCombinatorics<F> g(5);
        if (op == "factorial-negative") { g.factorial(-1); }
        else if (op == "factorial-bound") { g.factorial(6); }
        else if (op == "inverse-negative") { g.inverseFactorial(-1); }
        else if (op == "inverse-bound") { g.inverseFactorial(6); }
        else if (op == "combination-negative") { g.combiNR(-1,0); }
        else if (op == "combination-bound") { g.combiNR(6,1); }
        else if (op == "replacement-negative") { g.combiWR(-1,0); }
        else if (op == "replacement-bound") { g.combiWR(INT_MAX,INT_MAX); }
        else if (op == "permutation-negative") { g.permuNR(-1,0); }
        else if (op == "permutation-bound") { g.permuNR(6,1); }
        else if (op == "multinomial-negative") { g.multinomial({1,-1}); }
        else if (op == "multinomial-bound") { g.multinomial({3,3}); }
        else { std::exit(2); }}
    else if (s == "table-negative") { binomialTable<D>(-1); }
    else if (s == "derangement-table-negative") { derangementTable<D>(-1); }
    else { std::exit(2); }}
void oracle() {
    using M = DynModInt64<1206>; PrimeCombinatorics<M> f; LogCombinatorics lc;
    FieldCombinatorics<Rational<InfIntMini>> qm; FieldCombinatorics<Rational<InfInt>> qf; FieldCombinatorics<M> qp;
    string op; cout << std::setprecision(21);
    while (cin >> op) {
        if (op == "setup") { ulng p; int n; cin >> p >> n; M::setMod(p); f.reset(n); cout << "1\n"; continue; }
        if (op == "fs") { ulng p; int n; cin >> p >> n; M::setMod(p); qm.reset(n); qf.reset(n); qp.reset(n); cout << "1\n"; continue; }
        if (op == "ff") { int k; cin >> k; cout << qm.factorial(k) << ' ' << qm.inverseFactorial(k) << ' ' << qf.factorial(k) << ' ' << qf.inverseFactorial(k) << ' ' << qp.factorial(k) << ' ' << qp.inverseFactorial(k) << '\n'; continue; }
        if (op == "fc" || op == "fr" || op == "fp") {
            int a,b; cin >> a >> b;
            if (op == "fc") { cout << qm.combiNR(a,b) << ' ' << qf.combiNR(a,b) << ' ' << qp.combiNR(a,b) << '\n'; }
            else if (op == "fr") { cout << qm.combiWR(a,b) << ' ' << qf.combiWR(a,b) << ' ' << qp.combiWR(a,b) << '\n'; }
            else { cout << qm.permuNR(a,b) << ' ' << qf.permuNR(a,b) << ' ' << qp.permuNR(a,b) << '\n'; }
            continue;}
        if (op == "fm") { int k; cin >> k; vector<int> v(k); for (int &x : v) { cin >> x; } cout << qm.multinomial(v) << ' ' << qf.multinomial(v) << ' ' << qp.multinomial(v) << '\n'; continue; }
        if (op == "mj") { int a,b; cin >> a >> b; cout << f.combiNonAdjacent(a,b) << '\n'; continue; }
        if (op == "mu") {
            ulng a,b; cin >> a >> b;
            cout << multiFactorial<M>(a,b) << ' ' << multiFactorial<ulng>(a,b) << ' ' << multiFactorial<InfIntMini>(a,b).toString(16) << ' ' << multiFactorial<InfInt>(a,b).toString(16) << '\n'; continue;}
        if (op == "eu") { ulng a,b,out = 123; cin >> a >> b; bool ok = multiFactorialExact(a,b,out); cout << ok << ' ' << out << '\n'; continue; }
        if (op == "LF") { lng k; cin >> k; cout << logFactorialLarge(k) << '\n'; continue; }
        if (op == "LB" || op == "LP") { lng a,b; cin >> a >> b; cout << (op == "LB" ? logBinomialLarge(a,b) : logPermutationLarge(a,b)) << '\n'; continue; }
        if (op == "ring") { ulng p; cin >> p; M::setMod(p,0); cout << "1\n"; continue; }
        if (op == "mf") { int k; cin >> k; cout << f.factorial(k) << ' ' << f.inverseFactorial(k) << ' ' << f.derangement(k); }
        else if (op == "mc" || op == "mr" || op == "mp" || op == "mq") {
            int a,b; cin >> a >> b;
            cout << (op == "mc" ? f.combiNR(a,b) : op == "mr" ? f.combiWR(a,b) : op == "mp" ? f.permuNR(a,b) : f.permuWR(a,b));}
        else if (op == "ms" || op == "mb") { int a,b,s; cin >> a >> b >> s; cout << (op == "ms" ? f.starsBars(a,b,s) : f.ballot(a,b,s)); }
        else if (op == "mn" || op == "mi" || op == "mo") {
            int a,b; cin >> a >> b;
            cout << (op == "mn" ? f.combiNegative(a,b) : op == "mi" ? f.combiInverse(a,b) : f.circularPermutation(a,b));}
        else if (op == "md") { int k; cin >> k; cout << f.doubleFactorial(k); }
        else if (op == "pr") { int n; cin >> n; cout << primorial<InfIntMini>(n).toString(16) << ' ' << primorial<InfInt>(n).toString(16) << ' ' << primorial<M>(n) << ' ' << primorial<ulng>(n); }
        else if (op == "pb") { int n; cin >> n; cout << primorial<InfInt>(n).toString(16); }
        else if (op == "ls") { int n; cin >> n; lc.reset(n); cout << 1; }
        else if (op == "lf") { int k; cin >> k; cout << lc.logFactorial(k); }
        else if (op == "lb" || op == "lp") { int a,b; cin >> a >> b; cout << (op == "lb" ? lc.logBinomial(a,b) : lc.logPermutation(a,b)); }
        else if (op == "ml") { ulng a; int b; cin >> a >> b; cout << f.combiLarge(a,b); }
        else if (op == "rd" || op == "ud") {
            int n; cin >> n;
            if (op == "rd") { for (M x : derangementTable<M>(n)) { cout << x << ' '; } }
            else { for (ulng x : derangementTable<ulng>(n)) { cout << x << ' '; } }}
        else if (op == "rb") { int n; cin >> n; for (auto &row : binomialTable<M>(n)) { for (M x : row) { cout << x << ' '; } } }
        else if (op == "mt") { int n; cin >> n; cout << f.catalan(n); }
        else if (op == "mm") { int n; cin >> n; vector<int> v(n); for (int &x : v) { cin >> x; } cout << f.multinomial(v); }
        else if (op == "mg") { ulng n; cin >> n; auto [a,b] = fibonacciPair<M>(n); cout << a << ' ' << b << ' ' << lucas<M>(n); }
        else if (op == "rg") { ulng n; cin >> n; auto [a,b] = fibonacciPair<ulng>(n); cout << a << ' ' << b << ' ' << lucas<ulng>(n); }
        else if (op == "rg32") { ulng n; cin >> n; auto [a,b] = fibonacciPair<uint>(n); cout << a << ' ' << b << ' ' << lucas<uint>(n); }
        else if (op == "eg") {
            ulng n,a=123,b=123; pair<ulng,ulng> pair{123,456}; cin >> n;
            bool p = fibonacciPairExact(n,pair), x = fibonacciExact(n,a), y = lucasExact(n,b);
            cout << p << ' ' << pair.first << ' ' << pair.second << ' ' << x << ' ' << a << ' ' << y << ' ' << b;}
        else {
            ulng a=0,b=0,out=123; bool ok = false;
            if (op == "em") { int k; cin >> k; vector<ulng> v(k); for (ulng &x : v) { cin >> x; } ok = multinomialExact(v,out); }
            else if (op == "ef" || op == "ed" || op == "et" || op == "ex") { cin >> a; ok = op == "ef" ? factorialExact(a,out) : op == "ed" ? derangementExact(a,out) : op == "et" ? catalanExact(a,out) : doubleFactorialExact(a,out); }
            else { cin >> a >> b;
                if (op == "es" || op == "eb") { int s; cin >> s; ok = op == "es" ? starsBarsExact(a,b,out,s) : ballotExact(a,b,out,s); }
                else if (op == "ec") { ok = combiExact(a,b,out); }
                else if (op == "er") { ok = combiRepExact(a,b,out); }
                else if (op == "ep") { ok = permuExact(a,b,out); }
                else if (op == "eq") { ok = permuRepExact(a,b,out); }
                else { std::exit(2); }}
            cout << ok << ' ' << out;}
        cout << '\n';}}
int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") { invalid(argv[2]); return 0; }
    if (argc > 1 && string(argv[1]) == "--oracle") { oracle(); return 0; }
    smoke();}
