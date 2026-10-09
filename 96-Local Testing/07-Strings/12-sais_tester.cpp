#include "../../07-Strings/12-sais.hpp"
#include "../../07-Strings/07-suffixarray.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &op) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=true actual=false\n";
    std::exit(1);}
template<class T> string show(const vector<T> &s) {
    string res = "[";
    for (int i = 0; i < int(s.size()) && i < 40; ++i) {
        if constexpr (std::is_same_v<T, string>) { res += (i ? "," : "") + s[i]; }
        else { res += (i ? "," : "") + std::to_string(s[i]); }}
    return res + (s.size() > 40 ? ",...]" : "]");}
ulng rng_state;
ulng nextRandom() {
    rng_state += 0x9e3779b97f4a7c15ULL;
    ulng z = rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);}
// Independent oracles: comparison-sorted suffixes and character-by-character LCP.
template<class T> vector<int> naiveSa(const vector<T> &s) {
    vector<int> sa(s.size());
    iota(sa.begin(), sa.end(), 0);
    const T *p = s.data(), *end = p + s.size();
    sort(sa.begin(), sa.end(), [&](int i, int j) { return std::lexicographical_compare(p + i, end, p + j, end); });
    return sa;}
template<class T> vector<int> naiveLcp(const vector<T> &s, const vector<int> &sa) {
    vector<int> res;
    for (int k = 0; k + 1 < int(sa.size()); ++k) {
        int i = sa[k], j = sa[k + 1], len = 0;
        while (i + len < int(s.size()) && j + len < int(s.size()) && s[i + len] == s[j + len]) { ++len; }
        res.push_back(len);}
    return res;}
void verifyInt(const vector<int> &s, int upper) {
    ++cases;
    context = show(s) + " upper=" + std::to_string(upper);
    auto expected = naiveSa(s);
    check(sais(s, upper) == expected, "sais default threshold");
    check(sais<1>(s, upper) == expected, "sais pure SA-IS (NAIVE=1)");
    check(sais<4>(s, upper) == expected, "sais NAIVE=4");
    check(sais(s) == expected, "sais generic vector<int> (compressed)");
    check(lcpArray(s, expected) == naiveLcp(s, expected), "lcpArray vector<int>");
    auto [codes, k] = compressAlphabet(s);
    set<int> distinct(s.begin(), s.end());
    check(k == int(distinct.size()), "compressAlphabet k");
    for (int i = 0; i < int(s.size()); ++i) { check(codes[i] == int(std::distance(distinct.begin(), distinct.find(s[i]))), "compressAlphabet rank"); }}
void verifyBytes(const string &s) {
    ++cases;
    context = "bytes size=" + std::to_string(s.size());
    vector<int> u(s.begin(), s.end());
    for (int &c : u) { c = uint8_t(c); }
    auto expected = naiveSa(u);
    check(sais(string_view(s)) == expected, "sais string_view unsigned order");
    check(sais(s) == expected, "sais string");
    check(lcpArray(s, expected) == naiveLcp(u, expected), "lcpArray string");
    check(lcpArray(string_view(s), expected) == naiveLcp(u, expected), "lcpArray string_view");}
template<class T> void verifyGeneric(const vector<T> &s) {
    ++cases;
    context = "generic size=" + std::to_string(s.size());
    auto expected = naiveSa(s);
    check(sais(s) == expected, "sais generic vector<T>");
    check(lcpArray(s, expected) == naiveLcp(s, expected), "lcpArray generic");
    auto [codes, k] = compressAlphabet(s);
    vector<T> sorted = s;
    sort(sorted.begin(), sorted.end());
    sorted.erase(unique(sorted.begin(), sorted.end()), sorted.end());
    check(k == int(sorted.size()), "compressAlphabet generic k");
    for (int i = 0; i < int(s.size()); ++i) { check(sorted[codes[i]] == s[i] && codes[i] < k, "compressAlphabet generic rank"); }
    for (int i = 0; i < int(s.size()); ++i) {
        for (int j = 0; j < int(s.size()); ++j) { check((s[i] < s[j]) == (codes[i] < codes[j]), "compressAlphabet order"); }}}
void exhaustive(const string &mode) {
    int limit = mode == "quick" ? 7 : mode == "full" ? 10 : 11;
    vector<vector<int>> words{{}};
    for (int len = 0; len <= limit; ++len) {
        for (auto &w : words) {
            verifyInt(w, 2);
            if (len <= 6) { verifyInt(w, 4); }}
        vector<vector<int>> longer;
        for (auto &w : words) {
            for (int c = 0; c < 3; ++c) { longer.push_back(w); longer.back().push_back(c); }}
        words.swap(longer);}
    int binary = mode == "quick" ? 16 : 18;
    for (int len = 13; len <= binary; ++len) {
        int step = len <= 16 ? 1 : 7;
        for (int mask = 0; mask < (1 << len); mask += step) {
            vector<int> w(len);
            for (int i = 0; i < len; ++i) { w[i] = mask >> i & 1; }
            ++cases;
            context = show(w);
            auto expected = naiveSa(w);
            check(sais<1>(w, 1) == expected && sais(w, 1) == expected, "binary threshold neighbours");}}
    cout << "PASS exhaustive ternary texts through length " << limit << ", binary texts of lengths 13.." << binary << '\n';}
void randomCases(const string &mode) {
    int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 15000;
    for (int r = 0; r < rounds; ++r) {
        int n = r % 7 ? int(nextRandom() % 300) : 30 + r % 5, sigma = 1 + int(nextRandom() % 6), upper = sigma - 1 + int(nextRandom() % 3) * int(nextRandom() % 1000);
        vector<int> s(n);
        for (int &c : s) { c = int(nextRandom() % sigma) * (upper / max(sigma - 1, 1)); }
        if (r % 5 == 0) {
            for (int i = 0; i < n; ++i) { s[i] = i % (1 + r % 4) ? s[i % (1 + r % 4)] : int(nextRandom() % 2) * upper; }}
        verifyInt(s, upper);
        string b(nextRandom() % 200, '\0');
        for (char &c : b) { c = char(nextRandom() % (r % 3 ? 256 : 3)); }
        verifyBytes(b);
        vector<lng> wide(nextRandom() % 60);
        lng extremes[] = {std::numeric_limits<lng>::min(), -1, 0, 1, std::numeric_limits<lng>::max()};
        for (lng &x : wide) { x = extremes[nextRandom() % 5]; }
        verifyGeneric(wide);
        vector<signed char> sc(nextRandom() % 60);
        for (auto &x : sc) { x = static_cast<signed char>(nextRandom() % 256); }
        verifyGeneric(sc);
        vector<string> words(nextRandom() % 30);
        for (auto &x : words) { x = string(nextRandom() % 3, char('a' + nextRandom() % 2)); }
        verifyGeneric(words);}
    string all;
    for (int c = 0; c < 256; ++c) { all.push_back(char(c)); }
    verifyBytes(all);
    verifyBytes(string(all.rbegin(), all.rend()));
    verifyBytes(string(50, '\0'));
    verifyBytes("");
    verifyInt({}, 0);
    verifyInt({0}, 0);
    verifyInt({5}, 9);
    verifyGeneric(vector<ulng>{~0ULL, 0, ~0ULL, 1});
    cout << "PASS random integer/byte/lng/signed char/string-symbol texts rounds=" << rounds << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 5000 : mode == "full" ? 200000 : 700000;
    vector<vector<int>> texts;
    texts.push_back(vector<int>(n, 0));
    vector<int> periodic(n), fib, thue(n), random(n), big(n);
    for (int i = 0; i < n; ++i) { periodic[i] = i % 3; }
    texts.push_back(periodic);
    vector<int> a{0}, b{1};
    while (int(a.size()) < n) { vector<int> c = a; c.insert(c.end(), b.begin(), b.end()); b = a; a = c; }
    texts.push_back(vector<int>(a.begin(), a.begin() + n));
    for (int i = 0; i < n; ++i) { thue[i] = __builtin_popcount(uint(i)) & 1; }
    texts.push_back(thue);
    for (int &c : random) { c = int(nextRandom() % 4); }
    texts.push_back(random);
    for (int &c : big) { c = int(nextRandom() % 1000000); }
    texts.push_back(big);
    for (auto &s : texts) {
        ++cases;
        context = "large n=" + std::to_string(n) + " prefix=" + show(vector<int>(s.begin(), s.begin() + 20));
        int upper = *std::max_element(s.begin(), s.end());
        SuffixArray<int> oracle(s, false);
        auto sa = sais(s, upper);
        check(sa == oracle.sa, "large sais versus prefix-doubling SuffixArray");
        check(lcpArray(s, sa) == oracle.lcp, "large lcpArray versus SuffixArray");
        check(sais(s) == sa, "large generic sais");}
    cout << "PASS large unary/period-3/Fibonacci/Thue-Morse/random/sparse n=" << n << " against SuffixArray\n";}
int invalid(const string &probe) {
    if (probe == "negative-upper") { sais(vector<int>{}, -1); }
    if (probe == "symbol-above") { sais(vector<int>{0, 3}, 2); }
    if (probe == "negative-symbol") { sais(vector<int>{0, -1}, 2); }
    if (probe == "lcp-size") { lcpArray(string("ab"), vector<int>{0}); }
    return 0;}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { return invalid(argv[i + 1]); }}
    rng_state = test_seed;
    exhaustive(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS sais seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
