#pragma once
#include "../01-Core/01-template.hpp"
#include "01-point.hpp"

// Squared distances and all choices are exact. Approx suffixes refer to metric
// values/vertices: long-double conversions, division and roots may round.
struct ConvexDiameter { int first = -1, second = -1; lll squared = 0; };
struct ConvexWidthApprox { long double width = 0; int edge = -1, opposite = -1; };
struct CaliperBoxApprox {
    bool exists = false;
    array<dpoint, 4> vertices{}; // CCW; repetitions for lower-dimensional inputs.
    long double area = 0, perimeter = 0;
    array<int, 2> edges{-1, -1}; // Supporting input edges; rectangle uses edges[0].
};

// S: O(n), Q: O(n), M: O(n), including copied hull/supports; outputs O(n).
// Signed integral coordinates |x|, |y| <= 10^9; n <= INT_MAX / 2. Input is a
// strictly convex CCW cyclic hull, with no repeated start or collinear vertices;
// use convexHull(..., false). Any cyclic starting vertex is allowed. Empty,
// singleton and distinct two-point hulls are supported. Indices refer to input.
// Empty diameter/width have -1 witnesses; empty boxes have exists=false. A
// singleton diameter is (0,0), with no antipodal pairs; a segment has one pair,
// zero width/area and doubled segment length as enclosing-box perimeter.
// Equal optima use first encountered support configuration; diameter uses the
// lexicographically smallest sorted index pair. No epsilon predicates.
struct ConvexCalipers {
    vector<point> p, e;
    vector<int> opposite;
    vector<lll> width;
    int n;

    template<typename T> explicit ConvexCalipers(const vector<Point2<T>> &h) : n(0) {
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);
        assert(h.size() <= INT_MAX / 2);
        n = int(h.size());
        for (auto q : h) {
            assert(q.x >= -1000000000 && q.x <= 1000000000 && q.y >= -1000000000 && q.y <= 1000000000);
            p.push_back(q.template cast<lng>()); }
        if (n < 2) { return; }
        assert(p[0] != p[1]);
        if (n == 2) { return; }
        for (int i = 0; i < n; ++i) {
            assert(orient(p[i], p[(i + 1) % n], p[(i + 2) % n]) > 0);
            e.push_back(p[(i + 1) % n] - p[i]); }
        int j = 0;
        for (int k = 1; k < n; ++k) { if (cross(e[0], p[k] - p[0]) > cross(e[0], p[j] - p[0])) { j = k; } }
        for (int i = 0; i < n; ++i) {
            while (cross(e[i], p[(j + 1) % n] - p[i]) > cross(e[i], p[j] - p[i])) { j = (j + 1) % n; }
            opposite.push_back(j); width.push_back(cross(e[i], p[j] - p[i])); }
    }

    // Exact comparison a/b < c/d without overflowing cross products. Positive
    // denominators; bounded 128-bit Euclidean arithmetic is O(1) in this domain.
    static bool ratioLess(ulll a, ulll b, ulll c, ulll d) {
        assert(b && d);
        bool flip = false;
        for (;;) {
            ulll q = a / b, r = c / d;
            if (q != r) { return flip ? q > r : q < r; }
            a %= b; c %= d;
            if (!a || !c) { return flip ? a > c : a < c; }
            swap(a, b); swap(c, d); flip = !flip; }
    }

    // Each distinct unordered pair admitting parallel opposite support lines,
    // including all four endpoint combinations at parallel-edge tie events.
    vector<pair<int, int>> antipodalPairs() const {
        if (n < 2) { return {}; }
        if (n == 2) { return {{0, 1}}; }
        vector<pair<int, int>> pairs;
        for (int i = 0; i < n; ++i) {
            int j = opposite[(i + n - 1) % n], last = opposite[i];
            for (;;) {
                if (i < j) { pairs.emplace_back(i, j); }
                if (j == last) { break; } j = (j + 1) % n; }
            j = (last + 1) % n;
            if (cross(e[i], p[j] - p[i]) == width[i] && i < j) { pairs.emplace_back(i, j); } }
        return pairs; }

    ConvexDiameter diameter() const {
        ConvexDiameter ans;
        if (!n) { return ans; }
        ans.first = ans.second = 0;
        for (auto [i, j] : antipodalPairs()) {
            lll d = dist2(p[i], p[j]);
            if (d > ans.squared || (d == ans.squared && pair{i, j} < pair{ans.first, ans.second})) { ans = {i, j, d}; } }
        return ans; }

    ConvexWidthApprox minimumWidthApprox() const {
        if (n < 3) { return {0, n ? 0 : -1, n > 1 ? 1 : n ? 0 : -1}; }
        int best = 0;
        for (int i = 1; i < n; ++i) {
            if (ratioLess(ulll(width[i]) * width[i], norm2(e[i]), ulll(width[best]) * width[best], norm2(e[best]))) { best = i; } }
        return {static_cast<long double>(width[best]) / normApprox(e[best]), best, opposite[best]}; }

    CaliperBoxApprox degenerateBox() const {
        CaliperBoxApprox ans;
        if (n) {
            ans.exists = true; ans.vertices.fill(p[0].cast<long double>());
            if (n == 2) { ans.vertices[1] = ans.vertices[2] = p[1].cast<long double>(); ans.perimeter = 2 * distanceApprox(p[0], p[1]); } }
        return ans; }

    CaliperBoxApprox minimumAreaRectangleApprox() const { return rectangleApprox(false); }
    CaliperBoxApprox minimumPerimeterRectangleApprox() const { return rectangleApprox(true); }
    CaliperBoxApprox rectangleApprox(bool perimeter) const {
        if (n < 3) { return degenerateBox(); }
        int lo = 0, hi = 0, best = -1; lll left = 0, right = 0; ulll num = 0, den = 1;
        for (int j = 1; j < n; ++j) {
            if (dot(e[0], p[j]) < dot(e[0], p[lo])) { lo = j; }
            if (dot(e[0], p[j]) > dot(e[0], p[hi])) { hi = j; } }
        for (int i = 0; i < n; ++i) {
            while (dot(e[i], p[(lo + 1) % n]) < dot(e[i], p[lo])) { lo = (lo + 1) % n; }
            while (dot(e[i], p[(hi + 1) % n]) > dot(e[i], p[hi])) { hi = (hi + 1) % n; }
            lll u = dot(e[i], p[hi] - p[lo]), v = width[i];
            ulll a = perimeter ? ulll(u + v) * (u + v) : ulll(u) * v, b = norm2(e[i]);
            if (best == -1 || ratioLess(a, b, num, den)) {
                best = i; num = a; den = b; left = dot(e[i], p[lo] - p[i]); right = dot(e[i], p[hi] - p[i]); } }
        CaliperBoxApprox ans; ans.exists = true; ans.edges[0] = best;
        dpoint a = e[best].cast<long double>(), b = perp(a), o = p[best].cast<long double>();
        long double d = static_cast<long double>(norm2(e[best])), v = static_cast<long double>(width[best]);
        for (int k = 0; k < 4; ++k) {
            long double x = static_cast<long double>(k == 1 || k == 2 ? right : left), y = k >= 2 ? v : 0;
            ans.vertices[k] = o + a * (x / d) + b * (y / d); }
        long double u = static_cast<long double>(right - left);
        ans.area = u * v / d; ans.perimeter = 2 * (u + v) / std::sqrt(d);
        return ans; }

    // Vertices of polar(P-P) are +/- outward edge normals / strip width.
    // A maximum determinant pair of polar vertices gives the minimum-area
    // enclosing parallelogram; its second vertex is a monotone support pointer.
    CaliperBoxApprox minimumAreaParallelogramApprox() const {
        if (n < 3) { return degenerateBox(); }
        struct Support { point dir; lll width; int edge; };
        int first = 0, neg = 0;
        for (int i = 1; i < n; ++i) {
            if (polarLess(e[i], e[first])) { first = i; }
            if (polarLess(-e[i], -e[neg])) { neg = i; } }
        vector<Support> q;
        for (int i = 0, j = 0; i < n || j < n;) {
            int a = (first + i) % n, b = (neg + j) % n;
            Support s;
            if (j == n || (i < n && polarLess(e[a], -e[b]))) { s = {e[a], width[a], a}; ++i; }
            else { s = {-e[b], width[b], b}; ++j; }
            if (q.empty() || cross(q.back().dir, s.dir) || dot(q.back().dir, s.dir) < 0) { q.push_back(s); } }
        int m = int(q.size()), j = 0, a = -1, b = -1; ulll num = 0, den = 1;
        auto better = [&](int i, int x, int y) {
            lll u = cross(q[i].dir, q[x].dir), v = cross(q[i].dir, q[y].dir);
            if ((u < 0) != (v < 0)) { return v >= 0; }
            if (u < 0) { return ratioLess(-v, q[y].width, -u, q[x].width); }
            return ratioLess(u, q[x].width, v, q[y].width); };
        for (int k = 1; k < m; ++k) { if (better(0, j, k)) { j = k; } }
        for (int i = 0; i < m; ++i) {
            while (better(i, j, (j + 1) % m)) { j = (j + 1) % m; }
            lll d = cross(q[i].dir, q[j].dir); assert(d > 0);
            ulll u = ulll(q[i].width) * q[j].width;
            if (a == -1 || ratioLess(u, d, num, den)) { a = q[i].edge; b = q[j].edge; num = u; den = d; } }
        CaliperBoxApprox ans; ans.exists = true; ans.edges = {a, b};
        long double d = static_cast<long double>(cross(e[a], e[b]));
        lll low = cross(e[b], p[b] - p[a]);
        dpoint x = e[a].cast<long double>(), y = e[b].cast<long double>(), o = p[a].cast<long double>();
        for (int k = 0; k < 4; ++k) {
            long double s = k == 1 || k == 2 ? static_cast<long double>(width[a]) : 0;
            long double t = static_cast<long double>(low + (k >= 2 ? width[b] : 0));
            ans.vertices[k] = o + (y * s - x * t) / d; }
        if (d < 0) { swap(ans.vertices[1], ans.vertices[3]); }
        ans.area = static_cast<long double>(num) / static_cast<long double>(den);
        ans.perimeter = 2 * (distanceApprox(ans.vertices[0], ans.vertices[1]) + distanceApprox(ans.vertices[1], ans.vertices[2]));
        return ans; }
};
