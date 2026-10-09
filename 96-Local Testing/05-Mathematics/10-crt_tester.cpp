#include "../../05-Mathematics/10-crt.hpp"

lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
lng norm(lng a, lng m) { return (a % m + m) % m; }
string show(const vector<pair<lng, lng>> &c) {
    string s;
    for (auto [a, m] : c) { s += " (" + std::to_string(a) + " mod " + std::to_string(m) + ")"; }
    return s;}
// Least solution and period of the solution set of a_i * x = b_i mod m_i in [0, L), or {-1, -1}.
pair<lng, lng> bruteScaled(const vector<tuple<lng, lng, lng>> &eqs) {
    lng L = 1;
    for (auto [a, b, m] : eqs) { L = lcm(L, m); }
    vector<lng> sol;
    for (lng x = 0; x < L && sol.size() < 2; ++x) {
        bool ok = true;
        for (auto [a, b, m] : eqs) { ok &= norm(a * x - b, m) == 0; }
        if (ok) { sol.pb(x); }}
    if (sol.empty()) { return {-1, -1}; }
    return {sol[0], sol.size() == 2 ? sol[1] - sol[0] : L};}
bool sameResult(const CrtResult &r, pair<lng, lng> want) {
    if (want.fi < 0) { return !r.ok && !r.overflow; }
    return r.ok && !r.overflow && r.value == want.fi && r.modulus == want.se;}

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "crt2-zero") { crt2(0, 0, 0, 1); }
        if (s == "crt-zero") { crt({{1, 2}, {0, 0}}); }
        if (s == "scaled-zero") { crtScaled({{1, 1, 0}}); }
        if (s == "garner-mod") { garner({{1, 2}}, 0); }
        if (s == "digits-zero") { vector<lng> d; garnerDigits({{1, 0}}, d); }
        if (s == "crtmod-mod") { crtMod({{1, 2}}, 0); }
        if (s == "crtmod-zero") { crtMod({{1, 0}}, 5); }
        if (s == "signed-zero") { signedRepresentative(3, 0); }
        if (s == "incremental-zero") { CrtIncremental c; c.add(1, 0); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op;
        while (cin >> op) {
            int k; lng target; cin >> target >> k;
            if (op == "s") {
                vector<tuple<lng, lng, lng>> eqs(k);
                for (auto &[a, b, m] : eqs) { cin >> a >> b >> m; }
                auto r = crtScaled(eqs);
                auto legacy = superChiRemThm(eqs);
                cout << r.value << ' ' << r.modulus << ' ' << r.ok << ' ' << r.overflow << ' ' << legacy.fi << ' ' << legacy.se << '\n';
                continue;}
            vector<pair<lng, lng>> c(k);
            for (auto &[a, m] : c) { cin >> a >> m; }
            auto r = crt(c);
            CrtIncremental inc;
            for (auto [a, m] : c) { inc.add(a, m); }
            vector<lng> d;
            bool ok = garnerDigits(c, d);
            cout << r.value << ' ' << r.modulus << ' ' << r.ok << ' ' << r.overflow << ' ' << inc.value() << ' ' << inc.modulus() << ' ' << inc.ok() << ' ' << inc.res.overflow;
            cout << ' ' << crtMod(c, target) << ' ' << garner(c, target) << ' ' << ok;
            if (ok) { for (lng x : d) { cout << ' ' << x; } }
            cout << '\n';}
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    std::mt19937_64 rng(seed);
    int lim = mode == "quick" ? 12 : mode == "full" ? 24 : 28;

    for (lng m1 = 1; m1 <= lim; ++m1) {
        for (lng m2 = 1; m2 <= lim; ++m2) {
            for (lng a1 = -m1; a1 < 2 * m1; ++a1) {
                for (lng a2 = -m2; a2 < 2 * m2; a2 += 1 + (m2 > 12)) {
                    auto want = bruteScaled({{1, a1, m1}, {1, a2, m2}});
                    string tag = show({{a1, m1}, {a2, m2}});
                    check(sameResult(crt2(a1, m1, a2, m2), want), "crt2" + tag);
                    CrtIncremental inc;
                    inc.add(a1, m1); inc.add(a2, m2);
                    check(sameResult(inc.res, want) && inc.ok() == (want.fi >= 0) && (!inc.ok() || (inc.value() == want.fi && inc.modulus() == want.se)), "CrtIncremental" + tag);
                    lng target = lng(rng() % 50) + 1;
                    check(crtMod({{a1, m1}, {a2, m2}}, target) == (want.fi < 0 ? -1 : want.fi % target), "crtMod pair" + tag);}}}}
    cout << "PASS crt2, CrtIncremental and crtMod on every pair of moduli through " << lim << '\n';

    int rounds = mode == "quick" ? 2000 : mode == "full" ? 30000 : 150000;
    for (int it = 0; it < rounds; ++it) {
        int k = int(rng() % 5);
        vector<pair<lng, lng>> c;
        vector<tuple<lng, lng, lng>> eqs;
        lng base = lng(rng() % 1000);
        for (int i = 0; i < k; ++i) {
            lng m = lng(rng() % 12) + 1, a = it % 2 ? base % m : lng(rng() % 40) - 20;
            c.pb({a, m});
            eqs.pb({lng(rng() % 25) - 12, lng(rng() % 25) - 12, m});}
        string tag = show(c);
        auto want = bruteScaled([&] { vector<tuple<lng, lng, lng>> e; for (auto [a, m] : c) { e.pb({1, a, m}); } return e; }());
        check(sameResult(crt(c), want), "crt" + tag);
        lng target = lng(rng() % 1000) + 1;
        check(crtMod(c, target) == (want.fi < 0 ? -1 : want.fi % target), "crtMod" + tag);
        auto ws = bruteScaled(eqs);
        auto rs = crtScaled(eqs);
        check(sameResult(rs, ws), "crtScaled round " + std::to_string(it));
        check(superChiRemThm(eqs) == (ws.fi < 0 ? pair<lng, lng>{-1, -1} : ws), "superChiRemThm round " + std::to_string(it));
        bool coprime = true;
        for (int i = 0; i < k; ++i) { for (int j = 0; j < i; ++j) { coprime &= gcd(c[i].se, c[j].se) == 1; } }
        vector<lng> d{42};
        bool ok = garnerDigits(c, d);
        check(ok == coprime && garner(c, target) == (coprime ? want.fi % target : -1), "garner status/value" + tag);
        if (!ok) { check(d == vector<lng>{42}, "garnerDigits failure leaves output" + tag); continue; }
        lng x = 0, pre = 1;
        for (int i = 0; i < k; ++i) {
            check(0 <= d[i] && d[i] < c[i].se, "garnerDigits range" + tag);
            x += d[i] * pre; pre *= c[i].se;}
        check(int(d.size()) == k && x == want.fi, "garnerDigits reconstruction" + tag);}
    check(sameResult(crt({}), {0, 1}) && garner({}, 7) == 0 && crtMod({}, 7) == 0 && crtMod({{5, 9}}, 1) == 0, "empty systems");
    cout << "PASS crt, crtScaled, superChiRemThm, garner, garnerDigits, crtMod on " << rounds << " random systems\n";

    const lng P = 9223372036854775783LL, Q = 4611686018427387847LL;
    auto r = crt2(5, P, 7, 3);
    check(!r.ok && r.overflow, "crt2 lcm overflow");
    r = crt2(5, 6, 4, 2 * Q);
    check(!r.ok && !r.overflow, "crt2 inconsistent before overflow");
    r = crt2(LLONG_MIN, P, LLONG_MAX, 1);
    check(r.ok && r.modulus == P && r.value == lng(((lll(LLONG_MIN) % P) + P) % P), "crt2 extreme residues");
    r = crt2(1, Q, 2, 2);
    check(r.ok && r.modulus == 2 * Q && r.value % Q == 1 && r.value % 2 == 0, "crt2 near-limit lcm");
    CrtIncremental inc;
    check(inc.add(1, P) && !inc.add(0, 3) && !inc.ok() && inc.res.overflow && !inc.add(1, 1), "CrtIncremental overflow is sticky");
    CrtIncremental bad;
    check(bad.add(0, 2) && !bad.add(1, 4) && !bad.add(0, 1) && !bad.ok(), "CrtIncremental inconsistency is sticky");
    for (lng m = 1; m <= 50; ++m) {
        for (lng x = -200; x <= 200; ++x) {
            lng want = 0;
            for (lng y = -(m - 1) / 2; y <= m / 2; ++y) { if (norm(x - y, m) == 0) { want = y; } }
            check(signedRepresentative(x, m) == want, "signedRepresentative x=" + std::to_string(x) + " m=" + std::to_string(m));}}
    check(signedRepresentative(std::numeric_limits<lll>::min(), LLONG_MAX) <= LLONG_MAX / 2 && signedRepresentative(-1, LLONG_MAX) == -1, "signedRepresentative extremes");
    cout << "PASS overflow, extremes and signedRepresentative, " << checks << " checks\n";}
