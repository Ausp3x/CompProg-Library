#include "../../03-Geometry/05-closestpair.hpp"

static ulng seed = 20260927;
static string phase;
static int checks = 0;
string decimal(lll x) {
    if (!x) { return "0"; }
    string s; while (x) { s += char('0' + x % 10); x /= 10; } reverse(s.begin(), s.end()); return s;}
using point3 = Point3<lng>;
lll square(lll x) { return x * x; }
lll distance(point a, point b) { return square(lll(a.x) - b.x) + square(lll(a.y) - b.y); }
lll distance(point3 a, point3 b) { return square(lll(a.x) - b.x) + square(lll(a.y) - b.y) + square(lll(a.z) - b.z); }
void print(point q) { std::cerr << '(' << q.x << ',' << q.y << ')'; }
void print(point3 q) { std::cerr << '(' << q.x << ',' << q.y << ',' << q.z << ')'; }
ClosestPair solve(const vector<point> &p) { return closestPair(p); }
ClosestPair solve(const vector<point3> &p) { return closestPair3(p); }
// Independent O(n^2) oracle: the first strictly smaller distance in (i, j) order is the lexicographic tie winner.
template<typename P> ClosestPair oracle(const vector<P> &p) {
    ClosestPair ans;
    for (int i = 0; i < int(p.size()); ++i) {
        for (int j = i + 1; j < int(p.size()); ++j) {
            lll d = distance(p[i], p[j]);
            if (ans.ids.first < 0 || d < ans.distance2) { ans = {{i, j}, d}; }}}
    return ans;}
bool equal(ClosestPair a, ClosestPair b) { return a.ids == b.ids && a.distance2 == b.distance2; }
template<typename P> void require(bool ok, const vector<P> &p, ClosestPair want, ClosestPair got, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " n=" << p.size()
              << " expected=" << want.ids.first << ',' << want.ids.second << ':' << decimal(want.distance2)
              << " actual=" << got.ids.first << ',' << got.ids.second << ':' << decimal(got.distance2) << " input=";
    for (auto q : p) { print(q); }
    std::cerr << '\n'; std::exit(1);}
template<typename P> void verify(vector<P> p) {
    auto before = p; auto want = oracle(p), got = solve(p);
    if (!equal(want, got)) {
        // Greedy one-point deletion: a locally minimal, independently checked
        // reproducer; retain full seed/phase for the originally generated case.
        for (int i = 0; i < int(p.size()); ) {
            auto q = p; q.erase(q.begin() + i);
            if (!equal(oracle(q), solve(q))) { p = std::move(q); i = 0; }
            else { ++i; }}
        require(false, p, oracle(p), solve(p), "minimum pair (deletion-minimal)");}
    require(equal(want, got), p, want, got, "minimum pair");
    require(p == before, before, want, got, "input preserved");}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    bool protocol = false, three = false;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid = argv[++i]; }
        else if (a == "--oracle") { protocol = true; }
        else if (a == "--oracle3") { protocol = three = true; }}
    const lng lo = std::numeric_limits<lng>::min(), hi = std::numeric_limits<lng>::max();
    if (!invalid.empty()) {
        if (invalid == "span-x") { closestPair(vector<point>{{lo, 0}, {hi, 0}}); }
        else if (invalid == "span-y") { closestPair(vector<point>{{0, lo}, {0, hi}}); }
        else if (invalid == "diagonal") { closestPair(vector<point>{{lo, lo}, {0, 0}}); }
        else if (invalid == "span-z3") { closestPair3(vector<point3>{{0, 0, lo}, {0, 0, hi}}); }
        else if (invalid == "diagonal3") { closestPair3(vector<point3>{{lo / 2, lo / 2, lo / 2}, {hi / 2, hi / 2, hi / 2}}); }
        else { return 2; }
        return 0;}
    if (protocol) {
        int cases; cin >> cases;
        while (cases--) {
            int n; cin >> n; vector<point3> p(n);
            for (auto &q : p) { cin >> q.x >> q.y; if (three) { cin >> q.z; } }
            vector<point> flat; for (auto q : p) { flat.push_back({q.x, q.y}); }
            auto ans = three ? closestPair3(p) : closestPair(flat); cout << ans.ids.first << ' ' << ans.ids.second << ' ' << decimal(ans.distance2) << '\n';}
        return 0;}
    std::mt19937_64 rng(seed);
    phase = "regressions / full-width translated clusters";
    for (const vector<point> &p : vector<vector<point>>{
        {}, {{lo, hi}}, {{hi, lo}, {hi, lo}}, {{0, 0}, {3, 4}},
        {{lo, lo}, {-1, -1}}, {{hi, hi}, {hi - 1, hi - 2}, {hi - 8, hi - 9}},
        {{lo, lo}, {lo + 1, lo + 2}, {lo + 8, lo + 9}},
        {{4, 4}, {1, 1}, {1, 1}, {4, 4}, {2, 2}, {2, 2}},
        {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}, {{3, 0}, {2, 0}, {1, 0}, {0, 0}},
        {{0, 0}, {1, 1}, {-1, -1}, {1, -1}, {-1, 1}},
        {{2, 1}, {2, 2}, {0, 2}, {0, 1}, {1, 0}, {1, 3}}}) { verify(p); }
    phase = "all 2x2 ordered lists";
    int top = mode == "quick" ? 4 : mode == "full" ? 7 : 9, total = 1;
    for (int n = 0; n <= top; ++n) {
        for (int mask = 0; mask < total; ++mask) {
            vector<point> p; int x = mask;
            for (int i = 0; i < n; ++i) { p.push_back({x % 2, x / 2 % 2}); x /= 4; }
            verify(p);}
        total *= 4;}
    phase = "all 3x3 subsets / index permutations";
    for (int mask = 0; mask < (mode == "quick" ? 64 : 512); ++mask) {
        vector<point> p; for (int k = 0; k < 9; ++k) { if (mask >> k & 1) { p.push_back({k % 3 - 1, k / 3 - 1}); } }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);}
    phase = "random grid, sparse, tied-x and tied-y / isometries";
    int rounds = mode == "quick" ? 30 : mode == "full" ? 1200 : 10000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(rng() % 85); lng c = t % 3 ? 20 : 3000000000000000000LL;
        for (int k = 0; k < n; ++k) {
            lng x = lng(rng() % (2 * c + 1)) - c, y = lng(rng() % (2 * c + 1)) - c;
            p.push_back({t % 5 == 0 ? 0 : x, t % 5 == 1 ? 0 : y});}
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);
        for (auto &q : p) { q = {-q.y, q.x}; } verify(p);
        for (auto &q : p) { q = {q.x + 13, q.y - 17}; } verify(p);}
    phase = "coordinate type variants";
    auto a = closestPair(vector<Point2<int>>{{std::numeric_limits<int>::min(), 0}, {std::numeric_limits<int>::max(), 0}});
    require(a.ids == pair{0, 1} && a.distance2 == lll(4294967295LL) * 4294967295LL, vector<point>{}, {{0, 1}, lll(4294967295LL) * 4294967295LL}, a, "int");
    auto b = closestPair(vector<Point2<int16_t>>{{-32768, 0}, {32767, 0}});
    require(equal(b, {{0, 1}, 65535LL * 65535LL}), vector<point>{}, {{0, 1}, 65535LL * 65535LL}, b, "int16_t");
    auto c = closestPair(vector<Point2<int8_t>>{{-128, 0}, {127, 0}});
    require(equal(c, {{0, 1}, 65025}), vector<point>{}, {{0, 1}, 65025}, c, "int8_t");
    phase = "large no-duplicate equal-support grid and line";
    int n = mode == "quick" ? 500 : mode == "full" ? 50000 : 250000;
    for (int shape = 0; shape < 4; ++shape) {
        vector<point> p;
        for (int i = 0; i < n; ++i) {
            if (shape == 0) { p.push_back({i % 200, i / 200}); }
            if (shape == 1) { p.push_back({0, -i}); }
            if (shape == 2) { p.push_back({i, 0}); }
            if (shape == 3) { p.push_back({7, 11}); }}
        auto before = p; auto got = closestPair(p); ClosestPair want{{0, 1}, shape == 3 ? 0 : 1};
        require(equal(want, got), p, want, got, "large known minimum");
        require(before == p, p, want, got, "large input preserved");}
    phase = "3D all 2x2x2 ordered lists";
    int top3 = mode == "quick" ? 3 : mode == "full" ? 5 : 6;
    total = 1;
    for (int len = 0; len <= top3; ++len) {
        for (int mask = 0; mask < total; ++mask) {
            vector<point3> p; int x = mask;
            for (int i = 0; i < len; ++i) { p.push_back({x % 2, x / 2 % 2, x / 4 % 2}); x /= 8; }
            verify(p);}
        total *= 8;}
    phase = "3D all 3x3x3 subsets of size <= 4 and random subsets / permutations";
    for (int t = 0; t < (mode == "quick" ? 200 : mode == "full" ? 4000 : 40000); ++t) {
        vector<point3> p; int k = int(rng() % 28);
        for (int c = 0; c < 27; ++c) { if (rng() % 27 < ulng(k)) { p.push_back({c % 3 - 1, c / 3 % 3 - 1, c / 9 - 1}); } }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);}
    phase = "3D random grid, sparse, planar, tied axes and duplicates / isometries";
    for (int t = 0; t < rounds; ++t) {
        vector<point3> p; int len = int(rng() % 120); lng c = t % 3 == 0 ? 3000000000000000000LL : t % 3 == 1 ? 6 : 40;
        auto coord = [&]() { return lng(rng() % ulng(2 * c + 1)) - c; };
        for (int k = 0; k < len; ++k) {
            point3 q{coord(), coord(), coord()};
            if (t % 7 == 0) { q.x = 5; }
            if (t % 7 == 1) { q.y = -3; }
            if (t % 7 == 2) { q.z = 0; q.y = q.x; }
            p.push_back(q);}
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);
        for (auto &q : p) { q = {q.z, q.x, q.y}; } verify(p);
        for (auto &q : p) { q = {-q.y, q.x, q.z}; } verify(p);
        if (c < 100) { for (auto &q : p) { q = {q.x + 999, q.y - 7, q.z * 3}; } verify(p); }}
    phase = "3D full-width translated clusters and extreme singletons";
    for (const vector<point3> &p : vector<vector<point3>>{
        {}, {{lo, hi, lo}}, {{hi, hi, hi}, {hi, hi, hi}}, {{0, 0, 0}, {1, 2, 2}},
        {{lo, lo, lo}, {lo + 1, lo + 2, lo + 2}, {lo + 4, lo + 4, lo + 4}},
        {{hi, hi, hi}, {hi - 1, hi - 1, hi - 1}, {hi - 3, hi, hi - 1}, {hi - 1, hi - 1, hi - 1}},
        {{3, 0, 0}, {0, 3, 0}, {0, 0, 3}, {-3, 0, 0}, {0, -3, 0}, {0, 0, -3}}}) { verify(p); }
    phase = "3D coordinate type variants";
    auto d3 = closestPair3(vector<Point3<int>>{{std::numeric_limits<int>::min(), 0, 0}, {std::numeric_limits<int>::max(), 0, 0}});
    require(equal(d3, {{0, 1}, lll(4294967295LL) * 4294967295LL}), vector<point3>{}, {{0, 1}, lll(4294967295LL) * 4294967295LL}, d3, "3D int");
    auto e3 = closestPair3(vector<Point3<int8_t>>{{-128, -128, -128}, {127, 127, 127}, {0, 0, 1}});
    require(equal(e3, {{1, 2}, 127LL * 127 * 2 + 126LL * 126}), vector<point3>{}, {{1, 2}, 127LL * 127 * 2 + 126LL * 126}, e3, "3D int8_t");
    phase = "3D large lattice cube, plane and line";
    int m3 = mode == "quick" ? 10 : mode == "full" ? 40 : 70;
    for (int shape = 0; shape < 3; ++shape) {
        vector<point3> p;
        for (int i = 0; i < m3 * m3 * m3; ++i) {
            if (shape == 0) { p.push_back({2 * (i % m3), 2 * (i / m3 % m3), 2 * (i / m3 / m3)}); }
            if (shape == 1) { p.push_back({0, 3 * (i % (m3 * m3)), 3 * (i / (m3 * m3))}); }
            if (shape == 2) { p.push_back({lng(i) * 5, lng(i) * 5, lng(i) * 5}); }}
        p.push_back({1, 1, 1});
        int last = int(p.size()) - 1;
        ClosestPair want{{0, last}, 3};
        auto before = p; auto got = closestPair3(p);
        require(equal(want, got), vector<point3>{}, want, got, "3D large known minimum");
        require(before == p, vector<point3>{}, want, got, "3D large input preserved");}
    cout << "PASS closestpair seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
