#pragma once
#include "../01-Core/01-template.hpp"

namespace geometry_detail {
    template<typename T> using Wide = std::conditional_t<std::is_integral_v<T>, lll, long double>;
    template<typename T> constexpr int sign(T x) { return (x > 0) - (x < 0); }
    template<typename P> constexpr bool half(P p) { return p.y < 0 || (p.y == 0 && p.x < 0); }
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
    constexpr Point2 &operator/=(T k) { assert(k != 0); x = T(x / k); y = T(y / k); return *this; }
    friend constexpr Point2 operator+(Point2 a, Point2 b) { return a += b; }
    friend constexpr Point2 operator-(Point2 a, Point2 b) { return a -= b; }
    friend constexpr Point2 operator*(Point2 a, T k) { return a *= k; }
    friend constexpr Point2 operator*(T k, Point2 a) { return a *= k; }
    friend constexpr Point2 operator/(Point2 a, T k) { return a /= k; }
    friend ostream &operator<<(ostream &os, Point2 p) requires requires(T v) { os << +v; } { return os << +p.x << ' ' << +p.y; }
    friend istream &operator>>(istream &is, Point2 &p) requires requires(T &v) { is >> v; } {
        std::conditional_t<sizeof(T) == 1, int, T> x, y;
        if (is >> x >> y) {
            if (T(x) == x && T(y) == y) { p = {T(x), T(y)}; }
            else { is.setstate(std::ios::failbit); }}
        return is;}
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
    constexpr Point3 &operator/=(T k) { assert(k != 0); x = T(x / k); y = T(y / k); z = T(z / k); return *this; }
    friend constexpr Point3 operator+(Point3 a, Point3 b) { return a += b; }
    friend constexpr Point3 operator-(Point3 a, Point3 b) { return a -= b; }
    friend constexpr Point3 operator*(Point3 a, T k) { return a *= k; }
    friend constexpr Point3 operator*(T k, Point3 a) { return a *= k; }
    friend constexpr Point3 operator/(Point3 a, T k) { return a /= k; }
    friend ostream &operator<<(ostream &os, Point3 p) requires requires(T v) { os << +v; } { return os << +p.x << ' ' << +p.y << ' ' << +p.z; }
    friend istream &operator>>(istream &is, Point3 &p) requires requires(T &v) { is >> v; } {
        std::conditional_t<sizeof(T) == 1, int, T> x, y, z;
        if (is >> x >> y >> z) {
            if (T(x) == x && T(y) == y && T(z) == z) { p = {T(x), T(y), T(z)}; }
            else { is.setstate(std::ios::failbit); }}
        return is;}
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
template<typename T> constexpr Point2<T> rotateCW(Point2<T> a) { return -perp(a); }
template<typename T> constexpr auto dot(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return W(a.x) * b.x + W(a.y) * b.y + W(a.z) * b.z;}
template<typename T> constexpr auto cross(Point3<T> a, Point3<T> b) {
    using W = geometry_detail::Wide<T>;
    return Point3<W>{W(a.y) * b.z - W(a.z) * b.y, W(a.z) * b.x - W(a.x) * b.z, W(a.x) * b.y - W(a.y) * b.x};}
template<typename P = point> constexpr auto norm2(P a) { return dot(a, a); }
template<typename P = point> constexpr auto dist2(P a, P b) {
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
template<typename P = point> long double normApprox(P a) { return std::sqrt(static_cast<long double>(norm2(a))); }
template<typename P = point> long double distanceApprox(P a, P b) { return std::sqrt(static_cast<long double>(dist2(a, b))); }
template<typename P = point> auto unitApprox(P a) {
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
template<typename P = point> long double scalarProjectionApprox(P a, P b) {
    long double n = normApprox(b); assert(n > 0);
    return static_cast<long double>(dot(a, b)) / n;}
template<typename T> dpoint normalApprox(Point2<T> a) { return unitApprox(perp(a)); }
template<typename T> Point3<long double> normalApprox(Point3<T> a, Point3<T> b) { return unitApprox(cross(a, b).template cast<long double>()); }

// T: O(1), M: O(1). Integral only: strict weak order by angle in [0, 2 * pi) from +x, zero vector first, ties by length.
template<typename T> constexpr bool polarLess(Point2<T> a, Point2<T> b) {
    static_assert(std::is_integral_v<T>);
    if (geometry_detail::half(a) != geometry_detail::half(b)) { return geometry_detail::half(a) < geometry_detail::half(b); }
    auto c = cross(a, b);
    return c ? c > 0 : norm2(a) < norm2(b);}
// T: O(n * log(n)), M: O(n). Integral: argument in (-pi, pi], the zero vector at angle 0 before every (x > 0, 0), ties by length.
template<typename T> vector<Point2<T>> argumentSort(vector<Point2<T>> p) {
    sort(p.begin(), p.end(), [](Point2<T> a, Point2<T> b) { return polarLess(a, b); });
    std::rotate(p.begin(), std::find_if(p.begin(), p.end(), [](Point2<T> q) { return q.y < 0; }), p.end());
    return p;}
// T: O(n * log(n)), M: O(n). Any T: by rounded argApprox in (-pi, pi], the zero vector at angle 0, ties by norm2, then input order.
template<typename T> vector<Point2<T>> argumentSortApprox(const vector<Point2<T>> &p) {
    vector<tuple<long double, long double, int>> key;
    for (int i = 0; i < int(p.size()); ++i) { key.emplace_back(p[i] == Point2<T>{} ? 0.L : argApprox(p[i]), norm2(p[i]), i); }
    sort(key.begin(), key.end());
    vector<Point2<T>> res;
    for (auto [a, d, i] : key) { res.push_back(p[i]); }
    return res;}

// T: O(1), M: O(1). Integral, v != 0: the angle of v in [0, 2 * pi) plus t full turns (KACTL); + and - move the point v by a vector whose segment avoids the origin.
template<typename T = lng> struct Angle {
    static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
    Point2<T> v; lng t = 0;
    constexpr explicit Angle(Point2<T> v, lng t = 0) : v(v), t(t) { assert(v != Point2<T>{}); }

    constexpr bool half() const { return geometry_detail::half(v); }
    constexpr Angle t90() const { return Angle(perp(v), t + (half() && v.x >= 0)); }
    constexpr Angle t180() const { return Angle(-v, t + half()); }
    constexpr Angle t360() const { return Angle(v, t + 1); }

    friend constexpr bool operator<(Angle a, Angle b) {
        if (a.t != b.t) { return a.t < b.t; }
        return a.half() != b.half() ? a.half() < b.half() : cross(a.v, b.v) > 0;}
    friend constexpr Angle operator+(Angle a, Point2<T> b) {
        assert(cross(a.v, b) != 0 || dot(a.v, a.v + b) > 0);
        // The raw turn count is off by at most one turn, since the move changes the angle by less than pi.
        Angle r(a.v + b, a.t);
        if (a.t180() < r) { --r.t; }
        return r.t180() < a ? r.t360() : r;}
    friend constexpr Angle operator-(Angle a, Point2<T> b) { return a + -b; }
};
// T: O(1), M: O(1). angleDiff is b - a with lll coordinates (dot and cross), |coordinate| <= 10^9; segmentAngles needs a.t == b.t and returns the arc of at most pi between them.
template<typename T> constexpr Angle<geometry_detail::Wide<T>> angleDiff(Angle<T> a, Angle<T> b) {
    lng turns = b.t - a.t; a.t = b.t;
    return Angle<geometry_detail::Wide<T>>({dot(a.v, b.v), cross(a.v, b.v)}, turns - (b < a));}
template<typename T> constexpr pair<Angle<T>, Angle<T>> segmentAngles(Angle<T> a, Angle<T> b) {
    assert(a.t == b.t);
    if (b < a) { swap(a, b); }
    return b < a.t180() ? pair{a, b} : pair{b, a.t360()};}

// T: O(log(C)), M: O(1), C = max(|x|, |y|). Integral, coordinates above the minimum of T; d and -d share one key with y > 0 or (y == 0, x > 0).
template<typename T> constexpr Point2<T> canonicalDirection(Point2<T> a) {
    static_assert(std::is_integral_v<T>);
    assert(a.x != std::numeric_limits<T>::min() && a.y != std::numeric_limits<T>::min());
    if (T g = T(gcd(a.x, a.y))) { a /= g; }
    return a.y < 0 || (a.y == 0 && a.x < 0) ? -a : a;}
