#include "../../03-Geometry/16-circle_polygon.hpp"

// Oracles: adaptive polar Gauss-Legendre integration of ray winding intervals between vertex and crossing angles, and
// arc membership by polygonContains at midpoints between crossings found with 06-circle segment intersection.
static ulng seed = 20260927;
static std::string phase;
static vector<vector<dpoint>> input;
static CircleApprox disk;
static lng checks = 0;
const long double PI = std::numbers::pi_v<long double>;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << std::setprecision(20) << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " circle=("
              << disk.c.x << ',' << disk.c.y << ',' << disk.r << ") rings=";
    for (auto &p : input) { std::cerr << '['; for (auto x : p) { std::cerr << '(' << x.x << ',' << x.y << ')'; } std::cerr << ']'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
vector<long double> breakpoints(CircleApprox c, const vector<vector<dpoint>> &rings) {
    vector<long double> b{-PI, PI};
    for (auto &p : rings) {
        int n = int(p.size());
        for (int i = 0; i < n; ++i) {
            dpoint a = p[i], q = p[(i + 1) % n];
            if (a != c.c) { b.push_back(std::atan2(a.y - c.c.y, a.x - c.c.x)); }
            if (a == q) { continue; }
            auto z = circleSegmentIntersectionApprox(c, a, q);
            for (int k = 0; k < z.count; ++k) { b.push_back(std::atan2(z.points[k].y - c.c.y, z.points[k].x - c.c.x)); }}}
    sort(b.begin(), b.end());
    return b;}
// Winding-weighted area inside the disk along the ray at angle t, intervals found from infinity inward.
long double rayIntegrand(CircleApprox c, const vector<vector<dpoint>> &rings, long double t) {
    dpoint u{std::cos(t), std::sin(t)}; vector<pair<long double, int>> hits;
    for (auto &p : rings) {
        int n = int(p.size());
        for (int i = 0; i < n; ++i) {
            dpoint a = p[i] - c.c, b = p[(i + 1) % n] - c.c, e = b - a; long double den = cross(u, e);
            if (den == 0) { continue; }
            long double s = cross(a, e) / den, l = cross(a, u) / den;
            if (s > 0 && l >= 0 && l < 1) { hits.push_back({s, cross(e, u) > 0 ? 1 : -1}); }}}
    sort(hits.rbegin(), hits.rend());
    long double res = 0, prev = 1e300L; int w = 0;
    for (auto [s, d] : hits) {
        long double hi = min(prev, c.r), lo = min(s, c.r); res += w * (hi * hi - lo * lo) / 2;
        w -= d; prev = s;}
    long double hi = min(prev, c.r); res += w * hi * hi / 2;
    return res;}
long double areaOracle(CircleApprox c, const vector<vector<dpoint>> &rings) {
    static const long double X[8] = {0.0950125098376374401853193354249581L, 0.2816035507792589132304605014604961L, 0.4580167776572273863424194429835775L,
        0.6178762444026437484466717640487910L, 0.7554044083550030338951011948474422L, 0.8656312023878317438804678977123931L,
        0.9445750230732325760779884155346083L, 0.9894009349916499325961541734503326L};
    static const long double W[8] = {0.1894506104550684962853967232082831L, 0.1826034150449235888667636679692199L, 0.1691565193950025381893120790303600L,
        0.1495959888165767320815017305474785L, 0.1246289712555338720524762821920164L, 0.0951585116824927848099251076022463L,
        0.0622535239386478928628438369943776L, 0.0271524594117540948517805724560181L};
    auto gauss = [&](long double l, long double r) {
        long double m = (l + r) / 2, h = (r - l) / 2, s = 0;
        for (int k = 0; k < 8; ++k) { s += h * W[k] * (rayIntegrand(c, rings, m - h * X[k]) + rayIntegrand(c, rings, m + h * X[k])); }
        return s;};
    // Adaptive bisection: edges passing near the center make the ray integrand nearly singular.
    auto adapt = [&](auto &&adapt, long double l, long double r, long double whole, int depth) -> long double {
        long double m = (l + r) / 2, a = gauss(l, m), b = gauss(m, r);
        if (depth >= 40 || std::abs(a + b - whole) <= 1e-17L * max(1.L, std::abs(whole))) { return a + b; }
        return adapt(adapt, l, m, a, depth + 1) + adapt(adapt, m, r, b, depth + 1);};
    auto b = breakpoints(c, rings); long double res = 0;
    for (int i = 0; i + 1 < int(b.size()); ++i) {
        if (b[i + 1] > b[i]) { res += adapt(adapt, b[i], b[i + 1], gauss(b[i], b[i + 1]), 0); }}
    return res;}
vector<pair<long double, long double>> arcsOracle(CircleApprox c, const vector<vector<dpoint>> &rings) {
    auto b = breakpoints(c, rings); vector<pair<long double, long double>> res;
    for (int i = 0; i + 1 < int(b.size()); ++i) {
        long double l = b[i], r = b[i + 1], m = (l + r) / 2;
        if (r - l < 1e-15L) { continue; }
        bool in = polygonContains(rings, c.c + dpoint{std::cos(m), std::sin(m)} * c.r) != PolygonLocation::Outside;
        if (!in) { continue; }
        if (!res.empty() && std::abs(res.back().second - l) < 1e-15L) { res.back().second = r; }
        else { res.push_back({l, r}); }}
    if (res.size() >= 2 && std::abs(res[0].first + PI) < 1e-15L && std::abs(res.back().second - PI) < 1e-15L) {
        res[0] = {res.back().first, res[0].second + 2 * PI}; res.pop_back();
        sort(res.begin(), res.end());}
    return res;}
long double scaleOf(CircleApprox c, const vector<vector<dpoint>> &rings) {
    long double s = max(1.L, c.r);
    for (auto &p : rings) { for (auto x : p) { s = max({s, std::abs(x.x - c.c.x), std::abs(x.y - c.c.y)}); } }
    return s;}
// Rounding near tangencies may split or join arcs by ~1e-19 rad: drop slivers, then join hairline gaps (also across the wrap).
vector<pair<long double, long double>> tidy(vector<pair<long double, long double>> a) {
    std::erase_if(a, [](auto &x) { return x.second - x.first < 1e-12L; });
    for (auto &x : a) { if (x.first > PI - 1e-12L) { x.first -= 2 * PI; x.second -= 2 * PI; } }
    sort(a.begin(), a.end()); vector<pair<long double, long double>> res;
    for (auto x : a) {
        if (!res.empty() && x.first - res.back().second < 1e-12L) { res.back().second = max(res.back().second, x.second); }
        else { res.push_back(x); }}
    if (res.size() >= 2 && res.back().second - 2 * PI > res[0].first - 1e-12L) {
        res[0] = {res.back().first, max(res[0].second + 2 * PI, res.back().second)}; res.pop_back(); sort(res.begin(), res.end());}
    if (res.size() == 1 && res[0].second - res[0].first > 2 * PI - 1e-12L) { res = {{-PI, PI}}; }
    return res;}
void checkArcs(CircleApprox c, const vector<vector<dpoint>> &rings, const vector<pair<long double, long double>> &raw) {
    auto want = tidy(arcsOracle(c, rings)), got = tidy(raw);
    bool integral = c.c.x == std::floor(c.c.x) && c.c.y == std::floor(c.c.y) && c.r == std::floor(c.r);
    for (auto &p : rings) { for (auto x : p) { integral = integral && x.x == std::floor(x.x) && x.y == std::floor(x.y); } }
    if (integral && rings.size() == 1) { require(raw.size() == got.size(), "integral simple ring gives no slivers or hairline gaps"); }
    require(got.size() == want.size(), "arc count equals midpoint-membership arcs");
    for (int i = 0; i < int(min(got.size(), want.size())); ++i) {
        require(std::abs(got[i].first - want[i].first) < 1e-12L && std::abs(got[i].second - want[i].second) < 1e-12L, "arc endpoints match");}
    for (int i = 0; i < int(raw.size()); ++i) {
        require(-PI <= raw[i].first && raw[i].first < PI && raw[i].first < raw[i].second && raw[i].second <= raw[i].first + 2 * PI, "arc normalisation");
        if (i) { require(raw[i - 1].second <= raw[i].first, "arcs sorted and disjoint"); }}}
template<typename T> void ringsCase(CircleApprox c, const vector<vector<Point2<T>>> &rings) {
    disk = c; input.clear();
    for (auto &p : rings) { input.push_back({}); for (auto x : p) { input.back().push_back(x.template cast<long double>()); } }
    long double want = areaOracle(c, input), s = scaleOf(c, input), tol = 1e-13L * s * s;
    long double got = diskRingsAreaApprox(c, rings), sum = 0;
    for (auto &p : rings) {
        long double a = diskPolygonAreaApprox(c, p);
        sum += a;
        require(std::abs(a - areaOracle(c, {input[&p - &rings[0]]})) <= tol, "ring signed area equals polar integration");
        if (p.size() == 3 && c.c == p[0].template cast<long double>()) {
            require(std::abs(diskTriangleAreaApprox(c, p[1], p[2]) - a) <= tol, "fan triangle from the center");}}
    require(std::abs(got - sum) <= tol, "rings area sums ring areas");
    require(std::abs(got - want) <= tol, "rings area equals winding-weighted polar integration");
    if (c.r > 0) { checkArcs(c, input, circleArcsInPolygonApprox(c, rings)); }}
template<typename T> void ringCase(CircleApprox c, const vector<Point2<T>> &p) {
    ringsCase(c, vector<vector<Point2<T>>>{p});
    if (c.r > 0) { require(circleArcsInPolygonApprox(c, p) == circleArcsInPolygonApprox(c, vector<vector<Point2<T>>>{p}), "ring and rings arcs agree"); }}
void triangleCases(std::mt19937_64 &rng, int rounds) {
    phase = "fan triangles";
    for (int t = 0; t < rounds; ++t) {
        point a{lng(rng() % 21) - 10, lng(rng() % 21) - 10}, b{lng(rng() % 21) - 10, lng(rng() % 21) - 10};
        CircleApprox c({(long double)(rng() % 9) - 4, (long double)(rng() % 9) - 4}, (long double)(rng() % 12) / 2);
        vector<dpoint> tri{c.c, a.cast<long double>(), b.cast<long double>()};
        disk = c; input = {tri};
        long double got = diskTriangleAreaApprox(c, a, b), want = areaOracle(c, {tri});
        require(std::abs(got - want) <= 1e-13L * 400, "fan triangle equals polar integration");
        require(std::abs(got + diskTriangleAreaApprox(c, b, a)) <= 1e-15L * 400, "fan triangle is antisymmetric");}}
void fixed() {
    phase = "fixed areas";
    vector<point> sq{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    disk = CircleApprox({0, 0}, 1);
    require(std::abs(diskPolygonAreaApprox(disk, sq) - PI) < 1e-17L, "inscribed disk");
    disk = CircleApprox({0, 0}, std::sqrt(2.L));
    require(std::abs(diskPolygonAreaApprox(disk, sq) - 4) < 1e-17L, "circumscribed disk");
    disk = CircleApprox({0, 0}, 0);
    require(diskPolygonAreaApprox(disk, sq) == 0, "zero radius");
    disk = CircleApprox({5, 5}, 1);
    require(diskPolygonAreaApprox(disk, sq) == 0, "disjoint");
    auto rev = sq; reverse(rev.begin(), rev.end()); disk = CircleApprox({0, 0}, 1);
    require(std::abs(diskPolygonAreaApprox(disk, rev) + PI) < 1e-17L, "clockwise is negative");
    require(diskPolygonAreaApprox(disk, vector<point>{}) == 0 && diskRingsAreaApprox(disk, vector<vector<point>>{}) == 0, "empty polygon");
    for (auto &c : {CircleApprox({0, 0}, 1), CircleApprox({1, 0}, 1), CircleApprox({1, 1}, 1), CircleApprox({0, 0}, 1.5L), CircleApprox({0.5L, -0.25L}, 0.75L)}) {
        ringCase(c, sq); ringCase(c, rev);}
    phase = "holes";
    vector<vector<point>> holes{{{-4, -4}, {4, -4}, {4, 4}, {-4, 4}}, {{-1, -1}, {-1, 1}, {1, 1}, {1, -1}}};
    disk = CircleApprox({0, 0}, 3);
    require(std::abs(diskRingsAreaApprox(disk, holes) - (9 * PI - 4)) < 1e-15L, "disk minus square hole");
    for (auto &c : {CircleApprox({0, 0}, 3), CircleApprox({0, 0}, 1), CircleApprox({1, 0}, 1), CircleApprox({0, 0}, 5), CircleApprox({0, 0}, std::sqrt(2.L))}) { ringsCase(c, holes); }
    phase = "fixed arcs";
    disk = CircleApprox({0, 0}, 1);
    auto full = circleArcsInPolygonApprox(disk, vector<point>{{-5, -5}, {5, -5}, {5, 5}, {-5, 5}});
    require(full.size() == 1 && full[0].first == -PI && full[0].second == PI, "circle inside polygon is the full arc");
    require(circleArcsInPolygonApprox(disk, vector<point>{{3, 3}, {4, 3}, {4, 4}}).empty(), "circle outside polygon has no arc");
    vector<point> rect{{-5, -3}, {5, -3}, {5, 3}, {-5, 3}}; disk = CircleApprox({0, 0}, 5); input = {{{-5, -3}, {5, -3}, {5, 3}, {-5, 3}}};
    auto arcs = circleArcsInPolygonApprox(disk, rect); long double a = std::asin(0.6L);
    require(arcs.size() == 2 && std::abs(arcs[0].first + a) < 1e-17L && std::abs(arcs[0].second - a) < 1e-17L && std::abs(arcs[1].first - (PI - a)) < 1e-17L && std::abs(arcs[1].second - (PI + a)) < 1e-17L, "tangent rectangle sides give two arcs");
    ringCase(disk, rect);
    vector<point> box{{-3, -4}, {3, -4}, {3, 4}, {-3, 4}};
    require(circleArcsInPolygonApprox(disk, box).empty(), "circle through the vertices of an inscribed box");
    ringCase(disk, box);
    vector<vector<point>> holed{{{-9, -9}, {9, -9}, {9, 9}, {-9, 9}}, {{5, 0}, {0, -5}, {-5, 0}, {0, 5}}};
    full = circleArcsInPolygonApprox(disk, holed); input = {{{-9, -9}, {9, -9}, {9, 9}, {-9, 9}}, {{5, 0}, {0, -5}, {-5, 0}, {0, 5}}};
    require(full.size() == 1 && full[0].first == -PI && full[0].second == PI, "hole touching the circle from inside keeps the full arc");
    ringsCase(disk, holed);
    vector<vector<point>> shared{{{8, 8}, {-2, 0}, {27, -26}}, {{6, -1}, {-9, 24}, {-72, -41}}};
    ringsCase(disk, shared);
    vector<point> diamond{{5, 0}, {0, 5}, {-5, 0}, {0, -5}};
    require(circleArcsInPolygonApprox(disk, diamond).empty(), "vertices on the circle, edges inside");
    vector<point> star{{5, 0}, {1, 1}, {0, 5}, {-1, 1}, {-5, 0}, {-1, -1}, {0, -5}, {1, -1}};
    require(circleArcsInPolygonApprox(disk, star).empty(), "spikes touching the circle");
    vector<point> out{{5, 0}, {9, -2}, {9, 2}};
    require(circleArcsInPolygonApprox(disk, out).empty(), "outside triangle touching at a vertex");
    vector<point> wedge{{0, 0}, {9, -9}, {9, 9}}; arcs = circleArcsInPolygonApprox(disk, wedge);
    require(arcs.size() == 1 && std::abs(arcs[0].first + PI / 4) < 1e-17L && std::abs(arcs[0].second - PI / 4) < 1e-17L, "wedge from the center");
    for (auto &p : {rect, box, diamond, star, out, wedge}) { ringCase(disk, p); }
    vector<Point2<double>> fp{{-1.5, -1.5}, {1.5, -1.5}, {1.5, 1.5}, {-1.5, 1.5}};
    ringCase(CircleApprox({0.25L, 0}, 1.75L), fp);}
vector<point> randomSimple(std::mt19937_64 &rng, lng c, int n) {
    auto properCross = [](point a, point b, point p, point q) { return orient(a, b, p) * orient(a, b, q) < 0 && orient(p, q, a) * orient(p, q, b) < 0; };
    for (;;) {
        vector<point> p;
        for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}); }
        for (int it = 0; it < 100; ++it) {
            bool changed = false;
            for (int i = 0; i < n; ++i) { for (int j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) { continue; }
                if (properCross(p[i], p[i + 1], p[j], p[(j + 1) % n])) { reverse(p.begin() + i + 1, p.begin() + j + 1); changed = true; }}}
            if (!changed) { break; }}
        bool ok = signedArea2(p) != 0;
        for (int i = 0; i < n && ok; ++i) { for (int j = i + 2; j < n && ok; ++j) {
            if (i == 0 && j == n - 1) { continue; }
            ok = !properCross(p[i], p[i + 1], p[j], p[(j + 1) % n]) && !contains(Linear2::segment(p[i], p[i + 1]), p[j]) && !contains(Linear2::segment(p[j], p[(j + 1) % n]), p[i]);}}
        for (int i = 0; i < n && ok; ++i) { ok = p[i] != p[(i + 1) % n] && !(orient(p[i], p[(i + 1) % n], p[(i + 2) % n]) == 0 && dot(p[(i + 1) % n] - p[i], p[(i + 2) % n] - p[(i + 1) % n]) < 0); }
        if (ok) { return p; }}}
void invalid(const string &name) {
    if (name == "arcs-zero-radius") { (void)circleArcsInPolygonApprox(CircleApprox({0, 0}, 0), vector<point>{{0, 0}, {1, 0}, {0, 1}}); }
    if (name == "negative-radius") { (void)diskPolygonAreaApprox(CircleApprox({0, 0}, -1), vector<point>{{0, 0}, {1, 0}, {0, 1}}); }
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    fixed();
    int rounds = quick ? 150 : stress ? 30000 : 4000;
    triangleCases(rng, rounds);
    phase = "integer circles and polygons";
    for (int t = 0; t < rounds; ++t) {
        auto p = randomSimple(rng, 6, 3 + int(rng() % 8));
        CircleApprox c({(long double)(rng() % 13) - 6, (long double)(rng() % 13) - 6}, (long double)(1 + rng() % 8));
        if (rng() % 2) { reverse(p.begin(), p.end()); }
        ringCase(c, p);}
    phase = "real circles";
    for (int t = 0; t < rounds; ++t) {
        auto p = randomSimple(rng, 20, 3 + int(rng() % 10));
        std::uniform_real_distribution<long double> u(-20, 20), rr(0.1L, 25);
        ringCase(CircleApprox({u(rng), u(rng)}, rr(rng)), p);}
    phase = "pythagorean touches";
    vector<point> on{{5, 0}, {4, 3}, {3, 4}, {0, 5}, {-3, 4}, {-4, 3}, {-5, 0}, {-4, -3}, {-3, -4}, {0, -5}, {3, -4}, {4, -3}};
    for (int t = 0; t < rounds; ++t) {
        vector<point> p;
        int k = 3 + int(rng() % 6);
        for (int i = 0; i < k; ++i) { p.push_back(rng() % 2 ? on[rng() % on.size()] : point{lng(rng() % 15) - 7, lng(rng() % 15) - 7}); }
        sort(p.begin(), p.end()); p.erase(unique(p.begin(), p.end()), p.end());
        if (p.size() < 3) { continue; }
        std::shuffle(p.begin(), p.end(), rng);
        bool ok = true; int n = int(p.size());
        for (int i = 0; i < n && ok; ++i) { for (int j = i + 1; j < n && ok; ++j) {
            bool adj = j == i + 1 || (i == 0 && j == n - 1);
            ok = adj || !(orient(p[i], p[(i + 1) % n], p[j]) * orient(p[i], p[(i + 1) % n], p[(j + 1) % n]) <= 0 && orient(p[j], p[(j + 1) % n], p[i]) * orient(p[j], p[(j + 1) % n], p[(i + 1) % n]) <= 0);}}
        if (!ok || signedArea2(p) == 0) { continue; }
        ringCase(CircleApprox({0, 0}, 5), p);}
    phase = "holes random";
    for (int t = 0; t < rounds / 4; ++t) {
        lng s = 3 + lng(rng() % 6);
        vector<vector<point>> r{{{-10, -10}, {10, -10}, {10, 10}, {-10, 10}}, {{-s, -s}, {-s, s}, {s, s}, {s, -s}}};
        std::uniform_real_distribution<long double> u(-12, 12), rr(0.5L, 16);
        ringsCase(CircleApprox({u(rng), u(rng)}, rr(rng)), r);}
    phase = "large";
    int n = quick ? 2000 : stress ? 400000 : 200000; vector<point> big;
    for (int i = 0; i < n; ++i) {
        long double a = 2 * PI * i / n, r = i % 2 ? 9e8L : 4e8L;
        big.push_back({lng(std::llround(r * std::cos(a))), lng(std::llround(r * std::sin(a)))});}
    disk = CircleApprox({0, 0}, 6.5e8L); input = {};
    auto arcs = circleArcsInPolygonApprox(disk, big);
    require(int(arcs.size()) == n / 2, "large star: one arc per spike");
    long double area = diskPolygonAreaApprox(disk, big);
    require(area > 0 && area < PI * 6.5e8L * 6.5e8L, "large star area within the disk");
    std::cout << "PASS circle_polygon seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
