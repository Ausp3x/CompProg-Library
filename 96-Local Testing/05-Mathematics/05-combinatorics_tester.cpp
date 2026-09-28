#include "../../05-Mathematics/05-combinatorics.hpp"

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
            need(f.combiNR(a,b) == expected, "Pascal n,k=" + std::to_string(a) + "," + std::to_string(b)); }
        for (int b = a + 1; b > 0 && b <= n; --b) { row[b] += row[b - 1]; }}
    auto copy = f; auto moved = std::move(copy);
    need(moved.factorial(n) == f.fac[n] && moved.inv_fac == f.inv_fac && moved.der == f.der, "copy/move table");
    copy.reset(0); need(copy.combiWR(0,0) == 1 && copy.permuWR(0,0) == 1, "reset/zero choice empty count");
    f.reset(n); need(f.factorial(n) == factorial, "repeat reset");
    for (int k = 0; k <= min(n, 8); ++k) {
        vector<int> p(k); iota(p.begin(), p.end(), 0); int count = 0;
        do { bool ok = true; for (int i = 0; i < k; ++i) { ok &= p[i] != i; } count += ok; }
        while (std::next_permutation(p.begin(), p.end()));
        need(f.derangement(k) == M(count), "enumerated derangements n=" + std::to_string(k)); }}
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
    need(fibonacciPair<int>(20) == pair<int,int>{6765,10946} && lucas<int>(20) == 15127, "signed32 safe doubling domain");
    ulng alias = 20; need(factorialExact(alias,alias) && alias == 2432902008176640000ULL, "exact output aliases an input");
    alias = 21; need(!factorialExact(alias,alias) && alias == 21, "aliased exact overflow preserves input/output");
    ulng out = 123; need(catalanExact(36,out) && out == 11959798385860453492ULL, "Catalan fits despite overflowing binomial");
    need(!catalanExact(37,out) && out == 11959798385860453492ULL, "Catalan overflow preserves output");
    cout << "PASS static/dynamic tables, copy/move/reset, Pascal and enumerated derangements\n"; }
void invalid(const string &s) {
    using D = DynModInt64<1205>;
    if (s == "negative-n") { PrimeCombinatorics<D> f(-1); return; }
    if (s == "intmax-n") { PrimeCombinatorics<D> f(INT_MAX); return; }
    if (s == "modulus-one") { D::setMod(1); PrimeCombinatorics<D> f(0); return; }
    if (s == "composite") { D::setMod(8); PrimeCombinatorics<D> f(3); return; }
    if (s == "prime-flag") { D::setMod(7,0); PrimeCombinatorics<D> f(3); return; }
    if (s == "n-equals-mod") { D::setMod(7); PrimeCombinatorics<D> f(7); return; }
    PrimeCombinatorics<D> f(20);
    if (s == "stale-modulus") { D::setMod(97); f.combiNR(3,1); }
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
    else { std::exit(2); }}
void oracle() {
    using M = DynModInt64<1206>; PrimeCombinatorics<M> f;
    string op;
    while (cin >> op) {
        if (op == "setup") { ulng p; int n; cin >> p >> n; M::setMod(p); f.reset(n); cout << "1\n"; continue; }
        if (op == "ring") { ulng p; cin >> p; M::setMod(p,0); cout << "1\n"; continue; }
        if (op == "mf") { int k; cin >> k; cout << f.factorial(k) << ' ' << f.inverseFactorial(k) << ' ' << f.derangement(k); }
        else if (op == "mc" || op == "mr" || op == "mp" || op == "mq") {
            int a,b; cin >> a >> b;
            cout << (op == "mc" ? f.combiNR(a,b) : op == "mr" ? f.combiWR(a,b) : op == "mp" ? f.permuNR(a,b) : f.permuWR(a,b)); }
        else if (op == "ms" || op == "mb") { int a,b,s; cin >> a >> b >> s; cout << (op == "ms" ? f.starsBars(a,b,s) : f.ballot(a,b,s)); }
        else if (op == "mt") { int n; cin >> n; cout << f.catalan(n); }
        else if (op == "mm") { int n; cin >> n; vector<int> v(n); for (int &x : v) { cin >> x; } cout << f.multinomial(v); }
        else if (op == "mg") { ulng n; cin >> n; auto [a,b] = fibonacciPair<M>(n); cout << a << ' ' << b << ' ' << fibonacci<M>(n) << ' ' << lucas<M>(n); }
        else if (op == "rg") { ulng n; cin >> n; auto [a,b] = fibonacciPair<ulng>(n); cout << a << ' ' << b << ' ' << fibonacci<ulng>(n) << ' ' << lucas<ulng>(n); }
        else if (op == "rg32") { ulng n; cin >> n; auto [a,b] = fibonacciPair<uint>(n); cout << a << ' ' << b << ' ' << fibonacci<uint>(n) << ' ' << lucas<uint>(n); }
        else if (op == "eg") {
            ulng n,a=123,b=123; pair<ulng,ulng> pair{123,456}; cin >> n;
            bool p = fibonacciPairExact(n,pair), x = fibonacciExact(n,a), y = lucasExact(n,b);
            cout << p << ' ' << pair.first << ' ' << pair.second << ' ' << x << ' ' << a << ' ' << y << ' ' << b; }
        else {
            ulng a=0,b=0,out=123; bool ok = false;
            if (op == "em") { int k; cin >> k; vector<ulng> v(k); for (ulng &x : v) { cin >> x; } ok = multinomialExact(v,out); }
            else if (op == "ef" || op == "ed" || op == "et") { cin >> a; ok = op == "ef" ? factorialExact(a,out) : op == "ed" ? derangementExact(a,out) : catalanExact(a,out); }
            else { cin >> a >> b;
                if (op == "es" || op == "eb") { int s; cin >> s; ok = op == "es" ? starsBarsExact(a,b,out,s) : ballotExact(a,b,out,s); }
                else if (op == "ec") { ok = combiExact(a,b,out); }
                else if (op == "er") { ok = combiRepExact(a,b,out); }
                else if (op == "ep") { ok = permuExact(a,b,out); }
                else if (op == "eq") { ok = permuRepExact(a,b,out); }
                else { std::exit(2); }}
            cout << ok << ' ' << out; }
        cout << '\n'; }}
int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") { invalid(argv[2]); return 0; }
    if (argc > 1 && string(argv[1]) == "--oracle") { oracle(); return 0; }
    smoke(); }
