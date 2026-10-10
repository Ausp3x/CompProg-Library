#include "../../07-Strings/16-string_matching.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A, class B> void expect(const A &actual, const B &expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=";
    if constexpr (std::is_arithmetic_v<B>) { cerr << expected << " actual=" << actual; }
    else {
        for (auto x : expected) { cerr << x << ','; }
        cerr << " actual=";
        for (auto x : actual) { cerr << x << ','; }}
    cerr << '\n';
    std::exit(1);}
// Brute force: every start i in [0, n - m] compared symbol by symbol with std::equal.
template<class P, class S> vector<int> brute(const P &p, const S &t) {
    vector<int> res;
    for (int i = 0; i + int(p.size()) <= int(t.size()); ++i) {
        if (std::equal(p.begin(), p.end(), t.begin() + i)) { res.push_back(i); }}
    return res;}
template<class C> int symbol(C c) {
    if constexpr (std::is_same_v<C, char>) { return uint8_t(c); }
    else { return int(c); }}
// Brute strong good suffix: smallest s with p[k - s] = p[k] for k in (j, m), k >= s, and p[j - s] != p[j] when j >= s.
template<class P> vector<int> bruteGoodSuffix(const P &p) {
    int m = int(p.size());
    vector<int> res(m);
    for (int j = 0; j < m; ++j) {
        for (int s = 1; s <= m; ++s) {
            bool ok = j < s || p[j - s] != p[j];
            for (int k = j + 1; k < m && ok; ++k) { ok = k < s || p[k - s] == p[k]; }
            if (ok) { res[j] = s; break; }}}
    return res;}
// Checks every coded algorithm (symbols in [0, sigma)) and the generic ones on one pair.
template<class P, class S> void verify(const P &p, const S &t, int sigma, bool coded = true) {
    ++cases;
    auto expected = brute(p, t);
    int m = int(p.size());
    expect(naiveSearch(p, t), expected, "naiveSearch");
    expect(rabinKarp(p, t), expected, "rabinKarp");
    expect(rabinKarp(p, t, test_seed + ulng(cases)), expected, "rabinKarp seeded");
    expect(twoWay(p, t), expected, "twoWay");
    expect(allOccurrences(p, t), expected, "allOccurrences");
    expect(firstOccurrence(p, t), expected.empty() ? -1 : expected[0], "firstOccurrence");
    expect(isSubstring(p, t), !expected.empty(), "isSubstring");
    expect(countOccurrences(p, t), lng(expected.size()), "countOccurrences overlapping");
    lng disjoint = 0;
    for (lng i = 0, next = 0; i < lng(expected.size()); ++i) {
        if (expected[i] >= next) { ++disjoint; next = expected[i] + m; }}
    expect(countOccurrences(p, t, false), disjoint, "countOccurrences disjoint");
    if (!coded) { return; }
    expect(boyerMoore(p, t, sigma), expected, "boyerMoore");
    expect(horspool(p, t, sigma), expected, "horspool");
    expect(sunday(p, t, sigma), expected, "sunday");
    expect(dfaSearch(p, t, sigma), expected, "dfaSearch pattern");
    vector<int> codes;
    for (auto c : p) { codes.push_back(symbol(c)); }
    expect(dfaSearch(prefixAutomaton(codes, sigma), t), expected, "dfaSearch automaton");
    for (int len : {m, max(m - 1, 0), m / 2}) {
        vector<int> last(sigma, -1);
        for (int i = 0; i < len; ++i) { last[symbol(p[i])] = i; }
        expect(badCharacterTable(p, sigma, len), last, "badCharacterTable len=" + std::to_string(len));}
    expect(badCharacterTable(p, sigma), badCharacterTable(p, sigma, m), "badCharacterTable default");
    auto gs = goodSuffixTable(p);
    expect(gs, bruteGoodSuffix(p), "goodSuffixTable");
    if (m) { expect(gs[0], prefixPeriod(prefixFunction(p)), "goodSuffixTable period"); }}
void exhaustive(const string &mode) {
    for (auto [sigma, pm, tm] : vector<array<int, 3>>{{2, mode == "quick" ? 4 : mode == "full" ? 5 : 6, mode == "quick" ? 7 : mode == "full" ? 9 : 10},
                                                      {3, mode == "quick" ? 3 : mode == "full" ? 3 : 4, mode == "quick" ? 5 : mode == "full" ? 7 : 6}}) {
        vector<string> pats, texts;
        auto all = [&](int len, vector<string> &out) {
            for (int n = 0, count = 1; n <= len; ++n, count *= sigma) {
                for (int code = 0; code < count; ++code) {
                    string s(n, 'a');
                    for (int x = code, i = 0; i < n; ++i, x /= sigma) { s[i] = char('a' + x % sigma); }
                    out.push_back(s);}}};
        all(pm, pats);
        all(tm, texts);
        for (const auto &t : texts) {
            for (const auto &p : pats) {
                context = "pattern=" + p + " text=" + t;
                verify(p, t, 100);}}
        cout << "PASS exhaustive alphabet=" << sigma << " patterns<=" << pm << " texts<=" << tm << '\n';}}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 2000 : mode == "full" ? 8000 : 20000;
    for (int rep = 0; rep < count; ++rep) {
        int sigma = rep % 5 == 0 ? 256 : int(rng() % 4) + 1, n = int(rng() % 300), m = int(rng() % 70), period = int(rng() % 6) + 1;
        string t(n, '\0'), p;
        for (int i = 0; i < n; ++i) { t[i] = i >= period && rep % 3 == 1 ? t[i - period] : char(rng() % sigma); }
        if (n && rng() % 2) {
            int at = int(rng() % n);
            p = t.substr(at, m);}
        else {
            p.resize(m);
            for (char &c : p) { c = char(rng() % sigma); }}
        if (!p.empty() && rng() % 4 == 0) { p.back() = char(rng() % 256); }
        context = "random bytes case=" + std::to_string(rep) + " n=" + std::to_string(n) + " m=" + std::to_string(p.size());
        verify(p, t, 256);
        vector<int> ti(t.begin(), t.end()), pi(p.begin(), p.end());
        for (int &c : ti) { c = uint8_t(c); }
        for (int &c : pi) { c = uint8_t(c); }
        verify(pi, ti, 256);
        vector<lng> tl(n), pl(p.size());
        for (int i = 0; i < n; ++i) { tl[i] = ti[i] % 2 ? std::numeric_limits<lng>::min() + ti[i] : std::numeric_limits<lng>::max() - ti[i]; }
        for (int i = 0; i < int(p.size()); ++i) { pl[i] = pi[i] % 2 ? std::numeric_limits<lng>::min() + pi[i] : std::numeric_limits<lng>::max() - pi[i]; }
        verify(pl, tl, 0, false);
        vector<int> tc(ti), pc(pi);
        int small = 5;
        for (int &c : tc) { c %= small; }
        for (int &c : pc) { c %= small; }
        verify(pc, tc, small);}
    // Symbols equal modulo 2^61 - 1 collide in every Rabin-Karp window, so only verification separates them.
    constexpr lng P61 = (1LL << 61) - 1;
    for (int rep = 0; rep < count / 10; ++rep) {
        int n = int(rng() % 60), m = int(rng() % 6);
        vector<lng> t(n), p(m);
        for (lng &x : t) { x = 5 + P61 * lng(rng() % 2); }
        for (lng &x : p) { x = 5 + P61 * lng(rng() % 2); }
        context = "rabinKarp collisions case=" + std::to_string(rep);
        verify(p, t, 0, false);}
    cout << "PASS random bytes/periodic/ints/extreme lng/small alphabet/hash collisions cases=" << count << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 100000 : 1000000, m = mode == "quick" ? 2000 : 20000;
    std::mt19937_64 rng(test_seed + 7);
    auto run = [&](const string &p, const string &t, lng expected, const string &name) {
        context = "large " + name + " n=" + std::to_string(t.size()) + " m=" + std::to_string(p.size());
        auto kmp = kmpOccurrences(p, t);
        expect(lng(kmp.size()), expected, "kmp count");
        expect(boyerMoore(p, t), kmp, "boyerMoore");
        expect(twoWay(p, t), kmp, "twoWay");
        expect(allOccurrences(p, t), kmp, "allOccurrences");
        expect(dfaSearch(p, t), kmp, "dfaSearch");
        if (lng(kmp.size()) * lng(p.size()) <= 100000000) { expect(rabinKarp(p, t), kmp, "rabinKarp"); }
        expect(countOccurrences(p, t, false), expected ? lng(t.size()) / lng(max<size_t>(p.size(), 1)) : 0, "disjoint");
        vector<int> codes(t.size());
        for (int i = 0; i < n; ++i) { codes[i] = uint8_t(t[i]); }
        vector<int> pc(p.begin(), p.end());
        expect(twoWay(pc, codes), kmp, "twoWay ints");};
    // Unary text with a unary pattern: quadratic Boyer-Moore without Galil would need n * m comparisons.
    run(string(m, 'a'), string(n, 'a'), n - m + 1, "unary hit");
    run(string(m - 1, 'a') + 'b', string(n, 'a'), 0, "unary miss");
    string per(n, 'a');
    for (int i = 0; i < n; ++i) { per[i] = "aab"[i % 3]; }
    run(per.substr(0, 3 * (m / 3)), per, (n - 3 * (m / 3)) / 3 + 1, "period 3");
    string rnd(n, 'a');
    for (char &c : rnd) { c = char('a' + rng() % 2); }
    context = "large random binary";
    auto expected = kmpOccurrences(rnd.substr(500, 20), rnd);
    expect(horspool(rnd.substr(500, 20), rnd), expected, "horspool");
    expect(sunday(rnd.substr(500, 20), rnd), expected, "sunday");
    expect(naiveSearch(rnd.substr(500, 20), rnd), expected, "naiveSearch");
    cout << "PASS large unary/periodic/random against kmpOccurrences n=" << n << " m=" << m << '\n';}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; }}
#ifdef _GLIBCXX_DEBUG
    if (mode == "stress") { mode = "full"; }
#endif
    if (!invalid.empty()) {
        if (invalid == "boyer-moore-symbol") { boyerMoore(vector<int>{1, 5}, vector<int>{1, 2}, 3); }
        if (invalid == "horspool-text-symbol") { horspool(vector<int>{1}, vector<int>{1, 7}, 3); }
        if (invalid == "bad-character-length") { badCharacterTable(string("ab"), 256, 3); }
        if (invalid == "dfa-text-symbol") { dfaSearch(prefixAutomaton({0, 1}, 2), vector<int>{0, 2}); }
        if (invalid == "dfa-pattern-symbol") { dfaSearch(vector<int>{0, 3}, vector<int>{0, 1}, 2); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS string_matching seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
