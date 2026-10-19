#include "../../03-Geometry/19-lattice_geometry.hpp"
#include "../../03-Geometry/03-polygon.hpp"
#include "../../05-Mathematics/34-quadratic_congruence.hpp"

// Oracles: exact bounding-box enumeration, per-column rational floors, Pick (03), gaussCircleCount (05/34), closed-form floor sums.
static ulng seed = 20260927;
static std::string phase;
static lng checks = 0;
static std::string context;
void require(bool ok, const char *what) {
    ++checks; if (ok) { return; }
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " input=" << context << " expected=true actual=false\n";
    std::exit(1);}
std::string str(lll v) {
    if (v == 0) { return "0"; }
    bool neg = v < 0; std::string s;
    for (ulll u = neg ? ulll(-v) : ulll(v); u; u /= 10) { s += char('0' + int(u % 10)); }
    if (neg) { s += '-'; }
    return std::string(s.rbegin(), s.rend());}
lll fdiv(lll a, lll b) { return a / b - ((a % b != 0) && ((a < 0) != (b < 0))); }
lll cdiv(lll a, lll b) { return -fdiv(-a, b); }

lll diskBrute(lng a, lng b, lng q, lng n) {
    lll r = 0; while (r * r <= n) { ++r; }
    lll res = 0;
    for (lll x = cdiv(a - r, q); x <= fdiv(a + r, q); ++x) {
        for (lll y = cdiv(b - r, q); y <= fdiv(b + r, q); ++y) {
            lll u = q * x - a, v = q * y - b; res += u * u + v * v <= n;}}
    return res;}
void diskCase(lng a, lng b, lng q, lng n, bool brute) {
    context = "a=" + std::to_string(a) + " b=" + std::to_string(b) + " q=" + std::to_string(q) + " n=" + std::to_string(n);
    lll slow = latticeInDisk(a, b, q, n), fast = latticeInDiskFast(a, b, q, n);
    if (brute) { require(slow == diskBrute(a, b, q, n), "latticeInDisk equals enumeration"); }
    require(fast == slow, "latticeInDiskFast equals latticeInDisk");}

// Exact location of the lattice point (x, y) against a polygon with rational vertices: -1 inside, 0 boundary, 1 outside.
int locateRational(const vector<RationalPoint2> &v, lll x, lll y) {
    int n = int(v.size()); bool in = false;
    for (int i = 0; i < n; ++i) {
        const auto &p = v[i], &q = v[(i + 1) % n];
        // Scaled coordinates over the common denominator p.d * q.d.
        lll D = p.d * q.d, px = p.x * q.d, py = p.y * q.d, qx = q.x * p.d, qy = q.y * p.d, X = x * D, Y = y * D;
        lll c = (qx - px) * (Y - py) - (qy - py) * (X - px);
        if (c == 0 && min(px, qx) <= X && X <= max(px, qx) && min(py, qy) <= Y && Y <= max(py, qy)) { return 0; }
        if ((py > Y) != (qy > Y)) {
            bool right = (qy > py) ? c > 0 : c < 0;
            if (right) { in = !in; }}}
    return in ? -1 : 1;}
void polygonCase(const vector<RationalPoint2> &v) {
    context = "poly=";
    for (const auto &p : v) { context += "(" + str(p.x) + "/" + str(p.d) + "," + str(p.y) + "/" + str(p.d) + ")"; }
    lll lx = LLONG_MAX, hx = LLONG_MIN, ly = LLONG_MAX, hy = LLONG_MIN;
    for (const auto &p : v) { lx = min(lx, fdiv(p.x, p.d)); hx = max(hx, cdiv(p.x, p.d)); ly = min(ly, fdiv(p.y, p.d)); hy = max(hy, cdiv(p.y, p.d)); }
    lll in = 0, on = 0;
    for (lll x = lx; x <= hx; ++x) { for (lll y = ly; y <= hy; ++y) { int s = locateRational(v, x, y); in += s < 0; on += s == 0; } }
    require(latticeInRationalPolygon(v, false) == in, "latticeInRationalPolygon open equals enumeration");
    require(latticeInRationalPolygon(v, true) == in + on, "latticeInRationalPolygon closed equals enumeration");
    vector<RationalPoint2> w(v.rbegin(), v.rend());
    require(latticeInRationalPolygon(w, true) == in + on, "orientation independence");}
void underCase(const RationalPoint2 &p, const RationalPoint2 &q) {
    context = "p=" + str(p.x) + "/" + str(p.d) + "," + str(p.y) + "/" + str(p.d) + " q=" + str(q.x) + "/" + str(q.d) + "," + str(q.y) + "/" + str(q.d);
    lll X1 = p.x * q.d, X2 = q.x * p.d, Y1 = p.y * q.d, Y2 = q.y * p.d, D = p.d * q.d, want = 0;
    for (lll x = cdiv(min(X1, X2), D); x <= fdiv(max(X1, X2), D); ++x) {
        lll xx = x * D;
        want += fdiv(Y1 * (X2 - xx) + Y2 * (xx - X1), (X2 - X1) * D);}
    require(latticeUnderLine(p, q) == want, "latticeUnderLine equals per-column floors");
    require(latticeUnderLine(q, p) == want, "latticeUnderLine symmetric");}
void halfplaneCase(const vector<Halfplane> &h) {
    context = "h=";
    for (const auto &x : h) { context += "[" + std::to_string(x.a.x) + "," + std::to_string(x.a.y) + "->" + std::to_string(x.b.x) + "," + std::to_string(x.b.y) + "]"; }
    lll in = 0, cl = 0;
    for (lng x = -12; x <= 12; ++x) {
        for (lng y = -12; y <= 12; ++y) {
            bool c = true, s = true;
            for (const auto &z : h) { c &= z.contains(point{x, y}); s &= z.contains(point{x, y}, true); }
            cl += c; in += s;}}
    require(latticeInHalfplanePolygon(h, true) == cl, "latticeInHalfplanePolygon closed equals enumeration");
    require(latticeInHalfplanePolygon(h, false) == in, "latticeInHalfplanePolygon open equals enumeration");}
// Lattice points of [-W, W]^2 in the closed (or open) intersection, by exact per-column bounds on y.
lll windowCount(const vector<Halfplane> &h, lll W, bool strict) {
    lll res = 0;
    for (lll x = -W; x <= W; ++x) {
        lll lo = -W, hi = W;
        for (const auto &s : h) {
            // cross(b - a, (x, y) - a) >= 0 (> 0 when strict) is wx (y - a.y) >= wy (x - a.x).
            lll wx = s.b.x - s.a.x, c = lll(s.b.y - s.a.y) * (x - s.a.x);
            if (wx > 0) { lo = max(lo, s.a.y + (strict ? fdiv(c, wx) + 1 : cdiv(c, wx))); }
            else if (wx < 0) { hi = min(hi, s.a.y + (strict ? cdiv(c, wx) - 1 : fdiv(c, wx))); }
            else if (strict ? c >= 0 : c > 0) { hi = lo - 1; }}
        res += max<lll>(0, hi - lo + 1);}
    return res;}
// Unbounded sets hold 0 or infinitely many lattice points: windows past every vertex grow exactly when the count is infinite.
void unboundedCase(const vector<Halfplane> &h) {
    context = "h=";
    for (const auto &x : h) { context += "[" + std::to_string(x.a.x) + "," + std::to_string(x.a.y) + "->" + std::to_string(x.b.x) + "," + std::to_string(x.b.y) + "]"; }
    auto r = halfplaneIntersection(h);
    lll V = 0;
    for (const auto &p : r.vertices) { V = max({V, cdiv(abs(p.x), p.d), cdiv(abs(p.y), p.d)}); }
    for (int closed = 0; closed < 2; ++closed) {
        lll small = windowCount(h, 2 * V + 60, !closed), large = windowCount(h, 4 * V + 120, !closed), got = latticeInHalfplanePolygon(h, closed);
        require(small == large ? small == 0 && got == 0 : got == -1, "unbounded count is 0 or infinite (-1) as the windows show");}}
RationalPoint2 rq(std::mt19937_64 &rng, int c, int d) { lng den = 1 + lng(rng() % ulng(d)); return RationalPoint2(lll(rng() % ulng(2 * c * den + 1)) - c * den, lll(rng() % ulng(2 * c * den + 1)) - c * den, den); }
vector<RationalPoint2> star(std::mt19937_64 &rng, int c, int d, int n) {
    // Distinct angles around a rational centre give a simple polygon.
    vector<pair<long double, RationalPoint2>> a;
    RationalPoint2 o = rq(rng, 2, d);
    while (int(a.size()) < n) {
        RationalPoint2 p = rq(rng, c, d); dpoint u = p.approx() - o.approx();
        if (u == dpoint{}) { continue; }
        long double t = std::atan2(u.y, u.x); bool dup = false;
        for (auto &x : a) { dup |= std::abs(x.first - t) < 1e-9L; }
        if (!dup) { a.push_back({t, p}); }}
    sort(a.begin(), a.end(), [](auto &x, auto &y) { return x.first < y.first; });
    vector<RationalPoint2> v;
    for (auto &x : a) { v.push_back(x.second); }
    return v;}
// Exact simplicity on the common-denominator lattice: adjacent edges meet only at their shared vertex, others never meet.
bool simpleNonzero(const vector<RationalPoint2> &v) {
    int n = int(v.size()); lll l = 1;
    for (const auto &p : v) { l = l / gcd(l, p.d) * p.d; }
    if (l > 100000) { return false; }
    vector<point> w;
    for (const auto &p : v) { w.push_back({lng(p.x * (l / p.d)), lng(p.y * (l / p.d))}); }
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            auto x = intersect(Linear2::segment(w[i], w[(i + 1) % n]), Linear2::segment(w[j], w[(j + 1) % n]));
            bool adjacent = j == i + 1 || (i == 0 && j == n - 1);
            if (adjacent ? x.kind != IntersectionKind::Point : x.kind != IntersectionKind::Empty) { return false; }}}
    return signedArea2(w) != 0;}
void invalid(const string &name) {
    if (name == "disk-q") { (void)latticeInDisk(0, 0, 0, 5); }
    if (name == "disk-n") { (void)latticeInDiskFast(0, 0, 1, -1); }
    if (name == "under-vertical") { (void)latticeUnderLine(RationalPoint2(1, 0), RationalPoint2(1, 5)); }
    if (name == "polygon-small") { (void)latticeInRationalPolygon({RationalPoint2(0, 0), RationalPoint2(1, 0)}, true); }
    if (name == "polygon-flat") { (void)latticeInRationalPolygon({RationalPoint2(0, 0), RationalPoint2(1, 0), RationalPoint2(2, 0)}, true); }
    // A bounded triangle whose far vertex (2 * 10^9 * (10^9 - 1), about 2 * 10^18) leaves the |coordinate| <= 10^18 domain.
    if (name == "halfplane-far") { (void)latticeInHalfplanePolygon({Halfplane{{0, 0}, {999999999, 1}}, Halfplane{{1000000000, 3}, {0, 2}}, Halfplane{{0, 2}, {0, 0}}}, true); }
    std::exit(2);}

// 256-bit sign of a * b - c * d, written independently of the header.
int productSign(lll a, lll b, lll c, lll d) {
    auto mag = [](lll a, lll b) {
        ulll x = a < 0 ? ulll(0) - ulll(a) : ulll(a), y = b < 0 ? ulll(0) - ulll(b) : ulll(b);
        ulng xs[2] = {ulng(x), ulng(x >> 64)}, ys[2] = {ulng(y), ulng(y >> 64)}; array<ulng, 4> r{};
        for (int i = 0; i < 2; ++i) {
            ulng carry = 0;
            for (int j = 0; j < 2; ++j) { ulll t = ulll(xs[i]) * ys[j] + r[i + j] + carry; r[i + j] = ulng(t); carry = ulng(t >> 64); }
            r[i + 2] += carry;}
        return pair<int, array<ulng, 4>>{x && y ? ((a < 0) != (b < 0) ? -1 : 1) : 0, r};};
    auto [s1, m1] = mag(a, b); auto [s2, m2] = mag(c, d);
    if (s1 != s2) { return s1 > s2 ? 1 : -1; }
    int cmp = 0;
    for (int i = 3; i >= 0 && !cmp; --i) { cmp = m1[i] < m2[i] ? -1 : m1[i] > m2[i] ? 1 : 0; }
    return s1 * cmp;}
// Exact location of the lattice point (x, y) against a polygon with large denominators: -1 inside, 0 boundary, 1 outside.
int locateWide(const vector<RationalPoint2> &v, lll x, lll y) {
    int n = int(v.size()); bool in = false;
    for (int i = 0; i < n; ++i) {
        const auto &p = v[i], &q = v[(i + 1) % n];
        lll ux = q.x * p.d - p.x * q.d, uy = q.y * p.d - p.y * q.d, vx = x * p.d - p.x, vy = y * p.d - p.y;
        int c = productSign(ux, vy, uy, vx);
        auto inRange = [](lll t, const RationalPoint2 &a, lll an, const RationalPoint2 &b, lll bn) {
            bool ge = productSign(t, a.d, an, 1) >= 0, le = productSign(t, b.d, bn, 1) <= 0, ge2 = productSign(t, b.d, bn, 1) >= 0, le2 = productSign(t, a.d, an, 1) <= 0;
            return (ge && le) || (ge2 && le2);};
        if (c == 0 && inRange(x, p, p.x, q, q.x) && inRange(y, p, p.y, q, q.y)) { return 0; }
        bool pa = y * p.d < p.y, qa = y * q.d < q.y;
        if (pa != qa && (uy > 0 ? c > 0 : c < 0)) { in = !in; }}
    return in ? -1 : 1;}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid(argv[++i]); }
        else if (a == "--oracle") {
            auto parse = [](const string &t) { lll v = 0; bool neg = t[0] == '-'; for (char c : t.substr(neg)) { v = v * 10 + (neg ? '0' - c : c - '0'); } return v; };
            string kind, w[3];
            while (cin >> kind) {
                if (kind == "U") {
                    RationalPoint2 r[2];
                    for (auto &x : r) { cin >> w[0] >> w[1] >> w[2]; x = RationalPoint2(parse(w[0]), parse(w[1]), parse(w[2])); }
                    std::cout << str(latticeUnderLine(r[0], r[1])) << '\n';}
                else {
                    int closed, n; cin >> closed >> n; vector<RationalPoint2> v(n);
                    for (auto &x : v) { cin >> w[0] >> w[1] >> w[2]; x = RationalPoint2(parse(w[0]), parse(w[1]), parse(w[2])); }
                    std::cout << str(latticeInRationalPolygon(v, closed)) << '\n';}}
            return 0;}}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    phase = "disk exhaustive";
    for (lng q = 1; q <= (quick ? 2 : 4); ++q) {
        for (lng a = -2 * q; a <= 2 * q; ++a) {
            for (lng b = -2 * q; b <= 2 * q; ++b) {
                for (lng n = 0; n <= (quick ? 20 : 70); ++n) { diskCase(a, b, q, n, true); }}}}
    phase = "disk random";
    int rounds = quick ? 300 : stress ? 40000 : 5000;
    for (int t = 0; t < rounds; ++t) {
        lng q = 1 + lng(rng() % 12), a = lng(rng() % 2001) - 1000, b = lng(rng() % 2001) - 1000, n = lng(rng() % ulng(q * q * 900 + 1));
        diskCase(a, b, q, n, true);}
    phase = "disk large";
    for (int t = 0; t < (quick ? 4 : stress ? 400 : 60); ++t) {
        lng q = 1 + lng(rng() % 1000), a = lng(rng() % 2000000001) - 1000000000, b = lng(rng() % 2000000001) - 1000000000;
        lng n = lng(rng() % ulng((quick ? 100000LL : 1000000LL) * q)) * q;
        diskCase(a, b, q, n, false);}
    phase = "disk wide radius";
    for (int t = 0; t < (quick ? 1 : stress ? 40 : 4); ++t) {
        lng q = 1 + lng(rng() % 9), r = 300000 + lng(rng() % 700000), a = lng(rng() % 2000001) - 1000000, b = lng(rng() % 2000001) - 1000000;
        diskCase(a, b, q, r * r * q * q + lng(rng() % ulng(r * q)), false);}
    phase = "disk origin";
    for (lng n : {0LL, 1LL, 2LL, 25LL, 1000000LL, 999999937LL, 1000000000000LL}) {
        context = "n=" + std::to_string(n);
        require(latticeInDiskFast(0, 0, 1, n) == gaussCircleCount(n), "latticeInDiskFast equals gaussCircleCount at the origin");
        require(latticeInDiskFast(7 * 3, -5 * 3, 3, 9 * n) == gaussCircleCount(n), "integral centre equals a translated origin count");
        if (n <= 1000000) { require(latticeInDisk(0, 0, 1, n) == gaussCircleCount(n), "latticeInDisk equals gaussCircleCount"); }}
    for (auto [r, want] : vector<pair<lng, lll>>{{1000, 3141549}, {10000, 314159053}, {1000000, 3141592649625LL}}) {
        context = "r=" + std::to_string(r);
        require(latticeInDiskFast(0, 0, 1, r * r) == want, "known Gauss circle values");}
    if (!quick) {
        context = "extreme";
        lng n = 4000000000000000000LL;
        require(latticeInDiskFast(0, 0, 1, n) == gaussCircleCount(n), "n = 4 * 10^18 at the origin");
        require(latticeInDiskFast(1000000000000000000LL - 1, -1000000000000000000LL + 3, 1000000000000000000LL, n) == latticeInDisk(1000000000000000000LL - 1, -1000000000000000000LL + 3, 1000000000000000000LL, n), "extreme denominator");}

    phase = "under line";
    for (int t = 0; t < rounds; ++t) {
        RationalPoint2 p = rq(rng, 8, t % 3 ? 7 : 1), q = rq(rng, 8, t % 3 ? 7 : 1);
        if (p.x * q.d == q.x * p.d) { continue; }
        underCase(p, q);}
    context = "wide";
    require(latticeUnderLine(RationalPoint2(-1000000000, 0), RationalPoint2(1000000000, 1)) == 0 + 1, "wide segment");

    phase = "rational polygon";
    for (int t = 0; t < rounds; ++t) {
        int n = 3 + int(rng() % 7); auto v = star(rng, 6, t % 4 == 0 ? 1 : t % 4 == 1 ? 2 : 6, n);
        if (simpleNonzero(v)) { polygonCase(v); }}
    phase = "rational polygon large denominators";
    for (int t = 0; t < rounds / 4; ++t) {
        // Points near a circle of radius 1.9 with denominators up to 10^6: max value 2 times 10^18 = the domain edge.
        int m = 3 + int(rng() % 6); vector<pair<long double, RationalPoint2>> a;
        for (int i = 0; i < m; ++i) {
            long double ang = 2 * std::numbers::pi_v<long double> * (long double)(rng() % 1000000) / 1000000; lng den = 1 + lng(rng() % 1000000);
            a.push_back({ang, RationalPoint2(std::llround(1.9L * den * std::cos(ang)), std::llround(1.9L * den * std::sin(ang)), den)});}
        sort(a.begin(), a.end(), [](auto &x, auto &y) { return x.first < y.first; });
        vector<RationalPoint2> v;
        for (auto &x : a) { v.push_back(x.second); }
        auto orient3 = [](const RationalPoint2 &p, const RationalPoint2 &q, const RationalPoint2 &r) {
            lll ux = q.x * p.d - p.x * q.d, uy = q.y * p.d - p.y * q.d, vx = r.x * p.d - p.x * r.d, vy = r.y * p.d - p.y * r.d;
            return ux * vy - uy * vx > 0;};
        bool convex = true;
        for (int i = 0; i < m; ++i) { convex &= orient3(v[i], v[(i + 1) % m], v[(i + 2) % m]); }
        if (convex) { polygonCase(v); }}
    phase = "rational polygon huge denominators";
    for (int t = 0; t < rounds / 4; ++t) {
        // Values within 2 and denominators up to 2 * 10^18: max value times max denominator at the domain edge 4 * 10^18.
        int m = 3 + int(rng() % 6); vector<pair<long double, RationalPoint2>> a;
        for (int i = 0; i < m; ++i) {
            long double ang = 2 * std::numbers::pi_v<long double> * (long double)(rng() % 1000000) / 1000000; lll den = 1 + lll(rng() % 2000000000000000000ULL);
            a.push_back({ang, RationalPoint2(lll(std::llround(1.9L * std::cos(ang) * (long double)den)), lll(std::llround(1.9L * std::sin(ang) * (long double)den)), den)});}
        sort(a.begin(), a.end(), [](auto &x, auto &y) { return x.first < y.first; });
        vector<RationalPoint2> v;
        for (auto &x : a) { v.push_back(x.second); }
        bool convex = true;
        for (int i = 0; i < m; ++i) {
            const auto &p = v[i], &q = v[(i + 1) % m], &r = v[(i + 2) % m];
            convex &= productSign(q.x * p.d - p.x * q.d, r.y * p.d - p.y * r.d, q.y * p.d - p.y * q.d, r.x * p.d - p.x * r.d) > 0;}
        if (!convex) { continue; }
        context = "huge denominators";
        lll in = 0, on = 0;
        for (lll x = -3; x <= 3; ++x) { for (lll y = -3; y <= 3; ++y) { int z = locateWide(v, x, y); in += z < 0; on += z == 0; } }
        require(latticeInRationalPolygon(v, false) == in && latticeInRationalPolygon(v, true) == in + on, "huge denominators equal enumeration");}
    phase = "spiral";
    {
        // A unit-width rectilinear spiral at |coordinate| < 4 * 10^18: same-signed edge sums pass 2^127 before cancelling. Pick's formula is the oracle.
        lll V = 4000000000000000000LL - 100; int L = 3; vector<pair<lll, lll>> S, T, P;
        for (int i = 0; i < L; ++i) { S.push_back({-V + 2 * i, -V + 2 * i}); S.push_back({V - 2 * i, -V + 2 * i}); S.push_back({V - 2 * i, V - 2 * i}); S.push_back({-V + 2 * i + 2, V - 2 * i}); }
        T.push_back({-V, -V + 1});
        for (int i = 0; i < L; ++i) {
            if (i) { T.push_back({-V + 2 * i + 1, -V + 2 * i + 1}); }
            T.push_back({V - 2 * i - 1, -V + 2 * i + 1}); T.push_back({V - 2 * i - 1, V - 2 * i - 1}); T.push_back({-V + 2 * i + 3, V - 2 * i - 1});}
        T.push_back({-V + 2 * L, V - 2 * L + 1});
        vector<pair<lll, lll>> all = S; all.insert(all.end(), T.rbegin(), T.rend());
        for (int k = 0; k < int(all.size()); ++k) { if (all[k] != all[(k + int(all.size()) - 1) % all.size()]) { P.push_back(all[k]); } }
        ulll area = 0; lll boundary = 0; int m = int(P.size()); vector<RationalPoint2> v;
        for (int k = 0; k < m; ++k) {
            auto [x1, y1] = P[k]; auto [x2, y2] = P[(k + 1) % m];
            area += ulll(x1) * ulll(y2) - ulll(x2) * ulll(y1); boundary += gcd(abs(x2 - x1), abs(y2 - y1)); v.push_back(RationalPoint2(x1, y1));}
        lll a2 = abs(lll(area)), interior = (a2 - boundary + 2) / 2;
        context = "spiral";
        require(latticeInRationalPolygon(v, false) == interior && latticeInRationalPolygon(v, true) == interior + boundary, "wide spiral equals Pick");}
    phase = "halfplane against rational vertices";
    for (int t = 0; t < rounds / 4; ++t) {
        lng B = 500000;
        vector<Halfplane> h{{{-B, -B}, {B, -B}}, {{B, -B}, {B, B}}, {{B, B}, {-B, B}}, {{-B, B}, {-B, -B}}};
        for (int i = 0; i < 4; ++i) {
            point a{lng(rng() % (2 * B + 1)) - B, lng(rng() % (2 * B + 1)) - B}, b{lng(rng() % (2 * B + 1)) - B, lng(rng() % (2 * B + 1)) - B};
            if (a != b) { h.push_back({a, b}); }}
        auto r = halfplaneIntersection(h);
        if (r.kind != HalfplaneKind::Polygon) { continue; }
        context = "halfplane polygon";
        for (int closed = 0; closed < 2; ++closed) {
            require(latticeInHalfplanePolygon(h, closed) == latticeInRationalPolygon(r.vertices, closed), "integral lines and rational vertices agree");}
        auto shifted = r.vertices; lll dx = lll(rng() % 2000001) - 1000000, dy = lll(rng() % 2000001) - 1000000;
        for (auto &p : shifted) { p = RationalPoint2(p.x + dx * p.d, p.y + dy * p.d, p.d); }
        require(latticeInRationalPolygon(shifted, true) == latticeInRationalPolygon(r.vertices, true), "integral translation preserves the count");
        for (auto &p : shifted) { p = RationalPoint2(p.y, p.x, p.d); }
        require(latticeInRationalPolygon(shifted, false) == latticeInRationalPolygon(r.vertices, false), "translation and transposition preserve the count");}
    phase = "rational polygon axis";
    for (int t = 0; t < rounds / 4; ++t) {
        lng x0 = lng(rng() % 9) - 4, y0 = lng(rng() % 9) - 4, d = 1 + lng(rng() % 3);
        lng w = 1 + lng(rng() % 12), h = 1 + lng(rng() % 12);
        // Rectilinear L-shape with vertical and horizontal edges through lattice and half-lattice points.
        vector<RationalPoint2> v{{x0 * d, y0 * d, d}, {x0 * d + w, y0 * d, d}, {x0 * d + w, y0 * d + h / 2 + 1, d}, {x0 * d + w / 2 + 1, y0 * d + h / 2 + 1, d}, {x0 * d + w / 2 + 1, y0 * d + h + 2, d}, {x0 * d, y0 * d + h + 2, d}};
        if (simpleNonzero(v)) { polygonCase(v); }}
    phase = "lattice polygon pick";
    for (int t = 0; t < rounds / 2; ++t) {
        auto v = star(rng, 30, 1, 3 + int(rng() % 9));
        if (!simpleNonzero(v)) { continue; }
        vector<point> p;
        for (auto &x : v) { p.push_back({lng(x.x), lng(x.y)}); }
        context = "lattice polygon";
        lll interior = latticeInterior(p), boundary = latticeBoundary(p);
        require(latticeInRationalPolygon(v, false) == interior && latticeInRationalPolygon(v, true) == interior + boundary, "Pick agreement");}
    {
        phase = "rational polygon wide"; context = "triangle";
        vector<RationalPoint2> v{{-1000000000, -1000000000}, {1000000000, -1000000000}, {-1000000000, 1000000000}};
        vector<point> p{{-1000000000, -1000000000}, {1000000000, -1000000000}, {-1000000000, 1000000000}};
        require(latticeInRationalPolygon(v, true) == latticeInterior(p) + latticeBoundary(p), "wide lattice triangle");
        // Triangle (1/2, 0), (X + 1/2, 0), (1/2, Y): closed count X + sum over x in [1, X] of floor(Y * (2 * X + 1 - 2 * x) / (2 * X)).
        lng X = 1000000000, Y = 999999999;
        vector<RationalPoint2> h{{1, 0, 2}, {2 * X + 1, 0, 2}, {1, 2 * Y, 2}};
        lll want = X + floorSum(X, 2 * X, -2 * lll(Y), lll(Y) * (2 * X - 1));
        require(latticeInRationalPolygon(h, true) == want, "half-integer triangle closed count");
        require(latticeInRationalPolygon(h, true) - latticeInRationalPolygon(h, false) == X, "half-integer triangle boundary");}

    phase = "halfplane";
    for (int t = 0; t < rounds; ++t) {
        vector<Halfplane> h{{{-10, -10}, {10, -10}}, {{10, -10}, {10, 10}}, {{10, 10}, {-10, 10}}, {{-10, 10}, {-10, -10}}};
        int m = int(rng() % 6);
        for (int i = 0; i < m; ++i) {
            point a{lng(rng() % 13) - 6, lng(rng() % 13) - 6}, b{lng(rng() % 13) - 6, lng(rng() % 13) - 6};
            if (a != b) { h.push_back({a, b}); }}
        if (t % 5 == 0) { h.push_back({{0, 0}, {1, 1}}); h.push_back({{1, 1}, {0, 0}}); }
        if (t % 7 == 0) { h.push_back({{3, 0}, {3, 1}}); h.push_back({{3, 1}, {3, 0}}); h.push_back({{0, 2}, {1, 2}}); h.push_back({{1, 2}, {0, 2}}); }
        halfplaneCase(h);}
    phase = "halfplane unbounded";
    for (int t = 0; t < rounds / 5; ++t) {
        vector<Halfplane> h;
        int m = int(rng() % 4);
        for (int i = 0; i < m; ++i) {
            point a{lng(rng() % 11) - 5, lng(rng() % 11) - 5}, b{lng(rng() % 11) - 5, lng(rng() % 11) - 5};
            if (a != b) { h.push_back({a, b}); }}
        // Doubled lines give rays and lines; near-parallel pairs give strips and half-strips of lattice width 1 and 2.
        if (t % 3 == 0 && !h.empty()) { h.push_back({h[0].b, h[0].a}); }
        if (t % 4 == 1) {
            point d{lng(rng() % 7) - 3, lng(rng() % 7) - 3}, a{lng(rng() % 7) - 3, lng(rng() % 7) - 3}, e{lng(rng() % 5) - 2, lng(rng() % 5) - 2};
            if (d != point{}) { h.push_back({a, a + d}); h.push_back({a + e + d, a + e}); }}
        auto kind = halfplaneIntersection(h).kind;
        if (kind == HalfplaneKind::Ray || kind == HalfplaneKind::Line || kind == HalfplaneKind::Unbounded) { unboundedCase(h); }}
    {
        context = "unbounded fixed";
        auto both = [&](const vector<Halfplane> &h, lll open, lll closed) {
            require(latticeInHalfplanePolygon(h, false) == open && latticeInHalfplanePolygon(h, true) == closed, "fixed unbounded counts");
            unboundedCase(h);};
        both({}, -1, -1);
        both({{{0, 0}, {1, 0}}}, -1, -1);
        both({{{0, 0}, {1, 0}}, {{1, 1}, {0, 1}}}, 0, -1);
        both({{{0, 0}, {1, 0}}, {{1, 2}, {0, 2}}}, -1, -1);
        both({{{0, 0}, {4, 2}}, {{3, 2}, {1, 1}}}, 0, -1);
        both({{{0, 0}, {2, 1}}, {{2, 2}, {0, 1}}}, -1, -1);
        both({{{0, 0}, {1, 0}}, {{1, 1}, {0, 1}}, {{0, 1}, {0, 0}}}, 0, -1);
        both({{{0, 0}, {1, 0}}, {{1, 2}, {0, 2}}, {{0, 2}, {0, 0}}}, -1, -1);
        both({{{0, 0}, {1, 0}}, {{0, 1}, {0, 0}}}, -1, -1);
        both({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}, {{0, 1}, {0, 0}}}, 0, -1);
        both({{{0, 0}, {1, 0}}, {{1, 0}, {0, 0}}}, 0, -1);
        // Redundant parallel planes in either order: only the tightest strip 5 < y < 6 (width 1) or 5 < y < 7 (width 2) counts.
        both({{{0, 0}, {1, 0}}, {{0, 5}, {1, 5}}, {{1, 6}, {0, 6}}, {{0, 0}, {0, -1}}}, 0, -1);
        both({{{0, 5}, {1, 5}}, {{0, 0}, {1, 0}}, {{1, 6}, {0, 6}}, {{0, 0}, {0, -1}}}, 0, -1);
        both({{{0, 0}, {1, 0}}, {{0, 5}, {1, 5}}, {{1, 7}, {0, 7}}, {{0, 0}, {0, -1}}}, -1, -1);
        both({{{0, 5}, {1, 5}}, {{1, 6}, {0, 6}}, {{1, 9}, {0, 9}}}, 0, -1);}
    {
        context = "far";
        // x >= 0, y >= x / (10^9 - 1), y <= 1 + x / 10^9 meet at (10^9 * (10^9 - 1), 10^9).
        lng B = 1000000000, X = B * (B - 1);
        vector<Halfplane> h{{{0, 0}, {B - 1, 1}}, {{B, 2}, {0, 1}}, {{0, 1}, {0, 0}}};
        require(halfplaneIntersection(h).kind == HalfplaneKind::Polygon, "far polygon is bounded");
        lll want = 2 * lll(X + 1) + floorSum(X + 1, B, 1, 0) - ceilSum(X + 1, B - 1, 1, 0);
        require(latticeInHalfplanePolygon(h, true) == want, "far thin polygon closed count");
        require(latticeInHalfplanePolygon(h, true) - latticeInHalfplanePolygon(h, false) == 2 * B - 1 + 1, "far thin polygon boundary");}
    std::cout << "PASS lattice_geometry seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
