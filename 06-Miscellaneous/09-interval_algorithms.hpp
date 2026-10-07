#pragma once
#include "../01-Core/01-template.hpp"
#include "../02-Data Structures/02-fenwick.hpp"

enum class IntervalDomain { Continuous, Integer };

// T: O(1), M: O(1); lng endpoints with l <= r, default [l, r), point queries in doubled lll coordinates (3 is 1.5).
struct Interval {
    lng l = 0, r = 0;
    bool left_closed = true, right_closed = false;
    pair<lll, lll> bounds(IntervalDomain domain = IntervalDomain::Continuous) const {
        assert(l <= r);
        int step = domain == IntervalDomain::Integer ? 2 : 1;
        return {2 * lll(l) + step * !left_closed, 2 * lll(r) - step * !right_closed};}
    bool empty(IntervalDomain domain = IntervalDomain::Continuous) const {
        auto [a, b] = bounds(domain); return a > b;}
    bool contains(lll twice, IntervalDomain domain = IntervalDomain::Continuous) const {
        auto [a, b] = bounds(domain);
        return a <= twice && twice <= b && (domain == IntervalDomain::Continuous || twice % 2 == 0);}
    bool operator==(const Interval &) const = default;
};

// T: NA, M: O(1) (IntervalStabbing and IntervalSchedule O(out)); result records, an empty witness is never an absence marker.
struct IntervalEvent { lng x; int phase, id, delta; };
struct IntervalOverlap { int count = 0; lll twice = 0; };
struct IntervalStabbing { bool possible = true; vector<lll> twice; };
struct IntervalSchedule { lll weight = 0; vector<int> ids; };

namespace interval_detail {
    // T: O(1) normalize, O(n) nonempty, M: O(n); closed grid bounds of nonempty sets in input order, empty ids appended to empty.
    inline bool normalize(Interval &a, IntervalDomain domain) {
        auto [l, r] = a.bounds(domain);
        if (l > r) { return false; }
        if (domain == IntervalDomain::Integer) { a = {lng(l / 2), lng(r / 2), true, true}; }
        return true;}
    struct Bound { lll l, r; int id; };
    inline vector<Bound> nonempty(const vector<Interval> &a, IntervalDomain domain, vector<int> &empty) {
        assert(a.size() <= size_t(INT_MAX)); vector<Bound> res;
        for (int i = 0; i < int(a.size()); ++i) {
            auto [l, r] = a[i].bounds(domain);
            if (l <= r) { res.push_back({l, r, i}); } else { empty.push_back(i); }}
        return res;}
} // namespace interval_detail

// T: O(n * log(n + 1)), M: O(n); sorted maximal union components without empties, Integer output closed; measure is exact length or cardinality.
inline vector<Interval> mergeIntervals(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX)); vector<Interval> b, res;
    for (Interval x : a) {
        if (interval_detail::normalize(x, domain)) { b.push_back(x); }}
    sort(b.begin(), b.end(), [](Interval x, Interval y) -> bool {
        return tuple{x.l, !x.left_closed, x.r, x.right_closed} < tuple{y.l, !y.left_closed, y.r, y.right_closed};});
    for (Interval x : b) {
        if (res.empty()) { res.push_back(x); continue; }
        Interval &p = res.back();
        bool join = domain == IntervalDomain::Integer ? lll(x.l) <= lll(p.r) + 1 : x.l < p.r || (x.l == p.r && (x.left_closed || p.right_closed));
        if (!join) { res.push_back(x); continue; }
        if (x.r > p.r) { p.r = x.r; p.right_closed = x.right_closed; }
        else if (x.r == p.r) { p.right_closed |= x.right_closed; }}
    return res;}
inline lll intervalUnionMeasure(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    lll res = 0;
    for (Interval x : mergeIntervals(a, domain)) { res += lll(x.r) - x.l + (domain == IntervalDomain::Integer); }
    return res;}

// T: O(n * log(n + 1)), M: O(n); at most 2 * n events by (x, phase, id), phases 0 open right, 1 closed left, 2 closed right, 3 open left.
inline vector<IntervalEvent> intervalEvents(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() <= size_t(INT_MAX)); vector<IntervalEvent> res;
    for (int i = 0; i < int(a.size()); ++i) {
        Interval x = a[i];
        if (!interval_detail::normalize(x, domain)) { continue; }
        res.push_back({x.l, x.left_closed ? 1 : 3, i, 1});
        res.push_back({x.r, x.right_closed ? 2 : 0, i, -1});}
    sort(res.begin(), res.end(), [](const IntervalEvent &x, const IntervalEvent &y) -> bool {
        return tuple{x.x, x.phase, x.id} < tuple{y.x, y.phase, y.id};});
    return res;}

// T: O(n * log(n + 1)), M: O(n); maximum coverage and one doubled witness point, count 0 has no witness.
inline IntervalOverlap maximumIntervalOverlap(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    auto events = intervalEvents(a, domain); IntervalOverlap res; int active = 0;
    auto it = events.begin();
    while (it != events.end()) {
        lng x = it->x;
        while (it != events.end() && it->x == x && it->phase <= 1) { active += it->delta; ++it; }
        if (active > res.count) { res = {active, 2 * lll(x)}; }
        while (it != events.end() && it->x == x) { active += it->delta; ++it; }
        if (domain == IntervalDomain::Continuous && active > res.count) { res = {active, 2 * lll(x) + 1}; }}
    return res;}

// T: O((n + q) * log(n + 1)), M: O(n + q); coverage of each doubled query point in query order, odd points count 0 in Integer mode.
inline vector<int> intervalStabbingCounts(const vector<Interval> &a, const vector<lll> &twice, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty, res; vector<lll> starts, ends;
    for (auto [l, r, i] : interval_detail::nonempty(a, domain, empty)) { starts.push_back(l); ends.push_back(r); }
    sort(starts.begin(), starts.end()); sort(ends.begin(), ends.end()); res.reserve(twice.size());
    for (lll x : twice) {
        if (domain == IntervalDomain::Integer && x % 2 != 0) { res.push_back(0); continue; }
        res.push_back(int(upper_bound(starts.begin(), starts.end(), x) - starts.begin()) - int(lower_bound(ends.begin(), ends.end(), x) - ends.begin()));}
    return res;}

// T: O(n * log(n + 1)), M: O(n); fewest increasing doubled points hitting every set, possible = false with no points when a set is empty.
inline IntervalStabbing minimumIntervalStabbing(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty; auto b = interval_detail::nonempty(a, domain, empty); IntervalStabbing res;
    if (!empty.empty()) { return {false, {}}; }
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.r, x.l, x.id} < tuple{y.r, y.l, y.id}; });
    for (auto [l, r, i] : b) {
        if (res.twice.empty() || res.twice.back() < l) { res.twice.push_back(r); }}
    return res;}

// T: O(n * log(n + 1)), M: O(n); largest pairwise disjoint subset, empty sets first in input order, then by finishing order.
inline vector<int> maximumIntervalSchedule(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> res; auto b = interval_detail::nonempty(a, domain, res);
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.r, x.l, x.id} < tuple{y.r, y.l, y.id}; });
    bool have = false; lll end = 0;
    for (auto [l, r, i] : b) {
        if (!have || end < l) { res.push_back(i); end = r; have = true; }}
    return res;}

// T: O(n * log(n + 1)), M: O(n); maximum lng weight sum in lll, empty schedule allowed, positive empty sets first.
inline IntervalSchedule weightedIntervalSchedule(const vector<Interval> &a, const vector<lng> &weight, IntervalDomain domain = IntervalDomain::Continuous) {
    assert(a.size() == weight.size()); vector<int> empty; auto b = interval_detail::nonempty(a, domain, empty);
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.r, x.l, x.id} < tuple{y.r, y.l, y.id}; });
    int n = int(b.size()); vector<lll> dp(size_t(n) + 1); vector<int> from(n); vector<bool> take(n); IntervalSchedule res;
    for (int i : empty) {
        if (weight[i] > 0) { res.weight += weight[i]; res.ids.push_back(i); }}
    for (int i = 0; i < n; ++i) {
        from[i] = int(lower_bound(b.begin(), b.begin() + i, b[i].l, [](const auto &x, lll v) { return x.r < v; }) - b.begin());
        lll candidate = dp[from[i]] + weight[b[i].id];
        take[i] = candidate > dp[i]; dp[i + 1] = max(dp[i], candidate);}
    res.weight += dp[n]; vector<int> chosen;
    for (int i = n; i > 0;) {
        if (take[i - 1]) { chosen.push_back(b[i - 1].id); i = from[i - 1]; }
        else { --i; }}
    res.ids.insert(res.ids.end(), chosen.rbegin(), chosen.rend()); return res;}

// T: O(n * log(n + 1)), M: O(n); fewest sets whose union contains target, ids by increasing start; false leaves ids unchanged.
inline bool intervalCover(const vector<Interval> &a, Interval target, vector<int> &ids, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty, res; auto b = interval_detail::nonempty(a, domain, empty);
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.l, x.r, x.id} < tuple{y.l, y.r, y.id}; });
    auto [cur, hi] = target.bounds(domain);
    int step = domain == IntervalDomain::Integer ? 2 : 1, pick = -1, i = 0; lll reach = cur - step;
    while (cur <= hi) {
        for (; i < int(b.size()) && b[i].l <= cur; ++i) {
            if (b[i].r > reach) { reach = b[i].r; pick = b[i].id; }}
        if (reach < cur) { return false; }
        res.push_back(pick); cur = reach + step;}
    ids = std::move(res); return true;}

// T: O(n * log(n + 1)), M: O(n); {machines, machine of each set}, sets sharing a machine are disjoint, empty sets use machine 0.
inline pair<int, vector<int>> intervalPartitionAssignment(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty, machine(a.size()); auto b = interval_detail::nonempty(a, domain, empty);
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.l, x.r, x.id} < tuple{y.l, y.r, y.id}; });
    priority_queue<pair<lll, int>, vector<pair<lll, int>>, std::greater<>> busy; int machines = 0;
    for (auto [l, r, i] : b) {
        if (!busy.empty() && busy.top().first < l) { machine[i] = busy.top().second; busy.pop(); }
        else { machine[i] = machines++; }
        busy.push({r, machine[i]});}
    return {max(machines, int(!a.empty())), machine};}

// T: O(n * log(n + 1)), M: O(n); remove keeps sets inside no other set (first id of equal sets) by start, counts give {contained, containing} others.
inline vector<int> removeNestedIntervals(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty, res; auto b = interval_detail::nonempty(a, domain, empty);
    if (b.empty()) { return empty.empty() ? res : vector<int>{empty[0]}; }
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.l, -x.r, x.id} < tuple{y.l, -y.r, y.id}; });
    lll reach = b[0].r - 1;
    for (auto [l, r, i] : b) {
        if (r > reach) { res.push_back(i); reach = r; }}
    return res;}
inline vector<pair<int, int>> nestedIntervalCounts(const vector<Interval> &a, IntervalDomain domain = IntervalDomain::Continuous) {
    vector<int> empty; auto b = interval_detail::nonempty(a, domain, empty);
    int n = int(a.size()), m = int(b.size()), e = int(empty.size()); vector<pair<int, int>> res(n, {e, 0});
    for (int i : empty) { res[i] = {e - 1, n - 1}; }
    sort(b.begin(), b.end(), [](const auto &x, const auto &y) { return tuple{x.l, -x.r, x.id} < tuple{y.l, -y.r, y.id}; });
    vector<lll> ends;
    for (const auto &x : b) { ends.push_back(x.r); }
    sort(ends.begin(), ends.end()); ends.erase(unique(ends.begin(), ends.end()), ends.end());
    auto rank = [&](lll r) { return int(lower_bound(ends.begin(), ends.end(), r) - ends.begin()); };
    Fenwick<int> before(int(ends.size())), after(int(ends.size()));
    for (int i = 0, j = 0; i < m; i = j) {
        while (j < m && b[j].l == b[i].l && b[j].r == b[i].r) { ++j; }
        int r = rank(b[i].r), seen = before.sum(r, int(ends.size()));
        for (int k = i; k < j; ++k) { res[b[k].id].second += seen + j - i - 1; }
        before.add(r, j - i);}
    for (int j = m, i = m; j > 0; j = i) {
        while (i > 0 && b[i - 1].l == b[j - 1].l && b[i - 1].r == b[j - 1].r) { --i; }
        int r = rank(b[i].r), seen = after.prefixSum(r + 1);
        for (int k = i; k < j; ++k) { res[b[k].id].first += seen + j - i - 1; }
        after.add(r, j - i);}
    return res;}
