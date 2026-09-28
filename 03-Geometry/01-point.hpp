#pragma once
#include "../01-Core/01-template.hpp"

// Integral dot/determinant arithmetic widens before multiplication/subtraction.
// All intermediates must fit lll; vector arithmetic must fit T. Coordinates
// |x_i| <= 10^9 suffice for the predicates here, including tetraVolume6.
// cast<U>() requires values in U's range; floating destinations may round.
// Floating inputs/intermediates must be finite. Predicates use computed signs,
// without epsilon or certification; exact floating predicates belong to GE11.
namespace geometry_detail {
    template<typename T> using Wide = std::conditional_t<std::is_integral_v<T>, lll, long double>;
    template<typename T> constexpr int sign(T x) { return (x > 0) - (x < 0); }
} // namespace geometry_detail

// T: O(1), M: O(1). Signed integral or floating coordinate type T.
template<typename T = lng> struct Point2 {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    T x{}, y{};
    constexpr Point2() = default;
    constexpr Point2(T x, T y) : x(x), y(y) {}
    template<typename U> constexpr Point2<U> cast() const { return {U(x), U(y)}; }

    friend constexpr bool operator==(Point2, Point2) = default;
    friend constexpr bool operator<(Point2 a, Point2 b) { return a.x != b.x ? a.x < b.x : a.y < b.y; }
    constexpr Point2 operator-() const { return {-x, -y}; }
    constexpr Point2 &operator+=(Point2 p) { x += p.x; y += p.y; return *this; }
    constexpr Point2 &operator-=(Point2 p) { x -= p.x; y -= p.y; return *this; }
    constexpr Point2 &operator*=(T k) { x *= k; y *= k; return *this; }
    // Integral division truncates toward zero; k != 0 and quotients fit T.
    constexpr Point2 &operator/=(T k) { assert(k != 0); x /= k; y /= k; return *this; }
    friend constexpr Point2 operator+(Point2 a, Point2 b) { return a += b; }
    friend constexpr Point2 operator-(Point2 a, Point2 b) { return a -= b; }
    friend constexpr Point2 operator*(Point2 a, T k) { return a *= k; }
    friend constexpr Point2 operator*(T k, Point2 a) { return a *= k; }
    friend constexpr Point2 operator/(Point2 a, T k) { return a /= k; }
};

// T: O(1), M: O(1). Same coordinate/arithmetic contract as Point2.
template<typename T = lng> struct Point3 {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    T x{}, y{}, z{};
    constexpr Point3() = default;
    constexpr Point3(T x, T y, T z) : x(x), y(y), z(z) {}
    template<typename U> constexpr Point3<U> cast() const { return {U(x), U(y), U(z)}; }

    friend constexpr bool operator==(Point3, Point3) = default;
    friend constexpr bool operator<(Point3 a, Point3 b) {
        return a.x != b.x ? a.x < b.x : a.y != b.y ? a.y < b.y : a.z < b.z; }
    constexpr Point3 operator-() const { return {-x, -y, -z}; }
    constexpr Point3 &operator+=(Point3 p) { x += p.x; y += p.y; z += p.z; return *this; }
    constexpr Point3 &operator-=(Point3 p) { x -= p.x; y -= p.y; z -= p.z; return *this; }
    constexpr Point3 &operator*=(T k) { x *= k; y *= k; z *= k; return *this; }
    constexpr Point3 &operator/=(T k) { assert(k != 0); x /= k; y /= k; z /= k; return *this; }
    friend constexpr Point3 operator+(Point3 a, Point3 b) { return a += b; }
    friend constexpr Point3 operator-(Point3 a, Point3 b) { return a -= b; }
    friend constexpr Point3 operator*(Point3 a, T k) { return a *= k; }
    friend constexpr Point3 operator*(T k, Point3 a) { return a *= k; }
    friend constexpr Point3 operator/(Point3 a, T k) { return a /= k; }
};
using point = Point2<lng>;
using dpoint = Point2<long double>;

// T: O(1), M: O(1). Exact squared quantities for integral coordinates.
template<typename T> constexpr auto dot(Point2<T> a, Point2<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.x + W(a.y) * b.y; }
template<typename T> constexpr auto cross(Point2<T> a, Point2<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.y - W(a.y) * b.x; }
template<typename T> constexpr Point2<T> perp(Point2<T> a) { return {-a.y, a.x}; }
template<typename T> constexpr auto dot(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.x + W(a.y) * b.y + W(a.z) * b.z; }
template<typename T> constexpr auto cross(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return Point3<W>{W(a.y) * b.z - W(a.z) * b.y,
                     W(a.z) * b.x - W(a.x) * b.z, W(a.x) * b.y - W(a.y) * b.x}; }
template<typename P> constexpr auto norm2(P a) { return dot(a, a); }
template<typename P> constexpr auto dist2(P a, P b) {
    using W = geometry_detail::Wide<decltype(a.x)>;
    return norm2(a.template cast<W>() - b.template cast<W>()); }
template<typename P> long double normApprox(P a) { return std::sqrt(static_cast<long double>(norm2(a))); }
template<typename P> long double distanceApprox(P a, P b) { return std::sqrt(static_cast<long double>(dist2(a, b))); }

// T: O(1), M: O(1). Positive orientation is counterclockwise.
template<typename T> constexpr int orient(Point2<T> a, Point2<T> b, Point2<T> c) {
    using W = geometry_detail::Wide<T>;
    return geometry_detail::sign(cross(b.template cast<W>() - a.template cast<W>(),
                                       c.template cast<W>() - a.template cast<W>())); }
template<typename T> constexpr auto triple(Point3<T> a, Point3<T> b, Point3<T> c) {
    using W = geometry_detail::Wide<T>;
    return dot(a.template cast<W>(), cross(b, c)); }
template<typename T> constexpr auto tetraVolume6(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
    using W = geometry_detail::Wide<T>;
    auto p = a.template cast<W>();
    return triple(b.template cast<W>() - p, c.template cast<W>() - p, d.template cast<W>() - p); }

// T: O(1), M: O(1). Strict angle order [0, 2*pi), starting at +x; zero
// precedes nonzero vectors, same-angle ties use squared length. Integral only:
// floating determinant rounding can break comparator transitivity.
template<typename T> constexpr bool polarLess(Point2<T> a, Point2<T> b) {
    static_assert(std::is_integral_v<T>);
    if (a == Point2<T>{} || b == Point2<T>{}) { return a == Point2<T>{} && b != Point2<T>{}; }
    auto half = [](Point2<T> p) { return p.y < 0 || (p.y == 0 && p.x < 0); };
    if (half(a) != half(b)) { return half(a) < half(b); }
    auto c = cross(a, b);
    return c ? c > 0 : norm2(a) < norm2(b); }
