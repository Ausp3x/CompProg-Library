#include "../../07-Strings/07-suffixarray.hpp"

using Clock = std::chrono::steady_clock;
// Rank-weighted digest of sa and lcp; compared across methods outside the timing.
lng digest(const SuffixArray<int> &index) {
    lng res = 0;
    for (int i = 0; i < index.size(); ++i) { res = res * 1000003 + index.sa[i] + i; }
    for (int x : index.lcp) { res = res * 998244353 + x; }
    return res;}
lng digestSa(const vector<int> &sa) {
    lng res = 0;
    for (int i = 0; i < int(sa.size()); ++i) { res = res * 1000003 + sa[i] + i; }
    return res;}
struct Workload { string name; vector<vector<int>> texts; vector<string> bytes; bool threshold = false; };
lng runSais(const Workload &w) {
    lng res = 0;
    for (auto &s : w.texts) { res ^= digest(SuffixArray<int>(s, false)); }
    return res;}
lng runBytes(const Workload &w) {
    lng res = 0;
    for (auto &s : w.bytes) { res ^= digest(SuffixArray<int>(string_view(s), false)); }
    return res;}
lng runDoubling(const Workload &w) {
    lng res = 0;
    for (auto &s : w.texts) { res ^= digest(SuffixArray<int>::doubling(s, false)); }
    return res;}
// Code-range threshold: SA-IS over shifted codes (buckets of size max - min + 1) against SA-IS through compressAlphabet.
lng runShift(const Workload &w) {
    lng res = 0;
    for (auto &s : w.texts) {
        auto [lo, hi] = std::minmax_element(s.begin(), s.end());
        vector<int> codes(s.size());
        for (int i = 0; i < int(s.size()); ++i) { codes[i] = s[i] - *lo; }
        res ^= digestSa(sais(codes, *hi - *lo));}
    return res;}
lng runCompress(const Workload &w) {
    lng res = 0;
    for (auto &s : w.texts) { res ^= digestSa(sais(s)); }
    return res;}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261019;
    int reps = argc > 2 ? std::stoi(argv[2]) : 5;
    std::mt19937_64 rng(seed);
    vector<Workload> workloads;
    auto add = [&](string name, vector<vector<int>> texts) {
        Workload w{std::move(name), std::move(texts), {}};
        bool small = std::ranges::all_of(w.texts, [](auto &s) { return std::ranges::all_of(s, [](int c) { return c < 256; }); });
        for (auto &s : w.texts) {
            if (small) { w.bytes.emplace_back(s.begin(), s.end()); }}
        workloads.push_back(std::move(w));};
    for (int sigma : {4, 26}) {
        for (int n : {8, 16, 32, 64, 128, 256, 1024}) {
            vector<vector<int>> texts(2000000 / n, vector<int>(n));
            for (auto &s : texts) {
                for (int &c : s) { c = 'a' + int(rng() % sigma); }}
            add("chunks n=" + std::to_string(n) + " S=" + std::to_string(sigma), texts);}}
    for (int n : {16, 64, 256}) { add("unary chunks n=" + std::to_string(n), vector<vector<int>>(2000000 / n, vector<int>(n, 'a'))); }
    for (int sigma : {2, 26}) {
        vector<int> s(1000000);
        for (int &c : s) { c = 'a' + int(rng() % sigma); }
        add("random n=1000000 S=" + std::to_string(sigma), {s});}
    vector<int> sparse(1000000);
    for (int &c : sparse) { c = int(rng() % 1000000000); }
    add("sparse n=1000000 S=10^9", {sparse});
    vector<int> a{'a'}, b{'b'};
    while (int(a.size()) < 1000000) { vector<int> c = a; c.insert(c.end(), b.begin(), b.end()); b = a; a = c; }
    a.resize(1000000);
    add("fibonacci n=1000000", {a});
    add("unary n=1000000", {vector<int>(1000000, 'a')});
    auto spread = [&](int n, int range, int total) {
        vector<vector<int>> texts(total / n, vector<int>(n));
        for (auto &s : texts) {
            for (int &c : s) { c = int(rng() % ulng(range)); }
            s[0] = 0;
            s[1] = range - 1;}
        Workload w{"range " + std::to_string(range) + " n=" + std::to_string(n), texts, {}, true};
        workloads.push_back(std::move(w));};
    for (int n : {16, 64}) {
        for (int range : {64, 128, 256}) { spread(n, range, 2000000); }}
    for (int n : {10000, 1000000}) {
        for (int c2 : {1, 2, 4, 8, 16, 32}) { spread(n, n * c2 / 2, 2000000); }}
    using Method = pair<string, lng (*)(const Workload &)>;
    vector<Method> methods{{"vector", runSais}, {"bytes", runBytes}, {"doubling", runDoubling}, {"shift", runShift}, {"compress", runCompress}};
    cout << "[";
    bool first = true;
    for (auto &w : workloads) {
        vector<vector<double>> samples(methods.size());
        vector<lng> sums(methods.size());
        vector<int> active;
        for (int k = 0; k < int(methods.size()); ++k) {
            bool threshold_method = methods[k].first == "shift" || methods[k].first == "compress";
            if (threshold_method == w.threshold && (methods[k].first != "bytes" || !w.bytes.empty())) { active.push_back(k); }}
        for (int rep = -1; rep < reps; ++rep) {
            for (int t = 0; t < int(active.size()); ++t) {
                int at = active[(t + max(rep, 0)) % int(active.size())];
                auto start = Clock::now();
                sums[at] = methods[at].second(w);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
                if (rep >= 0) { samples[at].push_back(ms); }}}
        for (int k : active) {
            if (sums[k] != sums[active[0]]) { cerr << "checksum mismatch " << w.name << ' ' << methods[k].first << '\n'; return 1; }
            sort(samples[k].begin(), samples[k].end());
            cout << (first ? "" : ",") << "{\"workload\":\"" << w.name << "\",\"method\":\"" << methods[k].first << "\",\"median_ms\":" << samples[k][reps / 2] << "}";
            first = false;}}
    cout << "]\n";}
