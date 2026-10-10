#include "../../07-Strings/14-lyndon.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &op) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=true actual=false\n";
    std::exit(1);}
void expectEqual(lng actual, lng expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=" << expected << " actual=" << actual << '\n';
    std::exit(1);}
// Brute force on lng keys (bytes as unsigned): Lyndon means nonempty and strictly smaller than every proper suffix.
using Key = vector<lng>;
template<class S> Key keys(const S &s) {
    Key res;
    for (auto c : s) {
        if constexpr (std::is_same_v<decltype(c), char>) { res.push_back(uint8_t(c)); }
        else { res.push_back(lng(c)); }}
    return res;}
bool bruteLyndon(const Key &s, int l, int r) {
    if (l >= r) { return false; }
    for (int i = l + 1; i < r; ++i) {
        if (!std::lexicographical_compare(s.begin() + l, s.begin() + r, s.begin() + i, s.begin() + r)) { return false; }}
    return true;}
int bruteLongest(const Key &s, int l, int r) {
    for (int e = r; e > l; --e) {
        if (bruteLyndon(s, l, e)) { return e - l; }}
    return 0;}
vector<int> bruteCuts(const Key &s, int l, int r) {
    vector<int> res{l};
    while (l < r) { res.push_back(l += bruteLongest(s, l, r)); }
    return res;}
template<class S> void verify(const S &s) {
    ++cases;
    Key key = keys(s);
    int n = int(key.size());
    vector<int> cuts = bruteCuts(key, 0, n);
    check(duval(s) == cuts, "duval");
    for (int t = 1; t + 1 < int(cuts.size()); ++t) {
        check(!std::lexicographical_compare(key.begin() + cuts[t - 1], key.begin() + cuts[t], key.begin() + cuts[t], key.begin() + cuts[t + 1]), "brute factors non-increasing");}
    expectEqual(longestLyndonPrefix(s), bruteLongest(key, 0, n), "longestLyndonPrefix");
    check(isLyndon(s) == bruteLyndon(key, 0, n), "isLyndon");
    if (n >= 2 && bruteLyndon(key, 0, n)) {
        int split = 1;
        while (!bruteLyndon(key, split, n)) { ++split; }
        expectEqual(standardFactorization(s), split, "standardFactorization");
        check(bruteLyndon(key, 0, split), "standard left factor Lyndon");}
    vector<int> lambda = lyndonArray(s);
    expectEqual(int(lambda.size()), n, "lyndonArray size");
    for (int i = 0; i < n; ++i) { expectEqual(lambda[i], bruteLongest(key, i, n), "lyndonArray i=" + std::to_string(i)); }
    for (int i = 0; i <= n; ++i) { check(suffixFactorization(lambda, i) == bruteCuts(key, i, n), "suffixFactorization i=" + std::to_string(i)); }
    LyndonTree t = lyndonTree(lambda);
    int nodes = int(t.l.size());
    expectEqual(nodes, 2 * n - (int(cuts.size()) - 1), "lyndonTree node count");
    vector<int> roots;
    for (int v : t.roots) { roots.push_back(t.l[v]); }
    roots.push_back(n);
    check(n == 0 ? t.roots.empty() : roots == cuts, "lyndonTree roots are the factors");
    for (int v = 0; v < nodes; ++v) {
        check(bruteLyndon(key, t.l[v], t.r[v]), "lyndonTree node Lyndon");
        if (v < n) {
            check(t.l[v] == v && t.r[v] == v + 1 && t.left[v] < 0 && t.right[v] < 0, "lyndonTree leaf");
            continue;}
        int a = t.left[v], b = t.right[v], split = t.l[v] + 1;
        while (!bruteLyndon(key, split, t.r[v])) { ++split; }
        check(a >= 0 && b >= 0 && t.l[a] == t.l[v] && t.r[a] == t.l[b] && t.r[b] == t.r[v], "lyndonTree children partition");
        expectEqual(t.l[b], split, "lyndonTree right child is the longest proper Lyndon suffix");}
    vector<vector<int>> prefix(n + 1);
    vector<int> shortest(n + 1);
    for (int p = 0; p <= n; ++p) {
        prefix[p] = bruteCuts(key, 0, p);
        for (int i = p - 1; i >= 0; --i) {
            if (!shortest[p] || std::lexicographical_compare(key.begin() + i, key.begin() + p, key.begin() + (p - shortest[p]), key.begin() + p)) { shortest[p] = p - i; }}}
    IncrementalLyndon<lng> inc;
    for (int m = 0; m <= n; ++m) {
        if (m) { inc.add(key[m - 1]); }
        expectEqual(inc.size(), m, "incremental size");
        for (int p = 0; p <= m; ++p) {
            check(inc.factorize(p) == prefix[p], "incremental factorize p=" + std::to_string(p) + " after m=" + std::to_string(m));
            expectEqual(inc.minSuffix(p), shortest[p], "incremental minSuffix p=" + std::to_string(p));}}}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 8 : 9, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            string s(n, 'a');
            for (int x = code, i = 0; i < n; ++i, x /= 3) { s[i] = char('a' + x % 3); }
            context = "ternary " + s;
            verify(s);}}
    cout << "PASS exhaustive ternary brute Lyndon lengths=0.." << bound << '\n';}
void words(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 9 : 11;
    for (int k = 0; k <= 4; ++k) {
        for (int n = 0; n <= bound && lng(n) * k <= lng(4) * bound; ++n) {
            context = "lyndonWords n=" + std::to_string(n) + " k=" + std::to_string(k);
            vector<vector<int>> expected, exact, got, got_exact;
            vector<int> w;
            auto rec = [&](auto &&self) -> void {
                if (!w.empty()) {
                    Key kw(w.begin(), w.end());
                    if (bruteLyndon(kw, 0, int(kw.size()))) { expected.push_back(w); }}
                if (int(w.size()) == n) { return; }
                for (int c = 0; c < k; ++c) { w.push_back(c); self(self); w.pop_back(); }};
            rec(rec);
            sort(expected.begin(), expected.end());
            for (auto &x : expected) {
                if (int(x.size()) == n) { exact.push_back(x); }}
            lyndonWords(n, k, [&](const vector<int> &x) { got.push_back(x); });
            lyndonWords(n, k, [&](const vector<int> &x) { got_exact.push_back(x); }, true);
            check(got == expected, "lyndonWords up to n");
            check(got_exact == exact, "lyndonWords exact n");
            for (int t = 0; t + 1 < int(expected.size()); ++t) {
                vector<int> x = expected[t];
                check(nextLyndonWord(x, n, k) && x == expected[t + 1], "nextLyndonWord successor");}
            if (!expected.empty()) {
                vector<int> x = expected.back();
                check(!nextLyndonWord(x, n, k) && x.empty(), "nextLyndonWord last clears");}}}
    cout << "PASS lyndonWords/nextLyndonWord versus brute enumeration n<=" << bound << " k<=4\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 200 : mode == "full" ? 1500 : 10000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 31), sigma = rep % 3 == 0 ? 256 : int(rng() % 3) + 1, period = int(rng() % 4) + 1;
        string s(n, '\0');
        for (int i = 0; i < n; ++i) { s[i] = i >= period && rep % 4 == 1 ? s[i - period] : char(rng() % sigma); }
        context = "random bytes case=" + std::to_string(rep) + " n=" + std::to_string(n);
        verify(s);
        vector<lng> ints(n);
        for (int i = 0; i < n; ++i) { ints[i] = s[i] % 2 ? std::numeric_limits<lng>::min() + uint8_t(s[i]) : std::numeric_limits<lng>::max() - uint8_t(s[i]); }
        verify(ints);}
    string bytes;
    for (int c = 0; c < 256; ++c) { bytes += char(c); }
    context = "all 256 bytes ascending";
    verify(bytes);
    reverse(bytes.begin(), bytes.end());
    context = "all 256 bytes descending";
    verify(bytes);
    cout << "PASS random bytes/periodic/extreme integers cases=" << count << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 20000 : mode == "full" ? 300000 : 1000000;
    std::mt19937_64 rng(test_seed + 11);
    string s(n, 'a');
    context = "large unary n=" + std::to_string(n);
    vector<int> ones(n + 1);
    iota(ones.begin(), ones.end(), 0);
    check(duval(s) == ones && lyndonArray(s) == vector<int>(n, 1), "unary factors");
    for (int pass = 0; pass < 3; ++pass) {
        for (int i = 0; i < n; ++i) { s[i] = pass == 0 ? char('a' + rng() % 2) : pass == 1 ? "ab"[i % 7 == 6] : char('a' + __builtin_popcount(uint(i)) % 2); }
        context = string("large ") + (pass == 0 ? "random binary" : pass == 1 ? "a^6 b periodic" : "Thue-Morse");
        vector<int> cuts = duval(s), lambda = lyndonArray(s);
        IncrementalLyndon<char> inc;
        for (char c : s) { inc.add(c); }
        check(inc.factorize(n) == cuts, "duval versus incremental");
        check(suffixFactorization(lambda, 0) == cuts, "duval versus suffix-array Lyndon array");
        LyndonTree t = lyndonTree(lambda);
        expectEqual(int(t.l.size()), 2 * n - (int(cuts.size()) - 1), "tree size");
        for (int i = 0; i + 1 < int(t.roots.size()); ++i) { expectEqual(t.r[t.roots[i]], t.l[t.roots[i + 1]], "roots contiguous"); }}
    cout << "PASS large unary/random/periodic/Thue-Morse cross-checks n=" << n << '\n';}
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
        IncrementalLyndon<int> inc;
        inc.add(1);
        vector<int> empty, w{1};
        if (invalid == "standard-single") { standardFactorization(string("a")); }
        if (invalid == "standard-not-lyndon") { standardFactorization(string("ba")); }
        if (invalid == "tree-zero-length") { lyndonTree(vector<int>{0}); }
        if (invalid == "tree-overflow") { lyndonTree(vector<int>{1, 2}); }
        if (invalid == "suffix-negative") { suffixFactorization(vector<int>{1}, -1); }
        if (invalid == "suffix-high") { suffixFactorization(vector<int>{1}, 2); }
        if (invalid == "suffix-zero-length") { suffixFactorization(vector<int>{1, 0, 1}, 0); }
        if (invalid == "min-suffix-high") { inc.minSuffix(2); }
        if (invalid == "factorize-negative") { inc.factorize(-1); }
        if (invalid == "next-empty") { nextLyndonWord(empty, 3, 2); }
        if (invalid == "next-too-long") { nextLyndonWord(w, 0, 2); }
        if (invalid == "words-negative") { lyndonWords(-1, 2, [](const vector<int> &) {}); }
        return 0;}
    exhaustive(mode);
    words(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS lyndon seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
