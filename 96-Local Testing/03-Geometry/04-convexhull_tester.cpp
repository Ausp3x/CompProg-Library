#include "../../03-Geometry/04-convexhull.hpp"

static ulng seed = 20260927;
static vector<point> input, expected, actual;
static string phase;
static int checks = 0;
void dump(const char *name, const vector<point> &p) {
    std::cerr << name << '='; for (auto q : p) { std::cerr << '(' << q.x << ',' << q.y << ')'; } std::cerr << ' '; }
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << ' ';
    dump("input", input); dump("expected", expected); dump("actual", actual); std::cerr << '\n'; std::exit(1); }
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
                    edge.push_back(c); }}
            sort(edge.begin(), edge.end(), [a](point b, point c) { return square(a, b) < square(a, c); });
            hull.insert(hull.end(), edge.begin(), edge.end()); }
        else { hull.push_back(a); }
        a = b;
    } while (a != p[0]);
    return hull; }
void verify(const vector<point> &p) {
    input = p;
    for (bool retain : {false, true}) {
        expected = oracle(p, retain);
        actual = convexHull(p, retain); require(actual == expected, retain ? "Andrew retain" : "Andrew strict");
        actual = convexHullGraham(p, retain); require(actual == expected, retain ? "Graham retain" : "Graham strict");
        if (!actual.empty()) { require(actual.front() == *std::min_element(p.begin(), p.end()), "canonical minimum"); }
        auto copy = actual; sort(copy.begin(), copy.end()); require(std::adjacent_find(copy.begin(), copy.end()) == copy.end(), "duplicates removed");
        if (actual.size() >= 3 && det(actual[0], actual[1], actual.back()) != 0) {
            lll area = 0; for (int i = 0; i < int(actual.size()); ++i) {
                point a = actual[i], b = actual[(i + 1) % actual.size()]; area += lll(a.x) * b.y - lll(a.y) * b.x;
                for (point q : p) { require(det(a, b, q) >= 0, "supporting CCW edge"); }}
            require(area > 0, "positive orientation"); }}
}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }}
    std::mt19937_64 rng(seed);
    phase = "regressions";
    for (const vector<point> &p : vector<vector<point>>{
        {}, {{0, 0}}, {{0, 0}, {0, 0}}, {{3, 4}, {0, 0}}, {{0, -3}, {0, 2}, {0, 1}, {0, -3}},
        {{-2, 2}, {0, 0}, {1, -1}, {-1, 1}}, {{0, 0}, {1, -2}, {1, 2}, {2, 0}, {1, 0}},
        {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}},
        {{-1000000000, -1000000000}, {1000000000, -1000000000}, {1000000000, 1000000000}, {-1000000000, 1000000000}, {0, 0}}}) { verify(p); }
    phase = "all 3x3 subsets";
    int masks = mode == "quick" ? 64 : 512;
    for (int mask = 0; mask < masks; ++mask) {
        vector<point> p;
        for (int k = 0; k < 9; ++k) { if (mask >> k & 1) { p.push_back({k % 3 - 1, k / 3 - 1}); }}
        if (!p.empty() && mask % 3 == 0) { p.push_back(p[0]); }
        std::shuffle(p.begin(), p.end(), rng); verify(p); }
    phase = "random grid and extremes";
    int rounds = mode == "quick" ? 25 : mode == "full" ? 1000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(rng() % 45); lng c = t % 3 ? 12 : 1000000000;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);
        for (auto &q : p) { q = {-q.y, q.x}; } verify(p); }
    phase = "int coordinates";
    vector<Point2<int>> p{{0, 0}, {2, 0}, {0, 2}, {1, 1}, {0, 0}};
    vector<Point2<int>> h{{0, 0}, {2, 0}, {0, 2}};
    require(convexHull(p) == h && convexHullGraham(p) == h, "32-bit coordinates");
    phase = "large parabola";
    int n = mode == "quick" ? 100 : mode == "full" ? 10000 : 30000;
    input.clear(); expected.clear();
    for (int x = -n; x <= n; ++x) { expected.push_back({x, lng(x) * x}); }
    input = expected; std::shuffle(input.begin(), input.end(), rng);
    for (bool retain : {false, true}) {
        actual = convexHull(input, retain); require(actual == expected, "Andrew large all-extreme");
        actual = convexHullGraham(input, retain); require(actual == expected, "Graham large all-extreme"); }
    phase = "large collinear";
    input.clear(); for (int x = -n; x <= n; ++x) { input.push_back({x, 3 * x}); }
    expected = input; std::shuffle(input.begin(), input.end(), rng);
    actual = convexHull(input, true); require(actual == expected, "Andrew large line retained");
    actual = convexHullGraham(input, true); require(actual == expected, "Graham large line retained");
    expected = {expected.front(), expected.back()};
    actual = convexHull(input); require(actual == expected, "Andrew large line endpoints");
    actual = convexHullGraham(input); require(actual == expected, "Graham large line endpoints");
    std::cout << "PASS convexhull seed=" << seed << " mode=" << mode << " checks=" << checks << '\n'; }
