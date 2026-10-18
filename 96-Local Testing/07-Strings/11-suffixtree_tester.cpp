#include "../../07-Strings/11-suffixtree.hpp"

ulng test_seed = 0, rng_state = 0;
lng checks = 0, cases = 0, at_v = -1, at_a = 0, at_b = 0;
string context;
void fail(const char *op) {
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op
         << " node=" << at_v << " a=" << at_a << " b=" << at_b;}
void check(bool ok, const char *op) {
    ++checks;
    if (ok) { return; }
    fail(op);
    cerr << " expected=true actual=false\n";
    std::exit(1);}
void expectEqual(lll actual, lll expected, const char *op) {
    ++checks;
    if (actual == expected) { return; }
    fail(op);
    cerr << " expected=" << lng(expected) << " actual=" << lng(actual) << '\n';
    std::exit(1);}
void expectPair(pair<int, int> actual, pair<int, int> expected, const char *op) {
    ++checks;
    if (actual == expected) { return; }
    fail(op);
    cerr << " expected={" << expected.first << "," << expected.second << "} actual={" << actual.first << "," << actual.second << "}\n";
    std::exit(1);}
ulng nextRandom() {
    rng_state += 0x9e3779b97f4a7c15ULL;
    ulng z = rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);}
template<class T> string show(const vector<T> &s) {
    string res = "[";
    for (int i = 0; i < int(s.size()); ++i) { res += (i ? "," : "") + std::to_string(lng(s[i])); }
    return res + "]";}
template<class T> vector<T> randomText(int n, const vector<T> &alphabet) {
    vector<T> res;
    for (int i = 0; i < n; ++i) { res.push_back(alphabet[nextRandom() % alphabet.size()]); }
    return res;}
template<class T> bool hasPrefix(const vector<T> &x, const vector<T> &w) { return x.size() >= w.size() && std::equal(w.begin(), w.end(), x.begin()); }

// Independent oracle: the explicit nodes (root, suffixes, right-branching substrings) from direct substring enumeration.
template<class T> struct Brute {
    vector<T> s;
    int n;
    map<vector<T>, vector<int>> occ;
    vector<vector<T>> names;
    map<vector<T>, int> id;
    vector<int> parent, leaf;
    vector<vector<int>> kids;
    explicit Brute(const vector<T> &text) : s(text), n(int(text.size())), leaf(text.size()) {
        for (int i = 0; i <= n; ++i) {
            for (int j = i; j <= n; ++j) { occ[vector<T>(s.begin() + i, s.begin() + j)].push_back(i); }}
        for (auto &[w, starts] : occ) {
            int m = int(w.size());
            set<T> right;
            for (int p : starts) {
                if (p + m < n) { right.insert(s[p + m]); }}
            if (w.empty() || starts.back() + m == n || right.size() >= 2) { id[w] = int(names.size()); names.push_back(w); }}
        int V = int(names.size());
        parent.assign(V, -1);
        kids.resize(V);
        for (int v = 1; v < V; ++v) {
            for (int k = int(names[v].size()) - 1; k >= 0 && parent[v] < 0; --k) {
                auto it = id.find(vector<T>(names[v].begin(), names[v].begin() + k));
                if (it != id.end()) { parent[v] = it->second; }}
            kids[parent[v]].push_back(v);}
        for (int i = 0; i < n; ++i) { leaf[i] = id.at(vector<T>(s.begin() + i, s.end())); }}
    int depth(int v) const { return int(names[v].size()); }
    // Locus by extension: append the unique right extension until the string is explicit.
    int locus(vector<T> w) const {
        while (!id.count(w)) { w.push_back(s[occ.at(w)[0] + w.size()]); }
        return id.at(w);}
    // Locus of the length-len prefix of node v's string: its shallowest ancestor of depth >= len.
    int up(int v, int len) const {
        while (v && depth(parent[v]) >= len) { v = parent[v]; }
        return v;}
};

template<class T> void sameTree(const SuffixTree<T> &a, const SuffixTree<T> &b, const char *op) {
    bool ok = a.nodes.size() == b.nodes.size() && a.leaf == b.leaf && a.start == b.start && a.kids == b.kids && a.jump == b.jump;
    for (int v = 0; ok && v < int(a.nodes.size()); ++v) {
        auto x = a.nodes[v], y = b.nodes[v];
        ok = x.l == y.l && x.r == y.r && x.depth == y.depth && x.parent == y.parent && x.end == y.end;}
    check(ok, op);}

template<class T> void verify(const vector<T> &s, const vector<T> &symbols) {
    ++cases;
    context = show(s);
    at_v = -1, at_a = at_b = 0;
    SuffixTree<T> t(s);
    sameTree(t, SuffixTree<T>(SuffixArray<T>(s)), "construction from SuffixArray with RMQ");
    SuffixTree<T> reused(vector<T>(3, symbols[0]));
    reused.build(SuffixArray<T>(s, false));
    sameTree(t, reused, "build on a used object");
    Brute<T> b(s);
    int n = int(s.size()), V = int(b.names.size());
    expectEqual(lng(t.nodes.size()), V, "node count");
    expectEqual(t.root(), 0, "root");
    vector<vector<T>> suffixes;
    for (int i = 0; i < n; ++i) { suffixes.emplace_back(s.begin() + i, s.end()); }
    sort(suffixes.begin(), suffixes.end());
    for (int v = 0; v < V; ++v) {
        const vector<T> &w = b.names[v];
        int m = int(w.size()), l = 0, r, end = v;
        at_v = v;
        while (l < n && !hasPrefix(suffixes[l], w)) { ++l; }
        for (r = l; r < n && hasPrefix(suffixes[r], w); ++r) {}
        while (end < V && hasPrefix(b.names[end], w)) { ++end; }
        const auto &node = t.nodes[v];
        expectEqual(node.depth, m, "Node depth");
        expectEqual(t.depth(v), m, "depth");
        expectEqual(node.parent, b.parent[v], "Node parent");
        expectEqual(node.l, l, "Node l");
        expectEqual(node.r, r, "Node r");
        expectEqual(node.end, end, "Node end (dfsOrder subtree)");
        auto got = t.children(v);
        check(vector<int>(got.begin(), got.end()) == b.kids[v], "children");
        for (T c : symbols) {
            int want = -1;
            for (int u : b.kids[v]) {
                if (b.names[u][m] == c) { want = u; }}
            at_a = lng(c);
            expectEqual(t.child(v, c), want, "child(node, a)");}
        bool suffix = v && b.occ.at(w).back() + m == n;
        expectEqual(t.leafSuffix(v), suffix ? n - m : -1, "leafSuffix");
        vector<int> starts = b.occ.at(w);
        if (!v) { starts.pop_back(); }
        check(t.occurrences(v) == starts, "occurrences");
        for (int len = 0; len <= m; ++len) {
            at_a = len;
            expectEqual(t.weightedAncestor(v, len), b.up(v, len), "weightedAncestor(node, a)");}
        if (!v) { continue; }
        int pd = b.depth(b.parent[v]);
        auto [x, y] = t.edgeLabel(v);
        check(y - x == m - pd && x >= pd && y <= n && std::equal(w.begin(), w.end(), s.begin() + (x - pd)), "edgeLabel");
        auto link = b.id.find(vector<T>(w.begin() + 1, w.end()));
        check(link != b.id.end(), "oracle: suffix link target is explicit");
        expectEqual(t.suffixLink(v), link->second, "suffixLink");}
    at_v = -1;
    for (int i = 0; i < n; ++i) {
        at_a = i;
        expectEqual(t.leaf[i], b.leaf[i], "leaf[a]");}
    for (int i = 0; i <= n; ++i) {
        for (int j = i; j <= n; ++j) {
            at_a = i, at_b = j;
            expectEqual(t.weightedAncestor(i < n ? t.leaf[i] : 0, j - i), i < n ? b.up(b.leaf[i], j - i) : 0, "locus of s[a, b)");}}
    for (auto &[w, starts] : b.occ) {
        int m = int(w.size()), p0 = starts[0], u = p0 < n ? b.up(b.leaf[p0], m) : 0;
        at_a = p0, at_b = p0 + m;
        check(!m || b.locus(w) == u, "oracle: extension locus equals ancestor locus of s[a, b)");
        expectPair(t.locate(w), {u, m}, "locate s[a, b)");
        check(t.isSubstring(w), "isSubstring s[a, b)");
        for (T c : symbols) {
            pair<int, int> want{-1, 0};
            for (int p : starts) {
                if (p + m < n && s[p + m] == c) { want = {b.up(b.leaf[p], m + 1), m + 1}; }}
            expectPair(t.step({u, m}, c), want, "step from the locus of s[a, b)");}}
    for (int probe = 0; probe < 12; ++probe) {
        vector<T> p = randomText(int(nextRandom() % (n + 3)), symbols);
        int k = 0;
        while (k < int(p.size()) && b.occ.count(vector<T>(p.begin(), p.begin() + k + 1))) { ++k; }
        at_a = probe, at_b = k;
        expectPair(t.locate(p), {b.locus(vector<T>(p.begin(), p.begin() + k)), k}, "locate probe a, longest prefix b");
        check(t.isSubstring(p) == (k == int(p.size())), "isSubstring probe a");}
    vector<int> maximal, supermaximal;
    for (auto &[w, starts] : b.occ) {
        int m = int(w.size());
        bool pair_found = false;
        for (int x = 0; x < int(starts.size()) && !pair_found && m; ++x) {
            for (int y = x + 1; y < int(starts.size()) && !pair_found; ++y) {
                int p = starts[x], q = starts[y];
                pair_found = (!p || s[p - 1] != s[q - 1]) && (q + m == n || s[p + m] != s[q + m]);}}
        if (pair_found) {
            at_a = starts[0], at_b = starts[0] + m;
            check(b.id.count(w) > 0, "oracle: maximal repeat s[a, b) is explicit");
            maximal.push_back(b.id.at(w));}}
    for (int v : maximal) {
        const vector<T> &w = b.names[v];
        bool inside = false;
        for (int u : maximal) { inside = inside || (u != v && std::search(b.names[u].begin(), b.names[u].end(), w.begin(), w.end()) != b.names[u].end()); }
        if (!inside) { supermaximal.push_back(v); }}
    check(t.maximalRepeats() == maximal, "maximalRepeats (maximal pairs)");
    check(t.supermaximalRepeats() == supermaximal, "supermaximalRepeats (maximal repeats in no other)");
    expectEqual(t.distinctSubstrings(), lng(b.occ.size()) - 1, "distinctSubstrings");}

void verifyBytes(const string &s) {
    vector<int> bytes;
    for (char c : s) { bytes.push_back(uint8_t(c)); }
    vector<int> symbols{0, 1, 0x7f, 0x80, 0xff, 'a', 256, -1};
    verify(bytes, symbols);
    SuffixTree t(s);
    SuffixTree u{string_view(s)};
    SuffixTree w(bytes);
    sameTree(t, w, "string constructor equals unsigned byte vector");
    sameTree(u, w, "string_view constructor equals unsigned byte vector");
    for (int probe = 0; probe < 8; ++probe) {
        string p(nextRandom() % (s.size() + 2), ' ');
        for (char &c : p) { c = "\0\x01\x7f\x80\xff"[nextRandom() % 5]; }
        vector<int> q;
        for (char c : p) { q.push_back(uint8_t(c)); }
        expectPair(t.locate(p), t.locate(q), "locate(string_view) equals byte vector");
        check(t.isSubstring(p) == t.isSubstring(q), "isSubstring(string_view) equals byte vector");}}

// Hand-derived from Gusfield's definitions: maximal and supermaximal repeats as strings, distinct substring counts.
void regressions() {
    struct Case { string s; vector<string> maximal, supermaximal; lng distinct; };
    vector<Case> known{{"banana", {"a", "ana"}, {"ana"}, 15}, {"mississippi", {"i", "issi", "p", "s"}, {"issi", "p"}, 53},
                       {"abab", {"ab"}, {"ab"}, 7}, {"xabxac", {"xa"}, {"xa"}, 18}, {"aaaa", {"a", "aa", "aaa"}, {"aaa"}, 4}, {"", {}, {}, 0}};
    for (const auto &c : known) {
        context = '"' + c.s + '"';
        SuffixTree t(c.s);
        auto names = [&](const vector<int> &ids) {
            vector<string> res;
            for (int v : ids) { res.push_back(c.s.substr(t.index.sa[t.nodes[v].l], t.nodes[v].depth)); }
            return res;};
        check(names(t.maximalRepeats()) == c.maximal, "fixed maximalRepeats");
        check(names(t.supermaximalRepeats()) == c.supermaximal, "fixed supermaximalRepeats");
        expectEqual(t.distinctSubstrings(), c.distinct, "fixed distinctSubstrings");}
    cout << "PASS fixed regressions (banana, mississippi, abab, xabxac, aaaa, empty)\n";}
void small(const string &mode) {
    int ternary = mode == "quick" ? 6 : mode == "full" ? 7 : 8, binary = mode == "quick" ? 9 : mode == "full" ? 11 : 12;
    vector<vector<int>> words{{}};
    for (int len = 0; len <= max(ternary, binary); ++len) {
        for (auto &w : words) {
            if (len <= binary && std::ranges::count(w, 2) == 0) { verify(w, vector<int>{0, 1, 2, -1}); }
            else if (len <= ternary) { verify(w, vector<int>{0, 1, 2, 3, -1}); }}
        vector<vector<int>> longer;
        for (auto &w : words) {
            if (len >= ternary && std::ranges::count(w, 2)) { continue; }
            for (int c = 0; c < (len < ternary ? 3 : 2); ++c) {
                longer.push_back(w);
                longer.back().push_back(c);}}
        words.swap(longer);}
    SuffixTree<int> empty;
    context = "default constructed";
    check(empty.nodes.size() == 1 && empty.leaf.empty() && empty.distinctSubstrings() == 0 && empty.isSubstring("") && !empty.isSubstring("a"), "empty tree");
    check(empty.occurrences(0).empty() && empty.maximalRepeats().empty() && empty.supermaximalRepeats().empty() && empty.children(0).empty(), "empty tree queries");
    cout << "PASS exhaustive ternary texts through length " << ternary << " and binary through " << binary << '\n';}
void randomCases(const string &mode) {
    int rounds = mode == "quick" ? 80 : mode == "full" ? 800 : 1600;
    for (int round = 0; round < rounds; ++round) {
        int n = int(nextRandom() % 31), sigma = 1 + int(nextRandom() % 4);
        vector<int> alphabet(sigma);
        iota(alphabet.begin(), alphabet.end(), -1);
        vector<int> symbols = alphabet;
        symbols.push_back(sigma - 1);
        verify(randomText(n, alphabet), symbols);
        verify(randomText(n % 17, vector<bool>{false, true}), vector<bool>{false, true});
        vector<lng> wide{std::numeric_limits<lng>::min(), -1, 0, std::numeric_limits<lng>::max()};
        verify(randomText(n % 19, vector<lng>(wide.begin(), wide.begin() + 1 + int(nextRandom() % 4))), wide);
        vector<signed char> chars{-128, -1, 0, 127};
        verify(randomText(n % 13, chars), chars);
        string bytes;
        for (int i = 0; i < n % 15; ++i) { bytes.push_back("\0\x01\x7f\x80\xff"[nextRandom() % 5]); }
        verifyBytes(bytes);}
    int medium = mode == "quick" ? 2 : mode == "full" ? 16 : 30;
    for (int round = 0; round < medium; ++round) {
        int n = 60 + int(nextRandom() % (mode == "stress" ? 141 : 81));
        vector<int> fib{0}, prev{1};
        while (int(fib.size()) < n) {
            vector<int> nx = fib;
            nx.insert(nx.end(), prev.begin(), prev.end());
            prev = fib;
            fib = nx;}
        fib.resize(n);
        vector<int> periodic(n);
        for (int i = 0; i < n; ++i) { periodic[i] = i % (1 + round % 4); }
        verify(round % 3 == 0 ? fib : round % 3 == 1 ? periodic : randomText(n, vector<int>{0, 1, 2}), vector<int>{0, 1, 2, 3});}
    cout << "PASS random int/bool/lng/signed char/byte texts rounds=" << rounds << " and " << medium << " medium texts\n";}

// Independent oracle for large texts: the SuffixArray with RMQ (verified in 07), its LCE and pattern ranges.
pair<int, int> rangeOf(const SuffixArray<int> &sa, int i, int len) {
    int n = sa.size(), at = i < n ? sa.rank[i] : 0;
    if (!len) { return {0, n}; }
    int lo = 0, hi = at;
    while (lo < hi) {
        int mid = std::midpoint(lo, hi);
        if (sa.lce(sa.sa[mid], i) >= len) { hi = mid; }
        else { lo = mid + 1; }}
    int first = lo;
    lo = at + 1, hi = n;
    while (lo < hi) {
        int mid = std::midpoint(lo, hi);
        if (sa.lce(sa.sa[mid], i) >= len) { lo = mid + 1; }
        else { hi = mid; }}
    return {first, lo};}
void large(const string &mode) {
    int n = mode == "quick" ? 3000 : mode == "full" ? 200000 : 700000;
    auto randomOver = [&](const string &alphabet) {
        string res(n, ' ');
        for (char &c : res) { c = alphabet[nextRandom() % alphabet.size()]; }
        return res;};
    string fib = "a", prev = "b", periodic(n, ' '), thue(n, ' ');
    while (int(fib.size()) < n) {
        string nx = fib + prev;
        prev = fib;
        fib = nx;}
    for (int i = 0; i < n; ++i) { periodic[i] = "abc"[i % 3]; thue[i] = char('a' + __builtin_popcount(uint(i)) % 2); }
    vector<string> texts{string(n, 'a'), randomOver("ab"), randomOver("abcdefghijklmnopqrstuvwxyz"), fib.substr(0, n), periodic, thue};
    at_v = -1, at_a = at_b = 0;
    for (const string &s : texts) {
        ++cases;
        context = "large n=" + std::to_string(n) + " prefix=" + s.substr(0, 12);
        SuffixArray sa(s);
        SuffixTree t(sa);
        int V = int(t.nodes.size()), ends = 0;
        check(V <= 2 * n, "at most 2n nodes");
        for (int v = 0; v < V; ++v) {
            const auto &x = t.nodes[v];
            auto kids = t.children(v);
            check((x.r - x.l == 1) == kids.empty(), "leaf exactly when the interval is one suffix");
            if (kids.empty()) { check(x.depth == n - sa.sa[x.l], "leaf depth"); }
            else { expectEqual(x.depth, v ? sa.lce(sa.sa[x.l], sa.sa[x.r - 1]) : 0, "internal depth is the interval LCE (root 0)"); }
            ends += t.leafSuffix(v) >= 0;
            if (!v) { continue; }
            const auto &p = t.nodes[x.parent];
            int boundary = max(x.l ? sa.lce(sa.sa[x.l - 1], sa.sa[x.l]) : 0, x.r < n ? sa.lce(sa.sa[x.r - 1], sa.sa[x.r]) : 0);
            check(pair{t.nodes[v - 1].l, t.nodes[v - 1].depth} < pair{x.l, x.depth}, "ids in (l, depth) pre-order");
            check(x.parent < v && p.l <= x.l && x.r <= p.r && v < x.end && x.end <= p.end, "parent nesting and subtree end");
            expectEqual(p.depth, boundary, "parent depth is the larger boundary LCE");
            auto [a, e] = t.edgeLabel(v);
            check(a == sa.sa[x.l] + p.depth && e == sa.sa[x.l] + x.depth, "edgeLabel");
            int at = x.l + (t.leafSuffix(v) >= 0);
            for (int u : kids) {
                check(t.nodes[u].l == at && t.nodes[u].parent == v, "children partition the interval");
                at = t.nodes[u].r;}
            check(kids.empty() || at == x.r, "children cover the interval");}
        expectEqual(ends, n, "every suffix ends on exactly one node");
        for (int i = 0; i < n; ++i) {
            const auto &x = t.nodes[t.leaf[i]];
            check(x.depth == n - i && x.l == sa.rank[i] && t.leafSuffix(t.leaf[i]) == i, "leaf of suffix");}
        lng distinct = lng(n) * (n + 1) / 2;
        for (int h : sa.lcp) { distinct -= h; }
        expectEqual(t.distinctSubstrings(), distinct, "distinctSubstrings versus n(n+1)/2 - sum lcp");
        for (int q = 0; q < 2000; ++q) {
            int i = int(nextRandom() % n), len = q % 2 ? int(nextRandom() % (n - i + 1)) : min(n - i, int(nextRandom() % 40));
            int u = t.weightedAncestor(t.leaf[i], len);
            const auto &x = t.nodes[u];
            at_a = i, at_b = i + len;
            check(pair{x.l, x.r} == rangeOf(sa, i, len) && x.depth >= len && (!u ? !len : t.nodes[x.parent].depth < len), "weightedAncestor locus of s[a, b)");
            if (i + len < n) { expectPair(t.step({u, len}, uint8_t(s[i + len])), {t.weightedAncestor(t.leaf[i], len + 1), len + 1}, "step along the text"); }
            expectPair(t.step({u, len}, 'z' + 1), {-1, 0}, "step absent symbol");
            int m = min(n - i, int(nextRandom() % 40));
            string p = s.substr(i, m);
            if (q % 3 == 0 && m) { p.back() = char(p.back() == 'a' ? 'b' : 'a'); }
            auto [w, k] = t.locate(p);
            auto [lo, hi] = sa.patternRange(p.substr(0, k));
            check(lo < hi && pair{t.nodes[w].l, t.nodes[w].r} == pair{lo, hi} && t.nodes[w].depth >= k, "locate locus interval");
            auto [lo2, hi2] = sa.patternRange(p.substr(0, k + 1));
            check(k == int(p.size()) || lo2 == hi2, "locate is the longest occurring prefix");
            check(t.isSubstring(p) == (k == int(p.size())), "isSubstring");
            int v = 1 + int(nextRandom() % (V - 1)), d = t.nodes[v].depth;
            int link = t.suffixLink(v);
            at_v = v;
            check(t.nodes[link].depth == d - 1 && pair{t.nodes[link].l, t.nodes[link].r} == rangeOf(sa, sa.sa[t.nodes[v].l] + 1, d - 1), "suffixLink");
            if (t.nodes[v].r - t.nodes[v].l <= 200) {
                auto occ = t.occurrences(v);
                bool ok = int(occ.size()) == t.nodes[v].r - t.nodes[v].l && std::is_sorted(occ.begin(), occ.end());
                for (int o : occ) { ok = ok && sa.lce(o, sa.sa[t.nodes[v].l]) >= d; }
                check(ok, "occurrences");}
            at_v = -1;}
        auto maximal = t.maximalRepeats(), supermaximal = t.supermaximalRepeats();
        string rs(s.rbegin(), s.rend());
        SuffixTree rt(rs);
        auto rmaximal = rt.maximalRepeats();
        vector<char> reversed_maximal(rt.nodes.size());
        for (int u : rmaximal) { reversed_maximal[u] = 1; }
        expectEqual(lng(maximal.size()), lng(rmaximal.size()), "maximal repeats are closed under reversal");
        expectEqual(lng(supermaximal.size()), lng(rt.supermaximalRepeats().size()), "supermaximal repeats are closed under reversal");
        check(std::includes(maximal.begin(), maximal.end(), supermaximal.begin(), supermaximal.end()), "supermaximal repeats are maximal");
        for (int probe = 0; probe < 200 && !maximal.empty(); ++probe) {
            int v = maximal[nextRandom() % maximal.size()], d = t.nodes[v].depth, i = sa.sa[t.nodes[v].l];
            int u = rt.weightedAncestor(rt.leaf[n - i - d], d);
            at_v = v;
            check(rt.nodes[u].depth == d && reversed_maximal[u], "reversed maximal repeat");
            if (d <= 64) { expectPair(rt.locate(rs.substr(n - i - d, d)), {u, d}, "reversed maximal repeat locate"); }}
        at_v = -1;
        if (s == string(n, 'a')) {
            check(int(maximal.size()) == n - 1 && supermaximal.size() == 1 && t.nodes[supermaximal[0]].depth == n - 1, "unary repeats a^1..a^(n-1), supermaximal a^(n-1)");}}
    cout << "PASS large unary/random/Fibonacci/periodic/Thue-Morse n=" << n << " against suffix array\n";}

int invalid(const string &probe) {
    SuffixTree t("abab");
    int leaf = t.leaf[0], V = int(t.nodes.size());
    if (probe == "edge-root") { t.edgeLabel(0); }
    if (probe == "link-root") { t.suffixLink(0); }
    if (probe == "ancestor-deep") { t.weightedAncestor(leaf, t.depth(leaf) + 1); }
    if (probe == "ancestor-negative") { t.weightedAncestor(leaf, -1); }
    if (probe == "ancestor-node") { t.weightedAncestor(V, 0); }
    if (probe == "step-root-length") { t.step({0, 1}, 'a'); }
    if (probe == "step-noncanonical") { t.step({leaf, t.nodes[t.nodes[leaf].parent].depth}, 'a'); }
    if (probe == "step-deep") { t.step({leaf, t.depth(leaf) + 1}, 'a'); }
    if (probe == "leaf-range") { t.leafSuffix(-1); }
    if (probe == "occurrences-range") { t.occurrences(V); }
    if (probe == "depth-range") { t.depth(V); }
    if (probe == "children-range") { t.children(-1); }
    if (probe == "child-range") { t.child(V, 'a'); }
    return 0;}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { return invalid(argv[i + 1]); }}
    rng_state = test_seed;
    regressions();
    small(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS suffixtree seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
