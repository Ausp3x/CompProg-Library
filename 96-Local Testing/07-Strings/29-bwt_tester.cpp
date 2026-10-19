#include "../../07-Strings/07-suffixarray.hpp"
#include "../../07-Strings/29-bwt.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A, class B> void expect(const A &actual, const B &expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=";
    if constexpr (std::is_arithmetic_v<B>) { cerr << expected << " actual=" << actual; }
    else {
        for (auto x : expected) { cerr << lng(x) << ','; }
        cerr << " actual=";
        for (auto x : actual) { cerr << lng(x) << ','; }}
    cerr << '\n';
    std::exit(1);}
// Brute keys: bytes as unsigned, other symbols as themselves; the sentinel key sorts below everything.
template<class S> auto keys(const S &s) {
    vector<lll> res;
    for (auto c : s) {
        if constexpr (std::is_same_v<decltype(c), char>) { res.push_back(uint8_t(c)); }
        else { res.push_back(c); }}
    return res;}
vector<lll> rotation(const vector<lll> &k, int i) {
    vector<lll> res(k.begin() + i, k.end());
    res.insert(res.end(), k.begin(), k.begin() + i);
    return res;}
// Brute rotation order: materialized rotations sorted by (rotation, start).
vector<int> bruteRotations(const vector<lll> &k) {
    int n = int(k.size());
    vector<int> res(n);
    iota(res.begin(), res.end(), 0);
    std::stable_sort(res.begin(), res.end(), [&](int a, int b) { return rotation(k, a) < rotation(k, b); });
    return res;}
// Brute stable order of L by key, the sentinel row d first.
vector<int> brutePsi(const vector<lll> &k, int d) {
    vector<int> res(k.size());
    iota(res.begin(), res.end(), 0);
    std::stable_sort(res.begin(), res.end(), [&](int a, int b) { return (a == d ? -1 : 0) < (b == d ? -1 : 0) || ((a == d) == (b == d) && k[a] < k[b]); });
    return res;}
// Brute Lyndon factors by rotation comparison, conjugates in omega order via (L1 + L2)-prefixes of the repetitions.
vector<pair<int, int>> bruteFactors(const vector<lll> &k) {
    int n = int(k.size());
    vector<pair<int, int>> res;
    for (int i = 0; i < n;) {
        int best = i + 1;
        for (int j = i + 1; j <= n; ++j) {
            vector<lll> w(k.begin() + i, k.begin() + j);
            bool lyndon = true;
            for (int t = 1; t < int(w.size()) && lyndon; ++t) { lyndon = w < rotation(w, t); }
            if (lyndon) { best = j; }}
        res.push_back({i, best - i});
        i = best;}
    return res;}
vector<lll> bruteBijective(const vector<lll> &k) {
    vector<vector<lll>> conj;
    for (auto [b, len] : bruteFactors(k)) {
        vector<lll> w(k.begin() + b, k.begin() + b + len);
        for (int t = 0; t < len; ++t) { conj.push_back(rotation(w, t)); }}
    auto omega = [](const vector<lll> &u, const vector<lll> &v) {
        int len = int(u.size() + v.size());
        for (int i = 0; i < len; ++i) {
            if (u[i % u.size()] != v[i % v.size()]) { return u[i % u.size()] < v[i % v.size()]; }}
        return false;};
    std::stable_sort(conj.begin(), conj.end(), omega);
    vector<lll> res;
    for (const auto &w : conj) { res.push_back(w.back()); }
    return res;}
template<class S> void verify(const S &s, typename S::value_type sentinel) {
    ++cases;
    auto k = keys(s);
    int n = int(k.size());
    auto order = bruteRotations(k);
    expect(sortCyclicRotations(s), order, "sortCyclicRotations");
    // Sentinel BWT: rotations of s + sentinel with the sentinel key below every symbol.
    vector<lll> ks = k;
    ks.push_back(std::numeric_limits<lll>::min());
    auto sorder = bruteRotations(ks);
    S expected(n + 1, sentinel);
    for (int r = 0; r <= n; ++r) {
        int p = sorder[r] ? sorder[r] - 1 : n;
        if (p < n) { expected[r] = s[p]; }}
    S b = bwt(s, sentinel);
    expect(b, expected, "bwt sentinel");
    vector<int> sa;
    for (int r = 1; r <= n; ++r) { sa.push_back(sorder[r]); }
    expect(bwtFromSuffixArray(s, sa, sentinel), expected, "bwtFromSuffixArray");
    expect(inverseBwt(b, sentinel), s, "inverseBwt");
    int d = int(std::find(b.begin(), b.end(), sentinel) - b.begin());
    auto psi = brutePsi(keys(b), d);
    expect(psiMapping(b, sentinel), psi, "psiMapping sentinel");
    auto lf = lfMapping(b, sentinel);
    for (int r = 0; r <= n; ++r) {
        expect(lf[psi[r]], r, "lfMapping sentinel inverse");
        expect(sorder[lf[r]], sorder[r] ? sorder[r] - 1 : n, "lfMapping sentinel steps back");}
    // Rotation matrix: last column and the smallest row holding s.
    auto [last, primary] = bwtPrimary(s);
    S cyc = s;
    int first = 0;
    for (int r = n - 1; r >= 0; --r) {
        cyc[r] = s[order[r] ? order[r] - 1 : n - 1];
        if (rotation(k, order[r]) == k) { first = r; }}
    expect(last, cyc, "bwtPrimary last column");
    expect(primary, first, "bwtPrimary primary");
    expect(inverseBwtPrimary(last, primary), s, "inverseBwtPrimary");
    auto cpsi = brutePsi(keys(last), -1);
    expect(psiMapping(last), cpsi, "psiMapping rotations");
    auto clf = lfMapping(last);
    for (int r = 0; r < n; ++r) {
        expect(clf[cpsi[r]], r, "lfMapping rotations inverse");
        expect(rotation(k, order[clf[r]]), rotation(k, order[r] ? order[r] - 1 : n - 1), "lfMapping rotations steps back");}
    // Bijective BWT: brute omega-sorted conjugates of the brute Lyndon factors.
    S bij = bijectiveBwt(s);
    expect(keys(bij), bruteBijective(k), "bijectiveBwt");
    expect(inverseBijectiveBwt(bij), s, "inverseBijectiveBwt");
    expect(bijectiveBwt(inverseBijectiveBwt(s)), s, "bijectiveBwt onto");}
void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 7 : mode == "full" ? 9 : 10;
    for (int sigma = 1; sigma <= 3; ++sigma) {
        for (int n = 0, count = 1; n <= bound - (sigma == 3) * 2; ++n, count *= sigma) {
            for (int code = 0; code < count; ++code) {
                string s(n, 'a');
                for (int x = code, i = 0; i < n; ++i, x /= sigma) { s[i] = char('a' + x % sigma); }
                context = "exhaustive " + s;
                verify(s, '$');}}}
    cout << "PASS exhaustive alphabets 1..3 lengths<=" << bound << " (ternary " << bound - 2 << ")\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 300 : mode == "full" ? 3000 : 6000;
    for (int rep = 0; rep < count; ++rep) {
        int n = int(rng() % 40), sigma = rep % 3 == 0 ? 255 : int(rng() % 4) + 1, period = int(rng() % 5) + 1;
        string s(n, '\0');
        for (int i = 0; i < n; ++i) { s[i] = char(i >= period && rep % 4 == 1 ? s[i - period] : char(1 + rng() % sigma)); }
        context = "random bytes case=" + std::to_string(rep) + " n=" + std::to_string(n);
        verify(s, '\0');
        if (n) {
            string t = s;
            for (char &c : t) { c = char(uint8_t(c) - 1); }
            verify(t, '\xff' == t[0] ? '\x01' : '\xff');}
        vector<lng> v(n);
        for (int i = 0; i < n; ++i) { v[i] = uint8_t(s[i]) % 2 ? std::numeric_limits<lng>::max() - uint8_t(s[i]) : std::numeric_limits<lng>::min() + 1 + uint8_t(s[i]); }
        verify(v, std::numeric_limits<lng>::min());
        vector<int> w(n);
        for (int i = 0; i < n; ++i) { w[i] = int(int8_t(s[i])); }
        verify(w, 1000);}
    cout << "PASS random bytes/periodic/extreme lng/signed int cases=" << count << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 30000 : mode == "full" ? 200000 : 500000;
    std::mt19937_64 rng(test_seed + 9);
    string rnd(n, 'a'), per(n, 'a'), fib = "a", prev = "b";
    for (char &c : rnd) { c = char('a' + rng() % 4); }
    for (int i = 0; i < n; ++i) { per[i] = "abaab"[i % 5]; }
    while (int(fib.size()) < n) { string next = fib + prev; prev = fib; fib = next; }
    fib.resize(n);
    // Independent prefix-doubling suffix order over unsigned byte codes (SuffixArray's constructors run SA-IS, as bwt does).
    auto doublingSa = [](const string &t) {
        vector<int> codes(t.begin(), t.end());
        for (int &c : codes) { c = uint8_t(c); }
        return SuffixArray<int>::doubling(codes, false).sa;};
    for (auto [name, s] : vector<pair<string, string>>{{"random", rnd}, {"period-5", per}, {"fibonacci", fib}, {"unary", string(n, 'z')}}) {
        context = "large " + name + " n=" + std::to_string(n);
        string b = bwt(s, '$');
        expect(inverseBwt(b, '$'), s, "large inverseBwt");
        expect(bwtFromSuffixArray(s, doublingSa(s), '$'), b, "large bwt versus doubling SuffixArray");
        auto [last, primary] = bwtPrimary(s);
        expect(inverseBwtPrimary(last, primary), s, "large inverseBwtPrimary");
        auto order = sortCyclicRotations(s);
        expect(order[primary], 0, "large primary row");
        string bij = bijectiveBwt(s);
        expect(inverseBijectiveBwt(bij), s, "large inverseBijectiveBwt");
        string sorted = s, f = bij;
        sort(sorted.begin(), sorted.end());
        sort(f.begin(), f.end());
        expect(f, sorted, "large bijective permutation");}
    // Aperiodic random text: rotation order equals the suffix order of s + s restricted to starts below n.
    string d = rnd + rnd;
    vector<int> expected;
    for (int i : doublingSa(d)) {
        if (i < n) { expected.push_back(i); }}
    context = "large random rotations";
    expect(sortCyclicRotations(rnd), expected, "large sortCyclicRotations versus doubling SuffixArray");
    cout << "PASS large random/periodic/Fibonacci/unary round trips n=" << n << '\n';}
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
        if (invalid == "sentinel-in-text") { bwt(string("a$b"), '$'); }
        if (invalid == "no-sentinel") { inverseBwt(string("ab"), '$'); }
        if (invalid == "two-sentinels") { inverseBwt(string("a$$"), '$'); }
        if (invalid == "primary-range") { inverseBwtPrimary(string("ab"), 2); }
        if (invalid == "suffix-array-size") { bwtFromSuffixArray(string("ab"), vector<int>{0}, '$'); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS bwt seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
