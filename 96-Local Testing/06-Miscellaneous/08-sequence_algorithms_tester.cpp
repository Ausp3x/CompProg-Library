#include "../../06-Miscellaneous/08-sequence_algorithms.hpp"

ulng seed = 20260928;
string context;
void check(bool ok, const string &s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " smallest known reproducer=" << context << " operation=" << s << " expected=true actual=false" << '\n'; std::exit(1); } }
string number(lll x) {
    if (x == 0) { return "0"; }
    bool neg = x < 0; if (neg) { x = -x; } string s;
    while (x) { s += char('0' + x % 10); x /= 10; }
    if (neg) { s += '-'; } reverse(s.begin(), s.end()); return s; }
template<typename T>
string show(const vector<T> &a) {
    string s = "["; for (const auto &x : a) { s += number(lll(x)) + ','; } return s + ']'; }
void checkEqual(lll actual, lll expected, const string &s) {
    check(actual == expected, s + " expected=" + number(expected) + " actual=" + number(actual)); }

template<typename T>
SubarraySum subarrayOracle(const vector<T> &a, int lo, int hi) {
    SubarraySum best; int n = int(a.size());
    for (int l = 0; l <= n; ++l) {
        lll sum = 0;
        for (int r = l; r <= n; ++r) {
            if (r > l) { sum += a[r - 1]; }
            if (lo <= r - l && r - l <= hi && (!best.exists() || sum > best.sum)) { best = {sum, l, r}; } } }
    return best; }
template<typename T>
void subarrayWitness(const vector<T> &a, SubarraySum got, SubarraySum want, int lo, int hi) {
    check(got.exists() == want.exists(), "subarray presence expected=" + std::to_string(want.exists()));
    checkEqual(got.sum, want.sum, "subarray optimum");
    if (!got.exists()) { check(got.l == -1 && got.r == -1, "absent coordinates"); return; }
    int n = int(a.size()); check(0 <= got.l && got.l <= got.r && got.r <= n, "subarray bounds");
    check(lo <= got.r - got.l && got.r - got.l <= hi, "subarray length");
    lll sum = 0; for (int i = got.l; i < got.r; ++i) { sum += a[i]; } checkEqual(sum, got.sum, "subarray witness"); }
template<typename T>
void sums(const vector<T> &a, bool all_bounds) {
    context = "sums a=" + show(a); int n = int(a.size());
    for (bool empty : {false, true}) {
        subarrayWitness(a, maximumSubarray(a, empty), subarrayOracle(a, !empty, n), !empty, n);
        auto got = circularMaximumSubarray(a, empty); bool found = empty; lll best = 0;
        for (int l = 0; l < n; ++l) {
            lll sum = 0;
            for (int k = 1; k <= n; ++k) {
                sum += a[(l + k - 1) % n];
                if (!found || sum > best) { best = sum; found = true; } } }
        check(got.exists() == found, "circular presence"); checkEqual(got.sum, best, "circular optimum");
        if (got.exists()) {
            check(got.start >= 0 && got.start < max(1, n) && !empty <= got.length && got.length <= n, "circular bounds");
            lll sum = 0; for (int i = 0; i < got.length; ++i) { sum += a[(got.start + i) % n]; }
            checkEqual(sum, got.sum, "circular witness"); }
        else { check(got.start == -1 && got.length == 0, "circular absence"); } }
    auto oracle = subarrayOracle(a, 1, n);
    if (std::numeric_limits<lng>::min() <= oracle.sum && oracle.sum <= std::numeric_limits<lng>::max()) {
        checkEqual(kadane(a), oracle.sum, "legacy widened kadane"); }
    if (all_bounds) {
        for (int lo = 0; lo <= n + 1; ++lo) {
            for (int hi = lo; hi <= n + 1; ++hi) {
                context = "bounded a=" + show(a) + " lo=" + std::to_string(lo) + " hi=" + std::to_string(hi);
                subarrayWitness(a, boundedMaximumSubarray(a, lo, hi), subarrayOracle(a, lo, hi), lo, hi); } } }
    subarrayWitness(a, boundedMaximumSubarray(a, 0, INT_MAX), subarrayOracle(a, 0, n), 0, INT_MAX); }

template<typename T>
void rectangle(const vector<vector<T>> &a) {
    int n = int(a.size()), m = n ? int(a[0].size()) : 0;
    context = "rectangle "; for (const auto &row : a) { context += show(row); }
    for (bool empty : {false, true}) {
        bool found = empty; lll best = 0;
        for (int x = 0; x < n; ++x) { for (int y = 0; y < m; ++y) {
            for (int xx = x + 1; xx <= n; ++xx) { for (int yy = y + 1; yy <= m; ++yy) {
                lll sum = 0;
                for (int i = x; i < xx; ++i) { for (int j = y; j < yy; ++j) { sum += a[i][j]; } }
                if (!found || sum > best) { found = true; best = sum; } } } } }
        auto got = maximumSubrectangle(a, empty); check(got.exists() == found, "rectangle presence"); checkEqual(got.sum, best, "rectangle optimum");
        if (!got.exists()) { check(got.top == -1 && got.left == -1 && got.bottom == -1 && got.right == -1, "rectangle absence"); continue; }
        check(0 <= got.top && got.top <= got.bottom && got.bottom <= n && 0 <= got.left && got.left <= got.right && got.right <= m, "rectangle bounds");
        check(empty || (got.top < got.bottom && got.left < got.right), "rectangle nonempty");
        lll sum = 0; for (int i = got.top; i < got.bottom; ++i) { for (int j = got.left; j < got.right; ++j) { sum += a[i][j]; } }
        checkEqual(sum, got.sum, "rectangle witness"); } }

template<typename T, typename Compare>
void sequenceWitness(const vector<T> &a, const vector<int> &idx, bool strict, Compare cmp) {
    int last = -1;
    for (int i : idx) {
        check(last < i && i < int(a.size()), "subsequence indices");
        if (last >= 0) { check(strict ? cmp(a[last], a[i]) : !cmp(a[i], a[last]), "subsequence order"); }
        last = i; } }
template<typename T, typename W, typename Compare = std::less<T>>
void subsequences(const vector<T> &a, const vector<W> &w, Compare cmp = {}) {
    int n = int(a.size()); context = "subsequences a=" + show(a) + " w=" + show(w);
    for (bool strict : {false, true}) {
        int length = 0; ulng count = 0; lll best = 0; bool found = false;
        for (uint mask = 0; mask < (1U << n); ++mask) {
            int last = -1, len = 0; bool valid = true; lll sum = 0;
            for (int i = 0; i < n; ++i) {
                if ((mask >> i) & 1) {
                    if (last >= 0 && !(strict ? cmp(a[last], a[i]) : !cmp(a[i], a[last]))) { valid = false; }
                    last = i; ++len; sum += w[i]; } }
            if (!valid) { continue; }
            if (len > length) { length = len; count = 0; }
            if (len == length) { ++count; }
            if (mask && (!found || sum > best)) { best = sum; found = true; } }
        auto idx = longestIncreasingSubsequence(a, strict, cmp); sequenceWitness(a, idx, strict, cmp);
        checkEqual(idx.size(), length, "LIS length");
        auto got = countLongestIncreasingSubsequences<ulng>(a, strict, cmp);
        checkEqual(got.first, length, "LIS counted length"); checkEqual(got.second, count, "index-distinct LIS count");
        for (bool empty : {false, true}) {
            auto weighted = weightedIncreasingSubsequence(a, w, strict, empty, cmp);
            check(weighted.found == (found || empty), "weighted presence"); checkEqual(weighted.sum, empty ? max(lll(0), best) : best, "weighted optimum");
            sequenceWitness(a, weighted.indices, strict, cmp); check(!weighted.found || empty || !weighted.indices.empty(), "weighted nonempty");
            lll sum = 0; for (int i : weighted.indices) { sum += w[i]; } checkEqual(sum, weighted.sum, "weighted witness"); } }
    lng inversions = 0;
    for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { inversions += cmp(a[j], a[i]); } }
    checkEqual(inversionCount(a, cmp), inversions, "pairwise inversion oracle"); }

void frequency(const vector<int> &a) {
    context = "frequency a=" + show(a); map<int, int> counts;
    for (int x : a) { ++counts[x]; } int n = int(a.size());
    auto got = majorityElement(a); int value = 0, count = 0;
    for (auto [x, c] : counts) { if (c > n / 2) { value = x; count = c; } }
    checkEqual(got.second, count, "majority exact count");
    check(count ? (got.first >= 0 && got.first < n && a[got.first] == value) : got.first == -1, "verified majority index");
    for (int k = 1; k <= n + 2; ++k) {
        vector<pair<int, int>> want;
        for (auto [x, c] : counts) { if (c > n / k) { want.emplace_back(x, c); } }
        check(heavyHitters(a, k) == want, "verified Misra-Gries k=" + std::to_string(k)); }
    check(heavyHitters(a, INT_MAX) == vector<pair<int, int>>(counts.begin(), counts.end()), "k beyond n"); }

void windows(const vector<int> &a) {
    context = "windows a=" + show(a); int n = int(a.size());
    for (int limit = 0; limit <= 8; ++limit) {
        auto got = nonnegativeSumWindowEnds(a, limit);
        for (int l = 0; l < n; ++l) {
            int r = l; lng sum = 0;
            while (r < n && sum + a[r] <= limit) { sum += a[r++]; }
            checkEqual(got[l], r, "nonnegative window end l=" + std::to_string(l) + " limit=" + std::to_string(limit)); } }
    map<int, int> count; int adds = 0, removes = 0;
    auto ends = monotoneWindowEnds(n, [&](int r) { auto it = count.find(a[r]); return it == count.end() || it->second == 0; },
        [&](int r) { ++count[a[r]]; ++adds; }, [&](int l) { --count[a[l]]; ++removes; });
    check(adds == n && removes == n, "generic callbacks balance");
    for (int l = 0; l < n; ++l) {
        set<int> seen; int r = l; while (r < n && seen.insert(a[r]).second) { ++r; }
        checkEqual(ends[l], r, "generic distinct window end"); }
    for (int k = 1; k <= n + 1; ++k) {
        auto mn = slidingMinimum(a, k), mx = slidingMaximum(a, k), right = slidingMinimum(a, k, true);
        checkEqual(mn.size(), max(0, n - k + 1), "shared window size");
        for (int l = 0; l + k <= n; ++l) {
            int low = l, high = l, last = l;
            for (int r = l; r < l + k; ++r) {
                if (a[r] < a[low]) { low = r; }
                if (a[r] > a[high]) { high = r; }
                if (a[r] <= a[last]) { last = r; } }
            checkEqual(mn[l], low, "shared leftmost minimum"); checkEqual(mx[l], high, "shared leftmost maximum"); checkEqual(right[l], last, "shared rightmost minimum"); } }
    auto sorted = a; sort(sorted.begin(), sorted.end());
    for (int target = -1; target <= 10; ++target) {
        auto got = sortedPairSum(sorted, target); bool exists = false;
        for (int i = 0; i < n; ++i) { for (int j = i + 1; j < n; ++j) { exists |= sorted[i] + sorted[j] == target; } }
        check((got.first >= 0) == exists, "sorted two-pointer presence");
        if (exists) { check(0 <= got.first && got.first < got.second && got.second < n, "pair bounds"); checkEqual(sorted[got.first] + sorted[got.second], target, "pair sum"); }
        else { check(got == pair{-1, -1}, "pair absence"); } } }

void extremesAndGeneric(int large) {
#ifdef _GLIBCXX_DEBUG
    large = min(large, 2000); // Debug STL validates entire search ranges; correctness domains are unchanged.
#endif
    vector<lng> signed_values{std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max(), -1, 0, 1};
    vector<ulng> unsigned_values{0, std::numeric_limits<ulng>::max(), 1};
    for (lng x : signed_values) { for (lng y : signed_values) { for (lng z : signed_values) { sums(vector<lng>{x, y, z}, true); subsequences(vector<lng>{x, y, z}, vector<lng>{z, y, x}); } } }
    for (ulng x : unsigned_values) { for (ulng y : unsigned_values) { for (ulng z : unsigned_values) { sums(vector<ulng>{x, y, z}, true); subsequences(vector<ulng>{x, y, z}, vector<ulng>{z, y, x}); } } }
    rectangle(vector<vector<ulng>>(3, vector<ulng>(4, std::numeric_limits<ulng>::max())));
    rectangle(vector<vector<lng>>(4, vector<lng>(3, std::numeric_limits<lng>::min())));
    context = "generic comparator equivalence";
    auto cmp = [](int x, int y) { return x / 10 < y / 10; };
    subsequences(vector<int>{11, 19, 21, 10, 25}, vector<int>{-2, 7, 8, 5, -9}, cmp);
    auto heavy = heavyHitters(vector<int>{11, 12, 29, 15}, 2, cmp);
    check(heavy.size() == 1 && heavy[0].first / 10 == 1 && heavy[0].second == 3, "heavy hitter comparator equivalence");
    auto majority = majorityElement(vector<int>{11, 12, 29, 15}, [](int x, int y) { return x / 10 == y / 10; });
    checkEqual(majority.second, 3, "majority equality relation");
    vector<string> text{"b", "a", "a", "c"};
    auto idx = longestIncreasingSubsequence(text, false); sequenceWitness(text, idx, false, std::less<string>{}); checkEqual(idx.size(), 3, "string LIS");
    check(countLongestIncreasingSubsequences<ulng>(text) == pair<int, ulng>{2, 3}, "string LIS count");
    checkEqual(inversionCount(text), 2, "string inversion");
    check(heavyHitters(text, 3) == vector<pair<string, int>>{{"a", 2}}, "string heavy hitter");
    vector<int> doubled; for (int i = 0; i < 100; ++i) { doubled.push_back(i); doubled.push_back(i); }
    using Big = lll;
    auto exact = countLongestIncreasingSubsequences<Big>(doubled);
    check(exact.first == 100 && exact.second == (Big(1) << 100), "exact 128-bit 2^100 LIS count");
    checkEqual(countLongestIncreasingSubsequences<ulng>(doubled).second, 0, "intentional unsigned mod2^64 counting");
    vector<ulng> big{std::numeric_limits<ulng>::max(), std::numeric_limits<ulng>::max()};
    check(sortedPairSum(big, 2 * lll(big[0])) == pair{0, 1}, "wide pair sum");
    check(nonnegativeSumWindowEnds(big, std::numeric_limits<lll>::max()) == vector<int>{2, 2}, "maximum lll window limit");
    vector<int> a(large); iota(a.begin(), a.end(), 0);
    context = "large monotone n=" + std::to_string(large);
    checkEqual(longestIncreasingSubsequence(a).size(), large, "large LIS");
    checkEqual(countLongestIncreasingSubsequences<ulng>(a).second, 1, "large count");
    checkEqual(weightedIncreasingSubsequence(a, vector<int>(large, 1)).sum, large, "large weighted");
    reverse(a.begin(), a.end()); checkEqual(inversionCount(a), lng(large) * (large - 1) / 2, "large reverse inversion");
    vector<int> ones(large, 1); checkEqual(maximumSubarray(ones).sum, large, "large linear sum");
    checkEqual(circularMaximumSubarray(ones).sum, large, "large circular sum");
    checkEqual(boundedMaximumSubarray(ones, large / 2, large / 2).sum, large / 2, "large exact-length sum");
    auto ends = nonnegativeSumWindowEnds(ones, 31);
    for (int i = 0; i < large; ++i) { checkEqual(ends[i], min(i + 31, large), "large window"); }
    auto hh = heavyHitters(a, 97); check(hh.empty(), "large cancellation blocks");
    PrefixSum<lll> prefix(vector<lll>{1, -2, 4}); checkEqual(prefix.sum(1, 3), 2, "shared PrefixSum");
    DifferenceArray<lll> difference(vector<lll>{1, -2, 4}); difference.add(0, 2, 3);
    check(difference.values() == vector<lll>{4, 1, 4}, "shared DifferenceArray");
    PrefixSum2D<lll> prefix2(vector<vector<lll>>{{1, 2}, {3, 4}}); checkEqual(prefix2.sum(0, 1, 2, 2), 6, "shared PrefixSum2D");
    DifferenceArray2D<lll> diff2(2, 2); diff2.add(0, 0, 2, 1, 4);
    check(diff2.values() == vector<vector<lll>>{{4, 0}, {4, 0}}, "shared DifferenceArray2D"); }

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i];
            if (p == "negative-length") { boundedMaximumSubarray(vector<int>{1}, -1, 1); }
            else if (p == "reversed-length") { boundedMaximumSubarray(vector<int>{1}, 2, 1); }
            else if (p == "ragged") { maximumSubrectangle(vector<vector<int>>{{}, {1}}); }
            else if (p == "weight-size") { weightedIncreasingSubsequence(vector<int>{1}, vector<int>{}); }
            else if (p == "negative-window") { nonnegativeSumWindowEnds(vector<int>{-1}, 0); }
            else if (p == "negative-limit") { nonnegativeSumWindowEnds(vector<int>{}, -1); }
            else if (p == "bad-k") { heavyHitters(vector<int>{}, 0); }
            else if (p == "unsorted") { sortedPairSum(vector<int>{2, 1}, 3); }
            else if (p == "legacy-overflow") { kadane(vector<ulng>{std::numeric_limits<ulng>::max()}); }
            return 0; } }
    std::mt19937_64 rng(seed); int limit = mode == "quick" ? 4 : mode == "full" ? 7 : 8;
    int arrays = 0;
    for (int n = 0; n <= limit; ++n) {
        int total = 1; for (int i = 0; i < n; ++i) { total *= 3; }
        for (int code = 0; code < total; ++code) {
            int rest = code; vector<int> a(n), w(n);
            for (int i = 0; i < n; ++i) { a[i] = rest % 3 - 1; rest /= 3; w[i] = int(rng() % 9) - 4; }
            sums(a, true); subsequences(a, w); frequency(a);
            for (int &x : a) { ++x; } windows(a); ++arrays; } }
    cout << "PASS exhaustive arrays=" << arrays << " ternary length<=" << limit << " sums/LIS/weighted/count/inversion/frequency/windows\n";
    int rounds = mode == "quick" ? 40 : mode == "full" ? 350 : 1800;
    for (int rep = 0; rep < rounds; ++rep) {
        int n = int(rng() % 11); vector<int> a(n), w(n);
        for (int i = 0; i < n; ++i) { a[i] = int(rng() % 21) - 10; w[i] = int(rng() % 31) - 15; }
        sums(a, true); subsequences(a, w); subsequences(a, w, std::greater<int>{}); frequency(a);
        int r = int(rng() % 6), c = int(rng() % 7); vector<vector<int>> matrix(r, vector<int>(c));
        for (auto &row : matrix) { for (int &x : row) { x = int(rng() % 21) - 10; } } rectangle(matrix); }
    for (int r = 0; r <= 3; ++r) { for (int c = 0; c <= 3; ++c) {
        int cells = r * c;
        for (int mask = 0; mask < (1 << cells); ++mask) {
            vector<vector<int>> a(r, vector<int>(c));
            for (int i = 0; i < r; ++i) { for (int j = 0; j < c; ++j) { a[i][j] = ((mask >> (i * c + j)) & 1) ? 1 : -1; } } rectangle(a); } } }
    cout << "PASS random rounds=" << rounds << " and exhaustive <=3x3 signed rectangles, both orientations\n";
    extremesAndGeneric(mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000);
    cout << "PASS full-width sums, generic equivalence, wide/modular counts, shared engines and large adversarial seed=" << seed << '\n';
}
