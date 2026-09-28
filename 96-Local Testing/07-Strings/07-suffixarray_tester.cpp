#include "../../07-Strings/07-suffixarray.hpp"

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
template<class T> int directLcp(const vector<T> &a, int i, const vector<T> &b, int j) {
    int k = 0;
    while (i + k < int(a.size()) && j + k < int(b.size()) && a[i + k] == b[j + k]) { ++k; }
    return k;
}
template<class T> bool sliceLess(const vector<T> &s, int i, int j, int n) {
    return std::lexicographical_compare(s.begin() + i, s.begin() + i + n, s.begin() + j, s.begin() + j + n);
}
template<class T> pair<int, int> repeatedOracle(const vector<T> &s) {
    int start = 0, best = 0;
    for (int i = 0; i < int(s.size()); ++i) { for (int j = i + 1; j < int(s.size()); ++j) {
        int len = directLcp(s, i, s, j);
        if (len > best || (len == best && sliceLess(s, i, start, len))) { start = i; best = len; } } }
    return {start, best};
}
template<class T> tuple<int, int, int> commonOracle(const vector<T> &a, const vector<T> &b) {
    int x = 0, y = 0, best = 0;
    for (int i = 0; i < int(a.size()); ++i) { for (int j = 0; j < int(b.size()); ++j) {
        int len = directLcp(a, i, b, j);
        if (len > best || (len == best && sliceLess(a, i, x, len))) { best = len; x = i; y = j; } } }
    return {x, y, best};
}
template<class T> void patternCheck(const SuffixArray<T> &index, const vector<T> &p) {
    int n = index.size(), m = int(p.size()), l = 0, r = 0;
    for (int pos : index.sa) {
        int k = directLcp(index.text, pos, p, 0);
        bool less = k < m && (k == n - pos || index.text[pos + k] < p[k]);
        l += less; r += less || k == m; }
    auto got = index.patternRange(p);
    check(got == pair<int, int>{l, r}, "pattern range m=" + std::to_string(m)
        + " expected=" + std::to_string(l) + ',' + std::to_string(r)
        + " actual=" + std::to_string(got.first) + ',' + std::to_string(got.second));
}
template<class T> void verify(const vector<T> &s, bool byte_case = false) {
    ++cases; int n = int(s.size()); SuffixArray<T> index(s), plain(s, false);
    vector<int> expected(n); iota(expected.begin(), expected.end(), 0);
    sort(expected.begin(), expected.end(), [&](int i, int j) {
        return std::lexicographical_compare(s.begin() + i, s.end(), s.begin() + j, s.end()); });
    check(index.sa == expected && plain.sa == expected, "suffix order naive sorted suffixes");
    check(index.text == s && index.indexed && !plain.indexed && plain.rmq.v.empty(), "storage/configuration");
    expectEqual(index.size(), n, "size"); expectEqual(int(index.lcp.size()), max(0, n - 1), "LCP length");
    for (int i = 0; i < n; ++i) { expectEqual(index.rank[index.sa[i]], i, "inverse rank"); }
    for (int i = 0; i + 1 < n; ++i) {
        expectEqual(index.lcp[i], directLcp(s, index.sa[i], s, index.sa[i + 1]), "Kasai adjacent LCP"); }
    for (int i = 0; i <= n; ++i) { for (int j = 0; j <= n; ++j) {
        expectEqual(index.lce(i, j), directLcp(s, i, s, j), "LCE i=" + std::to_string(i) + " j=" + std::to_string(j)); } }
    if (n <= 5) {
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
            for (int a = 0; a <= n; ++a) { for (int b = a; b <= n; ++b) {
                int len = 0; while (l + len < r && a + len < b && s[l + len] == s[a + len]) { ++len; }
                expectEqual(index.substringLce(l, r, a, b), len, "substring LCE"); } } } } }
    auto testPattern = [&](const vector<T> &p) { patternCheck(index, p); patternCheck(plain, p); };
    testPattern({}); testPattern(vector<T>(n + 2, T(0))); testPattern(vector<T>{T(0)});
    testPattern(vector<T>{std::numeric_limits<T>::min()}); testPattern(vector<T>{std::numeric_limits<T>::max()});
    if (n <= 10) {
        for (int l = 0; l < n; ++l) { for (int r = l + 1; r <= n; ++r) {
            testPattern(vector<T>(s.begin() + l, s.begin() + r)); } } }
    else {
        for (int i = 0; i < min(25, n); ++i) {
            int l = i * 7 % n, r = min(n, l + i); testPattern(vector<T>(s.begin() + l, s.begin() + r)); } }
    check(index.longestRepeated() == repeatedOracle(s) && plain.longestRepeated() == repeatedOracle(s), "longest repeated length/witness/tie");
    plain.buildRmq(); check(plain.indexed && plain.lcp == index.lcp && plain.rank == index.rank, "deferred RMQ");
    for (int i = 0; i <= n; ++i) { expectEqual(plain.lce(i, n / 2), index.lce(i, n / 2), "deferred LCE"); }
    plain.buildRmq(); expectEqual(plain.lce(0, 0), n, "repeat RMQ build");
    if constexpr (std::is_same_v<T, int>) {
        if (byte_case) {
            string bytes; for (int c : s) { bytes += char(c); }
            SuffixArray byte_index{string_view(bytes)}, copy{bytes}, no_rmq(string_view(bytes), false);
            check(byte_index.sa == expected && copy.sa == expected && no_rmq.sa == expected, "unsigned byte initialization");
            check(byte_index.lcp == index.lcp && byte_index.rank == index.rank, "byte LCP/ranks");
            check(byte_index.patternRange(string_view(bytes)) == index.patternRange(s), "byte pattern embedded NUL");
            check(byte_index.patternRange(string_view()) == pair<int, int>{0, n}, "empty byte pattern"); } }
}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 5 : mode == "full" ? 7 : 8, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            vector<int> s(n); int x = code;
            for (int &c : s) { c = x % 3; x /= 3; }
            context = "ternary n=" + std::to_string(n) + " code=" + std::to_string(code); verify(s, true); } }
    cout << "PASS exhaustive suffix/rank/LCP/LCE/pattern/repeated ternary lengths=0.." << bound << '\n';
    int pair_bound = mode == "quick" ? 3 : mode == "full" ? 5 : 6;
    vector<vector<int>> texts{{}}; vector<string> bytes{""};
    for (int n = 1; n <= pair_bound; ++n) { for (int code = 0; code < (1 << n); ++code) {
        vector<int> s(n); string b(n, '\0');
        for (int i = 0; i < n; ++i) { s[i] = (code >> i & 1) * 255; b[i] = char(s[i]); }
        texts.push_back(s); bytes.push_back(b); } }
    for (int i = 0; i < int(texts.size()); ++i) { for (int j = 0; j < int(texts.size()); ++j) {
        context = "binary pair first=" + std::to_string(i) + " second=" + std::to_string(j);
        auto expected = commonOracle(texts[i], texts[j]);
        check(longestCommonSubstring(texts[i], texts[j]) == expected, "integer LCS length/witness/tie");
        check(longestCommonSubstring(bytes[i], bytes[j]) == expected, "byte LCS length/witness/tie"); } }
    cout << "PASS exhaustive two-input LCS binary lengths=0.." << pair_bound << '\n';
}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed); int count = mode == "quick" ? 150 : mode == "full" ? 1500 : 7000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 51), m = int(rng() % 41), alphabet = rep % 3 ? 4 : 256;
        vector<int> a(n), b(m); for (int &c : a) { c = int(rng() % alphabet); } for (int &c : b) { c = int(rng() % alphabet); }
        context = "random case=" + std::to_string(rep) + " text=";
        for (int c : a) { context += std::to_string(c) + ','; }
        verify(a, true); auto expected = commonOracle(a, b);
        string a_bytes, b_bytes; for (int c : a) { a_bytes += char(c); } for (int c : b) { b_bytes += char(c); }
        check(longestCommonSubstring(a, b) == expected, "random LCS integer");
        check(longestCommonSubstring(a_bytes, b_bytes) == expected, "random LCS byte");
        vector<lng> signed_a, signed_b; vector<ulng> unsigned_a, unsigned_b;
        auto encode = [](int c) { return c % 2 ? std::numeric_limits<lng>::min() + c / 2 : std::numeric_limits<lng>::max() - c / 2; };
        for (int c : a) { signed_a.push_back(encode(c)); unsigned_a.push_back(ulng(encode(c))); }
        for (int c : b) { signed_b.push_back(encode(c)); unsigned_b.push_back(ulng(encode(c))); }
        if (rep < count / 4) { verify(signed_a); verify(unsigned_a); }
        check(longestCommonSubstring(signed_a, signed_b) == commonOracle(signed_a, signed_b), "signed extreme LCS");
        check(longestCommonSubstring(unsigned_a, unsigned_b) == commonOracle(unsigned_a, unsigned_b), "unsigned extreme LCS");
        SuffixArray index(a); vector<int> pattern(int(rng() % 25));
        for (int &c : pattern) { c = int(rng() % alphabet); } patternCheck(index, pattern);
        for (int q = 0; q < 20; ++q) {
            int l = int(rng() % (n + 1)), r = int(rng() % (n + 1)), x = int(rng() % (n + 1)), y = int(rng() % (n + 1));
            if (l > r) { swap(l, r); } if (x > y) { swap(x, y); }
            int len = 0; while (l + len < r && x + len < y && a[l + len] == a[x + len]) { ++len; }
            expectEqual(index.substringLce(l, r, x, y), len, "random substring LCE"); } }
    vector<int> all; for (int c = 0; c < 256; ++c) { all.push_back(c); }
    context = "all 256 bytes ascending"; verify(all, true);
    reverse(all.begin(), all.end()); context = "all 256 bytes descending"; verify(all, true);
    context = "bool alphabet"; verify(vector<bool>{true, false, true, true, false});
    context = "signed char alphabet"; verify(vector<signed char>{127, -128, 0, -128});
    cout << "PASS seeded byte/sparse integer/extreme alphabets cases=" << count << '\n';
}
void large(const string &mode) {
    int n = mode == "quick" ? 5000 : mode == "full" ? 200000 : 700000;
    string s(n, char(255)); context = "large unary n=" + std::to_string(n); SuffixArray index(s);
    for (int i = 0; i < n; ++i) { expectEqual(index.sa[i], n - i - 1, "unary order"); }
    for (int i = 0; i + 1 < n; ++i) { expectEqual(index.lcp[i], i + 1, "unary LCP"); }
    check(index.longestRepeated() == pair<int, int>{0, n - 1}, "unary repeated");
    check(index.patternRange(string(n / 2, char(255))) == pair<int, int>{n / 2 - 1, n}, "long unary pattern");
    check(index.patternRange(string(n + 1, char(255))) == pair<int, int>{n, n}, "long absent pattern");
    for (int i = 0; i < n; ++i) { s[i] = char(i % 3); }
    SuffixArray periodic(s); context = "large period three n=" + std::to_string(n);
    for (int i = 1; i < n; ++i) {
        int x = periodic.sa[i - 1], y = periodic.sa[i];
        check(x % 3 < y % 3 || (x % 3 == y % 3 && x > y), "periodic suffix order");
        expectEqual(periodic.lcp[i - 1], x % 3 == y % 3 ? n - max(x, y) : 0, "periodic LCP"); }
    expectEqual(periodic.lce(0, 3), n - 3, "periodic LCE");
    check(periodic.longestRepeated() == pair<int, int>{0, n - 3}, "periodic repeated");
    SuffixArray copy = periodic, moved = std::move(copy); check(moved.sa == periodic.sa && moved.lce(0, 3) == n - 3, "copy/move destination");
    copy = SuffixArray<>(); check(copy.sa.empty() && copy.lce(0, 0) == 0, "reset moved-from");
    copy = index; check(copy.lcp == index.lcp && copy.lce(0, 1) == n - 1, "assignment");
    check(SuffixArray("banana").sa == vector<int>{5, 3, 1, 0, 4, 2}, "C-string deduction");
    check(longestCommonSubstring(string_view(s).substr(0, n / 2), string_view(s).substr(3)) == tuple<int, int, int>{0, 0, n / 2}, "large common substring");
    cout << "PASS large unary/periodic/lifecycle/LCS n=" << n << '\n';
}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; } }
    if (!invalid.empty()) {
        SuffixArray a("aba");
        if (invalid == "negative-lce") { a.lce(-1, 0); }
        if (invalid == "large-lce") { a.lce(4, 0); }
        if (invalid == "negative-second") { a.lce(0, -1); }
        if (invalid == "large-second") { a.lce(0, 4); }
        if (invalid == "missing-rmq") { SuffixArray("a", false).lce(0, 0); }
        if (invalid == "negative-substring") { a.substringLce(-1, 1, 0, 0); }
        if (invalid == "reversed-substring") { a.substringLce(2, 1, 0, 0); }
        if (invalid == "large-substring") { a.substringLce(0, 4, 0, 0); }
        if (invalid == "reversed-second") { a.substringLce(0, 0, 2, 1); }
        if (invalid == "byte-domain") { SuffixArray bad(vector<int>{-1}); bad.build(true, false); }
        return 0;
    }
    exhaustive(mode); randomCases(mode); large(mode);
    cout << "PASS suffixarray seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';
}
