#include "../../07-Strings/22-lexicographic_queries.hpp"
#include "../../07-Strings/10-suffixautomaton.hpp"

using V = vector<int>;
ulng test_seed = 0;
lng checks = 0, cases = 0;
string context;
template<class A> void print(const A &x) {
    if constexpr (std::is_arithmetic_v<A>) { cerr << x; }
    else if constexpr (requires { x.first; }) { cerr << '(' ; print(x.first); cerr << ','; print(x.second); cerr << ')'; }
    else if constexpr (requires { std::get<2>(x); }) { cerr << '(' << std::get<0>(x) << ',' << std::get<1>(x) << ',' << std::get<2>(x) << ')'; }
    else {
        cerr << '[';
        for (const auto &y : x) { print(y); cerr << ' '; }
        cerr << ']';}}
template<class A, class B> void expect(const A &actual, const B &expected, const char *op) {
    ++checks;
    if (actual == expected) { return; }
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << op << " expected=";
    print(expected);
    cerr << " actual=";
    print(actual);
    cerr << '\n';
    std::exit(1);}
string show(const V &s) {
    string res;
    for (int c : s) { res += std::to_string(c) + ' '; }
    return res.size() > 200 ? res.substr(0, 200) + "...(" + std::to_string(s.size()) + ")" : res;}
V sub(const V &s, int l, int r) { return V(s.begin() + l, s.begin() + r); }
int sign(const V &a, const V &b) { return a < b ? -1 : b < a ? 1 : 0; }
int brutePrefix(const V &s, int i, int j) {
    int k = 0;
    while (i + k < int(s.size()) && j + k < int(s.size()) && s[i + k] == s[j + k]) { ++k; }
    return k;}
// Every query of one text against brute substring enumeration; all is false for long texts, which sample range pairs.
void checkText(const V &s, int sigma, std::mt19937_64 &rng, bool all) {
    ++cases;
    context = "s=" + show(s) + " sigma=" + std::to_string(sigma);
    int n = int(s.size());
    SuffixArray<int> sa(s);
    map<V, vector<int>> occ;
    for (int i = 0; i <= n; ++i) {
        for (int j = i; j <= n; ++j) { occ[sub(s, i, j)].push_back(i); }}
    auto count = [&](const V &w) { auto it = occ.find(w); return it == occ.end() ? 0 : int(it->second.size()); };
    vector<pair<int, int>> ranges;
    for (int l = 0; l <= n; ++l) {
        for (int r = l; r <= n; ++r) { ranges.push_back({l, r}); }}
    vector<pair<pair<int, int>, pair<int, int>>> pairs;
    for (int t = 0; t < (all ? int(ranges.size() * ranges.size()) : 3000); ++t) {
        int x = all ? t / int(ranges.size()) : int(rng() % ranges.size()), y = all ? t % int(ranges.size()) : int(rng() % ranges.size());
        pairs.push_back({ranges[x], ranges[y]});}
    BidirectionalSuffixArray<int> bi(s);
    DictionaryOfBasicFactors<int> dbf(s);
    for (auto [x, y] : pairs) {
        auto [l, r] = x;
        auto [a, b] = y;
        V u = sub(s, l, r), w = sub(s, a, b);
        expect(compareSubstrings(sa, l, r, a, b), sign(u, w), "compareSubstrings");
        int k = 0;
        while (k < r - l && k < b - a && s[r - 1 - k] == s[b - 1 - k]) { ++k; }
        expect(bi.longestCommonSuffix(l, r, a, b), k, "longestCommonSuffix");
        expect(dbf.equal(l, r, a, b), u == w, "DictionaryOfBasicFactors::equal");
        expect(dbf.compare(l, r, a, b), sign(u, w), "DictionaryOfBasicFactors::compare");}
    for (int i = 0; i <= n; ++i) {
        for (int j = 0; j <= n; ++j) {
            if (all || rng() % 8 == 0) { expect(dbf.lcp(i, j), brutePrefix(s, i, j), "DictionaryOfBasicFactors::lcp"); }}}
    for (int k = 0; (1 << k) <= n; ++k) {
        set<V> factors;
        for (int i = 0; i + (1 << k) <= n; ++i) { factors.insert(sub(s, i, i + (1 << k))); }
        for (int i = 0; i + (1 << k) <= n; ++i) { expect(dbf.name(k, i), int(std::distance(factors.begin(), factors.find(sub(s, i, i + (1 << k))))), "DictionaryOfBasicFactors::name"); }}
    expect(dbf.size(), n, "DictionaryOfBasicFactors::size");
    expect(int(dbf.names.size()), n ? std::bit_width(uint(n)) : 0, "DictionaryOfBasicFactors levels");
    // Suffix order, occurrence ranges and counts.
    vector<V> suffixes;
    for (int i = 0; i < n; ++i) { suffixes.push_back(sub(s, i, n)); }
    sort(suffixes.begin(), suffixes.end());
    for (auto [l, r] : ranges) {
        V w = sub(s, l, r);
        int lo = 0, hi = 0;
        while (lo < n && !(sub(suffixes[lo], 0, min(int(suffixes[lo].size()), r - l)) == w && int(suffixes[lo].size()) >= r - l)) { ++lo; }
        hi = lo;
        while (hi < n && int(suffixes[hi].size()) >= r - l && sub(suffixes[hi], 0, r - l) == w) { ++hi; }
        if (l == r) { lo = 0; hi = n; }
        expect(occurrenceRange(sa, l, r), pair<int, int>{lo, hi}, "occurrenceRange");
        expect(occurrenceCount(sa, l, r), count(w), "occurrenceCount");}
    // Distinct and multiset orders.
    vector<V> distinct;
    vector<pair<V, int>> multi;
    for (const auto &[w, starts] : occ) {
        if (w.empty()) { continue; }
        distinct.push_back(w);
        for (size_t t = 0; t < starts.size(); ++t) { multi.push_back({w, starts[0]}); }}
    SubstringOrder<int> order(s);
    expect(order.kthDistinctSubstring(-1), pair<int, int>{-1, 0}, "kthDistinctSubstring(-1)");
    expect(order.kthSubstring(-1), pair<int, int>{-1, 0}, "kthSubstring(-1)");
    expect(order.kthDistinctSubstring(lng(distinct.size())), pair<int, int>{-1, 0}, "kthDistinctSubstring(end)");
    expect(order.kthSubstring(lng(multi.size())), pair<int, int>{-1, 0}, "kthSubstring(end)");
    for (size_t k = 0; k < distinct.size(); ++k) {
        if (all || rng() % 4 == 0) { expect(order.kthDistinctSubstring(lng(k)), pair<int, int>{occ[distinct[k]][0], int(distinct[k].size())}, "kthDistinctSubstring"); }}
    for (size_t k = 0; k < multi.size(); ++k) {
        if (all || rng() % 8 == 0) { expect(order.kthSubstring(lng(k)), pair<int, int>{multi[k].second, int(multi[k].first.size())}, "kthSubstring"); }}
    for (auto [l, r] : ranges) {
        if (!all && rng() % 4) { continue; }
        V w = sub(s, l, r);
        lng smaller = 0;
        for (const auto &x : multi) { smaller += x.first < w; }
        expect(order.distinctSubstringRank(l, r), lng(lower_bound(distinct.begin(), distinct.end(), w) - distinct.begin()), "distinctSubstringRank");
        expect(order.substringRank(l, r), smaller, "substringRank");}
    // sortSubstrings over a sample with duplicates and empty ranges.
    vector<pair<int, int>> list;
    for (int t = 0; t < 40; ++t) { list.push_back(ranges[rng() % ranges.size()]); }
    set<V> listed;
    for (auto [l, r] : list) { listed.insert(sub(s, l, r)); }
    vector<int> dense;
    for (auto [l, r] : list) { dense.push_back(int(std::distance(listed.begin(), listed.find(sub(s, l, r))))); }
    expect(sortSubstrings(sa, list), dense, "sortSubstrings");
    expect(sortSubstrings(sa, {}), vector<int>{}, "sortSubstrings empty");
    // Frequency statistics and LCP sums.
    vector<lng> freq(n + 1);
    for (const auto &w : distinct) { ++freq[count(w)]; }
    expect(frequencyStatistics(sa), freq, "frequencyStatistics");
    SuffixArray<int> bare(s, false);
    expect(frequencyStatistics(bare), freq, "frequencyStatistics without RMQ");
    vector<lng> sums(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) { sums[i] += brutePrefix(s, i, j); }}
    expect(lcpSums(sa), sums, "lcpSums");
    expect(lcpSums(bare), sums, "lcpSums without RMQ");
    // Minimal absent words over [0, sigma).
    // Candidates a u b need a u and u b to occur, so a ranges over symbols before u and b over symbols after it.
    map<V, pair<set<int>, set<int>>> sides;
    for (const auto &[w, starts] : occ) {
        if (w.empty()) { continue; }
        sides[V(w.begin() + 1, w.end())].first.insert(w[0]);
        sides[V(w.begin(), w.end() - 1)].second.insert(w.back());}
    set<V> maw;
    for (const auto &[u, ab] : sides) {
        for (int a : ab.first) {
            for (int b : ab.second) {
                V aub = u;
                aub.insert(aub.begin(), a); aub.push_back(b);
                if (!count(aub)) { maw.insert(aub); }}}}
    for (int c = 0; c < sigma; ++c) {
        if (!count(V{c})) { maw.insert(V{c}); }}
    set<V> got;
    auto words = minimalAbsentWords(sa, sigma);
    for (auto [b, start, len] : words) {
        V w{b};
        V tail = sub(s, start, start + len);
        w.insert(w.end(), tail.begin(), tail.end());
        expect(got.insert(w).second, true, "minimalAbsentWords duplicate");
        expect(start, len ? occ[tail][0] : 0, "minimalAbsentWords leftmost start");}
    expect(got, maw, "minimalAbsentWords");
    expect(minimalAbsentWords(bare, sigma) == words, true, "minimalAbsentWords without RMQ");
    // Minimal unique and shortest unique substrings.
    vector<pair<int, int>> mus;
    for (int i = 0; i < n; ++i) {
        for (int len = 1; i + len <= n; ++len) {
            if (count(sub(s, i, i + len)) == 1 && (len == 1 || (count(sub(s, i + 1, i + len)) > 1 && count(sub(s, i, i + len - 1)) > 1))) { mus.push_back({i, len}); }}}
    expect(minimalUniqueSubstrings(sa), mus, "minimalUniqueSubstrings");
    expect(minimalUniqueSubstrings(bare), mus, "minimalUniqueSubstrings without RMQ");
    vector<pair<int, int>> sus(n, {INT_MAX, INT_MAX});
    for (int p = 0; p < n; ++p) {
        pair<int, int> best{INT_MAX, INT_MAX};
        for (int i = 0; i <= p; ++i) {
            for (int j = p + 1; j <= n; ++j) {
                if (count(sub(s, i, j)) == 1) { best = min(best, {j - i, i}); break; }}}
        sus[p] = {best.second, best.first};}
    expect(shortestUniqueSubstrings(sa), sus, "shortestUniqueSubstrings");
    expect(shortestUniqueSubstrings(bare), sus, "shortestUniqueSubstrings without RMQ");
    // Per-prefix suffix extrema and distinct counts.
    vector<int> lo{0}, hi{0};
    vector<lng> per{0};
    for (int i = 1; i <= n; ++i) {
        int a = 0, b = 0;
        for (int j = 1; j < i; ++j) {
            if (sub(s, j, i) < sub(s, a, i)) { a = j; }
            if (sub(s, b, i) < sub(s, j, i)) { b = j; }}
        lo.push_back(a); hi.push_back(b);
        set<V> seen;
        for (int x = 0; x < i; ++x) {
            for (int y = x + 1; y <= i; ++y) { seen.insert(sub(s, x, y)); }}
        per.push_back(lng(seen.size()));}
    expect(lexMinSuffixPerPrefix(s), lo, "lexMinSuffixPerPrefix");
    expect(lexMaxSuffixPerPrefix(s), hi, "lexMaxSuffixPerPrefix");
    expect(distinctSubstringsPerPrefix(sa), per, "distinctSubstringsPerPrefix");}
// String collections: lce, compare and comparePieces against explicit concatenations.
void checkCollection(const vector<V> &strings, std::mt19937_64 &rng) {
    ++cases;
    context = "collection of " + std::to_string(strings.size());
    for (const V &s : strings) { context += " [" + show(s) + "]"; }
    StringCollection col(strings);
    int k = int(strings.size());
    for (int t = 0; t < 300 && k; ++t) {
        int i = int(rng() % k), j = int(rng() % k), ni = int(strings[i].size()), nj = int(strings[j].size());
        int l = int(rng() % (ni + 1)), r = l + int(rng() % (ni - l + 1)), a = int(rng() % (nj + 1)), b = a + int(rng() % (nj - a + 1));
        int e = 0;
        while (l + e < ni && a + e < nj && strings[i][l + e] == strings[j][a + e]) { ++e; }
        expect(col.lce(i, l, j, a), e, "StringCollection::lce");
        expect(col.compare(i, l, r, j, a, b), sign(sub(strings[i], l, r), sub(strings[j], a, b)), "StringCollection::compare");
        vector<array<int, 3>> x, y;
        V wx, wy;
        for (int side = 0; side < 2; ++side) {
            auto &pieces = side ? y : x;
            V &word = side ? wy : wx;
            for (int c = int(rng() % 4); c > 0; --c) {
                int q = int(rng() % k), nq = int(strings[q].size()), pl = int(rng() % (nq + 1)), pr = pl + int(rng() % (nq - pl + 1));
                pieces.push_back({q, pl, pr});
                word.insert(word.end(), strings[q].begin() + pl, strings[q].begin() + pr);}}
        if (t % 5 == 0) { y = x; wy = wx; }
        expect(col.comparePieces(x, y), sign(wx, wy), "StringCollection::comparePieces");}
    for (int i = 0; i < k; ++i) { expect(col.length(i), int(strings[i].size()), "StringCollection::length"); }}
V randomText(std::mt19937_64 &rng, int n, int sigma) {
    V s(n);
    for (int &c : s) { c = int(rng() % sigma); }
    return s;}
V fibonacci(int n) {
    string a = "0", b = "01";
    while (int(b.size()) < n) { a = std::exchange(b, b + a); }
    V s(n);
    for (int i = 0; i < n; ++i) { s[i] = b[i] - '0'; }
    return s;}
V thueMorse(int n) {
    V s(n);
    for (int i = 0; i < n; ++i) { s[i] = std::popcount(uint(i)) & 1; }
    return s;}
void exhaustive(const string &mode) {
    std::mt19937_64 rng(test_seed);
    int binary = mode == "quick" ? 7 : mode == "full" ? 9 : 12, ternary = mode == "quick" ? 4 : mode == "full" ? 6 : 7;
    lng before = cases;
    for (int sigma : {2, 3}) {
        for (int n = 0; n <= (sigma == 2 ? binary : ternary); ++n) {
            V s(n);
            for (lng code = 0, total = lng(std::pow(sigma, n)); code < total; ++code) {
                lng x = code;
                for (int i = 0; i < n; ++i) { s[i] = int(x % sigma); x /= sigma; }
                checkText(s, sigma, rng, n <= 6);}}}
    cout << "PASS exhaustive texts binary<=" << binary << " ternary<=" << ternary << " cases=" << cases - before << '\n';}
void randomCases(const string &mode) {
    std::mt19937_64 rng(test_seed + 1);
    int count = mode == "quick" ? 30 : mode == "full" ? 200 : 800, top = mode == "quick" ? 25 : mode == "full" ? 45 : 60;
    lng before = cases;
    for (int it = 0; it < count; ++it) {
        int n = int(rng() % (top + 1)), sigma = it % 5 == 4 ? 26 : 1 + int(rng() % 4);
        V s = it % 7 == 0 ? fibonacci(n) : it % 7 == 1 ? thueMorse(n) : randomText(rng, n, sigma);
        if (it % 7 < 2) { sigma = 2; }
        if (it % 7 == 2 && n) {
            int period = 1 + int(rng() % 3);
            for (int i = period; i < n; ++i) { s[i] = s[i - period]; }}
        checkText(s, sigma, rng, false);}
    for (int it = 0; it < count; ++it) {
        vector<V> strings(rng() % 5);
        for (V &x : strings) { x = randomText(rng, int(rng() % 9), 1 + int(rng() % 3)); }
        checkCollection(strings, rng);}
    cout << "PASS random texts up to " << top << " (Fibonacci, Thue-Morse, periodic, sigma 1..4 and 26) and collections cases=" << cases - before << '\n';}
// Other symbol types and the string overloads, with byte order checked on high bytes.
void typesAndStrings(const string &mode) {
    std::mt19937_64 rng(test_seed + 2);
    int count = mode == "quick" ? 20 : mode == "full" ? 100 : 400;
    for (int it = 0; it < count; ++it) {
        int n = int(rng() % 30);
        vector<lng> s(n);
        for (lng &c : s) { c = lng(rng() % 3) * 4000000000000LL - 4000000000000LL; }
        string bytes(n, ' ');
        V ranks(n);
        for (int i = 0; i < n; ++i) { ranks[i] = int((s[i] + 4000000000000LL) / 4000000000000LL); bytes[i] = char(126 + ranks[i]); }
        context = "lng/bytes n=" + std::to_string(n);
        SuffixArray<lng> sl(s);
        SuffixArray sb(bytes);
        SuffixArray<int> sr(ranks);
        SubstringOrder<lng> ol(s);
        SubstringOrder ob(bytes);
        SubstringOrder<int> orr(ranks);
        BidirectionalSuffixArray<lng> bl(s);
        BidirectionalSuffixArray bb(bytes);
        DictionaryOfBasicFactors<lng> dl(s);
        DictionaryOfBasicFactors db(bytes);
        DictionaryOfBasicFactors<int> dr(ranks);
        for (int t = 0; t < 50 && n; ++t) {
            int l = int(rng() % (n + 1)), r = l + int(rng() % (n - l + 1)), a = int(rng() % (n + 1)), b = a + int(rng() % (n - a + 1));
            int want = compareSubstrings(sr, l, r, a, b);
            expect(compareSubstrings(sl, l, r, a, b), want, "compareSubstrings lng");
            expect(compareSubstrings(sb, l, r, a, b), want, "compareSubstrings bytes");
            expect(dl.compare(l, r, a, b), want, "DictionaryOfBasicFactors lng");
            expect(db.compare(l, r, a, b), want, "DictionaryOfBasicFactors bytes");
            expect(dl.lcp(l, a), dr.lcp(l, a), "DictionaryOfBasicFactors::lcp lng");
            expect(bl.longestCommonSuffix(l, r, a, b), bb.longestCommonSuffix(l, r, a, b), "longestCommonSuffix lng/bytes");
            expect(occurrenceRange(sl, l, r), occurrenceRange(sr, l, r), "occurrenceRange lng");
            expect(occurrenceCount(sb, l, r), occurrenceCount(sr, l, r), "occurrenceCount bytes");
            lng k = lng(rng() % ulng(n * (n + 1) / 2));
            expect(ol.kthSubstring(k), orr.kthSubstring(k), "kthSubstring lng");
            expect(ob.kthDistinctSubstring(k % orr.distinct[n]), orr.kthDistinctSubstring(k % orr.distinct[n]), "kthDistinctSubstring bytes");
            expect(ol.substringRank(l, r), orr.substringRank(l, r), "substringRank lng");
            expect(ob.distinctSubstringRank(l, r), orr.distinctSubstringRank(l, r), "distinctSubstringRank bytes");}
        expect(frequencyStatistics(sl), frequencyStatistics(sr), "frequencyStatistics lng");
        expect(lcpSums(sb), lcpSums(sr), "lcpSums bytes");
        expect(minimalUniqueSubstrings(sl), minimalUniqueSubstrings(sr), "minimalUniqueSubstrings lng");
        expect(shortestUniqueSubstrings(sb), shortestUniqueSubstrings(sr), "shortestUniqueSubstrings bytes");
        expect(distinctSubstringsPerPrefix(sl), distinctSubstringsPerPrefix(sr), "distinctSubstringsPerPrefix lng");
        expect(lexMinSuffixPerPrefix(s), lexMinSuffixPerPrefix(ranks), "lexMinSuffixPerPrefix lng");
        expect(lexMaxSuffixPerPrefix(bytes), lexMaxSuffixPerPrefix(ranks), "lexMaxSuffixPerPrefix bytes");
        expect(lexMinSuffixPerPrefix(string_view(bytes)), lexMinSuffixPerPrefix(ranks), "lexMinSuffixPerPrefix string_view");
        vector<lng> shifted(n);
        for (int i = 0; i < n; ++i) { shifted[i] = ranks[i]; }
        SuffixArray<lng> ss(shifted);
        auto mw = minimalAbsentWords(ss, 3);
        auto mr = minimalAbsentWords(sr, 3);
        expect(mw.size(), mr.size(), "minimalAbsentWords lng");
        vector<string> words{bytes, bytes.substr(0, n / 2), "\xff", ""};
        vector<V> coded;
        for (const string &w : words) {
            V x;
            for (char c : w) { x.push_back(uint8_t(c)); }
            coded.push_back(x);}
        StringCollection cs(words), cv(coded);
        for (int t = 0; t < 30; ++t) {
            int i = int(rng() % 4), j = int(rng() % 4);
            int ni = int(words[i].size()), nj = int(words[j].size());
            expect(cs.compare(i, 0, ni, j, 0, nj), cv.compare(i, 0, ni, j, 0, nj), "StringCollection strings");}}
    cout << "PASS lng symbols, byte strings (high bytes) and string overloads cases=" << count << '\n';}
// Medium texts against brute extrema; large texts against the suffix automaton and counting identities.
void large(const string &mode) {
    std::mt19937_64 rng(test_seed + 3);
    int medium = mode == "quick" ? 300 : mode == "full" ? 1000 : 3000;
    for (int kind = 0; kind < 4; ++kind) {
        V s = kind == 0 ? randomText(rng, medium, 2) : kind == 1 ? fibonacci(medium) : kind == 2 ? randomText(rng, medium, 26) : thueMorse(medium);
        context = "medium kind=" + std::to_string(kind) + " n=" + std::to_string(medium);
        vector<int> lo{0}, hi{0};
        for (int i = 1; i <= medium; ++i) {
            int a = 0, b = 0;
            for (int j = 1; j < i; ++j) {
                auto less = [&](int x, int y) { return std::lexicographical_compare(s.begin() + x, s.begin() + i, s.begin() + y, s.begin() + i); };
                if (less(j, a)) { a = j; }
                if (less(b, j)) { b = j; }}
            lo.push_back(a); hi.push_back(b);}
        expect(lexMinSuffixPerPrefix(s), lo, "lexMinSuffixPerPrefix medium");
        expect(lexMaxSuffixPerPrefix(s), hi, "lexMaxSuffixPerPrefix medium");}
    int n = mode == "quick" ? 20000 : mode == "full" ? 50000 : 300000;
    for (int kind = 0; kind < 3; ++kind) {
        string text(n, 'a');
        V fib = fibonacci(n);
        for (int i = 0; i < n; ++i) { text[i] = kind == 0 ? char('a' + rng() % 3) : kind == 1 ? char('a' + fib[i]) : char('a' + (i % 5 == 4)); }
        context = "large kind=" + std::to_string(kind) + " n=" + std::to_string(n);
        SuffixArray sa(text);
        SuffixAutomaton sam;
        sam.build(text);
        SubstringOrder order(text);
        lng total = lng(n) * (n + 1) / 2;
        for (int t = 0; t < 200; ++t) {
            lng k = lng(rng() % ulng(total));
            expect(order.kthSubstring(k), sam.kthSubstring(k), "kthSubstring vs SuffixAutomaton");
            lng d = lng(rng() % ulng(order.distinct[n]));
            expect(order.kthDistinctSubstring(d), sam.kthSubstringDistinct(d), "kthDistinctSubstring vs SuffixAutomaton");
            auto [st, len] = order.kthDistinctSubstring(d);
            expect(order.distinctSubstringRank(st, st + len), d, "distinctSubstringRank inverse");
            auto [st2, len2] = order.kthSubstring(k);
            lng rank = order.substringRank(st2, st2 + len2);
            expect(rank <= k && k < rank + occurrenceCount(sa, st2, st2 + len2), true, "substringRank inverse");
            expect(occurrenceCount(sa, st2, st2 + len2), sam.occurrenceCount(string_view(text).substr(st2, len2)), "occurrenceCount vs SuffixAutomaton");}
        vector<lng> freq = frequencyStatistics(sa), sums = lcpSums(sa), per = distinctSubstringsPerPrefix(sa);
        lng weighted = 0, squares = 0, distinct = 0, pairs = 0;
        for (int k = 0; k <= n; ++k) { weighted += freq[k] * k; squares += freq[k] * k * k; distinct += freq[k]; }
        for (lng x : sums) { pairs += x; }
        expect(weighted, total, "frequencyStatistics occurrences");
        expect(squares, pairs, "lcpSums equals sum of squared occurrence counts");
        expect(distinct, sam.distinctSubstrings(), "frequencyStatistics distinct");
        expect(per, distinctSubstringsOnline(text), "distinctSubstringsPerPrefix vs distinctSubstringsOnline");
        auto mus = minimalUniqueSubstrings(sa);
        for (auto [i, len] : mus) {
            expect(sam.occurrenceCount(string_view(text).substr(i, len)), 1, "MUS unique");
            if (len > 1) {
                expect(sam.occurrenceCount(string_view(text).substr(i + 1, len - 1)) > 1, true, "MUS suffix repeats");
                expect(sam.occurrenceCount(string_view(text).substr(i, len - 1)) > 1, true, "MUS prefix repeats");}}
        auto sus = shortestUniqueSubstrings(sa);
        for (int t = 0; t < 200; ++t) {
            int p = int(rng() % n);
            auto [i, len] = sus[p];
            expect(i <= p && p < i + len && sam.occurrenceCount(string_view(text).substr(i, len)) == 1, true, "SUS covers p uniquely");
            if (len > 1 && len <= 300) {
                bool shorter = false;
                for (int x = max(0, p - len + 2); x <= p && !shorter; ++x) {
                    if (x + len - 1 <= n) { shorter = sam.occurrenceCount(string_view(text).substr(x, len - 1)) == 1 && x + len - 1 > p; }}
                expect(shorter, false, "SUS minimal");}}
        auto maw = minimalAbsentWords(sa, 128);
        for (int t = 0; t < 200 && !maw.empty(); ++t) {
            auto [b, start, len] = maw[rng() % maw.size()];
            string w = char(b) + text.substr(start, len);
            expect(sam.isSubstring(w), false, "MAW absent");
            expect(sam.isSubstring(w.substr(1)) && sam.isSubstring(w.substr(0, w.size() - 1)), true, "MAW proper factors present");}}
    cout << "PASS medium per-prefix extrema n=" << medium << " and large n=" << n << " against the suffix automaton\n";}
int main(int argc, char **argv) {
    string mode = "full", invalid;
    for (int i = 1; i + 1 < argc; i += 2) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[i + 1]; }
        else if (arg == "--seed") { test_seed = std::stoull(argv[i + 1]); }
        else if (arg == "--invalid") { invalid = argv[i + 1]; }}
#if defined(_GLIBCXX_DEBUG) || defined(__SANITIZE_ADDRESS__)
    if (mode == "stress") { mode = "full"; }
#endif
    if (!invalid.empty()) {
        SuffixArray sa(string("abc"));
        if (invalid == "compare-range") { compareSubstrings(sa, 0, 5, 0, 1); }
        if (invalid == "suffix-range") { BidirectionalSuffixArray(string("abc")).longestCommonSuffix(0, 4, 0, 1); }
        if (invalid == "collection-range") { StringCollection(vector<string>{"ab"}).compare(0, 0, 3, 0, 0, 1); }
        if (invalid == "collection-index") { StringCollection(vector<string>{"ab"}).lce(1, 0, 0, 0); }
        if (invalid == "dbf-name") { DictionaryOfBasicFactors(string("abc")).name(2, 0); }
        if (invalid == "dbf-range") { DictionaryOfBasicFactors(string("abc")).compare(2, 1, 0, 1); }
        if (invalid == "occurrence-range") { occurrenceRange(sa, 2, 1); }
        if (invalid == "no-rmq") { occurrenceRange(SuffixArray<int>(vector<int>{1, 2}, false), 0, 1); }
        if (invalid == "maw-symbol") { minimalAbsentWords(SuffixArray<int>(vector<int>{0, 2}), 2); }
        if (invalid == "maw-sigma") { minimalAbsentWords(SuffixArray<int>(vector<int>{}), 0); }
        if (invalid == "order-range") { SubstringOrder(string("abc")).distinctSubstringRank(0, 4); }
        return 0;}
    auto timed = [&](const char *name, void (*f)(const string &)) {
        auto start = std::chrono::steady_clock::now();
        f(mode);
        cout << "TIME " << name << ' ' << std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() << " s\n";};
    timed("exhaustive", exhaustive);
    timed("random", randomCases);
    timed("types", typesAndStrings);
    timed("large", large);
    cout << "PASS lexicographic_queries seed=" << test_seed << " cases=" << cases << " checks=" << checks << '\n';}
