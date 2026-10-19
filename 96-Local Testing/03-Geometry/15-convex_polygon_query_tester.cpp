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
// a * b as a 256-bit (high, low) pair; nonnegative fractions compare through these products at any 128-bit size.
pair<ulll, ulll> mul256(ulll a, ulll b) {
    ulll a0 = ulng(a), a1 = a >> 64, b0 = ulng(b), b1 = b >> 64, lo = a0 * b0, m1 = a0 * b1, m2 = a1 * b0;
    ulll mid = (lo >> 64) + ulng(m1) + ulng(m2);
    return {a1 * b1 + (m1 >> 64) + (m2 >> 64) + (mid >> 64), mid << 64 | ulng(lo)};}
// Exact squared distance from q to the closed segment u v, as num / den.
pair<lll, lll> segmentDistance2(point q, point u, point v) {
    if (dot(v - u, q - u) <= 0) { return {norm2(q - u), 1}; }
    if (dot(u - v, q - v) <= 0) { return {norm2(q - v), 1}; }
    lll h = cross(v - u, q - u);
    return {h * h, norm2(v - u)};}

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
    require(std::abs(cq.pointDistanceApprox(q) - want) <= 1e-17L * scale, "pointDistanceApprox equals the edge minimum");
    pair<lll, lll> exact{0, 1};
    if (loc == PolygonLocation::Outside) {
        exact = segmentDistance2(q, poly[0], poly[1 % n]);
        for (int i = 1; i < n; ++i) {
            auto d = segmentDistance2(q, poly[i], poly[(i + 1) % n]);
            if (mul256(ulll(d.first), ulll(exact.second)) < mul256(ulll(exact.first), ulll(d.second))) { exact = d; }}
        lll g = std::gcd(exact.first, exact.second); exact = {exact.first / g, exact.second / g};}
    require(cq.pointDistance2(q) == exact, "pointDistance2 equals the exact edge minimum, reduced");}
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
    require(std::abs(cq.areaLeftOfLineApprox(a, b) - area / 2) <= 1e-12L * scale * scale, "areaLeftOfLineApprox equals the clipped area");
    ExactArea2 e = cq.areaLeftOfLine2(a, b), f = cq.areaLeftOfLine2(b, a); lll total = signedArea2(poly);
    require(e.den > 0 && 0 <= e.num && e.num < e.den && std::gcd(e.num, e.den) == 1, "areaLeftOfLine2 is a reduced proper fraction");
    require(e.num == 0 ? f.num == 0 && e.whole + f.whole == total : e.den == f.den && e.num + f.num == e.den && e.whole + f.whole + 1 == total,
            "areaLeftOfLine2 of both sides sums to the polygon exactly");
    require(std::abs((long double)e.whole + (long double)e.num / (long double)e.den - area) <= 1e-12L * scale * scale, "areaLeftOfLine2 equals the clipped area");
    if (scale <= 100 && m >= 3) {
        // Exact shoelace over the cut ring: every term over the common denominator L.
        lll L = 1;
        for (int i = 0; i < m; ++i) { lll d = got[i].d * got[(i + 1) % m].d; L = L / std::gcd(L, d) * d; }
        lll N = 0;
        for (int i = 0; i < m; ++i) {
            const auto &x = got[i], &y = got[(i + 1) % m];
            N += (x.x * y.y - x.y * y.x) * (L / (x.d * y.d));}
        require(N * e.den == (e.whole * e.den + e.num) * L, "areaLeftOfLine2 equals the exact shoelace of the cut");}
    else if (m < 3) { require(e.whole == 0 && e.num == 0, "areaLeftOfLine2 is 0 without a polygon"); }}
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
    if (k < n) {
        // Hirschberg recomputation forced (no stored table above 2 (span + layers)) for both layer methods.
        ConvexPolygonQuery cq(poly); auto w = [&](int i, int j) { return cross(cq.at(i), cq.at(j)); };
        for (auto h : {convex_query_detail::inscribed<lll, false>(n, k, cq.pre[n], w, 0), convex_query_detail::inscribed<lll, true>(n, k, cq.pre[n], w, 0)}) {
            vector<point> c;
            for (int i : h.second) { c.push_back(poly[i]); }
            require(h.first == a.value && int(h.second.size()) == k && signedArea2(c) == a.value, "Hirschberg recomputation finds the same optimum");}}
    require(int(b2.vertices.size()) == int(b.vertices.size()) && std::abs(b2.value - b.value) <= 1e-15L * max(1.L, b.value), "SMAWK perimeter equals divide and conquer");
    auto special = [&](lll want) {
        if (k != 3 && k != 4) { return; }
        auto z = k == 3 ? maxAreaInscribedTriangle(poly) : maxAreaInscribedQuadrilateral(poly); vector<point> c;
        for (int t = 0; t < int(z.vertices.size()); ++t) {
            require(0 <= z.vertices[t] && z.vertices[t] < n && (t == 0 || z.vertices[t - 1] < z.vertices[t]), "linear k-gon vertices increase");
            c.push_back(poly[z.vertices[t]]);}
        require(int(z.vertices.size()) == min(k, n) && z.value == want && signedArea2(c) == want, "linear triangle or quadrilateral equals the optimum");};
    if (k >= n) {
        require(a.value == signedArea2(poly) && int(a.vertices.size()) == n, "k >= n takes the polygon");
        special(signedArea2(poly));
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
    special(best);
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
    require(maxAreaInscribedKgonSmawk(poly, k).value == best, "maxAreaInscribedKgonSmawk equals the all-roots dynamic program");
    ConvexPolygonQuery cq(poly); auto w = [&](int i, int j) { return cross(cq.at(i), cq.at(j)); };
    require(convex_query_detail::inscribed<lll, false>(n, k, cq.pre[n], w, 0).first == best && convex_query_detail::inscribed<lll, true>(n, k, cq.pre[n], w, 0).first == best,
            "forced Hirschberg recomputation equals the all-roots dynamic program");
    if (k == 3) { require(maxAreaInscribedTriangle(poly).value == best, "maxAreaInscribedTriangle equals the all-roots dynamic program"); }
    if (k == 4) { require(maxAreaInscribedQuadrilateral(poly).value == best, "maxAreaInscribedQuadrilateral equals the all-roots dynamic program"); }}
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
    require(maxAreaInscribedTriangle(poly).value == signedArea2(vector<point>{{1000, 1000}, {5000, 1000}, {3040, 4460}}), "Keikha 9-gon: the linear triangle");
    for (int k = 3; k <= 8; ++k) { kgonCase(k); }
    poly = convexHull(vector<point>{{26096, 6750}, {26130, 9933}, {25940, 10728}, {23090, 22189}, {18106, 23681}, {13484, 24407}, {13174, 24343}, {3090, 22189},
                                    {0, 17308}, {80, 14350}, {323, 13331}, {3090, 2189}, {8459, 385}, {12837, 0}, {13392, 114}, {23090, 2189}});
    require(n0() == 16, "Keikha 16-gon is in convex position");
    require(maxAreaInscribedKgon(poly, 4).value == signedArea2(vector<point>{{3090, 2189}, {23090, 2189}, {23090, 22189}, {3090, 22189}}), "Keikha 16-gon: the largest quadrilateral is a4 a8 a12 a16");
    require(maxAreaInscribedQuadrilateral(poly).value == signedArea2(vector<point>{{3090, 2189}, {23090, 2189}, {23090, 22189}, {3090, 22189}}), "Keikha 16-gon: the linear quadrilateral");
    for (int k = 3; k <= 8; ++k) { kgonCase(k); }
    phase = "kgon Boyce examples";
    // Boyce et al. (STOC 1982) Fig. 10, scaled by 10^4: the largest perimeter triangles rooted at A and A' do not interleave each other,
    // yet both interleave the overall maximum C'BC. The algorithm never assumes that rooted optima at different roots interleave.
    poly = convexHull(vector<point>{{-4134, 9060}, {-5866, 8260}, {-5866, -8060}, {-4134, -9260}, {10000, 1000}, {10000, -1000}});
    require(n0() == 6, "Boyce Fig. 10 is in convex position");
    {
        auto best = maxPerimeterInscribedKgonApprox(poly, 3); vector<point> c;
        for (int i : best.vertices) { c.push_back(poly[i]); }
        sort(c.begin(), c.end());
        require(c == vector<point>{{-5866, 8260}, {-4134, -9260}, {10000, 1000}}, "Boyce Fig. 10: the largest perimeter triangle is C'BC");}
    for (int k = 3; k <= 6; ++k) { kgonCase(k); }
    // Fig. 12, scaled by 100: seven points on which the Dobkin–Snyder walk misses the largest pentagon A D B D' C.
    poly = convexHull(vector<point>{{-10100, 0}, {0, 0}, {0, -10100}, {-5100, 101}, {-5000, 100}, {101, -5000}, {100, -5100}});
    require(n0() == 7, "Boyce Fig. 12 is in convex position");
    require(maxAreaInscribedKgon(poly, 5).value == signedArea2(vector<point>{{0, -10100}, {101, -5000}, {0, 0}, {-5100, 101}, {-10100, 0}}), "Boyce Fig. 12: the largest pentagon is A D B D' C");
    for (int k = 3; k <= 7; ++k) { kgonCase(k); }
    phase = "kgon ties";
    // Centrally symmetric hulls, their shears and lattice disks: many k-gons tie exactly, so tie-breaking rules matter.
    for (int t = 0; t < (quick ? 20 : stress ? 3000 : 300); ++t) {
        vector<point> s = randomPoints(rng, t % 2 ? 3 : 6, 2 + int(rng() % 6));
        for (int i = int(s.size()) - 1; i >= 0; --i) { s.push_back(-s[i]); }
        lng sh = lng(rng() % 3) - 1;
        for (auto &x : s) { x = {x.x + sh * x.y, x.y}; }
        poly = convexHull(s);
        if (n0() < 3 || n0() > 16) { continue; }
        std::rotate(poly.begin(), poly.begin() + rng() % poly.size(), poly.end());
        for (int k = 3; k <= min(n0() + 1, 8); ++k) { kgonCase(k); }}
    for (lng r = 1; r <= 5; ++r) {
        vector<point> s;
        for (lng x = -r; x <= r; ++x) { for (lng y = -r; y <= r; ++y) { if (x * x + y * y <= r * r) { s.push_back({x, y}); } } }
        poly = convexHull(s);
        for (int k = 3; k <= min(n0() + 1, 8); ++k) { kgonCase(k); }}
    poly = {{1, 0}, {1, 1}, {0, 1}, {-1, 0}, {-1, -1}, {0, -1}};
    for (int k = 3; k <= 6; ++k) { kgonCase(k); }
    phase = "linear triangle and quadrilateral";
    // Near-circular, flattened and uniform hulls up to 40 vertices against cubic and quartic scans. A 19-gon whose rooted optimum needs
    // the corners revisited before it is 3-stable is fixed first; area-only kill rules fail on about 1 in 1700 of these hulls.
    poly = {{-887248574, 150963464}, {-861203611, -261396902}, {-640957985, -631801283}, {-512770426, -739639433}, {-294092340, -850593731},
            {-143153078, -888542175}, {-140651386, -888941611}, {-110981948, -893131013}, {876860636, -202769390}, {876216708, 205534135},
            {728125266, 528993002}, {674920430, 595384257}, {540929741, 719301755}, {507057689, 743567415}, {435247302, 787756171},
            {291996431, 851315502}, {-728673296, 528237852}, {-743041335, 507828292}, {-880235530, 187577750}};
    kgonDp(3); kgonDp(4);
    for (int t = 0; t < (quick ? 400 : stress ? 40000 : 6000); ++t) {
        lng c = t % 3 == 0 ? 6 : t % 3 == 1 ? 1000 : 900000000; int kind = int(rng() % 3); vector<point> s;
        for (int i = 3 + int(rng() % 38); i > 0; --i) {
            if (kind == 0) { s.push_back(randomPoints(rng, c, 1)[0]); continue; }
            long double ang = (long double)(rng() % 1000000) / 1000000 * 2 * std::numbers::pi_v<long double>, rad = kind == 1 ? 1 : 1 + (long double)(rng() % 1000) / 1e5L;
            s.push_back({std::llround(c * rad * std::cos(ang)), std::llround(c * (kind == 2 ? 0.3L : 1) * rad * std::sin(ang))});}
        poly = convexHull(s);
        int n = n0();
        if (n < 3) { continue; }
        lll b3 = 0, b4 = 0;
        for (int i = 0; i < n; ++i) {
            for (int j = i + 1; j < n; ++j) {
                for (int k = j + 1; k < n; ++k) {
                    b3 = max(b3, cross(poly[j] - poly[i], poly[k] - poly[i]));
                    for (int l = k + 1; l < n && n <= 24; ++l) { b4 = max(b4, cross(poly[k] - poly[i], poly[l] - poly[j])); }}}}
        context = "n=" + std::to_string(n);
        require(maxAreaInscribedTriangle(poly).value == b3, "maxAreaInscribedTriangle equals the cubic scan");
        if (n <= 24) { require(maxAreaInscribedQuadrilateral(poly).value == (n == 3 ? b3 : b4), "maxAreaInscribedQuadrilateral equals the quartic scan"); }}
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
    // Sparse batches gallop over long runs of the fan.
    for (int q : {1, 2, 7, 60}) {
        vector<point> few = randomPoints(rng, 1000000000, q);
        sort(few.begin(), few.end(), [&](point x, point y) { return polarLess(x - o, y - o); });
        auto rf = big.batchedLocate(few);
        for (int i = 0; i < q; ++i) { require(rf[i] == big.contains(few[i]), "sparse batchedLocate equals contains"); }}
    vector<point> sub(poly.begin(), poly.begin() + min(int(poly.size()), quick ? 2000 : 20000));
    for (int k : {3, 4, 5, 40}) {
        auto x = maxAreaInscribedKgon(sub, k), y = maxAreaInscribedKgonSmawk(sub, k);
        require(int(x.vertices.size()) == k && x.value == y.value && x.vertices == y.vertices, "large k-gon: both versions agree");
        if (k <= 4) { require((k == 3 ? maxAreaInscribedTriangle(sub) : maxAreaInscribedQuadrilateral(sub)).value == x.value, "large linear k-gon equals the general one"); }}
    require(maxAreaInscribedTriangle(poly).value == maxAreaInscribedKgonSmawk(poly, 3).value && maxAreaInscribedQuadrilateral(poly).value == maxAreaInscribedKgonSmawk(poly, 4).value,
            "linear triangle and quadrilateral on the full large hull");
    std::cout << "PASS convex_polygon_query seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
