#include "../../07-Strings/12-sais.hpp"
#include "../../07-Strings/07-suffixarray.hpp"

using Clock = std::chrono::steady_clock;
// Sum of suffix-array entries weighted by rank; compared across methods outside the timing.
lng digest(const vector<int> &sa) {
    lng res = 0;
    for (int i = 0; i < int(sa.size()); ++i) { res = res * 1000003 + sa[i] + i; }
    return res;}
template<int NAIVE> lng run(const vector<vector<int>> &texts, int upper) {
    lng res = 0;
    for (auto &s : texts) { res ^= digest(sais<NAIVE>(s, upper)); }
    return res;}
lng runDoubling(const vector<vector<int>> &texts, int) {
    lng res = 0;
    for (auto &s : texts) { res ^= digest(SuffixArray<int>(s, false).sa); }
    return res;}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261009;
    int reps = argc > 2 ? std::stoi(argv[2]) : 5;
    std::mt19937_64 rng(seed);
    struct Workload { string name; vector<vector<int>> texts; int upper; };
    vector<Workload> workloads;
    for (int n : {8, 16, 24, 32, 64, 256}) {
        Workload w{"chunks n=" + std::to_string(n) + " S=4", {}, 3};
        for (int k = 0; k < 2000000 / n; ++k) {
            w.texts.emplace_back(n);
            for (int &c : w.texts.back()) { c = int(rng() % 4); }}
        workloads.push_back(w);}
    for (int n : {16, 32, 48, 64}) {
        Workload w{"unary chunks n=" + std::to_string(n), {}, 0};
        for (int k = 0; k < 2000000 / n; ++k) { w.texts.emplace_back(n, 0); }
        workloads.push_back(w);}
    for (int sigma : {2, 4, 26}) {
        Workload w{"random n=1000000 S=" + std::to_string(sigma), {vector<int>(1000000)}, sigma - 1};
        for (int &c : w.texts[0]) { c = int(rng() % sigma); }
        workloads.push_back(w);}
    vector<int> a{0}, b{1};
    while (int(a.size()) < 1000000) { vector<int> c = a; c.insert(c.end(), b.begin(), b.end()); b = a; a = c; }
    a.resize(1000000);
    workloads.push_back({"fibonacci n=1000000", {a}, 1});
    workloads.push_back({"unary n=1000000", {vector<int>(1000000, 0)}, 0});
    using Method = pair<string, lng (*)(const vector<vector<int>> &, int)>;
    vector<Method> methods{{"sais<1>", run<1>}, {"sais<8>", run<8>}, {"sais<16>", run<16>}, {"sais<32>", run<32>}, {"sais<64>", run<64>}, {"doubling", runDoubling}};
    cout << "[";
    bool first = true;
    for (auto &w : workloads) {
        vector<vector<double>> samples(methods.size());
        vector<lng> sums(methods.size());
        for (int rep = -1; rep < reps; ++rep) {
            for (int k = 0; k < int(methods.size()); ++k) {
                int at = (k + max(rep, 0)) % int(methods.size());
                auto start = Clock::now();
                sums[at] = methods[at].second(w.texts, w.upper);
                double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
                if (rep >= 0) { samples[at].push_back(ms); }}}
        for (int k = 0; k < int(methods.size()); ++k) {
            if (sums[k] != sums[0]) { cerr << "checksum mismatch " << w.name << ' ' << methods[k].first << '\n'; return 1; }
            sort(samples[k].begin(), samples[k].end());
            cout << (first ? "" : ",") << "{\"workload\":\"" << w.name << "\",\"method\":\"" << methods[k].first << "\",\"median_ms\":" << samples[k][reps / 2] << "}";
            first = false;}}
    cout << "]\n";}
