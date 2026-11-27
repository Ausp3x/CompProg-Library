#include "../../06-Miscellaneous/060-permutation_problems.hpp"

ulng seed = 20260927;
[[noreturn]] void fail(int n, lng k, std::string_view operation, const string &detail = "") {
    cerr << "FAIL seed=" << seed << " n=" << n << " k=" << k << " operation=" << operation << detail << '\n'; std::exit(1);}
string show(const vector<int> &a) {
    string s = "[";
    for (size_t i = 0; i < min(a.size(), size_t(40)); ++i) { s += std::to_string(a[i]) + ','; }
    return s + (a.size() > 40 ? "...]" : "]");}
void check(bool ok, int n, lng k, std::string_view operation) {
    if (!ok) { fail(n, k, operation); }}
void checkVector(const vector<int> &a, const vector<int> &b, int n, lng k, std::string_view operation) {
    if (a != b) { fail(n, k, operation, " expected=" + show(b) + " actual=" + show(a)); }}
lng maxInversions(int n) { return lng(n) * (n - 1) / 2; }
// Lexicographic order of permutations is that of their Lehmer codes (d_i = later smaller values, 0 <= d_i <= n - 1 - i),
// so the largest permutation with k inversions has the lexicographically largest code of sum k: d_i = min(rest, n - 1 - i).
vector<int> greedyDigits(int n, lng k) {
    vector<int> d(n);
    for (int i = 0; i < n; ++i) { d[i] = int(min(k, lng(n - 1 - i))); k -= d[i]; }
    return d;}
// Decodes a Lehmer code by taking the d_i-th smallest unused value, O(n^2).
vector<int> decode(const vector<int> &d) {
    vector<int> rest(d.size()), p; iota(rest.begin(), rest.end(), 0);
    for (int x : d) { p.push_back(rest[x]); rest.erase(rest.begin() + x); }
    return p;}
// Lehmer code of a permutation of [0, n) by a Fenwick tree over values, right to left.
vector<int> digitsOf(const vector<int> &p) {
    int n = int(p.size()); vector<int> bit(n + 1), d(n);
    for (int i = n - 1; i >= 0; --i) {
        for (int x = p[i]; x > 0; x -= x & -x) { d[i] += bit[x]; }
        for (int x = p[i] + 1; x <= n; x += x & -x) { ++bit[x]; }}
    return d;}
bool isPermutationOf(const vector<int> &p, int n) {
    vector<char> seen(n); bool ok = int(p.size()) == n;
    for (int x : p) { ok = ok && 0 <= x && x < n && !seen[x]++; }
    return ok;}
lng bruteInversions(const vector<int> &p) {
    lng res = 0;
    for (int i = 0; i < int(p.size()); ++i) {
        for (int j = i + 1; j < int(p.size()); ++j) { res += p[j] < p[i]; }}
    return res;}
// Merge-sort inversion counter, independent of Fenwick and Lehmer digits.
lng mergeInversions(vector<int> a) {
    int n = int(a.size()); vector<int> tmp(n); lng res = 0;
    for (int w = 1; w < n; w *= 2) {
        for (int l = 0; l + w < n; l += 2 * w) {
            int m = l + w, r = min(n, l + 2 * w), i = l, j = m, k = l;
            while (i < m || j < r) {
                if (j == r || (i < m && a[i] <= a[j])) { tmp[k++] = a[i++]; }
                else { res += m - i; tmp[k++] = a[j++]; }}
            std::copy(tmp.begin() + l, tmp.begin() + r, a.begin() + l);}}
    return res;}
// Every permutation of [0, n) by recursion over unused values, inversions counted as each value is placed; the largest
// per k by explicit comparison, the counts per k against the Mahonian recurrence M(n, k) = sum_j M(n - 1, k - j), j < n.
void exhaustive(int limit) {
    vector<ulng> mahonian{1};
    for (int n = 0; n <= limit; ++n) {
        if (n) {
            vector<ulng> next(size_t(maxInversions(n)) + 1);
            for (size_t k = 0; k < next.size(); ++k) {
                for (int j = 0; j < n && size_t(j) <= k; ++j) { next[k] += k - size_t(j) < mahonian.size() ? mahonian[k - size_t(j)] : 0; }}
            mahonian = std::move(next);}
        vector<vector<int>> best(mahonian.size()); vector<char> found(mahonian.size()); vector<ulng> count(mahonian.size());
        vector<int> p; vector<char> used(n);
        auto go = [&](auto &&go, lng inv) -> void {
            if (int(p.size()) == n) {
                check(bruteInversions(p) == inv, n, inv, "enumeration: incremental and brute inversion counts agree");
                ++count[size_t(inv)];
                if (!found[size_t(inv)] || p > best[size_t(inv)]) { best[size_t(inv)] = p; found[size_t(inv)] = 1; }
                return;}
            int larger = 0;
            for (int v = n - 1; v >= 0; --v) {
                if (used[v]) { ++larger; continue; }
                used[v] = 1; p.push_back(v); go(go, inv + larger); p.pop_back(); used[v] = 0;}};
        go(go, 0);
        check(count == mahonian, n, -1, "enumeration: permutations per inversion count equal the Mahonian numbers");
        for (lng k = 0; k <= maxInversions(n); ++k) {
            checkVector(decode(greedyDigits(n, k)), best[size_t(k)], n, k, "greedy Lehmer oracle against the enumeration");
            checkVector(permutationWithInversions(n, k), best[size_t(k)], n, k, "lexicographically largest of the enumeration");}}}
// k at the end of the descending prefix of length t: T_t = sum_{i < t} (n - 1 - i), shifted by -1, 0 or 1 within [0, max].
lng boundary(int n, int t, int shift) {
    return std::clamp(lng(t) * (2 * lng(n) - 1 - t) / 2 + shift, lng(0), maxInversions(n));}
lng randomK(int n, std::mt19937_64 &rng, int rep) {
    lng top = maxInversions(n);
    if (rep % 4 == 1) { return boundary(n, int(rng() % ulng(n + 1)), int(rng() % 3) - 1); }
    if (rep % 8 == 2) { return std::array<lng, 4>{0, 1, top - 1, top}[rng() % 4] % (top + 1); }
    return lng(rng() % ulng(top + 1));}
// Random n < 70: a permutation, exactly k inversions by brute and merge counts, equal to the decoded greedy code.
void smallRandom(std::mt19937_64 &rng, int reps) {
    for (int rep = 0; rep < reps; ++rep) {
        int n = int(rng() % 70); lng k = max(lng(0), randomK(n, rng, rep));
        vector<int> v = permutationWithInversions(n, k);
        check(isPermutationOf(v, n), n, k, "permutation of [0, n)");
        check(bruteInversions(v) == k, n, k, "brute inversion count");
        check(mergeInversions(v) == k, n, k, "merge inversion count");
        checkVector(v, decode(greedyDigits(n, k)), n, k, "lexicographically largest: decoded greedy Lehmer code");}}
// Large n: a permutation, exactly k inversions by the merge count, Lehmer code (Fenwick) equal to the greedy code.
void large(int n, std::mt19937_64 &rng, vector<lng> ks) {
    lng top = maxInversions(n);
    for (lng k : {lng(0), lng(1), top / 2, top - 1, top, lng(rng() % ulng(top + 1))}) { ks.push_back(k); }
    for (int s = -1; s <= 1; ++s) { ks.push_back(boundary(n, int(rng() % ulng(n + 1)), s)); }
    for (lng k : ks) {
        vector<int> v = permutationWithInversions(n, k);
        check(isPermutationOf(v, n), n, k, "large permutation of [0, n)");
        check(mergeInversions(v) == k, n, k, "large merge inversion count");
        if (digitsOf(v) != greedyDigits(n, k)) { fail(n, k, "large lexicographically largest: Lehmer code equals the greedy code"); }}}
void oracle() {
    int n; lng k;
    while (cin >> n >> k) {
        vector<int> v = permutationWithInversions(n, k);
        for (size_t i = 0; i < v.size(); ++i) { cout << (i ? " " : "") << v[i]; }
        cout << '\n';}
    check(cin.eof(), -1, -1, "Python input protocol");}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--oracle") { oracle(); return 0; }
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i];
            if (p == "inversions-large") { permutationWithInversions(3, 4); }
            else if (p == "inversions-negative-k") { permutationWithInversions(3, -1); }
            else if (p == "inversions-negative-n") { permutationWithInversions(-1, 0); }
            else if (p == "inversions-large-wide") { permutationWithInversions(65537, maxInversions(65537) + 1); }
            return 0;}}
    std::mt19937_64 rng(seed);
    bool quick = mode == "quick", full = mode == "full";
    int limit = quick ? 6 : full ? 8 : 9;
    exhaustive(limit);
    cout << "PASS every k through n=" << limit << ": lexicographically largest of an enumeration validated by the Mahonian numbers\n";
    smallRandom(rng, quick ? 100 : full ? 3000 : 30000);
    cout << "PASS random n < 70: brute and merge inversion counts, decoded greedy Lehmer code\n";
    for (int rep = 0; rep < (quick ? 5 : full ? 100 : 300); ++rep) { large(70 + int(rng() % 2931), rng, {}); }
    lng wide = lng(INT_MAX);
    large(65537, rng, {wide - 1, wide, wide + 1});
    int n = quick ? 1000 : full ? 100000 : 500000;
    large(n, rng, {});
    cout << "PASS n in [70, 3000], n=65537 around k=2^31 and n=" << n << ": merge inversion count and greedy Lehmer code\n";}
