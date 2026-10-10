#include "../../07-Strings/19-lcs.hpp"

#ifdef _GLIBCXX_DEBUG
const int SCALE = 4;
#else
const int SCALE = 1;
#endif
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
struct Mod {
    static constexpr lng P = 998244353;
    lng v = 0;
    Mod(lng x = 0) : v((x % P + P) % P) {}
    friend Mod operator+(Mod a, Mod b) { return Mod(a.v + b.v); }
    friend Mod operator-(Mod a, Mod b) { return Mod(a.v - b.v); }
    Mod &operator+=(Mod b) { return *this = *this + b; }
};
struct EqOnly {
    int v;
    bool operator==(const EqOnly &) const = default;
};
// Independent full-table DP.
template<class A, class B> int dpLcs(const A &a, const B &b) {
    int n = int(a.size()), m = int(b.size());
    vector<vector<int>> f(n + 1, vector<int>(m + 1));
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) { f[i][j] = a[i - 1] == b[j - 1] ? f[i - 1][j - 1] + 1 : max(f[i - 1][j], f[i][j - 1]); }}
    return f[n][m];}
template<class A, class B> void expectWitness(const A &a, const B &b, const vector<pair<int, int>> &w, int expected, const string &op) {
    expectEqual(int(w.size()), expected, op + " size");
    for (int t = 0; t < int(w.size()); ++t) {
        auto [i, j] = w[t];
        check(0 <= i && i < int(a.size()) && 0 <= j && j < int(b.size()) && a[i] == b[j], op + " matched symbols");
        check(!t || (w[t - 1].first < i && w[t - 1].second < j), op + " strictly increasing");}}
template<class A, class B> void expectDiff(const A &a, const B &b, const vector<DiffOp> &ops, int common) {
    int i = 0, j = 0, edits = 0;
    for (auto [op, x, y] : ops) {
        check(x == i && y == j, "myersDiff cursors");
        if (op == '=') { check(i < int(a.size()) && j < int(b.size()) && a[i] == b[j], "myersDiff keep"); ++i; ++j; }
        else if (op == '-') { check(i < int(a.size()), "myersDiff delete"); ++i; ++edits; }
        else { check(op == '+' && j < int(b.size()), "myersDiff insert"); ++j; ++edits; }}
    check(i == int(a.size()) && j == int(b.size()), "myersDiff consumes both");
    expectEqual(edits, int(a.size()) + int(b.size()) - 2 * common, "myersDiff shortest edit script");}
template<class S, class Q> bool subsequence(const S &sub, const Q &s) {
    int j = 0;
    for (int i = 0; i < int(s.size()) && j < int(sub.size()); ++i) { j += s[i] == sub[j]; }
    return j == int(sub.size());}
template<class A, class B> void verifyPair(const A &a, const B &b) {
    ++cases;
    int expected = dpLcs(a, b);
    expectEqual(lcs(a, b), expected, "lcs");
    expectWitness(a, b, hirschbergLcs(a, b), expected, "hirschbergLcs");
    expectWitness(a, b, lcsWitness(a, b), expected, "lcsWitness");
    expectDiff(a, b, myersDiff(a, b), expected);
    auto scs = shortestCommonSupersequence(a, b);
    expectEqual(int(scs.size()), int(a.size()) + int(b.size()) - expected, "shortestCommonSupersequence length");
    check(subsequence(a, scs) && subsequence(b, scs), "shortestCommonSupersequence contains both");
    if constexpr (std::totally_ordered<std::decay_t<decltype(a[0])>>) {
        expectEqual(bitsetLcs(a, b), expected, "bitsetLcs");
        expectEqual(huntSzymanski(a, b), expected, "huntSzymanski");}}
// Brute enumeration of subsequences (n <= 14): each one is encoded as a base-(n + 1) number over symbol ranks + 1.
template<class S> void verifySingle(const S &s) {
    int n = int(s.size()), longest = 0;
    vector<int> rank(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) { rank[i] += s[j] < s[i]; }}
    vector<lng> all, pals;
    lng index_pals = 0;
    for (int mask = 0; mask < 1 << n; ++mask) {
        int sub[14], k = 0;
        lng code = 0;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) { sub[k++] = rank[i]; code = code * (n + 1) + rank[i] + 1; }}
        all.push_back(code);
        bool palindrome = k > 0;
        for (int i = 0; i < k / 2; ++i) { palindrome &= sub[i] == sub[k - 1 - i]; }
        if (palindrome) { pals.push_back(code); ++index_pals; longest = max(longest, k); }}
    for (auto *v : {&all, &pals}) {
        sort(v->begin(), v->end());
        v->erase(unique(v->begin(), v->end()), v->end());}
    expectEqual(countDistinctSubsequences<lng>(s), lng(all.size()), "countDistinctSubsequences");
    expectEqual(countPalindromicSubsequences<lng>(s, true), lng(pals.size()), "countPalindromicSubsequences distinct");
    expectEqual(countPalindromicSubsequences<lng>(s, false), index_pals, "countPalindromicSubsequences multiset");
    expectEqual(countDistinctSubsequences<Mod>(s).v, lng(all.size()) % Mod::P, "countDistinctSubsequences mod");
    expectEqual(countPalindromicSubsequences<Mod>(s, true).v, lng(pals.size()) % Mod::P, "distinct palindromic mod");
    expectEqual(longestPalindromicSubsequence(s), longest, "longestPalindromicSubsequence");}
template<class S> void verifyTriple(const S &a, const S &b, const S &c) {
    int n = int(a.size()), best = 0;
    for (int mask = 0; mask < 1 << n; ++mask) {
        S sub;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) { sub.push_back(a[i]); }}
        if (int(sub.size()) > best && subsequence(sub, b) && subsequence(sub, c)) { best = int(sub.size()); }}
    expectEqual(lcs3(a, b, c), best, "lcs3");}
string word(std::mt19937_64 &rng, int n, int sigma) {
    string s(n, '\0');
    for (char &c : s) { c = char(rng() % sigma); }
    return s;}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 5 : mode == "full" ? 6 : 7;
    vector<string> words{""};
    for (int len = 1; len <= bound; ++len) {
        for (int code = 0; code < 1 << len; ++code) {
            string s(len, 'a');
            for (int i = 0; i < len; ++i) { s[i] = char('a' + (code >> i & 1)); }
            words.push_back(s);}}
    for (const string &a : words) {
        context = "binary single " + a;
        verifySingle(a);
        for (const string &b : words) {
            context = "binary pair " + a + " / " + b;
            verifyPair(a, b);}}
    cout << "PASS exhaustive binary pairs and subsequence counts lengths<=" << bound << '\n';}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = (mode == "quick" ? 300 : mode == "full" ? 2000 : 6000) / SCALE;
    for (int rep = 0; rep < count; ++rep) {
        int sigma = rep % 5 == 0 ? 256 : int(rng() % 4) + 1;
        string a = word(rng, int(rng() % 140), sigma), b = word(rng, rep % 3 ? int(rng() % 140) : int(rng() % 3) * 64 + int(rng() % 3) - 1 + 64, sigma);
        context = "random bytes case=" + std::to_string(rep) + " n=" + std::to_string(a.size()) + " m=" + std::to_string(b.size());
        verifyPair(a, b);
        vector<lng> x(a.size()), y(b.size());
        for (int i = 0; i < int(a.size()); ++i) { x[i] = uint8_t(a[i]) % 2 ? std::numeric_limits<lng>::min() + uint8_t(a[i]) : lng(rng() % 1000000007); }
        for (int j = 0; j < int(b.size()); ++j) { y[j] = rng() % 3 && !x.empty() ? x[rng() % x.size()] : lng(rng()); }
        verifyPair(x, y);
        vector<EqOnly> ea, eb;
        for (char c : a) { ea.push_back({c % 3}); }
        for (char c : b) { eb.push_back({c % 3}); }
        verifyPair(ea, eb);
        vector<string> la, lb;
        for (char c : a.substr(0, 40)) { la.push_back(string(1 + uint8_t(c) % 2, char('x' + uint8_t(c) % 3))); }
        for (char c : b.substr(0, 40)) { lb.push_back(string(1 + uint8_t(c) % 2, char('x' + uint8_t(c) % 3))); }
        verifyPair(la, lb);
        string s = word(rng, int(rng() % 13), int(rng() % 4) + 1);
        context = "random single " + std::to_string(rep);
        verifySingle(s);
        vector<int> ints(s.begin(), s.end());
        for (int &v : ints) { v = v ? INT_MAX - v : INT_MIN; }
        verifySingle(ints);
        string ta = word(rng, int(rng() % 11), 3), tb = word(rng, int(rng() % 12), 3), tc = word(rng, int(rng() % 12), 3);
        context = "random triple " + std::to_string(rep);
        verifyTriple(ta, tb, tc);}
    cout << "PASS random bytes/sparse integers/equality-only/strings cases=" << count << '\n';}
void medium(const string &mode) {
    std::mt19937_64 rng(test_seed + 5);
    int n = (mode == "quick" ? 300 : mode == "full" ? 1200 : 2500) / SCALE;
    for (int sigma : {2, 4, 26, 256}) {
        string a = word(rng, n, sigma), b = word(rng, n + 37, sigma);
        context = "medium sigma=" + std::to_string(sigma) + " n=" + std::to_string(n);
        verifyPair(a, b);
        string s = word(rng, n / 3, min(sigma, 4));
        vector<vector<lng>> lps(s.size() + 1, vector<lng>(s.size() + 1));
        for (int len = 1; len <= int(s.size()); ++len) {
            for (int l = 0; l + len <= int(s.size()); ++l) {
                int r = l + len - 1;
                lps[l][r] = len == 1 ? 1 : s[l] == s[r] ? lps[l + 1][r - 1] + 2 : max(lps[l + 1][r], lps[l][r - 1]);}}
        expectEqual(longestPalindromicSubsequence(s), s.empty() ? 0 : lps[0][s.size() - 1], "longestPalindromicSubsequence interval DP");
        int k = int(s.size()), letters = min(sigma, 4);
        vector<vector<int>> first(k + 1, vector<int>(letters, k)), last(k + 1, vector<int>(letters, -1));
        for (int i = k - 1; i >= 0; --i) { first[i] = first[i + 1]; first[i][int(s[i])] = i; }
        for (int j = 1; j <= k; ++j) { last[j] = last[j - 1]; last[j][int(s[j - 1])] = j - 1; }
        vector<vector<Mod>> f(k + 1, vector<Mod>(k + 1));
        for (int len = 1; len <= k; ++len) {
            for (int i = 0; i + len <= k; ++i) {
                int j = i + len;
                for (int c = 0; c < letters; ++c) {
                    int p = first[i][c], q = last[j][c];
                    if (p < j) { f[i][j] += Mod(1) + (p < q ? Mod(1) + f[p + 1][q] : Mod(0)); }}}}
        expectEqual(countPalindromicSubsequences<Mod>(s, true).v, f[0][k].v, "distinct palindromic per-letter recurrence");}
    cout << "PASS medium DP and per-letter recurrence oracles n=" << n << '\n';}
void large(const string &mode) {
    std::mt19937_64 rng(test_seed + 9);
    int n = (mode == "quick" ? 5000 : mode == "full" ? 40000 : 60000) / SCALE;
    vector<int> a(n), b(n + 11);
    for (int &v : a) { v = int(rng() % 500); }
    for (int &v : b) { v = int(rng() % 500); }
    context = "large sparse alphabet n=" + std::to_string(n);
    int length = bitsetLcs(a, b);
    expectEqual(huntSzymanski(a, b), length, "bitsetLcs versus huntSzymanski");
    expectWitness(a, b, lcsWitness(a, b), length, "large lcsWitness");
    string s = word(rng, 2 * n, 2), t = s;
    for (int e = 0; e < 40; ++e) {
        int p = int(rng() % t.size());
        if (e % 2) { t.erase(t.begin() + p); }
        else { t.insert(t.begin() + p, char(rng() % 2)); }}
    context = "large similar binary n=" + std::to_string(s.size());
    int common = lcs(s, t);
#ifdef _GLIBCXX_DEBUG
    int prefix = 400;
#else
    int prefix = 3000;
#endif
    expectEqual(huntSzymanski(s.substr(0, prefix), t.substr(0, prefix)), dpLcs(s.substr(0, prefix), t.substr(0, prefix)), "huntSzymanski prefix DP");
    expectDiff(s, t, myersDiff(s, t), common);
    check(int(s.size()) + int(t.size()) - 2 * common <= 40, "few edits");
    string u(n, 'a');
    context = "large unary";
    expectEqual(longestPalindromicSubsequence(u), n, "unary palindromic subsequence");
    expectEqual(countPalindromicSubsequences<Mod>(u.substr(0, 3000), true).v, min(3000, n), "unary distinct palindromes");
    cout << "PASS large sparse/similar/unary n=" << n << '\n';}
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
    if (invalid == "lcs3-table-overflow") {
        string big(50000, 'a');
        lcs3(string("a"), big, big);}
    if (!invalid.empty()) { return 0; }
    exhaustive(mode);
    randomCases(mode);
    medium(mode);
    large(mode);
    cout << "PASS lcs seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
