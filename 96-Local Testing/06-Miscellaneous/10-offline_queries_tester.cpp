#include "../../06-Miscellaneous/10-offline_queries.hpp"

ulng test_seed = 0;
lng checks = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
                    << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}

string indices(const vector<int> &a) {
    string result = "[";
    for (int x : a) { result += std::to_string(x) + ','; }
    return result + ']';}
void checkIndices(const vector<int> &actual, const vector<int> &expected, const string &what) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << indices(expected)
             << " actual=" << indices(actual) << '\n'; std::exit(1);}}

template<typename K, typename Compare>
void checkSweep(const vector<K> &a, const vector<OfflineThreshold<K>> &queries, Compare cmp) {
    int n = int(a.size()), q = int(queries.size());
    vector<int> update_order, query_order;
    vector<bool> used(n), answered(q);
    // Repeated minimum selection is independent of the implementation's sorting.
    for (int k = 0; k < n; ++k) {
        int best = -1;
        for (int i = 0; i < n; ++i) {
            if (!used[i] && (best < 0 || cmp(a[i], a[best]))) { best = i; }}
        used[best] = true; update_order.push_back(best);}
    for (int k = 0; k < q; ++k) {
        int best = -1;
        for (int i = 0; i < q; ++i) {
            if (answered[i]) { continue; }
            if (best < 0 || cmp(queries[i].key, queries[best].key)
                || (!cmp(queries[best].key, queries[i].key) && !cmp(queries[i].key, queries[best].key)
                    && !queries[i].inclusive && queries[best].inclusive)) { best = i; }}
        answered[best] = true; query_order.push_back(best);}
    vector<int> state, actual_queries; int last = 0;
    offlineSweep(a, queries, [&](int i) {
        check(0 <= i && i < n, "apply index bounds");
        check(int(state.size()) < n && update_order[state.size()] == i, "stable update callback order");
        state.push_back(i);}, [&](int i) {
        check(0 <= i && i < q, "answer index bounds");
        vector<int> expected;
        for (int j : update_order) {
            bool equal = !cmp(a[j], queries[i].key) && !cmp(queries[i].key, a[j]);
            if (cmp(a[j], queries[i].key) || (queries[i].inclusive && equal)) { expected.push_back(j); }}
        checkIndices(state, expected, "noncommutative state at query=" + std::to_string(i));
        actual_queries.push_back(i); last = int(state.size());}, cmp);
    checkIndices(actual_queries, query_order, "strict/inclusive/stable query callback order");
    check(int(state.size()) == last, "no update is applied after the last answer");}

template<typename K, typename Compare>
void checkRanges(const vector<K> &a, const vector<OfflineRangeThreshold<K>> &queries, Compare cmp) {
    vector<int> expected;
    for (const auto &query : queries) {
        int count = 0;
        for (int i = query.l; i < query.r; ++i) {
            bool equal = !cmp(a[i], query.key) && !cmp(query.key, a[i]);
            count += cmp(a[i], query.key) || (query.inclusive && equal);}
        expected.push_back(count);}
    checkIndices(offlineRangeCount(a, queries, cmp), expected, "range counts vs direct scan");}

void checkIntegerArray(const vector<lng> &a, bool exhaustive) {
    context = "array=["; for (lng x : a) { context += std::to_string(x) + ','; } context += ']';
    vector<OfflineThreshold<lng>> queries;
    for (lng key : {lng(2), lng(-2), lng(0), lng(1), lng(-1), lng(0)}) {
        queries.push_back({key}); queries.push_back({key, false}); queries.push_back({key});}
    auto original = a;
    checkSweep(a, queries, std::less<lng>{});
    checkSweep(a, queries, std::greater<lng>{});
    int n = int(a.size()); vector<OfflineRangeThreshold<lng>> ranges;
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        if (!exhaustive && l != 0 && r != n && r != l && r != l + 1) { continue; }
        for (lng key : {lng(-2), lng(-1), lng(0), lng(1), lng(2)}) {
            ranges.push_back({l, r, key}); ranges.push_back({l, r, key, false});}}}
    reverse(ranges.begin(), ranges.end());
    auto saved = ranges;
    checkRanges(a, ranges, std::less<lng>{});
    checkRanges(a, ranges, std::greater<lng>{});
    check(a == original, "update keys preserved");
    for (int i = 0; i < int(ranges.size()); ++i) {
        check(ranges[i].l == saved[i].l && ranges[i].r == saved[i].r && ranges[i].key == saved[i].key
              && ranges[i].inclusive == saved[i].inclusive, "range query inputs preserved");}}

struct Direction {
    bool down;
    Direction() = delete;
    explicit Direction(bool d) : down(d) {}
    bool operator()(lng a, lng b) const { return down ? a > b : a < b; }
};
struct LengthOrder {
    bool operator()(const string &a, const string &b) const { return a.size() < b.size(); }
};
struct OpaqueKey {
    int value;
    OpaqueKey() = delete;
    explicit OpaqueKey(int x) : value(x) {}
};
struct OpaqueOrder {
    bool operator()(const OpaqueKey &a, const OpaqueKey &b) const { return a.value / 3 < b.value / 3; }
};

void customCases() {
    context = "empty inputs and unused updates";
    checkSweep(vector<int>{}, vector<OfflineThreshold<int>>{}, std::less<int>{});
    checkSweep(vector<int>{1, 2, 3}, vector<OfflineThreshold<int>>{}, std::less<int>{});
    checkSweep(vector<int>{}, vector<OfflineThreshold<int>>{{2}, {0, false}, {0}}, std::less<int>{});
    checkSweep(vector<int>{2, 4, 3}, vector<OfflineThreshold<int>>{{1}}, std::less<int>{});
    checkRanges(vector<int>{}, vector<OfflineRangeThreshold<int>>{}, std::less<int>{});
    checkRanges(vector<int>{}, vector<OfflineRangeThreshold<int>>{{0, 0, INT_MIN}, {0, 0, INT_MAX, false}}, std::less<int>{});

    context = "full signed 64-bit keys and descending comparator state";
    vector<lng> signed_keys{INT64_MAX, 0, INT64_MIN, INT64_MAX, -1, INT64_MIN};
    vector<OfflineThreshold<lng>> signed_queries{{INT64_MIN}, {INT64_MAX, false}, {INT64_MAX}, {INT64_MIN, false}, {0}};
    checkSweep(signed_keys, signed_queries, std::less<lng>{});
    checkSweep(signed_keys, signed_queries, Direction{true});
    checkRanges(signed_keys, vector<OfflineRangeThreshold<lng>>{{0, 6, INT64_MIN}, {1, 5, INT64_MAX, false},
                {2, 6, INT64_MAX}, {1, 4, 0, false}, {3, 3, INT64_MIN}}, Direction{true});
    context = "full unsigned 64-bit keys";
    checkSweep(vector<ulng>{0, UINT64_MAX, ulng(1) << 63, UINT64_MAX},
               vector<OfflineThreshold<ulng>>{{UINT64_MAX}, {UINT64_MAX, false}, {0}, {0, false}}, std::less<ulng>{});
    context = "signed and unsigned 128-bit keys without arithmetic";
    lll low = -lll((ulll(1) << 127) - 1) - 1, high = lll((ulll(1) << 127) - 1);
    vector<lll> wide{high, low, 0, lll(INT64_MAX) + 1, lll(INT64_MIN) - 1, high};
    checkSweep(wide, vector<OfflineThreshold<lll>>{{low}, {low, false}, {high}, {high, false}, {0}}, std::less<lll>{});
    checkRanges(wide, vector<OfflineRangeThreshold<lll>>{{0, 6, high}, {1, 5, low}, {0, 6, low, false}, {2, 6, 0}}, std::less<lll>{});
    checkSweep(vector<ulll>{~ulll(0), 0, ulll(1) << 127, ~ulll(0)},
               vector<OfflineThreshold<ulll>>{{~ulll(0)}, {~ulll(0), false}, {0}, {0, false}}, std::less<ulll>{});
    context = "string keys with comparator equivalence";
    vector<string> words{"zz", "", "b", "aa", "ccc", "d", "ee"};
    vector<OfflineThreshold<string>> word_queries{{"XX"}, {"f", false}, {"XX", false}, {"gg"}, {""}, {"", false}, {"ffff"}};
    checkSweep(words, word_queries, LengthOrder{});
    checkRanges(words, vector<OfflineRangeThreshold<string>>{{0, 7, "aa"}, {1, 6, "q", false},
                {2, 5, "dd", false}, {0, 7, "eeee"}, {3, 3, ""}}, LengthOrder{});
    checkSweep(words, word_queries, std::less<string>{});
    context = "opaque non-default-constructible keys: comparator only, no equality/order operators";
    vector<OpaqueKey> opaque{OpaqueKey(5), OpaqueKey(0), OpaqueKey(3), OpaqueKey(8), OpaqueKey(4)};
    checkSweep(opaque, vector<OfflineThreshold<OpaqueKey>>{{OpaqueKey(4)}, {OpaqueKey(4), false},
                {OpaqueKey(0)}, {OpaqueKey(9), false}}, OpaqueOrder{});
    checkRanges(opaque, vector<OfflineRangeThreshold<OpaqueKey>>{{0, 5, OpaqueKey(4)},
                {1, 4, OpaqueKey(4), false}}, OpaqueOrder{});
    context = "bool proxy input keys";
    checkSweep(vector<bool>{true, false, true, false}, vector<OfflineThreshold<bool>>{{true},
                {false}, {true, false}, {false, false}}, std::less<bool>{});
    checkRanges(vector<bool>{true, false, true, false}, vector<OfflineRangeThreshold<bool>>{{0, 4, true},
                {1, 4, false}, {0, 3, true, false}}, std::less<bool>{});
    context = "default comparator and inclusivity, external state survives repeated calls";
    int accumulated = 0, answers = 0;
    for (int run = 1; run <= 2; ++run) {
        offlineSweep(vector<int>{2, 1, 2}, vector<OfflineThreshold<int>>{{2}},
                     [&](int) { ++accumulated; }, [&](int i) {
            check(i == 0 && accumulated == 3 * run, "state lifetime across invocations"); ++answers;});}
    check(answers == 2, "exactly one answer per invocation");
    checkIndices(offlineRangeCount(vector<int>{2, 1, 2}, vector<OfflineRangeThreshold<int>>{{0, 3, 2}, {0, 3, 2, false}}),
                 vector<int>{3, 1}, "default comparator and default inclusive aggregate");
    context = "callback exceptions propagate and preserve already accumulated state";
    accumulated = 0; bool caught = false;
    try {
        offlineSweep(vector<int>{3, 1, 2}, vector<OfflineThreshold<int>>{{3}}, [&](int) {
            if (++accumulated == 2) { throw std::runtime_error("apply"); }}, [&](int) { check(false, "answer cannot run after apply throws"); });} catch (const std::runtime_error &e) { caught = string(e.what()) == "apply"; }
    check(caught && accumulated == 2, "apply exception propagated without rollback");
    caught = false; accumulated = 0;
    try {
        offlineSweep(vector<int>{1, 3, 2}, vector<OfflineThreshold<int>>{{2}},
                     [&](int) { ++accumulated; }, [&](int) { throw std::runtime_error("answer"); });} catch (const std::runtime_error &e) { caught = string(e.what()) == "answer"; }
    check(caught && accumulated == 2, "answer exception propagated without rollback");
    cout << "PASS empty/default/repeated/exception semantics; custom comparators, equivalence, bool/string/opaque/64/128-bit keys\n";}

void randomCases(const string &mode) {
    std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 100 : mode == "full" ? 2000 : 20000;
    for (int round = 0; round < rounds; ++round) {
        int n = int(gen() % 101), q = int(gen() % 51);
        vector<lng> a(n);
        for (lng &x : a) { x = round % 2 ? std::bit_cast<lng>(gen()) : lng(gen() % 9) - 4; }
        vector<OfflineThreshold<lng>> queries;
        vector<OfflineRangeThreshold<lng>> ranges;
        context = "round=" + std::to_string(round) + " array=[";
        for (lng x : a) { context += std::to_string(x) + ','; } context += "] queries=[";
        for (int i = 0; i < q; ++i) {
            lng key = i % 3 == 0 && n ? a[gen() % n]
                    : round % 2 ? std::bit_cast<lng>(gen()) : lng(gen() % 11) - 5;
            int l = int(gen() % (n + 1)), r = int(gen() % (n + 1)); if (l > r) { swap(l, r); }
            bool inclusive = bool(gen() % 2);
            queries.push_back({key, inclusive}); ranges.push_back({l, r, key, inclusive});
            context += '(' + std::to_string(l) + ',' + std::to_string(r) + ',' + std::to_string(key)
                     + ',' + std::to_string(inclusive) + "),";}
        context += ']';
        auto original = a; auto saved_queries = queries;
        checkSweep(a, queries, std::less<lng>{}); checkRanges(a, ranges, std::less<lng>{});
        checkSweep(a, queries, Direction{true}); checkRanges(a, ranges, Direction{true});
        check(a == original, "random input keys preserved");
        for (int i = 0; i < q; ++i) {
            check(queries[i].key == saved_queries[i].key && queries[i].inclusive == saved_queries[i].inclusive,
                  "sweep query keys/flags preserved");}}
    cout << "PASS seeded random direct-scan/ordered-callback corpora rounds=" << rounds << '\n';}

void largeCases(const string &mode) {
    int n = mode == "quick" ? 5000 : 200000;
    context = "large equal keys n=" + std::to_string(n);
    vector<int> a(n, 7); vector<OfflineRangeThreshold<int>> queries; queries.reserve(n);
    for (int i = 0; i < n; ++i) { queries.push_back({i / 2, n - i / 2, 7, bool(i % 2)}); }
    auto result = offlineRangeCount(a, queries);
    check(int(result.size()) == n, "large output size");
    for (int i = 0; i < n; ++i) {
        check(result[i] == (i % 2 ? n - 2 * (i / 2) : 0), "large equal-key range count query=" + std::to_string(i));}
    context = "large reverse update order n=" + std::to_string(n);
    for (int i = 0; i < n; ++i) { a[i] = n - i; }
    vector<int> answers(4, -1); int applied = 0;
    offlineSweep(a, vector<OfflineThreshold<int>>{{n + 1}, {n / 2, false}, {0}, {n / 2}},
                 [&](int i) { check(i == n - 1 - applied, "large sorted update order"); ++applied; },
                 [&](int i) { answers[i] = applied; });
    checkIndices(answers, vector<int>{n, n / 2 - 1, 0, n / 2}, "large threshold counts");
    cout << "PASS large equal-key range and reverse-order sweep n=" << n << '\n';}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i]; vector<int> a{1, 2};
            if (probe == "negative-left") { offlineRangeCount(a, vector<OfflineRangeThreshold<int>>{{-1, 1, 0}}); }
            if (probe == "reversed-range") { offlineRangeCount(a, vector<OfflineRangeThreshold<int>>{{2, 1, 0}}); }
            if (probe == "right-past-end") { offlineRangeCount(a, vector<OfflineRangeThreshold<int>>{{0, 3, 0}}); }
            if (probe == "empty-past-end") { offlineRangeCount(vector<int>{}, vector<OfflineRangeThreshold<int>>{{0, 1, 0}}); }
            if (probe == "left-past-end") { offlineRangeCount(a, vector<OfflineRangeThreshold<int>>{{3, 3, 0}}); }
            return 1;}}
    int bound = mode == "quick" ? 4 : mode == "full" ? 6 : 7, arrays = 0;
    for (int n = 0, count = 1; n <= bound; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask;
            for (lng &x : a) { x = code % 3 - 1; code /= 3; }
            checkIntegerArray(a, true); ++arrays;}}
    cout << "PASS exhaustive ternary arrays=" << arrays << " max_length=" << bound
         << " all half-open ranges and strict/inclusive thresholds in both orders\n";
    customCases(); randomCases(mode); largeCases(mode);
    cout << "PASS offline sweep/range total checks=" << checks << " seed=" << test_seed << '\n';}
