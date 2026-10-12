#include "../../03-Geometry/17-polygon_distance.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

// Oracles: all edge pairs through 02 segmentDistanceApprox and intersect, candidate points tested by exact rational
// membership, exact all-pairs farthest search, and exact rational squared distances for point queries.
static ulng seed = 20260927;
static std::string phase;
static vector<point> lhs, rhs;
static lng checks = 0;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " p=";
    for (auto x : lhs) { std::cerr << '(' << x.x << ',' << x.y << ')'; }
    std::cerr << " q=";
    for (auto x : rhs) { std::cerr << '(' << x.x << ',' << x.y << ')'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
// Closed-region membership of the rational point x / d: degree-4 terms (b - a) x (q - a * d) fit lll for 02 intersection points.
bool containsQ(const vector<point> &p, const RationalPoint2 &x) {
    int n = int(p.size());
    auto side = [&](point a, point b) { return (lll(b.x) - a.x) * (x.y - lll(a.y) * x.d) - (lll(b.y) - a.y) * (x.x - lll(a.x) * x.d); };
    if (n == 1) { return x == RationalPoint2(p[0]); }
    for (int i = 0; i < n; ++i) {
        point a = p[i], b = p[(i + 1) % n];
        bool box = lll(min(a.x, b.x)) * x.d <= x.x && x.x <= lll(max(a.x, b.x)) * x.d && lll(min(a.y, b.y)) * x.d <= x.y && x.y <= lll(max(a.y, b.y)) * x.d;
        if (!side(a, b) && box) { return true; }}
    bool in = false;
    for (int i = 0; i < n; ++i) {
        point a = p[i], b = p[(i + 1) % n];
        if ((lll(a.y) * x.d > x.y) != (lll(b.y) * x.d > x.y) && (side(a, b) > 0) == (b.y > a.y)) { in = !in; }}
    return in;}
bool intersectOracle(const vector<point> &p, const vector<point> &q) {
    vector<RationalPoint2> cand;
    for (auto v : p) { cand.push_back(v); }
    for (auto v : q) { cand.push_back(v); }
    int n = int(p.size()), m = int(q.size());
    for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) {
        auto x = intersect(Linear2::segment(p[i], p[(i + 1) % n]), Linear2::segment(q[j], q[(j + 1) % m]));
        if (x.kind == IntersectionKind::Point) { cand.push_back(x.p); }
        if (x.kind == IntersectionKind::Segment) { cand.push_back(x.overlap.a); }}}
    for (auto &c : cand) { if (containsQ(p, c) && containsQ(q, c)) { return true; } }
    return false;}
long double distanceOracle(const vector<point> &p, const vector<point> &q) {
    if (intersectOracle(p, q)) { return 0; }
    long double res = 1e300L; int n = int(p.size()), m = int(q.size());
    for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) {
        res = min(res, segmentDistanceApprox(p[i], p[(i + 1) % n], q[j], q[(j + 1) % m]));}}
    return res;}
// Distance from a rounded witness to a closed integral polygon.
long double gapTo(const vector<point> &p, dpoint x) {
    vector<dpoint> f; for (auto v : p) { f.push_back(v.cast<long double>()); }
    if (f.size() >= 3 && polygonContains(f, x) != PolygonLocation::Outside) { return 0; }
    long double res = 1e300L; int n = int(f.size());
    for (int i = 0; i < n; ++i) { res = min(res, pointSegmentDistanceApprox(x, f[i], f[(i + 1) % n])); }
    return res;}
long double scale(const vector<point> &p, const vector<point> &q) {
    long double s = 1;
    for (auto &r : {p, q}) { for (auto v : r) { s = max({s, std::abs((long double)v.x), std::abs((long double)v.y)}); } }
    return s;}
void witness(const vector<point> &p, const vector<point> &q, const PolygonDistanceApprox &r, long double want, const char *what) {
    long double s = scale(p, q), tol = 1e-15L * s * 8;
    require(std::abs(r.distance - want) <= tol, what);
    require(gapTo(p, r.p) <= tol && gapTo(q, r.q) <= tol, "witnesses lie in their polygons");
    require(std::abs(distanceApprox(r.p, r.q) - r.distance) <= tol, "witness distance equals the reported distance");
    if (want == 0) { require(r.p == r.q, "meeting polygons share one witness"); }}
void convexCase(vector<point> p, vector<point> q, std::mt19937_64 &rng) {
    lhs = p; rhs = q;
    std::rotate(p.begin(), p.begin() + rng() % p.size(), p.end()); std::rotate(q.begin(), q.begin() + rng() % q.size(), q.end());
    bool meet = intersectOracle(p, q); long double want = distanceOracle(p, q);
    witness(p, q, convexPolygonDistanceApprox(p, q), want, "convex distance equals all-pairs distance");
    point axis{7, 7}; bool sep = convexSeparated(p, q, axis);
    require(sep == !meet, "separated iff no common point");
    if (sep) {
        lll lo = dot(axis, p[0]), hi = dot(axis, q[0]);
        for (auto v : p) { lo = min(lo, dot(axis, v)); }
        for (auto v : q) { hi = max(hi, dot(axis, v)); }
        require(lo > hi && gcd(abs(axis.x), abs(axis.y)) == 1, "primitive axis separates the projections");}
    else { require(axis == point{7, 7}, "axis untouched when the polygons meet"); }
    auto f = convexPolygonMaxDistance(p, q); ConvexDiameter g;
    for (int i = 0; i < int(p.size()); ++i) { for (int j = 0; j < int(q.size()); ++j) {
        lll s = dist2(p[i], q[j]);
        if (g.first < 0 || s > g.squared) { g = {i, j, s}; }}}
    require(f.first == g.first && f.second == g.second && f.squared == g.squared, "farthest pair equals all-pairs maximum with smallest indices");
    require(polygonsIntersect(p, q) == meet, "general intersect agrees on convex input");
    witness(p, q, polygonDistanceApprox(p, q), want, "general distance on convex input");}
void generalCase(const vector<point> &p, const vector<point> &q) {
    lhs = p; rhs = q;
    bool meet = intersectOracle(p, q);
    require(polygonsIntersect(p, q) == meet && polygonsIntersect(q, p) == meet, "general intersect equals candidate-point oracle");
    witness(p, q, polygonDistanceApprox(p, q), distanceOracle(p, q), "general distance equals all-pairs distance");}
void pointCase(const vector<point> &p, point x) {
    lhs = p; rhs = {x};
    int n = int(p.size()); bool in = containsQ(p, x);
    long double want = 0;
    if (!in) {
        // Exact rational squared distance minimised over edges, rounded once.
        lll bn = -1, bd = 1;
        for (int i = 0; i < n; ++i) {
            point a = p[i], b = p[(i + 1) % n], e = b - a; lll t = dot(x - a, e), l = norm2(e), num, den;
            if (l == 0 || t <= 0) { num = dist2(x, a); den = 1; }
            else if (t >= l) { num = dist2(x, b); den = 1; }
            else { lll c = cross(e, x - a); num = c * c; den = l; }
            if (bn < 0 || ConvexCalipers::ratioLess(ulll(num), ulll(den), ulll(bn), ulll(bd))) { bn = num; bd = den; }}
        want = std::sqrt(static_cast<long double>(bn) / static_cast<long double>(bd));}
    long double got = pointPolygonDistanceApprox(p, x);
    require(std::abs(got - want) <= 1e-15L * max(1.L, want) * 4, "point distance equals exact rational minimum");
    require(pointPolygonDistanceApprox(vector<vector<point>>{p}, x) == got, "rings overload agrees");}
vector<point> randomPoints(std::mt19937_64 &rng, lng c, int n) {
    vector<point> p;
    for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}); }
    return p;}
bool simple(const vector<point> &p) {
    int n = int(p.size());
    if (n < 3 || signedArea2(p) == 0) { return false; }
    for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) {
        point a = p[i], b = p[(i + 1) % n], c = p[j], d = p[(j + 1) % n];
        bool adjacent = j == i + 1 || (i == 0 && j == n - 1);
        if (adjacent) {
            point s = j == i + 1 ? a : b, t = j == i + 1 ? d : c, m = j == i + 1 ? b : a;
            if (orient(s, m, t) == 0 && dot(s - m, t - m) > 0) { return false; }
            continue;}
        if (polygon_distance_detail::touch(a, b, c, d)) { return false; }}}
    return true;}
vector<point> randomSimple(std::mt19937_64 &rng, lng c, int n, point shift) {
    for (;;) {
        auto p = randomPoints(rng, c, n);
        sort(p.begin(), p.end()); if (std::unique(p.begin(), p.end()) != p.end()) { continue; }
        std::shuffle(p.begin(), p.end(), rng);
        for (int it = 0; it < 200 && !simple(p); ++it) {
            for (int i = 0; i < n; ++i) { for (int j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) { continue; }
                point a = p[i], b = p[i + 1], x = p[j], y = p[(j + 1) % n];
                if (orient(a, b, x) * orient(a, b, y) < 0 && orient(x, y, a) * orient(x, y, b) < 0) { reverse(p.begin() + i + 1, p.begin() + j + 1); }}}}
        if (simple(p)) { for (auto &v : p) { v += shift; } return p; }}}
void fixed(std::mt19937_64 &rng) {
    phase = "fixed";
    vector<point> sq{{0, 0}, {2, 0}, {2, 2}, {0, 2}}, tri{{5, 0}, {7, 0}, {6, 3}}, pt{{3, 1}}, seg{{2, 2}, {4, 4}}, far{{10, 10}};
    for (auto &a : {sq, tri, pt, seg, far}) { for (auto &b : {sq, tri, pt, seg, far}) { convexCase(a, b, rng); } }
    vector<point> touching{{2, 1}, {4, 0}, {4, 2}}, inner{{1, 1}};
    convexCase(sq, touching, rng); convexCase(sq, inner, rng); convexCase(sq, sq, rng);
    vector<point> big{{-3, -3}, {5, -3}, {5, 5}, {-3, 5}};
    convexCase(sq, big, rng);
    lhs = sq; rhs = tri;
    auto r = convexPolygonDistanceApprox(sq, tri);
    require(r.distance == 3 && r.p == dpoint(2, 0) && r.q == dpoint(5, 0), "square to triangle witnesses");
    phase = "wide";
    const lng C = 1000000000;
    vector<point> w1{{-C, -C}, {-1, -C}, {-1, C}, {-C, C}}, w2{{1, -C}, {C, -C}, {C, C}, {1, C}}, w3{{0, -C}, {C, 0}, {0, C}};
    convexCase(w1, w2, rng); convexCase(w1, w3, rng); convexCase(w2, w3, rng);
    lhs = w1; rhs = w2;
    auto f = convexPolygonMaxDistance(w1, w2);
    require(f.squared == lll(2 * C) * (2 * C) + lll(2 * C) * (2 * C), "wide farthest squared distance");
    phase = "nonconvex fixed";
    vector<point> u{{0, 0}, {6, 0}, {6, 6}, {4, 6}, {4, 2}, {2, 2}, {2, 6}, {0, 6}}, inside{{3, 4}, {3, 5}, {3, 4}};
    vector<point> slot{{3, 3}, {3, 5}, {2, 4}};
    generalCase(u, {{3, 4}}); generalCase(u, slot); generalCase(u, {{2, 7}, {4, 7}, {3, 9}}); generalCase(u, {{-5, -5}, {12, -5}, {12, 12}, {-5, 12}});
    for (auto x : {point{3, 4}, point{1, 1}, point{7, 7}, point{2, 3}, point{3, 2}, point{-1, 3}}) { pointCase(u, x); }
    vector<vector<point>> holes{{{0, 0}, {9, 0}, {9, 9}, {0, 9}}, {{3, 3}, {3, 6}, {6, 6}, {6, 3}}};
    lhs = holes[0]; rhs = holes[1];
    require(std::abs(pointPolygonDistanceApprox(holes, point{4, 5}) - 1) < 1e-18L && pointPolygonDistanceApprox(holes, point{1, 1}) == 0, "distance into a hole");
    vector<Point2<int>> ip{{0, 0}, {2, 0}, {2, 2}, {0, 2}}, iq{{5, 0}, {7, 0}, {6, 3}};
    require(convexPolygonDistanceApprox(ip, iq).distance == 3 && !polygonsIntersect(ip, iq), "int coordinates");}
void invalid(const string &name) {
    if (name == "empty") { (void)convexPolygonDistanceApprox(vector<point>{}, vector<point>{{0, 0}}); }
    if (name == "clockwise") { point axis; (void)convexSeparated(vector<point>{{0, 0}, {0, 1}, {1, 0}}, vector<point>{{5, 5}}, axis); }
    if (name == "coordinate-bound") { (void)convexPolygonMaxDistance(vector<point>{{2000000000, 0}}, vector<point>{{0, 0}}); }
    if (name == "point-empty") { (void)pointPolygonDistanceApprox(vector<point>{}, point{}); }
    if (name == "intersect-empty") { (void)polygonsIntersect(vector<point>{}, vector<point>{{0, 0}}); }
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    fixed(rng);
    int rounds = quick ? 300 : stress ? 60000 : 8000;
    phase = "convex random";
    for (int t = 0; t < rounds; ++t) {
        lng c = t % 4 == 3 ? 1000000000 : 6;
        auto p = convexHull(randomPoints(rng, c, 1 + int(rng() % 12))), q = convexHull(randomPoints(rng, c, 1 + int(rng() % 12)));
        if (t % 4 != 3 && rng() % 2) { point s{lng(rng() % 13) - 6, lng(rng() % 13) - 6}; for (auto &v : q) { v += s; } }
        convexCase(p, q, rng);}
    phase = "general random";
    for (int t = 0; t < rounds / 2; ++t) {
        auto p = randomSimple(rng, 4, 3 + int(rng() % 7), {0, 0});
        auto q = rng() % 3 ? randomSimple(rng, 4, 3 + int(rng() % 7), {lng(rng() % 9) - 4, lng(rng() % 9) - 4}) : randomPoints(rng, 6, 1 + int(rng() % 2));
        if (q.size() == 2 && q[0] == q[1]) { q.pop_back(); }
        if (rng() % 2) { reverse(p.begin(), p.end()); }
        generalCase(p, q);}
    phase = "point queries";
    for (int t = 0; t < rounds; ++t) {
        auto p = randomSimple(rng, t % 3 ? 5 : 1000000000, 3 + int(rng() % 8), {0, 0});
        point x = t % 3 ? point{lng(rng() % 15) - 7, lng(rng() % 15) - 7} : randomPoints(rng, 1000000000, 1)[0];
        pointCase(p, x);}
    phase = "large convex";
    int n = quick ? 2000 : stress ? 400000 : 200000; vector<point> a, b;
    for (int i = 0; i < n; ++i) {
        long double ang = 2 * std::numbers::pi_v<long double> * i / n;
        a.push_back({lng(std::llround(4e8L * std::cos(ang))) - 500000000, lng(std::llround(4e8L * std::sin(ang)))});
        b.push_back({lng(std::llround(4e8L * std::cos(ang))) + 500000000, lng(std::llround(4e8L * std::sin(ang)))});}
    a = convexHull(a); b = convexHull(b); lhs = {}; rhs = {};
    auto r = convexPolygonDistanceApprox(a, b);
    require(std::abs(r.distance - 2e8L) < 1.0L && !polygonsIntersect(vector<point>{a[0]}, vector<point>{b[0]}), "large hull distance");
    point axis; require(convexSeparated(a, b, axis) && axis.x < 0, "large hull separated along x");
    std::cout << "PASS polygon_distance seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
