#include "../../07-Strings/05-manacher.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &op) {
    ++checks; if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << op << " expected=true actual=false\n"; std::exit(1);
}
void expectEqual(int actual, int expected, const string &op) {
    ++checks; if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual << '\n'; std::exit(1);
}
template<class S> bool palindrome(const S &s, int l, int r) {
    while (l < r) { if (!(s[l++] == s[--r])) { return false; } }
    return true;
}
template<class S> void verify(const S &s) {
    ++cases; int n = int(s.size()); Manacher radii(s);
    expectEqual(radii.size(), n, "size"); expectEqual(int(radii.even.size()), n + 1, "all even gaps");
    vector<int> odd(n), even(n + 1);
    // Enumerate all intervals and test each directly; no mirrored-radius recurrence.
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        if (!palindrome(s, l, r)) { continue; }
        int len = r - l, i = l + len / 2;
        if (len % 2) { odd[i] = max(odd[i], len / 2 + 1); }
        else { even[i] = max(even[i], len / 2); }
    } }
    for (int i = 0; i < n; ++i) {
        expectEqual(radii.odd[i], odd[i], "odd radius center=" + std::to_string(i));
        check(radii.oddInterval(i) == pair<int, int>{i - odd[i] + 1, i + odd[i]}, "maximum odd interval");
        check(radii.oddInclusive(i) == pair<int, int>{i - odd[i] + 1, i + odd[i] - 1}, "maximum odd inclusive");
        for (int k = 1; k <= odd[i]; ++k) {
            auto [l, r] = radii.oddInterval(i, k);
            check(l == i - k + 1 && r == i + k && palindrome(s, l, r), "nested odd interval");
            check(radii.oddInclusive(i, k) == pair<int, int>{l, r - 1}, "nested odd inclusive"); } }
    for (int i = 0; i <= n; ++i) {
        expectEqual(radii.even[i], even[i], "even radius gap=" + std::to_string(i));
        check(radii.evenInterval(i) == pair<int, int>{i - even[i], i + even[i]}, "maximum even interval");
        check(radii.evenInclusive(i) == pair<int, int>{i - even[i], i + even[i] - 1}, "maximum even inclusive");
        for (int k = 0; k <= even[i]; ++k) {
            auto [l, r] = radii.evenInterval(i, k);
            check(l == i - k && r == i + k && palindrome(s, l, r), "nested even interval");
            check(radii.evenInclusive(i, k) == pair<int, int>{l, r - 1}, "nested even inclusive"); } }
}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 8 : 9, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            string s(n, '\0'); int x = code;
            for (char &c : s) { c = char(x % 3); x /= 3; }
            context = "ternary n=" + std::to_string(n) + " code=" + std::to_string(code); verify(s); } }
    cout << "PASS exhaustive direct interval oracle ternary lengths=0.." << bound << '\n';
}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed); int count = mode == "quick" ? 300 : mode == "full" ? 2000 : 15000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 51); string s(n, '\0');
        for (char &c : s) { c = char(rng() % (rep % 2 ? 256 : 4)); }
        if (rep % 5 == 0) { for (int i = 0; i < n / 2; ++i) { s[n - 1 - i] = s[i]; } }
        context = "random case=" + std::to_string(rep) + " bytes=";
        for (unsigned char c : s) { context += std::to_string(c) + ','; }
        verify(string_view(s));
        vector<lng> ints;
        for (unsigned char c : s) { ints.push_back(c % 2 ? std::numeric_limits<lng>::min() + c : std::numeric_limits<lng>::max() - c); }
        verify(ints); }
    string bytes; for (int c = 0; c < 256; ++c) { bytes += char(c); }
    context = "all 256 bytes"; verify(bytes);
    cout << "PASS seeded byte/integer extreme alphabets cases=" << count << '\n';
}
void large(const string &mode) {
    int n = mode == "quick" ? 5000 : mode == "full" ? 300000 : 1000000;
    string s(n, char(255)); context = "large unary n=" + std::to_string(n); Manacher unary(s);
    for (int i = 0; i < n; ++i) { expectEqual(unary.odd[i], min(i + 1, n - i), "unary odd"); }
    for (int i = 0; i <= n; ++i) { expectEqual(unary.even[i], min(i, n - i), "unary even"); }
    for (int i = 0; i < n; ++i) { s[i] = char(i % 2); }
    context = "large alternating n=" + std::to_string(n); Manacher alternating(s);
    for (int i = 0; i < n; ++i) { expectEqual(alternating.odd[i], min(i + 1, n - i), "alternating odd"); }
    for (int i = 0; i <= n; ++i) { expectEqual(alternating.even[i], 0, "alternating even"); }
    Manacher copy = unary, moved = std::move(copy);
    check(moved.odd == unary.odd && moved.even == unary.even, "copy/move destination");
    copy = Manacher(); check(copy.odd.empty() && copy.even == vector<int>{0}, "reset moved-from");
    check(copy.evenInterval(0) == pair<int, int>{0, 0} && copy.evenInclusive(0) == pair<int, int>{0, -1}, "default empty intervals");
    copy = alternating; check(copy.odd == alternating.odd && copy.even == alternating.even, "assignment rebuild");
    cout << "PASS large unary/alternating and copy/move/reset n=" << n << '\n';
}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; } }
    if (!invalid.empty()) {
        Manacher m(string_view("aba"));
        if (invalid == "odd-negative-center") { m.oddInterval(-1); }
        if (invalid == "odd-end-center") { m.oddInterval(3); }
        if (invalid == "odd-empty") { Manacher().oddInterval(0); }
        if (invalid == "even-negative-center") { m.evenInterval(-1); }
        if (invalid == "even-large-center") { m.evenInterval(4); }
        if (invalid == "odd-zero-radius") { m.oddInterval(1, 0); }
        if (invalid == "odd-negative-radius") { m.oddInterval(1, -2); }
        if (invalid == "odd-large-radius") { m.oddInterval(1, 3); }
        if (invalid == "even-negative-radius") { m.evenInterval(1, -2); }
        if (invalid == "even-large-radius") { m.evenInterval(1, 1); }
        return 0;
    }
    exhaustive(mode); randomCases(mode); large(mode);
    cout << "PASS manacher seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';
}
