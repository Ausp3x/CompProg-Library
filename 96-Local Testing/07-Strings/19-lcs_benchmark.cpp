#include "../../07-Strings/19-lcs.hpp"

using Clock = std::chrono::steady_clock;
struct Mod {
    static constexpr lng P = 998244353;
    lng v = 0;
    Mod(lng x = 0) : v((x % P + P) % P) {}
    friend Mod operator+(Mod a, Mod b) { return Mod(a.v + b.v); }
    friend Mod operator-(Mod a, Mod b) { return Mod(a.v - b.v); }
    Mod &operator+=(Mod b) { return *this = *this + b; }
};
template<class F> double median(F f) {
    f();
    vector<double> t;
    for (int rep = 0; rep < 5; ++rep) {
        auto a = Clock::now();
        f();
        t.push_back(std::chrono::duration<double, std::milli>(Clock::now() - a).count());}
    sort(t.begin(), t.end());
    return t[2];}
void row(const string &work, const string &op, int n, int m, double ms, lng value) {
    cout << "{\"workload\": \"" << work << "\", \"operation\": \"" << op << "\", \"n\": " << n << ", \"m\": " << m << ", \"median_ms\": " << std::fixed << std::setprecision(2) << ms << ", \"result\": " << value << "}" << endl;}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261010;
    std::mt19937_64 rng(seed);
    auto ints = [&](int n, int sigma) { vector<int> v(n); for (int &x : v) { x = int(rng() % sigma); } return v; };
    for (auto [work, n, sigma] : vector<tuple<string, int, int>>{{"dna", 20000, 4}, {"bytes", 50000, 256}, {"sparse", 100000, 100000}}) {
        vector<int> a = ints(n, sigma), b = ints(n, sigma);
        lng bits = 0, hs = 0, scalar = -1, wit = 0;
        double tb = median([&] { bits = bitsetLcs(a, b); });
        double th = median([&] { hs = huntSzymanski(a, b); });
        double tw = median([&] { wit = lng(lcsWitness(a, b).size()); });
        row(work, "bitsetLcs", n, n, tb, bits);
        row(work, "huntSzymanski", n, n, th, hs);
        row(work, "lcsWitness", n, n, tw, wit);
        if (n <= 20000) {
            double ts = median([&] { scalar = lcs_detail::scalarRow(a, b, 0, n, 0, n, false).back(); });
            row(work, "scalar DP row", n, n, ts, scalar);
            double tr = median([&] { scalar = lng(hirschbergLcs(a, b).size()); });
            row(work, "hirschbergLcs", n, n, tr, scalar);}
        if (bits != hs || bits != wit || (scalar >= 0 && scalar != bits)) {
            cerr << "FAIL workload=" << work << " disagreement\n";
            return 1;}}
    for (int edits : {10, 1000}) {
        string s(1000000, 'a');
        for (char &c : s) { c = char('a' + rng() % 2); }
        string t = s;
        for (int e = 0; e < edits; ++e) {
            int p = int(rng() % t.size());
            if (e % 2) { t.erase(t.begin() + p); }
            else { t.insert(t.begin() + p, 'c'); }}
        lng ops = 0;
        double tm = median([&] { ops = lng(myersDiff(s, t).size()); });
        row("similar-" + std::to_string(edits), "myersDiff", int(s.size()), int(t.size()), tm, ops);}
    for (int n : {2000, 5000}) {
        string s(n, 'a');
        for (char &c : s) { c = char('a' + rng() % 4); }
        Mod d, k;
        double td = median([&] { d = countPalindromicSubsequences<Mod>(s, true); });
        double tk = median([&] { k = countPalindromicSubsequences<Mod>(s, false); });
        row("palindromic-4", "countPalindromicSubsequences distinct", n, 0, td, d.v);
        row("palindromic-4", "countPalindromicSubsequences multiset", n, 0, tk, k.v);}}
