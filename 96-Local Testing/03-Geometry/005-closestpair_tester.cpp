#include "../../03-Geometry/005-closestpair.hpp"

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
template<typename T> lll distance(Point2<T> a, Point2<T> b) { return square(lll(a.x) - b.x) + square(lll(a.y) - b.y); }
void print(point q) { std::cerr << '(' << q.x << ',' << q.y << ')'; }
void print(point3 q) { std::cerr << '(' << q.x << ',' << q.y << ',' << q.z << ')'; }
template<typename T> void print(Point2<T> q) {
    if constexpr (std::is_floating_point_v<T>) {
        std::cerr << std::hexfloat << '(' << static_cast<long double>(q.x) << ',' << static_cast<long double>(q.y) << ')' << std::defaultfloat;}
    else { std::cerr << '(' << +q.x << ',' << +q.y << ')'; }}
template<typename T> string typeName() {
    if constexpr (std::is_floating_point_v<T>) { return sizeof(T) == 4 ? "float" : sizeof(T) == 8 ? "double" : std::is_same_v<T, long double> ? "long double" : "__float128"; }
    else { return "int" + std::to_string(8 * sizeof(T)); }}
ClosestPair solve(const vector<point> &p) { return closestPair(p); }
ClosestPair solve(const vector<point3> &p) { return closestPair3(p); }
template<typename T> ClosestPair solve(const vector<Point2<T>> &p) { return closestPair(p); }
ClosestPair naiveSolve(const vector<point3> &p) { return closestPair3Naive(p); }
template<typename T> ClosestPair naiveSolve(const vector<Point2<T>> &p) { return closestPairNaive(p); }
// Seeds every closestPairRandomized call runs with; 0 and ~0 always, more drawn from a separate generator per mode.
static vector<ulng> seeds{0, ~0ULL};
// Greedy one-point deletion: a locally minimal input on which bad still holds.
template<typename V, typename Bad> V shrink(V p, Bad bad) {
    for (int i = 0; i < int(p.size()); ) {
        auto q = p; q.erase(q.begin() + i);
        if (bad(q)) { p = std::move(q); i = 0; }
        else { ++i; }}
    return p;}
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
// Las Vegas: every seed must return the oracle's ids and distance exactly.
template<typename T> void verifyRandomized(const vector<Point2<T>> &p, ClosestPair want) {
    for (ulng s : seeds) {
        auto got = closestPairRandomized(p, s);
        if (!equal(want, got)) {
            auto q = shrink(p, [&](const vector<Point2<T>> &r) { return !equal(oracle(r), closestPairRandomized(r, s)); });
            string what = "closestPairRandomized (deletion-minimal) type=" + typeName<T>() + " rngseed=" + std::to_string(s);
            require(false, q, oracle(q), closestPairRandomized(q, s), what.c_str());}
        ++checks;}}
// The ordered-set sweep: the O(n^2) oracle's answer, and closestPair's on their shared domain.
template<typename T> void verifySimple(const vector<Point2<T>> &p, ClosestPair want) {
    auto got = closestPairSimple(p);
    if (!equal(want, got)) {
        auto q = shrink(p, [](const vector<Point2<T>> &r) { return !equal(oracle(r), closestPairSimple(r)); });
        string what = "closestPairSimple (deletion-minimal) type=" + typeName<T>();
        require(false, q, oracle(q), closestPairSimple(q), what.c_str());}
    ++checks;
    auto fast = closestPair(p);
    require(equal(fast, got), p, fast, got, "closestPairSimple against closestPair");}
void extra(const vector<point3> &, ClosestPair) {}
template<typename T> void extra(const vector<Point2<T>> &p, ClosestPair want) { verifySimple(p, want); verifyRandomized(p, want); }
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
    require(equal(want, naiveSolve(p)), p, want, naiveSolve(p), "minimum pair, all-pairs scan");
    require(p == before, before, want, got, "input preserved");
    extra(p, want);}
void printIds(pair<int, int> a) { std::cerr << a.first << ',' << a.second; }
void printIds(array<int, 3> a) { std::cerr << a[0] << ',' << a[1] << ',' << a[2]; }
template<typename T, typename R> [[noreturn]] void failure(const string &what, const vector<Point2<T>> &p, R want, R got) {
    std::cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << what << " type=" << typeName<T>() << " n=" << p.size()
              << " expected="; printIds(want); std::cerr << " actual="; printIds(got); std::cerr << " input=";
    for (auto q : p) { print(q); }
    std::cerr << '\n'; std::exit(1);}
// Signs of zeros too; __builtin_signbit also takes __float128, which std::signbit does not.
template<typename T> bool negative(T x) { if constexpr (std::is_floating_point_v<T>) { return __builtin_signbit(x); } else { return x < 0; } }
template<typename T> bool same(const vector<Point2<T>> &a, const vector<Point2<T>> &b) {
    if (a.size() != b.size()) { return false; }
    for (size_t i = 0; i < a.size(); ++i) {
        if (!(a[i] == b[i]) || negative(a[i].x) != negative(b[i].x) || negative(a[i].y) != negative(b[i].y)) { return false; }}
    return true;}
// The contract's computed squared distance: each coordinate widened to long double, subtracted, squared, summed.
template<typename T> long double approx2(Point2<T> a, Point2<T> b) {
    long double dx = static_cast<long double>(a.x) - static_cast<long double>(b.x), dy = static_cast<long double>(a.y) - static_cast<long double>(b.y);
    return dx * dx + dy * dy;}
template<typename T> pair<int, int> approxOracle(const vector<Point2<T>> &p) {
    pair<int, int> best{-1, -1}; long double d = 0;
    for (int i = 0; i < int(p.size()); ++i) {
        for (int j = i + 1; j < int(p.size()); ++j) {
            long double v = approx2(p[i], p[j]);
            if (best.first < 0 || v < d) { best = {i, j}; d = v; }}}
    return best;}
// One extra closestPairApprox seed per case, drawn from a generator seeded by main.
static std::mt19937_64 caseSeeds;
// Las Vegas: the default seed, the shared seeds and one per case must each return the brute-force ids.
vector<ulng> approxSeeds() {
    vector<ulng> all{CustomHash::rnd};
    all.insert(all.end(), seeds.begin(), seeds.end());
    all.push_back(caseSeeds());
    return all;}
template<typename T> void verifyApprox(const vector<Point2<T>> &p) {
    auto before = p; auto want = approxOracle(p);
    for (ulng s : approxSeeds()) {
        auto got = closestPairApprox(p, s);
        ++checks;
        if (want != got) {
            auto q = shrink(p, [&](const vector<Point2<T>> &r) { return approxOracle(r) != closestPairApprox(r, s); });
            failure("closestPairApprox (deletion-minimal) rngseed=" + std::to_string(s), q, approxOracle(q), closestPairApprox(q, s));}
        ++checks;
        if (!same(p, before)) { failure("closestPairApprox input preserved rngseed=" + std::to_string(s), before, want, got); }
        // closestPairApprox takes the grid only from 1024 points; small cases drive it directly, and an answer it gives must be exact.
        if (p.size() < 2) { continue; }
        closest_detail::Best<long double> b;
        ++checks;
        if (closest_detail::incremental(p, b, s) && b.ids != want) {
            auto q = shrink(p, [&](const vector<Point2<T>> &r) {
                closest_detail::Best<long double> c;
                return r.size() >= 2 && closest_detail::incremental(r, c, s) && c.ids != approxOracle(r);});
            closest_detail::Best<long double> c;
            closest_detail::incremental(q, c, s);
            failure("closest_detail::incremental (deletion-minimal) rngseed=" + std::to_string(s), q, approxOracle(q), c.ids);}}}
// O(n^3) oracle with the contract's expression: d(i, j) + d(j, k) + d(i, k) on long double points, first strict minimum in (i, j, k) order.
template<typename T> array<int, 3> triangleOracle(const vector<Point2<T>> &p) {
    int n = int(p.size());
    array<int, 3> best{-1, -1, -1}; long double per = 0;
    vector<long double> d(size_t(n) * size_t(n));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            long double dx = static_cast<long double>(p[i].x) - static_cast<long double>(p[j].x);
            long double dy = static_cast<long double>(p[i].y) - static_cast<long double>(p[j].y);
            d[size_t(i) * size_t(n) + size_t(j)] = std::sqrt(dx * dx + dy * dy);}}
    auto at = [&](int i, int j) { return d[size_t(i) * size_t(n) + size_t(j)]; };
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) {
            for (int k = j + 1; k < n; ++k) {
                long double v = at(i, j) + at(j, k) + at(i, k);
                if (best[0] < 0 || v < per) { best = {i, j, k}; per = v; }}}}
    return best;}
template<typename T> void verifyTriangle(const vector<Point2<T>> &p) {
    auto before = p; auto want = triangleOracle(p), got = minimumPerimeterTriangleApprox(p);
    ++checks;
    if (want != minimumPerimeterTriangleApproxNaive(p)) { failure("minimumPerimeterTriangleApproxNaive", p, want, minimumPerimeterTriangleApproxNaive(p)); }
    ++checks;
    if (want != got) {
        auto q = shrink(p, [](const vector<Point2<T>> &r) { return triangleOracle(r) != minimumPerimeterTriangleApprox(r); });
        failure("minimumPerimeterTriangleApprox (deletion-minimal)", q, triangleOracle(q), minimumPerimeterTriangleApprox(q));}
    ++checks;
    if (!same(p, before)) { failure("minimumPerimeterTriangleApprox input preserved", before, want, got); }}
#ifdef NDEBUG
constexpr bool OPTIMIZED = true;
#else
constexpr bool OPTIMIZED = false;
#endif
// isqrt(2^127 - 1): the widest single-axis span the domain admits; 16 less leaves room for a small second-axis span.
constexpr ulng ROOT = 13043817825332782212ULL;
// Extreme one-axis spans at both ends of T's range, tight clusters, duplicate pools, lattices and diagonals.
template<typename T> void randomizedTyped(std::mt19937_64 &rng, int rounds) {
    using L = std::numeric_limits<T>;
    const ulng span = sizeof(T) == 8 ? ROOT - 16 : ulng(lll(L::max()) - L::min());
    const lll wideBox = sizeof(T) == 8 ? (lll(1) << 62) - 1 : lll(L::max());
    for (int t = 0; t < rounds; ++t) {
        int n = int(rng() % 40), cls = t % 7;
        bool high = t / 7 % 2;
        lll a = high ? lll(L::max()) - lll(span) : lll(L::min()), edge = high ? lll(L::max()) - 3 : lll(L::min());
        vector<Point2<T>> pool;
        for (int k = 0; k < 3; ++k) { pool.push_back({T(edge + lll(rng() % 4)), T(lll(rng() % 3) - 1)}); }
        vector<Point2<T>> p;
        for (int k = 0; k < n; ++k) {
            ulng r = rng() % 3;
            lll wide = r == 0 ? a + lll(span) - lll(rng() % 4) : r == 1 ? a + lll(rng() % 4) : a + lll(rng() % (span + 1));
            lll narrow = edge + lll(rng() % 4);
            lll c = (rng() % 2 ? 1 : -1) * (wideBox / 2) + lll(rng() % 3);
            if (cls == 0) { p.push_back({T(wide), T(narrow)}); }
            if (cls == 1) { p.push_back({T(narrow), T(wide)}); }
            if (cls == 2) { p.push_back({T(c), T(c - lll(rng() % 3))}); }
            if (cls == 3) { p.push_back(pool[rng() % (1 + t / 14 % 3)]); }
            if (cls == 4) { p.push_back({T(lll(rng() % 5) - 2), T(lll(rng() % 5) - 2)}); }
            if (cls == 5) { lll v = edge / 2 + lll(rng() % 6); p.push_back({T(v), T(v)}); }
            if (cls == 6) {
                auto u = [&] { return T(lll(rng() % ulng(2 * wideBox + 1)) - wideBox); };
                p.push_back({u(), u()});}}
        verify(p); std::shuffle(p.begin(), p.end(), rng); verify(p);}}
// Rounding ties (tiny dy beside unit dx, coordinates below one ulp apart), signed zeros, duplicates, collinear lines, wide exponents.
template<typename T> vector<Point2<T>> floatingCase(std::mt19937_64 &rng, int t) {
    int n = int(rng() % 60), cls = t % 8;
    auto u = [&](int m) { return static_cast<long double>(rng() % ulng(m)); };
    auto f = [](long double v) { return static_cast<T>(v); };
    vector<Point2<T>> p;
    for (int k = 0; k < n; ++k) {
        if (cls == 0) { p.push_back({f(u(7) - 3), f(u(7) - 3)}); }
        if (cls == 1) { p.push_back({f(u(4)), f(u(5) * 1e-11L)}); }
        if (cls == 2) { p.push_back({f(1e16L + u(8)), f(u(3))}); }
        if (cls == 3) { p.push_back({f(u(3)), f(-1e16L - u(8))}); }
        if (cls == 4) {
            auto w = [&] { return (rng() % 2 ? -1.0L : 1.0L) * static_cast<long double>(rng() >> 11) * 0x1p-53L * std::pow(10.0L, static_cast<long double>(int(rng() % 41) - 20)); };
            p.push_back({f(w()), f(w())});}
        if (cls == 5) {
            const long double zs[] = {0.0L, -0.0L, 1.0L, -1.0L};
            p.push_back({f(zs[rng() % 4]), f(zs[rng() % 2])});}
        if (cls == 6) {
            long double v = u(50) * 0.1L;
            int line = t / 8 % 3;
            p.push_back(line == 0 ? Point2<T>{f(0.1L), f(v)} : line == 1 ? Point2<T>{f(v), f(-0.1L)} : Point2<T>{f(v), f(v)});}
        if (cls == 7) {
            long double c = (u(3) - 1) * 1e12L;
            p.push_back({f(c + u(5) * 0.1L), f(c - u(5) * 0.1L)});}}
    return p;}
template<typename T> void approxTyped(std::mt19937_64 &rng, int rounds) {
    for (int t = 0; t < rounds; ++t) {
        auto p = floatingCase<T>(rng, t);
        verifyApprox(p); std::shuffle(p.begin(), p.end(), rng); verifyApprox(p);}}
// Grid fallbacks at any point of a seed's order: a repeat; a pair 1e-12 (or 1e-6, kept) apart in a 1000-wide cloud; a point at 1e20;
// sides at 1e600, 2^980 and 2^975 multiples (both sides of 2^980); 2^-1074, 2^-1030 (subnormal) and 2^-1000 multiples.
void approxFallbacks(std::mt19937_64 &rng, int rounds) {
    for (int t = 0; t < rounds; ++t) {
        int n = 2 + int(rng() % 300), cls = t % 7;
        auto unit = [&] { return double(rng() >> 11) * 0x1p-53 * 1000; };
        // Point k jittered by 0-2 units in its own 4 x 4 block of a 16-column lattice: distinct points 1 to 2 units apart.
        auto lattice = [&](int k, double s) { return Point2<double>{double(k % 16 * 4 + int(rng() % 3)) * s, double(k / 16 * 4 + int(rng() % 3)) * s}; };
        const double tiny[] = {0x1p-1074, 0x1p-1030, 0x1p-1000};
        if (cls == 4) {
            vector<Point2<long double>> p;
            for (int k = 0; k < n; ++k) { p.push_back({static_cast<long double>(rng() % 40) * 1e600L, static_cast<long double>(rng() % 40) * 1e600L}); }
            verifyApprox(p); std::shuffle(p.begin(), p.end(), rng); verifyApprox(p);
            continue;}
        vector<Point2<double>> p;
        for (int k = 0; k < n; ++k) {
            if (cls == 0) { p.push_back({double(rng() % 1000), double(rng() % 1000)}); }
            if (cls >= 1 && cls <= 3) { p.push_back({unit(), unit()}); }
            if (cls == 5) { p.push_back(lattice(k, t / 7 % 2 ? 0x1p980 : 0x1p975)); }
            if (cls == 6) { p.push_back(lattice(k, tiny[t / 7 % 3])); }}
        auto c = p[rng() % p.size()];
        auto at = p.begin() + ptrdiff_t(rng() % (p.size() + 1));
        if (cls == 0) { p.insert(at, c); }
        if (cls == 1 || cls == 2) { p.insert(at, {c.x + (cls == 1 ? 1e-12 : 1e-6), c.y}); }
        if (cls == 3) { p.insert(at, {1e20, unit()}); }
        verifyApprox(p); std::shuffle(p.begin(), p.end(), rng); verifyApprox(p);}}
// Small grids, duplicate pools (groups of 2 and 3+), exact pairs, collinear sets, thin strips, 1e9 and 1e18 clouds, far clusters.
template<typename T> void triangleTyped(std::mt19937_64 &rng, int rounds) {
    using L = std::numeric_limits<T>;
    auto clamp = [](long double v) { return std::is_integral_v<T> ? std::clamp(v, static_cast<long double>(L::min()), static_cast<long double>(L::max())) : v; };
    for (int t = 0; t < rounds; ++t) {
        int n = int(rng() % 30), cls = t % 8;
        auto u = [&](long double m) { return static_cast<long double>(rng() % ulng(m)); };
        auto f = [&](long double v) { return static_cast<T>(clamp(v)); };
        auto cloud = [&](long double r) { r = clamp(r); return f(std::floor(u(2 * r + 1) - r)); };
        vector<Point2<T>> p, pool;
        for (int k = 0; k < 5; ++k) { pool.push_back({f(u(9) - 4), f(u(9) - 4)}); }
        for (int k = 0; k < n; ++k) {
            if (cls == 0) { long double m = t / 8 % 2 ? 3 : 4; p.push_back({f(u(m)), f(u(m))}); }
            if (cls == 1) { p.push_back(pool[rng() % (2 + t / 8 % 4)]); }
            if (cls == 2) { p.push_back(pool[k / 2 % 5]); if (k % 2 == 0 && k + 1 == n) { p.push_back(pool[k / 2 % 5]); } }
            if (cls == 3) {
                long double v = u(20) - 10;
                int line = t / 8 % 3;
                p.push_back(line == 0 ? Point2<T>{f(3), f(v)} : line == 1 ? Point2<T>{f(v), f(-2)} : Point2<T>{f(v), f(-v)});}
            if (cls == 4) { p.push_back({cloud(1e9L), f(u(2))}); }
            if (cls == 5) { p.push_back({cloud(1e9L), cloud(1e9L)}); }
            if (cls == 6) { p.push_back({cloud(1e18L), cloud(1e18L)}); }
            if (cls == 7) { long double c = (u(3) - 1) * clamp(1e15L) / 2; p.push_back({f(c + u(4)), f(c - u(4))}); }}
        if (cls == 2) { std::shuffle(p.begin(), p.end(), rng); }
        verifyTriangle(p); std::shuffle(p.begin(), p.end(), rng); verifyTriangle(p);}}
// A known answer at large n: the jittered lattice keeps distinct cells >= 900 apart while the planted triangle has perimeter 2 + sqrt(2).
template<typename T> void triangleLarge(std::mt19937_64 &rng, int n) {
    for (int shape = 0; shape < 5; ++shape) {
        vector<Point2<T>> p;
        array<int, 3> want = shape == 4 ? array<int, 3>{1, 2, 3} : array<int, 3>{0, 1, 2};
        int side = int(std::sqrt(n)) + 1;
        for (int i = 0; i < n; ++i) {
            if (shape == 0) { p.push_back({T(1000 * (i % side) + lng(rng() % 100)), T(1000 * (i / side) + lng(rng() % 100))}); }
            if (shape == 1) { p.push_back({T(0), T(i)}); }
            if (shape == 2) { p.push_back({T(10 * (i / 2 % side)), T(10 * (i / 2 / side))}); }
            if (shape == 3) { p.push_back({T(-7), T(5)}); }
            if (shape == 4) { p.push_back({T(i ? 0 : 1), T(0)}); }}
        if (shape == 0) {
            std::shuffle(p.begin(), p.end(), rng);
            lng cx = 1000 * lng(rng() % ulng(side)) + 500, cy = 1000 * lng(rng() % ulng(side / 2)) + 500;
            const Point2<T> tri[] = {{T(cx), T(cy)}, {T(cx + 1), T(cy)}, {T(cx), T(cy + 1)}};
            for (int k = 0; k < 3; ++k) { want[k] = int(rng() % ulng(n)); p[size_t(want[k])] = tri[k]; }
            sort(want.begin(), want.end());
            if (want[0] == want[1] || want[1] == want[2]) { continue; }}
        auto before = p;
        auto start = std::chrono::steady_clock::now();
        auto got = minimumPerimeterTriangleApprox(p);
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        ++checks;
        if (got != want) { failure("minimumPerimeterTriangleApprox large known answer shape=" + std::to_string(shape), vector<Point2<T>>{}, want, got); }
        ++checks;
        if (!same(p, before)) { failure("minimumPerimeterTriangleApprox large input preserved", vector<Point2<T>>{}, want, got); }
        // Quadratic-blowup guard for the optimized build only: the measured cost is about 0.1 s at n = 200,000.
        if (OPTIMIZED) {
            ++checks;
            if (seconds > 20) { failure("minimumPerimeterTriangleApprox time " + std::to_string(seconds) + "s > 20s shape=" + std::to_string(shape), vector<Point2<T>>{}, want, got); }}}}
// Every seed returns want, keeps the input and, in the optimized build, finishes within 5 s (measured well under 0.1 s).
template<typename T> void timedApprox(const vector<Point2<T>> &p, pair<int, int> want, const string &what) {
    auto before = p;
    for (ulng s : approxSeeds()) {
        auto start = std::chrono::steady_clock::now();
        auto got = closestPairApprox(p, s);
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        string tag = " " + what + " rngseed=" + std::to_string(s);
        if (OPTIMIZED) {
            ++checks;
            if (seconds > 5) { failure("closestPairApprox time " + std::to_string(seconds) + "s > 5s" + tag, vector<Point2<T>>{}, want, got); }}
        ++checks;
        if (got != want) { failure("closestPairApprox large known answer" + tag, vector<Point2<T>>{}, want, got); }
        ++checks;
        if (!same(p, before)) { failure("closestPairApprox large input preserved" + tag, vector<Point2<T>>{}, want, got); }}}
// White-box: on distinct points within the grid's scale the incremental path itself must answer, for every seed, without the fallback.
template<typename T> void requireGridPath(const vector<Point2<T>> &p, pair<int, int> want, const string &what) {
    for (ulng s : approxSeeds()) {
        closest_detail::Best<long double> b;
        bool grid = closest_detail::incremental(p, b, s);
        ++checks;
        if (!grid || b.ids != want) { failure("closestPairApprox grid path (incremental returned " + std::to_string(grid) + ") " + what + " rngseed=" + std::to_string(s), vector<Point2<T>>{}, want, b.ids); }}}
template<typename T> void approxLarge(int n) {
    for (int shape = 0; shape < 4; ++shape) {
        vector<Point2<T>> p;
        for (int i = 0; i < n; ++i) {
            if (shape == 0) { p.push_back({T(i % 200), T(i / 200)}); }
            if (shape == 1) { p.push_back({T(0), T(-i)}); }
            if (shape == 2) { p.push_back({T(i), T(i)}); }
            if (shape == 3) { p.push_back({T(0), T(0)}); }}
        // Planted quarter-unit neighbour of point 0: the unique minimum, below every unit-spaced pair; shape 3 is all duplicates.
        p.push_back({T(0.25L), T(shape == 2 ? 0.25L : 0.0L)});
        if (shape == 1) { p.back() = {T(0), T(-0.25L)}; }
        timedApprox(p, {0, shape == 3 ? 1 : n}, "planted shape=" + std::to_string(shape));}}
// Multiples of 2^-6 below 2^25: 31-bit numerators square exactly in long double, so closestPair on them is an exact oracle;
// random, jittered lattice with a planted pair, shuffled line with tied gaps, 200-point clusters (repeats take the fallback).
void approxLargeExact(std::mt19937_64 &rng, int n) {
    int side = int(std::sqrt(n)) + 1;
    for (int shape = 0; shape < 4; ++shape) {
        vector<point> q;
        lng x = 0, cx = 0, cy = 0;
        pair<int, int> planted{-1, -1};
        for (int i = 0; i < n; ++i) {
            if (shape == 0) { q.push_back({lng(rng() % (1ULL << 31)), lng(rng() % (1ULL << 31))}); }
            if (shape == 1) { q.push_back({64000 * (i % side) + lng(rng() % 6400), 64000 * (i / side) + lng(rng() % 6400)}); }
            if (shape == 2) { q.push_back({x += lng(rng() % 1000) + 1, 0}); }
            if (shape == 3) {
                if (i % 200 == 0) { cx = lng(rng() % ((1ULL << 31) - 4096)); cy = lng(rng() % ((1ULL << 31) - 4096)); }
                q.push_back({cx + lng(rng() % 4096), cy + lng(rng() % 4096)});}}
        if (shape == 1) {
            // A pair 1/64 apart in the middle of a lattice square, far from every jittered lattice point.
            int a = int(rng() % ulng(n)), b = int(rng() % ulng(n - 1)); b += b >= a;
            lng c = 64000 * lng(rng() % ulng(side - 1)) + 32000, d = 64000 * lng(rng() % ulng(n / side - 1)) + 32000;
            q[size_t(a)] = {c, d}; q[size_t(b)] = {c + 1, d};
            planted = std::minmax(a, b);}
        if (shape == 2) { std::shuffle(q.begin(), q.end(), rng); }
        auto want = closestPair(q);
        ++checks;
        if (shape == 1 && want.ids != planted) { failure("closestPair planted lattice pair", vector<point>{}, planted, want.ids); }
        vector<Point2<double>> p;
        for (auto r : q) { p.push_back({double(r.x) / 64, double(r.y) / 64}); }
        timedApprox(p, want.ids, "exact double shape=" + std::to_string(shape));
        if (shape < 3) { requireGridPath(p, want.ids, "exact double shape=" + std::to_string(shape)); }}}
// A 40-wide lattice, 4 units apart and jittered by 0-2, times a power of two: the computed distances are exact, so closestPair on
// the numerators is the oracle; at 2^-1074 (double) and 2^-1063 (long double, about 1e-320) 1 / side overflows a double.
template<typename T> void approxTinyLattice(std::mt19937_64 &rng, int n, T unit) {
    vector<point> q;
    for (int k = 0; k < n; ++k) { q.push_back({k % 40 * 4 + lng(rng() % 3), k / 40 * 4 + lng(rng() % 3)}); }
    auto want = closestPair(q);
    vector<Point2<T>> p;
    for (auto r : q) { p.push_back({T(r.x) * unit, T(r.y) * unit}); }
    timedApprox(p, want.ids, "tiny lattice type=" + typeName<T>());}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    bool protocol = false, three = false, randomized = false, naive = false, simple = false;
    for (int i = 1; i < argc; ++i) {
        string a = argv[i]; if (a == "--mode") { mode = argv[++i]; }
        else if (a == "--seed") { seed = std::stoull(argv[++i]); }
        else if (a == "--invalid") { invalid = argv[++i]; }
        else if (a == "--oracle") { protocol = true; }
        else if (a == "--oracle3") { protocol = three = true; }
        else if (a == "--oracle-randomized") { protocol = randomized = true; }
        else if (a == "--oracle-naive") { protocol = naive = true; }
        else if (a == "--oracle-simple") { protocol = simple = true; }
        else if (a == "--oracle3-naive") { protocol = three = naive = true; }}
    const lng lo = std::numeric_limits<lng>::min(), hi = std::numeric_limits<lng>::max();
    if (!invalid.empty()) {
        if (invalid == "span-x") { closestPair(vector<point>{{lo, 0}, {hi, 0}}); }
        else if (invalid == "span-y") { closestPair(vector<point>{{0, lo}, {0, hi}}); }
        else if (invalid == "diagonal") { closestPair(vector<point>{{lo, lo}, {0, 0}}); }
        else if (invalid == "randomized-span-x") { closestPairRandomized(vector<point>{{lo, 0}, {hi, 0}}, 1); }
        else if (invalid == "randomized-span-y") { closestPairRandomized(vector<point>{{0, lo}, {0, hi}}, 1); }
        else if (invalid == "randomized-diagonal") { closestPairRandomized(vector<point>{{lo, lo}, {0, 0}}, 1); }
        else if (invalid == "span-z3") { closestPair3(vector<point3>{{0, 0, lo}, {0, 0, hi}}); }
        else if (invalid == "diagonal3") { closestPair3(vector<point3>{{lo / 2, lo / 2, lo / 2}, {hi / 2, hi / 2, hi / 2}}); }
        else if (invalid == "naive-span-x") { closestPairNaive(vector<point>{{lo, 0}, {hi, 0}}); }
        else if (invalid == "simple-span-x") { closestPairSimple(vector<point>{{lo, 0}, {hi, 0}}); }
        else if (invalid == "approx-nan") { closestPairApprox(vector<Point2<double>>{{0, 0}, {std::numeric_limits<double>::quiet_NaN(), 0}}, 1); }
        else if (invalid == "triangle-nan") { minimumPerimeterTriangleApprox(vector<Point2<double>>{{0, 0}, {1, 0}, {0, std::numeric_limits<double>::quiet_NaN()}}); }
        else if (invalid == "naive-diagonal3") { closestPair3Naive(vector<point3>{{lo / 2, lo / 2, lo / 2}, {hi / 2, hi / 2, hi / 2}}); }
        else { return 2; }
        return 0;}
    if (protocol) {
        int cases; cin >> cases;
        while (cases--) {
            int n; cin >> n; vector<point3> p(n);
            for (auto &q : p) { cin >> q.x >> q.y; if (three) { cin >> q.z; } }
            vector<point> flat; for (auto q : p) { flat.push_back({q.x, q.y}); }
            if (randomized) {
                // Three lines per case: seeds 0, ~0 and one derived from the case count.
                for (ulng s : std::initializer_list<ulng>{0, ~0ULL, CustomHash::splitMix64(seed + ulng(cases))}) {
                    auto ans = closestPairRandomized(flat, s); cout << ans.ids.first << ' ' << ans.ids.second << ' ' << decimal(ans.distance2) << '\n';}
                continue;}
            auto ans = three ? (naive ? closestPair3Naive(p) : closestPair3(p)) : naive ? closestPairNaive(flat) : simple ? closestPairSimple(flat) : closestPair(flat); cout << ans.ids.first << ' ' << ans.ids.second << ' ' << decimal(ans.distance2) << '\n';}
        return 0;}
    std::mt19937_64 rng(seed);
    std::mt19937_64 seedRng(seed ^ 0x5eed5eed5eedULL);
    for (int k = mode == "quick" ? 1 : mode == "full" ? 4 : 8; k--; ) { seeds.push_back(seedRng()); }
    caseSeeds.seed(seed ^ 0xca5e5eedULL);
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
    auto as = closestPairSimple(vector<Point2<int>>{{std::numeric_limits<int>::min(), 0}, {std::numeric_limits<int>::max(), 0}});
    require(equal(as, a), vector<point>{}, a, as, "closestPairSimple int");
    auto bs = closestPairSimple(vector<Point2<int16_t>>{{-32768, 0}, {32767, 0}});
    require(equal(bs, b), vector<point>{}, b, bs, "closestPairSimple int16_t");
    auto cs = closestPairSimple(vector<Point2<int8_t>>{{-128, 0}, {127, 0}});
    require(equal(cs, c), vector<point>{}, c, cs, "closestPairSimple int8_t");
    phase = "large no-duplicate equal-support grid and line";
    int n = mode == "quick" ? 500 : mode == "full" ? 50000 : 250000;
    for (int shape = 0; shape < 5; ++shape) {
        vector<point> p;
        for (int i = 0; i < n; ++i) {
            if (shape == 0) { p.push_back({i % 200, i / 200}); }
            if (shape == 1) { p.push_back({0, -i}); }
            if (shape == 2) { p.push_back({i, 0}); }
            if (shape == 3) { p.push_back({7, 11}); }
            if (shape == 4) { p.push_back(i + 1 < n ? point{0, 0} : point{1, 2}); }}
        auto before = p;
        auto start = std::chrono::steady_clock::now();
        auto got = closestPair(p);
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        ClosestPair want{{0, 1}, shape >= 3 ? 0 : 1};
        // Duplicates off a line: the strips must not run at distance 0 (measured 0.01 s at n = 50,000).
        require(!OPTIMIZED || seconds <= 5, vector<point>{}, want, got, "large known minimum time > 5s");
        require(equal(want, got), p, want, got, "large known minimum");
        require(before == p, p, want, got, "large input preserved");
        start = std::chrono::steady_clock::now();
        auto sim = closestPairSimple(p);
        seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        require(!OPTIMIZED || seconds <= 5, vector<point>{}, want, sim, "closestPairSimple large known minimum time > 5s");
        require(equal(want, sim), vector<point>{}, want, sim, "closestPairSimple large known minimum");
        require(before == p, vector<point>{}, want, sim, "closestPairSimple large input preserved");
        for (ulng s : seeds) {
            auto r = closestPairRandomized(p, s);
            require(equal(want, r), vector<point>{}, want, r, ("closestPairRandomized large known minimum rngseed=" + std::to_string(s)).c_str());}}
    phase = "closestPairRandomized and closestPairSimple large random, clustered and extreme-span clouds against closestPair";
    for (int shape = 0; shape < 4; ++shape) {
        vector<point> p;
        for (int i = 0; i < n; ++i) {
            lng x = lng(rng() % 2000001) - 1000000, y = lng(rng() % 2000001) - 1000000;
            if (shape == 0) { p.push_back({x, y}); }
            if (shape == 1) { p.push_back({x / 1000 * 1000 + lng(rng() % 3), y / 1000 * 1000}); }
            if (shape == 2) { p.push_back({lo + lng(rng() % 4), lo + lng(rng() % ulng(n))}); }
            if (shape == 3) { p.push_back({i % 2 ? lo + lng(rng() % ulng(n)) : lng(ulng(lo) + (ROOT - 16)) - lng(rng() % ulng(n)), hi - lng(rng() % 4)}); }}
        auto want = closestPair(p);
        auto start = std::chrono::steady_clock::now();
        auto sim = closestPairSimple(p);
        double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        require(!OPTIMIZED || seconds <= 5, vector<point>{}, want, sim, ("closestPairSimple large agreement time > 5s shape=" + std::to_string(shape)).c_str());
        require(equal(want, sim), vector<point>{}, want, sim, ("closestPairSimple large agreement shape=" + std::to_string(shape)).c_str());
        for (ulng s : seeds) {
            auto r = closestPairRandomized(p, s);
            require(equal(want, r), vector<point>{}, want, r, ("closestPairRandomized large agreement shape=" + std::to_string(shape) + " rngseed=" + std::to_string(s)).c_str());}}
    phase = "closestPairRandomized typed extremes, clusters, duplicates, lattices, diagonals";
    int typedRounds = mode == "quick" ? 28 : mode == "full" ? 700 : 5600;
    randomizedTyped<int8_t>(rng, typedRounds); randomizedTyped<int16_t>(rng, typedRounds);
    randomizedTyped<int>(rng, typedRounds); randomizedTyped<lng>(rng, typedRounds);
    phase = "closestPairApprox regressions";
    {
        using D = Point2<double>;
        const double z = -0.0;
        for (ulng s : approxSeeds()) {
            string tag = " rngseed=" + std::to_string(s);
            auto r1 = closestPairApprox(vector<D>{{0, 0}, {1, 1}, {z, 0}}, s);
            if (r1 != pair{0, 2}) { failure("closestPairApprox signed zero equals zero" + tag, vector<D>{{0, 0}, {1, 1}, {z, 0}}, pair{0, 2}, r1); }
            // True minimum is (2, 3); 1e-22 is below half an ulp of 1 in long double, so (0, 1) ties it and wins on ids.
            auto r2 = closestPairApprox(vector<D>{{0, 0}, {1, 1e-11}, {5, 0}, {6, 0}}, s);
            if (r2 != pair{0, 1}) { failure("closestPairApprox rounding tie" + tag, vector<D>{{0, 0}, {1, 1e-11}, {5, 0}, {6, 0}}, pair{0, 1}, r2); }
            checks += 2;}
        // E = 2^40 side for a unit closest distance: spans E - 1 keep the grid, E and above fall back; far unit pairs tie (0, 1) or lose it.
        const double e = 0x1p40 + 0x1p32, t = 0x1p-1074;
        for (const vector<D> &p : vector<vector<D>>{{}, {{z, z}}, {{1e300, -1e300}, {1e300, -1e300}}, {{1e16, 0}, {1e16 + 1, 0}, {0, 0}},
                                                    {{3, 0}, {2, 0}, {1, 0}, {0, 0}}, {{-1e-300, 0}, {1e-300, 0}, {0, 1e-300}},
                                                    {{0, 0}, {1, 1}, {0, 0}}, {{5, 5}, {0, 0}, {1, 0}, {5, 5}, {0, 0}, {9, 9}},
                                                    {{0, 0}, {1e-12, 0}, {1e20, 0}}, {{1e20, 0}, {3, 4}, {0, 0}, {0, 1e-12}},
                                                    {{e - 2, 0}, {e - 1, 0}, {0, 0}, {1, 0}}, {{e - 1, 0}, {e, 0}, {0, 0}, {1, 0}},
                                                    {{0, 0}, {1, 0}, {e - 2, 1}, {e - 1, 1}}, {{0, 0}, {1, 0}, {e, 1}, {e + 1, 1}},
                                                    {{0, 0}, {1.5, 0}, {e - 2, 0}, {e - 1, 0}, {e / 2, 3}}, {{0, 0}, {1.5, 0}, {e - 1, 0}, {e, 0}, {e / 2, 3}},
                                                    {{-1e308, 0}, {1e308, 0}, {1e308, 1}}, {{0, 0}, {0x1p978, 0}, {0x1p979, 0}},
                                                    {{0, 0}, {0x1p980, 0}, {0x1p981, 0x1p980}}, {{0, 0}, {0x1p980 * (1 - 0x1p-9), 0}, {0x1p982, 0}},
                                                    {{0, 0}, {300 * t, 0}, {301 * t, 0}, {511 * t, 0}, {0, 200 * t}}}) { verifyApprox(p); }
        verifyApprox(vector<Point2<float>>{{0.1f, 0}, {0.2f, 0}, {0.3f, 0}, {0.4f, 0}});
        verifyApprox(vector<Point2<float>>{{0, 0}, {0.5f, 0.5f}, {0, 0}, {1e30f, 0}});
        verifyApprox(vector<Point2<long double>>{{1e2000L, 0}, {1e2000L, 1}, {-1e2000L, 0}});
        verifyApprox(vector<Point2<long double>>{{0, 0}, {1e600L, 0}, {3e600L, 1e600L}, {-2e600L, 5e599L}, {1e600L, 1e600L}});
        verifyApprox(vector<Point2<long double>>{{0, 0}, {1e300L, 0}, {3e300L, 1e300L}, {-2e300L, 5e299L}});
        verifyApprox(vector<Point2<long double>>{{0, 0}, {1e-320L, 0}, {3e-320L, 0}, {2e-320L, 5e-321L}});
        // Spans of 2^60 from a zero minimum: past the span guard, cells from double differences split the tied pair (0, 1) whenever two
        // of the six unit-spaced points precede its second point, which most seeds give.
        verifyApprox(vector<Point2<long double>>{{0x1p60L + 383, 0}, {0x1p60L + 384, 0}, {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0}});
        verifyApprox(vector<Point2<long double>>{{0, 0x1p60L + 383}, {0, 0x1p60L + 384}, {0, 0}, {0, 1}, {0, 2}, {0, 3}, {0, 4}, {0, 5}});
    }
    phase = "closestPairApprox random rounding ties, signed zeros, duplicates, lines and wide exponents";
    int approxRounds = mode == "quick" ? 24 : mode == "full" ? 1200 : 8000;
    approxTyped<float>(rng, approxRounds); approxTyped<double>(rng, approxRounds); approxTyped<long double>(rng, approxRounds);
    phase = "closestPairApprox fallbacks: repeated points, span >= 2^40 side, side >= 2^980, subnormal distances";
    approxFallbacks(rng, mode == "quick" ? 14 : mode == "full" ? 700 : 5600);
    phase = "closestPairApprox large planted minimum";
    approxLarge<float>(n); approxLarge<double>(n); approxLarge<long double>(n);
    phase = "closestPairApprox large exact doubles: random, jittered lattice, line, clusters";
    approxLargeExact(rng, mode == "quick" ? 2000 : 200000);
    phase = "closestPairApprox subnormal and 1e-320 lattices (1 / side beyond double)";
    approxTinyLattice<double>(rng, mode == "quick" ? 2000 : 50000, 0x1p-1074);
    approxTinyLattice<long double>(rng, mode == "quick" ? 2000 : 50000, 0x1p-1063L);
    phase = "closestPairApprox __float128 coordinates and the non-finite entry check";
    {
        // Gaps 4 * 2^-60 at 1 and a point 2^-60 from the 33rd: cells computed in double merge them, long double ones (as dist2) do not.
        vector<Point2<__float128>> p;
        for (int k = 0; k < 1200; ++k) { p.push_back({1 + __float128(4 * k) * __float128(0x1p-60L), 0}); }
        p.push_back({1 + __float128(129) * __float128(0x1p-60L), 0});
        verifyApprox(p);
        requireGridPath(p, {32, 1200}, "__float128 gaps below the double ulp");
        // Distinct __float128 points that round to one long double point are at computed distance 0, so (1, 2) beats the repeat (3, 4).
        __float128 e = __float128(0x1p-100L);
        verifyApprox(vector<Point2<__float128>>{{100, 0}, {1 + e, 0}, {1, 0}, {5, 0}, {5, 0}});
        vector<Point2<__float128>> q;
        for (int k = 0; k < 2000; ++k) { q.push_back({__float128(3 * k + 10), __float128(7 * (k % 13))}); }
        q[5] = {1 + e, 0}; q[6] = {1, 0}; q[1998] = q[1999];
        verifyApprox(q);
        // Long double cases with points moved below the long double ulp, which round back onto them, and repeats.
        for (int t = 0; t < approxRounds; ++t) {
            auto r = floatingCase<__float128>(rng, t);
            for (auto &c : r) {
                if (rng() % 3 == 0) { c.x += (c.x != 0 ? c.x : 1) * e; }}
            if (!r.empty() && rng() % 2) { r.push_back(r[rng() % r.size()]); }
            verifyApprox(r); std::shuffle(r.begin(), r.end(), rng); verifyApprox(r);}}
#ifdef NDEBUG
    {
        // Past the violated finite precondition the entry check returns before any allocation or sort (Contracts, a memory-safe
        // boundedness check): NaN or infinite x or y.
        const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
        for (Point2<double> bad : {Point2<double>{nan, 0}, {0, nan}, {inf, 0}, {0, -inf}}) {
            vector<Point2<double>> p{{0, 0}, bad, {1, 0}};
            ++checks;
            if (closestPairApprox(p, 1) != pair{-1, -1}) { failure("closestPairApprox non-finite under NDEBUG", p, pair{-1, -1}, closestPairApprox(p, 1)); }}}
#endif
    phase = "minimumPerimeterTriangleApprox regressions";
    {
        auto r1 = minimumPerimeterTriangleApprox(vector<point>{{0, 0}, {5, 5}});
        if (r1 != array{-1, -1, -1}) { failure("minimumPerimeterTriangleApprox n < 3", vector<point>{{0, 0}, {5, 5}}, array{-1, -1, -1}, r1); }
        auto r2 = minimumPerimeterTriangleApprox(vector<point>{{9, 9}, {1, 1}, {0, 0}, {1, 1}, {1, 1}});
        if (r2 != array{1, 3, 4}) { failure("minimumPerimeterTriangleApprox three equal points", vector<point>{{9, 9}, {1, 1}, {0, 0}, {1, 1}, {1, 1}}, array{1, 3, 4}, r2); }
        checks += 2;
        for (const vector<point> &p : vector<vector<point>>{{}, {{lo, hi}}, {{0, 0}, {0, 0}}, {{0, 0}, {0, 0}, {0, 0}}, {{0, 0}, {1, 0}, {2, 0}},
                                                            {{lo, lo}, {hi, hi}, {lo, hi}, {hi, lo}}, {{lo, lo}, {lo + 1, lo}, {hi, hi}, {hi, hi - 1}, {hi - 1, hi}},
                                                            {{0, 0}, {0, 0}, {5, 0}, {7, 7}, {7, 7}, {7, 8}}}) { verifyTriangle(p); }
    }
    phase = "minimumPerimeterTriangleApprox ordered lists over 2x2 and collinear 1x3";
    for (int line = 0; line < 2; ++line) {
        int base = line ? 3 : 4, top2 = line ? 7 : mode == "quick" ? 5 : mode == "full" ? 7 : 8;
        total = 1;
        for (int len = 0; len <= top2; ++len) {
            for (int mask = 0; mask < total; ++mask) {
                vector<point> p; int x = mask;
                for (int i = 0; i < len; ++i) { p.push_back(line ? point{x % 3, 0} : point{x % 2, x / 2 % 2}); x /= base; }
                verifyTriangle(p);}
            total *= base;}}
    phase = "minimumPerimeterTriangleApprox all 3x3 and 4x4 grid subsets";
    for (int side = 3; side <= (mode == "quick" ? 3 : 4); ++side) {
        for (int mask = 0; mask < 1 << (side * side); ++mask) {
            vector<point> p; for (int k = 0; k < side * side; ++k) { if (mask >> k & 1) { p.push_back({k % side, k / side}); } }
            verifyTriangle(p);
            if (side == 3 || mask % 16 == 0) { std::shuffle(p.begin(), p.end(), rng); verifyTriangle(p); }}}
    phase = "minimumPerimeterTriangleApprox random grids, duplicates, lines, strips, 1e9/1e18 clouds, clusters";
    int triangleRounds = mode == "quick" ? 16 : mode == "full" ? 800 : 6000;
    triangleTyped<int8_t>(rng, triangleRounds); triangleTyped<int16_t>(rng, triangleRounds); triangleTyped<int>(rng, triangleRounds);
    triangleTyped<lng>(rng, triangleRounds); triangleTyped<float>(rng, triangleRounds); triangleTyped<double>(rng, triangleRounds);
    triangleTyped<long double>(rng, triangleRounds);
    phase = "minimumPerimeterTriangleApprox large planted triangle, line, duplicate pairs, all equal";
    int big = mode == "quick" ? 2000 : 200000;
    triangleLarge<lng>(rng, big); triangleLarge<double>(rng, big);
    if (mode != "quick") { triangleLarge<int>(rng, big); triangleLarge<float>(rng, big); }
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
    auto d3n = closestPair3Naive(vector<Point3<int>>{{std::numeric_limits<int>::min(), 0, 0}, {std::numeric_limits<int>::max(), 0, 0}});
    auto e3n = closestPair3Naive(vector<Point3<int8_t>>{{-128, -128, -128}, {127, 127, 127}, {0, 0, 1}});
    require(equal(d3n, d3), vector<point3>{}, d3, d3n, "3D int all-pairs scan");
    require(equal(e3n, e3), vector<point3>{}, e3, e3n, "3D int8_t all-pairs scan");
    phase = "3D large lattice cube, plane, line, y-line and two-row plane";
    int m3 = mode == "quick" ? 10 : mode == "full" ? 40 : 70, n3 = mode == "quick" ? 1000 : 1000000;
    // Shapes 3 and 4 put the whole input in one slab; a row grouping or z window that degrades is quadratic there.
    for (int shape = 0; shape < 5; ++shape) {
        vector<point3> p;
        for (int i = 0; i < (shape < 3 ? m3 * m3 * m3 : n3); ++i) {
            if (shape == 0) { p.push_back({2 * (i % m3), 2 * (i / m3 % m3), 2 * (i / m3 / m3)}); }
            if (shape == 1) { p.push_back({0, 3 * (i % (m3 * m3)), 3 * (i / (m3 * m3))}); }
            if (shape == 2) { p.push_back({lng(i) * 5, lng(i) * 5, lng(i) * 5}); }
            if (shape == 3) { p.push_back({0, 3 * lng(i), 0}); }
            if (shape == 4) { p.push_back({0, 3 * (i % 2), 3 * lng(i / 2)}); }}
        p.push_back({1, 1, 1});
        int last = int(p.size()) - 1;
        ClosestPair want{{0, last}, 3};
        auto before = p; auto got = closestPair3(p);
        require(equal(want, got), vector<point3>{}, want, got, "3D large known minimum");
        require(before == p, vector<point3>{}, want, got, "3D large input preserved");}
    cout << "PASS closestpair seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
