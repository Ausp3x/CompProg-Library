#include "../../03-Geometry/15-convex_polygon_query.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

// Oracles: 03 polygonContains, edge scans, 10 clipConvex, exhaustive k-subsets, an all-roots k-gon DP, exact line parameters.
static ulng seed = 20260927;
static std::string phase;
static vector<point> poly;
static lng checks = 0;
static std::string context;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " polygon=";
    for (auto x : poly) { std::cerr << '(' << x.x << ',' << x.y << ')'; }
    std::cerr << " " << context << " expected=true actual=false\n"; std::exit(1);}
vector<point> randomPoints(std::mt19937_64 &rng, lng c, int n) {
    vector<point> p(n);
    for (auto &x : p) { x = {lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}; }
    return p;}
vector<point> randomConvex(std::mt19937_64 &rng, lng c, int n) {
    while (true) {
        auto h = convexHull(randomPoints(rng, c, n));
        if (h.size() >= 3) {
            std::rotate(h.begin(), h.begin() + rng() % h.size(), h.end());
            return h;}}}
int n0() { return int(poly.size()); }
// Exact rational comparisons of a / b and c / d, b, d > 0, small values.
bool fracLess(lll a, lll b, lll c, lll d) { return a * d < c * b; }

void queryCase(const ConvexPolygonQuery &cq, point q) {
    context = "q=(" + std::to_string(q.x) + "," + std::to_string(q.y) + ")"; int n = n0();
    PolygonLocation loc = polygonContains(poly, q);
    require(cq.contains(q) == loc, "contains equals polygonContains");
    long double want = 0;
    if (loc == PolygonLocation::Outside) {
        want = std::numeric_limits<long double>::infinity();
        for (int i = 0; i < n; ++i) { want = min(want, pointSegmentDistanceApprox(q, poly[i], poly[(i + 1) % n])); }
        vector<int> vis(n);
        for (int i = 0; i < n; ++i) { vis[i] = orient(poly[i], poly[(i + 1) % n], q) < 0; }
        int s = -1, e = -1;
        for (int i = 0; i < n; ++i) {
            if (vis[i] && !vis[(i + n - 1) % n]) { s = i; }
            if (vis[i] && !vis[(i + 1) % n]) { e = (i + 1) % n; }}
        require(cq.tangents(q) == pair<int, int>{s, e}, "tangents equal the visible chain ends");}
    long double scale = 1;
    for (auto v : poly) { scale = max({scale, std::abs((long double)v.x), std::abs((long double)v.y)}); }
    require(std::abs(cq.pointDistanceApprox(q) - want) <= 1e-17L * scale, "pointDistanceApprox equals the edge minimum");}
void extremeCase(const ConvexPolygonQuery &cq, point d) {
    context = "d=(" + std::to_string(d.x) + "," + std::to_string(d.y) + ")"; int n = n0(); lll best = dot(d, poly[0]);
    for (auto v : poly) { best = max(best, dot(d, v)); }
    int want = -1;
    for (int i = 0; i < n; ++i) {
        if (dot(d, poly[i]) == best && (want < 0 || dot(d, poly[(i + n - 1) % n]) != best)) { want = i; }}
    require(cq.extreme(d) == want, "extreme equals the argmax, first of a tied pair");}
// Parameter t = dot(x - a, w) / |w|^2 of a rational point on the line a + t * w.
pair<lll, lll> param(const RationalPoint2 &x, point a, point w) {
    lll num = (x.x - lll(a.x) * x.d) * w.x + (x.y - lll(a.y) * x.d) * w.y, den = x.d * norm2(w);
    return {num, den};}
void lineCase(const ConvexPolygonQuery &cq, Linear2 s) {
    context = "s=(" + std::to_string(s.a.x) + "," + std::to_string(s.a.y) + ")->(" + std::to_string(s.b.x) + "," + std::to_string(s.b.y) + ") kind=" + std::to_string(int(s.kind));
    int n = n0(); auto r = cq.lineIntersection(s);
    if (s.a == s.b) {
        bool in = polygonContains(poly, s.a) != PolygonLocation::Outside;
        require(r.count == int(in) && (!in || r.points[0] == RationalPoint2(s.a)), "singleton query");
        return;}
    point w = s.b - s.a; bool any = false; pair<lll, lll> lo{0, 1}, hi{0, 1};
    auto add = [&](const RationalPoint2 &x) {
        auto t = param(x, s.a, w);
        if (!any || fracLess(t.first, t.second, lo.first, lo.second)) { lo = t; }
        if (!any || fracLess(hi.first, hi.second, t.first, t.second)) { hi = t; }
        any = true;};
    for (int i = 0; i < n; ++i) {
        auto x = intersect(Linear2::line(s.a, s.b), Linear2::segment(poly[i], poly[(i + 1) % n]));
        if (x.kind == IntersectionKind::Point) { add(x.p); }
        if (x.kind == IntersectionKind::Segment) { add(RationalPoint2(x.overlap.a)); add(RationalPoint2(x.overlap.b)); }}
    if (any && s.kind != LinearKind::Line && fracLess(lo.first, lo.second, 0, 1)) { lo = {0, 1}; }
    if (any && s.kind == LinearKind::Segment && fracLess(1, 1, hi.first, hi.second)) { hi = {1, 1}; }
    if (any && fracLess(hi.first, hi.second, lo.first, lo.second)) { any = false; }
    if (!any) { require(r.count == 0, "line misses the polygon"); return; }
    bool point_only = lo.first * hi.second == hi.first * lo.second;
    require(r.count == (point_only ? 1 : 2), "line intersection count");
    auto t0 = param(r.points[0], s.a, w);
    require(t0.first * lo.second == lo.first * t0.second, "first point at the smallest parameter");
    if (!point_only) {
        auto t1 = param(r.points[1], s.a, w);
        require(t1.first * hi.second == hi.first * t1.second, "second point at the largest parameter");}}
bool sameSet(const vector<RationalPoint2> &a, const vector<RationalPoint2> &b) {
    if (a.size() != b.size()) { return false; }
    for (const auto &x : a) {
        bool found = false;
        for (const auto &y : b) { found |= x == y; }
        if (!found) { return false; }}
    return true;}
void cutCase(const ConvexPolygonQuery &cq, point a, point b) {
    context = "line=(" + std::to_string(a.x) + "," + std::to_string(a.y) + ")->(" + std::to_string(b.x) + "," + std::to_string(b.y) + ")";
    auto got = cq.cut(a, b); auto want = clipConvex(poly, {Halfplane{a, b}});
    require(sameSet(got, want.vertices), "cut equals clipConvex");
    require((want.kind == HalfplaneKind::Polygon) == (got.size() >= 3), "cut is a polygon exactly when clipConvex is");
    long double area = 0; int m = int(got.size());
    for (int i = 0; i < m && m >= 3; ++i) {
        dpoint x = got[i].approx(), y = got[(i + 1) % m].approx();
        area += x.x * y.y - x.y * y.x;}
    if (m >= 3) {
        // The ring is counterclockwise and starts on the line.
        require(area > 0, "cut ring is counterclockwise");
        bool on = (lll(b.x) - a.x) * (got[0].y - lll(a.y) * got[0].d) == (lll(b.y) - a.y) * (got[0].x - lll(a.x) * got[0].d);
        require(on || int(got.size()) == n0(), "cut ring starts at a crossing");}
    long double scale = 1;
    for (auto v : poly) { scale = max({scale, std::abs((long double)v.x), std::abs((long double)v.y)}); }
    require(std::abs(cq.areaLeftOfLineApprox(a, b) - area / 2) <= 1e-12L * scale * scale, "areaLeftOfLineApprox equals the clipped area");}
// Lines at full scale: the intersection with a line is clipConvex with both of its halfplanes.
void bigLineCase(const ConvexPolygonQuery &cq, point a, point b) {
    context = "big line=(" + std::to_string(a.x) + "," + std::to_string(a.y) + ")->(" + std::to_string(b.x) + "," + std::to_string(b.y) + ")";
    auto r = cq.lineIntersection(Linear2::line(a, b)); auto w = clipConvex(poly, {Halfplane{a, b}, Halfplane{b, a}});
    vector<RationalPoint2> got(r.points.begin(), r.points.begin() + r.count);
    require(sameSet(got, w.vertices), "lineIntersection equals clipConvex with both halfplanes");}
void prefixCase(const ConvexPolygonQuery &cq) {
    int n = n0();
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            vector<point> c;
            for (int k = i; ; k = (k + 1) % n) { c.push_back(poly[k]); if (k == j) { break; } }
            require(cq.prefixArea2(i, j) == signedArea2(c), "prefixArea2 equals the chain polygon area");}}}
void batchCase(const ConvexPolygonQuery &cq, vector<point> qs) {
    point o = poly[0];
    sort(qs.begin(), qs.end(), [&](point x, point y) { return polarLess(x - o, y - o); });
    auto r = cq.batchedLocate(qs);
    for (int i = 0; i < int(qs.size()); ++i) { require(r[i] == polygonContains(poly, qs[i]), "batchedLocate equals polygonContains"); }}
void intersectionCase(const vector<point> &p, const vector<point> &q) {
    vector<Halfplane> h;
    for (int i = 0; i < int(q.size()); ++i) { h.push_back({q[i], q[(i + 1) % q.size()]}); }
    auto a = convexIntersection(p, q), b = clipConvex(p, h);
    require(a.kind == b.kind && sameSet(a.vertices, b.vertices), "convexIntersection equals clipConvex");}
// Exhaustive k-subsets: the largest area and the largest perimeter.
void kgonCase(int k) {
    int n = n0(); context = "k=" + std::to_string(k);
    auto a = maxAreaInscribedKgon(poly, k); auto b = maxPerimeterInscribedKgonApprox(poly, k);
    auto a2 = maxAreaInscribedKgonSmawk(poly, k); auto b2 = maxPerimeterInscribedKgonSmawkApprox(poly, k);
    require(a2.value == a.value && a2.vertices == a.vertices, "SMAWK and divide and conquer find the same area k-gon");
    require(int(b2.vertices.size()) == int(b.vertices.size()) && std::abs(b2.value - b.value) <= 1e-15L * max(1.L, b.value), "SMAWK perimeter equals divide and conquer");
    if (k >= n) {
        require(a.value == signedArea2(poly) && int(a.vertices.size()) == n, "k >= n takes the polygon");
        return;}
    lll best = -1; long double per = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (__builtin_popcount(uint(mask)) != k) { continue; }
        vector<point> c;
        for (int i = 0; i < n; ++i) { if (mask >> i & 1) { c.push_back(poly[i]); } }
        best = max(best, signedArea2(c)); per = max(per, polygonPerimeter(c));}
    vector<point> c;
    for (int t = 0; t < k; ++t) {
        require(0 <= a.vertices[t] && a.vertices[t] < n && (t == 0 || a.vertices[t - 1] < a.vertices[t]), "area vertices increase");
        c.push_back(poly[a.vertices[t]]);}
    require(a.value == best && signedArea2(c) == best, "maxAreaInscribedKgon equals the exhaustive optimum");
    c.clear();
    for (int t = 0; t < k; ++t) { c.push_back(poly[b.vertices[t]]); require(t == 0 || b.vertices[t - 1] < b.vertices[t], "perimeter vertices increase"); }
    require(std::abs(b.value - per) <= 1e-15L * per && std::abs(polygonPerimeter(c) - b.value) <= 1e-15L * per, "maxPerimeterInscribedKgonApprox equals the exhaustive optimum");}
// O(k * n^2) per root, every root: independent of monotonicity and interleaving.
void kgonDp(int k) {
    int n = n0(); context = "dp k=" + std::to_string(k); lll best = -1;
    for (int s = 0; s < n; ++s) {
        vector<lll> f(n, -1); f[0] = 0;
        for (int t = 1; t < k; ++t) {
            vector<lll> g(n, -1);
            for (int j = t; j < n; ++j) {
                for (int i = t - 1; i < j; ++i) {
                    if (f[i] >= 0 || (i == 0 && t == 1)) { g[j] = max(g[j], f[i] + cross(poly[(s + i) % n] - poly[s], poly[(s + j) % n] - poly[s])); }}}
            f = g;}
        for (int j = k - 1; j < n; ++j) { best = max(best, f[j]); }}
    require(maxAreaInscribedKgon(poly, k).value == best, "maxAreaInscribedKgon equals the all-roots dynamic program");
    require(maxAreaInscribedKgonSmawk(poly, k).value == best, "maxAreaInscribedKgonSmawk equals the all-roots dynamic program");}
void invalid(const string &name) {
    if (name == "clockwise") { ConvexPolygonQuery c(vector<point>{{0, 0}, {0, 1}, {1, 0}}); }
    if (name == "collinear") { ConvexPolygonQuery c(vector<point>{{0, 0}, {1, 0}, {2, 0}, {1, 1}}); }
    if (name == "small") { ConvexPolygonQuery c(vector<point>{{0, 0}, {1, 0}}); }
    if (name == "tangent-inside") { ConvexPolygonQuery c(vector<point>{{0, 0}, {4, 0}, {0, 4}}); (void)c.tangents({1, 1}); }
    if (name == "extreme-zero") { ConvexPolygonQuery c(vector<point>{{0, 0}, {4, 0}, {0, 4}}); (void)c.extreme({0, 0}); }
    if (name == "cut-degenerate") { ConvexPolygonQuery c(vector<point>{{0, 0}, {4, 0}, {0, 4}}); (void)c.cut({1, 1}, {1, 1}); }
    if (name == "batch-unsorted") { ConvexPolygonQuery c(vector<point>{{0, 0}, {4, 0}, {0, 4}}); (void)c.batchedLocate({{0, 5}, {5, 0}}); }
    if (name == "kgon-k") { (void)maxAreaInscribedKgon(vector<point>{{0, 0}, {4, 0}, {0, 4}}, 2); }
    if (name == "kgon-smawk-k") { (void)maxPerimeterInscribedKgonSmawkApprox(vector<point>{{0, 0}, {4, 0}, {0, 4}}, 2); }
    if (name == "pentagram") { ConvexPolygonQuery c(vector<point>{{0, 10}, {-6, -8}, {10, 3}, {-10, 3}, {6, -8}}); }
    if (name == "extreme-bound") { ConvexPolygonQuery c(vector<point>{{0, 0}, {4, 0}, {0, 4}}); (void)c.extreme({std::numeric_limits<lng>::min(), 1}); }
    if (name == "intersection-clockwise") { (void)convexIntersection(vector<point>{{0, 0}, {0, 1}, {1, 0}}, vector<point>{{0, 0}, {1, 0}, {0, 1}}); }
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    int rounds = quick ? 200 : stress ? 20000 : 3000;
    phase = "queries";
    for (int t = 0; t < rounds; ++t) {
        lng c = t % 5 == 4 ? 1000000000 : 8;
        poly = randomConvex(rng, c, 3 + int(rng() % 14)); ConvexPolygonQuery cq(poly);
        lng qc = c == 8 ? 11 : c;
        for (int i = 0; i < 12; ++i) { queryCase(cq, randomPoints(rng, qc, 1)[0]); }
        for (auto v : poly) { queryCase(cq, v); if (c == 8) { queryCase(cq, v + v - poly[0]); } }
        for (int i = 0; i < 6; ++i) {
            point d = randomPoints(rng, t % 2 ? 3 : 1000000000, 1)[0];
            if (d != point{}) { extremeCase(cq, d); }}
        for (int i = 0; i < int(poly.size()); ++i) { extremeCase(cq, perp(poly[(i + 1) % poly.size()] - poly[i])); extremeCase(cq, -perp(poly[(i + 1) % poly.size()] - poly[i])); }
        for (int i = 0; i < 3; ++i) {
            point d = randomPoints(rng, 1000000000000000000LL, 1)[0];
            if (d != point{}) { extremeCase(cq, d); }}
        if (c != 8) {
            for (int i = 0; i < 6; ++i) {
                point a = randomPoints(rng, c, 1)[0], b = randomPoints(rng, c, 1)[0];
                if (a != b) { bigLineCase(cq, a, b); cutCase(cq, a, b); }}
            for (int i = 0; i < int(poly.size()); ++i) {
                point a = poly[i], b = poly[(i + 1) % poly.size()];
                bigLineCase(cq, a, b); cutCase(cq, b, a);}
            prefixCase(cq);}
        if (c == 8) {
            for (int i = 0; i < 10; ++i) {
                point a = randomPoints(rng, 12, 1)[0], b = randomPoints(rng, 12, 1)[0];
                lineCase(cq, Linear2{a, b, LinearKind(rng() % 3)});
                if (a != b) { cutCase(cq, a, b); }}
            for (int i = 0; i < int(poly.size()); ++i) {
                point a = poly[i], b = poly[(i + 1) % poly.size()];
                lineCase(cq, Linear2::line(a, b)); lineCase(cq, Linear2::segment(b, a)); cutCase(cq, a, b); cutCase(cq, b, a);
                cutCase(cq, a, a + perp(b - a)); lineCase(cq, Linear2::ray(a + a - b, a));}
            prefixCase(cq);
            batchCase(cq, randomPoints(rng, 12, int(rng() % 40)));
            vector<point> on(poly); on.push_back(poly[0]); on.push_back(poly[0]); batchCase(cq, on);}
        else { batchCase(cq, randomPoints(rng, c, int(rng() % 40))); }}
    phase = "intersection";
    for (int t = 0; t < rounds; ++t) {
        lng c = t % 4 == 3 ? 1000000000 : 6;
        auto p = convexHull(randomPoints(rng, c, 3 + int(rng() % 10)), rng() % 2), q = convexHull(randomPoints(rng, c, 3 + int(rng() % 10)), rng() % 2);
        if (p.size() < 3 || q.size() < 3 || signedArea2(p) == 0 || signedArea2(q) == 0) { continue; }
        poly = p; intersectionCase(p, q); intersectionCase(q, p); intersectionCase(p, p);}
    phase = "kgon exhaustive";
    for (int t = 0; t < rounds / 3; ++t) {
        poly = randomConvex(rng, t % 3 ? 20 : 1000000000, 3 + int(rng() % 12));
        for (int k = 3; k <= min(n0() + 1, 8); ++k) { kgonCase(k); }}
    kgonCase(std::numeric_limits<int>::max()); // Nothing sized by k may be allocated once k >= n.
    phase = "kgon regular";
    for (int m : {8, 12, 16}) {
        poly.clear();
        for (int i = 0; i < m; ++i) {
            long double a = 2 * std::numbers::pi_v<long double> * i / m;
            poly.push_back({std::llround(1000 * std::cos(a)), std::llround(1000 * std::sin(a))});}
        poly = convexHull(poly);
        for (int k = 3; k <= 6; ++k) { kgonCase(k); }}
    poly = {{0, 0}, {2, 0}, {3, 1}, {3, 3}, {2, 4}, {0, 4}, {-1, 3}, {-1, 1}};
    for (int k = 3; k <= 8; ++k) { kgonCase(k); }
    phase = "kgon Keikha counterexamples";
    // arXiv 1705.11035 section 3 (9 vertices, largest triangle a0 b0 c0) and section 7.2 (16 vertices, largest quadrilateral a4 a8 a12 a16).
    poly = convexHull(vector<point>{{4752, 4262}, {3383, 413}, {759, 2927}, {4745, 4322}, {1213, 691}, {2506, 4423}, {3040, 4460}, {1000, 1000}, {5000, 1000}});
    require(n0() == 9, "Keikha 9-gon is in convex position");
    require(maxAreaInscribedKgon(poly, 3).value == signedArea2(vector<point>{{1000, 1000}, {5000, 1000}, {3040, 4460}}), "Keikha 9-gon: the largest triangle is a0 b0 c0");
    for (int k = 3; k <= 8; ++k) { kgonCase(k); }
    poly = convexHull(vector<point>{{26096, 6750}, {26130, 9933}, {25940, 10728}, {23090, 22189}, {18106, 23681}, {13484, 24407}, {13174, 24343}, {3090, 22189},
                                    {0, 17308}, {80, 14350}, {323, 13331}, {3090, 2189}, {8459, 385}, {12837, 0}, {13392, 114}, {23090, 2189}});
    require(n0() == 16, "Keikha 16-gon is in convex position");
    require(maxAreaInscribedKgon(poly, 4).value == signedArea2(vector<point>{{3090, 2189}, {23090, 2189}, {23090, 22189}, {3090, 22189}}), "Keikha 16-gon: the largest quadrilateral is a4 a8 a12 a16");
    for (int k = 3; k <= 8; ++k) { kgonCase(k); }
    phase = "kgon dp";
    for (int t = 0; t < (quick ? 3 : stress ? 200 : 30); ++t) {
        poly = randomConvex(rng, t % 2 ? 1000000 : 300, 30 + int(rng() % 60));
        for (int k : {3, 4, 7, n0() / 2}) { if (k >= 3 && k < n0()) { kgonDp(k); } }}
    phase = "large";
    int n = quick ? 4000 : stress ? 400000 : 200000;
    poly.clear();
    for (int i = 0; i < n; ++i) {
        long double a = 2 * std::numbers::pi_v<long double> * i / n;
        poly.push_back({std::llround(9e8L * std::cos(a)), std::llround(9e8L * std::sin(a))});}
    poly = convexHull(poly); ConvexPolygonQuery big(poly);
    for (int i = 0; i < 60; ++i) { queryCase(big, randomPoints(rng, 1000000000, 1)[0]); }
    vector<point> qs = randomPoints(rng, 1000000000, n);
    point o = poly[0];
    sort(qs.begin(), qs.end(), [&](point x, point y) { return polarLess(x - o, y - o); });
    auto r = big.batchedLocate(qs);
    for (int i = 0; i < n; ++i) { require(r[i] == big.contains(qs[i]), "large batchedLocate equals contains"); }
    vector<point> sub(poly.begin(), poly.begin() + min(int(poly.size()), quick ? 2000 : 20000));
    for (int k : {3, 5, 40}) {
        auto x = maxAreaInscribedKgon(sub, k), y = maxAreaInscribedKgonSmawk(sub, k);
        require(int(x.vertices.size()) == k && x.value == y.value && x.vertices == y.vertices, "large k-gon: both versions agree");}
    std::cout << "PASS convex_polygon_query seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
