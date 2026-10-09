#include "../../05-Mathematics/09-modinverse.hpp"

lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
bool brutePrime(lng p) {
    if (p < 2) { return false; }
    for (lng i = 2; i * i <= p; ++i) { if (p % i == 0) { return false; } }
    return true;}
template<typename M>
void tableCheck(int n, const string &name) {
    auto t = inverseTable<M>(n);
    check(int(t.size()) == n + 1 && t[0] == M(0), "inverseTable size " + name);
    for (int i = 1; i <= n; ++i) { check(t[i] * M(i) == M(1), "inverseTable " + name + " i=" + std::to_string(i)); }}
template<typename M>
void batchCheck(std::mt19937_64 &rng, int n, int zeros, const string &name) {
    vector<M> a(n);
    for (auto &x : a) { do { x = M(rng()); } while (x == M(0)); }
    for (int i = 0; i < zeros && n; ++i) { a[rng() % n] = M(0); }
    auto list = inverseList(a), orig = a;
    for (int i = 0; i < n; ++i) { check(a[i] == M(0) ? list[i] == M(0) : list[i] * a[i] == M(1), "inverseList " + name); }
    bool ok = inverseBatch(a);
    check(ok == (zeros == 0 || n == 0), "inverseBatch status " + name);
    for (int i = 0; i < n; ++i) { check(ok ? a[i] * orig[i] == M(1) : a[i] == orig[i], "inverseBatch values " + name); }}

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "xgcd-zero") { inverseXgcd(1, 0); }
        if (s == "fermat-one") { inverseFermat(1, 1); }
        if (s == "pierce-zero") { inversePierce(1, 0); }
        if (s == "table-negative") { inverseTable<mint>(-1); }
        if (s == "table-modulus") { inverseTable<ModInt<13>>(13); }
        if (s == "table-composite") { inverseTable<ModInt<12>>(5); }
        if (s == "list-composite") { inverseList(vector<ModInt<12>>{5}); }
        if (s == "power-negative") { powerTable<mint>(-1, 2); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op; lng a, m;
        while (cin >> op >> a >> m) {
            if (op == "x") { cout << inverseXgcd(a, m) << '\n'; }
            else { cout << inverseFermat(a, m) << ' ' << inversePierce(a, m) << '\n'; }}
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    int N = mode == "quick" ? 20000 : mode == "full" ? 1000000 : 5000000;
    std::mt19937_64 rng(seed);

    for (lng m = 1; m <= 120; ++m) {
        for (lng a = -3 * m; a <= 3 * m; ++a) {
            lng want = -1;
            for (lng x = 0; x < m && want < 0; ++x) { if ((((a % m) + m) % m * x) % m == 1 % m) { want = x; } }
            string tag = " a=" + std::to_string(a) + " m=" + std::to_string(m);
            check(inverseXgcd(a, m) == want, "inverseXgcd" + tag);
            if (brutePrime(m)) {
                lng w = a % m == 0 ? -1 : want;
                check(inverseFermat(a, m) == w && inversePierce(a, m) == w, "inverseFermat/inversePierce" + tag);}}}
    for (lng a : {LLONG_MIN, LLONG_MIN + 1, -1LL, 0LL, 1LL, LLONG_MAX}) {
        for (lng m : {1LL, 2LL, 3LL, LLONG_MAX, 9223372036854775783LL}) {
            lng x = inverseXgcd(a, m);
            lll r = lll(a) % m;
            if (r < 0) { r += m; }
            check(x == -1 ? gcd(r, lll(m)) != 1 : x >= 0 && x < m && r * x % m == 1 % m, "inverseXgcd extreme a=" + std::to_string(a) + " m=" + std::to_string(m));}}
    cout << "PASS scalar inverses exhaustive through m = 120\n";

    tableCheck<mint>(N, "998244353");
    tableCheck<ModInt<13>>(12, "13");
    tableCheck<ModInt<2>>(1, "2");
    tableCheck<ModInt61>(N / 10, "2^61-1");
    DynModInt<1>::setMod(1000000007);
    tableCheck<DynModInt<1>>(N / 10, "dynamic 1e9+7");
    tableCheck<mint>(0, "empty");
    cout << "PASS inverseTable\n";

    for (int n : {0, 1, 2, 17, 1000, N / 10}) {
        for (int zeros : {0, 1, 3}) {
            batchCheck<mint>(rng, n, zeros, "mint n=" + std::to_string(n));
            batchCheck<ModInt<13>>(rng, n, zeros, "13 n=" + std::to_string(n));}}
    DynModInt<2>::setMod(1000);
    vector<DynModInt<2>> units{1, 3, 7, 9, 11, 999}, bad{3, 10, 7};
    auto copy = units;
    check(inverseBatch(units), "inverseBatch composite modulus");
    for (int i = 0; i < int(units.size()); ++i) { check(units[i] * copy[i] == 1, "inverseBatch composite values"); }
    auto badCopy = bad;
    check(!inverseBatch(bad) && bad == badCopy, "inverseBatch composite nonunit leaves input");
    vector<mint> empty;
    check(inverseBatch(empty) && empty.empty() && inverseList(empty).empty(), "empty batch/list");
    cout << "PASS inverseBatch and inverseList\n";

    for (ulng k : {0ULL, 1ULL, 2ULL, 5ULL, 998244352ULL, ~0ULL}) {
        for (int n : {0, 1, 2, 3, N / 10}) {
            auto t = powerTable<mint>(n, k);
            check(int(t.size()) == n + 1, "powerTable size");
            for (int i = 0; i <= n; ++i) { check(t[i] == pow(mint(i), k), "powerTable mint i=" + std::to_string(i) + " k=" + std::to_string(k)); }
            auto c = powerTable<DynModInt<2>>(n, k);
            for (int i = 0; i <= n; ++i) {
                ulng want = 1 % 1000, b = ulng(i) % 1000;
                for (ulng e = k; e; e >>= 1, b = b * b % 1000) { if (e & 1) { want = want * b % 1000; } }
                check(c[i].val() == want, "powerTable composite i=" + std::to_string(i));}}}
    DynModInt<3>::setMod(1);
    check(powerTable<DynModInt<3>>(3, 0) == vector<DynModInt<3>>(4, 0), "powerTable modulus 1");
    cout << "PASS powerTable, " << checks << " checks\n";}
