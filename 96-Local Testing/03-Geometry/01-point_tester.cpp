#pragma GCC diagnostic error "-Wnarrowing"
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
        std::reverse(out.begin(), out.end()); return out;}
    string show(ulll x) {
        if (!x) { return "0"; }
        string out;
        while (x) { out += char('0' + x % 10); x /= 10; }
        std::reverse(out.begin(), out.end()); return out;}
    template<typename T> string show(const T &x) {
        std::ostringstream out; out << std::setprecision(24) << x; return out.str();}
    template<typename T> string show(Point2<T> p) { return "(" + show(p.x) + "," + show(p.y) + ")"; }
    template<typename T> string show(Point3<T> p) { return "(" + show(p.x) + "," + show(p.y) + "," + show(p.z) + ")"; }
    template<typename T> string show(Angle<T> a) { return show(a.v.template cast<lll>()) + "@" + show(lll(a.t)); }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &operation) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got));}}
    void close(long double got, long double want, const string &operation) {
        ++checks;
        long double tolerance = 128 * std::numeric_limits<long double>::epsilon() * std::max(1.L, std::abs(want));
        if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)
                                     + " absolute-tolerance=" + show(tolerance));}}
    void closeRelative(long double got, long double want, const string &operation) {
        ++checks;
        long double tolerance = 128 * std::numeric_limits<long double>::epsilon() * std::abs(want);
        if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)
                                     + " relative-tolerance=" + show(128 * std::numeric_limits<long double>::epsilon()));}}
    void closeWithin(long double got, long double want, long double tolerance, const string &operation) {
        ++checks;
        if (!std::isfinite(got) || std::abs(got - want) > tolerance) {
            throw std::runtime_error(context + " operation=" + operation + " expected=" + show(want) + " actual=" + show(got)
                                     + " absolute-tolerance=" + show(tolerance));}}

    // Leibniz determinant of homogeneous coordinates: independent of vector subtraction/cross.
    template<int N> lll determinant(const array<array<lll, N>, N> &a) {
        array<int, N> p; std::iota(p.begin(), p.end(), 0); lll answer = 0;
        do {
            lll term = 1; int inversions = 0;
            for (int i = 0; i < N; ++i) {
                term *= a[i][p[i]];
                for (int j = 0; j < i; ++j) { inversions += p[j] > p[i]; }}
            answer += inversions & 1 ? -term : term;} while (std::next_permutation(p.begin(), p.end()));
        return answer;}
    template<typename T> lll area(Point2<T> a, Point2<T> b, Point2<T> c) {
        return determinant<3>({{{1, lll(a.x), lll(a.y)}, {1, lll(b.x), lll(b.y)}, {1, lll(c.x), lll(c.y)}}});}
    template<typename T> lll volume(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
        return determinant<4>({{{1, lll(a.x), lll(a.y), lll(a.z)}, {1, lll(b.x), lll(b.y), lll(b.z)},
                               {1, lll(c.x), lll(c.y), lll(c.z)}, {1, lll(d.x), lll(d.y), lll(d.z)}}});}
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
        close(distanceApprox(a, b), std::hypot((long double)x, (long double)y), "2D distanceApprox");}
    template<typename T> void orient2(Point2<T> a, Point2<T> b, Point2<T> c) {
        context = "a=" + show(a) + " b=" + show(b) + " c=" + show(c);
        lll answer = area(a, b, c); int sign = (answer > 0) - (answer < 0);
        checkEqual(orient(a, b, c), sign, "homogeneous determinant orientation");
        checkEqual(orient(b, a, c), -sign, "orientation antisymmetry");
        checkEqual(orient(b, c, a), sign, "orientation cyclic permutation");}
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
                                              (long double)z), "3D distanceApprox");}
    template<typename T> void triple3(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
        context = "a=" + show(a) + " b=" + show(b) + " c=" + show(c) + " d=" + show(d);
        lll expected = volume(Point3<T>{}, a, b, c), tetra = volume(a, b, c, d);
        checkEqual(lll(triple(a, b, c)), expected, "scalar triple homogeneous determinant");
        checkEqual(lll(triple(b, a, c)), lll(-expected), "scalar triple antisymmetry");
        checkEqual(lll(triple(b, c, a)), expected, "scalar triple cyclic permutation");
        checkEqual(lll(tetraVolume6(a, b, c, d)), tetra, "oriented tetrahedron homogeneous determinant");
        checkEqual(lll(tetraVolume6(b, a, c, d)), lll(-tetra), "tetrahedron vertex transposition");}

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
        checkEqual(a / T(-3), Point2<T>{T(-2), T(3)}, "2D division by a negative scalar");
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
        checkEqual(c / T(-3), Point3<T>{T(-2), T(3), T(-4)}, "3D division by a negative scalar");
        auto q = c; checkEqual(&(q += d), &q, "3D += reference"); checkEqual(q, c + d, "3D +=");
        checkEqual(&(q -= d), &q, "3D -= reference"); checkEqual(q, c, "3D -=");
        checkEqual(&(q *= T(3)), &q, "3D *= reference"); checkEqual(q, c * T(3), "3D *=");
        checkEqual(&(q /= T(3)), &q, "3D /= reference"); checkEqual(q, c, "3D /=");
        q += q; checkEqual(q, T(2) * c, "3D += self"); q -= q; checkEqual(q, Point3<T>{}, "3D -= self");
        checkEqual(c.template cast<long double>(), Point3<long double>{6, -9, 12}, "3D cast");
        auto copied = a; auto moved = std::move(copied); checkEqual(moved, a, "2D copy and move");
        auto copied3 = c; auto moved3 = std::move(copied3); checkEqual(moved3, c, "3D copy and move");}

    void polar() {
        int bound = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
        vector<Point2<lng>> p;
        for (lng x = -bound; x <= bound; ++x) { for (lng y = -bound; y <= bound; ++y) { p.emplace_back(x, y); }}
        // atan2 is only an independent small-integer oracle, never the library comparator.
        auto angle = [](Point2<lng> a) {
            if (a == Point2<lng>{}) { return -1.L; }
            long double v = std::atan2((long double)a.y, (long double)a.x);
            return v < 0 ? v + 2 * std::acos(-1.L) : v;};
        auto reference = [&](Point2<lng> a, Point2<lng> b) {
            long double x = angle(a), y = angle(b);
            return x != y ? x < y : referenceNorm(a) < referenceNorm(b);};
        for (auto a : p) {
            context = "polar a=" + show(a); checkEqual(polarLess(a, a), false, "polar irreflexivity");
            for (auto b : p) {
                context = "polar a=" + show(a) + " b=" + show(b);
                checkEqual(polarLess(a, b), reference(a, b), "small-lattice angular oracle");
                checkEqual(polarLess(a, b) && polarLess(b, a), false, "polar asymmetry");
                for (auto c : p) {
                    if (polarLess(a, b) && polarLess(b, c)) {
                        context = "polar a=" + show(a) + " b=" + show(b) + " c=" + show(c);
                        checkEqual(polarLess(a, c), true, "polar transitivity");}}}}
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
        std::cout << "PASS polar zero/cut/radius policy, independent small-angle order and exhaustive strict ordering\n";}

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
        std::cout << "PASS exhaustive 2D small lattice/orientations and 3D cube determinant/degeneracy corpus\n";}

    void boundaries() {
        vector<lng> values{-1000000000, -999999999, -1, 0, 1, 999999999, 1000000000};
        for (lng x : values) { for (lng y : values) {
            pair2(Point2<lng>{x, y}, Point2<lng>{-y, -x});
            pair3(Point3<lng>{x, y, x}, Point3<lng>{-y, -x, -y});
            triple3(Point3<lng>{x, y, x}, Point3<lng>{-x, x, y}, Point3<lng>{y, -x, -y}, Point3<lng>{-y, -y, -x});}}
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
        std::cout << "PASS guaranteed coordinate bounds, int32/int64/lll promotion regressions and division/cast semantics\n";}

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
            closeRelative(distanceApprox(r, s), std::hypot((long double)r.x - s.x, (long double)r.y - s.y, (long double)r.z - s.z), "scale-varying 3D distance");}}

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
            checkEqual(tetraVolume6(d + translation3, e + translation3, f + translation3, g + translation3), tetraVolume6(d, e, f, g), "translation invariant volume");}
        std::cout << "PASS " << count << " seeded exact primitive/volume and translation-invariance cases\n";}

    // Python's math.atan2 keys order points whose keys differ by more than 1e-12; one group is one exact direction, ranked by exact norm.
    template<typename T> void approxFloatingCase(const vector<double> &x, const vector<double> &y, const vector<double> &key, const vector<int> &group, const vector<int> &rank) {
        int n = int(x.size()); vector<Point2<T>> p;
        for (int i = 0; i < n; ++i) { p.emplace_back(T(x[i]), T(y[i])); }
        auto got = argumentSortApprox(p);
        checkEqual(int(got.size()), n, "argumentSortApprox size");
        auto same = [](T a, T b) { return a == b && std::signbit(a) == std::signbit(b); };
        vector<int> index; vector<bool> used(n);
        for (auto q : got) {
            int j = 0;
            while (j < n && (used[j] || !same(q.x, p[j].x) || !same(q.y, p[j].y))) { ++j; }
            checkEqual(j < n, true, "argumentSortApprox output is an unused input element, signed zeros included");
            used[j] = true; index.push_back(j);}
        for (int a = 0; a < n; ++a) {
            for (int b = a + 1; b < n; ++b) {
                int i = index[a], j = index[b];
                bool ok = group[i] == group[j] ? pair{rank[i], i} < pair{rank[j], j} : std::abs(key[i] - key[j]) <= 1e-12 || key[i] < key[j];
                checkEqual(ok, true, ok ? string() : "Python key order positions=" + show(a) + "," + show(b) + " inputs=" + show(i) + "," + show(j)
                                                    + " keys=" + show(key[i]) + "," + show(key[j]) + " groups=" + show(group[i]) + "," + show(group[j]));}}}

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
            return negative ? -lll(value) : lll(value);};
        int dimension, count = 0, approx = 0;
        while (input >> dimension) {
            ++count;
            if (dimension == 4) {
                int n; input >> n; vector<double> x(n), y(n), key(n); vector<int> group(n), rank(n);
                for (int i = 0; i < n; ++i) {
                    string u, v, k; input >> u >> v >> k >> group[i] >> rank[i];
                    x[i] = std::strtod(u.c_str(), nullptr); y[i] = std::strtod(v.c_str(), nullptr); key[i] = std::strtod(k.c_str(), nullptr);}
                context = "Python argumentSortApprox record=" + show(count) + " n=" + show(n) + (n ? " first=(" + show(x[0]) + "," + show(y[0]) + ")" : string());
                approxFloatingCase<long double>(x, y, key, group, rank); approxFloatingCase<double>(x, y, key, group, rank); ++approx;}
            else if (dimension == 2) {
                Point2<lng> a, b, c; input >> a.x >> a.y >> b.x >> b.y >> c.x >> c.y;
                context = "Python exact record=" + show(count) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c);
                checkEqual(dot(a, b), expected(), "Python exact 2D dot"); checkEqual(cross(a, b), expected(), "Python exact 2D cross");
                checkEqual(norm2(a), expected(), "Python exact 2D norm2"); checkEqual(dist2(a, b), expected(), "Python exact 2D dist2");
                checkEqual(orient(a, b, c), expected(), "Python exact orientation");}
            else if (dimension == 3) {
                Point3<lng> a, b, c, d;
                input >> a.x >> a.y >> a.z >> b.x >> b.y >> b.z >> c.x >> c.y >> c.z >> d.x >> d.y >> d.z;
                context = "Python exact record=" + show(count) + " a=" + show(a) + " b=" + show(b) + " c=" + show(c) + " d=" + show(d);
                checkEqual(dot(a, b), expected(), "Python exact 3D dot"); auto normal = cross(a, b);
                checkEqual(normal.x, expected(), "Python exact 3D cross x"); checkEqual(normal.y, expected(), "Python exact 3D cross y");
                checkEqual(normal.z, expected(), "Python exact 3D cross z"); checkEqual(norm2(a), expected(), "Python exact 3D norm2");
                checkEqual(dist2(a, b), expected(), "Python exact 3D dist2"); checkEqual(triple(a, b, c), expected(), "Python exact scalar triple");
                checkEqual(tetraVolume6(a, b, c, d), expected(), "Python exact tetrahedron");}
            else { throw std::runtime_error("unknown Python oracle dimension=" + show(dimension)); }}
        checkEqual(count > approx && approx > 0, true, "nonempty Python exact and argumentSortApprox corpus");
        std::cout << "PASS " << count - approx << " independent arbitrary-precision Python primitive/orientation/volume records and " << approx
                  << " Python math.atan2 argumentSortApprox records (signed zeros, power-of-two scalings, duplicates)\n";}

    // Key oracle: brute-force primitive divisor search, independent of gcd.
    template<typename T> void canonicalCase(Point2<T> a) {
        context = "canonicalDirection a=" + show(a);
        auto k = canonicalDirection(a);
        if (a == Point2<T>{}) { checkEqual(k, a, "zero direction key"); return; }
        checkEqual(k.y > 0 || (k.y == 0 && k.x > 0), true, "key sign rule");
        checkEqual(lll(gcd(lll(k.x), lll(k.y))), lll(1), "key primitive");
        checkEqual(lll(cross(a, k)), lll(0), "key parallel to input");
        lll g = k.x ? lll(a.x) / k.x : lll(a.y) / k.y;
        checkEqual(lll(a.x) == g * k.x && lll(a.y) == g * k.y, true, "input is an integral multiple of key");
        checkEqual(canonicalDirection(-a), k, "opposite directions share key");
        checkEqual(canonicalDirection(k), k, "key idempotent");}
    void directions() {
        int bound = mode == "quick" ? 4 : mode == "full" ? 8 : 12;
        vector<Point2<lng>> p;
        for (lng x = -bound; x <= bound; ++x) { for (lng y = -bound; y <= bound; ++y) { p.emplace_back(x, y); }}
        for (auto a : p) {
            canonicalCase(a);
            auto k = canonicalDirection(a);
            bool primitive = true;
            for (lng d = 2; d <= bound; ++d) { primitive &= k.x % d != 0 || k.y % d != 0; }
            checkEqual(primitive || a == Point2<lng>{}, true, "key primitive by divisor search");
            for (auto b : p) {
                context = "canonicalDirection a=" + show(a) + " b=" + show(b);
                bool same = (a == Point2<lng>{}) == (b == Point2<lng>{}) && cross(a, b) == 0;
                checkEqual(canonicalDirection(a) == canonicalDirection(b), same, "equal keys iff parallel");}}
        std::mt19937_64 rng(seed ^ 0x5eed); std::uniform_int_distribution<lng> big(-1000000000000000000, 1000000000000000000), small(-1000, 1000);
        int count = mode == "quick" ? 200 : mode == "full" ? 5000 : 30000;
        for (int i = 0; i < count; ++i) {
            lng m = small(rng), x = small(rng), y = small(rng);
            canonicalCase(Point2<lng>{big(rng), big(rng)});
            if (m && (x || y) && std::gcd(x, y) == 1) {
                Point2<lng> a{m * x, m * y}, k = y > 0 || (y == 0 && x > 0) ? Point2<lng>{x, y} : Point2<lng>{-x, -y};
                context = "canonicalDirection scaled primitive a=" + show(a);
                checkEqual(canonicalDirection(a), k, "scaled primitive recovers key");}}
        canonicalCase(Point2<int>{INT_MAX, -INT_MAX}); canonicalCase(Point2<int>{INT_MIN + 2, 0});
        canonicalCase(Point2<lng>{std::numeric_limits<lng>::max(), std::numeric_limits<lng>::max() - 1});
        canonicalCase(Point2<lll>{lll(6) << 100, -(lll(4) << 100)});
        canonicalCase(Point2<int8_t>{-128 + 2, 42}); canonicalCase(Point2<int16_t>{0, -32767});
        context = "canonicalDirection fixed values";
        checkEqual(canonicalDirection(Point2<lll>{lll(6) << 100, -(lll(4) << 100)}), Point2<lll>{-3, 2}, "lll key");
        checkEqual(canonicalDirection(Point2<int>{0, -7}), Point2<int>{0, 1}, "vertical key");
        checkEqual(canonicalDirection(Point2<int>{-7, 0}), Point2<int>{1, 0}, "horizontal key");
        std::cout << "PASS canonicalDirection exhaustive parallel-class oracle, scaled primitives and width boundaries\n";}

    // Angle oracles are trigonometric identities and the exact polar order, not atan2 comparisons.
    template<typename T> void angleCase(Point2<T> a, Point2<T> b) {
        context = "angles a=" + show(a) + " b=" + show(b);
        long double pi = std::acos(-1.L), na = normApprox(a), c = static_cast<long double>(cross(a, b)), d = static_cast<long double>(dot(a, b));
        long double t = argApprox(a), u = angleApprox(a, b), v = signedAngleApprox(a, b), w = std::hypot(c, d);
        checkEqual(t > -pi && t <= pi, true, "arg range (-pi, pi]");
        close(std::cos(t), static_cast<long double>(a.x) / na, "arg cosine identity");
        close(std::sin(t), static_cast<long double>(a.y) / na, "arg sine identity");
        checkEqual(u >= 0 && u <= pi, true, "angle range [0, pi]");
        checkEqual(v > -pi && v <= pi, true, "signed angle range (-pi, pi]");
        close(std::cos(u), d / w, "angle cosine identity"); close(std::sin(u), std::abs(c) / w, "angle sine identity");
        close(std::cos(v), d / w, "signed angle cosine identity"); close(std::sin(v), c / w, "signed angle sine identity");
        checkEqual(geometry_detail::sign(v), c == 0 ? (d < 0 ? 1 : 0) : geometry_detail::sign(c), "signed angle exact sign");
        checkEqual(angleApprox(a, b), angleApprox(b, a), "angle symmetry");
        close(std::abs(v), u, "signed angle magnitude");
        auto e = unitApprox(a);
        close(norm2(e), 1, "unit length"); close(e.x, static_cast<long double>(a.x) / na, "unit x"); close(e.y, static_cast<long double>(a.y) / na, "unit y");
        Point3<T> a3{a.x, a.y, T(0)}, b3{b.x, b.y, T(0)};
        close(angleApprox(a3, b3), u, "3D angle of embedded plane vectors");
        auto e3 = unitApprox(a3); close(e3.x, e.x, "3D unit x"); close(e3.y, e.y, "3D unit y"); close(e3.z, 0, "3D unit z");}
    void angles() {
        int bound = mode == "quick" ? 2 : 4;
        vector<Point2<lng>> p;
        for (lng x = -bound; x <= bound; ++x) { for (lng y = -bound; y <= bound; ++y) { if (x || y) { p.emplace_back(x, y); }}}
        long double pi = std::acos(-1.L);
        auto turn = [&](long double x) { return x < 0 ? x + 2 * pi : x; };
        for (auto a : p) {
            for (auto b : p) {
                angleCase(a, b);
                context = "arg order a=" + show(a) + " b=" + show(b);
                bool parallel = cross(a, b) == 0 && dot(a, b) > 0;
                if (!parallel) { checkEqual(turn(argApprox(a)) < turn(argApprox(b)), polarLess(a, b), "arg agrees with exact polar order"); }}}
        std::mt19937_64 rng(seed ^ 0xa11); std::uniform_int_distribution<lng> dist(-1000000000, 1000000000);
        int count = mode == "quick" ? 200 : mode == "full" ? 5000 : 30000;
        for (int i = 0; i < count; ++i) {
            Point2<lng> a{dist(rng), dist(rng)}, b{dist(rng), dist(rng)};
            if (a != Point2<lng>{} && b != Point2<lng>{}) { angleCase(a, b); angleCase(a, a * 3); angleCase(a, -a); angleCase(a, perp(a)); }
            Point3<lng> c{dist(rng), dist(rng), dist(rng)}, d{dist(rng), dist(rng), dist(rng)};
            context = "3D angle c=" + show(c) + " d=" + show(d);
            long double u = angleApprox(c, d), n = std::hypot(static_cast<long double>(dot(c, d)), normApprox(cross(c, d).template cast<long double>()));
            close(std::cos(u), static_cast<long double>(dot(c, d)) / n, "3D angle cosine identity");
            close(std::sin(u), normApprox(cross(c, d).template cast<long double>()) / n, "3D angle sine identity");
            auto e = unitApprox(c); close(norm2(e), 1, "3D unit length");}
        context = "angle fixed values";
        close(angleApprox(Point2<int>{1, 0}, Point2<int>{-1, 0}), pi, "straight angle");
        close(signedAngleApprox(Point2<int>{1, 0}, Point2<int>{-1, 0}), pi, "straight signed angle is +pi");
        close(signedAngleApprox(Point2<int>{1, 0}, Point2<int>{0, -1}), -pi / 2, "clockwise quarter");
        close(argApprox(Point2<int>{-1, 0}), pi, "arg of -x is +pi");
        close(argApprox(Point2<double>{-1, -0.}), pi, "arg of -x with negative zero is +pi");
        close(signedAngleApprox(Point2<double>{-1, 0}, Point2<double>{1, 0}), pi, "floating opposite vectors give +pi");
        close(angleApprox(Point3<int>{1, 0, 0}, Point3<int>{0, 0, 5}), pi / 2, "3D right angle");
        closeRelative(angleApprox(Point2<double>{1, 1e-300}, Point2<double>{1, 0}), static_cast<long double>(1e-300), "tiny floating angle");
        closeRelative(angleApprox(Point2<lng>{1000000000, 999999999}, Point2<lng>{999999999, 999999998}), std::atan2(1.L, 1999999996000000002.L), "near-parallel cross -1");
        angleCase(Point2<double>{0.5, -0.25}, Point2<double>{1.25, 2}); angleCase(Point2<float>{-3, 4}, Point2<float>{4, 3});
        angleCase(Point2<double>{-1, 0}, Point2<double>{1, 0}); angleCase(Point2<long double>{-2, 0}, Point2<long double>{3, 0});
        angleCase(Point2<double>{-1, -0.}, Point2<double>{2, 0}); angleCase(Point2<double>{-1, -0.}, Point2<double>{-1, 1});
        std::cout << "PASS arg/angle/signed angle/unit identities, exact polar-order agreement and 3D embedding\n";}

    // Exact class key written apart from polarLess: y < 0, then zero, then the positive x-axis, then y > 0 with the negative x-axis.
    template<typename T> bool argumentLess(Point2<T> a, Point2<T> b) {
        auto group = [](Point2<T> p) { return p.y < 0 ? 0 : p.x == 0 && p.y == 0 ? 1 : p.y == 0 && p.x > 0 ? 2 : 3; };
        if (group(a) != group(b)) { return group(a) < group(b); }
        lll c = lll(a.x) * b.y - lll(a.y) * b.x;
        return c ? c > 0 : referenceNorm(a) < referenceNorm(b);}
    template<typename T> void argumentCase(const vector<Point2<T>> &p) {
        context = "argumentSort n=" + show(int(p.size())) + (p.empty() ? string() : " first=" + show(p[0].template cast<lll>()));
        vector<Point2<T>> want = p; std::sort(want.begin(), want.end(), argumentLess<T>);
        auto got = argumentSort(p);
        checkEqual(int(got.size()), int(want.size()), "argumentSort size");
        for (int i = 0; i < int(got.size()); ++i) { checkEqual(got[i].template cast<lll>(), want[i].template cast<lll>(), "argumentSort position=" + show(i)); }}
    template<typename T> vector<Point2<T>> randomPoints(std::mt19937_64 &rng, int n, lng bound) {
        std::uniform_int_distribution<lng> coordinate(-bound, bound); vector<Point2<T>> p;
        for (int i = 0; i < n; ++i) { p.emplace_back(T(coordinate(rng)), T(coordinate(rng))); }
        return p;}
    void argumentOrder() {
        int bound = mode == "quick" ? 2 : 3;
        vector<Point2<lng>> p;
        for (lng x = -bound; x <= bound; ++x) { for (lng y = -bound; y <= bound; ++y) { p.emplace_back(x, y); }}
        // atan2 validates the exact key on the small lattice, where distinct directions have distinct rounded arguments.
        auto reference = [](Point2<lng> a, Point2<lng> b) {
            long double x = std::atan2((long double)a.y, (long double)a.x), y = std::atan2((long double)b.y, (long double)b.x);
            return x != y ? x < y : referenceNorm(a) < referenceNorm(b);};
        for (auto a : p) {
            for (auto b : p) {
                context = "argument key a=" + show(a) + " b=" + show(b);
                checkEqual(argumentLess(a, b), reference(a, b), "exact argument key against atan2");}}
        std::mt19937_64 rng(seed ^ 0xa4c);
        auto doubled = p; doubled.insert(doubled.end(), p.begin(), p.end()); std::shuffle(doubled.begin(), doubled.end(), rng);
        argumentCase(doubled); argumentCase(vector<Point2<lng>>{}); argumentCase(vector<Point2<lng>>{{0, 0}});
        argumentCase(vector<Point2<lng>>{{0, 0}, {0, 0}}); argumentCase(vector<Point2<lng>>{{-1, 0}, {0, 0}, {1, 0}, {0, -1}});
        vector<Point2<int>> sample{{1, 0}, {0, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {2, 2}, {-1, -1}, {2, 0}, {1, -1}};
        vector<Point2<int>> order{{-1, -1}, {0, -1}, {1, -1}, {0, 0}, {1, 0}, {2, 0}, {1, 1}, {2, 2}, {0, 1}, {-1, 0}};
        context = "argumentSort hand-ordered sample";
        checkEqual(argumentSort(sample) == order, true, "Library Checker argument order with zero and ties");
        int count = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
        std::uniform_int_distribution<int> size(0, 24); std::uniform_int_distribution<lng> base(-1000, 1000), factor(-1000000, 1000000);
        for (int i = 0; i < count; ++i) {
            argumentCase(randomPoints<int8_t>(rng, size(rng), 3)); argumentCase(randomPoints<int16_t>(rng, size(rng), 3));
            argumentCase(randomPoints<int>(rng, size(rng), 3)); argumentCase(randomPoints<lng>(rng, size(rng), 3));
            auto wide = randomPoints<int8_t>(rng, size(rng), 127); wide.emplace_back(int8_t(-128), int8_t(-128)); wide.emplace_back(int8_t(-128), int8_t(0));
            argumentCase(wide);
            argumentCase(randomPoints<lng>(rng, size(rng), 1000000000)); argumentCase(randomPoints<lng>(rng, size(rng), 1000000000000000000));
            vector<Point2<lng>> ray; Point2<lng> d{base(rng), base(rng)};
            for (int k = size(rng); k--;) { ray.push_back(d * factor(rng)); }
            ray.push_back(perp(d)); ray.push_back(-perp(d)); argumentCase(ray);}
        std::cout << "PASS argumentSort against an exact class/cross key, hand-ordered sample, duplicates, collinear rays and int8..int64 extremes\n";}

    // Exact direction class: the primitive vector through gcd, the zero vector apart; distinct classes with |coordinate| <= 10^4 differ in angle by over 10^-9.
    template<typename T> Point2<lll> directionClass(Point2<T> p) {
        lll x = p.x, y = p.y, g = gcd(x < 0 ? -x : x, y < 0 ? -y : y);
        return g ? Point2<lll>{x / g, y / g} : Point2<lll>{};}
    lng sameDirection = 0, lengthInversions = 0; string firstInversion;
    template<typename T> void approxCase(const vector<Point2<T>> &p) {
        context = "argumentSortApprox n=" + show(int(p.size())) + (p.empty() ? string() : " first=" + show(p[0].template cast<lll>()));
        auto got = argumentSortApprox(p), want = argumentSort(p), sorted = p, permuted = got;
        std::sort(sorted.begin(), sorted.end()); std::sort(permuted.begin(), permuted.end());
        checkEqual(permuted == sorted, true, "argumentSortApprox output is a permutation of the input");
        vector<Point2<lll>> classes;
        for (auto q : got) { classes.push_back(directionClass(q)); }
        for (int i = 0; i < int(got.size()); ++i) {
            checkEqual(classes[i], directionClass(want[i]), "exact direction class against argumentSort position=" + show(i));
            if (i && classes[i] == classes[i - 1]) {
                ++sameDirection;
                if (referenceNorm(got[i]) < referenceNorm(got[i - 1]) && !lengthInversions++) { firstInversion = context + " position=" + show(i); }}}}
    void approxOrder() {
        vector<Point2<lng>> lattice;
        for (lng x = -3; x <= 3; ++x) { for (lng y = -3; y <= 3; ++y) { lattice.emplace_back(x, y); }}
        std::mt19937_64 rng(seed ^ 0xa99);
        auto doubled = lattice; doubled.insert(doubled.end(), lattice.begin(), lattice.end()); std::shuffle(doubled.begin(), doubled.end(), rng);
        approxCase(lattice); approxCase(doubled); approxCase(vector<Point2<lng>>{}); approxCase(vector<Point2<lng>>{{0, 0}});
        approxCase(vector<Point2<int>>{{0, 0}, {0, 0}, {-1, 0}, {1, 0}, {0, -1}});
        vector<Point2<int>> sample{{1, 0}, {0, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {2, 2}, {-1, -1}, {2, 0}, {1, -1}};
        vector<Point2<int>> order{{-1, -1}, {0, -1}, {1, -1}, {0, 0}, {1, 0}, {2, 0}, {1, 1}, {2, 2}, {0, 1}, {-1, 0}};
        context = "argumentSortApprox hand-ordered sample";
        checkEqual(argumentSortApprox(sample) == order, true, "Library Checker argument order with zero, ties and the angle pi last");
        vector<Point2<int8_t>> axis{{2, 0}, {0, 0}, {-128, 0}, {1, 0}, {-1, 0}, {0, 0}, {0, -128}, {-128, -128}};
        vector<Point2<int8_t>> axisOrder{{-128, -128}, {0, -128}, {0, 0}, {0, 0}, {1, 0}, {2, 0}, {-1, 0}, {-128, 0}};
        checkEqual(argumentSortApprox(axis) == axisOrder, true, "int8_t axes and extremes by angle, then length");
        // Every primitive direction d up to the bound with all its multiples k * d inside |coordinate| <= 10^4: the answer is d, 2 * d, ..., m * d.
        int bound = mode == "quick" ? 6 : mode == "full" ? 30 : 80;
        for (lng x = -bound; x <= bound; ++x) {
            for (lng y = -bound; y <= bound; ++y) {
                if (gcd(x, y) != 1) { continue; }
                vector<Point2<lng>> ray; lng m = 10000 / std::max(abs(x), abs(y));
                for (lng k = 1; k <= m; ++k) { ray.emplace_back(k * x, k * y); }
                std::shuffle(ray.begin(), ray.end(), rng);
                context = "argumentSortApprox ray d=" + show(Point2<lll>{x, y}) + " multiples=" + show(m);
                auto got = argumentSortApprox(ray); vector<bool> seen(m + 1);
                checkEqual(lng(got.size()), m, "ray size");
                for (int i = 0; i < int(got.size()); ++i) {
                    lng k = x ? got[i].x / x : got[i].y / y;
                    checkEqual(k >= 1 && k <= m && got[i] == Point2<lng>{k * x, k * y} && !seen[k], true, "ray output is a permutation of the multiples");
                    seen[k] = true;
                    if (i) {
                        ++sameDirection;
                        if (referenceNorm(got[i]) < referenceNorm(got[i - 1]) && !lengthInversions++) { firstInversion = context + " position=" + show(i); }}}}}
        int count = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
        std::uniform_int_distribution<int> size(0, 24); std::uniform_int_distribution<lng> base(-100, 100);
        for (int i = 0; i < count; ++i) {
            approxCase(randomPoints<int8_t>(rng, size(rng), 3)); approxCase(randomPoints<int16_t>(rng, size(rng), 10000));
            approxCase(randomPoints<int>(rng, size(rng), 10000)); approxCase(randomPoints<lng>(rng, size(rng), 10000));
            auto bytes = randomPoints<int8_t>(rng, size(rng), 127); bytes.emplace_back(int8_t(-128), int8_t(-128)); bytes.emplace_back(int8_t(-128), int8_t(0));
            approxCase(bytes);
            vector<Point2<lng>> ray; Point2<lng> d{base(rng), base(rng)};
            std::uniform_int_distribution<lng> factor(-10000 / std::max({lng(1), abs(d.x), abs(d.y)}), 10000 / std::max({lng(1), abs(d.x), abs(d.y)}));
            for (int k = size(rng); k--;) { ray.push_back(d * factor(rng)); }
            ray.push_back(perp(d)); ray.push_back(-perp(d)); approxCase(ray);
            auto large = randomPoints<lng>(rng, size(rng), 1000000000000000000);
            auto got = argumentSortApprox(large);
            std::sort(got.begin(), got.end()); std::sort(large.begin(), large.end());
            context = "argumentSortApprox |coordinate| <= 10^18"; checkEqual(got == large, true, "argumentSortApprox permutation at 10^18");}
        context = firstInversion;
        checkEqual(lengthInversions, lng(0), "same-direction vectors out of length order (empirical: equal rounded keys on this toolchain)");
        std::cout << "PASS argumentSortApprox against exact argumentSort direction classes for |coordinate| <= 10^4, full rays, ties, permutation at 10^18; same-direction pairs="
                  << sameDirection << " length-inversions=" << lengthInversions << '\n';}

    // Exact total-angle model: (turns, quadrant split at the axes, direction), written apart from Angle::half and Angle::operator<.
    struct Key { lng t; int q; lll x, y; };
    template<typename T> Key keyOf(Point2<T> v, lng t) {
        lll x = v.x, y = v.y;
        return {t, x > 0 && y >= 0 ? 0 : x <= 0 && y > 0 ? 1 : x < 0 && y <= 0 ? 2 : 3, x, y};}
    template<typename T> Key keyOf(Angle<T> a) { return keyOf(a.v, a.t); }
    bool keyLess(Key a, Key b) {
        if (a.t != b.t) { return a.t < b.t; }
        return a.q != b.q ? a.q < b.q : a.x * b.y - a.y * b.x > 0;}
    bool keySame(Key a, Key b) { return !keyLess(a, b) && !keyLess(b, a); }
    Key keyHalf(Key a) { return {a.t + (a.q >= 2), (a.q + 2) % 4, -a.x, -a.y}; }
    long double keyTotal(Key a) {
        long double pi = std::acos(-1.L), v = std::atan2(static_cast<long double>(a.y), static_cast<long double>(a.x));
        return (v < 0 ? v + 2 * pi : v) + 2 * pi * static_cast<long double>(a.t);}

    template<typename T> void angleSelfCase(Angle<T> a) {
        context = "Angle a=" + show(a);
        Key x = keyOf(a);
        auto r = a.t90(), h = a.t180(), f = a.t360();
        checkEqual(int(a.half()), int(x.q >= 2), "half against quadrant");
        checkEqual(r.v, perp(a.v), "t90 direction"); checkEqual(r.t, a.t + (x.q == 3), "t90 turn carry");
        checkEqual(keyOf(r).q, (x.q + 1) % 4, "t90 next quadrant");
        checkEqual(h.v, -a.v, "t180 direction"); checkEqual(h.t, keyHalf(x).t, "t180 turn carry");
        checkEqual(f.v, a.v, "t360 direction"); checkEqual(f.t, a.t + 1, "t360 turn");
        auto r4 = r.t90().t90().t90(), h2 = h.t180(), r2 = r.t90();
        checkEqual(r4.v == f.v && r4.t == f.t, true, "t90 four times is t360");
        checkEqual(h2.v == f.v && h2.t == f.t, true, "t180 twice is t360");
        checkEqual(r2.v == h.v && r2.t == h.t, true, "t90 twice is t180");
        checkEqual(a < r && r < h && h < f && !(f < h) && !(h < r) && !(r < a), true, "quarter turns increase");
        checkEqual(a < a, false, "Angle irreflexivity");}
    template<typename T> void angleOrderCase(Angle<T> a, Angle<T> b) {
        context = "Angle a=" + show(a) + " b=" + show(b);
        Key x = keyOf(a), y = keyOf(b);
        checkEqual(a < b, keyLess(x, y), "Angle order against exact quadrant model");
        if (a.t != b.t) { return; }
        auto [lo, hi] = segmentAngles(a, b);
        Key l = keyOf(lo), h = keyOf(hi);
        lll c = lll(a.v.x) * b.v.y - lll(a.v.y) * b.v.x, d = lll(a.v.x) * b.v.x + lll(a.v.y) * b.v.y;
        checkEqual((lo.v == a.v && hi.v == b.v) || (lo.v == b.v && hi.v == a.v), true, "segmentAngles endpoints");
        checkEqual(lo.t, a.t, "segmentAngles lower turn");
        checkEqual(hi.t == a.t || hi.t == a.t + 1, true, "segmentAngles upper turn");
        checkEqual(keyLess(h, l) || hi < lo, false, "segmentAngles lo <= hi");
        checkEqual(keyLess(keyHalf(l), h), false, "segmentAngles arc at most pi");
        checkEqual(keySame(keyHalf(l), h), c == 0 && d < 0, "segmentAngles arc is pi iff the origin is on the segment");
        for (int k = 0; k <= 8; ++k) {
            Point2<lll> s{lll(8 - k) * a.v.x + lll(k) * b.v.x, lll(8 - k) * a.v.y + lll(k) * b.v.y};
            if (s == Point2<lll>{}) { continue; }
            bool inside = false;
            for (lng m : {lo.t, lo.t + 1}) { Key z = keyOf(s, m); inside |= !keyLess(z, l) && !keyLess(h, z); }
            checkEqual(inside, true, "segmentAngles arc covers sampled point k/8=" + show(k));}}
    // Needs coordinates below 2^31 in magnitude, so the complex rotation below fits lll.
    template<typename T> void angleDiffCase(Angle<T> a, Angle<T> b) {
        context = "angleDiff a=" + show(a) + " b=" + show(b);
        auto d = angleDiff(a, b);
        static_assert(std::is_same_v<decltype(d), Angle<lll>>);
        lll dx = lll(a.v.x) * b.v.x + lll(a.v.y) * b.v.y, dy = lll(a.v.x) * b.v.y - lll(a.v.y) * b.v.x, n = referenceNorm(a.v);
        checkEqual(d.v, Point2<lll>{dx, dy}, "angleDiff direction (dot, cross)");
        // a.v times d.v as complex numbers is norm2(a.v) * b.v, so d rotates a onto b; the turns carry when arg(b) < arg(a).
        checkEqual(Point2<lll>{a.v.x * dx - a.v.y * dy, a.v.x * dy + a.v.y * dx}, Point2<lll>{n * b.v.x, n * b.v.y}, "angleDiff rotates a onto b");
        lng carry = keyLess(keyOf(b.v, 0), keyOf(a.v, 0));
        checkEqual(b.t, a.t + d.t + carry, "angleDiff turns add back to b");
        checkEqual(d < d.t90() && !(d.t360() < d), true, "lll Angle order");
        if (abs(a.t) <= 4 && abs(b.t) <= 4) {
            closeWithin(keyTotal(keyOf(d)), keyTotal(keyOf(b)) - keyTotal(keyOf(a)), 1e-15L, "angleDiff total angle is total(b) - total(a)");}}
    // Precondition: the segment from a.v to a.v + b avoids the origin.
    template<typename T> Angle<T> stepCase(Angle<T> a, Point2<T> b) {
        context = "Angle step a=" + show(a) + " b=" + show(b.template cast<lll>());
        auto r = a + b, s = a - -b;
        Point2<lll> p = a.v.template cast<lll>(), q = p + b.template cast<lll>();
        Key x = keyOf(a), y = keyOf(r);
        lll c = p.x * q.y - p.y * q.x;
        checkEqual(r.v.template cast<lll>(), q, "operator+ endpoint");
        checkEqual(s.v == r.v && s.t == r.t, true, "operator- is operator+ of the negation");
        bool moved = c > 0 ? keyLess(x, y) && keyLess(y, keyHalf(x)) : c < 0 ? keyLess(y, x) && keyLess(x, keyHalf(y)) : keySame(x, y);
        checkEqual(moved, true, "operator+ moves the total angle by the swept angle below pi");
        return r;}
    template<typename T> bool avoidsOrigin(const vector<Point2<T>> &p) {
        for (int i = 0; i < int(p.size()); ++i) {
            auto a = p[i], b = p[(i + 1) % int(p.size())];
            lll c = lll(a.x) * b.y - lll(a.y) * b.x, d = lll(a.x) * b.x + lll(a.y) * b.y;
            if (a == Point2<T>{} || (c == 0 && d <= 0)) { return false; }}
        return true;}
    // Crossing-number winding oracle: upward edges with the origin on their left count +1, downward edges with it on their right -1.
    template<typename T> lng windingNumber(const vector<Point2<T>> &p) {
        lng w = 0;
        for (int i = 0; i < int(p.size()); ++i) {
            auto a = p[i], b = p[(i + 1) % int(p.size())];
            lll c = lll(a.x) * b.y - lll(a.y) * b.x;
            if (a.y <= 0 && b.y > 0 && c > 0) { ++w; }
            else if (a.y > 0 && b.y <= 0 && c < 0) { --w; }}
        return w;}
    template<typename T> void walkCase(const vector<Point2<T>> &p, lng t0) {
        int n = int(p.size());
        Angle<T> a(p[0], t0);
        for (int i = 1; i <= n; ++i) { a = stepCase(a, p[i % n] - p[i - 1]); }
        context = "winding walk n=" + show(n) + " first=" + show(p[0].template cast<lll>()) + " t0=" + show(lll(t0));
        checkEqual(a.v, p[0], "walk returns to its start");
        checkEqual(a.t - t0, windingNumber(p), "turn count equals the crossing-number winding");}
    template<typename T> void windings(lng bound, int count, std::mt19937_64 &rng) {
        std::uniform_int_distribution<lng> coordinate(-bound, bound), loops(-3, 3), turns(-1000000000000, 1000000000000);
        std::uniform_int_distribution<int> size(3, 10); std::uniform_real_distribution<long double> unit(0, 1);
        long double pi = std::acos(-1.L); int wound = 0;
        for (int i = 0; i < count; ++i) {
            vector<Point2<T>> p;
            if (i % 3 == 0) { p = randomPoints<T>(rng, size(rng), bound); }
            else {
                // Jittered spiral: n >= 4 * |w| + 3 vertices, so consecutive arguments differ by less than pi.
                lng w = loops(rng); int n = int(4 * abs(w)) + size(rng);
                for (int k = 0; k < n; ++k) {
                    long double theta = 2 * pi * static_cast<long double>(w * k) / n + (unit(rng) - 0.5L) * pi / 8, r = static_cast<long double>(bound) * (0.25L + 0.75L * unit(rng));
                    p.emplace_back(T(std::llround(r * std::cos(theta))), T(std::llround(r * std::sin(theta))));}}
            if (!avoidsOrigin(p)) { continue; }
            wound += windingNumber(p) != 0;
            walkCase(p, i % 2 ? 0 : turns(rng));}
        context = "winding corpus bytes=" + show(int(sizeof(T)));
        checkEqual(wound > 0, true, "corpus contains polygons around the origin");}
    template<typename T> void angleLattice(int bound) {
        vector<Angle<T>> p;
        for (lng t = -1; t <= 1; ++t) {
            for (int x = -bound; x <= bound; ++x) { for (int y = -bound; y <= bound; ++y) { if (x || y) { p.emplace_back(Point2<T>{T(x), T(y)}, t); }}}}
        for (auto a : p) {
            angleSelfCase(a);
            Angle<T> twice(a.v * T(2), a.t);
            checkEqual(a < twice || twice < a, false, "equal angles of different lengths are equivalent");
            for (int x = -bound; x <= bound; ++x) {
                for (int y = -bound; y <= bound; ++y) {
                    Point2<T> b{T(x), T(y)};
                    if (avoidsOrigin(vector<Point2<T>>{a.v, a.v + b})) { stepCase(a, b); }}}
            for (auto b : p) {
                angleOrderCase(a, b); angleDiffCase(a, b);
                Key x = keyOf(a), y = keyOf(b);
                context = "angle model a=" + show(a) + " b=" + show(b);
                bool same = a.t == b.t && cross(a.v, b.v) == 0 && dot(a.v, b.v) > 0;
                checkEqual(keySame(x, y), same, "model equivalence is equal turn and direction");
                if (!same) { checkEqual(keyLess(x, y), keyTotal(x) < keyTotal(y), "exact model against atan2 total angle"); }}}}
    void turnAngles() {
        int bound = mode == "quick" ? 2 : 3;
        angleLattice<int8_t>(bound); angleLattice<int16_t>(bound); angleLattice<int>(bound); angleLattice<lng>(bound);
        context = "winding oracle fixed squares";
        vector<Point2<int>> square{{-1, -1}, {1, -1}, {1, 1}, {-1, 1}}, clockwise(square.rbegin(), square.rend()), outside{{1, 1}, {3, 1}, {3, 3}, {1, 3}}, twice = square;
        twice.insert(twice.end(), square.begin(), square.end());
        checkEqual(windingNumber(square), lng(1), "counterclockwise square"); checkEqual(windingNumber(clockwise), lng(-1), "clockwise square");
        checkEqual(windingNumber(outside), lng(0), "square beside the origin"); checkEqual(windingNumber(twice), lng(2), "square traced twice");
        walkCase(square, 0); walkCase(clockwise, -5); walkCase(outside, 7); walkCase(twice, -1);
        std::mt19937_64 rng(seed ^ 0xa9e);
        std::uniform_int_distribution<lng> nine(-1000000000, 1000000000), eighteen(-1000000000000000000, 1000000000000000000), turns(-1000000, 1000000), tiny(-2, 2);
        std::uniform_int_distribution<int> full(-INT_MAX, INT_MAX);
        auto vector9 = [&]() { Point2<lng> v; while (v == Point2<lng>{}) { v = {nine(rng), nine(rng)}; } return v; };
        int count = mode == "quick" ? 200 : mode == "full" ? 5000 : 30000;
        for (int i = 0; i < count; ++i) {
            Angle<lng> a(vector9(), turns(rng)), b(vector9(), i % 2 ? a.t : turns(rng));
            angleSelfCase(a); angleOrderCase(a, b); angleDiffCase(a, b);
            Point2<lng> nudge{tiny(rng), tiny(rng)};
            if (a.v + nudge != Point2<lng>{}) { Angle<lng> c(a.v + nudge, a.t); angleOrderCase(a, c); angleOrderCase(c, a); angleDiffCase(a, c); }
            Angle<lng> o(-a.v, a.t); angleOrderCase(a, o); angleDiffCase(a, o);
            Angle<lng> e(Point2<lng>{eighteen(rng), eighteen(rng)} + Point2<lng>{1, 0}, turns(rng)), f(Point2<lng>{eighteen(rng), eighteen(rng)} + Point2<lng>{0, 1}, e.t);
            angleSelfCase(e); angleOrderCase(e, f); angleOrderCase(e, Angle<lng>(-e.v, e.t));
            Angle<int> g(Point2<int>{full(rng), full(rng) | 1}, turns(rng)), h(Point2<int>{full(rng) | 1, full(rng)}, i % 2 ? g.t : turns(rng));
            angleSelfCase(g); angleOrderCase(g, h); angleDiffCase(g, h);
            Point2<lng> step = vector9();
            if (avoidsOrigin(vector<Point2<lng>>{a.v, a.v + step})) { stepCase(a, step); }}
        int polygons = mode == "quick" ? 60 : mode == "full" ? 1500 : 8000;
        windings<int8_t>(60, polygons, rng); windings<int16_t>(16000, polygons, rng); windings<int>(1000000000, polygons, rng);
        windings<lng>(1000000000, polygons, rng); windings<lng>(1000000000000000000, polygons, rng);
        context = "Angle fixed values";
        Angle<lng> x(Point2<lng>{1, 0}), y(Point2<lng>{0, -1}, -3);
        checkEqual(x.t, lng(0), "default turn count");
        auto z = x + Point2<lng>{0, -1};
        checkEqual(z.v == Point2<lng>{1, -1} && z.t == -1, true, "step below the positive x-axis drops a turn");
        auto w = y.t90();
        checkEqual(w.v == Point2<lng>{1, 0} && w.t == -2, true, "quarter turn from -y wraps to the next turn");
        auto [lo, hi] = segmentAngles(Angle<lng>(Point2<lng>{1, -1}), Angle<lng>(Point2<lng>{1, 1}));
        checkEqual(lo.v == Point2<lng>{1, -1} && lo.t == 0 && hi.v == Point2<lng>{1, 1} && hi.t == 1, true, "arc across the positive x-axis");
        auto u = angleDiff(Angle<lng>(Point2<lng>{0, 1}, 2), Angle<lng>(Point2<lng>{1, 0}, -1));
        checkEqual(u.v == Point2<lll>{0, -1} && u.t == -4, true, "negative angleDiff");
        Point2<lng> near{1000000000, 999999999}, nearer{999999999, 999999998};
        checkEqual(Angle<lng>(nearer) < Angle<lng>(near) && !(Angle<lng>(near) < Angle<lng>(nearer)), true, "near-parallel 1e9 order");
        std::cout << "PASS Angle half/t90/t180/t360/order/angleDiff/segmentAngles/operator+- against an exact quadrant model, atan2 and winding walks\n";}

    long double eps64() { return 64 * std::numeric_limits<long double>::epsilon(); }
    template<typename T> void projectionCase(Point2<T> a, Point2<T> b) {
        context = "projection a=" + show(a.template cast<long double>()) + " b=" + show(b.template cast<long double>());
        using E = std::conditional_t<std::is_integral_v<T>, lll, long double>;
        long double eps = eps64(), na = std::hypot((long double)a.x, (long double)a.y), nb = std::hypot((long double)b.x, (long double)b.y);
        closeWithin(scalarProjectionApprox(a, b), static_cast<long double>(E(a.x) * b.x + E(a.y) * b.y) / nb, eps * std::max(1.L, na), "2D scalar projection");
        closeWithin(scalarProjectionApprox(b, b), nb, eps * std::max(1.L, nb), "2D projection onto itself is the norm");
        closeWithin(scalarProjectionApprox(perp(b), b), 0, eps * std::max(1.L, nb), "2D projection of the perpendicular");
        checkEqual(scalarProjectionApprox(a, -b), -scalarProjectionApprox(a, b), "2D projection onto -b negates");
        auto n = normalApprox(b);
        static_assert(std::is_same_v<decltype(n), dpoint>);
        long double o = (long double)b.x * n.x + (long double)b.y * n.y, s = (long double)b.x * n.y - (long double)b.y * n.x;
        closeWithin(n.x, -(long double)b.y / nb, eps, "unit left normal x"); closeWithin(n.y, (long double)b.x / nb, eps, "unit left normal y");
        closeWithin(n.x * n.x + n.y * n.y, 1, eps, "2D normal unit length");
        closeWithin(o, 0, eps * std::max(1.L, nb), "2D normal orthogonal");
        checkEqual(s > 0, true, "2D normal on the left: cross(a, n) > 0"); closeWithin(s, nb, eps * std::max(1.L, nb), "cross with the normal is the norm");}
    template<typename T> void projection3Case(Point3<T> a, Point3<T> b) {
        context = "projection3 a=" + show(a.template cast<long double>()) + " b=" + show(b.template cast<long double>());
        using E = std::conditional_t<std::is_integral_v<T>, lll, long double>;
        long double eps = eps64(), na = std::hypot((long double)a.x, (long double)a.y, (long double)a.z), nb = std::hypot((long double)b.x, (long double)b.y, (long double)b.z);
        if (b != Point3<T>{}) {
            closeWithin(scalarProjectionApprox(a, b), static_cast<long double>(E(a.x) * b.x + E(a.y) * b.y + E(a.z) * b.z) / nb, eps * std::max(1.L, na), "3D scalar projection");
            closeWithin(scalarProjectionApprox(b, b), nb, eps * std::max(1.L, nb), "3D projection onto itself is the norm");}
        Point3<E> c{E(a.y) * b.z - E(a.z) * b.y, E(a.z) * b.x - E(a.x) * b.z, E(a.x) * b.y - E(a.y) * b.x};
        if (c == Point3<E>{}) { return; }
        long double cx = static_cast<long double>(c.x), cy = static_cast<long double>(c.y), cz = static_cast<long double>(c.z), nc = std::hypot(cx, cy, cz);
        // A floating cross product is rounded, so its direction error grows with |a| * |b| / |a x b|.
        long double kappa = std::is_integral_v<T> ? 1.L : std::max(1.L, na * nb / nc);
        auto n = normalApprox(a, b);
        static_assert(std::is_same_v<decltype(n), Point3<long double>>);
        closeWithin(n.x, cx / nc, eps, "3D normal x"); closeWithin(n.y, cy / nc, eps, "3D normal y"); closeWithin(n.z, cz / nc, eps, "3D normal z");
        closeWithin(n.x * n.x + n.y * n.y + n.z * n.z, 1, eps, "3D normal unit length");
        closeWithin((long double)a.x * n.x + (long double)a.y * n.y + (long double)a.z * n.z, 0, eps * kappa * std::max(1.L, na), "3D normal orthogonal to a");
        closeWithin((long double)b.x * n.x + (long double)b.y * n.y + (long double)b.z * n.z, 0, eps * kappa * std::max(1.L, nb), "3D normal orthogonal to b");
        long double s = cx * n.x + cy * n.y + cz * n.z;
        long double bx = (long double)b.y * n.z - (long double)b.z * n.y, by = (long double)b.z * n.x - (long double)b.x * n.z, bz = (long double)b.x * n.y - (long double)b.y * n.x;
        checkEqual(s > 0, true, "3D normal orientation: (a x b) . n > 0"); closeWithin(s, nc, eps * nc, "(a x b) . n is |a x b|");
        closeWithin((long double)a.x * bx + (long double)a.y * by + (long double)a.z * bz, nc, eps * std::max(1.L, na * nb), "triple(a, b, n) is |a x b|");}
    void projections() {
        int bound = mode == "quick" ? 2 : 3;
        for (int x = -bound; x <= bound; ++x) { for (int y = -bound; y <= bound; ++y) { for (int u = -bound; u <= bound; ++u) { for (int v = -bound; v <= bound; ++v) {
            if (u || v) { projectionCase(Point2<int>{x, y}, Point2<int>{u, v}); projectionCase(Point2<int8_t>{int8_t(x), int8_t(y)}, Point2<int8_t>{int8_t(u), int8_t(v)}); }}}}}
        vector<Point3<int>> q;
        for (int x = -1; x <= 1; ++x) { for (int y = -1; y <= 1; ++y) { for (int z = -1; z <= 1; ++z) { q.emplace_back(x, y, z); }}}
        for (auto a : q) { for (auto b : q) { projection3Case(a, b); }}
        std::mt19937_64 rng(seed ^ 0x9e0); std::uniform_int_distribution<lng> nine(-1000000000, 1000000000), tiny(-2, 2), small(-100, 100);
        std::uniform_real_distribution<double> real(-1000, 1000);
        int count = mode == "quick" ? 200 : mode == "full" ? 5000 : 30000;
        for (int i = 0; i < count; ++i) {
            Point2<lng> a{nine(rng), nine(rng)}, b{nine(rng), nine(rng) | 1};
            projectionCase(a, b); projectionCase(Point2<int16_t>{int16_t(small(rng)), int16_t(small(rng))}, Point2<int16_t>{int16_t(small(rng)), int16_t(small(rng) | 1)});
            Point3<lng> c{nine(rng), nine(rng), nine(rng)}, d{nine(rng), nine(rng), nine(rng)};
            projection3Case(c, d); projection3Case(c, c * 2 + Point3<lng>{tiny(rng), tiny(rng), tiny(rng)});
            projection3Case(Point3<int8_t>{int8_t(small(rng)), int8_t(small(rng)), int8_t(small(rng))}, Point3<int8_t>{int8_t(small(rng)), int8_t(small(rng)), int8_t(small(rng))});
            Point2<double> e{real(rng), real(rng)}, f{real(rng), real(rng)};
            if (f != Point2<double>{}) { projectionCase(e, f); projectionCase(e.cast<float>(), f.cast<float>()); projectionCase(e.cast<long double>(), f.cast<long double>()); }
            Point3<double> g{real(rng), real(rng), real(rng)}, h{real(rng), real(rng), real(rng)};
            projection3Case(g, h); projection3Case(g.cast<float>(), h.cast<float>()); projection3Case(g.cast<long double>(), h.cast<long double>());}
        Point3<lng> big{1000000000, 999999999, 999999998}, nearby{999999999, 999999998, 999999998};
        projection3Case(big, nearby); projection3Case(Point3<lng>{1000000000, -1000000000, 1000000000}, Point3<lng>{-1000000000, -1000000000, 1000000000});
        context = "projection and normal fixed values";
        checkEqual(scalarProjectionApprox(Point2<int>{3, 4}, Point2<int>{5, 0}), 3.L, "2D projection onto the x-axis");
        checkEqual(scalarProjectionApprox(Point3<int>{1, 2, 3}, Point3<int>{0, 0, 2}), 3.L, "3D projection onto the z-axis");
        checkEqual(scalarProjectionApprox(Point2<int>{3, 4}, Point2<int>{-1, 0}), -3.L, "negative projection");
        auto n = normalApprox(Point2<int>{3, 4}); close(n.x, -0.8L, "3-4-5 normal x"); close(n.y, 0.6L, "3-4-5 normal y");
        checkEqual(normalApprox(Point3<int>{1, 0, 0}, Point3<int>{0, 1, 0}), Point3<long double>{0, 0, 1}, "x cross y normal");
        checkEqual(normalApprox(Point3<int>{0, 1, 0}, Point3<int>{1, 0, 0}), Point3<long double>{0, 0, -1}, "y cross x normal");
        std::cout << "PASS scalarProjectionApprox and normalApprox 2D/3D against hypot oracles, unit/orthogonality/orientation identities and 1e9 coordinates\n";}

    template<typename P> concept Writable = requires(ostream &os, P p) { os << p; };
    template<typename P> concept Readable = requires(istream &is, P &p) { is >> p; };
    template<typename T> string text(Point2<T> p) { return std::to_string(+p.x) + ' ' + std::to_string(+p.y); }
    template<typename T> string text(Point3<T> p) { return std::to_string(+p.x) + ' ' + std::to_string(+p.y) + ' ' + std::to_string(+p.z); }
    template<typename P> void streamCase(P p) {
        string want = text(p); context = "stream text='" + want + "'";
        std::ostringstream out; out << p; checkEqual(out.str(), want, "operator<< numeric text");
        std::istringstream in(want); P q; in >> q;
        checkEqual(!in.fail(), true, "operator>> succeeds"); checkEqual(q.template cast<lll>(), p.template cast<lll>(), "operator>> round trip");}
    template<typename P> void failedRead(const string &input, P prior) {
        context = "failed extraction text='" + input + "'";
        std::istringstream in(input); P p = prior; in >> p;
        checkEqual(in.fail(), true, "failed extraction sets failbit");
        checkEqual(p == prior, true, "failed extraction leaves the point unchanged");}
    // 21 = max_digits10 of the 64-bit-mantissa long double, so the text names one value; snprintf is the independent formatter.
    void longDoubleRoundTrip(dpoint p) {
        char buffer[128]; std::snprintf(buffer, sizeof buffer, "%.21Lg %.21Lg", p.x, p.y);
        context = "long double stream text='" + string(buffer) + "'";
        std::ostringstream out; out << std::setprecision(21) << p; checkEqual(out.str(), string(buffer), "operator<< at precision 21");
        std::istringstream in(out.str()); dpoint q{7, 7}; in >> q;
        checkEqual(!in.fail(), true, "long double operator>> succeeds");
        checkEqual(q.x == p.x && q.y == p.y && std::signbit(q.x) == std::signbit(p.x) && std::signbit(q.y) == std::signbit(p.y), true, "21-digit long double round trip");}
    void streams() {
        streamCase(Point2<lng>{std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()}); streamCase(Point2<lng>{0, -1});
        streamCase(Point2<int>{INT_MIN, INT_MAX}); streamCase(Point2<int8_t>{-128, 127}); streamCase(Point2<int8_t>{100, -5}); streamCase(Point2<int8_t>{48, 65});
        streamCase(Point2<int16_t>{-32768, 32767}); streamCase(Point3<lng>{-1, 0, std::numeric_limits<lng>::max()}); streamCase(Point3<int8_t>{48, -1, 65});
        std::mt19937_64 rng(seed ^ 0x5e1); std::uniform_int_distribution<lng> any(std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max());
        std::uniform_int_distribution<int> byte(-128, 127), exponent(-16300, 16300); std::uniform_real_distribution<long double> mantissa(-1, 1);
        int count = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
        for (int i = 0; i < count; ++i) {
            streamCase(Point2<lng>{any(rng), any(rng)}); streamCase(Point2<int>{int(any(rng) >> 32), int(any(rng) >> 32)});
            streamCase(Point2<int8_t>{int8_t(byte(rng)), int8_t(byte(rng))}); streamCase(Point3<lng>{any(rng), any(rng), any(rng)});
            longDoubleRoundTrip(dpoint{std::ldexp(mantissa(rng), exponent(rng)), std::ldexp(mantissa(rng), exponent(rng))});}
        using limits = std::numeric_limits<long double>;
        static_assert(limits::digits == 64 && limits::max_digits10 == 21);
        for (long double v : {0.L, -0.L, limits::denorm_min(), limits::min(), limits::max(), limits::lowest(), 1.L / 3, -0.1L}) { longDoubleRoundTrip(dpoint{v, -v}); }
        context = "fixed stream values";
        auto written = [](auto p) { std::ostringstream out; out << p; return out.str(); };
        checkEqual(written(dpoint{1.5L, -2.25L}), string("1.5 -2.25"), "default long double text");
        checkEqual(written(Point3<float>{0.5f, -0.f, 8}), string("0.5 -0 8"), "float text with negative zero");
        checkEqual(written(Point2<double>{1. / 3, 1e100}), string("0.333333 1e+100"), "default six significant digits");
        auto read = [](const string &input, auto p) { std::istringstream in(input); in >> p; return pair{p, bool(in)}; };
        checkEqual(read(" \t-3\n\n 7 ", Point2<lng>{}) == pair{Point2<lng>{-3, 7}, true}, true, "whitespace and negative input");
        checkEqual(read("+4 -0", Point2<lng>{1, 1}) == pair{Point2<lng>{4, 0}, true}, true, "explicit signs");
        checkEqual(read("100 -5", Point2<int8_t>{}) == pair{Point2<int8_t>{100, -5}, true}, true, "int8_t reads numbers, not characters");
        checkEqual(read("-128\t127", Point2<int8_t>{}) == pair{Point2<int8_t>{-128, 127}, true}, true, "int8_t extremes input");
        checkEqual(read("1\n2\t3", Point3<int>{}) == pair{Point3<int>{1, 2, 3}, true}, true, "3D whitespace input");
        checkEqual(read("1e3 -.5", dpoint{}) == pair{dpoint{1000, -0.5L}, true}, true, "floating exponent and leading-point input");
        auto zero = read("-0 0", dpoint{1, 1}).first;
        checkEqual(zero == dpoint{} && std::signbit(zero.x) && !std::signbit(zero.y), true, "negative zero input");
        std::istringstream sequence("1 2 3 4 5 6 7"); Point2<lng> a, c; Point3<lng> b; sequence >> a >> b >> c;
        checkEqual(a == Point2<lng>{1, 2} && b == Point3<lng>{3, 4, 5} && c == Point2<lng>{6, 7} && bool(sequence), true, "consecutive extractions from one stream");
        sequence >> c; checkEqual(sequence.fail() && c == Point2<lng>{6, 7}, true, "extraction at end of stream leaves the point unchanged");
        for (string input : {"", "x", "5", "5 x", "- 3 4", "99999999999999999999 1", "1 99999999999999999999"}) { failedRead(input, Point2<lng>{7, -7}); }
        failedRead("5 x", Point2<int8_t>{7, -7}); failedRead("x 5", Point2<int8_t>{7, -7});
        for (string input : {"200 -3", "-129 3", "3 128", "3 -129"}) { failedRead(input, Point2<int8_t>{7, -7}); }
        for (string input : {"200 0 0", "-200 0 0", "0 128 0", "0 -129 0", "1 2 128", "1 2 -129"}) { failedRead(input, Point3<int8_t>{1, 2, 3}); }
        failedRead("4 5", Point3<int>{1, 2, 3}); failedRead("4 5 y", Point3<int>{1, 2, 3}); failedRead("4 x 6", Point3<int>{1, 2, 3});
        failedRead("1.5 x", dpoint{0.25L, -0.75L}); failedRead("1.5 2", Point3<long double>{1, 2, 3});
        std::istringstream failed("1 2"); failed.setstate(std::ios::failbit); Point2<lng> kept{7, -7}; failed >> kept;
        context = "stream already failed";
        checkEqual(kept == Point2<lng>{7, -7}, true, "extraction from a failed stream leaves the point unchanged");
        std::cout << "PASS operator<< and operator>> against std::to_string/snprintf text, int8_t as numbers, round trips, whitespace, signed zero and failed reads leaving the point unchanged\n";}

    template<typename T> void clockwiseCase(Point2<T> a) {
        context = "rotateCW a=" + show(a.template cast<long double>());
        auto r = rotateCW(a);
        checkEqual(r.x == a.y && r.y == T(-a.x), true, "rotateCW is (y, -x)");
        if constexpr (std::is_floating_point_v<T>) { checkEqual(std::signbit(r.x) == std::signbit(a.y) && std::signbit(r.y) != std::signbit(a.x), true, "rotateCW signed zeros"); }
        else {
            checkEqual(lll(cross(r, a)), referenceNorm(a), "rotateCW turns clockwise: cross(rotateCW(a), a) = |a|^2");
            checkEqual(lll(dot(r, a)), lll(0), "rotateCW orthogonal");}
        checkEqual(rotateCW(perp(a)) == a && perp(rotateCW(a)) == a, true, "rotateCW inverts perp");
        checkEqual(rotateCW(rotateCW(a)) == -a, true, "two clockwise rotations negate");
        checkEqual(rotateCW(rotateCW(rotateCW(rotateCW(a)))) == a, true, "four clockwise rotations");}
    void clockwise() {
        for (int x = -3; x <= 3; ++x) {
            for (int y = -3; y <= 3; ++y) {
                clockwiseCase(Point2<int8_t>{int8_t(x), int8_t(y)}); clockwiseCase(Point2<int>{x, y}); clockwiseCase(Point2<lng>{x, y});
                clockwiseCase(Point2<lll>{x, y}); clockwiseCase(Point2<double>{double(x), double(y)});}}
        for (double x : {0., -0., 1., -1.}) { for (double y : {0., -0., 1., -1.}) { clockwiseCase(Point2<double>{x, y}); clockwiseCase(Point2<long double>{x, y}); clockwiseCase(Point2<float>{float(x), float(y)}); }}
        std::mt19937_64 rng(seed ^ 0xc3); std::uniform_int_distribution<lng> big(-1000000000000000000, 1000000000000000000), wide(-(lng(1) << 62), lng(1) << 62);
        std::uniform_int_distribution<int> byte(-127, 127); std::uniform_real_distribution<double> real(-1000, 1000);
        int count = mode == "quick" ? 200 : mode == "full" ? 3000 : 20000;
        for (int i = 0; i < count; ++i) {
            clockwiseCase(Point2<lng>{big(rng), big(rng)}); clockwiseCase(Point2<lll>{wide(rng), wide(rng)});
            clockwiseCase(Point2<int8_t>{int8_t(byte(rng)), int8_t(byte(rng))}); clockwiseCase(Point2<double>{real(rng), real(rng)});
            clockwiseCase(Point2<long double>{real(rng), real(rng)}); clockwiseCase(Point2<float>{float(real(rng)), float(real(rng))});}
        context = "rotateCW fixed value";
        checkEqual(rotateCW(Point2<int>{6, -9}), Point2<int>{-9, -6}, "rotateCW of (6, -9)");
        std::cout << "PASS rotateCW against (y, -x), signed zeros, clockwise cross/dot identities, perp inverse and four-rotation identity\n";}
} // namespace

static_assert(std::is_same_v<point, Point2<lng>> && std::is_same_v<dpoint, Point2<long double>>);
static_assert(std::is_same_v<decltype(unitApprox(Point3<int>{})), Point3<long double>>);
static_assert(canonicalDirection(Point2<int8_t>{-4, 6}) == Point2<int8_t>{2, -3} * int8_t(-1));
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
static_assert(std::is_same_v<Angle<>, Angle<lng>> && !std::is_convertible_v<Point2<lng>, Angle<lng>>);
static_assert(std::is_same_v<decltype(angleDiff(Angle<int8_t>(Point2<int8_t>{1, 0}), Angle<int8_t>(Point2<int8_t>{0, 1}))), Angle<lll>>);
static_assert(std::is_same_v<decltype(segmentAngles(Angle<int>(Point2<int>{1, 0}), Angle<int>(Point2<int>{0, 1}))), pair<Angle<int>, Angle<int>>>);
static_assert(std::is_same_v<decltype(normalApprox(Point2<int>{})), dpoint> && std::is_same_v<decltype(normalApprox(Point3<float>{}, Point3<float>{})), Point3<long double>>);
static_assert(Angle<int>(Point2<int>{0, -1}).t90().t90().t90().t90().t == 1 && Angle<int>(Point2<int>{-1, 0}).t180().t180().t == 1);
static_assert(Angle<int>(Point2<int>{1, 0}) < Angle<int>(Point2<int>{0, 1}) && !(Angle<int>(Point2<int>{2, 0}) < Angle<int>(Point2<int>{1, 0})));
static_assert(tetraVolume6(Point3<int>{}, Point3<int>{1, 0, 0}, Point3<int>{0, 1, 0}, Point3<int>{0, 0, 1}) == 1);
static_assert(!Writable<Point2<lll>> && !Readable<Point2<lll>> && !Writable<Point3<lll>> && !Readable<Point3<lll>>);
static_assert(!Writable<Point2<__float128>> && !Readable<Point2<__float128>>);
static_assert(Writable<Point2<lng>> && Readable<Point2<lng>> && Writable<Point2<int>> && Readable<Point2<int>> && Writable<dpoint> && Readable<dpoint>);
static_assert(Writable<Point2<int8_t>> && Readable<Point2<int8_t>> && Writable<Point3<lng>> && Readable<Point3<lng>> && Writable<Point3<long double>> && Readable<Point3<long double>>);
static_assert(rotateCW(Point2<int>{2, 5}) == Point2<int>{5, -2} && rotateCW(Point2<lll>{0, 1}) == Point2<lll>{1, 0});

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
                else if (probe == "unit-zero") { (void)unitApprox(Point2<lng>{}); }
                else if (probe == "arg-zero") { (void)argApprox(Point2<lng>{}); }
                else if (probe == "angle-zero") { (void)angleApprox(Point2<lng>{1, 0}, Point2<lng>{}); }
                else if (probe == "signed-angle-zero") { (void)signedAngleApprox(Point2<lng>{}, Point2<lng>{1, 0}); }
                else if (probe == "angle3-zero") { (void)angleApprox(Point3<lng>{}, Point3<lng>{1, 0, 0}); }
                else if (probe == "canonical-minimum") { (void)canonicalDirection(Point2<int>{INT_MIN, 0}); }
                else if (probe == "angle-struct-zero") { (void)Angle<lng>(Point2<lng>{}, 3); }
                else if (probe == "angle-step-zero") { (void)(Angle<lng>(Point2<lng>{2, -1}) + Point2<lng>{-2, 1}); }
                else if (probe == "angle-step-through-origin") { (void)(Angle<lng>(Point2<lng>{1, 0}) + Point2<lng>{-2, 0}); }
                else if (probe == "angle-step-back-through-origin") { (void)(Angle<lng>(Point2<lng>{-1, 0}) - Point2<lng>{-2, 0}); }
                else if (probe == "segment-angles-turns") { (void)segmentAngles(Angle<lng>(Point2<lng>{1, 0}), Angle<lng>(Point2<lng>{0, 1}, 1)); }
                else if (probe == "normal-zero") { (void)normalApprox(Point2<lng>{}); }
                else if (probe == "normal3-zero") { (void)normalApprox(Point3<lng>{}, Point3<lng>{1, 2, 3}); }
                else if (probe == "normal3-parallel") { (void)normalApprox(Point3<lng>{1, 2, 3}, Point3<lng>{-2, -4, -6}); }
                else if (probe == "projection-zero") { (void)scalarProjectionApprox(Point2<lng>{1, 2}, Point2<lng>{}); }
                else if (probe == "projection3-zero") { (void)scalarProjectionApprox(Point3<lng>{1, 2, 3}, Point3<lng>{}); }
                else { throw std::runtime_error("unknown invalid probe=" + probe); }
                throw std::runtime_error("assertion probe returned=" + probe);}
            else { throw std::runtime_error("unknown argument=" + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode=" + mode); }
        std::cout << "RUN point mode=" << mode << " seed=" << seed << '\n';
        operators<int8_t>(); operators<int16_t>(); operators<int>(); operators<lng>(); operators<lll>(); operators<float>(); operators<double>(); operators<long double>();
        std::cout << "PASS all point constructors, arithmetic/mutating operators, aliasing, casts and value semantics\n";
        streams(); clockwise(); exhaustive(); boundaries(); polar(); floating<float>(); floating<double>(); floating<long double>();
        std::cout << "PASS float/double/long-double primitives, approximate roots and exact lexicographic comparison\n";
        directions(); angles(); argumentOrder(); approxOrder(); turnAngles(); projections(); randomCases(); pythonOracle();
        std::cout << "PASS point checks=" << checks << " mode=" << mode << " seed=" << seed << '\n';
        return 0;}
    catch (const std::exception &error) {
        std::cerr << "FAIL point mode=" << mode << " seed=" << seed << " smallest-known-reproducer: " << error.what() << '\n';
        return 1;}}
