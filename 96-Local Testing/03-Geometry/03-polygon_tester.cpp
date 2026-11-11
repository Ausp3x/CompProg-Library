#include "../../03-Geometry/03-polygon.hpp"

// Independent oracle: union of unit grid cells. Area and moments sum rectangles;
// membership reads incident occupied cells, perimeter counts exposed cell sides.
static ulng seed = 20260927;
static vector<point> input;
static std::string phase;
static int checks = 0;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " input=";
    for (auto p : input) { std::cerr << '(' << p.x << ',' << p.y << ')'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
bool near(long double a, long double b) { return std::abs(a - b) <= 1e-12L * max(1.L, std::abs(b)); }
using Cells = std::set<pair<int, int>>;
PolygonLocation cellLocation(const Cells &c, int x2, int y2) {
    int count = 0;
    for (int dx : {-1, 1}) { for (int dy : {-1, 1}) {
        int x = int(std::floor((4.L * x2 + dx) / 8)), y = int(std::floor((4.L * y2 + dy) / 8));
        count += int(c.count({x, y}));}}
    return count == 4 ? PolygonLocation::Inside : count ? PolygonLocation::Boundary : PolygonLocation::Outside;}
vector<point> histogram(const vector<int> &h) {
    int n = int(h.size()); vector<point> p{{0, 0}, {n, 0}, {n, h.back()}};
    for (int i = n - 1; i >= 0; --i) {
        p.push_back({i, h[i]});
        if (i && h[i - 1] != h[i]) { p.push_back({i, h[i - 1]}); }}
    return p;}
// Area of a unit cell left of a -> b by exact piecewise-linear integration of column heights.
long double cellCut(int x, int y, point a, point b) {
    long double ux = (long double)(b.x - a.x), uy = (long double)(b.y - a.y);
    long double f0 = ux * ((long double)y - a.y) - uy * ((long double)x - a.x), beta = -uy, gamma = ux;
    auto height = [&](long double s) {
        long double v = f0 + beta * s;
        if (gamma == 0) { return v >= 0 ? 1.L : 0.L; }
        return std::clamp(gamma > 0 ? 1 + v / gamma : -v / gamma, 0.L, 1.L);};
    vector<long double> cuts{0, 1};
    if (beta != 0) {
        for (long double t : {0.L, 1.L}) {
            long double s = gamma == 0 ? -f0 / beta : (-gamma * t - f0) / beta;
            if (0 < s && s < 1) { cuts.push_back(s); }}}
    sort(cuts.begin(), cuts.end());
    long double area = 0;
    for (int i = 0; i + 1 < int(cuts.size()); ++i) {
        long double l = cuts[i], r = cuts[i + 1], m = (l + r) / 2;
        area += gamma == 0 ? (r - l) * height(m) : (r - l) * (height(l) + height(r)) / 2;}
    return area;}
void verifyCut(const vector<point> &p, const Cells &cells, point a, point b) {
    auto q = polygonCutApprox(p, a, b);
    require(q.size() <= 2 * p.size(), "cut output size bound");
    long double want = 0, got = 0, scale = 1;
    for (auto [x, y] : cells) {
        want += cellCut(x, y, a, b);
        require(std::abs(cellCut(x, y, a, b) + cellCut(x, y, b, a) - 1) <= 1e-15L, "oracle complementary half-planes cover a cell");}
    for (int i = 0; i < int(q.size()); ++i) {
        got += cross(q[i], q[i + 1 == int(q.size()) ? 0 : i + 1]) / 2;
        scale = max({scale, std::abs(q[i].x), std::abs(q[i].y)});
        dpoint d = q[i] - a.cast<long double>(), u = (b - a).cast<long double>();
        require(cross(u, d) >= -1e-12L * normApprox(u) * scale, "cut vertex in closed left half-plane");}
    require(std::abs(got - want) <= 1e-12L * max(1.L, want), "cut area equals cell-integrated half-plane area");}
template<typename Polygon> void verifyCells(const Polygon &p, const Cells &cells, int bound) {
    lll sx = 0, sy = 0; lng perimeter = 0, boundary = 0, interior = 0;
    for (auto [x, y] : cells) {
        sx += 2 * x + 1; sy += 2 * y + 1;
        for (auto [dx, dy] : vector<pair<int, int>>{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
            perimeter += !cells.count({x + dx, y + dy});}}
    auto m = polygonMoments(p); lll area = cells.size();
    require(m.area2 == 2 * area && m.x6 == 3 * sx && m.y6 == 3 * sy, "cell moments expected sums of rectangle integrals");
    require(signedArea2(p) == 2 * area && near(signedAreaApprox(p), (long double)area), "signed area");
    RationalPoint2 exact; dpoint approximate;
    require(centroidExact(p, exact) && exact == RationalPoint2(sx, sy, 2 * area), "exact cell centroid");
    require(centroidApprox(p, approximate) && near(approximate.x, (long double)sx / (long double)(2 * area)) && near(approximate.y, (long double)sy / (long double)(2 * area)), "approx cell centroid");
    require(near(polygonPerimeter(p), perimeter), "exposed cell perimeter");
    auto scaled = p;
    if constexpr (std::is_same_v<Polygon, vector<point>>) { for (auto &q : scaled) { q *= lng(2); } }
    else { for (auto &ring : scaled) { for (auto &q : ring) { q *= lng(2); }} }
    for (int x = -2; x <= 2 * bound + 2; ++x) { for (int y = -2; y <= 2 * bound + 2; ++y) {
        auto expected = cellLocation(cells, x, y);
        require(polygonContains(scaled, point{x, y}) == expected, "incident cell location");
        auto w = polygonWinding(scaled, point{x, y});
        require(w.boundary == (expected == PolygonLocation::Boundary), "winding boundary");
        if (!w.boundary) { require(w.winding == (expected == PolygonLocation::Inside), "winding number"); }
        if (x % 2 == 0 && y % 2 == 0) {
            boundary += expected == PolygonLocation::Boundary; interior += expected == PolygonLocation::Inside;}}}
    require(latticeBoundary(p) == boundary, "enumerated lattice boundary");
    require(latticeInterior(p) == interior, "enumerated Pick interior");
    if constexpr (std::is_same_v<Polygon, vector<point>>) {
        std::mt19937_64 rng(seed ^ ulng(p.size()));
        for (int t = 0; t < 6; ++t) {
            point a{lng(rng() % (bound + 5)) - 2, lng(rng() % (bound + 5)) - 2}, b{lng(rng() % (bound + 5)) - 2, lng(rng() % (bound + 5)) - 2};
            if (a != b) { verifyCut(p, cells, a, b); verifyCut(p, cells, b, a); }}
        verifyCut(p, cells, {0, 1}, {1, 1}); verifyCut(p, cells, {1, 0}, {1, 1});}}
template<typename X> concept Orientable = requires(X x) { makeCCW(x); };
static_assert(Orientable<vector<point>> && Orientable<vector<dpoint>> && !Orientable<vector<vector<point>>>);
void basic() {
    phase = "degenerate"; vector<point> p;
    require(signedArea2(p) == 0 && polygonPerimeter(p) == 0 && latticeBoundary(p) == 0, "empty scalar outputs");
    require(!polygonConvex(p) && polygonContains(p, point{}) == PolygonLocation::Outside, "empty topology");
    RationalPoint2 exact(7, 8); dpoint approximate{7, 8};
    require(!centroidExact(p, exact) && exact == RationalPoint2(7, 8), "empty centroid unchanged");
    require(!centroidApprox(p, approximate) && approximate == dpoint(7, 8), "empty approximate unchanged");
    p = {{2, 3}}; require(polygonContains(p, point{2, 3}) == PolygonLocation::Boundary && polygonPerimeter(p) == 0, "singleton");
    p = {{0, 0}, {3, 4}}; require(near(polygonPerimeter(p), 10) && !centroidExact(p, exact), "segment");
    p = {{0, 0}, {1, 0}, {2, 0}}; require(!polygonConvex(p, false) && !centroidApprox(p, approximate), "collinear");
    vector<vector<point>> empty;
    require(signedArea2(empty) == 0 && polygonMoments(empty).x6 == 0 && polygonPerimeter(empty) == 0 && latticeBoundary(empty) == 0, "empty rings");
    require(polygonContains(empty, point{}) == PolygonLocation::Outside && !centroidExact(empty, exact) && !centroidApprox(empty, approximate), "empty rings topology");
    phase = "convexity";
    p = {{0, 0}, {2, 0}, {2, 2}, {0, 2}}; require(polygonConvex(p) && polygonConvex(p, false), "strict square");
    p.insert(p.begin() + 1, {1, 0}); require(!polygonConvex(p) && polygonConvex(p, false), "weak square");
    reverse(p.begin(), p.end()); require(!polygonConvex(p) && polygonConvex(p, false), "reversed weak square");
    p.insert(p.begin() + 1, p[0]); require(!polygonConvex(p, false), "zero edge");
    p = {{0, 0}, {2, 0}, {1, 0}, {2, 2}, {0, 2}}; require(!polygonConvex(p, false), "backtracking");
    p = {{0, 0}, {3, 0}, {3, 1}, {1, 1}, {1, 3}, {0, 3}}; require(!polygonConvex(p) && !polygonConvex(p, false), "concave");
    p = {{0, 0}, {-1, -1}, {0, -2}, {3, 1}, {0, 4}, {-1, 3}, {2, 0}, {3, 0}}; require(!polygonConvex(p, false), "collinear backtrack between consistent turns");
    p = {{0, 0}, {2, 0}, {0, 2}}; require(polygonConvex(p) && polygonConvex(p, false), "strict triangle");
    reverse(p.begin(), p.end()); require(polygonConvex(p) && polygonConvex(p, false), "clockwise triangle");
    phase = "signed winding"; p = {{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    auto m = polygonMoments(p); reverse(p.begin(), p.end()); auto r = polygonMoments(p);
    require(m.area2 == -r.area2 && m.x6 == -r.x6 && m.y6 == -r.y6, "reversed signed moments");
    require(polygonWinding(p, point{1, 1}).winding == -1 && latticeInterior(p) == 1, "reversed winding/Pick");
    require(centroidApprox(p, approximate) && approximate == dpoint(1, 1) && centroidExact(p, exact) && exact == RationalPoint2(1, 1), "clockwise centroid");
    vector<vector<point>> holed{{{0, 0}, {6, 0}, {6, 6}, {0, 6}}, {{1, 1}, {1, 4}, {4, 1}}};
    int open = 0;
    for (int x = 1; x <= 5; ++x) { for (int y = 1; y <= 5; ++y) { open += x + y > 5; }}
    require(latticeInterior(holed) == open && latticeBoundary(holed) == 33, "triangle hole Pick against enumeration");
    auto twice = p; twice.insert(twice.end(), p.begin(), p.end());
    require(polygonWinding(twice, point{1, 1}).winding == -2 && polygonContains(twice, point{1, 1}) == PolygonLocation::Inside, "nonzero winding fill");
    phase = "floating dyadic"; vector<dpoint> f{{0, 0}, {1.5L, 0}, {1.5L, .5L}, {0, .5L}};
    require(near(signedAreaApprox(f), .75L) && near(polygonPerimeter(f), 4) && polygonConvex(f), "floating metrics");
    require(centroidApprox(f, approximate) && approximate == dpoint(.75L, .25L), "floating centroid");
    require(polygonContains(f, dpoint{.5L, .25L}) == PolygonLocation::Inside && polygonContains(f, dpoint{0, .25L}) == PolygonLocation::Boundary, "floating location");
    vector<vector<dpoint>> fr{f}; require(near(polygonMoments(fr).area2, 1.5L) && centroidApprox(fr, approximate), "floating rings");
    phase = "extreme"; lng c = 1000000000; p = {{-c, -c}, {c, -c}, {c, c}, {-c, c}}; input = p;
    require(signedArea2(p) == lll(8) * c * c && polygonMoments(p).x6 == 0, "wide area/moments");
    require(centroidExact(p, exact) && exact == RationalPoint2(0, 0) && latticeBoundary(p) == lll(8) * c, "wide centroid/boundary");
    require(latticeInterior(p) == lll(2 * c - 1) * (2 * c - 1), "wide Pick");
    vector<Point2<int>> pi{{0, 0}, {3, 0}, {0, 3}};
    require(centroidExact(pi, exact) && exact == RationalPoint2(1, 1) && latticeInterior(pi) == 1, "int coordinates");
    phase = "cut regressions";
    require(polygonCutApprox(vector<point>{}, point{0, 0}, point{1, 0}).empty(), "empty cut");
    vector<point> square = p;
    auto half = polygonCutApprox(square, point{-c, -c}, point{c, c});
    require(std::abs(signedAreaApprox(half) - 2.L * c * c) <= 1e-15L * c * c && half.size() == 3, "wide diagonal cut keeps upper-left triangle");
    require(polygonCutApprox(square, point{-c, 0}, point{-c, 1}).size() == 2, "cut on an edge line keeps a zero-area edge");
    require(std::abs(signedAreaApprox(polygonCutApprox(square, point{c, 0}, point{c, 1}))) == 4.L * c * c, "whole polygon kept");
    vector<point> notch{{0, 0}, {4, 0}, {4, 4}, {3, 4}, {3, 1}, {1, 1}, {1, 4}, {0, 4}};
    auto u = polygonCutApprox(notch, point{0, 2}, point{4, 2});
    require(std::abs(signedAreaApprox(u) - 4) <= 1e-15L && u.size() == 8, "nonconvex cut keeps zero-width bridge and exact area");
    vector<Point2<double>> fp{{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    require(std::abs(signedAreaApprox(polygonCutApprox(fp, Point2<double>{0, 0.5}, Point2<double>{1, 0.5})) - 3) <= 1e-15L, "floating cut");}
// Independent orientation oracle: exact trapezoid sum, -2 * area in quarter units for floating rings.
template<typename T> lll fixed(T v) {
    if constexpr (std::is_integral_v<T>) { return lll(v); }
    else { return lll(v * 4); }}
template<typename T> lll trapezoid(const vector<Point2<T>> &p) {
    lll s = 0; int n = int(p.size());
    for (int i = 0; i < n; ++i) {
        auto a = p[i], b = p[i + 1 == n ? 0 : i + 1];
        s += (fixed(b.x) - fixed(a.x)) * (fixed(b.y) + fixed(a.y));}
    return s;}
template<typename T> void verifyCCW(const vector<Point2<T>> &p) {
    input.clear();
    for (auto q : p) { input.push_back({lng(fixed(q.x)), lng(fixed(q.y))}); }
    auto want = p, got = makeCCW(p);
    if (trapezoid(p) > 0) { reverse(want.begin(), want.end()); }
    require(got == want, "makeCCW reverses exactly the negative trapezoid rings");
    require(makeCCW(got) == got, "makeCCW idempotent");
    require(trapezoid(got) <= 0 && signedArea2(got) >= 0, "makeCCW nonnegative area");}
template<typename T> vector<Point2<T>> randomRing(std::mt19937_64 &rng, lng bound, T unit) {
    vector<Point2<T>> p(rng() % 9);
    for (auto &q : p) { q = {T(lng(rng() % ulng(2 * bound + 1)) - bound) / unit, T(lng(rng() % ulng(2 * bound + 1)) - bound) / unit}; }
    return p;}
template<typename T> void orientationRounds(std::mt19937_64 &rng, int rounds, lng bound, T unit) {
    for (int t = 0; t < rounds; ++t) {
        for (lng b : {lng(2), bound}) {
            auto p = randomRing<T>(rng, b, unit); verifyCCW(p);
            auto back = p; back.insert(back.end(), p.rbegin(), p.rend()); verifyCCW(back);
            reverse(p.begin(), p.end()); verifyCCW(p);}}}
void orientation(int rounds) {
    phase = "orientation cases"; const lng c = 1000000000; std::mt19937_64 rng(~seed);
    input = {{0, 0}, {0, 1}, {1, 0}}; require(makeCCW(vector<point>{{0, 0}, {0, 1}, {1, 0}}) == vector<point>{{1, 0}, {0, 1}, {0, 0}}, "makeCCW point ring");
    vector<vector<point>> cases{{}, {{2, 3}}, {{0, 0}, {3, 4}}, {{3, 4}, {0, 0}}, {{0, 0}, {1, 0}, {2, 0}, {5, 0}}, {{0, 0}, {5, 0}, {1, 0}, {3, 0}},
        {{0, 0}, {0, 2}, {2, 2}, {2, 0}}, {{0, 0}, {2, 0}, {2, 2}, {0, 2}}, {{0, 0}, {2, 2}, {2, 0}, {0, 2}}, {{0, 0}, {4, 4}, {4, 0}, {0, 2}},
        {{0, 0}, {0, 1}, {1, 1}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {1, 0}}, {{1, 1}, {1, 1}, {1, 1}}, {{-c, -c}, {-c, c}, {c, c}, {c, -c}},
        {{-c, -c}, {c, c}, {c - 1, c - 1}}, {{-c, -c}, {c, c}, {c, c - 1}}, {{-c, -c}, {c, c - 1}, {c, c}}, {{c, -c}, {-c, c}, {c, c}, {-c, -c}}};
    for (auto &p : cases) { verifyCCW(p); }
    require(trapezoid(cases[9]) > 0 && trapezoid(cases[8]) == 0 && makeCCW(cases[8]) == cases[8] && makeCCW(cases[9]) != cases[9], "self-intersecting zero and negative rings");
    vector<Point2<int>> ci{{int(c), int(-c)}, {int(-c), int(-c)}, {int(-c), int(c)}}; verifyCCW(ci);
    vector<dpoint> cf{{0, 0}, {.25L, 1.5L}, {.5L, 0}}; verifyCCW(cf);
    require(makeCCW(cf) == vector<dpoint>{{.5L, 0}, {.25L, 1.5L}, {0, 0}}, "floating clockwise reversed");
    phase = "orientation random lng"; orientationRounds<lng>(rng, 8 * rounds, c, 1);
    phase = "orientation random int"; orientationRounds<int>(rng, 8 * rounds, c, 1);
    phase = "orientation random long double quarters"; orientationRounds<long double>(rng, 8 * rounds, lng(1) << 22, 4);}
void invalid(const string &name) {
    if (name == "pick-empty") { (void)latticeInterior(vector<point>{}); }
    if (name == "pick-collinear") { (void)latticeInterior(vector<point>{{0, 0}, {1, 0}, {2, 0}}); }
    if (name == "holes-empty") { (void)latticeInterior(vector<vector<point>>{}); }
    if (name == "holes-orientation") { (void)latticeInterior(vector<vector<point>>{{{0, 0}, {0, 4}, {4, 4}, {4, 0}}}); }
    if (name == "holes-flat") { (void)latticeInterior(vector<vector<point>>{{{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {{1, 1}, {2, 2}, {3, 3}}}); }
    if (name == "holes-zero-total") { (void)latticeInterior(vector<vector<point>>{{{0, 0}, {2, 0}, {2, 2}, {0, 2}}, {{0, 2}, {2, 2}, {2, 0}, {0, 0}}}); }
    if (name == "holes-negative-total") { (void)latticeInterior(vector<vector<point>>{{{0, 0}, {2, 0}, {2, 2}, {0, 2}}, {{0, 4}, {4, 4}, {4, 0}, {0, 0}}}); }
    if (name == "cut-singleton") { (void)polygonCutApprox(vector<point>{{0, 0}, {1, 0}, {0, 1}}, point{}, point{}); }
    if (name == "lattice-bound") { (void)latticeBoundary(vector<point>{{1000000001, 0}}); }
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    basic(); std::mt19937_64 rng(seed);
    int rounds = mode == "quick" ? 15 : mode == "full" ? 180 : 1400;
    orientation(rounds);
    phase = "histogram cells";
    for (int t = 0; t < rounds; ++t) {
        int n = 1 + int(rng() % 8); vector<int> h(n); Cells cells;
        for (int x = 0; x < n; ++x) { h[x] = 1 + int(rng() % 8); for (int y = 0; y < h[x]; ++y) { cells.insert({x, y}); }}
        input = histogram(h); verifyCells(input, cells, 8);
        RationalPoint2 a, b; (void)centroidExact(input, a); reverse(input.begin(), input.end()); (void)centroidExact(input, b);
        require(a == b && polygonWinding(input, point{1, -1}).winding == 0, "orientation invariant centroid");}
    phase = "exhaustive small histograms";
    int limit = mode == "quick" ? 2 : 4;
    for (int n = 1, count = 3; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<int> h(n); Cells small; int code = mask;
            for (int x = 0; x < n; ++x) {
                h[x] = code % 3 + 1; code /= 3;
                for (int y = 0; y < h[x]; ++y) { small.insert({x, y}); }}
            input = histogram(h); verifyCells(input, small, 4);}}
    phase = "long wide walk";
    vector<point> walk; int copies = mode == "quick" ? 20 : mode == "full" ? 10000 : 100000;
    const lng c = 1000000000;
    for (int i = 0; i < copies; ++i) { walk.insert(walk.end(), {{c, c}, {c, -c}, {-c, c}}); }
    auto wide = polygonMoments(walk); RationalPoint2 center;
    require(wide.area2 == -lll(4) * c * c * copies && wide.x6 == -lll(4) * c * c * c * copies && wide.y6 == wide.x6, "wide accumulated moments");
    require(centroidExact(walk, center) && center == RationalPoint2(c, c, 3), "wide accumulated centroid");
    require(polygonWinding(walk, point{1, 1}).winding == -copies, "large winding multiplicity");
    phase = "holes cells";
    vector<vector<point>> rings{{{0, 0}, {8, 0}, {8, 8}, {0, 8}}, {{1, 1}, {1, 3}, {3, 3}, {3, 1}}, {{5, 4}, {5, 7}, {7, 7}, {7, 4}}};
    Cells cells;
    for (int x = 0; x < 8; ++x) { for (int y = 0; y < 8; ++y) {
        if (!((1 <= x && x < 3 && 1 <= y && y < 3) || (5 <= x && x < 7 && 4 <= y && y < 7))) { cells.insert({x, y}); }}}
    input = rings[0]; verifyCells(rings, cells, 8);
    phase = "triangle barycenter";
    for (int t = 0; t < rounds; ++t) {
        vector<point> tri;
        for (int k = 0; k < 3; ++k) { tri.push_back({lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}); }
        input = tri; RationalPoint2 c;
        if (!centroidExact(tri, c)) { continue; }
        require(c == RationalPoint2(lll(tri[0].x) + tri[1].x + tri[2].x, lll(tri[0].y) + tri[1].y + tri[2].y, 3), "triangle vertex mean");}
    std::cout << "PASS polygon seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
