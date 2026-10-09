#include "../../07-Strings/18-editdistance.hpp"

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
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op
         << " expected=" << expected << " actual=" << actual << '\n';
    std::exit(1);}
ulng rng_state;
ulng nextRandom() {
    rng_state += 0x9e3779b97f4a7c15ULL;
    ulng z = rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);}
lng randomIn(lng lo, lng hi) { return lo + lng(nextRandom() % ulng(hi - lo + 1)); }
using Columns = vector<pair<int, int>>;
struct Scoring {
    array<array<lng, 4>, 4> table;
    lng gap, open, extend, ins, del, sub;
    lng operator()(char x, char y) const { return table[(x - 'a') & 3][(y - 'a') & 3]; }
};

// Oracle 1: every alignment (column path from (0, 0) to (n, m)) by explicit enumeration.
template<class F> void alignments(int n, int m, F &&visit) {
    Columns cols;
    auto go = [&](auto &&self, int i, int j) -> void {
        if (i == n && j == m) { visit(cols); return; }
        if (i < n && j < m) { cols.push_back({i, j}); self(self, i + 1, j + 1); cols.pop_back(); }
        if (i < n) { cols.push_back({i, -1}); self(self, i + 1, j); cols.pop_back(); }
        if (j < m) { cols.push_back({-1, j}); self(self, i, j + 1); cols.pop_back(); }};
    go(go, 0, 0);}
bool validColumns(const Columns &cols, int n, int m) {
    int i = 0, j = 0;
    for (auto [x, y] : cols) {
        if (x < 0 && y < 0) { return false; }
        if (x >= 0 && x != i++) { return false; }
        if (y >= 0 && y != j++) { return false; }}
    return i == n && j == m;}
template<class S> lng unitCost(const S &a, const S &b, const Columns &cols) {
    lng res = 0;
    for (auto [x, y] : cols) { res += x < 0 || y < 0 || !(a[x] == b[y]); }
    return res;}
struct Brute {
    lng lev = LLONG_MAX, weighted = LLONG_MAX, nw = LLONG_MIN, semi = LLONG_MIN, affine = LLONG_MIN;
    vector<lng> banded;
};
Brute brute(const string &a, const string &b, const Scoring &sc) {
    int n = int(a.size()), m = int(b.size());
    Brute res;
    res.banded.assign(n + m + 2, LLONG_MAX);
    alignments(n, m, [&](const Columns &cols) {
        lng unit = unitCost(a, b, cols), w = 0, nw = 0, semi = 0, affine = 0;
        int i = 0, j = 0, spread = 0, run = 0;
        for (auto [x, y] : cols) {
            bool gap_a = x < 0, gap_b = y < 0;
            if (gap_a) { w += sc.ins; nw += sc.gap; semi += i == 0 || i == n ? 0 : sc.gap; }
            else if (gap_b) { w += sc.del; nw += sc.gap; semi += j == 0 || j == m ? 0 : sc.gap; }
            else { w += a[x] == b[y] ? 0 : sc.sub; nw += sc(a[x], b[y]); semi += sc(a[x], b[y]); affine += sc(a[x], b[y]); }
            int type = gap_a ? 1 : gap_b ? 2 : 0;
            if (type) { affine += (type != run ? sc.open : 0) + sc.extend; }
            run = type;
            i += x >= 0;
            j += y >= 0;
            spread = max(spread, abs(i - j));}
        res.lev = min(res.lev, unit);
        res.weighted = min(res.weighted, w);
        res.nw = max(res.nw, nw);
        res.semi = max(res.semi, semi);
        res.affine = max(res.affine, affine);
        for (int band = spread; band < int(res.banded.size()); ++band) { res.banded[band] = min(res.banded[band], unit); }});
    return res;}
// Oracle 2: shortest edit sequences over strings (insert, delete, substitute, optionally adjacent transposition) by BFS.
map<string, int> bfs(const string &source, const string &alphabet, int max_len, bool transpose) {
    map<string, int> dist{{source, 0}};
    deque<string> q{source};
    while (!q.empty()) {
        string u = q.front();
        q.pop_front();
        int d = dist[u];
        vector<string> next;
        for (int i = 0; i <= int(u.size()); ++i) {
            for (char c : alphabet) {
                if (int(u.size()) < max_len) { next.push_back(u.substr(0, i) + c + u.substr(i)); }
                if (i < int(u.size())) { string v = u; v[i] = c; next.push_back(v); }}
            if (i < int(u.size())) { next.push_back(u.substr(0, i) + u.substr(i + 1)); }
            if (transpose && i + 1 < int(u.size())) { string v = u; swap(v[i], v[i + 1]); next.push_back(v); }}
        for (auto &v : next) {
            if (dist.emplace(v, d + 1).second) { q.push_back(v); }}}
    return dist;}
// Oracle 3: optimal string alignment by its suffix-form recursion (transposed pairs are never edited again).
int osaRecursive(const string &a, const string &b) {
    int n = int(a.size()), m = int(b.size());
    vector<vector<int>> memo(n + 1, vector<int>(m + 1, -1));
    auto f = [&](auto &&self, int i, int j) -> int {
        if (i == n || j == m) { return n - i + m - j; }
        int &res = memo[i][j];
        if (res >= 0) { return res; }
        res = min({self(self, i + 1, j) + 1, self(self, i, j + 1) + 1, self(self, i + 1, j + 1) + (a[i] != b[j])});
        if (i + 1 < n && j + 1 < m && a[i] == b[j + 1] && a[i + 1] == b[j]) { res = min(res, self(self, i + 2, j + 2) + 1); }
        return res;};
    return f(f, 0, 0);}
// Oracle 4: textbook prefix-table DPs for larger inputs (themselves validated against oracles 1 and 2).
template<class S> int tableLevenshtein(const S &a, const S &b) {
    int n = int(a.size()), m = int(b.size());
    vector<vector<int>> d(n + 1, vector<int>(m + 1));
    for (int i = 0; i <= n; ++i) {
        for (int j = 0; j <= m; ++j) { d[i][j] = !i || !j ? i + j : min({d[i - 1][j] + 1, d[i][j - 1] + 1, d[i - 1][j - 1] + !(a[i - 1] == b[j - 1])}); }}
    return d[n][m];}
lng tableNw(const string &a, const string &b, const Scoring &sc) {
    int n = int(a.size()), m = int(b.size());
    vector<vector<lng>> d(n + 1, vector<lng>(m + 1));
    for (int i = 0; i <= n; ++i) {
        for (int j = 0; j <= m; ++j) {
            if (!i || !j) { d[i][j] = sc.gap * (i + j); }
            else { d[i][j] = max({d[i - 1][j] + sc.gap, d[i][j - 1] + sc.gap, d[i - 1][j - 1] + sc(a[i - 1], b[j - 1])}); }}}
    return d[n][m];}
template<class S> void checkUnitWitness(const S &a, const S &b, const pair<int, Columns> &w, int distance, const string &op) {
    check(validColumns(w.second, int(a.size()), int(b.size())), op + " columns valid");
    expectEqual(w.first, distance, op + " distance");
    expectEqual(unitCost(a, b, w.second), distance, op + " columns cost");}
// Unit-cost operations that must all equal the Levenshtein distance.
template<class S> void unitFamily(const S &a, const S &b, int expected, bool witness = true) {
    int n = int(a.size()), m = int(b.size());
    expectEqual(levenshtein(a, b), expected, "levenshtein");
    expectEqual(myersBitVector(a, b), expected, "myersBitVector");
    expectEqual(diagonalEditDistance(a, b), expected, "diagonalEditDistance");
    expectEqual(bandedEditDistance(a, b, max(n, m)), expected, "bandedEditDistance full band");
    expectEqual(bandedEditDistance(a, b, expected), expected, "bandedEditDistance band = distance");
    for (int k : {0, expected - 1, expected, expected + 1, n + m}) {
        if (k >= 0) { expectEqual(thresholdEditDistance(a, b, k), min(expected, k + 1), "thresholdEditDistance k=" + std::to_string(k)); }}
    expectEqual(weightedEditDistance(a, b, 1, 1, 1), expected, "weightedEditDistance unit costs");
    if (witness) {
        checkUnitWitness(a, b, levenshteinWitness(a, b), expected, "levenshteinWitness");
        checkUnitWitness(a, b, hirschberg(a, b), expected, "hirschberg unit");}}
void verifySmall(const string &a, const string &b, const Scoring &sc, const map<string, int> *lev_bfs, const map<string, int> &dl_bfs) {
    ++cases;
    context = "a=\"" + a + "\" b=\"" + b + "\" gap=" + std::to_string(sc.gap) + " open=" + std::to_string(sc.open) + " extend=" + std::to_string(sc.extend);
    int n = int(a.size()), m = int(b.size());
    Brute br = brute(a, b, sc);
    if (lev_bfs) { expectEqual(br.lev, lev_bfs->at(b), "alignment oracle versus BFS oracle"); }
    unitFamily(a, b, int(br.lev));
    unitFamily(string_view(a), string_view(b), int(br.lev));
    unitFamily(vector<int>(a.begin(), a.end()), vector<int>(b.begin(), b.end()), int(br.lev));
    for (int w = 0; w <= n + m; ++w) { expectEqual(bandedEditDistance(a, b, w), br.banded[w] == LLONG_MAX ? -1 : br.banded[w], "bandedEditDistance w=" + std::to_string(w)); }
    expectEqual(weightedEditDistance(a, b, sc.ins, sc.del, sc.sub), br.weighted, "weightedEditDistance");
    expectEqual(optimalStringAlignment(a, b), osaRecursive(a, b), "optimalStringAlignment");
    expectEqual(damerauLevenshtein(a, b), dl_bfs.at(b), "damerauLevenshtein versus BFS with transpositions");
    expectEqual(damerauLevenshtein(vector<lng>(a.begin(), a.end()), vector<lng>(b.begin(), b.end())), dl_bfs.at(b), "damerauLevenshtein non-byte symbols");
    check(damerauLevenshtein(a, b) <= optimalStringAlignment(a, b) && optimalStringAlignment(a, b) <= levenshtein(a, b), "DL <= OSA <= Levenshtein");
    expectEqual(needlemanWunsch(a, b, sc, sc.gap), br.nw, "needlemanWunsch");
    auto [score, cols] = hirschberg(a, b, sc, sc.gap);
    expectEqual(score, br.nw, "hirschberg score");
    check(validColumns(cols, n, m), "hirschberg columns valid");
    lng total = 0;
    for (auto [x, y] : cols) { total += x < 0 || y < 0 ? sc.gap : sc(a[x], b[y]); }
    expectEqual(total, score, "hirschberg columns score");
    expectEqual(gotoh(a, b, sc, sc.open, sc.extend), br.affine, "gotoh");
    Scoring local = sc;
    local.gap = min<lng>(sc.gap, 0);
    expectEqual(semiGlobalAlignment(a, b, local, local.gap), local.gap == sc.gap ? br.semi : brute(a, b, local).semi, "semiGlobalAlignment");
    lng best = 0;
    pair<int, int> end{0, 0};
    for (int ra = 0; ra <= n; ++ra) {
        for (int rb = 0; rb <= m; ++rb) {
            lng here = 0;
            for (int la = 0; la <= ra; ++la) {
                for (int lb = 0; lb <= rb; ++lb) { here = max(here, tableNw(a.substr(la, ra - la), b.substr(lb, rb - lb), local)); }}
            if (here > best) { best = here; end = {ra, rb}; }}}
    auto [sw, la, ra, lb, rb] = smithWaterman(a, b, local, local.gap);
    expectEqual(sw, best, "smithWaterman score");
    if (best > 0) {
        check(pair<int, int>{ra, rb} == end, "smithWaterman first row-major end");
        check(0 <= la && la <= ra && 0 <= lb && lb <= rb, "smithWaterman region bounds");
        expectEqual(tableNw(a.substr(la, ra - la), b.substr(lb, rb - lb), local), sw, "smithWaterman region is optimal");}
    else { check(tuple<lng, int, int, int, int>{sw, la, ra, lb, rb} == tuple<lng, int, int, int, int>{0, 0, 0, 0, 0}, "smithWaterman empty"); }}
Scoring fixedScoring() {
    Scoring sc{};
    for (int x = 0; x < 4; ++x) {
        for (int y = 0; y < 4; ++y) { sc.table[x][y] = x == y ? 2 : -1; }}
    sc.gap = -2; sc.open = -3; sc.extend = -1; sc.ins = 2; sc.del = 3; sc.sub = 4;
    return sc;}
Scoring randomScoring() {
    Scoring sc{};
    for (auto &row : sc.table) {
        for (lng &v : row) { v = randomIn(-3, 3); }}
    sc.gap = randomIn(-3, 1); sc.open = randomIn(-4, 1); sc.extend = randomIn(-2, 1);
    sc.ins = randomIn(0, 4); sc.del = randomIn(0, 4); sc.sub = randomIn(0, 4);
    return sc;}
vector<string> wordsUpTo(int limit, const string &alphabet) {
    vector<string> res{""};
    for (int i = 0; i < int(res.size()); ++i) {
        if (int(res[i].size()) < limit) {
            for (char c : alphabet) { res.push_back(res[i] + c); }}}
    return res;}
void exhaustive(const string &mode) {
    int limit = mode == "quick" ? 3 : 4;
    auto words = wordsUpTo(limit, "abc");
    Scoring sc = fixedScoring();
    for (auto &a : words) {
        auto lev = bfs(a, "abc", limit + 2, false), dl = bfs(a, "abc", limit + 2, true);
        for (auto &b : words) { verifySmall(a, b, sc, &lev, dl); }}
    cout << "PASS exhaustive ternary pairs through length " << limit << " (" << words.size() * words.size() << " pairs)\n";}
string randomString(int n, int sigma) {
    string res(n, 'a');
    for (char &c : res) { c = char('a' + nextRandom() % sigma); }
    return res;}
void randomCases(const string &mode) {
    int rounds = mode == "quick" ? 150 : mode == "full" ? 1500 : 2000;
    for (int r = 0; r < rounds; ++r) {
        int sigma = 1 + int(nextRandom() % 4);
        string a = randomString(int(nextRandom() % 6), sigma), b = randomString(int(nextRandom() % 6), sigma);
        if (r % 4 == 0) { b = a; if (!b.empty()) { b[nextRandom() % b.size()] = 'a'; } }
        string alphabet = string("abcd").substr(0, sigma);
        int cap = int(max(a.size(), b.size())) + 1;
        auto lev = r % 4 ? map<string, int>() : bfs(a, alphabet, cap, false);
        verifySmall(a, b, randomScoring(), r % 4 ? nullptr : &lev, bfs(a, alphabet, cap, true));}
    cout << "PASS random small pairs with random scoring rounds=" << rounds << '\n';}
void medium(const string &mode) {
    int rounds = mode == "quick" ? 40 : mode == "full" ? 300 : 450;
    vector<int> lengths{0, 1, 63, 64, 65, 127, 128, 129, 191, 192, 193, 300};
    for (int r = 0; r < rounds; ++r) {
        ++cases;
        int n = r < 144 ? lengths[r % 12] : int(nextRandom() % 400), m = r < 144 ? lengths[r / 12 % 12] : int(nextRandom() % 400);
        int sigma = 1 + int(nextRandom() % 5);
        string a = randomString(n, sigma), b = randomString(m, sigma);
        if (r % 3 == 0 && n) { b = a; for (int k = 0; k < 5 && !b.empty(); ++k) { b[nextRandom() % b.size()] = char('a' + nextRandom() % 5); } }
        context = "medium n=" + std::to_string(a.size()) + " m=" + std::to_string(b.size()) + " sigma=" + std::to_string(sigma);
        int expected = tableLevenshtein(a, b);
        unitFamily(a, b, expected);
        vector<int> x(a.size()), y(b.size());
        for (int i = 0; i < int(a.size()); ++i) { x[i] = (a[i] - 'a') * 1000003 - 7; }
        for (int j = 0; j < int(b.size()); ++j) { y[j] = (b[j] - 'a') * 1000003 - 7 + (j % 11 == 5 ? 1 : 0); }
        expectEqual(myersBitVector(x, y), tableLevenshtein(x, y), "myersBitVector wide integer symbols");
        vector<int> y0(b.size());
        for (int j = 0; j < int(b.size()); ++j) { y0[j] = (b[j] - 'a') * 1000003 - 7; }
        expectEqual(damerauLevenshtein(x, y0), damerauLevenshtein(a, b), "damerauLevenshtein invariant under symbol relabeling");
        expectEqual(optimalStringAlignment(x, y0), optimalStringAlignment(a, b), "optimalStringAlignment invariant under symbol relabeling");
        check(damerauLevenshtein(x, y) <= optimalStringAlignment(x, y) && optimalStringAlignment(x, y) <= tableLevenshtein(x, y), "medium DL <= OSA <= Levenshtein");
        Scoring sc = randomScoring();
        expectEqual(needlemanWunsch(a, b, sc, sc.gap), tableNw(a, b, sc), "needlemanWunsch medium");
        auto [score, cols] = hirschberg(a, b, sc, sc.gap);
        expectEqual(score, tableNw(a, b, sc), "hirschberg medium score");
        check(validColumns(cols, int(a.size()), int(b.size())), "hirschberg medium columns");}
    string bytes_a, bytes_b;
    for (int c = 0; c < 256; ++c) { bytes_a.push_back(char(c)); bytes_b.push_back(char(255 - c)); }
    context = "full byte alphabet";
    unitFamily(bytes_a, bytes_b, tableLevenshtein(bytes_a, bytes_b));
    vector<char> sa(bytes_a.begin(), bytes_a.end()), sb(bytes_b.begin(), bytes_b.end());
    unitFamily(sa, sb, tableLevenshtein(bytes_a, bytes_b));
    vector<lng> wa{LLONG_MIN, 0, LLONG_MAX, -1, LLONG_MIN}, wb{LLONG_MAX, LLONG_MIN, 0, -1};
    context = "lng extremes";
    unitFamily(wa, wb, tableLevenshtein(wa, wb));
    expectEqual(damerauLevenshtein(vector<int>{1, 2}, vector<int>{2, 1}), 1, "damerauLevenshtein transposition of absent-from-b symbols");
    expectEqual(damerauLevenshtein(string("ca"), string("abc")), 2, "damerauLevenshtein ca->abc");
    expectEqual(optimalStringAlignment(string("ca"), string("abc")), 3, "optimalStringAlignment ca->abc");
    cout << "PASS medium random/multiword-boundary pairs rounds=" << rounds << ", byte and lng extremes\n";}
void large(const string &mode) {
    int n = mode == "quick" ? 600 : mode == "full" ? 3000 : 5000;
    vector<pair<string, string>> pairs;
    pairs.push_back({randomString(n, 4), randomString(n, 4)});
    string base = randomString(n, 26), edited = base;
    for (int k = 0; k < 20; ++k) { edited[nextRandom() % n] = char('a' + nextRandom() % 26); }
    pairs.push_back({base, edited});
    pairs.push_back({string(n, 'a'), string(n / 2, 'a') + 'b'});
    pairs.push_back({randomString(50 * n, 3), randomString(5, 3)});
    pairs.push_back({string(n / 16, 'x'), string(10 * n, 'y')});
    for (auto &[a, b] : pairs) {
        ++cases;
        context = "large n=" + std::to_string(a.size()) + " m=" + std::to_string(b.size());
        int expected = tableLevenshtein(a, b);
        expectEqual(levenshtein(a, b), expected, "large levenshtein");
        expectEqual(myersBitVector(a, b), expected, "large myersBitVector");
        expectEqual(myersBitVector(b, a), expected, "large myersBitVector swapped");
        expectEqual(diagonalEditDistance(a, b), expected, "large diagonalEditDistance");
        expectEqual(thresholdEditDistance(a, b, 25), min(expected, 26), "large thresholdEditDistance");
        expectEqual(bandedEditDistance(a, b, int(max(a.size(), b.size()))), expected, "large bandedEditDistance");
        if (a.size() * b.size() <= 10000000ULL) { checkUnitWitness(a, b, hirschberg(a, b), expected, "large hirschberg"); }}
    cout << "PASS large random/near-equal/unary/unbalanced n=" << n << '\n';}
int invalid(const string &probe) {
    string a = "ab", b = "ba";
    if (probe == "band-negative") { bandedEditDistance(a, b, -1); }
    if (probe == "threshold-negative") { thresholdEditDistance(a, b, -1); }
    if (probe == "weighted-negative") { weightedEditDistance(a, b, 1, -1, 1); }
    if (probe == "semi-positive-gap") { semiGlobalAlignment(a, b, [](char x, char y) { return lng(x == y); }, 1); }
    if (probe == "local-positive-gap") { smithWaterman(a, b, [](char x, char y) { return lng(x == y); }, 1); }
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
    medium(mode);
    large(mode);
    cout << "PASS editdistance seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
