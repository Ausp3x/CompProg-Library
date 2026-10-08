#include "../../07-Strings/04-trie.hpp"

using Clock = std::chrono::steady_clock;
lng elapsed(Clock::time_point a, Clock::time_point b) { return std::chrono::duration_cast<std::chrono::nanoseconds>(b - a).count(); }
// One timed insert/query/erase pipeline; the checksum is compared across methods outside the timing.
template<class T> array<lng, 4> pipeline(const vector<string> &keys, const vector<string> &queries, lng &checksum) {
    auto start = Clock::now();
    T trie;
    for (const auto &s : keys) { trie.insert(s); }
    auto built = Clock::now();
    lng sum = 0;
    for (const auto &q : queries) { sum += trie.count(q) + 3 * trie.countPrefix(q) + trie.longestPrefix(q); }
    auto queried = Clock::now();
    for (const auto &s : keys) { sum += trie.erase(s); }
    auto erased = Clock::now();
    checksum = sum + lng(trie.size());
    return {elapsed(start, built), elapsed(built, queried), elapsed(queried, erased), elapsed(start, erased)};}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261008;
    int reps = argc > 2 ? std::stoi(argv[2]) : 5;
    std::mt19937_64 rng(seed);
    cout << "[";
    bool first = true;
    for (auto [n, len, sigma] : vector<tuple<int, int, int>>{{1000, 8, 26}, {200000, 8, 26}, {200000, 20, 4}, {20000, 200, 2}}) {
        vector<string> keys(n), queries(n);
        for (auto &s : keys) {
            s.resize(1 + rng() % len);
            for (char &c : s) { c = char('a' + rng() % sigma); }}
        for (int i = 0; i < n; ++i) { queries[i] = i % 2 ? keys[rng() % n].substr(0, 1 + rng() % len) : keys[rng() % n]; }
        array<vector<array<lng, 4>>, 2> samples;
        array<lng, 2> sums{};
        for (int run = 0; run <= reps; ++run) {
            for (int slot = 0; slot < 2; ++slot) {
                int method = (run + slot) % 2;
                auto t = method == 0 ? pipeline<Trie>(keys, queries, sums[0]) : pipeline<TrieDense<26, 'a'>>(keys, queries, sums[1]);
                if (run) { samples[method].push_back(t); }}
            if (sums[0] != sums[1]) { cerr << "FAIL checksum mismatch n=" << n << '\n'; return 1; }}
        for (int method = 0; method < 2; ++method) {
            cout << (first ? "" : ",") << "\n{\"method\":\"" << (method ? "dense26" : "map") << "\",\"n\":" << n << ",\"max_length\":" << len
                 << ",\"sigma\":" << sigma << ",\"checksum\":" << sums[method] << ",\"median_ms\":[";
            first = false;
            for (int k = 0; k < 4; ++k) {
                vector<lng> v;
                for (auto &t : samples[method]) { v.push_back(t[k]); }
                sort(v.begin(), v.end());
                cout << (k ? "," : "") << double(v[v.size() / 2]) / 1e6;}
            cout << "]}";}}
    cout << "\n]\n";}
