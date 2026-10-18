#include "../../03-Geometry/13-segment_union.hpp"

// Oracles: depths of the compressed-grid cells counted against every rectangle or segment; all pairs for orthogonal segments.
static ulng seed = 20260927;
static std::string phase;
static lng checks = 0;
static std::string context;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " input=" << context << " expected=true actual=false\n";
    std::exit(1);}
using Rect = array<lng, 4>;
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

struct Grid {
    vector<lng> xs, ys; vector<vector<int>> depth;
    explicit Grid(const vector<Rect> &a) {
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
int depthAt(const vector<Rect> &a, point p) {
    int d = 0;
    for (auto &r : a) { d += r[0] <= p.x && p.x < r[2] && r[1] <= p.y && p.y < r[3]; }
    return d;}
void rectangleCase(const vector<Rect> &a) {
    context = show(a);
    Grid g(a); int nx = int(g.depth.size()), ny = nx ? int(g.depth[0].size()) : 0;
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
        auto [ex, least] = rectangleCoverageArea(a, k);
        require(least == atLeast[k] && ex == atLeast[k] - atLeast[k + 1 < int(atLeast.size()) ? k + 1 : k] * (k + 1 < int(atLeast.size())), "rectangleCoverageArea equals the cell depths");}
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
    auto key = [](vector<array<point, 2>> v) {
        vector<array<lng, 4>> k;
        for (auto &x : v) { k.push_back({x[0].x, x[0].y, x[1].x, x[1].y}); }
        sort(k.begin(), k.end());
        return k;};
    require(key(rectangleUnionBoundaryGuting(a)) == key(b), "rectangleUnionBoundaryGuting returns the same segments");}

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
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    phase = "fixed";
    segmentCase({}); segmentCase({{3, 3}}); segmentCase({{5, 1}, {2, 8}, {10, 12}}); segmentCase({{0, 1}, {1, 2}});
    rectangleCase({}); rectangleCase({{0, 0, 0, 5}}); rectangleCase({{0, 0, 2, 2}});
    rectangleCase({{0, 0, 1, 1}, {1, 0, 2, 1}}); rectangleCase({{0, 0, 1, 1}, {1, 1, 2, 2}}); rectangleCase({{0, 0, 3, 3}, {1, 1, 2, 2}});
    rectangleCase({{0, 0, 3, 1}, {0, 2, 3, 3}, {0, 0, 1, 3}, {2, 0, 3, 3}});
    context = "picture";
    require(rectangleUnionPerimeter({{-15, 0, 5, 10}, {-5, 8, 20, 25}, {15, -4, 24, 14}, {0, -6, 16, 4}, {2, 15, 10, 22}, {30, 10, 36, 20}, {34, 0, 40, 16}}) == 228, "POJ 1177 sample perimeter");
    orthogonalCase({}, {}); orthogonalCase({{0, 0, 4}}, {{4, 0, 3}, {5, 0, 3}, {2, -1, -1}, {2, 0, 0}});
    int rounds = quick ? 300 : stress ? 40000 : 2000;
    phase = "segments random";
    for (int t = 0; t < rounds; ++t) {
        int n = int(rng() % 12); vector<pair<lng, lng>> s(n); lng c = t % 4 == 3 ? 4000000000000000000LL : 10;
        for (auto &x : s) { x = {lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}; }
        segmentCase(s);}
    phase = "rectangles small";
    for (int t = 0; t < rounds; ++t) { rectangleCase(randomRects(rng, t % 3 ? 5 : 12, int(rng() % 9))); }
    phase = "rectangles wide";
    for (int t = 0; t < rounds / 5; ++t) { rectangleCase(randomRects(rng, 4000000000000000000LL, int(rng() % 7))); }
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
#ifdef _GLIBCXX_DEBUG
    n = min(n, 4000); // Debug STL checks the partition on every lower_bound, which is linear; domains are unchanged.
#endif
#ifdef __SANITIZE_ADDRESS__
    n = min(n, 25000); // ASan quarantine keeps the O(n * log(n)) contour tree's reallocations alive; domains are unchanged.
#endif
    for (int i = 0; i < n; ++i) { a.push_back({lng(i), lng(i), lng(i) + 2, lng(i) + 2}); }
    context = "staircase";
    require(rectangleUnionArea(a) == 3 * lll(n) + 1 && rectangleUnionPerimeter(a) == 4 * lll(n) + 4, "staircase union");
    require(rectangleCoverageArea(a, 2).first == lll(n) - 1 && maxRectangleOverlap(a).first == 2, "staircase overlap");
    require(int(rectangleUnionBoundary(a).size()) == 4 * n && int(rectangleUnionBoundaryGuting(a).size()) == 4 * n, "staircase boundary segment count");
    // A lattice of g thin horizontal and g thin vertical bars: Theta(g^2) boundary segments.
    int bars = quick ? 60 : 400; vector<Rect> lattice;
    for (int i = 0; i < bars; ++i) { lattice.push_back({0, 4 * lng(i), 4 * lng(bars), 4 * lng(i) + 1}); lattice.push_back({4 * lng(i), 0, 4 * lng(i) + 1, 4 * lng(bars)}); }
    auto lb = rectangleUnionBoundary(lattice), lg = rectangleUnionBoundaryGuting(lattice);
    require(lb.size() == lg.size() && lg.size() >= size_t(4) * (bars - 1) * (bars - 1), "bar lattice boundary sizes agree");
    lll per = 0;
    for (auto &x : lg) { per += abs(lll(x[1].x) - x[0].x) + abs(lll(x[1].y) - x[0].y); }
    require(per == rectangleUnionPerimeter(lattice), "bar lattice boundary length equals the perimeter");
    // A g x g grid of crossing segments plus n disjoint short ones: g^2 reported pairs.
    vector<array<lng, 3>> h, v; int g = quick || n < 100000 ? 300 : 2000;
    for (int i = 0; i < g; ++i) { h.push_back({lng(i), 0, lng(g)}); v.push_back({lng(i), 0, lng(g)}); }
    for (int i = 0; i < n; ++i) { h.push_back({-1 - lng(i), 0, 1}); v.push_back({lng(g) + 1 + i, 0, 1}); }
    require(orthogonalIntersectionCount(h, v) == lng(g) * g && lng(orthogonalIntersections(h, v).size()) == lng(g) * g, "dense grid of segments");
    std::cout << "PASS segment_union seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
