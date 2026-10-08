#include "../../07-Strings/06-aho.hpp"

string mode, context;
ulng seed = 20260928, checks = 0;
std::mt19937_64 rng;
template<class T> string show(const T &x) { std::ostringstream out; out << +x; return out.str(); }
string show(const string &s);
template<class T> string show(const vector<T> &a) {
    string out = "[";
    for (int i = 0; i < int(a.size()); ++i) { out += (i ? "," : "") + show(a[i]); }
    return out + "]";}
string show(const string &s) { return show(vector<unsigned char>(s.begin(), s.end())); }
template<class T, class U> void check(const T &actual, const U &expected, string_view operation) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << operation
             << " smallest-known-reproducer=" << context << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n';
        std::exit(1);}}
int limit(int a, int b, int c) { return mode == "quick" ? a : mode == "full" ? b : c; }
bool suffix(string_view s, string_view t) { return s.size() >= t.size() && s.substr(s.size() - t.size()) == t; }
vector<string> words(int depth) {
    vector<string> a{""};
    for (int i = 0, start = 0; i < depth; ++i) {
        int end = int(a.size());
        for (int j = start; j < end; ++j) {
            for (char c : {'a', 'b'}) { a.push_back(a[j] + c); }}
        start = end;}
    return a;}
vector<string> stateWords(const AhoCorasick &ac) {
    vector<string> label(ac.size());
    for (int u : ac.order) {
        for (auto [c, v] : ac.nodes[u].next) { label[v] = label[u] + char(c); }}
    return label;}
int expectedState(const vector<string> &label, string_view s, int skip = -1) {
    int res = 0;
    for (int u = 1; u < int(label.size()); ++u) {
        if (u != skip && suffix(s, label[u]) && label[u].size() > label[res].size()) { res = u; }}
    return res;}

void testStructure(const AhoCorasick &ac, const vector<string> &p, const vector<string> &label) {
    check(ac.patterns(), int(p.size()), "pattern-size");
    vector<int> values(ac.size()), expected(ac.size());
    vector<string> strings(ac.size()), expected_strings(ac.size());
    for (int u = 0; u < ac.size(); ++u) {
        values[u] = 2 * u - 4;
        strings[u] = std::to_string(u) + "/";}
    for (int id = 0; id < int(p.size()); ++id) {
        check(label[ac.terminal[id]], p[id], "terminal-path");
        check(ac.length[id], int(p[id].size()), "pattern-length");}
    for (int u = 0; u < ac.size(); ++u) {
        int out = -1, best = -1, count = 0;
        check(ac.nodes[u].link, expectedState(label, label[u], u), "longest-proper-suffix-link");
        vector<int> states;
        for (int v = 0; v < ac.size(); ++v) {
            if (suffix(label[u], label[v])) { states.push_back(v); }}
        std::ranges::sort(states, [&](int a, int b) { return label[a].size() > label[b].size(); });
        for (int v : states) { expected[u] += values[v]; expected_strings[u] += strings[v]; }
        for (int id = 0; id < int(p.size()); ++id) {
            if (!suffix(label[u], p[id])) { continue; }
            ++count;
            if (p[id].size() < label[u].size() && int(p[id].size()) > best) { out = ac.terminal[id]; best = int(p[id].size()); }}
        check(ac.nodes[u].out, out, "nearest-proper-output-link");
        check(ac.matchCount(u), count, "suffix-output-count");
        check(ac.forbidden(u), count > 0, "forbidden-state");
        for (int x : {0, 97, 98, 99, 127, 128, 255}) {
            unsigned char c = uint8_t(x);
            int v = expectedState(label, label[u] + char(c)), bad = 0;
            check(ac.step(u, c), v, "arbitrary-state-transition");
            check(ac.sparseStep(u, c), v, "arbitrary-state-sparse-step");
            for (const auto &s : p) { bad += suffix(label[v], s); }
            check(ac.safeStep(u, c), count || bad ? -1 : v, "arbitrary-safe-transition");}}
    check(ac.suffixAggregate(values, std::plus<int>{}), expected, "suffix-add-fold");
    check(ac.suffixAggregate(strings, std::plus<string>{}), expected_strings, "suffix-noncommutative-fold");
    check(ac.safeStep(-1, 0), -1, "rejecting-sink");}
void testText(const AhoCorasick &ac, const vector<string> &p, const string &text) {
    context = "patterns=" + show(p) + " text=" + show(text) + " dense=" + show(ac.dense);
    int n = int(text.size());
    vector<lng> by_pattern(p.size());
    vector<int> by_position(n + 1);
    vector<vector<int>> expected(n + 1), actual(n + 1);
    for (int r = 0; r <= n; ++r) {
        for (int id = 0; id < int(p.size()); ++id) {
            if (suffix(string_view(text).substr(0, r), p[id])) { ++by_pattern[id]; ++by_position[r]; expected[r].push_back(id); }}
        std::ranges::stable_sort(expected[r], [&](int a, int b) { return p[a].size() > p[b].size(); });}
    check(ac.countPatterns(text), by_pattern, "per-pattern-naive");
    check(ac.countPositions(text), by_position, "per-boundary-naive");
    lng total = accumulate(by_pattern.begin(), by_pattern.end(), lng(0));
    check(ac.countMatches(text), total, "total-naive");
    check(ac.avoids(text), total == 0, "forbidden-text-naive");
    auto label = stateWords(ac);
    vector<lng> state_count(ac.size());
    for (int u = 0; u < ac.size(); ++u) {
        for (int r = 0; r <= n; ++r) { state_count[u] += suffix(string_view(text).substr(0, r), label[u]); }}
    check(ac.stateCounts(text), state_count, "all-prefix-occurrences");
    for (int r = 0, u = 0; r <= n; ++r) {
        vector<int> ids;
        check(ac.forEachOutput(u, [&](int id) { ids.push_back(id); return true; }), true, "output-completion");
        check(ids, expected[r], "streaming-output-order");
        check(u, expectedState(label, string_view(text).substr(0, r)), "longest-suffix-state");
        if (r < n) { u = ac.step(u, uint8_t(text[r])); }}
    check(ac.forEachMatch(text, [&](int id, int l, int r) {
        check(text.substr(l, r - l), p[id], "callback-span");
        actual[r].push_back(id);
        return true;}), true, "matches-completion");
    check(actual, expected, "all-matches-naive");
    if (!total) { return; }
    for (lng stop : {lng(1), (total + 1) / 2, total}) {
        lng visited = 0;
        check(ac.forEachMatch(text, [&](int, int, int) { return ++visited < stop; }), false, "callback-cancel");
        check(visited, stop, "callback-cancel-count");}}
void testAutomaton(AhoCorasick &ac, const vector<string> &p, const vector<string> &texts, const vector<string> &label) {
    for (bool dense : {false, true}) {
        ac.build(dense);
        testStructure(ac, p, label);
        for (const auto &s : texts) { testText(ac, p, s); }}
    ac.build(false);
    check(ac.go.capacity(), size_t(0), "sparse-rebuild-releases-dense-storage");
    if (!texts.empty()) { testText(ac, p, texts.back()); }}
void testDictionary(const vector<string> &p, const vector<string> &texts) {
    context = "patterns=" + show(p);
    AhoCorasick ac;
    vector<string> label{""};
    for (int i = 0; i < int(p.size()); ++i) {
        check(ac.add(p[i]), i, "insertion-ID");
        for (int k = 1; k <= int(p[i].size()); ++k) { label.push_back(p[i].substr(0, k)); }}
    std::ranges::sort(label);
    label.erase(unique(label.begin(), label.end()), label.end());
    check(ac.size(), int(label.size()), "trie-state-count");
    ac.build();
    testAutomaton(ac, p, texts, stateWords(ac));}

// Node strings come from the parent chain; every query is checked against the direct oracles in both representations.
void checkTrie(const vector<int> &parent, const string &label, const vector<int> &ends, const vector<string> &texts) {
    int n = int(parent.size());
    vector<string> word(n);
    for (int v = 0; v < n; ++v) {
        for (int u = v; u; u = parent[u]) { word[v].insert(word[v].begin(), label[u]); }}
    vector<string> p;
    for (int u : ends) { p.push_back(word[u]); }
    context = "fromTrie parent=" + show(parent) + " label=" + show(label) + " ends=" + show(ends);
    for (bool dense : {false, true}) {
        auto ac = AhoCorasick::fromTrie(parent, label, ends, dense);
        check(ac.size(), n, "fromTrie-size");
        check(ac.dense, dense, "fromTrie-representation");
        check(ac.go.empty(), !dense || ac.sigma == 0, "fromTrie-sparse-releases-table");
        check(ac.terminal, ends, "fromTrie-terminal-ids");
        for (int v = 1; v < n; ++v) { check(ac.step(parent[v], uint8_t(label[v])), v, "fromTrie-preserves-node-ids"); }
        testStructure(ac, p, word);
        for (const auto &s : texts) { testText(ac, p, s); }}}
// Random labelled tree with shuffled node ids.
void testTrie(int n, int alphabet, int ends_count, const vector<string> &texts) {
    vector<int> id(n), parent(n, -1);
    iota(id.begin(), id.end(), 0);
    std::shuffle(id.begin() + 1, id.end(), rng);
    string label(n, '\0');
    vector<std::set<char>> used(n);
    for (int i = 1; i < n; ++i) {
        int p = id[rng() % i];
        char c;
        do { c = char(rng() % alphabet); } while (used[p].count(c) && int(used[p].size()) < alphabet);
        if (used[p].count(c)) { p = id[0]; c = char(alphabet + i); }
        used[p].insert(c);
        parent[id[i]] = p;
        label[id[i]] = c;}
    vector<int> ends(ends_count);
    for (int &u : ends) { u = int(rng() % n); }
    checkTrie(parent, label, ends, texts);}
void forbiddenDP(const vector<string> &patterns) {
    context = "forbidden-patterns=" + show(patterns);
    AhoCorasick ac(patterns, true);
    vector<lng> dp(ac.size());
    dp[0] = !ac.forbidden(0);
    for (int n = 0; n <= 7; ++n) {
        lng expected = 0;
        for (int mask = 0; mask < 1 << n; ++mask) {
            string s(n, 'a');
            for (int i = 0; i < n; ++i) { s[i] = char(s[i] + ((mask >> i) & 1)); }
            expected += std::ranges::none_of(patterns, [&](const string &p) { return s.find(p) != string::npos; });}
        check(accumulate(dp.begin(), dp.end(), lng(0)), expected, "forbidden-language-DP-naive");
        vector<lng> next(ac.size());
        for (int u = 0; u < ac.size(); ++u) {
            for (unsigned char c : {'a', 'b'}) {
                int v = ac.safeStep(u, c);
                if (v >= 0) { next[v] += dp[u]; }}}
        dp = std::move(next);}}
int invalid(const string &probe) {
    AhoCorasick ac;
    ac.add("ab");
    ac.add("bc");
    if (probe == "unbuilt") { ac.step(0, 0); }
    if (probe == "unbuilt-sparse") { ac.sparseStep(2, 'c'); }
    if (probe == "unbuilt-sink") { ac.safeStep(-1, 0); }
    if (probe == "trie-duplicate-label") { AhoCorasick::fromTrie({-1, 0, 0}, "xaa"); }
    if (probe == "trie-unreachable") { AhoCorasick::fromTrie({-1, 2, 1}, "xab"); }
    if (probe == "trie-parent-range") { AhoCorasick::fromTrie({-1, 3}, "xa"); }
    if (probe == "trie-end-range") { AhoCorasick::fromTrie({-1, 0}, "xa", {2}); }
    if (probe == "trie-label-size") { AhoCorasick::fromTrie({-1, 0}, "x"); }
    ac.build();
    if (probe == "add-built") { ac.add(""); }
    if (probe == "negative-state") { ac.step(-1, 0); }
    if (probe == "large-state") { ac.matchCount(ac.size()); }
    if (probe == "large-sparse-state") { ac.sparseStep(ac.size(), 0); }
    if (probe == "aggregate-size") { ac.suffixAggregate(vector<int>{}, std::plus<int>{}); }
    if (probe == "safe-state") { ac.safeStep(-2, 0); }
    if (probe == "output-state") { ac.forEachOutput(-1, [](int) { return true; }); }
    return 3;}

int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--invalid") { return invalid(argv[2]); }
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    auto candidates = words(limit(1, 2, 2)), texts = words(limit(3, 4, 5));
    for (int mask = 0; mask < 1 << candidates.size(); ++mask) {
        vector<string> p;
        for (int i = 0; i < int(candidates.size()); ++i) {
            if (mask >> i & 1) { p.push_back(candidates[i]); }}
        testDictionary(p, texts);
        forbiddenDP(p);}
    for (const auto &a : candidates) {
        for (const auto &b : candidates) { testDictionary({a, b, a}, texts); }}
    auto randomText = [&](int n, int alphabet) {
        string s(rng() % n, '\0');
        for (char &c : s) { c = char(rng() % alphabet); }
        return s;};
    for (int rep = 0; rep < limit(100, 1200, 6000); ++rep) {
        int alphabet = rep % 3 == 0 ? 256 : rep % 3 == 1 ? 3 : 1;
        vector<string> p(rng() % 12);
        for (auto &s : p) { s = randomText(10, alphabet); }
        if (p.size() >= 2 && rep % 2 == 0) { p.back() = p.front(); }
        testDictionary(p, {randomText(45, alphabet), randomText(45, alphabet)});}
    for (int rep = 0; rep < limit(60, 600, 3000); ++rep) {
        int alphabet = rep % 4 == 0 ? 256 : 2 + rep % 3;
        testTrie(1 + int(rng() % (rep % 5 ? 12 : 40)), alphabet, int(rng() % 6), {randomText(30, alphabet), randomText(30, alphabet)});}
    testTrie(1, 2, 3, {"", string(1, '\0')});
    // Spines a^i and b a^i with a 'c' leaf below every b a^i; leaf depths exceed V * S from m = 13 (temporary dense links).
    auto adversarial = [](int m) {
        vector<int> parent{-1};
        string label = "x";
        for (int i = 1; i <= m; ++i) { parent.push_back(i - 1); label += 'a'; }
        for (int i = 0; i <= m; ++i) { parent.push_back(i ? m + i : 0); label += i ? 'a' : 'b'; }
        for (int i = 0; i <= m; ++i) { parent.push_back(m + 1 + i); label += 'c'; }
        return pair{parent, label};};
    for (int m : {3, 20}) {
        auto [parent, label] = adversarial(m);
        checkTrie(parent, label, {3 * m + 2, m, 2 * m + 1}, {"baaac", "abaaacbaaaaaaaaaaaaaac", string(m, 'a')});}
    vector<string> bytes;
    string all;
    for (int c = 0; c < 256; ++c) { bytes.emplace_back(1, char(c)); all += char(c); }
    bytes.push_back("");
    bytes.push_back(string("\0\377", 2));
    testDictionary(bytes, {"", all, all + all, string("\0\377\0", 3)});
    testDictionary({"he", "she", "his", "hers", "he", "", ""}, {"ushers", "hehe", "shershis"});
    testDictionary({"a", "aa", "aaa", "baaa", "aab", "ab", "b"}, {"baaaaabaaaba", "bbbbbaaaaa", "aaaab"});
    context = "copy-move-clear";
    vector<string> abba{"", "aba", "ba", "aba"};
    AhoCorasick ac(abba, true), copy = ac;
    check(ac.forEachMatch("aba", [&](int, int, int) {
        check(ac.countMatches("aba"), lng(7), "nested-const-count");
        vector<int> root_ids;
        ac.forEachOutput(0, [&](int id) { root_ids.push_back(id); return true; });
        check(root_ids, vector<int>{0}, "nested-const-outputs");
        return true;}), true, "nested-const-callbacks");
    testText(copy, abba, "ababa");
    AhoCorasick moved = std::move(copy);
    testText(moved, abba, "ababa");
    copy.clear();
    copy.add("x");
    copy.build();
    testText(copy, {"x"}, "xx");
    copy = ac;
    testText(copy, abba, "");
    ac.clear();
    ac.build(true);
    testText(ac, {}, "ab");
    int depth = limit(100, 1000, 2500), n = limit(10000, 150000, 600000);
    vector<string> nested{"", ""};
    for (int i = 1; i <= depth; ++i) { nested.emplace_back(i, 'a'); }
    string long_text(n, 'a');
    for (bool dense : {false, true}) {
        AhoCorasick large(nested, dense);
        context = "nested-count-only depth=" + show(depth) + " n=" + show(n);
        vector<lng> expected{n + 1, n + 1};
        for (int i = 1; i <= depth; ++i) { expected.push_back(n - i + 1); }
        check(large.countPatterns(long_text), expected, "large-no-output-materialization");
        check(large.countMatches(long_text), accumulate(expected.begin(), expected.end(), lng(0)), "large-total-exact");
        auto positions = large.countPositions(long_text);
        for (int r = 0; r <= n; ++r) { check(positions[r], min(r, depth) + 2, "large-boundary-counts"); }
        long_text.back() = 'b';
        auto count = large.countPatterns(long_text);
        for (int i = 2; i < int(count.size()); ++i) { check(count[i], expected[i] - 1, "long-failure-fallback"); }
        long_text.back() = 'a';}
    // Caterpillar trie: spine a^i with a 'b' leaf at every spine node; links and counts have closed forms.
    int spine = limit(2000, 50000, 200000);
    vector<int> parent{-1};
    string label = "x";
    for (int i = 1; i <= spine; ++i) {
        parent.push_back(i == 1 ? 0 : 2 * i - 3);
        label += 'a';
        parent.push_back(2 * i - 1);
        label += 'b';}
    context = "caterpillar-fromTrie spine=" + show(spine);
    for (bool dense : {false, true}) {
        auto big = AhoCorasick::fromTrie(parent, label, {2 * spine - 1, 2}, dense);
        for (int i = 2; i <= spine; ++i) {
            check(big.nodes[2 * i - 1].link, 2 * i - 3, "caterpillar-spine-link");
            check(big.nodes[2 * i].link, 2 * i - 2, "caterpillar-leaf-link");}
        string text(spine, 'a');
        text += 'b';
        check(big.countPatterns(text), vector<lng>{1, 1}, "caterpillar-counts");
        check(big.length, vector<int>{spine, 2}, "caterpillar-depths");}
    int adversarial_m = limit(500, 5000, 20000);
    auto [adv_parent, adv_label] = adversarial(adversarial_m);
    context = "adversarial-fromTrie m=" + show(adversarial_m);
    for (bool dense : {false, true}) {
        auto big = AhoCorasick::fromTrie(adv_parent, adv_label, {3 * adversarial_m + 2}, dense);
        check(big.dense, dense, "adversarial-representation");
        for (int i = 1; i <= adversarial_m; ++i) {
            check(big.nodes[i].link, i - 1, "adversarial-a-link");
            check(big.nodes[adversarial_m + 1 + i].link, i, "adversarial-b-link");
            check(big.nodes[2 * adversarial_m + 2 + i].link, 0, "adversarial-leaf-link");}
        string text = "b" + string(adversarial_m, 'a') + "cb";
        check(big.countMatches(text), lng(1), "adversarial-count");}
    cout << "PASS aho seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';}
