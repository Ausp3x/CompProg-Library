#include "../../07-Strings/15-minrotation.hpp"

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
// Brute force: rotation k is the slice [k, k + n) of key + key, compared element by element (bytes as unsigned).
template<class S> auto keys(const S &s) {
    using K = std::conditional_t<std::is_same_v<std::decay_t<decltype(s[0])>, char>, int, std::decay_t<decltype(s[0])>>;
    vector<K> res;
    for (auto c : s) {
        if constexpr (std::is_same_v<decltype(c), char>) { res.push_back(uint8_t(c)); }
        else { res.push_back(c); }}
    return res;}
template<class K> vector<K> rotate(const vector<K> &s, int k) {
    vector<K> res(s.begin() + k, s.end());
    res.insert(res.end(), s.begin(), s.begin() + k);
    return res;}
template<class K> int compareRotations(const vector<K> &dk, int a, int b, int n) {
    for (int t = 0; t < n; ++t) {
        if (dk[a + t] != dk[b + t]) { return dk[a + t] < dk[b + t] ? -1 : 1; }}
    return 0;}
template<class K> bool rotationIs(const vector<K> &dk, int k, const vector<K> &t) { return std::equal(t.begin(), t.end(), dk.begin() + k); }
template<class S> void verify(const S &s) {
    ++cases;
    auto key = keys(s), dk = key;
    dk.insert(dk.end(), key.begin(), key.end());
    int n = int(key.size()), lo = 0, hi = 0, second = n;
    for (int k = 1; k < n; ++k) {
        if (compareRotations(dk, k, lo, n) < 0) { lo = k; }
        if (compareRotations(dk, hi, k, n) < 0) { hi = k; }}
    for (int k = lo + 1; k < n && second == n; ++k) {
        if (!compareRotations(dk, k, lo, n)) { second = k; }}
    expectEqual(minRotation(s), lo, "minRotation");
    expectEqual(maxRotation(s), hi, "maxRotation");
    auto [first, gap] = minRotationIndices(s);
    expectEqual(first, lo, "minRotationIndices first");
    expectEqual(gap, second == n ? n : second - lo, "minRotationIndices gap");
    S canon = canonicalRotation(s);
    check(keys(canon) == rotate(key, lo), "canonicalRotation");
    vector<int> doubled(2 * n);
    for (int i = 0; i < 2 * n; ++i) { doubled[i] = int(i < n ? key[i] : key[i - n]); }
    if constexpr (std::is_integral_v<std::decay_t<decltype(key[0])>>) {
        if (std::all_of(key.begin(), key.end(), [](auto c) { return INT_MIN <= lng(c) && lng(c) <= INT_MAX; })) {
            expectEqual(minRotation(SuffixArray<int>(doubled)), lo, "minRotation suffix array");}}
    for (int k = 0; k < n; ++k) {
        S t(s.begin() + k, s.end());
        t.insert(t.end(), s.begin(), s.begin() + k);
        int expected = k;
        auto tk = keys(t);
        for (int q = 0; q < k; ++q) {
            if (rotationIs(dk, q, tk)) { expected = q; break; }}
        expectEqual(rotationOffset(s, t), expected, "rotationOffset rotation k=" + std::to_string(k));
        check(rotationEquivalent(s, t) && rotationEquivalent(t, s), "rotationEquivalent rotation");}
    if (n) {
        S t = s;
        t[n - 1] = t[(n - 1) / 2];
        auto tk = keys(t);
        bool rotation = false;
        for (int k = 0; k < n; ++k) { rotation |= rotationIs(dk, k, tk); }
        int expected = -1;
        for (int k = n - 1; k >= 0; --k) {
            if (rotationIs(dk, k, tk)) { expected = k; }}
        expectEqual(rotationOffset(s, t), expected, "rotationOffset modified");
        check(rotationEquivalent(s, t) == rotation, "rotationEquivalent modified");
        S shorter(s.begin(), s.end() - 1);
        expectEqual(rotationOffset(s, shorter), -1, "rotationOffset length mismatch");
        check(!rotationEquivalent(shorter, s), "rotationEquivalent length mismatch");}}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 7 : mode == "full" ? 9 : 10, count = 1;
    for (int n = 0; n <= bound; ++n, count *= 3) {
        for (int code = 0; code < count; ++code) {
            string s(n, 'a');
            for (int x = code, i = 0; i < n; ++i, x /= 3) { s[i] = char('a' + x % 3); }
            context = "ternary " + s;
            verify(s);}}
    cout << "PASS exhaustive ternary brute rotations lengths=0.." << bound << '\n';}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 300 : mode == "full" ? 3000 : 20000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 41), sigma = rep % 3 == 0 ? 256 : int(rng() % 4) + 1, period = int(rng() % 5) + 1;
        string s(n, '\0');
        for (int i = 0; i < n; ++i) { s[i] = i >= period && rep % 4 == 1 ? s[i - period] : char(rng() % sigma); }
        context = "random bytes case=" + std::to_string(rep) + " n=" + std::to_string(n);
        verify(s);
        vector<lng> ints(n);
        for (int i = 0; i < n; ++i) { ints[i] = s[i] % 2 ? std::numeric_limits<lng>::min() + uint8_t(s[i]) : std::numeric_limits<lng>::max() - uint8_t(s[i]); }
        verify(ints);
        vector<int> small(n);
        for (int i = 0; i < n; ++i) { small[i] = int(int8_t(s[i])); }
        verify(small);}
    string bytes;
    for (int c = 255; c >= 0; --c) { bytes += char(c); }
    context = "all 256 bytes descending";
    verify(bytes);
    cout << "PASS random bytes/periodic/extreme integers cases=" << count << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 20000 : mode == "full" ? 300000 : 1000000;
    std::mt19937_64 rng(test_seed + 7);
    string s(n, 'a');
    context = "large unary n=" + std::to_string(n);
    check(minRotationIndices(s) == pair<int, int>{0, 1} && maxRotation(s) == 0, "unary indices");
    s.resize(n - n % 3);
    for (int i = 0; i < int(s.size()); ++i) { s[i] = "bca"[i % 3]; }
    context = "large period 3";
    check(minRotationIndices(s) == pair<int, int>{2, 3} && maxRotation(s) == 1, "period-3 indices");
    s.resize(n);
    for (char &c : s) { c = char('a' + rng() % 2); }
    context = "large random binary";
    string d = s + s;
    int expected = minRotation(SuffixArray<int>(string_view(d)));
    expectEqual(minRotation(s), expected, "random binary versus suffix array");
    string t = s.substr(12345 % n) + s.substr(0, 12345 % n);
    check(rotationEquivalent(s, t) && canonicalRotation(t) == canonicalRotation(s), "large rotation");
    cout << "PASS large unary/periodic/random versus suffix array n=" << n << '\n';}
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
        if (invalid == "suffix-array-odd") { minRotation(SuffixArray<int>(string_view("abc"))); }
        if (invalid == "suffix-array-halves") { minRotation(SuffixArray<int>(string_view("abac"))); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS minrotation seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
