#pragma once
#include "../01-Core/01-template.hpp"
#include "../01-Core/09-rational.hpp"
#include "001-point.hpp"

// T: O(1), M: O(1). Rounded affine map p -> [[a, b], [c, d]] * p + t; finite values, CCW radians.
// f * g applies g first; inverse returns false on a computed zero determinant, leaving out unchanged.
struct Affine2Approx {
    long double a = 1, b = 0, c = 0, d = 1;
    dpoint t{};

    dpoint applyPoint(dpoint p) const { return applyVector(p) + t; }
    dpoint applyVector(dpoint v) const { return {a * v.x + b * v.y, c * v.x + d * v.y}; }
    Point3<long double> applyHomogeneous(Point3<long double> p) const {
        return {a * p.x + b * p.y + t.x * p.z, c * p.x + d * p.y + t.y * p.z, p.z};}
    long double determinant() const { return a * d - b * c; }
    int orientation() const { return geometry_detail::sign(determinant()); }
    bool inverse(Affine2Approx &out) const {
        long double det = determinant();
        if (det == 0) { return false; }
        Affine2Approx r{d / det, -b / det, -c / det, a / det};
        r.t = -r.applyVector(t); out = r; return true;}

    friend Affine2Approx operator*(const Affine2Approx &f, const Affine2Approx &g) {
        return {f.a * g.a + f.b * g.c, f.a * g.b + f.b * g.d,
                f.c * g.a + f.d * g.c, f.c * g.b + f.d * g.d, f.applyPoint(g.t)};}

    static Affine2Approx translation(dpoint v) { return {1, 0, 0, 1, v}; }
    static Affine2Approx scaling(long double x, long double y, dpoint center = {}) {
        return {x, 0, 0, y, {center.x * (1 - x), center.y * (1 - y)}};}
    static Affine2Approx rotation(long double angle, dpoint center = {}) {
        assert(std::isfinite(angle));
        long double co = std::cos(angle), si = std::sin(angle);
        Affine2Approx r{co, -si, si, co}; r.t = center - r.applyVector(center); return r;}
    static Affine2Approx projection(dpoint p, dpoint q) {
        auto v = q - p; long double len = std::hypot(v.x, v.y);
        assert(len > 0 && std::isfinite(len)); v /= len;
        Affine2Approx r{v.x * v.x, v.x * v.y, v.x * v.y, v.y * v.y};
        r.t = p - r.applyVector(p); return r;}
    static Affine2Approx reflection(dpoint p, dpoint q) {
        auto r = projection(p, q);
        return {2 * r.a - 1, 2 * r.b, 2 * r.c, 2 * r.d - 1, 2.L * r.t};}
    static Affine2Approx similarity(dpoint p, dpoint q, dpoint u, dpoint v) {
        auto x = q - p, y = v - u; long double len = std::hypot(x.x, x.y);
        assert(len > 0 && std::isfinite(len)); x /= len; y /= len;
        long double co = dot(x, y), si = cross(x, y);
        Affine2Approx r{co, -si, si, co}; r.t = u - r.applyVector(p); return r;}
};

// T: O(1), M: O(1). Returns false for w == 0, leaving out unchanged.
inline bool cartesianApprox(Point3<long double> p, dpoint &out) {
    if (p.z == 0) { return false; }
    out = {p.x / p.z, p.y / p.z}; return true;}

// T: O(1), M: O(1). Affine2Approx::rotation(angle, center) applied to p.
inline dpoint rotateApprox(dpoint p, long double angle, dpoint center = {}) { return Affine2Approx::rotation(angle, center).applyPoint(p); }

// T: O(1) operations of R, M: O(1). Exact map p -> [[a, b], [c, d]] * p + (tx, ty) over a field R: Rational<lll> asserts on overflow,
// Rational<InfInt> is unbounded. Affine2Approx's composition order, inverse status and factory domains; points are {x, y}.
template<typename R = Rational<lll>> struct Affine2 {
    R a{1}, b{}, c{}, d{1}, tx{}, ty{};

    static Affine2 translation(const array<R, 2> &v) { return {R(1), R(), R(), R(1), v[0], v[1]}; }
    static Affine2 scaling(const R &x, const R &y, const array<R, 2> &center = {}) {
        return {x, R(), R(), y, center[0] * (R(1) - x), center[1] * (R(1) - y)};}
    static Affine2 rotation(const R &co, const R &si, const array<R, 2> &center = {}) {
        assert(co * co + si * si == R(1));
        return Affine2{co, -si, si, co}.fixing(center, center);}
    static Affine2 projection(const array<R, 2> &p, const array<R, 2> &q) {
        R x = q[0] - p[0], y = q[1] - p[1], len2 = x * x + y * y;
        assert(len2 != R());
        return Affine2{x * x / len2, x * y / len2, x * y / len2, y * y / len2}.fixing(p, p);}
    static Affine2 reflection(const array<R, 2> &p, const array<R, 2> &q) {
        auto f = projection(p, q);
        return {f.a + f.a - R(1), f.b + f.b, f.c + f.c, f.d + f.d - R(1), f.tx + f.tx, f.ty + f.ty};}
    static Affine2 similarity(const array<R, 2> &p, const array<R, 2> &q, const array<R, 2> &u, const array<R, 2> &v) {
        R x = q[0] - p[0], y = q[1] - p[1], z = v[0] - u[0], w = v[1] - u[1], len2 = x * x + y * y;
        assert(len2 != R());
        R co = (x * z + y * w) / len2, si = (x * w - y * z) / len2;
        return Affine2{co, -si, si, co}.fixing(p, u);}

    array<R, 2> applyVector(const array<R, 2> &v) const { return {a * v[0] + b * v[1], c * v[0] + d * v[1]}; }
    array<R, 2> applyPoint(const array<R, 2> &p) const { auto v = applyVector(p); return {v[0] + tx, v[1] + ty}; }
    R determinant() const { return a * d - b * c; }
    Affine2 fixing(const array<R, 2> &p, const array<R, 2> &u) const {
        auto v = applyVector(p);
        return {a, b, c, d, u[0] - v[0], u[1] - v[1]};}
    bool inverse(Affine2 &out) const {
        R det = determinant();
        if (det == R()) { return false; }
        out = Affine2{d / det, -b / det, -c / det, a / det}.fixing({tx, ty}, {}); return true;}

    friend Affine2 operator*(const Affine2 &f, const Affine2 &g) {
        auto t = f.applyPoint({g.tx, g.ty});
        return {f.a * g.a + f.b * g.c, f.a * g.b + f.b * g.d, f.c * g.a + f.d * g.c, f.c * g.b + f.d * g.d, t[0], t[1]};}
};
