#include "../../03-Geometry/10-halfplaneintersection.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

// Oracle: Cramer vertices of boundary pairs with exact membership, a rational hull, witness-direction recession cones and
// parallel offset tests; Helly triples and boundary certificates replace the hull at |coordinate| = 10^9.
static ulng seed = 20260927;
static std::string phase;
static vector<Halfplane> input;
static lng checks = 0;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " input=";
    for (auto h : input) { std::cerr << '[' << h.a.x << ',' << h.a.y << "->" << h.b.x << ',' << h.b.y << ']'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
using Q = RationalPoint2;
point dirOf(const Halfplane &h) { return h.b - h.a; }
int sgn(lll x) { return (x > 0) - (x < 0); }
// Membership written from the line equation A x + B y <= C rather than the header's cross form.
int depth(const Halfplane &h, const Q &p) {
    lll A = h.b.y - h.a.y, B = h.a.x - h.b.x, C = A * h.a.x + B * h.a.y;
    return sgn(C * p.d - A * p.x - B * p.y);}
bool cramer(const Halfplane &s, const Halfplane &t, Q &out) {
    lll A1 = s.b.y - s.a.y, B1 = s.a.x - s.b.x, C1 = A1 * s.a.x + B1 * s.a.y;
    lll A2 = t.b.y - t.a.y, B2 = t.a.x - t.b.x, C2 = A2 * t.a.x + B2 * t.a.y, det = A1 * B2 - A2 * B1;
    if (!det) { return false; }
    out = Q(C1 * B2 - C2 * B1, A1 * C2 - A2 * C1, det); return true;}
bool lessQ(const Q &p, const Q &q) {
    lll a = p.x * q.d, b = q.x * p.d;
    return a != b ? a < b : p.y * q.d < q.y * p.d;}
int orientQ(const Q &p, const Q &q, const Q &r) {
    lll x1 = q.x * p.d - p.x * q.d, y1 = q.y * p.d - p.y * q.d, x2 = r.x * p.d - p.x * r.d, y2 = r.y * p.d - p.y * r.d;
    return sgn(x1 * y2 - y1 * x2);}
vector<Q> hullQ(vector<Q> p) {
    sort(p.begin(), p.end(), lessQ); p.erase(unique(p.begin(), p.end()), p.end());
    if (p.size() <= 1) { return p; }
    vector<Q> h;
    for (int pass = 0; pass < 2; ++pass) {
        int base = int(h.size());
        for (int i = 0; i < int(p.size()); ++i) {
            const Q &x = p[pass ? p.size() - 1 - i : i];
            while (int(h.size()) - base >= 2 && orientQ(h[h.size() - 2], h.back(), x) <= 0) { h.pop_back(); }
            h.push_back(x);}
        h.pop_back();}
    return h;}
bool inCone(const vector<point> &g, point r) {
    if (r == point{}) { return true; }
    for (auto a : g) { if (cross(a, r) == 0 && dot(a, r) > 0) { return true; } }
    for (auto a : g) { for (auto b : g) {
        lll d = cross(a, b);
        if (d == 0) { if (dot(a, b) < 0 && cross(a, r) == 0) { return true; } continue; }
        lll l = cross(r, b), m = cross(a, r);
        if (d < 0) { l = -l; m = -m; }
        if (l >= 0 && m >= 0) { return true; }}}
    return false;}
bool inRecession(const vector<Halfplane> &h, point r) {
    for (auto &x : h) { if (cross(dirOf(x), r) < 0) { return false; } }
    return true;}
// Candidate directions plus one strictly interior direction per angular gap decide equality of 2D cones.
vector<point> witnesses(vector<point> c) {
    c.erase(std::remove(c.begin(), c.end(), point{}), c.end());
    sort(c.begin(), c.end(), [](point a, point b) { return polarLess(a, b); });
    vector<point> w = c;
    for (int i = 0; i < int(c.size()); ++i) {
        point u = c[i], v = c[(i + 1) % c.size()];
        w.push_back(cross(u, v) > 0 ? u + v : perp(u));}
    if (c.empty()) { w = {{1, 0}}; }
    return w;}
struct Oracle {
    HalfplaneKind kind = HalfplaneKind::Empty;
    vector<Q> v;
    vector<point> cone;
};
vector<point> coneCandidates(const vector<Halfplane> &h, const vector<point> &extra) {
    vector<point> c = extra;
    for (auto &x : h) { point d = dirOf(x); c.insert(c.end(), {d, -d, perp(d), -perp(d)}); }
    c.insert(c.end(), {{1, 0}, {0, 1}, {-1, 0}, {0, -1}});
    return witnesses(c);}
Oracle oracle(const vector<Halfplane> &h) {
    Oracle o;
    for (auto r : coneCandidates(h, {})) { if (inRecession(h, r)) { o.cone.push_back(r); } }
    bool parallel = true;
    for (auto &x : h) { parallel = parallel && cross(dirOf(x), dirOf(h[0])) == 0; }
    if (h.empty()) { o.kind = HalfplaneKind::Unbounded; return o; }
    if (parallel) {
        point p = dirOf(h[0]); p /= gcd(abs(p.x), abs(p.y));
        bool lo = false, hi = false; lll L = 0, U = 0;
        for (auto &x : h) {
            lll s = cross(p, x.a);
            if (dot(dirOf(x), p) > 0) { L = lo ? max(L, s) : s; lo = true; }
            else { U = hi ? min(U, s) : s; hi = true; }}
        o.kind = lo && hi ? (L < U ? HalfplaneKind::Unbounded : L == U ? HalfplaneKind::Line : HalfplaneKind::Empty) : HalfplaneKind::Unbounded;
        if (o.kind == HalfplaneKind::Empty) { o.cone.clear(); }
        return o;}
    for (int i = 0; i < int(h.size()); ++i) { for (int j = i + 1; j < int(h.size()); ++j) {
        Q q;
        if (!cramer(h[i], h[j], q)) { continue; }
        bool ok = true;
        for (auto &x : h) { ok = ok && depth(x, q) >= 0; }
        if (ok) { o.v.push_back(q); }}}
    sort(o.v.begin(), o.v.end(), lessQ); o.v.erase(unique(o.v.begin(), o.v.end()), o.v.end());
    if (o.v.empty()) { o.cone.clear(); return o; }
    if (o.cone.empty()) {
        o.v = hullQ(o.v);
        o.kind = o.v.size() == 1 ? HalfplaneKind::Point : o.v.size() == 2 ? HalfplaneKind::Segment : HalfplaneKind::Polygon;
        return o;}
    bool ray = o.v.size() == 1;
    for (auto r : o.cone) { ray = ray && cross(r, o.cone[0]) == 0 && dot(r, o.cone[0]) > 0; }
    o.kind = ray ? HalfplaneKind::Ray : HalfplaneKind::Unbounded;
    return o;}
bool sameCone(const vector<Halfplane> &h, const vector<point> &g) {
    for (auto r : g) { if (r == point{} || !inRecession(h, r)) { return false; } }
    for (auto r : coneCandidates(h, g)) { if (inRecession(h, r) != inCone(g, r)) { return false; } }
    return true;}
bool samePoints(vector<Q> a, vector<Q> b) {
    sort(a.begin(), a.end(), lessQ); sort(b.begin(), b.end(), lessQ);
    return a == b;}
bool onBoundary(const Halfplane &h, const Q &p) { return depth(h, p) == 0; }
bool fromInput(const vector<Halfplane> &h, const Halfplane &x) {
    for (auto &y : h) { if (y.a == x.a && y.b == x.b) { return true; } }
    return false;}
void verify(const vector<Halfplane> &h, const HalfplaneResult &r, bool recurse = true) {
    Oracle o = oracle(h);
    require(r.kind == o.kind, "kind equals brute-force classification");
    for (auto &p : r.planes) { require(fromInput(h, p), "planes are input halfplanes"); }
    for (auto &q : r.vertices) {
        for (auto &x : h) { require(depth(x, q) >= 0 && x.contains(q), "vertex satisfies every halfplane"); }}
    switch (r.kind) {
        case HalfplaneKind::Empty:
            require(r.vertices.empty() && r.planes.empty() && r.recession.empty(), "empty result has no data"); break;
        case HalfplaneKind::Point:
        case HalfplaneKind::Segment:
            require(samePoints(r.vertices, o.v) && r.recession.empty(), "degenerate vertices"); break;
        case HalfplaneKind::Polygon: {
            int k = int(r.vertices.size());
            require(k == int(o.v.size()) && int(r.planes.size()) == k && r.recession.empty(), "polygon sizes");
            int s = int(std::find(o.v.begin(), o.v.end(), r.vertices[0]) - o.v.begin());
            for (int i = 0; i < k && s < k; ++i) { require(r.vertices[i] == o.v[(s + i) % k], "polygon is the CCW hull cycle"); }
            require(s < k, "polygon start is a hull vertex");
            for (int i = 0; i < k; ++i) {
                require(onBoundary(r.planes[i], r.vertices[i]) && onBoundary(r.planes[i], r.vertices[(i + 1) % k]), "edge i lies on planes[i]");
                if (i) { require(polarLess(dirOf(r.planes[0]), dirOf(r.planes[i])), "start edge has the smallest angle"); }}
            break;}
        case HalfplaneKind::Ray:
            require(samePoints(r.vertices, o.v) && r.recession.size() == 1 && sameCone(h, r.recession), "ray apex and direction"); break;
        case HalfplaneKind::Line:
            require(r.vertices.empty() && r.recession.size() == 2 && sameCone(h, r.recession), "line direction"); break;
        case HalfplaneKind::Unbounded: {
            require(samePoints(r.vertices, o.v) && sameCone(h, r.recession), "unbounded vertices and recession cone");
            int k = int(r.vertices.size());
            for (int i = 0; i < k; ++i) {
                require(onBoundary(r.planes[i], r.vertices[i]) && onBoundary(r.planes[i + 1], r.vertices[i]), "chain vertex i joins planes i, i + 1");}
            require(int(r.planes.size()) == k + 1 || (k == 0 && r.planes.size() <= 2), "chain plane count");
            break;}}
    if (recurse && r.kind != HalfplaneKind::Empty) {
        Oracle p = oracle(r.planes);
        require(p.kind == o.kind && samePoints(p.v, o.v), "planes reproduce the intersection");
        for (auto c : coneCandidates(h, {})) { require(inRecession(h, c) == inRecession(r.planes, c), "planes reproduce the recession cone"); }}}
vector<Halfplane> pool(int c) {
    vector<point> p;
    for (int x = -c; x <= c; ++x) { for (int y = -c; y <= c; ++y) { p.push_back({x, y}); } }
    vector<Halfplane> h;
    for (auto a : p) { for (auto b : p) { if (a != b) { h.push_back({a, b}); } } }
    return h;}
void run(const vector<Halfplane> &h) { input = h; verify(h, halfplaneIntersection(h)); }
// Degree-4 exact predicates at |coordinate| = 10^9: certificates instead of the rational hull.
bool tripleEmpty(const Halfplane &a, const Halfplane &b, const Halfplane &c) {
    vector<Halfplane> t{a, b, c};
    bool parallel = true;
    for (auto &x : t) { parallel = parallel && cross(dirOf(x), dirOf(a)) == 0; }
    if (parallel) { return oracle(t).kind == HalfplaneKind::Empty; }
    for (int i = 0; i < 3; ++i) { for (int j = i + 1; j < 3; ++j) {
        Q q; if (!cramer(t[i], t[j], q)) { continue; }
        bool ok = true;
        for (auto &x : t) { ok = ok && depth(x, q) >= 0; }
        if (ok) { return false; }}}
    return true;}
void certify(const vector<Halfplane> &h, const HalfplaneResult &r) {
    input = h;
    for (auto &q : r.vertices) { for (auto &x : h) { require(depth(x, q) >= 0, "wide vertex feasible"); } }
    if (r.kind == HalfplaneKind::Empty) {
        bool found = false; int n = int(h.size());
        for (int i = 0; i < n && !found; ++i) { for (int j = i; j < n && !found; ++j) { for (int k = j; k < n && !found; ++k) {
            found = tripleEmpty(h[i], h[j], h[k]);}}}
        require(found, "Helly: an empty triple certifies emptiness");}
    if (r.kind == HalfplaneKind::Polygon) {
        int k = int(r.vertices.size()); require(k >= 3 && int(r.planes.size()) == k, "wide polygon size");
        for (int i = 0; i < k; ++i) {
            require(onBoundary(r.planes[i], r.vertices[i]) && onBoundary(r.planes[i], r.vertices[(i + 1) % k]), "wide edge on plane");
            require(!(r.vertices[i] == r.vertices[(i + 1) % k]), "wide edges have positive length");
            require(cross(dirOf(r.planes[i]), dirOf(r.planes[(i + 1) % k])) > 0, "wide planes turn left");
            if (i) { require(polarLess(dirOf(r.planes[i - 1]), dirOf(r.planes[i])), "wide planes sorted once around"); }}}
    if (r.kind == HalfplaneKind::Unbounded || r.kind == HalfplaneKind::Ray || r.kind == HalfplaneKind::Line) {
        require(sameCone(h, r.recession), "wide recession cone");}}
Halfplane scaled(const Halfplane &h, lng s, point t) { return {h.a * s + t, h.b * s + t}; }
Q mapQ(const Q &q, lng s, point t) { return Q(q.x * s + lll(t.x) * q.d, q.y * s + lll(t.y) * q.d, q.d); }
void metamorphic(const vector<Halfplane> &h, lng s, point t) {
    auto r = halfplaneIntersection(h);
    vector<Halfplane> g; for (auto &x : h) { g.push_back(scaled(x, s, t)); }
    input = g; auto w = halfplaneIntersection(g);
    require(w.kind == r.kind && w.vertices.size() == r.vertices.size(), "scaled kind and size");
    vector<Q> m; for (auto &q : r.vertices) { m.push_back(mapQ(q, s, t)); }
    require(samePoints(m, w.vertices), "scaled vertices map exactly");
    require(w.recession == r.recession, "scaled recession unchanged");
    certify(g, w);}
// Inscribed circle by brute force over triples of boundary lines (3D LP vertices).
long double inradius(const vector<Halfplane> &h, dpoint &best) {
    long double res = -1; int n = int(h.size());
    auto dist = [&](const Halfplane &x, dpoint p) {
        dpoint u = dirOf(x).cast<long double>(); return cross(u, p - x.a.cast<long double>()) / normApprox(u);};
    for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { for (int k = j + 1; k < n; ++k) {
        long double M[3][4]; int id[3] = {i, j, k};
        for (int t = 0; t < 3; ++t) {
            dpoint u = dirOf(h[id[t]]).cast<long double>(), a = h[id[t]].a.cast<long double>(); long double l = normApprox(u);
            M[t][0] = -u.y / l; M[t][1] = u.x / l; M[t][2] = -1; M[t][3] = cross(u, a) / l;}
        auto det3 = [&](int c0, int c1, int c2) {
            return M[0][c0] * (M[1][c1] * M[2][c2] - M[1][c2] * M[2][c1]) - M[0][c1] * (M[1][c0] * M[2][c2] - M[1][c2] * M[2][c0])
                 + M[0][c2] * (M[1][c0] * M[2][c1] - M[1][c1] * M[2][c0]);};
        long double D = det3(0, 1, 2);
        if (std::abs(D) < 1e-12L) { continue; }
        dpoint p{det3(3, 1, 2) / D, det3(0, 3, 2) / D}; long double r = det3(0, 1, 3) / D;
        bool ok = r >= 0;
        for (auto &x : h) { ok = ok && dist(x, p) >= r - 1e-9L * max(1.L, r); }
        if (ok && r > res) { res = r; best = p; }}}}
    return res;}
void chebyshev(const vector<Halfplane> &h) {
    input = h; dpoint c; long double r = 0; bool ok = chebyshevCenterApprox(h, c, r);
    auto res = halfplaneIntersection(h);
    require(ok == (res.kind == HalfplaneKind::Polygon), "chebyshev status iff polygon");
    if (!ok) { return; }
    dpoint b; long double want = inradius(res.planes, b);
    require(std::abs(r - want) <= 1e-9L * max(1.L, want) && r > 0, "chebyshev radius equals triple LP optimum");
    for (auto &x : h) {
        dpoint u = dirOf(x).cast<long double>();
        require(cross(u, c - x.a.cast<long double>()) / normApprox(u) >= r - 1e-9L * max(1.L, r), "chebyshev center keeps radius to every line");}}
vector<point> convexRandom(std::mt19937_64 &rng, int c, int n, bool collinear) {
    vector<point> p;
    for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
    return convexHull(p, collinear);}
void clip(const vector<point> &p, const vector<Halfplane> &h) {
    vector<Halfplane> all = h; int n = int(p.size());
    for (int i = 0; i < n; ++i) { all.push_back({p[i], p[(i + 1) % n]}); }
    input = all; verify(all, clipConvex(p, h));
    vector<Point2<int>> q; for (auto x : p) { q.push_back(x.cast<int>()); }
    auto r = clipConvex(q, h); require(r.kind == halfplaneIntersection(all).kind, "clipConvex for int coordinates");}
void kernel(const vector<point> &p) {
    vector<Halfplane> h; int n = int(p.size()); auto s = signedArea2(p);
    for (int i = 0; i < n; ++i) {
        point a = p[i], b = p[(i + 1) % n];
        if (a != b) { h.push_back(s > 0 ? Halfplane{a, b} : Halfplane{b, a}); }}
    input = h; auto r = polygonKernel(p); verify(h, r);
    require(r.kind != HalfplaneKind::Unbounded && r.kind != HalfplaneKind::Ray && r.kind != HalfplaneKind::Line, "kernel is bounded");
    auto l = polygonKernelLinear(p);
    require(l.kind == r.kind && samePoints(l.vertices, r.vertices), "linear kernel equals the brute-force kernel");
    if (r.kind == HalfplaneKind::Polygon) { require(l.vertices == r.vertices, "linear kernel keeps the canonical vertex order"); }
    if (l.kind != HalfplaneKind::Empty) {
        Oracle o = oracle(l.planes);
        require(o.kind == r.kind && samePoints(o.v, oracle(h).v), "linear kernel planes reproduce the kernel");}}
// Corridor between two Archimedean spirals: simple, empty kernel, total turning far beyond 6 * pi.
vector<point> spiral(int n, long double turns) {
    vector<point> out, in;
    for (int i = 0; i < n; ++i) {
        long double t = turns * 2 * std::numbers::pi_v<long double> * i / (n - 1), r = 10 + 3 * t;
        out.push_back({lng(std::llround(1e4L * r * std::cos(t))), lng(std::llround(1e4L * r * std::sin(t)))});
        in.push_back({lng(std::llround(1e4L * (r - 2) * std::cos(t))), lng(std::llround(1e4L * (r - 2) * std::sin(t)))});}
    reverse(in.begin(), in.end()); out.insert(out.end(), in.begin(), in.end());
    return out;}
void kernelLarge(std::mt19937_64 &rng, int n) {
    phase = "kernel large star"; input = {};
    vector<point> p; const lng C = 1000000000;
    vector<long double> ang(n);
    for (auto &a : ang) { a = (long double)(rng() % 1000000007) / 1000000007.L * 2 * std::numbers::pi_v<long double>; }
    sort(ang.begin(), ang.end());
    for (int i = 0; i < n; ++i) {
        long double r = (i % 2 ? 0.55L : 0.95L) * C * (0.9L + 0.1L * (long double)(rng() % 1000) / 1000);
        point q{lng(std::llround(r * std::cos(ang[i]))), lng(std::llround(r * std::sin(ang[i])))};
        if (p.empty() || (q != p.back() && cross(p.back(), q) > 0)) { p.push_back(q); }}
    while (p.size() > 2 && cross(p.back(), p[0]) <= 0) { p.pop_back(); }
    auto a = polygonKernel(p), b = polygonKernelLinear(p);
    require(a.kind == b.kind && a.vertices == b.vertices, "large star: linear and sorted kernels agree");
    require(a.kind == HalfplaneKind::Polygon, "large star kernel contains the origin");
    phase = "kernel spiral";
    auto s = spiral(max(200, n / 4), 10);
    require(polygonKernel(s).kind == HalfplaneKind::Empty && polygonKernelLinear(s).kind == HalfplaneKind::Empty, "spiral corridor has an empty kernel");}
bool properCross(point a, point b, point c, point d) {
    return orient(a, b, c) * orient(a, b, d) < 0 && orient(c, d, a) * orient(c, d, b) < 0;}
bool simple(const vector<point> &p) {
    int n = int(p.size());
    for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) {
        point a = p[i], b = p[(i + 1) % n], c = p[j], d = p[(j + 1) % n];
        bool adjacent = j == i + 1 || (i == 0 && j == n - 1);
        if (adjacent) {
            point s = j == i + 1 ? a : b, t = j == i + 1 ? d : c, m = j == i + 1 ? b : a;
            if (orient(s, m, t) == 0 && dot(s - m, t - m) > 0) { return false; }
            continue;}
        Linear2 s = Linear2::segment(a, b), t = Linear2::segment(c, d);
        if (properCross(a, b, c, d) || contains(s, c) || contains(s, d) || contains(t, a) || contains(t, b)) { return false; }}}
    return signedArea2(p) != 0;}
vector<point> randomSimple(std::mt19937_64 &rng, int c, int n) {
    for (;;) {
        vector<point> p;
        for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        sort(p.begin(), p.end()); if (std::unique(p.begin(), p.end()) != p.end()) { continue; }
        std::shuffle(p.begin(), p.end(), rng);
        for (int it = 0; it < 200 && !simple(p); ++it) {
            for (int i = 0; i < n; ++i) { for (int j = i + 2; j < n; ++j) {
                if (i == 0 && j == n - 1) { continue; }
                if (properCross(p[i], p[i + 1], p[j], p[(j + 1) % n]) || contains(Linear2::segment(p[i], p[i + 1]), p[j]) || contains(Linear2::segment(p[j], p[(j + 1) % n]), p[i + 1])) {
                    reverse(p.begin() + i + 1, p.begin() + j + 1);}}}}
        if (simple(p)) { return p; }}}
vector<point> starShaped(std::mt19937_64 &rng, int c, int n) {
    for (;;) {
        vector<point> p;
        for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        sort(p.begin(), p.end(), [](point a, point b) { return polarLess(a, b); });
        bool ok = true;
        for (int i = 0; i < n; ++i) { ok = ok && p[i] != point{} && !(cross(p[i], p[(i + 1) % n]) == 0 && dot(p[i], p[(i + 1) % n]) > 0); }
        if (ok && simple(p)) { return p; }}}
void fixed() {
    phase = "contains";
    for (auto &h : pool(1)) {
        input = {h};
        for (lng x = -3; x <= 3; ++x) { for (lng y = -3; y <= 3; ++y) {
            int s = depth(h, point{x, y});
            require(h.contains(point{x, y}) == (s >= 0) && h.contains(point{x, y}, true) == (s > 0), "contains integral point, closed and strict");
            Q q(2 * x + 1, 2 * y - 1, 4); s = depth(h, q);
            require(h.contains(q) == (s >= 0) && h.contains(q, true) == (s > 0), "contains rational point, closed and strict");}}}
    const lng C = 1000000000; Halfplane w{{-C, -C}, {C, C - 1}};
    Q far(lll(C) * 8000000000000000000LL + 3, lll(C) * 8000000000000000000LL - 7, 8000000000000000000LL);
    input = {w}; require(w.contains(far) == (depth(w, far) >= 0) && w.contains(far, true) == (depth(w, far) > 0), "contains at the rational bound");
    phase = "fixed";
    run({});
    run({{{0, 0}, {1, 0}}});
    run({{{0, 0}, {1, 0}}, {{0, 0}, {2, 0}}, {{5, 0}, {6, 0}}});
    run({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}});
    run({{{0, 0}, {1, 0}}, {{1, 1}, {0, 1}}});
    run({{{0, 1}, {1, 1}}, {{1, 0}, {0, 0}}});
    run({{{0, 0}, {1, 0}}, {{0, 0}, {0, -1}}});
    run({{{0, 0}, {1, 0}}, {{3, 0}, {3, 1}}, {{1, 2}, {0, 2}}});
    run({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}, {{3, 0}, {3, 1}}});
    run({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}, {{3, 0}, {3, 1}}, {{3, 1}, {3, 0}}});
    run({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}, {{3, 0}, {3, 1}}, {{-2, 5}, {-2, 0}}});
    run({{{0, 0}, {1, 0}}, {{0, 0}, {-1, 1}}, {{0, 0}, {-1, -1}}});
    run({{{0, 0}, {1, 0}}, {{0, 0}, {-1, 1}}, {{0, 0}, {-1, -1}}, {{0, 0}, {0, 1}}, {{2, 2}, {-2, -2}}});
    run({{{0, 0}, {4, 0}}, {{4, 0}, {4, 4}}, {{4, 4}, {0, 4}}, {{0, 4}, {0, 0}}, {{0, 0}, {4, 4}}, {{4, 4}, {0, 0}}});
    run({{{0, 0}, {1, 0}}, {{1, 1}, {0, 1}}, {{0, 2}, {0, 3}}});
    run({{{0, 0}, {1, 0}}, {{2, 0}, {3, 1}}, {{0, 3}, {0, 2}}, {{5, 5}, {4, 6}}});
    phase = "chebyshev fixed";
    vector<Halfplane> sq{{{0, 0}, {4, 0}}, {{4, 0}, {4, 4}}, {{4, 4}, {0, 4}}, {{0, 4}, {0, 0}}};
    dpoint c; long double r; input = sq;
    require(chebyshevCenterApprox(sq, c, r) && std::abs(r - 2) < 1e-15L && std::abs(c.x - 2) < 1e-12L && std::abs(c.y - 2) < 1e-12L, "square incircle");
    vector<Halfplane> rect{{{0, 0}, {6, 0}}, {{6, 0}, {6, 2}}, {{6, 2}, {0, 2}}, {{0, 2}, {0, 0}}}; chebyshev(rect);
    require(!chebyshevCenterApprox({{{0, 0}, {1, 0}}}, c, r), "unbounded has no center");
    chebyshev({{{0, 0}, {3, 0}}, {{3, 0}, {0, 4}}, {{0, 4}, {0, 0}}});
    phase = "kernel out of domain";
#ifndef NDEBUG
    // Checked builds only: non-simple input passing every assert must stay in bounds (_GLIBCXX_DEBUG, ASan).
    for (auto p : {vector<point>{{0, 0}, {10, 10}, {-10, 20}, {10, 0}}, vector<point>{{0, 3}, {2, -3}, {-3, 1}, {3, 1}, {-2, -3}}}) {
        auto r = polygonKernelLinear(p); require(int(r.kind) >= 0, "non-simple input terminates");}
#endif
    phase = "kernel fixed";
    kernel({{0, 0}, {4, 0}, {4, 4}, {0, 4}});
    kernel({{0, 4}, {4, 4}, {4, 0}, {0, 0}});
    kernel({{0, 0}, {4, 0}, {4, 1}, {2, 1}, {2, 3}, {4, 3}, {4, 4}, {0, 4}});
    kernel({{0, 0}, {2, 0}, {2, 2}, {4, 2}, {4, 4}, {2, 4}, {2, 6}, {0, 6}, {0, 4}, {-2, 4}, {-2, 2}, {0, 2}});
    kernel({{0, 0}, {6, 0}, {6, 1}, {3, 1}, {6, 2}, {6, 3}, {0, 3}, {0, 2}, {3, 1}, {0, 1}});
    kernel({{0, 0}, {2, 0}, {2, 0}, {2, 2}, {1, 1}, {0, 2}});
    kernel({{0, 0}, {4, 0}, {4, 1}, {1, 1}, {1, 3}, {4, 3}, {4, 4}, {0, 4}});
    phase = "clip fixed";
    clip({{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {{{0, 0}, {4, 4}}});
    clip({{0, 0}, {2, 0}, {4, 0}, {4, 4}, {0, 4}}, {{{4, 4}, {0, 0}}, {{0, 4}, {4, 0}}});
    clip({{0, 0}, {2, 0}, {4, 0}, {4, 4}, {0, 4}, {0, 2}}, {{{-1, 0}, {5, 0}}});
    clip({{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {{{9, 9}, {8, 9}}});
    clip({{0, 0}, {4, 0}, {4, 4}, {0, 4}}, {});}
void wide(std::mt19937_64 &rng, int rounds) {
    phase = "wide metamorphic";
    const lng C = 1000000000; auto small = pool(1);
    for (int t = 0; t < rounds; ++t) {
        int k = 1 + int(rng() % 6); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) { h.push_back(small[rng() % small.size()]); }
        lng s = 1 + lng(rng() % (C / 4)); point tr{lng(rng() % (C / 2)) - C / 4, lng(rng() % (C / 2)) - C / 4};
        input = h; metamorphic(h, s, tr);}
    phase = "wide random";
    for (int t = 0; t < rounds; ++t) {
        int k = 1 + int(rng() % 9); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) {
            point a{lng(rng() % (2 * C + 1)) - C, lng(rng() % (2 * C + 1)) - C}, b{lng(rng() % (2 * C + 1)) - C, lng(rng() % (2 * C + 1)) - C};
            if (rng() % 4 == 0) { b = a + point{lng(rng() % 3) - 1, lng(rng() % 3) - 1}; if (b.x > C || b.x < -C || b.y > C || b.y < -C) { b = a; } }
            if (a != b) { h.push_back({a, b}); }}
        input = h; certify(h, halfplaneIntersection(h));}
    phase = "wide extremes";
    vector<Halfplane> box{{{-C, -C}, {C, -C}}, {{C, -C}, {C, C}}, {{C, C}, {-C, C}}, {{-C, C}, {-C, -C}}, {{-C, -C}, {C, C - 1}}, {{C, C}, {-C, -C + 1}}};
    input = box; auto r = halfplaneIntersection(box); certify(box, r);
    require(r.kind == HalfplaneKind::Polygon && r.vertices.size() == 4, "nearly parallel diagonals keep a sliver");
    vector<Halfplane> near{{{-C, -C}, {C, C - 1}}, {{C, C}, {-C, -C + 1}}, {{0, -C}, {0, C}}};
    input = near; certify(near, halfplaneIntersection(near));}
void large(std::mt19937_64 &rng, int n) {
    phase = "large circle tangents";
    vector<Halfplane> h; const long double R = 9e8L;
    vector<long double> ang(n);
    for (auto &a : ang) { a = (long double)(rng() % 1000000007) / 1000000007.L * 2 * std::numbers::pi_v<long double>; }
    for (auto a : ang) {
        point p{lng(std::llround(R * std::cos(a))), lng(std::llround(R * std::sin(a)))};
        point d{lng(std::llround(-1e5L * std::sin(a))), lng(std::llround(1e5L * std::cos(a)))};
        h.push_back({p, p + d});}
    input = {}; auto r = halfplaneIntersection(h);
    require(r.kind == HalfplaneKind::Polygon && r.vertices.size() >= 3, "large tangent family is a polygon");
    int k = int(r.vertices.size());
    for (int i = 0; i < k; ++i) {
        require(r.planes[i].contains(r.vertices[i]) && cross(dirOf(r.planes[i]), dirOf(r.planes[(i + 1) % k])) > 0, "large chain certificate");
        if (i % 97 == 0) { for (int j = 0; j < n; j += 101) { require(h[j].contains(r.vertices[i]), "large sampled feasibility"); } }}}
void invalid(const string &name) {
    if (name == "degenerate-plane") { (void)halfplaneIntersection({{{1, 1}, {1, 1}}}); }
    if (name == "coordinate-bound") { (void)halfplaneIntersection({{{0, 0}, {1000000001, 0}}}); }
    if (name == "clip-clockwise") { (void)clipConvex(vector<point>{{0, 0}, {0, 4}, {4, 4}, {4, 0}}, {}); }
    if (name == "clip-small") { (void)clipConvex(vector<point>{{0, 0}, {4, 0}}, {}); }
    if (name == "kernel-zero-area") { (void)polygonKernel(vector<point>{{0, 0}, {1, 0}, {2, 0}}); }
    if (name == "linear-zero-area") { (void)polygonKernelLinear(vector<point>{{0, 0}, {1, 1}, {2, 2}}); }
    if (name == "linear-spike") { (void)polygonKernelLinear(vector<point>{{0, 0}, {2, 0}, {1, 0}, {1, 1}}); }
    if (name == "linear-spike-first") { (void)polygonKernelLinear(vector<point>{{0, 3}, {0, 0}, {2, 0}, {2, 2}, {0, 2}}); }
    if (name == "linear-spike-last") { (void)polygonKernelLinear(vector<point>{{0, 0}, {2, 0}, {2, 2}, {0, 2}, {0, 3}}); }
    if (name == "linear-bound") { (void)polygonKernelLinear(vector<point>{{0, 0}, {1000000001, 0}, {0, 1}}); }
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
    phase = "exhaustive pairs and triples";
    auto p1 = pool(1); int m = int(p1.size());
    for (int i = 0; i < m; ++i) { for (int j = i; j < m; ++j) { run({p1[i], p1[j]}); } }
    int step = quick ? 7 : 1;
    for (int i = 0; i < m; i += step) { for (int j = i + 1; j < m; ++j) { for (int k = j + 1; k < m; ++k) { run({p1[i], p1[j], p1[k]}); } } }
    if (stress) {
        phase = "exhaustive quadruples";
        for (int i = 0; i < m; ++i) { for (int j = i + 1; j < m; ++j) { for (int k = j + 1; k < m; ++k) { for (int l = k + 1; l < m; ++l) {
            run({p1[i], p1[j], p1[k], p1[l]});}}}}}
    phase = "random small";
    auto p2 = pool(2); int rounds = quick ? 2000 : stress ? 400000 : 60000;
    for (int t = 0; t < rounds; ++t) {
        int k = 1 + int(rng() % 9); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) { h.push_back(p2[rng() % p2.size()]); }
        if (rng() % 3 == 0) { h.push_back({h[0].b, h[0].a}); }
        std::shuffle(h.begin(), h.end(), rng); run(h);}
    phase = "random concurrent";
    for (int t = 0; t < rounds / 4; ++t) {
        point o{lng(rng() % 7) - 3, lng(rng() % 7) - 3}; int k = 2 + int(rng() % 6); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) {
            point d{lng(rng() % 7) - 3, lng(rng() % 7) - 3};
            if (d != point{}) { h.push_back({o, o + d}); }}
        if (rng() % 2) { h.push_back(p2[rng() % p2.size()]); }
        run(h);}
    phase = "random medium";
    for (int t = 0; t < rounds / 10; ++t) {
        int k = 1 + int(rng() % 12); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) {
            point a{lng(rng() % 61) - 30, lng(rng() % 61) - 30}, b{lng(rng() % 61) - 30, lng(rng() % 61) - 30};
            if (a != b) { h.push_back({a, b}); }}
        run(h);}
    phase = "chebyshev random";
    for (int t = 0; t < rounds / 40; ++t) {
        int k = 3 + int(rng() % 7); vector<Halfplane> h;
        for (int i = 0; i < k; ++i) {
            point a{lng(rng() % 41) - 20, lng(rng() % 41) - 20}, b{lng(rng() % 41) - 20, lng(rng() % 41) - 20};
            if (a != b) { h.push_back({a, b}); }}
        chebyshev(h);}
    phase = "clip random";
    for (int t = 0; t < rounds / 20; ++t) {
        auto p = convexRandom(rng, 4, 3 + int(rng() % 8), rng() % 2);
        if (p.size() < 3 || signedArea2(p) == 0) { continue; }
        vector<Halfplane> h; int k = int(rng() % 5);
        for (int i = 0; i < k; ++i) { h.push_back(p2[rng() % p2.size()]); }
        clip(p, h);}
    phase = "kernel random";
    for (int t = 0; t < rounds / 6; ++t) {
        int c = 2 + int(rng() % 3), kind = int(rng() % 3);
        auto p = kind == 0 ? randomSimple(rng, c, 4 + int(rng() % 8)) : kind == 1 ? starShaped(rng, c + 1, 4 + int(rng() % 10)) : randomSimple(rng, 5, 8 + int(rng() % 8));
        if (rng() % 2) { reverse(p.begin(), p.end()); }
        std::rotate(p.begin(), p.begin() + rng() % p.size(), p.end());
        kernel(p);}
    if (stress) {
        phase = "kernel exhaustive 3x3 grid";
        vector<point> g;
        for (int x = 0; x < 3; ++x) { for (int y = 0; y < 3; ++y) { g.push_back({x, y}); } }
        vector<point> cur; vector<char> used(9, 0);
        auto rec = [&](auto &&rec) -> void {
            if (cur.size() >= 3 && simple(cur)) { kernel(cur); }
            if (cur.size() == 9) { return; }
            for (int i = 1; i < 9; ++i) {
                if (used[i] || g[i] < cur[0]) { continue; }
                used[i] = 1; cur.push_back(g[i]); rec(rec); cur.pop_back(); used[i] = 0;}};
        for (int s = 0; s < 9; ++s) { cur = {g[s]}; used.assign(9, 0); used[s] = 1; rec(rec); }}
    wide(rng, quick ? 300 : stress ? 60000 : 8000);
    large(rng, quick ? 2000 : stress ? 400000 : 200000);
    kernelLarge(rng, quick ? 2000 : stress ? 400000 : 200000);
    std::cout << "PASS halfplaneintersection seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
