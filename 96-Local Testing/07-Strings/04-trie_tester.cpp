#include "../../07-Strings/04-trie.hpp"

struct ByteLess {
    bool operator()(const string &a, const string &b) const {
        return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
            [](unsigned char x, unsigned char y) { return x < y; });}
};
using Dict = map<string, lng, ByteLess>;
ulng test_seed = 0;
lng checks = 0;
string context;
const Dict *current = nullptr;
string hex(string_view s) {
    string out = "0x";
    for (unsigned char c : s) { out += "0123456789abcdef"[c >> 4]; out += "0123456789abcdef"[c & 15]; }
    return out;}
void check(bool ok, const string &op, const string &expected = "true", const string &actual = "false") {
    ++checks;
    if (ok) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual << '\n';
    if (current) {
        for (const auto &[s, k] : *current) { cerr << hex(s) << ':' << k << ' '; }
        cerr << '\n';}
    std::exit(1);}
void expectEqual(lng actual, lng expected, const string &op) {
    check(actual == expected, op, std::to_string(expected), std::to_string(actual));}
vector<pair<string, lng>> expectedItems(const Dict &ref, string_view prefix) {
    vector<pair<string, lng>> out;
    for (const auto &[s, k] : ref) {
        if (s.starts_with(prefix)) { out.emplace_back(s, k); }}
    return out;}
template<class T> vector<pair<int, int>> children(const T &trie, int u) {
    vector<pair<int, int>> out;
    if constexpr (std::is_same_v<T, Trie>) {
        for (auto [c, v] : trie.nodes[u].next) { out.emplace_back(c, v); }}
    else {
        for (int c = 0; c < int(trie.nodes[u].next.size()); ++c) {
            if (trie.nodes[u].next[c]) { out.emplace_back(c, trie.nodes[u].next[c]); }}}
    return out;}
template<class T> void audit(const T &trie, const Dict &ref, string_view prefix = {}) {
    current = &ref;
    lng total = 0, count = 0;
    for (const auto &[s, k] : ref) {
        total += k;
        if (s.starts_with(prefix)) { count += k; }}
    expectEqual(trie.size(), total, "size"); check(trie.empty() == ref.empty(), "empty");
    expectEqual(trie.countPrefix(prefix), count, "countPrefix " + hex(prefix));
    auto it = ref.find(string(prefix));
    lng terminal = it == ref.end() ? 0 : it->second;
    expectEqual(trie.count(prefix), terminal, "count " + hex(prefix));
    check(trie.search(prefix) == bool(terminal), "search " + hex(prefix));
    int u = trie.findNode(prefix), w = 0;
    check((u < 0) == (count == 0 && !prefix.empty()), "findNode presence " + hex(prefix));
    if (u >= 0) {
        expectEqual(trie.nodes[u].terminal, terminal, "findNode terminal");
        expectEqual(trie.nodes[u].pass, count, "findNode pass");}
    for (char c : prefix) {
        if (w >= 0) { w = trie.step(w, c); }}
    expectEqual(w, u, "step walk equals findNode");
    vector<pair<int, lng>> stored, visited;
    for (int len = 0; len <= int(prefix.size()); ++len) {
        auto jt = ref.find(string(prefix.substr(0, len)));
        if (jt != ref.end()) { stored.emplace_back(len, jt->second); }}
    check(trie.forEachPrefixOf(prefix, [&](int len, lng k) { visited.emplace_back(len, k); return true; }), "forEachPrefixOf completes");
    check(visited == stored, "forEachPrefixOf " + hex(prefix));
    expectEqual(trie.longestPrefix(prefix), stored.empty() ? -1 : stored.back().first, "longestPrefix " + hex(prefix));
    if (!stored.empty()) {
        int calls = 0;
        check(!trie.forEachPrefixOf(prefix, [&](int, lng) { return ++calls < 1; }), "forEachPrefixOf cancel");
        expectEqual(calls, 1, "forEachPrefixOf cancel calls");}
    auto expected = expectedItems(ref, prefix);
    vector<pair<string, lng>> actual;
    check(trie.forEach(prefix, [&](string_view word, lng k) { actual.emplace_back(word, k); return true; }), "completed enumeration");
    check(actual == expected, "ordered enumeration prefix=" + hex(prefix));
    current = nullptr;}
template<class T> void structure(const T &trie) {
    vector<int> seen(trie.nodes.size()), stack{0};
    seen[0] = 1;
    while (!stack.empty()) {
        int u = stack.back();
        stack.pop_back();
        lng sum = trie.nodes[u].terminal;
        for (auto [c, v] : children(trie, u)) {
            check(0 < v && v < int(seen.size()), "edge index");
            check(!seen[v] && trie.nodes[v].pass > 0, "unique live child");
            seen[v] = 1; sum += trie.nodes[v].pass; stack.push_back(v);}
        expectEqual(trie.nodes[u].pass, sum, "subtree multiplicity invariant");}
    for (int u : trie.free) {
        check(0 < u && u < int(seen.size()) && !seen[u], "unique free slot");
        seen[u] = 1;
        check(children(trie, u).empty() && !trie.nodes[u].pass && !trie.nodes[u].terminal, "cleared free slot");}
    for (int x : seen) { check(x == 1, "every slot accounted for"); }}
template<class T> void exhaustive(const string &mode, const string &label) {
    vector<string> words{"", "a", "b"};
    if (mode != "quick") { words.insert(words.end(), {"aa", "ab", "ba", "bb"}); }
    int bound = 1;
    for (int i = 0; i < int(words.size()); ++i) { bound *= 3; }
    for (int mask = 0; mask < bound; ++mask) {
        T trie;
        Dict ref;
        int x = mask;
        context = label + " exhaustive mask=" + std::to_string(mask);
        for (const string &s : words) {
            int k = x % 3;
            x /= 3;
            if (k) { trie.insert(s, k); ref[s] = k; }}
        for (const string &s : words) { audit(trie, ref, s); audit(trie, ref, s + "b"); }
        audit(trie, ref, "missing"); structure(trie);
        for (const string &s : words) {
            lng k = ref.contains(s) ? ref.at(s) : 0;
            check(!trie.erase(s, k + 1), "insufficient erase");
            audit(trie, ref);
            if (k) {
                check(trie.erase(s), "erase one");
                if (!--ref[s]) { ref.erase(s); }
                audit(trie, ref);}
            if (ref.contains(s)) { check(trie.erase(s, ref[s]), "erase remainder"); ref.erase(s); }}
        structure(trie); check(trie.empty(), "fully erased");}
    cout << "PASS " << label << " exhaustive multiset/map oracle states=" << bound << '\n';}
template<class T> void special(const string &mode, const string &label) {
    T trie;
    Dict ref;
    context = label + " full-byte dictionary";
    trie.insert("", 3); ref[""] = 3;
    for (int c = 255; c >= 0; --c) {
        string s(1, char(c));
        trie.insert(s, c + 1); ref[s] = c + 1;
        s += char(255 - c);
        trie.insert(s); ref[s] = 1;}
    audit(trie, ref); structure(trie);
    for (int c = 0; c < 256; c += 15) { audit(trie, ref, string(1, char(c)) + char(255 - c) + "x"); }
    auto expected = expectedItems(ref, {});
    for (int stop : {1, 2, int(expected.size()) / 2, int(expected.size())}) {
        int calls = 0;
        check(!trie.forEach([&](string_view s, lng k) {
            check(expected[calls] == pair<string, lng>{string(s), k}, "cancelled prefix order");
            return ++calls < stop;}), "cancellation false including last");
        expectEqual(calls, stop, "cancellation calls");}
    int nested = 0;
    check(trie.forEach(string(1, char(255)), [&](string_view s, lng k) {
        expectEqual(trie.count(s), k, "nested count");
        trie.forEach(s, [&](string_view, lng) { ++nested; return true; });
        return true;}), "nested reads");
    check(nested > 0, "nested callbacks occurred");
    bool caught = false;
    try { trie.forEach([](string_view, lng) -> bool { throw 17; }); }
    catch (int x) { caught = x == 17; }
    check(caught, "callback exception propagation"); audit(trie, ref);
    T copy = trie;
    copy.insert("copy"); audit(trie, ref);
    T moved = std::move(copy);
    copy.clear(); check(copy.empty(), "moved-from clear");
    expectEqual(moved.count("copy"), 1, "move destination");
    moved = trie; audit(moved, ref);
    trie.clear(); check(trie.empty(), "clear"); structure(trie);

    context = label + " INT64_MAX multiplicity";
    lng limit = std::numeric_limits<lng>::max();
    trie.insert("", limit - 1); trie.insert("a");
    expectEqual(trie.size(), limit, "maximum total");
    expectEqual(trie.count(""), limit - 1, "maximum empty count");
    check(!trie.erase("", limit), "insufficient maximum erase");
    check(trie.erase("", limit - 1) && trie.erase("a"), "maximum erase");
    trie.insert("x", limit); expectEqual(trie.countPrefix("x"), limit, "maximum terminal");
    check(trie.erase("x", limit), "maximum terminal erase"); structure(trie);

    int n = (mode == "quick" ? 2000 : mode == "full" ? 100000 : 300000) / (std::is_same_v<T, Trie> ? 1 : 10);
    context = label + " deep key n=" + std::to_string(n);
    string deep(n, '\0');
    trie.insert(deep, 2);
    int slots = int(trie.nodes.size()), calls = 0;
    check(trie.forEach([&](string_view s, lng k) { check(s == deep && k == 2, "deep enumeration"); ++calls; return true; }), "deep completion");
    expectEqual(calls, 1, "deep callback count");
    expectEqual(trie.longestPrefix(deep + "x"), n, "deep longestPrefix");
    check(trie.erase(deep, 2), "deep erase"); structure(trie);
    expectEqual(lng(trie.free.size()), n, "deep erase frees every node");
    int last = trie.free.back(), u = trie.newNode();
    expectEqual(u, last, "newNode reuses the last freed slot");
    trie.free.push_back(u);
    for (int rep = 0; rep < 3; ++rep) {
        fill(deep.begin(), deep.end(), char(rep + 1));
        trie.insert(deep);
        expectEqual(lng(trie.nodes.size()), slots, "reused slots");
        check(trie.erase(deep), "reused erase");}
    structure(trie);
    T fresh;
    expectEqual(fresh.newNode(), 1, "newNode appends without free slots");
    expectEqual(lng(fresh.nodes.size()), 2, "newNode appended one node");
    fresh.free.push_back(1); structure(fresh);
    cout << "PASS " << label << " full byte/NUL/empty/count bounds/callback/copy/move/deep/reuse n=" << n << '\n';}
template<class T> void randomCases(const string &mode, const string &label, int sigma, char base, int queries_outside) {
    std::mt19937_64 rng(test_seed);
    T trie;
    Dict ref;
    vector<string> pool{""};
    for (int i = 0; i < 400; ++i) {
        string s(int(rng() % 21), '\0');
        for (char &c : s) { c = char(base + int(rng() % (i < 200 ? min(sigma, 4) : sigma))); }
        pool.push_back(s);}
    int steps = mode == "quick" ? 1000 : mode == "full" ? 7000 : 40000;
    for (int step = 0; step < steps; ++step) {
        string s = pool[rng() % pool.size()];
        lng k = lng(rng() % 5) + 1;
        int op = int(rng() % 4);
        context = label + " random step=" + std::to_string(step) + " op=" + std::to_string(op) + " key=" + hex(s) + " k=" + std::to_string(k);
        current = &ref;
        if (op < 2) { trie.insert(s, k); ref[s] += k; }
        else if (op == 2) {
            bool ok = ref.contains(s) && ref.at(s) >= k;
            check(trie.erase(s, k) == ok, "erase status");
            if (ok && !(ref[s] -= k)) { ref.erase(s); }}
        else { s.resize(rng() % (s.size() + 1)); }
        audit(trie, ref, s);
        if (queries_outside) {
            string t = s;
            t.insert(t.begin() + int(rng() % (t.size() + 1)), char(base + sigma + int(rng() % queries_outside)));
            audit(trie, ref, t);
            check(!trie.erase(t), "outside-alphabet erase");}
        if (step % 79 == 0) { audit(trie, ref); structure(trie); }}
    cout << "PASS " << label << " seeded mutation/map oracle operations=" << steps << '\n';}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; }}
    if (!invalid.empty()) {
        Trie trie;
        TrieDense<2, 'a'> dense;
        if (invalid == "insert-zero") { trie.insert("a", 0); }
        if (invalid == "insert-negative") { trie.insert("a", -1); }
        if (invalid == "insert-overflow") { trie.insert("", std::numeric_limits<lng>::max()); trie.insert("a"); }
        if (invalid == "erase-zero") { trie.erase("a", 0); }
        if (invalid == "erase-negative") { trie.erase("a", -1); }
        if (invalid == "dense-symbol-high") { dense.insert("abc"); }
        if (invalid == "dense-symbol-low") { dense.insert("a`"); }
        if (invalid == "dense-overflow") { dense.insert("a", std::numeric_limits<lng>::max()); dense.insert("b"); }
        return 0;}
    exhaustive<Trie>(mode, "map"); exhaustive<TrieDense<>>(mode, "dense26"); exhaustive<TrieDense<2, 'a'>>(mode, "dense2");
    special<Trie>(mode, "map"); special<TrieDense<256, 0>>(mode, "dense256");
    randomCases<Trie>(mode, "map", 256, 0, 0);
    randomCases<TrieDense<256, 0>>(mode, "dense256", 256, 0, 0);
    randomCases<TrieDense<4, 'a'>>(mode, "dense4", 4, 'a', 3);
    cout << "PASS trie seed=" << test_seed << " checks=" << checks << '\n';}
