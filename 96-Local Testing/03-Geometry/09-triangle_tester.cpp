#include "../../03-Geometry/09-triangle.hpp"
#include "../../01-Core/07-infint.hpp"

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
// Literal pi rounded once to long double; pi / 2 halves it exactly.
static const long double PI = 3.14159265358979323846264338327950288L, HALF_PI = PI / 2;
bool same(long double x, long double y) { return x == y && std::signbit(x) == std::signbit(y); }
bool same(dpoint p, dpoint q) { return same(p.x, q.x) && same(p.y, q.y); }
bool same(const CircleApprox &p, const CircleApprox &q) { return same(p.c, q.c) && same(p.r, q.r); }
// Members must equal the row 06 free functions bit for bit, a false return leaving out unchanged in both.
bool memberCircum(const TriangleApprox &t, CircleApprox &out) {
    CircleApprox free = out; bool want = circumcircleApprox(t.a, t.b, t.c, free), got = t.circumcircleApprox(out);
    require(got == want && same(out, free), "circumcircleApprox member equals free function", show(want) + " " + show(free.c.x) + " " + show(free.c.y) + " " + show(free.r), show(got) + " " + show(out.c.x) + " " + show(out.c.y) + " " + show(out.r));
    return got;}
bool memberIncircle(const TriangleApprox &t, CircleApprox &out) {
    CircleApprox free = out; bool want = incircleApprox(t.a, t.b, t.c, free), got = t.incircleApprox(out);
    require(got == want && same(out, free), "incircleApprox member equals free function", show(want) + " " + show(free.c.x) + " " + show(free.c.y) + " " + show(free.r), show(got) + " " + show(out.c.x) + " " + show(out.c.y) + " " + show(out.r));
    return got;}
// Six vertex orders: angles permute with the vertices bit for bit.
void anglePermutations(const TriangleApprox &t, const array<long double, 3> &angles) {
    array<dpoint, 3> v{t.a, t.b, t.c}; array<int, 3> p{0, 1, 2};
    for (bool more = true; more; more = std::next_permutation(p.begin(), p.end())) {
        auto got = TriangleApprox(v[p[0]], v[p[1]], v[p[2]]).angles();
        for (int i = 0; i < 3; ++i) { require(same(got[i], angles[p[i]]), "angles permutation equivariance", show(angles[p[i]]), show(got[i])); }}}
void angleSum(const array<long double, 3> &angles) {
    long double sum = angles[0] + angles[1] + angles[2];
    for (long double x : angles) { require(x >= 0 && x <= PI, "angle in [0, pi]", "[0, pi]", show(x)); }
    require(std::abs(sum - PI) <= 16 * EPS * PI, "angle sum pi", show(PI), show(sum));}
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
// Closed collinear-and-bounding-box membership in a segment, a repeated endpoint being a point.
bool onSegment(point u, point v, point p) {
    return det(u, v, p) == 0 && min(u.x, v.x) <= p.x && p.x <= max(u.x, v.x) && min(u.y, v.y) <= p.y && p.y <= max(u.y, v.y);}
// Sign classification of the exact barycentric weights; zero area: union of the three edges. The two agree on boundaries.
int locateOracle(point a, point b, point c, point p) {
    lll d = det(a, b, c); bool edge = onSegment(a, b, p) || onSegment(b, c, p) || onSegment(c, a, p);
    if (!d) { return edge ? 0 : 1; }
    array<lll, 3> w{det(p, b, c), det(a, p, c), det(a, b, p)}; int low = 1;
    for (lll x : w) { low = min(low, ((x > 0) - (x < 0)) * (d > 0 ? 1 : -1)); }
    int where = low < 0 ? 1 : low > 0 ? -1 : 0;
    require((where == 0) == edge, "locate oracle self-check: zero weight inside the closure is an edge point");
    return where;}
// Exact angle facts on integer triangles: a right angle is pi / 2 exactly at the vertex whose opposite squared side is the sum
// of the other two; collinear distinct vertices give 0, pi and 0 with pi at the vertex inside the others' bounding box.
void exactAngles(Triangle2 t) {
    array<point, 3> v{t.a, t.b, t.c};
    if (v[0] == v[1] || v[1] == v[2] || v[2] == v[0]) { return; }
    auto sq = [](point p, point q) { lll x = lll(p.x) - q.x, y = lll(p.y) - q.y; return x * x + y * y; };
    bool small = true, right = false, collinear = det(t.a, t.b, t.c) == 0;
    for (point p : v) { small = small && std::abs(p.x) <= 1000 && std::abs(p.y) <= 1000; }
    auto approx = t.approx(); auto angles = approx.angles();
    for (int i = 0; i < 3; ++i) {
        point p = v[i], q = v[(i + 1) % 3], r = v[(i + 2) % 3];
        lll opposite = sq(q, r), legs = sq(p, q) + sq(p, r);
        if (opposite == legs) { require(angles[i] == HALF_PI, "right angle exactly pi/2 at its vertex", show(HALF_PI), show(angles[i])); right = true; }
        else if (small) { require((angles[i] > HALF_PI) == (opposite > legs) && angles[i] != HALF_PI, "angle obtuse exactly where the law of cosines says", show(opposite > legs), show(angles[i])); }
        if (collinear) {
            bool inside = onSegment(q, r, p);
            require(angles[i] == (inside ? PI : 0), "collinear angles 0 and pi at the middle vertex", show(inside ? PI : 0), show(angles[i]));}}
    if (!collinear) { require((t.angleKind() == 0) == right, "angleKind zero exactly where an angle is pi/2"); }
    angleSum(angles); anglePermutations(approx, angles);}
// Exact centers in InfIntMini arithmetic: canonical d > 0 and gcd 1; O equidistant from the vertices (unique for nonzero area);
// H on all three altitudes; R^2 = |O - a|^2 in both integer types; a right angle puts O at the hypotenuse midpoint and H at its vertex.
void exactCenters(Triangle2 t) {
    using W = InfIntMini;
    array<point, 3> v{t.a, t.b, t.c}; RationalPoint2 o = t.circumcenter(), h = t.orthocenter();
    context = "exact centers triangle=" + show(t.a) + show(t.b) + show(t.c);
    for (RationalPoint2 q : {o, h}) { require(q.d > 0 && gcd(gcd(q.x, q.y), q.d) == 1, "canonical RationalPoint2", "d > 0, gcd 1", show(q.x) + "," + show(q.y) + "/" + show(q.d)); }
    auto dist2 = [](RationalPoint2 q, point p) { W x = W(q.x) - W(lll(p.x) * q.d), y = W(q.y) - W(lll(p.y) * q.d); return x * x + y * y; };
    W r = dist2(o, v[0]); auto mini = t.circumradius2(); auto full = t.circumradius2<InfInt>();
    for (int i = 0; i < 3; ++i) {
        point p = v[i], q = v[(i + 1) % 3], s = v[(i + 2) % 3];
        require(dist2(o, p) == r, "circumcenter equidistant from every vertex");
        require((W(h.x) - W(lll(p.x) * h.d)) * W(lll(q.x) - s.x) + (W(h.y) - W(lll(p.y) * h.d)) * W(lll(q.y) - s.y) == W(0), "orthocenter on the altitude through vertex " + std::to_string(i));
        lll legs = dot(q - p, q - p) + dot(s - p, s - p), opposite = dot(s - q, s - q);
        if (legs == opposite) {
            require(o == RationalPoint2(lll(q.x) + s.x, lll(q.y) + s.y, 2), "right triangle circumcenter is the hypotenuse midpoint");
            require(h == RationalPoint2(p), "right triangle orthocenter is the right-angle vertex");}}
    require(mini.den > W(0) && mini.num * (W(o.d) * W(o.d)) == r * mini.den, "circumradius2 equals |O - a|^2", r.toString() + "/d^2", mini.num.toString() + "/" + mini.den.toString());
    require(full.num.toString() == mini.num.toString() && full.den.toString() == mini.den.toString(), "circumradius2 InfInt equals InfIntMini",
        mini.num.toString() + "/" + mini.den.toString(), full.num.toString() + "/" + full.den.toString());
    // TriangleApprox::orthocenter agrees where the triangle is well conditioned (smallest angle above about 7 degrees).
    lll longest = max({dot(t.b - t.a, t.b - t.a), dot(t.c - t.b, t.c - t.b), dot(t.a - t.c, t.a - t.c)}), z = t.area2();
    if (8 * (z < 0 ? -z : z) >= longest) {
        dpoint approx; require(t.approx().orthocenter(approx), "approximate orthocenter exists");
        dpoint want = h.approx(); long double scale = max({1.L, length(want), length(t.a.cast<long double>()), length(t.b.cast<long double>()), length(t.c.cast<long double>())});
        closePoint(approx, want, "approximate orthocenter against exact", scale);}}
void exact(Triangle2 t, point p, lll d, array<lll, 3> w, int kind, int where) {
    context = "triangle=" + show(t.a) + show(t.b) + show(t.c) + " p=" + show(p);
    require(t.area2() == d, "exact signed area2", show(d), show(t.area2()));
    require(t.orient() == (d > 0) - (d < 0), "orient is the sign of the independent determinant", std::to_string((d > 0) - (d < 0)), std::to_string(t.orient()));
    require(t.locate(p) == where, "locate against barycentric signs / edge union", std::to_string(where), std::to_string(t.locate(p)));
    exactAngles(t);
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
    for (point a : vertices) { for (point b : vertices) { for (point c : vertices) {
        if (det(a, b, c)) { exactCenters({a, b, c}); }
        for (point p : queries) { exact({a, b, c}, p, det(a, b, c), {det(p, b, c), det(a, p, c), det(a, b, p)}, kindOracle(a, b, c), locateOracle(a, b, c, p)); ++cases; }}}}
    std::cout << "PASS exact grid area/orient/locate/angles/kind/centroid/barycentric/centers cases=" << cases << '\n';}
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
// Angles against 80-digit law-of-cosines references; cross and dot are exact for |coordinate| <= 10^9, so atan2 is a few ulp.
void angleFixture(istream &in) {
    point a, b, c; in >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y;
    array<long double, 3> want; for (auto &x : want) { in >> x; }
    Triangle2 exact(a, b, c); TriangleApprox t = exact.approx();
    context = "angle fixture triangle=" + show(a) + show(b) + show(c);
    auto check = [&](const TriangleApprox &q, const string &how) {
        auto got = q.angles();
        for (int i = 0; i < 3; ++i) { relative(got[i], want[i], 8 * EPS, how + " angle against Decimal law of cosines index=" + std::to_string(i)); }
        angleSum(got); anglePermutations(q, got);};
    check(t, "plain"); exactAngles(exact);
    // Reflection, quarter turn and a power-of-two scale are exact on these coordinates; the reflection flips area2 exactly.
    Affine2Approx flip{1, 0, 0, -1}, turn{0, -1, 1, 0}, shrink = Affine2Approx::scaling(std::ldexp(1.L, -40), std::ldexp(1.L, -40));
    check(t.transform(flip), "reflected"); check(t.transform(turn), "rotated"); check(t.transform(shrink), "scaled");
    require(t.transform(flip).area2() == -t.area2(), "exact reflection negates area2");
    TriangleApprox two = t.transform(turn).transform(flip), one = t.transform(flip * turn);
    require(same(two.a, one.a) && same(two.b, one.b) && same(two.c, one.c), "exact transform composition");}
// Circumcenter, orthocenter and squared circumradius against Python Fraction Cramer solutions of the bisector and altitude
// equations, canonical forms compared exactly, invariant under the six vertex orders.
void centerFixture(istream &in) {
    point a, b, c; in >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y;
    array<lll, 6> want; for (auto &x : want) { x = readWide(in); }
    string num, den; in >> num >> den;
    array<point, 3> v{a, b, c}; array<int, 3> q{0, 1, 2};
    for (bool more = true; more; more = std::next_permutation(q.begin(), q.end())) {
        Triangle2 t(v[q[0]], v[q[1]], v[q[2]]);
        context = "center fixture triangle=" + show(t.a) + show(t.b) + show(t.c);
        RationalPoint2 o = t.circumcenter(), h = t.orthocenter(); auto mini = t.circumradius2(); auto full = t.circumradius2<InfInt>();
        require(o.x == want[0] && o.y == want[1] && o.d == want[2], "circumcenter against Python Fraction bisectors",
            show(want[0]) + "," + show(want[1]) + "/" + show(want[2]), show(o.x) + "," + show(o.y) + "/" + show(o.d));
        require(h.x == want[3] && h.y == want[4] && h.d == want[5], "orthocenter against Python Fraction altitudes",
            show(want[3]) + "," + show(want[4]) + "/" + show(want[5]), show(h.x) + "," + show(h.y) + "/" + show(h.d));
        require(mini.num.toString() == num && mini.den.toString() == den, "circumradius2 InfIntMini against Python Fraction", num + "/" + den, mini.num.toString() + "/" + mini.den.toString());
        require(full.num.toString() == num && full.den.toString() == den, "circumradius2 InfInt against Python Fraction", num + "/" + den, full.num.toString() + "/" + full.den.toString());}
    exactCenters({a, b, c});}
// Orient and locate against Python bigint oracles at the coordinate bound, with set and orientation symmetry under vertex order.
void locateFixture(istream &in) {
    point a, b, c, p; int orient, where; in >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y >> p.x >> p.y >> orient >> where;
    context = "locate fixture triangle=" + show(a) + show(b) + show(c) + " p=" + show(p);
    require(locateOracle(a, b, c, p) == where, "C++ and Python locate oracles agree", std::to_string(where), std::to_string(locateOracle(a, b, c, p)));
    array<point, 3> v{a, b, c}; array<int, 3> q{0, 1, 2};
    for (bool more = true; more; more = std::next_permutation(q.begin(), q.end())) {
        Triangle2 t(v[q[0]], v[q[1]], v[q[2]]); int inversions = (q[0] > q[1]) + (q[0] > q[2]) + (q[1] > q[2]), sign = inversions % 2 ? -1 : 1;
        require(t.orient() == orient * sign, "orient against Python determinant sign", std::to_string(orient * sign), std::to_string(t.orient()));
        require(t.locate(p) == where, "locate against Python bigint oracle", std::to_string(where), std::to_string(t.locate(p)));}}
void fixtures() {
    const char *path = std::getenv("CP_TRIANGLE_ORACLE"); require(path, "Python oracle path supplied");
    std::ifstream in(path); require(bool(in), "open Python oracle"); string tag; int n = 0, h = 0, x = 0, an = 0, lo = 0, ce = 0;
    while (in >> tag) {
        if (tag == "E") {
            point a, b, c, p; in >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y >> p.x >> p.y;
            lll d = readWide(in); array<lll, 3> w; int kind;
            for (auto &v : w) { v = readWide(in); }
            in >> kind; auto center = Triangle2(a, b, c).centroid();
            lll cx = readWide(in), cxd = readWide(in), cy = readWide(in), cyd = readWide(in); int where; in >> where;
            exact({a, b, c}, p, d, w, kind, where);
            if (d) { exactCenters({a, b, c}); }
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
        else if (tag == "A") { angleFixture(in); ++an; }
        else if (tag == "L") { locateFixture(in); ++lo; }
        else if (tag == "C") { centerFixture(in); ++ce; }
        else { throw std::runtime_error("unknown oracle tag " + tag); }
        require(bool(in), "complete Python oracle fixture");}
    require(in.eof() && n && h && x && an && lo && ce, "nonempty complete Python fixtures");
    std::cout << "PASS Python arbitrary-integer/Fraction cases=" << n << " Decimal Heron cases=" << h << " Decimal excircle cases=" << x << " Decimal angle cases=" << an << " bigint orient/locate cases=" << lo << " Fraction center cases=" << ce << '\n';}
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
    dpoint p; CircleApprox circle;
    require(memberCircum(triangle, circle), "right circumcircle exists");
    closePoint(circle.c, {2, 1.5L}, "right circumcenter"); require(circle.r == 2.5L, "right circumradius", "2.5", show(circle.r));
    require(memberCircum(TriangleApprox({-0.L, 0}, {1, 1}, {-1, 1}), circle), "signed-zero circumcircle exists");
    require(same(circle.c.x, 0.L), "circumcenter keeps the free function's +0", "0", show(circle.c.x));
    // Pinned rounding (user decision 2026-11-15): a right angle at the opposite vertex takes the x * y + w branch.
    {
        dpoint u{1, 1}, v{-1, 1}; CircleApprox e;
        long double x = circle_detail::length(v), y = circle_detail::length(u), z = circle_detail::length(v - u), k = (x + y + z) / (2 * (x * y));
        require(TriangleApprox({}, u, v).excircle(0, e), "right-angle excircle exists");
        require(same(e.r, 2 * k) && same(e.c, (u * x + v * y) * k), "right-angle excircle takes the pinned branch", show(2 * k), show(e.r));}
    require(memberIncircle(triangle, circle), "right incircle exists");
    closePoint(circle.c, {1, 1}, "right incenter"); require(circle.r == 1, "right inradius", "1", show(circle.r));
    require(triangle.orthocenter(p), "right orthocenter exists"); closePoint(p, {}, "right orthocenter");
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
            if (operation == 1) { expected = {}; ok = q.orthocenter(out); }
            require(ok, "successful vertex-alias operation=" + std::to_string(operation));
            closePoint(out, expected, "successful vertex-alias result");};
        for (int operation = 0; operation < 2; ++operation) { aliasCheck(operation); }
        TriangleApprox q = triangle; array<dpoint *, 3> vertices{&q.a, &q.b, &q.c};
        dpoint before = *vertices[i];
        require(!q.fromBarycentric({1, -1, 0}, *vertices[i]) && *vertices[i] == before, "failed vertex-alias reconstruction unchanged");
        // Vertex i is read from the output circle's center.
        CircleApprox o(before, 7); array<const dpoint *, 3> in{&q.a, &q.b, &q.c}; in[i] = &o.c;
        require(circumcircleApprox(*in[0], *in[1], *in[2], o) && o.r == 2.5L, "circumcircle output aliases vertex"); closePoint(o.c, {2, 1.5L}, "center-alias circumcenter");
        o = {before, 7};
        require(incircleApprox(*in[0], *in[1], *in[2], o) && o.r == 1, "incircle output aliases vertex"); closePoint(o.c, {1, 1}, "center-alias incenter");}
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
        p = {7, 9}; circle = {{7, 9}, 123};
        require(!degenerate.orthocenter(p) && p == dpoint(7, 9), "degenerate orthocenter unchanged");
        require(!memberCircum(degenerate, circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate circumcircle unchanged");
        require(!memberIncircle(degenerate, circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate incircle unchanged");
        require(!degenerate.ninePointCircle(circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate nine-point circle unchanged");
        for (int i = 0; i < 3; ++i) { require(!degenerate.excircle(i, circle) && circle.c == dpoint(7, 9) && circle.r == 123, "degenerate excircle unchanged"); }
        dpoint before = degenerate.a; circle = {before, 123};
        require(!circumcircleApprox(circle.c, degenerate.b, degenerate.c, circle) && circle.c == before && circle.r == 123, "failed center-alias circumcircle unchanged");
        require(!incircleApprox(circle.c, degenerate.b, degenerate.c, circle) && circle.c == before && circle.r == 123, "failed center-alias incircle unchanged");
        require(!degenerate.orthocenter(degenerate.a) && degenerate.a == before, "failed vertex-alias orthocenter unchanged");}
    TriangleApprox copied = triangle, shifted = std::move(copied); shifted.a = {1, 0};
    require(shifted.area() == 4.5L && triangle.area() == 6, "floating copy/move and public vertex mutation");
    context = "orient, locate, angles, transform and member circle examples";
    require(t.orient() == 1 && moved.orient() == -1 && zero.orient() == 0 && corner.orient() == -1 && Triangle2({}, {1, 1}, {2, 2}).orient() == 0, "orient examples");
    for (auto [q, want] : vector<pair<point, int>>{{point{1, 1}, -1}, {point{}, 0}, {point{4, 0}, 0}, {point{0, 3}, 0}, {point{2, 0}, 0}, {point{0, 1}, 0},
                           {point{5, 0}, 1}, {point{-1, -1}, 1}, {point{2, 2}, 1}, {point{3, 1}, 1}}) {
        require(t.locate(q) == want, "locate example", std::to_string(want), std::to_string(t.locate(q)));
        require(moved.locate(point{-q.x, q.y}) == want, "locate mirrored clockwise example", std::to_string(want), std::to_string(moved.locate(point{-q.x, q.y})));}
    Triangle2 line({}, {2, 2}, {4, 4}), single({1, 1}, {1, 1}, {1, 1}), spur({}, {}, {3, 0});
    require(line.locate({3, 3}) == 0 && line.locate({4, 4}) == 0 && line.locate({5, 5}) == 1 && line.locate({1, 0}) == 1 && line.locate({-1, -1}) == 1, "collinear locate is its edges");
    require(single.locate({1, 1}) == 0 && single.locate({1, 2}) == 1 && spur.locate({2, 0}) == 0 && spur.locate({4, 0}) == 1 && spur.locate({2, 1}) == 1, "repeated-vertex locate");
    require(Triangle2({-1000000000, -1000000000}, {1000000000, -1000000000}, {-1000000000, 1000000000}).locate({0, 0}) == 0 &&
        Triangle2({-1000000000, -1000000000}, {1000000000, -1000000000}, {-1000000000, 1000000000}).locate({-1, 0}) == -1 &&
        Triangle2({-1000000000, -1000000000}, {1000000000, -1000000000}, {-1000000000, 1000000000}).locate({1, 0}) == 1, "coordinate-bound hypotenuse locate");
    auto angles = triangle.angles();
    require(angles[0] == HALF_PI && angles[1] + angles[2] <= HALF_PI * (1 + 4 * EPS) && angles[1] + angles[2] >= HALF_PI * (1 - 4 * EPS), "3-4-5 right angle at a");
    require(TriangleApprox({}, {1, 1}, {2, 2}).angles() == array<long double, 3>{0, PI, 0} &&
        TriangleApprox({-big, 0}, {big, 0}, {0, 0}).angles() == array<long double, 3>{0, 0, PI}, "collinear distinct angles");
    TriangleApprox equilateral({}, {2, 0}, {1, std::sqrt(3.L)});
    for (long double x : equilateral.angles()) { close(x, PI / 3, "equilateral angles"); }
    Affine2Approx identity; TriangleApprox same_t = triangle.transform(identity);
    require(same(same_t.a, triangle.a) && same(same_t.b, triangle.b) && same(same_t.c, triangle.c), "identity transform");
    TriangleApprox moved_t = triangle.transform(Affine2Approx::translation({1.5L, -2})), self = triangle; self = self.transform(Affine2Approx::scaling(-2, 4));
    require(moved_t.a == dpoint(1.5L, -2) && moved_t.b == dpoint(5.5L, -2) && moved_t.c == dpoint(1.5L, 1), "translation transform");
    require(self.a == dpoint{} && self.b == dpoint(-8, 0) && self.c == dpoint(0, 12) && self.area2() == -8 * triangle.area2(), "self-assigned scaling transform");
    TriangleApprox origin = TriangleApprox{}.transform(Affine2Approx::translation({3, 4}));
    require(origin.a == dpoint(3, 4) && origin.b == dpoint(3, 4) && origin.c == dpoint(3, 4) && origin.area2() == 0, "degenerate transform");}
void constructions(TriangleApprox t) {
    context = "triangle=" + show(t.a) + show(t.b) + show(t.c);
    array<dpoint, 3> v{t.a, t.b, t.c}; auto sides = t.sides(); long double area2 = t.area2();
    require(area2 != 0, "nondegenerate construction corpus");
    long double scale = max({1.L, length(t.a), length(t.b), length(t.c)}), area = t.area();
    close(area, std::abs(area2) / 2, "unsigned area orientation");
    for (int i = 0; i < 3; ++i) { close(sides[i], length(v[(i + 1) % 3] - v[(i + 2) % 3]), "opposite side length"); }
    close(t.perimeter(), sides[0] + sides[1] + sides[2], "perimeter");
    CircleApprox circum, incircle, nine; dpoint orthocenter;
    require(memberCircum(t, circum) && memberIncircle(t, incircle) && t.ninePointCircle(nine), "all triangle circles exist");
    require(t.orthocenter(orthocenter), "orthocenter exists");
    long double cr = circum.r, ir = incircle.r, big = max({scale, cr, length(orthocenter)});
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
    // Law of cosines from the side lengths and law of sines against the circumradius.
    auto angles = t.angles(); angleSum(angles); anglePermutations(t, angles);
    for (int i = 0; i < 3; ++i) {
        long double b = sides[(i + 1) % 3], c = sides[(i + 2) % 3];
        close(std::cos(angles[i]), (b * b + c * c - sides[i] * sides[i]) / (2 * b * c), "angle law of cosines", (b * b + c * c) / (b * c));
        close(sides[i], 2 * cr * std::sin(angles[i]), "angle law of sines", big);}
    array<int, 3> permutation{0, 1, 2};
    for (bool more = true; more; more = std::next_permutation(permutation.begin(), permutation.end())) {
        TriangleApprox q(v[permutation[0]], v[permutation[1]], v[permutation[2]]); CircleApprox cc, ii;
        require(memberCircum(q, cc) && memberIncircle(q, ii), "permuted circles exist");
        closePoint(cc.c, circum.c, "six-permutation circumcenter", big); close(cc.r, cr, "six-permutation circumradius", big);
        closePoint(ii.c, incircle.c, "six-permutation incenter", scale); close(ii.r, ir, "six-permutation inradius", scale);
        close(q.area(), area, "six-permutation unsigned area", area);}}
// Independent homogeneous evaluation summed in reverse order; magnitude bounds the transformed coordinates.
dpoint applyMatrix(const Affine2Approx &f, dpoint p) { return {f.t.x + f.b * p.y + f.a * p.x, f.t.y + f.d * p.y + f.c * p.x}; }
void transforms(const TriangleApprox &t, ulng round) {
    std::mt19937_64 rng(seed ^ (round * 0x9e3779b97f4a7c15ULL));
    auto unit = [&]() { return static_cast<long double>(int(rng() % 2001) - 1000) / 256; };
    auto spot = [&]() { return dpoint{unit(), unit()}; };
    dpoint p = spot(), q = p + dpoint{1 + unit() * unit(), unit()};
    long double s = std::ldexp(1.L, int(rng() % 9) - 4), size = max({1.L, length(t.a), length(t.b), length(t.c)});
    // Kind 0: similarity (angles kept), 1: also a reflection (area2 sign flips), 2: general affine.
    vector<pair<Affine2Approx, int>> maps{{Affine2Approx{}, 0}, {Affine2Approx::translation(spot()), 0}, {Affine2Approx::scaling(s, s, spot()), 0},
        {Affine2Approx::rotation(unit(), spot()), 0}, {Affine2Approx::similarity(p, q, spot(), spot() + dpoint{1, 0}), 0},
        {Affine2Approx::reflection(p, q), 1}, {Affine2Approx::scaling(-s, s, spot()), 1}, {Affine2Approx::scaling(unit(), unit(), spot()), 2},
        {Affine2Approx{unit(), unit(), unit(), unit(), spot()}, 2}};
    auto before = t.angles(); array<dpoint, 3> v{t.a, t.b, t.c};
    for (int k = 0; k < int(maps.size()); ++k) {
        auto [f, kind] = maps[k]; const Affine2Approx &g = maps[(k + 1) % maps.size()].first;
        context = "transform round=" + std::to_string(round) + " map=" + std::to_string(k) + " triangle=" + show(t.a) + show(t.b) + show(t.c);
        long double mag = 1 + max(std::abs(f.a) + std::abs(f.b), std::abs(f.c) + std::abs(f.d)) * size + std::abs(f.t.x) + std::abs(f.t.y);
        TriangleApprox image = t.transform(f), chained = t.transform(g).transform(f), composed = t.transform(f * g); array<dpoint, 3> w{image.a, image.b, image.c};
        for (int i = 0; i < 3; ++i) {
            require(same(w[i], f.applyPoint(v[i])), "transform vertex is applyPoint", show(f.applyPoint(v[i])), show(w[i]));
            closePoint(w[i], applyMatrix(f, v[i]), "transform against independent matrix evaluation", mag);}
        long double det = f.a * f.d - f.b * f.c, gmag = 1 + max(std::abs(g.a) + std::abs(g.b), std::abs(g.c) + std::abs(g.d)) * size + std::abs(g.t.x) + std::abs(g.t.y);
        close(image.area2(), det * t.area2(), "transformed area2 scales by the determinant", mag * mag);
        closePoint(chained.a, composed.a, "transform composition a", mag * gmag); closePoint(chained.b, composed.b, "transform composition b", mag * gmag);
        closePoint(chained.c, composed.c, "transform composition c", mag * gmag);
        if (kind < 2) {
            require((image.area2() > 0) == ((t.area2() > 0) == (kind == 0)), "similarity keeps orientation, reflection flips it");
            auto after = image.angles();
            for (int i = 0; i < 3; ++i) { close(after[i], before[i], "similarity-invariant angles"); }}}}
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
        require(memberCircum(t, before) && memberCircum(transformed, after), "similarity circumcircle exists");
        closePoint(after.c, f(before.c), "similarity circumcenter", before.r * 2); close(after.r, 2 * before.r, "similarity circumradius");
        require(memberIncircle(t, before) && memberIncircle(transformed, after), "similarity incircle exists");
        closePoint(after.c, f(before.c), "similarity incenter"); close(after.r, 2 * before.r, "similarity inradius");
        array<long double, 3> other; require(transformed.barycentric(f(p), other), "similarity barycentric exists");
        for (int j = 0; j < 3; ++j) { close(other[j], w[j], "affine-invariant barycentric weights"); }
        transforms(t, ulng(i));}
    for (long double scale : {1e-100L, 1e-8L, 1.L, 1e8L, 1e100L}) {
        TriangleApprox t({}, {4 * scale, 0}, {0, 3 * scale}); CircleApprox c; dpoint p;
        context = "scaled right triangle scale=" + show(scale);
        close(t.area() / scale / scale, 6, "scaled area"); close(t.perimeter() / scale, 12, "scaled perimeter");
        require(memberCircum(t, c), "scaled circumcircle"); close(c.r / scale, 2.5L, "scaled circumradius");
        require(memberIncircle(t, c), "scaled incircle"); close(c.r / scale, 1, "scaled inradius");
        require(t.excircle(0, c), "scaled excircle"); close(c.r / scale, 6, "scaled exradius");
        require(t.orthocenter(p), "scaled orthocenter"); closePoint(p / scale, {}, "scaled orthocenter value");
        require(t.ninePointCircle(c), "scaled nine-point circle"); close(c.r / scale, 1.25L, "scaled nine-point radius");
        auto angles = t.angles(), unscaled = TriangleApprox({}, {4, 0}, {0, 3}).angles();
        require(angles[0] == HALF_PI, "scaled right angle exactly pi/2", show(HALF_PI), show(angles[0]));
        for (int i = 1; i < 3; ++i) { relative(angles[i], unscaled[i], 16 * EPS, "scale-invariant angles"); }}
    for (long double h : {1e-4L, 1e-8L, 1e-12L}) {
        TriangleApprox t({}, {2, 0}, {1, h}); CircleApprox c; dpoint p;
        context = "thin isosceles triangle height=" + show(h);
        close(t.area() / h, 1, "thin area");
        require(memberCircum(t, c), "thin circumcircle exists"); close(c.c.x, 1, "thin circumcenter x");
        close(c.c.y * h, (h * h - 1) / 2, "thin circumcenter conditioned y");
        require(t.orthocenter(p), "thin orthocenter exists"); close(p.x, 1, "thin orthocenter x"); close(p.y * h, 1, "thin orthocenter conditioned y");}
    // ({0,0},{2,0},{1,h}): base angles atan(h) by its alternating series (h <= 1e-3 leaves h^9 / 9 below eps), apex pi - 2 * atan(h).
    for (long double h : {1e-3L, 1e-4L, 1e-8L, 1e-12L, 1e-20L, 1e-100L, 1e-1000L, 1e-2000L, 1e-4000L}) {
        TriangleApprox t({}, {2, 0}, {1, h}); auto angles = t.angles();
        long double base = h * (1 - h * h * (1.L / 3 - h * h * (1.L / 5 - h * h / 7)));
        context = "thin isosceles angles height=" + show(h);
        relative(angles[0], base, 8 * EPS, "thin base angle"); require(same(angles[1], angles[0]), "thin symmetric base angles");
        relative(angles[2], PI - 2 * base, 8 * EPS, "thin apex angle"); angleSum(angles); anglePermutations(t, angles);}
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
    if (name == "exact-coordinate-b") { (void)Triangle2({}, {0, -1000000001}, {}); }
    if (name == "exact-coordinate-c") { (void)Triangle2({}, {}, {1000000001, 0}); }
    if (name == "exact-query") { (void)exact.barycentric({0, -1000000001}, bary); }
    if (name == "circumcenter-degenerate") { (void)Triangle2({}, {1, 1}, {2, 2}).circumcenter(); }
    if (name == "circumradius2-degenerate") { (void)Triangle2({3, 3}, {3, 3}, {5, 0}).circumradius2(); }
    if (name == "orthocenter-degenerate") { (void)Triangle2({-1000000000, 0}, {0, 0}, {1000000000, 0}).orthocenter(); }
    if (name == "locate-query") { (void)exact.locate({1000000001, 0}); }
    if (name == "angles-repeated-vertex") { (void)TriangleApprox({1, 2}, {5, 0}, {1, 2}).angles(); }
    if (name == "transform-overflow") { (void)approximate.transform(Affine2Approx::scaling(std::numeric_limits<long double>::max(), 1)); }
    if (name == "angle-kind-degenerate") { (void)Triangle2({}, {1, 1}, {2, 2}).angleKind(); }
    if (name == "floating-nan") { (void)TriangleApprox({nan, 0}, {}, {}); }
    if (name == "floating-infinite") { (void)TriangleApprox({}, {0, inf}, {}); }
    if (name == "floating-third") { (void)TriangleApprox({}, {}, {-inf, 0}); }
    if (name == "floating-query") { (void)approximate.barycentric({nan, 0}, weights); }
    if (name == "weights-nan") { (void)approximate.fromBarycentric({nan, 0, 1}, p); }
    if (name == "weights-infinite") { (void)approximate.fromBarycentric({0, inf, 1}, p); }
    if (name == "excircle-index") { (void)approximate.excircle(3, circle); }
    if (name == "excircle-negative-index") { (void)approximate.excircle(-1, circle); }
    if (name == "excircle-gap-overflow") {
        long double x = 0.55e2466L; (void)TriangleApprox({}, {x, x}, {x, 1.0001L * x}).excircle(0, circle);}
    if (name == "excircle-gap-subnormal") {
        long double t = std::ldexp(1.L, -8191); (void)TriangleApprox({}, dpoint{-1000000006, -1} * t, dpoint{1000000007, 1} * t).excircle(0, circle);}
    if (name == "excircle-unrepresentable") { (void)TriangleApprox({}, {2e10L, 0}, {1e10L, 1e-2463L}).excircle(2, circle); }
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
