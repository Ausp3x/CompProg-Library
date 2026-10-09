#include "../../07-Strings/10-suffixautomaton.hpp"
#include "../../07-Strings/07-suffixarray.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &op) {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=true actual=false\n";
    std::exit(1);}
void expectEqual(lll actual, lll expected, const string &op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op
         << " expected=" << lng(expected) << " actual=" << lng(actual) << '\n';
    std::exit(1);}
string show(string_view s) {
    string res;
    for (char c : s) { res += 32 <= c && c < 127 ? string(1, c) : "\\x" + std::to_string(uint8_t(c)); }
    return '"' + res + '"';}
lng countOccurrences(const string &s, const string &w) {
    lng res = 0;
    for (int i = 0; i + int(w.size()) <= int(s.size()); ++i) { res += s.compare(i, w.size(), w) == 0; }
    return res;}

// Every check below uses direct substring enumeration over s.
template<class Sam> void verify(const string &s, const string &alphabet, const vector<string> &probes) {
    ++cases;
    context = show(s);
    int n = int(s.size());
    Sam online;
    for (int i = 0; i < n; ++i) {
        online.extend(s[i]);
        set<string> prefix;
        for (int l = 0; l <= i; ++l) {
            for (int r = l + 1; r <= i + 1; ++r) { prefix.insert(s.substr(l, r - l)); }}
        expectEqual(online.distinctSubstrings(), lng(prefix.size()), "online distinct prefix=" + std::to_string(i + 1));
        expectEqual(online.firstOccurrence(s.substr(0, i + 1)), 0, "online first occurrence");}
    check(!online.built, "extend clears built");
    online.build();
    Sam sam(s);
    check(sam.built && online.built && sam.nodes.size() == online.nodes.size(), "online versus build(string_view)");
    map<string, vector<int>> occ;
    for (int i = 0; i < n; ++i) {
        for (int len = 1; i + len <= n; ++len) { occ[s.substr(i, len)].push_back(i); }}
    expectEqual(sam.size(), n, "size");
    expectEqual(sam.distinctSubstrings(), lng(occ.size()), "distinctSubstrings");
    lll total = 0;
    for (auto &[w, starts] : occ) { total += lll(w.size()); }
    expectEqual(sam.totalSubstringLength(), total, "totalSubstringLength");
    map<int, vector<int>> lengths;
    map<vector<int>, int> classes;
    for (auto &[w, starts] : occ) {
        int u = sam.findNode(w), m = int(w.size());
        check(u > 0 && sam.isSubstring(w), "findNode/isSubstring present " + show(w));
        expectEqual(sam.occurrenceCount(w), lng(starts.size()), "occurrenceCount " + show(w));
        expectEqual(sam.firstOccurrence(w), starts[0], "firstOccurrence " + show(w));
        expectEqual(sam.lastOccurrence(w), starts.back(), "lastOccurrence " + show(w));
        check(sam.occurrences(w) == starts, "occurrences " + show(w));
        expectEqual(sam.endposSize(u), lng(starts.size()), "endposSize " + show(w));
        expectEqual(online.occurrenceCount(w), lng(starts.size()), "online occurrenceCount");
        lengths[u].push_back(m);
        vector<int> ends;
        for (int i : starts) { ends.push_back(i + m - 1); }
        auto [it, fresh] = classes.insert({ends, u});
        check(it->second == u, "equal endpos share a state " + show(w));}
    expectEqual(lng(sam.nodes.size()), lng(classes.size()) + 1, "one state per endpos class plus root");
    for (auto &[u, ls] : lengths) {
        expectEqual(*std::max_element(ls.begin(), ls.end()), sam.nodes[u].len, "Node len is the longest member");
        expectEqual(*std::min_element(ls.begin(), ls.end()), sam.minimalLength(u), "minimalLength");
        expectEqual(lng(ls.size()), sam.nodes[u].len - sam.minimalLength(u) + 1, "state lengths contiguous");
        int link = sam.nodes[u].link;
        check(link >= 0 && sam.nodes[link].len == sam.minimalLength(u) - 1, "Node link length");}
    for (int x : {-1000, -1, Sam::A, Sam::A + 97, 353}) { expectEqual(sam.step(0, x), -1, "step out-of-range code " + std::to_string(x)); }
    for (int x = 0; x < Sam::A; ++x) {
        string w(1, char(x + (Sam::A == 256 ? 0 : int(alphabet[0]))));
        expectEqual(sam.step(0, x), occ.count(w) ? sam.findNode(w) : -1, "step code " + std::to_string(x));}
    expectEqual(sam.minimalLength(0), 0, "root minimalLength");
    expectEqual(sam.endposSize(0), n + 1, "root endposSize");
    expectEqual(sam.findNode(""), 0, "empty pattern state");
    expectEqual(sam.occurrenceCount(""), n + 1, "empty occurrenceCount");
    expectEqual(sam.firstOccurrence(""), 0, "empty firstOccurrence");
    expectEqual(sam.lastOccurrence(""), n, "empty lastOccurrence");
    vector<int> all(n + 1);
    iota(all.begin(), all.end(), 0);
    check(sam.occurrences("") == all, "empty occurrences");
    for (const string &p : probes) {
        bool present = s.find(p) != string::npos;
        check(sam.isSubstring(p) == present, "isSubstring probe " + show(p));
        if (!present) {
            check(sam.findNode(p) == -1 && sam.occurrenceCount(p) == 0 && sam.firstOccurrence(p) == -1 && sam.lastOccurrence(p) == -1, "absent pattern " + show(p));
            check(sam.occurrences(p).empty(), "absent occurrences " + show(p));}}
    vector<string> keys;
    for (auto &[w, starts] : occ) { keys.push_back(w); }
    for (int k = 0; k < int(keys.size()); ++k) {
        auto [start, len] = sam.kthSubstringDistinct(k);
        check(len > 0 && s.substr(start, len) == keys[k] && start == occ[keys[k]][0], "kthSubstringDistinct k=" + std::to_string(k));}
    check(sam.kthSubstringDistinct(-1) == pair<int, int>{-1, 0} && sam.kthSubstringDistinct(lng(keys.size())) == pair<int, int>{-1, 0}, "kthSubstringDistinct out of range");
    vector<string> multi;
    for (auto &[w, starts] : occ) { multi.insert(multi.end(), starts.size(), w); }
    expectEqual(lng(multi.size()), lng(n) * (n + 1) / 2, "multiset size");
    for (int k = 0; k < int(multi.size()); ++k) {
        auto [start, len] = sam.kthSubstring(k);
        check(len > 0 && s.substr(start, len) == multi[k] && start == occ[multi[k]][0], "kthSubstring k=" + std::to_string(k));}
    check(sam.kthSubstring(-1) == pair<int, int>{-1, 0} && sam.kthSubstring(lng(multi.size())) == pair<int, int>{-1, 0}, "kthSubstring out of range");
    vector<string> walked;
    check(sam.lexicographicWalk([&](string_view w, int u) {
        check(u == sam.findNode(w), "lexicographicWalk state");
        walked.emplace_back(w);
        return true;}), "lexicographicWalk complete");
    check(walked == keys, "lexicographicWalk order");
    for (int stop = 0; stop < min<int>(3, int(keys.size())); ++stop) {
        int seen = 0;
        check(!sam.lexicographicWalk([&](string_view, int) { return seen++ < stop; }) && seen == stop + 1, "lexicographicWalk early stop");}
    for (const string &t : probes) {
        auto ms = sam.matchingStatistics(t);
        expectEqual(lng(ms.size()), lng(t.size()), "matchingStatistics size");
        int best = 0, at = 0;
        for (int j = 0; j < int(t.size()); ++j) {
            int len = 0;
            while (len <= j && occ.count(t.substr(j - len, len + 1))) { ++len; }
            expectEqual(ms[j], len, "matchingStatistics " + show(t) + " j=" + std::to_string(j));
            if (len > best) { best = len; at = j - len + 1; }}
        tuple<int, int, int> expected{0, 0, 0};
        if (best) { expected = {occ[t.substr(at, best)][0], at, best}; }
        check(sam.longestCommonSubstring(t) == expected, "longestCommonSubstring " + show(t));
        set<string> rotations;
        for (int i = 0; i < int(t.size()); ++i) { rotations.insert(t.substr(i) + t.substr(0, i)); }
        lng count = t.empty() ? n + 1 : 0;
        for (const string &r : rotations) { count += countOccurrences(s, r); }
        expectEqual(sam.cyclicShiftOccurrences(t), count, "cyclicShiftOccurrences " + show(t));}
    string absent;
    for (int len = 1; absent.empty(); ++len) {
        vector<string> words{""};
        for (int step = 0; step < len; ++step) {
            vector<string> longer;
            for (auto &w : words) {
                for (char c : alphabet) { longer.push_back(w + c); }}
            words.swap(longer);}
        for (auto &w : words) {
            if (!occ.count(w)) { absent = w; break; }}}
    check(sam.shortestAbsentString() == absent, "shortestAbsentString expected=" + show(absent) + " actual=" + show(sam.shortestAbsentString()));
    auto tree = sam.suffixLinkTree();
    expectEqual(lng(tree.size()), lng(sam.nodes.size()), "suffixLinkTree size");
    lng edges = 0;
    for (int u = 0; u < int(tree.size()); ++u) {
        check(std::is_sorted(tree[u].begin(), tree[u].end()), "suffixLinkTree children sorted");
        for (int v : tree[u]) { ++edges; check(sam.nodes[v].link == u && sam.nodes[u].len < sam.nodes[v].len, "suffixLinkTree edge"); }}
    expectEqual(edges, lng(sam.nodes.size()) - 1, "suffixLinkTree edge count");
    auto per = distinctSubstringsOnline(s);
    expectEqual(lng(per.size()), n + 1, "distinctSubstringsOnline size");
    for (int i = 0; i <= n; ++i) {
        set<string> prefix;
        for (int l = 0; l < i; ++l) {
            for (int r = l + 1; r <= i; ++r) { prefix.insert(s.substr(l, r - l)); }}
        expectEqual(per[i], lng(prefix.size()), "distinctSubstringsOnline i=" + std::to_string(i));}}

ulng rng_state;
ulng nextRandom() {
    rng_state += 0x9e3779b97f4a7c15ULL;
    ulng z = rng_state;
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);}
string randomString(int n, const string &alphabet) {
    string res(n, ' ');
    for (char &c : res) { c = alphabet[nextRandom() % alphabet.size()]; }
    return res;}
vector<string> probesFor(const string &s, const string &alphabet) {
    vector<string> res{"", s, s + alphabet[0], string(1, alphabet.back()) + s, string(1, char(alphabet.back() + 1))};
    for (int k = 0; k < 6; ++k) { res.push_back(randomString(int(nextRandom() % (s.size() + 3)), alphabet)); }
    for (int k = 0; k < 4 && !s.empty(); ++k) {
        int l = int(nextRandom() % s.size()), len = 1 + int(nextRandom() % (s.size() - l));
        string w = s.substr(l, len);
        rotate(w.begin(), w.begin() + nextRandom() % w.size(), w.end());
        res.push_back(w);}
    return res;}
void small(const string &mode) {
    int limit = mode == "quick" ? 5 : mode == "full" ? 7 : 8;
    string abc = "abc", full = "abcdefghijklmnopqrstuvwxyz";
    vector<string> words{""};
    for (int len = 0; len <= limit; ++len) {
        for (auto &w : words) {
            verify<SuffixAutomatonDense<3, 'a'>>(w, abc, probesFor(w, abc));
            verify<SuffixAutomaton>(w, string(1, '\0') + abc, probesFor(w, abc));
            if (len <= 3) { verify<SuffixAutomatonDense<>>(w, full, probesFor(w, abc)); }}
        vector<string> longer;
        for (auto &w : words) {
            for (char c : abc) { longer.push_back(w + c); }}
        words.swap(longer);}
    cout << "PASS exhaustive ternary texts through length " << limit << " (dense S=3, map, dense S=26)\n";}
void randomCases(const string &mode) {
    int rounds = mode == "quick" ? 60 : mode == "full" ? 600 : 3000;
    string bytes;
    for (int c = 0; c < 256; ++c) { bytes.push_back(char(c)); }
    for (int r = 0; r < rounds; ++r) {
        int n = int(nextRandom() % 26);
        string bin = randomString(n, "ab");
        verify<SuffixAutomatonDense<2, 'a'>>(bin, "ab", probesFor(bin, "ab"));
        string tri = randomString(n, "xyz");
        verify<SuffixAutomatonDense<3, 'x'>>(tri, "xyz", probesFor(tri, "xyz"));
        string wide = randomString(n % 14, string("\0\x01\x7f\x80\xff", 5));
        string order;
        for (int c = 0; c < 256; ++c) { order.push_back(char(c)); }
        verify<SuffixAutomaton>(wide, order, probesFor(wide, string("\0\x01\x7f\x80\xff", 5)));}
    SuffixAutomaton all(bytes + bytes.substr(0, 3));
    string expected;
    for (int x = 0; x < 256 && expected.empty(); ++x) {
        for (int y = 0; y < 256 && expected.empty(); ++y) {
            string w{char(x), char(y)};
            if ((bytes + bytes.substr(0, 3)).find(w) == string::npos) { expected = w; }}}
    context = "all 256 bytes";
    check(all.shortestAbsentString() == expected, "shortestAbsentString needs two bytes");
    set<string> every;
    string t = bytes + bytes.substr(0, 3);
    for (int i = 0; i < int(t.size()); ++i) {
        for (int len = 1; i + len <= int(t.size()); ++len) { every.insert(t.substr(i, len)); }}
    expectEqual(all.distinctSubstrings(), lng(every.size()), "all bytes distinct count");
    cout << "PASS random binary/ternary/byte texts rounds=" << rounds << '\n';}
// Independent oracle: suffix array distinct count n(n+1)/2 - sum(lcp) and k-th distinct substring by rank scanning.
void large(const string &mode) {
    int n = mode == "quick" ? 3000 : mode == "full" ? 200000 : 700000;
    vector<string> texts{string(n, 'a'), randomString(n, "ab"), randomString(n, "abcdefghijklmnopqrstuvwxyz")};
    string fib = "a", prev = "b";
    while (int(fib.size()) < n) { string next = fib + prev; prev = fib; fib = next; }
    texts.push_back(fib.substr(0, n));
    for (const string &s : texts) {
        ++cases;
        context = "large n=" + std::to_string(n) + " prefix=" + show(s.substr(0, 8));
        SuffixAutomaton sam(s);
        SuffixAutomatonDense<> dense(s);
        SuffixArray sa(s, false);
        lng distinct = lng(n) * (n + 1) / 2;
        lll total = lll(n) * (n + 1) * (n + 2) / 6;
        for (int i = 0; i + 1 < n; ++i) {
            distinct -= sa.lcp[i];
            total -= lll(sa.lcp[i]) * (sa.lcp[i] + 1) / 2;}
        expectEqual(sam.distinctSubstrings(), distinct, "large distinct versus suffix array");
        expectEqual(dense.distinctSubstrings(), distinct, "large dense distinct");
        expectEqual(sam.totalSubstringLength(), total, "large total length versus suffix array");
        for (int probe = 0; probe < 20; ++probe) {
            lng k = lng(nextRandom() % ulng(distinct));
            lng left = k;
            int start = -1, len = 0;
            for (int i = 0; i < n; ++i) {
                int before = i ? sa.lcp[i - 1] : 0, fresh = n - sa.sa[i] - before;
                if (left < fresh) { start = sa.sa[i]; len = before + int(left) + 1; break; }
                left -= fresh;}
            auto [got_start, got_len] = sam.kthSubstringDistinct(k);
            check(got_len == len && s.compare(got_start, len, s, start, len) == 0, "large kthSubstringDistinct k=" + std::to_string(k));
            check(dense.kthSubstringDistinct(k) == pair<int, int>{got_start, got_len}, "large dense kth");
            int l = int(nextRandom() % n), m = 1 + int(nextRandom() % min(12, n - l));
            string p = s.substr(l, m);
            auto [lo, hi] = sa.patternRange(p);
            expectEqual(sam.occurrenceCount(p), hi - lo, "large occurrenceCount");
            int first = n, last = -1;
            for (int i = lo; i < hi; ++i) { first = min(first, sa.sa[i]); last = max(last, sa.sa[i]); }
            expectEqual(sam.firstOccurrence(p), first, "large firstOccurrence");
            expectEqual(sam.lastOccurrence(p), last, "large lastOccurrence");
            if (hi - lo <= 1000) {
                vector<int> starts(sa.sa.begin() + lo, sa.sa.begin() + hi);
                sort(starts.begin(), starts.end());
                check(sam.occurrences(p) == starts, "large occurrences");}}
        expectEqual(sam.occurrenceCount(""), n + 1, "large empty count");
        if (s == string(n, 'a')) {
            expectEqual(sam.cyclicShiftOccurrences("aaa"), n - 2, "unary cyclic shifts");
            check(sam.shortestAbsentString() == string(1, '\0') && dense.shortestAbsentString() == "b", "unary absent");
            check(sam.kthSubstring(lng(n) * (n + 1) / 2 - 1) == pair<int, int>{0, n}, "unary last kthSubstring");}}
    cout << "PASS large unary/random/Fibonacci n=" << n << " against suffix array\n";}
int invalid(const string &probe) {
    SuffixAutomaton a;
    a.extend('a');
    if (probe == "dense-domain") { SuffixAutomatonDense<3, 'a'> d; d.extend('d'); }
    if (probe == "unbuilt-count") { a.occurrenceCount("a"); }
    if (probe == "unbuilt-last") { a.lastOccurrence("a"); }
    if (probe == "unbuilt-occurrences") { a.occurrences("a"); }
    if (probe == "unbuilt-endpos") { a.endposSize(1); }
    if (probe == "unbuilt-kth") { a.kthSubstringDistinct(0); }
    if (probe == "unbuilt-kth-multi") { a.kthSubstring(0); }
    if (probe == "unbuilt-cyclic") { a.cyclicShiftOccurrences("a"); }
    if (probe == "unbuilt-absent") { a.shortestAbsentString(); }
    if (probe == "stale-after-extend") { SuffixAutomaton b("ab"); b.extend('c'); b.occurrenceCount("c"); }
    return 0;}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { return invalid(argv[i + 1]); }}
    rng_state = test_seed;
    small(mode);
    randomCases(mode);
    large(mode);
    cout << "PASS suffixautomaton seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
