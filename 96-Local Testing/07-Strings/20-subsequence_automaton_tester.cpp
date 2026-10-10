#include "../../07-Strings/20-subsequence_automaton.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A, class B> void expect(const A &actual, const B &expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=";
    if constexpr (!requires { expected.begin(); }) { cerr << expected << " actual=" << actual; }
    else {
        for (auto x : expected) { cerr << x << ','; }
        cerr << " actual=";
        for (auto x : actual) { cerr << x << ','; }}
    cerr << '\n';
    std::exit(1);}
void check128(bool ok, const string &op) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=true actual=false\n";
    std::exit(1);}
// Ring oracle type: integers modulo a small prime so that counts wrap.
struct Mod {
    static constexpr lng P = 1000003;
    lng v = 0;
    Mod(lng x = 0) : v((x % P + P) % P) {}
    Mod operator+(Mod o) const { return Mod(v + o.v); }
    Mod operator-(Mod o) const { return Mod(v - o.v); }
    bool operator==(const Mod &) const = default;
    friend std::ostream &operator<<(std::ostream &os, Mod x) { return os << x.v; }
};
lng sat(lll x) { return lng(min<lll>(x, std::numeric_limits<lng>::max())); }
// Independent references from a brute next table: sum recurrence counts, O(n * S) absent DP, O(n * m) occurrence DP.
struct Reference {
    int n, sigma;
    vector<int> s;
    vector<vector<int>> nx, pv;
    vector<lng> cnt;
    Reference(const vector<int> &text, int size) : n(int(text.size())), sigma(size), s(text), nx(n + 1, vector<int>(size, n)), pv(n + 1, vector<int>(size, -1)) {
        if (lng(n) * n * sigma > 30000000) {
            for (int i = n - 1; i >= 0; --i) { nx[i] = nx[i + 1]; nx[i][s[i]] = i; }
            for (int i = 0; i < n; ++i) { pv[i + 1] = pv[i]; pv[i + 1][s[i]] = i; }
            cnt = counts<lng>(std::numeric_limits<lng>::max());
            return;}
        for (int i = 0; i <= n; ++i) {
            for (int c = 0; c < sigma; ++c) {
                for (int j = i; j < n; ++j) {
                    if (s[j] == c) { nx[i][c] = j; break; }}
                for (int j = i - 1; j >= 0; --j) {
                    if (s[j] == c) { pv[i][c] = j; break; }}}}
        cnt = counts<lng>(std::numeric_limits<lng>::max());}
    template<class T> vector<T> counts(T cap) const {
        vector<T> res(n + 1);
        for (int i = n; i >= 0; --i) {
            lll x = 1;
            for (int c = 0; c < sigma; ++c) {
                if (nx[i][c] < n) { x = min<lll>(x + res[nx[i][c] + 1], cap); }}
            res[i] = T(x);}
        return res;}
    vector<Mod> countsMod() const {
        vector<Mod> res(n + 1);
        for (int i = n; i >= 0; --i) {
            res[i] = Mod(1);
            for (int c = 0; c < sigma; ++c) {
                if (nx[i][c] < n) { res[i] = res[i] + res[nx[i][c] + 1]; }}}
        return res;}
    bool kth(lng k, vector<int> &res) const {
        res.clear();
        if (k < 0 || k >= cnt[0]) { return false; }
        int i = 0;
        while (k) {
            --k;
            for (int c = 0; c < sigma; ++c) {
                if (nx[i][c] == n) { continue; }
                if (k < cnt[nx[i][c] + 1]) { res.push_back(c); i = nx[i][c] + 1; break; }
                k -= cnt[nx[i][c] + 1];}}
        return true;}
    vector<int> absent() const {
        vector<int> len(n + 2), res;
        for (int i = n; i >= 0; --i) {
            len[i] = INT_MAX;
            for (int c = 0; c < sigma; ++c) { len[i] = min(len[i], 1 + len[nx[i][c] + 1]); }}
        for (int i = 0; i <= n;) {
            for (int c = 0; c < sigma; ++c) {
                if (1 + len[nx[i][c] + 1] == len[i]) { res.push_back(c); i = nx[i][c] + 1; break; }}}
        return res;}
    template<class T> T occurrences(const vector<int> &t) const {
        int m = int(t.size());
        vector<T> dp(m + 1, T(0));
        dp[0] = T(1);
        for (int i = 0; i < n; ++i) {
            for (int j = m - 1; j >= 0; --j) {
                if (t[j] != s[i]) { continue; }
                if constexpr (std::is_integral_v<T>) { dp[j + 1] = T(min<lll>(lll(dp[j + 1]) + dp[j], std::numeric_limits<T>::max())); }
                else { dp[j + 1] = dp[j + 1] + dp[j]; }}}
        return dp[m];}
};
// Brute: every subsequence of s[l, r) by bitmask.
std::set<vector<int>> subsequences(const vector<int> &s, int l, int r) {
    std::set<vector<int>> res;
    for (int mask = 0; mask < 1 << (r - l); ++mask) {
        vector<int> x;
        for (int i = 0; i < r - l; ++i) {
            if (mask >> i & 1) { x.push_back(s[l + i]); }}
        res.insert(x);}
    return res;}
lng bruteOccurrences(const vector<int> &s, const vector<int> &t) {
    int n = int(s.size());
    lng res = 0;
    for (int mask = 0; mask < 1 << n; ++mask) {
        if (std::popcount(uint(mask)) != int(t.size())) { continue; }
        vector<int> x;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) { x.push_back(s[i]); }}
        res += x == t;}
    return res;}
template<class A> void verifyCommon(const A &a, const Reference &ref, std::mt19937_64 &rng) {
    int n = ref.n, sigma = ref.sigma;
    for (int i = 0; i <= n; ++i) {
        for (int c = 0; c < min(sigma, 260); ++c) {
            expect(a.next(i, c), ref.nx[i][c], "next i=" + std::to_string(i));
            expect(a.prev(i, c), ref.pv[i][c], "prev i=" + std::to_string(i));}}
    for (int state = 0; state <= n + 1; ++state) {
        for (int c : {-1, 0, sigma - 1, sigma}) {
            int expected = state > n || c < 0 || c >= sigma ? n + 1 : ref.nx[state][c] + 1;
            expect(a.step(state, c), expected, "step state=" + std::to_string(state) + " c=" + std::to_string(c));}}
    auto cnt = ref.counts<lng>(std::numeric_limits<lng>::max());
    expect(a.countDistinctSubsequences(), cnt, "countDistinctSubsequences lng");
    expect(a.template countDistinctSubsequences<int>(), ref.counts<int>(INT_MAX), "countDistinctSubsequences int saturated");
    expect(a.template countDistinctSubsequences<Mod>(), ref.countsMod(), "countDistinctSubsequences ring");
    expect(a.shortestAbsentSubsequence(), ref.absent(), "shortestAbsentSubsequence");
    vector<lng> ks{-1, 0, 1, 2, cnt[0] - 1, cnt[0], std::numeric_limits<lng>::max() - 1, std::numeric_limits<lng>::max()};
    for (int q = 0; q < 20; ++q) { ks.push_back(lng(rng() % ulng(min<lng>(cnt[0], lng(1) << 62)))); }
    for (lng k : ks) {
        vector<int> got, expected;
        bool ok = ref.kth(k, expected);
        expect(a.kthDistinctSubsequence(k, got), ok, "kthDistinctSubsequence status k=" + std::to_string(k));
        expect(got, expected, "kthDistinctSubsequence k=" + std::to_string(k));}
    for (int q = 0; q < 20; ++q) {
        int m = int(rng() % 6), l = int(rng() % (n + 1)), r = l + int(rng() % (n - l + 1));
        vector<int> t(m);
        for (int &c : t) { c = int(rng() % (sigma + 1)) - (rng() % 9 == 0); }
        int state = l;
        for (int c : t) { state = state > n || c < 0 || c >= sigma ? n + 1 : ref.nx[state][c] + 1; }
        expect(a.isSubsequence(t, l, r), state <= r, "isSubsequence range");
        bool coded = std::all_of(t.begin(), t.end(), [&](int c) { return 0 <= c && c < sigma; });
        expect(a.countSubsequenceOccurrences(t), coded ? ref.occurrences<lng>(t) : 0, "countSubsequenceOccurrences");
        expect(a.template countSubsequenceOccurrences<int>(t), coded ? ref.occurrences<int>(t) : 0, "countSubsequenceOccurrences int");
        expect(a.template countSubsequenceOccurrences<Mod>(t), coded ? ref.occurrences<Mod>(t) : Mod(0), "countSubsequenceOccurrences ring");}}
template<class A> void verifySmall(const A &a, const vector<int> &s, int sigma) {
    int n = int(s.size());
    auto all = subsequences(s, 0, n);
    vector<vector<int>> sorted(all.begin(), all.end());
    for (lng k = -1; k <= lng(sorted.size()); ++k) {
        vector<int> got;
        bool ok = a.kthDistinctSubsequence(k, got);
        expect(ok, 0 <= k && k < lng(sorted.size()), "brute kth status");
        if (ok) { expect(got, sorted[k], "brute kth k=" + std::to_string(k)); }}
    auto cnt = a.countDistinctSubsequences();
    for (int i = 0; i <= n; ++i) { expect(cnt[i], lng(subsequences(s, i, n).size()), "brute distinct count i=" + std::to_string(i)); }
    vector<int> absent;
    for (int len = 1; absent.empty(); ++len) {
        vector<int> x(len, 0);
        while (true) {
            if (!all.count(x)) { absent = x; break; }
            int i = len - 1;
            while (i >= 0 && x[i] == sigma - 1) { x[i--] = 0; }
            if (i < 0) { break; }
            ++x[i];}}
    expect(a.shortestAbsentSubsequence(), absent, "brute shortestAbsentSubsequence");
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) {
            auto range = subsequences(s, l, r);
            for (const auto &x : sorted) { expect(a.isSubsequence(x, l, r), range.count(x) > 0, "brute isSubsequence range"); }}}
    for (const auto &x : sorted) {
        if (x.size() <= 4) { expect(a.countSubsequenceOccurrences(x), bruteOccurrences(s, x), "brute countSubsequenceOccurrences"); }}}
void run(const vector<int> &s, int sigma, bool small, std::mt19937_64 &rng) {
    ++cases;
    Reference ref(s, sigma);
    SubsequenceAutomaton sparse(s, sigma);
    verifyCommon(sparse, ref, rng);
    if (small) { verifySmall(sparse, s, sigma); }
    if (lng(s.size() + 1) * sigma <= 2000000) {
        SubsequenceAutomatonDense dense(s, sigma);
        verifyCommon(dense, ref, rng);
        if (small) { verifySmall(dense, s, sigma); }}}
void exhaustive(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int bound = mode == "quick" ? 6 : mode == "full" ? 7 : 8;
    for (int sigma = 1; sigma <= 3; ++sigma) {
        for (int n = 0, count = 1; n <= bound - (sigma == 3); ++n, count *= sigma) {
            for (int code = 0; code < count; ++code) {
                vector<int> s(n);
                for (int x = code, i = 0; i < n; ++i, x /= sigma) { s[i] = x % sigma; }
                context = "exhaustive sigma=" + std::to_string(sigma) + " n=" + std::to_string(n) + " code=" + std::to_string(code);
                run(s, sigma, true, rng);}}}
    cout << "PASS exhaustive alphabets 1..3 lengths<=" << bound << " against brute subsequence sets\n";}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed + 1);
    int count = mode == "quick" ? 200 : mode == "full" ? 1000 : 2000;
    for (int rep = 0; rep < count; ++rep) {
        int sigma = rep % 6 == 0 ? 256 : int(rng() % 5) + 1, n = int(rng() % (rep % 4 == 0 ? 400 : 40));
        vector<int> s(n);
        for (int &c : s) { c = int(rng() % min(sigma, 5)) * (sigma == 256 ? 63 : 1); }
        context = "random case=" + std::to_string(rep) + " sigma=" + std::to_string(sigma) + " n=" + std::to_string(n);
        run(s, sigma, false, rng);
        if (sigma == 256) {
            string bytes(n, '\0');
            for (int i = 0; i < n; ++i) { bytes[i] = char(s[i]); }
            SubsequenceAutomaton a(bytes);
            expect(a.text, s, "byte text codes");
            string t = n ? bytes.substr(0, 1) + bytes.substr(n - 1) : string("\xff");
            expect(a.isSubsequence(t), n >= 2, "byte isSubsequence");}}
    cout << "PASS random alphabets 1..5 and 256 lengths<400 cases=" << count << '\n';}
void large(const string &mode) {
    std::mt19937_64 rng(test_seed + 2);
    int n = mode == "quick" ? 20000 : 200000;
    vector<int> s(n);
    for (int &c : s) { c = int(rng() % 3); }
    context = "large ternary n=" + std::to_string(n);
    Reference ref(vector<int>(s.begin(), s.begin() + 3000), 3);
    SubsequenceAutomaton prefix(vector<int>(s.begin(), s.begin() + 3000), 3);
    expect(prefix.shortestAbsentSubsequence(), ref.absent(), "large absent");
    expect(prefix.countDistinctSubsequences(), ref.counts<lng>(std::numeric_limits<lng>::max()), "large saturated counts");
    SubsequenceAutomaton a(s, 3);
    SubsequenceAutomatonDense d(s, 3);
    auto absent = a.shortestAbsentSubsequence();
    expect(d.shortestAbsentSubsequence(), absent, "large absent dense");
    expect(a.isSubsequence(absent), false, "large absent is absent");
    absent.pop_back();
    expect(a.isSubsequence(absent), true, "large absent prefix present");
    expect(a.template countDistinctSubsequences<Mod>(), d.template countDistinctSubsequences<Mod>(), "large ring counts sparse versus dense");
    vector<int> wide(n);
    for (int &c : wide) { c = int(rng() % 1000000); }
    SubsequenceAutomaton w(wide, 1000000);
    vector<char> seen(1000000);
    for (int c : wide) { seen[c] = 1; }
    expect(w.shortestAbsentSubsequence(), vector<int>{int(std::find(seen.begin(), seen.end(), 0) - seen.begin())}, "wide alphabet absent");
    // 200 distinct symbols: res[i] = 2^(200 - i) exactly until it exceeds the 128-bit cap.
    vector<int> distinct(200);
    iota(distinct.begin(), distinct.end(), 0);
    SubsequenceAutomaton z(distinct, 200);
    auto u = z.template countDistinctSubsequences<ulll>();
    auto v = z.template countDistinctSubsequences<lll>();
    context = "128-bit saturation";
    for (int i = 0; i <= 200; ++i) {
        int e = 200 - i;
        check128(u[i] == (e < 128 ? ulll(1) << e : ~ulll(0)), "countDistinctSubsequences unsigned __int128 i=" + std::to_string(i));
        check128(v[i] == (e < 127 ? lll(1) << e : std::numeric_limits<lll>::max()), "countDistinctSubsequences __int128 i=" + std::to_string(i));}
    cout << "PASS large ternary/wide-alphabet n=" << n << ", 128-bit saturation\n";}
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
        if (invalid == "build-symbol") { SubsequenceAutomaton(vector<int>{0, 3}, 3); }
        if (invalid == "next-index") { SubsequenceAutomatonDense(vector<int>{0, 1}, 2).next(3, 0); }
        if (invalid == "prev-symbol") { SubsequenceAutomaton(vector<int>{0, 1}, 2).prev(1, 2); }
        if (invalid == "range") { SubsequenceAutomaton(vector<int>{0, 1}, 2).isSubsequence(vector<int>{0}, 2, 1); }
        if (invalid == "range-negative") { SubsequenceAutomaton(vector<int>{0, 1}, 2).isSubsequence(vector<int>{0}, 0, -5); }
        if (invalid == "step-state") { SubsequenceAutomaton(vector<int>{0, 1}, 2).step(4, 0); }
        return 0;}
    exhaustive(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS subsequence_automaton seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
