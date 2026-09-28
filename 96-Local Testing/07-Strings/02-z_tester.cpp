#include "../../07-Strings/02-z.hpp"
#include "01-prefix_z_test_support.hpp"

struct OversizedSequence {
    size_t size() const { return INT_MAX; }
    int operator[](int) const { return 0; }
};

void testString(const vector<int> &s) {
    context = "s=" + show(s); auto z = naiveLcp(s, s), pi = naivePi(s), periods = naivePeriods(s);
    check(zFunction(s), z, "zFunction"); check(validZFunction(z), true, "validZFunction");
    check(zBorders(z), naiveBorders(s), "zBorders"); check(zBorders(z, true), naiveBorders(s, true), "zBorders-full");
    check(zPeriods(z), periods, "zPeriods"); check(zPeriod(z), periods.empty() ? 0 : periods[0], "zPeriod");
    int whole = 0; for (int p : periods) { if (int(s.size()) % p == 0) { whole = p; break; } }
    check(zPeriod(z, true), whole, "zPeriod-whole");
    vector<int> out{99}; check(prefixToZ(pi, out), true, "prefixToZ-status"); check(out, z, "prefixToZ");
    check(zToPrefix(z, out), true, "zToPrefix-status"); check(out, pi, "zToPrefix");
    out = pi; check(prefixToZ(out, out), true, "prefixToZ-alias-status"); check(out, z, "prefixToZ-alias");
    check(zToPrefix(out, out), true, "zToPrefix-alias-status"); check(out, pi, "zToPrefix-alias");
}
void testMatch(const vector<int> &p, const vector<int> &s) {
    context = "p=" + show(p) + " s=" + show(s);
    check(extendedZ(p, s), naiveLcp(p, s), "extendedZ");
    check(zOccurrences(p, s), naiveMatches(p, s), "zOccurrences");
}
void validation() {
    for (int n = 0; n <= limit(5, 8, 9); ++n) {
        map<vector<int>, vector<int>> valid;
        partitions(n, [&](const auto &s) { valid[naiveLcp(s, s)] = naivePi(s); });
        vector<int> z(n); if (n) { z[0] = n; }
        auto go = [&](auto &&self, int i) -> void {
            if (i == n) {
                context = "z=" + show(z); bool ok = valid.contains(z); vector<int> out{99};
                check(validZFunction(z), ok, "exhaustive-z-validation");
                check(zToPrefix(z, out), ok, "exhaustive-z-conversion-status");
                check(out, ok ? valid[z] : vector<int>{}, "exhaustive-z-conversion"); return; }
            for (z[i] = 0; z[i] <= n - i; ++z[i]) { self(self, i + 1); }
        };
        go(go, n ? 1 : 0); }
    for (auto z : vector<vector<int>>{{-1}, {0}, {2}, {2, 2}, {2, -1}, {3, 2, 0}, {2, INT_MAX}, {2, INT_MIN}}) {
        context = "z=" + show(z); auto alias = z; vector<int> out{9};
        check(zToPrefix(z, out), false, "invalid-z-status"); check(out.empty(), true, "invalid-z-clear");
        check(zToPrefix(alias, alias), false, "invalid-z-alias-status"); check(alias.empty(), true, "invalid-z-alias-clear"); }
    for (auto pi : vector<vector<int>>{{-1}, {1}, {0, 1, 1}, {0, 0, 2}, {0, INT_MAX}}) {
        context = "pi=" + show(pi); check(prefixToZ(pi, pi), false, "invalid-pi-alias-status"); check(pi.empty(), true, "invalid-pi-clear"); }
}
int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--invalid") {
        if (string(argv[2]) == "length") { zFunction(OversizedSequence{}); }
        if (string(argv[2]) == "pattern-length") { extendedZ(OversizedSequence{}, vector<int>{}); }
        if (string(argv[2]) == "text-length") { extendedZ(vector<int>{}, OversizedSequence{}); }
        return 3; }
    configure(argc, argv);
    words(limit(5, 8, 9), 2, testString);
    words(limit(3, 5, 6), 2, [&](const auto &p) { words(limit(3, 5, 6), 2, [&](const auto &s) { testMatch(p, s); }); });
    for (int rep = 0; rep < limit(100, 1200, 8000); ++rep) {
        vector<int> s(rng() % 60), p(rng() % 30);
        for (auto &c : s) { c = int(rng() % 5) - 2; } for (auto &c : p) { c = int(rng() % 5) - 2; }
        testString(s); testMatch(p, s); }
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
    validation(); cout << "PASS z seed=" << seed << " mode=" << mode << " checks=" << cases << '\n';
}
