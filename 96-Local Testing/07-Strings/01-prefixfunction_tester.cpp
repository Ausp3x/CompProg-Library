#include "../../07-Strings/01-prefixfunction.hpp"
#include "01-prefix_z_test_support.hpp"

struct OversizedSequence {
    size_t size() const { return INT_MAX; }
    char operator[](int) const { return 'a'; }
};

vector<int> naiveNext(const vector<int> &p) {
    int m = int(p.size());
    vector<int> next(m + 1, -1);
    for (int i = 0; i <= m; ++i) {
        for (int b = i - 1; b >= 0; --b) {
            if (std::equal(p.begin(), p.begin() + b, p.begin() + i - b) && (i == m || p[b] != p[i])) { next[i] = b; break; }}}
    return next;}
void testString(const vector<int> &s) {
    context = "s=" + show(s); auto pi = naivePi(s), periods = naivePeriods(s);
    check(prefixFunction(s), pi, "prefixFunction");
    check(validPrefixFunction(pi), true, "validPrefixFunction");
    vector<int> witness{99};
    check(validPrefixFunction(pi, witness), true, "validPrefixFunction-witness-status");
    check(naivePi(witness), pi, "validPrefixFunction-witness");
    auto next = naiveNext(s);
    check(kmpNext(s), next, "kmpNext");
    check(kmpNext(s, pi), next, "kmpNext-with-pi");
    check(KmpMatcher(s).next, next, "matcher-next");
    check(prefixOccurrences(pi), naiveCounts(s, s), "prefixOccurrences-self");
    check(prefixPeriods(pi), periods, "prefixPeriods");
    check(prefixPeriod(pi), periods.empty() ? 0 : periods[0], "prefixPeriod");
    int whole = 0;
    for (int p : periods) { if (int(s.size()) % p == 0) { whole = p; break; } }
    check(prefixPeriod(pi, true), whole, "prefixPeriod-whole");
    check(prefixBorders(pi), naiveBorders(s), "prefixBorders-default");
    for (int n = 0; n <= int(s.size()); ++n) {
        vector<int> p(s.begin(), s.begin() + n);
        check(prefixBorders(pi, n), naiveBorders(p), "prefixBorders-prefix");
        check(prefixBorders(pi, n, true), naiveBorders(p, true), "prefixBorders-full");}
    int sigma = 0; for (int c : s) { sigma = max(sigma, c + 1); }
    if (sigma > 10) { return; }
    auto aut = prefixAutomaton(s, sigma + 1);
    check(int(aut.size()), int(s.size()) + 1, "automaton-rows");
    for (int q = 0; q <= int(s.size()); ++q) {
        check(int(aut[q].size()), sigma + 1, "automaton-columns");
        for (int c = 0; c <= sigma; ++c) {
            vector<int> t(s.begin(), s.begin() + q); t.push_back(c);
            check(aut[q][c], naiveState(s, t), "automaton-transition q=" + std::to_string(q) + " c=" + std::to_string(c));}}}
void testMatch(const vector<int> &p, const vector<int> &s) {
    context = "p=" + show(p) + " s=" + show(s);
    check(kmpOccurrences(p, s), naiveMatches(p, s), "kmpOccurrences");
    check(prefixOccurrences(p, s), naiveCounts(p, s), "prefixOccurrences-text");
    KmpMatcher matcher(p); vector<int> seen, found;
    check(matcher.matched(), p.empty(), "initial-matched");
    if (matcher.matched()) { found.push_back(0); }
    for (int i = 0; i < int(s.size()); ++i) {
        seen.push_back(s[i]); bool match = matcher.step(s[i]);
        int q = naiveState(p, seen);
        check(matcher.state, q, "stream-state");
        check(match, q == int(p.size()), "stream-matched");
        check(matcher.processed, lng(i + 1), "stream-processed");
        if (match) { found.push_back(i + 1 - int(p.size())); }
        if (i == int(s.size()) / 2) {
            auto copy = matcher; auto moved = std::move(copy);
            check(moved.step(-99), naiveState(p, [&]() { auto t = seen; t.push_back(-99); return t; }()) == int(p.size()), "copy-move");}}
    check(found, naiveMatches(p, s), "stream-matches");
    matcher.reset(); check(matcher.state, 0, "reset-state"); check(matcher.processed, lng(0), "reset-processed");
    for (int c : s) { matcher.step(c); }
    check(matcher.state, naiveState(p, s), "reset-replay");}
void validation() {
    for (int n = 0; n <= limit(5, 8, 9); ++n) {
        set<vector<int>> valid;
        partitions(n, [&](const auto &s) { valid.insert(naivePi(s)); });
        vector<int> p(n);
        auto go = [&](auto &&self, int i) -> void {
            if (i == n) { context = "pi=" + show(p); check(validPrefixFunction(p), valid.contains(p), "exhaustive-feasibility"); return; }
            for (p[i] = 0; p[i] <= i; ++p[i]) { self(self, i + 1); }};
        go(go, 0);}
    for (auto p : vector<vector<int>>{{-1}, {1}, {0, 0, 2}, {0, 1, 1}, {0, INT_MAX}, {0, INT_MIN}}) {
        context = "pi=" + show(p); check(validPrefixFunction(p), false, "invalid-regression");
        vector<int> witness{7};
        check(validPrefixFunction(p, witness), false, "invalid-witness-status");
        check(witness.empty(), true, "invalid-witness-cleared");}}
int main(int argc, char **argv) {
    configure(argc, argv);
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string probe = argv[2];
        if (probe == "size-large") { prefixFunction(OversizedSequence{}); }
        if (probe == "border-negative") { prefixBorders({}, -2); }
        if (probe == "border-large") { prefixBorders({0}, 2); }
        if (probe == "alphabet-negative") { prefixAutomaton({}, -1); }
        if (probe == "symbol-negative") { prefixAutomaton({-1}, 3); }
        if (probe == "symbol-large") { prefixAutomaton({3}, 3); }
        if (probe == "stream-overflow") { KmpMatcher<> m; m.processed = std::numeric_limits<lng>::max(); m.step('a'); }
        return 2;}
    words(limit(5, 8, 9), 2, testString);
    words(limit(3, 5, 6), 2, [&](const auto &p) { words(limit(3, 5, 6), 2, [&](const auto &s) { testMatch(p, s); }); });
    for (int rep = 0; rep < limit(100, 1200, 8000); ++rep) {
        vector<int> s(rng() % 45), p(rng() % 20);
        for (auto &c : s) { c = int(rng() % 6); } for (auto &c : p) { c = int(rng() % 6); }
        testString(s); testMatch(p, s);}
    vector<int> bytes(256); iota(bytes.begin(), bytes.end(), 0);
    string all; for (int c : bytes) { all += char(c); }
    check(prefixFunction(all), naivePi(bytes), "full-byte-alphabet");
    check(kmpOccurrences(string_view(all).substr(128, 128), all + all), vector<int>({128, 384}), "byte-high-bit-matching");
    check(prefixFunction(vector<lng>{LLONG_MIN, LLONG_MAX, LLONG_MIN}), vector<int>({0, 0, 1}), "wide-integer-alphabet");
    check(kmpOccurrences(vector<unsigned char>{255}, vector<int>{-1}), vector<int>{}, "mixed-types-no-narrowing");
    check(prefixOccurrences(vector<unsigned char>{255}, vector<int>{-1}), vector<lng>({2, 0}), "mixed-types-counts");
    check(int(prefixAutomaton({}, 0).size()), 1, "empty-zero-alphabet");
    vector<int> large(limit(10000, 200000, 1000000), 7), expected(large.size());
    iota(expected.begin(), expected.end(), 0); check(prefixFunction(large), expected, "large-unary");
    KmpMatcher matcher(vector<int>(1000, 7));
    for (int c : large) { matcher.step(c); } check(matcher.state, 1000, "large-stream-overlaps");
    large.back() = 9; check(prefixFunction(large).back(), 0, "large-fallback");
    context = "mixed-signedness regressions";
    check(kmpOccurrences(vector<uint>{UINT_MAX}, vector<int>{-1}), vector<int>{}, "uint-int-no-match");
    check(kmpOccurrences(vector<ulng>{~0ULL}, vector<int>{-1}), vector<int>{}, "ulng-int-no-match");
    check(kmpOccurrences(vector<int>{-1}, vector<ulng>{~0ULL}), vector<int>{}, "int-ulng-no-match");
    check(kmpOccurrences(vector<char32_t>{char32_t(0xFFFFFFFF)}, vector<int>{-1}), vector<int>{}, "char32-int-no-match");
    check(prefixOccurrences(vector<uint>{UINT_MAX}, vector<int>{-1}), vector<lng>({2, 0}), "uint-int-counts");
    check(kmpOccurrences(vector<uint>{UINT_MAX, 5}, vector<lng>{UINT_MAX, 5, -1, 5}), vector<int>{0}, "uint-lng-equal-values");
    check(KmpMatcher<>::same(uint(UINT_MAX), -1), false, "same-uint-int");
    check(KmpMatcher<>::same(ulng(~0ULL), lng(-1)), false, "same-ulng-lng");
    check(KmpMatcher<>::same('a', 97), true, "same-char-int");
    check(KmpMatcher<>::same(-1, lng(-1)), true, "same-int-lng");
    check(KmpMatcher<>::same(true, 1), true, "same-bool-int");
    check(KmpMatcher<>::same(2.5, 2.5), true, "same-double");
    check(KmpMatcher<>::same(~ulll(0), -1), false, "same-ulll-int");
    check(KmpMatcher<>::same(ulll(1) << 127, -(lll(1) << 126) * 2), false, "same-ulll-lll-min");
    check(KmpMatcher<>::same(ulll(5), lng(5)), true, "same-ulll-lng");
    check(KmpMatcher<>::same(lll(-7), -7), true, "same-lll-int");
    check(kmpOccurrences(vector<ulll>{~ulll(0)}, vector<int>{-1}), vector<int>{}, "ulll-int-no-match");
    vector<lng> pool{0, 1, -1, UINT_MAX, lng(UINT_MAX) + 1, LLONG_MIN};
    for (int rep = 0; rep < limit(200, 3000, 20000); ++rep) {
        vector<ulng> p(rng() % 4);
        vector<lng> t(rng() % 12);
        for (auto &x : p) { x = ulng(pool[rng() % 4]); }
        for (auto &x : t) { x = pool[rng() % pool.size()]; }
        auto [a, b] = valueIds(p, t);
        context = "mixed p=" + show(p) + " t=" + show(t);
        check(kmpOccurrences(p, t), naiveMatches(a, b), "mixed-kmpOccurrences");
        check(prefixOccurrences(p, t), naiveCounts(a, b), "mixed-prefixOccurrences");}
    for (int rep = 0; rep < limit(100, 1000, 5000); ++rep) {
        IndexOnly p{vector<int>(rng() % 5)}, t{vector<int>(rng() % 20)};
        for (auto &x : p.v) { x = int(rng() % 3); }
        for (auto &x : t.v) { x = int(rng() % 3); }
        context = "index-only p=" + show(p.v) + " t=" + show(t.v);
        check(kmpOccurrences(p.v, t), naiveMatches(p.v, t.v), "index-only-kmpOccurrences");
        check(prefixOccurrences(p.v, t), naiveCounts(p.v, t.v), "index-only-prefixOccurrences");
        check(prefixFunction(t), naivePi(t.v), "index-only-prefixFunction");}
    vector<int> fib{0}, prev{1};
    while (fib.size() < 2000) { auto x = fib; x.insert(x.end(), prev.begin(), prev.end()); prev = fib; fib = x; }
    vector<int> fib_text(fib.begin(), fib.end());
    fib.resize(987);
    context = "Fibonacci word pattern";
    KmpMatcher fm(fib);
    vector<int> seen;
    for (int i = 0; i < int(fib_text.size()); ++i) {
        fm.step(fib_text[i]);
        if (i % 97 == 0) { seen.assign(fib_text.begin(), fib_text.begin() + i + 1); check(fm.state, naiveState(fib, seen), "fibonacci-stream-state"); }}
    check(kmpOccurrences(fib, fib_text), naiveMatches(fib, fib_text), "fibonacci-occurrences");
    validation(); cout << "PASS prefixfunction seed=" << seed << " mode=" << mode << " checks=" << cases << '\n';}
