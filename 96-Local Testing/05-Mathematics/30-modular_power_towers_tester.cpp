#include "../../05-Mathematics/30-modular_power_towers.hpp"

string context;
lng checks = 0;
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string show(const vector<ulng> &a) {
    string s = "[";
    for (ulng x : a) { s += std::to_string(x) + ","; }
    return s + "]";}

// Brute phi, lambda (lcm of unit orders) and threshold (least t with a^t = a^(t + lambda) for all a) by direct iteration.
ulng brutePhi(ulng m) {
    ulng c = 0;
    for (ulng a = 0; a < m; ++a) { c += gcd(a, m) == 1; }
    return m == 1 ? 1 : c;}
ulng bruteLambda(ulng m) {
    ulng res = 1;
    for (ulng a = 1; a < m; ++a) {
        if (gcd(a, m) != 1) { continue; }
        ulng x = a % m, o = 1;
        while (x != 1 % m) { x = x * a % m; ++o; }
        res = lcm(res, o);}
    return res;}
int bruteThreshold(ulng m) {
    ulng l = bruteLambda(m);
    for (int t = 0;; ++t) {
        bool ok = true;
        for (ulng a = 0; a < m && ok; ++a) {
            ulng x = 1 % m;
            for (int i = 0; i < t; ++i) { x = x * a % m; }
            ulng y = x;
            for (ulng i = 0; i < l; ++i) { y = y * a % m; }
            ok = x == y;}
        if (ok) { return t; }}}

// Uniform-cap evaluation from the top: min(b^min(v, C), C) = min(b^v, C) for every b when C >= 1.
ulng satPow(ulng b, ulng e, ulng cap) {
    if (e == 0) { return min<ulng>(1, cap); }
    if (b <= 1) { return min(b, cap); }
    ulll r = 1;
    for (ulng i = 0; i < e; ++i) { r *= b; if (r >= cap) { return cap; } }
    return ulng(r);}
ulng capTower(const vector<ulng> &a, size_t i, ulng top, ulng cap) {
    ulng v = min(top, cap);
    for (size_t j = a.size(); j-- > i;) { v = satPow(a[j], v, cap); }
    return v;}
// Cycle oracle: simulate the powers of each base mod m to find its preperiod mu and period pi, then recurse modulo pi.
ulng cycleTower(const vector<ulng> &a, size_t i, ulng top, ulng m) {
    if (m == 1) { return 0; }
    if (i == a.size()) { return top % m; }
    vector<int> seen(m, -1);
    vector<ulng> seq;
    ulng x = 1 % m, b = a[i] % m;
    while (seen[x] < 0) { seen[x] = int(seq.size()); seq.pb(x); x = x * b % m; }
    ulng mu = ulng(seen[x]), pi = seq.size() - mu, e = capTower(a, i + 1, top, max<ulng>(mu, 1));
    if (e < mu) { return seq[e]; }
    ulng r = cycleTower(a, i + 1, top, pi);
    return seq[mu + (r + pi - mu % pi) % pi];}

void chains(ulng m) {
    context = "m=" + std::to_string(m);
    auto e = eulerChain(m), c = carmichaelChain(m);
    need(e[0] == m && e.back() == 1 && c[0] == m && c.back() == 1, "chain ends");
    for (size_t i = 0; i + 1 < e.size(); ++i) { need(e[i + 1] == brutePhi(e[i]) && e[i] > 1, "eulerChain step"); }
    for (size_t i = 0; i + 1 < c.size(); ++i) { need(c[i + 1] == bruteLambda(c[i]) && c[i] > 1, "carmichaelChain step"); }
    need(towerThreshold(m) == bruteThreshold(m), "towerThreshold");}

void cycleCase(const vector<ulng> &a, ulng m) {
    context = show(a) + " m=" + std::to_string(m);
    ulng want = cycleTower(a, 0, 1, m);
    need(powerTowerMod(a, eulerChain(m)) == want, "powerTowerMod euler vs cycle oracle");
    need(powerTowerMod(a, carmichaelChain(m)) == want, "powerTowerMod carmichael vs cycle oracle");
    if (!a.empty() && std::all_of(a.begin(), a.end(), [&](ulng x) { return x == a[0]; })) {
        need(iteratedPowerMod(a[0], a.size(), 1, carmichaelChain(m)) == want && tetrationMod(a[0], a.size(), eulerChain(m)) == want, "iterated/tetration vs cycle");}
    if (a.size() >= 2 && std::all_of(a.begin(), a.end() - 1, [&](ulng x) { return x == a[0]; })) {
        need(iteratedPowerMod(a[0], a.size() - 1, a.back(), carmichaelChain(m)) == want, "iterated top vs cycle");}}

void exactCase(std::mt19937_64 &rng, const vector<ulng> &a) {
    context = show(a);
    const ulng BIG = ulng(1) << 62;
    ulng v = capTower(a, 0, 1, BIG);
    for (ulng cap : {ulng(0), ulng(1), ulng(2), ulng(64), ulng(1000), BIG, ~ulng(0), rng() >> (rng() % 64)}) {
        if (v < BIG || cap <= BIG) { need(powerTowerCapped(a, cap) == min(v, cap), "powerTowerCapped vs uniform cap"); }}
    if (v == BIG) { return; }
    for (int t = 0; t < 2; ++t) {
        ulng m = t == 0 ? ~ulng(0) - rng() % 100 : max<ulng>(1, rng() >> (rng() % 64));
        need(powerTowerMod(a, carmichaelChain(m)) == v % m, "powerTowerMod exact value mod m");}}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261019;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            // Rows: kind m then data; L: list, I: a k x0, C: chains and threshold.
            string kind; ulng m;
            while (cin >> kind >> m) {
                if (kind == "L") {
                    int n; cin >> n; vector<ulng> a(n);
                    for (auto &x : a) { cin >> x; }
                    cout << powerTowerMod(a, eulerChain(m)) << ' ' << powerTowerMod(a, carmichaelChain(m)) << '\n';}
                else if (kind == "I") {
                    ulng a, k, x0; cin >> a >> k >> x0;
                    cout << iteratedPowerMod(a, k, x0, carmichaelChain(m)) << ' ' << iteratedPowerMod(a, k, x0, eulerChain(m)) << '\n';}
                else {
                    for (ulng x : eulerChain(m)) { cout << x << ' '; }
                    cout << "|";
                    for (ulng x : carmichaelChain(m)) { cout << ' ' << x; }
                    cout << " | " << towerThreshold(m) << '\n';}}
            return 0;}}
    if (!invalid.empty()) {
        vector<ulng> bad{10, 4, 2};
        if (invalid == "euler-zero") { eulerChain(0); }
        else if (invalid == "carmichael-zero") { carmichaelChain(0); }
        else if (invalid == "threshold-zero") { towerThreshold(0); }
        else if (invalid == "chain-end") { powerTowerMod(vector<ulng>{2, 3}, bad); }
        else if (invalid == "chain-empty") { iteratedPowerMod(2, 3, 1, vector<ulng>{}); }
        else if (invalid == "chain-order") { tetrationMod(2, 3, vector<ulng>{4, 6, 1}); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    for (ulng m = 1; m <= (quick ? 120u : 400u); ++m) { chains(m); }
    cout << "PASS chains and threshold vs brute phi, lambda, threshold checks=" << checks << '\n';

    // Exhaustive short towers over small bases (zeros and ones included) against the cycle oracle.
    int B = quick ? 4 : 6;
    for (ulng m = 1; m <= (quick ? 40u : 90u); ++m) {
        for (int len = 0; len <= 3; ++len) {
            int total = 1;
            for (int i = 0; i < len; ++i) { total *= B; }
            for (int code = 0; code < total; ++code) {
                vector<ulng> a(len);
                for (int i = 0, c = code; i < len; ++i, c /= B) { a[i] = ulng(c % B); }
                cycleCase(a, m);}}}
    cout << "PASS exhaustive towers of length <= 3 over bases < " << B << " vs cycle oracle checks=" << checks << '\n';

    int R = quick ? 300 : stress ? 30000 : 4000;
    for (int r = 0; r < R; ++r) {
        int len = int(rng() % (r % 5 == 0 ? 40 : 9));
        vector<ulng> a(len);
        for (auto &x : a) {
            ulng c = rng() % 10;
            x = c < 3 ? rng() % 3 : c < 8 ? rng() % 20 : c == 8 ? rng() : ~ulng(0) - rng() % 3;}
        if (r % 4 == 0 && len) { std::fill(a.begin(), a.end(), a[0]); }
        if (r % 7 == 0) { for (auto &x : a) { if (rng() % 3 == 0) { x = 0; } } }
        cycleCase(a, 1 + rng() % (quick ? 500 : 3000));}
    cout << "PASS seeded towers mod m <= 3000 vs cycle oracle checks=" << checks << '\n';

    for (int r = 0; r < R / 2; ++r) {
        int len = int(rng() % 6);
        vector<ulng> a(len);
        for (auto &x : a) { x = rng() % 5 == 0 ? rng() >> (rng() % 64) : rng() % 7; }
        exactCase(rng, a);}
    cout << "PASS exact and capped towers vs saturating evaluation checks=" << checks << '\n';

    // Huge counts: towers of k equal bases stabilize; compare with the same parity at a depth beyond every chain.
    for (int r = 0; r < (quick ? 40 : stress ? 3000 : 300); ++r) {
        ulng a = rng() % 4 == 0 ? rng() % 3 : rng() >> (rng() % 64), x0 = rng() % 3 ? rng() % 4 : rng(), m = max<ulng>(1, rng() >> (rng() % 64));
        ulng k = rng() % 2 ? ~ulng(0) - rng() % 2 : 200 + rng() % 1000;
        context = "a=" + std::to_string(a) + " k=" + std::to_string(k) + " x0=" + std::to_string(x0) + " m=" + std::to_string(m);
        auto c = carmichaelChain(m), e = eulerChain(m);
        ulng v = iteratedPowerMod(a, k, x0, c);
        need(v == iteratedPowerMod(a, 150 + k % 2, x0, c) && v == iteratedPowerMod(a, k, x0, e), "iterated stabilization and chain independence");
        need(tetrationMod(a, k, c) == iteratedPowerMod(a, k, 1, c), "tetration = iterated top 1");}
    cout << "PASS huge counts: stabilization, parity and both chains checks=" << checks << '\n';

    // Edges: m = 1, empty tower, 0^0 = 1, cap 0, all-zero runs, a↑↑0 = 1.
    context = "edges";
    need(powerTowerMod(vector<ulng>{}, carmichaelChain(10)) == 1 && powerTowerMod(vector<ulng>{5, 7}, vector<ulng>{1}) == 0, "empty tower, m = 1");
    need(powerTowerMod(vector<ulng>{0, 0}, carmichaelChain(7)) == 1 && powerTowerMod(vector<ulng>{0, 0, 0}, carmichaelChain(7)) == 0, "0^0 = 1");
    need(powerTowerCapped(vector<ulng>{2, 3}, 0) == 0 && powerTowerCapped(vector<ulng>{}, 5) == 1, "cap 0, empty capped");
    need(tetrationMod(0, 0, carmichaelChain(9)) == 1 && tetrationMod(0, 1, carmichaelChain(9)) == 0 && tetrationMod(0, 2, carmichaelChain(9)) == 1, "0 tetration parity");
    need(tetrationMod(3, 0, carmichaelChain(1)) == 0 && tetrationMod(2, 3, carmichaelChain(1000000007)) == 16, "small tetration");
    // The largest 64-bit prime: phi = lambda = m - 1 sits above 2^64 - 64, where a residue exponent is raised past the threshold.
    const ulng P = ~ulng(0) - 58;
    for (auto c : {eulerChain(P), carmichaelChain(P)}) {
        need(powerTowerMod(vector<ulng>{P, P - 1}, c) == 0 && powerTowerMod(vector<ulng>{0, P - 1, 2}, c) == 0, "m = 2^64 - 59 regressions");
        for (int t = 0; t < 50; ++t) {
            ulng a = 2 + rng() % (P - 2);
            context = "a=" + std::to_string(a);
            need(powerTowerMod(vector<ulng>{a, P - 2, 2}, c) == a && powerTowerMod(vector<ulng>{a, P, 1}, c) == a, "Fermat: exponent = 1 mod p - 1");}}
    vector<ulng> zeros(100000, 0);
    need(powerTowerMod(zeros, carmichaelChain(13)) == 1 && powerTowerCapped(zeros, 7) == 1, "long zero run parity");
    cout << "PASS edges checks=" << checks << '\n';}
