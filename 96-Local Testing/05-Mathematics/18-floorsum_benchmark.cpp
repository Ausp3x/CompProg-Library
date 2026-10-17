#include "../../05-Mathematics/18-floorsum.hpp"
#include "../../05-Mathematics/20-continued_fraction.hpp"

// The shipped kernel against the same Euclidean loop with every division in 128 bits, and ACL's 64-bit loop (n, m < 2^32 only).
ulll wide(ulng n, ulng m, ulng a, ulng b) {
    ulll res = 0;
    while (true) {
        if (a >= m) { res += ulll(n) * (n - 1) / 2 * (a / m); a %= m; }
        if (b >= m) { res += ulll(n) * (b / m); b %= m; }
        ulll y = ulll(a) * n + b;
        if (y < m) { return res; }
        n = ulng(y / m); b = ulng(y % m); swap(m, a);}}
ulll narrow(ulng n, ulng m, ulng a, ulng b) {
    ulng res = 0;
    while (true) {
        if (a >= m) { res += n * (n - 1) / 2 * (a / m); a %= m; }
        if (b >= m) { res += n * (b / m); b %= m; }
        ulng y = a * n + b;
        if (y < m) { return res; }
        n = y / m; b = y % m; swap(m, a);}}

template<typename F> double median(int reps, F f) {
    vector<double> t;
    f();
    for (int i = 0; i < reps; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        t.pb(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());}
    sort(t.begin(), t.end()); return t[t.size() / 2];}

int main(int argc, char **argv) {
    int reps = argc > 1 ? std::stoi(argv[1]) : 5;
    std::mt19937_64 rng(20261018);
    cout << "[";
    bool first = true;
    for (auto [name, lim] : vector<pair<string, ulng>>{{"n, m <= 10^9 (Library Checker)", 1000000000ULL}, {"n, m < 2^62", 1ULL << 62}}) {
        vector<array<ulng, 4>> qs(1000000);
        for (auto &q : qs) { q[0] = rng() % lim; q[1] = rng() % lim + 1; q[2] = rng() % q[1]; q[3] = rng() % q[1]; }
        ulll s1 = 0, s2 = 0, s3 = 0;
        double shipped = median(reps, [&] { s1 = 0; for (auto &q : qs) { s1 += ulll(floorSum(lng(q[0]), lng(q[1]), q[2], q[3])); }});
        double w = median(reps, [&] { s2 = 0; for (auto &q : qs) { s2 += wide(q[0], q[1], q[2], q[3]); }});
        bool small = lim < (1ULL << 32);
        double nw = small ? median(reps, [&] { s3 = 0; for (auto &q : qs) { s3 += narrow(q[0], q[1], q[2], q[3]); }}) : 0;
        if (s1 != s2 || (small && s1 != s3)) { cerr << "mismatch " << name << '\n'; return 1; }
        cout << (first ? "" : ",") << "\n {\"workload\": \"1e6 floorSum, " << name << "\", \"shipped_ms\": " << shipped << ", \"wide_ms\": " << w
            << ", \"acl64_ms\": " << (small ? std::to_string(nw) : "null") << "}";
        first = false;}
    for (lng n : {lng(1000000), lng(1000000000)}) {
        lng k = n / 3, total = 0;
        pair<lng, lng> f;
        double kth = median(reps, [&] { f = fareyKth(n, k); });
        double count = median(reps, [&] { total = lng(countRationalsInRange(n, 0, 1, f.fi, f.se)); });
        if (total != k) { cerr << "farey rank mismatch\n"; return 1; }
        cout << ",\n {\"workload\": \"fareyKth and countRationalsInRange, n = " << n << "\", \"kth_ms\": " << kth << ", \"count_ms\": " << count << "}";}
    cout << "\n]\n";}
