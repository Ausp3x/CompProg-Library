#include "../../05-Mathematics/07-primality_factorization.hpp"

lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
string str(ulll x) {
    string s;
    do { s.push_back(char('0' + x % 10)); x /= 10; } while (x);
    reverse(s.begin(), s.end()); return s;}
bool trialPrime(ulng n) {
    if (n < 2) { return false; }
    for (ulng i = 2; i <= n / i; ++i) { if (n % i == 0) { return false; } }
    return true;}
vector<pair<ulng, int>> trialFactor(ulng n) {
    vector<pair<ulng, int>> res;
    for (ulng i = 2; i <= n / i; ++i) {
        if (n % i) { continue; }
        res.pb({i, 0});
        while (n % i == 0) { n /= i; ++res.back().se; }}
    if (n > 1) { res.pb({n, 1}); }
    return res;}
vector<ulng> trialDivisors(ulng n) {
    vector<ulng> lo, hi;
    for (ulng i = 1; i <= n / i; ++i) {
        if (n % i) { continue; }
        lo.pb(i);
        if (i != n / i) { hi.pb(n / i); }}
    lo.insert(lo.end(), hi.rbegin(), hi.rend()); return lo;}

// Coarsest base: group primes by their primitive exponent vector over the list.
vector<ulng> coarsestBase(const vector<ulng> &a) {
    int k = int(a.size());
    map<ulng, vector<int>> vec;
    for (int i = 0; i < k; ++i) {
        for (auto [p, e] : trialFactor(a[i])) { vec[p].resize(k); vec[p][i] = e; }}
    map<vector<int>, ulng> group;
    for (auto &[p, w] : vec) {
        w.resize(k);
        int g = 0;
        for (int x : w) { g = gcd(g, x); }
        for (int &x : w) { x /= g; }
        ulng pk = 1;
        for (int i = 0; i < g; ++i) { pk *= p; }
        auto it = group.find(w);
        if (it == group.end()) { group[w] = pk; }
        else { it->se *= pk; }}
    vector<ulng> res;
    for (auto &[w, x] : group) { res.pb(x); }
    sort(res.begin(), res.end()); return res;}
void checkComposite(ulng n, const vector<ulng> &ps) {
    string tag = " n=" + str(n);
    check(!isPrime(n) && !millerRabin64(n) && !isPrimeCompact(n) && !millerRabin64Compact(n), "composite accepted" + tag);
    vector<ulng> got = primeFactors(n), want = ps;
    sort(want.begin(), want.end());
    check(got == want, "factorize composite" + tag);
    vector<ulng> compact;
    for (auto [p, e] : factorizeCompact(n)) { compact.insert(compact.end(), e, p); }
    check(compact == want, "factorizeCompact composite" + tag);
    ulng d = pollardRhoBrent(n, 7);
    check(d > 1 && d < n && n % d == 0, "pollardRhoBrent proper divisor" + tag);
    d = pollardRhoBrentCompact(n, 7);
    check(d > 1 && d < n && n % d == 0, "pollardRhoBrentCompact proper divisor" + tag);}

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "factorize-zero") { factorize(0); }
        if (s == "compact-zero") { factorizeCompact(0); }
        if (s == "trial-zero") { factorizeTrial(0); }
        if (s == "primefactors-zero") { primeFactors(0); }
        if (s == "divisors-zero") { divisors(ulng(0)); }
        if (s == "count-zero") { divisorCount(0); }
        if (s == "sum-zero") { divisorSum(0); }
        if (s == "squarefree-zero") { isSquarefree(0); }
        if (s == "radical-zero") { radical(0); }
        if (s == "base-zero") { coprimeBase({3, 0}); }
        if (s == "hcn-zero") { maxDivisorCount(0); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op; ulng n;
        while (cin >> op >> n) {
            if (op == "p") {
                cout << isPrime(n) << ' ' << millerRabin64(n) << ' ' << pollardRhoBrent(n, n ^ 99) << ' ';
                cout << isPrimeCompact(n) << ' ' << millerRabin64Compact(n) << ' ' << pollardRhoBrentCompact(n, n ^ 99) << '\n';
                continue;}
            if (op == "t") { cout << isPrimeTrial(n) << '\n'; continue; }
            auto f = factorize(n), fc = factorizeCompact(n);
            cout << fc.size();
            for (auto [p, e] : fc) { cout << ' ' << p << ' ' << e; }
            cout << ' ';
            cout << f.size();
            for (auto [p, e] : f) { cout << ' ' << p << ' ' << e; }
            cout << ' ' << divisorCount(n) << ' ' << str(divisorSum(n)) << ' ' << isPrimePower(n) << ' ' << isSquarefree(n) << ' ' << radical(n);
            auto pf = primeFactors(n);
            cout << ' ' << pf.size();
            for (ulng p : pf) { cout << ' ' << p; }
            if (op == "d") {
                auto ds = divisors(n);
                cout << ' ' << ds.size();
                for (ulng d : ds) { cout << ' ' << d; }}
            cout << '\n';}
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    int lim = mode == "quick" ? 100000 : mode == "full" ? 2000000 : 10000000;
    int flim = mode == "quick" ? 20000 : mode == "full" ? 200000 : 1000000;
    std::mt19937_64 rng(seed);

    vector<char> sv(lim + 1, 1);
    sv[0] = sv[1] = 0;
    for (int i = 2; lng(i) * i <= lim; ++i) { if (sv[i]) { for (int j = i * i; j <= lim; j += i) { sv[j] = 0; } } }
    LinearSieve ls(lim / 2);
    for (int n = 0; n <= lim; ++n) {
        bool want = sv[n], spf = isPrimeSpf(ulng(n), ls);
        string tag = " n=" + std::to_string(n);
        check(isPrime(ulng(n)) == want && millerRabin64(ulng(n)) == want && spf == want, "primality" + tag);
        check(isPrimeCompact(ulng(n)) == want && millerRabin64Compact(ulng(n)) == want, "Compact primality" + tag);
        if (n <= flim) { check(isPrimeTrial(ulng(n)) == want, "isPrimeTrial" + tag); }}
    cout << "PASS primality exhaustive through " << lim << '\n';

    vector<lng> dcount(flim + 1, 0);
    for (int d = 1; d <= flim; ++d) { for (int m = d; m <= flim; m += d) { ++dcount[m]; } }
    pair<ulng, lng> best{1, 1};
    for (int n = 1; n <= flim; ++n) {
        ulng u = ulng(n);
        string tag = " n=" + std::to_string(n);
        auto f = trialFactor(u);
        check(factorize(u) == f && factorizeTrial(u) == f && factorizeCompact(u) == f, "factorize" + tag);
        vector<ulng> pf;
        ulng rad = 1; lng cnt = 1; ulll sum = 1; bool sqf = true;
        for (auto [p, e] : f) {
            pf.insert(pf.end(), e, p); rad *= p; cnt *= e + 1; sqf &= e == 1;
            ulll s = 1, pk = 1;
            for (int k = 0; k < e; ++k) { pk *= p; s += pk; }
            sum *= s;}
        auto ds = trialDivisors(u);
        ulll dsum = 0;
        for (ulng d : ds) { dsum += d; }
        check(lng(ds.size()) == dcount[n] && dsum == sum, "brute divisor oracle" + tag);
        check(primeFactors(u) == pf && divisors(u) == ds && divisors(f) == ds, "primeFactors/divisors" + tag);
        check(divisorCount(u) == cnt && divisorSum(u) == dsum, "divisorCount/divisorSum" + tag);
        check(isPrimePower(u) == (f.size() == 1 ? f[0].fi : 0), "isPrimePower" + tag);
        check(isSquarefree(u) == sqf && radical(u) == rad, "isSquarefree/radical" + tag);
        ulng d = pollardRhoBrent(u, seed + u);
        if (n >= 4 && !sv[n]) { check(d > 1 && d < u && u % d == 0, "pollardRhoBrent" + tag); }
        else { check(d == 0, "pollardRhoBrent no factor" + tag); }
        check(pollardRhoBrentCompact(u, seed + u) == d && pollardRhoBrentCompact(u, seed, 1) == pollardRhoBrent(u, seed, 1), "pollardRhoBrentCompact matches the fast divisor" + tag);
        d = pollardRhoBrentCompact(u, seed + u);
        if (n >= 4 && !sv[n]) { check(d > 1 && d < u && u % d == 0, "pollardRhoBrentCompact" + tag); }
        else { check(d == 0, "pollardRhoBrentCompact no factor" + tag); }
        if (dcount[n] > best.se) { best = {u, dcount[n]}; }
        if (n <= 20000 || n % 101 == 0) { check(maxDivisorCount(u) == best, "maxDivisorCount" + tag); }}
    cout << "PASS factorization family exhaustive through " << flim << '\n';

    for (ulng b : {2ULL, 7ULL, 61ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
        for (ulng d : trialDivisors(b)) {
            bool want = trialPrime(d);
            check(isPrime(d) == want && millerRabin64(d) == want && isPrimeCompact(d) == want && millerRabin64Compact(d) == want, "base divisor n=" + str(d));}}
    for (ulng n : {2047ULL, 1373653ULL, 25326001ULL, 3215031751ULL, 2152302898747ULL, 3474749660383ULL, 341550071728321ULL,
                   3825123056546413051ULL, 4759123141ULL, 1122004669633ULL, 561ULL, 41041ULL, 825265ULL, 321197185ULL}) {
        check(!isPrime(n) && !millerRabin64(n) && !isPrimeTrial(n) && !isPrimeCompact(n) && !millerRabin64Compact(n), "pseudoprime accepted n=" + str(n));
        auto f = trialFactor(n);
        check(factorize(n) == f && factorizeTrial(n) == f && factorizeCompact(n) == f, "pseudoprime factorization n=" + str(n));}
    int carmichael = 0;
    for (ulng k = 1; carmichael < (mode == "quick" ? 30 : 400) && k < 240000; ++k) {
        ulng p = 6 * k + 1, q = 12 * k + 1, r = 18 * k + 1;
        if (!trialPrime(p) || !trialPrime(q) || !trialPrime(r)) { continue; }
        ++carmichael; checkComposite(p * q * r, {p, q, r});}
    cout << "PASS " << carmichael << " Chernick Carmichael numbers and base-divisor/pseudoprime regressions\n";

    vector<ulng> bigp;
    for (ulng x = (1ULL << 32) - 1; bigp.size() < 6; --x) { if (trialPrime(x)) { bigp.pb(x); } }
    for (ulng p : bigp) {
        check(!isPrime(p * p) && isPrimePower(p * p) == p && !isSquarefree(p * p) && radical(p * p) == p, "prime square p=" + str(p));
        for (ulng q : bigp) { if (q < p) { checkComposite(p * q, {p, q}); } }}
    for (ulng x = ~0ULL, found = 0; found < 3; --x) {
        if (!isPrime(x)) { continue; }
        ++found;
        check(factorize(x) == vector<pair<ulng, int>>{{x, 1}} && isPrimePower(x) == x && pollardRhoBrent(x) == 0, "large prime n=" + str(x));
        check(isPrimeCompact(x) && factorizeCompact(x) == vector<pair<ulng, int>>{{x, 1}} && pollardRhoBrentCompact(x) == 0, "Compact large prime n=" + str(x));
        check(divisors(x) == vector<ulng>{1, x} && divisorSum(x) == ulll(x) + 1, "large prime divisors n=" + str(x));}
    for (ulng p : {2ULL, 3ULL, 5ULL, 7ULL, 61ULL, 65537ULL, 4294967291ULL}) {
        ulng pk = p;
        for (int k = 1;; ++k) {
            check(isPrimePower(pk) == p && factorize(pk) == vector<pair<ulng, int>>{{p, k}} && factorizeCompact(pk) == factorize(pk), "prime power p=" + str(p) + " k=" + std::to_string(k));
            check(isPrimePower(pk + 1) != p || pk + 1 == 2, "prime power neighbour p=" + str(p));
            if (pk > ~0ULL / p) { break; }
            pk *= p;}}
    check(pollardRhoBrent(91, 3, 0) == 0 && pollardRhoBrent(1) == 0 && pollardRhoBrent(3) == 0, "rho budget and small n");
    check(pollardRhoBrentCompact(91, 3, 0) == 0 && pollardRhoBrentCompact(1) == 0 && pollardRhoBrentCompact(3) == 0, "Compact rho budget and small n");
    for (int i = 0; i < 50; ++i) {
        ulng n = bigp[i % 6] * bigp[(i + 1) % 6], s = rng();
        check(pollardRhoBrent(n, s) == pollardRhoBrent(n, s) && pollardRhoBrentCompact(n, s) == pollardRhoBrent(n, s), "rho reproducible and Compact identical");}
    cout << "PASS large primes, prime powers, semiprimes and rho seeds\n";

    int rounds = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
    for (int it = 0; it < rounds; ++it) {
        int k = int(rng() % 7);
        vector<ulng> a;
        for (int i = 0; i < k; ++i) {
            ulng x = 1;
            int parts = int(rng() % 8);
            for (int j = 0; j < parts; ++j) {
                ulng p = it % 3 == 0 ? bigp[rng() % 6] : ulng(2 + rng() % 40);
                if (x <= ~0ULL / p / p) { x *= p; }}
            a.pb(x);}
        auto b = coprimeBase(a);
        string tag = " list size " + std::to_string(k) + " round " + std::to_string(it);
        for (int i = 0; i < int(b.size()); ++i) {
            check(b[i] > 1 && (i == 0 || b[i - 1] < b[i]), "coprimeBase sorted > 1" + tag);
            for (int j = 0; j < i; ++j) { check(gcd(b[i], b[j]) == 1, "coprimeBase pairwise coprime" + tag); }
            bool divides = false;
            for (ulng x : a) { divides |= x % b[i] == 0; }
            check(divides, "coprimeBase element divides an input" + tag);}
        for (ulng x : a) {
            for (ulng q : b) { while (x % q == 0) { x /= q; } }
            check(x == 1, "coprimeBase spans input" + tag);}
        if (it % 3) { check(b == coarsestBase(a), "coprimeBase equals the coarsest base" + tag); }}
    check(coprimeBase({}).empty() && coprimeBase({1, 1}).empty() && coprimeBase({6, 10}) == vector<ulng>{2, 3, 5}, "coprimeBase fixed cases");
    check(coprimeBase({12, 18}) == vector<ulng>{2, 3} && coprimeBase({ulng(1e18), ulng(1e18)}) == vector<ulng>{ulng(1e18)} && coprimeBase({4, 8}) == vector<ulng>{2}, "coprimeBase duplicates");
    check(maxDivisorCount(1000000000000000000ULL) == pair<ulng, lng>{897612484786617600ULL, 103680}, "maxDivisorCount 1e18");
    check(maxDivisorCount(~0ULL) == pair<ulng, lng>{18401055938125660800ULL, 184320}, "maxDivisorCount 2^64 - 1");
    check(maxDivisorCount(1) == pair<ulng, lng>{1, 1}, "maxDivisorCount 1");
    cout << "PASS coprimeBase and maxDivisorCount, " << checks << " checks\n";}
