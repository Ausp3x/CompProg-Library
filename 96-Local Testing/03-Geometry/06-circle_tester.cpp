#include "../../03-Geometry/06-circle.hpp"

static ulng seed = 20260927;
static std::string phase, input;
static int checks = 0;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what
              << " input=" << input << " expected=true actual=false\n"; std::exit(1); }
bool near(long double a, long double b, long double scale = 1) {
    return std::abs(a - b) <= 2e-12L * max({scale, std::abs(a), std::abs(b)}); }
long double length(dpoint p) { return std::sqrt(p.x * p.x + p.y * p.y); }
void record(CircleApprox a, CircleApprox b) {
    std::ostringstream s; s << std::setprecision(22) << "a=(" << a.c.x << ',' << a.c.y << ',' << a.r
        << ") b=(" << b.c.x << ',' << b.c.y << ',' << b.r << ')'; input = s.str(); }
void verifyIntersections(CircleApprox a, CircleApprox b, int expected) {
    record(a, b); auto z = circleIntersectionApprox(a, b);
    require(z.count == expected, "circle intersection count (exact squared-distance oracle)");
    auto w = circleIntersectionApprox(b, a); require(w.count == z.count, "intersection symmetry");
    for (int i = 0; i < z.count; ++i) {
        auto p = z.points[i]; require(near(length(p - a.c), a.r) && near(length(p - b.c), b.r), "two boundary residuals");
        bool found = false;
        for (int j = 0; j < w.count; ++j) { found |= near(length(p - w.points[j]), 0); }
        require(found, "swapped point set"); }
    if (z.count == 2) { require(z.points[0] != z.points[1], "distinct intersection points"); }
}
void verifyTangents(CircleApprox a, CircleApprox b, bool inner, int expected) {
    record(a, b); input += inner ? " inner=true" : " inner=false";
    auto z = commonTangentsApprox(a, b, inner); require(z.count == expected, "common tangent count");
    for (int i = 0; i < z.count; ++i) {
        auto t = z.lines[i];
        require(near(length(t.a - a.c), a.r) && near(length(t.b - b.c), b.r), "contact on boundaries");
        require(near(length(t.direction), 1), "unit tangent direction");
        require(near(dot(t.a - a.c, t.direction), 0) && near(dot(t.b - b.c, t.direction), 0), "tangent perpendicular to radii");
        require(near(cross(t.b - t.a, t.direction), 0), "contacts share line");
        if (a.r && b.r) {
            require(near(dot(t.a - a.c, t.b - b.c), (inner ? -1 : 1) * a.r * b.r), "inner/outer center sides"); }}
    if (z.count == 2) {
        auto p = z.lines[0], q = z.lines[1];
        require(!near(cross(p.direction, q.direction), 0) || !near(cross(q.a - p.a, p.direction), 0), "distinct tangent lines"); }
}
void basic() {
    phase = "degenerate and measures";
    CircleApprox z, c({1, 2}, 5); long double pi = std::numbers::pi_v<long double>;
    require(z.c == dpoint{} && z.r == 0 && z.contains(dpoint{}) && z.onBoundary(dpoint{}), "default singleton disk");
    require(c.locate({1, 2}) == -1 && c.locate({6, 2}) == 0 && c.locate({7, 2}) == 1, "inside/boundary/outside");
    require(c.contains(dpoint{4, 6}) && c.onBoundary({4, 6}) && !c.onBoundary(c.c), "closed versus boundary");
    require(c.locate({6.01L, 2}, .02L) == 0 && c.contains(dpoint{6.01L, 2}, .02L), "explicit length tolerance");
    require(c.contains(CircleApprox({4, 2}, 2)) && !c.contains(CircleApprox({4, 2}, 3)), "closed disk containment");
    require(c.contains(CircleApprox({4, 2}, 2.01L), .02L), "disk containment tolerance");
    require(near(c.area(), 25 * pi) && near(c.perimeter(), 10 * pi), "disk area and boundary length");
    require(near(c.arcLength(-3 * pi), 15 * pi) && near(c.sectorArea(-3 * pi), -37.5L * pi), "signed sweep and repeated revolutions");
    require(c.arcLength(0) == 0 && c.sectorArea(0) == 0 && c.segmentArea(0) == 0, "zero sweep");
    require(near(c.segmentArea(pi), c.area() / 2) && near(c.segmentArea(2 * pi), c.area()), "semicircle/full-circle segments");
    require(near(c.segmentArea(pi / 2), 25 * (pi / 4 - .5L)), "quarter circle segment");
    long double t = 1e-7L;
    require(std::abs(c.segmentArea(t) / (25 * t * t * t / 12) - 1) < 1e-14L, "small-cap cancellation regression");
    verifyIntersections(z, z, 1); verifyIntersections(c, c, -1);
    verifyIntersections(c, CircleApprox(c.c, 4), 0);
    verifyTangents(z, z, false, -1); verifyTangents(z, z, true, -1);
    verifyTangents(c, c, false, -1); verifyTangents(c, c, true, 0);
    require(pointTangentsApprox(c, c.c).count == 0, "interior point tangents");
    require(pointTangentsApprox(c, {6, 2}).count == 1 && pointTangentsApprox(c, {7, 2}).count == 2, "boundary/exterior point tangents");
    require(pointTangentsApprox(z, {}).count == -1 && pointTangentsApprox(z, {1, 1}).count == 1, "point-circle tangent lines");
    auto a = circleCentersApprox({0, 0}, {6, 0}, 5);
    require(a.count == 2, "two radius-constrained circles");
    for (int i = 0; i < a.count; ++i) { require(a.points[i].x == 3 && std::abs(a.points[i].y) == 4, "radius-constrained centers"); }
    require(circleCentersApprox({0, 0}, {6, 0}, 3).count == 1 && circleCentersApprox({0, 0}, {6, 0}, 2).count == 0, "diameter/impossible centers");
    require(circleCentersApprox({1, 1}, {1, 1}, 2).count == -1 && circleCentersApprox({1, 1}, {1, 1}, 0).count == 1, "repeated center constraints");
    CircleApprox out({8, 9}, 10);
    require(!circumcircleApprox({}, {1, 1}, {2, 2}, out) && out.c == dpoint(8, 9) && out.r == 10, "collinear circumcircle unchanged");
    require(!incircleApprox({}, {}, {1, 1}, out) && out.c == dpoint(8, 9) && out.r == 10, "repeated incircle unchanged");
    require(circumcircleApprox({}, {4, 0}, {0, 3}, out) && out.c == dpoint(2, 1.5L) && out.r == 2.5L, "3-4-5 circumcircle");
    require(incircleApprox({}, {4, 0}, {0, 3}, out) && out.c == dpoint(1, 1) && out.r == 1, "3-4-5 incircle");
    require(diskOverlapAreaApprox(c, c) == c.area() && diskOverlapAreaApprox(c, CircleApprox(c.c, 2)) == 4 * pi, "coincident/nested disks");
    require(diskOverlapAreaApprox(c, z) == 0 && diskOverlapAreaApprox(z, z) == 0, "point disk area");
    require(diskOverlapAreaApprox({{}, 1}, {{2, 0}, 1}) == 0 && diskOverlapAreaApprox({{}, 1}, {{3, 0}, 1}) == 0, "tangent/disjoint disk area");
    require(near(diskOverlapAreaApprox({{}, 1}, {{1, 0}, 1}), 2 * pi / 3 - std::sqrt(3.L) / 2), "unit lens closed form");
}
void exhaustive(int bound) {
    phase = "exhaustive integer circle classification";
    for (int x = -bound; x <= bound; ++x) { for (int y = -bound; y <= bound; ++y) {
        for (int r = 0; r <= bound; ++r) { for (int s = 0; s <= bound; ++s) {
            int q = x * x + y * y, lo = (r - s) * (r - s), hi = (r + s) * (r + s);
            CircleApprox a({0, 0}, r), b({x, y}, s);
            int count = q == 0 ? (r != s ? 0 : r ? -1 : 1) : q < lo || q > hi ? 0 : q == lo || q == hi ? 1 : 2;
            verifyIntersections(a, b, count);
            for (bool inner : {false, true}) {
                int v = inner ? r + s : r - s, tangent_count = q == 0 ? (v ? 0 : -1) :
                    q < v * v ? 0 : q == v * v || (r == 0 && s == 0) ? 1 : 2;
                verifyTangents(a, b, inner, tangent_count); }
            require(a.contains(b) == (r >= s && q <= (r - s) * (r - s)), "exact disk containment oracle"); }}}}
    phase = "exhaustive integer circle-line classification";
    for (int ax = -bound; ax <= bound; ++ax) { for (int ay = -bound; ay <= bound; ++ay) {
        for (int dx = -2; dx <= 2; ++dx) { for (int dy = -2; dy <= 2; ++dy) {
            if (dx == 0 && dy == 0) { continue; }
            for (int r = 0; r <= bound; ++r) {
                dpoint a{ax, ay}, b{ax + dx, ay + dy}; CircleApprox c({}, r);
                int q = dx * ay - dy * ax, disc = r * r * (dx * dx + dy * dy) - q * q;
                auto z = circleLineIntersectionApprox(c, a, b);
                input = "a=" + std::to_string(ax) + "," + std::to_string(ay) + " d=" + std::to_string(dx) + "," + std::to_string(dy) + " r=" + std::to_string(r);
                require(z.count == (disc < 0 ? 0 : disc == 0 ? 1 : 2), "line count from integer discriminant");
                for (int i = 0; i < z.count; ++i) {
                    require(near(length(z.points[i]), r) && near(cross(z.points[i] - a, b - a), 0), "line/circle residuals"); } }}}}}
}
// Independent area oracle: midpoint integration of the smaller vertical disk
// sections, with a conservative O(sqrt(dx)) endpoint-error allowance.
long double overlapQuadrature(long double r, long double s, long double d, int n) {
    long double l = max(-r, d - s), h = min(r, d + s);
    if (l >= h) { return 0; }
    long double sum = 0, dx = (h - l) / n;
    for (int i = 0; i < n; ++i) {
        long double x = l + (i + .5L) * dx;
        sum += 2 * min(std::sqrt(max(0.L, r * r - x * x)), std::sqrt(max(0.L, s * s - (x - d) * (x - d)))); }
    return sum * dx; }
void randomChecks(int rounds, bool quick) {
    std::mt19937_64 rng(seed); auto coord = [&]() { return (int(rng() % 20001) - 10000) / 100.L; };
    phase = "random triangle circles and similarities";
    for (int t = 0; t < rounds; ++t) {
        dpoint a{coord(), coord()}, b{coord(), coord()}, c{coord(), coord()};
        std::ostringstream description; description << std::setprecision(22) << "round=" << t
            << " triangle=(" << a.x << ',' << a.y << ")(" << b.x << ',' << b.y << ")(" << c.x << ',' << c.y << ')';
        input = description.str(); CircleApprox out;
        long double det = cross(b - a, c - a); if (std::abs(det) < 1e-3L) { continue; }
        require(circumcircleApprox(a, b, c, out), "noncollinear circumcircle exists");
        require(near(length(a - out.c), out.r) && near(length(b - out.c), out.r) && near(length(c - out.c), out.r), "equidistant circumcircle");
        CircleApprox other; require(circumcircleApprox(c, b, a, other), "reversed circumcircle exists");
        require(near(length(other.c - out.c), 0, out.r) && near(other.r, out.r), "circumcircle permutation");
        require(incircleApprox(a, b, c, out), "incircle exists");
        for (auto [p, q] : vector<pair<dpoint, dpoint>>{{a, b}, {b, c}, {c, a}}) {
            require(near(std::abs(cross(out.c - p, q - p)) / length(q - p), out.r), "equal incircle side distances");
            require(cross(q - p, out.c - p) * det > 0, "incenter lies inside triangle"); }
        require(incircleApprox(b, a, c, other) && near(length(other.c - out.c), 0) && near(other.r, out.r), "incircle permutation");
        CircleApprox disk({coord(), coord()}, 1 + std::abs(coord())); dpoint p{coord(), coord()};
        auto ts = pointTangentsApprox(disk, p);
        for (int i = 0; i < ts.count; ++i) {
            auto q = ts.lines[i]; require(near(length(q.a - disk.c), disk.r) && near(dot(q.a - disk.c, p - q.a), 0, disk.r * disk.r), "point tangent right triangle"); }
        // Algebraic quadratic oracle, separate from the geometric projection.
        dpoint v = b - a, w = a - disk.c; long double aa = dot(v, v), bb = dot(w, v);
        long double cc = dot(w, w) - disk.r * disk.r, disc = bb * bb - aa * cc;
        if (aa > 0 && std::abs(disc) > 1e-9L * max(1.L, bb * bb)) {
            auto q = circleLineIntersectionApprox(disk, a, b), rev = circleLineIntersectionApprox(disk, b, a);
            require(q.count == (disc > 0 ? 2 : 0) && q.count == rev.count, "random line quadratic count and reversal");
            for (int i = 0; i < q.count; ++i) {
                require(near(length(q.points[i] - disk.c), disk.r) && near(cross(q.points[i] - a, v), 0, aa), "random line boundary residuals"); }}
    }
    phase = "disk lens independent quadrature";
    int n = quick ? 10000 : 100000, samples = quick ? 15 : 75;
    for (int t = 0; t < samples; ++t) {
        long double r = 1 + (rng() % 100) / 10.L, s = 1 + (rng() % 100) / 10.L;
        long double d = std::abs(r - s) + (r + s - std::abs(r - s)) * (.01L + .98L * (rng() % 1000) / 1000);
        CircleApprox a({}, r), b({d, 0}, s); record(a, b);
        long double area = diskOverlapAreaApprox(a, b), numeric = overlapQuadrature(r, s, d, n);
        require(std::abs(area - numeric) <= 8e-6L * max(1.L, min(a.area(), b.area())), "lens versus independent vertical-slice integral");
        require(near(area, diskOverlapAreaApprox(b, a)), "lens symmetry");
        CircleApprox aa({3, -4}, r * 2), bb({3, -4 + 2 * d}, s * 2);
        require(near(diskOverlapAreaApprox(aa, bb), area * 4), "lens rotation/translation/scaling");
        require(area > 0 && area <= min(a.area(), b.area()), "lens bounds"); }
    phase = "tiny/huge scale and near tangency";
    for (long double s : {1e-100L, 1e-8L, 1.L, 1e8L, 1e100L}) {
        CircleApprox a({}, 5 * s), b({6 * s, 0}, 5 * s); auto q = circleIntersectionApprox(a, b);
        require(q.count == 2, "scaled intersection count");
        for (int i = 0; i < 2; ++i) { require(near(q.points[i].x / s, 3) && near(std::abs(q.points[i].y) / s, 4), "scaled intersection coordinates"); }
        CircleApprox c; require(circumcircleApprox({}, {4 * s, 0}, {0, 3 * s}, c) && near(c.r / s, 2.5L), "scaled circumcircle");
        require(incircleApprox({}, {4 * s, 0}, {0, 3 * s}, c) && near(c.r / s, 1), "scaled incircle"); }
    for (long double e : {1e-4L, 1e-8L, 1e-12L}) {
        CircleApprox a({}, 1), b({2 - e, 0}, 1); long double area = diskOverlapAreaApprox(a, b);
        require(area > 0 && std::abs(area / (4 * e * std::sqrt(e) / 3) - 1) < max(1e-6L, e), "small external lens asymptotic");
        require(circleIntersectionApprox(a, b).count == 2 && circleIntersectionApprox(a, CircleApprox({2 + e, 0}, 1)).count == 0, "near external tangency sides"); }
}
void invalid(const string &name) {
    long double inf = std::numeric_limits<long double>::infinity(), nan = std::numeric_limits<long double>::quiet_NaN();
    CircleApprox c({}, 1);
    if (name == "negative-radius") { (void)CircleApprox({}, -1); }
    if (name == "infinite-center") { (void)CircleApprox({inf, 0}, 1); }
    if (name == "nan-radius") { (void)CircleApprox({}, nan); }
    if (name == "same-line-points") { (void)circleLineIntersectionApprox(c, {}, {}); }
    if (name == "negative-tolerance") { (void)c.locate({}, -1); }
    if (name == "nan-point") { (void)c.locate({nan, 0}); }
    if (name == "infinite-sweep") { (void)c.arcLength(inf); }
    if (name == "negative-segment") { (void)c.segmentArea(-1); }
    if (name == "large-segment") { (void)c.segmentArea(7); }
    std::exit(2); }
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    basic(); exhaustive(mode == "quick" ? 2 : mode == "full" ? 5 : 8);
    randomChecks(mode == "quick" ? 80 : mode == "full" ? 1600 : 16000, mode == "quick");
    std::cout << "PASS circle mode=" << mode << " seed=" << seed << " checks=" << checks << '\n'; }
