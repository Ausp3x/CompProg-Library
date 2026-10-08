#include "../../07-Strings/08-palindrome_queries.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
ostream &operator<<(ostream &out, pair<int, int> p) { return out << '[' << p.first << ',' << p.second << ')'; }
template<class T> void expectEqual(const T &actual, const T &expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op
         << " expected=" << expected << " actual=" << actual << '\n';
    std::exit(1);}
template<class S> bool direct(const S &s, int l, int r) {
    while (l < r) {
        if (s[l++] != s[--r]) { return false; }}
    return true;}
template<class S> void verifyExact(const S &s) {
    ++cases;
    int n = int(s.size());
    Manacher m(s);
    pair<int, int> best{0, 0};
    lng count = 0;
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) {
            bool expected = direct(s, l, r);
            expectEqual(isPalindrome(m, l, r), expected, "exact [" + std::to_string(l) + ',' + std::to_string(r) + ')');
            count += expected && r > l;
            if (expected && r - l > best.second - best.first) { best = {l, r}; }}}
    auto got = longestPalindrome(m);
    expectEqual(got.first, best.first, "leftmost longest start");
    expectEqual(got.second, best.second, "leftmost longest end");
    expectEqual(direct(s, got.first, got.second), true, "longest witness");
    expectEqual(countPalindromes(m), count, "palindromic substring count");}
template<int KIND, class S> void verifyHash(const S &s, ulng seed) {
    StringHash<KIND> h(s, StringHash<KIND>::randomBase(seed));
    int n = int(s.size());
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) {
            expectEqual(maybePalindrome(h, l, r), direct(s, l, r), "hash KIND=" + std::to_string(KIND) + " sampled oracle [" + std::to_string(l) + ',' + std::to_string(r) + ')');}}}
template<class S> void verifyHashes(const S &s, ulng seed) {
    verifyHash<0>(s, seed);
    verifyHash<1>(s, seed + 1);
    verifyHash<2>(s, seed + 2);}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 5 : mode == "full" ? 7 : 9;
    for (int n = 0, count = 1; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            string s(n, '\0');
            for (int i = 0, x = code; i < n; ++i, x /= 3) { s[i] = char(x % 3); }
            context = "ternary n=" + std::to_string(n) + " code=" + std::to_string(code);
            verifyExact(s);
            verifyHashes(string_view(s), test_seed);}}
    cout << "PASS exhaustive direct interval/longest/count oracle ternary lengths=0.." << bound << '\n';}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 80 : mode == "full" ? 500 : 3000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 65);
        string s(n, '\0');
        for (char &c : s) { c = char(rng() % (rep % 2 ? 256 : 4)); }
        if (rep % 5 == 0) {
            for (int i = 0; i < n / 2; ++i) { s[n - 1 - i] = s[i]; }}
        context = "random=" + std::to_string(rep) + " bytes=";
        for (unsigned char c : s) { context += std::to_string(c) + ','; }
        verifyExact(string_view(s));
        verifyHashes(string_view(s), rng());
        vector<lng> wide;
        vector<uint> primes, words;
        for (unsigned char c : s) {
            wide.push_back(c % 2 ? std::numeric_limits<lng>::min() + c : std::numeric_limits<lng>::max() - c);
            primes.push_back(1000000005 - c);
            words.push_back(UINT32_MAX - c);}
        verifyExact(wide);
        verifyHash<0>(primes, rng());
        verifyHash<1>(words, rng());
        verifyHash<2>(words, rng());}
    string bytes;
    for (int c = 0; c < 256; ++c) { bytes += char(c); }
    context = "all bytes";
    verifyExact(bytes);
    verifyHashes(string_view(bytes), test_seed);
    cout << "PASS seeded byte/integer/full-width alphabet cases=" << count << '\n';}
void lifecycleAndCollisions() {
    context = "empty and value semantics";
    Manacher empty;
    expectEqual(isPalindrome(empty, 0, 0), true, "default empty");
    expectEqual(longestPalindrome(empty), pair<int, int>{0, 0}, "empty longest");
    expectEqual(countPalindromes(empty), lng(0), "empty count");
    expectEqual(maybePalindrome(StringHash<>(), 0, 0), true, "default empty hash");
    expectEqual(maybePalindrome(StringHash64(), 0, 0), true, "default empty word hash");
    expectEqual(maybePalindrome(StringHash61(), 0, 0), true, "default empty Mersenne hash");
    string s = "babad";
    Manacher m(s), copy = m;
    Manacher moved = std::move(copy);
    expectEqual(longestPalindrome(moved), pair<int, int>{0, 3}, "odd tie chooses bab");
    expectEqual(countPalindromes(moved), lng(7), "babad count");
    copy = Manacher(string_view("abbacddc"));
    expectEqual(longestPalindrome(copy), pair<int, int>{0, 4}, "even tie chooses abba");
    s.assign(5, 'x');
    expectEqual(isPalindrome(m, 0, 5), false, "no source borrowing");
    auto hash = StringHash<>(string_view("abbacddc"));
    auto hcopy = hash;
    auto hmoved = std::move(hcopy);
    expectEqual(maybePalindrome(hmoved, 0, 4), true, "hash copy/move destination");
    hcopy = StringHash<>(string_view("xy"));
    expectEqual(maybePalindrome(hcopy, 0, 2), false, "hash moved-from assignment");
    // Thue-Morse of length 2^11 reverses to its complement; the difference polynomial is divisible by 2^64 for odd bases.
    string collision(2048, '\0');
    for (int i = 0; i < 2048; ++i) { collision[i] = char(std::popcount(uint(i)) % 2); }
    context = "nonpalindromic length-2048 Thue-Morse";
    Manacher exact(collision);
    expectEqual(direct(collision, 0, 2048), false, "independent nonpalindrome");
    expectEqual(isPalindrome(exact, 0, 2048), false, "exact rejects hash collision");
    for (int i = 0; i < 10; ++i) {
        StringHash64 h(collision, StringHash64::randomBase(test_seed + i));
        expectEqual(maybePalindrome(h, 0, 2048), true, "expected word hash false positive");}
    cout << "PASS lifecycle, ties and explicit probabilistic false positive\n";}
void large(const string &mode) {
    int n = mode == "quick" ? 5000 : mode == "full" ? 200000 : 1000000;
    string s(n, char(255));
    context = "large unary n=" + std::to_string(n);
    Manacher unary(s);
    expectEqual(longestPalindrome(unary), pair<int, int>{0, n}, "unary longest");
    expectEqual(countPalindromes(unary), lng(n) * (n + 1) / 2, "unary count");
    for (int i = 0; i <= n; ++i) { expectEqual(isPalindrome(unary, i, n), true, "unary suffix"); }
    for (int i = 0; i < n; ++i) { s[i] = char(i % 2); }
    Manacher alternating(s);
    StringHash<> h(s);
    StringHash64 word(s);
    StringHash61 field(s);
    context = "large alternating n=" + std::to_string(n);
    expectEqual(longestPalindrome(alternating), pair<int, int>{0, n - 1}, "alternating longest");
    lng count = 0;
    for (int i = 0; i < n; ++i) { count += min(i, n - 1 - i) + 1; }
    expectEqual(countPalindromes(alternating), count, "alternating count");
    for (int i = 0; i <= n; ++i) {
        bool expected = i == n || (n - i) % 2;
        expectEqual(isPalindrome(alternating, i, n), expected, "alternating suffix exact");
        expectEqual(maybePalindrome(h, i, n), expected, "alternating suffix hash");
        expectEqual(maybePalindrome(word, i, n), expected, "alternating suffix word hash");
        expectEqual(maybePalindrome(field, i, n), expected, "alternating suffix Mersenne hash");}
    cout << "PASS large unary/alternating n=" << n << '\n';}
int invalid(const string &probe) {
    Manacher m(string_view("aba"));
    StringHash<> h(string_view("aba"));
    if (probe == "exact-negative") { isPalindrome(m, -1, 1); }
    if (probe == "exact-reversed") { isPalindrome(m, 2, 1); }
    if (probe == "exact-past-end") { isPalindrome(m, 0, 4); }
    if (probe == "hash-negative") { maybePalindrome(h, -1, 1); }
    if (probe == "hash-reversed") { maybePalindrome(h, 2, 1); }
    if (probe == "hash-past-end") { maybePalindrome(h, 0, 4); }
    return 0;}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { return invalid(argv[i + 1]); }}
    exhaustive(mode);
    randomCases(mode);
    lifecycleAndCollisions();
    large(mode);
    cout << "PASS palindrome queries seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
