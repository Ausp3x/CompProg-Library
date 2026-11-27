#include "../../06-Miscellaneous/004-compression.hpp"
#include "../../02-Data Structures/061-hashmap.hpp"

ulng test_seed = 0;
string context;
lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
                    << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}

// The default unordered_map and the Data Structures HashMap as HashedEncounterCompression's Map.
template<typename T, typename H = CustomHash>
using StdHashed = HashedEncounterCompression<T, H>;
template<typename T, typename H = CustomHash>
using FlatHashed = HashedEncounterCompression<T, H, HashMap<T, int, H>>;
template<typename M>
auto hasher(const M &m) {
    if constexpr (requires { m.hash_function(); }) { return m.hash_function(); }
    else { return m.hash; }}
template<template<typename, typename = CustomHash> class HC>
constexpr const char *mapName = std::is_same_v<HC<lng>, StdHashed<lng>> ? "unordered_map" : "HashMap";

template<template<typename, typename = CustomHash> class HC>
void checkHashed(const vector<lng> &input, const vector<lng> &seen) {
    string saved = context; context += string(" map=") + mapName<HC>;
    HC<lng> h; map<lng, int> first;
    for (lng x : input) {
        auto [it, fresh] = first.emplace(x, int(first.size()));
        check(h.add(x) == it->second && h.id(x) == it->second && h.value(it->second) == x && h.size() == int(first.size()), "hashed first-encounter map oracle");}
    check(h.values == seen, "hashed first-encounter inverse");
    for (lng x : vector<lng>{-3, -2, -1, 0, 1, 2, 3, LLONG_MIN, LLONG_MAX}) {
        auto it = first.find(x);
        check(h.id(x) == (it == first.end() ? -1 : it->second), "hashed id present/absent x=" + std::to_string(x));}
    for (lng x : input) { check(h.id(x) == first[x], "hashed append preserves prior ids"); }
    h.clear(); check(h.size() == 0 && (input.empty() || h.id(input[0]) == -1) && h.add(42) == 0 && h.value(0) == 42, "hashed clear/reset");
    context = saved;}

void checkVector(const vector<lng> &input) {
    context = "input="; for (lng x : input) { context += std::to_string(x) + ','; }
    vector<pair<lng, lng>> intervals; set<lng> ends;
    for (int i = 0; i + 1 < int(input.size()); ++i) {
        lng l = min(input[i], input[i + 1]), r = max(input[i], input[i + 1]);
        intervals.push_back({l, r}); ends.insert(l); ends.insert(r);}
    auto endpoints = compressEndpoints(intervals);
    check(endpoints.v == vector<lng>(ends.begin(), ends.end()), "compressEndpoints vs ordered endpoint set");
    int n = int(input.size());
    vector<int> stable = stableRanks(input), want(n), order = argsort(input);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) { want[i] += input[j] < input[i] || (input[j] == input[i] && j < i); }
        check(stable[i] == want[i], "stableRanks counting oracle i=" + std::to_string(i));}
    check(int(order.size()) == n, "argsort size");
    for (int i = 0; i < n; ++i) { check(order[want[i]] == i, "argsort index-tie oracle, inverse of stableRanks i=" + std::to_string(i)); }
    set<lng> distinct(input.begin(), input.end());
    if (distinct.empty() || lll(*distinct.rbegin()) - *distinct.begin() < (1 << 16)) {
        auto [ids, values] = compressCounting(input);
        check(values == vector<lng>(distinct.begin(), distinct.end()), "compressCounting values vs ordered set");
        check(int(ids.size()) == n, "compressCounting ids size");
        for (int i = 0; i < n; ++i) { check(ids[i] == int(std::distance(distinct.begin(), distinct.find(input[i]))), "compressCounting dense id i=" + std::to_string(i)); }
        CoordinateCompression<lng> direct(input);
        check(direct.encode(input) == ids && direct.v == values, "compressCounting equals CoordinateCompression encode");
        check(stableRanksCounting(input) == want, "stableRanksCounting brute oracle");}
    vector<pair<lng, lng>> points; set<lng> xs, ys;
    for (int i = 0; i < n; ++i) {
        points.push_back({input[i], ~input[n - 1 - i]});
        xs.insert(points[i].first); ys.insert(points[i].second);}
    auto pc = compressPoints(points);
    check(pc.xs.v == vector<lng>(xs.begin(), xs.end()) && pc.ys.v == vector<lng>(ys.begin(), ys.end()), "compressPoints per-axis brute values");
    check(int(pc.ids.size()) == n, "compressPoints ids size");
    for (int i = 0; i < n; ++i) {
        pair<int, int> id{int(std::distance(xs.begin(), xs.find(points[i].first))), int(std::distance(ys.begin(), ys.find(points[i].second)))};
        check(pc.ids[i] == id, "compressPoints per-axis brute id i=" + std::to_string(i));}
    EncounterCompression<lng> e; vector<lng> seen;
    for (lng x : input) {
        auto it = std::find(seen.begin(), seen.end(), x);
        int id = int(it - seen.begin()); if (it == seen.end()) { seen.push_back(x); }
        check(e.add(x) == id && e.id(x) == id && e.value(id) == x, "first-encounter linear oracle");
        for (int i = 0; i < int(seen.size()); ++i) { check(e.id(seen[i]) == i, "append preserves prior IDs"); }}
    check(e.values == seen, "first-encounter inverse");
    e.clear(); check(e.size() == 0 && e.id(0) == -1 && e.add(42) == 0, "clear/reset");
    checkHashed<StdHashed>(input, seen); checkHashed<FlatHashed>(input, seen);}

struct LengthOrder { bool operator()(const string &a, const string &b) const { return a.size() < b.size(); } };
struct Direction {
    bool down;
    bool operator()(int a, int b) const { return down ? a > b : a < b; }
};
struct ZeroHash { size_t operator()(lng) const { return 0; } };
struct LowBits {
    int bits;
    size_t operator()(lng x) const { return size_t(ulng(x) & ((ulng(1) << bits) - 1)); }
};

template<template<typename, typename = CustomHash> class HC>
void hashedTypes() {
    context = string("hashed encounter types and hashes, map=") + mapName<HC>;
    HC<string> names;
    check(names.add("bb") == 0 && names.add("cc") == 1 && names.add("") == 2 && names.add("bb") == 0, "hashed string equality, not equivalence");
    check(names.add("abcdefghij") == 3 && names.add("abcdefghik") == 4 && names.id("abcdefghij") == 3 && names.id("a") == -1 && names.value(1) == "cc", "hashed long strings");
    check(names.add(names.value(4)) == 4, "hashed alias existing value");
    auto copy = names; copy.add("new");
    check(names.size() == 5 && copy.size() == 6 && names.id("new") == -1, "hashed copy independence");
    auto moved_names = std::move(copy); check(moved_names.id("new") == 5 && moved_names.value(0) == "bb", "hashed move");
    HC<lng, ZeroHash> zero;
    for (lng x = 0; x < 200; ++x) { check(zero.add(x * 7 - 600) == int(x), "hashed all-colliding hash add"); }
    for (lng x = 0; x < 200; ++x) { check(zero.id(x * 7 - 600) == int(x) && zero.id(x * 7 - 599) == -1, "hashed all-colliding hash id"); }
    HC<lng, LowBits> low(LowBits{3});
    check(hasher(low.ids).bits == 3 && low.add(8) == 0 && low.add(0) == 1 && low.add(16) == 2 && low.id(24) == -1 && low.id(0) == 1, "hashed stateful hash kept");
    HC<pair<int, int>> pairs(CustomHash(7));
    check(hasher(pairs.ids).seed == 7 && pairs.add({1, 2}) == 0 && pairs.add({2, 1}) == 1 && pairs.id({1, 2}) == 0 && pairs.id({2, 2}) == -1, "hashed seeded CustomHash on pairs");
    HC<lll> wide_ids;
    check(wide_ids.add(lll(1) << 100) == 0 && wide_ids.add((lll(1) << 100) + 1) == 1 && wide_ids.add(1) == 2 && wide_ids.id(lll(1) << 64) == -1, "hashed 128-bit keys");
    HC<bool> flags;
    check(flags.add(true) == 0 && flags.value(0) && flags.add(false) == 1 && !flags.value(1) && flags.add(true) == 0, "hashed bool proxy storage");
    HC<lng> grow;
    for (lng x = 0; x < 5000; ++x) { grow.add(x * 1000003); }
    bool kept = grow.size() == 5000;
    for (lng x = 0; x < 5000; ++x) { kept = kept && grow.id(x * 1000003) == int(x) && grow.value(int(x)) == x * 1000003; }
    check(kept, "hashed ids survive rehashing");}

void customCases() {
    context = "custom comparator / types";
    EncounterCompression<string, LengthOrder> encounter;
    check(encounter.add("bb") == 0 && encounter.add("cc") == 0 && encounter.add("a") == 1,
          "encounter comparator equality");
    check(encounter.value(0) == "bb", "first representative retained");
    check(encounter.add(encounter.value(0)) == 0, "alias existing representative");
    auto moved = std::move(encounter); check(moved.id("dd") == 0, "encounter move");
    EncounterCompression<bool> bit_ids;
    check(bit_ids.add(true) == 0 && bit_ids.value(0) && bit_ids.add(false) == 1 && !bit_ids.value(1), "bool proxy storage");
    auto endpoints = compressEndpoints(vector<pair<lng, lng>>{{INT64_MIN, INT64_MAX}, {INT64_MAX, INT64_MAX}, {-4, 7}});
    check(endpoints.v == vector<lng>{INT64_MIN, -4, 7, INT64_MAX}, "full-width offline endpoints");
    check(endpoints.pointRange(INT64_MAX, INT64_MAX) == pair{3, 3}
          && endpoints.pointRange(INT64_MAX, INT64_MAX, true, true) == pair{3, 4}, "max endpoint closure");
    lll width = 0;
    for (int i = endpoints.index(INT64_MIN); i < endpoints.index(INT64_MAX); ++i) {
        width += lll(endpoints.decode(i + 1)) - endpoints.decode(i);}
    check(width == (lll(1) << 64) - 1, "slab metric recovered without overflow");
    check(compressEndpoints(vector<pair<int, int>>{}).size() == 0, "empty endpoints");
    check(stableRanks(vector<string>{"bb", "a", "cc", "", "d"}, LengthOrder{}) == vector<int>{3, 1, 4, 0, 2}, "stableRanks comparator equivalence by index");
    check(stableRanks(vector<int>{1, 3, 2}, Direction{true}) == vector<int>{2, 0, 1} && stableRanks(vector<int>{}).empty(), "stableRanks stateful comparator and empty");
    auto down = compressEndpoints(vector<pair<int, int>>{{9, 2}, {5, 5}}, Direction{true});
    check(down.v == vector<int>{9, 5, 2}, "descending endpoints");
    auto lengths = compressEndpoints(vector<pair<string, string>>{{"b", "ccc"}, {"dd", "ee"}}, LengthOrder{});
    check(lengths.size() == 3 && lengths.decode(0) == "b" && lengths.decode(1).size() == 2 && lengths.decode(2) == "ccc", "endpoint comparator equivalence");
    context = "argsort comparators";
    check(argsort(vector<string>{"bb", "a", "cc", "", "d"}, LengthOrder{}) == vector<int>{3, 1, 4, 0, 2}, "argsort comparator equivalence by index");
    check(argsort(vector<int>{1, 3, 2, 3}, Direction{true}) == vector<int>{1, 3, 2, 0} && argsort(vector<int>{}).empty(), "argsort stateful comparator and empty");
    check(argsort(vector<lng>{LLONG_MAX, LLONG_MIN, 0, LLONG_MIN}) == vector<int>{1, 3, 2, 0}, "argsort signed extrema");
    context = "counting builds at the lng extremes";
    using Counted = pair<vector<int>, vector<lng>>;
    check(compressCounting({}) == Counted{} && stableRanksCounting({}).empty(), "counting empty");
    check(compressCounting({LLONG_MIN, LLONG_MIN + 3, LLONG_MIN}) == Counted{{0, 1, 0}, {LLONG_MIN, LLONG_MIN + 3}}, "compressCounting at LLONG_MIN");
    check(compressCounting({LLONG_MAX, LLONG_MAX - 2, LLONG_MAX}) == Counted{{1, 0, 1}, {LLONG_MAX - 2, LLONG_MAX}}, "compressCounting at LLONG_MAX");
    check(compressCounting({7}) == Counted{{0}, {7}} && stableRanksCounting({7, 7, 7}) == vector<int>{0, 1, 2}, "counting single value, span 0");
    check(stableRanksCounting({LLONG_MIN + 1, LLONG_MIN, LLONG_MIN + 1}) == vector<int>{1, 0, 2}, "stableRanksCounting at LLONG_MIN");
    check(stableRanksCounting({LLONG_MAX, LLONG_MAX - 1, LLONG_MAX}) == vector<int>{1, 0, 2}, "stableRanksCounting at LLONG_MAX");
    vector<lng> wide{-(lng(1) << 21), 5, lng(1) << 21, 5, -(lng(1) << 21)};
    check(compressCounting(wide) == Counted{{0, 1, 2, 1, 0}, {-(lng(1) << 21), 5, lng(1) << 21}}, "compressCounting span 2^22 + 1");
    check(stableRanksCounting(wide) == vector<int>{0, 2, 4, 3, 1}, "stableRanksCounting span 2^22 + 1");
    context = "compressPoints comparators";
    auto grid = compressPoints(vector<pair<int, int>>{{1, 5}, {3, 5}, {1, 2}}, Direction{true});
    check(grid.xs.v == vector<int>{3, 1} && grid.ys.v == vector<int>{5, 2} && grid.ids == vector<pair<int, int>>{{1, 0}, {0, 0}, {1, 1}}, "compressPoints descending comparator");
    check(grid.xs.pointRange(3, 1, true, true) == pair{0, 2} && grid.ys.index(4) == -1, "compressPoints axes are CoordinateCompression");
    auto words = compressPoints(vector<pair<string, string>>{{"ab", "x"}, {"cd", "yy"}, {"", "z"}}, LengthOrder{});
    check(words.xs.size() == 2 && words.ys.size() == 2 && words.ids == vector<pair<int, int>>{{1, 0}, {1, 1}, {0, 0}}, "compressPoints comparator equivalence");
    auto none = compressPoints(vector<pair<lng, lng>>{});
    check(none.xs.empty() && none.ys.empty() && none.ids.empty(), "compressPoints empty");
    hashedTypes<StdHashed>(); hashedTypes<FlatHashed>();
    cout << "PASS custom comparators/equivalence, proxy storage, offline endpoint slabs, argsort, counting extrema, points, hashed encounters\n";}

void oracle() {
    int n, q, counting;
    while (cin >> n >> q >> counting) {
        vector<lng> input(n); for (auto &x : input) { cin >> x; }
        vector<pair<lng, lng>> intervals(q); for (auto &[l, r] : intervals) { cin >> l >> r; }
        EncounterCompression<lng> e;
        for (lng x : input) { cout << e.add(x) << ' '; } cout << '\n';
        for (int x : stableRanks(input)) { cout << x << ' '; } cout << '\n';
        auto c = compressEndpoints(intervals);
        cout << c.size(); for (lng x : c.v) { cout << ' ' << x; } cout << '\n';
        for (int x : argsort(input)) { cout << x << ' '; } cout << '\n';
        HashedEncounterCompression<lng> h;
        for (lng x : input) { cout << h.add(x) << ' '; } cout << '\n';
        vector<pair<lng, lng>> points;
        for (int i = 0; i < n; ++i) { points.push_back({input[i], input[n - 1 - i]}); }
        auto pc = compressPoints(points);
        cout << pc.xs.size() << ' ' << pc.ys.size(); for (auto [x, y] : pc.ids) { cout << ' ' << x << ' ' << y; } cout << '\n';
        if (counting) {
            auto [ids, values] = compressCounting(input);
            for (int x : ids) { cout << x << ' '; } cout << '\n';
            for (lng x : values) { cout << x << ' '; } cout << '\n';
            for (int x : stableRanksCounting(input)) { cout << x << ' '; } cout << '\n';}}}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--oracle") { oracle(); return 0; }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string p = argv[++i]; EncounterCompression<int> e; e.add(1);
            if (p == "encounter-negative") { e.value(-1); }
            if (p == "encounter-end") { e.value(1); }
            if (p == "encounter-past-end") { e.value(2); }
            if (p == "endpoint-order") { compressEndpoints(vector<pair<int, int>>{{2, 1}}); }
            if (p == "counting-span") { compressCounting({-1, INT_MAX - 1}); }
            if (p == "counting-full-span") { compressCounting({LLONG_MIN, LLONG_MAX}); }
            if (p == "counting-ranks-span") { stableRanksCounting({LLONG_MAX - INT_MAX, LLONG_MAX}); }
            if (p == "counting-ranks-full-span") { stableRanksCounting({LLONG_MAX, LLONG_MIN}); }
            if (p == "hashed-negative") { HashedEncounterCompression<int> h; h.add(1); (void)h.value(-1); }
            if (p == "hashed-end") { HashedEncounterCompression<int> h; h.add(1); (void)h.value(1); }
            if (p == "hashed-past-end") { HashedEncounterCompression<int> h; h.add(1); (void)h.value(2); }
            // INT_MAX + 1 packed bools (256 MiB): the size assert fires before argsort allocates its index array.
            if (p == "argsort-max-size") { argsort(vector<bool>(size_t(INT_MAX) + 1)); }
            return 1;}}
    int bound = mode == "quick" ? 4 : mode == "full" ? 6 : 7;
    int arrays = 0;
    for (int n = 0, total = 1; n <= bound; ++n, total *= 5) {
        for (int mask = 0; mask < total; ++mask) {
            vector<lng> input(n); int code = mask;
            for (auto &x : input) { x = code % 5 - 2; code /= 5; }
            checkVector(input); ++arrays;}}
    cout << "PASS exhaustive arrays=" << arrays << " max_length=" << bound << '\n';
    customCases(); std::mt19937_64 gen(test_seed);
    int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 10000;
    for (int i = 0; i < count; ++i) {
        int n = int(gen() % 101); vector<lng> input(n);
        for (auto &x : input) {
            ulng r = gen();
            x = i % 5 == 1 ? std::bit_cast<lng>(r) : i % 5 == 2 ? LLONG_MIN + lng(r % 20) : i % 5 == 3 ? LLONG_MAX - lng(r % 20) : i % 5 == 4 ? lng(r % (1 << 14)) - (1 << 13) : lng(r % 11) - 5;}
        checkVector(input);}
    cout << "PASS random arrays=" << count << " total checks=" << checks << '\n';}
