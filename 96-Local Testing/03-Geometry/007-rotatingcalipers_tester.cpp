#include "../../03-Geometry/007-rotatingcalipers.hpp"
#include "../../03-Geometry/004-convexhull.hpp"

static ulng seed = 20260927;
static vector<point> input;
static string phase;
static int checks = 0;
void require(bool ok, const char *what, long double expected = 0, long double actual = 0) {
    ++checks; if (ok) { return; }
    std::cerr << std::setprecision(25) << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what
              << " expected=" << expected << " actual=" << actual << " hull=";
    for (auto p : input) { std::cerr << '(' << p.x << ',' << p.y << ')'; }
    std::cerr << '\n'; std::exit(1);}
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
            out[i + j] = uint(value); carry = value >> 32;}
        out[i + 4] = uint(carry);}
    reverse(out.begin(), out.end()); return out;}
struct Fraction {
    ulll a = 0, b = 1;
    bool operator<(const Fraction &r) const { return product(a, r.b) < product(r.a, b); }
    long double value() const { return static_cast<long double>(a) / static_cast<long double>(b); }
};
const long double PI = std::numbers::pi_v<long double>;
// Expected-box-area tolerance relative to the squared diameter D^2 (the answer lies in [D^2 / pi, D^2]).
const long double BOX_TOL = 1e-15L;
static long double box_scale = 0;
static bool quadrature = true;
lll diameter2(const vector<point> &h) {
    lll best = 0;
    for (auto a : h) { for (auto b : h) { best = max(best, scalar(a - b, a - b)); } }
    return best;}
// Independent of the header's support-function closed form: rotates the vertices literally and integrates the
// bounding-box area by 10-node Gauss-Legendre between consecutive kinks (outward edge normals plus multiples of pi / 2).
long double quadratureBoxArea(const vector<point> &h) {
    int n = int(h.size());
    if (n < 2) { return 0; }
    static array<long double, 10> node, weight;
    if (weight[0] == 0) {
        for (int k = 0; k < 10; ++k) {
            long double x = std::cos(PI * (k + 0.75L) / 10.5L), dp = 1;
            for (int it = 0; it < 100; ++it) {
                long double p0 = 1, p1 = x;
                for (int j = 2; j <= 10; ++j) { long double p2 = ((2 * j - 1) * x * p1 - (j - 1) * p0) / j; p0 = p1; p1 = p2; }
                dp = 10 * (x * p1 - p0) / (x * x - 1);
                long double dx = p1 / dp; x -= dx;
                if (std::abs(dx) < 1e-19L) { break; }}
            node[k] = x; weight[k] = 2 / ((1 - x * x) * dp * dp);}
        long double sum = 0, moment = 0;
        for (int k = 0; k < 10; ++k) { sum += weight[k]; moment += weight[k] * std::pow(node[k], 18); }
        require(std::abs(sum - 2) < 1e-17L && std::abs(moment - 2.0L / 19) < 1e-17L, "Gauss-Legendre nodes integrate x^0 and x^18 exactly", 2.0L / 19, moment);}
    vector<long double> cut{0, 2 * PI};
    for (int i = 0; i < n; ++i) {
        point d = h[(i + 1) % n] - h[i];
        long double a = std::atan2(static_cast<long double>(-d.x), static_cast<long double>(d.y));
        for (int k = 0; k < 4; ++k) { cut.push_back(std::fmod(a + k * PI / 2 + 4 * PI, 2 * PI)); }}
    sort(cut.begin(), cut.end());
    auto area = [&](long double t) {
        long double c = std::cos(t), s = std::sin(t), xl = 0, xh = 0, yl = 0, yh = 0;
        for (auto v : h) {
            long double vx = static_cast<long double>(v.x - h[0].x), vy = static_cast<long double>(v.y - h[0].y);
            long double x = vx * c + vy * s, y = vy * c - vx * s;
            xl = min(xl, x); xh = max(xh, x); yl = min(yl, y); yh = max(yh, y);}
        return (xh - xl) * (yh - yl);};
    long double res = 0;
    for (int i = 0; i + 1 < int(cut.size()); ++i) {
        long double mid = (cut[i] + cut[i + 1]) / 2, half = (cut[i + 1] - cut[i]) / 2;
        for (int k = 0; k < 10 && half > 0; ++k) { res += weight[k] * half * area(mid + half * node[k]); }}
    return res / (2 * PI);}
bool sameBox(long double a, long double b, long double scale) { return std::abs(a - b) <= BOX_TOL * scale; }
void sameAs(long double want, long double got, const char *what) { require(sameBox(got, want, box_scale), what, want, got); }
// expectedBoxAreaApprox: exact 0 below two vertices, L^2 / pi for a segment, D^2 / pi <= E <= D^2, the quadrature oracle.
long double checkExpected(const vector<point> &h, const ConvexCalipers &calipers, bool quad, lll diam = -1) {
    long double got = calipers.expectedBoxAreaApprox(), d2 = static_cast<long double>(diam < 0 ? diameter2(h) : diam);
    box_scale = d2;
    if (h.size() < 2) { require(got == 0, "expected box area is exactly 0 below two vertices", 0, got); return got; }
    require(got >= d2 / PI - BOX_TOL * d2 && got <= d2 * (1 + BOX_TOL), "expected box area within [D^2 / pi, D^2]", d2, got);
    if (h.size() == 2) { require(sameBox(got, d2 / PI, d2), "segment expected box area L^2 / pi", d2 / PI, got); }
    if (quad) { long double want = quadratureBoxArea(h); require(sameBox(got, want, d2), "expected box area against literal-rotation quadrature", want, got); }
    return got;}
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
        require(std::abs(angle) <= 1e-11L * scale * scale, "rectangle orthogonality");}
    long double a = 0, peri = 0;
    for (int i = 0; i < 4; ++i) {
        auto e = p[(i + 1) % 4] - p[i]; a += cross(p[i] - p[0], p[(i + 1) % 4] - p[0]); peri += normApprox(e);
        for (auto v : input) { require(cross(e, v.cast<long double>() - p[i]) >= -1e-11L * scale * scale, "box CCW containment"); }}
    require(std::abs(a / 2 - box.area) <= 2e-12L * box.area + 128 * std::numeric_limits<long double>::epsilon() * scale * scale, "vertex area", box.area, a / 2);
    require(near(peri, box.perimeter), "vertex perimeter", box.perimeter, peri);}
// Every edge against every vertex: perpendicular range [lo, hi], projection extent u, exact optima and the first edge attaining each.
struct EdgeOracle {
    vector<point> e; vector<lll> lo, hi, w, u; Fraction width, area, perimeter; int area_edge = -1, perimeter_edge = -1;
    explicit EdgeOracle(const vector<point> &h) {
        int n = int(h.size());
        for (int i = 0; i < n && n >= 3; ++i) {
            e.push_back(h[(i + 1) % n] - h[i]);
            lll l = 0, r = 0, xlo = scalar(e[i], h[0]), xhi = xlo;
            for (auto q : h) {
                lll s = det(e[i], q - h[i]); l = min(l, s); r = max(r, s);
                s = scalar(e[i], q); xlo = min(xlo, s); xhi = max(xhi, s);}
            lo.push_back(l); hi.push_back(r); w.push_back(r - l); u.push_back(xhi - xlo);
            Fraction fw{ulll(w[i]) * w[i], norm(i)}, fa = areaAt(i), fp{ulll(u[i] + w[i]) * (u[i] + w[i]), norm(i)};
            if (!i || fw < width) { width = fw; }
            if (!i || fa < area) { area = fa; area_edge = i; }
            if (!i || fp < perimeter) { perimeter = fp; perimeter_edge = i; }}}
    ulll norm(int i) const { return ulll(scalar(e[i], e[i])); }
    Fraction areaAt(int i) const { return {ulll(u[i]) * w[i], norm(i)}; }
};
bool same(long double a, long double b) { return a == b && std::signbit(a) == std::signbit(b); }
bool identical(const CaliperBoxApprox &a, const CaliperBoxApprox &b) {
    bool ok = a.exists == b.exists && a.edges == b.edges && same(a.area, b.area) && same(a.perimeter, b.perimeter);
    for (int k = 0; k < 4; ++k) { ok = ok && same(a.vertices[k].x, b.vertices[k].x) && same(a.vertices[k].y, b.vertices[k].y); }
    return ok;}
// Freeman-Shapira forms: bitwise the calipers' boxes, flush with the first edge of least exact objective, and the box oracle.
void verifyQuadratic(const EdgeOracle &o, const CaliperBoxApprox &ar, const CaliperBoxApprox &pr) {
    int n = int(input.size());
    auto qa = minimumAreaRectangleApproxQuadratic(input), qp = minimumPerimeterRectangleApproxQuadratic(input);
    require(identical(qa, ar), "minimumAreaRectangleApproxQuadratic bitwise equals minimumAreaRectangleApprox", ar.area, qa.area);
    require(identical(qp, pr), "minimumPerimeterRectangleApproxQuadratic bitwise equals minimumPerimeterRectangleApprox", pr.perimeter, qp.perimeter);
    if (n < 3) {
        long double small_perimeter = n == 2 ? 2 * distanceApprox(input[0], input[1]) : 0;
        for (auto box : {qa, qp}) {
            require(box.edges[0] == -1 && box.edges[1] == -1, "degenerate quadratic box has no supporting edge");
            for (int k = 0; k < 4 && n; ++k) { require(box.vertices[k] == input[n == 2 && (k == 1 || k == 2)].cast<long double>(), "degenerate quadratic box vertex layout"); }
            verifyBox(box, true, 0, small_perimeter);}
        return;}
    require(qa.edges[0] == o.area_edge && qa.edges[1] == -1, "quadratic area rectangle on the first edge of least exact area", o.area_edge, qa.edges[0]);
    require(qp.edges[0] == o.perimeter_edge && qp.edges[1] == -1, "quadratic perimeter rectangle on the first edge of least exact perimeter", o.perimeter_edge, qp.edges[0]);
    verifyBox(qa, true, o.area.value());
    verifyBox(qp, true, o.areaAt(o.perimeter_edge).value(), 2 * std::sqrt(o.perimeter.value()));}
long double verify(vector<point> p) {
    input = std::move(p); int n = int(input.size()); ConvexCalipers calipers(input);
    long double box = checkExpected(input, calipers, quadrature && n <= 64);
    auto diameter = calipers.diameter();
    if (!n) { require(diameter.first == -1 && diameter.second == -1 && diameter.squared == 0, "empty diameter"); }
    else {
        lll best = -1; pair<int, int> pair{-1, -1};
        for (int i = 0; i < n; ++i) { for (int j = i; j < n; ++j) {
            auto d = input[i] - input[j]; lll value = scalar(d, d);
            if (value > best) { best = value; pair = {i, j}; }}}
        require(lll(diameter.squared) == best && std::pair{diameter.first, diameter.second} == pair, "brute diameter with lexicographic ties");}
    auto pairs = calipers.antipodalPairs(); set<pair<int, int>> actual(pairs.begin(), pairs.end()), expected;
    require(actual.size() == pairs.size(), "antipodal unique pairs");
    if (n == 2) { expected.emplace(0, 1); }
    EdgeOracle oracle(input); const auto &e = oracle.e; const auto &w = oracle.w; const auto &u = oracle.u;
    Fraction min_width = oracle.width, min_area = oracle.area, min_perimeter = oracle.perimeter, min_para;
    for (int i = 0; i < n && n >= 3; ++i) { for (int j = 0; j < n; ++j) { for (int k = j + 1; k < n; ++k) {
        lll a = det(e[i], input[j] - input[i]), b = det(e[i], input[k] - input[i]), lo = oracle.lo[i], hi = oracle.hi[i];
        if ((a == lo && b == hi) || (a == hi && b == lo)) { expected.emplace(j, k); }}}}
    require(actual == expected, "all antipodal support events");
    bool found = false;
    for (int i = 0; i < n && n >= 3; ++i) { for (int j = i + 1; j < n; ++j) {
        lll d = det(e[i], e[j]); if (d < 0) { d = -d; } if (!d) { continue; }
        Fraction area{ulll(w[i]) * w[j], ulll(d)}; if (!found || area < min_para) { min_para = area; found = true; }}}
    auto width = calipers.minimumWidthApprox(); long double want_width = std::sqrt(min_width.value());
    require(near(width.width, want_width), "brute minimum width", want_width, width.width);
    if (n >= 3) {
        require(width.opposite >= 0 && width.edge >= 0, "width witnesses exist");
        require(det(e[width.edge], input[width.opposite] - input[width.edge]) == w[width.edge], "width support witness");}
    else {
        require(width.edge == (n ? 0 : -1) && width.opposite == (n == 2 ? 1 : n ? 0 : -1), "degenerate width witnesses");
        for (auto box : {calipers.minimumAreaRectangleApprox(), calipers.minimumPerimeterRectangleApprox(), calipers.minimumAreaParallelogramApprox()}) {
            require(box.edges[0] == -1 && box.edges[1] == -1, "degenerate box has no supporting edge");
            for (int k = 0; k < 4 && n; ++k) { require(box.vertices[k] == input[n == 2 && (k == 1 || k == 2)].cast<long double>(), "degenerate box vertex layout"); }}}
    require(calipers.n == n && calipers.p == input, "calipers keep the hull");
    require(int(calipers.e.size()) == (n >= 3 ? n : 0) && calipers.opposite.size() == calipers.e.size() && calipers.width.size() == calipers.e.size(), "edge fields hold one entry per edge from three points, none below");
    if (n >= 3) {
        auto point_box = calipers.degenerateBox();
        require(point_box.exists && point_box.edges[0] == -1 && point_box.edges[1] == -1 && point_box.area == 0 && point_box.perimeter == 0, "degenerateBox on three or more points is the first point's box");
        for (int k = 0; k < 4; ++k) { require(point_box.vertices[k] == input[0].cast<long double>(), "degenerateBox on three or more points repeats the first point"); }}
    long double small_perimeter = n == 2 ? 2 * distanceApprox(input[0], input[1]) : 0;
    auto ar = calipers.minimumAreaRectangleApprox(), pr = calipers.minimumPerimeterRectangleApprox(), pa = calipers.minimumAreaParallelogramApprox();
    verifyBox(ar, true, min_area.value(), n < 3 ? small_perimeter : -1);
    verifyBox(pr, true, n < 3 ? 0 : (Fraction{ulll(u[pr.edges[0]]) * w[pr.edges[0]], ulll(scalar(e[pr.edges[0]], e[pr.edges[0]]))}).value(), n < 3 ? small_perimeter : 2 * std::sqrt(min_perimeter.value()));
    verifyBox(pa, false, min_para.value(), n < 3 ? small_perimeter : -1);
    verifyQuadratic(oracle, ar, pr);
    require(box >= ar.area * (1 - BOX_TOL), "expected box area at least the minimum-area rectangle", ar.area, box);
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
        require(!(width_value < min_width) && !(min_width < width_value), "exact width optimum");}
    return box;}
// Independent all-pairs farthest pair: the first strictly larger distance in (i, j) order wins ties.
void verifyFarthest(const vector<point> &cloud) {
    input = cloud; int n = int(cloud.size());
    ConvexDiameter want;
    if (n == 1) { want = {0, 0, 0}; }
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            auto d = cloud[i] - cloud[j]; lll value = scalar(d, d);
            if (want.first < 0 || want.first == want.second || value > want.squared) { want = {i, j, value}; }}}
    auto before = cloud; auto got = farthestPair(cloud);
    require(got.first == want.first && got.second == want.second && got.squared == want.squared, "farthest pair over original indices", static_cast<long double>(want.squared), static_cast<long double>(got.squared));
    require(before == cloud, "farthest pair input preserved");
    auto naive = farthestPairNaive(cloud);
    require(naive.first == want.first && naive.second == want.second && naive.squared == want.squared, "farthestPairNaive over original indices", static_cast<long double>(want.squared), static_cast<long double>(naive.squared));}

int main(int argc, char **argv) {
    if (argc > 1 && string(argv[1]) == "--fractions") {
        auto read = [](string s) { ulll x = 0; for (char c : s) { x = x * 10 + c - '0'; } return x; };
        string a, b, c, d;
        while (std::cin >> a >> b >> c >> d) {
            auto x = read(a), y = read(b), z = read(c), w = read(d);
            for (auto v : product(x, w)) { std::cout << std::hex << std::setw(8) << std::setfill('0') << v; }
            std::cout << ' ';
            for (auto v : product(z, y)) { std::cout << std::hex << std::setw(8) << std::setfill('0') << v; }
            std::cout << ' ' << ConvexCalipers::ratioLess(x, y, z, w) << '\n';}
        return 0;}
    if (argc > 1 && string(argv[1]) == "--expected") {
        int n;
        while (std::cin >> n) {
            vector<point> h(n);
            for (auto &q : h) { std::cin >> q.x >> q.y; }
            std::cout << std::setprecision(21) << ConvexCalipers(h).expectedBoxAreaApprox() << '\n';}
        return 0;}
    string mode = "full", invalid;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid = argv[++i]; }}
    if (!invalid.empty()) {
        if (invalid == "denominator") { return ConvexCalipers::ratioLess(0, 0, 0, 1); }
        if (invalid == "farthest-bound") { return farthestPair(vector<point>{{0, 0}, {-1000000001, 0}}).first; }
        if (invalid == "farthest-naive-bound") { return farthestPairNaive(vector<point>{{0, 0}, {0, 1000000001}}).first; }
        int form = 0;
        for (auto [prefix, f] : {pair{string("area-quadratic-"), 1}, pair{string("perimeter-quadratic-"), 2}}) {
            if (invalid.starts_with(prefix)) { invalid = invalid.substr(prefix.size()); form = f; }}
        vector<point> p = invalid == "clockwise" ? vector<point>{{0,0},{0,1},{1,0}} : invalid == "collinear" ? vector<point>{{0,0},{1,0},{2,0}} : invalid == "duplicate" ? vector<point>{{0,0},{0,0}} : vector<point>{{1000000001,0}};
        if (form == 1) { return minimumAreaRectangleApproxQuadratic(p).exists; }
        if (form == 2) { return minimumPerimeterRectangleApproxQuadratic(p).exists; }
        ConvexCalipers c(p); return c.n == -1;}
    std::mt19937_64 rng(seed);
    phase = "regressions and extreme coordinates";
    for (auto p : vector<vector<point>>{{},{{0,0}},{{0,0},{3,4}},{{0,0},{4,0},{0,3}},{{-2,-1},{2,-1},{2,1},{-2,1}},{{0,0},{7,2},{9,6},{2,4}},{{-1000000000,-1000000000},{1000000000,-1000000000},{1000000000,1000000000},{-1000000000,1000000000}},{{-1000000000,-999999999},{999999999,999999998},{1000000000,1000000000}},{{0,0},{1,-4},{4,-5},{7,-4},{9,0},{6,6},{1,5}}}) { verify(convexHull(p)); }
    phase = "all small-grid subsets"; int side = mode == "stress" ? 4 : 3, masks = mode == "quick" ? 64 : 1 << (side * side);
    for (int mask = 0; mask < masks; ++mask) {
        vector<point> p;
        for (int k = 0; k < side * side; ++k) { if (mask >> k & 1) { p.push_back({k % side, k / side}); } }
        p = convexHull(p); long double box = 0;
        for (int i = 0; i < max(1, int(p.size())); ++i) {
            quadrature = i == 0; long double got = verify(p);
            if (i) { sameAs(box, got, "expected box area over every cyclic start"); } else { box = got; }
            if (!p.empty()) { std::rotate(p.begin(), p.begin() + 1, p.end()); }}}
    quadrature = true;
    phase = "random hulls and cyclic/rigid/scale metamorphisms";
    int rounds = mode == "quick" ? 50 : mode == "full" ? 1000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(3 + rng() % 45); lng c = t % 3 ? 30 : 1000000000;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, lng(rng() % (2 * c + 1)) - c}); }
        p = convexHull(p); long double box = verify(p);
        if (!p.empty()) { std::rotate(p.begin(), p.begin() + rng() % p.size(), p.end()); sameAs(box, verify(p), "expected box area cyclic start"); }
        for (auto &q : p) { q = {-q.y, q.x}; } sameAs(box, verify(p), "expected box area quarter turn");
        if (c == 30) { for (auto &q : p) { q = {q.x * 7 + 999999000, q.y * 7 - 999999000}; } sameAs(49 * box, verify(p), "expected box area translation and 7x scale"); }}
    phase = "thin high-dynamic-range hulls";
    for (int t = 0; t < rounds / 4; ++t) {
        vector<point> p;
        for (int k = 0; k < 30; ++k) {
            lng x = lng(rng() % 1999999901) - 999999950;
            p.push_back({x, x + lng(rng() % 61) - 30});}
        verify(convexHull(p));}
    phase = "expected box area known answers, needles, tiny triangles and invariances";
    // Rectangle a x b: E = ab + (a^2 + b^2) / pi since |sin t * cos t| averages 1 / pi; a square of side s gives s^2 * (1 + 2 / pi).
    for (auto [a, b] : vector<pair<lng, lng>>{{1, 1}, {2, 3}, {7, 1000}, {1, 2000000000}, {2000000000, 1}, {1999999999, 2000000000}, {2000000000, 2000000000}}) {
        lng o = -1000000000; vector<point> r{{o, o}, {o + a, o}, {o + a, o + b}, {o, o + b}};
        long double x = static_cast<long double>(a), y = static_cast<long double>(b), want = x * y + (x * x + y * y) / PI;
        for (int i = 0; i < 4; ++i) { sameAs(want, verify(r), "rectangle a x b known answer"); std::rotate(r.begin(), r.begin() + 1, r.end()); }
        for (auto &q : r) { q = {-q.y, q.x}; } sameAs(want, verify(r), "rectangle known answer after a quarter turn");}
    for (auto s : vector<vector<point>>{{{0, 0}, {1, 0}}, {{0, 0}, {3, 4}}, {{-1000000000, -1000000000}, {1000000000, 1000000000}}, {{1000000000, -1000000000}, {-1000000000, 999999999}}}) {
        long double want = static_cast<long double>(diameter2(s)) / PI;
        sameAs(want, verify(s), "segment L^2 / pi"); swap(s[0], s[1]); sameAs(want, verify(s), "segment L^2 / pi reversed");}
    {
        long double tiny = verify({{0, 0}, {1, 0}, {0, 1}});
        sameAs(tiny, verify({{999999999, -1000000000}, {1000000000, -1000000000}, {999999999, -999999999}}), "tiny triangle translated to the corner");
        sameAs(tiny, verify({{1000000000, 999999999}, {1000000000, 1000000000}, {999999999, 999999999}}), "tiny triangle quarter turn and translation");
        for (auto needle : vector<vector<point>>{{{-1000000000, -1000000000}, {1000000000, 1000000000}, {1000000000, 999999999}},
                {{-1000000000, -1000000000}, {999999999, 1000000000}, {1000000000, 1000000000}, {-999999999, -1000000000}},
                {{-1000000000, 0}, {1000000000, 1}, {999999999, 1}}, {{-3, 0}, {1000000000, 0}, {-3, 1}}}) {
            auto h = convexHull(needle); long double box = verify(h);
            for (auto &q : h) { q = {-q.y, q.x}; } sameAs(box, verify(h), "needle quarter turn");}
        auto circle = [](lng r) {
            vector<point> c;
            for (lng x = -r; x <= r; ++x) {
                lng t = r * r - x * x, y = lng(std::sqrt(static_cast<double>(t)));
                while (y * y > t) { --y; }
                while ((y + 1) * (y + 1) <= t) { ++y; }
                if (y * y == t) { c.push_back({x, y}); c.push_back({x, -y}); }}
            return convexHull(c);};
        for (auto [r, count] : mode == "quick" ? vector<pair<lng, int>>{{32045, 324}} : vector<pair<lng, int>>{{32045, 324}, {1185665, 972}}) {
            auto h = circle(r); input = h; int n = int(h.size());
            require(n == count, "lattice circle vertex count", count, n);
            long double box = checkExpected(h, ConvexCalipers(h), true), gap = 0, side = 2 * static_cast<long double>(r);
            for (int i = 0; i < n; ++i) {
                point a = h[i], b = h[(i + 1) % n];
                gap = max(gap, std::atan2(static_cast<long double>(det(a, b)), static_cast<long double>(scalar(a, b))));}
            long double inner = side * std::cos(gap / 2);
            require(box >= inner * inner * (1 - BOX_TOL) && box <= side * side * (1 + BOX_TOL), "lattice circle between inscribed and circumscribed squares", side * side, box);
            std::rotate(h.begin(), h.begin() + n / 3, h.end());
            sameAs(box, checkExpected(h, ConvexCalipers(h), false), "lattice circle cyclic start");
            lng k = 1000000000 / r; for (auto &q : h) { q = q * k; } input = h;
            sameAs(static_cast<long double>(k * k) * box, checkExpected(h, ConvexCalipers(h), false), "lattice circle scaled near 10^9");}}
    phase = "fraction comparator full 128-bit random operands";
    for (int i = 0; i < rounds * 10; ++i) {
        ulll a = ulll(rng()) << 64 | rng(), b = ulll(rng()) << 64 | rng(), c = ulll(rng()) << 64 | rng(), d = ulll(rng()) << 64 | rng();
        b |= 1; d |= 1;
        require(ConvexCalipers::ratioLess(a, b, c, d) == (product(a, d) < product(c, b)), "256-bit schoolbook fraction comparison");}
    phase = "farthest pair over clouds with duplicates, collinear points and ties";
    for (auto p : vector<vector<point>>{{}, {{5, 5}}, {{5, 5}, {5, 5}}, {{1, 1}, {1, 1}, {1, 1}}, {{0, 0}, {2, 0}, {1, 0}, {0, 0}, {2, 0}},
        {{0, 0}, {1, 0}, {1, 1}, {0, 1}}, {{1, 1}, {0, 0}, {1, 0}, {0, 1}, {1, 1}}, {{-1000000000, -1000000000}, {1000000000, 1000000000}, {0, 0}}}) { verifyFarthest(p); }
    for (int t = 0; t < rounds; ++t) {
        vector<point> p; int n = int(rng() % 40); lng c = t % 4 == 3 ? 1000000000 : t % 4 == 2 ? 1 : 6;
        for (int k = 0; k < n; ++k) { p.push_back({lng(rng() % (2 * c + 1)) - c, t % 5 == 0 ? 3 : lng(rng() % (2 * c + 1)) - c}); }
        if (n && t % 3 == 0) { for (int k = 0; k < n / 2; ++k) { p.push_back(p[rng() % p.size()]); } }
        verifyFarthest(p); std::shuffle(p.begin(), p.end(), rng); verifyFarthest(p);}
    phase = "int coordinates";
    ConvexCalipers small(vector<Point2<int>>{{0,0},{3,0},{0,4}}); require(small.diameter().squared == 25, "32-bit input");
    auto far32 = farthestPair(vector<Point2<int>>{{0, 0}, {3, 0}, {0, 4}, {3, 0}});
    require(far32.first == 1 && far32.second == 2 && far32.squared == 25, "32-bit farthest pair");
    auto far32n = farthestPairNaive(vector<Point2<int>>{{0, 0}, {3, 0}, {0, 4}, {3, 0}});
    auto far8n = farthestPairNaive(vector<Point2<int8_t>>{{-128, -128}, {127, 127}, {127, 127}});
    require(far32n.first == 1 && far32n.second == 2 && far32n.squared == 25, "32-bit farthestPairNaive");
    require(far8n.first == 0 && far8n.second == 1 && far8n.squared == 2 * 255 * 255, "8-bit farthestPairNaive");
    // Right triangles: all three edges tie in area (first edge 0); edges 0 and 2 tie in perimeter, the hypotenuse loses.
    vector<Point2<int>> tri32{{0, 0}, {3, 0}, {0, 4}};
    vector<Point2<int8_t>> tri8{{-128, -128}, {127, -128}, {-128, 127}};
    auto qa32 = minimumAreaRectangleApproxQuadratic(tri32), qp32 = minimumPerimeterRectangleApproxQuadratic(tri32);
    auto qa8 = minimumAreaRectangleApproxQuadratic(tri8), qp8 = minimumPerimeterRectangleApproxQuadratic(tri8);
    require(identical(qa32, small.minimumAreaRectangleApprox()) && identical(qp32, small.minimumPerimeterRectangleApprox()), "32-bit quadratic rectangles bitwise equal the calipers'");
    require(identical(qa8, ConvexCalipers(tri8).minimumAreaRectangleApprox()) && identical(qp8, ConvexCalipers(tri8).minimumPerimeterRectangleApprox()), "8-bit quadratic rectangles bitwise equal the calipers'");
    require(qa32.edges[0] == 0 && qa32.area == 12 && qa32.perimeter == 14 && qp32.edges[0] == 0 && qp32.area == 12 && qp32.perimeter == 14, "32-bit quadratic rectangles known answer", 12, qa32.area);
    require(qa8.edges[0] == 0 && qa8.area == 65025 && qa8.perimeter == 1020 && qp8.edges[0] == 0 && qp8.perimeter == 1020, "8-bit quadratic rectangles known answer", 65025, qa8.area);
    phase = "large strict convex parabola";
    int half = mode == "quick" ? 50 : mode == "full" ? 5000 : 15000; vector<point> p;
    for (int x = -half; x <= half; ++x) { p.push_back({x, lng(x) * x}); }
    input = p; ConvexCalipers large(p); auto diam = large.diameter();
    lll expected = max(lll(4) * half * half, lll(half) * half + lll(half) * half * half * half);
    require(lll(diam.squared) == expected, "large parabola diameter");
    auto ar = large.minimumAreaRectangleApprox(), pr = large.minimumPerimeterRectangleApprox(), pa = large.minimumAreaParallelogramApprox();
    require(ar.exists && pr.exists && pa.exists && pa.area <= ar.area * (1 + 1e-12L) && pr.perimeter <= ar.perimeter * (1 + 1e-12L), "large linear sweeps");
    require(large.antipodalPairs().size() <= 4 * p.size(), "linear antipodal output bound");
    long double box = checkExpected(p, large, p.size() <= 1000, expected);
    require(box >= ar.area * (1 - BOX_TOL), "large expected box area at least the minimum-area rectangle", ar.area, box);
    std::rotate(p.begin(), p.begin() + half, p.end()); sameAs(box, ConvexCalipers(p).expectedBoxAreaApprox(), "large expected box area cyclic start");
    for (auto &q : p) { q = {-q.y, q.x}; } sameAs(box, ConvexCalipers(p).expectedBoxAreaApprox(), "large expected box area quarter turn");
    if (mode != "quick") {
        phase = "Freeman-Shapira rectangles on a 3001-vertex parabola, two starts";
        vector<point> h;
        for (int x = -1500; x <= 1500; ++x) { h.push_back({x, lng(x) * x}); }
        for (int start : {0, 1000}) {
            std::rotate(h.begin(), h.begin() + start, h.end()); input = h; ConvexCalipers c(h);
            verifyQuadratic(EdgeOracle(h), c.minimumAreaRectangleApprox(), c.minimumPerimeterRectangleApprox());}}
    std::cout << "PASS rotatingcalipers seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
