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
    std::cerr << " expected=true actual=false\n"; std::exit(1); }
bool near(long double a, long double b) { return std::abs(a - b) <= 1e-12L * max(1.L, std::abs(b)); }
using Cells = std::set<pair<int, int>>;
PolygonLocation cellLocation(const Cells &c, int x2, int y2) {
    int count = 0;
    for (int dx : {-1, 1}) { for (int dy : {-1, 1}) {
        int x = int(std::floor((4.L * x2 + dx) / 8)), y = int(std::floor((4.L * y2 + dy) / 8));
        count += c.count({x, y}); }}
    return count == 4 ? PolygonLocation::Inside : count ? PolygonLocation::Boundary : PolygonLocation::Outside; }
vector<point> histogram(const vector<int> &h) {
    int n = int(h.size()); vector<point> p{{0, 0}, {n, 0}, {n, h.back()}};
    for (int i = n - 1; i >= 0; --i) {
        p.push_back({i, h[i]});
        if (i && h[i - 1] != h[i]) { p.push_back({i, h[i - 1]}); }}
    return p; }
template<typename Polygon> void verifyCells(const Polygon &p, const Cells &cells, int bound) {
    lll sx = 0, sy = 0; lng perimeter = 0, boundary = 0, interior = 0;
    for (auto [x, y] : cells) {
        sx += 2 * x + 1; sy += 2 * y + 1;
        for (auto [dx, dy] : vector<pair<int, int>>{{1, 0}, {-1, 0}, {0, 1}, {0, -1}}) {
            perimeter += !cells.count({x + dx, y + dy}); }}
    auto m = polygonMoments(p); lll area = cells.size();
    require(m.area2 == 2 * area && m.x6 == 3 * sx && m.y6 == 3 * sy, "cell moments expected sums of rectangle integrals");
    require(signedArea2(p) == 2 * area && near(signedAreaApprox(p), (long double)area), "signed area");
    RationalPoint2 exact; dpoint approximate;
    require(centroidExact(p, exact) && exact == RationalPoint2(sx, sy, 2 * area), "exact cell centroid");
    require(centroidApprox(p, approximate) && near(approximate.x, (long double)sx / (2 * area)) && near(approximate.y, (long double)sy / (2 * area)), "approx cell centroid");
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
            boundary += expected == PolygonLocation::Boundary; interior += expected == PolygonLocation::Inside; }}}
    require(latticeBoundary(p) == boundary, "enumerated lattice boundary");
    require(latticeInterior(p) == interior, "enumerated Pick interior"); }
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
    phase = "signed winding"; p = {{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    auto m = polygonMoments(p); reverse(p.begin(), p.end()); auto r = polygonMoments(p);
    require(m.area2 == -r.area2 && m.x6 == -r.x6 && m.y6 == -r.y6, "reversed signed moments");
    require(polygonWinding(p, point{1, 1}).winding == -1 && latticeInterior(p) == 1, "reversed winding/Pick");
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
}
void invalid(const string &name) {
    if (name == "pick-empty") { (void)latticeInterior(vector<point>{}); }
    if (name == "pick-collinear") { (void)latticeInterior(vector<point>{{0, 0}, {1, 0}, {2, 0}}); }
    if (name == "holes-empty") { (void)latticeInterior(vector<vector<point>>{}); }
    if (name == "holes-orientation") { (void)latticeInterior(vector<vector<point>>{{{0, 0}, {0, 4}, {4, 4}, {4, 0}}}); }
    if (name == "lattice-bound") { (void)latticeBoundary(vector<point>{{1000000001, 0}}); }
    std::exit(2); }
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    basic(); std::mt19937_64 rng(seed);
    int rounds = mode == "quick" ? 15 : mode == "full" ? 180 : 1400;
    phase = "histogram cells";
    for (int t = 0; t < rounds; ++t) {
        int n = 1 + int(rng() % 8); vector<int> h(n); Cells cells;
        for (int x = 0; x < n; ++x) { h[x] = 1 + int(rng() % 8); for (int y = 0; y < h[x]; ++y) { cells.insert({x, y}); }}
        input = histogram(h); verifyCells(input, cells, 8);
        RationalPoint2 a, b; (void)centroidExact(input, a); reverse(input.begin(), input.end()); (void)centroidExact(input, b);
        require(a == b && polygonWinding(input, point{1, -1}).winding == 0, "orientation invariant centroid"); }
    phase = "exhaustive small histograms";
    int limit = mode == "quick" ? 2 : 4;
    for (int n = 1, count = 3; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<int> h(n); Cells small; int code = mask;
            for (int x = 0; x < n; ++x) {
                h[x] = code % 3 + 1; code /= 3;
                for (int y = 0; y < h[x]; ++y) { small.insert({x, y}); }}
            input = histogram(h); verifyCells(input, small, 4); }}
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
        require(c == RationalPoint2(lll(tri[0].x) + tri[1].x + tri[2].x, lll(tri[0].y) + tri[1].y + tri[2].y, 3), "triangle vertex mean"); }
    std::cout << "PASS polygon seed=" << seed << " mode=" << mode << " checks=" << checks << '\n'; }
