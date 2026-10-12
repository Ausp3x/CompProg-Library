#include "../../06-Miscellaneous/04-compression.hpp"

ulng test_seed = 0;
string context;
lng checks = 0;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
                    << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}

void checkVector(const vector<lng> &input) {
    context = "input="; for (lng x : input) { context += std::to_string(x) + ','; }
    vector<pair<lng, lng>> intervals; set<lng> ends;
    for (int i = 0; i + 1 < int(input.size()); ++i) {
        lng l = min(input[i], input[i + 1]), r = max(input[i], input[i + 1]);
        intervals.push_back({l, r}); ends.insert(l); ends.insert(r);}
    auto endpoints = compressEndpoints(intervals);
    check(endpoints.v == vector<lng>(ends.begin(), ends.end()), "compressEndpoints vs ordered endpoint set");
    vector<int> stable = stableRanks(input);
    for (int i = 0; i < int(input.size()); ++i) {
        int want = 0;
        for (int j = 0; j < int(input.size()); ++j) { want += input[j] < input[i] || (input[j] == input[i] && j < i); }
        check(stable[i] == want, "stableRanks counting oracle i=" + std::to_string(i));}
    EncounterCompression<lng> e; vector<lng> seen;
    for (lng x : input) {
        auto it = std::find(seen.begin(), seen.end(), x);
        int id = int(it - seen.begin()); if (it == seen.end()) { seen.push_back(x); }
        check(e.add(x) == id && e.id(x) == id && e.value(id) == x, "first-encounter linear oracle");
        for (int i = 0; i < int(seen.size()); ++i) { check(e.id(seen[i]) == i, "append preserves prior IDs"); }}
    check(e.values == seen, "first-encounter inverse");
    e.clear(); check(e.size() == 0 && e.id(0) == -1 && e.add(42) == 0, "clear/reset");}

struct LengthOrder { bool operator()(const string &a, const string &b) const { return a.size() < b.size(); } };
struct Direction {
    bool down;
    bool operator()(int a, int b) const { return down ? a > b : a < b; }
};

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
    cout << "PASS custom comparators/equivalence, proxy storage, offline endpoint slabs\n";}

void oracle() {
    int n, q;
    while (cin >> n >> q) {
        vector<lng> input(n); for (auto &x : input) { cin >> x; }
        vector<pair<lng, lng>> intervals(q); for (auto &[l, r] : intervals) { cin >> l >> r; }
        EncounterCompression<lng> e;
        for (lng x : input) { cout << e.add(x) << ' '; } cout << '\n';
        for (int x : stableRanks(input)) { cout << x << ' '; } cout << '\n';
        auto c = compressEndpoints(intervals);
        cout << c.size(); for (lng x : c.v) { cout << ' ' << x; } cout << '\n';}}

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
            if (p == "endpoint-order") { compressEndpoints(vector<pair<int, int>>{{2, 1}}); }
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
        for (auto &x : input) { x = i % 2 ? std::bit_cast<lng>(gen()) : lng(gen() % 11) - 5; }
        checkVector(input);}
    cout << "PASS random arrays=" << count << " total checks=" << checks << '\n';}
