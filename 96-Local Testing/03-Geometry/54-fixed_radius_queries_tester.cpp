#include "../../03-Geometry/54-fixed_radius_queries.hpp"

// Oracles: all pairs; for maxPointsInDisk every point and both centres of every close pair, tested by an exact one-radical sign.
static ulng seed = 20260927;
static std::string phase;
static vector<point> pts;
static lng checks = 0;
static std::string context;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " " << context << " points=";
    for (auto x : pts) { std::cerr << '(' << x.x << ',' << x.y << ')'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
vector<point> randomPoints(std::mt19937_64 &rng, lng c, int n) {
    vector<point> p(n);
    for (auto &x : p) { x = {lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}; }
    return p;}
int sgn(lll x) { return (x > 0) - (x < 0); }
// 256-bit magnitudes for the oracle, written independently of the header's 384-bit comparator.
using Limbs = array<ulng, 4>;
Limbs square(ulll u) {
    ulng a[2] = {ulng(u), ulng(u >> 64)}; Limbs r{};
    for (int i = 0; i < 2; ++i) {
        ulng c = 0;
        for (int j = 0; j < 2; ++j) { ulll x = ulll(a[i]) * a[j] + r[i + j] + c; r[i + j] = ulng(x); c = ulng(x >> 64); }
        r[i + 2] += c;}
    return r;}
Limbs times(Limbs a, ulng m) {
    Limbs r{}; ulng c = 0;
    for (int i = 0; i < 4; ++i) { ulll x = ulll(a[i]) * m + c; r[i] = ulng(x); c = ulng(x >> 64); }
    return r;}
// Sign of a * sqrt(x) + b * sqrt(y) for 0 <= x, y < 2^64 and a^2 x, b^2 y < 2^256 (|a|, |b| < 2^96 suffices).
int radicalSign(lll a, lll x, lll b, lll y) {
    int s = x ? sgn(a) : 0, t = y ? sgn(b) : 0;
    if (s == t || !t) { return s; }
    if (!s) { return t; }
    Limbs l = times(square(ulll(a < 0 ? -a : a)), ulng(x)), r = times(square(ulll(b < 0 ? -b : b)), ulng(y));
    for (int i = 3; i >= 0; --i) {
        if (l[i] != r[i]) { return l[i] > r[i] ? s : t; }}
    return 0;}
// Points inside the closed disk of radius sqrt(r2) centred at p + (d + e * perp(d) * sqrt(X / D)) / 2.
int coverCount(point p, point d, int e, lng r2) {
    lll D = norm2(d), X = 4 * lll(r2) - D; int c = 0;
    for (auto q : pts) {
        point g = q - p;
        if (norm2(g) > 4 * lll(r2)) { continue; }
        lll a = norm2(d - g - g) + X - 4 * lll(r2), b = -4 * e * cross(d, g);
        c += radicalSign(a, D, b, X) <= 0;}
    return c;}
int bruteMax(lng r2) {
    int best = 0;
    for (auto p : pts) {
        int c = 0;
        for (auto q : pts) { c += dist2(p, q) <= r2; }
        best = max(best, c);
        for (auto q : pts) {
            lll D = dist2(p, q);
            if (D > 0 && D <= 4 * lll(r2)) { best = max({best, coverCount(p, q - p, -1, r2), coverCount(p, q - p, 1, r2)}); }}}
    return best;}
void diskCase(lng r2, bool brute) {
    context = "r2=" + std::to_string(r2);
    auto [cnt, c] = maxPointsInDisk(pts, r2);
    if (brute) { require(cnt == bruteMax(r2), "maxPointsInDisk equals the exhaustive centre search"); }
    long double scale = 1, r = std::sqrt((long double)r2);
    for (auto q : pts) { scale = max({scale, std::abs((long double)q.x), std::abs((long double)q.y)}); }
    int in = 0;
    for (auto q : pts) { in += distanceApprox(q.cast<long double>(), c) <= r + 1e-12L * scale; }
    require(in >= cnt, "the rounded centre covers the count up to rounding");}
void gridCase(lng r) {
    context = "r=" + std::to_string(r);
    GridHash g(pts, r); SortedGrid h(pts, r); int n = int(pts.size());
    vector<pair<int, int>> want; vector<int> cover(n, 0);
    for (int i = 0; i < n; ++i) {
        vector<int> nb;
        for (int j = 0; j < n; ++j) {
            if (dist2(pts[i], pts[j]) <= lll(r) * r) {
                ++cover[i];
                if (j != i) { nb.push_back(j); }
                if (i < j) { want.push_back({i, j}); }}}
        auto got = g.neighbours(i);
        require(h.neighbours(i) == got, "SortedGrid neighbours equal GridHash's, in order");
        sort(got.begin(), got.end());
        require(got == nb, "neighbours equals the pair scan");}
    auto pr = g.pairsWithin();
    require(h.pairsWithin() == pr, "SortedGrid pairsWithin equals GridHash's, in order");
    sort(pr.begin(), pr.end());
    require(pr == want, "pairsWithin equals the pair scan");
    require(g.diskCoverCounts() == cover && h.diskCoverCounts() == cover, "diskCoverCounts equals the pair scan");}
// Events for pivot offsets d with 0 < |d|^2 <= 4 r2; near the circle |d| = min(2 sqrt(r2), 2 sqrt(2) * 10^9) when edge is set.
void comparatorCase(std::mt19937_64 &rng, lng r2, lng c, bool edge = false) {
    using fixed_radius_detail::Event;
    vector<Event> e; lll q = 4 * lll(r2);
    for (int i = 0; i < 6; ++i) {
        point d = randomPoints(rng, c, 1)[0];
        if (edge) {
            long double rho = std::min(std::sqrt((long double)q), 2828427124.0L) - (long double)(rng() % 64), a = (long double)(rng() % 1000000) * 2e-6L * std::numbers::pi_v<long double>;
            d = {lng(rho * std::cos(a)), lng(rho * std::sin(a))};
            d.x = std::clamp<lng>(d.x, -2000000000, 2000000000); d.y = std::clamp<lng>(d.y, -2000000000, 2000000000);}
        lll D = norm2(d);
        if (D == 0 || D > q) { continue; }
        e.push_back({d, D, q - D, rng() % 2 ? 1 : -1});}
    auto angle = [](const Event &x) {
        long double t = std::sqrt((long double)x.X / (long double)x.D); dpoint d = x.d.cast<long double>(), v = d + perp(d) * (x.e * t);
        long double a = std::atan2(v.y, v.x);
        return a < 0 ? a + 2 * std::numbers::pi_v<long double> : a;};
    auto before = [](const Event &a, const Event &b) { return fixed_radius_detail::before<true>(a, b); };
    for (auto &x : e) {
        require(!before(x, x), "irreflexive");
        for (auto &y : e) {
            bool xy = before(x, y), yx = before(y, x);
            require(!(xy && yx), "asymmetric");
            if (r2 <= (lng(1) << 30)) { require(xy == fixed_radius_detail::before<false>(x, y), "lll and wide comparators agree"); }
            long double ax = angle(x), ay = angle(y);
            if (std::abs(ax - ay) > 1e-12L) { require(xy == (ax < ay), "agrees with separated rounded angles"); }
            for (auto &z : e) {
                if (xy && before(y, z)) { require(before(x, z), "transitive"); }}}}}
void invalid(const string &name) {
    if (name == "grid-r") { GridHash g(vector<point>{{0, 0}}, 0); }
    if (name == "grid-coordinate") { GridHash g(vector<point>{{2000000000, 0}}, 1); }
    if (name == "grid-index") { GridHash g(vector<point>{{0, 0}}, 1); (void)g.neighbours(1); }
    if (name == "sorted-r") { SortedGrid g(vector<point>{{0, 0}}, 0); }
    if (name == "sorted-coordinate") { SortedGrid g(vector<point>{{0, -2000000000}}, 1); }
    if (name == "sorted-index") { SortedGrid g(vector<point>{{0, 0}}, 1); (void)g.neighbours(-1); }
    if (name == "disk-r2") { (void)maxPointsInDisk(vector<point>{{0, 0}}, 4000000000000000001LL); }
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
    pts = {}; gridCase(3); require(maxPointsInDisk(pts, 4).first == 0, "empty");
    pts = {{0, 0}}; gridCase(1); diskCase(0, true); diskCase(9, true);
    pts = {{0, 0}, {0, 0}, {0, 0}, {5, 0}}; gridCase(1); diskCase(0, true); diskCase(6, true); diskCase(7, true);
    pts = {{0, 0}, {2, 0}}; diskCase(1, true); require(maxPointsInDisk(pts, 1).first == 2, "diameter pair on a closed disk");
    pts = {{0, 0}, {3, 0}}; diskCase(2, true); require(maxPointsInDisk(pts, 2).first == 1, "pair farther than the diameter");
    pts = {{-5, 0}, {5, 0}, {0, 5}, {0, -5}, {3, 4}, {-4, -3}}; diskCase(25, true); require(maxPointsInDisk(pts, 25).first == 6, "cocircular points");
    pts = {{-2, -2}, {-2, 2}, {2, -2}, {2, 2}}; diskCase(8, true); diskCase(7, true);
    int rounds = quick ? 150 : stress ? 12000 : 1500;
    phase = "grid random";
    for (int t = 0; t < rounds; ++t) {
        lng c = t % 4 == 3 ? 1000000000 : 12;
        pts = randomPoints(rng, c, int(rng() % 60));
        gridCase(c == 12 ? 1 + lng(rng() % 8) : 1 + lng(rng() % 4000000000LL));}
    phase = "grid dense";
    for (int t = 0; t < rounds / 10; ++t) {
        // Clusters inside a cell and clusters just beyond r in the neighbouring cells; coordinates at both domain ends.
        lng r = 1 + lng(rng() % 1000), off = t % 2 ? 999990000 : -1000000000; pts.clear();
        for (int k = 0; k < 4; ++k) {
            point centre = {off + lng(rng() % 6000), lng(rng() % 6000) - 3000};
            for (int j = int(rng() % 40); j-- > 0;) { pts.push_back({centre.x + lng(rng() % ulng(r / 2 + 1)), centre.y + lng(rng() % ulng(r / 2 + 1))}); }
            point far = {centre.x + r + r / 5 + 1, centre.y};
            for (int j = int(rng() % 40); j-- > 0;) { pts.push_back({far.x + lng(rng() % ulng(r / 8 + 1)), far.y}); }}
        gridCase(r);}
    phase = "disk random";
    for (int t = 0; t < rounds; ++t) {
        pts = randomPoints(rng, t % 3 ? 6 : 30, 1 + int(rng() % 18));
        if (t % 4 == 0) { pts.push_back(pts[0]); }
        diskCase(lng(rng() % 60), true);}
    phase = "disk wide";
    for (int t = 0; t < rounds / 5; ++t) {
        lng r2 = 1 + lng(rng() % 400000000), off = lng(rng() % 1000000000) - 500000000;
        pts = randomPoints(rng, 60000, 4 + int(rng() % 26));
        for (auto &x : pts) { x.x += off; }
        diskCase(r2, true);}
    context = "domain edge";
    pts = {{-20000, 0}, {20000, 0}, {0, 0}, {1000000000, 1000000000}}; diskCase(400000000, true);
    require(maxPointsInDisk(pts, 400000000).first == 3, "diameter 4 * 10^4 at the radius bound");
    phase = "disk wide radius";
    for (int t = 0; t < rounds / 3; ++t) {
        lng S = t % 3 == 0 ? 1000 : t % 3 == 1 ? 1000000 : 1000000000;
        pts = randomPoints(rng, S, 3 + int(rng() % 22));
        if (t % 5 == 0) { pts.push_back(pts[0]); }
        lll lo = lll(S) * S / 16, hi = min(lll(4) * S * S, lll(4000000000000000000LL));
        diskCase(lng(lo + lll(rng() % ulng(hi - lo + 1))), true);}
    context = "wide domain edge";
    lng B = 1000000000;
    pts = {{-B, -B}, {B, -B}, {B, B}, {-B, B}, {0, 0}, {B, 0}};
    diskCase(2000000000000000000LL, true); diskCase(4000000000000000000LL, true); diskCase(1999999999999999999LL, true);
    require(maxPointsInDisk(pts, 2000000000000000000LL).first == 6, "corners on a radius sqrt(2) * 10^9 disk");
    phase = "dispatch threshold";
    for (int t = 0; t < rounds / 10; ++t) {
        pts = randomPoints(rng, 40000, 3 + int(rng() % 20));
        diskCase(lng(1) << 30, true); diskCase((lng(1) << 30) + 1, true);}
    context = "wide cocircular";
    // The 32 lattice points of x^2 + y^2 = 1105 = 5 * 13 * 17, scaled past the threshold: equal event angles need exact wide ties.
    pts.clear();
    for (lng x = -33; x <= 33; ++x) {
        for (lng y = -33; y <= 33; ++y) { if (x * x + y * y == 1105) { pts.push_back({1000 * x + 123456789, 1000 * y - 987654}); } }}
    require(pts.size() == 32, "32 lattice points on the circle");
    diskCase(1105000000, true); diskCase(1104999999, true);
    require(maxPointsInDisk(pts, 1105000000).first == 32, "cocircular points on a wide radius");
    // An acute triangle with circumradius sqrt(1105) s: at every pivot an entry and an exit coincide, so ties must put entries first.
    for (lng s : {1LL, 1000LL, 30000000LL}) {
        pts = {{33 * s, 4 * s}, {-24 * s, 23 * s}, {-9 * s, -32 * s}}; context = "acute triangle s=" + std::to_string(s);
        require(maxPointsInDisk(pts, 1105 * s * s).first == 3 && maxPointsInDisk(pts, 1105 * s * s - 1).first == 2, "circumscribed acute triangle");
        diskCase(1105 * s * s, true);}
    context = "threshold diameters";
    pts = {{-32767, 0}, {32768, 0}, {0, 0}, {-46340, 1}, {0, 46341}};
    diskCase(lng(1) << 30, true); diskCase((lng(1) << 30) + 1, true);
    phase = "near diameter";
    for (int t = 0; t < rounds / 5; ++t) {
        // Points on a ring of radius just under the diameter, where |l| approaches D1 * D2; both comparators, at and past the threshold.
        lng r2 = t % 5 == 0 ? lng(1) << 30 : t % 5 == 1 ? (lng(1) << 30) + 1 : t % 5 == 2 ? 900000000 + lng(rng() % 173741824) : t % 5 == 3 ? lng(1) << 31 : 1 + lng(rng() % 4000000000000000000ULL);
        bool corner = t % 2; long double reach = 2 * std::sqrt((long double)r2), R = std::min(reach, corner ? 2800000000.0L : 1000000000.0L);
        point o = corner ? point{-1000000000, -1000000000} : point{0, 0}; pts = {o};
        for (int j = 0; j < 14; ++j) {
            long double rho = std::floor(R) - (long double)(rng() % 64), a = (long double)(rng() % 1000000) * (corner ? 0.5e-6L : 2e-6L) * std::numbers::pi_v<long double>;
            point x = o + point{lng(rho * std::cos(a)), lng(rho * std::sin(a))};
            if (std::abs(x.x) <= 1000000000 && std::abs(x.y) <= 1000000000) { pts.push_back(x); }}
        if (t % 7 == 0) { pts.push_back(pts.back()); }
        diskCase(r2, true);}
    phase = "comparator";
    for (int t = 0; t < rounds * 4; ++t) {
        lng r2 = t % 3 == 0 ? 1 + lng(rng() % 50) : t % 3 == 1 ? 1 + lng(rng() % 400000000) : 1 + lng(rng() % 4000000000000000000ULL);
        lng c = t % 3 == 0 ? 15 : t % 3 == 1 ? 40000 : 2000000000;
        comparatorCase(rng, r2, c);}
    phase = "comparator edges";
    for (int t = 0; t < rounds; ++t) {
        lng r2 = t % 4 == 0 ? lng(1) << 30 : t % 4 == 1 ? (lng(1) << 30) + 1 : t % 4 == 2 ? 2000000000000000000LL : 4000000000000000000LL;
        comparatorCase(rng, r2, 1, true);}
    phase = "medium brute";
    for (int t = 0; t < (quick ? 1 : stress ? 40 : 6); ++t) {
        pts = randomPoints(rng, 200, quick ? 60 : 200);
        diskCase(1 + lng(rng() % 2500), true);}
    phase = "large";
    pts = randomPoints(rng, 1000000, quick ? 3000 : stress ? 200000 : 60000);
    auto [cnt, centre] = maxPointsInDisk(pts, 100000000);
    context = "large"; int in = 0;
    for (auto q : pts) { in += distanceApprox(q.cast<long double>(), centre) <= 10000 + 1e-6L; }
    require(cnt >= 1 && in >= cnt, "large disk witness");
    GridHash g(pts, 3000); SortedGrid h(pts, 3000);
    auto pairs = g.pairsWithin(); lng cov = 0;
    for (int c : g.diskCoverCounts()) { cov += c - 1; }
    require(2 * lng(pairs.size()) == cov, "pairs and cover counts agree");
    require(h.pairsWithin() == pairs && h.diskCoverCounts() == g.diskCoverCounts(), "large SortedGrid equals GridHash");
    std::cout << "PASS fixed_radius_queries seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
