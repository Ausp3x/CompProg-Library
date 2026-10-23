#include "../../06-Miscellaneous/55-fractional_programming.hpp"

constexpr ulng SEED = 20261024;
constexpr int REPS = 5;
string context;

void require(bool ok, const string &details) {
    if (!ok) { cerr << "FAIL benchmark seed=" << SEED << ' ' << context << ' ' << details << '\n'; std::exit(1); }}

// The replaced engine: Dinkelbach from the first k items over an nth_element oracle; counts oracle calls.
FractionalSelection dinkelbachSelect(const vector<lng> &a, const vector<lng> &b, int k, int &calls) {
    int n = int(a.size());
    vector<int> id(n), best;
    vector<lll> key(n);
    iota(id.begin(), id.end(), 0);
    lng p = 0, q = 0;
    for (int i = 0; i < k; ++i) { p += a[i]; q += b[i]; }
    auto oracle = [&](lng s, lng t) {
        ++calls;
        for (int i = 0; i < n; ++i) { key[i] = lll(t) * a[i] - lll(s) * b[i]; }
        std::nth_element(id.begin(), id.begin() + (k - 1), id.end(), [&](int i, int j) { return key[i] != key[j] ? key[i] > key[j] : i < j; });
        best.assign(id.begin(), id.begin() + k);
        lng x = 0, y = 0;
        for (int i : best) { x += a[i]; y += b[i]; }
        return pair(x, y);};
    auto [num, den] = dinkelbach(oracle, p, q);
    sort(best.begin(), best.end());
    return {num, den, best};}

void checkWitness(const FractionalSelection &got, const vector<lng> &a, const vector<lng> &b, int k) {
    require(int(got.items.size()) == k && std::adjacent_find(got.items.begin(), got.items.end(), std::greater_equal<>()) == got.items.end(), "witness size or order");
    lll x = 0, y = 0;
    for (int i : got.items) { x += a[i]; y += b[i]; }
    require(x == got.num && y == got.den, "witness sums");}

// Independent certificate: at t = num / den the k largest keys a - t * b sum to exactly zero.
void certify(const FractionalSelection &got, const vector<lng> &a, const vector<lng> &b, int k) {
    int n = int(a.size());
    vector<lll> key(n);
    for (int i = 0; i < n; ++i) { key[i] = lll(got.den) * a[i] - lll(got.num) * b[i]; }
    std::nth_element(key.begin(), key.begin() + (k - 1), key.end(), std::greater<>());
    lll top = 0;
    for (int i = 0; i < k; ++i) { top += key[i]; }
    require(top == 0, "optimality certificate");}

int main() {
    const char *names[] = {"random", "wide", "duplicates", "concurrent", "tangent"};
    for (int family = 0; family < 5; ++family) {
        for (int n : {1000, 100000, 1000000}) {
            for (int k : {1, n / 2, n - 1}) {
                std::mt19937_64 gen(SEED + ulng(family) * 1000003 + ulng(n) * 7 + ulng(k));
                vector<lng> a(n), b(n);
                for (int i = 0; i < n; ++i) {
                    b[i] = family == 1 ? 1 + lng(gen() % (1 << 20)) : family == 2 ? 1 + lng(gen() % 3) : 1 + lng(gen() % 1000);
                    a[i] = family == 0 ? lng(gen() % 1000000) : family == 1 ? lng(gen() % (ulng(1) << 41)) - (lng(1) << 40) : family == 2 ? lng(gen() % 5) : 7 * b[i] - 5;}
                // Tangents to (2^30 - t)^2 at distances 2^(j+1), behind fillers (0, 2^30): Newton, hence Dinkelbach, halves the gap once per call for k = 1.
                for (int i = 0; family == 4 && i < n; ++i) {
                    int j = i - (n - 28);
                    b[i] = j < 0 ? lng(1) << 30 : lng(4) << j;
                    a[i] = j < 0 ? 0 : b[i] * ((lng(1) << 30) - (lng(1) << j));}
                context = string(names[family]) + " n=" + std::to_string(n) + " k=" + std::to_string(k);
                FractionalSelection ref{0, 0, {}};
                for (int rep = -1; rep < REPS; ++rep) {
                    for (int turn = 0; turn < 2; ++turn) {
                        int method = (turn + max(rep, 0)) % 2, calls = 0;
                        auto start = std::chrono::steady_clock::now();
                        auto got = method ? dinkelbachSelect(a, b, k, calls) : zeroOneFractionalSelect(a, b, k, SEED + ulng(rep + 1));
                        double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                        checkWitness(got, a, b, k);
                        if (!ref.den) {
                            certify(got, a, b, k);
                            ref = got;}
                        require(lll(got.num) * ref.den == lll(ref.num) * got.den, "ratio differs between engines or seeds");
                        if (rep >= 0) { cout << names[family] << ' ' << n << ' ' << k << ' ' << (method ? "dinkelbach" : "eppstein") << ' ' << rep << ' ' << calls << ' ' << ms << '\n'; }}}}}}
    return 0;}
