#include "../../03-Geometry/07-rotatingcalipers.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

static ulng seed = 20260927;
static vector<point> input;
static string phase;
static int checks = 0;
void require(bool ok, const char *what, long double expected = 0, long double actual = 0) {
    ++checks; if (ok) { return; }
    std::cerr << std::setprecision(25) << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what
              << " expected=" << expected << " actual=" << actual << " hull=";
    for (auto p : input) { std::cerr << '(' << p.x << ',' << p.y << ')'; }
    std::cerr << '\n'; std::exit(1); }
bool near(long double a, long double b) { return std::abs(a - b) <= 2e-12L * max(1.0L, max(std::abs(a), std::abs(b))); }
lll det(point a, point b) { return lll(a.x) * b.y - lll(a.y) * b.x; }
lll scalar(point a, point b) { return lll(a.x) * b.x + lll(a.y) * b.y; }
// Independent base-2^32 schoolbook product; unlike the library's continued
// fractions this computes the complete 256-bit cross products directly.
array<uint, 8> product(ulll a, ulll b) {
    array<uint, 8> out{};
    for (int i = 0; i < 4; ++i) {
        ulng carry = 0;
        for (int j = 0; j < 4; ++j) {
            ulng value = ulng(uint(a >> (32 * i))) * uint(b >> (32 * j)) + out[i + j] + carry;
            out[i + j] = uint(value); carry = value >> 32; }
        out[i + 4] = uint(carry); }
    reverse(out.begin(), out.end()); return out; }
struct Fraction {
    ulll a = 0, b = 1;
    bool operator<(const Fraction &r) const { return product(a, r.b) < product(r.a, b); }
    long double value() const { return static_cast<long double>(a) / static_cast<long double>(b); }
};
void verifyBox(const CaliperBoxApprox &box, bool rectangle, long double area, long double perimeter = -1) {
    require(box.exists == !input.empty(), "box existence");
    require(near(box.area, area), "box area", area, box.area);
    if (perimeter >= 0) { require(near(box.perimeter, perimeter), "box perimeter", perimeter, box.perimeter); }
    if (input.size() < 3) { return; }
    long double scale = 1;
    for (auto q : input) { scale = max(scale, max(std::abs(static_cast<long double>(q.x)), std::abs(static_cast<long double>(q.y)))); }
    auto p = box.vertices;
    require(normApprox((p[0] + p[2]) - (p[1] + p[3])) < 1e-10L * scale, "parallelogram diagonal midpoints");
    if (rectangle) {
        long double angle = dot(p[1] - p[0], p[2] - p[1]);
        require(std::abs(angle) <= 1e-11L * scale * scale, "rectangle orthogonality"); }
    long double a = 0, peri = 0;
    for (int i = 0; i < 4; ++i) {
        auto e = p[(i + 1) % 4] - p[i]; a += cross(p[i] - p[0], p[(i + 1) % 4] - p[0]); peri += normApprox(e);
        for (auto v : input) { require(cross(e, v.cast<long double>() - p[i]) >= -1e-11L * scale * scale, "box CCW containment"); } }
    require(std::abs(a / 2 - box.area) <= 2e-12L * box.area + 128 * std::numeric_limits<long double>::epsilon() * scale * scale, "vertex area", box.area, a / 2);
    require(near(peri, box.perimeter), "vertex perimeter", box.perimeter, peri); }
void verify(vector<point> p) {
    input = std::move(p); int n = int(input.size()); ConvexCalipers calipers(input);
    auto diameter = calipers.diameter();
    if (!n) { require(diameter.first == -1 && diameter.second == -1 && diameter.squared == 0, "empty diameter"); }
    else {
        lll best = -1; pair<int, int> pair{-1, -1};
        for (int i = 0; i < n; ++i) { for (int j = i; j < n; ++j) {
            auto d = input[i] - input[j]; lll value = scalar(d, d);
            if (value > best) { best = value; pair = {i, j}; } } }
        require(lll(diameter.squared) == best && std::pair{diameter.first, diameter.second} == pair, "brute diameter with lexicographic ties"); }
    auto pairs = calipers.antipodalPairs(); set<pair<int, int>> actual(pairs.begin(), pairs.end()), expected;
    require(actual.size() == pairs.size(), "antipodal unique pairs");
    if (n == 2) { expected.emplace(0, 1); }
    vector<point> e; vector<lll> w, u; Fraction min_width, min_area, min_perimeter, min_para;
    for (int i = 0; i < n && n >= 3; ++i) {
        e.push_back(input[(i + 1) % n] - input[i]);
        lll lo = 0, hi = 0, xlo = scalar(e[i], input[0]), xhi = xlo;
        for (auto q : input) {
            lll s = det(e[i], q - input[i]); lo = min(lo, s); hi = max(hi, s);
            s = scalar(e[i], q); xlo = min(xlo, s); xhi = max(xhi, s); }
        for (int j = 0; j < n; ++j) { for (int k = j + 1; k < n; ++k) {
            lll a = det(e[i], input[j] - input[i]), b = det(e[i], input[k] - input[i]);
            if ((a == lo && b == hi) || (a == hi && b == lo)) { expected.emplace(j, k); } } }
        w.push_back(hi - lo); u.push_back(xhi - xlo); lll norm = scalar(e[i], e[i]);
        Fraction width{ulll(w[i]) * w[i], ulll(norm)}, area{ulll(u[i]) * w[i], ulll(norm)}, perimeter{ulll(u[i] + w[i]) * (u[i] + w[i]), ulll(norm)};
        if (!i || width < min_width) { min_width = width; }
        if (!i || area < min_area) { min_area = area; }
        if (!i || perimeter < min_perimeter) { min_perimeter = perimeter; } }
    require(actual == expected, "all antipodal support events");
    bool found = false;
    for (int i = 0; i < n && n >= 3; ++i) { for (int j = i + 1; j < n; ++j) {
        lll d = det(e[i], e[j]); if (d < 0) { d = -d; } if (!d) { continue; }
        Fraction area{ulll(w[i]) * w[j], ulll(d)}; if (!found || area < min_para) { min_para = area; found = true; } } }
    auto width = calipers.minimumWidthApprox(); long double want_width = std::sqrt(min_width.value());
    require(near(width.width, want_width), "brute minimum width", want_width, width.width);
    if (n >= 3) {
        require(width.opposite >= 0 && width.edge >= 0, "width witnesses exist");
        require(det(e[width.edge], input[width.opposite] - input[width.edge]) == w[width.edge], "width support witness"); }
    else { require(width.edge == (n ? 0 : -1), "degenerate width witness"); }
    long double small_perimeter = n == 2 ? 2 * distanceApprox(input[0], input[1]) : 0;
    auto ar = calipers.minimumAreaRectangleApprox(), pr = calipers.minimumPerimeterRectangleApprox(), pa = calipers.minimumAreaParallelogramApprox();
    verifyBox(ar, true, min_area.value(), n < 3 ? small_perimeter : -1);
    verifyBox(pr, true, n < 3 ? 0 : (Fraction{ulll(u[pr.edges[0]]) * w[pr.edges[0]], ulll(scalar(e[pr.edges[0]], e[pr.edges[0]]))}).value(), n < 3 ? small_perimeter : 2 * std::sqrt(min_perimeter.value()));
    verifyBox(pa, false, min_para.value(), n < 3 ? small_perimeter : -1);
    if (n >= 3) {
        require(pa.edges[0] >= 0 && pa.edges[1] >= 0 && pa.edges[0] != pa.edges[1], "parallelogram support edges");
        lll d = det(e[pa.edges[0]], e[pa.edges[1]]); if (d < 0) { d = -d; }
        Fraction witness{ulll(w[pa.edges[0]]) * w[pa.edges[1]], ulll(d)};
        require(!(witness < min_para) && !(min_para < witness), "exact parallelogram optimum");
        Fraction area{ulll(u[ar.edges[0]]) * w[ar.edges[0]], ulll(scalar(e[ar.edges[0]], e[ar.edges[0]]))};
        require(!(area < min_area) && !(min_area < area), "exact rectangle optimum");
        int edge = pr.edges[0]; Fraction perimeter{ulll(u[edge] + w[edge]) * (u[edge] + w[edge]), ulll(scalar(e[edge], e[edge]))};
        require(!(perimeter < min_perimeter) && !(min_perimeter < perimeter), "exact perimeter optimum");
        edge = width.edge; Fraction width_value{ulll(w[edge]) * w[edge], ulll(scalar(e[edge], e[edge]))};
        require(!(width_value < min_width) && !(min_width < width_value), "exact width optimum"); } }

int main(int argc, char **argv) {
    if (argc > 1 && string(argv[1]) == "--fractions") {
        auto read = [](string s) { ulll x = 0; for (char c : s) { x = x * 10 + c - '0'; } return x; };
        string a, b, c, d;
        while (std::cin >> a >> b >> c >> d) {
            auto x = read(a), y = read(b), z = read(c), w = read(d);
            for (auto v : product(x, w)) { std::cout << std::hex << std::setw(8) << std::setfill('0') << v; }
            std::cout << ' ';
            for (auto v : product(z, y)) { std::cout << std::hex << std::setw(8) << std::setfill('0') << v; }
            std::cout << ' ' << ConvexCalipers::ratioLess(x, y, z, w) << '\n'; }
        return 0; }
    string mode = "full", invalid;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid = argv[++i]; } }
    if (!invalid.empty()) {
        if (invalid == "denominator") { return ConvexCalipers::ratioLess(0, 0, 0, 1); }
        vector<point> p = invalid == "clockwise" ? vector<point>{{0,0},{0,1},{1,0}} : invalid == "collinear" ? vector<point>{{0,0},{1,0},{2,0}} : invalid == "duplicate" ? vector<point>{{0,0},{0,0}} : vector<point>{{1000000001,0}};
        ConvexCalipers c(p); return c.n == -1; }
    std::mt19937_64 rng(seed);
    phase = "regressions and extreme coordinates";
    for (auto p : vector<vector<point>>{{},{{0,0}},{{0,0},{3,4}},{{0,0},{4,0},{0,3}},{{-2,-1},{2,-1},{2,1},{-2,1}},{{0,0},{7,2},{9,6},{2,4}},{{-1000000000,-1000000000},{1000000000,-1000000000},{1000000000,1000000000},{-1000000000,1000000000}},{{-1000000000,-999999999},{999999999,999999998},{1000000000,1000000000}},{{0,0},{1,-4},{4,-5},{7,-4},{9,0},{6,6},{1,5}}}) { verify(convexHull(p)); }
    phase = "all small-grid subsets"; int side = mode == "stress" ? 4 : 3, masks = mode == "quick" ? 64 : 1 << (side * side);
    for (int mask = 0; mask < masks; ++mask) {
        vector<point> p;
        for (int k = 0; k < side * side; ++k) { if (mask >> k & 1) { p.push_back({k % side, k / side}); } }
        p = convexHull(p);
        for (int i = 0; i < max(1, int(p.size())); ++i) {
            verify(p); if (!p.empty()) { std::rotate(p.begin(), p.begin() + 1, p.end()); } } }
    phase = "random hulls and cyclic/rigid/scale metamorphisms";
    int rounds = mode == "quick" ? 50 : mode == "full" ? 1000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = 3 + rng() % 45; lng c = t % 3 ? 30 : 1000000000;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        p = convexHull(p); verify(p);
        if (!p.empty()) { std::rotate(p.begin(), p.begin() + rng() % p.size(), p.end()); verify(p); }
        for (auto &q : p) { q = {-q.y, q.x}; } verify(p);
        if (c == 30) { for (auto &q : p) { q = {q.x * 7 + 999999000, q.y * 7 - 999999000}; } verify(p); } }
    phase = "thin high-dynamic-range hulls";
    for (int t = 0; t < rounds / 4; ++t) {
        vector<point> p;
        for (int k = 0; k < 30; ++k) {
            lng x = lng(rng() % 1999999901) - 999999950;
            p.push_back({x, x + lng(rng() % 61) - 30}); }
        verify(convexHull(p)); }
    phase = "fraction comparator full 128-bit random operands";
    for (int i = 0; i < rounds * 10; ++i) {
        ulll a = ulll(rng()) << 64 | rng(), b = ulll(rng()) << 64 | rng(), c = ulll(rng()) << 64 | rng(), d = ulll(rng()) << 64 | rng();
        b |= 1; d |= 1;
        require(ConvexCalipers::ratioLess(a, b, c, d) == (product(a, d) < product(c, b)), "256-bit schoolbook fraction comparison"); }
    phase = "int coordinates";
    ConvexCalipers small(vector<Point2<int>>{{0,0},{3,0},{0,4}}); require(small.diameter().squared == 25, "32-bit input");
    phase = "large strict convex parabola";
    int half = mode == "quick" ? 50 : mode == "full" ? 5000 : 15000; vector<point> p;
    for (int x = -half; x <= half; ++x) { p.push_back({x, lng(x) * x}); }
    input = p; ConvexCalipers large(p); auto diam = large.diameter();
    lll expected = max(lll(4) * half * half, lll(half) * half + lll(half) * half * half * half);
    require(lll(diam.squared) == expected, "large parabola diameter");
    auto ar = large.minimumAreaRectangleApprox(), pr = large.minimumPerimeterRectangleApprox(), pa = large.minimumAreaParallelogramApprox();
    require(ar.exists && pr.exists && pa.exists && pa.area <= ar.area * (1 + 1e-12L) && pr.perimeter <= ar.perimeter * (1 + 1e-12L), "large linear sweeps");
    require(large.antipodalPairs().size() <= 4 * p.size(), "linear antipodal output bound");
    std::cout << "PASS rotatingcalipers seed=" << seed << " mode=" << mode << " checks=" << checks << '\n'; }
