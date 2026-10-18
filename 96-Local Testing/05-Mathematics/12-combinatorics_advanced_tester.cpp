#include "../../05-Mathematics/12-combinatorics_advanced.hpp"

string context;
lng checks = 0;
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string str(std::initializer_list<lng> v) {
    string s;
    for (lng x : v) { s += std::to_string(x) + " "; }
    return s;}
using P2 = ModInt<1000003>;

// Pascal's triangle modulo any m >= 1 by additions only.
vector<vector<ulng>> pascal(int n, ulng m) {
    vector<vector<ulng>> c(n + 1);
    for (int i = 0; i <= n; ++i) {
        c[i].assign(i + 1, 1 % m);
        for (int j = 1; j < i; ++j) { c[i][j] = ulng((ulll(c[i - 1][j - 1]) + c[i - 1][j]) % m); }}
    return c;}
ulng at(const vector<vector<ulng>> &c, lng n, lng k) { return k < 0 || k > n ? 0 : c[n][k]; }
template<typename M> vector<vector<M>> stirling1Dp(int n) {
    vector<vector<M>> s(n + 1, vector<M>(n + 2, 0));
    s[0][0] = 1;
    for (int i = 1; i <= n; ++i) { for (int k = 1; k <= i; ++k) { s[i][k] = s[i - 1][k - 1] - M(i - 1) * s[i - 1][k]; } }
    return s;}
template<typename M> vector<vector<M>> stirling2Dp(int n) {
    vector<vector<M>> s(n + 1, vector<M>(n + 2, 0));
    s[0][0] = 1;
    for (int i = 1; i <= n; ++i) { for (int k = 1; k <= i; ++k) { s[i][k] = s[i - 1][k - 1] + M(k) * s[i - 1][k]; } }
    return s;}

void valuations(std::mt19937_64 &rng, bool quick) {
    for (ulng p : {2, 3, 5, 7, 11, 13, 97}) {
        ulng v = 0;
        for (ulng n = 0; n <= (quick ? 600u : 3000u); ++n) {
            for (ulng x = n; x && x % p == 0; x /= p) { ++v; }
            context = str({lng(n), lng(p)});
            need(factorialValuation(n, p) == v, "factorialValuation vs sum of v_p(i)");}
        auto c = pascal(quick ? 60 : 200, ~ulng(0));
        for (int n = 0; n <= (quick ? 60 : 200); ++n) {
            for (int k = 0; k <= n; ++k) {
                context = str({n, k, lng(p)});
                int carries = kummerCarries(ulng(n), ulng(k), p);
                need(lng(factorialValuation(n, p) - factorialValuation(k, p) - factorialValuation(n - k, p)) == carries, "kummerCarries vs Legendre");}}}
    for (int r = 0; r < 2000; ++r) {
        ulng n = rng(), k = rng() % (n + 1), p = 2 + rng() % 1000;
        context = str({lng(n), lng(k), lng(p)});
        ulng d = 0;
        for (ulng a = n, b = k, c = n - k; a || b || c; a /= p, b /= p, c /= p) { d += b % p + c % p - a % p; }
        need(ulng(kummerCarries(n, k, p)) * (p - 1) == d, "kummerCarries vs digit sums");}
    context = "carry width"; need(kummerCarries(~ulng(0), ulng(1) << 63, ~ulng(0)) == 1, "kummerCarries 128-bit digit sum");
    cout << "PASS valuations and carries checks=" << checks << '\n';}

void binomials(std::mt19937_64 &rng, bool quick) {
    int N = quick ? 120 : 300;
    // Prime moduli: Lucas, the p-free table and Granville with e = 1.
    for (uint p : {2u, 3u, 5u, 7u, 13u, 101u}) {
        DynModInt<1>::setMod(p);
        PrimeCombinatorics<DynModInt<1>> pc(int(p) - 1);
        BinomialSmallPrimeTable<DynModInt<1>> st(N);
        BinomialPrimePower bp(p, 1);
        auto c = pascal(N, p);
        for (int n = 0; n <= N; ++n) {
            for (int k = -2; k <= n + 2; ++k) {
                context = str({n, k, p});
                ulng want = at(c, n, k);
                need(st(n, k).val() == want, "BinomialSmallPrimeTable");
                if (k >= 0) { need(binomialLucas(ulng(n), ulng(k), pc).val() == want && bp(ulng(n), ulng(k)) == want, "binomialLucas, BinomialPrimePower e = 1"); }}}}
    for (auto [p, e] : vector<pair<uint, uint>>{{2, 1}, {2, 3}, {2, 10}, {3, 4}, {5, 3}, {7, 2}, {31, 2}, {2, 20}}) {
        BinomialPrimePower bp(p, e);
        auto c = pascal(N, bp.q);
        for (int n = 0; n <= N; ++n) {
            for (int k = 0; k <= n + 1; ++k) { context = str({n, k, p, e}); need(bp(ulng(n), ulng(k)) == at(c, n, k), "BinomialPrimePower"); }}}
    for (uint m : {1u, 2u, 12u, 360u, 1000000u, 999999u, 3145728u, 223092870u, 2147483646u}) {
        BinomialCompositeMod bc(m);
        auto c = pascal(quick ? 60 : 160, m);
        for (int n = 0; n <= (quick ? 60 : 160); ++n) {
            for (int k = 0; k <= n + 1; ++k) { context = str({n, k, m}); need(bc(ulng(n), ulng(k)) == at(c, n, k), "BinomialCompositeMod"); }}}
    for (int r = 0; r < (quick ? 8 : 40); ++r) {
        ulng m = r % 4 == 0 ? (ulng(1) << 63) - 1 - rng() % 1000 : r % 4 == 1 ? 1 + rng() % 1000 : max<ulng>(1, rng() >> (1 + rng() % 63));
        int n = int(rng() % ulng(N + 1));
        auto c = pascal(n, m);
        auto row = binomialRowCompositeMod(n, m);
        context = "n=" + std::to_string(n) + " m=" + std::to_string(m);
        need(row == c[n], "binomialRowCompositeMod vs Pascal");}
    cout << "PASS binomials modulo primes, prime powers and composites vs Pascal checks=" << checks << '\n';}

void prefixSums(std::mt19937_64 &rng, bool quick) {
    int N = quick ? 80 : 250;
    ModFac pc(N);
    auto c = pascal(N, mint::mod());
    for (mint r : {mint(1), mint(2), mint(-1), mint(0), mint(rng())}) {
        for (int n = 0; n <= N; ++n) {
            mint s = 0, rp = 1;
            for (int k = -1; k <= n + 1; ++k) {
                if (k >= 0 && k <= n) { s += rp * mint(c[n][k]); rp *= r; }
                context = str({n, k, lng(r.val())});
                need(binomialPrefixSum(ulng(n), k, pc, r) == (k < 0 ? mint(0) : s), "binomialPrefixSum");}}}
    for (int t = 0; t < (quick ? 5 : 30); ++t) {
        int q = 1 + int(rng() % 300);
        mint r = t % 3 == 0 ? mint(1) : t % 3 == 1 ? mint(rng()) : mint(5);
        vector<pair<int, int>> qs(q);
        for (auto &[n, k] : qs) { n = int(rng() % ulng(N + 1)); k = int(rng() % ulng(N + 3)) - 1; }
        auto res = binomialPrefixSumQueries(qs, pc, r);
        for (int i = 0; i < q; ++i) {
            context = str({qs[i].first, qs[i].second, t});
            need(res[i] == binomialPrefixSum(ulng(qs[i].first), qs[i].second, pc, r), "binomialPrefixSumQueries");}}
    for (int B : {0, 1, 3, 17, 1000}) {
        BinomialPrefixSumTable<mint> tb(N, B);
        for (int n = 0; n <= N; n += 1 + n / 50) {
            for (lng k = -1; k <= N + 2; ++k) { context = str({n, k, B}); need(tb.sum(n, k) == binomialPrefixSum(ulng(n), k, pc), "BinomialPrefixSumTable"); }}}
    cout << "PASS prefix sums vs Pascal, Mo's queries and the block table checks=" << checks << '\n';}

void factorialPowers(std::mt19937_64 &rng) {
    auto c = pascal(200, mint::mod());
    ModFac pc(200);
    for (int x = 0; x <= 100; ++x) {
        for (int n = 0; n <= 100; ++n) {
            context = str({x, n});
            mint fall = x >= n ? mint(c[x][n]) * pc.factorial(n) : mint(0);
            need(fallingFactorial(mint(x), n) == fall, "fallingFactorial vs C(x, n) n!");
            need(risingFactorial(mint(x), n) == (x + n - 1 >= n || n == 0 ? fallingFactorial(mint(x + n - 1), n) : mint(0)), "risingFactorial");}}
    for (int r = 0; r < 200; ++r) {
        ulng x = rng(); lng n = lng(rng() % 300);
        ulng want = 1, wr = 1;
        for (lng i = 0; i < n; ++i) { want *= x - ulng(i); wr *= x + ulng(i); }
        context = str({lng(x), n});
        need(fallingFactorial(x, n) == want && risingFactorial(x, n) == wr, "factorial powers modulo 2^64");}
    cout << "PASS rising and falling factorials checks=" << checks << '\n';}

template<typename M>
void stirlingTables(int N) {
    auto s1 = stirling1Dp<M>(N), s2 = stirling2Dp<M>(N);
    for (int n = 0; n <= N; n += 1 + n / 40) {
        auto a = stirling1Row<M>(n);
        context = str({n, lng(M::mod())});
        need(int(a.size()) == n + 1 && std::equal(a.begin(), a.end(), s1[n].begin()), "stirling1Row");
        if (M::is_prime && ulng(n) < M::mod()) {
            auto b = stirling2Row<M>(n);
            need(int(b.size()) == n + 1 && std::equal(b.begin(), b.end(), s2[n].begin()), "stirling2Row");}}
    if (!M::is_prime) { return; }
    for (int k = 0; k <= N + 1; k += 1 + k / 30) {
        auto a = stirling1Column<M>(N, k), b = stirling2Column<M>(N, k);
        for (int n = 0; n <= N; ++n) {
            context = str({n, k, lng(M::mod())});
            need(a[n] == (k <= n ? s1[n][k] : M(0)) && b[n] == (k <= n ? s2[n][k] : M(0)), "stirling columns");}}}

void stirlings(std::mt19937_64 &rng, bool quick) {
    stirlingTables<mint>(quick ? 60 : 220);
    stirlingTables<P2>(quick ? 40 : 120);
    stirlingTables<ModInt<1000000>>(quick ? 30 : 80);
    for (uint p : {2u, 3u, 5u, 7u, 11u, 13u, 101u}) {
        DynModInt<2>::setMod(p);
        StirlingSmallPrime sp(p);
        int N = quick ? 120 : 400;
        auto s1 = stirling1Dp<DynModInt<2>>(N), s2 = stirling2Dp<DynModInt<2>>(N);
        for (int n = 0; n <= N; ++n) {
            for (int k = 0; k <= n + 2; ++k) {
                context = str({n, k, p});
                need(sp.stirling1(ulng(n), ulng(k)) == (k <= n ? s1[n][k].val() : 0) && sp.stirling2(ulng(n), ulng(k)) == (k <= n ? s2[n][k].val() : 0), "StirlingSmallPrime vs DP");}}
        // Huge n with k < p: S(n, k) = sum (-1)^(k - i) C(k, i) i^n / k!; also n within 3p of 2^64.
        for (int r = 0; r < 300; ++r) {
            ulng n = r % 3 ? rng() : ~ulng(0) - rng() % (3 * p), k = rng() % p;
            ulng s = 0, fk = 1;
            for (ulng i = 1; i <= k; ++i) { fk = fk * i % p; }
            auto c = pascal(int(p), p);
            for (ulng i = 0; i <= k; ++i) { s = (s + ((k - i) % 2 ? p - 1 : 1) * c[k][i] % p * modPow64(i, n, p)) % p; }
            context = str({lng(n), lng(k), p});
            need(sp.stirling2(n, k) == s * modPow64(fk, p - 2, p) % p, "StirlingSmallPrime huge n");
            // s(n, n) = 1 and s(n, n - 1) = -C(n, 2).
            ulng c2 = ulng((ulll(n) * (n - 1) / 2) % p);
            need(sp.stirling1(n, n) == 1 % p && sp.stirling1(n, n - 1) == (p - c2) % p, "StirlingSmallPrime first kind at 64-bit n");}}
    cout << "PASS Stirling rows, columns and small-prime queries vs DP checks=" << checks << '\n';}

void bellEulerian(std::mt19937_64 &rng, bool quick) {
    int N = quick ? 100 : 500;
    vector<mint> bell(N + 1);
    vector<mint> row{1};
    bell[0] = 1;
    for (int n = 1; n <= N; ++n) {
        vector<mint> nx{row.back()};
        for (mint x : row) { nx.pb(nx.back() + x); }
        row = nx; bell[n] = row[0];}
    context = "bellTable";
    need(bellTable(N) == bell, "bellTable vs Bell triangle");
    for (uint p : {2u, 3u, 5u, 7u, 13u, 101u}) {
        DynModInt<3>::setMod(p);
        int M = quick ? 300 : 1500;
        vector<DynModInt<3>> b(M + 1), r{1};
        b[0] = 1;
        for (int n = 1; n <= M; ++n) {
            vector<DynModInt<3>> nx{r.back()};
            for (auto x : r) { nx.pb(nx.back() + x); }
            r = nx; b[n] = r[0];}
        for (int n = 0; n <= M; ++n) { context = str({n, p}); need(bellNumberLarge<DynModInt<3>>(ulng(n)) == b[n], "bellNumberLarge vs triangle"); }
        ulng pm = 1;
        for (int m = 1; pm <= ~ulng(0) / p; ++m) { pm *= p; context = str({lng(pm), p}); need(bellNumberLarge<DynModInt<3>>(pm) == DynModInt<3>(m + 1), "B_(p^m) = m + 1 mod p"); }
        if (p <= 5) {
            ulng period = 0;
            for (ulng i = 0, t = 1; i < p; ++i, t *= p) { period += t; }
            for (int t = 0; t < 30; ++t) {
                ulng n = rng() >> 2;
                context = str({lng(n), p});
                need(bellNumberLarge<DynModInt<3>>(n) == bellNumberLarge<DynModInt<3>>(n + period), "Bell period (p^p - 1) / (p - 1)");}}}
    vector<vector<mint>> A(N + 1, vector<mint>(N + 1, 0));
    A[0][0] = 1;
    for (int n = 1; n <= N; ++n) { for (int k = 0; k < n; ++k) { A[n][k] = mint(k + 1) * A[n - 1][k] + (k ? mint(n - k) * A[n - 1][k - 1] : mint(0)); } }
    for (int n = 0; n <= N; n += 1 + n / 30) {
        auto e = eulerianRow(n);
        context = str({n});
        need(int(e.size()) == max(n, 1) && std::equal(e.begin(), e.end(), A[n].begin()), "eulerianRow");
        for (int k = -1; k <= n + 1; ++k) { context = str({n, k}); need(eulerianNumber(ulng(n), k) == (k >= 0 && k <= n ? A[n][k] : mint(0)), "eulerianNumber"); }}
    for (int r = 0; r < 300; ++r) {
        ulng n = rng() | 4;
        context = str({lng(n)});
        mint two = pow(mint(2), n), three = pow(mint(3), n), nn(n);
        need(eulerianNumber(n, 0) == 1 && eulerianNumber(n, 1) == two - nn - 1, "eulerian closed forms k <= 1");
        need(eulerianNumber(n, 2) == three - (nn + 1) * two + nn * (nn + 1) / 2, "eulerian closed form k = 2");
        if (n - 1 <= ulng(std::numeric_limits<lng>::max())) { need(eulerianNumber(n, lng(n - 1)) == 1 && eulerianNumber(n, lng(n - 2)) == two - nn - 1, "eulerian symmetry"); }}
    cout << "PASS Bell and Eulerian vs triangles, congruences and closed forms checks=" << checks << '\n';}

void smallFamilies(bool quick) {
    // Entringer: down-up permutations of [n + 1] starting with k + 1.
    int E = quick ? 6 : 8;
    auto ent = entringerTable<lng>(E);
    for (int n = 0; n <= E; ++n) {
        vector<int> p(n + 1);
        iota(p.begin(), p.end(), 1);
        vector<lng> cnt(n + 1, 0);
        do {
            bool ok = true;
            for (int i = 0; i + 1 <= n && ok; ++i) { ok = i % 2 ? p[i] < p[i + 1] : p[i] > p[i + 1]; }
            if (ok) { ++cnt[p[0] - 1]; }} while (std::next_permutation(p.begin(), p.end()));
        if (n == 0) { cnt[0] = 1; }
        context = str({n});
        need(ent[n] == cnt, "entringerTable vs alternating permutations");}
    ModFac pc(400);
    vector<vector<mint>> L(201, vector<mint>(202, 0));
    L[0][0] = 1;
    for (int n = 0; n < 200; ++n) { for (int k = 1; k <= n + 1; ++k) { L[n + 1][k] = mint(n + k) * L[n][k] + L[n][k - 1]; } }
    for (int n = 0; n <= 200; ++n) { for (int k = -1; k <= n + 1; ++k) { context = str({n, k}); need(lahNumber(n, k, pc) == (k >= 0 && k <= n ? L[n][k] : mint(0)), "lahNumber"); } }
    // Surjections k (surj(n - 1, k) + surj(n - 1, k - 1)), bounded tuples by polynomial powers.
    vector<vector<mint>> S(201, vector<mint>(201, 0));
    S[0][0] = 1;
    for (int n = 1; n <= 200; ++n) { for (int k = 1; k <= n; ++k) { S[n][k] = mint(k) * (S[n - 1][k] + S[n - 1][k - 1]); } }
    for (int n = 0; n <= 200; n += 1 + n / 20) { for (int k = 0; k <= 200; k += 1 + k / 15) { context = str({n, k}); need(surjectionCount(ulng(n), k, pc) == S[n][k], "surjectionCount"); } }
    for (int c = 0; c <= 6; ++c) {
        vector<mint> poly{1}, unit(c + 1, 1);
        for (int k = 0; k <= 12; ++k) {
            for (int n = 0; n <= 40; ++n) { context = str({n, k, c}); need(boundedStarsBars(n, k, c, pc) == (n < int(poly.size()) ? poly[n] : mint(0)), "boundedStarsBars"); }
            poly = convolutionNaive(poly, unit);}}
    cout << "PASS Entringer, Lah, surjections and bounded tuples checks=" << checks << '\n';}

void partitions(bool quick) {
    int N = quick ? 300 : 2000;
    vector<mint> all(N + 1, 0), distinct(N + 1, 0);
    all[0] = distinct[0] = 1;
    for (int part = 1; part <= N; ++part) {
        for (int s = N; s >= part; --s) { distinct[s] += distinct[s - part]; }
        for (int s = part; s <= N; ++s) { all[s] += all[s - part]; }}
    context = "partitions";
    need(partitionCount(N) == all && partitionsDistinct(N) == distinct, "partitionCount, partitionsDistinct vs coin DP");
    auto pt = partitionTable<lng>(300);
    auto pu = partitionTable<ulng>(N);
    for (int n = 0; n <= 300; ++n) { need(mint(pt[n]) == all[n], "partitionTable lng"); }
    vector<ulng> wrap(N + 1, 0);
    wrap[0] = 1;
    for (int part = 1; part <= N; ++part) { for (int s = part; s <= N; ++s) { wrap[s] += wrap[s - part]; } }
    need(pu == wrap, "partitionTable modulo 2^64");
    for (int k : {0, 1, 2, 3, 7, 50, N, N + 5}) {
        vector<mint> b(N + 1, 0);
        b[0] = 1;
        for (int part = 1; part <= min(k, N); ++part) { for (int s = part; s <= N; ++s) { b[s] += b[s - part]; } }
        context = str({k});
        need(partitionsBoundedParts(N, k) == b, "partitionsBoundedParts");}
    // Enumeration in reverse lexicographic order against a recursive generator.
    for (int n = 0; n <= (quick ? 18 : 30); ++n) {
        vector<vector<int>> want, got;
        vector<int> cur;
        auto gen = [&](auto &&self, int left, int mx) -> void {
            if (left == 0) { want.pb(cur); return; }
            for (int x = min(left, mx); x >= 1; --x) { cur.pb(x); self(self, left - x, x); cur.pop_back(); }};
        gen(gen, n, n);
        enumeratePartitions(n, [&](std::span<const int> s) { got.emplace_back(s.begin(), s.end()); });
        context = str({n});
        need(got == want, "enumeratePartitions order and content");}
    lng cnt = 0;
    enumeratePartitions(quick ? 40 : 60, [&](std::span<const int>) { ++cnt; });
    need(mint(cnt) == all[quick ? 40 : 60], "enumeratePartitions count");
    cout << "PASS partitions vs coin DP and a recursive generator checks=" << checks << '\n';}

void qAnalogues(std::mt19937_64 &rng, bool quick) {
    for (uint p : {2u, 3u, 5u, 7u, 11u, 13u, 17u, 101u}) {
        DynModInt<4>::setMod(p);
        using D = DynModInt<4>;
        for (uint q = 0; q < p; q += p > 20 ? 7 : 1) {
            int N = int(p) - 1;
            QCombinatorics<D> qc(D(q), N);
            vector<vector<D>> c(N + 1, vector<D>(N + 2, 0));
            vector<D> qp(N + 2, 1);
            for (int i = 1; i <= N + 1; ++i) { qp[i] = qp[i - 1] * D(q); }
            for (int n = 0; n <= N; ++n) {
                c[n][0] = 1;
                for (int k = 1; k <= n; ++k) { c[n][k] = c[n - 1][k - 1] + qp[k] * c[n - 1][k]; }}
            D f = 1, s = 0;
            for (int n = 0; n <= N; ++n) {
                if (n) { s = s + qp[n - 1]; f *= s; }
                context = str({n, q, p});
                need(qc.qFactorial(n) == f, "qFactorial");
                for (int k = -1; k <= n + 1; ++k) { need(qc.qBinomial(n, k) == (k >= 0 && k <= n ? c[n][k] : D(0)), "qBinomial vs q-Pascal"); }}}}
    // reset on a live object: a larger table first, then a smaller one with another q.
    DynModInt<4>::setMod(13);
    QCombinatorics<DynModInt<4>> live(DynModInt<4>(3), 12);
    for (uint q : {5u, 1u, 0u, 12u}) {
        int N = int(q % 7) + 3;
        live.reset(DynModInt<4>(q), N);
        vector<vector<DynModInt<4>>> c(N + 1, vector<DynModInt<4>>(N + 1, 0));
        for (int n = 0; n <= N; ++n) {
            c[n][0] = 1;
            DynModInt<4> qk = 1;
            for (int k = 1; k <= n; ++k) { qk *= DynModInt<4>(q); c[n][k] = c[n - 1][k - 1] + qk * (k < n ? c[n - 1][k] : DynModInt<4>(0)); }}
        for (int n = 0; n <= N; ++n) { for (int k = 0; k <= n; ++k) { context = str({n, k, q}); need(live.qBinomial(n, k) == c[n][k], "QCombinatorics reset reuse"); } }}
    for (int t = 0; t < (quick ? 3 : 12); ++t) {
        mint q = t == 0 ? mint(1) : t == 1 ? mint(0) : t == 2 ? mint(-1) : mint(rng());
        int N = quick ? 150 : 600;
        QCombinatorics<mint> qc(q, N);
        vector<mint> prev{1};
        for (int n = 1; n <= N; ++n) {
            vector<mint> cur(n + 1, 1);
            mint qk = 1;
            for (int k = 1; k < n; ++k) { qk *= q; cur[k] = prev[k - 1] + qk * prev[k]; }
            prev = cur;
            if (n % 37 == 0 || n == N) { for (int k = 0; k <= n; ++k) { context = str({n, k, t}); need(qc.qBinomial(n, k) == cur[k], "qBinomial large prime"); } }}}
    cout << "PASS q-analogues vs q-Pascal for every q modulo small primes checks=" << checks << '\n';}

void transforms(std::mt19937_64 &rng, bool quick) {
    int N = quick ? 60 : 250;
    auto c = pascal(N, mint::mod());
    auto s1 = stirling1Dp<mint>(N), s2 = stirling2Dp<mint>(N);
    for (int n : {0, 1, 2, 3, 5, 17, N}) {
        vector<mint> a(n);
        for (auto &x : a) { x = mint(rng()); }
        vector<mint> bt(n, 0), ib(n, 0), st(n, 0), is(n, 0);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j <= i; ++j) {
                bt[i] += mint(c[i][j]) * a[j];
                ib[i] += (i - j) % 2 ? -mint(c[i][j]) * a[j] : mint(c[i][j]) * a[j];
                st[i] += s2[i][j] * a[j];
                is[i] += s1[i][j] * a[j];}}
        context = str({n});
        need(binomialTransform(a) == bt && inverseBinomialTransform(a) == ib, "binomial transforms vs direct");
        need(stirlingTransform(a) == st && inverseStirlingTransform(a) == is, "Stirling transforms vs direct");
        need(inverseStirlingTransform(stirlingTransform(a)) == a && inverseBinomialTransform(binomialTransform(a)) == a, "transform round trips");}
    cout << "PASS binomial and Stirling transforms vs direct sums checks=" << checks << '\n';}

void searches(std::mt19937_64 &rng, bool quick) {
    // Count n <= X avoiding every divisor in the list: pruned search over lcm vs a direct scan.
    for (int r = 0; r < (quick ? 50 : 400); ++r) {
        int k = int(rng() % 9);
        lng X = 1 + lng(rng() % 20000);
        vector<lng> d(k);
        for (auto &x : d) { x = 2 + lng(rng() % (r % 2 ? 30 : 3000)); }
        lng want = 0;
        for (lng v = 1; v <= X; ++v) { want += std::none_of(d.begin(), d.end(), [&](lng x) { return v % x == 0; }); }
        lng visited = 0;
        lng got = inclusionExclusion<lng>(k, lng(1), [&](lng acc, int i, lng &nx) { nx = std::lcm(acc, d[i]); return nx <= X; },
                                          [&](lng l) { ++visited; return X / l; });
        lng full = inclusionExclusion<lng>(k, lng(1), [&](lng acc, int i, lng &nx) { nx = min<lng>(std::lcm(acc, d[i]), X + 1); return true; }, [&](lng l) { return X / l; });
        context = "k=" + std::to_string(k) + " X=" + std::to_string(X);
        need(got == want && full == want && visited <= (lng(1) << k), "inclusionExclusion vs scan, pruned and unpruned");}
    int T = quick ? 300 : 2000;
    for (int r = 0; r < (quick ? 10 : 40); ++r) {
        int n = int(rng() % 60);
        vector<int> s(n);
        for (auto &x : s) { x = r % 3 == 0 ? int(rng() % 4) : int(rng() % ulng(T + 50)); }
        vector<mint> dp(T + 1, 0);
        dp[0] = 1;
        for (int x : s) { for (int v = T; v >= x; --v) { dp[v] += dp[v - x]; } }
        context = "n=" + std::to_string(n);
        need(subsetSumCount(s, T) == dp, "subsetSumCount vs knapsack");}
    cout << "PASS pruned inclusion-exclusion and subset sums checks=" << checks << '\n';}

void fibonacci(std::mt19937_64 &rng, bool quick) {
    for (ulng m = 1; m <= (quick ? 2000u : 20000u); ++m) {
        ulng a = 0, b = 1 % m, n = 0;
        do { ulng c = (a + b) % m; a = b; b = c; ++n; } while (a != 0 || b != 1 % m);
        context = str({lng(m)});
        need(pisanoPeriod(m) == n, "pisanoPeriod vs simulation");}
    for (int r = 0; r < 3000; ++r) {
        ulng n = r < 1000 ? ulng(r) : r % 3 ? rng() : ~ulng(0) - rng() % 100;
        auto z = zeckendorf(n);
        array<ulng, 94> f{0, 1};
        for (int i = 2; i < 94; ++i) { f[i] = f[i - 1] + f[i - 2]; }
        ulng s = 0;
        bool ok = true;
        for (int i = 0; i < int(z.size()); ++i) {
            ok = ok && z[i] >= 2 && z[i] <= 93 && (i == 0 || z[i - 1] >= z[i] + 2);
            s += f[z[i]];}
        context = str({lng(n)});
        need(ok && s == n, "zeckendorf: valid and sums to n");}
    vector<mint> fac(401, 1), want(401, 0);
    for (int i = 1; i <= 400; ++i) { fac[i] = fac[i - 1] * i; }
    for (int n = 1; n <= 400; ++n) {
        want[n] = fac[n];
        for (int k = 1; k < n; ++k) { want[n] -= want[k] * fac[n - k]; }}
    context = "indecomposable";
    need(indecomposablePermutations(400) == want, "indecomposablePermutations vs first-block recurrence");
    for (int n = 1; n <= 8; ++n) {
        vector<int> p(n);
        iota(p.begin(), p.end(), 0);
        lng cnt = 0;
        do {
            int mx = -1;
            bool ok = true;
            for (int i = 0; i + 1 < n && ok; ++i) { mx = max(mx, p[i]); ok = mx != i; }
            cnt += ok;} while (std::next_permutation(p.begin(), p.end()));
        context = str({n});
        need(indecomposablePermutations(n)[n] == mint(cnt), "indecomposablePermutations vs enumeration");}
    cout << "PASS Pisano, Zeckendorf and indecomposable permutations checks=" << checks << '\n';}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261019;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            string op;
            while (cin >> op) {
                if (op == "binom") {
                    ulng n, k; uint p, e, m; cin >> n >> k >> p >> e >> m;
                    DynModInt<5>::setMod(p);
                    cout << binomialLucas(n, k, PrimeCombinatorics<DynModInt<5>>(int(p) - 1)) << ' ' << BinomialPrimePower(p, e)(n, k) << ' ' << BinomialCompositeMod(m)(n, k) << '\n';}
                else if (op == "row") {
                    int n; ulng m; cin >> n >> m;
                    auto row = binomialRowCompositeMod(n, m);
                    ulng h = 0;
                    for (ulng x : row) { h = h * 1000003 + x; }
                    cout << h << '\n';}
                else if (op == "prefix") {
                    ulng n; lng k; ulng r; cin >> n >> k >> r;
                    cout << binomialPrefixSum(n, k, ModFac(200), mint(r)) << '\n';}
                else if (op == "val") {
                    ulng n, k, p; cin >> n >> k >> p;
                    cout << factorialValuation(n, p) << ' ' << kummerCarries(n, k, p) << '\n';}
                else if (op == "pisano") { ulng m; cin >> m; cout << pisanoPeriod(m) << '\n'; }
                else if (op == "eulerian") { ulng n; lng k; cin >> n >> k; cout << eulerianNumber(n, k) << '\n'; }
                else if (op == "surj") { ulng n; int k; cin >> n >> k; cout << surjectionCount(n, k, ModFac(200)) << '\n'; }}
            return 0;}}
    if (!invalid.empty()) {
        ModFac pc(10);
        if (invalid == "valuation-p") { factorialValuation(10, 1); }
        else if (invalid == "carries-k") { kummerCarries(3, 4, 2); }
        else if (invalid == "lucas-digit") { binomialLucas(100, 3, pc); }
        else if (invalid == "small-table-n") { BinomialSmallPrimeTable<ModInt<7>>(5)(6, 1); }
        else if (invalid == "prime-power-q") { BinomialPrimePower(2, 31); }
        else if (invalid == "prime-power-composite") { BinomialPrimePower(4, 1); }
        else if (invalid == "composite-zero") { BinomialCompositeMod(0); }
        else if (invalid == "row-m") { binomialRowCompositeMod(5, 0); }
        else if (invalid == "prefix-k") { binomialPrefixSum(100, 20, pc); }
        else if (invalid == "queries-n") { binomialPrefixSumQueries(vector<pair<int, int>>{{11, 2}}, pc); }
        else if (invalid == "table-n") { BinomialPrefixSumTable<mint>(5).sum(6, 1); }
        else if (invalid == "rising-n") { risingFactorial(mint(3), -1); }
        else if (invalid == "stirling-small-prime") { StirlingSmallPrime(9); }
        else if (invalid == "bell-composite") { DynModInt<6>::setMod(9); bellNumberLarge<DynModInt<6>>(20); }
        else if (invalid == "q-table") { QCombinatorics<ModInt<7>>(ModInt<7>(2), 7); }
        else if (invalid == "partitions-negative") { partitionTable<lng>(-1); }
        else if (invalid == "enumerate-negative") { enumeratePartitions(-1, [](std::span<const int>) {}); }
        else if (invalid == "subset-negative") { subsetSumCount(vector<int>{-1}, 5); }
        else if (invalid == "pisano-zero") { pisanoPeriod(0); }
        else if (invalid == "surjection-k") { surjectionCount(5, 11, pc); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    for (int round = 0; round < (stress ? 3 : 1); ++round) {
        valuations(rng, quick);
        binomials(rng, quick);
        prefixSums(rng, quick);
        factorialPowers(rng);
        stirlings(rng, quick);
        bellEulerian(rng, quick);
        smallFamilies(quick);
        partitions(quick);
        qAnalogues(rng, quick);
        transforms(rng, quick);
        searches(rng, quick);
        fibonacci(rng, quick);}}
