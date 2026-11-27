#include "../../06-Miscellaneous/012-enumeration.hpp"

ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=true actual=false\n"; std::exit(1);}}
string show(int x) { return std::to_string(x); }
template<class U> string show(U x) {
    string text;
    do { text += "0123456789abcdef"[int(x & 15)]; x >>= 4; } while (x);
    reverse(text.begin(), text.end()); return "0x" + text;}
template<class A, class B> string show(const pair<A, B> &p) { return '(' + show(p.first) + ',' + show(p.second) + ')'; }
template<class T> string show(const vector<T> &a) {
    string text = "[";
    for (const auto &x : a) { text += show(x) + ','; }
    return text + ']';}
template<class T> void checkEqual(const T &actual, const T &expected, const string &what) {
    ++checks;
    if (actual != expected) {
        cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context
             << " operation=" << what << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1);}}

// Each recursive leaf independently specifies a mathematical object; production
// steppers, low-mask helpers and the binary reflected-Gray formula are not oracles.
vector<vector<int>> combinationOracle(int n, int k, bool replacement = false) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int first) -> void {
        if (int(state.size()) == k) { out.push_back(state); return; }
        for (int i = first; i < n; ++i) {
            state.push_back(i); visit(visit, i + !replacement); state.pop_back();}};
    visit(visit, 0); return out;}
vector<vector<int>> subsetOracle(int n) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int i) -> void {
        if (i < 0) { out.push_back(state); return; }
        visit(visit, i - 1); state.push_back(i); visit(visit, i - 1); state.pop_back();};
    visit(visit, n - 1); return out;}
vector<vector<int>> productOracle(const vector<int> &radices, int first = 0, bool gray = false) {
    if (first == int(radices.size())) { return {{}}; }
    auto suffixes = productOracle(radices, first + 1, gray); vector<vector<int>> out;
    for (int x = 0; x < radices[first]; ++x) {
        for (const auto &suffix : suffixes) {
            vector<int> state{x}; state.insert(state.end(), suffix.begin(), suffix.end()); out.push_back(state);}
        if (gray) { reverse(suffixes.begin(), suffixes.end()); }}
    return out;}

template<class T, class Generate>
void sequenceCheck(const vector<T> &expected, Generate generate, bool thorough = false) {
    ++cases;
    int total = int(expected.size()), seen = 0;
    check(generate([&](const T &state) {
        check(seen < total, "no surplus output at=" + std::to_string(seen));
        checkEqual(state, expected[seen], "complete output at=" + std::to_string(seen)); ++seen; return true;}), "true return after natural completion");
    check(seen == total, "complete callback count");
    vector<int> stops;
    if (total && thorough) { for (int i = 1; i <= total; ++i) { stops.push_back(i); } }
    else if (total) { stops = {1, (total + 1) / 2, total}; }
    for (int stop : stops) {
        seen = 0;
        check(!generate([&](const T &state) {
            check(seen < stop, "no callback after cancellation");
            checkEqual(state, expected[seen], "cancelled prefix at=" + std::to_string(seen));
            return ++seen < stop;}), "false return even when last item cancels");
        check(seen == stop, "exact cancellation callback count");}}
template<class Generate>
void grayVectorCheck(const vector<vector<int>> &expected, Generate generate, bool thorough) {
    sequenceCheck(expected, [&](auto visit) {
        int seen = 0; vector<int> previous;
        return generate([&](auto &state, int changed) {
            static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>);
            int want = -1;
            if (seen) {
                for (int i = 0; i < int(state.size()); ++i) {
                    if (state[i] != previous[i]) {
                        check(want < 0 && abs(state[i] - previous[i]) == 1, "exactly one unit Gray change"); want = i;}}
                check(want >= 0, "Gray output differs from predecessor");}
            check(changed == want, "reported Gray changed dimension");
            previous = state; ++seen; return visit(state);});}, thorough);}
// Revolving door, Knuth TAOCP 7.2.1.3 (25)/(29): Gamma(m,k) = Gamma(m-1,k), then reversed Gamma(m-1,k-1) with m-1 appended.
vector<vector<vector<vector<int>>>> revolvingDoorOracle(int n, int max_k) {
    vector<vector<vector<vector<int>>>> gamma(n + 1);
    for (int m = 0; m <= n; ++m) {
        gamma[m].resize(min(m, max_k) + 1);
        for (int k = 0; k <= min(m, max_k); ++k) {
            auto &out = gamma[m][k];
            if (k == 0 || k == m) {
                out.emplace_back(k); iota(out[0].begin(), out[0].end(), 0);
                continue;}
            out = gamma[m - 1][k];
            for (int i = int(gamma[m - 1][k - 1].size()) - 1; i >= 0; --i) {
                out.push_back(gamma[m - 1][k - 1][i]); out.back().push_back(m - 1);}}}
    return gamma;}
// Gamma(n, n - c) for c <= max_c from the same recurrence, one row of m at a time; Gamma(m, m - c) needs only row m - 1.
vector<vector<vector<int>>> revolvingDoorTopOracle(int n, int max_c) {
    vector<vector<vector<int>>> row(1, vector<vector<int>>(1));
    for (int m = 1; m <= n; ++m) {
        vector<vector<vector<int>>> next(min(m, max_c) + 1);
        for (int c = 0; c <= min(m, max_c); ++c) {
            int k = m - c;
            auto &out = next[c];
            if (k == 0 || k == m) {
                out.emplace_back(k); iota(out[0].begin(), out[0].end(), 0);
                continue;}
            out = row[c - 1];
            for (int i = int(row[c].size()) - 1; i >= 0; --i) {
                out.push_back(row[c][i]); out.back().push_back(m - 1);}}
        row = std::move(next);}
    return row;}
// Wraps a vector visitor; checks (-1,-1) first, then that exactly removed left and added entered against the previous output.
template<class Visit> auto revolvingDoorSteps(Visit visit) {
    return [visit, seen = 0, previous = vector<int>()](auto &state, int removed, int added) mutable {
        static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>);
        bool go = visit(state);
        if (!seen) { check(removed == -1 && added == -1, "first revolving-door change is (-1,-1)"); }
        else {
            vector<int> gone, fresh;
            std::set_difference(previous.begin(), previous.end(), state.begin(), state.end(), std::back_inserter(gone));
            std::set_difference(state.begin(), state.end(), previous.begin(), previous.end(), std::back_inserter(fresh));
            checkEqual(gone, vector<int>{removed}, "revolving-door removed element");
            checkEqual(fresh, vector<int>{added}, "revolving-door added element");}
        previous = state; ++seen; return go;};}

void vectorCases(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 10 : 12;
    for (int n = 0; n <= bound; ++n) {
        for (int k = 0; k <= n; ++k) {
            context = "combination n=" + std::to_string(n) + " k=" + std::to_string(k);
            sequenceCheck(combinationOracle(n, k), [&](auto visit) {
                return forEachCombination(n, k, [&](auto &state) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 4);}
        context = "empty combination n=" + std::to_string(n);
        for (int k : {n + 1, INT_MAX}) {
            sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachCombination(n, k, visit); });}
        context = "subset n=" + std::to_string(n);
        sequenceCheck(subsetOracle(n), [&](auto visit) {
            return forEachSubset(n, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 4);}
    cout << "PASS vector combinations/subsets independent recursive oracle n=0.." << bound << '\n';
    int multi_bound = mode == "quick" ? 4 : mode == "full" ? 6 : 8;
    for (int n = 0; n <= multi_bound; ++n) { for (int k = 0; k <= multi_bound; ++k) {
        context = "multicombination n=" + std::to_string(n) + " k=" + std::to_string(k);
        sequenceCheck(combinationOracle(n, k, true), [&](auto visit) {
            return forEachMulticombination(n, k, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 3 && k <= 3);}}
    context = "empty multicombination n=0 k=INT_MAX";
    sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachMulticombination(0, INT_MAX, visit); });
    cout << "PASS multicombinations independent recursive oracle n,k=0.." << multi_bound << '\n';
    int door_bound = mode == "quick" ? 10 : 14;
    auto gamma = revolvingDoorOracle(door_bound, door_bound);
    const vector<vector<int>> none;
    for (int n = 0; n <= door_bound; ++n) {
        for (int k = 0; k <= n + 1; ++k) {
            context = "revolving door n=" + std::to_string(n) + " k=" + std::to_string(k);
            const auto &expected = k <= n ? gamma[n][k] : none;
            if (k <= n) {
                auto sorted = expected; sort(sorted.begin(), sorted.end());
                checkEqual(sorted, combinationOracle(n, k), "revolving-door oracle holds each combination once");}
            sequenceCheck(expected, [&](auto visit) { return forEachGrayCombination(n, k, revolvingDoorSteps(visit)); }, n <= 4);}}
    context = "revolving door n=0 k=INT_MAX";
    sequenceCheck(none, [&](auto visit) { return forEachGrayCombination(0, INT_MAX, revolvingDoorSteps(visit)); });
    for (int k = 0; k <= 3; ++k) {
        // Gamma(n,k) opens with Gamma(k+4,k) for every n >= k+4.
        const auto &prefix = gamma[k + 4][k];
        int seen = 0, total = int(prefix.size());
        context = "revolving door INT_MAX prefix k=" + std::to_string(k);
        check(!forEachGrayCombination(INT_MAX, k, revolvingDoorSteps([&](const vector<int> &state) {
            checkEqual(state, prefix[seen], "INT_MAX revolving-door prefix"); return ++seen < total;})), "INT_MAX revolving-door cancellation");
        check(seen == total, "INT_MAX revolving-door callback count");}
    auto wide = revolvingDoorOracle(100, 2);
    context = "revolving door n=100 k=2";
    checkEqual(lng(wide[100][2].size()), lng(4950), "C(100,2) revolving-door count");
    sequenceCheck(wide[100][2], [&](auto visit) { return forEachGrayCombination(100, 2, revolvingDoorSteps(visit)); });
    context = "revolving door top oracle";
    auto small_top = revolvingDoorTopOracle(door_bound, 2);
    for (int c = 0; c <= 2; ++c) { checkEqual(small_top[c], gamma[door_bound][door_bound - c], "top-row oracle agrees with the full table"); }
    // k > 3(n - k + 1) runs the loopless branch.
    auto top = revolvingDoorTopOracle(100, 2);
    for (int c = 0; c <= 2; ++c) {
        context = "revolving door n=100 k=" + std::to_string(100 - c);
        sequenceCheck(top[c], [&](auto visit) { return forEachGrayCombination(100, 100 - c, revolvingDoorSteps(visit)); });}
    context = "revolving door n=100 k=98 cancellation";
    int stopped_at = 0;
    check(!forEachGrayCombination(100, 98, [&](const vector<int> &, int, int) { return ++stopped_at < 1000; }), "loopless revolving-door cancellation");
    checkEqual(stopped_at, 1000, "loopless revolving-door callback count");
    cout << "PASS revolving-door combinations Knuth recurrence n=0.." << door_bound << " k=0..n+1, n=100 k=2,98..100, INT_MAX prefixes k=0..3\n";
    int max_dims = mode == "quick" ? 3 : mode == "full" ? 5 : 6, inputs = 0;
    auto check_radices = [&](const vector<int> &radices) {
        auto saved = radices;
        context = "product radices=" + show(radices);
        sequenceCheck(productOracle(radices), [&](auto visit) {
            return forEachProduct(radices, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, radices.size() <= 2);
        context = "Gray product radices=" + show(radices);
        grayVectorCheck(productOracle(radices, 0, true), [&](auto visit) {
            return forEachGrayProduct(radices, visit);}, radices.size() <= 2);
        checkEqual(radices, saved, "input radices remain unchanged"); ++inputs;};
    for (int n = 0; n <= max_dims; ++n) {
        vector<int> radices(n);
        auto visit = [&](auto &&visit, int i) -> void {
            if (i == n) { check_radices(radices); return; }
            for (int x = 0; x <= 3; ++x) { radices[i] = x; visit(visit, i + 1); }};
        visit(visit, 0);}
    std::mt19937_64 gen(test_seed);
    int rounds = mode == "quick" ? 100 : mode == "full" ? 1500 : 10000;
    for (int round = 0; round < rounds; ++round) {
        vector<int> radices(int(gen() % 9)); int count = 1;
        for (int &r : radices) { r = int(gen() % 5) + (round % 2); if (count * r > 4096) { r = 1; } count *= r; }
        check_radices(radices);}
    cout << "PASS products/reflected mixed-radix Gray exhaustive dims=0.." << max_dims
         << " radices=0..3 plus random=" << rounds << " total_inputs=" << inputs << '\n';}

template<class U> vector<U> maskOracle(int n, int k = -1) {
    vector<U> out;
    auto visit = [&](auto &&visit, int bit, U word, int count) -> void {
        if (bit < 0) { if (k < 0 || count == k) { out.push_back(word); } return; }
        visit(visit, bit - 1, word, count); visit(visit, bit - 1, U(word | (U(1) << bit)), count + 1);};
    visit(visit, n - 1, 0, 0); return out;}
// Cool-lex (Ruskey-Williams 2009): from 1^k 0^(n-k), rotate right by one the shortest prefix of length >= 3 ending in 0,1,x,
// else the whole string, the first min(C(n,k), limit) strings; string position i+1 is vector index i and mask bit i.
vector<vector<char>> coolLexStrings(int n, int k, lng limit = lng(1) << 60) {
    vector<vector<char>> out;
    if (k > n) { return out; }
    ulll total = 1;
    for (int i = 0; i < min(k, n - k) && total <= ulll(limit); ++i) { total = total * ulll(n - i) / ulll(i + 1); }
    vector<char> b(n);
    fill(b.begin(), b.begin() + k, char(1));
    for (ulll t = 0; t < min(total, ulll(limit)); ++t) {
        out.push_back(b);
        int len = 3;
        while (len <= n && !(b[len - 3] == 0 && b[len - 2] == 1)) { ++len; }
        len = min(len, n);
        if (len > 1) { std::rotate(b.begin(), b.begin() + len - 1, b.begin() + len); }}
    return out;}
template<class U> vector<U> coolLexOracle(int n, int k, lng limit = lng(1) << 60) {
    vector<U> out;
    for (const auto &b : coolLexStrings(n, k, limit)) {
        U x = 0;
        for (int i = 0; i < n; ++i) { if (b[i]) { x = U(x | U(U(1) << i)); } }
        out.push_back(x);}
    return out;}
template<class U> vector<U> grayOracle(int n) {
    vector<U> out{0};
    for (int bit = 0; bit < n; ++bit) {
        int count = int(out.size());
        for (int i = count - 1; i >= 0; --i) { out.push_back(U(out[i] | (U(1) << bit))); }}
    return out;}
template<class U, class Generate>
void grayMaskCheck(const vector<U> &expected, Generate generate) {
    sequenceCheck(expected, [&](auto visit) {
        int seen = 0; U previous = 0;
        return generate([&](U state, int changed) {
            int want = -1;
            if (seen) {
                for (int i = 0; i < int(sizeof(U) * CHAR_BIT); ++i) {
                    if (((state >> i) & 1) != ((previous >> i) & 1)) {
                        check(want < 0, "one Gray-mask bit changes"); want = i;}}
                check(want >= 0, "Gray mask differs from predecessor");}
            check(changed == want, "reported Gray-mask bit"); previous = state; ++seen; return visit(state);});}, expected.size() <= 16);}
template<class U> void submaskCheck(U mask) {
    vector<int> bits;
    for (int i = int(sizeof(U) * CHAR_BIT) - 1; i >= 0; --i) {
        if ((mask >> i) & 1) { bits.push_back(i); }}
    vector<U> expected;
    auto visit = [&](auto &&visit, int i, U state) -> void {
        if (i == int(bits.size())) { expected.push_back(state); return; }
        visit(visit, i + 1, U(state | (U(1) << bits[i]))); visit(visit, i + 1, state);};
    visit(visit, 0, 0);
    context = "submask width=" + std::to_string(sizeof(U) * CHAR_BIT) + " mask=" + show(mask);
    sequenceCheck(expected, [&](auto visitor) { return forEachSubmask(mask, visitor); }, bits.size() <= 3);}
// Supermasks: subsets of the free bits, OR mask, sorted; independent of BitOps stepping.
template<class U> void supermaskCheck(U mask, int n) {
    U full = n == int(sizeof(U) * CHAR_BIT) ? U(~U(0)) : U((U(1) << n) - 1);
    vector<int> free_bits;
    for (int i = 0; i < n; ++i) { if (!((mask >> i) & 1)) { free_bits.push_back(i); } }
    vector<U> expected;
    auto visit = [&](auto &&visit, int i, U state) -> void {
        if (i == int(free_bits.size())) { expected.push_back(state); return; }
        visit(visit, i + 1, state); visit(visit, i + 1, U(state | (U(1) << free_bits[i])));};
    visit(visit, 0, mask); sort(expected.begin(), expected.end());
    for (U x : expected) { check((x & mask) == mask && (x & U(~full)) == 0, "supermask oracle domain"); }
    context = "supermask width=" + std::to_string(sizeof(U) * CHAR_BIT) + " n=" + std::to_string(n) + " mask=" + show(mask);
    sequenceCheck(expected, [&](auto visitor) { return forEachSupermask(mask, n, visitor); }, free_bits.size() <= 3);}
// Set bits: a loop over every bit position.
template<class U> void setBitCheck(U x) {
    int width = int(sizeof(U) * CHAR_BIT);
    vector<int> expected;
    for (int i = 0; i < width; ++i) { if ((x >> i) & 1) { expected.push_back(i); } }
    context = "set bits width=" + std::to_string(width) + " x=" + show(x);
    sequenceCheck(expected, [&](auto visit) {
        return forEachSetBit(x, [&](auto i) { static_assert(std::is_same_v<decltype(i), int>); return visit(i); });}, expected.size() <= 3);}
// k-bit submasks: recursive choice of k set bits, then sorted; independent of the carry stepping.
template<class U> vector<U> chooseSetBits(U mask, int k) {
    vector<int> bits;
    for (int i = 0; i < int(sizeof(U) * CHAR_BIT); ++i) { if ((mask >> i) & 1) { bits.push_back(i); } }
    vector<U> out;
    auto visit = [&](auto &&visit, int i, int left, U state) -> void {
        if (!left) { out.push_back(state); return; }
        if (int(bits.size()) - i < left) { return; }
        visit(visit, i + 1, left - 1, U(state | (U(1) << bits[i]))); visit(visit, i + 1, left, state);};
    visit(visit, 0, k, 0); sort(out.begin(), out.end()); return out;}
template<class U> void combinationSubmaskCheck(U mask, int k, const vector<U> &expected) {
    context = "combination submask width=" + std::to_string(sizeof(U) * CHAR_BIT) + " mask=" + show(mask) + " k=" + std::to_string(k);
    sequenceCheck(expected, [&](auto visit) {
        return forEachCombinationSubmask(mask, k, [&](auto sub) { static_assert(std::is_same_v<decltype(sub), U>); return visit(sub); });}, expected.size() <= 3);}
// Submask pairs: masks ascending, every value below the mask descending, filtered to submasks.
template<class U> vector<pair<U, U>> submaskPairOracle(int n) {
    vector<pair<U, U>> out;
    for (lng mask = 0; mask < (lng(1) << n); ++mask) {
        for (lng sub = mask; sub >= 0; --sub) { if (!(sub & ~mask)) { out.emplace_back(U(mask), U(sub)); } }}
    return out;}
template<class U> void wordCases(const string &mode) {
    int width = int(sizeof(U) * CHAR_BIT), bound = min(width, mode == "quick" ? 6 : mode == "full" ? 10 : 12);
    for (int n = 0; n <= bound; ++n) {
        context = "subset-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
        sequenceCheck(maskOracle<U>(n), [&](auto visit) { return forEachSubsetMask<U>(n, visit); }, n <= 4);
        context = "Gray-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
        grayMaskCheck(grayOracle<U>(n), [&](auto visit) { return forEachGrayMask<U>(n, visit); });
        context = "submask pair width=" + std::to_string(width) + " n=" + std::to_string(n);
        sequenceCheck(submaskPairOracle<U>(n), [&](auto visit) {
            return forEachSubmaskPair<U>(n, [&](auto mask, auto sub) {
                static_assert(std::is_same_v<decltype(mask), U> && std::is_same_v<decltype(sub), U>); return visit(pair{mask, sub});});}, n <= 2);
        for (int k = 0; k <= n; ++k) {
            context = "combination-mask width=" + std::to_string(width) + " n=" + std::to_string(n) + " k=" + std::to_string(k);
            sequenceCheck(maskOracle<U>(n, k), [&](auto visit) { return forEachCombinationMask<U>(n, k, visit); }, n <= 4);}
        for (int k : {n + 1, INT_MAX}) {
            context = "empty combination-mask width=" + std::to_string(width) + " n=" + std::to_string(n);
            sequenceCheck(vector<U>{}, [&](auto visit) { return forEachCombinationMask<U>(n, k, visit); });}}
    for (int x = 0; x < (1 << bound); ++x) { submaskCheck(U(x)); }
    for (int n = 0; n <= min(bound, 8); ++n) { for (int x = 0; x < (1 << n); ++x) { supermaskCheck(U(x), n); } }
    for (int bit = 0; bit < width; ++bit) {
        submaskCheck(U(U(1) << bit)); submaskCheck(U((U(1) << bit) | 1));}
    context = "full-width boundaries width=" + std::to_string(width);
    U all = U(~U(0));
    sequenceCheck(vector<U>{all}, [&](auto visit) { return forEachCombinationMask<U>(width, width, visit); });
    sequenceCheck(vector<U>{0}, [&](auto visit) { return forEachCombinationMask<U>(width, 0, visit); });
    vector<U> singles, missing;
    for (int bit = 0; bit < width; ++bit) { singles.push_back(U(U(1) << bit)); }
    for (int bit = width - 1; bit >= 0; --bit) { missing.push_back(U(all ^ (U(1) << bit))); }
    sequenceCheck(singles, [&](auto visit) { return forEachCombinationMask<U>(width, 1, visit); });
    sequenceCheck(missing, [&](auto visit) { return forEachCombinationMask<U>(width, width - 1, visit); });
    int seen = 0;
    check(!forEachSubsetMask<U>(width, [&](U state) { checkEqual(state, U(seen), "full-width subset prefix"); return ++seen < 10; }), "cancel full-width subsets");
    check(seen == 10, "full-width subset cancellation count"); seen = 0;
    check(!forEachSubmask(all, [&](U state) { checkEqual(state, U(all - seen), "full-width submask prefix"); return ++seen < 10; }), "cancel full-width submasks");
    check(seen == 10, "full-width submask cancellation count"); seen = 0;
    auto gray = grayOracle<U>(4);
    check(!forEachGrayMask<U>(width, [&](U state, int changed) {
        checkEqual(state, gray[seen], "full-width Gray prefix");
        if (!seen) { check(changed == -1, "full-width Gray initial change"); }
        return ++seen < 10;}), "cancel full-width Gray masks");
    check(seen == 10, "full-width Gray cancellation count"); seen = 0;
    auto pairs = submaskPairOracle<U>(4);
    check(!forEachSubmaskPair<U>(width, [&](U mask, U sub) {
        checkEqual(pair{mask, sub}, pairs[seen], "full-width submask-pair prefix"); return ++seen < 10;}), "cancel full-width submask pairs");
    check(seen == 10, "full-width submask-pair cancellation count");
    // Every mask below 2^bits with every k: filter all values below 2^bits, bucketed by popcount in ascending order.
    int bits = min(width, mode == "quick" ? 8 : mode == "full" ? 10 : 12);
    for (int x = 0; x < (1 << bits); ++x) {
        int p = std::popcount(uint(x));
        vector<vector<U>> by_count(p + 2);
        for (int y = 0; y < (1 << bits); ++y) { if (!(y & ~x)) { by_count[std::popcount(uint(y))].push_back(U(y)); } }
        setBitCheck(U(x));
        for (int k = 0; k <= p + 1; ++k) { combinationSubmaskCheck(U(x), k, by_count[k]); }
        combinationSubmaskCheck(U(x), INT_MAX, vector<U>{});}
    context = "full-width set bits and combination submasks width=" + std::to_string(width);
    setBitCheck(all); setBitCheck(U(U(1) << (width - 1))); setBitCheck(U(all ^ 1));
    for (int k : {0, 1, 2, width - 1, width, width + 1}) { combinationSubmaskCheck(all, k, chooseSetBits(all, k)); }
    std::mt19937_64 gen(test_seed + width);
    int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 3000;
    for (int round = 0; round < rounds; ++round) {
        U mask = 0; int count = int(gen() % 11);
        for (int i = 0; i < count; ++i) { mask |= U(U(1) << (gen() % width)); }
        submaskCheck(mask);
        U dense = U(~U(0));
        for (int i = 0; i < count; ++i) { dense &= U(~(U(1) << (gen() % width))); }
        int n = width - int(gen() % 3);
        if (n < width) { dense &= U((U(1) << n) - 1); }
        supermaskCheck(dense, n);
        U word = U(gen());
        if (width > 64) { word = U((ulll(word) << 64) | gen()); }
        setBitCheck(word); setBitCheck(mask); setBitCheck(dense);
        // Sparse masks of up to 20 bits: every k up to 10 bits, the k near 0 and near popcount above that.
        U sparse = 0; int p = int(gen() % (min(width, 20) + 1));
        while (std::popcount(sparse) < p) { sparse |= U(U(1) << (gen() % width)); }
        for (int k = 0; k <= p + 1; ++k) {
            if (p <= 10 || k <= 2 || k >= p - 2) { combinationSubmaskCheck(sparse, k, chooseSetBits(sparse, k)); }}}
    cout << "PASS words width=" << width << " exhaustive n=0.." << bound << " submasks=" << (1 << bound)
         << " submask pairs n=0.." << bound << " set bits/k-submasks every mask below 2^" << bits
         << " random_sparse=" << rounds << " full-width edges\n";
    // Cool-lex: the rotation oracle, itself checked against the ascending k-bit masks below 2^n.
    int cool = min(width, mode == "quick" ? 10 : mode == "full" ? 14 : 16);
    for (int n = 0; n <= cool; ++n) {
        for (int k = 0; k <= n + 1; ++k) {
            context = "cool-lex width=" + std::to_string(width) + " n=" + std::to_string(n) + " k=" + std::to_string(k);
            auto expected = coolLexOracle<U>(n, k), sorted = expected;
            sort(sorted.begin(), sorted.end());
            checkEqual(sorted, maskOracle<U>(n, k), "cool-lex oracle holds each k-bit mask below 2^n once");
            sequenceCheck(expected, [&](auto visit) {
                return forEachCombinationMaskCoolLex<U>(n, k, [&](auto x) { static_assert(std::is_same_v<decltype(x), U>); return visit(x); });}, n <= 4);}
        context = "empty cool-lex width=" + std::to_string(width) + " n=" + std::to_string(n) + " k=INT_MAX";
        sequenceCheck(vector<U>{}, [&](auto visit) { return forEachCombinationMaskCoolLex<U>(n, INT_MAX, visit); });}
    // Full width: the whole order up to 5000 (quick) or 50000 outputs, else a prefix and, up to 10^6 outside quick, every output's count, popcount and distinctness.
    int prefix = mode == "quick" ? 2000 : 20000;
    lng enumerated = mode == "quick" ? 0 : 1000000;
    for (int k : {0, 1, 2, 3, 4, width - 4, width - 3, width - 2, width - 1, width, width + 1}) {
        context = "full-width cool-lex width=" + std::to_string(width) + " k=" + std::to_string(k);
        int r = min(k, width - k);
        lng count = r < 0 ? 0 : 1;
        for (int i = 0; i < r; ++i) { count = count * (width - i) / (i + 1); }
        auto expected = coolLexOracle<U>(width, k, prefix);
        U last = U(U(1) << (width - 1));
        if (k > 1) { last = U(last | U(all >> (width - k + 1))); }
        if (1 <= k && k <= width) { checkEqual(expected.front(), U(all >> (width - k)), "cool-lex first mask is the low k bits"); }
        if (count <= (mode == "quick" ? 5000 : 50000)) {
            expected = coolLexOracle<U>(width, k);
            checkEqual(lng(expected.size()), count, "C(width,k) cool-lex count");
            auto sorted = expected;
            sort(sorted.begin(), sorted.end());
            // Above W/2 the complements of the (W-k)-subsets, a smaller recursion.
            bool flip = k <= width && width - k < k;
            auto want = chooseSetBits(all, flip ? width - k : k);
            if (flip) {
                for (U &x : want) { x = U(all ^ x); }
                sort(want.begin(), want.end());}
            checkEqual(sorted, want, "full-width cool-lex oracle holds each k-bit mask once");
            if (1 <= k && k <= width) { checkEqual(expected.back(), last, "cool-lex last mask is the low k-1 bits and bit n-1"); }
            sequenceCheck(expected, [&](auto visit) { return forEachCombinationMaskCoolLex<U>(width, k, visit); });
            continue;}
        int seen = 0, stop = int(expected.size());
        check(!forEachCombinationMaskCoolLex<U>(width, k, [&](U x) {
            checkEqual(x, expected[seen], "full-width cool-lex prefix at=" + std::to_string(seen)); return ++seen < stop;}), "full-width cool-lex prefix cancellation");
        check(seen == stop, "full-width cool-lex prefix callback count");
        if (count > enumerated) { continue; }
        vector<U> masks;
        check(forEachCombinationMaskCoolLex<U>(width, k, [&](U x) { masks.push_back(x); return true; }), "full-width cool-lex completion");
        checkEqual(lng(masks.size()), count, "C(width,k) cool-lex count");
        checkEqual(masks.back(), last, "cool-lex last mask is the low k-1 bits and bit n-1");
        for (U x : masks) {
            int bits_set = 0;
            for (U y = x; y; y = U(y & U(y - 1))) { ++bits_set; }
            check(bits_set == k, "full-width cool-lex popcount k");}
        sort(masks.begin(), masks.end());
        check(std::adjacent_find(masks.begin(), masks.end()) == masks.end(), "full-width cool-lex masks distinct");}
    cout << "PASS cool-lex width=" << width << " rotation oracle n=0.." << cool << " k=0..n+1, full width k=0..4,W-4..W+1\n";}

// Partitions: recursion over the first part, largest first, gives reverse lexicographic order.
vector<vector<int>> partitionOracle(int n, int max_part) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int left, int cap) -> void {
        if (!left) { out.push_back(state); return; }
        for (int x = min(left, cap); x >= 1; --x) { state.push_back(x); visit(visit, left - x, x); state.pop_back(); }};
    visit(visit, n, max_part); return out;}
// Set partitions: assign each element to an existing block or a new one, then label blocks by first element.
vector<vector<int>> setPartitionOracle(int n) {
    vector<vector<int>> out; vector<vector<int>> blocks; vector<int> owner(n);
    auto visit = [&](auto &&visit, int i) -> void {
        if (i == n) { out.push_back(owner); return; }
        for (int b = 0; b <= int(blocks.size()); ++b) {
            if (b == int(blocks.size())) { blocks.emplace_back(); }
            blocks[b].push_back(i); owner[i] = b; visit(visit, i + 1); blocks[b].pop_back();
            if (blocks[b].empty()) { blocks.pop_back(); }}};
    visit(visit, 0); sort(out.begin(), out.end()); return out;}
// Pairings: the smallest free element takes each free partner ascending, then (odd free count) stays unpaired.
vector<vector<int>> pairingOracle(int n) {
    vector<vector<int>> out; vector<int> mate(n, -2);
    auto visit = [&](auto &&visit, int u, int free) -> void {
        while (u < n && mate[u] != -2) { ++u; }
        if (u == n) { out.push_back(mate); return; }
        for (int v = u + 1; v < n; ++v) {
            if (mate[v] == -2) { mate[u] = v; mate[v] = u; visit(visit, u + 1, free - 2); mate[v] = -2; }}
        if (free & 1) { mate[u] = -1; visit(visit, u + 1, free - 1); }
        mate[u] = -2;};
    visit(visit, 0, n); return out;}
// Labelled trees: every connected (n-1)-edge subset of K_n, its Pruefer code and (leaf, neighbour) edges by naive
// smallest-leaf removal, sorted by code.
vector<pair<vector<int>, vector<pair<int, int>>>> treeOracle(int n) {
    vector<pair<int, int>> all;
    for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { all.emplace_back(u, v); } }
    vector<pair<vector<int>, vector<pair<int, int>>>> out; vector<int> chosen;
    auto removal = [&](const vector<vector<int>> &adj) {
        vector<bool> alive(n, true); vector<int> code; vector<pair<int, int>> edges;
        auto neighbours = [&](int v) {
            vector<int> res;
            for (int w : adj[v]) { if (alive[w]) { res.push_back(w); } }
            return res;};
        for (int step = 0; step < n - 2; ++step) {
            int v = 0;
            while (!alive[v] || neighbours(v).size() != 1) { ++v; }
            int p = neighbours(v)[0];
            code.push_back(p); edges.emplace_back(v, p); alive[v] = false;}
        if (n > 1) {
            int a = 0;
            while (!alive[a]) { ++a; }
            check(alive[n - 1] && a < n - 1, "naive removal leaves n-1 last");
            edges.emplace_back(a, n - 1);}
        out.emplace_back(code, edges);};
    auto visit = [&](auto &&visit, int i) -> void {
        if (int(chosen.size()) == n - 1) {
            vector<vector<int>> adj(n);
            for (int e : chosen) { adj[all[e].first].push_back(all[e].second); adj[all[e].second].push_back(all[e].first); }
            vector<bool> seen(n); vector<int> stack{0}; seen[0] = true; int reached = 1;
            while (!stack.empty()) {
                int v = stack.back(); stack.pop_back();
                for (int w : adj[v]) { if (!seen[w]) { seen[w] = true; ++reached; stack.push_back(w); } }}
            if (reached == n) { removal(adj); }
            return;}
        if (int(all.size()) - i < n - 1 - int(chosen.size())) { return; }
        chosen.push_back(i); visit(visit, i + 1); chosen.pop_back(); visit(visit, i + 1);};
    visit(visit, 0); sort(out.begin(), out.end()); return out;}
// Pruefer decoding with an ordered set of leaves, a second independent decoder for codes too large for the brute force.
vector<pair<int, int>> prueferDecode(int n, const vector<int> &code) {
    vector<int> degree(n, 1);
    for (int v : code) { ++degree[v]; }
    set<int> leaves;
    for (int v = 0; v < n; ++v) { if (degree[v] == 1) { leaves.insert(v); } }
    vector<pair<int, int>> edges;
    for (int v : code) {
        edges.emplace_back(*leaves.begin(), v); leaves.erase(leaves.begin());
        if (--degree[v] == 1) { leaves.insert(v); }}
    if (n > 1) { edges.emplace_back(*leaves.begin(), *prev(leaves.end())); }
    return edges;}
// Pascal triangle with saturation at 2^64: independent of the header's multiplicative binomial.
vector<vector<ulll>> pascal(int n) {
    ulll cap = ulll(1) << 64; vector<vector<ulll>> c(n + 1);
    for (int i = 0; i <= n; ++i) {
        c[i].assign(i + 1, 1);
        for (int j = 1; j < i; ++j) { c[i][j] = min(cap, c[i - 1][j - 1] + c[i - 1][j]); }}
    return c;}
void objectCases(const string &mode) {
    int bound = mode == "quick" ? 12 : mode == "full" ? 22 : 28, total = 0;
    for (int n = 0; n <= bound; ++n) {
        for (int max_part : {n ? 1 : 0, 2, 3, n / 2 + 1, n, n + 1, INT_MAX}) {
            if (n && max_part < 1) { continue; }
            context = "integer partition n=" + std::to_string(n) + " max_part=" + std::to_string(max_part);
            auto expected = partitionOracle(n, max_part); total += int(expected.size());
            sequenceCheck(expected, [&](auto visit) {
                return forEachIntegerPartition(n, max_part, [&](auto &state) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 5);}
        context = "integer partition default n=" + std::to_string(n);
        sequenceCheck(partitionOracle(n, n), [&](auto visit) { return forEachIntegerPartition(n, visit); });}
    vector<lng> count(bound + 1); count[0] = 1;
    for (int part = 1; part <= bound; ++part) { for (int s = part; s <= bound; ++s) { count[s] += count[s - part]; } }
    for (int n = 0; n <= bound; ++n) {
        lng seen = 0; forEachIntegerPartition(n, [&](const vector<int> &) { ++seen; return true; });
        checkEqual(seen, count[n], "partition count against coin-change p(n) n=" + std::to_string(n));}
    int big = mode == "quick" ? 1000 : 1000000, seen = 0;
    context = "large integer partition prefix n=" + std::to_string(big);
    vector<vector<int>> prefix{{big}, {big - 1, 1}, {big - 2, 2}, {big - 2, 1, 1}};
    check(!forEachIntegerPartition(big, [&](const vector<int> &state) {
        checkEqual(state, prefix[seen], "large partition prefix"); return ++seen < 4;}), "large partition cancellation");
    cout << "PASS integer partitions recursive oracle n=0.." << bound << " outputs=" << total << " p(n) counts, large prefix\n";

    int set_bound = mode == "quick" ? 7 : mode == "full" ? 10 : 11;
    vector<lng> bell{1}; vector<lng> row{1};
    for (int n = 1; n <= set_bound; ++n) {
        vector<lng> next{row.back()};
        for (lng x : row) { next.push_back(next.back() + x); }
        row = next; bell.push_back(row.front());}
    // Stirling numbers of the second kind, S(n,k) = k S(n-1,k) + S(n-1,k-1), saturated at 2^64.
    vector<vector<ulll>> stirling(61, vector<ulll>(62));
    stirling[0][0] = 1;
    for (int n = 1; n <= 60; ++n) {
        for (int k = 1; k <= n; ++k) { stirling[n][k] = min(ulll(1) << 64, ulll(k) * stirling[n - 1][k] + stirling[n - 1][k - 1]); }}
    checkEqual(stirling[20][2], ulll((1 << 19) - 1), "Stirling S(n,2) = 2^(n-1) - 1");
    checkEqual(stirling[60][59], ulll(60 * 59 / 2), "Stirling S(n,n-1) = C(n,2)");
    for (int n = 0; n <= set_bound; ++n) {
        context = "set partition n=" + std::to_string(n);
        auto all = setPartitionOracle(n);
        checkEqual(lng(all.size()), bell[n], "Bell triangle count");
        sequenceCheck(all, [&](auto visit) {
            return forEachSetPartition(n, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 4);
        for (int k = 0; k <= n + 1; ++k) {
            context = "set partition n=" + std::to_string(n) + " k=" + std::to_string(k);
            vector<vector<int>> expected;
            for (const auto &a : all) { if (int(set<int>(a.begin(), a.end()).size()) == k) { expected.push_back(a); } }
            checkEqual(ulll(expected.size()), stirling[n][k], "Stirling triangle count");
            sequenceCheck(expected, [&](auto visit) {
                return forEachSetPartition(n, k, [&](auto &state) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 4);}
        context = "set partition n=" + std::to_string(n) + " k=INT_MAX";
        sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachSetPartition(n, INT_MAX, visit); });}
    // Beyond the brute force: each output is a restricted growth string with exactly k blocks, strictly increasing.
    for (auto [n, k] : vector<pair<int, int>>{{20, 0}, {20, 1}, {20, 2}, {20, 18}, {20, 19}, {20, 20}, {20, 21}, {60, 1}, {60, 59}, {60, 60}}) {
        context = "set partition count n=" + std::to_string(n) + " k=" + std::to_string(k);
        ulll seen = 0; vector<int> previous;
        check(forEachSetPartition(n, k, [&](const vector<int> &a) {
            bool ok = int(a.size()) == n && (previous.empty() || previous < a);
            int top = -1;
            for (int x : a) { ok = ok && 0 <= x && x <= top + 1; top = max(top, x); }
            check(ok && top == k - 1, "restricted growth string with k blocks after its predecessor");
            previous = a; ++seen; return true;}), "k-block set partition completion");
        checkEqual(seen, stirling[n][k], "Stirling count");}
    cout << "PASS set partitions block-assignment oracle, Bell and Stirling triangles n=0.." << set_bound
         << " k=0..n+1, counts n=20 k<=2 or k>=18, n=60 k=1,59,60\n";

    int pairing_bound = mode == "quick" ? 10 : mode == "full" ? 12 : 13;
    for (int n = 0; n <= pairing_bound; ++n) {
        context = "pairing n=" + std::to_string(n);
        auto expected = pairingOracle(n);
        lng count = 1;
        for (int x = 1; x <= n; x += 2) { count *= x; }
        checkEqual(lng(expected.size()), count, "double-factorial pairing count");
        vector<int> previous;
        for (const auto &mate : expected) {
            vector<int> key(mate);
            int unpaired = 0;
            for (int u = 0; u < n; ++u) {
                if (mate[u] < 0) { ++unpaired; key[u] = n; }
                else { check(mate[u] != u && mate[mate[u]] == u, "oracle mate is an involution"); }}
            check(unpaired == (n & 1), "oracle leaves n mod 2 elements unpaired");
            check(previous.empty() || previous < key, "oracle lexicographic with -1 as n");
            previous = key;}
        sequenceCheck(expected, [&](auto visit) {
            return forEachPairing(n, [&](auto &state) {
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 4);}
    cout << "PASS pairings smallest-free recursion, double factorials, lexicographic order n=0.." << pairing_bound << '\n';

    int tree_bound = mode == "stress" ? 7 : 6;
    for (int n = 1; n <= tree_bound; ++n) {
        context = "labeled tree n=" + std::to_string(n);
        auto trees = treeOracle(n);
        lng count = 1;
        for (int i = 0; i < n - 2; ++i) { count *= n; }
        checkEqual(lng(trees.size()), count, "Cayley count n^(n-2)");
        vector<vector<pair<int, int>>> expected;
        for (int i = 0; i < int(trees.size()); ++i) {
            check(!i || trees[i - 1].first < trees[i].first, "distinct Pruefer codes");
            checkEqual(prueferDecode(n, trees[i].first), trees[i].second, "set decoder agrees with naive removal");
            expected.push_back(trees[i].second);}
        sequenceCheck(expected, [&](auto visit) {
            return forEachLabeledTree(n, [&](auto &state) {
                static_assert(std::is_same_v<std::remove_cvref_t<decltype(state)>, vector<pair<int, int>>>);
                static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 3);}
    cout << "PASS labeled trees brute-force K_n spanning trees, naive Pruefer removal n=1.." << tree_bound << '\n';

    auto c = pascal(70);
    // Gamma(n,k) keeps Gamma(n-1,k) first, then reversed Gamma(n-1,k-1) with n-1 appended; a = the first k entries.
    auto gray_rank = [&](auto &&gray_rank, const vector<int> &a, int n, int k) -> ulll {
        if (!k || k == n) { return 0; }
        if (a[k - 1] < n - 1) { return gray_rank(gray_rank, a, n - 1, k); }
        return c[n - 1][k] + c[n - 1][k - 1] - 1 - gray_rank(gray_rank, a, n - 1, k - 1);};
    // Ruskey-Williams Theorem 2, one-based c_j = a[j-1]+1: C(c_r, r) - 1 + sum_{j>r} (C(c_j - 1, j) - 1), r the first j with c_j > j.
    auto cool_rank = [&](const vector<int> &a) -> ulll {
        int k = int(a.size()), r = 1;
        while (r <= k && a[r - 1] + 1 == r) { ++r; }
        if (r > k) { return 0; }
        ulll res = c[a[r - 1] + 1][r] - 1;
        for (int j = r + 1; j <= k; ++j) { res += c[a[j - 1]][j] - 1; }
        return res;};
    // Theorem 2 for k <= 2 at any n: x; (0,y) gives C(y+1,2) - 1, (x,y) with x > 0 gives x + C(y,2) - 1.
    auto cool_small = [](const vector<int> &a) -> ulll {
        if (a.empty()) { return 0; }
        if (a.size() == 1) { return ulll(a[0]); }
        ulll x = ulll(a[0]), y = ulll(a[1]);
        return x ? x + y * (y - 1) / 2 - 1 : y * (y + 1) / 2 - 1;};
    int rank_bound = mode == "quick" ? 9 : mode == "full" ? 13 : 15;
    auto gamma = revolvingDoorOracle(rank_bound, rank_bound);
    for (int n = 0; n <= rank_bound; ++n) {
        for (int k = 0; k <= n; ++k) {
            context = "combination rank n=" + std::to_string(n) + " k=" + std::to_string(k);
            auto expected = combinationOracle(n, k);
            checkEqual(ulll(expected.size()), c[n][k], "Pascal count");
            for (ulng r = 0; r < expected.size(); ++r) {
                checkEqual(combinationRank(n, expected[r]), r, "rank of oracle position");
                checkEqual(combinationUnrank(n, k, r), expected[r], "unrank to oracle position");}
            context = "Gray combination rank n=" + std::to_string(n) + " k=" + std::to_string(k);
            const auto &door = gamma[n][k];
            checkEqual(ulll(door.size()), c[n][k], "Gamma Pascal count");
            for (ulng r = 0; r < door.size(); ++r) {
                checkEqual(ulng(gray_rank(gray_rank, door[r], n, k)), r, "recurrence rank oracle on Gamma position");
                checkEqual(grayCombinationRank(n, door[r]), r, "Gray rank of Gamma position");
                checkEqual(grayCombinationUnrank(n, k, r), door[r], "Gray unrank to Gamma position");}
            context = "cool-lex combination rank n=" + std::to_string(n) + " k=" + std::to_string(k);
            auto cool = coolLexStrings(n, k);
            checkEqual(ulll(cool.size()), c[n][k], "cool-lex Pascal count");
            for (ulng r = 0; r < cool.size(); ++r) {
                vector<int> a;
                for (int i = 0; i < n; ++i) { if (cool[r][i]) { a.push_back(i); } }
                checkEqual(ulng(cool_rank(a)), r, "Theorem 2 rank oracle on cool-lex position");
                if (k <= 2) { checkEqual(ulng(cool_small(a)), r, "k<=2 closed form on cool-lex position"); }
                checkEqual(combinationRankCoolLex(n, a), r, "cool-lex rank of rotation-oracle position");}}}
    std::mt19937_64 gen(test_seed ^ 0x5eed);
    int rounds = mode == "quick" ? 2000 : mode == "full" ? 50000 : 300000;
    for (int round = 0; round < rounds; ++round) {
        int n = round % 3 ? 1 + int(gen() % 67) : int(gen() % INT_MAX) + 1, k;
        if (n <= 67) {
            k = int(gen() % (n + 1));
            if (c[n][k] >= ulll(1) << 64) { continue; }}
        else { k = int(gen() % 3); }
        ulll total_c = n <= 67 ? c[n][k] : k == 0 ? 1 : k == 1 ? ulll(n) : ulll(n) * (n - 1) / 2;
        ulng r = ulng(ulll(gen()) % total_c);
        context = "random rank n=" + std::to_string(n) + " k=" + std::to_string(k) + " r=" + std::to_string(r);
        auto a = combinationUnrank(n, k, r);
        check(int(a.size()) == k, "unrank size");
        for (int i = 0; i < k; ++i) { check(a[i] >= 0 && a[i] < n && (!i || a[i - 1] < a[i]), "unrank increasing in range"); }
        checkEqual(combinationRank(n, a), r, "rank(unrank(r)) round trip");
        if (n <= 67) {
            ulll direct = 0;
            for (int i = 0, v = 0; i < k; ++v) {
                if (v == a[i]) { ++i; }
                else { direct += c[n - 1 - v][k - 1 - i]; }}
            checkEqual(ulng(direct), r, "Pascal-sum rank oracle");}
        if (k == 1) { checkEqual(ulng(a[0]), r, "k=1 closed form"); }
        if (k == 2) {
            ulll x = ulll(a[0]), want = x * (2 * ulll(n) - x - 1) / 2 + ulll(a[1] - a[0] - 1);
            checkEqual(ulng(want), r, "k=2 closed form");}
        if (n <= 67 && r + 1 < total_c) {
            auto b = combinationUnrank(n, k, r + 1);
            check(a < b, "consecutive ranks increase lexicographically");}
        context = "random Gray rank n=" + std::to_string(n) + " k=" + std::to_string(k) + " r=" + std::to_string(r);
        auto g = grayCombinationUnrank(n, k, r);
        check(int(g.size()) == k, "Gray unrank size");
        for (int i = 0; i < k; ++i) { check(g[i] >= 0 && g[i] < n && (!i || g[i - 1] < g[i]), "Gray unrank increasing in range"); }
        checkEqual(grayCombinationRank(n, g), r, "Gray rank(unrank(r)) round trip");
        checkEqual(grayCombinationUnrank(n, k, grayCombinationRank(n, a)), a, "Gray unrank(rank(a)) round trip on a random combination");
        if (n <= 67) {
            checkEqual(ulng(gray_rank(gray_rank, g, n, k)), r, "Gamma recurrence rank oracle");
            checkEqual(grayCombinationRank(n, a), ulng(gray_rank(gray_rank, a, n, k)), "Gamma recurrence rank of a random combination");}
        if (k == 1) { checkEqual(ulng(g[0]), r, "Gray k=1 closed form"); }
        if (k == 2) { checkEqual(ulng(lng(g[1]) * (g[1] - 1) / 2 + g[1] - 1 - g[0]), r, "Gray k=2 closed form C(a1,2)+a1-1-a0"); }
        if (r + 1 < total_c) {
            auto h = grayCombinationUnrank(n, k, r + 1);
            vector<int> gone, fresh;
            std::set_difference(g.begin(), g.end(), h.begin(), h.end(), std::back_inserter(gone));
            std::set_difference(h.begin(), h.end(), g.begin(), g.end(), std::back_inserter(fresh));
            check(gone.size() == 1 && fresh.size() == 1, "consecutive Gray ranks differ by one revolving-door exchange");}
        context = "random cool-lex rank n=" + std::to_string(n) + " k=" + std::to_string(k) + " r=" + std::to_string(r);
        for (const auto &x : {a, g}) {
            ulng got = combinationRankCoolLex(n, x);
            check(got < total_c, "cool-lex rank below C(n,k)");
            checkEqual(ulng(n <= 67 ? cool_rank(x) : cool_small(x)), got, n <= 67 ? "Theorem 2 Pascal rank oracle" : "k<=2 closed form");}}
    context = "rank boundaries near 2^64";
    for (auto [n, k] : vector<pair<int, int>>{{67, 33}, {67, 34}, {66, 33}, {64, 32}, {0, 0}, {1, 1}, {9, 8}, {INT_MAX, 0}, {INT_MAX, 1}, {INT_MAX, 2},
                                              {100000, 99999}, {100000, 100000}}) {
        ulll total_c = n <= 67 ? c[n][k] : (k == 0 || k == n) ? 1 : k == 2 ? ulll(n) * (n - 1) / 2 : ulll(n);
        string where = " n=" + std::to_string(n) + " k=" + std::to_string(k);
        for (ulll r : {ulll(0), total_c / 2, total_c - 1}) {
            auto a = combinationUnrank(n, k, ulng(r));
            checkEqual(combinationRank(n, a), ulng(r), "boundary round trip" + where);
            auto g = grayCombinationUnrank(n, k, ulng(r));
            checkEqual(grayCombinationRank(n, g), ulng(r), "Gray boundary round trip" + where);
            if (n <= 67 || k <= 2) {
                for (const auto &x : {a, g}) {
                    checkEqual(combinationRankCoolLex(n, x), ulng(n <= 67 ? cool_rank(x) : cool_small(x)), "cool-lex boundary rank oracle" + where);}}}
        // Gamma(n,k) runs from 0..k-1 to the first of Gamma(n-1,k-1) with n-1 appended, 0..k-2 then n-1.
        vector<int> first(k), last(k);
        iota(first.begin(), first.end(), 0); iota(last.begin(), last.end(), 0);
        if (k) { last.back() = n - 1; }
        checkEqual(grayCombinationUnrank(n, k, 0), first, "Gray rank 0 is 0..k-1" + where);
        checkEqual(grayCombinationRank(n, first), ulng(0), "Gray rank of 0..k-1" + where);
        checkEqual(grayCombinationUnrank(n, k, ulng(total_c - 1)), last, "Gray rank C-1 is 0..k-2,n-1" + where);
        checkEqual(grayCombinationRank(n, last), ulng(total_c - 1), "Gray rank of 0..k-2,n-1" + where);
        // Cool-lex runs from 1^k 0^(n-k) to 1^(k-1) 0^(n-k) 1: the same two index vectors.
        checkEqual(combinationRankCoolLex(n, first), ulng(0), "cool-lex rank of 0..k-1" + where);
        checkEqual(combinationRankCoolLex(n, last), ulng(total_c - 1), "cool-lex rank of 0..k-2,n-1" + where);}
    vector<int> last(33); iota(last.begin(), last.end(), 34);
    checkEqual(combinationRank(67, last), ulng(c[67][33] - 1), "last 33-combination of 67 has rank C-1");
    cout << "PASS combination, Gray and cool-lex combination rank/unrank exhaustive n=0.." << rank_bound << " (recursive, Gamma and rotation lists), random="
         << rounds << " (Pascal, Gamma recurrence and Theorem 2 oracles), closed forms, 2^64 boundary\n";}

// Compositions: each position takes every part >= lo in increasing value, which is lexicographic order.
vector<vector<int>> compositionOracle(int n, int k, int lo) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int left) -> void {
        if (int(state.size()) == k) { if (!left) { out.push_back(state); } return; }
        for (int x = lo; x <= left; ++x) { state.push_back(x); visit(visit, left - x); state.pop_back(); }};
    visit(visit, n); return out;}
// Exact-part partitions: nonincreasing m-part sequences by recursion, sorted by their reversals (colexicographic order).
vector<vector<int>> partitionPartsOracle(int n, int m) {
    vector<vector<int>> out; vector<int> state;
    auto visit = [&](auto &&visit, int left, int cap) -> void {
        if (int(state.size()) == m) { if (!left) { out.push_back(state); } return; }
        for (int x = 1; x <= min(left, cap); ++x) { state.push_back(x); visit(visit, left - x, x); state.pop_back(); }};
    visit(visit, n, n);
    sort(out.begin(), out.end(), [](const vector<int> &a, const vector<int> &b) {
        return std::lexicographical_compare(a.rbegin(), a.rend(), b.rbegin(), b.rend());});
    return out;}
void compositionCases(const string &mode) {
    int bound = mode == "quick" ? 6 : mode == "full" ? 9 : 10;
    auto c = pascal(2 * bound);
    for (int n = 0; n <= bound; ++n) {
        for (int k = 0; k <= bound; ++k) {
            for (int lo : {1, 0}) {
                context = string(lo ? "composition" : "weak composition") + " n=" + std::to_string(n) + " k=" + std::to_string(k);
                auto expected = compositionOracle(n, k, lo);
                ulll count = !k ? ulll(!n) : lo ? (n >= k ? c[n - 1][k - 1] : ulll(0)) : c[n + k - 1][k - 1];
                checkEqual(ulll(expected.size()), count, lo ? "C(n-1,k-1) compositions" : "C(n+k-1,k-1) weak compositions");
                for (int i = 1; i < int(expected.size()); ++i) { check(expected[i - 1] < expected[i], "composition oracle strictly lexicographic"); }
                sequenceCheck(expected, [&](auto visit) {
                    auto inner = [&](auto &state) {
                        static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);};
                    return lo ? forEachComposition(n, k, inner) : forEachWeakComposition(n, k, inner);}, n <= 3 && k <= 3);}}}
    int big = 100000, seen = 0;
    context = "large-dimension compositions k=" + std::to_string(big);
    sequenceCheck(vector<vector<int>>{vector<int>(big)}, [&](auto visit) { return forEachWeakComposition(0, big, visit); });
    sequenceCheck(vector<vector<int>>{vector<int>(big, 1)}, [&](auto visit) { return forEachComposition(big, big, visit); });
    sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachComposition(big - 1, big, visit); });
    check(!forEachWeakComposition(1, big, [&](const vector<int> &a) {
        vector<int> want(big); want[big - 1 - seen] = 1;
        checkEqual(a, want, "large weak composition prefix"); return ++seen < 5;}), "large weak composition cancellation");
    check(seen == 5, "large weak composition callback count");
    // k = INT_MAX with no output must not allocate k parts (8 GB, over the tester's memory cap).
    context = "empty compositions k=INT_MAX";
    for (int n : {0, 5}) { sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachComposition(n, INT_MAX, visit); }); }
    // n = INT_MAX: the last part carries the rest; parts before it count up from the lowest values.
    for (int k : {2, 3}) {
        for (int lo : {1, 0}) {
            context = string(lo ? "composition" : "weak composition") + " INT_MAX prefix k=" + std::to_string(k); seen = 0;
            auto visit = [&](const vector<int> &a) {
                vector<int> want(k, lo);
                want[k - 2] = lo + seen; want[k - 1] = INT_MAX - (k - 1) * lo - seen;
                checkEqual(a, want, "INT_MAX composition prefix"); return ++seen < 5;};
            check(!(lo ? forEachComposition(INT_MAX, k, visit) : forEachWeakComposition(INT_MAX, k, visit)), "INT_MAX composition cancellation");
            check(seen == 5, "INT_MAX composition callback count");}}
    cout << "PASS compositions and weak compositions increasing-part recursion, binomial counts n,k=0.." << bound
         << ", dimension " << big << ", empty k=INT_MAX, INT_MAX prefixes k=2,3\n";

    int part_bound = mode == "quick" ? 14 : mode == "full" ? 24 : 30;
    // p(n,m) = p(n-1,m-1) + p(n-m,m): a part 1, or every part reduced by one.
    vector<vector<lng>> p(part_bound + 1, vector<lng>(part_bound + 2));
    p[0][0] = 1;
    for (int n = 1; n <= part_bound; ++n) { for (int m = 1; m <= n; ++m) { p[n][m] = p[n - 1][m - 1] + p[n - m][m]; } }
    for (int n = 0; n <= part_bound; ++n) {
        checkEqual(accumulate(p[n].begin(), p[n].end(), lng(0)), lng(partitionOracle(n, n).size()), "sum of p(n,m) is p(n)");
        for (int m = 0; m <= n + 1; ++m) {
            context = "partition parts n=" + std::to_string(n) + " m=" + std::to_string(m);
            auto expected = partitionPartsOracle(n, m);
            checkEqual(lng(expected.size()), p[n][m], "p(n,m) count");
            sequenceCheck(expected, [&](auto visit) {
                return forEachIntegerPartitionParts(n, m, [&](auto &state) {
                    static_assert(std::is_const_v<std::remove_reference_t<decltype(state)>>); return visit(state);});}, n <= 5);}
        context = "partition parts n=" + std::to_string(n) + " m=INT_MAX";
        sequenceCheck(vector<vector<int>>{}, [&](auto visit) { return forEachIntegerPartitionParts(n, INT_MAX, visit); });}
    context = "large partition parts n=m and n=m+2, m=" + std::to_string(big);
    vector<int> ones(big, 1), three(ones), twos(ones);
    three[0] = 3; twos[0] = twos[1] = 2;
    sequenceCheck(vector<vector<int>>{ones}, [&](auto visit) { return forEachIntegerPartitionParts(big, big, visit); });
    sequenceCheck(vector<vector<int>>{three, twos}, [&](auto visit) { return forEachIntegerPartitionParts(big + 2, big, visit); });
    // n = INT_MAX: the first part gives one unit at a time to the second.
    for (int m : {2, 3}) {
        context = "partition parts INT_MAX prefix m=" + std::to_string(m); seen = 0;
        check(!forEachIntegerPartitionParts(INT_MAX, m, [&](const vector<int> &a) {
            vector<int> want(m, 1);
            want[0] = INT_MAX - m + 1 - seen; want[1] = 1 + seen;
            checkEqual(a, want, "INT_MAX partition-parts prefix"); return ++seen < 5;}), "INT_MAX partition-parts cancellation");
        check(seen == 5, "INT_MAX partition-parts callback count");}
    cout << "PASS exact-part partitions colex-sorted recursion, p(n,m) triangle n=0.." << part_bound
         << " m=0..n+1, m=" << big << " with n=m,m+2, INT_MAX prefixes m=2,3\n";}

// Cool-lex bitstrings: the rotation oracle, its set checked against the recursive combinations (or their complements).
void coolLexCases(const string &mode) {
    auto strings_of = [](int n, const vector<vector<int>> &combinations, bool complement) {
        vector<vector<char>> out;
        for (const auto &a : combinations) {
            vector<char> b(n, char(complement));
            for (int i : a) { b[i] = char(!complement); }
            out.push_back(b);}
        sort(out.begin(), out.end()); return out;};
    auto check_case = [&](int n, int k, bool thorough) {
        auto expected = coolLexStrings(n, k), sorted = expected;
        sort(sorted.begin(), sorted.end());
        bool complement = n - k < k;
        checkEqual(sorted, k > n ? vector<vector<char>>{} : strings_of(n, combinationOracle(n, complement ? n - k : k), complement), "cool-lex oracle holds each k-subset once");
        sequenceCheck(expected, [&](auto visit) {
            return forEachCombinationCoolLex(n, k, [&](auto &bits) {
                static_assert(std::is_same_v<std::remove_cvref_t<decltype(bits)>, vector<char>>);
                static_assert(std::is_const_v<std::remove_reference_t<decltype(bits)>>); return visit(bits);});}, thorough);
        return expected;};
    int bound = mode == "quick" ? 10 : mode == "full" ? 14 : 16;
    for (int n = 0; n <= bound; ++n) {
        for (int k = 0; k <= n + 1; ++k) {
            context = "cool-lex vector n=" + std::to_string(n) + " k=" + std::to_string(k);
            auto expected = check_case(n, k, n <= 4);
            // Supporting only: the mask form lists the same strings.
            vector<vector<char>> masks;
            forEachCombinationMaskCoolLex<ulng>(n, k, [&](ulng x) {
                vector<char> b(n);
                for (int i = 0; i < n; ++i) { b[i] = char((x >> i) & 1); }
                masks.push_back(b); return true;});
            checkEqual(masks, expected, "vector and mask cool-lex agree");}
        context = "cool-lex vector n=" + std::to_string(n) + " k=INT_MAX";
        sequenceCheck(vector<vector<char>>{}, [&](auto visit) { return forEachCombinationCoolLex(n, INT_MAX, visit); });}
    // Beyond every word: k-subsets of 200 (quick 60) near both ends, counts C(n,r) for r <= 2, first 1^k 0^(n-k), last 1^(k-1) 0^(n-k) 1.
    int n = mode == "quick" ? 60 : 200;
    for (int k : {0, 1, 2, n - 2, n - 1, n, n + 1}) {
        context = "cool-lex vector n=" + std::to_string(n) + " k=" + std::to_string(k);
        auto expected = check_case(n, k, false);
        int r = min(k, n - k);
        checkEqual(lng(expected.size()), r < 0 ? lng(0) : r == 0 ? lng(1) : r == 1 ? lng(n) : lng(n) * (n - 1) / 2, "C(n,k) cool-lex count");
        if (k <= n) {
            vector<char> first(n), last(n);
            fill(first.begin(), first.begin() + k, char(1));
            if (k) { fill(last.begin(), last.begin() + k - 1, char(1)); last[n - 1] = 1; }
            checkEqual(expected.front(), first, "cool-lex first string 1^k 0^(n-k)");
            checkEqual(expected.back(), last, "cool-lex last string 1^(k-1) 0^(n-k) 1");}}
    // Streamed beyond the stored sizes: each output against the rotation rule applied in step, and its rank against its position.
    int streamed = mode == "quick" ? 0 : mode == "full" ? 18 : 20;
    for (int k = 0; streamed && k <= streamed; ++k) {
        context = "cool-lex streamed n=" + std::to_string(streamed) + " k=" + std::to_string(k);
        vector<char> b(streamed);
        fill(b.begin(), b.begin() + k, char(1));
        ulng position = 0;
        forEachCombinationCoolLex(streamed, k, [&](const vector<char> &bits) {
            checkEqual(bits, b, "cool-lex string equals the rotation rule's");
            vector<int> a;
            for (int i = 0; i < streamed; ++i) { if (bits[i]) { a.push_back(i); } }
            checkEqual(combinationRankCoolLex(streamed, a), position++, "cool-lex rank equals streamed position");
            int len = 3;
            while (len <= streamed && !(b[len - 3] == 0 && b[len - 2] == 1)) { ++len; }
            len = min(len, streamed);
            if (len > 1) { std::rotate(b.begin(), b.begin() + len - 1, b.begin() + len); }
            return true;});
        ulll count = 1;
        for (int i = 0; i < k; ++i) { count = count * ulll(streamed - i) / ulll(i + 1); }
        checkEqual(ulll(position), count, "cool-lex streamed count C(n,k)");}
    cout << "PASS cool-lex vectors rotation oracle n=0.." << bound << " k=0..n+1 and k=INT_MAX, n=" << n << " k=0,1,2,n-2..n+1";
    if (streamed) { cout << ", streamed n=" << streamed << " every k with ranks"; }
    cout << "\n";}

void stateCases(const string &mode) {
    context = "default unsigned mask type";
    sequenceCheck(vector<ulng>{0, 1, 2, 3}, [](auto visit) { return forEachSubsetMask(2, visit); });
    sequenceCheck(vector<ulng>{1, 2, 4}, [](auto visit) { return forEachCombinationMask(3, 1, visit); });
    grayMaskCheck(vector<ulng>{0, 1, 3, 2}, [](auto visit) { return forEachGrayMask(2, visit); });
    sequenceCheck(vector<ulng>{3, 6, 5}, [](auto visit) {
        return forEachCombinationMaskCoolLex(3, 2, [&](auto x) { static_assert(std::is_same_v<decltype(x), ulng>); return visit(x); });});
    sequenceCheck(submaskPairOracle<ulng>(2), [](auto visit) {
        return forEachSubmaskPair(2, [&](auto mask, auto sub) {
            static_assert(std::is_same_v<decltype(mask), ulng> && std::is_same_v<decltype(sub), ulng>); return visit(pair{mask, sub});});});
    context = "nested callbacks keep traversal state independent";
    int outer = 0, inner = 0;
    check(forEachCombination(4, 2, [&](const vector<int> &state) {
        auto saved = state;
        check(forEachSubset(3, [&](const vector<int> &) { ++inner; return true; }), "nested completion");
        checkEqual(state, saved, "outer borrowed state survives nested traversal"); ++outer; return true;}), "outer completion");
    check(outer == 6 && inner == 48, "nested callback counts");
    outer = inner = 0;
    int doors = 0;
    check(forEachLabeledTree(4, [&](const vector<pair<int, int>> &edges) {
        auto saved = edges;
        check(forEachLabeledTree(3, [&](const vector<pair<int, int>> &) { return true; }), "nested tree completion");
        check(forEachPairing(4, [&](const vector<int> &mate) {
            auto mate_saved = mate;
            check(forEachPairing(3, [&](const vector<int> &) { ++inner; return true; }), "nested pairing completion");
            checkEqual(mate, mate_saved, "outer pairing survives nested pairing"); return true;}), "middle pairing completion");
        check(forEachGrayCombination(5, 2, [&](const vector<int> &a, int, int) {
            auto door_saved = a;
            check(forEachGrayCombination(4, 2, [&](const vector<int> &, int, int) { ++doors; return true; }), "nested revolving door");
            checkEqual(a, door_saved, "outer revolving door survives nested call"); return true;}), "middle revolving door");
        checkEqual(edges, saved, "outer tree survives nested traversals"); ++outer; return true;}), "outer tree completion");
    check(outer == 16 && inner == 16 * 3 * 3 && doors == 16 * 10 * 6, "nested tree/pairing/revolving-door counts");
    int bits = 0;
    check(forEachSubmaskPair(3, [&](ulng mask, ulng sub) {
        check(forEachCombinationSubmask(mask, 1, [&](ulng single) {
            return forEachSetBit(sub & single, [&](int) { ++bits; return true; });}), "nested submask completion");
        return true;}), "outer submask pair completion");
    check(bits == 27, "nested submask pair/set-bit count");
    outer = inner = 0;
    check(forEachSetPartition(4, 2, [&](const vector<int> &a) {
        auto saved = a;
        check(forEachSetPartition(3, 2, [&](const vector<int> &) { ++inner; return true; }), "nested k-block completion");
        check(forEachSetPartition(3, [&](const vector<int> &) { ++inner; return true; }), "nested set partition completion");
        checkEqual(a, saved, "outer k-block partition survives nested traversals"); ++outer; return true;}), "outer k-block completion");
    check(outer == 7 && inner == 7 * (3 + 5), "nested k-block set partition counts");
    context = "exception propagation";
    for (int api = 0; api < 25; ++api) {
        bool caught = false; int seen = 0;
        auto fail = [&](auto &&...) -> bool { if (++seen == 2) { throw std::runtime_error("visit"); } return true; };
        try {
            if (api == 0) { forEachCombination(3, 1, fail); }
            if (api == 1) { forEachSubset(2, fail); }
            if (api == 2) { forEachProduct({2, 3}, fail); }
            if (api == 3) { forEachGrayProduct({2, 3}, fail); }
            if (api == 4) { forEachSubmask(ulng(3), fail); }
            if (api == 5) { forEachSubsetMask(2, fail); }
            if (api == 6) { forEachCombinationMask(3, 1, fail); }
            if (api == 7) { forEachGrayMask(2, fail); }
            if (api == 8) { forEachMulticombination(2, 3, fail); }
            if (api == 9) { forEachSupermask(ulng(1), 3, fail); }
            if (api == 10) { forEachIntegerPartition(4, fail); }
            if (api == 11) { forEachIntegerPartition(4, 2, fail); }
            if (api == 12) { forEachSetPartition(3, fail); }
            if (api == 13) { forEachSetBit(ulng(7), fail); }
            if (api == 14) { forEachCombinationSubmask(ulng(7), 1, fail); }
            if (api == 15) { forEachSubmaskPair(2, fail); }
            if (api == 16) { forEachLabeledTree(4, fail); }
            if (api == 17) { forEachGrayCombination(4, 2, fail); }
            if (api == 18) { forEachPairing(4, fail); }
            if (api == 19) { forEachSetPartition(4, 2, fail); }
            if (api == 20) { forEachComposition(4, 2, fail); }
            if (api == 21) { forEachWeakComposition(2, 2, fail); }
            if (api == 22) { forEachIntegerPartitionParts(6, 2, fail); }
            if (api == 23) { forEachCombinationMaskCoolLex(3, 1, fail); }
            if (api == 24) { forEachCombinationCoolLex(3, 1, fail); }} catch (const std::runtime_error &e) { caught = string(e.what()) == "visit"; }
        check(caught && seen == 2, "exception propagates at callback two api=" + std::to_string(api));}
    int n = mode == "quick" ? 1000 : mode == "full" ? 100000 : 500000, seen = 0;
    context = "large dimensions n=" + std::to_string(n);
    check(!forEachSubset(n, [&](const vector<int> &state) {
        vector<int> expected;
        for (int bit = 3; bit >= 0; --bit) { if ((seen >> bit) & 1) { expected.push_back(bit); } }
        checkEqual(state, expected, "large subset first ten"); return ++seen < 10;}), "large subset cancellation");
    vector<int> radices(n, 1); radices[n / 2] = 3; radices.back() = 2;
    seen = 0;
    check(forEachProduct(radices, [&](const vector<int> &state) {
        check(int(state.size()) == n && state[n / 2] == seen / 2 && state.back() == seen % 2, "large product active digits");
        check(accumulate(state.begin(), state.end(), 0) == seen / 2 + seen % 2, "large product inactive digits"); ++seen; return true;}), "large sparse product completion"); check(seen == 6, "large sparse product count");
    seen = 0;
    check(forEachGrayProduct(radices, [&](const vector<int> &state, int changed) {
        int a = seen / 2, b = (a % 2) ? 1 - seen % 2 : seen % 2;
        check(int(state.size()) == n && state[n / 2] == a && state.back() == b, "large Gray product active digits");
        check(changed == (!seen ? -1 : seen % 2 ? n - 1 : n / 2), "large Gray product active change");
        check(accumulate(state.begin(), state.end(), 0) == a + b, "large Gray inactive digits"); ++seen; return true;}), "large sparse Gray product completion"); check(seen == 6, "large sparse Gray product count");
    seen = 0;
    check(forEachMulticombination(1, n, [&](const vector<int> &state) {
        check(int(state.size()) == n && accumulate(state.begin(), state.end(), 0) == 0, "large constant multicombination");
        ++seen; return true;}) && seen == 1, "large multicombination completes without recursion");
    // The first codes in lexicographic order are 0...0j; the set decoder gives their trees.
    seen = 0;
    check(!forEachLabeledTree(n, [&](const vector<pair<int, int>> &edges) {
        vector<int> code(n - 2); code.back() = seen;
        checkEqual(edges, prueferDecode(n, code), "large labeled-tree prefix"); return ++seen < 5;}), "large labeled-tree cancellation");
    check(seen == 5, "large labeled-tree callback count");
    // The first pairings fix (0,1),(2,3),... and run through the pairings of the last 4 (even) or 5 (odd) elements.
    for (int big : {n, n + 1}) {
        int t = big & 1 ? 5 : 4;
        auto tail = pairingOracle(t);
        seen = 0;
        check(!forEachPairing(big, [&](const vector<int> &mate) {
            vector<int> want(big);
            for (int u = 0; u < big - t; u += 2) { want[u] = u + 1; want[u + 1] = u; }
            for (int u = 0; u < t; ++u) { want[big - t + u] = tail[seen][u] < 0 ? -1 : tail[seen][u] + big - t; }
            checkEqual(mate, want, "large pairing prefix n=" + std::to_string(big)); return ++seen < int(tail.size());}), "large pairing cancellation");
        check(seen == int(tail.size()), "large pairing callback count");}
    // k=1 and k=n give one string; k=2 counts in binary after a[0]=0; k=n-1 opens with the 15 of n=6, then 5..n-2.
    vector<int> zeros(n), identity(n);
    iota(identity.begin(), identity.end(), 0);
    for (auto [k, want] : vector<pair<int, vector<int>>>{{1, zeros}, {n, identity}}) {
        seen = 0;
        check(forEachSetPartition(n, k, [&](const vector<int> &a) {
            checkEqual(a, want, "large single k-block partition k=" + std::to_string(k)); ++seen; return true;}) && seen == 1, "large single k-block completion");}
    seen = 0;
    check(!forEachSetPartition(n, 2, [&](const vector<int> &a) {
        vector<int> want(n);
        for (int bit = 0; bit < 5; ++bit) { want[n - 1 - bit] = ((seen + 1) >> bit) & 1; }
        checkEqual(a, want, "large two-block prefix"); return ++seen < 10;}), "large two-block cancellation");
    check(seen == 10, "large two-block callback count");
    vector<vector<int>> head;
    for (const auto &a : setPartitionOracle(6)) { if (set<int>(a.begin(), a.end()).size() == 5) { head.push_back(a); } }
    check(head.size() == 15, "C(6,2) strings with 5 blocks");
    seen = 0;
    check(!forEachSetPartition(n, n - 1, [&](const vector<int> &a) {
        auto want = head[seen];
        for (int i = 6; i < n; ++i) { want.push_back(i - 1); }
        checkEqual(a, want, "large (n-1)-block prefix"); return ++seen < 15;}), "large (n-1)-block cancellation");
    check(seen == 15, "large (n-1)-block callback count");
    context = "INT_MAX coordinates/radices without output-count arithmetic"; seen = 0;
    check(!forEachCombination(INT_MAX, 2, [&](const vector<int> &state) {
        checkEqual(state, vector<int>{0, seen + 1}, "INT_MAX combination prefix"); return ++seen < 3;}), "INT_MAX combination cancellation");
    seen = 0;
    check(!forEachMulticombination(INT_MAX, 2, [&](const vector<int> &state) {
        checkEqual(state, vector<int>{0, seen}, "INT_MAX multicombination prefix"); return ++seen < 3;}), "INT_MAX multicombination cancellation");
    for (bool gray : {false, true}) {
        seen = 0;
        auto visit = [&](const vector<int> &state) {
            checkEqual(state, vector<int>{0, seen}, "INT_MAX radix prefix"); return ++seen < 3;};
        bool done = gray ? forEachGrayProduct({INT_MAX, INT_MAX}, [&](const vector<int> &s, int changed) {
            check(changed == (seen ? 1 : -1), "INT_MAX Gray dimension"); return visit(s);}) : forEachProduct({INT_MAX, INT_MAX}, visit);
        check(!done && seen == 3, "INT_MAX product cancellation");}
    cout << "PASS default types, nesting, all callback exceptions, large n=" << n << " (subsets, products, trees, pairings, k-block set partitions), INT_MAX coordinates/radices\n";}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i]; auto visit = [](auto &&...) { return true; };
            if (probe == "combination-negative-n") { forEachCombination(-1, 0, visit); }
            if (probe == "combination-negative-k") { forEachCombination(3, -1, visit); }
            if (probe == "multicombination-negative-n") { forEachMulticombination(-1, 0, visit); }
            if (probe == "multicombination-negative-k") { forEachMulticombination(3, -1, visit); }
            if (probe == "subset-negative-n") { forEachSubset(-1, visit); }
            if (probe == "product-negative") { forEachProduct({2, -1}, visit); }
            if (probe == "product-zero-negative") { forEachProduct({0, -1}, visit); }
            if (probe == "gray-product-negative") { forEachGrayProduct({2, -1}, visit); }
            if (probe == "gray-product-zero-negative") { forEachGrayProduct({0, -1}, visit); }
            if (probe == "subset-mask-negative") { forEachSubsetMask(-1, visit); }
            if (probe == "subset-mask-large") { forEachSubsetMask<uint8_t>(9, visit); }
            if (probe == "combination-mask-negative-n") { forEachCombinationMask(-1, 0, visit); }
            if (probe == "combination-mask-large-n") { forEachCombinationMask<ulll>(129, 0, visit); }
            if (probe == "combination-mask-negative-k") { forEachCombinationMask(2, -1, visit); }
            if (probe == "gray-mask-negative") { forEachGrayMask(-1, visit); }
            if (probe == "gray-mask-large") { forEachGrayMask<uint8_t>(9, visit); }
            if (probe == "supermask-outside") { forEachSupermask(ulng(8), 3, visit); }
            if (probe == "supermask-outside-stop") { forEachSupermask(ulng(8), 3, [](ulng) { return false; }); }
            if (probe == "supermask-large") { forEachSupermask(uint8_t(0), 9, visit); }
            if (probe == "partition-negative") { forEachIntegerPartition(-1, visit); }
            if (probe == "partition-negative-capped") { forEachIntegerPartition(-1, 5, visit); }
            if (probe == "partition-zero-part") { forEachIntegerPartition(3, 0, visit); }
            if (probe == "set-partition-negative") { forEachSetPartition(-1, visit); }
            if (probe == "combination-submask-negative-k") { forEachCombinationSubmask(ulng(7), -1, visit); }
            if (probe == "submask-pair-negative") { forEachSubmaskPair(-1, visit); }
            if (probe == "submask-pair-large") { forEachSubmaskPair<uint8_t>(9, visit); }
            if (probe == "labeled-tree-zero") { forEachLabeledTree(0, visit); }
            if (probe == "gray-combination-negative-n") { forEachGrayCombination(-1, 0, visit); }
            if (probe == "gray-combination-negative-k") { forEachGrayCombination(3, -1, visit); }
            if (probe == "pairing-negative") { forEachPairing(-1, visit); }
            if (probe == "pairing-int-max") { forEachPairing(INT_MAX, visit); }
            if (probe == "rank-overflow") { vector<int> a(34); iota(a.begin(), a.end(), 0); combinationRank(68, a); }
            if (probe == "rank-unsorted") { combinationRank(5, {2, 1}); }
            if (probe == "rank-range") { combinationRank(5, {1, 5}); }
            if (probe == "rank-k-large") { combinationRank(1, {0, 1}); }
            if (probe == "unrank-k-large") { combinationUnrank(3, 4, 0); }
            if (probe == "unrank-rank-large") { combinationUnrank(5, 2, 10); }
            if (probe == "unrank-overflow") { combinationUnrank(68, 34, 0); }
            if (probe == "gray-rank-overflow") { vector<int> a(34); iota(a.begin(), a.end(), 0); grayCombinationRank(68, a); }
            if (probe == "gray-rank-unsorted") { grayCombinationRank(5, {2, 1}); }
            if (probe == "gray-rank-range") { grayCombinationRank(5, {1, 5}); }
            if (probe == "gray-rank-above") { grayCombinationRank(5, {1, 7}); }
            if (probe == "gray-rank-negative") { grayCombinationRank(5, {-1, 2}); }
            if (probe == "gray-unrank-negative-k") { grayCombinationUnrank(3, -1, 0); }
            if (probe == "gray-rank-k-large") { grayCombinationRank(1, {0, 1}); }
            if (probe == "gray-unrank-rank-large") { grayCombinationUnrank(5, 2, 10); }
            if (probe == "gray-unrank-k-large") { grayCombinationUnrank(3, 4, 0); }
            if (probe == "gray-unrank-overflow") { grayCombinationUnrank(68, 34, 0); }
            if (probe == "set-partition-k-negative-n") { forEachSetPartition(-1, 0, visit); }
            if (probe == "set-partition-k-negative-k") { forEachSetPartition(3, -1, visit); }
            if (probe == "composition-negative-n") { forEachComposition(-1, 2, visit); }
            if (probe == "composition-negative-k") { forEachComposition(3, -1, visit); }
            if (probe == "weak-composition-negative-n") { forEachWeakComposition(-1, 2, visit); }
            if (probe == "weak-composition-negative-k") { forEachWeakComposition(3, -1, visit); }
            if (probe == "partition-parts-negative-n") { forEachIntegerPartitionParts(-1, 0, visit); }
            if (probe == "partition-parts-negative-m") { forEachIntegerPartitionParts(3, -1, visit); }
            if (probe == "cool-lex-negative-k") { forEachCombinationMaskCoolLex(3, -1, visit); }
            if (probe == "cool-lex-negative-n") { forEachCombinationMaskCoolLex(-1, 0, visit); }
            if (probe == "cool-lex-large-n") { forEachCombinationMaskCoolLex<uint8_t>(9, 1, visit); }
            if (probe == "cool-lex-vector-negative-n") { forEachCombinationCoolLex(-1, 0, visit); }
            if (probe == "cool-lex-vector-negative-k") { forEachCombinationCoolLex(3, -1, visit); }
            if (probe == "cool-rank-unsorted") { combinationRankCoolLex(5, {2, 1}); }
            if (probe == "cool-rank-range") { combinationRankCoolLex(5, {1, 5}); }
            if (probe == "cool-rank-negative") { combinationRankCoolLex(5, {-1, 2}); }
            if (probe == "cool-rank-k-large") { combinationRankCoolLex(1, {0, 1}); }
            if (probe == "cool-rank-overflow") { vector<int> a(34); iota(a.begin(), a.end(), 0); combinationRankCoolLex(68, a); }
            return 1;}}
    vectorCases(mode);
    wordCases<uint8_t>(mode); wordCases<uint16_t>(mode); wordCases<uint>(mode);
    wordCases<ulng>(mode); wordCases<ulll>(mode); objectCases(mode); compositionCases(mode); coolLexCases(mode); stateCases(mode);
    cout << "PASS enumeration cases=" << cases << " checks=" << checks << " seed=" << test_seed << '\n';}
