#include "../../06-Miscellaneous/09-interval_algorithms.hpp"

ulng seed = 20260928;
string context;
int cases = 0;
string show(lll x) {
    if (!x) { return "0"; }
    bool neg = x < 0; ulll v = neg ? ulll(0) - ulll(x) : ulll(x); string out;
    while (v) { out += char('0' + v % 10); v /= 10; }
    if (neg) { out += '-'; } reverse(out.begin(), out.end()); return out;}
void check(bool ok, const string &message) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << message << '\n'; std::exit(1); }}
bool contains(Interval a, lll x, IntervalDomain domain) {
    if (domain == IntervalDomain::Integer && x % 2) { return false; }
    return (2 * lll(a.l) < x || (2 * lll(a.l) == x && a.left_closed))
        && (x < 2 * lll(a.r) || (x == 2 * lll(a.r) && a.right_closed));}
vector<lll> probes(const vector<Interval> &a) {
    vector<lll> x{-(lll(1) << 100), lll(1) << 100};
    for (Interval v : a) {
        for (lng p : {v.l, v.r}) { for (int d = -2; d <= 2; ++d) { x.push_back(2 * lll(p) + d); } }}
    sort(x.begin(), x.end()); x.erase(unique(x.begin(), x.end()), x.end()); return x;}
int countAt(const vector<Interval> &a, lll x, IntervalDomain domain) {
    int count = 0; for (Interval v : a) { count += contains(v, x, domain); } return count;}
lll measure(const vector<Interval> &a, IntervalDomain domain) {
    vector<lll> cuts;
    for (Interval v : a) {
        cuts.push_back(lll(v.l) + (domain == IntervalDomain::Integer && !v.left_closed));
        cuts.push_back(lll(v.r) + (domain == IntervalDomain::Integer && v.right_closed));}
    sort(cuts.begin(), cuts.end()); cuts.erase(unique(cuts.begin(), cuts.end()), cuts.end()); lll out = 0;
    for (int i = 1; i < int(cuts.size()); ++i) {
        lll point = domain == IntervalDomain::Integer ? 2 * cuts[i - 1] : cuts[i - 1] + cuts[i];
        if (countAt(a, point, domain)) { out += cuts[i] - cuts[i - 1]; }}
    return out;}
void runCase(const vector<Interval> &a, const vector<lng> &w, IntervalDomain domain) {
    ++cases; int n = int(a.size()); context = domain == IntervalDomain::Integer ? "integer " : "continuous ";
    for (int i = 0; i < n; ++i) {
        context += string(a[i].left_closed ? "[" : "(") + std::to_string(a[i].l) + ',' + std::to_string(a[i].r)
                   + (a[i].right_closed ? "]" : ")") + ':' + std::to_string(w[i]) + ' ';}
    auto points = probes(a); vector<uint> conflict(n); vector<bool> nonempty(n); vector<uint> covered;
    int peak = 0; vector<int> expected;
    for (lll x : points) {
        uint mask = 0; int at = countAt(a, x, domain); expected.push_back(at); peak = max(peak, at);
        for (int i = 0; i < n; ++i) {
            check(a[i].contains(x, domain) == contains(a[i], x, domain), "contains point=" + show(x));
            if (contains(a[i], x, domain)) { mask |= uint(1) << i; nonempty[i] = true; }}
        covered.push_back(mask);
        for (int i = 0; i < n; ++i) { if (mask >> i & 1) { conflict[i] |= mask ^ (uint(1) << i); } }}
    for (int i = 0; i < n; ++i) { check(a[i].empty(domain) == !nonempty[i], "empty input=" + std::to_string(i)); }
    auto actual = intervalStabbingCounts(a, points, domain);
    for (int i = 0; i < int(points.size()); ++i) {
        check(actual[i] == expected[i], "stabbing counts point=" + show(points[i]) + " expected=" + std::to_string(expected[i])
              + " actual=" + std::to_string(actual[i]));}
    auto merged = mergeIntervals(a, domain);
    for (lll x : points) {
        check(bool(countAt(a, x, domain)) == bool(countAt(merged, x, domain)), "merge membership point=" + show(x));
        check(countAt(merged, x, domain) <= 1, "merge disjoint components");}
    check(mergeIntervals(merged, domain) == merged, "merge idempotence");
    for (int i = 0; i < int(merged.size()); ++i) {
        check(!merged[i].empty(domain), "merge omits empties");
        if (domain == IntervalDomain::Integer) { check(merged[i].left_closed && merged[i].right_closed, "integer merge canonical"); }
        if (i) {
            check(merged[i - 1].r <= merged[i].l, "merge sorted");
            bool gap = domain == IntervalDomain::Integer ? lll(merged[i - 1].r) + 1 < merged[i].l
                       : merged[i - 1].r < merged[i].l || (!merged[i - 1].right_closed && !merged[i].left_closed);
            check(gap, "merge maximal components");}}
    lll expected_measure = measure(a, domain), actual_measure = intervalUnionMeasure(a, domain);
    check(actual_measure == expected_measure, "measure expected=" + show(expected_measure) + " actual=" + show(actual_measure));
    auto overlap = maximumIntervalOverlap(a, domain);
    check(overlap.count == peak, "maximum overlap expected=" + std::to_string(peak) + " actual=" + std::to_string(overlap.count));
    if (peak) { check(countAt(a, overlap.twice, domain) == peak, "maximum overlap witness=" + show(overlap.twice)); }
    auto events = intervalEvents(a, domain); int active = 0; vector<int> balance(n);
    for (int i = 0; i < int(events.size()); ++i) {
        auto e = events[i]; check(e.id >= 0 && e.id < n, "event input id");
        check(e.phase >= 0 && e.phase < 4 && e.delta == (e.phase % 2 ? 1 : -1), "event phase/delta");
        if (i) { auto p = events[i - 1]; check(tuple{p.x, p.phase, p.id} < tuple{e.x, e.phase, e.id}, "event stable phase ordering"); }
        balance[e.id] += e.delta; check(balance[e.id] >= 0 && balance[e.id] <= 1, "event per-interval lifetime");}
    for (int x : balance) { check(x == 0, "event lifetime closed"); }
    auto it = events.begin();
    while (it != events.end()) {
        lng x = it->x;
        while (it != events.end() && it->x == x && it->phase <= 1) { active += (it++)->delta; }
        check(active == countAt(a, 2 * lll(x), domain), "event endpoint coverage=" + std::to_string(x));
        while (it != events.end() && it->x == x) { active += (it++)->delta; }
        if (domain == IntervalDomain::Continuous) {
            check(active == countAt(a, 2 * lll(x) + 1, domain), "event right-neighborhood coverage=" + std::to_string(x));}}
    check(active == 0, "events final balance");
    vector<bool> valid(1 << n); vector<lll> sums(1 << n); valid[0] = true; lll optimum = 0; int cardinality = 0;
    for (uint mask = 1; mask < uint(1) << n; ++mask) {
        int i = std::countr_zero(mask); uint rest = mask ^ (uint(1) << i);
        valid[mask] = valid[rest] && !(conflict[i] & rest); sums[mask] = sums[rest] + w[i];
        if (valid[mask]) { optimum = max(optimum, sums[mask]); cardinality = max(cardinality, std::popcount(mask)); }}
    auto verifyIds = [&](const vector<int> &ids) -> lll {
        uint mask = 0; lll sum = 0; bool started = false; lll finish = 0;
        for (int i : ids) {
            check(0 <= i && i < n, "schedule witness range"); check(!(mask >> i & 1), "schedule witness distinct");
            if (!nonempty[i]) { check(!started, "empty schedule items precede nonempty items"); }
            else {
                lll last = 2 * lll(a[i].r) - (a[i].right_closed ? 0 : domain == IntervalDomain::Integer ? 2 : 1);
                check(!started || finish <= last, "schedule witness finish order"); finish = last; started = true;}
            mask |= uint(1) << i; sum += w[i];}
        check(valid[mask], "schedule witness compatible"); return sum;};
    auto schedule = maximumIntervalSchedule(a, domain); verifyIds(schedule);
    check(int(schedule.size()) == cardinality, "maximum cardinality expected=" + std::to_string(cardinality)
          + " actual=" + std::to_string(schedule.size()));
    auto weighted = weightedIntervalSchedule(a, w, domain);
    check(verifyIds(weighted.ids) == weighted.weight, "weighted witness sum");
    check(weighted.weight == optimum, "weighted optimum expected=" + show(optimum) + " actual=" + show(weighted.weight));
    vector<int> dp(1 << n, n + 1); dp[0] = 0;
    for (uint mask = 0; mask < uint(1) << n; ++mask) {
        for (uint add : covered) { dp[mask | add] = min(dp[mask | add], dp[mask] + 1); }}
    auto hit = minimumIntervalStabbing(a, domain); int best = dp.back();
    check(hit.possible == (best <= n), "stabbing feasibility expected=" + std::to_string(best <= n));
    if (hit.possible) {
        check(int(hit.twice.size()) == best, "minimum stabbing expected=" + std::to_string(best) + " actual=" + std::to_string(hit.twice.size()));
        for (int i = 0; i < n; ++i) {
            bool yes = false; for (lll x : hit.twice) { yes |= contains(a[i], x, domain); } check(yes, "stabbing witness input=" + std::to_string(i));}
        for (int i = 1; i < int(hit.twice.size()); ++i) { check(hit.twice[i - 1] < hit.twice[i], "stabbing witness increasing"); }}
    else { check(hit.twice.empty(), "infeasible stabbing returns no partial witness"); }
    auto sub = [&](int i, int j) {
        for (uint mask : covered) {
            if ((mask >> i & 1) && !(mask >> j & 1)) { return false; }}
        return true;};
    auto nested = nestedIntervalCounts(a, domain); vector<int> keep;
    for (int i = 0; i < n; ++i) {
        int inside = 0, outside = 0; bool kept = true;
        for (int j = 0; j < n; ++j) {
            if (j == i) { continue; }
            inside += sub(j, i); outside += sub(i, j);
            if (sub(i, j) && (!sub(j, i) || j < i)) { kept = false; }}
        check(nested[i] == pair{inside, outside}, "nested counts input=" + std::to_string(i) + " expected=" + std::to_string(inside) + "," + std::to_string(outside)
              + " actual=" + std::to_string(nested[i].first) + "," + std::to_string(nested[i].second));
        if (kept) { keep.push_back(i); }}
    auto kept = removeNestedIntervals(a, domain), sorted = kept; sort(sorted.begin(), sorted.end());
    check(sorted == keep, "remove nested keeps exactly the maximal sets");
    for (int i = 1; i < int(kept.size()); ++i) {
        auto x = a[kept[i - 1]].bounds(domain), y = a[kept[i]].bounds(domain);
        check(x.first < y.first && x.second < y.second, "remove nested output by increasing start");}
    auto [machines, machine] = intervalPartitionAssignment(a, domain);
    check(machines == max(peak, int(n > 0)) && int(machine.size()) == n, "partition machines expected=" + std::to_string(max(peak, int(n > 0))) + " actual=" + std::to_string(machines));
    for (int i = 0; i < n; ++i) {
        check(0 <= machine[i] && machine[i] < machines && (nonempty[i] || machine[i] == 0), "partition machine range");
        for (int j = i + 1; j < n; ++j) { check(machine[i] != machine[j] || !(conflict[i] >> j & 1), "partition machine disjoint"); }}
    vector<Interval> targets{{-1, 1, bool(cases & 1), bool(cases & 2)}, {1, 1, true, bool(cases & 1)}};
    if (n) {
        targets.push_back(a[0]); Interval hull{a[0].l, a[0].r, true, true};
        for (Interval v : a) { hull.l = min(hull.l, v.l); hull.r = max(hull.r, v.r); }
        targets.push_back(hull);}
    for (Interval target : targets) {
        auto all = a; all.push_back(target); vector<uint> need;
        for (lll x : probes(all)) {
            if (!contains(target, x, domain)) { continue; }
            uint mask = 0;
            for (int i = 0; i < n; ++i) { mask |= uint(contains(a[i], x, domain)) << i; }
            need.push_back(mask);}
        int fewest = n + 1;
        for (uint set = 0; set < uint(1) << n; ++set) {
            bool ok = true;
            for (uint mask : need) { ok &= (mask & set) != 0; }
            if (ok) { fewest = min(fewest, std::popcount(set)); }}
        vector<int> ids{-7}; bool done = intervalCover(a, target, ids, domain);
        check(done == (fewest <= n), "cover feasibility expected=" + std::to_string(fewest <= n));
        if (!done) { check(ids == vector<int>{-7}, "failed cover leaves ids unchanged"); continue; }
        check(int(ids.size()) == fewest, "minimum cover expected=" + std::to_string(fewest) + " actual=" + std::to_string(ids.size()));
        uint set = 0;
        for (int k = 0; k < int(ids.size()); ++k) {
            check(0 <= ids[k] && ids[k] < n && !(set >> ids[k] & 1), "cover witness ids");
            set |= uint(1) << ids[k];
            if (k) { check(a[ids[k - 1]].bounds(domain).first <= a[ids[k]].bounds(domain).first, "cover witness by increasing start"); }}
        for (uint mask : need) { check((mask & set) != 0, "cover witness covers target"); }}}
void both(const vector<Interval> &a, const vector<lng> &w) {
    for (auto d : {IntervalDomain::Continuous, IntervalDomain::Integer}) { runCase(a, w, d); }}
void exhaustive(const string &mode) {
    vector<Interval> all;
    for (lng l = -1; l <= 1; ++l) { for (lng r = l; r <= 1; ++r) {
        for (int flags = 0; flags < 4; ++flags) { all.push_back({l, r, bool(flags & 1), bool(flags & 2)}); }}}
    both({}, {});
    for (int i = 0; i < int(all.size()); ++i) {
        both({all[i]}, {i % 7 - 3});
        for (int j = 0; j < int(all.size()); ++j) {
            both({all[i], all[j]}, {i % 7 - 3, j % 7 - 3});
            if (mode == "quick") { continue; }
            for (int k = 0; k < int(all.size()); ++k) {
                if (mode == "full" && !(i <= j && j <= k)) { continue; }
                both({all[i], all[j], all[k]}, {i % 7 - 3, j % 7 - 3, k % 7 - 3});}}}
    cout << "PASS exhaustive endpoint flags/domain/empty sets cases=" << cases << '\n';}
void boundaries() {
    lng low = std::numeric_limits<lng>::min(), high = std::numeric_limits<lng>::max();
    vector<Interval> a{{low, low, true, true}, {low, high, false, false}, {high, high, true, true},
                       {low, high, true, true}, {low, low, false, false}, {high, high, true, false}};
    both(a, {high, high, high, low, high, low});
    both({{low, low + 1, false, false}, {high - 1, high, false, false}, {low, low, true, true},
          {high, high, true, true}}, {low, high, high, high});
    both({{0, 2}, {1, 3}, {2, 4}}, {high, high, high});
    both({{0, 0}, {1, 1}, {1, 1, false, false}}, {high, high, high});
    for (auto d : {IntervalDomain::Continuous, IntervalDomain::Integer}) {
        lll expected = (lll(1) << 64) - (d == IntervalDomain::Continuous);
        check(intervalUnionMeasure({{low, high, true, true}}, d) == expected, "full-width exact measure");}
    check(Interval{0, 1}.contains(0) && !Interval{0, 1}.contains(2), "default half-open contract");
    check(Interval{}.empty() && Interval{}.bounds() == pair<lll, lll>{0, -1}, "default empty interval");
    check(weightedIntervalSchedule({{0, 2}, {1, 3}}, {5, 5}).ids == vector<int>{0}, "weighted ties skip later finish");
    check(weightedIntervalSchedule({{0, 2}, {0, 2}}, {5, 5}).ids == vector<int>{0}, "weighted exact duplicates tie by input id");
    check(weightedIntervalSchedule({{}, {}, {}}, {0, -1, 1}).ids == vector<int>{2}, "weighted empty nonpositive items omitted");
    vector<int> ids;
    check(intervalCover({{low, 0, true, true}, {0, high, false, true}}, {low, high, true, true}, ids) && ids == vector<int>{0, 1}, "full-width cover");
    check(!intervalCover({{low, 0, true, false}, {0, high, false, true}}, {low, high, true, true}, ids) && ids == vector<int>{0, 1}, "open gap at zero");
    check(intervalCover({{low, 0, true, true}, {1, high, true, true}}, {low, high, true, true}, ids, IntervalDomain::Integer) && ids == vector<int>{0, 1}, "integer adjacency cover");
    check(!intervalCover({{low, 0, true, true}, {1, high, true, true}}, {low, high, true, true}, ids), "continuous gap between adjacent integers");
    check(intervalCover({}, {}, ids) && ids.empty(), "empty target needs no interval");
    check(intervalPartitionAssignment({{}, {}}) == pair<int, vector<int>>{1, {0, 0}}, "only empty sets use one machine");
    check(intervalPartitionAssignment({}).first == 0, "no sets use no machine");
    check(removeNestedIntervals({{}, {}}) == vector<int>{0} && removeNestedIntervals({{}, {0, 1}}) == vector<int>{1}, "empty sets nest in every set");
    check(nestedIntervalCounts({{low, high, true, true}, {low, high, false, false}, {}}) == vector<pair<int, int>>{{2, 0}, {1, 1}, {0, 2}}, "full-width nesting counts");
    cout << "PASS full-width endpoints/128-bit sums/half-open defaults\n";}
void randomCases(std::mt19937_64 &rng, int rounds) {
    for (int rep = 0; rep < rounds; ++rep) {
        int n = int(rng() % 10); vector<Interval> a; vector<lng> w;
        for (int i = 0; i < n; ++i) {
            lng l = lng(rng() % 15) - 7, r = lng(rng() % 15) - 7;
            if (l > r) { swap(l, r); }
            a.push_back({l, r, bool(rng() & 1), bool(rng() & 1)}); w.push_back(lng(rng() % 31) - 15);}
        both(a, w);
        if (rep % 10 == 0) {
            reverse(a.begin(), a.end()); reverse(w.begin(), w.end()); both(a, w);
            for (Interval &x : a) { x.l += 100; x.r += 100; } both(a, w);}}
    cout << "PASS independent subset schedules/set-cover hitting DP/random reorder and translation cases=" << rounds << '\n';}
void largeCase(int n) {
    #ifdef _GLIBCXX_DEBUG
    n = min(n, 2000); // Debug std::lower_bound validates each entire search range.
    #endif
    context = "large disjoint/touching n=" + std::to_string(n); vector<Interval> a; vector<lng> w(n, std::numeric_limits<lng>::max());
    for (int i = 0; i < n; ++i) { a.push_back({i, lng(i) + 1}); }
    for (auto d : {IntervalDomain::Continuous, IntervalDomain::Integer}) {
        check(maximumIntervalSchedule(a, d).size() == size_t(n), "large cardinality");
        auto weighted = weightedIntervalSchedule(a, w, d);
        check(weighted.weight == lll(n) * w[0] && weighted.ids.size() == size_t(n), "large weighted exact");
        check(intervalUnionMeasure(a, d) == n, "large union measure");
        check(mergeIntervals(a, d).size() == 1, "large merge connected");
        check(minimumIntervalStabbing(a, d).twice.size() == size_t(n), "large stabbing");
        check(maximumIntervalOverlap(a, d).count == 1, "large overlap");
        vector<int> ids; check(intervalCover(a, {0, n}, ids, d) && int(ids.size()) == n, "large touching cover");
        check(intervalPartitionAssignment(a, d).first == 1 && int(removeNestedIntervals(a, d).size()) == n, "large disjoint partition and nesting");}
    for (Interval &x : a) { x = {0, n, false, false}; }
    check(maximumIntervalSchedule(a).size() == 1 && minimumIntervalStabbing(a).twice.size() == 1, "large identical sets");
    check(maximumIntervalOverlap(a).count == n, "large peak count");
    check(intervalPartitionAssignment(a).first == n && removeNestedIntervals(a) == vector<int>{0}, "large identical partition and nesting");
    auto nested = nestedIntervalCounts(a);
    check(std::all_of(nested.begin(), nested.end(), [&](pair<int, int> x) { return x == pair{n - 1, n - 1}; }), "large identical nesting counts");
    cout << "PASS large disjoint/touching/identical n=" << n << '\n';}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i + 1 < argc; i += 2) {
        string key = argv[i], value = argv[i + 1];
        if (key == "--mode") { mode = value; } else if (key == "--seed") { seed = std::stoull(value); }
        else if (key == "--invalid") { invalid = value; }}
    if (!invalid.empty()) {
        vector<Interval> a{{2, 1}};
        if (invalid == "bounds-reversed") { a[0].bounds(); }
        else if (invalid == "merge-reversed") { mergeIntervals(a); }
        else if (invalid == "events-reversed") { intervalEvents(a); }
        else if (invalid == "counts-reversed") { intervalStabbingCounts(a, {}); }
        else if (invalid == "stabbing-reversed") { minimumIntervalStabbing(a); }
        else if (invalid == "schedule-reversed") { maximumIntervalSchedule(a); }
        else if (invalid == "weights-reversed") { weightedIntervalSchedule(a, {1}); }
        else if (invalid == "weights-size") { weightedIntervalSchedule({{0, 1}}, {}); }
        else if (invalid == "cover-reversed") { vector<int> ids; intervalCover(a, {0, 1}, ids); }
        else if (invalid == "cover-target-reversed") { vector<int> ids; intervalCover({}, {2, 1}, ids); }
        else if (invalid == "partition-reversed") { intervalPartitionAssignment(a); }
        else if (invalid == "nested-reversed") { removeNestedIntervals(a); }
        else if (invalid == "nested-counts-reversed") { nestedIntervalCounts(a); }
        return 0;}
    std::mt19937_64 rng(seed); exhaustive(mode); boundaries();
    randomCases(rng, mode == "quick" ? 100 : mode == "full" ? 2500 : 15000);
    largeCase(mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000);
    cout << "PASS interval algorithms seed=" << seed << " cases=" << cases << '\n';}
