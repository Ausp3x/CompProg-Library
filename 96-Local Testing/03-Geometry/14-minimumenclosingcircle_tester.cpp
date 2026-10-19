#include "../../03-Geometry/14-minimumenclosingcircle.hpp"
#include "../../03-Geometry/04-convexhull.hpp"

// Oracles: brute-force minimum over 1-, 2- and 3-point circles; the hull-of-boundary-points criterion; 256-bit exact sides;
// radius2 against the squared distance from the exact centre in InfInt; the rounded circle against the exact one; Python Fraction brute force (--oracle).
static ulng seed = 20260927;
static std::string phase;
static vector<point> pts;
static lng checks = 0;
static long double worst = 0;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " points=";
    for (auto x : pts) { std::cerr << '(' << x.x << ',' << x.y << ')'; }
    std::cerr << " expected=true actual=false\n"; std::exit(1);}
// Squared radius as a fraction and the centre of the circle through 1, 2 or 3 points (3 non-collinear).
struct Exact { lll num, den; RationalPoint2 c; };
Exact exactCircle(const vector<point> &s) {
    if (s.size() == 1) { return {0, 1, RationalPoint2(s[0])}; }
    if (s.size() == 2) { return {dist2(s[0], s[1]), 4, RationalPoint2(lll(s[0].x) + s[1].x, lll(s[0].y) + s[1].y, 2)}; }
    point a = s[0], u = s[1] - a, v = s[2] - a; lll z = cross(u, v);
    lll num = norm2(u) * norm2(v) * dist2(s[1], s[2]), den = 4 * z * z;
    lll g = gcd(num, den);
    lll cx = 2 * z * a.x + v.y * norm2(u) - u.y * norm2(v), cy = 2 * z * a.y - v.x * norm2(u) + u.x * norm2(v);
    return {num / g, den / g, RationalPoint2(cx, cy, 2 * z)};}
// Sign of |q - c|^2 - r^2 for small coordinates.
int exactSide(const Exact &e, point q) {
    lll dx = lll(q.x) * e.c.d - e.c.x, dy = lll(q.y) * e.c.d - e.c.y;
    lll lhs = (dx * dx + dy * dy) * e.den, rhs = e.num * e.c.d * e.c.d;
    return lhs < rhs ? -1 : lhs > rhs ? 1 : 0;}
void bruteCheck(const EnclosingCircle &c) {
    int n = int(pts.size()); Exact best{-1, 1, {}};
    auto consider = [&](const vector<point> &s) {
        if (s.size() == 3 && orient(s[0], s[1], s[2]) == 0) { return; }
        Exact e = exactCircle(s);
        for (auto q : pts) { if (exactSide(e, q) > 0) { return; } }
        if (best.num < 0 || e.num * best.den < best.num * e.den) { best = e; }};
    for (int i = 0; i < n; ++i) {
        consider({pts[i]});
        for (int j = i + 1; j < n; ++j) {
            consider({pts[i], pts[j]});
            for (int k = j + 1; k < n; ++k) { consider({pts[i], pts[j], pts[k]}); }}}
    Exact got = exactCircle(c.points);
    require(got.num * best.den == best.num * got.den && c.center() == best.c, "minimal circle equals the brute-force minimum");
    require(c.radius2() == Rational<InfInt>(InfInt(best.num), InfInt(best.den)), "radius2 equals the brute-force squared radius");
    for (auto q : pts) { require(c.locate(q) == exactSide(got, q), "locate equals the exact rational side"); }}
// |s d - N|^2 / d^2 for the exact centre N / d, in InfInt: a different derivation from radius2's side lengths.
Rational<InfInt> centreRadius2(const EnclosingCircle &c, point s) {
    RationalPoint2 z = c.center(); InfInt dx(lll(s.x) * z.d - z.x), dy(lll(s.y) * z.d - z.y);
    return Rational<InfInt>(dx * dx + dy * dy, InfInt(z.d) * InfInt(z.d));}
long double scale(const vector<point> &v) {
    long double s = 1;
    for (auto q : v) { s = max({s, std::abs((long double)q.x), std::abs((long double)q.y)}); }
    return s;}
void structureCheck(const EnclosingCircle &c) {
    int k = int(c.support.size());
    require(k >= 1 && k <= 3 && int(c.points.size()) == k, "support size");
    for (int i = 0; i < k; ++i) {
        require(c.support[i] >= 0 && c.support[i] < int(pts.size()) && pts[c.support[i]] == c.points[i], "support indices name their points");
        require(c.locate(c.points[i]) == 0, "support lies on the circle");
        for (int j = 0; j < i; ++j) { require(c.points[i] != c.points[j], "support points are distinct"); }}
    for (auto q : pts) { require(c.contains(q), "every point is enclosed"); }
    dpoint m = c.center().approx(); long double s = scale(pts);
    require(std::abs(c.circle.c.x - m.x) <= 1e-15L * s && std::abs(c.circle.c.y - m.y) <= 1e-15L * s, "circle centre approximates the exact centre");
    require(std::abs(c.circle.r - distanceApprox(m, c.points[0].cast<long double>())) <= 1e-15L * s, "circle radius approximates the distance to the support");
    Rational<InfInt> r2 = c.radius2();
    for (int i = 0; i < k; ++i) { require(r2 == centreRadius2(c, c.points[i]), "radius2 equals the squared distance from the exact centre to each support point"); }
    require(std::abs(std::sqrt(r2.toLongDouble()) - c.circle.r) <= 1e-15L * s, "radius2 rounds to the circle radius");}
// Signed comparison of 256-bit values held as (high, low) after adding a bias, for products of two |lll| < 2^127.
using Wide = pair<ulll, ulll>;
Wide mul256(ulll x, ulll y) {
    ulll x0 = ulng(x), x1 = x >> 64, y0 = ulng(y), y1 = y >> 64;
    ulll p00 = x0 * y0, p01 = x0 * y1, p10 = x1 * y0, mid = (p00 >> 64) + ulng(p01) + ulng(p10);
    return {x1 * y1 + (p01 >> 64) + (p10 >> 64) + (mid >> 64), (mid << 64) | ulng(p00)};}
Wide add256(Wide a, Wide b) { ulll lo = a.second + b.second; return {a.first + b.first + (lo < a.second), lo}; }
Wide square(lll v) { ulll u = v < 0 ? ulll(-v) : ulll(v); return mul256(u, u); }
// Exact side of q against the circle through the support at any coordinate in the domain: compares |q d - N|^2 with |s d - N|^2.
int wideSide(const EnclosingCircle &c, point q) {
    RationalPoint2 z = c.center(); point s = c.points[0];
    Wide a = add256(square(lll(q.x) * z.d - z.x), square(lll(q.y) * z.d - z.y)), b = add256(square(lll(s.x) * z.d - z.x), square(lll(s.y) * z.d - z.y));
    return a < b ? -1 : a > b ? 1 : 0;}
// Optimality: the exact centre lies in the closed convex hull of the boundary points; each product stays below 7 * 10^37 at 10^9.
void hullCheck(const EnclosingCircle &c) {
    vector<point> on;
    for (auto q : pts) { if (c.locate(q) == 0) { on.push_back(q); } }
    auto h = convexHull(on); RationalPoint2 z = c.center();
    auto side = [&](point a, point b) {
        lll u = (lll(b.x) - a.x) * (z.y - lll(a.y) * z.d), v = (lll(b.y) - a.y) * (z.x - lll(a.x) * z.d);
        return u > v ? 1 : u < v ? -1 : 0;};
    bool inside = true;
    if (h.size() == 1) { inside = z == RationalPoint2(h[0]); }
    else if (h.size() == 2) {
        inside = side(h[0], h[1]) == 0;
        RationalPoint2 mid(lll(h[0].x) + h[1].x, lll(h[0].y) + h[1].y, 2); inside &= z == mid;}
    else { for (int i = 0; i < int(h.size()); ++i) { inside &= side(h[i], h[(i + 1) % h.size()]) >= 0; } }
    require(inside, "centre lies in the hull of the boundary points");}
// The rounded circle of the same integer points as double, float and long double against the exact circle: error at most 1e-9 of the scale.
void approxCheck(ulng sd) {
    vector<Point2<double>> f; vector<Point2<long double>> l; vector<Point2<float>> g; bool small = true;
    for (auto q : pts) {
        f.push_back({double(q.x), double(q.y)}); l.push_back({(long double)q.x, (long double)q.y});
        g.push_back({float(q.x), float(q.y)}); small &= std::abs(q.x) < (1 << 24) && std::abs(q.y) < (1 << 24);}
    auto a = minimumEnclosingCircleApprox(f, sd), b = minimumEnclosingCircleApprox(l, sd); auto e = minimumEnclosingCircle(pts, sd);
    require(a.c == b.c && a.r == b.r, "double and long double inputs of the same values give the same circle");
    if (small) { auto h = minimumEnclosingCircleApprox(g, sd); require(h.c == a.c && h.r == a.r, "float inputs of the same values give the same circle"); }
    dpoint m = e.center().approx(); long double s = scale(pts), r = std::sqrt(e.radius2().toLongDouble());
    long double err = max({std::abs(a.c.x - m.x), std::abs(a.c.y - m.y), std::abs(a.r - r)}) / s; worst = max(worst, err);
    require(err <= 1e-9L, "rounded circle equals the exact circle within 1e-9 of the scale");
    for (auto q : pts) { require(distanceApprox(q.cast<long double>(), a.c) <= a.r + 1e-9L * s, "rounded circle encloses every point within 1e-9 of the scale"); }}
// The exact algorithm through run() with a forced budget, against the public result.
void budgetCheck(const EnclosingCircle &c, lng budget) {
    int n = int(pts.size()), m = 0; vector<int> o; vector<point> q(n); array<point, 3> w; array<int, 3> s;
    auto load = [&] { for (int i = 0; i < n; ++i) { q[i] = pts[o[i]]; } };
    auto fit = [&](const array<int, 3> &t, int k) { m = k; for (int i = 0; i < k; ++i) { w[i] = q[t[i]]; } };
    int k = enclosing_detail::run(n, 5, budget, o, s, load, fit, [&](int i) { return enclosing_detail::locate(w.data(), m, q[i]) > 0; });
    EnclosingCircle e;
    for (int t = 0; t < k; ++t) { e.points.push_back(q[s[t]]); }
    require(e.center() == c.center() && e.radius2() == c.radius2(), "a run past its move-to-front budget restarts and finds the same circle");
    array<int, 3> u;
    if (n >= 2) { require(enclosing_detail::welzl(n, u, true, 0, fit, [&](int) { return true; }) == 0, "welzl reports an exhausted budget"); }}
vector<point> randomPoints(std::mt19937_64 &rng, lng c, int n) {
    vector<point> p(n);
    for (auto &x : p) { x = {lng(rng() % ulng(2 * c + 1)) - c, lng(rng() % ulng(2 * c + 1)) - c}; }
    return p;}
void invalid(const string &name) {
    if (name == "empty") { (void)minimumEnclosingCircle(vector<point>{}); }
    if (name == "coordinate-bound") { (void)minimumEnclosingCircle(vector<point>{{2000000000, 0}}); }
    if (name == "no-support") { EnclosingCircle c; (void)c.locate(point{0, 0}); }
    if (name == "radius-no-support") { EnclosingCircle c; (void)c.radius2(); }
    if (name == "approx-empty") { (void)minimumEnclosingCircleApprox(vector<Point2<double>>{}); }
    if (name == "approx-nonfinite") {
        // A NaN that the shuffle does not place first is never selected by the tests, so only the entry check rejects it.
        vector<Point2<double>> f{{0, 0}, {1, 0}, {0, 1}, {1, 1}, {2, 0}, {0, 2}, {2, 2}, {1, 2}, {2, 1}};
        f.push_back({std::numeric_limits<double>::quiet_NaN(), 1}); (void)minimumEnclosingCircleApprox(f, 1);}
    std::exit(2);}
// Python oracle rows: "E n x y ..." (integers) prints the exact squared radius and centre; "F n x y ..." (doubles) prints the rounded circle.
void oracle() {
    string kind; int n;
    std::cout << std::setprecision(21);
    while (std::cin >> kind >> n) {
        if (kind == "E") {
            vector<point> v(n);
            for (auto &q : v) { std::cin >> q.x >> q.y; }
            auto c = minimumEnclosingCircle(v, seed); RationalPoint2 z = c.center();
            std::cout << c.radius2().toString() << ' ' << InfInt(z.x).toString() << ' ' << InfInt(z.y).toString() << ' ' << InfInt(z.d).toString() << '\n';}
        else {
            vector<Point2<double>> v(n);
            for (auto &q : v) { std::cin >> q.x >> q.y; }
            auto c = minimumEnclosingCircleApprox(v, seed);
            std::cout << c.c.x << ' ' << c.c.y << ' ' << c.r << '\n';}}}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }
        else if (a == "--oracle") { oracle(); return 0; }}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    phase = "fixed";
    pts = {{3, 4}}; auto c = minimumEnclosingCircle(pts, 1); structureCheck(c); bruteCheck(c); approxCheck(1);
    require(c.circle.r == 0 && c.support == vector<int>{0} && c.radius2() == Rational<InfInt>(InfInt(0)), "single point");
    pts = {{5, 5}, {5, 5}, {5, 5}}; c = minimumEnclosingCircle(pts, 2); structureCheck(c); bruteCheck(c); approxCheck(2);
    pts = {{0, 0}, {4, 0}, {2, 0}, {1, 0}}; c = minimumEnclosingCircle(pts, 3); structureCheck(c); bruteCheck(c); approxCheck(3);
    require(c.points.size() == 2 && c.center() == RationalPoint2(2, 0) && c.radius2() == Rational<InfInt>(InfInt(4)), "collinear points use the extreme pair");
    pts = {{0, 0}, {2, 0}, {0, 2}, {2, 2}, {1, 1}}; c = minimumEnclosingCircle(pts, 4); structureCheck(c); bruteCheck(c); approxCheck(4);
    pts = {{0, 0}, {6, 0}, {3, 1}}; c = minimumEnclosingCircle(pts, 5); structureCheck(c); bruteCheck(c); approxCheck(5);
    require(c.points.size() == 2, "obtuse triangle uses its longest side");
    pts = {{0, 0}, {4, 0}, {2, 3}}; c = minimumEnclosingCircle(pts, 6); structureCheck(c); bruteCheck(c); approxCheck(6);
    require(c.points.size() == 3 && c.center() == RationalPoint2(12, 5, 6) && c.radius2() == Rational<InfInt>(InfInt(169), InfInt(36)), "acute triangle circumcentre");
    phase = "cocircular";
    pts.clear();
    for (lng x = -25; x <= 25; ++x) { for (lng y = -25; y <= 25; ++y) { if (x * x + y * y == 625) { pts.push_back({x, y}); } } }
    for (int t = 0; t < 20; ++t) {
        std::shuffle(pts.begin(), pts.end(), rng); ulng sd = rng(); c = minimumEnclosingCircle(pts, sd);
        structureCheck(c); hullCheck(c); approxCheck(sd);
        require(c.center() == RationalPoint2(0, 0) && c.radius2() == Rational<InfInt>(InfInt(625)), "radius 25 lattice circle");}
    phase = "small brute";
    int rounds = quick ? 400 : stress ? 60000 : 8000;
    for (int t = 0; t < rounds; ++t) {
        pts = randomPoints(rng, t % 3 ? 3 : 20, 1 + int(rng() % 9));
        if (t % 5 == 0) { pts.push_back(pts[0]); }
        ulng sd = rng(); c = minimumEnclosingCircle(pts, sd); structureCheck(c); bruteCheck(c); approxCheck(sd);
        if (t % 4 == 0) { budgetCheck(c, lng(rng() % (2 * pts.size()))); }}
    phase = "seed independence";
    for (int t = 0; t < rounds / 8; ++t) {
        pts = randomPoints(rng, 1000, 2 + int(rng() % 30));
        auto a = minimumEnclosingCircle(pts, 7), b = minimumEnclosingCircle(pts, 8), a2 = minimumEnclosingCircle(pts, 7);
        require(a.center() == b.center() && a.support == a2.support, "same circle for every seed, same support for one seed");
        require(a.radius2() == b.radius2(), "same radius for every seed");
        auto f = minimumEnclosingCircleApprox(vector<Point2<double>>(pts.size()), 7);
        require(f.r == 0, "all-equal floating points give radius 0");}
    phase = "medium hull criterion";
    for (int t = 0; t < rounds / 4; ++t) {
        pts = randomPoints(rng, 100000000, 3 + int(rng() % 60));
        ulng sd = rng(); c = minimumEnclosingCircle(pts, sd); structureCheck(c); hullCheck(c); approxCheck(sd);
        if (t % 8 == 0) { budgetCheck(c, lng(rng() % (4 * pts.size()))); }}
    phase = "extreme";
    lng B = 1000000000;
    pts = {{-B, -B}, {B, -B}, {B, B}, {-B, B}, {0, 0}, {B - 1, B}, {-B, 1 - B}}; c = minimumEnclosingCircle(pts, 9); structureCheck(c); approxCheck(9);
    require(c.center() == RationalPoint2(0, 0) && c.radius2() == Rational<InfInt>(InfInt(2) * InfInt(B) * InfInt(B)), "square corners at 10^9");
    pts = {{-B, -B}, {B, -B + 1}, {-B + 1, B}}; c = minimumEnclosingCircle(pts, 10); structureCheck(c); approxCheck(10);
    for (int t = 0; t < (quick ? 20 : 400); ++t) {
        pts = randomPoints(rng, B, 2 + int(rng() % 40)); ulng sd = rng(); c = minimumEnclosingCircle(pts, sd); structureCheck(c); hullCheck(c); approxCheck(sd);
        for (auto q : pts) { require(c.locate(q) == wideSide(c, q), "locate equals the 256-bit exact side at 10^9"); }}
    vector<Point2<int>> ip{{0, 0}, {10, 0}, {0, 10}};
    auto ic = minimumEnclosingCircle(ip, 11); pts = {{0, 0}, {10, 0}, {0, 10}};
    require(ic.center() == RationalPoint2(5, 5) && ic.support.size() == 2, "int coordinates");
    phase = "floating";
    for (int t = 0; t < rounds / 8; ++t) {
        int n = 1 + int(rng() % 40); vector<Point2<double>> f(n); long double s = 1;
        for (auto &q : f) { q = {std::ldexp(double(rng() % 2000001) - 1e6, -int(rng() % 40)), std::ldexp(double(rng() % 2000001) - 1e6, -int(rng() % 40))}; s = max({s, (long double)std::abs(q.x), (long double)std::abs(q.y)}); }
        auto a = minimumEnclosingCircleApprox(f, rng()), b = minimumEnclosingCircleApprox(f, rng());
        require(std::abs(a.r - b.r) <= 1e-12L * s, "floating radius is seed independent");
        for (auto q : f) { require(distanceApprox(q.cast<long double>(), a.c) <= a.r + 1e-12L * s, "floating circle encloses every point"); }}
    phase = "large";
    int n = quick ? 20000 : stress ? 2000000 : 400000; pts.clear();
    for (int i = 0; i < n; ++i) {
        long double a = 2 * std::numbers::pi_v<long double> * (long double)(rng() % 1000000) / 1000000, r = 9e8L * std::sqrt((long double)(rng() % 1000001) / 1000000);
        pts.push_back({std::llround(r * std::cos(a)), std::llround(r * std::sin(a))});}
    sort(pts.begin(), pts.end());
    c = minimumEnclosingCircle(pts, 12);
    for (auto q : pts) { require(c.contains(q), "large set enclosed"); }
    require(c.circle.r <= 9.0001e8L && c.circle.r >= 8.9e8L, "large set radius");
    structureCheck(c); hullCheck(c); approxCheck(12); budgetCheck(c, lng(n));
    pts.assign(n / 4, point{7, -7}); c = minimumEnclosingCircle(pts, 13);
    require(c.support.size() == 1 && c.circle.r == 0, "many duplicates");
    approxCheck(13);
    pts.clear();
    for (int i = 0; i < n / 4; ++i) { pts.push_back({lng(i) * 3 - 500000, lng(i) * 2}); }
    c = minimumEnclosingCircle(pts, 14);
    require(c.support.size() == 2 && c.center() == RationalPoint2(lll(pts.front().x) + pts.back().x, lll(pts.front().y) + pts.back().y, 2), "long collinear run");
    approxCheck(14);
    pts = {};
    std::cout << "PASS minimumenclosingcircle seed=" << seed << " mode=" << mode << " checks=" << checks << " worst approx error=" << double(worst) << '\n';}
