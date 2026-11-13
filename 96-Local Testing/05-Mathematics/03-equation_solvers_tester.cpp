#include "../../05-Mathematics/03-equation_solvers.hpp"

// Debug mode checks the whole heap on every push and pop, so the checked build skips the 3 * 10^5-entry heap case.
#ifdef _GLIBCXX_DEBUG
constexpr bool CHECKED = true;
#else
constexpr bool CHECKED = false;
#endif
string context;
lng checks = 0;
string decimal(ulll x) {
    string s;
    do { s += char('0' + x % 10); x /= 10; } while (x);
    reverse(s.begin(), s.end()); return s;}
string decimal(lll x) { return x < 0 ? "-" + decimal(ulll(0) - ulll(x)) : decimal(ulll(x)); }
void need(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string join(const vector<lng> &v) {
    string s;
    for (lng x : v) { s += (s.empty() ? "" : ",") + std::to_string(x); }
    return s;}
ulng magnitude(lng x) { return x < 0 ? ulng(0) - ulng(x) : ulng(x); }
void equation(lng a, lng b, lng c) {
    context = std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
    auto s = solveDioEq(a, b, c);
    lll g = std::gcd(magnitude(a), magnitude(b));
    bool exists = g == 0 ? c == 0 : c % g == 0;
    need((s.dimension >= 0) == exists, "existence expected=" + std::to_string(exists) + " actual dimension=" + std::to_string(s.dimension));
    need(s.g == g, "gcd expected=" + decimal(g) + " actual=" + decimal(s.g));
    if (!exists) { return; }
    if (g == 0) { need(s.dimension == 2, "whole plane"); }
    else {
        need(s.dimension == 1, "line dimension");
        need(lll(a) * s.x + lll(b) * s.y == c, "particular residual");
        need(lll(a) * s.dx + lll(b) * s.dy == 0, "homogeneous residual");
        need(s.dx == b / g && s.dy == -lll(a) / g, "primitive direction");
        if (b != 0) { need(0 <= s.x && s.x < lll(magnitude(b)) / g, "canonical x"); }}}
void boxBrute(lng a, lng b, lng c, lng xl, lng xr, lng yl, lng yr) {
    context = std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c) + " box="
        + std::to_string(xl) + "," + std::to_string(xr) + "," + std::to_string(yl) + "," + std::to_string(yr);
    auto z = solveDioBox(a, b, c, xl, xr, yl, yr);
    auto &s = z.solution;
    ulll count = 0;
    for (lng x = xl; x < xr; ++x) { for (lng y = yl; y < yr; ++y) {
        if (lll(a) * x + lll(b) * y != c) { continue; }
        ++count;
        if (s.dimension == 2) { continue; }
        lll delta = s.dx != 0 ? lll(x) - s.x : lll(y) - s.y;
        lll step = s.dx != 0 ? s.dx : s.dy;
        need(step != 0 && delta % step == 0, "every point lies on primitive line");
        lll t = delta / step;
        need(z.l <= t && t < z.r && s.x + t * s.dx == x && s.y + t * s.dy == y, "every point parameterized");}}
    need(z.count == count, "box count expected=" + decimal(count) + " actual=" + decimal(z.count));
    if (s.dimension == 1 && z.count > 0) {
        need(z.r - z.l == lll(count), "parameter cardinality");
        for (lll t = z.l; t < z.r; ++t) {
            lll x = s.x + t * s.dx, y = s.y + t * s.dy;
            need(xl <= x && x < xr && yl <= y && y < yr && lll(a) * x + lll(b) * y == c, "no extra parameter points");}}}
void modBrute(lng a, lng b, lng m) {
    context = std::to_string(a) + " " + std::to_string(b) + " modulus=" + std::to_string(m);
    auto [x, period] = solveModEq(a, b, m);
    vector<lng> want;
    for (lng v = 0; v < m; ++v) { if ((lll(a) * v - b) % m == 0) { want.pb(v); }}
    if (want.empty()) { need(x == -1 && period == -1, "mod no-solution sentinel"); return; }
    need(x == want.front() && period == m / lng(want.size()), "least representative and least period");
    for (lng v = 0; v < m; ++v) { need(((lll(a) * v - b) % m == 0) == (v >= x && (v - x) % period == 0), "all modular solutions"); }}
void minBrute(lng a, lng b, lng c, lng p, lng q, lng xl, lng xr, lng yl, lng yr) {
    context = "min-linear " + join({a, b, c, p, q}) + " box=" + join({xl, xr, yl, yr});
    auto [ok, x, y] = solveDioMinLinear(a, b, c, p, q, xl, xr, yl, yr);
    // Tie rule: the plane takes the least x, then y; a line takes the least parameter, i.e. least (dx, dy) . (x, y).
    lll g = std::gcd(magnitude(a), magnitude(b)), dx = g ? b / g : 0, dy = g ? -lll(a) / g : 0;
    bool found = false;
    tuple<lll, lll, lll> best;
    for (lng u = xl; u < xr; ++u) { for (lng v = yl; v < yr; ++v) {
        if (lll(a) * u + lll(b) * v != c) { continue; }
        tuple<lll, lll, lll> key{lll(p) * u + lll(q) * v, g ? dx * u + dy * v : u, v};
        if (!found || key < best) { best = key; found = true; }}}
    need(ok == found, "min-linear existence expected=" + std::to_string(found));
    if (!found) { need(x == 0 && y == 0, "min-linear none sentinel"); return; }
    need(xl <= x && x < xr && yl <= y && y < yr && lll(a) * x + lll(b) * y == c, "min-linear point solves in box");
    need(lll(p) * x + lll(q) * y == std::get<0>(best), "min-linear objective expected=" + decimal(std::get<0>(best)) + " actual=" + decimal(lll(p) * x + lll(q) * y));
    need(g ? dx * x + dy * y == std::get<1>(best) : x == std::get<1>(best) && y == std::get<2>(best), "min-linear tie rule point");
    auto box = solveDioBox(a, b, c, xl, xr, yl, yr);
    auto &s = box.solution;
    if (s.dimension == 2) { need(x == (p < 0 ? xr - 1 : xl) && y == (q < 0 ? yr - 1 : yl), "min-linear plane corner"); return; }
    lll t = lll(p) * s.dx + lll(q) * s.dy < 0 ? box.r - 1 : box.l;
    need(x == s.x + t * s.dx && y == s.y + t * s.dy, "min-linear parameter t=" + decimal(t));}
vector<char> sieve(const vector<lng> &a, lng limit) {
    vector<char> reach(size_t(limit) + 1);
    reach[0] = 1;
    for (lng t = 1; t <= limit; ++t) { for (lng b : a) { if (b <= t && reach[size_t(t - b)]) { reach[size_t(t)] = 1; break; } } }
    return reach;}
void residueBrute(const vector<lng> &a) {
    context = "residue a=" + join(a);
    lng m = *std::min_element(a.begin(), a.end()), limit = m * *std::max_element(a.begin(), a.end()) + m, frob = -1;
    auto reach = sieve(a, limit);
    vector<lng> want(size_t(m), -1);
    for (lng t = limit; t >= 0; --t) {
        if (reach[size_t(t)]) { want[size_t(t % m)] = t; }
        else if (frob < 0) { frob = t; }}
    auto d = residueShortestPath(a), e = residueShortestPathDijkstra(a);
    need(d == want, "residue table expected=" + join(want) + " actual=" + join(d));
    need(e == want, "Dijkstra residue table expected=" + join(want) + " actual=" + join(e));
    bool all = std::find(want.begin(), want.end(), -1) == want.end();
    lng f = all ? frob : std::numeric_limits<lng>::max();
    need(frobeniusNumber(d) == f, "Frobenius expected=" + std::to_string(f) + " actual=" + std::to_string(frobeniusNumber(d)));
    need(countRepresentable(d, -1) == 0 && countRepresentable(d, std::numeric_limits<lng>::min()) == 0, "count below zero");
    ulng count = 0;
    for (lng t = 0; t <= limit; ++t) {
        count += reach[size_t(t)];
        if (t > 2 * m && t % 11 != 0 && t + 1 < frob) { continue; }
        need(countRepresentable(d, t) == count, "prefix count x=" + std::to_string(t) + " expected=" + std::to_string(count));}
    lng gaps = countNonRepresentable(d);
    if (!all) { need(gaps == std::numeric_limits<lng>::max(), "non-representable sentinel gcd > 1 actual=" + std::to_string(gaps)); return; }
    ulng missing = ulng(limit) + 1 - count;
    need(countRepresentable(d, std::numeric_limits<lng>::max()) == (ulng(1) << 63) - missing, "count to INT64_MAX");
    need(gaps == lng(missing), "non-representable count vs sieve expected=" + std::to_string(missing) + " actual=" + std::to_string(gaps));
    need(gaps == frob + 1 - lng(countRepresentable(d, frob)) && 2 * gaps >= frob + 1, "gaps = F + 1 - countRepresentable(F), at least half of [0, F]");}
void pairCheck(lng u, lng v, lng x) {
    context = "pair a=" + join({u, v}) + " x=" + std::to_string(x);
    lng m = min(u, v), b = max(u, v);
    auto d = residueShortestPath({u, v});
    need(lng(d.size()) == m, "pair table size");
    need(residueShortestPathDijkstra({u, v}) == d, "pair Dijkstra table equals the round robin");
    for (lll j = 0; j < m; ++j) { need(d[size_t(j * b % m)] == j * b, "pair table entry j*b j=" + decimal(j)); }
    lll f = lll(m) * b - m - b, half = lll(m - 1) * (b - 1) / 2;
    need(frobeniusNumber(d) == f, "Sylvester Frobenius expected=" + decimal(f) + " actual=" + std::to_string(frobeniusNumber(d)));
    need(countRepresentable(d, -1) == 0 && countRepresentable(d, std::numeric_limits<lng>::min()) == 0, "pair count below zero");
    for (lll t : {f, f + 1, lll(x), lll(std::numeric_limits<lng>::max())}) {
        if (t < f || t < 0 || t > std::numeric_limits<lng>::max()) { continue; }
        need(countRepresentable(d, lng(t)) == ulll(t + 1 - half), "Sylvester count x=" + decimal(t) + " expected=" + decimal(t + 1 - half));}
    if (f >= 1) { need(countRepresentable(d, lng(f - 1)) == countRepresentable(d, lng(f)), "Frobenius not counted"); }
    need(countNonRepresentable(d) == half && frobeniusNumberSylvester(u, v) == f && countNonRepresentableSylvester(u, v) == half,
        "pair gap count and Sylvester forms expected=" + decimal(f) + "," + decimal(half) + " actual=" + std::to_string(countNonRepresentable(d)) + ","
        + decimal(frobeniusNumberSylvester(u, v)) + "," + decimal(countNonRepresentableSylvester(u, v)));}
void sylvesterCheck(lng a, lng b, bool brute) {
    context = "sylvester a=" + join({a, b});
    constexpr lng TABLE_NONE = std::numeric_limits<lng>::max();
    constexpr lll NONE = std::numeric_limits<lll>::max();
    lll f = frobeniusNumberSylvester(a, b), half = countNonRepresentableSylvester(a, b);
    auto d = residueShortestPath({a, b});
    lng tf = frobeniusNumber(d), th = countNonRepresentable(d);
    bool coprime = gcd(a, b) == 1;
    need(coprime ? f == tf && half == th : f == NONE && half == NONE && tf == TABLE_NONE && th == TABLE_NONE,
        "Sylvester vs table expected=" + std::to_string(tf) + "," + std::to_string(th) + " actual=" + decimal(f) + "," + decimal(half));
    if (!brute) { return; }
    lng limit = a * b + max(a, b), frob = -1, gaps = 0;
    auto reach = sieve({a, b}, limit);
    for (lng t = 0; t <= limit; ++t) { if (!reach[size_t(t)]) { frob = t; ++gaps; } }
    if (!coprime) { need(!(reach[size_t(limit)] && reach[size_t(limit - 1)]), "sieve gcd > 1 leaves gaps at the top"); return; }
    need(std::all_of(reach.end() - min(a, b), reach.end(), [](char c) { return c != 0; }), "sieve range closes: last min(a, b) values reachable");
    need(f == frob && half == gaps, "Sylvester vs sieve expected=" + std::to_string(frob) + "," + std::to_string(gaps) + " actual=" + decimal(f) + "," + decimal(half));}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20260927;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            lng a, b, c, xl, xr, yl, yr, m;
            while (cin >> a >> b >> c >> xl >> xr >> yl >> yr >> m) {
                auto s = solveDioEq(a, b, c); auto z = solveDioBox(a, b, c, xl, xr, yl, yr);
                auto [x, period] = solveModEq(a, c, m);
                cout << s.dimension << ' ' << decimal(s.x) << ' ' << decimal(s.y) << ' ' << decimal(s.dx) << ' '
                    << decimal(s.dy) << ' ' << decimal(s.g) << ' ' << decimal(z.l) << ' ' << decimal(z.r) << ' '
                    << decimal(z.count) << ' ' << x << ' ' << period << '\n';}
            return 0;}
        else if (arg == "--oracle-min") {
            lng a, b, c, p, q, xl, xr, yl, yr;
            while (cin >> a >> b >> c >> p >> q >> xl >> xr >> yl >> yr) {
                auto [ok, x, y] = solveDioMinLinear(a, b, c, p, q, xl, xr, yl, yr);
                cout << ok << ' ' << x << ' ' << y << '\n';}
            return 0;}
        else if (arg == "--oracle-residue") {
            size_t k, nx;
            while (cin >> k) {
                vector<lng> a(k);
                for (lng &v : a) { cin >> v; }
                auto d = residueShortestPath(a);
                if (residueShortestPathDijkstra(a) != d) { cout << "Dijkstra-differs "; }
                for (lng v : d) { cout << v << ' '; }
                cout << frobeniusNumber(d);
                cin >> nx;
                for (lng x; nx-- && cin >> x;) { cout << ' ' << countRepresentable(d, x); }
                cout << '\n';}
            return 0;}
        else if (arg == "--oracle-sylvester") {
            for (lng a, b; cin >> a >> b;) { cout << decimal(frobeniusNumberSylvester(a, b)) << ' ' << decimal(countNonRepresentableSylvester(a, b)) << '\n'; }
            return 0;}}
    constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
    if (!invalid.empty()) {
        if (invalid == "mod-zero") { solveModEq(1, 2, 0); }
        else if (invalid == "mod-negative") { solveModEq(1, 2, -1); }
        else if (invalid == "mod-zero-nosolution") { solveModEq(2, 1, 0); }
        else if (invalid == "mod-negative-nosolution") { solveModEq(2, 1, -4); }
        else if (invalid == "box-x") { solveDioBox(1, 2, 3, 1, 0, 0, 1); }
        else if (invalid == "box-y") { solveDioBox(1, 2, 3, 0, 1, 1, 0); }
        else if (invalid == "residue-empty") { residueShortestPath({}); }
        else if (invalid == "residue-zero") { residueShortestPath({3, 0, 5}); }
        else if (invalid == "residue-negative") { residueShortestPath({3, -2, 5}); }
        else if (invalid == "residue-overflow") { residueShortestPath({5, HI / 3 + 1, 3}); }
        else if (invalid == "dijkstra-empty") { residueShortestPathDijkstra({}); }
        else if (invalid == "dijkstra-zero") { residueShortestPathDijkstra({3, 0, 5}); }
        else if (invalid == "dijkstra-negative") { residueShortestPathDijkstra({3, -2, 5}); }
        else if (invalid == "dijkstra-overflow") { residueShortestPathDijkstra({5, HI / 3 + 1, 3}); }
        else if (invalid == "sylvester-zero") { frobeniusNumberSylvester(0, 3); }
        else if (invalid == "sylvester-negative") { countNonRepresentableSylvester(5, -2); }
        else { return 2; }
        return 0;}
    int n = mode == "quick" ? 3 : 6;
    for (int a = -n; a <= n; ++a) { for (int b = -n; b <= n; ++b) { for (int c = -2*n; c <= 2*n; ++c) {
        equation(a, b, c); boxBrute(a, b, c, -8, 9, -8, 9);}}}
    cout << "PASS signed exhaustive parameterization checks=" << checks << '\n';
    n = mode == "quick" ? 2 : 4;
    for (int a = -n; a <= n; ++a) { for (int b = -n; b <= n; ++b) { for (int c = -n; c <= n; ++c) {
        for (int xl = -2; xl <= 3; ++xl) { for (int xr = xl; xr <= 3; ++xr) {
            for (int yl = -2; yl <= 3; ++yl) { for (int yr = yl; yr <= 3; ++yr) { boxBrute(a,b,c,xl,xr,yl,yr); }}}}}}}
    cout << "PASS exhaustive half-open boxes checks=" << checks << '\n';
    n = mode == "quick" ? 12 : 40;
    for (int a = -n; a <= n; ++a) { for (int b = -n; b <= n; ++b) { for (int m = 1; m <= n; ++m) { modBrute(a,b,m); }}}
    cout << "PASS exhaustive linear congruences checks=" << checks << '\n';
    vector<lng> edges{LO, LO+1, LO+2, -2, -1, 0, 1, 2, HI-2, HI-1, HI};
    for (lng a : edges) { for (lng b : edges) { for (lng c : edges) { equation(a,b,c); }}}
    context = "LO 0 0 and 0 -1 LO";
    need(solveDioEq(LO,0,0).g == lll(1)<<63 && solveDioEq(0,-1,LO).y == -lll(LO), "g and y beyond lng");
    auto huge = solveDioBox(0,0,0,LO,HI,LO,HI);
    ulll width = (ulll(1)<<64)-1;
    need(huge.count == width*width && huge.count > (ulll(1)<<127), "unsigned128 plane count");
    auto diagonal = solveDioBox(1,-1,0,LO,HI,LO,HI);
    need(diagonal.count == width, "maximum line count");
    std::mt19937_64 rng(seed);
    int rounds = mode == "quick" ? 1000 : mode == "full" ? 20000 : 200000;
    for (int i = 0; i < rounds; ++i) {
        lng a = std::bit_cast<lng>(rng()), b = std::bit_cast<lng>(rng()), c = std::bit_cast<lng>(rng());
        equation(a,b,c);
        lng xl = lng(rng()%101)-50, yl = lng(rng()%101)-50;
        boxBrute(a,b,c,xl,xl+lng(rng()%8),yl,yl+lng(rng()%8));
        if (i%4 == 0) { equation(a,b,0); boxBrute(a,b,0,-4,5,-4,5); }}
    cout << "PASS extrema and seeded random seed=" << seed << " cases=" << rounds << " checks=" << checks << '\n';
    n = mode == "quick" ? 2 : 4;
    int w = mode == "quick" ? 2 : 3;
    vector<array<lng, 4>> boxes{{-4, 5, -4, 5}, {0, 0, -2, 3}, {-1, 2, 1, 1}, {1, 2, -3, 4}, {-3, 3, 2, 3}, {-5, -1, 0, 6}, {0, 1, 0, 1}, {2, 7, -6, -1}};
    for (int a = -n; a <= n; ++a) { for (int b = -n; b <= n; ++b) { for (int c = -2*n; c <= 2*n; ++c) {
        for (int p = -w; p <= w; ++p) { for (int q = -w; q <= w; ++q) {
            for (auto [xl, xr, yl, yr] : boxes) { minBrute(a, b, c, p, q, xl, xr, yl, yr); }}}}}}
    for (lng p : edges) { for (lng q : edges) {
        minBrute(0, 0, 0, p, q, -3, 4, -2, 3); minBrute(0, 0, 1, p, q, -3, 4, -2, 3); minBrute(0, 3, 6, p, q, -3, 4, -2, 3);
        minBrute(2, 0, -4, p, q, -3, 4, -2, 3); minBrute(1, -1, 0, p, q, -4, 4, -3, 5); minBrute(2, 3, 1, p, q, -6, 7, -5, 6);}}
    for (lng k = -3; k <= 3; ++k) {
        minBrute(2, 3, 1, 2 * k, 3 * k, -9, 10, -8, 9); minBrute(1, -1, 0, k, -k, -4, 4, -3, 5); minBrute(-4, 6, 2, -2 * k, 3 * k, -9, 10, -8, 9);
        minBrute(0, 0, 0, 0, k, -3, 4, -2, 3); minBrute(0, 0, 0, k, 0, -3, 4, -2, 3);}
    minBrute(1, -1, 0, HI, -HI, -4, 4, -3, 5); minBrute(1, 1, 0, LO, LO, -4, 4, -4, 4); minBrute(3, 0, 6, 0, LO, 1, 4, -3, 3);
    for (int i = 0; i < rounds; ++i) {
        lng a = lng(rng()%2001)-1000, b = lng(rng()%2001)-1000, xl = lng(rng()%101)-50, yl = lng(rng()%101)-50;
        lng xr = xl+lng(rng()%9), yr = yl+lng(rng()%9), c = i%2 ? a*(xl+3)+b*(yl+2) : lng(rng()%41)-20;
        minBrute(a, b, c, std::bit_cast<lng>(rng()), std::bit_cast<lng>(rng()), xl, xr, yl, yr);}
    cout << "PASS min-linear objective vs enumeration checks=" << checks << '\n';
    int pairs = mode == "quick" ? 12 : 25, triples = mode == "quick" ? 7 : 12, quads = mode == "quick" ? 0 : 7;
    for (lng a = 1; a <= pairs; ++a) { for (lng b = 1; b <= pairs; ++b) {
        residueBrute({a, b});
        if (gcd(a, b) == 1) { pairCheck(a, b, a * b); }}}
    for (lng a = 1; a <= triples; ++a) { for (lng b = 1; b <= triples; ++b) { for (lng c = 1; c <= triples; ++c) { residueBrute({a, b, c}); }}}
    for (lng a = 1; a <= quads; ++a) { for (lng b = 1; b <= quads; ++b) { for (lng c = 1; c <= quads; ++c) {
        for (lng e = 1; e <= quads; ++e) { residueBrute({a, b, c, e}); }}}}
    for (auto a : vector<vector<lng>>{{1}, {7}, {5, 5, 5}, {4, 6}, {6, 10, 15}, {6, 10, 14}, {1, 40, 40}, {40, 39, 38, 37, 36, 35}}) { residueBrute(a); }
    int sets = mode == "quick" ? 200 : mode == "full" ? 3000 : 30000;
    for (int i = 0; i < sets; ++i) {
        lng g = i%5 ? 1 : lng(2+rng()%3);
        vector<lng> a(1+rng()%6);
        for (lng &v : a) { v = g * lng(1+rng()%ulng(60/g)); }
        if (i%7 == 0) { a.pb(a[rng()%a.size()]); }
        if (i%11 == 0) { a.pb(1); }
        residueBrute(a);}
    cout << "PASS residue table/Frobenius/count vs knapsack DP sets=" << sets << " checks=" << checks << '\n';
    for (int i = 0; i < rounds / 10; ++i) {
        lng m = lng(1+rng()%60), b;
        do { b = m + 1 + lng(rng() % ulng(HI / m - m)); } while (gcd(m, b) != 1);
        lng x = lng(rng() >> 1);
        if (i%2) { pairCheck(m, b, x); }
        else { pairCheck(b, m, x); }}
    pairCheck(2, HI / 2, HI); pairCheck(HI, 1, 0); pairCheck(3, HI / 3 - 1, HI - 5);
    context = "gcd 7 boundary";
    auto d7 = residueShortestPath({HI / 7, 7});
    need(residueShortestPathDijkstra({HI / 7, 7}) == d7 && residueShortestPathDijkstra({HI, 1}) == vector<lng>{0}, "Dijkstra at INT64_MAX = 7 * max and m = 1");
    need(d7 == vector<lng>{0, -1, -1, -1, -1, -1, -1} && frobeniusNumber(d7) == HI && countRepresentable(d7, HI) == ulng(HI / 7) + 1, "INT64_MAX = 7 * max, gcd 7");
    need(countNonRepresentable(d7) == HI && countNonRepresentable({0}) == 0, "gap count gcd 7 sentinel, m = 1");
    context = "stale heap entries";
    vector<lng> many{2};
    for (lng b = CHECKED ? 2001 : 600001; b >= 3; b -= 2) { many.push_back(b); }
    need(residueShortestPathDijkstra(many) == vector<lng>{0, 3} && residueShortestPath(many) == vector<lng>{0, 3}, "one residue improved by every odd coin, each stale entry skipped");
    cout << "PASS Sylvester pairs at signed64 scale cases=" << rounds / 10 << " checks=" << checks << '\n';
    context = "sieve 3 5";
    need(sieve({3, 5}, 8) == vector<char>{1, 0, 0, 1, 0, 1, 1, 0, 1}, "sieve self-check");
    for (lng a = 1; a <= 60; ++a) { for (lng b = 1; b <= 60; ++b) { sylvesterCheck(a, b, true); }}
    int coins = mode == "quick" ? 40 : mode == "full" ? 400 : 4000;
    for (int i = 0; i < coins; ++i) {
        lng a = 1 + lng(rng() % (i % 2 ? 1000 : 100000)), b = 1 + lng(rng() % 1000000);
        if (i % 4 == 0) { lng g = lng(2 + rng() % 9); a = g * max(lng(1), a / g); b = g * max(lng(1), b / g); }
        if (i % 3 == 0) { swap(a, b); }
        sylvesterCheck(a, b, false);}
    cout << "PASS Sylvester forms vs sieve on [1, 60]^2 and table route pairs=" << coins << " checks=" << checks << '\n';}
