#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// T: O(1), M: O(1). Approximate 2D affine map p -> A * p + t;
// A = [[a,b],[c,d]]. All inputs/intermediates/results must be finite.
// Computed determinant signs have no epsilon or exact-predicate guarantee;
// inversion near singularity is ill-conditioned. Angles are CCW radians.
struct Affine2Approx {
    long double a = 1, b = 0, c = 0, d = 1;
    dpoint t{};

    dpoint applyPoint(dpoint p) const { return applyVector(p) + t; }
    dpoint applyVector(dpoint v) const { return {a * v.x + b * v.y, c * v.x + d * v.y}; }
    // Homogeneous (x,y,w): points have w != 0; directions have w == 0.
    // The algebraic zero vector is allowed but is not a projective point.
    Point3<long double> applyHomogeneous(Point3<long double> p) const {
        return {a * p.x + b * p.y + t.x * p.z, c * p.x + d * p.y + t.y * p.z, p.z}; }
    long double determinant() const { return a * d - b * c; }
    // +1 preserves orientation, -1 reverses it, 0 collapses dimension.
    int orientation() const { return geometry_detail::sign(determinant()); }

    // f * g applies g first, then f. Singular maps compose normally.
    friend Affine2Approx operator*(const Affine2Approx &f, const Affine2Approx &g) {
        return {f.a * g.a + f.b * g.c, f.a * g.b + f.b * g.d,
                f.c * g.a + f.d * g.c, f.c * g.b + f.d * g.d, f.applyPoint(g.t)}; }
    // Returns false on computed singularity; out is unchanged, and may alias *this.
    bool inverse(Affine2Approx &out) const {
        long double det = determinant();
        if (det == 0) { return false; }
        Affine2Approx r{d / det, -b / det, -c / det, a / det};
        r.t = -r.applyVector(t); out = r; return true; }

    static Affine2Approx translation(dpoint v) { return {1, 0, 0, 1, v}; }
    static Affine2Approx scaling(long double x, long double y, dpoint center = {}) {
        return {x, 0, 0, y, {center.x * (1 - x), center.y * (1 - y)}}; }
    static Affine2Approx rotation(long double angle, dpoint center = {}) {
        assert(std::isfinite(angle));
        long double co = std::cos(angle), si = std::sin(angle);
        Affine2Approx r{co, -si, si, co}; r.t = center - r.applyVector(center); return r; }
    // Orthogonal projection/reflection in the infinite line through p and q.
    // Distinct endpoints are required, including a nonzero computed length.
    static Affine2Approx projection(dpoint p, dpoint q) {
        auto v = q - p; long double len = std::hypot(v.x, v.y);
        assert(len > 0 && std::isfinite(len)); v /= len;
        Affine2Approx r{v.x * v.x, v.x * v.y, v.x * v.y, v.y * v.y};
        r.t = p - r.applyVector(p); return r; }
    static Affine2Approx reflection(dpoint p, dpoint q) {
        auto r = projection(p, q);
        return {2 * r.a - 1, 2 * r.b, 2 * r.c, 2 * r.d - 1, 2.L * r.t}; }
    // Direct similarity mapping p -> u and q -> v. q != p; u == v
    // is allowed and produces a constant map. Reflection is not included.
    static Affine2Approx similarity(dpoint p, dpoint q, dpoint u, dpoint v) {
        auto x = q - p, y = v - u; long double len = std::hypot(x.x, x.y);
        assert(len > 0 && std::isfinite(len)); x /= len; y /= len;
        long double co = dot(x, y), si = cross(x, y);
        Affine2Approx r{co, -si, si, co}; r.t = u - r.applyVector(p); return r; }
};

// T: O(1), M: O(1). w == 0 has no Cartesian point; out stays unchanged.
// Finite input and finite nonzero-w quotients required. No normalization needed.
inline bool cartesianApprox(Point3<long double> p, dpoint &out) {
    if (p.z == 0) { return false; }
    out = {p.x / p.z, p.y / p.z}; return true; }
