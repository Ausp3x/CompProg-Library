#include "../../02-Data Structures/32-static_range_queries.hpp"

namespace {
    // Debug-mode lower_bound checks partitioning in O(n) per call, so the checked build scales the large corpus down.
#ifdef _GLIBCXX_DEBUG
    constexpr bool CHECKED = true;
#else
    constexpr bool CHECKED = false;
#endif
    // Instrumented builds run fewer stress rounds to stay inside the runner's per-binary timeout.
#if defined(_GLIBCXX_DEBUG) || defined(__SANITIZE_ADDRESS__)
    constexpr bool SLOW = true;
#else
    constexpr bool SLOW = false;
#endif
    ulng seed = 20261102;
    string mode = "full", context;
    lng checks = 0;
    int roundsOverride = -1;
    using Rng = std::mt19937_64;
    lng between(Rng &rng, lng lo, lng hi) {
        ulng span = ulng(hi) - ulng(lo);
        return lng(ulng(lo) + (span == ~0ULL ? rng() : rng() % (span + 1)));}
    lng rnd(Rng &rng, lng lo, lng hi) { return between(rng, lo, hi); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename A, typename B> string show(const pair<A, B> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    // what() builds the operation text only on a mismatch.
    template<typename T, typename U, typename F> void expect(const T &got, const U &want, F &&what) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + what() + " expected=" + show(want) + " actual=" + show(got)); }}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) { expect(got, want, [&] { return op; }); }

    // Ordered by < only, without ==: the header and the oracles must compare it by equivalence.
    struct Key {
        int v = 0;
        bool operator<(const Key &o) const { return v < o.v; }
    };
    std::ostream &operator<<(std::ostream &s, const Key &k) { return s << "K" << k.v; }
    // Lexicographic pair with < and ==, as RangeMajority and RangeThresholdFrequency require.
    struct Pt {
        int a = 0, b = 0;
        bool operator<(const Pt &o) const { return a != o.a ? a < o.a : b < o.b; }
        bool operator==(const Pt &o) const { return a == o.a && b == o.b; }
    };
    std::ostream &operator<<(std::ostream &s, const Pt &p) { return s << "P" << p.a << ":" << p.b; }

    template<typename T> string typeName() {
        if constexpr (std::is_same_v<T, int>) { return "int"; }
        else if constexpr (std::is_same_v<T, lng>) { return "lng"; }
        else if constexpr (std::is_same_v<T, uint>) { return "uint"; }
        else if constexpr (std::is_same_v<T, ulng>) { return "ulng"; }
        else if constexpr (std::is_same_v<T, int8_t>) { return "int8_t"; }
        else if constexpr (std::is_same_v<T, string>) { return "string"; }
        else if constexpr (std::is_same_v<T, Key>) { return "Key"; }
        else { return "Pt"; }}
    // Raw value range of make<T>: lng spans its own range, every other type the int range.
    template<typename T> pair<lng, lng> span() { return std::is_same_v<T, lng> ? pair<lng, lng>{LLONG_MIN, LLONG_MAX} : pair<lng, lng>{INT_MIN, INT_MAX}; }
    // Injective maps from raw values; string order differs from numeric order on purpose.
    template<typename T> T make(lng x) {
        if constexpr (std::is_same_v<T, string>) { return std::to_string(x); }
        else if constexpr (std::is_same_v<T, Key>) { return Key{int(x)}; }
        else if constexpr (std::is_same_v<T, Pt>) { return Pt{int(x / 4), int(x % 4)}; }
        else { return T(x); }}
    template<typename T> T sentinel() {
        if constexpr (std::is_same_v<T, string>) { return "#sentinel"; }
        else if constexpr (std::is_same_v<T, Pt>) { return Pt{-777, -777}; }
        else { return T(424242); }}

    // Brute force: an ordered map of the range (only < is used), and a direct count by equivalence.
    template<typename T> std::map<T, int> tally(const vector<T> &a, int l, int r) {
        std::map<T, int> c;
        for (int i = l; i < r; ++i) { ++c[a[i]]; }
        return c;}
    template<typename T> int bfCount(const vector<T> &a, int l, int r, const T &x) {
        int c = 0;
        for (int i = l; i < r; ++i) { c += !(a[i] < x) && !(x < a[i]); }
        return c;}
    template<typename T> bool bfMajority(const std::map<T, int> &c, int len, T &res) {
        for (auto &[x, k] : c) {
            if (2 * k > len) { res = x; return true; }}
        return false;}
    template<typename T> vector<pair<T, int>> bfFrequent(const std::map<T, int> &c, int t) {
        vector<pair<T, int>> res;
        for (auto &[x, k] : c) {
            if (k >= t) { res.push_back({x, k}); }}
        return res;}
    template<typename M> int bfMex(const vector<M> &a, int l, int r) {
        vector<char> seen(r - l + 1);
        for (int i = l; i < r; ++i) {
            if (std::cmp_greater_equal(a[i], 0) && std::cmp_less_equal(a[i], r - l)) { seen[a[i]] = 1; }}
        int m = 0;
        while (seen[m]) { ++m; }
        return m;}

    // Every operation of T on one array; xs are the count probes, allT tries every threshold 1..len+1.
    template<typename T>
    void generic(const vector<T> &a, const vector<pair<int, int>> &qs, const vector<T> &xs, bool allT) {
        int n = int(a.size());
        auto d = staticRangeCountDistinct(a, qs);
        checkEqual(d.size(), qs.size(), "staticRangeCountDistinct result count");
        StaticRangeFrequency<T> f(a);
        checkEqual(f.n, n, "StaticRangeFrequency n");
        for (size_t j = 0; j < qs.size(); ++j) {
            auto [l, r] = qs[j];
            auto c = tally(a, l, r);
            expect(d[j], int(c.size()), [&] { return "staticRangeCountDistinct query#" + show(j) + " " + show(qs[j]); });
            for (const T &x : xs) { expect(f.count(l, r, x), bfCount(a, l, r, x), [&] { return "count(" + show(l) + "," + show(r) + "," + show(x) + ")"; }); }}
        if constexpr (std::equality_comparable<T>) {
            RangeMajority<T> m(a);
            RangeThresholdFrequency<T> tf(a);
            for (auto [l, r] : qs) {
                auto c = tally(a, l, r);
                T got = sentinel<T>(), want = sentinel<T>();
                bool ok = bfMajority(c, r - l, want);
                expect(m.majority(l, r, got), ok, [&] { return "majority(" + show(l) + "," + show(r) + ") found"; });
                expect(got, want, [&] { return "majority(" + show(l) + "," + show(r) + ") value, sentinel when absent"; });
                int len = r - l;
                vector<int> ts;
                if (allT) {
                    for (int t = 1; t <= len + 1; ++t) { ts.push_back(t); }}
                else { ts = {1, 2, len / 2, len / 2 + 1, len, len + 1, len / 3 + 1, INT_MAX}; }
                for (int t : ts) {
                    if (t >= 1) { expect(tf.frequent(l, r, t), bfFrequent(c, t), [&] { return "frequent(" + show(l) + "," + show(r) + "," + show(t) + ")"; }); }}}}}

    template<typename M>
    void mexAll(const vector<M> &a, const vector<pair<int, int>> &qs) {
        auto got = rangeMex(a, qs);
        checkEqual(got.size(), qs.size(), "rangeMex result count");
        PersistentRangeMex p(a);
        checkEqual(p.n, int(a.size()), "PersistentRangeMex n");
        for (size_t j = 0; j < qs.size(); ++j) {
            auto [l, r] = qs[j];
            int want = bfMex(a, l, r);
            expect(got[j], want, [&] { return "rangeMex query#" + show(j) + " " + show(qs[j]); });
            expect(p.mex(l, r), want, [&] { return "PersistentRangeMex.mex(" + show(l) + "," + show(r) + ")"; });}}

    // Hand cases with literal answers validate the oracles and the header together.
    void hand() {
        context = "hand";
        vector<int> a{3, 1, 3, 3, 2, 1};
        vector<pair<int, int>> qs{{0, 6}, {1, 4}, {4, 6}, {2, 2}, {6, 6}, {2, 4}};
        vector<int> wantD{3, 2, 2, 0, 0, 1};
        checkEqual(staticRangeCountDistinct(a, qs), wantD, "distinct literal");
        for (size_t j = 0; j < qs.size(); ++j) { checkEqual(int(tally(a, qs[j].first, qs[j].second).size()), wantD[j], "oracle distinct literal " + show(qs[j])); }
        StaticRangeFrequency<int> f(a);
        checkEqual(f.count(0, 6, 3), 3, "count literal"); checkEqual(bfCount(a, 0, 6, 3), 3, "oracle count literal");
        checkEqual(f.count(1, 3, 3), 1, "count literal 2"); checkEqual(f.count(0, 6, 4), 0, "count absent");
        RangeMajority<int> m(a);
        int res = -5, w = -5;
        checkEqual(m.majority(0, 6, res), false, "half is no majority"); checkEqual(res, -5, "untouched");
        checkEqual(bfMajority(tally(a, 0, 6), 6, w), false, "oracle half");
        checkEqual(m.majority(0, 4, res), true, "majority 3 of 4"); checkEqual(res, 3, "majority value");
        checkEqual(bfMajority(tally(a, 0, 4), 4, w), true, "oracle 3 of 4"); checkEqual(w, 3, "oracle value");
        checkEqual(m.majority(4, 6, res), false, "two distinct");
        RangeThresholdFrequency<int> t(a);
        checkEqual(t.frequent(0, 6, 1), vector<pair<int, int>>{{1, 2}, {2, 1}, {3, 3}}, "frequent t=1");
        checkEqual(t.frequent(0, 6, 2), vector<pair<int, int>>{{1, 2}, {3, 3}}, "frequent t=2");
        checkEqual(t.frequent(0, 6, 3), vector<pair<int, int>>{{3, 3}}, "frequent t=3");
        checkEqual(t.frequent(0, 6, 4), vector<pair<int, int>>{}, "frequent t=4");
        checkEqual(bfFrequent(tally(a, 0, 6), 2), vector<pair<int, int>>{{1, 2}, {3, 3}}, "oracle frequent");
        vector<int> b{0, 2, 1, 5, 0, 3};
        vector<pair<int, int>> mq{{0, 6}, {1, 3}, {0, 3}, {3, 6}, {3, 3}};
        vector<int> wantM{4, 0, 3, 1, 0};
        checkEqual(rangeMex(b, mq), wantM, "mex literal");
        PersistentRangeMex p(b);
        for (size_t j = 0; j < mq.size(); ++j) {
            checkEqual(bfMex(b, mq[j].first, mq[j].second), wantM[j], "oracle mex literal " + show(mq[j]));
            checkEqual(p.mex(mq[j].first, mq[j].second), wantM[j], "persistent mex literal " + show(mq[j]));}
        std::cout << "PASS hand\n";}

    void edges() {
        context = "edges";
        vector<int> e;
        checkEqual(staticRangeCountDistinct(e, {{0, 0}}), vector<int>{0}, "distinct n=0");
        checkEqual(staticRangeCountDistinct(e, {}), vector<int>{}, "distinct n=0 no queries");
        checkEqual(staticRangeCountDistinct(vector<int>{5, 5}, {}), vector<int>{}, "distinct no queries");
        StaticRangeFrequency<int> f0, f1(e);
        checkEqual(f0.n + f1.n, 0, "frequency n=0"); checkEqual(f0.count(0, 0, 7), 0, "frequency default count");
        checkEqual(rangeMex(e, {{0, 0}, {0, 0}}), vector<int>{0, 0}, "mex n=0");
        checkEqual(rangeMex(vector<int>{0}, {}), vector<int>{}, "mex no queries");
        PersistentRangeMex p0, p1(e);
        checkEqual(p0.mex(0, 0), 0, "persistent default"); checkEqual(p1.mex(0, 0), 0, "persistent n=0");
        RangeMajority<int> m0;
        int res = 42;
        checkEqual(m0.majority(0, 0, res), false, "majority default"); checkEqual(res, 42, "majority default untouched");
        RangeThresholdFrequency<int> t0;
        checkEqual(t0.frequent(0, 0, 1), vector<pair<int, int>>{}, "frequent default");
        RangeMajority<string> ms(vector<string>{"x"});
        string s = "keep";
        checkEqual(ms.majority(1, 1, s), false, "majority empty range at n"); checkEqual(s, string("keep"), "empty range untouched");
        checkEqual(ms.majority(0, 1, s), true, "singleton majority"); checkEqual(s, string("x"), "singleton value");
        RangeThresholdFrequency<string> ts(vector<string>{"x"});
        checkEqual(ts.frequent(0, 1, 1), vector<pair<string, int>>{{"x", 1}}, "singleton frequent");
        checkEqual(ts.frequent(0, 1, 2), vector<pair<string, int>>{}, "singleton above length");
        checkEqual(ts.frequent(1, 1, 1), vector<pair<string, int>>{}, "frequent empty range at n");
        // Values beyond 32 bits or the sign bit must not be truncated into [0, n).
        checkEqual(rangeMex(vector<ulng>{1ULL << 32, 0}, {{0, 1}, {0, 2}}), vector<int>{0, 1}, "mex 2^32 ulng");
        checkEqual(rangeMex(vector<lng>{(1LL << 32) + 1, 0}, {{0, 2}}), vector<int>{1}, "mex 2^32+1 lng");
        checkEqual(rangeMex(vector<uint>{UINT_MAX, 0, 1U << 31}, {{0, 3}}), vector<int>{1}, "mex UINT_MAX uint");
        checkEqual(rangeMex(vector<int>{INT_MIN, -1, 1}, {{0, 3}}), vector<int>{0}, "mex negatives");
        checkEqual(PersistentRangeMex(vector<ulng>{ULLONG_MAX, (1ULL << 32) + 1, 0}).mex(0, 3), 1, "persistent huge ulng");
        checkEqual(PersistentRangeMex(vector<lng>{LLONG_MIN, (1LL << 32), 1, 0}).mex(0, 4), 2, "persistent huge lng");
        checkEqual(PersistentRangeMex(vector<uint>{1U << 31, UINT_MAX}).mex(0, 2), 0, "persistent huge uint");
        std::cout << "PASS edges\n";}

    vector<pair<int, int>> allRanges(int n) {
        vector<pair<int, int>> qs;
        for (int l = 0; l <= n; ++l) {
            for (int r = l; r <= n; ++r) { qs.push_back({l, r}); }}
        return qs;}

    // All arrays over cand[1], cand[3], cand[5] of length <= maxN, all (l, r), count probes at all seven sorted candidates, every threshold.
    template<typename T>
    void exhaustive(const vector<T> &cand, int maxN) {
        vector<T> alpha{cand[1], cand[3], cand[5]};
        for (int n = 0; n <= maxN; ++n) {
            auto qs = allRanges(n);
            int total = 1;
            for (int i = 0; i < n; ++i) { total *= 3; }
            for (int code = 0; code < total; ++code) {
                vector<T> a(n, alpha[0]);
                for (int i = 0, c = code; i < n; ++i, c /= 3) { a[i] = alpha[c % 3]; }
                context = "exhaustive<" + typeName<T>() + "> a=" + show(a) + " seed=" + show(seed);
                generic(a, qs, cand, true);}}
        std::cout << "PASS exhaustive<" << typeName<T>() << "> n<=" << maxN << '\n';}

    template<typename M>
    void mexExhaustive(const vector<M> &alpha, int maxN) {
        int k = int(alpha.size());
        for (int n = 0; n <= maxN; ++n) {
            auto qs = allRanges(n);
            int total = 1;
            for (int i = 0; i < n; ++i) { total *= k; }
            for (int code = 0; code < total; ++code) {
                vector<M> a(n);
                for (int i = 0, c = code; i < n; ++i, c /= k) { a[i] = alpha[c % k]; }
                context = "mexExhaustive<" + typeName<M>() + "> a=" + show(a) + " seed=" + show(seed);
                mexAll(a, qs);}}
        std::cout << "PASS mexExhaustive<" << typeName<M>() << "> n<=" << maxN << '\n';}

    template<typename T> vector<T> alphabet(Rng &rng, int k, bool wide) {
        auto [lo, hi] = span<T>();
        vector<T> res;
        for (int i = 0; i < max(1, k); ++i) {
            lng x = wide ? (rng() % 8 == 0 ? (rng() % 2 ? lo : hi) : between(rng, lo, hi)) : rnd(rng, -k, k);
            res.push_back(make<T>(x));}
        return res;}
    // Profiles: 0 tiny alphabet, 1 wide alphabet of size n + 1, 2 one dominant value, 3 all equal, 4 sorted or reversed, 5 runs.
    constexpr int PROFILES = 6;
    template<typename T> vector<T> randomArray(Rng &rng, int n, int profile) {
        vector<T> a;
        a.reserve(n);
        auto pick = [&](const vector<T> &al) { return al[rng() % al.size()]; };
        if (profile == 0) {
            auto al = alphabet<T>(rng, int(rnd(rng, 1, 4)), rng() % 2);
            for (int i = 0; i < n; ++i) { a.push_back(pick(al)); }}
        else if (profile == 1) {
            auto al = alphabet<T>(rng, n + 1, true);
            for (int i = 0; i < n; ++i) { a.push_back(pick(al)); }}
        else if (profile == 2) {
            T d = alphabet<T>(rng, 1, rng() % 2)[0];
            auto al = alphabet<T>(rng, int(rnd(rng, 1, max(1, n))), rng() % 2);
            int p = std::array{50, 51, 60, 70, 95}[rng() % 5];
            for (int i = 0; i < n; ++i) { a.push_back(int(rng() % 100) < p ? d : pick(al)); }}
        else if (profile == 3) { a.assign(n, alphabet<T>(rng, 1, true)[0]); }
        else if (profile == 4) {
            auto al = alphabet<T>(rng, int(rnd(rng, 1, n / 2 + 1)), rng() % 2);
            for (int i = 0; i < n; ++i) { a.push_back(pick(al)); }
            sort(a.begin(), a.end());
            if (rng() % 2) { reverse(a.begin(), a.end()); }}
        else {
            auto al = alphabet<T>(rng, int(rnd(rng, 1, 6)), rng() % 2);
            while (int(a.size()) < n) {
                T v = pick(al);
                for (int len = int(rnd(rng, 1, 6)); len > 0 && int(a.size()) < n; --len) { a.push_back(v); }}}
        return a;}
    // Mex profiles: 0 permutation, 1 values in [0, k] with gaps, 2 heavy small values, 3 edge values (n - 1, n, n + 1, extremes, 2^31, 2^32), 4 uniform around [0, n], 5 sorted pairs.
    template<typename M> vector<M> mexArray(Rng &rng, int n, int profile) {
        using L = std::numeric_limits<M>;
        constexpr bool S = std::is_signed_v<M>;
        vector<M> a(n);
        if (profile == 0) {
            for (int i = 0; i < n; ++i) { a[i] = M(i); }
            std::shuffle(a.begin(), a.end(), rng);}
        else if (profile == 1) {
            int k = int(rnd(rng, 0, n + 1));
            vector<M> ok;
            for (int v = 0; v <= k; ++v) {
                if (rng() % 4) { ok.push_back(M(v)); }}
            if (ok.empty()) { ok.push_back(M(k)); }
            for (M &x : a) { x = ok[rng() % ok.size()]; }}
        else if (profile == 2) {
            for (M &x : a) { x = M(rnd(rng, 0, 4)); }}
        else if (profile == 3) {
            vector<M> e{M(max(n - 1, 0)), M(n), M(n + 1), M(0), M(1), L::max(), M(L::max() - 1), M(L::max() / 2 + 1)};
            if constexpr (S) { e.insert(e.end(), {L::min(), M(-1), M(-2)}); }
            if constexpr (sizeof(M) == 8) { e.insert(e.end(), {M(lng(1) << 32), M((lng(1) << 32) + 1), M((lng(1) << 32) + 2)}); }
            if constexpr (!S) { e.push_back(M(M(1) << (L::digits - 1))); }
            for (M &x : a) { x = rng() % 2 ? e[rng() % e.size()] : M(rnd(rng, 0, max(0, n - 1))); }}
        else if (profile == 4) {
            for (M &x : a) { x = M(rnd(rng, S ? -3 : 0, n + 3)); }}
        else {
            for (int i = 0; i < n; ++i) { a[i] = M(i / 2); }
            if (rng() % 2) { reverse(a.begin(), a.end()); }}
        return a;}

    // Uniform, empty anywhere, l == r == n, full, prefix, suffix, short, and repeats of earlier queries; already in arbitrary order.
    vector<pair<int, int>> queries(Rng &rng, int n, int q) {
        vector<pair<int, int>> qs;
        qs.reserve(q);
        for (int j = 0; j < q; ++j) {
            int kind = int(rng() % 12), l = 0, r = n;
            if (kind == 0 && !qs.empty()) { qs.push_back(qs[rng() % qs.size()]); continue; }
            if (kind == 1) { l = r = int(rnd(rng, 0, n)); }
            else if (kind == 2) { l = r = n; }
            else if (kind == 3) { l = 0; r = n; }
            else if (kind == 4) { r = int(rnd(rng, 0, n)); }
            else if (kind == 5) { l = int(rnd(rng, 0, n)); }
            else if (kind <= 8) { l = int(rnd(rng, 0, n)); r = min(n, l + int(rnd(rng, 0, 8))); }
            else {
                l = int(rnd(rng, 0, n)); r = int(rnd(rng, 0, n));
                if (l > r) { swap(l, r); }}
            qs.push_back({l, r});}
        return qs;}

    template<typename T> vector<T> probes(Rng &rng, const vector<T> &a) {
        auto [lo, hi] = span<T>();
        vector<T> xs{make<T>(between(rng, lo, hi)), make<T>(lo), make<T>(hi)};
        for (int k = 0; k < 2 && !a.empty(); ++k) { xs.push_back(a[rng() % a.size()]); }
        if constexpr (std::is_same_v<T, string>) { xs.insert(xs.end(), {"", "~"}); }
        return xs;}

    int rounds(int quickR, int fullR, int stressR) {
        if (roundsOverride >= 0) { return roundsOverride; }
        return mode == "quick" ? quickR : mode == "full" ? fullR : SLOW ? stressR / 8 : stressR;}

    template<typename T>
    void randomGeneric(Rng &rng, int r, int maxN) {
        for (int profile = 0; profile < PROFILES; ++profile) {
            for (int s = 0; s < r; ++s) {
                int n = int(rnd(rng, 0, maxN));
                auto a = randomArray<T>(rng, n, profile);
                auto qs = queries(rng, n, int(rnd(rng, 0, 64)));
                context = "random<" + typeName<T>() + "> profile=" + show(profile) + " round=" + show(s) + " n=" + show(n) + " seed=" + show(seed) + (n <= 64 ? " a=" + show(a) : "");
                generic(a, qs, probes(rng, a), false);}}
        std::cout << "PASS random<" << typeName<T>() << "> rounds=" << r << " n<=" << maxN << '\n';}

    template<typename M>
    void randomMex(Rng &rng, int r, int maxN) {
        for (int profile = 0; profile < PROFILES; ++profile) {
            for (int s = 0; s < r; ++s) {
                int n = int(rnd(rng, 0, maxN));
                auto a = mexArray<M>(rng, n, profile);
                auto qs = queries(rng, n, int(rnd(rng, 0, 64)));
                context = "randomMex<" + typeName<M>() + "> profile=" + show(profile) + " round=" + show(s) + " n=" + show(n) + " seed=" + show(seed) + (n <= 64 ? " a=" + show(a) : "");
                mexAll(a, qs);}}
        std::cout << "PASS randomMex<" << typeName<M>() << "> rounds=" << r << " n<=" << maxN << '\n';}

    // Mo's order: blocks of left ends, right ends alternating by block.
    vector<int> moOrder(const vector<pair<int, int>> &qs, int n) {
        int q = int(qs.size()), b = max(1, int(n / max(1.0, std::sqrt(double(q)))));
        vector<int> ord(q);
        iota(ord.begin(), ord.end(), 0);
        sort(ord.begin(), ord.end(), [&](int x, int y) {
            int bx = qs[x].first / b, by = qs[y].first / b;
            if (bx != by) { return bx < by; }
            return bx % 2 ? qs[x].second > qs[y].second : qs[x].second < qs[y].second;});
        return ord;}
    // Mo's oracle for distinct counts over compressed ids in [0, k).
    vector<int> moDistinct(const vector<int> &ids, int k, const vector<pair<int, int>> &qs) {
        vector<int> cnt(k), res(qs.size());
        int L = 0, R = 0, cur = 0;
        for (int j : moOrder(qs, int(ids.size()))) {
            auto [l, r] = qs[j];
            while (R < r) { cur += cnt[ids[size_t(R++)]]++ == 0; }
            while (L > l) { cur += cnt[ids[size_t(--L)]]++ == 0; }
            while (R > r) { cur -= --cnt[ids[size_t(--R)]] == 0; }
            while (L < l) { cur -= --cnt[ids[size_t(L++)]] == 0; }
            res[j] = cur;}
        return res;}
    // Mo's oracle for mex: v[i] in [0, n] or -1, presence counts per sqrt block of the value domain [0, n].
    vector<int> moMex(const vector<int> &v, const vector<pair<int, int>> &qs) {
        int n = int(v.size()), dom = n + 1, s = max(1, int(std::sqrt(double(dom))));
        vector<int> cnt(dom), full(dom / s + 1), res(qs.size());
        auto add = [&](int x) { if (x >= 0 && cnt[x]++ == 0) { ++full[x / s]; } };
        auto del = [&](int x) { if (x >= 0 && --cnt[x] == 0) { --full[x / s]; } };
        int L = 0, R = 0;
        for (int j : moOrder(qs, n)) {
            auto [l, r] = qs[j];
            while (R < r) { add(v[R++]); }
            while (L > l) { add(v[--L]); }
            while (R > r) { del(v[--R]); }
            while (L < l) { del(v[L++]); }
            int b = 0;
            while (full[b] == min(s, dom - b * s)) { ++b; }
            int m = b * s;
            while (cnt[m]) { ++m; }
            res[j] = m;}
        return res;}
    // Offline prefix sweep: count = (# of x in [0, r)) - (# of x in [0, l)); xid < 0 marks an absent value.
    vector<int> sweepCount(const vector<int> &ids, int k, const vector<pair<int, int>> &qs, const vector<int> &xid) {
        int n = int(ids.size());
        vector<vector<pair<int, int>>> at(n + 1);
        for (size_t j = 0; j < qs.size(); ++j) { at[qs[j].first].push_back({int(j), -1}); at[qs[j].second].push_back({int(j), 1}); }
        vector<int> cnt(k), res(qs.size());
        for (int p = 0; p <= n; ++p) {
            for (auto [j, sg] : at[p]) {
                if (xid[j] >= 0) { res[j] += sg * cnt[xid[size_t(j)]]; }}
            if (p < n) { ++cnt[ids[size_t(p)]]; }}
        return res;}

    // Profiles: 0 ten values, 1 n values, 2 one value at 60% plus 1000 others, 3 runs over sqrt(n) values. Sorted alphabet with gaps >= 2, so id order is value order.
    void scaleGeneric(Rng &rng, int n, int q, int profile, int samples) {
        context = "scaleGeneric n=" + show(n) + " q=" + show(q) + " profile=" + show(profile) + " seed=" + show(seed);
        int k = profile == 0 ? 10 : profile == 1 ? n : profile == 2 ? 1001 : max(1, int(std::sqrt(double(n))));
        vector<int> alpha(k);
        lng cur = -1000000000 + rnd(rng, 0, 100);
        for (int i = 0; i < k; ++i) { alpha[i] = int(cur); cur += rnd(rng, 2, max(2LL, 2000000000LL / k)); }
        vector<int> ids(n), a(n);
        int dom = int(rnd(rng, 0, k - 1));
        for (int i = 0; i < n; ++i) {
            if (profile == 2) { ids[i] = rng() % 100 < 60 ? dom : int(rnd(rng, 0, k - 1)); }
            else if (profile == 3) { ids[i] = i && rng() % 8 ? ids[i - 1] : int(rnd(rng, 0, k - 1)); }
            else { ids[i] = int(rnd(rng, 0, k - 1)); }
            a[i] = alpha[ids[size_t(i)]];}
        auto qs = queries(rng, n, q);
        auto d = staticRangeCountDistinct(a, qs), want = moDistinct(ids, k, qs);
        for (size_t j = 0; j < qs.size(); ++j) { expect(d[j], want[j], [&] { return "staticRangeCountDistinct " + show(qs[j]) + " vs Mo"; }); }
        vector<int> xs(q), xid(q);
        for (int j = 0; j < q; ++j) {
            int kind = int(rng() % 10), id = int(rnd(rng, 0, k - 1));
            if (kind < 7 && n) { id = ids[rng() % ulng(n)]; xs[j] = alpha[id]; xid[j] = id; }
            else if (kind == 7) { xs[j] = alpha[id] + 1; xid[j] = -1; }
            else if (kind == 8) { xs[j] = alpha[0] - 1; xid[j] = -1; }
            else { xs[j] = alpha.back() + 1; xid[j] = -1; }}
        StaticRangeFrequency<int> f(a);
        want = sweepCount(ids, k, qs, xid);
        for (int j = 0; j < q; ++j) { expect(f.count(qs[j].first, qs[j].second, xs[j]), want[j], [&] { return "count" + show(qs[j]) + " x=" + show(xs[j]) + " vs sweep"; }); }
        RangeMajority<int> m(a);
        RangeThresholdFrequency<int> tf(a);
        vector<int> cnt(k);
        for (int s = 0; s < samples && n; ++s) {
            auto [l, r] = s % 5 ? qs[rng() % qs.size()] : pair<int, int>{int(rnd(rng, 0, n / 4)), int(rnd(rng, 3 * n / 4, n))};
            int len = r - l, maj = -1;
            vector<int> seen;
            for (int i = l; i < r; ++i) {
                int x = ids[i];
                if (cnt[x]++ == 0) { seen.push_back(x); }
                if (2 * cnt[x] > len) { maj = x; }}
            sort(seen.begin(), seen.end());
            int got = -7;
            expect(m.majority(l, r, got), maj >= 0, [&] { return "majority(" + show(l) + "," + show(r) + ") found vs tally"; });
            expect(got, maj >= 0 ? alpha[maj] : -7, [&] { return "majority(" + show(l) + "," + show(r) + ") value vs tally"; });
            vector<int> ts{len / 2 + 1, max(1, len / int(rnd(rng, 1, CHECKED ? 8 : 40))), int(rnd(rng, CHECKED ? max(1, len / 8) : 1, len + 1)), len + 1};
            if (len <= (CHECKED ? 64 : 4096)) { ts.push_back(1); }
            for (int t : ts) {
                vector<pair<int, int>> fw;
                for (int x : seen) {
                    if (cnt[x] >= t) { fw.push_back({alpha[x], cnt[x]}); }}
                expect(tf.frequent(l, r, t), fw, [&] { return "frequent(" + show(l) + "," + show(r) + "," + show(t) + ") vs tally"; });}
            for (int x : seen) { cnt[x] = 0; }}
        std::cout << "PASS scaleGeneric n=" << n << " q=" << q << " profile=" << profile << '\n';}

    template<typename M>
    void scaleMex(Rng &rng, int n, int q, int profile) {
        context = "scaleMex<" + typeName<M>() + "> n=" + show(n) + " q=" + show(q) + " profile=" + show(profile) + " seed=" + show(seed);
        auto a = mexArray<M>(rng, n, profile);
        vector<int> v(n);
        for (int i = 0; i < n; ++i) { v[i] = std::cmp_greater_equal(a[i], 0) && std::cmp_less_equal(a[i], n) ? int(a[i]) : -1; }
        auto qs = queries(rng, n, q);
        auto want = moMex(v, qs), got = rangeMex(a, qs);
        PersistentRangeMex p(a);
        for (size_t j = 0; j < qs.size(); ++j) {
            expect(got[j], want[j], [&] { return "rangeMex " + show(qs[j]) + " vs Mo"; });
            expect(p.mex(qs[j].first, qs[j].second), want[j], [&] { return "PersistentRangeMex.mex" + show(qs[j]) + " vs Mo"; });}
        std::cout << "PASS scaleMex<" << typeName<M>() << "> n=" << n << " q=" << q << " profile=" << profile << '\n';}

    void invalid(const string &name) {
        vector<int> a{1, 2, 1};
        StaticRangeFrequency<int> f(a);
        PersistentRangeMex p(a);
        RangeMajority<int> m(a);
        RangeThresholdFrequency<int> t(a);
        int res = 0;
        if (name == "distinct-reversed") { (void)staticRangeCountDistinct(a, {{2, 1}}); }
        else if (name == "distinct-right") { (void)staticRangeCountDistinct(a, {{0, 4}}); }
        else if (name == "distinct-negative") { (void)staticRangeCountDistinct(a, {{-1, 1}}); }
        else if (name == "distinct-empty-right") { (void)staticRangeCountDistinct(vector<int>{}, {{0, 1}}); }
        else if (name == "frequency-reversed") { (void)f.count(2, 1, 1); }
        else if (name == "frequency-right") { (void)f.count(0, 4, 1); }
        else if (name == "frequency-negative") { (void)f.count(-1, 1, 1); }
        else if (name == "frequency-default-right") { StaticRangeFrequency<int> g; (void)g.count(0, 1, 1); }
        else if (name == "mex-reversed") { (void)rangeMex(a, {{2, 1}}); }
        else if (name == "mex-right") { (void)rangeMex(a, {{0, 4}}); }
        else if (name == "mex-negative") { (void)rangeMex(a, {{-1, 0}}); }
        else if (name == "persistent-reversed") { (void)p.mex(2, 1); }
        else if (name == "persistent-right") { (void)p.mex(0, 4); }
        else if (name == "persistent-negative") { (void)p.mex(-1, 1); }
        else if (name == "persistent-default-right") { PersistentRangeMex g; (void)g.mex(0, 1); }
        else if (name == "majority-reversed") { (void)m.majority(2, 1, res); }
        else if (name == "majority-right") { (void)m.majority(0, 4, res); }
        else if (name == "majority-negative") { (void)m.majority(-1, 2, res); }
        else if (name == "frequent-reversed") { (void)t.frequent(2, 1, 1); }
        else if (name == "frequent-right") { (void)t.frequent(0, 4, 1); }
        else if (name == "frequent-negative") { (void)t.frequent(-1, 1, 1); }
        else if (name == "frequent-zero-threshold") { (void)t.frequent(0, 3, 0); }
        else if (name == "frequent-zero-threshold-empty") { (void)t.frequent(1, 1, 0); }
        else if (name == "frequent-negative-threshold") { (void)t.frequent(0, 3, -1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--rounds" && i + 1 < argc) { roundsOverride = std::stoi(argv[++i]); }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "static_range_queries seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        bool quick = mode == "quick", stress = mode == "stress";
        hand(); edges();
        int en = quick ? 4 : stress ? 7 : 6, mn = quick ? 4 : stress ? 6 : 5;
        exhaustive<int>({INT_MIN, -3, 0, 2, 5, 9, INT_MAX}, en);
        exhaustive<string>({"", "a", "ab", "b", "ba", "c", "z"}, en);
        exhaustive<Key>({Key{-9}, Key{-5}, Key{0}, Key{1}, Key{2}, Key{8}, Key{INT_MAX}}, en);
        exhaustive<Pt>({Pt{0, 0}, Pt{0, 1}, Pt{0, 2}, Pt{1, -5}, Pt{1, 1}, Pt{2, 0}, Pt{5, 5}}, en);
        mexExhaustive<int>({-1, 0, 1, 2, 3, 4}, mn);
        mexExhaustive<lng>({LLONG_MIN, 0, 1, 2, 3, (1LL << 32) + 1}, mn);
        mexExhaustive<uint>({0, 1, 2, 3, 4, UINT_MAX}, mn);
        mexExhaustive<ulng>({0, 1, 2, 3, 1ULL << 32, ULLONG_MAX}, mn);
        mexExhaustive<int8_t>({-128, -1, 0, 1, 2, 127}, mn);
        int gr = rounds(10, 60, 400), mr = rounds(10, 100, 600), gn = quick ? 24 : stress ? 160 : 64;
        randomGeneric<int>(rng, gr, gn); randomGeneric<lng>(rng, gr, gn); randomGeneric<string>(rng, gr, gn);
        randomGeneric<Key>(rng, gr, gn); randomGeneric<Pt>(rng, gr, gn);
        randomMex<int>(rng, mr, gn); randomMex<lng>(rng, mr, gn); randomMex<uint>(rng, mr, gn); randomMex<ulng>(rng, mr, gn);
        int big = quick ? 1000 : CHECKED ? 3000 : stress && !SLOW ? 500000 : 200000, bq = stress && !SLOW ? 100000 : big;
        for (int profile = 0; profile < 4; ++profile) { scaleGeneric(rng, big, bq, profile, quick ? 100 : CHECKED ? 200 : 600); }
        for (int profile : {0, 1, 3, 4}) { scaleMex<int>(rng, big, bq, profile); }
        scaleMex<lng>(rng, big, bq, 3); scaleMex<ulng>(rng, big, bq, 3); scaleMex<uint>(rng, big, bq, 2);
        std::cout << "PASS static_range_queries checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL static_range_queries seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
