#include "../../07-Strings/13-palindromictree.hpp"

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
    static constexpr int P = 1000000007;
    int v = 0;
    Mod(lng x = 0) : v(int((x % P + P) % P)) {}
    friend Mod operator+(Mod a, Mod b) { return Mod(lng(a.v) + b.v); }
};
using Sym = vector<int>;
const lng BIG = std::numeric_limits<lng>::max();
bool pal(const Sym &s, int l, int r) {
    for (--r; l < r; ++l, --r) {
        if (s[l] != s[r]) { return false; }}
    return true;}
int longestPalSuffix(const Sym &s, int l, int r) {
    for (int b = l; b < r; ++b) {
        if (pal(s, b, r)) { return r - b; }}
    return 0;}
// Brute DP over a direct palindrome table: count (mod 2^64) or min (~0 = none) partitions, even pieces only when even.
vector<ulng> brutePartitions(const Sym &s, bool even, bool minimum) {
    int n = int(s.size());
    vector<ulng> dp(n + 1, minimum ? ~0ULL : 0);
    dp[0] = minimum ? 0 : 1;
    for (int i = 1; i <= n; ++i) {
        for (int j = 0; j < i; ++j) {
            if ((even && (i - j) % 2) || !pal(s, j, i) || (minimum && dp[j] == ~0ULL)) { continue; }
            dp[i] = minimum ? min(dp[i], dp[j] + 1) : dp[i] + dp[j];}}
    return dp;}
// Tree checks against brute force on the symbols of string `which` occupying [l, r) of the joint storage.
template<class Tree> void verifyTree(const Tree &t, const vector<Sym> &strings) {
    Sym all;
    for (const Sym &x : strings) { all.insert(all.end(), x.begin(), x.end()); }
    vector<int> begin;
    for (int at = 0; const Sym &x : strings) { begin.push_back(at); at += int(x.size()); }
    int total = int(all.size());
    vector<int> own(total);
    for (int k = 0, at = 0; k < int(strings.size()); ++k) {
        for (int i = 0; i < int(strings[k].size()); ++i) { own[at++] = begin[k]; }}
    map<Sym, pair<int, int>> first;
    for (int r = 1; r <= total; ++r) {
        for (int l = own[r - 1]; l < r; ++l) {
            if (pal(all, l, r)) { first.emplace(Sym(all.begin() + l, all.begin() + r), pair<int, int>{l, r}); }}}
    expectEqual(t.distinctPalindromes(), int(first.size()), "distinctPalindromes");
    expectEqual(t.size(), int(first.size()) + 2, "size");
    check(t.s == all && int(t.suffix.size()) == total, "stored symbols");
    map<Sym, int> id;
    auto strOf = [&](int v) { auto [l, r] = t.palindrome(v); return Sym(all.begin() + l, all.begin() + r); };
    for (int v = 2; v < t.size(); ++v) {
        Sym p = strOf(v);
        check(first.count(p) && first[p] == t.palindrome(v), "palindrome first occurrence v=" + std::to_string(v));
        check(!id.count(p), "distinct nodes");
        id[p] = v;
        expectEqual(t.nodes[v].len, int(p.size()), "len");}
    auto node = [&](const Sym &p) -> int { return p.empty() ? 1 : id.count(p) ? id[p] : -1; };
    expectEqual(t.nodes[0].len, -1, "root len -1");
    expectEqual(t.nodes[1].len, 0, "root len 0");
    for (int v = 2; v < t.size(); ++v) {
        Sym p = strOf(v);
        int len = int(p.size()), link = longestPalSuffix(p, 1, len);
        expectEqual(t.nodes[v].link, node(Sym(p.end() - link, p.end())), "link v=" + std::to_string(v));
        expectEqual(t.nodes[v].parent, len == 1 ? 0 : node(Sym(p.begin() + 1, p.end() - 1)), "parent");
        expectEqual(t.nodes[v].diff, len - link, "diff");
        vector<int> chain{len};
        while (chain.back() > 0) { chain.push_back(longestPalSuffix(p, len - chain.back() + 1, len)); }
        chain.push_back(-1);
        int k = 1;
        while (chain[k] > 0 && chain[k] - chain[k + 1] == len - link) { ++k; }
        expectEqual(t.nodes[t.nodes[v].series].len, chain[k], "series length");
        check(t.nodes[v].series == (chain[k] > 0 ? node(Sym(p.end() - chain[k], p.end())) : chain[k] == 0 ? 1 : 0), "series node");}
    set<int> alphabet(all.begin(), all.end());
    alphabet.insert(-5);
    for (int v = 0; v < t.size(); ++v) {
        for (int c : alphabet) {
            Sym p = v >= 2 ? strOf(v) : Sym();
            p.insert(p.begin(), c);
            if (v != 0) { p.push_back(c); }
            int expected = id.count(p) ? id[p] : -1;
            expectEqual(t.step(v, c), expected, "step v=" + std::to_string(v) + " c=" + std::to_string(c));}}
    for (int r = 0; r <= total; ++r) {
        int v = t.longestSuffixPalindrome(r);
        if (r == 0) { expectEqual(v, 1, "longestSuffixPalindrome empty"); continue; }
        int len = longestPalSuffix(all, own[r - 1], r);
        expectEqual(v, node(Sym(all.begin() + (r - len), all.begin() + r)), "longestSuffixPalindrome r=" + std::to_string(r));
        expectEqual(t.suffix[r - 1], v, "suffix field");
        for (int l = own[r - 1]; l <= r; ++l) { expectEqual(t.substringSuffixPalindrome(l, r), longestPalSuffix(all, l, r), "substringSuffixPalindrome l=" + std::to_string(l) + " r=" + std::to_string(r)); }}
    vector<int> at = t.occurrencesAt();
    for (int i = 0; i < total; ++i) {
        int count = 0;
        for (int l = own[i]; l <= i; ++l) { count += pal(all, l, i + 1); }
        expectEqual(at[i], count, "occurrencesAt i=" + std::to_string(i));}
    vector<pair<int, int>> ranges{{0, total}};
    for (int k = 0; k < int(strings.size()); ++k) { ranges.push_back({begin[k], begin[k] + int(strings[k].size())}); }
    ranges.push_back({total / 3, total - total / 4});
    for (auto [l, r] : ranges) {
        vector<int> occ = (l == 0 && r == total) ? t.occurrenceCounts() : t.occurrenceCounts(l, r);
        expectEqual(int(occ.size()), t.size(), "occurrenceCounts size");
        for (int v = 2; v < t.size(); ++v) {
            Sym p = strOf(v);
            int len = int(p.size()), count = 0;
            for (int e = max(l, len - 1); e < r; ++e) { count += e - len + 1 >= own[e] && Sym(all.begin() + (e - len + 1), all.begin() + (e + 1)) == p; }
            expectEqual(occ[v], count, "occurrenceCounts [" + std::to_string(l) + "," + std::to_string(r) + ") v=" + std::to_string(v));}}}
// Partition DPs on the current (last) string.
template<class Tree> void verifyPartitions(const Tree &t, const Sym &s) {
    int n = int(s.size());
    vector<ulng> mins = brutePartitions(s, false, true), counts = brutePartitions(s, false, false), evens = brutePartitions(s, true, false), even_mins = brutePartitions(s, true, true);
    vector<int> len = t.palindromicLength();
    vector<ulng> even = t.template evenPalindromePartition<ulng>();
    vector<ulng> all = t.palindromicFactorization(ulng(0), ulng(1), [](ulng a, ulng b) { return a + b; }, [](ulng a) { return a; });
    vector<lng> emin = t.palindromicFactorization(BIG, lng(0), [](lng a, lng b) { return min(a, b); }, [](lng a) { return a == BIG ? a : a + 1; }, true);
    expectEqual(int(len.size()), n + 1, "palindromicLength size");
    for (int i = 0; i <= n; ++i) {
        expectEqual(len[i], lng(mins[i]), "palindromicLength i=" + std::to_string(i));
        expectEqual(lng(even[i]), lng(evens[i]), "evenPalindromePartition mod 2^64 i=" + std::to_string(i));
        expectEqual(lng(all[i]), lng(counts[i]), "palindromicFactorization count mod 2^64 i=" + std::to_string(i));
        expectEqual(emin[i], even_mins[i] == ~0ULL ? BIG : lng(even_mins[i]), "palindromicFactorization even min i=" + std::to_string(i));}
    vector<int> best(n + 1, 0);
    for (int i = 1; i <= n; ++i) {
        for (int j = 0; j < i; ++j) {
            if (pal(s, j, i) && mins[j] + 1 == mins[i]) { best[i] = j; break; }}}
    vector<int> cuts{n};
    for (int x = n; x;) { cuts.push_back(x = best[x]); }
    reverse(cuts.begin(), cuts.end());
    check(t.minPalindromePartition() == cuts, "minPalindromePartition witness (longest last piece)");}
void verify(const Sym &s, bool dense, const Sym &second = {}) {
    ++cases;
    PalindromicTree t(s);
    verifyTree(t, {s});
    verifyPartitions(t, s);
    if (dense) {
        BasicPalindromicTree<3, 0> d(s);
        check(d.size() == t.size() && d.suffix == t.suffix, "dense suffix nodes");
        for (int v = 0; v < t.size(); ++v) {
            const auto &a = t.nodes[v];
            const auto &b = d.nodes[v];
            check(a.len == b.len && a.link == b.link && a.diff == b.diff && a.series == b.series && a.parent == b.parent && a.pos == b.pos, "dense node fields");}
        check(d.minPalindromePartition() == t.minPalindromePartition() && d.palindromicLength() == t.palindromicLength(), "dense partitions");}
    PalindromicTree joint;
    for (int c : s) { joint.append(c); }
    joint.newString();
    for (int c : second) { joint.append(c); }
    verifyTree(joint, {s, second});
    verifyPartitions(joint, second);
    PalindromicTree rebuilt(second);
    rebuilt.build(s);
    check(rebuilt.suffix == t.suffix && rebuilt.size() == t.size(), "build resets");}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 8 : 9, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            Sym s(n);
            for (int x = code, i = 0; i < n; ++i, x /= 3) { s[i] = x % 3; }
            Sym second(s.rbegin(), s.rend());
            second.resize(n / 2);
            context = "ternary n=" + std::to_string(n) + " code=" + std::to_string(code);
            verify(s, true, second);}}
    cout << "PASS exhaustive ternary brute palindromes, joint trees, partitions lengths=0.." << bound << '\n';}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 100 : mode == "full" ? 800 : 5000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 40), sigma = rep % 4 == 0 ? 3 : int(rng() % 3) + 1;
        Sym s(n), second(int(rng() % 20));
        for (int &c : s) { c = int(rng() % sigma); }
        for (int &c : second) { c = int(rng() % sigma); }
        if (rep % 5 == 0) {
            for (int i = 0; i < n / 2; ++i) { s[n - 1 - i] = s[i]; }}
        if (rep % 7 == 0) {
            for (int &c : s) { c = c ? INT_MAX - c : INT_MIN; }
            for (int &c : second) { c = c ? INT_MAX - c : INT_MIN; }}
        context = "random case=" + std::to_string(rep) + " n=" + std::to_string(n);
        verify(s, rep % 7 != 0, second);}
    PalindromicTree bytes(string_view("\xff\x80\x00\x80\xff\x01", 6));
    context = "byte string with NUL and high bytes";
    verifyTree(bytes, {Sym{255, 128, 0, 128, 255, 1}});
    PalindromicTreeDense<> letters("abacabadabacaba");
    check(letters.distinctPalindromes() == 15 && letters.minPalindromePartition() == vector<int>{0, 15}, "dense letters");
    cout << "PASS random/extreme-symbol/byte/dense trees cases=" << count << '\n';}
void medium(const string &mode) {
    int n = mode == "quick" ? 400 : mode == "full" ? 2000 : 4000;
    std::mt19937_64 rng(test_seed + 3);
    for (int pass = 0; pass < 2; ++pass) {
        Sym s(n);
        for (int i = 0; i < n; ++i) { s[i] = pass ? __builtin_popcount(uint(i)) % 2 : int(rng() % 2); }
        context = string("medium ") + (pass ? "Thue-Morse" : "random binary") + " n=" + std::to_string(n);
        PalindromicTree t(s);
        verifyPartitions(t, s);
        vector<int> at = t.occurrencesAt();
        lng total = 0, expected = 0;
        for (int x : at) { total += x; }
        for (int l = 0; l < n; ++l) {
            for (int r = l + 1; r <= n; ++r) { expected += pal(s, l, r); }}
        expectEqual(total, expected, "total palindromic substrings");}
    cout << "PASS medium O(n^2) partition and count oracles n=" << n << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 50000 : mode == "full" ? 500000 : 1000000;
    string s(n, 'a');
    context = "large unary n=" + std::to_string(n);
    PalindromicTreeDense<> unary(s);
    expectEqual(unary.distinctPalindromes(), n, "unary distinct");
    vector<int> at = unary.occurrencesAt(), occ = unary.occurrenceCounts();
    for (int i = 0; i < n; ++i) { expectEqual(at[i], i + 1, "unary occurrencesAt"); }
    for (int v = 2; v < unary.size(); ++v) { expectEqual(occ[v], n - unary.nodes[v].len + 1, "unary occurrences"); }
    check(unary.minPalindromePartition() == vector<int>{0, n}, "unary partition");
    vector<Mod> even = unary.evenPalindromePartition<Mod>();
    for (int i = 0, power = 1; i <= n; ++i) {
        expectEqual(even[i].v, i % 2 ? 0 : i ? power : 1, "unary even partitions 2^(i/2-1) mod p i=" + std::to_string(i));
        if (i % 2 == 0 && i) { power = int(lng(power) * 2 % Mod::P); }}
    string f = "a", g = "ab";
    while (int(g.size()) < n) { string h = g + f; f = g; g = h; }
    g.resize(n);
    context = "large Fibonacci word n=" + std::to_string(n);
    PalindromicTree fw(g);
    vector<int> len = fw.palindromicLength();
    check(len[0] == 0 && fw.minPalindromePartition().size() == size_t(len[n]) + 1, "fibonacci partition witness size");
    check(fw.distinctPalindromes() <= n, "distinct palindromes at most n");
    for (int v = 2; v < fw.size(); ++v) {
        const auto &u = fw.nodes[v];
        check(fw.nodes[u.parent].len == u.len - 2 && fw.step(u.parent, uint8_t(g[u.pos])) == v, "fibonacci parent edge");
        check(g[u.pos - u.len + 1] == g[u.pos] && fw.nodes[u.link].len < u.len && u.diff == u.len - fw.nodes[u.link].len, "fibonacci node invariants");}
    for (int i = 0; i < n; ++i) {
        const auto &u = fw.nodes[fw.suffix[i]];
        check(u.len <= i + 1 && g[i - u.len + 1] == g[i], "fibonacci suffix ends");}
    cout << "PASS large unary/Fibonacci n=" << n << '\n';}
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
        PalindromicTree t("aba");
        PalindromicTreeDense<> d;
        if (invalid == "palindrome-root") { t.palindrome(1); }
        if (invalid == "palindrome-high") { t.palindrome(t.size()); }
        if (invalid == "suffix-negative") { t.longestSuffixPalindrome(-1); }
        if (invalid == "suffix-high") { t.longestSuffixPalindrome(4); }
        if (invalid == "substring-reversed") { t.substringSuffixPalindrome(2, 1); }
        if (invalid == "substring-high") { t.substringSuffixPalindrome(0, 4); }
        if (invalid == "counts-high") { t.occurrenceCounts(0, 4); }
        if (invalid == "counts-reversed") { t.occurrenceCounts(2, 1); }
        if (invalid == "dense-below") { d.append('a' - 1); }
        if (invalid == "dense-above") { d.append('z' + 1); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    medium(mode);
    large(mode);
    cout << "PASS palindromictree seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
