#include "../../05-Mathematics/19-josephus.hpp"

string context;
lng checks = 0;
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
constexpr lng HI = std::numeric_limits<lng>::max();

// Direct simulation: the removal order of people 0..n-1.
vector<int> simulate(int n, lng k) {
    vector<int> ppl(n), out;
    iota(ppl.begin(), ppl.end(), 0);
    ulng cur = 0;
    while (!ppl.empty()) {
        cur = (cur + ulng(k - 1)) % ppl.size();
        out.pb(ppl[cur]); ppl.erase(ppl.begin() + lng(cur));}
    return out;}
void brute(int n, lng k) {
    context = "n=" + std::to_string(n) + " k=" + std::to_string(k);
    auto o = simulate(n, k);
    need(josephusOrder(n, k) == o, "josephusOrder");
    if (n == 0) { return; }
    need(josephus(n, k) == o.back(), "josephus");
    if (k == 2) { need(josephusK2(n) == o.back(), "josephusK2"); }
    for (int t = 0; t < n; ++t) {
        need(josephusKth(n, k, t) == o[t], "josephusKth");
        need(josephusTime(n, k, o[t]) == t, "josephusTime");}}
// Order against josephusKth and josephusTime on sampled steps for large n.
void sampled(std::mt19937_64 &rng, int n, lng k, int samples) {
    context = "n=" + std::to_string(n) + " k=" + std::to_string(k);
    auto o = josephusOrder(n, k);
    vector<int> seen(n);
    for (int p : o) { need(0 <= p && p < n && !seen[p]++, "order is a permutation"); }
    need(josephus(n, k) == o.back(), "survivor is the last removed");
    for (int i = 0; i < samples; ++i) {
        int t = i < 2 ? (i ? n - 1 : 0) : int(rng() % ulng(n));
        need(josephusKth(n, k, t) == o[t] && josephusTime(n, k, o[t]) == t, "sampled kth and time");}}
// Huge n: josephusKth and josephusTime are mutually inverse bijections; josephusK2 and josephus agree at k = 2.
void huge(std::mt19937_64 &rng, lng n, lng k) {
    context = "n=" + std::to_string(n) + " k=" + std::to_string(k);
    lng t = lng(rng() % ulng(n)), p = lng(rng() % ulng(n)), x = josephusKth(n, k, t), y = josephusTime(n, k, p);
    need(0 <= x && x < n && josephusTime(n, k, x) == t, "time(kth(t)) = t");
    need(0 <= y && y < n && josephusKth(n, k, y) == p, "kth(time(p)) = p");
    need(josephusTime(n, k, josephus(n, k)) == n - 1, "the survivor is removed last");
    if (k == 2) { need(josephusK2(n) == josephus(n, 2), "josephusK2 = josephus(n, 2)"); }}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261018;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            lng n, k, t, p;
            while (cin >> n >> k >> t >> p) {
                cout << josephus(n, k) << ' ' << josephusKth(n, k, t) << ' ' << josephusTime(n, k, p) << ' '
                    << (k == 2 ? std::to_string(josephusK2(n)) : "-") << '\n';}
            return 0;}}
    if (!invalid.empty()) {
        if (invalid == "josephus-n") { josephus(0, 3); }
        else if (invalid == "josephus-k") { josephus(3, 0); }
        else if (invalid == "k2-n") { josephusK2(0); }
        else if (invalid == "kth-t") { josephusKth(5, 2, 5); }
        else if (invalid == "time-p") { josephusTime(5, 2, -1); }
        else if (invalid == "order-k") { josephusOrder(5, 0); }
        else if (invalid == "order-n") { josephusOrder(-1, 2); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    int N = quick ? 30 : 60;
    for (int n = 0; n <= N; ++n) {
        for (lng k = 1; k <= N + 10; ++k) { brute(n, k); }
        for (lng k : {lng(1) << 40, HI - 1, HI}) { brute(n, k); }}
    cout << "PASS exhaustive simulation n <= " << N << " checks=" << checks << '\n';
    for (int i = 0; i < (quick ? 4 : stress ? 60 : 16); ++i) {
        int n = 1 + int(rng() % (quick ? 500 : 3000));
        lng k = i % 3 == 0 ? std::bit_cast<lng>(rng() >> 1) + 1 : 1 + lng(rng() % ulng(i % 3 == 1 ? 10 : n + 10));
        brute(n, max<lng>(1, k));}
    cout << "PASS seeded simulation n <= 3000 checks=" << checks << '\n';
    for (int i = 0; i < (quick ? 2 : stress ? 30 : 8); ++i) {
        int n = 1 + int(rng() % 200000);
        sampled(rng, n, 1 + lng(rng() % ulng(i % 2 ? 5 : n + 5)), 300);}
    cout << "PASS order against kth and time n <= 200000 checks=" << checks << '\n';
    for (int i = 0; i < (quick ? 300 : stress ? 30000 : 3000); ++i) {
        lng n = max<lng>(1, lng(rng() >> (1 + rng() % 63)));
        lng k = i % 4 == 0 ? 2 : 1 + lng(rng() % (i % 4 == 1 ? 3 : i % 4 == 2 ? 100 : 5000));
        huge(rng, n, k);}
    for (lng n : {HI, HI - 1, lng(1) << 62, (lng(1) << 62) - 1}) { for (lng k : {1, 2, 3, 1000}) { huge(rng, n, k); }}
    cout << "PASS huge n inverse pairs checks=" << checks << '\n';}
