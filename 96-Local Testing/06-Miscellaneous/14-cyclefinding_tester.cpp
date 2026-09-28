#include "../../06-Miscellaneous/14-cyclefinding.hpp"

ulng test_seed = 0;
lng checks = 0, runs = 0, orbits = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1); }
}
template<class T> void checkEqual(const T &actual, const T &expected, const string &what) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << expected << " actual=" << actual << '\n'; std::exit(1); }
}
struct Reference { int entry; ulng tail, length; };
Reference orbitOracle(const vector<int> &successor, int start) {
    vector<int> first(successor.size(), -1); int x = start, visited = 0;
    while (first[x] < 0) { first[x] = visited++; x = successor[x]; }
    return {x, ulng(first[x]), ulng(visited - first[x])};
}
string show(const vector<int> &successor, int start) {
    string text = "start=" + std::to_string(start) + " successor=[";
    for (int x : successor) { text += std::to_string(x) + ','; }
    return text + ']';
}
template<class T, class F, class Equal = std::equal_to<T>>
CycleResult<T> runCycle(bool brent, T start, F &&next, ulng budget, Equal equal = Equal{}) {
    ++runs;
    if (brent) { return brentCycle(std::move(start), std::forward<F>(next), budget, std::move(equal)); }
    return floydCycle(std::move(start), std::forward<F>(next), budget, std::move(equal));
}
void checkOrbit(const vector<int> &successor, int start, bool all_budgets, const string &label = "") {
    ++orbits; Reference expected = orbitOracle(successor, start);
    string fixture = label.empty() ? show(successor, start) : label + " start=" + std::to_string(start);
    for (bool brent : {false, true}) {
        context = (brent ? "Brent " : "Floyd ") + fixture;
        ulng calls = 0, budget = UINT64_MAX;
        auto next = [&](auto &x) {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(x)>>);
            check(calls < budget, "successor cannot be called beyond budget");
            check(0 <= x && x < int(successor.size()), "successor input within state domain");
            ++calls; return successor[x];
        };
        auto full = runCycle(brent, start, next, budget);
        check(full.found, "finite map orbit found with full unsigned budget");
        checkEqual(full.entry, expected.entry, "cycle entry against first visits");
        checkEqual(full.tail, expected.tail, "tail length against first visits");
        checkEqual(full.length, expected.length, "cycle length against first visits");
        checkEqual(full.evaluations, calls, "reported all-phase successor calls");
        check(calls > 0 && calls <= 8 * (expected.tail + expected.length) + 8, "linear successor work bound");
        ulng predicted;
        if (brent) {
            ulng power = 1;
            while (power <= expected.tail || power < expected.length) { power *= 2; }
            predicted = power - 1 + 2 * expected.length + 2 * expected.tail; }
        else {
            ulng meeting = max(ulng(1), (expected.tail + expected.length - 1) / expected.length) * expected.length;
            predicted = 3 * meeting + 2 * expected.tail + expected.length; }
        checkEqual(calls, predicted, "independent phase-length evaluation formula");
        ulng required = calls;
        vector<ulng> budgets{0, 1, required / 2, required - 1, required, required + 1, UINT64_MAX};
        if (required > 1) { budgets.push_back(required - 2); }
        if (all_budgets) { budgets.clear(); for (ulng b = 0; b <= required + 1; ++b) { budgets.push_back(b); } }
        sort(budgets.begin(), budgets.end()); budgets.erase(unique(budgets.begin(), budgets.end()), budgets.end());
        for (ulng b : budgets) {
            context = (brent ? "Brent " : "Floyd ") + fixture + " budget=" + std::to_string(b);
            calls = 0; budget = b; auto result = runCycle(brent, start, next, budget);
            checkEqual(result.evaluations, calls, "exact successor accounting including recovery phases");
            check(calls <= b, "reported calls bounded");
            check(result.found == (b >= required), "first successful budget matches deterministic complete work");
            if (result.found) {
                checkEqual(result.entry, expected.entry, "budgeted entry");
                checkEqual(result.tail, expected.tail, "budgeted tail");
                checkEqual(result.length, expected.length, "budgeted cycle length");
                checkEqual(calls, required, "larger budget does not change work"); }
            else {
                checkEqual(calls, b, "failure consumes exactly the budget");
                check(result.entry == start && result.tail == 0 && result.length == 0, "failure returns untouched start and zero lengths"); }}}
}

void exhaustiveCases(const string &mode) {
    int bound = mode == "quick" ? 3 : mode == "full" ? 5 : 6, maps = 0;
    for (int n = 1; n <= bound; ++n) {
        vector<int> successor(n);
        auto enumerate = [&](auto &&enumerate, int i) -> void {
            if (i == n) {
                for (int start = 0; start < n; ++start) { checkOrbit(successor, start, true); }
                ++maps; return; }
            for (int j = 0; j < n; ++j) { successor[i] = j; enumerate(enumerate, i + 1); }
        };
        enumerate(enumerate, 0); }
    cout << "PASS all finite maps n=1.." << bound << " maps=" << maps << " start_orbits=" << orbits
         << " every budget through first success+1, both algorithms\n";
}
void structuredCases(const string &mode) {
    int max_power = mode == "quick" ? 8 : mode == "full" ? 14 : 17, inputs = 0;
    auto fixture = [&](int tail, int length, bool thorough = false) {
        vector<int> successor(tail + length); iota(successor.begin(), successor.end(), 1); successor.back() = tail;
        checkOrbit(successor, 0, thorough, "structured tail=" + std::to_string(tail) + " length=" + std::to_string(length)); ++inputs;
    };
    for (int tail = 0; tail <= 12; ++tail) { for (int period = 1; period <= 12; ++period) { fixture(tail, period, true); } }
    for (int power = 1; power <= max_power; ++power) {
        int n = 1 << power;
        for (int delta : {-1, 0, 1}) {
            fixture(0, n + delta); fixture(n + delta, 1); fixture(n + delta, n - delta);
            fixture(1, n + delta); fixture(n + delta, 3); }}
    cout << "PASS structured tails/periods and powers-of-two +/-1 max_power=" << max_power << " inputs=" << inputs << '\n';
}
void randomCases(const string &mode) {
    std::mt19937_64 gen(test_seed); int rounds = mode == "quick" ? 100 : mode == "full" ? 3000 : 20000;
    for (int round = 0; round < rounds; ++round) {
        int n = 1 + int(gen() % 201); vector<int> successor(n);
        for (int &x : successor) { x = int(gen() % n); }
        int start = int(gen() % n); checkOrbit(successor, start, round < 30); }
    cout << "PASS seeded random finite maps rounds=" << rounds << " max_states=201\n";
}

struct Opaque {
    int id, payload;
    Opaque() = delete;
    Opaque(int x, int p) : id(x), payload(p) {}
};
struct OpaqueEqual { bool operator()(const Opaque &a, const Opaque &b) const { return a.id == b.id; } };
struct LiveState {
    static inline int live = 0, peak = 0;
    int id;
    LiveState() = delete;
    explicit LiveState(int x) : id(x) { peak = max(peak, ++live); }
    LiveState(const LiveState &x) : LiveState(x.id) {}
    LiveState &operator=(const LiveState &) = default;
    ~LiveState() { --live; }
};
struct MoveOnlyNext {
    std::unique_ptr<ulng> calls = std::make_unique<ulng>(0);
    int operator()(const int &x) { ++*calls; return (x + 1) % 5; }
};
void genericCases(const string &mode) {
    for (bool brent : {false, true}) {
        context = brent ? "Brent generic states" : "Floyd generic states";
        auto text_next = [](const string &x) { return x.substr(1) + x.front(); };
        auto text = runCycle(brent, string("abcde"), text_next, 1000);
        check(text.found && text.entry == "abcde" && !text.tail && text.length == 5, "string state/default equality");
        const vector<int> successor{1, 2, 3, 4, 2};
        auto next = [&](const Opaque &x) { return Opaque(successor[x.id], x.payload + 1); };
        for (ulng budget : {ulng(0), ulng(1), ulng(2), ulng(5), ulng(1000), UINT64_MAX}) {
            ulng calls = 0;
            auto counted = [&](const Opaque &x) { check(calls < budget, "opaque budget monitor"); ++calls; return next(x); };
            auto result = runCycle(brent, Opaque(0, 91), counted, budget, OpaqueEqual{});
            checkEqual(result.evaluations, calls, "opaque successor accounting");
            if (result.found) {
                check(result.entry.id == 2 && result.tail == 2 && result.length == 3, "custom equivalence ignores increasing payload");
                check(result.entry.payload == 93, "returned representative is actual f^tail(start)"); }
            else {
                check(result.entry.id == 0 && result.entry.payload == 91 && !result.tail && !result.length,
                      "opaque failure preserves start object exactly"); checkEqual(calls, budget, "opaque exhaustion"); }}
        MoveOnlyNext move_only;
        auto moved = runCycle(brent, 0, move_only, UINT64_MAX);
        check(moved.found && moved.length == 5 && moved.evaluations == *move_only.calls, "move-only lvalue successor state retained");
        auto equal = [owned = std::make_unique<int>(0)](const int &a, const int &b) mutable { ++*owned; return a == b; };
        auto equality = runCycle(brent, 0, [](const int &x) { return (x + 1) % 3; }, 100, std::move(equal));
        check(equality.found && equality.length == 3, "move-only explicit equality");
        context = brent ? "Brent nested traversals" : "Floyd nested traversals";
        ulng calls = 0;
        auto nested = runCycle(brent, 0, [&](const int &x) {
            ++calls;
            auto inside = runCycle(!brent, 3, [](const int &) { return 3; }, 100);
            check(inside.found && inside.entry == 3 && inside.tail == 0 && inside.length == 1, "nested orbit independent");
            return (x + 1) % 7;
        }, 1000);
        check(nested.found && nested.length == 7 && nested.evaluations == calls, "outer orbit after nested callbacks");
        context = brent ? "Brent constant live-state storage" : "Floyd constant live-state storage";
        check(LiveState::live == 0, "no state leaked before lifetime test"); LiveState::peak = 0;
        int period = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000;
        {
            auto result = runCycle(brent, LiveState(0), [&](const LiveState &x) { return LiveState((x.id + 1) % period); },
                                   UINT64_MAX, [](const LiveState &a, const LiveState &b) { return a.id == b.id; });
            check(result.found && result.length == ulng(period) && result.tail == 0, "non-default/non-equality state long orbit");
            check(LiveState::peak <= 32, "constant number of live states independent of orbit length"); }
        check(LiveState::live == 0, "all local states released"); }
    cout << "PASS strings, no-default opaque states, congruent custom equality, move-only callbacks, nesting, constant live-state storage\n";
}

void boundedAndThrowingCases(const string &mode) {
    for (bool brent : {false, true}) {
        for (ulng budget : {ulng(0), ulng(1), ulng(2), ulng(3), ulng(7), ulng(31), ulng(1000), ulng(mode == "quick" ? 10000 : 1000000)}) {
            context = string(brent ? "Brent" : "Floyd") + " infinite integer orbit budget=" + std::to_string(budget);
            ulng calls = 0;
            auto result = runCycle(brent, ulng(0), [&](const ulng &x) {
                check(calls < budget, "infinite-orbit strict budget"); ++calls; return x + 1;
            }, budget);
            check(!result.found && result.entry == 0 && result.tail == 0 && result.length == 0, "infinite-orbit exhausted result");
            checkEqual(result.evaluations, budget, "infinite-orbit exact budget metadata");
            checkEqual(calls, budget, "infinite-orbit callback count"); }
        for (int fail_at : {1, 2, 3, 7, 20}) {
            context = string(brent ? "Brent" : "Floyd") + " successor throws call=" + std::to_string(fail_at);
            int calls = 0; bool caught = false;
            try {
                runCycle(brent, 0, [&](const int &x) {
                    if (++calls == fail_at) { throw std::runtime_error("successor"); }
                    return (x + 1) % 101;
                }, 10000);
            } catch (const std::runtime_error &error) { caught = string(error.what()) == "successor"; }
            check(caught && calls == fail_at, "successor exception immediately propagates");
            context = string(brent ? "Brent" : "Floyd") + " equality throws comparison=" + std::to_string(fail_at);
            int comparisons = 0; caught = false;
            try {
                runCycle(brent, 0, [](const int &x) { return (x + 1) % 101; }, 10000,
                         [&](const int &a, const int &b) {
                    if (++comparisons == fail_at) { throw std::runtime_error("equal"); }
                    return a == b;
                });
            } catch (const std::runtime_error &error) { caught = string(error.what()) == "equal"; }
            check(caught && comparisons == fail_at, "equality exception immediately propagates"); }
        context = brent ? "Brent zero budget never evaluates successor" : "Floyd zero budget never evaluates successor";
        auto result = runCycle(brent, 42, [](const int &) -> int { throw std::runtime_error("not called"); }, 0);
        check(!result.found && !result.evaluations && result.entry == 42, "zero-budget callback untouched"); }
    cout << "PASS bounded nonrepeating prefixes, zero budget, successor/equality exceptions\n";
}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }}
    exhaustiveCases(mode); structuredCases(mode); randomCases(mode); genericCases(mode); boundedAndThrowingCases(mode);
    cout << "PASS cyclefinding orbits=" << orbits << " runs=" << runs << " checks=" << checks << " seed=" << test_seed << '\n';
}
