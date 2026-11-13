#include "../../07-Strings/02-z.hpp"
#include "01-prefix_z_test_support.hpp"

struct OversizedSequence {
    size_t size_ = INT_MAX;
    size_t size() const { return size_; }
    int operator[](int) const { return 0; }
};
// Counts symbol comparisons, so the linear bounds are checked exactly instead of by timing.
struct Counted {
    int v;
    static inline lng calls = 0;
    bool operator==(const Counted &o) const { ++calls; return v == o.v; }
};
void comparisonBounds() {
    int n = limit(2000, 20000, 100000);
    vector<vector<int>> texts{vector<int>(n, 0), vector<int>(n, 0), {0}, {}, {}};
    texts[1].back() = 1;
    while (int(texts[2].size()) < n) { texts[2].push_back(texts[2].size() % 3 == 2); }
    vector<int> fib_a{0}, fib_b{0, 1};
    while (int(fib_b.size()) < n) { auto t = fib_b; t.insert(t.end(), fib_a.begin(), fib_a.end()); fib_a = fib_b; fib_b = t; }
    texts[3].assign(fib_b.begin(), fib_b.begin() + n);
    for (int i = 0; i < n; ++i) { texts[4].push_back(int(rng() % 2)); }
    auto counted = [](const vector<int> &s, int len) { vector<Counted> res; for (int i = 0; i < len; ++i) { res.push_back({s[i]}); } return res; };
    for (int t = 0; t < int(texts.size()); ++t) {
        context = "comparison-bound text=" + std::to_string(t) + " n=" + std::to_string(n);
        auto s = counted(texts[t], n); Counted::calls = 0; zFunction(s);
        check(Counted::calls <= 2LL * n, true, "zFunction-comparisons");
        for (int u = 0; u < int(texts.size()); ++u) {
            for (int m : {1, 7, n / 3, n}) {
                auto p = counted(texts[u], m); Counted::calls = 0; extendedZ(p, s);
                context = "comparison-bound pattern=" + std::to_string(u) + " m=" + std::to_string(m) + " text=" + std::to_string(t);
                check(Counted::calls <= 2LL * (n + m), true, "extendedZ-comparisons");}}}}

void testString(const vector<int> &s) {
    context = "s=" + show(s); auto z = naiveLcp(s, s), pi = naivePi(s), periods = naivePeriods(s);
    check(zFunction(s), z, "zFunction");
    check(zBorders(z), naiveBorders(s), "zBorders"); check(zBorders(z, true), naiveBorders(s, true), "zBorders-full");
    check(zPeriods(z), periods, "zPeriods"); check(zPeriod(z), periods.empty() ? 0 : periods[0], "zPeriod");
    int whole = 0; for (int p : periods) { if (int(s.size()) % p == 0) { whole = p; break; } }
    check(zPeriod(z, true), whole, "zPeriod-whole");
    vector<int> out{99}; check(prefixToZ(pi, out), true, "prefixToZ-status"); check(out, z, "prefixToZ");
    check(zToPrefix(z, out), true, "zToPrefix-status"); check(out, pi, "zToPrefix");
    out = pi; check(prefixToZ(out, out), true, "prefixToZ-alias-status"); check(out, z, "prefixToZ-alias");
    check(zToPrefix(out, out), true, "zToPrefix-alias-status"); check(out, pi, "zToPrefix-alias");}
void testMatch(const vector<int> &p, const vector<int> &s) {
    context = "p=" + show(p) + " s=" + show(s);
    check(extendedZ(p, s), naiveLcp(p, s), "extendedZ");
    check(zOccurrences(p, s), naiveMatches(p, s), "zOccurrences");}
void validation() {
    for (int n = 0; n <= limit(5, 8, 9); ++n) {
        map<vector<int>, vector<int>> valid;
        partitions(n, [&](const auto &s) { valid[naiveLcp(s, s)] = naivePi(s); });
        vector<int> z(n); if (n) { z[0] = n; }
        auto go = [&](auto &&self, int i) -> void {
            if (i == n) {
                context = "z=" + show(z); bool ok = valid.contains(z); vector<int> out{99};
                check(zToPrefix(z, out), ok, "exhaustive-z-conversion-status");
                check(out, ok ? valid[z] : vector<int>{}, "exhaustive-z-conversion"); return;}
            for (z[i] = 0; z[i] <= n - i; ++z[i]) { self(self, i + 1); }};
        go(go, n ? 1 : 0);}
    for (auto z : vector<vector<int>>{{-1}, {0}, {2}, {2, 2}, {2, -1}, {3, 2, 0}, {2, INT_MAX}, {2, INT_MIN}}) {
        context = "z=" + show(z); auto alias = z; vector<int> out{9};
        check(zToPrefix(z, out), false, "invalid-z-status"); check(out.empty(), true, "invalid-z-clear");
        check(zToPrefix(alias, alias), false, "invalid-z-alias-status"); check(alias.empty(), true, "invalid-z-alias-clear");}
    for (auto pi : vector<vector<int>>{{-1}, {1}, {0, 1, 1}, {0, 0, 2}, {0, INT_MAX}}) {
        context = "pi=" + show(pi); check(prefixToZ(pi, pi), false, "invalid-pi-alias-status"); check(pi.empty(), true, "invalid-pi-clear");}}
struct EqOnly {
    int v;
    bool operator==(const EqOnly &) const = default;
};

// Distinct-substring oracles over symbol ids: a set of every substring, and sorted suffixes minus adjacent LCPs.
lng naiveDistinct(const vector<int> &s) {
    set<vector<int>> seen;
    for (int i = 0; i < int(s.size()); ++i) {
        for (int j = i + 1; j <= int(s.size()); ++j) { seen.emplace(s.begin() + i, s.begin() + j); }}
    return lng(seen.size());}
lng suffixDistinct(const vector<int> &s) {
    int n = int(s.size()); vector<int> sa(n); std::iota(sa.begin(), sa.end(), 0);
    std::sort(sa.begin(), sa.end(), [&](int a, int b) { return std::lexicographical_compare(s.begin() + a, s.end(), s.begin() + b, s.end()); });
    lng res = 0;
    for (int k = 0; k < n; ++k) {
        int l = 0; while (k && sa[k] + l < n && sa[k - 1] + l < n && s[sa[k] + l] == s[sa[k - 1] + l]) { ++l; }
        res += n - sa[k] - l;}
    return res;}
// Applies operation kind (0 pushBack, 1 pushFront, 2 popBack, 3 popFront) of symbol pool[c] to d and its id mirror m.
template<class T> string apply(DistinctSubstringsDeque<T> &d, vector<int> &m, const vector<T> &pool, int kind, int c) {
    if (kind == 0) { d.pushBack(pool[c]); m.push_back(c); }
    if (kind == 1) { d.pushFront(pool[c]); m.insert(m.begin(), c); }
    if (kind == 2) { d.popBack(); m.pop_back(); }
    if (kind == 3) { d.popFront(); m.erase(m.begin()); }
    return string(1, "BFbf"[kind]) + (kind < 2 ? std::to_string(c) : "") + " ";}
template<class T> void verify(const DistinctSubstringsDeque<T> &d, const vector<int> &m, const vector<T> &pool, lng expected, const string &operation) {
    bool same = d.s.size() == m.size();
    for (int i = 0; same && i < int(m.size()); ++i) { same = d.s[i] == pool[m[i]]; }
    vector<int> ids;
    for (const auto &x : d.s) {
        if (!same) { ids.push_back(int(std::find(pool.begin(), pool.end(), x) - pool.begin())); }}
    check(d.distinctSubstrings(), expected, operation); check(d.total, expected, operation + "-total"); check(same ? m : ids, m, operation + "-state");}
void dequeExhaustive() {
    vector<int> pool{0, 1}; map<vector<int>, lng> oracle;
    auto go = [&](auto &&self, const DistinctSubstringsDeque<int> &d, const vector<int> &m, const string &ops, int left) -> void {
        context = "deque ops=" + ops;
        // Both oracles depend on the string only, so each of the at most 2^10 strings is computed and cross-checked once.
        auto it = oracle.find(m);
        if (it == oracle.end()) {
            lng want = naiveDistinct(m); check(suffixDistinct(m), want, "deque-oracle-agreement");
            it = oracle.emplace(m, want).first;}
        verify(d, m, pool, it->second, "deque-exhaustive");
        if (!left) { return; }
        for (int kind = 0; kind < 4; ++kind) {
            for (int c = 0; c < (kind < 2 ? 2 : 1); ++c) {
                if (kind >= 2 && m.empty()) { continue; }
                auto e = d; auto n = m; auto op = apply(e, n, pool, kind, c); self(self, e, n, ops + op, left - 1);}}};
    go(go, DistinctSubstringsDeque<int>{}, {}, "", limit(6, 8, 9));}
// Random sequences over alphabets of 1-4 symbols drawn from pool, at most 150 symbols, checked after every operation.
template<class T> void dequeRandom(const vector<T> &pool, const string &name) {
    for (int rep = 0; rep < limit(20, 150, 800); ++rep) {
        vector<int> alpha(pool.size()); std::iota(alpha.begin(), alpha.end(), 0); std::shuffle(alpha.begin(), alpha.end(), rng);
        alpha.resize(1 + rng() % min<size_t>(4, pool.size()));
        DistinctSubstringsDeque<T> d; vector<int> m; string ops;
        for (int step = int(rng() % 200); step >= 0; --step) {
            bool push = m.empty() || (m.size() < 150 && rng() % 3);
            ops += apply(d, m, pool, int(rng() % 2) + (push ? 0 : 2), alpha[rng() % alpha.size()]);
            context = "deque " + name + " alphabet=" + show(alpha) + " ops=" + ops; verify(d, m, pool, suffixDistinct(m), "deque-random-" + name);}}}
void dequeBytes() {
    vector<char> pool; for (int c = 0; c < 256; ++c) { pool.push_back(char(c)); }
    DistinctSubstringsDeque<> d; vector<int> m; string ops;
    for (int c = 0; c < 256; ++c) {
        ops += apply(d, m, pool, int(rng() % 2), c);
        context = "deque bytes ops=" + ops; verify(d, m, pool, suffixDistinct(m), "deque-byte-alphabet");}
    check(d.distinctSubstrings(), 256LL * 257 / 2, "deque-byte-alphabet-closed-form");
    for (int k = 0; k < 2; ++k) {
        DistinctSubstringsDeque<> e; vector<int> n; ops.clear();
        for (int step = 0; step < 120; ++step) {
            ops += apply(e, n, pool, int(rng() % 2), vector<int>{0, 255, 128, 127, 97}[rng() % 5]);
            context = "deque NUL/negative ops=" + ops; verify(e, n, pool, suffixDistinct(n), "deque-signed-bytes");}
        for (auto *x : {&d, &e}) {
            auto &y = x == &d ? m : n;
            while (!y.empty()) {
                ops += apply(*x, y, pool, k ? 2 + int(rng() % 2) : 2 + int(y.size() % 2), 0);
                context = "deque drain ops=" + ops; verify(*x, y, pool, suffixDistinct(y), "deque-drain");}
            check(x->s.empty(), true, "deque-drain-empty"); check(x->distinctSubstrings(), 0LL, "deque-drain-zero");}}}
void dequeLarge() {
    int n = limit(1500, 10000, 30000); context = "deque large n=" + std::to_string(n);
    DistinctSubstringsDeque<int> d, a;
    for (int i = 0; i < n; ++i) {
        if (i % 2) { d.pushBack(7); } else { d.pushFront(7); }
        check(d.distinctSubstrings(), lng(i + 1), "deque-large-unary-push");}
    d.pushBack(9); check(d.distinctSubstrings(), 2LL * n + 1, "deque-large-near-unary-back");
    d.pushFront(9); check(d.distinctSubstrings(), 3LL * n + 2, "deque-large-near-unary-both");
    d.popFront(); check(d.distinctSubstrings(), 2LL * n + 1, "deque-large-near-unary-pop-front");
    d.popBack(); check(d.distinctSubstrings(), lng(n), "deque-large-near-unary-pop-back");
    for (int i = n; i > 0; --i) {
        if (i % 2) { d.popBack(); } else { d.popFront(); }
        check(d.distinctSubstrings(), lng(i - 1), "deque-large-unary-pop");}
    for (int i = 0; i < n; ++i) { a.pushBack(i % 2); check(a.distinctSubstrings(), 2LL * i + 1, "deque-large-alternating"); }}
void dequeCopy() {
    vector<int> pool{0, 1, 2}; DistinctSubstringsDeque<int> d; vector<int> m; string ops;
    for (int i = 0; i < 40; ++i) { ops += apply(d, m, pool, int(rng() % 2), int(rng() % 3)); }
    context = "deque copy ops=" + ops; lng want = suffixDistinct(m);
    auto c = d; auto cm = m;
    apply(c, cm, pool, 0, 2); apply(c, cm, pool, 1, 0); apply(c, cm, pool, 3, 0); apply(c, cm, pool, 2, 0); apply(c, cm, pool, 2, 0);
    verify(d, m, pool, want, "deque-copy-original"); verify(c, cm, pool, suffixDistinct(cm), "deque-copy-mutated");
    c = d; cm = m; while (!m.empty()) { apply(d, m, pool, 3, 0); }
    verify(d, m, pool, 0, "deque-copy-assigned-source"); verify(c, cm, pool, want, "deque-copy-assigned");}
int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--invalid") {
        if (string(argv[2]) == "length") { zFunction(OversizedSequence{}); }
        if (string(argv[2]) == "pattern-length") { extendedZ(OversizedSequence{}, vector<int>{}); }
        if (string(argv[2]) == "text-length") { extendedZ(vector<int>{}, OversizedSequence{}); }
        if (string(argv[2]) == "length-huge") { zFunction(OversizedSequence{size_t(INT_MAX) + 1}); }
        if (string(argv[2]) == "text-length-huge") { extendedZ(vector<int>{}, OversizedSequence{size_t(INT_MAX) + 1}); }
        if (string(argv[2]) == "deque-pop-back-empty") { DistinctSubstringsDeque<> d; d.popBack(); }
        if (string(argv[2]) == "deque-pop-front-empty") { DistinctSubstringsDeque<> d; d.popFront(); }
        return 3;}
    configure(argc, argv);
    words(limit(5, 8, 9), 2, testString);
    words(limit(3, 5, 6), 2, [&](const auto &p) { words(limit(3, 5, 6), 2, [&](const auto &s) { testMatch(p, s); }); });
    for (int rep = 0; rep < limit(100, 1200, 8000); ++rep) {
        vector<int> s(rng() % 60), p(rng() % 30);
        for (auto &c : s) { c = int(rng() % 5) - 2; } for (auto &c : p) { c = int(rng() % 5) - 2; }
        testString(s); testMatch(p, s);}
    string bytes; vector<int> integer_bytes;
    for (int c = 0; c < 256; ++c) { bytes += char(c); integer_bytes.push_back(c); }
    check(zFunction(bytes), naiveLcp(integer_bytes, integer_bytes), "full-byte-alphabet");
    check(zOccurrences(string_view(bytes).substr(0, 128), bytes + bytes), vector<int>({0, 256}), "embedded-NUL-matching");
    check(zFunction(vector<lng>{LLONG_MIN, LLONG_MAX, LLONG_MIN}), vector<int>({3, 0, 1}), "wide-integer-alphabet");
    vector<int> large(limit(10000, 200000, 1000000), 7), expected(large.size());
    for (int i = 0; i < int(large.size()); ++i) { expected[i] = int(large.size()) - i; }
    check(zFunction(large), expected, "large-unary");
    vector<int> pi; check(zToPrefix(expected, pi), true, "large-conversion-status");
    check(pi.back(), int(large.size()) - 1, "large-conversion");
    check(extendedZ(vector<int>(1000, 7), large).back(), 1, "large-extended");
    large.back() = 9; check(zFunction(large)[1], int(large.size()) - 2, "large-near-unary");
    context = "mixed-signedness regressions";
    check(extendedZ(vector<uint>{UINT_MAX}, vector<int>{-1}), vector<int>{0}, "uint-int-extendedZ");
    check(zOccurrences(vector<ulng>{~0ULL}, vector<int>{-1}), vector<int>{}, "ulng-int-zOccurrences");
    check(extendedZ(vector<ulll>{~ulll(0)}, vector<int>{-1}), vector<int>{0}, "ulll-int-extendedZ");
    vector<lng> pool{0, 1, -1, UINT_MAX, lng(UINT_MAX) + 1, LLONG_MIN};
    for (int rep = 0; rep < limit(200, 3000, 20000); ++rep) {
        vector<ulng> p(rng() % 4);
        vector<lng> t(rng() % 12);
        for (auto &x : p) { x = ulng(pool[rng() % 4]); }
        for (auto &x : t) { x = pool[rng() % pool.size()]; }
        auto [a, b] = valueIds(p, t);
        context = "mixed p=" + show(p) + " t=" + show(t);
        check(extendedZ(p, t), naiveLcp(a, b), "mixed-extendedZ");
        check(zOccurrences(p, t), naiveMatches(a, b), "mixed-zOccurrences");}
    for (int rep = 0; rep < limit(100, 1000, 5000); ++rep) {
        IndexOnly p{vector<int>(rng() % 5)}, t{vector<int>(rng() % 20)};
        for (auto &x : p.v) { x = int(rng() % 3); }
        for (auto &x : t.v) { x = int(rng() % 3); }
        context = "index-only p=" + show(p.v) + " t=" + show(t.v);
        check(zFunction(t), naiveLcp(t.v, t.v), "index-only-zFunction");
        check(extendedZ(p, t), naiveLcp(p.v, t.v), "index-only-extendedZ");
        check(zOccurrences(p, t), naiveMatches(p.v, t.v), "index-only-zOccurrences");}
    dequeExhaustive(); dequeRandom(vector<int>{-2, -1, 0, 1}, "int");
    dequeRandom(vector<lng>{LLONG_MIN, LLONG_MAX, 0, -1}, "lng"); dequeRandom(vector<EqOnly>{{-3}, {0}, {INT_MIN}, {INT_MAX}}, "equality-only");
    dequeBytes(); dequeLarge(); dequeCopy(); comparisonBounds();
    validation(); cout << "PASS z seed=" << seed << " mode=" << mode << " checks=" << cases << '\n';}
