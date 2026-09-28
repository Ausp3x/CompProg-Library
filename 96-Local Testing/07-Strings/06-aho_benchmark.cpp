#include "../../07-Strings/06-aho.hpp"

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260928; std::mt19937_64 rng(seed);
    struct Work { string name; int k, m, n, alphabet; };
    for (auto [name, k, m, n, alphabet] : vector<Work>{{"small", 20, 5, 10000, 26},
            {"dna", 500, 12, 500000, 4}, {"letters", 500, 12, 500000, 26},
            {"bytes", 2000, 20, 500000, 256}, {"nested", 500, 500, 300000, 1}}) {
        vector<string> patterns; string text(n, 'a');
        for (char &c : text) { c = char(rng() % alphabet); }
        for (int i = 0; i < k; ++i) {
            string s(name == "nested" ? i + 1 : m, '\0');
            if (name != "nested") { for (char &c : s) { c = char(rng() % alphabet); } } patterns.push_back(s); }
        lng expected = 0;
        if (name == "nested") { expected = lng(k) * (n + 1) - lng(k) * (k + 1) / 2; }
        else { for (const auto &s : patterns) {
            size_t pos = 0;
            while ((pos = text.find(s, pos)) != string::npos) { ++expected; ++pos; } } }
        for (bool dense : {false, true}) {
            vector<double> setup, scan, total; int states = 0; size_t table = 0;
            for (int rep = 0; rep < 6; ++rep) {
                auto start = std::chrono::steady_clock::now(); AhoCorasick ac(patterns, dense);
                auto ready = std::chrono::steady_clock::now(); lng actual = ac.countMatches(text);
                auto done = std::chrono::steady_clock::now(); states = ac.size(); table = ac.go.size() * sizeof(int);
                if (actual != expected) {
                    cerr << "FAIL benchmark seed=" << seed << " workload=" << name << " dense=" << dense
                         << " expected=" << expected << " actual=" << actual << '\n'; return 1; }
                if (rep) {
                    setup.push_back(std::chrono::duration<double, std::milli>(ready - start).count());
                    scan.push_back(std::chrono::duration<double, std::milli>(done - ready).count());
                    total.push_back(std::chrono::duration<double, std::milli>(done - start).count()); } }
            sort(setup.begin(), setup.end()); sort(scan.begin(), scan.end()); sort(total.begin(), total.end());
            cout << "{\"workload\":\"" << name << "\",\"dense\":" << (dense ? "true" : "false")
                 << ",\"seed\":" << seed << ",\"patterns\":" << k << ",\"text_length\":" << n
                 << ",\"alphabet\":" << alphabet << ",\"states\":" << states << ",\"dense_table_bytes\":" << table
                 << ",\"expected_matches\":" << expected << ",\"build_ms\":" << setup[2] << ",\"scan_ms\":" << scan[2]
                 << ",\"total_ms\":" << total[2] << "}\n"; } }
}
