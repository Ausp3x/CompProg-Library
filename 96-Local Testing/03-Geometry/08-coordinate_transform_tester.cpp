#include "../../03-Geometry/08-coordinate_transform.hpp"

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
            cerr << "FAIL assertion probe returned: " << probe << '\n'; return 1;}}
    test::fractions(); test::geometry(); cout << "PASS checks=" << test::checks << " seed=" << test::seed << '\n';}
