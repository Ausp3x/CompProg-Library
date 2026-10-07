#pragma once
#include "../01-Core/01-template.hpp"

namespace geometry_detail {
    template<typename T> using Wide = std::conditional_t<std::is_integral_v<T>, lll, long double>;
    template<typename T> constexpr int sign(T x) { return (x > 0) - (x < 0); }
} // namespace geometry_detail

// T: O(1), M: O(1)
// Signed integral or floating T; vector arithmetic must fit T, integral products widen to lll.
template<typename T = lng> struct Point2 {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    T x{}, y{};
    constexpr Point2() = default;
    constexpr Point2(T x, T y) : x(x), y(y) {}
    template<typename U> constexpr Point2<U> cast() const { return {U(x), U(y)}; }

    friend constexpr bool operator==(Point2, Point2) = default;
    friend constexpr bool operator<(Point2 a, Point2 b) { return a.x != b.x ? a.x < b.x : a.y < b.y; }
    constexpr Point2 operator-() const { return {T(-x), T(-y)}; }
    constexpr Point2 &operator+=(Point2 p) { x += p.x; y += p.y; return *this; }
    constexpr Point2 &operator-=(Point2 p) { x -= p.x; y -= p.y; return *this; }
    constexpr Point2 &operator*=(T k) { x *= k; y *= k; return *this; }
    constexpr Point2 &operator/=(T k) { assert(k != 0); x /= k; y /= k; return *this; }
    friend constexpr Point2 operator+(Point2 a, Point2 b) { return a += b; }
    friend constexpr Point2 operator-(Point2 a, Point2 b) { return a -= b; }
    friend constexpr Point2 operator*(Point2 a, T k) { return a *= k; }
    friend constexpr Point2 operator*(T k, Point2 a) { return a *= k; }
    friend constexpr Point2 operator/(Point2 a, T k) { return a /= k; }
};

// T: O(1), M: O(1)
template<typename T = lng> struct Point3 {
    static_assert((std::is_integral_v<T> && std::is_signed_v<T>) || std::is_floating_point_v<T>);
    T x{}, y{}, z{};
    constexpr Point3() = default;
    constexpr Point3(T x, T y, T z) : x(x), y(y), z(z) {}
    template<typename U> constexpr Point3<U> cast() const { return {U(x), U(y), U(z)}; }

    friend constexpr bool operator==(Point3, Point3) = default;
    friend constexpr bool operator<(Point3 a, Point3 b) {
        return a.x != b.x ? a.x < b.x : a.y != b.y ? a.y < b.y : a.z < b.z;}
    constexpr Point3 operator-() const { return {T(-x), T(-y), T(-z)}; }
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

// T: O(1), M: O(1). Exact lll for integral T (long double otherwise); orient > 0 is counterclockwise.
template<typename T> constexpr auto dot(Point2<T> a, Point2<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.x + W(a.y) * b.y;}
template<typename T> constexpr auto cross(Point2<T> a, Point2<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.y - W(a.y) * b.x;}
template<typename T> constexpr Point2<T> perp(Point2<T> a) { return {T(-a.y), a.x}; }
template<typename T> constexpr auto dot(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.x + W(a.y) * b.y + W(a.z) * b.z;}
template<typename T> constexpr auto cross(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return Point3<W>{W(a.y) * b.z - W(a.z) * b.y, W(a.z) * b.x - W(a.x) * b.z, W(a.x) * b.y - W(a.y) * b.x};}
template<typename P> constexpr auto norm2(P a) { return dot(a, a); }
template<typename P> constexpr auto dist2(P a, P b) {
    using W = geometry_detail::Wide<decltype(a.x)>;
    return norm2(a.template cast<W>() - b.template cast<W>());}
template<typename T> constexpr int orient(Point2<T> a, Point2<T> b, Point2<T> c) {
    using W = geometry_detail::Wide<T>;
    return geometry_detail::sign(cross(b.template cast<W>() - a.template cast<W>(), c.template cast<W>() - a.template cast<W>()));}
template<typename T> constexpr auto triple(Point3<T> a, Point3<T> b, Point3<T> c) {
    using W = geometry_detail::Wide<T>;
    return dot(a.template cast<W>(), cross(b, c));}
template<typename T> constexpr auto tetraVolume6(Point3<T> a, Point3<T> b, Point3<T> c, Point3<T> d) {
    using W = geometry_detail::Wide<T>;
    auto p = a.template cast<W>();
    return triple(b.template cast<W>() - p, c.template cast<W>() - p, d.template cast<W>() - p);}

// T: O(1), M: O(1). Rounded long double; unit and angle forms assert nonzero vectors; angles in [0, pi] or (-pi, pi].
template<typename P> long double normApprox(P a) { return std::sqrt(static_cast<long double>(norm2(a))); }
template<typename P> long double distanceApprox(P a, P b) { return std::sqrt(static_cast<long double>(dist2(a, b))); }
template<typename P> auto unitApprox(P a) {
    long double n = normApprox(a); assert(n > 0);
    return a.template cast<long double>() / n;}
template<typename T> long double argApprox(Point2<T> a) {
    assert(a != Point2<T>{});
    return std::atan2(static_cast<long double>(a.y) + 0.L, static_cast<long double>(a.x));}
template<typename T> long double angleApprox(Point2<T> a, Point2<T> b) {
    assert(a != Point2<T>{} && b != Point2<T>{});
    return std::atan2(abs(static_cast<long double>(cross(a, b))), static_cast<long double>(dot(a, b)));}
template<typename T> long double angleApprox(Point3<T> a, Point3<T> b) {
    assert(a != Point3<T>{} && b != Point3<T>{});
    return std::atan2(normApprox(cross(a, b).template cast<long double>()), static_cast<long double>(dot(a, b)));}
template<typename T> long double signedAngleApprox(Point2<T> a, Point2<T> b) {
    assert(a != Point2<T>{} && b != Point2<T>{});
    return std::atan2(static_cast<long double>(cross(a, b)) + 0.L, static_cast<long double>(dot(a, b)));}

// T: O(1), M: O(1). Integral only: strict weak order by angle in [0, 2 * pi) from +x, zero vector first, ties by length.
template<typename T> constexpr bool polarLess(Point2<T> a, Point2<T> b) {
    static_assert(std::is_integral_v<T>);
    if (a == Point2<T>{} || b == Point2<T>{}) { return a == Point2<T>{} && b != Point2<T>{}; }
    auto half = [](Point2<T> p) { return p.y < 0 || (p.y == 0 && p.x < 0); };
    if (half(a) != half(b)) { return half(a) < half(b); }
    auto c = cross(a, b);
    return c ? c > 0 : norm2(a) < norm2(b);}

// T: O(log(C)), M: O(1), C = max(|x|, |y|). Integral, coordinates above the minimum of T; d and -d share one key with y > 0 or (y == 0, x > 0).
template<typename T> constexpr Point2<T> canonicalDirection(Point2<T> a) {
    static_assert(std::is_integral_v<T>);
    assert(a.x != std::numeric_limits<T>::min() && a.y != std::numeric_limits<T>::min());
    if (T g = T(gcd(a.x, a.y))) { a /= g; }
    return a.y < 0 || (a.y == 0 && a.x < 0) ? -a : a;}
