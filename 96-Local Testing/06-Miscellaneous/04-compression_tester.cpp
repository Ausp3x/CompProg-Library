#include "../../06-Miscellaneous/04-compression.hpp"

ulng test_seed = 0;
string context;
lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
                    << " operation=" << what << " expected=true actual=false\n"; std::exit(1); }}

void checkVector(const vector<lng> &input) {
    context = "input="; for (lng x : input) { context += std::to_string(x) + ','; }
    Compression<lng> c(input);
    set<lng> distinct(input.begin(), input.end()); vector<lng> expected(distinct.begin(), distinct.end());
    check(c.v == expected && c.size() == int(expected.size()), "sorted unique vs ordered set");
    for (int i = 0; i < c.size(); ++i) {
        check(c.id(c.value(i)) == i && c.value(i) == expected[i], "inverse mapping"); }
    vector<int> encoded = c.encode(input);
    for (int i = 0; i < int(input.size()); ++i) {
        check(c.value(encoded[i]) == input[i], "encode preserves original values"); }
    for (lng x = -3; x <= 3; ++x) {
        int lo = 0, hi = 0;
        for (lng y : expected) { lo += y < x; hi += y <= x; }
        check(c.lowerBound(x) == lo && c.upperBound(x) == hi, "linear bound oracle x=" + std::to_string(x));
        check(c.id(x) == (lo < hi ? lo : -1), "absent/present x=" + std::to_string(x)); }
    for (lng l = -3; l <= 3; ++l) { for (lng r = l; r <= 3; ++r) {
        for (bool lc : {false, true}) { for (bool rc : {false, true}) {
            auto [a, b] = c.pointRange(l, r, lc, rc); vector<lng> actual, want;
            for (lng x : expected) { if ((lc ? x >= l : x > l) && (rc ? x <= r : x < r)) { want.push_back(x); }}
            for (int i = a; i < b; ++i) { actual.push_back(c.value(i)); }
            check(0 <= a && a <= b && b <= c.size() && actual == want,
                  "point interval l=" + std::to_string(l) + " r=" + std::to_string(r)
                  + " left_closed=" + std::to_string(lc) + " right_closed=" + std::to_string(rc)); }}}}
    auto snapshot = c; auto moved = std::move(snapshot);
    check(moved.v == c.v, "snapshot copy/move");
    c.assign({17, 17, -19}); check(c.size() == 2 && c.value(0) == -19 && c.id(17) == 1, "rebuild resets ranks");
    EncounterCompression<lng> e; vector<lng> seen;
    for (lng x : input) {
        auto it = std::find(seen.begin(), seen.end(), x);
        int id = int(it - seen.begin()); if (it == seen.end()) { seen.push_back(x); }
        check(e.add(x) == id && e.id(x) == id && e.value(id) == x, "first-encounter linear oracle");
        for (int i = 0; i < int(seen.size()); ++i) { check(e.id(seen[i]) == i, "append preserves prior IDs"); }}
    check(e.values == seen, "first-encounter inverse");
    e.clear(); check(e.size() == 0 && e.id(0) == -1 && e.add(42) == 0, "clear/reset"); }

struct LengthOrder { bool operator()(const string &a, const string &b) const { return a.size() < b.size(); } };
struct Direction {
    bool down;
    bool operator()(int a, int b) const { return down ? a > b : a < b; }
};

void customCases() {
    context = "custom comparator / types";
    Compression<int, Direction> c({3, 1, 2, 3}, Direction{true});
    check(c.v == vector<int>{3, 2, 1}, "descending stateful comparator");
    check(c.pointRange(3, 1) == pair{0, 2} && c.pointRange(3, 1, true, true) == pair{0, 3}, "descending ranges");
    check(c.lowerBound(2) == 1 && c.upperBound(2) == 2 && c.id(4) == -1, "descending bounds");
    check(c.encode({4, 2, 0, 3}) == vector<int>{-1, 1, -1, 0}, "encode absent and present values");
    Compression<string, LengthOrder> lengths({"aaa", "b", "cc", "ddd", "", "e"});
    check(lengths.size() == 4 && lengths.id("zz") == 2 && lengths.value(2).size() == 2, "equivalence classes");
    auto copy = lengths; lengths.assign({"wwww", "x"});
    check(copy.id("aaa") == 3 && lengths.id("aaa") == -1, "copy independent of rebuild");
    EncounterCompression<string, LengthOrder> encounter;
    check(encounter.add("bb") == 0 && encounter.add("cc") == 0 && encounter.add("a") == 1,
          "encounter comparator equality");
    check(encounter.value(0) == "bb", "first representative retained");
    check(encounter.add(encounter.value(0)) == 0, "alias existing representative");
    auto moved = std::move(encounter); check(moved.id("dd") == 0, "encounter move");
    Compression<bool> bits({true, false, true});
    EncounterCompression<bool> bit_ids;
    check(!bits.value(0) && bits.value(1) && bit_ids.add(true) == 0 && bit_ids.value(0), "bool proxy storage");
    Compression<pair<int, int>> pairs({{1, 2}, {1, 1}, {1, 2}});
    check(pairs.size() == 2 && pairs.id({1, 2}) == 1, "composite keys");
    Compression<lll> wide({lll(INT64_MAX) + 1, lll(INT64_MIN) - 1, 0});
    check(wide.size() == 3 && wide.id(0) == 1, "128-bit coordinates");
    auto endpoints = compressEndpoints(vector<pair<lng, lng>>{{INT64_MIN, INT64_MAX}, {INT64_MAX, INT64_MAX}, {-4, 7}});
    check(endpoints.v == vector<lng>{INT64_MIN, -4, 7, INT64_MAX}, "full-width offline endpoints");
    check(endpoints.pointRange(INT64_MAX, INT64_MAX) == pair{3, 3}
          && endpoints.pointRange(INT64_MAX, INT64_MAX, true, true) == pair{3, 4}, "max endpoint closure");
    lll width = 0;
    for (int i = endpoints.id(INT64_MIN); i < endpoints.id(INT64_MAX); ++i) {
        width += lll(endpoints.value(i + 1)) - endpoints.value(i); }
    check(width == (lll(1) << 64) - 1, "slab metric recovered without overflow");
    check(compressEndpoints(vector<pair<int, int>>{}).size() == 0, "empty endpoints");
    auto down = compressEndpoints(vector<pair<int, int>>{{9, 2}, {5, 5}}, Direction{true});
    check(down.v == vector<int>{9, 5, 2}, "descending endpoints");
    cout << "PASS custom comparators/equivalence, proxy/composite/128-bit types, offline endpoint slabs\n"; }

void oracle() {
    int n, q;
    while (cin >> n >> q) {
        vector<lng> input(n); for (auto &x : input) { cin >> x; }
        Compression<lng> c(input); EncounterCompression<lng> e;
        cout << c.size(); for (lng x : c.v) { cout << ' ' << x; } cout << '\n';
        for (int x : c.encode(input)) { cout << x << ' '; } cout << '\n';
        for (lng x : input) { cout << e.add(x) << ' '; } cout << '\n';
        for (int i = 0; i < q; ++i) {
            lng l, r; int lc, rc; cin >> l >> r >> lc >> rc; auto [a, b] = c.pointRange(l, r, lc, rc);
            cout << c.id(l) << ' ' << c.lowerBound(l) << ' ' << c.upperBound(l) << ' ' << a << ' ' << b << '\n'; }}
}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--oracle") { oracle(); return 0; }
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string p = argv[++i]; Compression<int> c({1, 2}); EncounterCompression<int> e; e.add(1);
            if (p == "snapshot-negative") { c.value(-1); }
            if (p == "snapshot-end") { c.value(2); }
            if (p == "encounter-negative") { e.value(-1); }
            if (p == "encounter-end") { e.value(1); }
            if (p == "range-order") { c.pointRange(2, 1); }
            if (p == "endpoint-order") { compressEndpoints(vector<pair<int, int>>{{2, 1}}); }
            return 1; }}
    int bound = mode == "quick" ? 4 : mode == "full" ? 6 : 7;
    int arrays = 0;
    for (int n = 0, total = 1; n <= bound; ++n, total *= 5) {
        for (int mask = 0; mask < total; ++mask) {
            vector<lng> input(n); int code = mask;
            for (auto &x : input) { x = code % 5 - 2; code /= 5; }
            checkVector(input); ++arrays; }}
    cout << "PASS exhaustive arrays=" << arrays << " max_length=" << bound << '\n';
    customCases(); std::mt19937_64 gen(test_seed);
    int count = mode == "quick" ? 100 : mode == "full" ? 1000 : 10000;
    for (int i = 0; i < count; ++i) {
        int n = int(gen() % 101); vector<lng> input(n);
        for (auto &x : input) { x = i % 2 ? std::bit_cast<lng>(gen()) : lng(gen() % 11) - 5; }
        checkVector(input); }
    cout << "PASS random arrays=" << count << " total checks=" << checks << '\n';
}
