#include "../../05-Mathematics/03-equation_solvers.hpp"

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
ulng magnitude(lng x) { return x < 0 ? ulng(0) - ulng(x) : ulng(x); }
void equation(lng a, lng b, lng c) {
    context = std::to_string(a) + " " + std::to_string(b) + " " + std::to_string(c);
    auto s = solveDioEq(a, b, c);
    lll g = std::gcd(magnitude(a), magnitude(b));
    bool exists = g == 0 ? c == 0 : c % g == 0;
    need((s.dimension >= 0) == exists, "existence expected=" + std::to_string(exists) + " actual dimension=" + std::to_string(s.dimension));
    need(s.g == g, "gcd expected=" + decimal(g) + " actual=" + decimal(s.g));
    if (!exists) {
        lng x = 11, y = 22, h = 33;
        need(!solveDioEq(a, b, c, x, y, h) && x == 11 && y == 22 && h == 33, "legacy no-solution preserves outputs"); return;}
    if (g == 0) { need(s.dimension == 2, "whole plane"); }
    else {
        need(s.dimension == 1, "line dimension");
        need(lll(a) * s.x + lll(b) * s.y == c, "particular residual");
        need(lll(a) * s.dx + lll(b) * s.dy == 0, "homogeneous residual");
        need(s.dx == b / g && s.dy == -lll(a) / g, "primitive direction");
        if (b != 0) { need(0 <= s.x && s.x < lll(magnitude(b)) / g, "canonical x"); }}
    constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
    if (LO <= s.x && s.x <= HI && LO <= s.y && s.y <= HI && s.g <= HI) {
        lng x = 0, y = 0, h = 0;
        need(solveDioEq(a, b, c, x, y, h) && x == s.x && y == s.y && h == s.g, "legacy agrees on its domain");}}
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
            return 0;}}
    constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();
    if (!invalid.empty()) {
        if (invalid == "mod-zero") { solveModEq(1, 2, 0); }
        else if (invalid == "mod-negative") { solveModEq(1, 2, -1); }
        else if (invalid == "box-x") { solveDioBox(1, 2, 3, 1, 0, 0, 1); }
        else if (invalid == "box-y") { solveDioBox(1, 2, 3, 0, 1, 1, 0); }
        else if (invalid == "legacy-gcd") { lng x, y, g; solveDioEq(LO, 0, 0, x, y, g); }
        else if (invalid == "legacy-point") { lng x, y, g; solveDioEq(0, -1, LO, x, y, g); }
        else if (invalid == "legacy-xy") { lng x, g; solveDioEq(1, 2, 3, x, x, g); }
        else if (invalid == "legacy-xg") { lng x, y; solveDioEq(1, 2, 3, x, y, x); }
        else if (invalid == "legacy-yg") { lng x, y; solveDioEq(1, 2, 3, x, y, y); }
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
    cout << "PASS extrema and seeded random seed=" << seed << " cases=" << rounds << " checks=" << checks << '\n';}
