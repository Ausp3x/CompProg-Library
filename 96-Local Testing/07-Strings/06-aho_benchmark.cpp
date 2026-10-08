#include "../../07-Strings/06-aho.hpp"

using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260928;
    std::mt19937_64 rng(seed);
    struct Work { string name; int k, m, n, alphabet; };
    for (auto [name, k, m, n, alphabet] : vector<Work>{{"small", 20, 5, 10000, 26},
            {"dna", 500, 12, 500000, 4}, {"letters", 500, 12, 500000, 26}, {"bytes", 2000, 20, 500000, 256},
            {"nested", 500, 500, 300000, 1}, {"trie-letters", 20000, 12, 500000, 26}, {"trie-caterpillar", 1, 200000, 500000, 2}, {"trie-adversarial", 1, 20000, 500000, 3}}) {
        bool nested = name == "nested", external = name.starts_with("trie"), caterpillar = name == "trie-caterpillar", adversarial = name == "trie-adversarial";
        vector<string> patterns;
        string text(n, 'a');
        for (char &c : text) { c = char('a' + rng() % alphabet); }
        for (int i = 0; i < k; ++i) {
            string s(nested ? i + 1 : m, 'a');
            if (!nested) {
                for (char &c : s) { c = char('a' + rng() % alphabet); }}
            patterns.push_back(s);}
        // External tries: the patterns' trie as a parent/label tree; the caterpillar adds a 'b' leaf below every spine node.
        // Adversarial: spines a^i and b a^i with a 'c' leaf below every b a^i, so each sparse leaf link walks a^i down to the root.
        vector<int> parent{-1}, ends;
        string label = "x";
        if (caterpillar) {
            for (int i = 1; i <= m; ++i) {
                parent.push_back(i == 1 ? 0 : 2 * i - 3);
                label += 'a';
                parent.push_back(2 * i - 1);
                label += 'b';}
            patterns = {string(m, 'a'), "ab"};
            ends = {2 * m - 1, 2};}
        else if (adversarial) {
            for (int i = 1; i <= m; ++i) { parent.push_back(i - 1); label += 'a'; }
            for (int i = 0; i <= m; ++i) { parent.push_back(i ? m + i : 0); label += i ? 'a' : 'b'; }
            for (int i = 0; i <= m; ++i) { parent.push_back(m + 1 + i); label += 'c'; }
            patterns = {'b' + string(m, 'a') + 'c'};
            ends = {3 * m + 2};}
        else if (external) {
            AhoCorasick trie(patterns);
            parent.resize(trie.size());
            label.resize(trie.size());
            for (int u = 0; u < trie.size(); ++u) {
                for (auto [c, v] : trie.nodes[u].next) { parent[v] = u; label[v] = char(c); }}
            ends = trie.terminal;}
        lng expected = 0;
        if (nested) { expected = lng(k) * (n + 1) - lng(k) * (k + 1) / 2; }
        else {
            for (const auto &s : patterns) {
                for (size_t pos = text.find(s); pos != string::npos; pos = text.find(s, pos + 1)) { ++expected; }}}
        for (bool dense : {false, true}) {
            vector<double> setup, scan, total;
            int states = 0;
            size_t table = 0;
            for (int rep = 0; rep < 6; ++rep) {
                auto start = Clock::now();
                AhoCorasick ac = external ? AhoCorasick::fromTrie(parent, label, ends, dense) : AhoCorasick(patterns, dense);
                auto ready = Clock::now();
                lng actual = ac.countMatches(text);
                auto done = Clock::now();
                states = ac.size();
                table = ac.go.size() * sizeof(int);
                if (actual != expected) {
                    cerr << "FAIL benchmark seed=" << seed << " workload=" << name << " dense=" << dense
                         << " expected=" << expected << " actual=" << actual << '\n';
                    return 1;}
                if (rep) {
                    setup.push_back(ms(start, ready));
                    scan.push_back(ms(ready, done));
                    total.push_back(ms(start, done));}}
            sort(setup.begin(), setup.end());
            sort(scan.begin(), scan.end());
            sort(total.begin(), total.end());
            cout << "{\"workload\":\"" << name << "\",\"dense\":" << (dense ? "true" : "false")
                 << ",\"seed\":" << seed << ",\"patterns\":" << patterns.size() << ",\"text_length\":" << n
                 << ",\"alphabet\":" << alphabet << ",\"states\":" << states << ",\"dense_table_bytes\":" << table
                 << ",\"expected_matches\":" << expected << ",\"build_ms\":" << setup[2] << ",\"scan_ms\":" << scan[2]
                 << ",\"total_ms\":" << total[2] << "}\n";}}}
