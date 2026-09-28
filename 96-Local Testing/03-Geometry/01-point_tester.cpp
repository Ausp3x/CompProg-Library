#include "../../03-Geometry/01-point.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0;

    string show(lll x) {
        if (!x) { return "0"; }
        bool negative = x < 0; ulll value = negative ? ulll(0) - ulll(x) : ulll(x); string out;
        while (value) { out += char('0' + value % 10); value /= 10; }
        if (negative) { out += '-'; }
        std::reverse(out.begin(), out.end()); return out; }
    string show(ulll x) {
        if (!x) { return "0"; }
        string out;
        while (x) { out += char('0' + x % 10); x /= 10; }
        std::reverse(out.begin(), out.end()); return out; }
    template<typename T> string show(const T &x) {
        std::ostringstream out; out << std::setprecision(24) << x; return out.str(); }
    template<typename T> string show(Point2<T> p) { return "(" + show(p.x) + "," + show(p.y) + ")"; }
    template<typename T> string show(Point3<T> p) { return "(" + show(p.x) + "," + show(p.y) + "," + show(p.z) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &operation) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)); }}
    void close(long double got, long double want, const string &operation) {
        ++checks;
        long double tolerance = 128 * std::numeric_limits<long double>::epsilon() * std::max(1.L, std::abs(want));
        if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)
                                     + " absolute-tolerance=" + show(tolerance)); }}
    void closeRelative(long double got, long double want, const string &operation) {
        ++checks;
        long double tolerance = 128 * std::numeric_limits<long double>::epsilon() * std::abs(want);
        if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)
                                     + " relative-tolerance=" + show(128 * std::numeric_limits<long double>::epsilon())); }}

    // Leibniz determinant of homogeneous coordinates: independent of vector subtraction/cross.
    template<int N> lll determinant(const array<array<lll, N>, N> &a) {
        array<int, N> p; std::iota(p.begin(), p.end(), 0); lll answer = 0;
        do {
            lll term = 1; int inversions = 0;
            for (int i = 0; i < N; ++i) {
                term *= a[i][p[i]];
                for (int j = 0; j < i; ++j) { inversions += p[j] > p[i]; }}
            if (inversions & 1) { answer -= term; } else { answer += term; }
        } while (std::next_permutation(p.begin(), p.end()));
        return answer; }
    template<typename T> lll area(Point2<T> a, Point2<T> b, Point2<T> c) {
        return determinant<3>({{{1, lll(a.x), lll(a.y)}, {1, lll(b.x), lll(b.y)}, {1, lll(c.x), lll(c.y)}}}); }
    template<typename T> lll volume(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
        return determinant<4>({{{1, lll(a.x), lll(a.y), lll(a.z)}, {1, lll(b.x), lll(b.y), lll(b.z)},
                               {1, lll(c.x), lll(c.y), lll(c.z)}, {1, lll(d.x), lll(d.y), lll(d.z)}}}); }
    template<typename T> lll referenceNorm(Point2<T> p) { return lll(p.x) * p.x + lll(p.y) * p.y; }
    template<typename T> lll referenceNorm(Point3<T> p) { return lll(p.x) * p.x + lll(p.y) * p.y + lll(p.z) * p.z; }
    template<typename T> void pair2(Point2<T> a, Point2<T> b) {
        context = "a=" + show(a) + " b=" + show(b);
        lll x = lll(a.x) - b.x, y = lll(a.y) - b.y;
        checkEqual(lll(dot(a, b)), lll(lll(a.x) * b.x + lll(a.y) * b.y), "2D dot");
        checkEqual(lll(cross(a, b)), lll(lll(a.x) * b.y - lll(a.y) * b.x), "2D cross");
        checkEqual(lll(norm2(a)), referenceNorm(a), "2D norm2");
        checkEqual(lll(dist2(a, b)), lll(x * x + y * y), "2D dist2");
        checkEqual(a < b, std::tie(a.x, a.y) < std::tie(b.x, b.y), "2D lexicographic ordering");
        checkEqual(a == b, a.x == b.x && a.y == b.y, "2D equality");
        checkEqual(ulll(dot(a, b)) * ulll(dot(a, b)) + ulll(cross(a, b)) * ulll(cross(a, b)),
              ulll(referenceNorm(a)) * ulll(referenceNorm(b)), "2D Lagrange identity modulo 2^128");
        close(normApprox(a), std::hypot((long double)a.x, (long double)a.y), "2D normApprox");
        close(distanceApprox(a, b), std::hypot((long double)x, (long double)y), "2D distanceApprox"); }
    template<typename T> void orient2(Point2<T> a, Point2<T> b, Point2<T> c) {
        context = "a=" + show(a) + " b=" + show(b) + " c=" + show(c);
        lll answer = area(a, b, c); int sign = (answer > 0) - (answer < 0);
        checkEqual(orient(a, b, c), sign, "homogeneous determinant orientation");
        checkEqual(orient(b, a, c), -sign, "orientation antisymmetry");
        checkEqual(orient(b, c, a), sign, "orientation cyclic permutation"); }
    template<typename T> void pair3(Point3<T> a, Point3<T> b) {
        context = "a=" + show(a) + " b=" + show(b);
        lll x = lll(a.x) - b.x, y = lll(a.y) - b.y, z = lll(a.z) - b.z;
        auto c = cross(a, b);
        checkEqual(lll(c.x), lll(lll(a.y) * b.z - lll(a.z) * b.y), "3D cross x");
        checkEqual(lll(c.y), lll(lll(a.z) * b.x - lll(a.x) * b.z), "3D cross y");
        checkEqual(lll(c.z), lll(lll(a.x) * b.y - lll(a.y) * b.x), "3D cross z");
        checkEqual(lll(dot(a, b)), lll(lll(a.x) * b.x + lll(a.y) * b.y + lll(a.z) * b.z), "3D dot");
        checkEqual(lll(norm2(a)), referenceNorm(a), "3D norm2");
        checkEqual(lll(dist2(a, b)), lll(x * x + y * y + z * z), "3D dist2");
        checkEqual(a < b, std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z), "3D lexicographic ordering");
        checkEqual(a == b, a.x == b.x && a.y == b.y && a.z == b.z, "3D equality");
        checkEqual(lll(c.x) * a.x + lll(c.y) * a.y + lll(c.z) * a.z, lll(0), "3D cross orthogonal to first");
        checkEqual(lll(c.x) * b.x + lll(c.y) * b.y + lll(c.z) * b.z, lll(0), "3D cross orthogonal to second");
        checkEqual(ulll(dot(a, b)) * ulll(dot(a, b)) + ulll(referenceNorm(c)), ulll(referenceNorm(a)) * ulll(referenceNorm(b)), "3D Lagrange identity modulo 2^128");
        close(normApprox(a), std::hypot((long double)a.x, (long double)a.y, (long double)a.z), "3D normApprox");
        close(distanceApprox(a, b), std::hypot((long double)x, (long double)y,
                                              (long double)z), "3D distanceApprox"); }
    template<typename T> void triple3(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
        context = "a=" + show(a) + " b=" + show(b) + " c=" + show(c) + " d=" + show(d);
        lll expected = volume(Point3<T>{}, a, b, c), tetra = volume(a, b, c, d);
        checkEqual(lll(triple(a, b, c)), expected, "scalar triple homogeneous determinant");
        checkEqual(lll(triple(b, a, c)), lll(-expected), "scalar triple antisymmetry");
        checkEqual(lll(triple(b, c, a)), expected, "scalar triple cyclic permutation");
        checkEqual(lll(tetraVolume6(a, b, c, d)), tetra, "oriented tetrahedron homogeneous determinant");
        checkEqual(lll(tetraVolume6(b, a, c, d)), lll(-tetra), "tetrahedron vertex transposition"); }

    template<typename T> void operators() {
        context = "coordinate type bytes=" + show(sizeof(T));
        Point2<T> a{T(6), T(-9)}, b{T(2), T(3)};
        Point3<T> c{T(6), T(-9), T(12)}, d{T(2), T(3), T(-4)};
        checkEqual(Point2<T>{}, Point2<T>{T(0), T(0)}, "default 2D constructor");
        checkEqual(Point3<T>{}, Point3<T>{T(0), T(0), T(0)}, "default 3D constructor");
        checkEqual(a + b, Point2<T>{T(8), T(-6)}, "2D addition");
        checkEqual(a - b, Point2<T>{T(4), T(-12)}, "2D subtraction");
        checkEqual(-a, Point2<T>{T(-6), T(9)}, "2D unary negation");
        checkEqual(a * T(-2), Point2<T>{T(-12), T(18)}, "2D right scaling");
        checkEqual(T(-2) * a, a * T(-2), "2D left scaling");
        checkEqual(a / T(3), Point2<T>{T(2), T(-3)}, "2D division");
        checkEqual(perp(a), Point2<T>{T(9), T(6)}, "perpendicular vector");
        checkEqual(perp(perp(a)), -a, "two quarter rotations");
        auto p = a; checkEqual(&(p += b), &p, "2D += reference"); checkEqual(p, a + b, "2D +=");
        checkEqual(&(p -= b), &p, "2D -= reference"); checkEqual(p, a, "2D -=");
        checkEqual(&(p *= T(3)), &p, "2D *= reference"); checkEqual(p, a * T(3), "2D *=");
        checkEqual(&(p /= T(3)), &p, "2D /= reference"); checkEqual(p, a, "2D /=");
        p += p; checkEqual(p, T(2) * a, "2D += self"); p -= p; checkEqual(p, Point2<T>{}, "2D -= self");
        checkEqual(a.template cast<long double>(), Point2<long double>{6, -9}, "2D cast");
        checkEqual(c + d, Point3<T>{T(8), T(-6), T(8)}, "3D addition");
        checkEqual(c - d, Point3<T>{T(4), T(-12), T(16)}, "3D subtraction");
        checkEqual(-c, Point3<T>{T(-6), T(9), T(-12)}, "3D unary negation");
        checkEqual(c * T(-2), Point3<T>{T(-12), T(18), T(-24)}, "3D right scaling");
        checkEqual(T(-2) * c, c * T(-2), "3D left scaling");
        checkEqual(c / T(3), Point3<T>{T(2), T(-3), T(4)}, "3D division");
        auto q = c; checkEqual(&(q += d), &q, "3D += reference"); checkEqual(q, c + d, "3D +=");
        checkEqual(&(q -= d), &q, "3D -= reference"); checkEqual(q, c, "3D -=");
        checkEqual(&(q *= T(3)), &q, "3D *= reference"); checkEqual(q, c * T(3), "3D *=");
        checkEqual(&(q /= T(3)), &q, "3D /= reference"); checkEqual(q, c, "3D /=");
        q += q; checkEqual(q, T(2) * c, "3D += self"); q -= q; checkEqual(q, Point3<T>{}, "3D -= self");
        checkEqual(c.template cast<long double>(), Point3<long double>{6, -9, 12}, "3D cast");
        auto copied = a; auto moved = std::move(copied); checkEqual(moved, a, "2D copy and move");
        auto copied3 = c; auto moved3 = std::move(copied3); checkEqual(moved3, c, "3D copy and move"); }

    void polar() {
        int bound = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
        vector<Point2<lng>> p;
        for (lng x = -bound; x <= bound; ++x) { for (lng y = -bound; y <= bound; ++y) { p.emplace_back(x, y); }}
        // atan2 is only an independent small-integer oracle, never the library comparator.
        auto angle = [](Point2<lng> a) {
            if (a == Point2<lng>{}) { return -1.L; }
            long double v = std::atan2((long double)a.y, (long double)a.x);
            return v < 0 ? v + 2 * std::acos(-1.L) : v; };
        auto reference = [&](Point2<lng> a, Point2<lng> b) {
            long double x = angle(a), y = angle(b);
            return x != y ? x < y : referenceNorm(a) < referenceNorm(b); };
        for (auto a : p) {
            context = "polar a=" + show(a); checkEqual(polarLess(a, a), false, "polar irreflexivity");
            for (auto b : p) {
                context = "polar a=" + show(a) + " b=" + show(b);
                checkEqual(polarLess(a, b), reference(a, b), "small-lattice angular oracle");
                checkEqual(polarLess(a, b) && polarLess(b, a), false, "polar asymmetry");
                for (auto c : p) {
                    if (polarLess(a, b) && polarLess(b, c)) {
                        context = "polar a=" + show(a) + " b=" + show(b) + " c=" + show(c);
                        checkEqual(polarLess(a, c), true, "polar transitivity"); }}}}
        std::mt19937_64 rng(seed); std::shuffle(p.begin(), p.end(), rng);
        vector<Point2<lng>> want = p;
        std::sort(p.begin(), p.end(), [](auto a, auto b) { return polarLess(a, b); });
        std::sort(want.begin(), want.end(), reference);
        for (int i = 0; i < int(p.size()); ++i) { checkEqual(p[i], want[i], "polar sort position=" + show(i)); }
        Point2<lng> a{1000000000, 999999999}, b{999999999, 999999998};
        context = "polar determinant unit cancellation a=" + show(a) + " b=" + show(b);
        checkEqual(polarLess(b, a), true, "large nearly parallel exact order");
        checkEqual(polarLess(a, b), false, "large nearly parallel reversed order");
        checkEqual(polarLess(Point2<lng>{}, a), true, "zero before nonzero");
        checkEqual(polarLess(Point2<lng>{1, 0}, Point2<lng>{2, 0}), true, "same angle shorter first");
        std::cout << "PASS polar zero/cut/radius policy, independent small-angle order and exhaustive strict ordering\n"; }

    void exhaustive() {
        vector<Point2<int>> p;
        for (int x = -2; x <= 2; ++x) { for (int y = -2; y <= 2; ++y) { p.emplace_back(x, y); }}
        for (auto a : p) { for (auto b : p) { pair2(a, b); for (auto c : p) { orient2(a, b, c); }}}
        vector<Point3<int>> q;
        for (int x = -1; x <= 1; ++x) { for (int y = -1; y <= 1; ++y) { for (int z = -1; z <= 1; ++z) { q.emplace_back(x, y, z); }}}
        for (auto a : q) { for (auto b : q) { pair3(a, b); }}
        vector<Point3<int>> corners;
        for (int x : {-1, 1}) { for (int y : {-1, 1}) { for (int z : {-1, 1}) { corners.emplace_back(x, y, z); }}}
        for (auto a : corners) { for (auto b : corners) { for (auto c : corners) { for (auto d : corners) { triple3(a, b, c, d); }}}}
        std::cout << "PASS exhaustive 2D small lattice/orientations and 3D cube determinant/degeneracy corpus\n"; }

    void boundaries() {
        vector<lng> values{-1000000000, -999999999, -1, 0, 1, 999999999, 1000000000};
        for (lng x : values) { for (lng y : values) {
            pair2(Point2<lng>{x, y}, Point2<lng>{-y, -x});
            pair3(Point3<lng>{x, y, x}, Point3<lng>{-y, -x, -y});
            triple3(Point3<lng>{x, y, x}, Point3<lng>{-x, x, y}, Point3<lng>{y, -x, -y}, Point3<lng>{-y, -y, -x}); }}
        Point2<int> a{INT_MIN, INT_MIN}, b{INT_MAX, INT_MAX}, c{INT_MIN, INT_MAX};
        pair2(a, b); orient2(a, b, c);
        Point3<int> d{INT_MIN, INT_MIN, INT_MIN}, e{INT_MAX, INT_MIN, INT_MIN}, f{INT_MIN, INT_MAX, INT_MIN}, g{INT_MIN, INT_MIN, INT_MAX};
        pair3(d, e); triple3(d, e, f, g);
        Point2<lng> low{std::numeric_limits<lng>::min(), 0}, high{std::numeric_limits<lng>::max(), 0};
        pair2(low, Point2<lng>{}); orient2(low, high, Point2<lng>{0, 1});
        Point3<lng> low3{std::numeric_limits<lng>::min(), 0, 0};
        pair3(low3, Point3<lng>{});
        lll large = lll(1) << 80;
        orient2(Point2<lll>{-large, 0}, Point2<lll>{large, 0}, Point2<lll>{0, 1});
        context = "extended integer coordinate x=2^80 and basis vector";
        checkEqual(lll(dot(Point2<lll>{large, 0}, Point2<lll>{1, 0})), lll(large), "lll coordinate dot");
        checkEqual(lll(cross(Point2<lll>{large, 0}, Point2<lll>{0, 1})), lll(large), "lll coordinate cross");
        checkEqual(Point2<int>{-7, 7} / 2, Point2<int>{-3, 3}, "integer division truncates toward zero");
        checkEqual(Point3<int>{-7, 7, -1} / 2, Point3<int>{-3, 3, 0}, "3D integer division truncates toward zero");
        checkEqual(Point2<double>{2.75, -3.5}.cast<int>(), Point2<int>{2, -3}, "explicit floating to integral cast");
        checkEqual(Point3<double>{2.75, -3.5, 0.5}.cast<int>(), Point3<int>{2, -3, 0}, "explicit 3D floating to integral cast");
        std::cout << "PASS guaranteed coordinate bounds, int32/int64/lll promotion regressions and division/cast semantics\n"; }

    template<typename T> void floating() {
        Point2<T> a{T(0.5), T(-0.25)}, b{T(1.25), T(2)};
        Point3<T> c{T(0.5), T(-0.25), T(1.5)}, d{T(1.25), T(2), T(-2)};
        context = "floating coordinate bytes=" + show(sizeof(T));
        close(dot(a, b), 0.125L, "floating 2D dot"); close(cross(a, b), 1.3125L, "floating 2D cross");
        close(norm2(a), 0.3125L, "floating 2D norm2"); close(dist2(a, b), 5.625L, "floating 2D dist2");
        close(normApprox(a), std::sqrt(0.3125L), "floating 2D normApprox");
        close(distanceApprox(a, b), std::sqrt(5.625L), "floating 2D distanceApprox");
        checkEqual(orient(a, b, Point2<T>{}), 1, "floating orientation sign");
        checkEqual(orient(a, a, b), 0, "floating orientation duplicate");
        close(dot(c, d), -2.875L, "floating 3D dot");
        auto n = cross(c, d);
        close(n.x, -2.5L, "floating 3D cross x"); close(n.y, 2.875L, "floating 3D cross y"); close(n.z, 1.3125L, "floating 3D cross z");
        close(norm2(c), 2.5625L, "floating 3D norm2"); close(dist2(c, d), 17.875L, "floating 3D dist2");
        close(normApprox(c), std::sqrt(2.5625L), "floating 3D normApprox");
        close(distanceApprox(c, d), std::sqrt(17.875L), "floating 3D distanceApprox");
        close(triple(c, d, Point3<T>{T(0), T(0), T(2)}), 2.625L, "floating scalar triple");
        close(tetraVolume6(Point3<T>{}, c, d, Point3<T>{T(0), T(0), T(2)}), 2.625L, "floating tetrahedron");
        T near = std::nextafter(T(1), T(2));
        checkEqual(Point2<T>{T(1), T(3)} < Point2<T>{near, T(-3)}, true, "epsilon-free floating lexicographic x");
        checkEqual(Point3<T>{T(1), T(3), T(4)} < Point3<T>{T(1), T(3), std::nextafter(T(4), T(5))}, true, "epsilon-free floating lexicographic z");
        checkEqual(Point2<T>{T(-0.), T(0)} == Point2<T>{}, true, "signed zero equality");
        checkEqual(Point3<T>{T(-0.), T(0), T(-0.)} == Point3<T>{}, true, "3D signed zero equality");
        int exponent = std::is_same_v<T, float> ? 18 : std::is_same_v<T, double> ? 150 : 2000;
        for (int power : {-exponent, exponent}) {
            T scale = T(std::pow(10.L, power));
            Point2<T> p{scale, T(-scale / 2)}, q{T(-scale / 4), T(scale / 2)};
            context = "floating coordinate bytes=" + show(sizeof(T)) + " scale=10^" + show(power);
            closeRelative(normApprox(p), std::hypot((long double)p.x, (long double)p.y), "scale-varying 2D norm");
            closeRelative(distanceApprox(p, q), std::hypot((long double)p.x - q.x, (long double)p.y - q.y), "scale-varying 2D distance");
            Point3<T> r{p.x, p.y, q.x}, s{q.x, q.y, p.x};
            closeRelative(normApprox(r), std::hypot((long double)r.x, (long double)r.y, (long double)r.z), "scale-varying 3D norm");
            closeRelative(distanceApprox(r, s), std::hypot((long double)r.x - s.x, (long double)r.y - s.y, (long double)r.z - s.z), "scale-varying 3D distance"); }}

    void randomCases() {
        std::mt19937_64 rng(seed); std::uniform_int_distribution<lng> dist(-1000000000, 1000000000);
        auto point2 = [&]() { return Point2<lng>{dist(rng), dist(rng)}; };
        auto point3 = [&]() { return Point3<lng>{dist(rng), dist(rng), dist(rng)}; };
        int count = mode == "quick" ? 100 : mode == "full" ? 3000 : 15000;
        for (int i = 0; i < count; ++i) {
            auto a = point2(), b = point2(), c = point2(); pair2(a, b); orient2(a, b, c);
            auto d = point3(), e = point3(), f = point3(), g = point3(); pair3(d, e); triple3(d, e, f, g);
            auto translation = point2();
            checkEqual(orient(a + translation, b + translation, c + translation), orient(a, b, c), "translation invariant orientation");
            auto translation3 = point3();
            checkEqual(tetraVolume6(d + translation3, e + translation3, f + translation3, g + translation3), tetraVolume6(d, e, f, g), "translation invariant volume"); }
        std::cout << "PASS " << count << " seeded exact primitive/volume and translation-invariance cases\n"; }

    void pythonOracle() {
        const char *path = std::getenv("CP_POINT_ORACLE");
        if (!path) { throw std::runtime_error("CP_POINT_ORACLE absent; run 01-point_tester.py to generate exact Python fixtures"); }
        std::ifstream input(path);
        if (!input) { throw std::runtime_error("cannot open Python oracle=" + string(path)); }
        auto expected = [&]() {
            string text; input >> text;
            if (text.empty()) { throw std::runtime_error("truncated Python oracle"); }
            bool negative = text[0] == '-'; ulll value = 0;
            for (int i = negative; i < int(text.size()); ++i) { value = 10 * value + text[i] - '0'; }
            return negative ? -lll(value) : lll(value); };
        int dimension, count = 0;
        while (input >> dimension) {
            ++count;
            if (dimension == 2) {
                Point2<lng> a, b, c; input >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y;
                context = "Python exact record=" + show(count) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c);
                checkEqual(dot(a, b), expected(), "Python exact 2D dot"); checkEqual(cross(a, b), expected(), "Python exact 2D cross");
                checkEqual(norm2(a), expected(), "Python exact 2D norm2"); checkEqual(dist2(a, b), expected(), "Python exact 2D dist2");
                checkEqual(orient(a, b, c), expected(), "Python exact orientation"); }
            else if (dimension == 3) {
                Point3<lng> a, b, c, d;
                input >> a.x >> a.y >> a.z >> b.x >> b.y >> b.z >> c.x >> c.y >> c.z >> d.x >> d.y >> d.z;
                context = "Python exact record=" + show(count) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c) + " d=" + show(d);
                checkEqual(dot(a, b), expected(), "Python exact 3D dot"); auto normal = cross(a, b);
                checkEqual(normal.x, expected(), "Python exact 3D cross x"); checkEqual(normal.y, expected(), "Python exact 3D cross y");
                checkEqual(normal.z, expected(), "Python exact 3D cross z"); checkEqual(norm2(a), expected(), "Python exact 3D norm2");
                checkEqual(dist2(a, b), expected(), "Python exact 3D dist2"); checkEqual(triple(a, b, c), expected(), "Python exact scalar triple");
                checkEqual(tetraVolume6(a, b, c, d), expected(), "Python exact tetrahedron"); }
            else { throw std::runtime_error("unknown Python oracle dimension=" + show(dimension)); }}
        checkEqual(count > 0, true, "nonempty Python exact corpus");
        std::cout << "PASS " << count << " independent arbitrary-precision Python primitive/orientation/volume records\n"; }
}

static_assert(std::is_same_v<decltype(dot(Point2<int>{}, Point2<int>{})), lll>);
static_assert(std::is_same_v<decltype(cross(Point2<lng>{}, Point2<lng>{})), lll>);
static_assert(std::is_same_v<decltype(cross(Point3<int>{}, Point3<int>{})), Point3<lll>>);
static_assert(std::is_same_v<decltype(dot(Point3<double>{}, Point3<double>{})), long double>);
static_assert(std::is_same_v<decltype(cross(Point3<double>{}, Point3<double>{})), Point3<long double>>);
static_assert(std::is_same_v<decltype(norm2(Point2<float>{})), long double>);
static_assert(std::is_same_v<decltype(distanceApprox(Point3<lng>{}, Point3<lng>{})), long double>);
static_assert(Point2<int>{1, 2} + Point2<int>{3, 4} == Point2<int>{4, 6});
static_assert(Point3<int>{1, 2, 3} * 2 == Point3<int>{2, 4, 6});
static_assert(dot(Point2<int>{1, 2}, Point2<int>{3, 4}) == 11);
static_assert(cross(Point3<int>{1, 0, 0}, Point3<int>{0, 1, 0}) == Point3<lll>{0, 0, 1});
static_assert(norm2(Point3<int>{2, 3, 6}) == 49 && dist2(Point2<int>{1, 2}, Point2<int>{4, 6}) == 25);
static_assert(orient(Point2<int>{}, Point2<int>{1, 0}, Point2<int>{0, 1}) == 1);
static_assert(polarLess(Point2<int>{1, 0}, Point2<int>{0, 1}));
static_assert(tetraVolume6(Point3<int>{}, Point3<int>{1, 0, 0}, Point3<int>{0, 1, 0}, Point3<int>{0, 0, 1}) == 1);

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid" && i + 1 < argc) {
                string probe = argv[++i];
                if (probe == "division-zero-2d") { Point2<lng> p{1, 2}; p /= 0; }
                else if (probe == "division-zero-3d") { Point3<lng> p{1, 2, 3}; p /= 0; }
                else { throw std::runtime_error("unknown invalid probe=" + probe); }
                throw std::runtime_error("assertion probe returned=" + probe); }
            else { throw std::runtime_error("unknown argument=" + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode=" + mode); }
        std::cout << "RUN point mode=" << mode << " seed=" << seed << '\n';
        operators<int>(); operators<lng>(); operators<lll>(); operators<float>(); operators<double>(); operators<long double>();
        std::cout << "PASS all point constructors, arithmetic/mutating operators, aliasing, casts and value semantics\n";
        exhaustive(); boundaries(); polar(); floating<float>(); floating<double>(); floating<long double>();
        std::cout << "PASS float/double/long-double primitives, approximate roots and exact lexicographic comparison\n";
        randomCases(); pythonOracle();
        std::cout << "PASS point checks=" << checks << " mode=" << mode << " seed=" << seed << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << "FAIL point mode=" << mode << " seed=" << seed << " smallest-known-reproducer: " << error.what() << '\n'; return 1; }
}
