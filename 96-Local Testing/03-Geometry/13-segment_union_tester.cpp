#include "../../03-Geometry/13-segment_union.hpp"

// Oracles: depths of the compressed-grid cells counted against every rectangle or segment; all pairs for orthogonal segments;
// for rings, the boundary pieces, the cell areas and the winding number of every cell centre.
static ulng seed = 20260927;
static std::string phase;
static lng checks = 0;
static std::string context;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " input=" << context << " expected=true actual=false\n";
    std::exit(1);}
using Rect = array<lng, 4>;
using RectF = array<long double, 4>;
std::string show(const vector<Rect> &a) {
    std::string s;
    for (auto &r : a) { s += "[" + std::to_string(r[0]) + "," + std::to_string(r[1]) + "," + std::to_string(r[2]) + "," + std::to_string(r[3]) + "]"; }
    return s;}

void segmentCase(const vector<pair<lng, lng>> &s) {
    context = "segments";
    for (auto [l, r] : s) { context += "(" + std::to_string(l) + "," + std::to_string(r) + ")"; }
    vector<lng> xs;
    for (auto [l, r] : s) { if (l != r) { xs.push_back(l); xs.push_back(r); } }
    sort(xs.begin(), xs.end()); xs.erase(unique(xs.begin(), xs.end()), xs.end());
    lll uni = 0; vector<lll> cov(s.size() + 1);
    for (int i = 0; i + 1 < int(xs.size()); ++i) {
        int d = 0;
        for (auto [l, r] : s) { d += min(l, r) <= xs[i] && xs[i + 1] <= max(l, r); }
        lll w = lll(xs[i + 1]) - xs[i];
        if (d) { uni += w; }
        cov[d] += w;}
    require(segmentUnionLength(s) == uni, "segmentUnionLength equals the cell union");
    require(coverageCount(s) == cov, "coverageCount equals the cell depths");
    vector<pair<long double, long double>> f; vector<pair<double, double>> g;
    for (auto [l, r] : s) { f.push_back({l / 2.L, r / 2.L}); g.push_back({static_cast<double>(l) / 2., static_cast<double>(r) / 2.}); }
    if (s.size() < 40 && xs.size() && abs(xs[0]) < (1LL << 40) && abs(xs.back()) < (1LL << 40)) {
        require(segmentUnionLength(f) == static_cast<long double>(uni) / 2, "long double endpoints are exact for halves");
        auto c = coverageCount(g);
        for (int k = 0; k <= int(s.size()); ++k) { require(c[k] == static_cast<long double>(cov[k]) / 2, "double coverageCount is exact for halves"); }}}

template<typename T> struct Grid {
    vector<T> xs, ys; vector<vector<int>> depth;
    explicit Grid(const vector<array<T, 4>> &a) {
        for (auto &r : a) {
            if (r[0] < r[2] && r[1] < r[3]) { xs.push_back(r[0]); xs.push_back(r[2]); ys.push_back(r[1]); ys.push_back(r[3]); }}
        sort(xs.begin(), xs.end()); xs.erase(unique(xs.begin(), xs.end()), xs.end());
        sort(ys.begin(), ys.end()); ys.erase(unique(ys.begin(), ys.end()), ys.end());
        int nx = max(0, int(xs.size()) - 1), ny = max(0, int(ys.size()) - 1);
        depth.assign(nx, vector<int>(ny));
        for (int i = 0; i < nx; ++i) {
            for (int j = 0; j < ny; ++j) {
                for (auto &r : a) { depth[i][j] += r[0] <= xs[i] && xs[i + 1] <= r[2] && r[1] <= ys[j] && ys[j + 1] <= r[3]; }}}}
    bool on(int i, int j) const { return i >= 0 && j >= 0 && i < int(depth.size()) && j < int(depth[0].size()) && depth[i][j] > 0; }
};
template<typename T> int depthAt(const vector<array<T, 4>> &a, Point2<T> p) {
    int d = 0;
    for (auto &r : a) { d += r[0] <= p.x && p.x < r[2] && r[1] <= p.y && p.y < r[3]; }
    return d;}
vector<array<lng, 4>> key(const vector<array<point, 2>> &v) {
    vector<array<lng, 4>> k;
    for (auto &x : v) { k.push_back({x[0].x, x[0].y, x[1].x, x[1].y}); }
    sort(k.begin(), k.end());
    return k;}
// Rings: their edges are the boundary pieces, signed areas sum to the union, every cell centre has winding number 1 inside and 0 outside,
// each ring starts at its smallest vertex, rings are in that order, consecutive edges turn, and a vertex shared by four pieces turns right.
void contourCase(const vector<Rect> &a, const Grid<lng> &g, const vector<array<point, 2>> &pieces, lll area) {
    auto rings = rectangleUnionContours(a); vector<array<point, 2>> edges; map<point, int> seen; ulll area2 = 0;
    for (int t = 0; t < int(rings.size()); ++t) {
        const auto &r = rings[t]; int n = int(r.size());
        require(n >= 4, "a ring has at least four corners");
        if (t) { require(rings[t - 1][0] < r[0], "rings are ordered by their first vertex"); }
        for (int i = 0; i < n; ++i) {
            point p = r[i], q = r[(i + 1) % n], s = r[(i + 2) % n];
            require(i == 0 || r[0] < p, "a ring starts at its smallest vertex");
            edges.push_back({p, q}); ++seen[q];
            area2 += ulll(lll(p.x - r[0].x) * (q.y - r[0].y) - lll(p.y - r[0].y) * (q.x - r[0].x));
            require(lll(q.x - p.x) * (s.y - q.y) - lll(q.y - p.y) * (s.x - q.x) != 0, "consecutive ring edges turn");}}
    require(key(edges) == key(pieces), "ring edges are the boundary pieces");
    require(lll(area2) == 2 * area, "ring areas sum to the union area");
    for (const auto &r : rings) {
        int n = int(r.size());
        for (int i = 0; i < n; ++i) {
            point p = r[(i + n - 1) % n], q = r[i], s = r[(i + 1) % n];
            require(seen[q] <= 2, "at most four pieces meet at a vertex");
            if (seen[q] == 2) { require(lll(q.x - p.x) * (s.y - q.y) - lll(q.y - p.y) * (s.x - q.x) < 0, "rings turn right where four pieces meet"); }}}
    int nx = int(g.depth.size()), ny = nx ? int(g.depth[0].size()) : 0;
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            lll cx = lll(g.xs[i]) + g.xs[i + 1], cy = lll(g.ys[j]) + g.ys[j + 1]; int w = 0;
            for (const auto &r : rings) {
                for (int k = 0; k < int(r.size()); ++k) {
                    point p = r[k], q = r[(k + 1) % r.size()];
                    if (p.x == q.x && 2 * lll(p.x) > cx && min(2 * lll(p.y), 2 * lll(q.y)) < cy && cy < max(2 * lll(p.y), 2 * lll(q.y))) { w += q.y > p.y ? 1 : -1; }}}
            require(w == (g.depth[i][j] > 0), "winding number of every cell centre is its coverage");}}}
// The same rectangles at half scale in long double (and double): every result is exactly half or a quarter of the integral one.
void halvesCase(const vector<Rect> &a, lll area, lll per, const vector<array<point, 2>> &pieces) {
    vector<RectF> f; vector<array<double, 4>> d;
    for (auto &r : a) { f.push_back({r[0] / 2.L, r[1] / 2.L, r[2] / 2.L, r[3] / 2.L}); d.push_back({static_cast<double>(r[0]) / 2, static_cast<double>(r[1]) / 2, static_cast<double>(r[2]) / 2, static_cast<double>(r[3]) / 2}); }
    require(rectangleUnionArea(f) == static_cast<long double>(area) / 4 && rectangleUnionPerimeter(f) == static_cast<long double>(per) / 2, "long double union at half scale");
    require(rectangleUnionArea(d) == rectangleUnionArea(f) && rectangleUnionPerimeter(d) == rectangleUnionPerimeter(f), "double equals long double");
    for (int k = 1; k <= int(a.size()) + 1; ++k) {
        auto [ex, least] = rectangleCoverageArea(a, k); auto [fe, fl] = rectangleCoverageArea(f, k);
        require(fe == static_cast<long double>(ex) / 4 && fl == static_cast<long double>(least) / 4, "long double coverage at half scale");
        require(rectangle_detail::coverageBlocks(f, k) == rectangle_detail::coverageTree(f, k), "long double coverage paths agree");}
    auto [c, w] = maxRectangleOverlap(a); auto [cf, wf] = maxRectangleOverlap(f);
    require(cf == c && wf.x == w.x / 2.L && wf.y == w.y / 2.L, "long double overlap at half scale");
    auto half = [](const vector<array<point, 2>> &v) {
        vector<array<long double, 4>> k;
        for (auto &x : v) { k.push_back({x[0].x / 2.L, x[0].y / 2.L, x[1].x / 2.L, x[1].y / 2.L}); }
        sort(k.begin(), k.end());
        return k;};
    auto keyF = [](const vector<array<Point2<long double>, 2>> &v) {
        vector<array<long double, 4>> k;
        for (auto &x : v) { k.push_back({x[0].x, x[0].y, x[1].x, x[1].y}); }
        sort(k.begin(), k.end());
        return k;};
    require(keyF(rectangleUnionBoundary(f)) == half(pieces) && keyF(rectangleUnionBoundaryGuting(f)) == half(pieces), "long double boundary at half scale");
    auto ri = rectangleUnionContours(a); auto rf = rectangleUnionContours(f); bool same = ri.size() == rf.size();
    for (int t = 0; same && t < int(ri.size()); ++t) {
        same = ri[t].size() == rf[t].size();
        for (int i = 0; same && i < int(ri[t].size()); ++i) { same = rf[t][i].x == ri[t][i].x / 2.L && rf[t][i].y == ri[t][i].y / 2.L; }}
    require(same, "long double rings at half scale");}
void rectangleCase(const vector<Rect> &a) {
    context = show(a);
    Grid<lng> g(a); int nx = int(g.depth.size()), ny = nx ? int(g.depth[0].size()) : 0;
    lll uni = 0, per = 0; int best = 0; vector<lll> atLeast(a.size() + 2);
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            lll w = (lll(g.xs[i + 1]) - g.xs[i]) * (lll(g.ys[j + 1]) - g.ys[j]); int d = g.depth[i][j];
            if (d) { uni += w; }
            for (int k = 1; k <= d; ++k) { atLeast[k] += w; }
            best = max(best, d);}}
    // Boundary pieces: {fixed coordinate, low end, high end, direction} on elementary grid intervals.
    vector<array<lll, 4>> want, got;
    for (int i = 0; i <= nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            bool l = g.on(i - 1, j), r = g.on(i, j);
            if (l != r) { per += lll(g.ys[j + 1]) - g.ys[j]; want.push_back({0, g.xs[i], g.ys[j], r ? -1 : 1}); }}}
    for (int j = 0; j <= ny; ++j) {
        for (int i = 0; i < nx; ++i) {
            bool d = g.on(i, j - 1), u = g.on(i, j);
            if (d != u) { per += lll(g.xs[i + 1]) - g.xs[i]; want.push_back({1, g.ys[j], g.xs[i], u ? 1 : -1}); }}}
    require(rectangleUnionArea(a) == uni, "rectangleUnionArea equals the cell union");
    require(rectangleUnionPerimeter(a) == per, "rectangleUnionPerimeter equals the cell boundary length");
    for (int k = 1; k <= int(a.size()) + 1; ++k) {
        lll next = k + 1 < int(atLeast.size()) ? atLeast[k + 1] : 0;
        pair<lll, lll> want2{atLeast[k] - next, atLeast[k]};
        require(rectangleCoverageArea(a, k) == want2, "rectangleCoverageArea equals the cell depths");
        require(rectangle_detail::coverageTree(a, k) == want2 && rectangle_detail::coverageBlocks(a, k) == want2, "both coverage paths equal the cell depths");}
    auto [cnt, w] = maxRectangleOverlap(a);
    require(cnt == best && depthAt(a, w) == best, "maxRectangleOverlap depth and witness");
    auto b = rectangleUnionBoundary(a);
    for (auto &s : b) {
        point p = s[0], q = s[1];
        require((p.x == q.x) != (p.y == q.y), "boundary segments are axis-parallel and nondegenerate");
        if (p.x == q.x) {
            int dir = q.y > p.y ? 1 : -1; lng lo = min(p.y, q.y), hi = max(p.y, q.y);
            int j = int(lower_bound(g.ys.begin(), g.ys.end(), lo) - g.ys.begin());
            require(j < int(g.ys.size()) && g.ys[j] == lo, "vertical piece starts on the grid");
            for (; j + 1 < int(g.ys.size()) && g.ys[j] < hi; ++j) { got.push_back({0, p.x, g.ys[j], dir}); }
            require(j < int(g.ys.size()) && g.ys[j] == hi, "vertical piece ends on the grid");}
        else {
            int dir = q.x > p.x ? 1 : -1; lng lo = min(p.x, q.x), hi = max(p.x, q.x);
            int i = int(lower_bound(g.xs.begin(), g.xs.end(), lo) - g.xs.begin());
            require(i < int(g.xs.size()) && g.xs[i] == lo, "horizontal piece starts on the grid");
            for (; i + 1 < int(g.xs.size()) && g.xs[i] < hi; ++i) { got.push_back({1, p.y, g.xs[i], dir}); }
            require(i < int(g.xs.size()) && g.xs[i] == hi, "horizontal piece ends on the grid");}}
    sort(want.begin(), want.end()); sort(got.begin(), got.end());
    require(want == got, "rectangleUnionBoundary pieces and directions equal the cell boundary");
    for (auto &s : b) { for (auto &t : b) { require(!(s[1] == t[0] && orient(s[0], s[1], t[1]) == 0 && dot(s[1] - s[0], t[1] - t[0]) > 0), "boundary segments are maximal"); } }
    require(key(rectangleUnionBoundaryGuting(a)) == key(b), "rectangleUnionBoundaryGuting returns the same segments");
    contourCase(a, g, b, uni);
    bool small = true;
    // Below 2^30 every quarter-unit area and partial sum stays under 2^62, exact in a 64-bit mantissa.
    for (auto &r : a) { for (lng v : r) { small = small && abs(v) < (1LL << 30); } }
    if (small) { halvesCase(a, uni, per, b); }}

// Floating rectangles with arbitrary real coordinates against the long double grid: exact structure, rounded sums.
void realCase(const vector<RectF> &a) {
    context = "real rectangles";
    Grid<long double> g(a); int nx = int(g.depth.size()), ny = nx ? int(g.depth[0].size()) : 0;
    long double uni = 0; vector<long double> atLeast(a.size() + 2); int best = 0;
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            long double w = (g.xs[i + 1] - g.xs[i]) * (g.ys[j + 1] - g.ys[j]); int d = g.depth[i][j];
            if (d) { uni += w; }
            for (int k = 1; k <= d; ++k) { atLeast[k] += w; }
            best = max(best, d);}}
    auto near = [](long double x, long double y) { return std::abs(x - y) <= 1e-15L * max(1.L, std::abs(y)); };
    require(near(rectangleUnionArea(a), uni), "real union area");
    for (int k = 1; k <= int(a.size()) + 1; ++k) {
        long double next = k + 1 < int(atLeast.size()) ? atLeast[k + 1] : 0;
        auto [ex, least] = rectangleCoverageArea(a, k); auto [be, bl] = rectangle_detail::coverageBlocks(a, k);
        require(near(least, atLeast[k]) && near(ex, atLeast[k] - next) && near(bl, atLeast[k]) && near(be, atLeast[k] - next), "real coverage area");}
    auto [cnt, w] = maxRectangleOverlap(a);
    require(cnt == best && depthAt(a, w) == best, "real overlap depth and witness");
    long double area = 0, length = 0;
    for (const auto &r : rectangleUnionContours(a)) {
        for (int i = 0; i < int(r.size()); ++i) {
            auto p = r[i], q = r[(i + 1) % r.size()];
            area += p.x * q.y - p.y * q.x; length += std::abs(q.x - p.x) + std::abs(q.y - p.y);}}
    require(near(area / 2, uni) && near(length, rectangleUnionPerimeter(a)), "real rings enclose the union");
    // The perimeter from the cells: every side of a covered cell whose neighbour is uncovered.
    auto in = [&](int i, int j) { return 0 <= i && i < nx && 0 <= j && j < ny && g.depth[i][j] > 0; };
    long double per = 0, pieces = 0;
    for (int i = 0; i < nx; ++i) {
        for (int j = 0; j < ny; ++j) {
            if (!in(i, j)) { continue; }
            per += (!in(i - 1, j) + !in(i + 1, j)) * (g.ys[j + 1] - g.ys[j]) + (!in(i, j - 1) + !in(i, j + 1)) * (g.xs[i + 1] - g.xs[i]);}}
    auto b = rectangleUnionBoundary(a), c = rectangleUnionBoundaryGuting(a);
    for (const auto &s : b) { pieces += std::abs(s[1].x - s[0].x) + std::abs(s[1].y - s[0].y); }
    auto flat = [](const vector<array<Point2<long double>, 2>> &v) {
        vector<array<long double, 4>> k;
        for (const auto &s : v) { k.push_back({s[0].x, s[0].y, s[1].x, s[1].y}); }
        sort(k.begin(), k.end());
        return k;};
    require(near(rectangleUnionPerimeter(a), per) && near(pieces, per), "real perimeter and boundary pieces equal the cell boundary");
    require(flat(b) == flat(c), "real rectangleUnionBoundaryGuting returns the sweep's segments");}
// Real orthogonal segments from a small pool of coordinates, so that touching happens: all pairs, compared exactly.
void realOrthogonalCase(const vector<array<long double, 3>> &h, const vector<array<long double, 3>> &v) {
    context = "real orthogonal";
    vector<pair<int, int>> want;
    for (int i = 0; i < int(h.size()); ++i) {
        for (int j = 0; j < int(v.size()); ++j) {
            auto [y, a, b] = h[i]; auto [x, c, d] = v[j];
            if (min(a, b) <= x && x <= max(a, b) && min(c, d) <= y && y <= max(c, d)) { want.push_back({i, j}); }}}
    auto got = orthogonalIntersections(h, v);
    sort(got.begin(), got.end());
    require(orthogonalIntersectionCount(h, v) == lng(want.size()) && got == want, "real orthogonal segments equal all pairs");}

// Coverage on hundreds of rectangles, where the blocks hold several intervals: a plain depth array updated interval by interval.
void coverageCase(const vector<Rect> &a, const vector<int> &ks) {
    context = "coverage of " + std::to_string(a.size()) + " rectangles";
    vector<lng> ys; vector<array<lng, 4>> e;
    for (auto &r : a) {
        if (r[0] < r[2] && r[1] < r[3]) { ys.push_back(r[1]); ys.push_back(r[3]); e.push_back({r[0], 1, r[1], r[3]}); e.push_back({r[2], -1, r[1], r[3]}); }}
    sort(ys.begin(), ys.end()); ys.erase(unique(ys.begin(), ys.end()), ys.end()); sort(e.begin(), e.end());
    vector<int> depth(ys.size()); vector<pair<lll, lll>> want(ks.size());
    for (int i = 0, j = 0; i < int(e.size()); i = j) {
        for (; j < int(e.size()) && e[j][0] == e[i][0]; ++j) {
            for (int y = int(lower_bound(ys.begin(), ys.end(), e[j][2]) - ys.begin()); ys[y] < e[j][3]; ++y) { depth[y] += int(e[j][1]); }}
        if (j == int(e.size())) { break; }
        lll dx = lll(e[j][0]) - e[i][0];
        for (int t = 0; t < int(ks.size()); ++t) {
            for (int y = 0; y + 1 < int(ys.size()); ++y) {
                lll w = (lll(ys[y + 1]) - ys[y]) * dx;
                if (depth[y] == ks[t]) { want[t].first += w; }
                if (depth[y] >= ks[t]) { want[t].second += w; }}}}
    for (int t = 0; t < int(ks.size()); ++t) {
        require(rectangleCoverageArea(a, ks[t]) == want[t] && rectangle_detail::coverageBlocks(a, ks[t]) == want[t], "coverage equals the depth array");
        if (ks[t] <= 40) { require(rectangle_detail::coverageTree(a, ks[t]) == want[t], "coverage tree equals the depth array"); }}}

void orthogonalCase(const vector<array<lng, 3>> &h, const vector<array<lng, 3>> &v) {
    context = "orthogonal";
    vector<pair<int, int>> want;
    for (int i = 0; i < int(h.size()); ++i) {
        for (int j = 0; j < int(v.size()); ++j) {
            auto [y, a, b] = h[i]; auto [x, c, d] = v[j];
            if (min(a, b) <= x && x <= max(a, b) && min(c, d) <= y && y <= max(c, d)) { want.push_back({i, j}); }}}
    auto got = orthogonalIntersections(h, v);
    require(orthogonalIntersectionCount(h, v) == lng(want.size()), "orthogonalIntersectionCount equals all pairs");
    vector<char> closed(v.size());
    for (int t = 1; t < int(got.size()); ++t) {
        auto a = got[t - 1], b = got[t];
        if (a.second != b.second) { closed[a.second] = 1; }
        bool order = a.second == b.second ? h[a.first][0] <= h[b.first][0] : v[a.second][0] <= v[b.second][0] && !closed[b.second];
        require(order, "orthogonalIntersections grouped by vertical in x order, then horizontal y");}
    vector<array<long double, 3>> hf, vf;
    for (auto &s : h) { hf.push_back({s[0] / 2.L, s[1] / 2.L, s[2] / 2.L}); }
    for (auto &s : v) { vf.push_back({s[0] / 2.L, s[1] / 2.L, s[2] / 2.L}); }
    if (h.size() + v.size() && std::abs(static_cast<long double>(h.size() ? h[0][0] : v[0][0])) < (1LL << 40)) {
        require(orthogonalIntersections(hf, vf) == got && orthogonalIntersectionCount(hf, vf) == lng(got.size()), "long double segments at half scale");}
    sort(got.begin(), got.end());
    require(got == want, "orthogonalIntersections equals all pairs");}

vector<Rect> randomRects(std::mt19937_64 &rng, lng c, int n) {
    vector<Rect> a(n);
    for (auto &r : a) {
        lng x1 = lng(rng() % ulng(2 * c + 1)) - c, x2 = lng(rng() % ulng(2 * c + 1)) - c, y1 = lng(rng() % ulng(2 * c + 1)) - c, y2 = lng(rng() % ulng(2 * c + 1)) - c;
        r = {min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)};
        if (rng() % 10 == 0) { r[2] = r[0]; }}
    return a;}
void invalid(const string &name) {
    if (name == "coordinate-bound") { (void)rectangleUnionArea({Rect{0, 0, 5000000000000000000LL, 1}}); }
    if (name == "coverage-k") { (void)rectangleCoverageArea({Rect{0, 0, 1, 1}}, 0); }
    if (name == "nan-rectangle") { (void)rectangleUnionArea(vector<RectF>{{0, 0, std::numeric_limits<long double>::quiet_NaN(), 1}}); }
    if (name == "nonfinite-segment") { (void)orthogonalIntersectionCount(vector<array<double, 3>>{{0, 0, std::numeric_limits<double>::infinity()}}, {}); }
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    bool quick = mode == "quick", stress = mode == "stress", checked = false, asan = false;
#ifdef _GLIBCXX_DEBUG
    checked = true; // Debug STL checks every lower_bound partition in linear time: fewer and smaller cases in the heavy phases; domains are unchanged.
#endif
    std::mt19937_64 rng(seed);
    phase = "fixed";
    segmentCase({}); segmentCase({{3, 3}}); segmentCase({{5, 1}, {2, 8}, {10, 12}}); segmentCase({{0, 1}, {1, 2}});
    rectangleCase({}); rectangleCase({{0, 0, 0, 5}}); rectangleCase({{0, 0, 2, 2}});
    rectangleCase({{0, 0, 1, 1}, {1, 0, 2, 1}}); rectangleCase({{0, 0, 1, 1}, {1, 1, 2, 2}}); rectangleCase({{0, 0, 3, 3}, {1, 1, 2, 2}});
    rectangleCase({{0, 0, 3, 1}, {0, 2, 3, 3}, {0, 0, 1, 3}, {2, 0, 3, 3}});
    context = "picture";
    require(rectangleUnionPerimeter({{-15, 0, 5, 10}, {-5, 8, 20, 25}, {15, -4, 24, 14}, {0, -6, 16, 4}, {2, 15, 10, 22}, {30, 10, 36, 20}, {34, 0, 40, 16}}) == 228, "POJ 1177 sample perimeter");
    // Pinch conventions: corner-touching squares form one ring through the corner twice; a hole touching the outside at a corner is its own ring.
    context = "corner-touching squares";
    auto pinch = rectangleUnionContours({{0, 0, 1, 1}, {1, 1, 2, 2}});
    require(pinch.size() == 1 && pinch[0] == vector<point>{{0, 0}, {1, 0}, {1, 1}, {2, 1}, {2, 2}, {1, 2}, {1, 1}, {0, 1}}, "one ring visits the shared corner twice");
    context = "hole touching the outside";
    auto hole = rectangleUnionContours({{0, 0, 1, 1}, {1, 1, 3, 2}, {2, -1, 3, 1}, {0, -1, 3, 0}});
    require(hole == vector<vector<point>>{{{0, -1}, {3, -1}, {3, 2}, {1, 2}, {1, 1}, {0, 1}}, {{1, 0}, {1, 1}, {2, 1}, {2, 0}}}, "the hole is a separate clockwise ring");
    context = "hole starting at a shared corner";
    auto corner = rectangleUnionContours({{0, 1, 1, 3}, {1, 0, 3, 1}, {2, 1, 3, 3}, {1, 2, 2, 3}});
    require(corner == vector<vector<point>>{{{0, 1}, {1, 1}, {1, 0}, {3, 0}, {3, 3}, {0, 3}}, {{1, 1}, {1, 2}, {2, 2}, {2, 1}}}, "a hole ring may start at a corner shared with the outer ring");
    context = "frame";
    auto frame = rectangleUnionContours({{0, 0, 3, 1}, {0, 2, 3, 3}, {0, 0, 1, 3}, {2, 0, 3, 3}});
    require(frame == vector<vector<point>>{{{0, 0}, {3, 0}, {3, 3}, {0, 3}}, {{1, 1}, {1, 2}, {2, 2}, {2, 1}}}, "frame: outer counterclockwise, hole clockwise");
    orthogonalCase({}, {}); orthogonalCase({{0, 0, 4}}, {{4, 0, 3}, {5, 0, 3}, {2, -1, -1}, {2, 0, 0}});
    int rounds = quick ? 300 : stress ? 40000 : 2000;
    phase = "rectangles exhaustive";
    vector<Rect> cells;
    for (lng x1 = 0; x1 < 3; ++x1) { for (lng x2 = x1 + 1; x2 <= 3; ++x2) { for (lng y1 = 0; y1 < 3; ++y1) { for (lng y2 = y1 + 1; y2 <= 3; ++y2) { cells.push_back({x1, y1, x2, y2}); } } } }
    for (int i = 0; i < int(cells.size()); ++i) {
        for (int j = i; j < int(cells.size()); ++j) {
            rectangleCase({cells[i], cells[j]});
            for (int k = j; k < int(cells.size()) && !quick && !checked; ++k) { rectangleCase({cells[i], cells[j], cells[k]}); }}}
    phase = "segments random";
    for (int t = 0; t < rounds; ++t) {
        int n = int(rng() % 12); vector<pair<lng, lng>> s(n); lng c = t % 4 == 3 ? 4000000000000000000LL : 10;
        for (auto &x : s) { x = {lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}; }
        segmentCase(s);}
    phase = "rectangles small";
    for (int t = 0; t < rounds; ++t) { rectangleCase(randomRects(rng, t % 3 ? 5 : 12, int(rng() % 9))); }
    phase = "rectangles deep";
    for (int t = 0; t < (checked ? 50 : rounds / 10); ++t) { rectangleCase(randomRects(rng, t % 2 ? 4 : 20, 13 + int(rng() % 28))); }
    phase = "coverage blocks";
    for (int t = 0; t < (checked ? 5 : rounds / 20); ++t) {
        int n = 64 + int(rng() % (checked ? 137 : 537)); lng c = 1000000, side = t % 2 ? c : 3 * c / 10; vector<Rect> a(n);
        for (auto &r : a) {
            lng x = lng(rng() % ulng(c)), y = lng(rng() % ulng(c));
            r = {x, y, x + 1 + lng(rng() % ulng(side)), y + 1 + lng(rng() % ulng(side))};}
        vector<int> ks = {1, 2, 3, 5, 16, 17, 18, 1 + int(rng() % ulng(n)), n};
        coverageCase(a, ks);}
    phase = "rectangles wide";
    for (int t = 0; t < rounds / 5; ++t) { rectangleCase(randomRects(rng, 4000000000000000000LL, int(rng() % 7))); }
    phase = "rectangles real";
    for (int t = 0; t < rounds / 5; ++t) {
        vector<RectF> a(rng() % 24); std::uniform_real_distribution<long double> u(-1, 1);
        for (auto &r : a) { long double x1 = u(rng), x2 = u(rng), y1 = u(rng), y2 = u(rng); r = {min(x1, x2), min(y1, y2), max(x1, x2), max(y1, y2)}; }
        realCase(a);
        vector<long double> pool(6);
        for (auto &x : pool) { x = u(rng); }
        vector<array<long double, 3>> h(rng() % 10), v(rng() % 10);
        for (auto &s : h) { s = {pool[rng() % 6], pool[rng() % 6], pool[rng() % 6]}; }
        for (auto &s : v) { s = {pool[rng() % 6], pool[rng() % 6], pool[rng() % 6]}; }
        realOrthogonalCase(h, v);}
    phase = "orthogonal random";
    for (int t = 0; t < rounds; ++t) {
        int n = int(rng() % 10), m = int(rng() % 10); lng c = t % 5 == 4 ? 4000000000000000000LL : 6;
        auto r = [&]() { return lng(rng() % ulng(2 * c + 1)) - c; };
        vector<array<lng, 3>> h(n), v(m);
        for (auto &x : h) { x = {r(), r(), r()}; }
        for (auto &x : v) { x = {r(), r(), r()}; }
        orthogonalCase(h, v);}
    phase = "large";
    int n = quick ? 2000 : stress ? 400000 : 100000; vector<Rect> a;
    if (checked) { n = min(n, 1500); }
#ifdef __SANITIZE_ADDRESS__
    asan = true; // ASan redzones and quarantine multiply the contour trees' memory: smaller large cases; domains are unchanged.
#endif
    if (asan) { n = min(n, 25000); }
    for (int i = 0; i < n; ++i) { a.push_back({lng(i), lng(i), lng(i) + 2, lng(i) + 2}); }
    context = "staircase";
    require(rectangleUnionArea(a) == 3 * lll(n) + 1 && rectangleUnionPerimeter(a) == 4 * lll(n) + 4, "staircase union");
    require(rectangleCoverageArea(a, 2).first == lll(n) - 1 && maxRectangleOverlap(a).first == 2, "staircase overlap");
    require(rectangle_detail::coverageBlocks(a, 2).first == lll(n) - 1 && rectangleCoverageArea(a, 17) == pair<lll, lll>{0, 0}, "staircase blocks");
    require(int(rectangleUnionBoundary(a).size()) == 4 * n && int(rectangleUnionBoundaryGuting(a).size()) == 4 * n, "staircase boundary segment count");
    auto stair = rectangleUnionContours(a);
    require(stair.size() == 1 && int(stair[0].size()) == 4 * n && stair[0][0] == point{0, 0}, "staircase is one ring");
    // n nested squares [i, 2n - i)^2: area at least k is (2 (n - k + 1))^2, through both coverage paths.
    context = "nested squares"; vector<Rect> nest; int depth = min(n, checked ? 1000 : asan ? 5000 : 20000);
    for (int i = 0; i < depth; ++i) { nest.push_back({lng(i), lng(i), 2 * lng(depth) - i, 2 * lng(depth) - i}); }
    for (int k : {1, 2, 16, 17, 100, depth / 2, depth, depth + 1}) {
        auto sq = [&](lng j) { return j > depth ? lll(0) : 4 * lll(depth - j + 1) * (depth - j + 1); };
        pair<lll, lll> want{sq(k) - sq(k + 1), sq(k)};
        require(rectangleCoverageArea(nest, k) == want && rectangle_detail::coverageBlocks(nest, k) == want, "nested squares coverage");
        if (k <= 64) { require(rectangle_detail::coverageTree(nest, k) == want, "nested squares coverage tree"); }}
    require(maxRectangleOverlap(nest).first == depth && rectangleUnionContours(nest).size() == 1, "nested squares depth and ring");
    // A lattice of g thin horizontal and g thin vertical bars: Theta(g^2) boundary segments and (g - 1)^2 holes.
    int bars = quick ? 60 : checked ? 100 : asan ? 200 : 400; vector<Rect> lattice;
    for (int i = 0; i < bars; ++i) { lattice.push_back({0, 4 * lng(i), 4 * lng(bars), 4 * lng(i) + 1}); lattice.push_back({4 * lng(i), 0, 4 * lng(i) + 1, 4 * lng(bars)}); }
    auto lb = rectangleUnionBoundary(lattice), lg = rectangleUnionBoundaryGuting(lattice);
    require(lb.size() == lg.size() && lg.size() >= size_t(4) * (bars - 1) * (bars - 1), "bar lattice boundary sizes agree");
    lll per = 0;
    for (auto &x : lg) { per += abs(lll(x[1].x) - x[0].x) + abs(lll(x[1].y) - x[0].y); }
    require(per == rectangleUnionPerimeter(lattice), "bar lattice boundary length equals the perimeter");
    auto lr = rectangleUnionContours(lattice); size_t corners = 0;
    for (auto &r : lr) { corners += r.size(); }
    require(lr.size() == size_t(bars - 1) * (bars - 1) + 1 && corners == lg.size(), "bar lattice rings: one outer and (g - 1)^2 holes");
    // A g x g grid of crossing segments plus n disjoint short ones: g^2 reported pairs.
    vector<array<lng, 3>> h, v; int g = quick || n < 100000 ? 300 : 2000;
    for (int i = 0; i < g; ++i) { h.push_back({lng(i), 0, lng(g)}); v.push_back({lng(i), 0, lng(g)}); }
    for (int i = 0; i < n; ++i) { h.push_back({-1 - lng(i), 0, 1}); v.push_back({lng(g) + 1 + i, 0, 1}); }
    require(orthogonalIntersectionCount(h, v) == lng(g) * g && lng(orthogonalIntersections(h, v).size()) == lng(g) * g, "dense grid of segments");
    std::cout << "PASS segment_union seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
