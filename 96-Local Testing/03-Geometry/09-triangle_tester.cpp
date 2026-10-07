#include "../../03-Geometry/09-triangle.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
static const long double EPS = std::numeric_limits<long double>::epsilon();
string show(lll n) {
    if (!n) { return "0"; }
    bool neg = n < 0; ulll u = neg ? ulll(0) - ulll(n) : ulll(n); string s;
    while (u) { s += char('0' + u % 10); u /= 10; }
    if (neg) { s += '-'; }
    reverse(s.begin(), s.end()); return s;}
string show(long double x) { std::ostringstream s; s << std::setprecision(24) << x; return s.str(); }
string show(bool x) { return x ? "true" : "false"; }
string show(point p) { return "(" + show(lll(p.x)) + "," + show(lll(p.y)) + ")"; }
string show(dpoint p) { return "(" + show(p.x) + "," + show(p.y) + ")"; }
void require(bool ok, const string &operation, const string &want = "true", const string &got = "false") {
    ++checks; if (ok) { return; }
    throw std::runtime_error(context + " operation=" + operation + " expected=" + want + " actual=" + got);}
// Residuals include subtraction at the triangle's coordinate and output scale; Heron is purely relative.
void close(long double got, long double want, const string &operation, long double scale = 1) {
    long double error = 4e-13L * max({scale, std::abs(got), std::abs(want)});
    require(std::isfinite(got) && std::abs(got - want) <= error, operation, show(want), show(got));}
void closePoint(dpoint got, dpoint want, const string &operation, long double scale = 1) {
    close(got.x, want.x, operation + " x", scale); close(got.y, want.y, operation + " y", scale);}
void relative(long double got, long double want, long double tolerance, const string &operation) {
    require(std::isfinite(got) && std::abs(got - want) <= tolerance * std::abs(want), operation, show(want), show(got));}
long double length(dpoint p) { return std::hypot(p.x, p.y); }
lll readWide(istream &in) {
    string s; in >> s; bool neg = !s.empty() && s[0] == '-'; lll x = 0;
    for (int i = neg; i < int(s.size()); ++i) { x = 10 * x + s[i] - '0'; }
    return neg ? -x : x;}
// Expanded shoelace identity, independent from local edge-vector cross.
lll det(point a, point b, point c) {
    return lll(a.x) * b.y + lll(b.x) * c.y + lll(c.x) * a.y
         - lll(a.y) * b.x - lll(b.y) * c.x - lll(c.y) * a.x;}
// Law of cosines on sorted squared sides; 2 marks zero area.
int kindOracle(point a, point b, point c) {
    if (det(a, b, c) == 0) { return 2; }
    auto sq = [](point p, point q) { lll x = lll(p.x) - q.x, y = lll(p.y) - q.y; return x * x + y * y; };
    array<lll, 3> s{sq(a, b), sq(b, c), sq(c, a)}; sort(s.begin(), s.end());
    return (s[0] + s[1] > s[2]) - (s[0] + s[1] < s[2]);}
void exact(Triangle2 t, point p, lll d, array<lll, 3> w, int kind) {
    context = "triangle=" + show(t.a) + show(t.b) + show(t.c) + " p=" + show(p);
    require(t.area2() == d, "exact signed area2", show(d), show(t.area2()));
    close(t.areaApprox(), std::abs(static_cast<long double>(d)) / 2, "exact approximate unsigned area");
    if (kind != 2) { require(t.angleKind() == kind, "exact angle kind", std::to_string(kind), std::to_string(t.angleKind())); }
    auto center = t.centroid();
    require(center.d > 0 && 3 * center.x == (lll(t.a.x) + t.b.x + t.c.x) * center.d &&
        3 * center.y == (lll(t.a.y) + t.b.y + t.c.y) * center.d, "exact rational centroid including degeneracy");
    Barycentric2 out; out.w = {7, 11, 13}; out.d = 31; auto before = out;
    require(t.barycentric(p, out) == bool(d), "exact barycentric existence");
    auto floating = t.approx(); array<long double, 3> floating_weights{7, 11, 13};
    require(floating.area2() == static_cast<long double>(d), "floating signed area on exact binary integer corpus");
    require(floating.barycentric(p.cast<long double>(), floating_weights) == bool(d), "floating barycentric existence on exact corpus");
    if (!d) {
        require(out.w == before.w && out.d == before.d, "exact no-answer leaves barycentric unchanged");
        require(floating_weights == array<long double, 3>{7, 11, 13}, "floating no-answer leaves barycentric unchanged"); return;}
    require(out.d > 0 && out.w[0] + out.w[1] + out.w[2] == out.d, "positive homogeneous denominator and partition of unity");
    auto approximate = out.approx();
    for (int i = 0; i < 3; ++i) {
        require(out.w[i] * d == w[i] * out.d, "exact barycentric independent determinant " + std::to_string(i));
        close(approximate[i], static_cast<long double>(w[i]) / static_cast<long double>(d), "barycentric approximate weight");
        close(floating_weights[i], static_cast<long double>(w[i]) / static_cast<long double>(d), "floating barycentric independent rational weight");}
    require(out.w[0] * t.a.x + out.w[1] * t.b.x + out.w[2] * t.c.x == out.d * p.x &&
        out.w[0] * t.a.y + out.w[1] * t.b.y + out.w[2] * t.c.y == out.d * p.y, "exact homogeneous reconstruction");}
void exhaustive(const string &mode) {
    int bound = mode == "stress" ? 2 : 1, query = mode == "quick" ? 1 : 2; lng cases = 0;
    vector<point> vertices, queries;
    for (int x = -bound; x <= bound; ++x) { for (int y = -bound; y <= bound; ++y) { vertices.push_back({x, y}); }}
    for (int x = -query; x <= query; ++x) { for (int y = -query; y <= query; ++y) { queries.push_back({x, y}); }}
    for (point a : vertices) { for (point b : vertices) { for (point c : vertices) { for (point p : queries) {
        exact({a, b, c}, p, det(a, b, c), {det(p, b, c), det(a, p, c), det(a, b, p)}, kindOracle(a, b, c)); ++cases;}}}}
    std::cout << "PASS exact grid area/kind/centroid/barycentric cases=" << cases << '\n';}
void excircleFixture(istream &in) {
    array<dpoint, 3> v;
    for (auto &p : v) { in >> p.x >> p.y; }
    TriangleApprox t(v[0], v[1], v[2]);
    long double scale = max({length(v[0]), length(v[1]), length(v[2])}), longest = 0;
    for (auto s : t.sides()) { longest = max(longest, s); }
    context = "excircle fixture triangle=" + show(v[0]) + show(v[1]) + show(v[2]);
    for (int i = 0; i < 3; ++i) {
        dpoint want; long double r; in >> want.x >> want.y >> r;
        CircleApprox got; require(t.excircle(i, got), "fixture excircle exists");
        relative(got.r, r, 64 * EPS, "excircle exradius against 200-digit reference index=" + std::to_string(i));
        // The excenter is conditioned like r * longest^2 / |area2| under coordinate rounding.
        long double tolerance = 64 * EPS * (scale + r * longest * longest / std::abs(t.area2()));
        require(std::abs(got.c.x - want.x) <= tolerance && std::abs(got.c.y - want.y) <= tolerance,
            "excenter against 200-digit reference index=" + std::to_string(i), show(want), show(got.c));}}
void fixtures() {
    const char *path = std::getenv("CP_TRIANGLE_ORACLE"); require(path, "Python oracle path supplied");
    std::ifstream in(path); require(bool(in), "open Python oracle"); string tag; int n = 0, h = 0, x = 0;
    while (in >> tag) {
        if (tag == "E") {
            point a, b, c, p; in >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y >> p.x >> p.y;
            lll d = readWide(in); array<lll, 3> w; int kind;
            for (auto &v : w) { v = readWide(in); }
            in >> kind; exact({a, b, c}, p, d, w, kind);
            auto center = Triangle2(a, b, c).centroid();
            lll cx = readWide(in), cxd = readWide(in), cy = readWide(in), cyd = readWide(in);
            require(center.x * cxd == cx * center.d && center.y * cyd == cy * center.d, "Python Fraction centroid"); ++n;}
        else if (tag == "H") {
            long double a, b, c, want, got = -91; int valid; in >> a >> b >> c >> valid >> want;
            context = "Heron sides=(" + show(a) + "," + show(b) + "," + show(c) + ")";
            bool ok = heronAreaApprox(a, b, c, got);
            require(ok == bool(valid), "Heron triangle inequality classification", show(bool(valid)), show(ok));
            if (!valid) { require(got == -91, "invalid side lengths leave Heron output unchanged"); }
            else if (!want) { require(got == 0, "degenerate Heron area is valid zero", "0", show(got)); }
            else { relative(got, want, 8e-16L, "Heron high-precision relative oracle"); }
            ++h;}
        else if (tag == "X") { excircleFixture(in); ++x; }
        else { throw std::runtime_error("unknown oracle tag " + tag); }
        require(bool(in), "complete Python oracle fixture");}
    require(in.eof() && n && h && x, "nonempty complete Python fixtures");
    std::cout << "PASS Python arbitrary-integer/Fraction cases=" << n << " Decimal Heron cases=" << h << " Decimal excircle cases=" << x << '\n';}
void basic() {
    context = "defaults, ordinary examples, copy/move, field mutation and degenerate outcomes";
    Barycentric2 initial; require(initial.w == array<lll, 3>{1, 0, 0} && initial.d == 1, "default barycentric vertex a");
    require(initial.approx() == array<long double, 3>{1, 0, 0}, "default approximate weights");
    Triangle2 zero; require(zero.a == point{} && zero.b == point{} && zero.c == point{} && zero.area2() == 0, "default exact zero triangle");
    Triangle2 t({0, 0}, {4, 0}, {0, 3}); Triangle2 copy = t, moved = std::move(copy);
    require(moved.area2() == 12 && moved.centroid() == RationalPoint2(4, 3, 3), "exact copy and move");
    moved.b = {-4, 0}; require(moved.area2() == -12, "exact public field mutation");
    require(t.angleKind() == 0 && Triangle2({0, 0}, {4, 0}, {1, 3}).angleKind() == 1 && Triangle2({0, 0}, {4, 0}, {-1, 3}).angleKind() == -1, "right/acute/obtuse kinds");
    long double big = 1000000000;
    Triangle2 corner({-1000000000, -1000000000}, {1000000000, 1000000000}, {1000000000, -1000000000});
    require(corner.angleKind() == 0 && Triangle2({-1000000000, -999999999}, {1000000000, 1000000000}, {1000000000, -1000000000}).angleKind() == 1, "coordinate-bound right and acute kinds");
    require(Triangle2({-1000000000, 0}, {1000000000, 0}, {0, 1}).angleKind() == -1, "coordinate-bound thin obtuse kind");
    auto triangle = t.approx(); require(triangle.a == dpoint{} && triangle.b == dpoint(4, 0) && triangle.c == dpoint(0, 3), "explicit exact to approximate conversion");
    require(triangle.area2() == 12 && triangle.area() == 6 && triangle.perimeter() == 12, "3-4-5 area/perimeter");
    require(triangle.sides() == array<long double, 3>{5, 3, 4}, "side order is opposite a,b,c");
    closePoint(triangle.centroid(), {4.L / 3, 1}, "floating centroid");
    dpoint p; long double radius;
    require(triangle.circumcenter(p), "right circumcenter exists"); closePoint(p, {2, 1.5L}, "right circumcenter");
    require(triangle.incenter(p), "right incenter exists"); closePoint(p, {1, 1}, "right incenter");
    require(triangle.orthocenter(p), "right orthocenter exists"); closePoint(p, {}, "right orthocenter");
    require(triangle.circumradius(radius) && radius == 2.5L && triangle.inradius(radius) && radius == 1, "right triangle radii");
    CircleApprox circle;
    array<dpoint, 3> excenters{dpoint{6, 6}, dpoint{-2, 2}, dpoint{3, -3}};
    array<long double, 3> exradii{6, 2, 3};
    for (int i = 0; i < 3; ++i) {
        require(triangle.excircle(i, circle), "right excircle exists"); closePoint(circle.c, excenters[i], "right excenter index");
        close(circle.r, exradii[i], "right exradius index");}
    require(triangle.ninePointCircle(circle), "right nine-point circle exists");
    closePoint(circle.c, {1, .75L}, "right nine-point center"); close(circle.r, 1.25L, "right nine-point radius");
    array<long double, 3> weights;
    for (int i = 0; i < 3; ++i) {
        array<dpoint, 3> vertices{triangle.a, triangle.b, triangle.c};
        require(triangle.barycentric(vertices[i], weights), "vertex barycentric exists");
        for (int j = 0; j < 3; ++j) { close(weights[j], i == j, "vertex barycentric Kronecker delta"); }}
    require(triangle.fromBarycentric({-2, 2, 2}, p), "signed homogeneous weights"); closePoint(p, {4, 3}, "outside reconstruction");
    require(triangle.fromBarycentric({2, -2, -2}, p), "negative homogeneous denominator"); closePoint(p, {4, 3}, "homogeneous sign invariance");
    p = {7, 9}; require(!triangle.fromBarycentric({1, -1, 0}, p) && p == dpoint(7, 9), "zero weight sum leaves output unchanged");
    for (int i = 0; i < 3; ++i) {
        auto aliasCheck = [&](int operation) {
            TriangleApprox q = triangle; array<dpoint *, 3> vertices{&q.a, &q.b, &q.c};
            dpoint &out = *vertices[i]; bool ok = false; dpoint expected;
            if (operation == 0) { expected = {4, 3}; ok = q.fromBarycentric({-2, 2, 2}, out); }
            if (operation == 1) { expected = {2, 1.5L}; ok = q.circumcenter(out); }
            if (operation == 2) { expected = {1, 1}; ok = q.incenter(out); }
            if (operation == 3) { expected = {}; ok = q.orthocenter(out); }
            require(ok, "successful vertex-alias operation=" + std::to_string(operation));
            closePoint(out, expected, "successful vertex-alias result");};
        for (int operation = 0; operation < 4; ++operation) { aliasCheck(operation); }
        TriangleApprox q = triangle; array<dpoint *, 3> vertices{&q.a, &q.b, &q.c};
        dpoint before = *vertices[i];
        require(!q.fromBarycentric({1, -1, 0}, *vertices[i]) && *vertices[i] == before, "failed vertex-alias reconstruction unchanged");}
    TriangleApprox radius_alias = triangle;
    require(radius_alias.circumradius(radius_alias.a.x) && radius_alias.a.x == 2.5L, "circumradius aliases coordinate");
    radius_alias = triangle;
    require(radius_alias.inradius(radius_alias.b.y) && radius_alias.b.y == 1, "inradius aliases coordinate");
    long double side_alias = 3; require(heronAreaApprox(side_alias, 4, 5, side_alias) && side_alias == 6, "Heron output aliases input");
    long double tiny = std::numeric_limits<long double>::denorm_min();
    for (int multiple : {2, 4, 8}) {
        array<long double, 3> sides{multiple * tiny, 1, 1};
        for (bool more = true; more; more = std::next_permutation(sides.begin(), sides.end())) {
            long double area = -1;
            require(heronAreaApprox(sides[0], sides[1], sides[2], area) && area == (multiple / 2) * tiny,
                "Heron subnormal third side and subnormal result", show((multiple / 2) * tiny), show(area));}}
    for (int e : {(std::numeric_limits<long double>::min_exponent - 1) / 2,
                  (std::numeric_limits<long double>::min_exponent - std::numeric_limits<long double>::digits + 6) / 2}) {
        long double scale = std::scalbn(1.L, e), area = -1, expected = std::scalbn(6.L, 2 * e);
        require(heronAreaApprox(3 * scale, 4 * scale, 5 * scale, area) && area == expected,
            "Heron near-min-normal and subnormal 3-4-5 area", show(expected), show(area));}
    for (TriangleApprox degenerate : {TriangleApprox{}, TriangleApprox({2, 3}, {2, 3}, {2, 3}),
        TriangleApprox({}, {1, 1}, {2, 2}), TriangleApprox({4, 2}, {4, 2}, {9, -5}), TriangleApprox({-big, 0}, {0, 0}, {big, 0})}) {
        require(degenerate.area2() == 0 && degenerate.area() == 0, "zero-side/collinear area");
        weights = {7, 9, 11}; require(!degenerate.barycentric({3, 4}, weights) && weights == array<long double, 3>{7, 9, 11}, "degenerate barycentric unchanged");
        require(degenerate.fromBarycentric({1, 1, 1}, p), "degenerate homogeneous reconstruction exists");
        closePoint(p, degenerate.centroid(), "degenerate reconstruction centroid", big);
        p = {7, 9}; radius = 123; circle = {{7, 9}, 123};
        require(!degenerate.circumcenter(p) && p == dpoint(7, 9), "degenerate circumcenter unchanged");
        require(!degenerate.incenter(p) && p == dpoint(7, 9), "degenerate incenter unchanged");
        require(!degenerate.orthocenter(p) && p == dpoint(7, 9), "degenerate orthocenter unchanged");
        require(!degenerate.circumradius(radius) && radius == 123, "degenerate circumradius unchanged");
        require(!degenerate.inradius(radius) && radius == 123, "degenerate inradius unchanged");
        require(!degenerate.circumcircle(circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate circumcircle unchanged");
        require(!degenerate.incircle(circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate incircle unchanged");
        require(!degenerate.ninePointCircle(circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate nine-point circle unchanged");
        for (int i = 0; i < 3; ++i) { require(!degenerate.excircle(i, circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate excircle unchanged"); }
        dpoint before = degenerate.a;
        require(!degenerate.circumcenter(degenerate.a) && degenerate.a == before, "failed vertex-alias circumcenter unchanged");
        require(!degenerate.incenter(degenerate.a) && degenerate.a == before, "failed vertex-alias incenter unchanged");
        require(!degenerate.orthocenter(degenerate.a) && degenerate.a == before, "failed vertex-alias orthocenter unchanged");}
    TriangleApprox copied = triangle, shifted = std::move(copied); shifted.a = {1, 0};
    require(shifted.area() == 4.5L && triangle.area() == 6, "floating copy/move and public vertex mutation");}
void constructions(TriangleApprox t) {
    context = "triangle=" + show(t.a) + show(t.b) + show(t.c);
    array<dpoint, 3> v{t.a, t.b, t.c}; auto sides = t.sides(); long double area2 = t.area2();
    require(area2 != 0, "nondegenerate construction corpus");
    long double scale = max({1.L, length(t.a), length(t.b), length(t.c)}), area = t.area();
    close(area, std::abs(area2) / 2, "unsigned area orientation");
    for (int i = 0; i < 3; ++i) { close(sides[i], length(v[(i + 1) % 3] - v[(i + 2) % 3]), "opposite side length"); }
    close(t.perimeter(), sides[0] + sides[1] + sides[2], "perimeter");
    CircleApprox circum, incircle, nine;
    dpoint circumcenter, incenter, orthocenter; long double cr, ir;
    require(t.circumcircle(circum) && t.incircle(incircle) && t.ninePointCircle(nine), "all triangle circles exist");
    require(t.circumcenter(circumcenter) && t.incenter(incenter) && t.orthocenter(orthocenter), "all named centers exist");
    require(t.circumradius(cr) && t.inradius(ir), "both radii exist");
    closePoint(circumcenter, circum.c, "circumcenter wrapper"); closePoint(incenter, incircle.c, "incenter wrapper");
    close(cr, circum.r, "circumradius wrapper"); close(ir, incircle.r, "inradius wrapper");
    long double big = max({scale, cr, length(orthocenter)});
    closePoint(t.centroid(), (t.a + t.b + t.c) / 3.L, "centroid independent mean", scale);
    closePoint(orthocenter + 2.L * circum.c, 3.L * t.centroid(), "Euler line identity", big);
    long double product = 1, sum = 0;
    for (int i = 0; i < 3; ++i) {
        dpoint a = v[i], b = v[(i + 1) % 3], c = v[(i + 2) % 3], edge = c - b;
        close(length(a - circum.c), circum.r, "circumcircle equal vertex distance", big);
        close(dot(orthocenter - a, edge), 0, "orthocenter altitude perpendicularity", big * length(edge));
        close(std::abs(cross(incircle.c - b, edge)) / sides[i], incircle.r, "incircle line distance", scale);
        require(cross(c - b, incircle.c - b) * area2 > 0, "incenter interior orientation");
        CircleApprox ex; require(t.excircle(i, ex), "excircle exists");
        product *= ex.r; sum += ex.r;
        for (int j = 0; j < 3; ++j) {
            dpoint x = v[(j + 1) % 3], y = v[(j + 2) % 3];
            close(std::abs(cross(ex.c - x, y - x)) / sides[j], ex.r, "excircle equal line distances", max(scale, ex.r));
            require((cross(y - x, ex.c - x) * area2 > 0) == (i != j), "excenter lies across only its opposite side");}
        dpoint midpoint = (b + c) / 2.L;
        close(length(midpoint - nine.c), nine.r, "nine-point side midpoint", big);
        dpoint foot = b + edge * (dot(a - b, edge) / dot(edge, edge));
        close(length(foot - nine.c), nine.r, "nine-point altitude foot", big);
        close(length((a + orthocenter) / 2.L - nine.c), nine.r, "nine-point vertex-orthocenter midpoint", big);}
    // r_a * r_b * r_c = r * s^2 and r_a + r_b + r_c = 4 * R + r: cancellation-free sums and products.
    long double semi = t.perimeter() / 2;
    relative(product, ir * semi * semi, 512 * EPS, "exradius product identity");
    relative(sum, 4 * cr + ir, 512 * EPS, "exradius sum identity");
    close(incircle.r * t.perimeter(), 2 * area, "inradius area identity", area);
    close(4 * area * circum.r, sides[0] * sides[1] * sides[2], "circumradius area identity", area * circum.r);
    closePoint(nine.c, (circum.c + orthocenter) / 2.L, "nine-point center identity", big); close(nine.r, cr / 2, "nine-point radius");
    array<int, 3> permutation{0, 1, 2};
    for (bool more = true; more; more = std::next_permutation(permutation.begin(), permutation.end())) {
        TriangleApprox q(v[permutation[0]], v[permutation[1]], v[permutation[2]]); CircleApprox cc, ii;
        require(q.circumcircle(cc) && q.incircle(ii), "permuted circles exist");
        closePoint(cc.c, circum.c, "six-permutation circumcenter", big); close(cc.r, cr, "six-permutation circumradius", big);
        closePoint(ii.c, incircle.c, "six-permutation incenter", scale); close(ii.r, ir, "six-permutation inradius", scale);
        close(q.area(), area, "six-permutation unsigned area", area);}}
void randomChecks(int count) {
    std::mt19937_64 rng(seed);
    auto coord = [&]() { return static_cast<long double>(int(rng() % 201) - 100) / 4; };
    for (int i = 0; i < count; ++i) {
        TriangleApprox t({coord(), coord()}, {coord(), coord()}, {coord(), coord()});
        if (std::abs(t.area2()) < .0625L) { --i; continue; }
        constructions(t); dpoint p{coord(), coord()}, result;
        context = "random round=" + std::to_string(i) + " triangle=" + show(t.a) + show(t.b) + show(t.c) + " p=" + show(p);
        array<long double, 3> w; require(t.barycentric(p, w), "floating barycentric exists");
        close(w[0] + w[1] + w[2], 1, "floating barycentric partition of unity", max({1.L, std::abs(w[0]), std::abs(w[1]), std::abs(w[2])}));
        require(t.fromBarycentric(w, result), "floating barycentric reconstruction exists");
        closePoint(result, p, "floating barycentric round-trip", max({1.L, length(t.a), length(t.b), length(t.c)}));
        TriangleApprox transformed;
        auto f = [](dpoint q) { return dpoint{11 - 2 * q.y, -13 + 2 * q.x}; };
        transformed = {f(t.a), f(t.b), f(t.c)};
        close(transformed.area(), 4 * t.area(), "similarity area"); close(transformed.perimeter(), 2 * t.perimeter(), "similarity perimeter");
        closePoint(transformed.centroid(), f(t.centroid()), "similarity centroid");
        CircleApprox before, after;
        require(t.circumcircle(before) && transformed.circumcircle(after), "similarity circumcircle exists");
        closePoint(after.c, f(before.c), "similarity circumcenter", before.r * 2); close(after.r, 2 * before.r, "similarity circumradius");
        require(t.incircle(before) && transformed.incircle(after), "similarity incircle exists");
        closePoint(after.c, f(before.c), "similarity incenter"); close(after.r, 2 * before.r, "similarity inradius");
        array<long double, 3> other; require(transformed.barycentric(f(p), other), "similarity barycentric exists");
        for (int j = 0; j < 3; ++j) { close(other[j], w[j], "affine-invariant barycentric weights"); }}
    for (long double scale : {1e-100L, 1e-8L, 1.L, 1e8L, 1e100L}) {
        TriangleApprox t({}, {4 * scale, 0}, {0, 3 * scale}); CircleApprox c; dpoint p;
        context = "scaled right triangle scale=" + show(scale);
        close(t.area() / scale / scale, 6, "scaled area"); close(t.perimeter() / scale, 12, "scaled perimeter");
        require(t.circumcircle(c), "scaled circumcircle"); close(c.r / scale, 2.5L, "scaled circumradius");
        require(t.incircle(c), "scaled incircle"); close(c.r / scale, 1, "scaled inradius");
        require(t.excircle(0, c), "scaled excircle"); close(c.r / scale, 6, "scaled exradius");
        require(t.orthocenter(p), "scaled orthocenter"); closePoint(p / scale, {}, "scaled orthocenter value");
        require(t.ninePointCircle(c), "scaled nine-point circle"); close(c.r / scale, 1.25L, "scaled nine-point radius");}
    for (long double h : {1e-4L, 1e-8L, 1e-12L}) {
        TriangleApprox t({}, {2, 0}, {1, h}); CircleApprox c; dpoint p;
        context = "thin isosceles triangle height=" + show(h);
        close(t.area() / h, 1, "thin area");
        require(t.circumcircle(c), "thin circumcircle exists"); close(c.c.x, 1, "thin circumcenter x");
        close(c.c.y * h, (h * h - 1) / 2, "thin circumcenter conditioned y");
        require(t.orthocenter(p), "thin orthocenter exists"); close(p.x, 1, "thin orthocenter x"); close(p.y * h, 1, "thin orthocenter conditioned y");}
    // Opposite the apex of ({0,0},{2,0},{1,h}): center (1,-r), r = h / (sqrt(1+h^2) - 1) = (sqrt(1+h^2) + 1) / h.
    for (long double h : {1e-1L, 1e-3L, 1e-6L, 1e-8L, 1e-9L, 1e-10L, 1e-15L, 1e-20L, 1e-100L, 1e-1000L, 1e-2000L}) {
        TriangleApprox t({}, {2, 0}, {1, h}); CircleApprox c;
        context = "thin obtuse excircle height=" + show(h);
        long double r = (std::sqrt(1 + h * h) + 1) / h;
        require(t.excircle(2, c), "thin obtuse excircle exists");
        relative(c.r, r, 8 * EPS, "thin obtuse exradius closed form");
        relative(c.c.y, -r, 8 * EPS, "thin obtuse excenter y"); require(std::abs(c.c.x - 1) <= 8 * EPS, "thin obtuse excenter x", "1", show(c.c.x));}
    std::cout << "PASS random circle/center/barycentric/permutation/similarity constructions=" << count << '\n';}
void invalid(const string &name) {
    long double nan = std::numeric_limits<long double>::quiet_NaN(), inf = std::numeric_limits<long double>::infinity(), out;
    Triangle2 exact({}, {4, 0}, {0, 3}); TriangleApprox approximate({}, {4, 0}, {0, 3});
    Barycentric2 bary; array<long double, 3> weights; dpoint p; CircleApprox circle;
    if (name == "exact-coordinate") { (void)Triangle2({1000000001, 0}, {}, {}); }
    if (name == "exact-query") { (void)exact.barycentric({0, -1000000001}, bary); }
    if (name == "angle-kind-degenerate") { (void)Triangle2({}, {1, 1}, {2, 2}).angleKind(); }
    if (name == "floating-nan") { (void)TriangleApprox({nan, 0}, {}, {}); }
    if (name == "floating-infinite") { (void)TriangleApprox({}, {0, inf}, {}); }
    if (name == "floating-query") { (void)approximate.barycentric({nan, 0}, weights); }
    if (name == "weights-nan") { (void)approximate.fromBarycentric({nan, 0, 1}, p); }
    if (name == "weights-infinite") { (void)approximate.fromBarycentric({0, inf, 1}, p); }
    if (name == "excircle-index") { (void)approximate.excircle(3, circle); }
    if (name == "excircle-negative-index") { (void)approximate.excircle(-1, circle); }
    if (name == "excircle-unrepresentable") { (void)TriangleApprox({}, {2, 0}, {1, 1e-4940L}).excircle(2, circle); }
    if (name == "negative-side") { (void)heronAreaApprox(1, -1, 1, out); }
    if (name == "nan-side") { (void)heronAreaApprox(nan, 1, 1, out); }
    if (name == "infinite-side") { (void)heronAreaApprox(1, 1, inf, out); }
    if (name == "heron-factor-overflow") {
        long double x = std::numeric_limits<long double>::max() / 2; (void)heronAreaApprox(x, x, x, out);}
    if (name == "heron-area-overflow") {
        long double x = std::numeric_limits<long double>::max() / 4; (void)heronAreaApprox(x, x, x, out);}
    if (name == "heron-area-underflow") {
        long double x = std::numeric_limits<long double>::denorm_min(); (void)heronAreaApprox(x, x, x, out);}
    std::exit(2);}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }}
    try {
        basic(); exhaustive(mode); fixtures(); randomChecks(mode == "quick" ? 100 : mode == "full" ? 2000 : 12000);
        std::cout << "PASS triangle mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';}
    catch (const std::exception &error) {
        std::cerr << "FAIL triangle mode=" << mode << " seed=" << seed << " smallest-known-reproducer=" << error.what() << '\n'; return 1;}}
