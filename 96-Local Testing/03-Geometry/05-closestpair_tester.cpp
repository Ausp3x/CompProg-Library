#include "../../03-Geometry/05-closestpair.hpp"

static ulng seed = 20260927;
static string phase;
static int checks = 0;
string decimal(lll x) {
    if (!x) { return "0"; }
    string s; while (x) { s += char('0' + x % 10); x /= 10; } reverse(s.begin(), s.end()); return s; }
ClosestPair oracle(const vector<point> &p) {
    ClosestPair ans;
    for (int i = 0; i < int(p.size()); ++i) { for (int j = i + 1; j < int(p.size()); ++j) {
        lll x = lll(p[i].x) - p[j].x, y = lll(p[i].y) - p[j].y, d = x * x + y * y;
        if (ans.ids.first < 0 || d < ans.distance2) { ans = {{i, j}, d}; } }}
    return ans; }
bool equal(ClosestPair a, ClosestPair b) { return a.ids == b.ids && a.distance2 == b.distance2; }
void require(bool ok, const vector<point> &p, ClosestPair want, ClosestPair got, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " n=" << p.size()
              << " expected=" << want.ids.first << ',' << want.ids.second << ':' << decimal(want.distance2)
              << " actual=" << got.ids.first << ',' << got.ids.second << ':' << decimal(got.distance2) << " input=";
    for (point q : p) { std::cerr << '(' << q.x << ',' << q.y << ')'; }
    std::cerr << '\n'; std::exit(1); }
void verify(vector<point> p) {
    auto before = p; auto want = oracle(p), got = closestPair(p);
    if (!equal(want, got)) {
        // Greedy one-point deletion: a locally minimal, independently checked
        // reproducer; retain full seed/phase for the originally generated case.
        for (int i = 0; i < int(p.size()); ) {
            auto q = p; q.erase(q.begin() + i);
            if (!equal(oracle(q), closestPair(q))) { p = std::move(q); i = 0; }
            else { ++i; }}
        require(false, p, oracle(p), closestPair(p), "minimum pair (deletion-minimal)"); }
    require(equal(want, got), p, want, got, "minimum pair");
    require(p == before, before, want, got, "input preserved"); }
int main(int argc, char **argv) {
    string mode = "full", invalid;
    bool protocol = false;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid = argv[++i]; }
        else if (a == "--oracle") { protocol = true; }}
    const lng lo = std::numeric_limits<lng>::min(), hi = std::numeric_limits<lng>::max();
    if (!invalid.empty()) {
        if (invalid == "span-x") { closestPair(vector<point>{{lo, 0}, {hi, 0}}); }
        else if (invalid == "span-y") { closestPair(vector<point>{{0, lo}, {0, hi}}); }
        else if (invalid == "diagonal") { closestPair(vector<point>{{lo, lo}, {0, 0}}); }
        else { return 2; }
        return 0; }
    if (protocol) {
        int cases; cin >> cases;
        while (cases--) {
            int n; cin >> n; vector<point> p(n); for (auto &q : p) { cin >> q.x >> q.y; }
            auto ans = closestPair(p); cout << ans.ids.first << ' ' << ans.ids.second << ' ' << decimal(ans.distance2) << '\n'; }
        return 0; }
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
            verify(p); }
        total *= 4; }
    phase = "all 3x3 subsets / index permutations";
    for (int mask = 0; mask < (mode == "quick" ? 64 : 512); ++mask) {
        vector<point> p; for (int k = 0; k < 9; ++k) { if (mask >> k & 1) { p.push_back({k % 3 - 1, k / 3 - 1}); } }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p); }
    phase = "random grid, sparse, tied-x and tied-y / isometries";
    int rounds = mode == "quick" ? 30 : mode == "full" ? 1200 : 10000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(rng() % 85); lng c = t % 3 ? 20 : 3000000000000000000LL;
        for (int k = 0; k < n; ++k) {
            lng x = lng(rng() % (2 * c + 1)) - c, y = lng(rng() % (2 * c + 1)) - c;
            p.push_back({t % 5 == 0 ? 0 : x, t % 5 == 1 ? 0 : y}); }
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);
        for (auto &q : p) { q = {-q.y, q.x}; } verify(p);
        for (auto &q : p) { q = {q.x + 13, q.y - 17}; } verify(p); }
    phase = "coordinate type variants";
    auto a = closestPair(vector<Point2<int>>{{std::numeric_limits<int>::min(), 0}, {std::numeric_limits<int>::max(), 0}});
    require(a.ids == pair{0, 1} && a.distance2 == lll(4294967295LL) * 4294967295LL, {}, {{0, 1}, lll(4294967295LL) * 4294967295LL}, a, "int");
    auto b = closestPair(vector<Point2<int16_t>>{{-32768, 0}, {32767, 0}});
    require(equal(b, {{0, 1}, 65535LL * 65535LL}), {}, {{0, 1}, 65535LL * 65535LL}, b, "int16_t");
    auto c = closestPair(vector<Point2<int8_t>>{{-128, 0}, {127, 0}});
    require(equal(c, {{0, 1}, 65025}), {}, {{0, 1}, 65025}, c, "int8_t");
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
        require(before == p, p, want, got, "large input preserved"); }
    cout << "PASS closestpair seed=" << seed << " mode=" << mode << " checks=" << checks << '\n'; }
