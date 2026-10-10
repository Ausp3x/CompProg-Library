#include "../../05-Mathematics/15-linear_recurrence.hpp"

lng checks = 0;
std::mt19937_64 rng;
bool quick = false, stress = false, fast = false;
void check(bool ok, const string &what) {
    ++checks;
    if (!ok) { cout << "FAIL " << what << endl; exit(1); }}
string str(lng x) { return std::to_string(x); }
template<typename M> vector<M> rnd(int n) {
    vector<M> v(n);
    for (auto &x : v) { x = M(rng()); }
    return v;}
template<typename M> vector<M> extend(const vector<M> &c, vector<M> a, int n) {
    int d = int(c.size());
    a.resize(d);
    while (int(a.size()) < n) {
        M s = 0;
        for (int j = 1; j <= d; ++j) { s += c[j - 1] * a[a.size() - j]; }
        a.pb(s);}
    a.resize(n);
    return a;}
template<typename M> M bpowS(M a, ulng e) {
    M r = 1;
    for (; e; e >>= 1, a = a * a) { if (e & 1) { r = r * a; } }
    return r;}
template<typename M> using Mat = vector<vector<M>>;
template<typename M> Mat<M> mmul(const Mat<M> &x, const Mat<M> &y) {
    int n = int(x.size()), m = int(y[0].size()), k = int(y.size());
    Mat<M> z(n, vector<M>(m));
    for (int i = 0; i < n; ++i) { for (int t = 0; t < k; ++t) { for (int j = 0; j < m; ++j) { z[i][j] += x[i][t] * y[t][j]; } } }
    return z;}
template<typename M> Mat<M> mpow(Mat<M> x, ulng k) {
    int n = int(x.size());
    Mat<M> r(n, vector<M>(n));
    for (int i = 0; i < n; ++i) { r[i][i] = 1; }
    for (; k; k >>= 1, x = mmul(x, x)) { if (k & 1) { r = mmul(r, x); } }
    return r;}
// a_k through the companion matrix on the state (a_i, ..., a_{i+d-1}).
template<typename M> M bigTerm(const vector<M> &c, const vector<M> &a, ulng k) {
    int d = int(c.size());
    if (!d) { return 0; }
    Mat<M> t(d, vector<M>(d));
    for (int i = 0; i + 1 < d; ++i) { t[i][i + 1] = 1; }
    for (int j = 1; j <= d; ++j) { t[d - 1][d - j] = c[j - 1]; }
    auto p = mpow(t, k);
    M s = 0;
    for (int j = 0; j < d; ++j) { s += p[0][j] * a[j]; }
    return s;}
// Prefix sum through the (d + 1)-dimensional state (S_i, a_i, ..., a_{i+d-1}).
template<typename M> M bigPrefix(const vector<M> &c, const vector<M> &a, ulng k) {
    int d = int(c.size());
    if (!d) { return 0; }
    Mat<M> t(d + 1, vector<M>(d + 1));
    t[0][0] = 1; t[0][1] = 1;
    for (int i = 1; i < d; ++i) { t[i][i + 1] = 1; }
    for (int j = 1; j <= d; ++j) { t[d][d + 1 - j] = c[j - 1]; }
    auto p = mpow(t, k);
    M s = 0;
    for (int j = 0; j < d; ++j) { s += p[0][j + 1] * a[j]; }
    return s;}
template<typename M> vector<M> seriesDiv(const vector<M> &p, const vector<M> &q, int n) {
    vector<M> u(n);
    M c = M(1) / q[0];
    for (int i = 0; i < n; ++i) {
        M s = i < int(p.size()) ? p[i] : M(0);
        for (int j = 1; j <= i && j < int(q.size()); ++j) { s -= q[j] * u[i - j]; }
        u[i] = s * c;}
    return u;}
template<typename M> bool valid(const vector<M> &s, const vector<M> &c) {
    int l = int(c.size());
    for (int i = l; i < int(s.size()); ++i) {
        M t = 0;
        for (int j = 1; j <= l; ++j) { t += c[j - 1] * s[i - j]; }
        if (t != s[i]) { return false; }}
    return true;}
// [x^k] P / Q for any k: polynomial part plus the order-d recurrence of the proper part.
template<typename M> M bigRational(vector<M> p, vector<M> q, ulng k) {
    while (!p.empty() && p.back() == M(0)) { p.pop_back(); }
    while (!q.empty() && q.back() == M(0)) { q.pop_back(); }
    int d = int(q.size()) - 1;
    auto [t, r] = polyDivmod(p, q);
    M res = k < t.size() ? t[k] : M(0);
    if (!d) { return res; }
    vector<M> c(d);
    for (int j = 1; j <= d; ++j) { c[j - 1] = -q[j] / q[0]; }
    return res + bigTerm(c, seriesDiv(r, q, d), k);}

template<typename M>
void exhaustiveMinimal(const string &name, int maxN) {
    int q = int(M::mod());
    for (int n = 0; n <= maxN; ++n) {
        int total = 1;
        for (int i = 0; i < n; ++i) { total *= q; }
        for (int code = 0; code < total; ++code) {
            vector<M> s(n);
            for (int i = 0, x = code; i < n; ++i, x /= q) { s[i] = M(x % q); }
            int best = -1;
            for (int l = 0; l <= n && best < 0; ++l) {
                int cnt = 1;
                for (int i = 0; i < l; ++i) { cnt *= q; }
                for (int cc = 0; cc < cnt && best < 0; ++cc) {
                    vector<M> c(l);
                    for (int i = 0, x = cc; i < l; ++i, x /= q) { c[i] = M(x % q); }
                    if (valid(s, c)) { best = l; }}}
            auto b = berlekampMassey(s), h = findLinearRecurrenceHalfGcd(s);
            vector<M> f;
            check(findLinearRecurrence(s, f) && f == b, name + " findLinearRecurrence = berlekampMassey");
            check(valid(s, b) && int(b.size()) == best, name + " berlekampMassey minimal n=" + str(n) + " code=" + str(code));
            check(valid(s, h) && int(h.size()) == best, name + " findLinearRecurrenceHalfGcd minimal n=" + str(n) + " code=" + str(code));}}
    cout << "PASS " << name << ": exhaustive minimal recurrences up to n=" << maxN << "\n";}

template<typename M>
void findSuite(const string &name) {
    vector<int> ls{0, 1, 2, 5, 30, 63, 64, 65, 100, 200};
    if (!quick) { ls.insert(ls.end(), {500, 511, 520, fast ? 1500 : 700}); }
    for (int l : ls) {
        auto c = rnd<M>(l), a = rnd<M>(l);
        if (l) { c.back() = c.back() == M(0) ? M(1) : c.back(); }
        for (int extra : {0, 1, 7}) {
            auto s = extend(c, a, 2 * l + extra);
            auto b = berlekampMassey(s), h = findLinearRecurrenceHalfGcd(s);
            check(valid(s, b) && valid(s, h) && b.size() == h.size() && int(b.size()) <= l, name + " recurrence l=" + str(l) + " extra=" + str(extra));}}
    for (int rep = 0; rep < (quick ? 50 : (stress && fast ? 4 : 1) * 300); ++rep) {
        int n = int(rng() % 300);
        auto s = rnd<M>(n);
        if (rep % 3 == 0) { for (auto &x : s) { x = M(rng() % 2); } }
        if (rep % 5 == 0) { s.assign(n, M(0)); if (n) { s[rng() % n] = 1; } }
        auto b = berlekampMassey(s), h = findLinearRecurrenceHalfGcd(s);
        check(valid(s, b) && valid(s, h) && b.size() == h.size(), name + " random sequence n=" + str(n));}
    cout << "PASS " << name << ": berlekampMassey, findLinearRecurrence, findLinearRecurrenceHalfGcd\n";}

template<typename M>
void termSuite(const string &name) {
    for (int rep = 0; rep < (quick ? 40 : (stress && fast ? 4 : 1) * 250); ++rep) {
        int d = int(rng() % (rep % 4 ? 12 : 70));
        auto c = rnd<M>(d), a = rnd<M>(d + int(rng() % 3));
        if (rep % 6 == 0) { for (auto &x : c) { x = M(rng() % 2); } }
        int n = 3 * d + 300;
        auto s = extend(c, a, n);
        vector<ulng> ks{0, ulng(max(d - 1, 0)), ulng(d), ulng(d + 1), rng() % ulng(n - 60)};
        for (ulng k : ks) {
            check(kitamasa(c, a, k) == s[k] && nthTermLinearRecurrence(c, a, k) == s[k], name + " kitamasa/nthTerm small k d=" + str(d));
            auto r = fiduccia(c, k);
            M t = 0;
            for (int i = 0; i < d; ++i) { t += r[i] * a[i]; }
            check(int(r.size()) == d && t == s[k], name + " fiduccia small k");
            M ps = 0;
            for (ulng i = 0; i < k; ++i) { ps += s[i]; }
            check(prefixSumLinearRecurrence(c, a, k) == ps, name + " prefixSumLinearRecurrence small k");
            int m = int(rng() % 40);
            check(consecutiveTerms(c, a, k, m) == vector<M>(s.begin() + k, s.begin() + k + m), name + " consecutiveTerms small k d=" + str(d));}
        auto [p, q] = rationalGeneratingFunction(c, a);
        check(int(p.size()) == d && int(q.size()) == d + 1 && q[0] == M(1) && seriesDiv(p, q, n) == s, name + " rationalGeneratingFunction");
        if (d <= (fast ? 24 : 10)) {
            for (int t = 0; t < 3; ++t) {
                ulng k = rng() >> (rng() % 64);
                M want = bigTerm(c, a, k);
                check(kitamasa(c, a, k) == want && nthTermLinearRecurrence(c, a, k) == want, name + " kitamasa/nthTerm huge k d=" + str(d));
                auto r = fiduccia(c, k);
                M x = 0;
                for (int i = 0; i < d; ++i) { x += r[i] * a[i]; }
                check(x == want, name + " fiduccia huge k");
                check(prefixSumLinearRecurrence(c, a, k) == bigPrefix(c, a, k), name + " prefixSumLinearRecurrence huge k");
                int m = int(rng() % 30);
                auto got = consecutiveTerms(c, a, k, m);
                bool ok = int(got.size()) == m;
                for (int i = 0; i < m && ok; ++i) { ok = got[i] == bigTerm(c, a, k + ulng(i)); }
                check(ok, name + " consecutiveTerms huge k d=" + str(d));}}}
    // Fiduccia remainder against repeated multiplication by x and reduction.
    for (int d : {1, 2, 5, 40, 129, 200}) {
        auto c = rnd<M>(d);
        vector<M> r(d);
        r[0] = 1;
        ulng k = quick ? 300 : 3000;
        for (ulng t = 0; t < k; ++t) {
            M top = r[d - 1];
            for (int i = d - 1; i > 0; --i) { r[i] = r[i - 1] + top * c[d - 1 - i]; }
            r[0] = top * c[d - 1];}
        check(fiduccia(c, k) == r, name + " fiduccia brute d=" + str(d));}
    check(fiduccia(vector<M>{}, 5).empty() && kitamasa(vector<M>{}, vector<M>{}, 3) == M(0) && nthTermLinearRecurrence(vector<M>{}, vector<M>{}, 9) == M(0), name + " order-0 recurrence");
    cout << "PASS " << name << ": kitamasa, fiduccia, nthTermLinearRecurrence, prefixSumLinearRecurrence, rationalGeneratingFunction, consecutiveTerms\n";}

template<typename M>
void rationalSuite(const string &name) {
    for (int rep = 0; rep < (quick ? 60 : (stress && fast ? 4 : 1) * 400); ++rep) {
        int dp = int(rng() % 30), dq = int(rng() % (rep % 5 ? 10 : 60));
        auto p = rnd<M>(dp), q = rnd<M>(dq + 1);
        if (q[0] == M(0)) { q[0] = 1; }
        if (rep % 7 == 0) { q.pb(M(0)); p.pb(M(0)); }
        int n = 2000;
        auto u = seriesDiv(p, q, n);
        for (ulng k : {ulng(0), ulng(1), ulng(dp), ulng(dq), rng() % 1900}) {
            check(bostanMori(p, q, k) == u[k], name + " bostanMori small k");
            ulng r = k + rng() % 100;
            check(sliceRationalFps(p, q, k, r) == vector<M>(u.begin() + k, u.begin() + r), name + " sliceRationalFps small l=" + str(lng(k)));}
        if (dq <= (fast ? 20 : 8)) {
            ulng k = rng() >> (rng() % 64);
            check(bostanMori(p, q, k) == bigRational(p, q, k), name + " bostanMori huge k dq=" + str(dq));
            auto sl = sliceRationalFps(p, q, k, k + 15);
            bool ok = sl.size() == 15;
            for (int i = 0; i < 15 && ok; ++i) { ok = sl[i] == bigRational(p, q, k + ulng(i)); }
            check(ok, name + " sliceRationalFps huge l dq=" + str(dq));}}
    check(sliceRationalFps(vector<M>{1, 2, 3}, vector<M>{2}, 1, 5) == vector<M>{M(1), M(3) / M(2), 0, 0} && sliceRationalFps(vector<M>{1, 2, 3}, vector<M>{2}, ulng(1) << 60, (ulng(1) << 60) + 2) == vector<M>(2), name + " sliceRationalFps constant Q");
    check(sliceRationalFps(vector<M>{}, vector<M>{1, 1}, 5, 5).empty() && bostanMori(vector<M>{}, vector<M>{3, 1}, ulng(1) << 62) == M(0), name + " zero numerator");
    cout << "PASS " << name << ": bostanMori, sliceRationalFps\n";}

template<typename M>
void interpolationSuite(const string &name) {
    vector<int> ls{0, 1, 3, 20, 100};
    if (!quick) { ls.insert(ls.end(), {511, 512, 513, fast ? 1500 : 600}); }
    int bm = polynomial_detail::cut<M>(recurrence_detail::BM);
    if (!quick && (fast || bm < 2000)) { ls.insert(ls.end(), {bm / 2 - 1, bm / 2, bm / 2 + 1}); }
    for (int l : ls) {
        auto c = rnd<M>(l), a = rnd<M>(l);
        auto s = extend(c, a, 2 * l + int(rng() % 3));
        auto far = extend(c, a, 2 * l + 5000);
        for (int t = 0; t < 4; ++t) {
            ulng k = rng() % far.size();
            check(interpolateLinearRecurrence(s, k) == far[k], name + " interpolateLinearRecurrence l=" + str(l));}
        ulng k = rng();
        if (l <= 12) { check(interpolateLinearRecurrence(s, k) == bigTerm(c, a, k), name + " interpolateLinearRecurrence huge k"); }
        else { check(interpolateLinearRecurrence(s, k) == nthTermLinearRecurrence(c, a, k), name + " interpolateLinearRecurrence huge k vs nthTerm"); }}
    for (int rep = 0; rep < (quick ? 100 : (stress && fast ? 4 : 1) * 600); ++rep) {
        int n = int(rng() % 25) + 1;
        auto f = rnd<M>(n);
        M r = rep % 10 == 0 ? M(0) : rep % 10 == 1 ? M(1) : M(rng());
        vector<M> s(n);
        auto val = [&](ulng k) {
            M x = M(k), v = 0, w = 1, base = r;
            for (int i = n - 1; i >= 0; --i) { v = v * x + f[i]; }
            for (ulng e = k; e; e >>= 1, base *= base) { if (e & 1) { w *= base; } }
            return v * w;};
        for (int i = 0; i < n; ++i) { s[i] = val(ulng(i)); }
        for (ulng k : {ulng(0), ulng(n - 1), ulng(n), ulng(M::mod()), ulng(M::mod()) + ulng(n / 2), rng()}) {
            check(interpolatePolyExp(s, r, k) == val(k), name + " interpolatePolyExp n=" + str(n) + " k=" + str(lng(k % 1000000)));}}
    {
        // n = p samples: the Lagrange weights use factorials up to (p - 1)! only.
        using S = ModInt<7>;
        vector<S> s(7);
        for (int i = 0; i < 7; ++i) { s[i] = S(i * i + 1) * bpowS(S(3), ulng(i)); }
        for (ulng k : {ulng(7), ulng(50), ulng(1) << 62}) { check(interpolatePolyExp(s, S(3), k) == S(lng((k % 7) * (k % 7) + 1)) * bpowS(S(3), k), "interpolatePolyExp n = p = 7 k=" + str(lng(k % 1000))); }}
    cout << "PASS " << name << ": interpolateLinearRecurrence, interpolatePolyExp\n";}

void periodicSuite() {
    // Exhaustive: binary sequences with preperiod and period at most b, all 3b-prefixes, queries up to 60.
    for (int b = 1; b <= (quick ? 3 : 4); ++b) {
        for (int mu = 0; mu <= b; ++mu) {
            for (int lam = 1; lam <= b; ++lam) {
                for (int code = 0; code < 1 << (mu + lam); ++code) {
                    auto at = [&](ulng i) { return int(i < ulng(mu) ? code >> i & 1 : code >> (mu + (i - ulng(mu)) % ulng(lam)) & 1); };
                    vector<int> s(3 * b);
                    for (int i = 0; i < 3 * b; ++i) { s[i] = at(ulng(i)); }
                    for (ulng k = 0; k < 60; ++k) { check(interpolatePeriodicSequence(s, b, k) == at(k), "interpolatePeriodicSequence exhaustive b=" + str(b)); }
                    check(interpolatePeriodicSequence(s, b, ulng(1) << 62) == at(ulng(1) << 62), "interpolatePeriodicSequence huge k");}}}}
    for (int rep = 0; rep < (quick ? 200 : (stress && fast ? 4 : 1) * 2000); ++rep) {
        int b = int(rng() % 50) + 1, mu = int(rng() % (b + 1)), lam = int(rng() % b) + 1, alpha = int(rng() % 3) + 1;
        vector<string> pre(mu), cyc(lam);
        for (auto &x : pre) { x = string(1, char('a' + rng() % alpha)); }
        for (auto &x : cyc) { x = string(1, char('a' + rng() % alpha)); }
        auto at = [&](ulng i) { return i < ulng(mu) ? pre[i] : cyc[(i - ulng(mu)) % ulng(lam)]; };
        vector<string> s(3 * b + int(rng() % 10));
        for (int i = 0; i < int(s.size()); ++i) { s[i] = at(ulng(i)); }
        ulng k = rng();
        check(interpolatePeriodicSequence(s, b, k) == at(k), "interpolatePeriodicSequence random strings b=" + str(b));}
    cout << "PASS interpolatePeriodicSequence\n";}

void semiringSuite() {
    const lng INF = lng(4e18);
    auto mn = [](lng x, lng y) { return min(x, y); };
    auto add = [INF](lng x, lng y) { return x >= INF || y >= INF ? INF : x + y; };
    auto mx = [](lng x, lng y) { return max(x, y); };
    auto addm = [INF](lng x, lng y) { return x <= -INF || y <= -INF ? -INF : x + y; };
    for (int rep = 0; rep < (quick ? 60 : (stress && fast ? 4 : 1) * 400); ++rep) {
        int d = int(rng() % 9);
        vector<lng> c(d), a(d);
        for (auto &x : c) { x = rng() % 4 == 0 ? INF : lng(rng() % 1000); }
        for (auto &x : a) { x = rng() % 5 == 0 ? INF : lng(rng() % 1000000); }
        vector<lng> s = a;
        int n = 3000;
        while (int(s.size()) < n) {
            lng v = INF;
            for (int j = 1; j <= d; ++j) { v = min(v, add(c[j - 1], s[s.size() - j])); }
            s.pb(v);}
        for (ulng k : {ulng(0), ulng(d), rng() % ulng(n)}) { check(linearRecurrenceSemiring(c, a, k, mn, add, INF, lng(0)) == (d ? s[k] : INF), "min-plus small k d=" + str(d)); }
        if (d) {
            // Min-plus matrix power on the companion matrix for a huge index (values stay below 1000 * 2^20 + 1e6).
            ulng k = rng() % (ulng(1) << 20);
            vector<vector<lng>> t(d, vector<lng>(d, INF)), r(d, vector<lng>(d, INF));
            for (int i = 0; i + 1 < d; ++i) { t[i][i + 1] = 0; }
            for (int j = 1; j <= d; ++j) { t[d - 1][d - j] = c[j - 1]; }
            for (int i = 0; i < d; ++i) { r[i][i] = 0; }
            auto mm = [&](const vector<vector<lng>> &x, const vector<vector<lng>> &y) {
                vector<vector<lng>> z(d, vector<lng>(d, INF));
                for (int i = 0; i < d; ++i) { for (int u = 0; u < d; ++u) { for (int j = 0; j < d; ++j) { z[i][j] = min(z[i][j], add(x[i][u], y[u][j])); } } }
                return z;};
            for (ulng e = k; e; e >>= 1, t = mm(t, t)) { if (e & 1) { r = mm(r, t); } }
            lng want = INF;
            for (int j = 0; j < d; ++j) { want = min(want, add(r[0][j], a[j])); }
            check(linearRecurrenceSemiring(c, a, k, mn, add, INF, lng(0)) == want, "min-plus huge k d=" + str(d));}
        vector<lng> cm(d), am(d), sm;
        for (auto &x : cm) { x = lng(rng() % 1000) - 500; }
        for (auto &x : am) { x = lng(rng() % 1000) - 500; }
        sm = am;
        while (int(sm.size()) < 500) {
            lng v = -INF;
            for (int j = 1; j <= d; ++j) { v = max(v, addm(cm[j - 1], sm[sm.size() - j])); }
            sm.pb(v);}
        ulng k = rng() % 500;
        check(linearRecurrenceSemiring(cm, am, k, mx, addm, -INF, lng(0)) == (d ? sm[k] : -INF), "max-plus d=" + str(d));
        vector<int> cb(d), ab(d), sb;
        for (auto &x : cb) { x = int(rng() % 2); }
        for (auto &x : ab) { x = int(rng() % 2); }
        sb = ab;
        while (int(sb.size()) < 500) {
            int v = 0;
            for (int j = 1; j <= d; ++j) { v |= cb[j - 1] & sb[sb.size() - j]; }
            sb.pb(v);}
        check(linearRecurrenceSemiring(cb, ab, k, [](int x, int y) { return x | y; }, [](int x, int y) { return x & y; }, 0, 1) == (d ? sb[k] : 0), "boolean semiring d=" + str(d));}
    cout << "PASS linearRecurrenceSemiring (min-plus, max-plus, boolean)\n";}

template<typename M>
void matrixSuite(const string &name) {
    for (int rep = 0; rep < (quick ? 40 : (stress && fast ? 4 : 1) * 200); ++rep) {
        int d = int(rng() % (rep % 10 ? 12 : (fast ? 90 : 40)));
        Mat<M> a(d, vector<M>(d));
        int kind = rep % 6;
        for (int i = 0; i < d; ++i) {
            for (int j = 0; j < d; ++j) {
                if (kind == 0) { a[i][j] = M(rng()); }
                if (kind == 1) { a[i][j] = i < j ? M(rng()) : M(0); }
                if (kind == 2) { a[i][j] = i == j ? M(1) : M(0); }
                if (kind == 3) { a[i][j] = M(rng() % 2); }
                if (kind == 4) { a[i][j] = i / 3 == j / 3 ? M(rng() % 3) : M(0); }
                if (kind == 5) { a[i][j] = j == 0 ? M(i + 1) : M(0); }}}
        auto v = rnd<M>(d);
        if (rep % 7 == 0) { v.assign(d, M(0)); }
        if (rep % 7 == 1 && d) { v.assign(d, M(0)); v[rng() % d] = 1; }
        for (ulng k : {ulng(0), ulng(1), ulng(d), rng() % 100, rng()}) {
            vector<M> want(d);
            if (d) {
                auto p = mpow(a, k);
                for (int i = 0; i < d; ++i) { for (int j = 0; j < d; ++j) { want[i] += p[i][j] * v[j]; } }}
            check(matrixPowerVector(a, v, k) == want, name + " matrixPowerVector d=" + str(d) + " kind=" + str(kind));}}
    cout << "PASS " << name << ": matrixPowerVector\n";}

void compositeSuite() {
    using D = DynModInt<4>;
    D::setMod(12);
    vector<D> c;
    check(!findLinearRecurrence(vector<D>{2, 1}, c), "findLinearRecurrence composite failure on a nonunit discrepancy");
    auto fib = extend(vector<D>{1, 1}, vector<D>{0, 1}, 40);
    check(findLinearRecurrence(fib, c) && c == vector<D>{1, 1}, "findLinearRecurrence composite Fibonacci");
    int ok = 0;
    for (int rep = 0; rep < (quick ? 200 : (stress && fast ? 4 : 1) * 2000); ++rep) {
        auto s = rnd<D>(int(rng() % 20));
        if (findLinearRecurrence(s, c)) { ++ok; check(valid(s, c), "findLinearRecurrence composite valid on success"); }}
    check(ok > 0, "findLinearRecurrence composite has successes");
    // Every transform-based recurrence routine is generic over the modulus.
    auto cc = vector<D>{1, 5, 7}, a = vector<D>{3, 2, 11};
    auto s = extend(cc, a, 400);
    for (ulng k : {ulng(0), ulng(5), ulng(399)}) { check(nthTermLinearRecurrence(cc, a, k) == s[k] && kitamasa(cc, a, k) == s[k], "composite nthTerm/kitamasa"); }
    check(bigTerm(cc, a, ulng(1) << 61) == nthTermLinearRecurrence(cc, a, ulng(1) << 61) && consecutiveTerms(cc, a, 100, 50) == vector<D>(s.begin() + 100, s.begin() + 150), "composite huge k and consecutive terms");
    cout << "PASS composite modulus 12: findLinearRecurrence status, generic recurrence routines\n";}

int main(int argc, char **argv) {
    using M = mint;
    if (argc > 2 && string(argv[1]) == "--invalid") {
        string s = argv[2];
        if (s == "bm-composite") { using D = DynModInt<5>; D::setMod(12); berlekampMassey(vector<D>{2, 1}); }
        if (s == "kitamasa-short") { kitamasa(vector<M>{1, 1}, vector<M>{1}, 5); }
        if (s == "semiring-short") { linearRecurrenceSemiring(vector<int>{1, 1}, vector<int>{1}, 5, std::plus<int>(), std::multiplies<int>(), 0, 1); }
        if (s == "bostan-zero") { bostanMori(vector<M>{1}, vector<M>{0, 1}, 5); }
        if (s == "slice-order") { sliceRationalFps(vector<M>{1}, vector<M>{1}, 5, 3); }
        if (s == "slice-zero-q") { sliceRationalFps(vector<M>{1}, vector<M>{0, 1}, 0, 3); }
        if (s == "consecutive-negative") { consecutiveTerms(vector<M>{1}, vector<M>{1}, 0, -1); }
        if (s == "rational-short") { rationalGeneratingFunction(vector<M>{1, 1}, vector<M>{1}); }
        if (s == "periodic-short") { interpolatePeriodicSequence(vector<int>(5), 2, 10); }
        if (s == "periodic-zero-bound") { interpolatePeriodicSequence(vector<int>(5), 0, 10); }
        if (s == "polyexp-empty") { interpolatePolyExp(vector<M>{}, M(2), 5); }
        if (s == "polyexp-long") { interpolatePolyExp(vector<ModInt<7>>(8), ModInt<7>(2), 50); }
        if (s == "matrix-shape") { matrixPowerVector(Mat<M>(2, vector<M>(2)), vector<M>(3), 5); }
        return 0;}
    string mode = "full"; ulng seed = 1;
    for (int i = 1; i + 1 < argc; ++i) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }}
    rng.seed(seed);
    quick = mode == "quick"; stress = mode == "stress";
#ifdef NDEBUG
    fast = true;
#endif
    using P = ModInt<1000000007>;
    using L = ModInt64<4179340454199820289ULL>;
    exhaustiveMinimal<ModInt<2>>("GF(2)", quick ? 7 : 10);
    exhaustiveMinimal<ModInt<3>>("GF(3)", quick ? 4 : 6);
    findSuite<M>("998244353"); findSuite<P>("1e9+7"); findSuite<ModInt<2>>("2");
    termSuite<M>("998244353"); termSuite<P>("1e9+7"); termSuite<L>("64-bit"); termSuite<ModInt<7>>("7");
    rationalSuite<M>("998244353"); rationalSuite<P>("1e9+7");
    interpolationSuite<M>("998244353"); interpolationSuite<P>("1e9+7");
    periodicSuite();
    semiringSuite();
    matrixSuite<M>("998244353"); matrixSuite<ModInt<5>>("5");
    compositeSuite();
    cout << "PASS all, " << checks << " checks\n";}
