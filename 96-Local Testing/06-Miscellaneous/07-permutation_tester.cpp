#include "../../06-Miscellaneous/07-permutation.hpp"

ulng seed = 20260927;
string context;
void check(bool ok, const string &s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << s << '\n'; std::exit(1); } }
string show(const vector<int> &a) {
    string s = "[";
    for (int x : a) { s += std::to_string(x) + ','; }
    return s + ']'; }
void checkVector(const vector<int> &a, const vector<int> &b, const string &operation) {
    check(a == b, operation + " expected=" + show(b) + " actual=" + show(a)); }
vector<vector<int>> enumerate(vector<int> counts, vector<int> keys) {
    vector<vector<int>> all; vector<int> p; int n = accumulate(counts.begin(), counts.end(), 0);
    auto go = [&](auto &&go) -> void {
        if (int(p.size()) == n) { all.push_back(p); return; }
        for (int i = 0; i < int(counts.size()); ++i) {
            if (counts[i]) { --counts[i]; p.push_back(keys[i]); go(go); p.pop_back(); ++counts[i]; }} };
    go(go); return all; }
vector<int> powerOracle(const vector<int> &p, lng k) {
    int n = int(p.size()); vector<int> r(n), q = p;
    iota(r.begin(), r.end(), 0);
    if (k < 0) { for (int i = 0; i < n; ++i) { q[p[i]] = i; } }
    ulng e = k < 0 ? ulng(0) - ulng(k) : ulng(k);
    while (e) {
        if (e & 1) { for (int &x : r) { x = q[x]; } }
        vector<int> square(n);
        for (int i = 0; i < n; ++i) { square[i] = q[q[i]]; }
        q = std::move(square); e >>= 1; }
    return r; }
void distinct(int limit) {
    for (int n = 0; n <= limit; ++n) {
        vector<int> id(n); iota(id.begin(), id.end(), 0);
        auto all = enumerate(vector<int>(n, 1), id); vector<int> p = id;
        for (ulng rank = 0; rank < all.size(); ++rank) {
            context = "distinct n=" + std::to_string(n) + " rank=" + std::to_string(rank);
            checkVector(p, all[rank], "next order independently enumerated");
            check(isPermutation(p), "valid permutation");
            check(permutationRank(p) == rank, "rank expected=" + std::to_string(rank));
            vector<int> decoded{-42}; check(permutationUnrank(n, rank, decoded), "unrank exists");
            checkVector(decoded, p, "unrank");
            vector<int> digits(n), inv(n);
            for (int i = 0; i < n; ++i) {
                inv[p[i]] = i;
                for (int j = i + 1; j < n; ++j) { digits[i] += p[j] < p[i]; } }
            checkVector(permutationLehmer(p), digits, "Lehmer brute inversion counts");
            checkVector(permutationFromLehmer(digits), p, "Lehmer decoding");
            checkVector(permutationInverse(p), inv, "inverse");
            checkVector(permutationCompose(p, inv), id, "inverse composition identity");
            if (n <= 5 || rank % 101 == 0) {
                for (lng k : {-3, -1, 0, 1, 2, 5}) { checkVector(permutationPower(p, k), powerOracle(p, k), "small power=" + std::to_string(k)); } }
            vector<int> previous = p; bool prev = previousPermutation(previous);
            check(prev == (rank != 0), "previous bool"); checkVector(previous, all[(rank + all.size() - 1) % all.size()], "previous wrap");
            check(nextPermutation(p) == (rank + 1 < all.size()), "next bool"); }
        checkVector(p, id, "next final wrap"); vector<int> old{-42};
        check(!permutationUnrank(n, all.size(), old) && old == vector<int>{-42}, "unrank absence preserves destination"); }
    check(!isPermutation({0, 0}) && !isPermutation({-1}) && !isPermutation({1}), "invalid permutation predicate");
    vector<int> descending{2, 1, 1}, after{1, 2, 1};
    check(nextPermutation(descending, std::greater<int>{}), "custom comparator next"); checkVector(descending, after, "descending comparator order");
    check(previousPermutation(descending, std::greater<int>{}), "custom comparator previous"); checkVector(descending, {2, 1, 1}, "descending comparator reverse");
    vector<string> words{"a", "a", "b"}; check(nextPermutation(words), "generic strings");
    check(words == vector<string>{"a", "b", "a"}, "generic duplicate sequence");
    vector<pair<int, int>> records{{1, 9}, {1, 7}, {2, 0}};
    auto cmp = [](auto a, auto b) -> bool { return a.first < b.first; };
    check(nextPermutation(records, cmp), "comparator equivalence classes");
    check(records[0].first == 1 && records[1].first == 2 && records[2].first == 1, "equivalent values need not be equal records");
    sort(records.begin(), records.end());
    check(records == vector<pair<int, int>>{{1, 7}, {1, 9}, {2, 0}}, "records preserved"); }
void multisetCases(int limit) {
    for (int a = 0; a <= limit; ++a) {
        for (int b = 0; b <= limit; ++b) {
            for (int c = 0; c <= limit; ++c) {
                vector<int> counts{a, b, c}; auto all = enumerate(counts, {INT_MIN, 0, INT_MAX});
                context = "multiset counts=" + show(counts); ulng total = 42;
                check(multisetPermutationCount(counts, total) && total == all.size(), "multinomial independent enumeration");
                vector<int> next = all.front();
                for (ulng i = 0; i < all.size(); ++i) {
                    checkVector(next, all[i], "multiset next order"); ulng rank = 42;
                    check(multisetPermutationRank(all[i], rank) && rank == i, "multiset rank expected=" + std::to_string(i));
                    vector<int> out{-42}; check(multisetPermutationUnrank(all.back(), i, out), "multiset unrank exists"); checkVector(out, all[i], "multiset unrank order");
                    vector<int> previous = all[i]; check(previousPermutation(previous) == (i != 0), "multiset previous bool");
                    checkVector(previous, all[(i + all.size() - 1) % all.size()], "multiset previous order");
                    check(nextPermutation(next) == (i + 1 < all.size()), "multiset next bool"); }
                vector<int> out{-42};
                check(!multisetPermutationUnrank(all[0], total, out) && out == vector<int>{-42}, "multiset absent preserves destination"); } } }
    context = "multiset overflow boundaries"; ulng count = 42;
    check(multisetPermutationCount({INT_MAX}, count) && count == 1, "huge all-equal count");
    check(multisetPermutationCount({INT_MAX - 1, 1}, count) && count == ulng(INT_MAX), "huge one rare count");
    check(multisetPermutationCount({INT_MAX - 2, 2}, count) && count == ulng(INT_MAX) * (INT_MAX - 1) / 2, "huge two rare count");
    count = 42; check(!multisetPermutationCount({INT_MAX - 3, 3}, count) && count == 42, "huge count overflow preserves");
    check(multisetPermutationCount({34, 33}, count) && count == 14226520737620288370ULL, "C(67,33) fits");
    count = 42; check(!multisetPermutationCount({34, 34}, count) && count == 42, "C(68,34) overflow");
    vector<int> unique(21); iota(unique.begin(), unique.end(), 0); vector<int> out{-42};
    check(!multisetPermutationRank(unique, count) && count == 42, "21! rank overflow preserves");
    check(!multisetPermutationUnrank(unique, 0, out) && out == vector<int>{-42}, "21! unrank rejects overflow even rank0");
    vector<int> duplicate{0, 0, 1, 1, 2}; check(multisetPermutationUnrank(duplicate, 7, duplicate), "alias-safe multiset unrank");
    check(multisetPermutationRank(duplicate, count) && count == 7, "aliased result"); }
void randomized(std::mt19937_64 &rng, int rounds, int large) {
    for (int rep = 0; rep < rounds; ++rep) {
        int n = int(rng() % 70); vector<int> p(n), q(n), expected(n); iota(p.begin(), p.end(), 0); q = p;
        std::shuffle(p.begin(), p.end(), rng); std::shuffle(q.begin(), q.end(), rng);
        context = "random rep=" + std::to_string(rep) + " p=" + show(p) + " q=" + show(q);
        for (int i = 0; i < n; ++i) { expected[i] = p[q[i]]; }
        checkVector(permutationCompose(p, q), expected, "composition order");
        for (lng k : {std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), std::bit_cast<lng>(rng())}) {
            checkVector(permutationPower(p, k), powerOracle(p, k), "full exponent domain k=" + std::to_string(k)); }
        checkVector(permutationFromLehmer(permutationLehmer(p)), p, "large factorial-independent digits"); }
    context = "large linear and nlogn domains n=" + std::to_string(large);
    vector<int> p(large); iota(p.begin(), p.end(), 0); reverse(p.begin(), p.end());
    auto d = permutationLehmer(p);
    for (int i = 0; i < large; ++i) { check(d[i] == large - i - 1, "large reverse Lehmer expected=" + std::to_string(large - i - 1)); }
    checkVector(permutationFromLehmer(d), p, "large Lehmer decode");
    checkVector(permutationPower(p, std::numeric_limits<lng>::min()), powerOracle(p, std::numeric_limits<lng>::min()), "large min exponent");
    for (int i = 0; i < large; ++i) { p[i] = (i + 1) % large; }
    checkVector(permutationPower(p, std::numeric_limits<lng>::min()), powerOracle(p, std::numeric_limits<lng>::min()), "large single cycle min exponent");
    vector<int> same(large, INT_MIN), out; ulng rank = 42;
    check(multisetPermutationRank(same, rank) && rank == 0, "large all-equal rank");
    check(multisetPermutationUnrank(same, 0, out), "large all-equal unrank"); checkVector(out, same, "large all-equal result");
    same.back() = INT_MAX;
    check(multisetPermutationUnrank(same, ulng(large - 1), out), "large one rare unrank");
    check(out.front() == INT_MAX && std::count(out.begin(), out.end(), INT_MIN) == large - 1, "large rare last permutation"); }
void oracle() {
    char mode;
    while (cin >> mode) {
        int n; cin >> n; vector<int> a(n); for (int &x : a) { cin >> x; } ulng requested; cin >> requested;
        ulng rank = 42, count = 42; vector<int> out{-42}; bool fits, ok;
        if (mode == 'D') {
            rank = permutationRank(a); count = 1; for (int i = 2; i <= n; ++i) { count *= i; }
            fits = true; ok = permutationUnrank(n, requested, out); }
        else {
            map<int, int> freq; for (int x : a) { ++freq[x]; } vector<int> counts;
            for (auto [x, c] : freq) { counts.push_back(c); }
            fits = multisetPermutationCount(counts, count);
            check(multisetPermutationRank(a, rank) == fits, "Python rank/count status agreement");
            ok = multisetPermutationUnrank(a, requested, out); }
        cout << fits << ' ' << count << ' ' << rank << ' ' << ok << ' ' << out.size();
        for (int x : out) { cout << ' ' << x; } cout << '\n'; }
    check(cin.eof(), "Python input protocol"); }
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--oracle") { oracle(); return 0; }
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i]; ulng x = 0; vector<int> out;
            if (p == "inverse-duplicate") { permutationInverse({0, 0}); }
            else if (p == "inverse-negative") { permutationInverse({-1}); }
            else if (p == "inverse-range") { permutationInverse({1}); }
            else if (p == "compose-size") { permutationCompose({}, {0}); }
            else if (p == "compose-invalid") { permutationCompose({0, 0}, {0, 1}); }
            else if (p == "power-invalid") { permutationPower({1, 1}, 0); }
            else if (p == "lehmer-invalid") { permutationLehmer({1, 1}); }
            else if (p == "digits-negative") { permutationFromLehmer({-1}); }
            else if (p == "digits-range") { permutationFromLehmer({1}); }
            else if (p == "rank-size") { permutationRank(vector<int>(21)); }
            else if (p == "rank-invalid") { permutationRank({0, 0}); }
            else if (p == "unrank-negative") { permutationUnrank(-1, 0, out); }
            else if (p == "unrank-size") { permutationUnrank(21, 0, out); }
            else if (p == "count-negative") { multisetPermutationCount({2, -1}, x); }
            else if (p == "count-size") { multisetPermutationCount({INT_MAX, 1}, x); }
            return 0; } }
    std::mt19937_64 rng(seed);
    distinct(mode == "quick" ? 6 : mode == "full" ? 8 : 9);
    cout << "PASS independently enumerated distinct order/rank/digits/algebra\n";
    multisetCases(mode == "quick" ? 2 : mode == "full" ? 3 : 4);
    cout << "PASS independently enumerated multisets and exact count boundaries\n";
    randomized(rng, mode == "quick" ? 100 : mode == "full" ? 3000 : 30000,
               mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000);
    cout << "PASS full-domain exponent oracle, aliases and large inputs\n"; }
