#include "../../07-Strings/17-bitap.hpp"

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
// A pattern is a list of accepted-symbol sets; plain patterns use singletons.
using Classes = vector<vector<int>>;
bool accepts(const Classes &p, int i, int c) { return std::find(p[i].begin(), p[i].end(), c) != p[i].end(); }
vector<int> bruteHamming(const Classes &p, const vector<int> &t, int k) {
    int n = int(t.size()), m = int(p.size());
    vector<int> res;
    for (int s = 0; s + m <= n; ++s) {
        int bad = 0;
        for (int i = 0; i < m && bad <= k; ++i) { bad += !accepts(p, i, t[s + i]); }
        if (bad <= k) { res.push_back(s); }}
    return res;}
// Brute edit oracle: Levenshtein DP of p against every substring t[s, e).
int levenshtein(const Classes &p, const vector<int> &t, int s, int e) {
    int m = int(p.size()), len = e - s;
    vector<vector<int>> d(m + 1, vector<int>(len + 1));
    for (int i = 0; i <= m; ++i) {
        for (int j = 0; j <= len; ++j) {
            if (!i || !j) { d[i][j] = i + j; continue; }
            d[i][j] = min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + !accepts(p, i - 1, t[s + j - 1])});}}
    return d[m][len];}
vector<int> bruteEdit(const Classes &p, const vector<int> &t, int k) {
    int n = int(t.size());
    vector<int> res;
    for (int e = 0; e <= n; ++e) {
        bool ok = false;
        for (int s = 0; s <= e && !ok; ++s) { ok = levenshtein(p, t, s, e) <= k; }
        if (ok) { res.push_back(e); }}
    return res;}
// Sellers semi-global DP (free start in t), checked against bruteEdit on small cases.
vector<int> sellers(const Classes &p, const vector<int> &t, int k) {
    int n = int(t.size()), m = int(p.size());
    vector<int> col(m + 1), res;
    iota(col.begin(), col.end(), 0);
    if (col[m] <= k) { res.push_back(0); }
    for (int j = 1; j <= n; ++j) {
        int diag = col[0];
        col[0] = 0;
        for (int i = 1; i <= m; ++i) {
            int up = col[i];
            col[i] = min({col[i] + 1, col[i - 1] + 1, diag + !accepts(p, i - 1, t[j - 1])});
            diag = up;}
        if (col[m] <= k) { res.push_back(j); }}
    return res;}
void verifyMasks(const PatternMasks &masks, const Classes &p, int sigma) {
    expect(masks.m, int(p.size()), "patternMasks m");
    expect(masks.words, (int(p.size()) + 63) / 64, "patternMasks words");
    for (int i = 0; i < int(p.size()); ++i) {
        for (int c = 0; c < sigma; ++c) { expect(masks.accepts(i, c), accepts(p, i, c), "patternMasks accepts"); }}}
// Runs every searcher on one (pattern, text); plain = the pattern is a symbol sequence (singletons).
void verify(const Classes &p, const vector<int> &t, int sigma, bool small) {
    ++cases;
    int m = int(p.size());
    bool plain = std::all_of(p.begin(), p.end(), [](const vector<int> &x) { return x.size() == 1; });
    auto masks = patternMasks(p, sigma);
    verifyMasks(masks, p, sigma);
    auto exact = bruteHamming(p, t, 0);
    if (!m) {
        exact.resize(t.size() + 1);
        iota(exact.begin(), exact.end(), 0);}
    vector<int> seq;
    for (const auto &x : p) { seq.push_back(plain ? x[0] : 0); }
    if (m <= 64) {
        expect(shiftAnd(masks, t), exact, "shiftAnd masks");
        expect(shiftOr(masks, t), exact, "shiftOr masks");
        if (plain) {
            expect(shiftAnd(seq, t, sigma), exact, "shiftAnd pattern");
            expect(shiftOr(seq, t, sigma), exact, "shiftOr pattern");}}
    expect(shiftOrMultiword(masks, t), exact, "shiftOrMultiword masks");
    if (plain) {
        verifyMasks(patternMasks(seq, sigma), p, sigma);
        expect(shiftOrMultiword(seq, t, sigma), exact, "shiftOrMultiword pattern");}
    vector<int> ks{0, 1, 2, 3, m / 2, m - 1, m, m + 2};
    sort(ks.begin(), ks.end());
    ks.erase(unique(ks.begin(), ks.end()), ks.end());
    for (int k : ks) {
        if (k < 0) { continue; }
        auto ham = m ? bruteHamming(p, t, k) : exact;
        expect(bitapHamming(masks, t, k), ham, "bitapHamming k=" + std::to_string(k));
        auto edit = sellers(p, t, k);
        if (small) { expect(edit, bruteEdit(p, t, k), "oracle sellers versus brute"); }
        expect(bitapEdit(masks, t, k), edit, "bitapEdit k=" + std::to_string(k));
        if (plain) {
            expect(bitapHamming(seq, t, k, sigma), ham, "bitapHamming pattern");
            expect(bitapEdit(seq, t, k, sigma), edit, "bitapEdit pattern");}}}
Classes singletons(const vector<int> &s) {
    Classes res;
    for (int c : s) { res.push_back({c}); }
    return res;}
void exhaustive(const string &mode) {
    int pm = mode == "quick" ? 3 : mode == "full" ? 4 : 5, tm = mode == "quick" ? 5 : mode == "full" ? 7 : 8;
    auto all = [&](int len, int sigma) {
        vector<vector<int>> res;
        for (int n = 0, count = 1; n <= len; ++n, count *= sigma) {
            for (int code = 0; code < count; ++code) {
                vector<int> s(n);
                for (int x = code, i = 0; i < n; ++i, x /= sigma) { s[i] = x % sigma; }
                res.push_back(s);}}
        return res;};
    for (const auto &t : all(tm, 2)) {
        for (const auto &p : all(pm, 2)) {
            context = "binary pattern size=" + std::to_string(p.size()) + " text size=" + std::to_string(t.size());
            verify(singletons(p), t, 2, true);}}
    // Class patterns over {0, 1, 2}: each position accepts one of the 8 subsets.
    for (const auto &t : all(mode == "stress" ? 5 : 4, 3)) {
        for (const auto &code : all(2, 8)) {
            Classes p;
            for (int x : code) {
                p.push_back({});
                for (int c = 0; c < 3; ++c) {
                    if (x >> c & 1) { p.back().push_back(c); }}}
            context = "ternary class pattern size=" + std::to_string(p.size()) + " text size=" + std::to_string(t.size());
            verify(p, t, 3, true);}}
    cout << "PASS exhaustive binary patterns<=" << pm << " texts<=" << tm << ", ternary class patterns\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int count = mode == "quick" ? 300 : mode == "full" ? 1500 : 6000;
    for (int rep = 0; rep < count; ++rep) {
        int sigma = rep % 7 == 0 ? 256 : int(rng() % 4) + 1, n = int(rng() % 60), m = int(rng() % 200), period = int(rng() % 4) + 1;
        if (rep % 2) { m = int(rng() % 70); }
        vector<int> t(n);
        for (int i = 0; i < n; ++i) { t[i] = i >= period && rep % 3 == 1 ? t[i - period] : int(rng() % sigma); }
        Classes p;
        for (int i = 0; i < m; ++i) {
            if (rep % 5 == 2) {
                p.push_back({});
                for (int c = 0; c < min(sigma, 6); ++c) {
                    if (rng() % 2) { p.back().push_back(c); }}}
            else { p.push_back({n && rng() % 2 ? t[(i + int(rng() % 2)) % n] : int(rng() % sigma)}); }}
        context = "random case=" + std::to_string(rep) + " sigma=" + std::to_string(sigma) + " n=" + std::to_string(n) + " m=" + std::to_string(m);
        verify(p, t, sigma, n <= 8 && m <= 6);
        if (sigma == 256) {
            string ts(n, '\0'), ps(m, '\0');
            for (int i = 0; i < n; ++i) { ts[i] = char(t[i]); }
            bool plain = std::all_of(p.begin(), p.end(), [](const vector<int> &x) { return x.size() == 1; });
            if (!plain) { continue; }
            for (int i = 0; i < m; ++i) { ps[i] = char(p[i][0]); }
            auto exact = bruteHamming(p, t, 0);
            if (!m) { exact.resize(n + 1); iota(exact.begin(), exact.end(), 0); }
            if (m <= 64) { expect(shiftAnd(ps, ts), exact, "shiftAnd bytes"); expect(shiftOr(ps, ts), exact, "shiftOr bytes"); }
            expect(shiftOrMultiword(ps, ts), exact, "shiftOrMultiword bytes");
            expect(bitapHamming(ps, ts, 2), m ? bruteHamming(p, t, 2) : exact, "bitapHamming bytes");
            expect(bitapEdit(ps, ts, 2), sellers(p, t, 2), "bitapEdit bytes");}}
    cout << "PASS random symbol/class patterns through 200 symbols, byte strings cases=" << count << '\n';}
void bitsetText(const string &mode) {
    std::mt19937_64 rng(test_seed + 3);
    int count = mode == "quick" ? 100 : mode == "full" ? 500 : 5000;
    auto brute = [](const vector<int> &t, const vector<int> &p, int l, int r) {
        vector<int> res;
        for (int i = l; i + int(p.size()) <= r; ++i) {
            if (std::equal(p.begin(), p.end(), t.begin() + i)) { res.push_back(i); }}
        return res;};
    for (int rep = 0; rep < count; ++rep) {
        int sigma = rep % 5 == 0 ? 256 : int(rng() % 3) + 1, n = int(rng() % 400);
        vector<int> t(n);
        for (int &c : t) { c = int(rng() % sigma); }
        BitsetText bt(t, sigma);
        context = "BitsetText case=" + std::to_string(rep) + " sigma=" + std::to_string(sigma) + " n=" + std::to_string(n);
        for (int q = 0; q < 30; ++q) {
            if (n && rng() % 3 == 0) {
                int i = int(rng() % n), c = int(rng() % sigma);
                bt.set(i, c);
                t[i] = c;
                expect(bt.get(i), c, "get");
                continue;}
            int m = int(rng() % 4 == 0 ? rng() % 150 : rng() % 6), l = n ? int(rng() % (n + 1)) : 0, r = l + int(rng() % (n - l + 1));
            vector<int> p(m);
            for (int &c : p) { c = int(rng() % sigma); }
            if (m && n && rng() % 2) {
                int at = int(rng() % n);
                for (int j = 0; j < m; ++j) { p[j] = t[(at + j) % n]; }}
            auto expected = brute(t, p, l, r);
            if (!m) { expected.resize(r - l + 1); iota(expected.begin(), expected.end(), l); }
            expect(bt.occurrences(p, l, r), expected, "occurrences l=" + std::to_string(l) + " r=" + std::to_string(r));
            expect(bt.count(p, l, r), int(expected.size()), "count");
            auto whole = m ? brute(t, p, 0, n) : vector<int>();
            if (!m) { whole.resize(n + 1); iota(whole.begin(), whole.end(), 0); }
            expect(bt.occurrences(p), whole, "occurrences default range");
            expect(bt.count(p), int(whole.size()), "count default range");
            if (m) {
                p[rng() % m] = rng() % 2 ? -1 : sigma;
                expect(bt.count(p, l, r), 0, "count foreign symbol");}}
        if (sigma == 256) {
            string s(n, '\0');
            for (int i = 0; i < n; ++i) { s[i] = char(t[i]); }
            BitsetText bs(s);
            string p = n >= 3 ? s.substr(n / 3, 3) : s;
            vector<int> pv(p.size());
            for (int i = 0; i < int(p.size()); ++i) { pv[i] = uint8_t(p[i]); }
            expect(bs.occurrences(p), brute(t, pv, 0, n), "byte text occurrences");}}
    int n = mode == "quick" ? 20000 : 200000;
    vector<int> t(n);
    for (int &c : t) { c = int(rng() % 2); }
    BitsetText bt(t, 2);
    for (int q = 0; q < 20; ++q) {
        int m = 1 + int(rng() % 100), at = int(rng() % (n - m)), l = int(rng() % n), r = l + int(rng() % (n - l + 1));
        vector<int> p(t.begin() + at, t.begin() + at + m);
        context = "BitsetText large n=" + std::to_string(n) + " m=" + std::to_string(m);
        expect(bt.occurrences(p, l, r), brute(t, p, l, r), "large occurrences");
        int i = int(rng() % n);
        bt.set(i, 1 - t[i]);
        t[i] = 1 - t[i];}
    cout << "PASS BitsetText random updates/ranges/foreign symbols cases=" << count << " plus n=" << n << '\n';}
void large(const string &mode) {
    int n = mode == "quick" ? 50000 : mode == "full" ? 100000 : 500000;
    std::mt19937_64 rng(test_seed + 5);
    vector<int> t(n);
    for (int &c : t) { c = int(rng() % 2); }
    for (int m : {1, 63, 64, 65, 130, 1000}) {
        vector<int> p(t.begin() + 100, t.begin() + 100 + m);
        context = "large binary n=" + std::to_string(n) + " m=" + std::to_string(m);
        auto expected = bruteHamming(singletons(p), t, 0);
        if (m <= 64) { expect(shiftAnd(p, t, 2), expected, "shiftAnd"); expect(shiftOr(p, t, 2), expected, "shiftOr"); }
        expect(shiftOrMultiword(p, t, 2), expected, "shiftOrMultiword");
        if (m <= 130) {
            expect(bitapHamming(p, t, 3, 2), bruteHamming(singletons(p), t, 3), "bitapHamming");
            expect(bitapEdit(p, t, 2, 2), sellers(singletons(p), t, 2), "bitapEdit");}}
    vector<int> unary(n, 0), p(300, 0);
    context = "large unary";
    expect(int(shiftOrMultiword(p, unary, 1).size()), n - 299, "unary shiftOrMultiword");
    cout << "PASS large binary word-boundary lengths n=" << n << '\n';}
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
        if (invalid == "shift-and-long") { shiftAnd(string(65, 'a'), string("a")); }
        if (invalid == "shift-or-long") { shiftOr(string(65, 'a'), string("a")); }
        if (invalid == "text-symbol") { shiftOrMultiword(vector<int>{0}, vector<int>{0, 2}, 2); }
        if (invalid == "pattern-symbol") { patternMasks(vector<int>{3}, 2); }
        if (invalid == "class-symbol") { patternMasks(Classes{{0, 5}}, 2); }
        if (invalid == "negative-k") { bitapEdit(string("ab"), string("ab"), -1); }
        if (invalid == "bitset-set") { BitsetText(string("ab")).set(2, 'a'); }
        if (invalid == "bitset-range") { BitsetText(string("ab")).count(string("a"), 2, 1); }
        if (invalid == "bitset-negative-r") { BitsetText(string("ab")).occurrences(string("a"), 0, -5); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    bitsetText(mode);
    large(mode);
    cout << "PASS bitap seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
