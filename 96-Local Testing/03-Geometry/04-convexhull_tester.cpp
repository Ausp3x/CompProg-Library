#include "../../03-Geometry/04-convexhull.hpp"

static ulng seed = 20260927;
static std::mt19937_64 rng;
static vector<point> input, expected, actual;
static string phase;
static int checks = 0;
void dump(const char *name, const vector<point> &p) {
    std::cerr << name << '='; for (auto q : p) { std::cerr << '(' << q.x << ',' << q.y << ')'; } std::cerr << ' ';}
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << ' ';
    dump("input", input); dump("expected", expected); dump("actual", actual); std::cerr << '\n'; std::exit(1);}
lll det(point a, point b, point c) { return (lll(b.x) - a.x) * (lll(c.y) - a.y) - (lll(b.y) - a.y) * (lll(c.x) - a.x); }
lll square(point a, point b) { lll x = lll(a.x) - b.x, y = lll(a.y) - b.y; return x * x + y * y; }
// Independent Jarvis march chooses a supporting edge among ALL remaining points
// at each step, then inserts points on that edge for the retained policy.
vector<point> oracle(vector<point> p, bool retain) {
    sort(p.begin(), p.end()); p.erase(unique(p.begin(), p.end()), p.end());
    int n = int(p.size()); if (n <= 2) { return p; }
    bool line = true; for (point q : p) { line &= det(p[0], p[1], q) == 0; }
    if (line) { return retain ? p : vector<point>{p.front(), p.back()}; }
    vector<point> hull; point a = p[0];
    do {
        point b = p[0] == a ? p[1] : p[0];
        for (point c : p) {
            lll d = det(a, b, c);
            if (d < 0 || (d == 0 && square(a, b) < square(a, c))) { b = c; }}
        if (retain) {
            vector<point> edge;
            for (point c : p) {
                if (!det(a, b, c) && min(a.x, b.x) <= c.x && c.x <= max(a.x, b.x) && min(a.y, b.y) <= c.y && c.y <= max(a.y, b.y) && c != b) {
                    edge.push_back(c);}}
            sort(edge.begin(), edge.end(), [a](point b, point c) { return square(a, b) < square(a, c); });
            hull.insert(hull.end(), edge.begin(), edge.end());}
        else { hull.push_back(a); }
        a = b;} while (a != p[0]);
    return hull;}
// The oracle's CCW cycle from the lexicographic minimum is the lower chain up to the maximum (a vertical right edge
// included), the rest closed and reversed the upper chain (a vertical left edge included); collinear input: both whole.
pair<vector<point>, vector<point>> chains(const vector<point> &p, bool retain) {
    vector<point> c = oracle(p, retain);
    bool line = true; for (point q : p) { line &= det(c.front(), c.back(), q) == 0; }
    if (line) { return {c, c}; }
    int m = int(std::max_element(c.begin(), c.end()) - c.begin());
    vector<point> lower(c.begin(), c.begin() + m + 1), upper(c.begin() + m, c.end());
    upper.push_back(c[0]); reverse(upper.begin(), upper.end());
    return {lower, upper};}
void checkMelkman(const vector<point> &q, const vector<point> &want, bool retain = false) {
    input = q; expected = want; actual = convexHullMelkman(q, retain);
    require(actual == expected, retain ? "Melkman retain" : "Melkman strict"); require(q == input, "Melkman input unchanged");}
// Strict and retained oracle hulls; the strict one drops the retained cycle's vertices interior to an edge, or keeps
// the ends of a collinear one.
pair<vector<point>, vector<point>> hulls(const vector<point> &q) {
    vector<point> c = oracle(q, true), s; int n = int(c.size());
    bool line = true; for (point x : c) { line &= det(c.front(), c.back(), x) == 0; }
    for (int i = 0; i < n; ++i) {
        if (line ? i == 0 || i == n - 1 : det(c[(i + n - 1) % n], c[i], c[(i + 1) % n]) != 0) { s.push_back(c[i]); }}
    return {s, c};}
void checkMelkman(const vector<point> &q, const pair<vector<point>, vector<point>> &h) { checkMelkman(q, h.first); checkMelkman(q, h.second, true); }
void checkMelkman(const vector<point> &q) { checkMelkman(q, hulls(q)); }
// A simple polyline or ring (then also rotated), its reversal, a contiguous piece and a copy with repeated neighbours;
// all but the piece share the point set and so its hulls.
void melkman(vector<point> p, bool ring) {
    auto h = hulls(p);
    for (int turn = 0; turn < 2; ++turn) {
        checkMelkman(p, h);
        int n = int(p.size());
        if (n) {
            if (ring) { std::rotate(p.begin(), p.begin() + int(rng() % ulng(n)), p.end()); checkMelkman(p, h); }
            int l = int(rng() % ulng(n)), r = l + 1 + int(rng() % ulng(n - l));
            checkMelkman(vector<point>(p.begin() + l, p.begin() + r));
            vector<point> s; for (point q : p) { s.push_back(q); if (rng() % 4 == 0) { s.push_back(q); }}
            checkMelkman(s, h);}
        reverse(p.begin(), p.end());}}
// Indices must name the expected points in order, each the first occurrence of its point.
void verifyIndices(const vector<point> &p, const vector<int> &id, const char *what) {
    actual.clear();
    for (int i : id) {
        require(0 <= i && i < int(p.size()), "hull index in range");
        actual.push_back(p[i]);
        require(std::find(p.begin(), p.end(), p[i]) - p.begin() == i, "hull index is first duplicate");}
    require(actual == expected, what);}
void verify(const vector<point> &p) {
    const vector<point> before = p;
    auto sorted = p; sort(sorted.begin(), sorted.end());
    for (bool retain : {false, true}) {
        input = p; expected = oracle(p, retain);
        actual = convexHull(p, retain); require(actual == expected, retain ? "Andrew retain" : "Andrew strict");
        actual = convexHullGraham(p, retain); require(actual == expected, retain ? "Graham retain" : "Graham strict");
        actual = convexHullSorted(sorted, retain); require(actual == expected, retain ? "Sorted retain" : "Sorted strict");
        actual = convexHullJarvis(p, retain); require(actual == expected, retain ? "Jarvis retain" : "Jarvis strict");
        verifyIndices(p, convexHullIndices(p, retain), retain ? "indices retain" : "indices strict");
        if (!actual.empty()) { require(actual.front() == *std::min_element(p.begin(), p.end()), "canonical minimum"); }
        auto copy = actual; sort(copy.begin(), copy.end()); require(std::adjacent_find(copy.begin(), copy.end()) == copy.end(), "duplicates removed");
        if (actual.size() >= 3 && det(actual[0], actual[1], actual.back()) != 0) {
            lll area = 0; for (int i = 0; i < int(actual.size()); ++i) {
                point a = actual[i], b = actual[(i + 1) % actual.size()]; area += lll(a.x) * b.y - lll(a.y) * b.x;
                for (point q : p) { require(det(a, b, q) >= 0, "supporting CCW edge"); }}
            require(area > 0, "positive orientation");}
        auto [lower, upper] = chains(p, retain);
        expected = lower; actual = lowerHull(p, retain); require(actual == expected, retain ? "lowerHull retain" : "lowerHull strict");
        expected = upper; actual = upperHull(p, retain); require(actual == expected, retain ? "upperHull retain" : "upperHull strict");
        actual = lowerHullSorted(sorted, retain); require(actual == lower, retain ? "lowerHullSorted retain" : "lowerHullSorted strict");
        if (!retain) { require(lowerHullSorted(sorted) == lower && upperHullSorted(sorted) == upper, "Sorted chains default policy"); }
        actual = upperHullSorted(sorted, retain); require(actual == upper, retain ? "upperHullSorted retain" : "upperHullSorted strict");
        expected = lower; verifyIndices(p, lowerHullIndices(p, retain), retain ? "lowerHullIndices retain" : "lowerHullIndices strict");
        expected = upper; verifyIndices(p, upperHullIndices(p, retain), retain ? "upperHullIndices retain" : "upperHullIndices strict");
        if (!retain) { require(lowerHullIndices(p) == lowerHullIndices(p, false) && upperHullIndices(p) == upperHullIndices(p, false), "chain indices default policy"); }}
    // A lexicographically sorted sequence is a simple polyline; the retained hull is a simple ring unless collinear.
    melkman(sorted, false);
    auto ring = oracle(p, true);
    melkman(ring, ring.size() >= 3 && det(ring[0], ring[1], ring.back()) != 0);
    input = p; require(p == before, "callers' inputs unchanged");}
// One of the eight lattice symmetries, which keep a polyline simple, then a translation within [-c, c].
vector<point> moved(vector<point> p, lng c) {
    int s = int(rng() % 8); lng dx = lng(rng() % ulng(2 * c + 1)) - c, dy = lng(rng() % ulng(2 * c + 1)) - c;
    for (auto &q : p) {
        point r = s & 1 ? point{q.y, q.x} : q;
        if (s & 2) { r.x = -r.x; }
        if (s & 4) { r.y = -r.y; }
        q = {r.x + dx, r.y + dy};}
    return p;}
// x never decreases and a vertical run never reverses, so the polyline is simple; steps repeat for straight runs,
// always over a random collinear prefix.
vector<point> monotone(int n) {
    vector<point> p{{0, 0}};
    lng dx = lng(rng() % 3), dy = dx ? lng(rng() % 5) - 2 : 1;
    int prefix = int(rng() % 6);
    while (int(p.size()) < n) {
        if (int(p.size()) > prefix && rng() % 3) {
            lng ex = lng(rng() % 3);
            dy = ex ? lng(rng() % 5) - 2 : (dx == 0 ? (dy > 0 ? 1 : -1) : (rng() % 2 ? 1 : -1)) * lng(1 + rng() % 2);
            dx = ex;}
        p.push_back({p.back().x + dx, p.back().y + dy});}
    return p;}
// Distinct lattice points sorted by exact angle about the origin, ties outward; the four axis points keep every
// angular gap below pi, so each edge stays in its open wedge and the ring is simple (star-shaped).
vector<point> star(lng r, int n) {
    lng a = 1 + lng(rng() % ulng(r));
    set<pair<lng, lng>> s{{a, 0}, {0, a}, {-a, 0}, {0, -a}};
    for (int k = 0; k < n; ++k) {
        lng x = lng(rng() % ulng(2 * r + 1)) - r, y = lng(rng() % ulng(2 * r + 1)) - r;
        if (x || y) { s.insert({x, y}); }}
    vector<point> p; for (auto [x, y] : s) { p.push_back({x, y}); }
    auto low = [](point q) { return q.y < 0 || (q.y == 0 && q.x < 0); };
    sort(p.begin(), p.end(), [&](point u, point v) {
        if (low(u) != low(v)) { return low(v); }
        lll c = det(point{}, u, v); return c ? c > 0 : square(point{}, u) < square(point{}, v);});
    return p;}
// Unit lattice steps that never revisit a point (a self-avoiding walk, hence simple), straight with probability 3/4.
vector<point> walk(int n) {
    const int ex[4] = {1, 0, -1, 0}, ey[4] = {0, 1, 0, -1}, order[4] = {0, 1, 3, 2};
    vector<point> p{{0, 0}}; set<pair<lng, lng>> seen{{0, 0}}; int d = int(rng() % 4);
    while (int(p.size()) < n) {
        if (rng() % 4 == 0) { d = (d + (rng() % 2 ? 1 : 3)) % 4; }
        int k = 0;
        for (; k < 4; ++k) {
            int e = (d + order[k]) % 4; point q{p.back().x + ex[e], p.back().y + ey[e]};
            if (seen.insert({q.x, q.y}).second) { p.push_back(q); d = e; break; }}
        if (k == 4) { break; }}
    return p;}
// Drops random vertices interior to straight runs of a polyline without reversals; its point set is unchanged.
vector<point> thin(const vector<point> &p) {
    vector<point> q;
    for (int i = 0; i < int(p.size()); ++i) {
        if (i == 0 || i + 1 == int(p.size()) || det(p[i - 1], p[i], p[i + 1]) || rng() % 2) { q.push_back(p[i]); }}
    return q;}
// x-monotone ring: lower chain left to right, then the upper chain back, strictly above it between the ends; an end
// whose chains differ is a vertical edge; flat chains trace a rectangle through every lattice point of its boundary.
vector<point> mountain(int m, lng h) {
    vector<point> lo, hi; bool flat = rng() % 4 == 0; lng x = 0;
    lng a0 = lng(rng() % ulng(h)), b0 = a0 + 1 + lng(rng() % ulng(h));
    for (int k = 0; k <= m; ++k, x += flat ? 1 : 1 + lng(rng() % 2)) {
        lng a = flat ? a0 : lng(rng() % ulng(h)), b = flat ? b0 : a + 1 + lng(rng() % ulng(h));
        if ((k == 0 || k == m) && rng() % 2) { b = a; }
        lo.push_back({x, a}); hi.push_back({x, b});}
    vector<point> p = lo;
    for (int k = m; k >= 0; --k) { if (hi[k] != lo[k]) { p.push_back(hi[k]); }}
    return p;}
// Square spiral of unit steps with run lengths 1, 1, 2, 2, ...: self-avoiding.
vector<point> spiral(int n) {
    const int ex[4] = {1, 0, -1, 0}, ey[4] = {0, 1, 0, -1};
    vector<point> p{{0, 0}};
    for (int run = 1, d = 0; int(p.size()) < n; ++d) {
        for (int k = 0; k < run && int(p.size()) < n; ++k) { p.push_back({p.back().x + ex[d % 4], p.back().y + ey[d % 4]}); }
        if (d % 2) { ++run; }}
    return p;}
// Every lattice point of the boundary of [0, w] x [0, h], CCW from the origin.
vector<point> rectangle(lng w, lng h) {
    vector<point> p;
    for (lng x = 0; x < w; ++x) { p.push_back({x, 0}); }
    for (lng y = 0; y < h; ++y) { p.push_back({w, y}); }
    for (lng x = w; x > 0; --x) { p.push_back({x, h}); }
    for (lng y = h; y > 0; --y) { p.push_back({0, y}); }
    return p;}
// Scaled strictly convex lattice polygon v, CCW from its lexicographic minimum v0; after v_j the ring takes lattice points
// of edge j, or (2 <= j < m - 1) dents along diagonal v_j v0 and leaves inside triangle v0 v_j v_{j+1}: simple.
vector<point> fan(int n, lng r) {
    vector<point> s;
    for (int k = 0; k < n; ++k) { s.push_back({lng(rng() % ulng(2 * r + 1)) - r, lng(rng() % ulng(2 * r + 1)) - r}); }
    vector<point> v = oracle(s, false), p;
    if (v.size() < 3) { return v; }
    lng f = 2 + lng(rng() % 3); int m = int(v.size());
    for (auto &q : v) { q = {q.x * f, q.y * f}; }
    for (int j = 0; j < m; ++j) {
        point a = v[j], c = 2 <= j && j < m - 1 && rng() % 2 ? v[0] : v[(j + 1) % m];
        lng g = gcd(abs(c.x - a.x), abs(c.y - a.y));
        p.push_back(a);
        for (lng k = 1; k < g; ++k) {
            if (rng() % 2) { p.push_back({a.x + (c.x - a.x) / g * k, a.y + (c.y - a.y) / g * k}); }}}
    return p;}
void exact(const vector<point> &got, const vector<point> &want, const char *what) { actual = got; expected = want; require(actual == expected, what); }
vector<point> pick(const vector<point> &p, const vector<int> &id) { vector<point> q; for (int i : id) { q.push_back(p.at(i)); } return q; }
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") {
            string probe = argv[++i];
            if (probe == "sorted-unsorted") { (void)convexHullSorted(vector<point>{{1, 0}, {0, 0}}); }
            else if (probe == "lower-sorted-unsorted") { (void)lowerHullSorted(vector<point>{{0, 1}, {0, 0}}); }
            else if (probe == "upper-sorted-unsorted") { (void)upperHullSorted(vector<point>{{2, 0}, {1, 5}, {0, 0}}); }
            else if (probe == "sorted-unsorted-retain") { (void)convexHullSorted(vector<point>{{0, 0}, {1, 0}, {0, 0}}, true); }
            else if (probe == "lower-sorted-unsorted-retain") { (void)lowerHullSorted(vector<point>{{0, 0}, {2, 2}, {1, 1}, {2, 2}}, true); }
            else if (probe == "upper-sorted-unsorted-retain") { (void)upperHullSorted(vector<point>{{1, 1}, {0, 3}, {1, 1}}, true); }
            else { throw std::runtime_error("unknown invalid probe=" + probe); }
            cerr << "FAIL probe returned without firing: " << probe << '\n'; return 1;}}
    rng.seed(seed);
    phase = "regressions";
    for (const vector<point> &p : vector<vector<point>>{
        {}, {{0, 0}}, {{0, 0}, {0, 0}}, {{3, 4}, {0, 0}}, {{0, -3}, {0, 2}, {0, 1}, {0, -3}},
        {{-2, 2}, {0, 0}, {1, -1}, {-1, 1}}, {{0, 0}, {1, -2}, {1, 2}, {2, 0}, {1, 0}},
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}},
        {{-1000000000, -1000000000}, {1000000000, -1000000000}, {1000000000, 1000000000}, {-1000000000, 1000000000}, {0, 0}}}) { verify(p); }
    phase = "chain conventions";
    input = {{2, 2}, {0, 0}, {0, 2}, {2, 0}, {1, 0}, {0, 1}, {2, 1}, {1, 2}, {1, 1}, {0, 0}};
    exact(lowerHull(input), {{0, 0}, {2, 0}, {2, 2}}, "lowerHull square, vertical right edge lower");
    exact(lowerHull(input, true), {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}}, "lowerHull square retain");
    exact(upperHull(input), {{0, 0}, {0, 2}, {2, 2}}, "upperHull square, vertical left edge upper");
    exact(upperHull(input, true), {{0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}}, "upperHull square retain");
    input = {{0, 5}, {0, -1}, {0, 2}, {0, -1}};
    for (bool retain : {false, true}) {
        vector<point> want = retain ? vector<point>{{0, -1}, {0, 2}, {0, 5}} : vector<point>{{0, -1}, {0, 5}};
        exact(lowerHull(input, retain), want, "lowerHull vertical segment"); exact(upperHull(input, retain), want, "upperHull vertical segment");}
    input = {{2, 0}, {0, 0}, {2, 3}, {2, 0}};
    exact(lowerHull(input), {{0, 0}, {2, 0}, {2, 3}}, "lowerHull vertical right edge"); exact(upperHull(input), {{0, 0}, {2, 3}}, "upperHull vertical right edge");
    input = {{2, 1}, {0, 3}, {0, 0}};
    exact(lowerHull(input), {{0, 0}, {2, 1}}, "lowerHull vertical left edge"); exact(upperHull(input), {{0, 0}, {0, 3}, {2, 1}}, "upperHull vertical left edge");
    input = {{3, 3}, {3, 3}};
    exact(lowerHull(input), {{3, 3}}, "lowerHull one distinct point"); exact(upperHull(input, true), {{3, 3}}, "upperHull one distinct point");
    input.clear(); exact(lowerHull(input), {}, "lowerHull empty"); exact(upperHull(input), {}, "upperHull empty");
    phase = "Melkman hand cases";
    exact(convexHullMelkman(vector<point>{{1, 0}, {0, 0}}), {{0, 0}, {1, 0}}, "Melkman descending segment");
    exact(convexHullMelkman(vector<point>{{2, 2}, {1, 1}, {1, 1}, {0, 0}}), {{0, 0}, {2, 2}}, "Melkman descending collinear");
    exact(convexHullMelkman(vector<point>{{0, 3}, {0, 1}}), {{0, 1}, {0, 3}}, "Melkman descending vertical");
    for (auto [p, want] : vector<pair<vector<point>, vector<point>>>{
        {{}, {}}, {{{1, 1}}, {{1, 1}}}, {{{1, 1}, {1, 1}}, {{1, 1}}}, {{{3, 3}, {1, 1}, {1, 1}, {-2, -2}}, {{-2, -2}, {3, 3}}},
        {{{2, 0}, {1, 0}, {0, 0}, {0, 1}}, {{0, 0}, {2, 0}, {0, 1}}}, {{{0, 0}, {1, 0}, {1, 0}, {2, 0}, {2, -1}}, {{0, 0}, {2, -1}, {2, 0}}},
        {{{0, 0}, {0, 0}, {1, 0}, {2, 0}, {2, 1}, {1, 1}, {1, 2}, {0, 2}}, {{0, 0}, {2, 0}, {2, 1}, {1, 2}, {0, 2}}},
        {{{1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}}, {{0, 0}, {2, 0}, {2, 2}, {0, 2}}},
        {{{2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}, {0, 0}, {1, 0}, {2, 0}}, {{0, 0}, {2, 0}, {2, 2}, {0, 2}}},
        {{{1, 1}, {3, 1}, {3, 3}, {2, 2}, {1, 3}}, {{1, 1}, {3, 1}, {3, 3}, {1, 3}}}}) {
        checkMelkman(p, want); checkMelkman(p);}
    for (auto [p, want] : vector<pair<vector<point>, vector<point>>>{
        {{}, {}}, {{{1, 1}}, {{1, 1}}}, {{{1, 1}, {1, 1}}, {{1, 1}}}, {{{3, 3}, {1, 1}}, {{1, 1}, {3, 3}}},
        {{{3, 3}, {1, 1}, {1, 1}, {-2, -2}}, {{-2, -2}, {1, 1}, {3, 3}}}, {{{0, 4}, {0, 4}, {0, 2}, {0, 0}, {0, 0}}, {{0, 0}, {0, 2}, {0, 4}}},
        {{{2, 0}, {1, 0}, {0, 0}, {0, 1}}, {{0, 0}, {1, 0}, {2, 0}, {0, 1}}}, {{{0, 0}, {1, 0}, {1, 0}, {2, 0}, {2, -1}}, {{0, 0}, {2, -1}, {2, 0}, {1, 0}}},
        {{{1, 0}, {0, 0}, {0, 1}, {0, 2}, {1, 2}, {2, 2}, {2, 1}, {2, 0}}, {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}}},
        {{{1, 1}, {3, 1}, {3, 3}, {2, 2}, {1, 3}}, {{1, 1}, {3, 1}, {3, 3}, {1, 3}}},
        {{{0, 0}, {3, 0}, {6, 0}, {6, 3}, {4, 2}, {2, 1}, {3, 6}, {2, 6}, {0, 6}, {0, 3}}, {{0, 0}, {3, 0}, {6, 0}, {6, 3}, {3, 6}, {2, 6}, {0, 6}, {0, 3}}},
        {{{0, 0}, {4, 0}, {4, 2}, {2, 1}, {4, 4}, {2, 2}, {1, 1}, {0, 4}}, {{0, 0}, {4, 0}, {4, 2}, {4, 4}, {0, 4}}}}) {
        checkMelkman(p, want, true);}
    phase = "all 3x3 subsets";
    int masks = mode == "quick" ? 64 : 512;
    for (int mask = 0; mask < masks; ++mask) {
        vector<point> p;
        for (int k = 0; k < 9; ++k) { if (mask >> k & 1) { p.push_back({k % 3 - 1, k / 3 - 1}); }}
        if (!p.empty() && mask % 3 == 0) { p.push_back(p[0]); }
        std::shuffle(p.begin(), p.end(), rng); verify(p);}
    phase = "random grid and extremes";
    int rounds = mode == "quick" ? 25 : mode == "full" ? 1000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(rng() % 45); lng c = t % 3 ? 12 : 1000000000;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);
        for (auto &q : p) { q = {-q.y, q.x}; } verify(p);}
    phase = "duplicate-heavy and collinear multisets";
    for (int t = 0; t < rounds / 2; ++t) {
        vector<point> p; int n = int(rng() % 60); lng c = 1 + t % 2, dx = lng(rng() % 5) - 2, dy = lng(rng() % 5) - 2;
        for (int k = 0; k < n; ++k) {
            lng s = lng(rng() % 7) - 3;
            p.push_back(t % 3 ? point{lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c} : point{s * dx, s * dy});}
        verify(p);}
    phase = "boundary location";
    for (int t = 0; t < rounds / 4 + 20; ++t) {
        vector<point> p; int n = 3 + int(rng() % 12); lng c = 1 + t % 6;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        auto h = hulls(p).first;
        if (h.size() < 3) { continue; }
        for (lng x = -c; x <= c; ++x) { for (lng y = -c; y <= c; ++y) {
            bool inside = true, edge = false;
            for (int i = 0; i < int(h.size()); ++i) {
                lll d = det(h[i], h[(i + 1) % h.size()], point{x, y});
                inside &= d >= 0; edge |= d == 0;}
            if (inside) { require(hull_detail::boundary(h, point{x, y}) == edge, "boundary location against edge determinants"); }}}}
    phase = "simple polylines and rings";
    int shapes = mode == "quick" ? 25 : mode == "full" ? 1500 : 10000;
    for (int t = 0; t < shapes; ++t) {
        lng c = t % 5 ? 3 : 100000000;
        melkman(moved(monotone(1 + int(rng() % 40)), c), false);
        melkman(moved(star(1 + lng(rng() % 6), int(rng() % 30)), c), true);
        auto w = walk(1 + int(rng() % 60)); if (rng() % 2) { w = thin(w); }
        melkman(moved(w, c), w.size() >= 4 && square(w.front(), w.back()) == 1);
        auto m = mountain(1 + int(rng() % 15), lng(1) << rng() % 4); melkman(moved(m, c), m.size() >= 3);
        if (t % 2) { melkman(moved(rectangle(1 + lng(rng() % 6), 1 + lng(rng() % 6)), c), true); continue; }
        // A translation keeps v0 the lexicographic minimum; the other symmetries generally do not.
        auto f = fan(3 + int(rng() % 20), 1 + lng(rng() % 5));
        for (auto &q : f) { q = {q.x + c, q.y - c}; }
        melkman(t % 4 == 2 ? moved(f, c) : f, f.size() >= 3);}
    phase = "int coordinates";
    vector<Point2<int>> p{{0, 0}, {2, 0}, {0, 2}, {1, 1}, {0, 0}}, sorted = p;
    vector<Point2<int>> h{{0, 0}, {2, 0}, {0, 2}};
    sort(sorted.begin(), sorted.end());
    require(convexHull(p) == h && convexHullGraham(p) == h && convexHullSorted(sorted) == h && convexHullJarvis(p) == h, "32-bit coordinates");
    require(convexHullMelkman(vector<Point2<int>>{{0, 0}, {2, 0}, {1, 1}, {0, 2}}) == h, "32-bit Melkman");
    require(convexHullMelkman(vector<Point2<int>>{{0, 0}, {2, 0}, {1, 1}, {0, 2}}, true) == vector<Point2<int>>{{0, 0}, {2, 0}, {1, 1}, {0, 2}}, "32-bit Melkman retain");
    require(lowerHullIndices(p) == vector<int>{0, 1} && lowerHullIndices(p, true) == vector<int>{0, 1}, "32-bit lowerHullIndices");
    require(upperHullIndices(p) == vector<int>{0, 2, 1} && upperHullIndices(p, true) == vector<int>{0, 2, 3, 1}, "32-bit upperHullIndices");
    require(lowerHullIndices(vector<point>{}).empty() && upperHullIndices(vector<point>{}, true).empty(), "chain indices empty input");
    require(lowerHullIndices(vector<point>{{5, 5}, {1, 1}, {5, 5}, {1, 1}}) == vector<int>{1, 0} && upperHullIndices(vector<point>{{5, 5}, {1, 1}, {5, 5}}, true) == vector<int>{1, 0}, "duplicate segment chain indices");
    require(lowerHull(p) == vector<Point2<int>>{{0, 0}, {2, 0}} && upperHull(p) == vector<Point2<int>>{{0, 0}, {0, 2}, {2, 0}}, "32-bit chains");
    require(convexHullIndices(p) == vector<int>{0, 1, 2} && convexHullIndices(vector<point>{}).empty(), "32-bit indices and empty input");
    require(convexHullIndices(vector<point>{{5, 5}, {1, 1}, {5, 5}, {1, 1}}) == vector<int>{1, 0}, "duplicate segment indices");
    phase = "large parabola";
    int n = mode == "quick" ? 100 : mode == "full" ? 10000 : 30000;
    input.clear(); expected.clear();
    for (int x = -n; x <= n; ++x) { expected.push_back({x, lng(x) * x}); }
    vector<point> curve = expected, ends{expected.front(), expected.back()};
    input = expected; std::shuffle(input.begin(), input.end(), rng);
    for (bool retain : {false, true}) {
        actual = convexHull(input, retain); require(actual == expected, "Andrew large all-extreme");
        actual = convexHullGraham(input, retain); require(actual == expected, "Graham large all-extreme");
        actual = convexHullSorted(curve, retain); require(actual == expected, "Sorted large all-extreme");
        auto id = convexHullIndices(input, retain); require(int(id.size()) == int(expected.size()), "indices large all-extreme size");
        for (int i = 0; i < int(id.size()); ++i) { require(input[id[i]] == expected[i], "indices large all-extreme"); }
        exact(pick(input, lowerHullIndices(input, retain)), curve, "lowerHullIndices large all-extreme");
        exact(pick(input, upperHullIndices(input, retain)), ends, "upperHullIndices large chord"); expected = curve;
        actual = lowerHull(input, retain); require(actual == expected, "lowerHull large all-extreme");
        actual = lowerHullSorted(curve, retain); require(actual == expected, "lowerHullSorted large all-extreme");
        expected = ends; actual = upperHull(input, retain); require(actual == expected, "upperHull large chord");
        actual = upperHullSorted(curve, retain); require(actual == expected, "upperHullSorted large chord");
        expected = curve;}
    auto back = curve; reverse(back.begin(), back.end());
    for (bool retain : {false, true}) {
        checkMelkman(curve, curve, retain); checkMelkman(back, curve, retain);}
    std::rotate(back.begin(), back.begin() + n / 3, back.end());
    for (bool retain : {false, true}) { checkMelkman(back, curve, retain); }
    phase = "large spiral and random";
    auto coil = spiral(2 * n + 1); melkman(coil, false); melkman(thin(coil), false);
    for (bool retain : {false, true}) {
        expected = convexHull(coil, retain); input = coil;
        actual = convexHullJarvis(coil, retain); require(actual == expected && coil == input, "Jarvis large spiral");}
    input.clear(); for (int k = 0; k <= 2 * n; ++k) { input.push_back({lng(rng() % 2000000001) - 1000000000, lng(rng() % 2000000001) - 1000000000}); }
    for (bool retain : {false, true}) {
        expected = convexHull(input, retain); actual = convexHullJarvis(input, retain); require(actual == expected, "Jarvis large random");}
    phase = "large collinear";
    input.clear(); for (int x = -n; x <= n; ++x) { input.push_back({x, 3 * x}); }
    expected = input; std::shuffle(input.begin(), input.end(), rng);
    vector<point> line = expected;
    actual = convexHull(input, true); require(actual == expected, "Andrew large line retained");
    actual = convexHullGraham(input, true); require(actual == expected, "Graham large line retained");
    actual = convexHullSorted(line, true); require(actual == expected, "Sorted large line retained");
    actual = lowerHull(input, true); require(actual == expected, "lowerHull large line retained");
    actual = upperHull(input, true); require(actual == expected, "upperHull large line retained");
    actual = lowerHullSorted(line, true); require(actual == expected, "lowerHullSorted large line retained");
    actual = upperHullSorted(line, true); require(actual == expected, "upperHullSorted large line retained");
    vector<point> two{line.front(), line.back()};
    exact(pick(input, lowerHullIndices(input, true)), line, "lowerHullIndices large line retained");
    exact(pick(input, upperHullIndices(input, true)), line, "upperHullIndices large line retained");
    exact(pick(input, lowerHullIndices(input)), two, "lowerHullIndices large line endpoints");
    exact(pick(input, upperHullIndices(input)), two, "upperHullIndices large line endpoints");
    expected = two;
    actual = convexHull(input); require(actual == expected, "Andrew large line endpoints");
    actual = convexHullGraham(input); require(actual == expected, "Graham large line endpoints");
    actual = convexHullSorted(line); require(actual == expected, "Sorted large line endpoints");
    actual = convexHullJarvis(input); require(actual == expected, "Jarvis large line endpoints");
    actual = lowerHull(input); require(actual == expected, "lowerHull large line endpoints");
    actual = upperHull(input); require(actual == expected, "upperHull large line endpoints");
    auto all = line;
    for (int turn = 0; turn < 2; ++turn) {
        checkMelkman(line, two); checkMelkman(line, all, true);
        reverse(line.begin(), line.end());}
    std::cout << "PASS convexhull seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
