#include "../../03-Geometry/08-coordinate_transform.hpp"
#include "../../01-Core/07-infint.hpp"

static_assert(std::same_as<Affine2<>, Affine2<Rational<lll>>>, "Affine2 defaults to Rational<lll>");

namespace test {
    ulng seed = 20260927, checks = 0;
    long double point_scale = 1;
    string mode = "full", context;
    void require(bool ok, const string &op) {
        ++checks;
        if (!ok) { cerr << "FAIL seed=" << seed << " mode=" << mode << " op=" << op
                       << " input=" << context << '\n'; std::exit(1);}}
    void close(long double got, long double want, const string &op, long double tol = 2e-15L, long double scale = 1) {
        std::ostringstream s; s << std::setprecision(22) << op << " expected=" << want << " actual=" << got;
        require(std::isfinite(got) && abs(got - want) <= tol * max({scale, abs(got), abs(want)}), s.str());}
    void close(dpoint got, dpoint want, const string &op, long double tol = 2e-15L) {
        close(got.x, want.x, op + " x", tol, point_scale); close(got.y, want.y, op + " y", tol, point_scale);}
    void readMap(std::istream &in, Affine2Approx &f) { in >> f.a >> f.b >> f.c >> f.d >> f.t.x >> f.t.y; }
    dpoint readPoint(std::istream &in) { dpoint p; in >> p.x >> p.y; return p; }
    bool bitwise(dpoint x, dpoint y) {
        return x.x == y.x && x.y == y.y && std::signbit(x.x) == std::signbit(y.x) && std::signbit(x.y) == std::signbit(y.y);}

    void fractions() {
        const char *path = std::getenv("CP_TRANSFORM_ORACLE"); require(path, "fixture environment");
        std::ifstream in(path); require(bool(in), "fixture opened");
        string line; int count = 0;
        while (std::getline(in, line)) {
            context = "Fraction fixture " + std::to_string(count++) + " " + line;
            std::istringstream row(line); Affine2Approx f, g; readMap(row, f); readMap(row, g);
            auto p = readPoint(row); Point3<long double> h; row >> h.x >> h.y >> h.z;
            auto point_want = readPoint(row), vector_want = readPoint(row);
            Point3<long double> homogeneous; row >> homogeneous.x >> homogeneous.y >> homogeneous.z;
            auto composed = readPoint(row); long double det; row >> det;
            auto inverse_want = readPoint(row), cartesian = readPoint(row); require(bool(row), "valid fixture");
            close(f.applyPoint(p), point_want, "point vs Fraction");
            close(f.applyVector(p), vector_want, "vector vs Fraction");
            auto got = f.applyHomogeneous(h);
            close(got.x, homogeneous.x, "homogeneous x"); close(got.y, homogeneous.y, "homogeneous y");
            close(got.z, homogeneous.z, "homogeneous w");
            close((f * g).applyPoint(p), composed, "composition vs Fraction");
            close(f.determinant(), det, "determinant vs Fraction");
            require(f.orientation() == (det > 0) - (det < 0), "orientation vs exact sign");
            Affine2Approx inv{2, 3, 4, 5, {6, 7}}, saved = inv;
            require(f.inverse(inv) == (det != 0), "inverse existence");
            if (det) {
                // Inverse application can subtract huge terms to give a small
                // coordinate. Bound roundoff by pre-cancellation term sizes.
                long double x = abs(p.x) + abs(f.t.x), y = abs(p.y) + abs(f.t.y);
                long double sx = max(1.L, (abs(f.d) * x + abs(f.b) * y) / abs(det));
                long double sy = max(1.L, (abs(f.c) * x + abs(f.a) * y) / abs(det));
                auto got_inverse = inv.applyPoint(p);
                close(got_inverse.x, inverse_want.x, "inverse vs Fraction x", 2e-14L, sx);
                close(got_inverse.y, inverse_want.y, "inverse vs Fraction y", 2e-14L, sy);
                auto alias = f; require(alias.inverse(alias), "inverse alias exists");
                got_inverse = alias.applyPoint(p);
                close(got_inverse.x, inverse_want.x, "inverse alias x", 2e-14L, sx);
                close(got_inverse.y, inverse_want.y, "inverse alias y", 2e-14L, sy);}
            else {
                require(inv.a == saved.a && inv.b == saved.b && inv.c == saved.c && inv.d == saved.d && inv.t == saved.t,
                        "singular output unchanged");}
            dpoint result{123, 456}; require(cartesianApprox(h, result) == (h.z != 0), "Cartesian existence");
            close(result, h.z ? cartesian : dpoint{123, 456}, "Cartesian Fraction or unchanged output");}
        require(count > 0, "nonempty fixtures"); cout << "PASS Fraction records=" << count << '\n';}

    void geometry() {
        std::mt19937_64 rng(seed); int rounds = mode == "quick" ? 100 : mode == "full" ? 5000 : 40000;
        long double pi = std::acos(-1.L);
        auto value = [&]() { return static_cast<long double>(int(rng() % 201) - 100); };
        for (int i = 0; i < rounds; ++i) {
            long double scale = i % 3 == 0 ? 1e-8L : i % 3 == 1 ? 1 : 1e8L;
            // Cancellation can make an output zero: absolute error scales with
            // the input coordinates (bounded by 100 * scale), not that output.
            point_scale = max(1.L, 100 * scale);
            dpoint p{value() * scale, value() * scale}, center{value() * scale, value() * scale};
            dpoint v{value() * scale, value() * scale}; if (v == dpoint{}) { v.x = scale; }
            long double angle = value() / 100 * pi;
            std::ostringstream desc; desc << std::setprecision(20) << "i=" << i << " p=" << p.x << ',' << p.y
                << " center=" << center.x << ',' << center.y << " v=" << v.x << ',' << v.y << " angle=" << angle;
            context = desc.str();
            close(Affine2Approx{}.applyPoint(p), p, "identity");
            close(Affine2Approx::translation(v).applyPoint(p), p + v, "translation");
            close(Affine2Approx::translation(v).applyVector(p), p, "translation preserves vector");
            auto rotation = Affine2Approx::rotation(angle, center);
            auto z = p - center; long double radius = std::hypot(z.x, z.y), phase = std::atan2(z.y, z.x) + angle;
            close(rotation.applyPoint(p), center + dpoint{radius * std::cos(phase), radius * std::sin(phase)},
                  "polar rotation oracle", 2e-12L);
            close(rotation.applyPoint(center), center, "rotation fixes center", 2e-12L);
            close(rotation.determinant(), 1, "rotation determinant");
            close((Affine2Approx::rotation(-angle, center) * rotation).applyPoint(p), p, "inverse rotation", 2e-12L);
            auto turned = rotateApprox(p, angle, center), around = rotateApprox(p, angle);
            require(bitwise(turned, rotation.applyPoint(p)), "rotateApprox equals rotation bitwise");
            close(turned, center + dpoint{radius * std::cos(phase), radius * std::sin(phase)}, "rotateApprox polar oracle", 2e-12L);
            require(bitwise(around, Affine2Approx::rotation(angle).applyPoint(p)), "rotateApprox default center bitwise");
            long double r0 = std::hypot(p.x, p.y), phase0 = std::atan2(p.y, p.x) + angle;
            close(around, {r0 * std::cos(phase0), r0 * std::sin(phase0)}, "rotateApprox origin polar oracle", 2e-12L);
            auto reflection = Affine2Approx::reflection(center, center + v);
            auto projection = Affine2Approx::projection(center, center + v);
            // Implicit normal equation gives an independent point-projection oracle.
            auto n = perp(v); long double distance = (dot(n, p) - dot(n, center)) / dot(n, n);
            auto foot = p - distance * n;
            close(projection.applyPoint(p), foot, "implicit projection", 2e-12L);
            close(reflection.applyPoint(p), 2.L * foot - p, "implicit reflection", 2e-12L);
            close((projection * projection).applyPoint(p), foot, "projection idempotence", 2e-12L);
            close((reflection * reflection).applyPoint(p), p, "reflection involution", 2e-12L);
            close(reflection.determinant(), -1, "reflection reverses orientation");
            close(reflection.applyPoint(center + 2.L * v), center + 2.L * v, "reflection fixes line", 2e-12L);
            auto scaling = Affine2Approx::scaling(-2, 3, center);
            close(scaling.applyPoint(p), center + dpoint{-2 * z.x, 3 * z.y}, "anisotropic scaling", 2e-12L);
            require(scaling.orientation() == -1, "negative scaling orientation");
            auto constant = Affine2Approx::scaling(0, 0, center);
            close(constant.applyPoint(p), center, "rank zero map"); require(!constant.inverse(constant), "rank zero inverse");
            auto similarity = Affine2Approx::similarity(center, center + v, p, p + perp(v) * 2.L);
            close(similarity.applyPoint(center), p, "similarity first endpoint", 2e-12L);
            close(similarity.applyPoint(center + v), p + perp(v) * 2.L, "similarity second endpoint", 2e-12L);
            close(similarity.applyVector(v), perp(v) * 2.L, "similarity vector", 2e-12L);
            close(Affine2Approx::similarity(center, center + v, p, p).applyPoint(v), p, "constant similarity");
            dpoint finite; auto h = rotation.applyHomogeneous({p.x * -3, p.y * -3, -3});
            require(cartesianApprox(h, finite), "homogeneous finite conversion");
            close(finite, rotation.applyPoint(p), "homogeneous scale invariance", 2e-12L);}
        context = "axis projection singular and near-singular map";
        auto f = Affine2Approx::projection({1, 2}, {1, 9}); require(f.orientation() == 0 && !f.inverse(f), "rank one inverse");
        Affine2Approx tiny{1, 0, 0, 1e-30L}, inverse;
        require(tiny.inverse(inverse), "nonzero small determinant not suppressed"); close(inverse.d, 1e30L, "small inverse");
        cout << "PASS geometric cases=" << rounds << '\n';}

    // Exact Affine2 over Rational<lng>, Rational<lll> (the default R) and Rational<InfInt> against Python Fraction 3x3
    // matrices built pointwise from the definitions; every coefficient and image is compared exactly.
    template<typename R> using Exact = std::conditional_t<std::same_as<R, Rational<lll>>, Affine2<>, Affine2<R>>;
    template<typename R> R token(std::istream &in) {
        string s; in >> s; R x; require(bool(in) && x.read(s), "exact token " + s); return x;}
    template<typename R> array<R, 6> coefficients(const Affine2<R> &f) { return {f.a, f.b, f.c, f.d, f.tx, f.ty}; }
    template<typename R, size_t N> void same(const array<R, N> &got, const array<R, N> &want, const char *op) {
        for (size_t i = 0; i < N; ++i) {
            if (!(got[i] == want[i])) {
                std::ostringstream s; s << op << " index=" << i << " expected=" << want[i] << " actual=" << got[i];
                require(false, s.str());}}
        require(true, op);}

    template<typename R> Exact<R> exactMap(std::istream &in, string &op, vector<array<R, 2>> &marks) {
        using A = Exact<R>; using P = array<R, 2>;
        auto x = [&] { return token<R>(in); };
        auto pt = [&] { R u = x(); R v = x(); return P{u, v}; };
        in >> op; marks.clear();
        if (op == "id") { return A{}; }
        if (op == "raw") { array<R, 6> v; for (auto &y : v) { y = x(); } return A{v[0], v[1], v[2], v[3], v[4], v[5]}; }
        if (op == "tr") { return A::translation(pt()); }
        if (op == "sc") { R s = x(), t = x(); return A::scaling(s, t); }
        if (op == "scc") { R s = x(), t = x(); P c = pt(); return A::scaling(s, t, c); }
        if (op == "ro") { R co = x(), si = x(); marks = {P{}}; return A::rotation(co, si); }
        if (op == "roc") { R co = x(), si = x(); P c = pt(); marks = {c}; return A::rotation(co, si, c); }
        if (op == "pr" || op == "re") { P p = pt(), q = pt(); return op == "pr" ? A::projection(p, q) : A::reflection(p, q); }
        if (op == "si") { P p = pt(), q = pt(), u = pt(), v = pt(); marks = {p, q, u, v}; return A::similarity(p, q, u, v); }
        require(false, "exact map op " + op); return A{};}

    template<typename R> void exactRecord(std::istream &in) {
        using A = Exact<R>; using P = array<R, 2>; using C = array<R, 6>;
        string fop, gop; vector<P> marks, unused;
        A f = exactMap<R>(in, fop, marks), g = exactMap<R>(in, gop, unused);
        auto x = [&] { return token<R>(in); };
        auto pt = [&] { R u = x(); R v = x(); return P{u, v}; };
        auto map = [&] { C r; for (auto &y : r) { y = x(); } return r; };
        P p = pt(), u = pt(), v = pt();
        C fw = map(), gw = map(); P pw = pt(), vw = pt(); R dw = x();
        C fixw = map(), fgw = map(), gfw = map(); P fgpw = pt(); int flag = -1; in >> flag; C invw = map();
        string rest; require(bool(in) && !(in >> rest), "exact record shape");
        const C identity{R(1), R(), R(), R(1), R(), R()};
        same(coefficients(f), fw, "factory f vs Fraction"); same(coefficients(g), gw, "factory g vs Fraction");
        same(A{}.applyPoint(p), p, "default identity");
        same(f.applyPoint(p), pw, "applyPoint vs Fraction"); same(f.applyVector(v), vw, "applyVector vs Fraction");
        same(array<R, 1>{f.determinant()}, array<R, 1>{dw}, "determinant vs Fraction");
        auto fixed = f.fixing(p, u);
        same(coefficients(fixed), fixw, "fixing vs Fraction"); same(fixed.applyPoint(p), u, "fixing maps p to u");
        same(coefficients(f * g), fgw, "f * g vs Fraction (g first)"); same(coefficients(g * f), gfw, "g * f vs Fraction");
        same((f * g).applyPoint(p), fgpw, "composed point"); same(f.applyPoint(g.applyPoint(p)), fgpw, "g then f point");
        const A sentinel{R(2), R(3), R(5), R(7), R(11), R(13)};
        A out = sentinel, alias = f;
        require(f.inverse(out) == (flag == 1) && (flag == 0 || flag == 1), "inverse existence");
        if (flag == 1) {
            same(coefficients(out), invw, "inverse vs Fraction");
            same(coefficients(f * out), identity, "f * inverse is identity"); same(coefficients(out * f), identity, "inverse * f is identity");
            require(alias.inverse(alias), "self-aliased inverse exists"); same(coefficients(alias), invw, "self-aliased inverse");}
        else {
            same(coefficients(out), coefficients(sentinel), "singular inverse leaves out unchanged");
            require(!alias.inverse(alias), "self-aliased singular inverse"); same(coefficients(alias), fw, "singular self alias unchanged");}
        if (fop == "pr") { same(coefficients(f * f), fw, "projection idempotence"); }
        if (fop == "re") { same(coefficients(f * f), identity, "reflection involution"); same(array<R, 1>{dw}, array<R, 1>{R(-1)}, "reflection determinant"); }
        if (fop == "ro" || fop == "roc") { same(f.applyPoint(marks[0]), marks[0], "rotation fixes center"); same(array<R, 1>{dw}, array<R, 1>{R(1)}, "rotation determinant"); }
        if (fop == "si") { same(f.applyPoint(marks[0]), marks[2], "similarity maps p to u"); same(f.applyPoint(marks[1]), marks[3], "similarity maps q to v"); }}

    void exact() {
        const char *path = std::getenv("CP_TRANSFORM_EXACT"); require(path, "exact fixture environment");
        std::ifstream in(path); require(bool(in), "exact fixture opened");
        string line; int count = 0, types[3] = {};
        while (std::getline(in, line)) {
            context = "exact fixture " + std::to_string(count++) + " " + line;
            std::istringstream row(line); string type; row >> type;
            if (type == "L") { exactRecord<Rational<lng>>(row); ++types[0]; }
            else if (type == "X") { exactRecord<Rational<lll>>(row); ++types[1]; }
            else if (type == "I") { exactRecord<Rational<InfInt>>(row); ++types[2]; }
            else { require(false, "exact record type"); }}
        require(types[0] > 0 && types[1] > 0 && types[2] > 0, "every exact type has records");
        cout << "PASS exact Affine2 records=" << count << " lng=" << types[0] << " lll=" << types[1] << " InfInt=" << types[2] << '\n';}
} // namespace test

int main(int argc, char **argv) {
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { test::mode = argv[++i]; }
        else if (arg == "--seed") { test::seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "rotation-nan") { Affine2Approx::rotation(std::numeric_limits<long double>::quiet_NaN()); }
            if (probe == "projection-point") { Affine2Approx::projection({1, 2}, {1, 2}); }
            if (probe == "reflection-point") { Affine2Approx::reflection({1, 2}, {1, 2}); }
            if (probe == "similarity-point") { Affine2Approx::similarity({1, 2}, {1, 2}, {}, {3, 4}); }
            using Q = Rational<lll>; using P = array<Q, 2>;
            if (probe == "rotate-approx-nan") { rotateApprox({1, 2}, std::numeric_limits<long double>::quiet_NaN()); }
            if (probe == "exact-rotation-nonunit") { Affine2<>::rotation(Q(3, 5), Q(3, 5)); }
            if (probe == "exact-rotation-over") { Affine2<>::rotation(Q(4, 5), Q(4, 5)); }
            if (probe == "projection-infinite") { long double m = std::numeric_limits<long double>::max(); Affine2Approx::projection({-m, 0}, {m, 0}); }
            if (probe == "similarity-infinite") { long double m = std::numeric_limits<long double>::max(); Affine2Approx::similarity({-m, 0}, {m, 0}, {}, {1, 0}); }
            if (probe == "exact-projection-point") { Affine2<>::projection(P{Q(1), Q(2)}, P{Q(1), Q(2)}); }
            if (probe == "exact-reflection-point") { Affine2<>::reflection(P{Q(1), Q(2)}, P{Q(1), Q(2)}); }
            if (probe == "exact-similarity-point") { Affine2<>::similarity(P{Q(1), Q(2)}, P{Q(1), Q(2)}, P{}, P{Q(3), Q(4)}); }
            if (probe == "exact-overflow") { auto big = Affine2<>::scaling(Q(lll(1) << 100), Q(1)); auto square = big * big; cout << square.a << '\n'; }
            cerr << "FAIL assertion probe returned: " << probe << '\n'; return 1;}}
    test::fractions(); test::geometry(); test::exact(); cout << "PASS checks=" << test::checks << " seed=" << test::seed << '\n';}
