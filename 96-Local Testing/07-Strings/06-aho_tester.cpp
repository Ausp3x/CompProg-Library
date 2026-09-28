#include "../../07-Strings/06-aho.hpp"

string mode, context;
ulng seed = 20260928, checks = 0;
std::mt19937_64 rng;
template<class T> string show(const T &x) { std::ostringstream out; out << +x; return out.str(); }
string show(const string &s);
template<class T> string show(const vector<T> &a) {
    string out = "[";
    for (int i = 0; i < int(a.size()); ++i) { out += (i ? "," : "") + show(a[i]); } return out + "]";
}
string show(const string &s) { return show(vector<unsigned char>(s.begin(), s.end())); }
template<class T, class U> void check(const T &actual, const U &expected, string_view operation) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << operation
             << " smallest-known-reproducer=" << context << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1); }
}
int limit(int a, int b, int c) { return mode == "quick" ? a : mode == "full" ? b : c; }
bool suffix(string_view s, string_view t) { return s.size() >= t.size() && s.substr(s.size() - t.size()) == t; }
vector<string> words(int depth) {
    vector<string> a{""}; int start = 0;
    for (int i = 0; i < depth; ++i) {
        int end = int(a.size());
        for (int j = start; j < end; ++j) { for (char c : {'a', 'b'}) { a.push_back(a[j] + c); } } start = end; }
    return a;
}
vector<string> stateWords(const AhoCorasick &ac) {
    vector<string> label(ac.size());
    for (int u = 0; u < ac.size(); ++u) { for (auto [c, v] : ac.nodes[u].next) { label[v] = label[u] + char(c); } }
    return label;
}
int expectedState(const vector<string> &label, string_view s, int skip = -1) {
    int answer = 0;
    for (int u = 1; u < int(label.size()); ++u) {
        if (u != skip && suffix(s, label[u]) && label[u].size() > label[answer].size()) { answer = u; } }
    return answer;
}
void testStructure(const AhoCorasick &ac, const vector<string> &p) {
    auto label = stateWords(ac); check(ac.patterns(), int(p.size()), "pattern-size");
    vector<int> values(ac.size()), expected(ac.size()); vector<string> strings(ac.size()), expected_strings(ac.size());
    for (int u = 0; u < ac.size(); ++u) { values[u] = 2 * u - 4; strings[u] = std::to_string(u) + "/"; }
    for (int id = 0; id < int(p.size()); ++id) {
        check(label[ac.terminal[id]], p[id], "terminal-path"); check(ac.length[id], int(p[id].size()), "pattern-length"); }
    for (int u = 0; u < ac.size(); ++u) {
        int f = expectedState(label, label[u], u), out = -1, best = -1, count = 0;
        check(ac.nodes[u].link, f, "longest-proper-suffix-link");
        vector<int> states;
        for (int v = 0; v < ac.size(); ++v) { if (suffix(label[u], label[v])) { states.push_back(v); } }
        sort(states.begin(), states.end(), [&](int a, int b) { return label[a].size() > label[b].size(); });
        for (int v : states) { expected[u] += values[v]; expected_strings[u] += strings[v]; }
        for (int id = 0; id < int(p.size()); ++id) { if (suffix(label[u], p[id])) {
            ++count;
            if (p[id].size() < label[u].size() && int(p[id].size()) > best) { out = ac.terminal[id]; best = int(p[id].size()); } } }
        check(ac.nodes[u].out, out, "nearest-proper-output-link"); check(ac.matchCount(u), count, "suffix-output-count");
        check(ac.forbidden(u), count > 0, "forbidden-state");
        for (unsigned char c : {0, 97, 98, 99, 127, 128, 255}) {
            int v = expectedState(label, label[u] + char(c)); check(ac.step(u, c), v, "arbitrary-state-transition");
            int bad = 0; for (const auto &s : p) { bad += suffix(label[v], s); }
            check(ac.safeStep(u, c), count || bad ? -1 : v, "arbitrary-safe-transition"); } }
    check(ac.suffixAggregate(values, std::plus<int>{}), expected, "suffix-add-fold");
    check(ac.suffixAggregate(strings, std::plus<string>{}), expected_strings, "suffix-noncommutative-fold");
    check(ac.safeStep(-1, 0), -1, "rejecting-sink");
}
void testText(const AhoCorasick &ac, const vector<string> &p, const string &text) {
    context = "patterns=" + show(p) + " text=" + show(text) + " dense=" + show(ac.dense);
    vector<lng> by_pattern(p.size()); vector<int> by_position(text.size() + 1); vector<vector<int>> expected(text.size() + 1);
    for (int r = 0; r <= int(text.size()); ++r) {
        for (int id = 0; id < int(p.size()); ++id) {
            if (suffix(string_view(text).substr(0, r), p[id])) { ++by_pattern[id]; ++by_position[r]; expected[r].push_back(id); } }
        sort(expected[r].begin(), expected[r].end(), [&](int a, int b) {
            return p[a].size() == p[b].size() ? a < b : p[a].size() > p[b].size(); }); }
    check(ac.countPatterns(text), by_pattern, "per-pattern-naive"); check(ac.countPositions(text), by_position, "per-boundary-naive");
    lng total = accumulate(by_pattern.begin(), by_pattern.end(), lng(0));
    check(ac.countMatches(text), total, "total-naive"); check(ac.avoids(text), total == 0, "forbidden-text-naive");
    auto label = stateWords(ac); vector<lng> state_count(ac.size());
    for (int u = 0; u < ac.size(); ++u) {
        for (int r = 0; r <= int(text.size()); ++r) { state_count[u] += suffix(string_view(text).substr(0, r), label[u]); } }
    check(ac.stateCounts(text), state_count, "all-prefix-occurrences");
    vector<vector<int>> actual(text.size() + 1); int u = 0;
    for (int r = 0; r <= int(text.size()); ++r) {
        vector<int> ids;
        check(ac.forEachOutput(u, [&](int id) { ids.push_back(id); return true; }), true, "output-completion");
        check(ids, expected[r], "streaming-output-order");
        check(u, expectedState(label, string_view(text).substr(0, r)), "longest-suffix-state");
        if (r < int(text.size())) { u = ac.step(u, static_cast<unsigned char>(text[r])); } }
    check(ac.forEachMatch(text, [&](int id, int l, int r) {
        check(text.substr(l, r - l), p[id], "callback-span"); actual[r].push_back(id); return true;
    }), true, "matches-completion"); check(actual, expected, "all-matches-naive");
    if (total) {
        for (lng stop : {lng(1), (total + 1) / 2, total}) {
            lng visited = 0;
            check(ac.forEachMatch(text, [&](int, int, int) { return ++visited < stop; }), false, "callback-cancel");
            check(visited, stop, "callback-cancel-count"); } }
}
void testDictionary(const vector<string> &p, const vector<string> &texts) {
    context = "patterns=" + show(p); AhoCorasick ac;
    for (int i = 0; i < int(p.size()); ++i) { check(ac.add(p[i]), i, "insertion-ID"); }
    for (bool dense : {false, true}) {
        ac.build(dense); testStructure(ac, p);
        for (const auto &s : texts) { testText(ac, p, s); } }
    ac.build(false); check(ac.go.capacity(), size_t(0), "sparse-rebuild-releases-dense-storage");
    if (!texts.empty()) { testText(ac, p, texts.back()); }
}
void forbiddenDP(const vector<string> &patterns) {
    context = "forbidden-patterns=" + show(patterns); AhoCorasick ac(patterns, true);
    vector<lng> dp(ac.size()); dp[0] = !ac.forbidden(0);
    for (int n = 0; n <= 7; ++n) {
        lng expected = 0;
        for (int mask = 0; mask < 1 << n; ++mask) {
            string s(n, 'a'); for (int i = 0; i < n; ++i) { s[i] += (mask >> i) & 1; }
            bool good = true; for (const auto &p : patterns) { good &= s.find(p) == string::npos; } expected += good; }
        check(accumulate(dp.begin(), dp.end(), lng(0)), expected, "forbidden-language-DP-naive");
        vector<lng> next(ac.size());
        for (int u = 0; u < ac.size(); ++u) { for (unsigned char c : {'a', 'b'}) {
            int v = ac.safeStep(u, c); if (v >= 0) { next[v] += dp[u]; } } } dp = std::move(next); }
}
int main(int argc, char **argv) {
    if (argc == 3 && string(argv[1]) == "--invalid") {
        string probe = argv[2]; AhoCorasick ac;
        if (probe == "unbuilt") { ac.step(0, 0); }
        if (probe == "unbuilt-sink") { ac.safeStep(-1, 0); }
        ac.build();
        if (probe == "add-built") { ac.add(""); }
        if (probe == "negative-state") { ac.step(-1, 0); }
        if (probe == "large-state") { ac.matchCount(1); }
        if (probe == "aggregate-size") { ac.suffixAggregate(vector<int>{}, std::plus<int>{}); }
        if (probe == "safe-state") { ac.safeStep(-2, 0); } return 3; }
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); } }
    rng.seed(seed); auto candidates = words(limit(1, 2, 2)), texts = words(limit(3, 4, 5));
    for (int mask = 0; mask < 1 << candidates.size(); ++mask) {
        vector<string> p; for (int i = 0; i < int(candidates.size()); ++i) { if (mask >> i & 1) { p.push_back(candidates[i]); } }
        testDictionary(p, texts); forbiddenDP(p); }
    for (const auto &a : candidates) { for (const auto &b : candidates) { testDictionary({a, b, a}, texts); } }
    for (int rep = 0; rep < limit(100, 1200, 6000); ++rep) {
        vector<string> p(rng() % 12); int alphabet = rep % 3 == 0 ? 256 : rep % 3 == 1 ? 3 : 1;
        for (auto &s : p) { s.resize(rng() % 10); for (char &c : s) { c = char(rng() % alphabet); } }
        if (p.size() >= 2 && rep % 2 == 0) { p.back() = p.front(); }
        vector<string> random_texts(2); for (auto &s : random_texts) { s.resize(rng() % 45); for (char &c : s) { c = char(rng() % alphabet); } }
        testDictionary(p, random_texts); }
    vector<string> bytes; string all;
    for (int c = 0; c < 256; ++c) { bytes.emplace_back(1, char(c)); all += char(c); }
    bytes.push_back(""); bytes.push_back(string("\0\377", 2)); testDictionary(bytes, {"", all, all + all, string("\0\377\0", 3)});
    testDictionary({"he", "she", "his", "hers", "he", "", ""}, {"ushers", "hehe", "shershis"});
    testDictionary({"a", "aa", "aaa", "baaa", "aab", "ab", "b"}, {"baaaaabaaaba", "bbbbbaaaaa", "aaaab"});
    context = "copy-move-clear"; AhoCorasick ac(vector<string>{"", "aba", "ba", "aba"}, true), copy = ac;
    check(ac.forEachMatch("aba", [&](int, int, int) {
        check(ac.countMatches("aba"), lng(7), "nested-const-count");
        vector<int> root_ids; ac.forEachOutput(0, [&](int id) { root_ids.push_back(id); return true; });
        check(root_ids, vector<int>{0}, "nested-const-outputs"); return true;
    }), true, "nested-const-callbacks");
    testText(copy, {"", "aba", "ba", "aba"}, "ababa"); AhoCorasick moved = std::move(copy);
    testText(moved, {"", "aba", "ba", "aba"}, "ababa"); copy.clear(); copy.add("x"); copy.build(); testText(copy, {"x"}, "xx");
    copy = ac; testText(copy, {"", "aba", "ba", "aba"}, ""); ac.clear(); ac.build(true); testText(ac, {}, "ab");
    int depth = limit(100, 1000, 2500), n = limit(10000, 150000, 600000); vector<string> nested{"", ""};
    for (int i = 1; i <= depth; ++i) { nested.emplace_back(i, 'a'); }
    string long_text(n, 'a');
    for (bool dense : {false, true}) {
        AhoCorasick large(nested, dense); context = "nested-count-only depth=" + show(depth) + " n=" + show(n);
        vector<lng> expected{n + 1, n + 1}; for (int i = 1; i <= depth; ++i) { expected.push_back(n - i + 1); }
        check(large.countPatterns(long_text), expected, "large-no-output-materialization");
        check(large.countMatches(long_text), accumulate(expected.begin(), expected.end(), lng(0)), "large-total-exact");
        auto positions = large.countPositions(long_text);
        for (int r = 0; r <= n; ++r) { check(positions[r], min(r, depth) + 2, "large-boundary-counts"); }
        long_text.back() = 'b'; auto count = large.countPatterns(long_text);
        for (int i = 2; i < int(count.size()); ++i) { check(count[i], expected[i] - 1, "long-failure-fallback"); } long_text.back() = 'a'; }
    cout << "PASS aho seed=" << seed << " mode=" << mode << " checks=" << checks << '\n';
}
