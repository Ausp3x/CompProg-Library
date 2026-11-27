#pragma once

#include "../01-Core/01-template.hpp"

// T is a commutative ring built from lng; [x1, x2) x [y1, y2) holds lattice points; an update reaches the later queries; point coordinates < INT64_MAX.
// S: O(1), U: O(1) amortized, Q: O(k * log(k) * log(r + 1)) for solve, M: O(k); k events, r maximal runs of updates or of queries
template<typename T>
struct OfflineRectangleSweep {
    struct Event { lng x1, y1, x2, y2; T w; int id; bool rect; };
    vector<Event> ev;
    int q = 0;

    void addPoint(lng x, lng y, T w) { assert(x < INT64_MAX && y < INT64_MAX); ev.push_back({x, y, x + 1, y + 1, w, -1, false}); }
    void addRectangle(lng x1, lng y1, lng x2, lng y2, T w) { assert(x1 <= x2 && y1 <= y2); ev.push_back({x1, y1, x2, y2, w, -1, true}); }
    int addQuery(lng x1, lng y1, lng x2, lng y2) { assert(x1 <= x2 && y1 <= y2); ev.push_back({x1, y1, x2, y2, T(0), q, true}); return q++; }
    int addQuery(lng x, lng y) { assert(x < INT64_MAX && y < INT64_MAX); ev.push_back({x, y, x + 1, y + 1, T(0), q, false}); return q++; }

    vector<T> solve() const {
        bool rectUpdate = false, rectQuery = false;
        for (const Event &e : ev) { (e.id < 0 ? rectUpdate : rectQuery) |= e.rect; }
        return rectUpdate && rectQuery ? sweep<4>(false) : sweep<1>(!rectQuery);}

    // Corners (x, y) dominated strictly by query corners; K = 4 carries the (X - x) * (Y - y) area of rectangle updates.
    template<int K>
    vector<T> sweep(bool dual) const {
        struct Item { lng x, y; int t, id, sign, p; array<T, K> c; };
        array<T, K> zero;
        zero.fill(T(0));
        vector<lng> ys;
        for (const Event &e : ev) {
            if (e.id >= 0) { continue; }
            ys.push_back(e.y1);
            if (dual || e.rect) { ys.push_back(e.y2); }}
        sort(ys.begin(), ys.end());
        ys.erase(unique(ys.begin(), ys.end()), ys.end());
        int ny = int(ys.size());
        auto at = [&](lng y) { return int(lower_bound(ys.begin(), ys.end(), y) - ys.begin()); };
        vector<Item> it;
        int run = 0;
        for (int i = 0; i < int(ev.size()); ++i) {
            const Event &e = ev[i];
            run += i && (e.id < 0) != (ev[i - 1].id < 0);
            if (e.id >= 0 && dual) { it.push_back({e.x2, e.y2, run, e.id, 1, at(e.y2), zero}); continue; }
            int p1 = at(e.y1);
            if (e.id < 0 && !dual && !e.rect) {
                array<T, K> c = zero;
                c[K - 1] = e.w; it.push_back({e.x1, e.y1, run, -1, 1, p1, c});
                continue;}
            int p2 = at(e.y2);
            for (int s = 0; s < 4; ++s) {
                lng x = s & 1 ? e.x2 : e.x1, y = s & 2 ? e.y2 : e.y1;
                int sign = s == 1 || s == 2 ? -1 : 1;
                array<T, K> c = zero;
                if (e.id < 0) {
                    T v = sign > 0 ? e.w : T(0) - e.w;
                    if constexpr (K == 1) { c[0] = v; }
                    else { c = {v, T(0) - v * T(y), T(0) - v * T(x), v * T(x) * T(y)}; }}
                it.push_back({x, y, run, e.id, sign, s & 2 ? p2 : p1, c});}}
        sort(it.begin(), it.end(), [](const Item &a, const Item &b) { return a.x != b.x ? a.x < b.x : a.id > b.id; });
        vector<array<T, K>> fw(ny, zero);
        vector<T> res(q, T(0));
        auto cdq = [&](auto &&self, int lo, int hi, int a, int b) -> void {
            if (b - a < 2 || hi - lo < 2) { return; }
            int m = std::midpoint(a, b);
            for (int j = lo; j < hi; ++j) {
                const Item &e = it[j];
                if (e.id < 0 && e.t < m) {
                    for (int i = e.p; i < ny; i |= i + 1) {
                        for (int d = 0; d < K; ++d) { fw[i][d] += e.c[d]; }}}
                else if (e.id >= 0 && e.t >= m) {
                    array<T, K> s = zero;
                    for (int i = e.p; i > 0; i &= i - 1) {
                        for (int d = 0; d < K; ++d) { s[d] += fw[i - 1][d]; }}
                    T v = s[0];
                    if constexpr (K == 4) { v = s[0] * T(e.x) * T(e.y) + s[1] * T(e.x) + s[2] * T(e.y) + s[3]; }
                    res[e.id] = e.sign > 0 ? res[e.id] + v : res[e.id] - v;}}
            for (int j = lo; j < hi; ++j) {
                if (it[j].id < 0 && it[j].t < m) {
                    for (int i = it[j].p; i < ny; i |= i + 1) { fw[i] = zero; }}}
            if (m - a < 2 && b - m < 2) { return; }
            int md = int(std::stable_partition(it.begin() + lo, it.begin() + hi, [&](const Item &e) { return e.t < m; }) - it.begin());
            self(self, lo, md, a, m); self(self, md, hi, m, b);};
        cdq(cdq, 0, int(it.size()), 0, run + 1);
        return res;}
};
