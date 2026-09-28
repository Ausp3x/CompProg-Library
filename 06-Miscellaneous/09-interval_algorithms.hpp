#pragma once
#include "../01-Core/01-template.hpp"

enum class IntervalDomain { Continuous, Integer };

// T: O(1), M: O(1). Integral endpoints, l <= r; default [l,r).
// Continuous means subsets of the real line; Integer means subsets of Z.
// Exact query/witness coordinates are doubled: twice=3 represents 1.5.
struct Interval {
    lng l = 0, r = 0;
    bool left_closed = true, right_closed = false;
    // Closed doubled bounds on the representative grid, NOT real endpoints:
    // continuous (0,1) gives {1,1}. Every nonempty intersection with integral
    // endpoints contains a grid point. Integer mode uses only even grid points.
    pair<lll, lll> bounds(IntervalDomain domain = IntervalDomain::Continuous) const {
        assert(l <= r);
        int step = domain == IntervalDomain::Integer ? 2 : 1;
        return {2 * lll(l) + step * !left_closed, 2 * lll(r) - step * !right_closed}; }
    bool empty(IntervalDomain domain = IntervalDomain::Continuous) const {
        auto [a, b] = bounds(domain); return a > b; }
    bool contains(lll twice, IntervalDomain domain = IntervalDomain::Continuous) const {
        auto [a, b] = bounds(domain);
        return a <= twice && twice <= b && (domain == IntervalDomain::Continuous || twice % 2 == 0); }
    bool operator==(const Interval &) const = default;
};

namespace interval_detail {
    inline bool normalize(Interval &a, IntervalDomain domain) {
        auto [l, r] = a.bounds(domain);
        if (l > r) { return false; }
        if (domain == IntervalDomain::Integer) { a = {lng(l / 2), lng(r / 2), true, true}; }
        return true; }
    inline vector<int> finishOrder(const vector<Interval> &a, IntervalDomain domain) {
        assert(a.size() <= size_t(INT_MAX)); vector<int> ids;
        for (int i = 0; i < int(a.size()); ++i) { if (!a[i].empty(domain)) { ids.push_back(i); } }
        sort(ids.begin(), ids.end(), [&](int i, int j) -> bool {
            auto x = a[i].bounds(domain), y = a[j].bounds(domain);
            return tuple{x.second, x.first, i} < tuple{y.second, y.first, j}; });
        return ids; }
} // namespace interval_detail

// T: O(n * log(n + 1)), M: O(n) workspace and returned storage. Does not mutate a.
// Drop empty sets; merge connected unions. In Integer mode adjacent integers
// coalesce and results are closed. Continuous touching intervals coalesce iff
// at least one includes the shared endpoint. No arithmetic in endpoint width.
inline vector<Interval> mergeIntervals(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX)); vector<Interval> b, out;
    for (Interval x : a) { if (interval_detail::normalize(x, domain)) { b.push_back(x); } }
    sort(b.begin(), b.end(), [](Interval x, Interval y) -> bool {
        return tuple{x.l, !x.left_closed, x.r, x.right_closed} < tuple{y.l, !y.left_closed, y.r, y.right_closed}; });
    for (Interval x : b) {
        if (out.empty()) { out.push_back(x); continue; }
        Interval &p = out.back();
        bool join = domain == IntervalDomain::Integer ? lll(x.l) <= lll(p.r) + 1
                    : x.l < p.r || (x.l == p.r && (x.left_closed || p.right_closed));
        if (!join) { out.push_back(x); continue; }
        if (x.r > p.r) { p.r = x.r; p.right_closed = x.right_closed; }
        else if (x.r == p.r) { p.right_closed |= x.right_closed; } }
    return out; }

// T: O(n * log(n + 1)), M: O(n). Continuous length / Integer cardinality, exact.
// The entire lng integer domain has cardinality 2^64, represented in lll.
inline lll intervalUnionMeasure(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    lll total = 0;
    for (Interval x : mergeIntervals(a, domain)) { total += lll(x.r) - x.l + (domain == IntervalDomain::Integer); }
    return total; }

// phase 0: remove open right; 1: add closed left; 2: remove closed right;
// 3: add open left. After phase 1 the active set is exactly at x; after phase 3
// it is just to the right. Equal-phase events are ordered by original input id.
struct IntervalEvent { lng x; int phase, id, delta; };
// T: O(n * log(n + 1)), M: O(n) returned storage; at most 2*n events.
// Empty sets omitted. Integer events use each set's closed integer hull;
// inspect phase 1 at integer coordinates; between consecutive integers is void.
inline vector<IntervalEvent> intervalEvents(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX)); vector<IntervalEvent> out;
    for (int i = 0; i < int(a.size()); ++i) {
        Interval x = a[i]; if (!interval_detail::normalize(x, domain)) { continue; }
        out.push_back({x.l, x.left_closed ? 1 : 3, i, 1});
        out.push_back({x.r, x.right_closed ? 2 : 0, i, -1}); }
    sort(out.begin(), out.end(), [](const IntervalEvent &x, const IntervalEvent &y) -> bool {
        return tuple{x.x, x.phase, x.id} < tuple{y.x, y.phase, y.id}; });
    return out; }

struct IntervalOverlap { int count = 0; lll twice = 0; };
// T: O(n * log(n + 1)), M: O(n). Maximum coverage and one exact point attaining
// it. count=0 has no witness. Integer witnesses are always even doubled values.
inline IntervalOverlap maximumIntervalOverlap(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    auto events = intervalEvents(a, domain); IntervalOverlap out; int active = 0;
    auto it = events.begin();
    while (it != events.end()) {
        lng x = it->x;
        while (it != events.end() && it->x == x && it->phase <= 1) { active += it->delta; ++it; }
        if (active > out.count) { out = {active, 2 * lll(x)}; }
        while (it != events.end() && it->x == x) { active += it->delta; ++it; }
        if (domain == IntervalDomain::Continuous && active > out.count) { out = {active, 2 * lll(x) + 1}; } }
    return out; }

// T: O(n * log(n + 1) + q * log(n + 1)), M: O(n) workspace, O(q) returned.
// Query integer/half-integer points in input order; arbitrary lll doubled query
// coordinates allowed. In Integer mode odd doubled coordinates have count zero.
inline vector<int> intervalStabbingCounts(const vector<Interval> &a, const vector<lll> &twice,
                                         IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX)); vector<lll> starts, ends;
    for (Interval x : a) {
        auto [l, r] = x.bounds(domain);
        if (l <= r) { starts.push_back(l); ends.push_back(r); } }
    sort(starts.begin(), starts.end()); sort(ends.begin(), ends.end()); vector<int> out; out.reserve(twice.size());
    for (lll x : twice) {
        if (domain == IntervalDomain::Integer && x % 2 != 0) { out.push_back(0); continue; }
        out.push_back(int(upper_bound(starts.begin(), starts.end(), x) - starts.begin())
                      - int(lower_bound(ends.begin(), ends.end(), x) - ends.begin())); }
    return out; }

struct IntervalStabbing { bool possible = true; vector<lll> twice; };
// T: O(n * log(n + 1)), M: O(n) workspace and returned storage.
// Minimum points hitting EVERY input set; an empty interval makes this impossible
// and returns no points. Empty input succeeds with none. Points are increasing.
inline IntervalStabbing minimumIntervalStabbing(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX));
    for (Interval x : a) { if (x.empty(domain)) { return {false, {}}; } }
    IntervalStabbing out;
    for (int i : interval_detail::finishOrder(a, domain)) {
        auto [l, r] = a[i].bounds(domain);
        if (out.twice.empty() || out.twice.back() < l) { out.twice.push_back(r); } }
    return out; }

// T: O(n * log(n + 1)), M: O(n) workspace and returned storage.
// Largest pairwise disjoint subset: empty sets first (input order), then nonempty
// intervals in finishing order. Touching is compatible iff their intersection
// is empty under the chosen domain. Empty intervals are mutually compatible.
inline vector<int> maximumIntervalSchedule(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    auto ids = interval_detail::finishOrder(a, domain); vector<int> out;
    for (int i = 0; i < int(a.size()); ++i) { if (a[i].empty(domain)) { out.push_back(i); } }
    bool have = false; lll end = 0;
    for (int i : ids) {
        auto [l, r] = a[i].bounds(domain);
        if (!have || end < l) { out.push_back(i); end = r; have = true; } }
    return out; }

struct IntervalSchedule { lll weight = 0; vector<int> ids; };
// T: O(n * log(n + 1)), M: O(n) workspace and returned storage. n <= INT_MAX;
// weights are any lng, exact sum in lll. Empty schedule allowed. All positive-
// weight empty sets are included first; nonempty witness is in finishing order.
// Ties skip the later finish-order item. Zero-weight empty intervals are skipped.
inline IntervalSchedule weightedIntervalSchedule(const vector<Interval> &a, const vector<lng> &weight,
                                                 IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() == weight.size()); auto ids = interval_detail::finishOrder(a, domain);
    int n = int(ids.size()); vector<lll> ends; ends.reserve(n);
    for (int i : ids) { ends.push_back(a[i].bounds(domain).second); }
    vector<lll> dp(size_t(n) + 1); vector<int> from(n); vector<bool> take(n); IntervalSchedule out;
    for (int i = 0; i < int(a.size()); ++i) {
        if (a[i].empty(domain) && weight[i] > 0) { out.weight += weight[i]; out.ids.push_back(i); } }
    for (int i = 0; i < n; ++i) {
        from[i] = int(lower_bound(ends.begin(), ends.begin() + i, a[ids[i]].bounds(domain).first) - ends.begin());
        lll candidate = dp[from[i]] + weight[ids[i]];
        take[i] = candidate > dp[i]; dp[i + 1] = max(dp[i], candidate); }
    out.weight += dp[n]; vector<int> chosen;
    for (int i = n; i > 0;) {
        if (take[i - 1]) { chosen.push_back(ids[i - 1]); i = from[i - 1]; }
        else { --i; } }
    out.ids.insert(out.ids.end(), chosen.rbegin(), chosen.rend()); return out; }
