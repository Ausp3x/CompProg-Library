#include "../../07-Strings/08-palindrome_queries.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class T> void expectEqual(const T &actual, const T &expected, const string &op) {
    ++checks; if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual << '\n'; std::exit(1);
}
template<class S> bool direct(const S &s, int l, int r) {
    while (l < r) { if (s[l++] != s[--r]) { return false; } }
    return true;
}
template<class S> void verifyExact(const S &s) {
    ++cases; int n = int(s.size()); Manacher m(s); pair<int, int> best{0, 0};
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        bool expected = direct(s, l, r);
        expectEqual(isPalindrome(m, l, r), expected, "exact [" + std::to_string(l) + ',' + std::to_string(r) + ')');
        if (expected && r - l > best.second - best.first) { best = {l, r}; }
    } }
    auto got = longestPalindrome(m);
    expectEqual(got.first, best.first, "leftmost longest start"); expectEqual(got.second, best.second, "leftmost longest end");
    expectEqual(direct(s, got.first, got.second), true, "longest witness");
}
template<bool WRAP64, class S> void verifyHash(const S &s, ulng seed) {
    StringHash<WRAP64> h(s, StringHash<WRAP64>::randomBase(seed)); int n = int(s.size());
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
        expectEqual(maybePalindrome(h, l, r), direct(s, l, r), "hash sampled oracle [" + std::to_string(l) + ',' + std::to_string(r) + ')');
    } }
}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 5 : mode == "full" ? 7 : 9, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) { for (int code = 0; code < count; ++code) {
        string s(n, '\0'); int x = code;
        for (char &c : s) { c = char(x % 3); x /= 3; }
        context = "ternary n=" + std::to_string(n) + " code=" + std::to_string(code);
        verifyExact(s); verifyHash<false>(string_view(s), test_seed); verifyHash<true>(string_view(s), test_seed);
    } }
    cout << "PASS exhaustive direct interval/longest oracle ternary lengths=0.." << bound << '\n';
}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed); int count = mode == "quick" ? 80 : mode == "full" ? 500 : 3000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 65); string s(n, '\0');
        for (char &c : s) { c = char(rng() % (rep % 2 ? 256 : 4)); }
        if (rep % 5 == 0) { for (int i = 0; i < n / 2; ++i) { s[n - 1 - i] = s[i]; } }
        context = "random=" + std::to_string(rep) + " bytes=";
        for (unsigned char c : s) { context += std::to_string(c) + ','; }
        verifyExact(string_view(s)); verifyHash<false>(string_view(s), rng()); verifyHash<true>(string_view(s), rng());
        vector<lng> wide; vector<uint> primes, words;
        for (unsigned char c : s) {
            wide.push_back(c % 2 ? std::numeric_limits<lng>::min() + c : std::numeric_limits<lng>::max() - c);
            primes.push_back(1000000005 - c); words.push_back(UINT32_MAX - c); }
        verifyExact(wide); verifyHash<false>(primes, rng()); verifyHash<true>(words, rng()); }
    string bytes; for (int c = 0; c < 256; ++c) { bytes += char(c); }
    context = "all bytes"; verifyExact(bytes); verifyHash<false>(string_view(bytes), test_seed); verifyHash<true>(string_view(bytes), test_seed);
    cout << "PASS seeded byte/integer/full-width alphabet cases=" << count << '\n';
}
void lifecycleAndCollisions() {
    context = "empty and value semantics"; Manacher empty;
    expectEqual(isPalindrome(empty, 0, 0), true, "default empty"); expectEqual(longestPalindrome(empty).second, 0, "empty longest");
    expectEqual(maybePalindrome(StringHash<>(), 0, 0), true, "default empty hash");
    expectEqual(maybePalindrome(StringHash64(), 0, 0), true, "default empty word hash");
    string s = "babad"; Manacher m(s), copy = m; Manacher moved = std::move(copy);
    expectEqual(longestPalindrome(moved).first, 0, "odd tie chooses bab"); expectEqual(longestPalindrome(moved).second, 3, "odd tie length");
    copy = Manacher(string_view("abbacddc"));
    expectEqual(longestPalindrome(copy).first, 0, "even tie chooses abba"); expectEqual(longestPalindrome(copy).second, 4, "even tie length");
    s.assign(5, 'x'); expectEqual(isPalindrome(m, 0, 5), false, "no source borrowing");
    auto hash = StringHash<>(string_view("abbacddc")); auto hcopy = hash; auto hmoved = std::move(hcopy);
    expectEqual(maybePalindrome(hmoved, 0, 4), true, "hash copy/move destination");
    hcopy = StringHash<>(string_view("xy")); expectEqual(maybePalindrome(hcopy, 0, 2), false, "hash moved-from assignment");
    // Odd-depth Thue-Morse reverses to its complement. Its difference polynomial
    // is product(1-b^(2^j),j=0..10), divisible by 2^64 for every odd b.
    string collision(2048, '\0');
    for (int i = 0; i < 2048; ++i) { collision[i] = char(std::popcount(uint(i)) % 2); }
    context = "nonpalindromic length-2048 Thue-Morse"; Manacher exact(collision);
    expectEqual(direct(collision, 0, 2048), false, "independent nonpalindrome");
    expectEqual(isPalindrome(exact, 0, 2048), false, "exact rejects hash collision");
    for (int i = 0; i < 10; ++i) {
        StringHash64 h(collision, StringHash64::randomBase(test_seed + i));
        expectEqual(maybePalindrome(h, 0, 2048), true, "expected word hash false positive"); }
    cout << "PASS lifecycle, ties and explicit probabilistic false positive\n";
}
void large(const string &mode) {
    int n = mode == "quick" ? 5000 : mode == "full" ? 200000 : 1000000;
    string s(n, char(255)); context = "large unary n=" + std::to_string(n); Manacher unary(s);
    auto p = longestPalindrome(unary); expectEqual(p.first, 0, "unary longest start"); expectEqual(p.second, n, "unary longest end");
    for (int i = 0; i <= n; ++i) { expectEqual(isPalindrome(unary, i, n), true, "unary suffix"); }
    for (int i = 0; i < n; ++i) { s[i] = char(i % 2); }
    Manacher alternating(s); StringHash<> h(s); StringHash64 word(s); context = "large alternating n=" + std::to_string(n);
    p = longestPalindrome(alternating); expectEqual(p.first, 0, "alternating longest start"); expectEqual(p.second, n - 1, "alternating longest end");
    for (int i = 0; i <= n; ++i) {
        bool expected = i == n || (n - i) % 2;
        expectEqual(isPalindrome(alternating, i, n), expected, "alternating suffix exact");
        expectEqual(maybePalindrome(h, i, n), expected, "alternating suffix hash");
        expectEqual(maybePalindrome(word, i, n), expected, "alternating suffix word hash"); }
    cout << "PASS large unary/alternating n=" << n << '\n';
}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; } }
    if (!invalid.empty()) {
        Manacher m(string_view("aba")); StringHash<> h(string_view("aba"));
        if (invalid == "exact-negative") { isPalindrome(m, -1, 1); }
        if (invalid == "exact-reversed") { isPalindrome(m, 2, 1); }
        if (invalid == "exact-past-end") { isPalindrome(m, 0, 4); }
        if (invalid == "hash-negative") { maybePalindrome(h, -1, 1); }
        if (invalid == "hash-reversed") { maybePalindrome(h, 2, 1); }
        if (invalid == "hash-past-end") { maybePalindrome(h, 0, 4); }
        return 0;
    }
    exhaustive(mode); randomCases(mode); lifecycleAndCollisions(); large(mode);
    cout << "PASS palindrome queries seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';
}
