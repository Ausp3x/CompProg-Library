#include "../../06-Miscellaneous/13-knapsack.hpp"
#include "../../01-Core/05-modint.hpp"

// Test-only exact nonnegative integers. Production needs only +,-,*; Python
// independently verifies large decimal outputs because Boost is unavailable.
struct Big {
    static constexpr ulng BASE = 1000000000;
    vector<uint> digits;
    Big(lng x = 0) {
        if (x < 0) { throw std::runtime_error("negative Big construction"); }
        while (x) { digits.push_back(uint(x % BASE)); x /= BASE; }}
    void trim() { while (!digits.empty() && !digits.back()) { digits.pop_back(); } }
    friend bool operator==(const Big &, const Big &) = default;
    friend Big operator+(const Big &a, const Big &b) {
        Big out; int n = max(int(a.digits.size()), int(b.digits.size())); ulng carry = 0;
        for (int i = 0; i < n || carry; ++i) {
            ulng x = carry + (i < int(a.digits.size()) ? a.digits[i] : 0) + (i < int(b.digits.size()) ? b.digits[i] : 0);
            out.digits.push_back(uint(x % BASE)); carry = x / BASE;}
        return out;}
    friend Big operator-(const Big &a, const Big &b) {
        Big out; lng borrow = 0;
        for (int i = 0; i < int(a.digits.size()); ++i) {
            lng x = lng(a.digits[i]) - (i < int(b.digits.size()) ? b.digits[i] : 0) - borrow;
            borrow = x < 0; out.digits.push_back(uint(x + (borrow ? BASE : 0)));}
        if (borrow || b.digits.size() > a.digits.size()) { throw std::runtime_error("negative Big subtraction"); }
        out.trim(); return out;}
    friend Big operator*(const Big &a, const Big &b) {
        Big out; out.digits.resize(a.digits.size() + b.digits.size());
        for (int i = 0; i < int(a.digits.size()); ++i) {
            ulng carry = 0;
            for (int j = 0; j < int(b.digits.size()) || carry; ++j) {
                ulng x = out.digits[i + j] + carry + (j < int(b.digits.size()) ? ulng(a.digits[i]) * b.digits[j] : 0);
                out.digits[i + j] = uint(x % BASE); carry = x / BASE;}}
        out.trim(); return out;}
    Big &operator+=(const Big &b) { *this = *this + b; return *this; }
    Big &operator++() { return *this += Big(1); }
    friend int operator%(const Big &a, int modulus) {
        lng out = 0;
        for (int i = int(a.digits.size()) - 1; i >= 0; --i) { out = (out * BASE + a.digits[i]) % modulus; }
        return int(out);}
    friend Big operator<<(Big a, int shift) { while (shift--) { a = a * Big(2); } return a; }
    friend ostream &operator<<(ostream &out, const Big &a) {
        if (a.digits.empty()) { return out << 0; }
        out << a.digits.back();
        for (int i = int(a.digits.size()) - 2; i >= 0; --i) { out << std::setw(9) << std::setfill('0') << a.digits[i]; }
        return out;}
};
ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}
string number(lll x) {
    bool negative = x < 0; ulll value = negative ? ulll(0) - ulll(x) : ulll(x); string out;
    do { out += char('0' + value % 10); value /= 10; } while (value);
    if (negative) { out += '-'; } reverse(out.begin(), out.end()); return out;}
template<class T> string number(const T &x) { std::ostringstream out; out << x; return out.str(); }
template<class T> void checkEqual(const T &actual, const T &expected, const string &what) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << number(expected) << " actual=" << number(actual) << '\n';
        std::exit(1);}}
struct RefItem { int weight; lng value; int count; };
string show(const vector<RefItem> &items, int axis) {
    string out = "axis=" + std::to_string(axis) + " items=[";
    for (const auto &x : items) {
        out += '(' + std::to_string(x.weight) + ',' + std::to_string(x.value) + ',' + std::to_string(x.count) + "),";}
    return out + ']';}
struct RefState { bool reachable = false, unbounded = false, infinite_count = false; lll best = 0; Big count = 0; };

// Enumerate multiplicity vectors, not the production DP's transitions. A free
// unlimited item has one canonical representative and a separate infinity flag.
vector<RefState> capacityOracle(const vector<RefItem> &items, int cap) {
    vector<RefState> out(cap + 1); bool unbounded = false, infinite_count = false;
    for (const auto &item : items) {
        if (!item.weight && item.count == -1) {
            infinite_count = true; unbounded |= item.value > 0;}}
    auto visit = [&](auto &&visit, int i, int weight, lll value) -> void {
        if (i == int(items.size())) {
            auto &state = out[weight];
            if (!state.reachable || value > state.best) { state.best = value; }
            state.reachable = true; state.unbounded = unbounded; state.infinite_count = infinite_count; ++state.count;
            return;}
        auto item = items[i];
        lng count = item.count < 0 ? (item.weight ? (cap - weight) / item.weight : 0) : item.count;
        if (item.weight) { count = min(count, lng((cap - weight) / item.weight)); }
        for (lng used = 0; used <= count; ++used) {
            visit(visit, i + 1, weight + int(used * item.weight), value + lll(used) * item.value);}};
    visit(visit, 0, 0, 0); return out;}
vector<RefState> valueOracle(const vector<RefItem> &items, int target) {
    vector<RefState> out(target + 1);
    auto visit = [&](auto &&visit, int i, int value, lll weight) -> void {
        if (i == int(items.size())) {
            auto &state = out[value];
            if (!state.reachable || weight < state.best) { state.best = weight; }
            state.reachable = true; return;}
        auto item = items[i];
        lng count = item.count < 0 ? (item.value ? (target - value) / item.value : 0) : item.count;
        if (item.value) { count = min(count, (target - value) / item.value); }
        for (lng used = 0; used <= count; ++used) {
            visit(visit, i + 1, value + int(used * item.value), weight + lll(used) * item.weight);}};
    visit(visit, 0, 0, 0); return out;}

vector<KnapsackItem> convert(const vector<RefItem> &items) {
    vector<KnapsackItem> out;
    for (auto x : items) { out.push_back({x.weight, x.value, x.count}); }
    return out;}
KnapsackStatus status(const RefState &state, bool counting = false) {
    return !state.reachable ? KnapsackStatus::unreachable
         : (counting ? state.infinite_count : state.unbounded) ? KnapsackStatus::unbounded : KnapsackStatus::finite;}
void checkOptimum(const KnapsackResult &actual, const RefState &expected, const string &what) {
    check(actual.status == status(expected), what + " status");
    if (actual.status == KnapsackStatus::finite) { checkEqual(actual.value, expected.best, what + " value"); }
    if (actual.status == KnapsackStatus::unreachable) { check(actual.target == -1, what + " absent target sentinel"); }}
void checkWitness(const vector<int> &used, const vector<RefItem> &items, int target, lll optimum, bool value_axis) {
    check(used.size() == items.size(), "restored multiplicity vector length");
    lll weight = 0, value = 0;
    for (int i = 0; i < int(items.size()); ++i) {
        check(used[i] >= 0 && (items[i].count < 0 || used[i] <= items[i].count), "restored multiplicity bound item=" + std::to_string(i));
        weight += lll(used[i]) * items[i].weight; value += lll(used[i]) * items[i].value;}
    checkEqual(value_axis ? value : weight, lll(target), "restored exact target");
    checkEqual(value_axis ? weight : value, optimum, "restored objective");}
void checkCapacity(const vector<RefItem> &items, int cap) {
    ++cases; context = "capacity " + show(items, cap);
    auto input = convert(items); auto expected = capacityOracle(items, cap);
    KnapsackDP plain(cap, input), traced(cap, input, true);
    KnapsackCounts<Big> counts(cap, input); KnapsackCounts<ModInt<2>> parity(cap, input);
    auto feasible = knapsackFeasible(cap, input);
    check(int(feasible.size()) == cap + 1, "feasibility vector axis length");
    RefState prefix; int target = -1;
    for (int w = 0; w <= cap; ++w) {
        context = "capacity " + show(items, cap) + " query=" + std::to_string(w);
        auto result = plain.exact(w); checkOptimum(result, expected[w], "exact max");
        auto trace_result = traced.exact(w); checkOptimum(trace_result, expected[w], "traced exact max");
        check(bool(feasible[w]) == expected[w].reachable, "exact feasibility independent of values/count modulus");
        if (result.status == KnapsackStatus::finite) {
            check(result.target == w && trace_result.target == w, "exact finite target");
            checkWitness(traced.restore(w), items, w, result.value, false);}
        auto counted = counts.exact(w); auto reduced = parity.exact(w);
        check(counted.status == status(expected[w], true), "exact count status");
        check(reduced.status == status(expected[w], true), "modular exact count status");
        if (counted.status == KnapsackStatus::finite) {
            checkEqual(counted.count, expected[w].count, "exact multiplicity-vector count");
            checkEqual(int(reduced.count), int(expected[w].count % 2), "modular count including reachable zero");}
        if (expected[w].reachable) {
            if (!prefix.reachable || (!prefix.unbounded && (expected[w].unbounded || expected[w].best > prefix.best))) {
                prefix.best = expected[w].best; prefix.unbounded = expected[w].unbounded; target = w;}
            prefix.reachable = true; prefix.infinite_count |= expected[w].infinite_count; prefix.count += expected[w].count;}
        result = plain.atMost(w); checkOptimum(result, prefix, "at-most max");
        checkOptimum(traced.atMost(w), prefix, "traced at-most max");
        if (result.status == KnapsackStatus::finite) {
            check(result.target == target, "at-most smallest optimal weight");
            checkWitness(traced.restore(result.target), items, result.target, result.value, false);}
        counted = counts.atMost(w); reduced = parity.atMost(w);
        check(counted.status == status(prefix, true), "at-most count status");
        check(reduced.status == status(prefix, true), "modular at-most count status");
        if (counted.status == KnapsackStatus::finite) {
            checkEqual(counted.count, prefix.count, "at-most multiplicity-vector count");
            checkEqual(int(reduced.count), int(prefix.count % 2), "at-most modular count");}}
    auto copied = traced; checkOptimum(copied.atMost(cap), prefix, "copied DP snapshot");
    if (prefix.reachable && !prefix.unbounded) { checkWitness(copied.restore(target), items, target, prefix.best, false); }
    auto copied_counts = counts; check(copied_counts.atMost(cap).status == status(prefix, true), "copied count status");
    if (!prefix.infinite_count) { checkEqual(copied_counts.atMost(cap).count, prefix.count, "copied count snapshot"); }
    check(int(input.size()) == int(items.size()), "input length preserved");
    for (int i = 0; i < int(items.size()); ++i) {
        check(input[i].weight == items[i].weight && input[i].value == items[i].value && input[i].count == items[i].count,
              "input items preserved");}}
void checkValue(const vector<RefItem> &items, int axis) {
    ++cases; context = "value " + show(items, axis);
    auto expected = valueOracle(items, axis); auto input = convert(items);
    ValueKnapsackDP plain(axis, input), traced(axis, input, true);
    for (int value = 0; value <= axis; ++value) {
        context = "value " + show(items, axis) + " query=" + std::to_string(value);
        auto result = plain.minWeight(value); checkOptimum(result, expected[value], "exact-value minimum weight");
        auto trace_result = traced.minWeight(value); checkOptimum(trace_result, expected[value], "traced minimum weight");
        if (result.status == KnapsackStatus::finite) {
            check(result.target == value && trace_result.target == value, "minimum-weight target");
            checkWitness(traced.restore(value), items, value, result.value, true);}}
    vector<lng> budgets{0, 1, INT_MAX, INT64_MAX};
    for (const auto &state : expected) {
        if (state.reachable && state.best <= INT64_MAX) {
            budgets.push_back(lng(state.best)); if (state.best) { budgets.push_back(lng(state.best - 1)); }}}
    for (lng budget : budgets) {
        int best = 0;
        for (int value = 0; value <= axis; ++value) {
            if (expected[value].reachable && expected[value].best <= budget) { best = value; }}
        checkEqual(plain.bestWithin(budget), best, "budget best within truncated value axis");
        checkEqual(traced.bestWithin(budget), best, "traced budget best");}
    auto copied = traced;
    checkOptimum(copied.minWeight(axis), expected.back(), "copied minimum-weight snapshot");
    if (expected.back().reachable) { checkWitness(copied.restore(axis), items, axis, expected.back().best, true); }}

void exhaustiveCases(const string &mode) {
    vector<RefItem> capacity_alphabet{{0, -2, -1}, {0, 0, -1}, {0, 3, -1}, {0, -1, 2},
        {0, 2, 2}, {1, -2, 1}, {1, 0, 2}, {1, 3, -1}, {2, -1, 2}, {2, 3, 1}, {3, 2, 0}, {2, 0, 5}};
    vector<RefItem> value_alphabet{{0, 0, -1}, {0, 1, -1}, {0, 2, 2}, {1, 0, 2}, {2, 0, -1},
        {1, 1, 1}, {2, 1, 2}, {3, 2, -1}, {0, 3, 0}, {2, 3, 1}, {1, 1, 5}};
    int bound = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
    int axis = mode == "quick" ? 4 : mode == "full" ? 6 : 8, capacity_inputs = 0, value_inputs = 0;
    for (bool value_axis : {false, true}) {
        const auto &alphabet = value_axis ? value_alphabet : capacity_alphabet;
        for (int n = 0; n <= bound; ++n) {
            vector<RefItem> items(n);
            auto visit = [&](auto &&visit, int i) -> void {
                if (i == n) {
                    if (value_axis) { checkValue(items, axis); ++value_inputs; }
                    else { checkCapacity(items, axis); ++capacity_inputs; }
                    return;}
                for (auto item : alphabet) { items[i] = item; visit(visit, i + 1); }};
            visit(visit, 0);}}
    cout << "PASS exhaustive item-alphabet products max_items=" << bound << " axis=" << axis
         << " capacity_inputs=" << capacity_inputs << " value_inputs=" << value_inputs << '\n';
    for (int weight = 0; weight <= 4; ++weight) { for (lng value = -3; value <= 3; ++value) {
        for (int count : {-1, 0, 1, 2, 5}) { for (int cap = 0; cap <= 7; ++cap) {
            checkCapacity({{weight, value, count}}, cap);
            if (value >= 0) { checkValue({{weight, value, count}}, cap); }}}}}
    cout << "PASS exhaustive single-item domains weight=0..4 value=-3..3 count=-1,0,1,2,5 axis=0..7\n";}

void regressionCases() {
    for (const vector<RefItem> &items : vector<vector<RefItem>>{
        {}, {{1, 4, 5}}, {{1, 2, 5}, {1, 2, 5}}, {{0, -9, 2}, {0, 4, 5}, {2, 1, 2}},
        {{0, 1, -1}, {2, -3, 1}}, {{0, -1, -1}, {2, 4, 2}}, {{0, 0, -1}, {2, 4, 1}},
        {{2, 10, 2}, {3, 1, 1}, {1, 0, 0}}, {{2, 5, 1}, {2, 6, 2}, {1, -2, 1}},
        {{1, INT64_MIN, 1}}, {{1, INT64_MAX, 2}, {1, INT64_MIN, 2}},
        {{INT_MAX, INT64_MIN, INT_MAX}}, {{0, 4, 0}, {0, INT64_MIN, 2}, {1, INT64_MAX, 2}}}) {
        checkCapacity(items, 7); checkCapacity(items, 0);}
    for (const vector<RefItem> &items : vector<vector<RefItem>>{
        {}, {{0, 2, -1}}, {{0, 0, -1}, {2, 3, 1}}, {{1, 1, 5}, {2, 1, 5}},
        {{INT_MAX, 1, 3}}, {{1, INT64_MAX, INT_MAX}}, {{0, INT64_MAX, -1}},
        {{0, 0, 2}, {INT_MAX, 1, 2}, {0, 2, 1}}, {{1, 2, 2}, {2, 1, 1}, {0, 0, 0}}}) {
        checkValue(items, 7); checkValue(items, 0);}
    context = "bounded count=5 must not duplicate chunk subsets 1,2,2";
    KnapsackCounts<Big> count_five(5, {{1, 4, 5}});
    for (int w = 0; w <= 5; ++w) { checkEqual(count_five.exact(w).count, Big(1), "each bounded multiplicity counted exactly once"); }
    context = "default item count is zero-one and at-most tie picks smallest weight";
    KnapsackDP defaults(3, {{1, 0}, {2, 0}}, true);
    check(defaults.atMost(3).target == 0 && defaults.atMost(3).value == 0, "smallest-weight zero tie");
    check(defaults.exact(3).status == KnapsackStatus::finite, "default zero-one items");
    checkWitness(defaults.restore(3), {{1, 0, 1}, {2, 0, 1}}, 3, 0, false);
    context = "modulus-one counts keep reachable-zero distinct from absence";
    KnapsackCounts<ModInt<1>> zero_modulus(3, {{2, 1, 1}});
    check(zero_modulus.exact(0).status == KnapsackStatus::finite && int(zero_modulus.exact(0).count) == 0, "empty multiplicity vector modulo one");
    check(zero_modulus.exact(1).status == KnapsackStatus::unreachable, "odd target absent modulo one");
    check(zero_modulus.exact(2).status == KnapsackStatus::finite && int(zero_modulus.exact(2).count) == 0, "nonempty reachable zero modulo one");
    context = "free bounded INT_MAX multiplicity and full signed 64-bit values";
    vector<RefItem> extreme{{0, INT64_MAX, INT_MAX}, {0, INT64_MAX, INT_MAX}, {0, INT64_MIN, INT_MAX}};
    auto input = convert(extreme); KnapsackDP huge(0, input, true);
    lll best = 2 * lll(INT64_MAX) * INT_MAX;
    checkEqual(huge.exact(0).value, best, "wide objective exceeds signed 64-bit");
    checkWitness(huge.restore(0), extreme, 0, best, false);
    KnapsackCounts<Big> huge_counts(0, input); Big factor = Big(INT_MAX) + 1;
    checkEqual(huge_counts.exact(0).count, Big(factor * factor * factor), "zero-weight count factors INT_MAX+1");
    KnapsackCounts<ModInt<2>> huge_parity(0, input);
    check(huge_parity.exact(0).status == KnapsackStatus::finite && int(huge_parity.exact(0).count) == 0, "large zero-weight count factors modulo two");
    context = "repeated constructors and copied state remain independent";
    KnapsackDP first(5, {{2, 3, 2}}, true); auto copied = first;
    KnapsackDP second(5, {{0, 1, -1}}); ValueKnapsackDP value_second(5, {{0, 1, -1}});
    check(first.exact(4).status == KnapsackStatus::finite && first.exact(4).value == 6, "first state unchanged after second constructor");
    check(copied.exact(4).value == 6 && copied.restore(4) == vector<int>{2}, "copy retains independent trace");
    check(second.exact(0).status == KnapsackStatus::unbounded && second.exact(1).status == KnapsackStatus::unreachable, "unbounded versus unreachable");
    check(value_second.bestWithin(0) == 5, "value query respects explicit finite truncation");
    cout << "PASS zero/negative/free/infinite/count/chunk/trace/copy/default/modular and full-width regressions\n";}

void randomCases(const string &mode) {
    std::mt19937_64 gen(test_seed); int rounds = mode == "quick" ? 100 : mode == "full" ? 2000 : 10000;
    const int multiplicities[]{-1, 0, 1, 2, 5};
    for (int round = 0; round < rounds; ++round) {
        int n = int(gen() % 8), cap = int(gen() % 11); vector<RefItem> items;
        for (int i = 0; i < n; ++i) {
            items.push_back({int(gen() % 8), lng(gen() % 11) - 5, multiplicities[gen() % 5]});}
        checkCapacity(items, cap);
        for (auto &item : items) { item.value = abs(item.value); }
        checkValue(items, cap);}
    cout << "PASS seeded recursive multiplicity oracles rounds=" << rounds << " max_items=7 max_axis=10\n";}

void largeCases(const string &mode) {
    int n = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
    context = "many irrelevant items n=" + std::to_string(n);
    vector<KnapsackItem> items(n, {2, 2, INT_MAX});
    KnapsackDP many(1, items); ValueKnapsackDP many_values(1, items); KnapsackCounts<Big> many_counts(1, items);
    auto feasible = knapsackFeasible(1, items);
    check(many.exact(0).value == 0 && many.exact(1).status == KnapsackStatus::unreachable, "many items capacity axis");
    check(many_values.minWeight(0).value == 0 && many_values.minWeight(1).status == KnapsackStatus::unreachable, "many items value axis");
    check(many_counts.exact(0).count == 1 && many_counts.exact(1).status == KnapsackStatus::unreachable, "many items count axis");
    check(feasible == vector<char>({1, 0}), "many items feasibility axis");
    int axis = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
    context = "wide capacity/value axes=" + std::to_string(axis);
    vector<RefItem> wide_items{{1, 2, -1}, {5, 11, 2}, {0, -3, 2}};
    KnapsackDP wide(axis, convert(wide_items)), wide_trace(axis, convert(wide_items), true);
    KnapsackCounts<Big> wide_counts(axis, convert(wide_items));
    for (int w = 0; w <= axis; ++w) {
        int upgrades = min(2, w / 5); lll optimum = 2 * lll(w) + upgrades;
        checkEqual(wide.exact(w).value, optimum, "wide capacity analytic optimum");
        checkEqual(wide_counts.exact(w).count, Big(3 * (upgrades + 1)), "wide bounded-count analytic formula");}
    checkWitness(wide_trace.restore(axis), wide_items, axis, wide.exact(axis).value, false);
    vector<RefItem> value_items{{2, 1, -1}, {3, 2, 2}, {2, 0, -1}};
    ValueKnapsackDP values(axis, convert(value_items), true);
    for (int v = 0; v <= axis; ++v) {
        checkEqual(values.minWeight(v).value, 2 * lll(v) - min(2, v / 2), "wide value analytic minimum weight");}
    checkWitness(values.restore(axis), value_items, axis, values.minWeight(axis).value, true);
    int bits = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
    context = "exact arbitrary-size count 2^" + std::to_string(bits);
    KnapsackCounts<Big> powers(0, vector<KnapsackItem>(bits, {0, -1, 1}));
    checkEqual(powers.exact(0).count, Big(Big(1) << bits), "arbitrary precision count exceeds fixed-width words");
    cout << "PASS many items=" << n << " wide axis=" << axis << " exact count=2^" << bits << '\n';}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--oracle-counts") {
            int n, cap;
            while (cin >> n >> cap) {
                vector<KnapsackItem> items(n);
                for (auto &item : items) { cin >> item.weight >> item.count; item.value = -1; }
                KnapsackCounts<Big> result(cap, items);
                for (int w = 0; w <= cap; ++w) {
                    auto exact = result.exact(w), prefix = result.atMost(w);
                    cout << int(exact.status) << ' ' << exact.count << ' ' << int(prefix.status) << ' ' << prefix.count << '\n';}}
            return 0;}
        if (arg == "--invalid") {
            string probe = argv[++i];
            if (probe == "capacity-negative") { KnapsackDP(-1, {}); }
            if (probe == "capacity-intmax") { KnapsackDP(INT_MAX, {}); }
            if (probe == "capacity-weight") { KnapsackDP(0, {{-1, 2, 0}}); }
            if (probe == "capacity-count") { KnapsackDP(0, {{INT_MAX, 2, -2}}); }
            if (probe == "capacity-exact-negative") { KnapsackDP(0, {}).exact(-1); }
            if (probe == "capacity-exact-large") { KnapsackDP(0, {}).exact(1); }
            if (probe == "capacity-atmost-negative") { KnapsackDP(0, {}).atMost(-1); }
            if (probe == "capacity-atmost-large") { KnapsackDP(0, {}).atMost(1); }
            if (probe == "capacity-restore-disabled") { KnapsackDP(0, {}).restore(0); }
            if (probe == "capacity-restore-unreachable") { KnapsackDP(1, {}, true).restore(1); }
            if (probe == "capacity-restore-unbounded") { KnapsackDP(0, {{0, 1, -1}}, true).restore(0); }
            if (probe == "capacity-restore-negative") { KnapsackDP(0, {}, true).restore(-1); }
            if (probe == "capacity-restore-large") { KnapsackDP(0, {}, true).restore(1); }
            if (probe == "value-negative") { ValueKnapsackDP(-1, {}); }
            if (probe == "value-intmax") { ValueKnapsackDP(INT_MAX, {}); }
            if (probe == "value-weight") { ValueKnapsackDP(0, {{-1, INT64_MAX, 0}}); }
            if (probe == "value-count") { ValueKnapsackDP(0, {{0, INT64_MAX, -2}}); }
            if (probe == "value-negative-item") { ValueKnapsackDP(0, {{0, -1, 0}}); }
            if (probe == "value-query-negative") { ValueKnapsackDP(0, {}).minWeight(-1); }
            if (probe == "value-query-large") { ValueKnapsackDP(0, {}).minWeight(1); }
            if (probe == "value-budget-negative") { ValueKnapsackDP(0, {}).bestWithin(-1); }
            if (probe == "value-restore-disabled") { ValueKnapsackDP(0, {}).restore(0); }
            if (probe == "value-restore-unreachable") { ValueKnapsackDP(1, {}, true).restore(1); }
            if (probe == "value-restore-negative") { ValueKnapsackDP(0, {}, true).restore(-1); }
            if (probe == "value-restore-large") { ValueKnapsackDP(0, {}, true).restore(1); }
            if (probe == "feasible-negative") { knapsackFeasible(-1, {}); }
            if (probe == "feasible-intmax") { knapsackFeasible(INT_MAX, {}); }
            if (probe == "feasible-weight") { knapsackFeasible(0, {{-1, 0, 0}}); }
            if (probe == "feasible-count") { knapsackFeasible(0, {{INT_MAX, 0, -2}}); }
            if (probe == "counts-negative") { KnapsackCounts<Big>(-1, {}); }
            if (probe == "counts-intmax") { KnapsackCounts<Big>(INT_MAX, {}); }
            if (probe == "counts-weight") { KnapsackCounts<Big>(0, {{-1, 0, 0}}); }
            if (probe == "counts-count") { KnapsackCounts<Big>(0, {{INT_MAX, 0, -2}}); }
            if (probe == "counts-exact-negative") { KnapsackCounts<Big>(0, {}).exact(-1); }
            if (probe == "counts-exact-large") { KnapsackCounts<Big>(0, {}).exact(1); }
            if (probe == "counts-atmost-negative") { KnapsackCounts<Big>(0, {}).atMost(-1); }
            if (probe == "counts-atmost-large") { KnapsackCounts<Big>(0, {}).atMost(1); }
            return 1;}}
    exhaustiveCases(mode); regressionCases(); randomCases(mode); largeCases(mode);
    cout << "PASS knapsack cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
