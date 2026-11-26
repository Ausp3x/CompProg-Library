#include "../../06-Miscellaneous/13-knapsack.hpp"
#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/06-modintmini.hpp"

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
ulng low64(const Big &a) {
    ulng out = 0;
    for (int i = int(a.digits.size()) - 1; i >= 0; --i) { out = out * Big::BASE + a.digits[i]; }
    return out;}
template<class M> int residue(const M &x) { return int(x.val()); }
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
struct RefItem { lng weight; lng value; int count; };
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
    KnapsackCounts<ModInt<998244353>> prime(cap, input); KnapsackCounts<ModIntMini<998244353>> prime_mini(cap, input);
    KnapsackCounts<ModIntMini<2>> parity_mini(cap, input); KnapsackCounts<ulng> word(cap, input);
    // Full and mini modular types and wrapping ulng against the exact count, at one weight or prefix.
    auto residues = [&](int w, bool at_most, const RefState &state) {
        auto get = [&](const auto &table) { return at_most ? table.atMost(w) : table.exact(w); };
        auto a = get(prime); auto b = get(prime_mini); auto c = get(parity_mini); auto d = get(word);
        auto st = status(state, true); string what = at_most ? "at-most count" : "exact count";
        check(a.status == st && b.status == st && c.status == st && d.status == st, what + " status for ModInt/ModIntMini/ulng");
        if (st == KnapsackStatus::finite) {
            checkEqual(residue(a.count), state.count % 998244353, what + " ModInt<998244353>");
            checkEqual(residue(b.count), state.count % 998244353, what + " ModIntMini<998244353>");
            checkEqual(residue(c.count), state.count % 2, what + " ModIntMini<2>");
            checkEqual(d.count, low64(state.count), what + " ulng modulo 2^64");}};
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
        residues(w, false, expected[w]);
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
            checkEqual(int(reduced.count), int(prefix.count % 2), "at-most modular count");}
        residues(w, true, prefix);}
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
        {{0, 0, 2}, {INT_MAX, 1, 2}, {0, 2, 1}}, {{1, 2, 2}, {2, 1, 1}, {0, 0, 0}},
        {{INT64_MAX, 3, 2}, {lng(1) << 40, 1, -1}, {3000000000, 2, 1}}, {{INT64_MAX, 1, -1}, {INT64_MAX, 2, INT_MAX}},
        {{lng(INT_MAX) + 1, 1, 5}, {0, 2, 1}, {INT64_MAX - 1, 4, 1}}}) {
        checkValue(items, 7); checkValue(items, 0);}
    for (const vector<RefItem> &items : vector<vector<RefItem>>{
        {{lng(1) << 40, 5, 1}, {1, 1, 1}}, {{INT64_MAX, 9, -1}, {2, 3, 2}}, {{lng(INT_MAX) + 1, -1, INT_MAX}, {0, 2, 1}}}) {
        checkCapacity(items, 7); checkCapacity(items, 0);}
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
        checkValue(items, cap);
        for (auto &item : items) { item.weight = lng(gen() >> 1); }
        checkValue(items, cap);}
    cout << "PASS seeded recursive multiplicity oracles rounds=" << rounds << " max_items=7 max_axis=10 value-axis weights<2^63\n";}

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
    KnapsackCounts<Big> wide_counts(axis, convert(wide_items)); KnapsackCounts<ulng> wide_words(axis, convert(wide_items));
    KnapsackCounts<ModIntMini<998244353>> wide_mini(axis, convert(wide_items));
    for (int w = 0; w <= axis; ++w) {
        int upgrades = min(2, w / 5); lll optimum = 2 * lll(w) + upgrades;
        checkEqual(wide.exact(w).value, optimum, "wide capacity analytic optimum");
        checkEqual(wide_counts.exact(w).count, Big(3 * (upgrades + 1)), "wide bounded-count analytic formula");
        checkEqual(wide_words.exact(w).count, ulng(3 * (upgrades + 1)), "wide bounded-count ulng");
        checkEqual(residue(wide_mini.exact(w).count), 3 * (upgrades + 1), "wide bounded-count ModIntMini");}
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

// Bounded layers: counts 1, 2, 3, 5, bound - 1, bound and bound + 1 for every step 1..cap+1, alone and among other layers.
void boundedCountCases(const string &mode) {
    int top = mode == "quick" ? 10 : mode == "full" ? 16 : 24, inputs = 0;
    for (int cap = 0; cap <= top; ++cap) {
        for (int step = 1; step <= cap + 1; ++step) {
            int bound = cap / step;
            for (int count : {1, 2, 3, 5, bound - 1, bound, bound + 1}) {
                if (count < 0) { continue; }
                RefItem item{step, 1, count};
                for (const auto &items : vector<vector<RefItem>>{
                    {item}, {{1, 0, 1}, item}, {item, {2, 0, 3}, {0, 0, 1}}, {{1, 0, -1}, item, {3, 0, 2}}}) {
                    checkCapacity(items, cap); ++inputs;}}}}
    std::mt19937_64 gen(test_seed ^ 0xb0dedULL); int rounds = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
    for (int round = 0; round < rounds; ++round) {
        int cap = int(gen() % 17); vector<RefItem> items(gen() % 6);
        for (auto &item : items) {
            int weight = 1 + int(gen() % ulng(cap + 1)), bound = cap / weight;
            int choices[]{-1, 0, 1, 2, 3, 5, max(bound - 1, 0), bound};
            item = {weight, lng(gen() % 7) - 3, choices[gen() % 8]};}
        checkCapacity(items, cap);}
    cout << "PASS bounded count layers caps=0.." << top << " inputs=" << inputs << " mixed 0/1/bounded/unlimited rounds=" << rounds << '\n';}

string show(const vector<int> &values, int total) {
    string out = "total=" + std::to_string(total) + " values=[";
    for (int x : values) { out += std::to_string(x) + ','; }
    return out + ']';}
template<class T> void checkTable(const vector<T> &actual, const vector<T> &expected, const string &what) {
    checkEqual(actual.size(), expected.size(), what + " table length");
    for (size_t i = 0; i < actual.size(); ++i) {
        if (!(actual[i] == expected[i])) { context += " at=" + std::to_string(i); checkEqual(actual[i], expected[i], what); }}
    checks += lng(actual.size());}

// Fewest coins by enumerating coin multiplicity vectors; a zero coin adds a coin and no total, so zero copies suffice.
vector<int> coinMultisetOracle(const vector<int> &coins, int total) {
    vector<int> out(total + 1, -1);
    auto visit = [&](auto &&visit, int i, int sum, int used) -> void {
        if (i == int(coins.size())) {
            if (out[sum] < 0 || used < out[sum]) { out[sum] = used; }
            return;}
        int most = coins[i] ? (total - sum) / coins[i] : 0;
        for (int k = 0; k <= most; ++k) { visit(visit, i + 1, sum + k * coins[i], used + k); }};
    visit(visit, 0, 0, 0); return out;}
// Fewest coins as unit-edge shortest paths from total 0.
vector<int> coinBfsOracle(const vector<int> &coins, int total) {
    vector<int> dist(total + 1, -1); queue<int> todo; dist[0] = 0; todo.push(0);
    while (!todo.empty()) {
        int at = todo.front(); todo.pop();
        for (int c : coins) {
            if (c > 0 && c <= total - at && dist[at + c] < 0) { dist[at + c] = dist[at] + 1; todo.push(at + c); }}}
    return dist;}
void checkCoins(const vector<int> &coins, int total, bool enumerate) {
    ++cases; context = "minimumCoins " + show(coins, total);
    auto expected = coinBfsOracle(coins, total); auto copy = coins;
    if (enumerate) { checkTable(coinMultisetOracle(coins, total), expected, "BFS oracle against multiset enumeration"); }
    checkTable(minimumCoins(total, coins), expected, "fewest coins per total");
    check(copy == coins, "coin list preserved");}

// Ordered sequences of listed part indices, enumerated depth first; each prefix is one sequence of its sum.
vector<Big> sequenceOracle(const vector<int> &parts, int total) {
    vector<lng> raw(total + 1);
    auto visit = [&](auto &&visit, int sum) -> void {
        ++raw[sum];
        for (int p : parts) { if (p <= total - sum) { visit(visit, sum + p); } }};
    visit(visit, 0);
    return vector<Big>(raw.begin(), raw.end());}
// Ordered sequences counted by their number of parts: layer k holds the k-part sequences.
vector<Big> lengthOracle(const vector<int> &parts, int total) {
    vector<Big> out(total + 1), layer(total + 1); out[0] = layer[0] = Big(1);
    for (int k = 1; k <= total; ++k) {
        vector<Big> next(total + 1); bool any = false;
        for (int s = 0; s <= total; ++s) {
            if (layer[s].digits.empty()) { continue; }
            for (int p : parts) { if (p <= total - s) { next[s + p] += layer[s]; any = true; } }}
        if (!any) { break; }
        layer = next;
        for (int s = 0; s <= total; ++s) { out[s] += layer[s]; }}
    return out;}
void checkCompositions(const vector<int> &parts, int total, bool enumerate) {
    ++cases; context = "compositionCountTable " + show(parts, total);
    auto expected = lengthOracle(parts, total); auto copy = parts;
    if (enumerate) { checkTable(sequenceOracle(parts, total), expected, "length oracle against sequence enumeration"); }
    checkTable(compositionCountTable<Big>(total, parts), expected, "exact ordered compositions");
    vector<int> prime, two; vector<ulng> word;
    for (const auto &x : expected) { prime.push_back(x % 998244353); two.push_back(x % 2); word.push_back(low64(x)); }
    vector<int> got_prime, got_two, mini_prime, mini_two;
    for (auto x : compositionCountTable<ModInt<998244353>>(total, parts)) { got_prime.push_back(int(x)); }
    for (auto x : compositionCountTable<ModInt<2>>(total, parts)) { got_two.push_back(int(x)); }
    for (auto x : compositionCountTable<ModIntMini<998244353>>(total, parts)) { mini_prime.push_back(residue(x)); }
    for (auto x : compositionCountTable<ModIntMini<2>>(total, parts)) { mini_two.push_back(residue(x)); }
    checkTable(got_prime, prime, "ordered compositions modulo 998244353");
    checkTable(got_two, two, "ordered compositions modulo 2");
    checkTable(mini_prime, prime, "ordered compositions ModIntMini<998244353>");
    checkTable(mini_two, two, "ordered compositions ModIntMini<2>");
    checkTable(compositionCountTable<ulng>(total, parts), word, "ordered compositions modulo 2^64");
    check(copy == parts, "part list preserved");}

void coinCompositionCases(const string &mode) {
    int bound = mode == "quick" ? 2 : mode == "full" ? 3 : 4, axis = mode == "quick" ? 6 : mode == "full" ? 8 : 10;
    int coin_inputs = 0, part_inputs = 0;
    for (bool coins : {true, false}) {
        vector<int> alphabet = coins ? vector<int>{0, 1, 2, 3, 5, axis + 1} : vector<int>{1, 2, 3, 5, axis + 1};
        int top = coins ? axis : axis - 1;
        for (int n = 0; n <= bound; ++n) {
            vector<int> values(n);
            auto visit = [&](auto &&visit, int i) -> void {
                if (i == n) {
                    for (int total = 0; total <= top; ++total) {
                        if (coins) { checkCoins(values, total, true); ++coin_inputs; }
                        else { checkCompositions(values, total, total == top); ++part_inputs; }}
                    return;}
                for (int x : alphabet) { values[i] = x; visit(visit, i + 1); }};
            visit(visit, 0);}}
    cout << "PASS exhaustive coin/part alphabet products max_values=" << bound << " totals=0.." << axis
         << " coin_inputs=" << coin_inputs << " part_inputs=" << part_inputs << '\n';
    context = "hand-computed coin and composition tables";
    checkTable(minimumCoins(0, {}), vector<int>{0}, "empty coins total 0");
    checkTable(minimumCoins(3, {0, 0}), vector<int>{0, -1, -1, -1}, "zero coins are useless");
    checkTable(minimumCoins(6, {4, 3, 1}), vector<int>{0, 1, 2, 1, 1, 2, 2}, "greedy-failing coin system");
    checkTable(minimumCoins(4, {INT_MAX, 2, 2}), vector<int>{0, -1, 1, -1, 2}, "INT_MAX coin and duplicates");
    checkTable(compositionCountTable<lng>(0, {}), vector<lng>{1}, "empty parts total 0");
    checkTable(compositionCountTable<lng>(3, {}), vector<lng>{1, 0, 0, 0}, "empty parts");
    checkTable(compositionCountTable<lng>(2, {1, 1}), vector<lng>{1, 2, 4}, "duplicate part values are distinct types");
    checkEqual(compositionCountTable<lng>(9, {5, 2, 3})[9], lng(8), "CSES 1635 sample");
    checkTable(compositionCountTable<lng>(2, {INT_MAX, 3}), vector<lng>{1, 0, 0}, "parts above total");
    for (const auto &[values, total] : vector<pair<vector<int>, int>>{
        {{}, 0}, {{}, 7}, {{0}, 5}, {{7}, 3}, {{INT_MAX}, 4}, {{2, 2}, 9}, {{1, 3, 4}, 12}, {{5, 10}, 23},
        {{6, 9, 20}, 60}, {{0, INT_MAX, 1, 1}, 9}}) {
        checkCoins(values, total, true);}
    for (const auto &[values, total] : vector<pair<vector<int>, int>>{
        {{}, 0}, {{}, 7}, {{7}, 3}, {{INT_MAX}, 4}, {{1, 1}, 12}, {{2, 1, 2}, 11}, {{3, 5}, 30}, {{9, 1, 4, 1}, 10}}) {
        checkCompositions(values, total, true);}
    std::mt19937_64 gen(test_seed ^ 0x5eedc0175ULL); int rounds = mode == "quick" ? 100 : mode == "full" ? 2000 : 10000;
    for (int round = 0; round < rounds; ++round) {
        vector<int> values(gen() % 7); int total = int(gen() % 31);
        for (int &x : values) { x = int(gen() % 14); }
        checkCoins(values, total, values.size() <= 4 && total <= 20);
        values.resize(gen() % 5); total = int(gen() % 21);
        for (int &x : values) { x = 1 + int(gen() % 12); }
        checkCompositions(values, total, false);}
    cout << "PASS seeded coin/composition oracles rounds=" << rounds << " coins<=6 values<=13 totals<=30, parts<=4 values<=12 totals<=20\n";
    int large = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
    context = "minimumCoins large total=" + std::to_string(large);
    vector<int> coins{7, 11, 0, 13, 7, large + 1, INT_MAX}; ++cases;
    checkTable(minimumCoins(large, coins), coinBfsOracle(coins, large), "large fewest coins against BFS");
    vector<int> many(large, large + 1); many.push_back(3);
    auto thirds = minimumCoins(large, many); vector<int> expected_thirds(large + 1);
    for (int w = 0; w <= large; ++w) { expected_thirds[w] = w % 3 ? -1 : w / 3; }
    ++cases; checkTable(thirds, expected_thirds, "many unusable coins and one coin 3");
    context = "compositionCountTable large total=" + std::to_string(large);
    const ulng p = 998244353;
    vector<int> fibonacci, powers;
    for (int w = 0; w <= large; ++w) {
        // Fast doubling gives (F(m), F(m+1)) mod p; parts {1,2} give F(w+1) sequences.
        ulng a = 0, b = 1;
        for (int bit = 20; bit >= 0; --bit) {
            ulng c = a * ((2 * b + p - a) % p) % p, d = (a * a + b * b) % p;
            if ((w + 1) >> bit & 1) { a = d; b = (c + d) % p; } else { a = c; b = d; }}
        fibonacci.push_back(int(a));}
    for (ulng w = 0, x = 1; w <= ulng(large); ++w, x = x * 2 % p) { powers.push_back(int(x)); }
    vector<int> got; ++cases;
    for (auto x : compositionCountTable<ModInt<998244353>>(large, {2, 1, large + 1})) { got.push_back(int(x)); }
    checkTable(got, fibonacci, "parts {1,2} give Fibonacci modulo 998244353");
    got.clear(); ++cases;
    for (auto x : compositionCountTable<ModIntMini<998244353>>(large, {2, 1, large + 1})) { got.push_back(residue(x)); }
    checkTable(got, fibonacci, "parts {1,2} give Fibonacci in ModIntMini<998244353>");
    got.clear(); ++cases;
    many.assign(large, large + 1); many.push_back(1); many.push_back(1);
    for (auto x : compositionCountTable<ModInt<998244353>>(large, many)) { got.push_back(int(x)); }
    checkTable(got, powers, "many unusable parts and parts {1,1} give 2^w modulo 998244353");
    vector<ulng> words; ++cases;
    for (int w = 0; w <= large; ++w) { words.push_back(w < 64 ? ulng(1) << w : 0); }
    checkTable(compositionCountTable<ulng>(large, {1, 1}), words, "parts {1,1} wrap modulo 2^64");
    cout << "PASS large coin/composition totals=" << large << " unusable values=" << large << '\n';}

// Every sign vector enumerated: signed sum -> number of vectors.
map<lng, lng> signOracle(const vector<int> &values) {
    map<lng, lng> out; int n = int(values.size());
    for (lng mask = 0; mask < (lng(1) << n); ++mask) {
        lng sum = 0;
        for (int i = 0; i < n; ++i) { sum += mask >> i & 1 ? lng(values[i]) : -lng(values[i]); }
        ++out[sum];}
    return out;}
// Ordered assignments (A, B) of positions with equal sums; each unordered split appears twice unless n = 0.
lng partitionOracle(const vector<int> &values) {
    lng ordered = 0; int n = int(values.size());
    for (lng mask = 0; mask < (lng(1) << n); ++mask) {
        lng first = 0, second = 0;
        for (int i = 0; i < n; ++i) { (mask >> i & 1 ? first : second) += values[i]; }
        ordered += first == second;}
    return n ? ordered / 2 : 1;}
void checkSplits(const vector<int> &values, vector<lng> targets, bool orders = false) {
    ++cases; context = "equalPartitionCount/signedSumCount " + show(values, 0);
    auto signs = signOracle(values); lng split = partitionOracle(values), sum = 0, vectors = 0;
    for (int x : values) { sum += x; }
    for (auto [total, count] : signs) { vectors += count; }
    check(vectors == lng(1) << values.size() && (values.empty() ? split == 1 : 2 * split == signs[0]), "brute-force oracles agree");
    checkEqual(equalPartitionCount<Big>(values), Big(split), "exact equal-sum splits");
    checkEqual(int(equalPartitionCount<ModInt<998244353>>(values)), int(split % 998244353), "equal-sum splits modulo 998244353");
    checkEqual(int(equalPartitionCount<ModInt<2>>(values)), int(split % 2), "equal-sum splits modulo 2");
    checkEqual(equalPartitionCount<ulng>(values), ulng(split), "equal-sum splits as ulng");
    checkEqual(residue(equalPartitionCount<ModIntMini<998244353>>(values)), int(split % 998244353), "equal-sum splits ModIntMini<998244353>");
    checkEqual(residue(equalPartitionCount<ModIntMini<2>>(values)), int(split % 2), "equal-sum splits ModIntMini<2>");
    if (targets.empty()) { for (lng t = -sum - 2; t <= sum + 2; ++t) { targets.push_back(t); } }
    targets.push_back(INT64_MIN); targets.push_back(INT64_MAX);
    for (lng t : targets) {
        lng want = signs.count(t) ? signs[t] : 0; string at = " target=" + number(lll(t));
        checkEqual(signedSumCount<Big>(values, t), Big(want), "exact sign vectors" + at);
        checkEqual(int(signedSumCount<ModInt<998244353>>(values, t)), int(want % 998244353), "sign vectors modulo 998244353" + at);
        checkEqual(residue(signedSumCount<ModIntMini<998244353>>(values, t)), int(want % 998244353), "sign vectors ModIntMini<998244353>" + at);
        checkEqual(signedSumCount<ulng>(values, t), ulng(want), "sign vectors as ulng" + at);}
    if (!orders) { return; }
    // The largest value may sit anywhere: every reordering keeps both counts.
    auto order = values; sort(order.begin(), order.end());
    for (bool more = true; more; more = std::next_permutation(order.begin(), order.end())) {
        checkEqual(equalPartitionCount<Big>(order), Big(split), "equal-sum splits of a reordering");
        for (lng t : targets) { checkEqual(signedSumCount<Big>(order, t), Big(signs.count(t) ? signs[t] : 0), "sign vectors of a reordering"); }}}

void splitCases(const string &mode) {
    int bound = mode == "quick" ? 4 : mode == "full" ? 5 : 6, inputs = 0;
    vector<int> alphabet{0, 1, 2, 3, 7};
    for (int n = 0; n <= bound; ++n) {
        vector<int> values(n);
        auto visit = [&](auto &&visit, int i) -> void {
            if (i == n) { checkSplits(values, {}); ++inputs; return; }
            for (int x : alphabet) { values[i] = x; visit(visit, i + 1); }};
        visit(visit, 0);}
    cout << "PASS exhaustive split/sign alphabet {0,1,2,3,7} max_values=" << bound << " inputs=" << inputs << " every target in [-S-2,S+2]\n";
    context = "hand-computed split and sign counts";
    checkEqual(equalPartitionCount<lng>({}), lng(1), "no items give one split");
    checkEqual(equalPartitionCount<lng>({1, 2, 3, 4, 5, 6, 7}), lng(4), "CSES 1093 sample");
    checkEqual(equalPartitionCount<lng>({0, 0}), lng(2), "zeros double the splits");
    checkEqual(equalPartitionCount<lng>({1, 2}), lng(0), "odd sum");
    checkEqual(signedSumCount<lng>({}, 0), lng(1), "empty sign vector");
    checkEqual(signedSumCount<lng>({1, 1, 1, 1}, 0), lng(6), "four ones to zero");
    checkEqual(signedSumCount<lng>({0}, 0), lng(2), "a zero value takes both signs");
    checkEqual(signedSumCount<lng>({3}, 1), lng(0), "parity mismatch");
    const lng M = INT_MAX;
    for (const auto &[values, targets] : vector<pair<vector<int>, vector<lng>>>{
        {{INT_MAX, INT_MAX}, {-2 * M - 1, -2 * M, 2 * M, 2 * M + 1}},
        {{1, 1, INT_MAX, INT_MAX}, {2 * M + 2, 2 * M, 2 * M - 2, -2 * M - 2, -2 * M, 2 * M + 3}},
        {{INT_MAX, 5, 3}, {M - 8, M - 2, M, M + 2, M + 8, M + 9, -M - 2, -M + 8, M - 1}},
        {{0, 0, 0, INT_MAX, 0}, {M, -M, M - 1, M + 1}},
        {{1, INT_MAX, 1, INT_MAX, 0}, {2 * M + 2, 2 * M, 2 * M - 2, -2 * M - 2, -2 * M, 2 * M + 3, 2 * M + 1}},
        {{INT_MAX, INT_MAX, 4, 2, 2}, {2 * M + 8, 2 * M + 4, 2 * M, 2 * M - 8, -2 * M - 4, 2 * M + 7, 2 * M + 9}},
        {{7, 1, 2, 3, 1}, {}}}) {
        checkSplits(values, targets, true);}
    auto order = vector<int>{7, 6, 5, 4, 3, 2, 1};
    do { checkEqual(equalPartitionCount<lng>(order), lng(4), "every ordering of CSES 1093 values 1..7"); } while (std::next_permutation(order.begin(), order.end(), std::greater<int>()));
    std::mt19937_64 gen(test_seed ^ 0x5b117ULL); int rounds = mode == "quick" ? 100 : mode == "full" ? 2000 : 10000;
    for (int round = 0; round < rounds; ++round) {
        vector<int> values(gen() % 11); lng sum = 0;
        for (int &x : values) { x = int(gen() % 4 ? gen() % 31 : gen() % 3); sum += x; }
        vector<lng> targets{0, sum, -sum, sum - 1, -sum + 1, sum + 1, sum + 2, -sum - 2};
        for (int k = 0; k < 6; ++k) { targets.push_back(lng(gen() % ulng(2 * sum + 5)) - sum - 2); }
        checkSplits(values, targets, values.size() <= 5 && round % 4 == 0);}
    cout << "PASS seeded split/sign brute force rounds=" << rounds << " values<=10 of 0..30\n";
    int m = mode == "quick" ? 1000 : mode == "full" ? 3000 : 6000;
    context = "ones-only sign vectors m=" + std::to_string(m);
    const ulng p = 998244353; vector<ulng> fact(m + 1, 1), inverse(m + 1, 1);
    for (int i = 1; i <= m; ++i) { fact[i] = fact[i - 1] * ulng(i) % p; }
    auto power = [&](ulng x, ulng e) { ulng r = 1; for (; e; e >>= 1, x = x * x % p) { if (e & 1) { r = r * x % p; } } return r; };
    inverse[m] = power(fact[m], p - 2);
    for (int i = m; i > 0; --i) { inverse[i - 1] = inverse[i] * ulng(i) % p; }
    vector<int> ones(m, 1);
    for (lng t : {lng(0), lng(2), lng(-2), lng(m - 4), lng(m), lng(-m), lng(m + 1), lng(m + 2), lng(1)}) {
        ++cases; lng up = m + t;
        int want = up % 2 || up < 0 || up > 2 * m ? 0 : int(fact[m] * inverse[up / 2] % p * inverse[m - up / 2] % p);
        checkEqual(int(signedSumCount<ModInt<998244353>>(ones, t)), want, "ones give binomial C(m,(m+t)/2) target=" + std::to_string(t));
        checkEqual(residue(signedSumCount<ModIntMini<998244353>>(ones, t)), want, "ModIntMini ones give C(m,(m+t)/2) target=" + std::to_string(t));}
    int halves = int(fact[m - 1] * inverse[m / 2] % p * inverse[m / 2 - 1] % p);
    checkEqual(int(equalPartitionCount<ModInt<998244353>>(ones)), halves, "ones split C(m-1,m/2-1)");
    checkEqual(residue(equalPartitionCount<ModIntMini<998244353>>(ones)), halves, "ModIntMini ones split C(m-1,m/2-1)");
    int k = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
    context = "zeros double counts k=" + std::to_string(k);
    vector<int> zeros(k, 0); zeros.push_back(3); zeros.insert(zeros.begin(), 3); cases += 2;
    checkEqual(int(equalPartitionCount<ModInt<998244353>>(zeros)), int(power(2, k)), "k zeros and {3,3} give 2^k splits");
    checkEqual(equalPartitionCount<ulng>(zeros), k < 64 ? ulng(1) << k : 0, "2^k splits wrap modulo 2^64");
    checkEqual(int(signedSumCount<ModInt<998244353>>(zeros, 0)), int(power(2, k + 1)), "k zeros and {3,3} give 2^(k+1) sign vectors");
    checkEqual(residue(equalPartitionCount<ModIntMini<998244353>>(zeros)), int(power(2, k)), "ModIntMini 2^k splits");
    checkEqual(residue(signedSumCount<ModIntMini<998244353>>(zeros, 0)), int(power(2, k + 1)), "ModIntMini 2^(k+1) sign vectors");
    checkEqual(signedSumCount<Big>(vector<int>(200, 0), 0), Big(Big(1) << 200), "exact 2^200 sign vectors");
    cout << "PASS large ones=" << m << " zeros=" << k << '\n';}

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
                KnapsackCounts<Big> result(cap, items); KnapsackCounts<ModInt<998244353>> prime(cap, items);
                KnapsackCounts<ModIntMini<998244353>> mini(cap, items); KnapsackCounts<ulng> word(cap, items);
                for (int w = 0; w <= cap; ++w) {
                    auto exact = result.exact(w), prefix = result.atMost(w);
                    cout << int(exact.status) << ' ' << exact.count << ' ' << int(prefix.status) << ' ' << prefix.count << ' '
                         << prime.exact(w).count << ' ' << residue(mini.exact(w).count) << ' ' << word.exact(w).count << '\n';}}
            return 0;}
        if (arg == "--oracle-splits") {
            int n; lng target;
            while (cin >> n >> target) {
                vector<int> values(n);
                for (int &x : values) { cin >> x; }
                cout << equalPartitionCount<Big>(values) << ' ' << signedSumCount<Big>(values, target) << ' '
                     << equalPartitionCount<ModInt<998244353>>(values) << ' ' << signedSumCount<ModInt<998244353>>(values, target) << ' '
                     << residue(equalPartitionCount<ModIntMini<998244353>>(values)) << ' ' << residue(signedSumCount<ModIntMini<998244353>>(values, target)) << '\n';}
            return 0;}
        if (arg == "--oracle-compositions") {
            int n, total;
            while (cin >> n >> total) {
                vector<int> parts(n);
                for (int &p : parts) { cin >> p; }
                auto exact = compositionCountTable<Big>(total, parts);
                auto prime = compositionCountTable<ModInt<998244353>>(total, parts);
                auto word = compositionCountTable<ulng>(total, parts);
                auto mini = compositionCountTable<ModIntMini<998244353>>(total, parts);
                for (int w = 0; w <= total; ++w) { cout << exact[w] << ' ' << prime[w] << ' ' << word[w] << ' ' << residue(mini[w]) << '\n'; }}
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
            if (probe == "coins-negative") { minimumCoins(-1, {}); }
            if (probe == "coins-intmax") { minimumCoins(INT_MAX, {}); }
            if (probe == "coins-coin") { minimumCoins(0, {-1}); }
            if (probe == "compositions-negative") { compositionCountTable<lng>(-1, {}); }
            if (probe == "compositions-intmax") { compositionCountTable<lng>(INT_MAX, {}); }
            if (probe == "compositions-zero") { compositionCountTable<lng>(0, {0}); }
            if (probe == "compositions-part") { compositionCountTable<lng>(0, {-1}); }
            if (probe == "partition-negative") { equalPartitionCount<lng>({1, -1}); }
            if (probe == "partition-wide") { equalPartitionCount<lng>({INT_MAX, INT_MAX, INT_MAX, INT_MAX, 10, 0}); }  // H = 2^32 + 3 - (2^31 - 1) = 2^31 + 4 reaches INT_MAX
            if (probe == "signed-negative") { signedSumCount<lng>({-1}, -1); }
            if (probe == "signed-wide") { signedSumCount<lng>({INT_MAX, INT_MAX, INT_MAX, INT_MAX, 6}, 0); }  // half 2^32 + 1 would wrap to int 1
            return 1;}}
    exhaustiveCases(mode); regressionCases(); randomCases(mode); largeCases(mode); boundedCountCases(mode); coinCompositionCases(mode); splitCases(mode);
    cout << "PASS knapsack cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
