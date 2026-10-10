#include "../../03-Geometry/11-minkowskisum.hpp"

// Independent oracle: the convex hull of every pairwise vertex sum (or difference); the hull itself is
// cross-checked against a supporting-line brute force on small sets.
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
template<typename T> vector<Point2<T>> sums(const vector<Point2<T>> &p, const vector<Point2<T>> &q, int sign = 1) {
    vector<Point2<T>> s;
    for (auto a : p) { for (auto b : q) { s.push_back(sign > 0 ? a + b : a - b); } }
    return s;}
// Supporting-line oracle for the strict hull of a small set: x is a hull vertex iff some edge x -> y keeps every point left-or-on and none beyond y.
bool hullVertex(const vector<point> &s, point x) {
    for (auto y : s) {
        if (y == x) { continue; }
        bool ok = true;
        for (auto z : s) { ok = ok && (orient(x, y, z) > 0 || (orient(x, y, z) == 0 && dot(z - x, y - x) >= 0 && dot(z - y, x - y) >= 0) || z == x); }
        if (ok) { return true; }}
    return s.size() == 1 || (s.size() > 1 && std::all_of(s.begin(), s.end(), [&](point z) { return z == x; }));}
void checkHull(const vector<point> &s, const vector<point> &h) {
    auto t = s; sort(t.begin(), t.end()); t.erase(unique(t.begin(), t.end()), t.end());
    if (t.size() > 40) { return; }
    int cnt = 0;
    for (auto x : t) { cnt += hullVertex(t, x); }
    if (t.size() == 1) { cnt = 1; }
    require(cnt == int(h.size()) || (t.size() >= 2 && h.size() == 2), "oracle hull validated by supporting lines");
    for (auto x : h) { require(t.size() <= 2 || hullVertex(t, x), "oracle hull vertex supports a line"); }}
void pairCase(vector<point> p, vector<point> q, std::mt19937_64 &rng) {
    lhs = p; rhs = q;
    std::rotate(p.begin(), p.begin() + (p.empty() ? 0 : rng() % p.size()), p.end());
    std::rotate(q.begin(), q.begin() + (q.empty() ? 0 : rng() % q.size()), q.end());
    auto want = convexHull(sums(p, q)), got = minkowskiSum(p, q);
    if (p.empty() || q.empty()) { want.clear(); }
    require(got == want, "minkowskiSum equals the hull of all vertex sums");
    checkHull(sums(p, q), want);
    auto diff = convexHull(sums(p, q, -1)); if (p.empty() || q.empty()) { diff.clear(); }
    require(minkowskiDifference(p, q) == diff, "minkowskiDifference equals the hull of all differences");
    require(minkowskiSum(q, p) == got, "sum is commutative");
    vector<vector<point>> fam{p, q};
    require(minkowskiSum(fam) == got, "two-polygon family equals the pairwise sum");}
vector<point> randomPoints(std::mt19937_64 &rng, lng c, int n) {
    vector<point> p;
    for (int i = 0; i < n; ++i) { p.push_back({lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}); }
    return p;}
void fixed(std::mt19937_64 &rng) {
    phase = "degenerate";
    vector<point> e, a{{1, 2}}, s{{0, 0}, {3, 1}}, t{{0, 0}, {6, 2}}, u{{0, 0}, {0, 5}}, sq{{0, 0}, {2, 0}, {2, 2}, {0, 2}};
    for (auto &x : {e, a, s, t, u, sq}) { for (auto &y : {e, a, s, t, u, sq}) { pairCase(x, y, rng); } }
    lhs = s; rhs = t;
    require(minkowskiSum(s, t) == vector<point>{{0, 0}, {9, 3}}, "collinear segments sum to a segment");
    require(minkowskiSum(a, sq) == vector<point>{{1, 2}, {3, 2}, {3, 4}, {1, 4}}, "point translates");
    require(minkowskiSum(s, u) == vector<point>{{0, 0}, {3, 1}, {3, 6}, {0, 5}}, "segments sum to a parallelogram");
    vector<vector<point>> none;
    require(minkowskiSum(none) == vector<point>{{0, 0}}, "empty family is the origin");
    require(minkowskiSum(vector<vector<point>>{sq, e}).empty(), "empty member empties the family");
    require(minkowskiSumHull(vector<point>{}, sq).empty(), "hull of empty sum");
    phase = "wide";
    const lng C = 1000000000000000000;
    vector<point> big{{-C, -C}, {C, -C}, {C, C}, {-C, C}}, tri{{-C, 0}, {C, -C}, {0, C}};
    pairCase(big, tri, rng);
    require(minkowskiDifference(big, big) == vector<point>{{-2 * C, -2 * C}, {2 * C, -2 * C}, {2 * C, 2 * C}, {-2 * C, 2 * C}}, "wide difference");
    phase = "int coordinates";
    vector<Point2<int>> pi{{0, 0}, {4, 0}, {0, 4}}, qi{{1, 1}, {2, 1}, {2, 2}, {1, 2}};
    auto r = minkowskiSum(pi, qi), w = convexHull(sums(pi, qi));
    require(r == w, "int sum");}
void invalid(const string &name) {
    if (name == "clockwise") { (void)minkowskiSum(vector<point>{{0, 0}, {0, 1}, {1, 0}}, vector<point>{{0, 0}}); }
    if (name == "collinear-vertex") { (void)minkowskiSum(vector<point>{{0, 0}, {1, 0}, {2, 0}, {1, 1}}, vector<point>{{0, 0}}); }
    if (name == "repeated-pair") { (void)minkowskiSum(vector<point>{{1, 1}, {1, 1}}, vector<point>{{0, 0}}); }
    if (name == "family-nonconvex") { (void)minkowskiSum(vector<vector<point>>{{{0, 0}, {2, 0}, {1, 1}, {2, 2}, {0, 2}}}); }
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
    phase = "exhaustive 3x3 hulls";
    vector<point> g;
    for (int x = 0; x < 3; ++x) { for (int y = 0; y < 3; ++y) { g.push_back({x, y}); } }
    vector<vector<point>> hulls;
    for (int mask = 1; mask < 512; ++mask) {
        vector<point> s;
        for (int b = 0; b < 9; ++b) { if (mask >> b & 1) { s.push_back(g[b]); } }
        auto h = convexHull(s);
        if (std::find(hulls.begin(), hulls.end(), h) == hulls.end()) { hulls.push_back(h); }}
    int step = quick ? 5 : 1;
    for (int i = 0; i < int(hulls.size()); i += step) { for (int j = 0; j < int(hulls.size()); j += step) { pairCase(hulls[i], hulls[j], rng); } }
    phase = "random hulls";
    int rounds = quick ? 300 : stress ? 60000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        lng c = t % 3 == 0 ? 5 : t % 3 == 1 ? 1000 : 1000000000000000000;
        auto p = convexHull(randomPoints(rng, c, 1 + int(rng() % 30))), q = convexHull(randomPoints(rng, c, 1 + int(rng() % 30)));
        pairCase(p, q, rng);}
    phase = "families";
    for (int t = 0; t < rounds / 4; ++t) {
        int k = 1 + int(rng() % 6); vector<vector<point>> fam; vector<point> acc{{0, 0}};
        for (int i = 0; i < k; ++i) {
            fam.push_back(convexHull(randomPoints(rng, 50, 1 + int(rng() % 12))));
            acc = convexHull(sums(acc, fam.back()));}
        lhs = fam[0]; rhs = acc;
        require(minkowskiSum(fam) == acc, "family sum equals iterated hull of sums");}
    phase = "nonconvex hull of sum";
    for (int t = 0; t < rounds / 4; ++t) {
        auto p = randomPoints(rng, 20, 1 + int(rng() % 15)), q = randomPoints(rng, 20, 1 + int(rng() % 15));
        lhs = p; rhs = q;
        require(minkowskiSumHull(p, q) == convexHull(sums(p, q)), "minkowskiSumHull equals the hull of all sums");}
    phase = "large";
    int n = quick ? 2000 : stress ? 500000 : 200000;
    vector<point> big, other;
    for (int i = 0; i < n; ++i) { big.push_back({i, lng(i) * i}); other.push_back({-lng(i) * i, i}); }
    big = convexHull(big); other = convexHull(other); lhs = {}; rhs = {};
    auto r = minkowskiSum(big, other);
    require(int(r.size()) <= int(big.size() + other.size()) && r[0] == big[0] + *std::min_element(other.begin(), other.end()), "large sum size and start");
    for (int i = 0; i < int(r.size()); ++i) { require(orient(r[i], r[(i + 1) % r.size()], r[(i + 2) % r.size()]) > 0, "large sum strictly convex"); }
    vector<vector<point>> many(n / 100, vector<point>{{0, 0}, {1, 0}, {1, 1}, {0, 1}});
    auto m = minkowskiSum(many);
    require(m == vector<point>{{0, 0}, {n / 100, 0}, {n / 100, n / 100}, {0, n / 100}}, "many squares merge parallel edges");
    std::cout << "PASS minkowskisum seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
