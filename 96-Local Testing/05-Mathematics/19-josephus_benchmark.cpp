#include "../../05-Mathematics/19-josephus.hpp"

template<typename F> double median(int reps, F f) {
    vector<double> t;
    f();
    for (int i = 0; i < reps; ++i) {
        auto t0 = std::chrono::steady_clock::now();
        f();
        t.pb(std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());}
    sort(t.begin(), t.end()); return t[t.size() / 2];}
// The plain O(n) survivor recurrence J(i) = (J(i - 1) + k) mod i, the removed former josephus.
lng plain(lng n, lng k) {
    ulng res = 0;
    for (ulng i = 2; i <= ulng(n); ++i) { res = (res + ulng(k)) % i; }
    return lng(res);}

// The shipped josephus (run skipping over the same recurrence) against the plain loop, small to huge k.
int main(int argc, char **argv) {
    int reps = argc > 1 ? std::stoi(argv[1]) : 5;
    cout << "[";
    bool first = true;
    for (auto [n, k] : vector<pair<lng, lng>>{{10000000, 3}, {10000000, 1000}, {10000000, 1000000}, {10000000, 5000000}, {10000000, 10000001}, {10000000, 1000000000000000000}}) {
        lng a = -1, b = -2;
        double loop = median(reps, [&] { a = plain(n, k); });
        double skip = median(reps, [&] { b = josephus(n, k); });
        if (a != b) { cerr << "mismatch n=" << n << " k=" << k << '\n'; return 1; }
        cout << (first ? "" : ",") << "\n {\"workload\": \"survivor n = " << n << ", k = " << k << "\", \"plain_ms\": " << loop << ", \"josephus_ms\": " << skip << "}";
        first = false;}
    cout << "\n]\n";}
