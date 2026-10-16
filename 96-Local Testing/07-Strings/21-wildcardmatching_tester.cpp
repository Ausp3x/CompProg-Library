#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wshadow"
#include "../../06-Miscellaneous/02-customhash.hpp"
#pragma GCC diagnostic pop
#include "../../07-Strings/21-wildcardmatching.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A, class B> void expect(const A &actual, const B &expected, const char *op) {
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
template<class S> string show(const S &s) {
    string res;
    for (auto c : s) {
        if constexpr (std::is_same_v<typename S::value_type, char>) { res += c; }
        else { res += std::to_string(c) + ' '; }}
    return res.size() > 120 ? res.substr(0, 120) + "...(" + std::to_string(s.size()) + ")" : res;}
// Brute wildcard oracle: every start, every position.
template<class S, class W> vector<int> bruteWildcard(const S &t, const S &p, W w) {
    int n = int(t.size()), m = int(p.size());
    vector<int> res;
    for (int i = 0; i + m <= n; ++i) {
        bool ok = true;
        for (int j = 0; j < m && ok; ++j) { ok = p[j] == w || t[i + j] == w || p[j] == t[i + j]; }
        if (ok) { res.push_back(i); }}
    return res;}
// Glob DP over (text prefix, pattern prefix); row[i] = text[0, i) matches the whole pattern.
vector<char> globTable(const string &t, const string &p, char any, char star) {
    int n = int(t.size()), m = int(p.size());
    vector<char> cur(m + 1), next(m + 1), res(n + 1);
    cur[0] = 1;
    for (int j = 1; j <= m; ++j) { cur[j] = cur[j - 1] && p[j - 1] == star; }
    res[0] = cur[m];
    for (int i = 1; i <= n; ++i) {
        next[0] = 0;
        for (int j = 1; j <= m; ++j) {
            char c = p[j - 1];
            next[j] = c == star ? (next[j - 1] || cur[j]) : (cur[j - 1] && (c == any || c == t[i - 1]));}
        swap(cur, next);
        res[i] = cur[m];}
    return res;}
// Backtracking glob, exponential, validates the DP on tiny inputs.
bool globBacktrack(const string &t, size_t i, const string &p, size_t j, char any, char star) {
    if (j == p.size()) { return i == t.size(); }
    if (p[j] == star) {
        for (size_t k = i; k <= t.size(); ++k) {
            if (globBacktrack(t, k, p, j + 1, any, star)) { return true; }}
        return false;}
    return i < t.size() && (p[j] == any || p[j] == t[i]) && globBacktrack(t, i + 1, p, j + 1, any, star);}
lng fallbacks = 0;
// Counts the cases whose budgeted scan hands the remaining starts to the block convolution.
template<class S, class W> void checkWildcard(const S &t, const S &p, W w, ulng seed) {
    ++cases;
    context = "text=" + show(t) + " pattern=" + show(p);
    int n = int(t.size()), m = int(p.size());
    int from = wildcard_detail::scan(0, n, m, wildcard_detail::BUDGET, [&](int i, int j) { return p[j] == w || t[i] == w || p[j] == t[i]; }, [](int) { return false; });
    fallbacks += from + m <= n;
    auto expected = bruteWildcard(t, p, w);
    expect(wildcardMatch(t, p, w, seed), expected, "wildcardMatch");
    expect(wildcardMatchDeterministic(t, p, w), expected, "wildcardMatchDeterministic");}
void checkGlob(const string &t, const string &p, char any = '?', char star = '*') {
    ++cases;
    context = "text=" + show(t) + " pattern=" + show(p);
    auto table = globTable(t, p, any, star);
    expect(globMatch(t, p, any, star), bool(table.back()), "globMatch");
    GlobAutomaton g(p, any, star);
    expect(g.accepted(), bool(table[0]), "GlobAutomaton empty prefix");
    for (int i = 0; i < int(t.size()); ++i) { expect(g.step(t[i]), bool(table[i + 1]), "GlobAutomaton::step"); }
    g.reset();
    expect(g.accepted(), bool(table[0]), "GlobAutomaton::reset");
    if (!t.empty()) { expect(g.step(t[0]), bool(table[1]), "GlobAutomaton after reset"); }}
string randomString(std::mt19937_64 &rng, int n, const string &alphabet) {
    string s(n, ' ');
    for (char &c : s) { c = alphabet[rng() % alphabet.size()]; }
    return s;}
void exhaustive(const string &mode) {
    int tn = mode == "quick" ? 5 : mode == "full" ? 7 : 8, pm = mode == "quick" ? 3 : mode == "full" ? 4 : 5;
    lng before = cases;
    auto each = [&](int len, const string &alphabet, auto f) {
        string s(len, alphabet[0]);
        for (lng code = 0, total = lng(std::pow(alphabet.size(), len)); code < total; ++code) {
            lng x = code;
            for (int i = 0; i < len; ++i) { s[i] = alphabet[x % alphabet.size()]; x /= lng(alphabet.size()); }
            f(s);}};
    for (int n = 0; n <= tn; ++n) {
        each(n, "ab?", [&](const string &t) {
            for (int m = 0; m <= pm; ++m) { each(m, "ab?", [&](const string &p) { checkWildcard(t, p, '?', test_seed + ulng(m)); }); }});}
    for (int n = 0; n <= tn - 1; ++n) {
        each(n, "ab", [&](const string &t) {
            for (int m = 0; m <= pm + 1; ++m) {
                each(m, "ab?*", [&](const string &p) {
                    if (n <= 5 && m <= 4) {
                        context = "backtrack text=" + t + " pattern=" + p;
                        expect(bool(globTable(t, p, '?', '*').back()), globBacktrack(t, 0, p, 0, '?', '*'), "glob DP self-check");}
                    checkGlob(t, p);});}});}
    cout << "PASS exhaustive wildcard over {a,b,?} texts<=" << tn << " patterns<=" << pm << ", glob over {a,b,?,*} cases=" << cases - before << '\n';}
// Random wildcard cases: cheap texts stay in the scan, low-entropy texts and late unary tails hand over to the blocks.
void randomWildcard(const string &mode) {
    std::mt19937_64 rng(test_seed + 1);
    int count = mode == "quick" ? 150 : mode == "full" ? 1200 : 6000;
    lng before = cases, fell = fallbacks;
    for (int it = 0; it < count; ++it) {
        int m = it % 4 == 0 ? 14 + int(rng() % 6) : 1 + int(rng() % 300), n = m + int(rng() % (it % 3 == 0 ? 12 * m : 3 * m + 1));
        int tw = int(rng() % 4) * 10, pw = int(rng() % 5) * 20, sigma = it % 3 == 1 ? 1 : 1 + int(rng() % 3);
        string t(n, ' '), p(m, ' ');
        for (char &c : t) { c = int(rng() % 100) < tw ? '?' : char('a' + rng() % sigma); }
        for (char &c : p) { c = int(rng() % 100) < pw ? '?' : char('a' + rng() % sigma); }
        if (it % 3 == 2) {
            for (int i = int(rng() % (n + 1)); i < n; ++i) { t[i] = 'a'; }
            for (char &c : p) { c = c == '?' || rng() % 8 ? c : 'a'; }}
        if (it % 5 == 0) {
            int at = int(rng() % (n - m + 1));
            for (int j = 0; j < m; ++j) {
                if (p[j] != '?' && t[at + j] != '?') { t[at + j] = p[j]; }}}
        checkWildcard(t, p, '?', rng());
        if (it % 7 == 0) {
            vector<lng> tv(n), pv(m);
            for (int i = 0; i < n; ++i) { tv[i] = t[i] == '?' ? LLONG_MIN : (t[i] - 'a') * 1000000000000LL - 5; }
            for (int j = 0; j < m; ++j) { pv[j] = p[j] == '?' ? LLONG_MIN : (p[j] - 'a') * 1000000000000LL - 5; }
            checkWildcard(tv, pv, LLONG_MIN, rng());}
        if (it % 11 == 0) {
            string tb = t, pb = p;
            for (char &c : tb) { c = c == '?' ? '\0' : char(c - 'a' + 200); }
            for (char &c : pb) { c = c == '?' ? '\0' : char(c - 'a' + 200); }
            checkWildcard(tb, pb, '\0', rng());}}
    for (int m : {15, 16, 17, 64, 200}) {
        for (int n : {m - 1, m, m + 1, 8 * m, 8 * m + 1, 30 * m}) {
            string t = randomString(rng, n, "ab"), p = randomString(rng, m, "ab??");
            checkWildcard(t, p, '?', rng());
            checkWildcard(t, string(m, '?'), '?', rng());
            checkWildcard(string(n, '?'), p, '?', rng());
            checkWildcard(string(n, 'a'), string(m - 1, 'a') + "b", '?', rng());}}
    // Wide alphabets on the block path: a long zero prefix makes every start expensive, the tail carries up to 2000 distinct symbols.
    for (int r = 0; r < (mode == "quick" ? 10 : 60); ++r) {
        int k = 40 + int(rng() % 60), tail = 1 + int(rng() % 2000), m = k + tail, quiet = 2 * m + 1000, n = quiet + 2 * m + int(rng() % 2000), alphabet = 2 + int(rng() % 3000);
        vector<int> p(k, 0), t(n, 0);
        for (int j = 0; j < tail; ++j) { p.push_back(rng() % 10 ? 1 + int(rng() % alphabet) : -7); }
        for (int &x : t) { x = rng() % 50 ? 0 : 1 + int(rng() % alphabet); }
        for (int c = 0; c < 2; ++c) {
            int at = quiet + int(rng() % (n - quiet - m + 1));
            for (int j = 0; j < int(p.size()); ++j) { t[at + j] = p[j] == -7 ? int(rng() % alphabet) : p[j]; }}
        lng was = fallbacks;
        checkWildcard(t, p, -7, rng());
        expect(fallbacks > was, true, "wide alphabet hands over to the blocks");}
    vector<int> ti(500, 1), pi(60, 1);
    for (int i = 0; i < 500; i += 3) { ti[i] = int(rng() % 3) - 1; }
    for (int &x : pi) { x = rng() % 4 ? 1 : 0; }
    checkWildcard(ti, pi, 0, rng());
    expect(fallbacks - fell > count / 10, true, "convolution fallbacks exercised");
    cout << "PASS random wildcard (scan budget 16, block handover, lng/int/byte symbols) cases=" << cases - before << " fallbacks=" << fallbacks - fell << '\n';}
// Glob cases: KMP segments, short and long segments with any, stars at both ends, consecutive stars, word boundaries.
void randomGlob(const string &mode) {
    std::mt19937_64 rng(test_seed + 2);
    int count = mode == "quick" ? 150 : mode == "full" ? 1500 : 6000;
    lng before = cases;
    for (int it = 0; it < count; ++it) {
        int n = int(rng() % (it % 4 ? 60 : 400)), sigma = 1 + int(rng() % 3);
        string t = randomString(rng, n, string("abc").substr(0, sigma)), p;
        int pieces = int(rng() % 5);
        for (int k = 0; k < pieces; ++k) {
            if (k || rng() % 2) { p += string(1 + rng() % 2, '*'); }
            int len = it % 6 == 0 ? 41 + int(rng() % 60) : int(rng() % 6);
            int at = n > len ? int(rng() % (n - len + 1)) : 0;
            for (int j = 0; j < len; ++j) {
                char c = at + j < n && rng() % 4 ? t[at + j] : char('a' + rng() % sigma);
                p += rng() % 5 == 0 ? '?' : c;}}
        if (rng() % 2) { p += '*'; }
        checkGlob(t, p);
        if (it % 9 == 0) { checkGlob(t, p, '#', '%'); }}
    for (int k : {63, 64, 65, 127, 128, 129}) {
        string t = randomString(rng, 3 * k, "ab"), p = "*" + t.substr(k / 2, k) + "*";
        p[k / 3 + 1] = '?';
        checkGlob(t, p);
        checkGlob(t, t.substr(0, k));
        checkGlob(t, string(k, '?') + "*");}
    for (int r = 0; r < (mode == "quick" ? 4 : 20); ++r) {
        string t = randomString(rng, 3000 + int(rng() % 3000), "ab"), seg = t.substr(t.size() - 60 - rng() % 200, 45 + rng() % 10);
        seg[rng() % seg.size()] = '?';
        if (r % 3 == 0) { seg[rng() % seg.size()] ^= 3; }
        checkGlob(t, "*" + seg + "*");
        checkGlob(t, t.substr(0, 3) + "*" + seg + "*" + t.substr(t.size() - 2));
        string unary(2000 + rng() % 3000, 'a'), heavy = string(60 + rng() % 40, 'a') + "b";
        unary[unary.size() - 1 - rng() % 50] = 'b';
        heavy[rng() % (heavy.size() - 1)] = '?';
        string wide = string(30 + rng() % 30, 'a');
        for (int j = 0; j < 80; ++j) { wide += rng() % 6 ? char(65 + rng() % 60) : '?'; }
        string host(4000 + rng() % 2000, 'a');
        for (int j = 0; j < int(wide.size()); ++j) { host[host.size() - 300 + j] = wide[j] == '?' ? char(65 + rng() % 60) : wide[j]; }
        bool early = false;
        int wm = int(wide.size()), wn = wildcard_detail::scan(0, int(host.size()), wm, wildcard_detail::BUDGET, [&](int i, int j) { return wide[j] == '?' || wide[j] == host[i]; }, [&](int) { return early = true; });
        expect(!early && wn + wm <= int(host.size()), true, "wide glob segment hands over to the blocks");
        checkGlob(host, "*" + wide + "*");
        int hm = int(heavy.size()), next = wildcard_detail::scan(0, int(unary.size()), hm, wildcard_detail::BUDGET, [&](int i, int j) { return heavy[j] == '?' || heavy[j] == unary[i]; }, [](int) { return true; });
        expect(next + hm <= int(unary.size()), true, "glob segment hands over to the blocks");
        checkGlob(unary, "*" + heavy + "*");
        checkGlob(unary, "a*" + heavy + "*" + heavy);}
    string bytes = "\x80\xff?*\x01";
    checkGlob(bytes, "\x80*\x01");
    checkGlob(bytes, "?\xff\\?*");
    cout << "PASS random glob (KMP, naive and convolution segments, metacharacters, word boundaries) cases=" << cases - before << '\n';}
// Large inputs: early-exit brute on random text, an analytic oracle on periodic text, planted globs.
void large(const string &mode) {
    std::mt19937_64 rng(test_seed + 3);
    int n = mode == "quick" ? 30000 : mode == "full" ? 300000 : 1500000;
    for (int m : {41, 1000, min(30000, n / 4)}) {
        string t = randomString(rng, n, "abcd"), p = t.substr(n / 3, m);
        for (int j = 0; j < m; j += 3) { p[j] = '?'; }
        for (int r = 0; r < 4; ++r) { t[rng() % n] = '?'; }
        context = "large random n=" + std::to_string(n) + " m=" + std::to_string(m);
        auto expected = bruteWildcard(t, p, '?');
        expect(wildcardMatch(t, p, '?', rng()), expected, "wildcardMatch large");
        expect(wildcardMatchDeterministic(t, p, '?'), expected, "wildcardMatchDeterministic large");}
    string periodic(n, 'a'), p(5000, 'a');
    for (int i = 1; i < n; i += 2) { periodic[i] = 'b'; }
    for (int j = 1; j < 5000; j += 2) { p[j] = j % 7 == 0 ? '?' : 'b'; }
    vector<int> even;
    for (int i = 0; i + 5000 <= n; i += 2) { even.push_back(i); }
    context = "large periodic n=" + std::to_string(n);
    expect(wildcardMatch(periodic, p, '?', rng()), even, "wildcardMatch periodic");
    expect(wildcardMatchDeterministic(periodic, p, '?'), even, "wildcardMatchDeterministic periodic");
    string g = randomString(rng, n, "ab"), pattern;
    for (int k = 0; k < 6; ++k) {
        string piece = g.substr(size_t(k) * (n / 6) + 7, 2000);
        piece[100] = '?';
        pattern += "*" + piece;}
    context = "large glob planted n=" + std::to_string(n);
    expect(globMatch(g, pattern + "*"), true, "globMatch planted");
    expect(globMatch(g, pattern + "*c"), false, "globMatch absent symbol");
    GlobAutomaton automaton(pattern + "*");
    bool any = false;
    for (char c : g) { any = automaton.step(c); }
    expect(any, true, "GlobAutomaton planted");
    cout << "PASS large wildcard n=" << n << " (m=41/1000/min(30000, n/4), periodic) and planted glob\n";}
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
        string longer((1 << 22) + 1, 'a');
        if (invalid == "pattern-limit") { wildcardMatch(longer, longer, '?'); }
        if (invalid == "deterministic-limit") { wildcardMatchDeterministic(longer, longer, '?'); }
        if (invalid == "glob-limit") { longer[5] = '?'; globMatch(longer + longer, "*" + longer + "*"); }
        return 0;}
    exhaustive(mode);
    randomWildcard(mode);
    randomGlob(mode);
    large(mode);
    cout << "PASS wildcardmatching seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
