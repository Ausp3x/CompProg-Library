#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

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

    friend Affine2Approx operator*(const Affine2Approx &f, const Affine2Approx &g) {
        return {f.a * g.a + f.b * g.c, f.a * g.b + f.b * g.d,
                f.c * g.a + f.d * g.c, f.c * g.b + f.d * g.d, f.applyPoint(g.t)};}
    bool inverse(Affine2Approx &out) const {
        long double det = determinant();
        if (det == 0) { return false; }
        Affine2Approx r{d / det, -b / det, -c / det, a / det};
        r.t = -r.applyVector(t); out = r; return true;}

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
