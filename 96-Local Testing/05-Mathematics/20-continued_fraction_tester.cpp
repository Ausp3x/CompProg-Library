#include "../../05-Mathematics/20-continued_fraction.hpp"

using Frac = pair<lng, lng>;
string context;
lng checks = 0;
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string show(Frac f) { return std::to_string(f.fi) + "/" + std::to_string(f.se); }
// Exact comparisons; q = 0 is +infinity for a positive numerator.
bool lessF(Frac a, Frac b) { return lll(a.fi) * b.se < lll(b.fi) * a.se; }
bool reduced(Frac f) { return gcd(f.fi, f.se) == 1; }
lll det(Frac hi, Frac lo) { return lll(hi.fi) * lo.se - lll(lo.fi) * hi.se; }
constexpr lng LO = std::numeric_limits<lng>::min(), HI = std::numeric_limits<lng>::max();

void cfCheck(lng p, lng q) {
    context = std::to_string(p) + "/" + std::to_string(q);
    auto a = continuedFraction(p, q);
    lng g = lng(gcd64(p, q));
    need(a.size() == 1 || a.back() >= 2, "canonical last term");
    for (int i = 1; i < int(a.size()); ++i) { need(a[i] >= 1, "positive partial quotients"); }
    auto c = convergents(a);
    need(c.size() == a.size() && c.back() == Frac{p / g, q / g}, "last convergent is p/q reduced");
    for (int i = 0; i < int(c.size()); ++i) {
        lll num = 1, den = 0;
        for (int j = i; j >= 0; --j) { lll t = a[j] * num + den; den = num; num = t; }
        need(num == c[i].fi && den == c[i].se, "convergent equals the evaluated prefix");
        if (i) { need(det(c[i], c[i - 1]) == (i % 2 ? 1 : -1), "consecutive convergent determinant (-1)^(i-1)"); }}}
void semiCheck(const vector<lng> &a) {
    context = "semiconvergents of " + std::to_string(a.size()) + " terms, a0=" + std::to_string(a[0]);
    auto c = convergents(a), s = semiconvergents(a);
    vector<Frac> want{c[0]};
    for (int i = 1; i < int(a.size()); ++i) {
        Frac p2 = i >= 2 ? c[i - 2] : Frac{1, 0}, p1 = c[i - 1];
        for (lng t = 1; t <= a[i]; ++t) { want.pb({t * p1.fi + p2.fi, t * p1.se + p2.se}); }}
    need(s == want, "semiconvergents by definition");
    for (int i = 1; i < int(s.size()); ++i) { need(s[i - 1].se <= s[i].se, "semiconvergents sorted by denominator"); }}

// {lo, hi} bracket x/y; equal to x/y reduced exactly when it qualifies; else Stern-Brocot neighbours whose mediant leaves the bounds.
void bestCertificate(lng x, lng y, lng n) {
    context = "x/y=" + std::to_string(x) + "/" + std::to_string(y) + " n=" + std::to_string(n);
    auto [lo, hi] = bestRationalApproximation(x, y, n);
    lng g = gcd(x, y);
    Frac v{x / g, y / g};
    bool fits = v.fi <= n && v.se <= n;
    need(0 <= lo.fi && lo.fi <= n && 1 <= lo.se && lo.se <= n && reduced(lo), "lower bounded and reduced");
    need(hi == Frac{1, 0} || (0 <= hi.fi && hi.fi <= n && 1 <= hi.se && hi.se <= n && reduced(hi)), "upper bounded and reduced");
    if (fits) { need(lo == v && hi == v, "both equal x/y when it qualifies"); return; }
    need(lessF(lo, v) && (hi.se == 0 || lessF(v, hi)), "strict bracket");
    need(det(hi, lo) == 1 && (lll(lo.fi) + hi.fi > n || lll(lo.se) + hi.se > n), "adjacent with mediant out of bounds");}
// (L, R) of p/q from sternBrocotRange holds every fraction with smaller depth outside (a/b, c/d).
void intervalCertificate(lng a, lng b, lng c, lng d) {
    context = std::to_string(a) + "/" + std::to_string(b) + " " + std::to_string(c) + "/" + std::to_string(d);
    Frac r = rationalInInterval(a, b, c, d);
    need(r.se >= 1 && reduced(r) && lessF({a, b}, r) && (d == 0 || lessF(r, {c, d})), "inside the open interval");
    auto [L, R] = sternBrocotRange(r.fi, r.se);
    need(!lessF({a, b}, L) && (R.se == 0 || (d != 0 && !lessF(R, {c, d}))), "interval inside the subtree of the result");}
void searchCertificate(lng n, Frac lo, Frac hi, auto f) {
    need(lo == Frac{0, 1} || (1 <= lo.fi && lo.fi <= n && 1 <= lo.se && lo.se <= n && !f(lo.fi, lo.se)), "lo is false and bounded");
    need(hi == Frac{1, 0} || (1 <= hi.fi && hi.fi <= n && 1 <= hi.se && hi.se <= n && f(hi.fi, hi.se)), "hi is true and bounded");
    need(det(hi, lo) == 1 && (lll(lo.fi) + hi.fi > n || lll(lo.se) + hi.se > n), "adjacent with mediant out of bounds");}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261018;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            string op;
            auto put = [](const vector<lng> &v) { for (lng x : v) { cout << x << ' '; } };
            while (cin >> op) {
                if (op == "cf") {
                    lng p, q; cin >> p >> q;
                    auto a = continuedFraction(p, q); put(a); cout << "|";
                    for (auto [x, y] : convergents(a)) { cout << ' ' << x << ' ' << y; }
                    lll total = 0;
                    for (int j = 1; j < int(a.size()); ++j) { total += a[j]; }
                    cout << " |";
                    if (total <= 3000) { for (auto [x, y] : semiconvergents(a)) { cout << ' ' << x << ' ' << y; } }}
                else if (op == "quad") {
                    lng P, D, Q; cin >> P >> D >> Q;
                    auto r = continuedFractionQuadratic(P, D, Q); put(r.pre); cout << "| "; put(r.period);}
                else if (op == "sb") {
                    lng p, q, k; cin >> p >> q >> k;
                    for (auto [c, t] : sternBrocotEncode(p, q)) { cout << c << ' ' << t << ' '; }
                    auto [lo, hi] = sternBrocotRange(p, q); auto [cl, cr] = sternBrocotChildren(p, q); auto an = sternBrocotAncestor(p, q, k);
                    cout << "| " << lo.fi << ' ' << lo.se << ' ' << hi.fi << ' ' << hi.se << ' ' << cl.fi << ' ' << cl.se << ' ' << cr.fi << ' ' << cr.se
                        << ' ' << an.fi << ' ' << an.se;}
                else if (op == "lca") {
                    lng p, q, r, s; cin >> p >> q >> r >> s;
                    auto [x, y] = sternBrocotLca(p, q, r, s); cout << x << ' ' << y;}
                else if (op == "interval") {
                    lng a, b, c, d; cin >> a >> b >> c >> d;
                    auto [x, y] = rationalInInterval(a, b, c, d); cout << x << ' ' << y;}
                else if (op == "best") {
                    lng x, y, n; cin >> x >> y >> n;
                    auto [lo, hi] = bestRationalApproximation(x, y, n); cout << lo.fi << ' ' << lo.se << ' ' << hi.fi << ' ' << hi.se;}
                else if (op == "farey") {
                    lng n, k, a, b, c, d; cin >> n >> k >> a >> b >> c >> d;
                    auto [x, y] = fareyKth(n, k);
                    Frac next = x < y ? fareyNext(x, y, n) : Frac{-1, -1};
                    cout << x << ' ' << y << ' ' << next.fi << ' ' << next.se << ' ' << lng(countRationalsInRange(n, a, b, c, d));}
                cout << '\n';}
            return 0;}}
    if (!invalid.empty()) {
        auto f = [](lng, lng) { return true; };
        if (invalid == "cf-q") { continuedFraction(1, 0); }
        else if (invalid == "quad-q") { continuedFractionQuadratic(1, 2, 0); }
        else if (invalid == "quad-d") { continuedFractionQuadratic(1, -2, 1); }
        else if (invalid == "convergents-term") { convergents({1, 0, 2}); }
        else if (invalid == "convergents-overflow") { convergents({HI, HI, HI}); }
        else if (invalid == "semi-term") { semiconvergents({1, 2, -1}); }
        else if (invalid == "best-y") { bestRationalApproximation(1, 0, 5); }
        else if (invalid == "interval-order") { rationalInInterval(1, 2, 1, 3); }
        else if (invalid == "search-n") { fractionBinarySearch(0, f); }
        else if (invalid == "encode-coprime") { sternBrocotEncode(2, 4); }
        else if (invalid == "decode-char") { sternBrocotDecode({{'X', 1}}); }
        else if (invalid == "ancestor-k") { sternBrocotAncestor(2, 3, -1); }
        else if (invalid == "range-zero") { sternBrocotRange(0, 1); }
        else if (invalid == "farey-n") { fareySequence(0); }
        else if (invalid == "next-p") { fareyNext(3, 3, 5); }
        else if (invalid == "kth-k") { fareyKth(3, 5); }
        else if (invalid == "count-order") { countRationalsInRange(5, 1, 2, 1, 3); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    int C = quick ? 25 : 60;
    for (lng p = -C; p <= C; ++p) { for (lng q = 1; q <= C; ++q) { cfCheck(p, q); semiCheck(continuedFraction(p, q)); }}
    for (lng p : {LO, LO + 1, lng(-1), lng(0), lng(1), HI - 1, HI}) { for (lng q : {lng(1), lng(2), HI - 1, HI}) { cfCheck(p, q); }}
    for (int i = 0; i < (quick ? 2000 : stress ? 200000 : 20000); ++i) {
        lng q = max<lng>(1, lng(rng() >> (1 + rng() % 63))), p = std::bit_cast<lng>(rng()) >> (rng() % 63);
        cfCheck(p, q);
        vector<lng> a{lng(rng() % 21) - 10};
        for (int j = int(rng() % 8); j > 0; --j) { a.pb(1 + lng(rng() % 30)); }
        semiCheck(a);}
    cout << "PASS continued fractions, convergents, semiconvergents checks=" << checks << '\n';
    for (lng D = 0; D <= (quick ? 300 : 3000); ++D) {
        context = "sqrt " + std::to_string(D);
        auto r = continuedFractionSqrt(D);
        lng s = lng(continued_fraction_detail::isqrt(ulll(D)));
        need(r.pre == vector<lng>{s}, "sqrt preperiod is a0");
        if (s * s == D) { need(r.period.empty(), "square has no period"); continue; }
        need(!r.period.empty() && r.period.back() == 2 * s, "sqrt period ends with 2 a0");
        need(std::equal(r.period.begin(), r.period.end() - 1, r.period.rbegin() + 1), "sqrt period body is a palindrome");}
    lng big = (lng(1) << 31) - 1;
    vector<array<lng, 3>> quads{{big, big * big + 1, 1}, {-big, big * big + 1, big}, {big, big * big, -big}, {-big, (lng(1) << 62) - 1, 1}, {0, 2, big}};
    for (int i = 0; i < (quick ? 2000 : stress ? 100000 : 20000); ++i) {
        lng Q = lng(rng() % 61) - 30;
        quads.push_back({lng(rng() % 61) - 30, lng(rng() % (quick ? 2000 : 5000)), Q ? Q : 1});}
    for (auto [P, D, Q] : quads) {
        context = "quad P=" + std::to_string(P) + " D=" + std::to_string(D) + " Q=" + std::to_string(Q);
        auto r = continuedFractionQuadratic(P, D, Q);
        if (r.period.empty()) { continue; }
        need(r.pre.empty() || r.pre.back() != r.period.back(), "minimal preperiod");
        int l = int(r.period.size());
        for (int d = 1; d < l; ++d) { if (l % d == 0) { need(!std::equal(r.period.begin() + d, r.period.end(), r.period.begin()), "primitive period"); }}}
    cout << "PASS quadratic structure checks=" << checks << '\n';
    for (lng x = 0; x <= (quick ? 15 : 30); ++x) { for (lng y = 1; y <= (quick ? 15 : 30); ++y) { for (lng n = 1; n <= 12; ++n) {
        context = "x/y=" + std::to_string(x) + "/" + std::to_string(y) + " n=" + std::to_string(n);
        Frac lo{0, 1}, hi{1, 0};
        for (lng p = 0; p <= n; ++p) { for (lng q = 1; q <= n; ++q) {
            if (!reduced({p, q})) { continue; }
            if (!lessF({x, y}, {p, q}) && lessF(lo, {p, q})) { lo = {p, q}; }
            if (!lessF({p, q}, {x, y}) && (hi.se == 0 || lessF({p, q}, hi))) { hi = {p, q}; }}}
        need(bestRationalApproximation(x, y, n) == pair{lo, hi}, "bestRationalApproximation brute");
        auto f = [&](lng p, lng q) { return lll(p) * y >= lll(x) * q; };
        Frac flo{0, 1}, fhi{1, 0};
        for (lng p = 1; p <= n; ++p) { for (lng q = 1; q <= n; ++q) {
            if (!reduced({p, q})) { continue; }
            if (!f(p, q) && lessF(flo, {p, q})) { flo = {p, q}; }
            if (f(p, q) && (fhi.se == 0 || lessF({p, q}, fhi))) { fhi = {p, q}; }}}
        need(fractionBinarySearch(n, f) == pair{flo, fhi}, "fractionBinarySearch brute");}}}
    int I = quick ? 8 : 12;
    for (lng a = 0; a <= I; ++a) { for (lng b = 1; b <= I; ++b) { for (lng c = 0; c <= I; ++c) { for (lng d = 0; d <= I; ++d) {
        if ((d == 0 && c != 1) || lll(a) * d >= lll(c) * b) { continue; }
        context = std::to_string(a) + "/" + std::to_string(b) + " " + std::to_string(c) + "/" + std::to_string(d);
        Frac best{-1, -1};
        for (lng q = 1; q <= b + d && best.fi < 0; ++q) { for (lng p = 0; p <= a + c; ++p) {
            if (lll(a) * q < lll(p) * b && (d == 0 || lll(p) * d < lll(c) * q)) { best = {p, q}; break; }}}
        need(rationalInInterval(a, b, c, d) == best, "rationalInInterval brute");}}}}
    cout << "PASS approximation, interval and search brute force checks=" << checks << '\n';
    for (int i = 0; i < (quick ? 3000 : stress ? 300000 : 30000); ++i) {
        lng x = lng(rng() >> (1 + rng() % 63)), y = max<lng>(1, lng(rng() >> (1 + rng() % 63))), n = max<lng>(1, lng(rng() >> (1 + rng() % 63)));
        bestCertificate(x, y, n);
        lng a = lng(rng() >> (2 + rng() % 62)), b = max<lng>(1, lng(rng() >> (2 + rng() % 62))), c = lng(rng() >> (2 + rng() % 62)), d = lng(rng() >> (2 + rng() % 62));
        if (lll(a) * d < lll(c) * b) { intervalCertificate(a, b, c, d); }
        else if (lll(c) * b < lll(a) * d && d >= 1) { intervalCertificate(c, d, a, b); }
        if (i % 16 == 0) { intervalCertificate(a, b, 1, 0); }
        int calls = 0;
        auto f = [&](lng p, lng q) { ++calls; return lll(p) * y >= lll(x) * q; };
        context = "search x/y=" + std::to_string(x) + "/" + std::to_string(y) + " n=" + std::to_string(n);
        auto [lo, hi] = fractionBinarySearch(n, f);
        searchCertificate(n, lo, hi, f);
        need(calls <= 6 * std::bit_width(ulng(n)) + 8, "O(log(n)) predicate calls");
        if (i % 64 == 0) {
            auto root2 = [](lng p, lng q) { return lll(p) * p >= 2 * lll(q) * q; };
            lng m = max<lng>(1, lng(rng() >> (rng() % 32 + 32)));
            auto [l2, h2] = fractionBinarySearch(m, root2);
            context = "sqrt(2) search n=" + std::to_string(m);
            searchCertificate(m, l2, h2, root2);}}
    cout << "PASS large certificates for approximation, interval, search checks=" << checks << '\n';
    struct Node { Frac v, lo, hi; int par, depth; };
    vector<Node> t{{{1, 1}, {0, 1}, {1, 0}, -1, 0}};
    std::map<Frac, int> id{{{1, 1}, 0}};
    int depth = quick ? 9 : 11;
    for (int i = 0; i < int(t.size()); ++i) {
        if (t[i].depth == depth) { continue; }
        Node cur = t[i];
        Frac l{cur.lo.fi + cur.v.fi, cur.lo.se + cur.v.se}, r{cur.v.fi + cur.hi.fi, cur.v.se + cur.hi.se};
        id[l] = int(t.size()); t.pb({l, cur.lo, cur.v, i, cur.depth + 1});
        id[r] = int(t.size()); t.pb({r, cur.v, cur.hi, i, cur.depth + 1});}
    for (int i = 0; i < int(t.size()); ++i) {
        auto [p, q] = t[i].v;
        context = "node " + show(t[i].v);
        vector<int> anc;
        for (int j = i; j >= 0; j = t[j].par) { anc.pb(j); }
        reverse(anc.begin(), anc.end());
        string word;
        for (int j = 1; j < int(anc.size()); ++j) { word += lessF(t[anc[j]].v, t[anc[j - 1]].v) ? 'L' : 'R'; }
        vector<pair<char, lng>> runs;
        for (char ch : word) { if (runs.empty() || runs.back().fi != ch) { runs.pb({ch, 0}); } ++runs.back().se; }
        need(sternBrocotEncode(p, q) == runs, "encode equals the BFS path");
        need(sternBrocotDecode(runs) == t[i].v, "decode");
        need(sternBrocotRange(p, q) == pair{t[i].lo, t[i].hi}, "range");
        if (t[i].depth < depth) {
            auto [cl, cr] = sternBrocotChildren(p, q);
            need(id.count(cl) && id.count(cr) && t[id[cl]].par == i && t[id[cr]].par == i && lessF(cl, cr), "children");}
        for (lng k = 0; k <= t[i].depth + 2; ++k) { need(sternBrocotAncestor(p, q, k) == (k <= t[i].depth ? t[anc[k]].v : Frac{-1, -1}), "ancestor"); }
        if (i % 5 == 0) {
            std::set<int> up(anc.begin(), anc.end());
            for (int j = 0; j < int(t.size()); j += 7) {
                int y = j;
                while (!up.count(y)) { y = t[y].par; }
                need(sternBrocotLca(p, q, t[j].v.fi, t[j].v.se) == t[y].v, "lca");}}}
    cout << "PASS Stern-Brocot tree against BFS to depth " << depth << " checks=" << checks << '\n';
    for (int i = 0; i < (quick ? 2000 : stress ? 200000 : 20000); ++i) {
        lng p = max<lng>(1, lng(rng() >> (2 + rng() % 62))), q = max<lng>(1, lng(rng() >> (2 + rng() % 62)));
        lng g = gcd(p, q); p /= g; q /= g;
        context = "node " + show({p, q});
        auto path = sternBrocotEncode(p, q);
        need(sternBrocotDecode(path) == Frac{p, q}, "large roundtrip");
        lng total = 0;
        for (auto [c, s] : path) { total += s; }
        need(sternBrocotAncestor(p, q, total) == Frac{p, q} && sternBrocotAncestor(p, q, total + 1) == Frac{-1, -1}, "depth by ancestor");
        auto [lo, hi] = sternBrocotRange(p, q);
        need(lo.fi + hi.fi == p && lo.se + hi.se == q && det(hi, lo) == 1, "node is the mediant of its range");
        lng k = total ? lng(rng() % ulng(total)) : 0;
        Frac an = sternBrocotAncestor(p, q, k);
        auto [alo, ahi] = sternBrocotRange(an.fi, an.se);
        need(lessF(alo, {p, q}) && (ahi.se == 0 || lessF({p, q}, ahi)), "ancestor range contains the node");
        lng r = max<lng>(1, lng(rng() >> (2 + rng() % 62))), s = max<lng>(1, lng(rng() >> (2 + rng() % 62)));
        g = gcd(r, s); r /= g; s /= g;
        Frac w = sternBrocotLca(p, q, r, s);
        auto [wlo, whi] = sternBrocotRange(w.fi, w.se);
        auto inside = [&](Frac x, Frac l, Frac h) { return lessF(l, x) && (h.se == 0 || lessF(x, h)); };
        need(inside({p, q}, wlo, whi) && inside({r, s}, wlo, whi), "lca range contains both");
        bool sideP = lessF({p, q}, w), sideR = lessF({r, s}, w);
        need(Frac{p, q} == w || Frac{r, s} == w || sideP != sideR, "both nodes leave the lca on different sides");}
    cout << "PASS large Stern-Brocot certificates checks=" << checks << '\n';
    for (int n = 1; n <= (quick ? 25 : 60); ++n) {
        context = "farey n=" + std::to_string(n);
        vector<Frac> fs;
        for (lng q = 1; q <= n; ++q) { for (lng p = 0; p <= q; ++p) { if (gcd(p, q) == 1) { fs.pb({p, q}); }}}
        sort(fs.begin(), fs.end(), lessF);
        auto seq = fareySequence(n);
        need(seq.size() == fs.size(), "fareySequence length");
        for (int i = 0; i < int(fs.size()); ++i) {
            need(Frac{seq[i].fi, seq[i].se} == fs[i], "fareySequence term");
            need(fareyKth(n, i) == fs[i], "fareyKth");
            if (i + 1 < int(fs.size())) { need(fareyNext(fs[i].fi, fs[i].se, n) == fs[i + 1], "fareyNext"); }}
        for (int i = 0; i < int(fs.size()); i += 3) { for (int j = i; j < int(fs.size()); j += 5) {
            need(countRationalsInRange(n, fs[i].fi, fs[i].se, fs[j].fi, fs[j].se) == j - i, "countRationalsInRange between terms");}}
        for (lng x = 0; x <= 20; ++x) { for (lng y = 1; y <= 7; ++y) {
            lng want = 0;
            for (lng q = 1; q <= n; ++q) { for (lng p = 0; p * y < x * q; ++p) { want += gcd(p, q) == 1; }}
            need(countRationalsInRange(n, 0, 1, x, y) == want, "countRationalsInRange below x/y, also above 1");}}}
    cout << "PASS Farey brute force checks=" << checks << '\n';
    // Independent count: R[s] = ceil(s x / y) - sum over proper divisors d of R[d], an O(n log(n)) divisor sieve.
    auto dpCount = [](int n, lng x, lng y) {
        vector<lng> R(n + 1);
        for (int s = 1; s <= n; ++s) { R[s] += lng(ceilDiv(lll(s) * x, y)); for (int m = 2 * s; m <= n; m += s) { R[m] -= R[s]; }}
        return accumulate(R.begin(), R.end(), lng(0));};
    for (int i = 0; i < (quick ? 6 : stress ? 200 : 30); ++i) {
        int n = 1 + int(rng() % (quick ? 3000 : 200000));
        lng y = 1 + lng(rng() % 1000000), x = lng(rng() % ulng(3 * y));
        context = "farey n=" + std::to_string(n) + " x/y=" + std::to_string(x) + "/" + std::to_string(y);
        need(countRationalsInRange(n, 0, 1, x, y) == dpCount(n, x, y), "countRationalsInRange against the divisor sieve");
        lng total = dpCount(n, 1, 1) + 1, k = lng(rng() % ulng(total));
        Frac f = fareyKth(n, k);
        need(reduced(f) && 1 <= f.se && f.se <= n && f.fi <= f.se && (f.fi == 0 ? k == 0 : dpCount(n, f.fi, f.se) == k), "fareyKth has exactly k smaller terms");
        if (f.fi < f.se) {
            Frac g = fareyNext(f.fi, f.se, n);
            need(det(g, f) == 1 && g.se <= n && g.se + f.se > n, "fareyNext certificate");}}
    for (lng n : {lng(1000000000), lng(999999937)}) {
        context = "farey n=" + std::to_string(n);
        for (int i = 0; i < (quick ? 1 : 3); ++i) {
            lng total = lng(countRationalsInRange(n, 0, 1, 1, 1)) + 1, k = lng(rng() % ulng(total));
            Frac f = fareyKth(n, k);
            need(reduced(f) && f.se <= n && countRationalsInRange(n, 0, 1, f.fi, f.se) == k, "large fareyKth rank");
            Frac g = fareyNext(f.fi, f.se, n);
            need(det(g, f) == 1 && g.se <= n && g.se + f.se > n && countRationalsInRange(n, 0, 1, g.fi, g.se) == k + 1, "large fareyNext");}}
    cout << "PASS Farey rank, kth and next certificates checks=" << checks << '\n';}
