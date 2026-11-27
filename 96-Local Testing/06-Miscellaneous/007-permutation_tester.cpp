#include "../../06-Miscellaneous/007-permutation.hpp"
#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/06-modintmini.hpp"

using small = ModInt<13>;
using smallmini = ModIntMini<13>;
ulng seed = 20260927;
string context;
void check(bool ok, std::string_view s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << s << '\n'; std::exit(1); }}
string show(const vector<int> &a) {
    string s = "[";
    for (int x : a) { s += std::to_string(x) + ','; }
    return s + ']';}
void checkVector(const vector<int> &a, const vector<int> &b, std::string_view operation) {
    if (a != b) { check(false, string(operation) + " expected=" + show(b) + " actual=" + show(a)); }}
vector<int> iotaOf(int n) {
    vector<int> a(n); iota(a.begin(), a.end(), 0);
    return a;}
vector<vector<int>> enumerate(vector<int> counts, vector<int> keys) {
    vector<vector<int>> all; vector<int> p; int n = accumulate(counts.begin(), counts.end(), 0);
    auto go = [&](auto &&go) -> void {
        if (int(p.size()) == n) { all.push_back(p); return; }
        for (int i = 0; i < int(counts.size()); ++i) {
            if (counts[i]) { --counts[i]; p.push_back(keys[i]); go(go); p.pop_back(); ++counts[i]; }}};
    go(go); return all;}
vector<int> powerOracle(const vector<int> &p, lng k) {
    int n = int(p.size()); vector<int> r(n), q = p;
    iota(r.begin(), r.end(), 0);
    if (k < 0) { for (int i = 0; i < n; ++i) { q[p[i]] = i; } }
    ulng e = k < 0 ? ulng(0) - ulng(k) : ulng(k);
    while (e) {
        if (e & 1) { for (int &x : r) { x = q[x]; } }
        vector<int> square(n);
        for (int i = 0; i < n; ++i) { square[i] = q[q[i]]; }
        q = std::move(square); e >>= 1;}
    return r;}
void checkCycles(const vector<int> &p) {
    int n = int(p.size()); auto cycles = permutationCycles(p); vector<int> seen(n);
    int previous = -1, transpositions = 0;
    for (const auto &c : cycles) {
        check(!c.empty() && c[0] > previous && *min_element(c.begin(), c.end()) == c[0], "cycle starts at its minimum, ordered");
        previous = c[0]; transpositions += int(c.size()) - 1;
        for (int j = 0; j < int(c.size()); ++j) {
            check(0 <= c[j] && c[j] < n && !seen[c[j]]++, "cycles partition [0, n)");
            check(p[c[j]] == c[(j + 1) % c.size()], "cycle follows p");}}
    int inversions = 0;
    for (int i = 0; i < n; ++i) {
        for (int j = i + 1; j < n; ++j) { inversions += p[j] < p[i]; }}
    check(int(std::count(seen.begin(), seen.end(), 1)) == n && transpositions % 2 == inversions % 2, "cycle cover and parity");
    check(permutationSign(p) == (inversions % 2 ? -1 : 1), "sign equals inversion parity");}
ulng orderOracle(const vector<int> &p) {
    int n = int(p.size()); vector<int> id(n), q = p; iota(id.begin(), id.end(), 0); ulng t = 1;
    while (q != id) {
        for (int &x : q) { x = p[x]; }
        ++t;}
    return t;}
// An element without a default constructor, for permute and inversePermute.
struct Tag {
    int v;
    explicit Tag(int x) : v(x) {}
    bool operator==(const Tag &o) const { return v == o.v; }
};
// a[k] counts the cycles of length k, each counted once from its smallest element by a fresh walk.
vector<int> cycleTypeOracle(const vector<int> &p) {
    int n = int(p.size()); vector<int> a(n + 1);
    for (int i = 0; i < n; ++i) {
        int m = 1; bool smallest = true;
        for (int v = p[i]; v != i; v = p[v]) { smallest &= v > i; ++m; }
        if (smallest) { ++a[m]; }}
    return a;}
// Myrvold & Ruskey 2001, rank1 and unrank1 transcribed recursively: s = pi[n-1], two swaps, s + n * rank1(n - 1).
template<typename T>
T rankLinearOracle(int n, vector<int> &pi, vector<int> &inv) {
    if (n <= 1) { return T(0); }
    int s = pi[n - 1];
    std::swap(pi[n - 1], pi[inv[n - 1]]); std::swap(inv[s], inv[n - 1]);
    return T(s) + T(n) * rankLinearOracle<T>(n - 1, pi, inv);}
void unrankLinearOracle(int n, ulng r, vector<int> &pi) {
    if (n > 0) { std::swap(pi[n - 1], pi[r % ulng(n)]); unrankLinearOracle(n - 1, r / ulng(n), pi); }}
// Rounds of left-to-right passes, each collecting the next wanted values in order.
int collectOracle(const vector<int> &p) {
    int n = int(p.size()), next = 0, rounds = 0;
    while (next < n) {
        ++rounds;
        for (int x : p) { next += x == next; }}
    return rounds;}
// Arrays that one round of disjoint swaps sorts: every matching applied to the identity, recursing on the first free position.
std::set<vector<int>> oneRoundArrays(int n) {
    std::set<vector<int>> res; vector<int> a(n); vector<bool> used(n); iota(a.begin(), a.end(), 0);
    auto go = [&](auto &&go, int i) -> void {
        while (i < n && used[i]) { ++i; }
        if (i == n) { res.insert(a); return; }
        used[i] = true; go(go, i + 1);
        for (int j = i + 1; j < n; ++j) {
            if (used[j]) { continue; }
            used[j] = true; std::swap(a[i], a[j]); go(go, i + 1); std::swap(a[i], a[j]); used[j] = false;}
        used[i] = false;};
    go(go, 0); return res;}
int involutionRounds(const vector<int> &p) {
    bool identity = true, involution = true;
    for (int i = 0; i < int(p.size()); ++i) { identity &= p[i] == i; involution &= p[p[i]] == i; }
    return identity ? 0 : involution ? 1 : 2;}
void checkSwapRounds(const vector<int> &p, int expected) {
    int n = int(p.size()); auto rounds = swapRoundSort(p); vector<int> a = p, stamp(n, -1);
    check(int(rounds.size()) == expected, "swapRoundSort minimal round count expected=" + std::to_string(expected) + " actual=" + std::to_string(rounds.size()));
    for (int r = 0; r < int(rounds.size()); ++r) {
        for (auto [i, j] : rounds[r]) {
            check(0 <= i && i < j && j < n && stamp[i] != r && stamp[j] != r, "swapRoundSort disjoint swaps i < j round=" + std::to_string(r));
            stamp[i] = stamp[j] = r; std::swap(a[i], a[j]);}}
    bool sorted = true;
    for (int i = 0; i < n; ++i) { sorted &= a[i] == i; }
    check(sorted, "swapRoundSort rounds applied in order sort p");}
// cycleType, permutationFromCycles, permute/inversePermute, swapRoundSort, collectingNumbers and the linear rank on one permutation.
void checkConstructions(const vector<int> &p, int rounds, std::mt19937_64 &rng) {
    int n = int(p.size());
    checkVector(cycleType(p), cycleTypeOracle(p), "cycleType against minimum-start cycle walks");
    auto cycles = permutationCycles(p);
    checkVector(permutationFromCycles(n, cycles), p, "permutationFromCycles inverts permutationCycles");
    std::shuffle(cycles.begin(), cycles.end(), rng);
    for (auto &c : cycles) { std::rotate(c.begin(), c.begin() + ptrdiff_t(rng() % c.size()), c.end()); }
    checkVector(permutationFromCycles(n, cycles), p, "permutationFromCycles in shuffled order with rotated cycles");
    vector<int> a(n), b(n), c(n);
    for (int &x : a) { x = int(rng() % 7) - 3; }
    for (int i = 0; i < n; ++i) { b[i] = a[p[i]]; c[p[i]] = a[i]; }
    checkVector(permute(a, p), b, "permute b[i] = a[p[i]]"); checkVector(inversePermute(a, p), c, "inversePermute b[p[i]] = a[i]");
    checkVector(inversePermute(permute(a, p), p), a, "inversePermute undoes permute");
    vector<int> self = a; self = permute(self, p); checkVector(self, b, "permute assigned over its input");
    vector<string> s; vector<Tag> t;
    for (int x : a) { s.push_back("s" + std::to_string(x)); t.emplace_back(x); }
    auto ps = permute(s, p), is = inversePermute(s, p); auto pt = permute(t, p), it = inversePermute(t, p);
    bool ok = int(ps.size()) == n && int(is.size()) == n && int(pt.size()) == n && int(it.size()) == n;
    for (int i = 0; ok && i < n; ++i) { ok = ps[i] == s[p[i]] && is[p[i]] == s[i] && pt[i] == t[p[i]] && it[p[i]] == t[i]; }
    check(ok, "permute and inversePermute on string and non-default-constructible elements");
    checkSwapRounds(p, rounds);
    int collected = collectingNumbers(p), simulated = collectOracle(p);
    check(collected == simulated, "collectingNumbers simulation expected=" + std::to_string(simulated) + " actual=" + std::to_string(collected));
    vector<int> inv(n);
    for (int i = 0; i < n; ++i) { inv[p[i]] = i; }
    vector<int> pi = p, pinv = inv; ulng r = rankLinearOracle<ulng>(n, pi, pinv);
    pi = p; pinv = inv; mint rm = rankLinearOracle<mint>(n, pi, pinv);
    pi = p; pinv = inv; mintmini rs = rankLinearOracle<mintmini>(n, pi, pinv);
    check(permutationRankLinear<ulng>(p) == r && permutationRankLinear<mint>(p) == rm && permutationRankLinear<mintmini>(p) == rs, "rankLinear against recursive rank1 in ulng, mint, mintmini expected=" + std::to_string(r));
    ulng request = n <= 20 ? r : rng();
    vector<int> out{-42}, expect(n); iota(expect.begin(), expect.end(), 0); unrankLinearOracle(n, request, expect);
    check(permutationUnrankLinear(n, request, out), "unrankLinear exists rank=" + std::to_string(request));
    checkVector(out, expect, "unrankLinear against recursive unrank1 rank=" + std::to_string(request));
    if (n <= 20) { checkVector(out, p, "unrankLinear inverts rankLinear"); }}
void distinct(int limit, std::mt19937_64 &rng) {
    for (int n = 0; n <= limit; ++n) {
        vector<int> id(n); iota(id.begin(), id.end(), 0);
        auto all = enumerate(vector<int>(n, 1), id); vector<int> p = id;
        // The constructions run through n = 8 in every mode above quick, which keeps the stress sanitizer run light.
        bool constructions = n <= 8; std::set<vector<int>> one; map<vector<int>, ulng> types;
        vector<int> hit(constructions ? all.size() : 0);
        if (constructions) {
            one = oneRoundArrays(n); ulng involutions = 1, before = 1;
            for (int m = 2; m <= n; ++m) { ulng next = involutions + ulng(m - 1) * before; before = involutions; involutions = next; }
            context = "one-round arrays n=" + std::to_string(n);
            check(one.size() == involutions, "matching enumeration counts the involutions t(n) = t(n-1) + (n-1) t(n-2)");}
        for (ulng rank = 0; rank < all.size(); ++rank) {
            context = "distinct n=" + std::to_string(n) + " rank=" + std::to_string(rank);
            checkVector(p, all[rank], "next order independently enumerated");
            check(isPermutation(p), "valid permutation");
            check(permutationRank(p) == rank, "rank expected=" + std::to_string(rank));
            check(permutationRankMod<ulng>(p) == rank && permutationRankMod<mint>(p) == mint(rank) && permutationRankMod<mintmini>(p) == mintmini(rank), "rank reduced into T expected=" + std::to_string(rank));
            check(permutationRankModNaive<ulng>(p) == rank && permutationRankModNaive<mint>(p) == mint(rank) && permutationRankModNaive<mintmini>(p) == mintmini(rank), "naive rank reduced into T expected=" + std::to_string(rank));
            if (n <= 6 || rank % 11 == 0) {
                check(multisetPermutationRankMod<mint>(id, p) == mint(rank) && multisetPermutationRankMod<small>(p, p) == small(rank), "distinct multiset rank mod");
                check(multisetPermutationRankMod<mintmini>(id, p) == mintmini(rank) && multisetPermutationRankMod<smallmini>(p, p) == smallmini(rank), "distinct multiset rank mod mini");}
            vector<int> decoded{-42}; check(permutationUnrank(n, rank, decoded), "unrank exists");
            checkVector(decoded, p, "unrank");
            vector<int> digits(n), inv(n);
            for (int i = 0; i < n; ++i) {
                inv[p[i]] = i;
                for (int j = i + 1; j < n; ++j) { digits[i] += p[j] < p[i]; }}
            checkVector(permutationLehmer(p), digits, "Lehmer brute inversion counts");
            checkVector(permutationFromLehmer(digits), p, "Lehmer decoding");
            checkVector(permutationLehmerNaive(p), digits, "naive Lehmer brute inversion counts");
            checkVector(permutationFromLehmerNaive(digits), p, "naive Lehmer decoding");
            checkVector(permutationInverse(p), inv, "inverse");
            checkVector(permutationCompose(p, inv), id, "inverse composition identity");
            if (n <= 5 || rank % 101 == 0) {
                for (lng k : {-3, -1, 0, 1, 2, 5}) { checkVector(permutationPower(p, k), powerOracle(p, k), "small power=" + std::to_string(k)); }}
            checkCycles(p);
            if (constructions) {
                checkConstructions(p, p == id ? 0 : one.count(p) ? 1 : 2, rng);
                ulng lin = permutationRankLinear<ulng>(p);
                check(lin < all.size() && !hit[lin]++, "rankLinear bijection onto [0, n!) rank=" + std::to_string(lin));
                ++types[cycleTypeOracle(p)];}
            ulng order = 0; check(permutationOrder(p, order) && order == orderOracle(p), "order equals smallest identity power");
            check(permutationOrderMod<lll>(p) == lll(order) && permutationOrderMod<mint>(p) == mint(order) && permutationOrderMod<mintmini>(p) == mintmini(order), "order reduced into T");
            lll total = lll(all.size());
            for (lng k : {lng(0), lng(1), lng(-1), lng(-rank), lng(-rank) - 1, lng(all.size() - rank) - 1, lng(all.size() - rank),
                          lng(-2 * total - 1), lng(3 * total + 2), std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()}) {
                vector<int> q = p; lll target = lll(rank) + k;
                bool inside = 0 <= target && target < total;
                bool ok = kthNextPermutation(q, k);
                target %= total; if (target < 0) { target += total; }
                check(ok == inside, "kthNext wrap flag k=" + std::to_string(k));
                checkVector(q, all[size_t(target)], "kthNext lexicographic target k=" + std::to_string(k));}
            vector<int> previous = p; bool prev = std::prev_permutation(previous.begin(), previous.end());
            check(prev == (rank != 0), "previous bool"); checkVector(previous, all[(rank + all.size() - 1) % all.size()], "previous wrap");
            check(std::next_permutation(p.begin(), p.end()) == (rank + 1 < all.size()), "next bool");}
        checkVector(p, id, "next final wrap"); vector<int> old{-42}; ulng arrangements = 42;
        check(distinctPermutationCount(id, arrangements) && arrangements == all.size(), "distinct labels count n!");
        check(!permutationUnrank(n, all.size(), old) && old == vector<int>{-42}, "unrank absence preserves destination");
        if (!constructions) { continue; }
        context = "constructions n=" + std::to_string(n);
        check(!permutationUnrankLinear(n, all.size(), old) && !permutationUnrankLinear(n, ~0ULL, old) && old == vector<int>{-42}, "unrankLinear at n! and 2^64 - 1 absent, out preserved");
        ulng total = 0;
        for (auto [key, count] : types) {
            vector<int> a = key;
            ulng exact = 42; total += count;
            check(cycleTypeCount(a, exact) && exact == count, "cycleTypeCount against enumerated cycle types type=" + show(a) + " expected=" + std::to_string(count));
            check(cycleTypeCountMod<mint>(a) == mint(count) && cycleTypeCountMod<mintmini>(a) == mintmini(count) && cycleTypeCountMod<small>(a) == small(count), "cycleTypeCountMod mint/mintmini/13 type=" + show(a));
            a.resize(a.size() + 2); check(cycleTypeCount(a, exact) && exact == count && cycleTypeCountMod<mintmini>(a) == mintmini(count), "trailing zero counts");
            while (a.size() > 1 && a.back() == 0) { a.pop_back(); }
            check(cycleTypeCount(a, exact) && exact == count && cycleTypeCountMod<mint>(a) == mint(count), "trimmed type vector");}
        check(total == all.size(), "cycle type classes partition the n! permutations");}
    check(!isPermutation({0, 0}) && !isPermutation({-1}) && !isPermutation({1}), "invalid permutation predicate");
    vector<int> descending{2, 1, 1}, after{1, 2, 1};
    check(std::next_permutation(descending.begin(), descending.end(), std::greater<int>{}), "custom comparator next"); checkVector(descending, after, "descending comparator order");
    check(std::prev_permutation(descending.begin(), descending.end(), std::greater<int>{}), "custom comparator previous"); checkVector(descending, {2, 1, 1}, "descending comparator reverse");
    vector<string> words{"a", "a", "b"}; check(std::next_permutation(words.begin(), words.end()), "generic strings");
    check(words == vector<string>{"a", "b", "a"}, "generic duplicate sequence");
    vector<pair<int, int>> records{{1, 9}, {1, 7}, {2, 0}};
    auto cmp = [](auto a, auto b) -> bool { return a.first < b.first; };
    check(std::next_permutation(records.begin(), records.end(), cmp), "comparator equivalence classes");
    check(records[0].first == 1 && records[1].first == 2 && records[2].first == 1, "equivalent values need not be equal records");
    sort(records.begin(), records.end());
    vector<bool> bits{true, false, true};
    check(std::next_permutation(bits.begin(), bits.end()) && bits == vector<bool>{true, true, false}, "packed bool sequence");
    check(records == vector<pair<int, int>>{{1, 7}, {1, 9}, {2, 0}}, "records preserved");}
// Arrangements in all (each distinct arrangement once) strictly below target, by std::lexicographical_compare.
ulng belowOracle(const vector<vector<int>> &all, const vector<int> &target) {
    auto less = [&](const vector<int> &q) { return std::lexicographical_compare(q.begin(), q.end(), target.begin(), target.end()); };
    return ulng(std::count_if(all.begin(), all.end(), less));}
void checkBelow(const vector<int> &values, const vector<int> &target, ulng expected) {
    uint got[4] = {multisetPermutationRankMod<mint>(values, target).val(), multisetPermutationRankMod<mintmini>(values, target).val(), 0, 0};
    bool ok = got[0] == mint(expected).val() && got[1] == mintmini(expected).val();
    if (values.size() < 13) {
        got[2] = multisetPermutationRankMod<small>(values, target).val(); got[3] = multisetPermutationRankMod<smallmini>(values, target).val();
        ok = ok && got[2] == expected % 13 && got[3] == expected % 13;}
    if (!ok) {
        check(false, "multiset rank mod mint/mintmini/13/13-mini values=" + show(values) + " target=" + show(target) + " expected=" + std::to_string(expected) +
              " actual=" + show({int(got[0]), int(got[1]), int(got[2]), int(got[3])}));}}
void multisetCases(int limit, std::mt19937_64 &rng) {
    vector<int> alphabet{INT_MIN, -1, 0, 1, INT_MAX};
    for (int a = 0; a <= limit; ++a) {
        for (int b = 0; b <= limit; ++b) {
            for (int c = 0; c <= limit; ++c) {
                vector<int> counts{a, b, c}; auto all = enumerate(counts, {INT_MIN, 0, INT_MAX});
                context = "multiset counts=" + show(counts); ulng total = 42;
                check(multinomialExact({ulng(a), ulng(b), ulng(c)}, total) && total == all.size(), "multinomial independent enumeration");
                ulng arrangements = 42; int n = a + b + c;
                check(distinctPermutationCount(all[rng() % all.size()], arrangements) && arrangements == all.size(), "distinct count enumeration");
                for (int t = 0; t < 16; ++t) {
                    vector<int> target = all[rng() % all.size()];
                    if (t % 4 == 0) {
                        target.resize(rng() % ulng(n + 3));
                        for (int &x : target) { x = alphabet[rng() % 5]; }}
                    else if (t % 4 == 1) { target.resize(rng() % ulng(n + 1)); }
                    else if (t % 4 == 2) { target.push_back(alphabet[rng() % 5]); }
                    else if (n) { target[rng() % ulng(n)] = alphabet[rng() % 5]; }
                    checkBelow(all.back(), target, belowOracle(all, target));}
                vector<int> next = all.front();
                for (ulng i = 0; i < all.size(); ++i) {
                    checkVector(next, all[i], "multiset next order"); ulng rank = 42;
                    check(multisetPermutationRank(all[i], rank) && rank == i, "multiset rank expected=" + std::to_string(i));
                    checkBelow(all[(i * 7 + 3) % all.size()], all[i], i);
                    vector<int> out{-42}; check(multisetPermutationUnrank(all.back(), i, out), "multiset unrank exists"); checkVector(out, all[i], "multiset unrank order");
                    vector<int> previous = all[i]; check(std::prev_permutation(previous.begin(), previous.end()) == (i != 0), "multiset previous bool");
                    checkVector(previous, all[(i + all.size() - 1) % all.size()], "multiset previous order");
                    check(std::next_permutation(next.begin(), next.end()) == (i + 1 < all.size()), "multiset next bool");}
                vector<int> out{-42};
                check(!multisetPermutationUnrank(all[0], total, out) && out == vector<int>{-42}, "multiset absent preserves destination");}}}
    context = "multiset overflow boundaries"; ulng count = 42;
    check(multinomialExact({INT_MAX}, count) && count == 1, "huge all-equal count");
    check(multinomialExact({INT_MAX - 1, 1}, count) && count == ulng(INT_MAX), "huge one rare count");
    check(multinomialExact({INT_MAX - 2, 2}, count) && count == ulng(INT_MAX) * (INT_MAX - 1) / 2, "huge two rare count");
    count = 42; check(!multinomialExact({INT_MAX - 3, 3}, count) && count == 42, "huge count overflow preserves");
    check(multinomialExact({34, 33}, count) && count == 14226520737620288370ULL, "C(67,33) fits");
    count = 42; check(!multinomialExact({34, 34}, count) && count == 42, "C(68,34) overflow");
    vector<int> unique(21); iota(unique.begin(), unique.end(), 0); vector<int> out{-42};
    check(!multisetPermutationRank(unique, count) && count == 42, "21! rank overflow preserves");
    check(!multisetPermutationUnrank(unique, 0, out) && out == vector<int>{-42}, "21! unrank rejects overflow even rank0");
    vector<int> duplicate{0, 0, 1, 1, 2}; check(multisetPermutationUnrank(duplicate, 7, duplicate), "alias-safe multiset unrank");
    check(multisetPermutationRank(duplicate, count) && count == 7, "aliased result");
    context = "distinct count and multiset rank mod boundaries"; count = 42;
    check(distinctPermutationCount({}, count) && count == 1, "empty list has one arrangement");
    vector<int> two(67, INT_MAX); std::fill(two.begin(), two.begin() + 34, INT_MIN); std::shuffle(two.begin(), two.end(), rng);
    check(distinctPermutationCount(two, count) && count == 14226520737620288370ULL, "C(67,33) distinct count fits");
    two.push_back(INT_MAX); count = 42; check(!distinctPermutationCount(two, count) && count == 42, "C(68,34) distinct count overflow preserves");
    check(!distinctPermutationCount(unique, count) && count == 42, "21 distinct labels overflow preserves");
    unique.pop_back(); check(distinctPermutationCount(unique, count) && count == 2432902008176640000ULL, "20! fits");
    check(distinctPermutationCount({INT_MIN, INT_MAX, INT_MIN, -1}, count) && count == 12, "extreme labels count");
    check(multisetPermutationRankMod<mint>({}, {}) == mint(0) && multisetPermutationRankMod<mint>({}, {INT_MIN}) == mint(1), "empty values: equal target 0, nonempty target 1");
    check(multisetPermutationRankMod<mint>({5, 5, 7}, {}) == mint(0) && multisetPermutationRankMod<mint>({5, 5, 7}, {5}) == mint(0), "empty and prefix targets rank 0");
    check(multisetPermutationRankMod<mint>({5, 5, 7}, {7, 5, 5}) == mint(2) && multisetPermutationRankMod<mint>({5, 5, 7}, {7, 5, 5, INT_MIN}) == mint(3), "last arrangement and its extension");
    check(multisetPermutationRankMod<mint>({5, 5, 7}, {INT_MAX}) == mint(3) && multisetPermutationRankMod<small>({5, 5, 7}, {6}) == small(2), "absent target labels");
    check(multisetPermutationRankMod<mintmini>({}, {INT_MIN}) == mintmini(1) && multisetPermutationRankMod<smallmini>({5, 5, 7}, {7, 5, 5, 0}) == smallmini(3), "mini boundaries");
    vector<int> twelve{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11};
    check(multisetPermutationRankMod<small>(twelve, {11}) == small(11) && multisetPermutationRankMod<smallmini>(twelve, {11}) == smallmini(11), "n = mod - 1: 11 * 11! = 11 mod 13 (Wilson)");
    check(multisetPermutationRankMod<small>(twelve, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 11, 10}) == small(1), "n = mod - 1 rank 1");
    check(permutationRankMod<mint>({}) == mint(0) && permutationRankMod<ulng>({}) == 0, "empty permutation rank mod");}
void randomized(std::mt19937_64 &rng, int rounds, int large) {
    for (int rep = 0; rep < rounds; ++rep) {
        int n = int(rng() % 70); vector<int> p(n), q(n), expected(n); iota(p.begin(), p.end(), 0); q = p;
        std::shuffle(p.begin(), p.end(), rng); std::shuffle(q.begin(), q.end(), rng);
        context = "random rep=" + std::to_string(rep) + " p=" + show(p) + " q=" + show(q);
        for (int i = 0; i < n; ++i) { expected[i] = p[q[i]]; }
        checkVector(permutationCompose(p, q), expected, "composition order");
        for (lng k : {std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), std::bit_cast<lng>(rng())}) {
            checkVector(permutationPower(p, k), powerOracle(p, k), "full exponent domain k=" + std::to_string(k));}
        checkVector(permutationFromLehmer(permutationLehmer(p)), p, "large factorial-independent digits");
        vector<int> digits(n);
        for (int i = 0; i < n; ++i) { digits[i] = int(rng() % ulng(n - i)); }
        checkVector(permutationLehmerNaive(p), permutationLehmer(p), "naive and Fenwick digits agree");
        checkVector(permutationFromLehmerNaive(digits), permutationFromLehmer(digits), "naive and Fenwick decodes agree");
        checkVector(permutationLehmerNaive(permutationFromLehmerNaive(digits)), digits, "naive digit round trip");
        check(permutationRankModNaive<mint>(p) == permutationRankMod<mint>(p) && permutationRankModNaive<mintmini>(p) == permutationRankMod<mintmini>(p), "naive and Fenwick ranks agree mod p");
        checkCycles(p);
        checkConstructions(p, involutionRounds(p), rng);
        vector<int> w(n); iota(w.begin(), w.end(), 0);
        for (int i = 0; i + 1 < n; i += 2) {
            if (rng() % 3) { w[q[i]] = q[i + 1]; w[q[i + 1]] = q[i]; }}
        checkConstructions(w, involutionRounds(w), rng);
        ulng order = 0; check(permutationOrder(p, order) && permutationOrderMod<ulng>(p) == order, "random order exact and reduced");
        check(permutationPower(p, lng(order)) == permutationPower(p, 0), "order annihilates p");
        int m = rep % 21; vector<int> r(m), id(m); iota(id.begin(), id.end(), 0); r = id;
        if (rep % 7 == 1) { reverse(r.begin(), r.end()); }
        else if (rep % 7) { std::shuffle(r.begin(), r.end(), rng); }
        ulng expect = 0, fact = 1;
        for (int i = m - 1; i >= 0; --i) {
            ulng d = 0;
            for (int j = i + 1; j < m; ++j) { d += r[j] < r[i]; }
            expect += d * fact; fact *= ulng(m - i);}
        context = "scalar rank n=" + std::to_string(m) + " p=" + show(r);
        check(permutationRank(r) == expect && permutationRankMod<ulng>(r) == expect, "rank equals counting oracle expected=" + std::to_string(expect));
        vector<int> out{-42}, rev(id.rbegin(), id.rend());
        check(permutationUnrank(m, expect, out), "unrank counting rank"); checkVector(out, r, "unrank inverts rank");
        check(permutationUnrank(m, 0, out), "unrank 0"); checkVector(out, id, "unrank 0 is identity");
        check(permutationUnrank(m, fact - 1, out), "unrank n!-1"); checkVector(out, rev, "unrank n!-1 is reversed");
        check(!permutationUnrank(m, fact, out) && out == rev && !permutationUnrank(m, fact + rng() % (~0ULL - fact) + 1, out) && out == rev, "unrank past n! preserves out");
        context = "random rep=" + std::to_string(rep) + " p=" + show(p) + " q=" + show(q);
        lng k = rep % 3 ? lng(rng() % 2001) - 1000 : std::bit_cast<lng>(rng());
        vector<int> moved = p, walk = p; bool ok = kthNextPermutation(moved, k), flag = true;
        if (k >= -1000 && k <= 1000) {
            for (lng i = 0; i < (k < 0 ? -k : k); ++i) { flag &= k > 0 ? std::next_permutation(walk.begin(), walk.end()) : std::prev_permutation(walk.begin(), walk.end()); }
            checkVector(moved, walk, "kthNext equals repeated std steps k=" + std::to_string(k)); check(ok == flag, "kthNext flag equals any std wrap");}
        vector<int> back = moved; kthNextPermutation(back, k == std::numeric_limits<lng>::min() ? k + 1 : -k);
        if (k != std::numeric_limits<lng>::min()) { checkVector(back, p, "kthNext inverse offset"); }}
    {
        context = "order overflow boundary";
        vector<int> primes{2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53}, cyc;
        for (int c : primes) {
            int start = int(cyc.size());
            for (int j = 0; j < c; ++j) { cyc.push_back(start + (j + 1) % c); }
            if (c == 47) {
                ulng order = 0; check(permutationOrder(cyc, order) && order == 614889782588491410ULL, "product of first 15 primes fits");}}
        ulng order = 42; check(!permutationOrder(cyc, order) && order == 42, "first 16 primes overflow preserves out");
        check(permutationOrderMod<mint>(cyc) == mint(614889782588491410ULL) * 53 && permutationOrderMod<mintmini>(cyc) == mintmini(614889782588491410ULL) * 53, "reduced order past ulng");
        ulng one = 0; check(permutationOrder({}, one) && one == 1 && permutationSign({}) == 1 && permutationCycles({}).empty(), "empty permutation");
        vector<int> big(25); iota(big.begin(), big.end(), 0); vector<int> last(big.rbegin(), big.rend());
        check(!kthNextPermutation(last, 1) && last == big, "n > 21 wraps through the prefix to identity");
        check(kthNextPermutation(last, std::numeric_limits<lng>::max()) && !kthNextPermutation(last, std::numeric_limits<lng>::min()), "n > 21 extreme offsets");
        check(last == vector<int>(big.rbegin(), big.rend()) && !kthNextPermutation(last, 1) && last == big, "n > 21 backward wrap to the last permutation");}
    context = "large linear and nlogn domains n=" + std::to_string(large);
    vector<int> p(large); iota(p.begin(), p.end(), 0); reverse(p.begin(), p.end());
    auto d = permutationLehmer(p);
    for (int i = 0; i < large; ++i) { check(d[i] == large - i - 1, "large reverse Lehmer expected=" + std::to_string(large - i - 1)); }
    checkVector(permutationFromLehmer(d), p, "large Lehmer decode");
    mint fact = 1, cut = 1;
    for (int i = 2; i <= large; ++i) { fact *= i; }
    check(permutationRankMod<mint>(p) == fact - 1 && permutationRankMod<mintmini>(p) == mintmini(fact.val()) - 1, "large reversed rank n!-1 mod p");
    // The quadratic forms run at 5000 at most, which keeps the checked and sanitized builds fast.
    int t = min(large, 5000); vector<int> rev(t); iota(rev.begin(), rev.end(), 0); reverse(rev.begin(), rev.end()); mint ft = 1;
    for (int i = 2; i <= t; ++i) { ft *= i; }
    check(permutationRankModNaive<mint>(rev) == ft - 1 && permutationRankModNaive<mintmini>(rev) == mintmini(ft.val()) - 1, "naive reversed rank t!-1 mod p");
    auto dn = permutationLehmerNaive(rev); bool down = true;
    for (int i = 0; i < t; ++i) { down &= dn[i] == t - i - 1; }
    check(down, "naive reverse Lehmer"); checkVector(permutationFromLehmerNaive(dn), rev, "naive reverse decode");
    // m distinct labels cost O(m) per lower_bound under _GLIBCXX_DEBUG's sortedness check, so the multiset form stays at 5000.
    int m = min(large, 5000); vector<int> r(m); iota(r.begin(), r.end(), 0); reverse(r.begin(), r.end());
    for (int i = 2; i <= m; ++i) { cut *= i; }
    check(multisetPermutationRankMod<mint>(r, r) == cut - 1 && multisetPermutationRankMod<mintmini>(r, r) == mintmini(cut.val()) - 1, "long distinct multiset reversed rank m!-1");
    check(permutationRankMod<mint>(vector<int>(p.rbegin(), p.rend())) == mint(0), "large identity rank 0");
    checkVector(permutationPower(p, std::numeric_limits<lng>::min()), powerOracle(p, std::numeric_limits<lng>::min()), "large min exponent");
    for (int i = 0; i < large; ++i) { p[i] = (i + 1) % large; }
    checkVector(permutationPower(p, std::numeric_limits<lng>::min()), powerOracle(p, std::numeric_limits<lng>::min()), "large single cycle min exponent");
    vector<int> same(large, INT_MIN), out; ulng rank = 42;
    check(multisetPermutationRank(same, rank) && rank == 0, "large all-equal rank");
    ulng count = 42; check(distinctPermutationCount(same, count) && count == 1, "large all-equal count");
    check(multisetPermutationRankMod<mint>(same, same) == mint(0) && multisetPermutationRankMod<mint>(same, vector<int>(large + 1, INT_MIN)) == mint(1), "large all-equal rank mod and extension");
    check(multisetPermutationUnrank(same, 0, out), "large all-equal unrank"); checkVector(out, same, "large all-equal result");
    same.back() = INT_MAX;
    check(multisetPermutationUnrank(same, ulng(large - 1), out), "large one rare unrank");
    check(out.front() == INT_MAX && std::count(out.begin(), out.end(), INT_MIN) == large - 1, "large rare last permutation");
    check(distinctPermutationCount(out, count) && count == ulng(large), "large one rare count");
    check(multisetPermutationRankMod<mint>(same, out) == mint(large - 1) && multisetPermutationRankMod<mint>(same, {INT_MIN, INT_MAX}) == mint(large - 2), "large one rare rank mod");
    check(multisetPermutationRankMod<mintmini>(same, out) == mintmini(large - 1), "large one rare rank mintmini");}
void largeConstructions(int n, std::mt19937_64 &rng) {
    context = "cycleTypeCount exact boundaries"; ulng count = 42;
    check(cycleTypeCount({}, count) && count == 1 && cycleTypeCount({0}, count) && count == 1 && cycleTypeCount({0, 0, 0}, count) && count == 1, "n = 0 has one permutation");
    check(cycleTypeCountMod<mint>({}) == mint(1) && cycleTypeCountMod<mintmini>({0, 0}) == mintmini(1), "n = 0 mod");
    check(cycleTypeCount({0, INT_MAX}, count) && count == 1, "INT_MAX fixed points");
    check(cycleTypeCount({0, INT_MAX - 2, 1}, count) && count == ulng(INT_MAX) * (INT_MAX - 1) / 2, "one transposition at n = INT_MAX is C(n, 2)");
    vector<int> a21(22), a22(23); a21[21] = 1; a22[22] = 1; ulng fact20 = 1; mint fact21 = 1;
    for (int i = 2; i <= 20; ++i) { fact20 *= ulng(i); }
    for (int i = 2; i <= 21; ++i) { fact21 *= i; }
    check(cycleTypeCount(a21, count) && count == fact20, "a 21-cycle: 20! fits");
    count = 42; check(!cycleTypeCount(a22, count) && count == 42, "a 22-cycle: 21! overflows, out preserved");
    check(cycleTypeCountMod<mint>(a22) == fact21 && cycleTypeCountMod<mintmini>(a22) == mintmini(fact21.val()), "a 22-cycle mod p");
    context = "large constructions n=" + std::to_string(n);
    vector<int> lab = iotaOf(n), type(n + 1), p(n); std::shuffle(lab.begin(), lab.end(), rng); vector<vector<int>> cycles;
    for (int at = 0; at < n;) {
        int rest = n - at, len = cycles.empty() ? max(1, rest / 2) : rng() % 4 == 0 ? 2 : int(rng() % ulng(min(rest, 50))) + 1;
        len = min(len, rest); cycles.emplace_back(lab.begin() + at, lab.begin() + at + len); ++type[len]; at += len;}
    for (const auto &c : cycles) {
        for (int j = 0; j < int(c.size()); ++j) { p[c[j]] = c[(j + 1) % int(c.size())]; }}
    check(cycleType(p) == type, "cycleType of a constructed conjugated cycle structure");
    std::shuffle(cycles.begin(), cycles.end(), rng);
    for (auto &c : cycles) { std::rotate(c.begin(), c.begin() + ptrdiff_t(rng() % c.size()), c.end()); }
    check(permutationFromCycles(n, cycles) == p, "permutationFromCycles from the constructed cycles, shuffled and rotated");
    checkSwapRounds(p, 2);
    vector<int> w = iotaOf(n);
    for (int i = 0; i + 1 < n; i += 2) {
        if (rng() % 2) { w[lab[i]] = lab[i + 1]; w[lab[i + 1]] = lab[i]; }}
    checkSwapRounds(w, 1); checkSwapRounds(iotaOf(n), 0);
    vector<int> a(n), b(n), c(n);
    for (int &x : a) { x = int(rng() % 2001) - 1000; }
    for (int i = 0; i < n; ++i) { b[i] = a[p[i]]; c[p[i]] = a[i]; }
    check(permute(a, p) == b && inversePermute(a, p) == c, "large permute and inversePermute");
    vector<int> chunks;
    for (int r = 2; r >= 0; --r) {
        for (int v = r * n / 3; v < (r + 1) * n / 3; ++v) { chunks.push_back(v); }}
    vector<int> rev(lab.size()); iota(rev.rbegin(), rev.rend(), 0);
    check(collectingNumbers(iotaOf(n)) == collectOracle(iotaOf(n)) && collectingNumbers(chunks) == collectOracle(chunks), "large identity (1) and three descending chunks (3) by simulation");
    check(collectingNumbers(rev) == n, "large reversed collects one value per round");
    // Swaps from random digits d_k < k build the permutation of rank d_n + n (d_{n-1} + (n-1) (...)) by unrank1.
    vector<int> pi = iotaOf(n), digits(n + 1); mint want = 0; mintmini wants = 0;
    for (int k = 2; k <= n; ++k) { digits[k] = int(rng() % ulng(k)); }
    for (int k = 2; k <= n; ++k) { want = mint(digits[k]) + mint(k) * want; wants = mintmini(digits[k]) + mintmini(k) * wants; }
    for (int k = n; k > 1; --k) { std::swap(pi[k - 1], pi[digits[k]]); }
    check(permutationRankLinear<mint>(pi) == want && permutationRankLinear<mintmini>(pi) == wants, "large rankLinear of unrank1 digits mod p");
    ulng r = rng(); vector<int> out{-42}, expect = iotaOf(n); ulng left = r;
    for (int k = n; k > 0; --k) { std::swap(expect[k - 1], expect[left % ulng(k)]); left /= ulng(k); }
    check(permutationUnrankLinear(n, r, out) && out == expect && permutationRankLinear<ulng>(out) == r, "large unrankLinear of a 64-bit rank and back");}
void extremeCycles(std::mt19937_64 &rng) {
    context = "extreme cycles";
    // A descending single cycle makes any walk that stops early at a larger label quadratic.
    int n = 1000000; vector<int> p(n);
    for (int i = 0; i < n; ++i) { p[i] = (i + n - 1) % n; }
    for (lng k : {lng(1), lng(-1), lng(999999), lng(3) * n + 7, std::numeric_limits<lng>::min()}) {
        vector<int> q = permutationPower(p, k); lng s = ((k % n) + n) % n; bool ok = true;
        for (int i = 0; i < n; ++i) { ok &= q[i] == int((i - s + n) % n); }
        check(ok, "descending cycle power k=" + std::to_string(k));}
    checkSwapRounds(p, 2);
    vector<int> shuffled = iotaOf(n); std::shuffle(shuffled.begin(), shuffled.end(), rng);
    checkSwapRounds(shuffled, involutionRounds(shuffled));
    // Cycles of the prime factors of 2^64 - 1 give an order that exactly fills ulng.
    vector<int> cyc; ulng order = 0;
    for (int len : {3, 5, 17, 257, 641, 65537, 6700417}) {
        int start = int(cyc.size());
        for (int j = 0; j < len; ++j) { cyc.push_back(start + (j + 1) % len); }}
    check(permutationOrder(cyc, order) && order == std::numeric_limits<ulng>::max(), "order exactly 2^64 - 1 fits");
    check(permutationOrderMod<lll>(cyc) == lll(std::numeric_limits<ulng>::max()), "order 2^64 - 1 in lll");}
void oracle() {
    char mode;
    while (cin >> mode) {
        int n; cin >> n; vector<int> a(n); for (int &x : a) { cin >> x; } ulng requested; cin >> requested;
        ulng rank = 42, count = 42; vector<int> out{-42}; bool fits, ok; uint modrank;
        if (mode == 'R') {
            cout << permutationRankMod<ulng>(a) << ' ' << permutationRankMod<mint>(a).val() << ' ' << multisetPermutationRankMod<mint>(a, a).val() << ' ' << multisetPermutationRankMod<mintmini>(a, a).val() << '\n';
            continue;}
        if (mode == 'P') {
            vector<int> t(requested);
            for (int &x : t) { cin >> x; }
            fits = distinctPermutationCount(a, count);
            cout << multisetPermutationRankMod<mint>(a, t).val() << ' ' << multisetPermutationRankMod<mintmini>(a, t).val() << ' ' << fits << ' ' << count << '\n';
            continue;}
        if (mode == 'K' || mode == 'O') {
            lng k = std::bit_cast<lng>(requested);
            if (mode == 'K') { bool ok2 = kthNextPermutation(a, k); cout << ok2; for (int x : a) { cout << ' ' << x; } cout << '\n'; }
            else { ulng order = 0; bool fit = permutationOrder(a, order); cout << fit << ' ' << (fit ? order : 0) << ' ' << permutationOrderMod<mint>(a).val() << ' ' << permutationSign(a) << ' ' << permutationCycles(a).size() << '\n'; }
            continue;}
        if (mode == 'C') {
            ulng exact = 42; bool fit = cycleTypeCount(a, exact);
            cout << fit << ' ' << exact << ' ' << cycleTypeCountMod<mint>(a).val() << ' ' << cycleTypeCountMod<mintmini>(a).val() << '\n';
            continue;}
        if (mode == 'L') {
            bool found = permutationUnrankLinear(n, requested, out);
            cout << permutationRankLinear<ulng>(a) << ' ' << permutationRankLinear<mint>(a).val() << ' ' << permutationRankLinear<mintmini>(a).val() << ' ' << found << ' ' << out.size();
            for (int x : out) { cout << ' ' << x; } cout << '\n';
            continue;}
        if (mode == 'D') {
            rank = permutationRank(a); count = 1; for (int i = 2; i <= n; ++i) { count *= i; }
            fits = true; ok = permutationUnrank(n, requested, out); modrank = permutationRankMod<mint>(a).val();
            check(permutationRankMod<ulng>(a) == rank, "Python rank mod ulng agreement");}
        else {
            map<int, int> freq; for (int x : a) { ++freq[x]; } vector<ulng> counts;
            for (auto [x, c] : freq) { counts.push_back(ulng(c)); }
            ulng exact = 42; fits = distinctPermutationCount(a, count);
            check(multinomialExact(counts, exact) == fits && exact == count, "Python distinct count/multinomial agreement");
            check(multisetPermutationRank(a, rank) == fits, "Python rank/count status agreement");
            ok = multisetPermutationUnrank(a, requested, out); modrank = multisetPermutationRankMod<mint>(a, a).val();}
        cout << fits << ' ' << count << ' ' << rank << ' ' << ok << ' ' << modrank << ' ' << out.size();
        for (int x : out) { cout << ' ' << x; } cout << '\n';}
    check(cin.eof(), "Python input protocol");}
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
            else if (p == "kth-invalid") { vector<int> q{0, 0}; kthNextPermutation(q, 1); }
            else if (p == "cycles-invalid") { permutationCycles({1, 1}); }
            else if (p == "sign-invalid") { permutationSign({2}); }
            else if (p == "order-invalid") { permutationOrder({0, 0}, x); }
            else if (p == "order-mod-invalid") { permutationOrderMod<ulng>({-1}); }
            else if (p == "rank-mod-invalid") { permutationRankMod<mint>({0, 0}); }
            else if (p == "rank-mod-naive-invalid") { permutationRankModNaive<mint>({0, 0}); }
            else if (p == "lehmer-naive-invalid") { permutationLehmerNaive({1, 1}); }
            else if (p == "digits-naive-negative") { permutationFromLehmerNaive({-1}); }
            else if (p == "digits-naive-range") { permutationFromLehmerNaive({1}); }
            else if (p == "digits-naive-late") { permutationFromLehmerNaive({0, 1}); }
            else if (p == "digits-naive-over") { permutationFromLehmerNaive({2}); }
            else if (p == "multiset-rank-mod-size") { multisetPermutationRankMod<small>(vector<int>(13), {}); }
            else if (p == "multiset-rank-mod-size-mini") { multisetPermutationRankMod<smallmini>(vector<int>(13), {}); }
            else if (p == "cycle-type-invalid") { cycleType({1, 1}); }
            else if (p == "from-cycles-range") { permutationFromCycles(2, vector<vector<int>>{{0, 2}}); }
            else if (p == "from-cycles-negative-label") { permutationFromCycles(1, vector<vector<int>>{{-1}}); }
            else if (p == "from-cycles-duplicate") { permutationFromCycles(2, vector<vector<int>>{{0, 1}, {1}}); }
            else if (p == "from-cycles-missing") { permutationFromCycles(3, vector<vector<int>>{{0, 2}}); }
            else if (p == "from-cycles-negative-n") { permutationFromCycles(-1, {}); }
            else if (p == "from-cycles-huge") { permutationFromCycles(2, vector<vector<int>>{{0, INT_MAX}}); }
            else if (p == "from-cycles-repeat") { permutationFromCycles(2, vector<vector<int>>{{0}, {1}, {1}}); }
            else if (p == "from-cycles-repeat-zero") { permutationFromCycles(1, vector<vector<int>>{{0}, {0}}); }
            else if (p == "permute-size") { permute(vector<int>{5, 6}, {0}); }
            else if (p == "permute-size-smaller") { permute(vector<int>{5}, {0, 1}); }
            else if (p == "permute-invalid") { permute(vector<int>{5, 6}, {1, 1}); }
            else if (p == "inverse-permute-size") { inversePermute(vector<string>{"a"}, {0, 1}); }
            else if (p == "inverse-permute-size-larger") { inversePermute(vector<string>{"a", "b"}, {0}); }
            else if (p == "inverse-permute-invalid") { inversePermute(vector<string>{"a", "b"}, {0, 2}); }
            else if (p == "swap-rounds-invalid") { swapRoundSort({0, 0}); }
            else if (p == "collecting-invalid") { collectingNumbers({1}); }
            else if (p == "rank-linear-invalid") { permutationRankLinear<ulng>({0, 0}); }
            else if (p == "unrank-linear-negative") { permutationUnrankLinear(-1, 0, out); }
            else if (p == "cycle-count-zero") { cycleTypeCount({1}, x); }
            else if (p == "cycle-count-negative-zero") { cycleTypeCount({-1}, x); }
            else if (p == "cycle-count-negative") { cycleTypeCount({0, 2, -1}, x); }
            else if (p == "cycle-count-size") { cycleTypeCount({0, INT_MAX, 1}, x); }
            else if (p == "cycle-count-mod-zero") { cycleTypeCountMod<mint>({1}); }
            else if (p == "cycle-count-mod-negative") { cycleTypeCountMod<mintmini>({0, -1}); }
            else if (p == "cycle-count-mod-size") { cycleTypeCountMod<mint>({0, INT_MAX, 1}); }
            return 0;}}
    std::mt19937_64 rng(seed);
    distinct(mode == "quick" ? 6 : mode == "full" ? 8 : 9, rng);
    cout << "PASS independently enumerated distinct order/rank/digits/algebra\n";
    multisetCases(mode == "quick" ? 2 : mode == "full" ? 3 : 4, rng);
    cout << "PASS independently enumerated multisets, ranks below arbitrary targets and exact count boundaries\n";
    randomized(rng, mode == "quick" ? 100 : mode == "full" ? 3000 : 30000,
               mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000);
    cout << "PASS full-domain exponent oracle, aliases and large inputs\n";
    largeConstructions(mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000, rng);
    cout << "PASS cycleTypeCount boundaries; large cycle structures, swap rounds and linear ranks\n";
    if (mode != "quick") {
        extremeCycles(rng);
        cout << "PASS million-element descending cycle powers, swap rounds and the order 2^64 - 1\n";}}
