#include "../../05-Mathematics/34-quadratic_congruence.hpp"
#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/07-infint.hpp"

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
// Floor square root from a long double estimate corrected in exact 128-bit arithmetic, independent of the header.
lng rootFloor(lll n) {
    lll r = lll(std::sqrt((long double)n));
    while (r > 0 && r * r > n) { --r; }
    while ((r + 1) * (r + 1) <= n) { ++r; }
    return lng(r);}
bool isSquare(lll n) { lng r = rootFloor(n); return n >= 0 && lll(r) * r == n; }
using Form = BinaryQuadraticForm;
bool same(const Form &f, const Form &g) { return f.a == g.a && f.b == g.b && f.c == g.c; }
string showForm(const Form &f) { return show({f.a, f.b, f.c}); }
bool isReduced(const Form &f) {
    lng ab = f.b < 0 ? -f.b : f.b;
    return f.a > 0 && ab <= f.a && f.a <= f.c && (f.b >= 0 || (ab != f.a && f.a != f.c));}
// f(x + q y, y) and f(-y, x): the generators of SL2(Z) acting on forms.
Form shiftForm(const Form &f, lng q) { return {f.a, f.b + 2 * f.a * q, f.a * q * q + f.b * q + f.c}; }
Form swapForm(const Form &f) { return {f.c, -f.b, f.a}; }
// Every primitive reduced form of discriminant D by scanning a <= sqrt(|D| / 3) and b in (-a, a].
vector<Form> bruteReduced(lng D) {
    vector<Form> res;
    for (lng a = 1; 3 * a * a <= -D; ++a) {
        for (lng b = -a + 1; b <= a; ++b) {
            lng num = b * b - D;
            if (num % (4 * a)) { continue; }
            Form f{a, b, num / (4 * a)};
            if (isReduced(f) && gcd(gcd(f.a, f.b), f.c) == 1) { res.pb(f); }}}
    return res;}
// Dirichlet composition through a united form: gcd(a1, a2) = 1, B = b1 mod 2 a1, B = b2 mod 2 a2, by search.
Form dirichlet(const Form &f, const Form &g, lng D) {
    lng A = f.a * g.a;
    for (lng B = 0; B < 2 * A; ++B) {
        if ((B - f.b) % (2 * f.a) == 0 && (B - g.b) % (2 * g.a) == 0 && (B * B - D) % (4 * A) == 0) { return {A, B, (B * B - D) / (4 * A)}; }}
    return {0, 0, 0};}
// Representations by brute force inside the ellipse bound |x| <= sqrt(4 c n / |D|), |y| <= sqrt(4 a n / |D|).
vector<pair<lng, lng>> bruteRepresent(const Form &f, lng n) {
    lng D = f.b * f.b - 4 * f.a * f.c, X = rootFloor(lll(4) * f.c * n / -D) + 1, Y = rootFloor(lll(4) * f.a * n / -D) + 1;
    vector<pair<lng, lng>> res;
    for (lng x = -X; x <= X; ++x) {
        for (lng y = -Y; y <= Y; ++y) { if (lll(f.a) * x * x + lll(f.b) * x * y + lll(f.c) * y * y == n) { res.pb({x, y}); } }}
    return res;}
template<typename T> void printPairs(const vector<pair<T, T>> &v) {
    cout << v.size();
    for (auto &[x, y] : v) { cout << ' ' << x << ' ' << y; }}

int main(int argc, char **argv) {
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "corn-d") { cornacchia(0, 5); }
        if (s == "corn-m") { cornacchia(1, 0); }
        if (s == "corn-gcd") { cornacchia(2, 6); }
        if (s == "two-negative") { twoSquares(-1); }
        if (s == "twoall-negative") { twoSquaresAll(-1); }
        if (s == "twocount-negative") { twoSquaresCount(-5); }
        if (s == "three-negative") { threeSquares(-1); }
        if (s == "four-negative") { fourSquares(-1); }
        if (s == "tri-negative") { threeTriangular(-1); }
        if (s == "tri-large") { threeTriangular((std::numeric_limits<lng>::max() - 3) / 8 + 1); }
        if (s == "pyth-negative") { pythagoreanTriples(-1); }
        if (s == "gauss-negative") { gaussCircleCount(-1); }
        if (s == "pellgen-square") { pellGeneralized<lll>(49, 3); }
        if (s == "pellgen-small") { pellGeneralized<lll>(1, 3); }
        if (s == "pellgen-min") { pellGeneralized<lll>(2, std::numeric_limits<lng>::min()); }
        if (s == "forms-positive") { Form::reducedForms(5); }
        if (s == "forms-residue") { Form::reducedForms(-6); }
        if (s == "reduce-indefinite") { Form{1, 3, 1}.reduce(); }
        if (s == "reduce-negative") { Form{-1, 0, -1}.reduce(); }
        if (s == "compose-disc") { Form{1, 0, 1}.compose(Form{1, 1, 1}); }
        if (s == "compose-primitive") { Form{2, 0, 2}.compose(Form{1, 0, 4}); }
        if (s == "represent-indefinite") { Form{1, 3, 1}.represent(5); }
        if (s == "reduce-disc") { lng a = (1LL << 61) + 1; Form{a, 1, a}.reduce(); }
        if (s == "pellsol-unit") { pellSolutions<lll>(1, 0, 3, 1, 3, 2); }
        return 0;}
    if (argc > 1 && string(argv[1]) == "--oracle") {
        string op;
        while (cin >> op) {
            if (op == "A") {
                lng n; cin >> n;
                auto [x, y] = twoSquares(n);
                cout << x << ' ' << y << ' ' << twoSquaresCount(n) << ' ';
                printPairs(twoSquaresAll(n));}
            if (op == "B") {
                lng n; cin >> n;
                auto t = threeSquares(n);
                auto f = fourSquares(n);
                cout << t[0] << ' ' << t[1] << ' ' << t[2] << ' ' << f[0] << ' ' << f[1] << ' ' << f[2] << ' ' << f[3];
                if (n <= (std::numeric_limits<lng>::max() - 3) / 8) { auto r = threeTriangular(n); cout << ' ' << r[0] << ' ' << r[1] << ' ' << r[2]; }}
            if (op == "C") { lng d, m; cin >> d >> m; printPairs(cornacchia(d, m)); }
            if (op == "P") { lng D; cin >> D; auto [x, y] = pellFundamental<iint>(D); cout << x << ' ' << y; }
            if (op == "G") { lng D, N; cin >> D >> N; printPairs(pellGeneralized<iint>(D, N)); }
            if (op == "R") { lng a, b, c, n; cin >> a >> b >> c >> n; printPairs(Form{a, b, c}.represent(n)); }
            if (op == "H") { lng D; cin >> D; cout << Form::reducedForms(D).size(); }
            if (op == "X") {
                lng a, b, c, d, e, f; cin >> a >> b >> c >> d >> e >> f;
                auto r = Form{a, b, c}.compose(Form{d, e, f});
                cout << r.a << ' ' << r.b << ' ' << r.c;}
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
    const lng MAX = std::numeric_limits<lng>::max();

    // Sums of squares: every n up to the limit against enumerated pairs; Legendre's three-square set by enumeration; Gauss circle prefix sums.
    lng limT = array<lng, 3>{5000, 60000, 200000}[lvl];
    vector<vector<pair<lng, lng>>> reps(limT + 1);
    vector<lng> r2(limT + 1, 0);
    for (lng x = -rootFloor(limT); x <= rootFloor(limT); ++x) {
        for (lng y = -rootFloor(limT - x * x); x * x + y * y <= limT; ++y) {
            ++r2[x * x + y * y];
            if (x >= 0 && y >= 0) { reps[x * x + y * y].pb({x, y}); }}}
    vector<char> three(limT + 1, 0);
    for (lng x = 0; x * x <= limT; ++x) {
        for (lng y = x; x * x + y * y <= limT; ++y) {
            for (lng z = y; x * x + y * y + z * z <= limT; ++z) { three[x * x + y * y + z * z] = 1; }}}
    lll circle = 0;
    for (lng n = 0; n <= limT; ++n) {
        string tag = show({n});
        sort(reps[n].begin(), reps[n].end());
        check(twoSquaresAll(n) == reps[n], "twoSquaresAll" + tag);
        expect(twoSquaresCount(n), r2[n], "twoSquaresCount" + tag);
        auto [x, y] = twoSquares(n);
        check(reps[n].empty() ? x == -1 && y == -1 : x >= y && y >= 0 && x * x + y * y == n, "twoSquares" + tag);
        auto t = threeSquares(n);
        check(three[n] ? t[0] >= t[1] && t[1] >= t[2] && t[2] >= 0 && t[0] * t[0] + t[1] * t[1] + t[2] * t[2] == n : t[0] == -1 && t[1] == -1 && t[2] == -1, "threeSquares" + tag);
        auto f = fourSquares(n);
        check(f[0] >= f[1] && f[1] >= f[2] && f[2] >= f[3] && f[3] >= 0 && f[0] * f[0] + f[1] * f[1] + f[2] * f[2] + f[3] * f[3] == n, "fourSquares" + tag);
        auto g = threeTriangular(n);
        check(g[0] >= g[1] && g[1] >= g[2] && g[2] >= 0 && g[0] * (g[0] + 1) / 2 + g[1] * (g[1] + 1) / 2 + g[2] * (g[2] + 1) / 2 == n, "threeTriangular" + tag);
        circle += r2[n];
        check(gaussCircleCount(n) == circle, "gaussCircleCount" + tag);}
    // Random 64-bit n: validity, Legendre's exclusion 4^a (8 b + 7), two-square counts against the r2 product over a trial factorization of smooth n.
    int casesT = array<int, 3>{300, 3000, 12000}[lvl];
    for (int it = 0; it < casesT; ++it) {
        lng n = it % 3 ? rnd(0, MAX) : rnd(0, 1LL << rnd(1, 62));
        if (it % 5 == 0) { n = 1; for (int j = int(rnd(1, 12)); j > 0; --j) { lng p = vector<lng>{2, 3, 5, 7, 13, 17, 29, 37, 41}[rnd(0, 8)]; if (n <= MAX / p) { n *= p; } } }
        string tag = show({n});
        lng m = n;
        while (m && m % 4 == 0) { m /= 4; }
        auto t = threeSquares(n);
        if (n && m % 8 == 7) { check(t[0] == -1 && t[1] == -1 && t[2] == -1, "threeSquares excluded" + tag); }
        else { check(t[0] >= t[1] && t[1] >= t[2] && t[2] >= 0 && lll(t[0]) * t[0] + lll(t[1]) * t[1] + lll(t[2]) * t[2] == n, "threeSquares large" + tag); }
        auto f = fourSquares(n);
        check(f[0] >= f[1] && f[1] >= f[2] && f[2] >= f[3] && f[3] >= 0 && lll(f[0]) * f[0] + lll(f[1]) * f[1] + lll(f[2]) * f[2] + lll(f[3]) * f[3] == n, "fourSquares large" + tag);
        if (n <= (MAX - 3) / 8) {
            auto g = threeTriangular(n);
            check(g[0] >= g[1] && g[1] >= g[2] && g[2] >= 0 && lll(g[0]) * (g[0] + 1) / 2 + lll(g[1]) * (g[1] + 1) / 2 + lll(g[2]) * (g[2] + 1) / 2 == n, "threeTriangular large" + tag);}
        auto [x, y] = twoSquares(n);
        auto all = twoSquaresAll(n);
        lng c = twoSquaresCount(n);
        check(x == -1 ? all.empty() && c == 0 : x >= y && y >= 0 && lll(x) * x + lll(y) * y == n && !all.empty(), "twoSquares large" + tag);
        for (auto [u, v] : all) { check(u >= 0 && v >= 0 && lll(u) * u + lll(v) * v == n, "twoSquaresAll large" + tag); }
        check(std::is_sorted(all.begin(), all.end()) && std::adjacent_find(all.begin(), all.end()) == all.end(), "twoSquaresAll sorted" + tag);
        expect(lng(all.size()), n ? c / 4 + isSquare(n) : 1, "twoSquaresAll count" + tag);}
    for (lng n : vector<lng>{MAX, MAX - 1, 1LL << 62, (1LL << 62) - 1, 999999999999999989LL, 1000000000000000000LL}) {
        auto f = fourSquares(n);
        check(lll(f[0]) * f[0] + lll(f[1]) * f[1] + lll(f[2]) * f[2] + lll(f[3]) * f[3] == n, "fourSquares extreme" + show({n}));}
    for (lng n : vector<lng>{0, 1, 2, 4, 1000000, 999999999999LL, 1000000000000LL, lvl ? 4000000000000LL : 10000000000LL}) {
        lll want = 0;
        for (lng x = -rootFloor(n); x <= rootFloor(n); ++x) { want += 2 * rootFloor(n - x * x) + 1; }
        check(gaussCircleCount(n) == want, "gaussCircleCount large" + show({n}));}

    // cornacchia: every coprime (d, m) up to the limits against enumerated primitive representations.
    lng limCd = array<lng, 3>{12, 40, 80}[lvl], limCm = array<lng, 3>{2000, 8000, 20000}[lvl];
    for (lng d = 1; d <= limCd; ++d) {
        vector<vector<pair<lng, lng>>> prim(limCm + 1);
        for (lng x = 0; x * x <= limCm; ++x) {
            for (lng y = 0; x * x + d * y * y <= limCm; ++y) { if (gcd(x, y) == 1) { prim[x * x + d * y * y].pb({x, y}); } }}
        for (lng m = 1; m <= limCm; ++m) {
            if (gcd(d, m) != 1) { continue; }
            sort(prim[m].begin(), prim[m].end());
            check(cornacchia(d, m) == prim[m], "cornacchia" + show({d, m}));}}
    for (int it = 0; it < array<int, 3>{200, 2000, 8000}[lvl]; ++it) {
        lng d = rnd(1, it % 2 ? 10 : MAX), m = rnd(1, MAX);
        while (gcd(d, m) != 1) { m = rnd(1, MAX); }
        auto res = cornacchia(d, m);
        for (auto [x, y] : res) { check(gcd(x, y) == 1 && lll(x) * x + lll(d) * y * y == m, "cornacchia large" + show({d, m, x, y})); }
        check(std::is_sorted(res.begin(), res.end()) && std::adjacent_find(res.begin(), res.end()) == res.end(), "cornacchia large sorted" + show({d, m}));}

    // pythagoreanTriples: brute force for small N, Euclid's parametrization for large N.
    for (lng N : vector<lng>{0, 1, 4, 5, 12, 13, 16, 17, 25, 29, 100, 997, lvl ? 2500LL : 600LL}) {
        set<array<lng, 3>> want;
        for (lng c = 1; c <= N; ++c) {
            for (lng a = 1; a < c; a += 2) {
                lng b2 = c * c - a * a, b = rootFloor(b2);
                if (b * b == b2 && b % 2 == 0 && gcd(a, b) == 1) { want.insert({a, b, c}); }}}
        auto got = pythagoreanTriples(N);
        check(set<array<lng, 3>>(got.begin(), got.end()) == want && got.size() == want.size(), "pythagoreanTriples" + show({N}));}
    for (lng N : vector<lng>{10000, lvl ? 3000000LL : 100000LL}) {
        set<array<lng, 3>> want;
        for (lng u = 2; u * u + 1 <= N; ++u) {
            for (lng v = 1 + u % 2; v < u && u * u + v * v <= N; v += 2) { if (gcd(u, v) == 1) { want.insert({u * u - v * v, 2 * u * v, u * u + v * v}); } }}
        auto got = pythagoreanTriples(N);
        check(set<array<lng, 3>>(got.begin(), got.end()) == want && got.size() == want.size(), "pythagoreanTriples Euclid" + show({N}));}

    // BinaryQuadraticForm: reducedForms against scanning, reduce under random SL2(Z) moves, represent against the ellipse scan, compose against Dirichlet's united forms.
    lng limF = array<lng, 3>{1500, 12000, 30000}[lvl];
    for (lng D = -3; D >= -limF; --D) {
        if (D % 4 != 0 && D % 4 != -3) { continue; }
        auto want = bruteReduced(D), got = Form::reducedForms(D);
        bool ok = got.size() == want.size();
        for (size_t i = 0; ok && i < want.size(); ++i) { ok = same(got[i], want[i]); }
        check(ok, "reducedForms" + show({D}));}
    lng limR = array<lng, 3>{150, 400, 800}[lvl];
    for (lng D = -3; D >= -limR; --D) {
        if (D % 4 != 0 && D % 4 != -3) { continue; }
        auto forms = bruteReduced(D);
        for (const Form &F : forms) {
            Form G = F;
            for (int s = int(rnd(1, 5)); s > 0; --s) { G = swapForm(shiftForm(G, rnd(-3, 3))); }
            if (G.a > 4000000 || G.c > 4000000) { G = shiftForm(F, rnd(-3, 3)); }
            check(same(G.reduce(), F) && isReduced(G.reduce()), "reduce" + showForm(G) + showForm(F));
            for (int s = 0; s < (lvl ? 4 : 2); ++s) {
                lng n = s < 2 ? rnd(0, 60) : rnd(0, 3000), k = s == 3 ? rnd(2, 4) : 1;
                Form H{k * G.a, k * G.b, k * G.c};
                check(H.represent(n) == bruteRepresent(H, n), "represent" + showForm(H) + show({n}));}}
        for (const Form &F : forms) {
            for (const Form &G : forms) {
                Form C = F.compose(G);
                check(isReduced(C) && lll(C.b) * C.b - 4 * lll(C.a) * C.c == D, "compose shape" + showForm(F) + showForm(G));
                // Move G to an equivalent form whose first coefficient is coprime to F.a, then compose by definition.
                Form H = G;
                for (int s = 0; gcd(F.a, H.a) != 1 && s < 400; ++s) { H = swapForm(shiftForm(G, rnd(-30, 30))); }
                if (gcd(F.a, H.a) == 1) { check(same(dirichlet(F, H, D).reduce(), C), "compose Dirichlet" + showForm(F) + showForm(G)); }}}}
    // Group laws and Lagrange's theorem for large discriminants.
    for (int it = 0; it < array<int, 3>{40, 200, 600}[lvl]; ++it) {
        lng D = -rnd(3, it % 2 ? 20000000 : 400000) * 4 - (it % 4 < 2 ? 0 : 1);
        D = D % 4 == -2 || D % 4 == -1 ? D - 1 : D;
        if (D % 4 != 0 && D % 4 != -3) { continue; }
        lng b = D & 1;
        Form F{}, G{}, I{1, b, (b - D) / 4};
        // Random primitive forms from small primes p with D a square mod 4p.
        auto randomForm = [&]() {
            for (;;) {
                lng p = rnd(1, 500), bb = rnd(0, 2 * p);
                if ((bb - D) % 2 == 0 && (bb * bb - D) % (4 * p) == 0) {
                    Form f{p, bb, (bb * bb - D) / (4 * p)};
                    if (gcd(gcd(f.a, f.b), f.c) == 1) { return f.reduce(); }}}};
        F = randomForm(); G = randomForm();
        Form H = randomForm();
        check(same(F.compose(I), F) && same(F.compose(G), G.compose(F)), "compose identity commutative" + show({D}));
        check(same(F.compose(Form{F.a, -F.b, F.c}), I.reduce()), "compose inverse" + show({D}));
        check(same(F.compose(G).compose(H), F.compose(G.compose(H))), "compose associative" + show({D}));
        if (-D <= 4000000) {
            lng h = lng(Form::reducedForms(D).size());
            Form P = I.reduce();
            for (lng i = 0; i < h; ++i) { P = P.compose(F); }
            check(same(P, I.reduce()), "compose order divides h" + show({D, h}));}}

    // Pell: fundamental solutions checked by the equation and, when small, by scanning y; modular images; family steps by repeated multiplication.
    lng limP = array<lng, 3>{300, 2500, 8000}[lvl];
    for (lng D = -3; D <= limP; ++D) {
        string tag = show({D});
        if (D < 2 || isSquare(D)) {
            auto [x, y] = pellFundamental<lll>(D);
            check(x == 0 && y == 0, "pellFundamental square" + tag);
            continue;}
        auto [x, y] = pellFundamental<iint>(D);
        check(x * x - iint(D) * y * y == iint(1) && y > iint(0) && x > iint(0), "pellFundamental" + tag);
        if (y < iint(20000)) {
            lng ys = lng(y);
            for (lng t = 1; t < ys; ++t) { check(!isSquare(lll(D) * t * t + 1), "pellFundamental minimal" + tag); }}
        auto [mx, my] = pellFundamental<mint>(D);
        check(mx.val() == ulng(x % iint(998244353)) && my.val() == ulng(y % iint(998244353)), "pellFundamental mint" + tag);
        if (D <= 400) {
            iint u = 1, v = 0;
            for (lng k = 0; k <= 6; ++k) {
                auto [px, py] = pellSolutions<iint>(iint(1), iint(0), x, y, D, k);
                check(px == u && py == v, "pellSolutions" + show({D, k}));
                auto [qx, qy] = pellSolutions<iint>(u, v, x, y, D, -k);
                check(qx == iint(1) && qy == iint(0), "pellSolutions inverse" + show({D, k}));
                std::tie(u, v) = pair<iint, iint>{u * x + iint(D) * v * y, u * y + v * x};}}
        // Huge k modulo a prime against a 2 x 2 matrix power written here.
        lng k = rnd(0, MAX);
        array<mint, 4> M{mx, mint(D) * my, my, mx}, R{1, 0, 0, 1};
        for (lng e = k; e; e >>= 1) {
            auto mul = [](const array<mint, 4> &A, const array<mint, 4> &B) { return array<mint, 4>{A[0] * B[0] + A[1] * B[2], A[0] * B[1] + A[1] * B[3], A[2] * B[0] + A[3] * B[2], A[2] * B[1] + A[3] * B[3]}; };
            if (e & 1) { R = mul(R, M); }
            M = mul(M, M);}
        auto [sx, sy] = pellSolutions<mint>(mint(3), mint(5), mx, my, D, k);
        check(sx == R[0] * 3 + R[1] * 5 && sy == R[2] * 3 + R[3] * 5, "pellSolutions mint" + show({D, k}));}

    // pellGeneralized: fundamental solutions per class against an exhaustive scan below Nagell's bounds.
    lng limG = array<lng, 3>{40, 120, 250}[lvl], limN = array<lng, 3>{25, 80, 150}[lvl];
    lng scanned = 0;
    for (lng D = 2; D <= limG; ++D) {
        if (isSquare(D)) { continue; }
        auto [x1, y1] = pellFundamental<lll>(D);
        if (x1 > 1000000000000LL) { continue; }
        for (lng N = -limN; N <= limN; ++N) {
            string tag = show({D, N});
            auto got = pellGeneralized<lll>(D, N);
            if (!N) { check(got.size() == 1 && got[0].fi == 0 && got[0].se == 0, "pellGeneralized zero" + tag); continue; }
            long double bound = N > 0 ? (long double)(y1) * std::sqrt((long double)N / (2 * (long double)(x1 + 1))) : (long double)(y1) * std::sqrt((long double)-N / (2 * (long double)(x1 - 1)));
            if (bound > 100000) { continue; }
            ++scanned;
            vector<pair<lll, lll>> sols, want;
            for (lng t = 0; t <= lng(bound) + 1; ++t) {
                lll v = N + lll(D) * t * t;
                if (v >= 0 && isSquare(v)) { lng r = rootFloor(v); sols.pb({r, t}); if (r) { sols.pb({-r, t}); } }}
            lll an = N < 0 ? -N : N;
            for (auto [sx, sy] : sols) {
                bool seen = false;
                for (auto [fx, fy] : want) { seen |= (sx * fx - D * sy * fy) % an == 0 && (sx * fy - fx * sy) % an == 0; }
                if (!seen) { want.pb({sx, sy}); }}
            sort(want.begin(), want.end(), [](auto &l, auto &r) { return l.se < r.se || (l.se == r.se && l.fi < r.fi); });
            check(got == want, "pellGeneralized" + tag + " classes=" + std::to_string(want.size()) + " got=" + std::to_string(got.size()));
            if (D <= 30 && N % 7 == 0) {
                auto big = pellGeneralized<iint>(D, N);
                bool eq = big.size() == got.size();
                for (size_t i = 0; eq && i < got.size(); ++i) { eq = big[i].fi == iint(got[i].fi) && big[i].se == iint(got[i].se); }
                check(eq, "pellGeneralized iint" + tag);}}}
    check(scanned > 100, "pellGeneralized scanned cases");
    // Large fundamental units (beyond the lll bound) with iint: validity, (y, x) order, minimality against both unit neighbours, distinct classes; D = 1021 regression.
    for (lng D : vector<lng>{1021, 1117, 991, 661, 9949}) {
        auto [ux, uy] = pellFundamental<iint>(D);
        for (lng N : vector<lng>{-1020, -1116, 1, -1, 4, -4, 12345, -99991, 1000000007}) {
            auto sols = pellGeneralized<iint>(D, N);
            string tag = show({D, N});
            iint an = iint(N < 0 ? -N : N);
            for (size_t i = 0; i < sols.size(); ++i) {
                auto &[x, y] = sols[i];
                check(x * x - iint(D) * y * y == iint(N) && y >= iint(0), "pellGeneralized iint equation" + tag);
                for (int sgn : {1, -1}) {
                    iint ny = x * uy * iint(sgn) + y * ux;
                    check((ny < iint(0) ? -ny : ny) >= y, "pellGeneralized iint minimal" + tag);}
                for (size_t j = 0; j < i; ++j) {
                    auto &[x2, y2] = sols[j];
                    check(!((x * x2 - iint(D) * y * y2) % an == iint(0) && (x * y2 - x2 * y) % an == iint(0)), "pellGeneralized iint distinct classes" + tag);
                    check(y2 < y || (y2 == y && x2 < x), "pellGeneralized iint order" + tag);}}
            if (D == 1021 && N == -1020) {
                bool found = false;
                for (auto &[x, y] : sols) { found |= x == iint(-1022) && y == iint(32); }
                check(found, "pellGeneralized D = 1021 regression");}}}
    // Moduli near 2^63: square-root classes whose step reaches the modulus (enumeration must not overflow).
    for (lng N : vector<lng>{MAX, -MAX, MAX - 6, -(MAX - 24), 4611686018427387847LL, -4611686018427387847LL}) {
        for (lng D : vector<lng>{2, 3, 5, 7, 13}) {
            auto sols = pellGeneralized<iint>(D, N);
            for (auto &[x, y] : sols) { check(x * x - iint(D) * y * y == iint(N) && y >= iint(0), "pellGeneralized near 2^63" + show({D, N})); }}}
    for (lng n : vector<lng>{MAX, MAX - 1, (1LL << 62) + 1, 4611686018427387847LL, 2305843009213693951LL * 2, 5LL * 5 * 13 * 13 * 17 * 29 * 37 * 41 * 53 * 61 * 73 * 89}) {
        auto rep = Form{1, 0, 1}.represent(n), rep3 = Form{1, 1, 1}.represent(n);
        for (auto [x, y] : rep) { check(lll(x) * x + lll(y) * y == n, "represent near 2^63" + show({n, x, y})); }
        for (auto [x, y] : rep3) { check(lll(x) * x + lll(x) * y + lll(y) * y == n, "represent Eisenstein near 2^63" + show({n, x, y})); }
        expect(lng(rep.size()), twoSquaresCount(n), "represent x^2 + y^2 count near 2^63" + show({n}));}

    cout << "PASS 34-quadratic_congruence " << checks << " checks seed=" << seed << " (" << scanned << " Pell (D, N) scans)" << std::endl;
    return 0;}
