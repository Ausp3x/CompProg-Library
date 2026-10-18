#include "../../05-Mathematics/67-lattice_paths.hpp"

string context;
lng checks = 0;
void need(bool ok, string_view what) {
    ++checks;
    if (!ok) { cerr << "FAIL " << what << " input=" << context << '\n'; std::exit(1); }}
string str(std::initializer_list<lng> v) {
    string s;
    for (lng x : v) { s += std::to_string(x) + " "; }
    return s;}

// Grid DP over [0, X] x [0, Y] with right, up and optional diagonal steps; ok(x, y) filters points.
template<typename M, typename F>
M gridDp(int x0, int y0, int X, int Y, F ok, bool diag = false) {
    if (X < x0 || Y < y0) { return 0; }
    vector<vector<M>> d(X - x0 + 1, vector<M>(Y - y0 + 1, 0));
    for (int x = x0; x <= X; ++x) {
        for (int y = y0; y <= Y; ++y) {
            if (!ok(x, y)) { continue; }
            M &c = d[x - x0][y - y0];
            if (x == x0 && y == y0) { c = 1; continue; }
            if (x > x0) { c += d[x - x0 - 1][y - y0]; }
            if (y > y0) { c += d[x - x0][y - y0 - 1]; }
            if (diag && x > x0 && y > y0) { c += d[x - x0 - 1][y - y0 - 1]; }}}
    return d[X - x0][Y - y0];}
// Walk DP with steps in S, heights kept in [0, H].
template<typename M = mint>
M walkDp(int n, int h1, int h2, const vector<int> &S, int H) {
    vector<M> d(H + 1, 0);
    if (h1 > H) { return 0; }
    d[h1] = 1;
    for (int i = 0; i < n; ++i) {
        vector<M> e(H + 1, 0);
        for (int h = 0; h <= H; ++h) { for (int s : S) { if (h + s >= 0 && h + s <= H) { e[h + s] += d[h]; } } }
        d = e;}
    return h2 <= H ? d[h2] : M(0);}

void small(const ModFac &pc, std::mt19937_64 &rng, bool quick) {
    int G = quick ? 7 : 10;
    for (int dx = -2; dx <= G; ++dx) {
        for (int dy = -2; dy <= G; ++dy) {
            context = str({dx, dy});
            need(gridPaths(dx, dy, pc) == gridDp<mint>(0, 0, dx, dy, [](int, int) { return true; }), "gridPaths");}}
    for (int n = 0; n <= 2 * G; ++n) {
        for (int h1 = 0; h1 <= G; ++h1) {
            for (int h2 = 0; h2 <= G; ++h2) {
                context = str({n, h1, h2});
                need(dyckPaths(n, h1, h2, pc) == walkDp(n, h1, h2, {-1, 1}, 40), "dyckPaths");
                need(motzkinPaths(n, h1, h2, pc) == walkDp(n, h1, h2, {-1, 0, 1}, 40), "motzkinPaths");}}}
    int B = quick ? 5 : 7;
    for (int x1 = 0; x1 <= B; ++x1) { for (int y1 = 0; y1 <= B; ++y1) { for (int x2 = 0; x2 <= B; ++x2) { for (int y2 = 0; y2 <= B; ++y2) {
        context = str({x1, y1, x2, y2});
        need(pathsBelowDiagonal(x1, y1, x2, y2, pc) == gridDp<mint>(x1, y1, x2, y2, [](int x, int y) { return y <= x; }), "pathsBelowDiagonal");
        for (int lo = -3; lo <= 1; ++lo) {
            for (int hi = lo; hi <= 3; ++hi) {
                context = str({x1, y1, x2, y2, lo, hi});
                need(pathsBetweenLines(x1, y1, x2, y2, lo, hi, pc) == gridDp<mint>(x1, y1, x2, y2, [&](int x, int y) { return lo <= y - x && y - x <= hi; }), "pathsBetweenLines");}}}}}}
    for (int mu = 0; mu <= 3; ++mu) { for (int b = 0; b <= 3; ++b) { for (int d = b; d <= 4; ++d) { for (int a = 0; a <= 12; ++a) { for (int c = a; c <= 14; ++c) {
        context = str({a, b, c, d, mu});
        need(pathsBelowLine(a, b, c, d, mu, pc) == gridDp<mint>(a, b, c, d, [&](int x, int y) { return x >= mu * y; }), "pathsBelowLine");}}}}}
    // Ballot words: A steps right, B steps up; weak keeps #A >= mu #B, strict #A > mu #B on nonempty prefixes.
    for (int mu = 0; mu <= 3; ++mu) {
        for (int c = 0; c <= 12; ++c) {
            for (int d = 0; d <= 6; ++d) {
                context = str({c, d, mu});
                need(ballotGeneral(c, d, mu, false, pc) == gridDp<mint>(0, 0, c, d, [&](int x, int y) { return x >= mu * y; }), "ballotGeneral weak");
                mint strict = c + d == 0 ? mint(1) : gridDp<mint>(0, 0, c, d, [&](int x, int y) { return (x == 0 && y == 0) || x > mu * y; });
                need(ballotGeneral(c, d, mu, true, pc) == strict, "ballotGeneral strict");}}}
    vector<mint> cat(30);
    for (int n = 0; n < 30; ++n) { cat[n] = walkDp(2 * n, 0, 0, {-1, 1}, 64); }
    vector<mint> pw{1};
    for (int k = 0; k <= 6; ++k) {
        for (int n = 0; n < 20; ++n) {
            context = str({n, k});
            need(catalanConvolution(n, k, pc) == (n < int(pw.size()) ? pw[n] : mint(0)), "catalanConvolution");}
        pw = truncatedConvolution(pw, cat, 30);}
    auto tri = catalanTriangle<lng>(12);
    for (int i = 0; i <= 12; ++i) {
        for (int j = 0; j <= i; ++j) {
            context = str({i, j});
            need(mint(tri[i][j]) == gridDp<mint>(0, 0, i, j, [](int x, int y) { return x >= y; }), "catalanTriangle");}}
    // Narayana: Dyck words of semilength n with k peaks, enumerated.
    for (int n = 0; n <= (quick ? 8 : 11); ++n) {
        vector<lng> peaks(n + 1, 0);
        for (int w = 0; w < (1 << (2 * n)); ++w) {
            int h = 0, pk = 0;
            bool ok = std::popcount(uint(w)) == n;
            for (int i = 0; i < 2 * n && ok; ++i) {
                bool up = (w >> i) & 1;
                h += up ? 1 : -1; ok = h >= 0;
                if (up && i + 1 < 2 * n && !((w >> (i + 1)) & 1)) { ++pk; }}
            if (ok) { ++peaks[pk]; }}
        for (int k = -1; k <= n + 1; ++k) {
            context = str({n, k});
            need(narayana(n, k, pc) == mint(k >= 0 && k <= n ? peaks[k] : 0), "narayana");}}
    int T = quick ? 12 : 30;
    auto mt = motzkinTable(T), sl = schroderLarge(T), ss = schroderSmall(T), dc = delannoyCentral(T);
    for (int n = 0; n <= T; ++n) {
        context = str({n});
        need(mt[n] == walkDp(n, 0, 0, {-1, 0, 1}, 64), "motzkinTable");
        need(sl[n] == gridDp<mint>(0, 0, n, n, [](int x, int y) { return y <= x; }, true), "schroderLarge");
        // Small Schroder: large Schroder paths without diagonal steps on the diagonal.
        vector<vector<mint>> d(n + 1, vector<mint>(n + 1, 0));
        d[0][0] = 1;
        for (int x = 0; x <= n; ++x) { for (int y = 0; y <= x; ++y) {
            if (x > 0 && y <= x - 1) { d[x][y] += d[x - 1][y]; }
            if (y > 0) { d[x][y] += d[x][y - 1]; }
            if (x > 0 && y > 0 && y - 1 < x - 1) { d[x][y] += d[x - 1][y - 1]; }}}
        need(ss[n] == d[n][n], "schroderSmall");
        need(dc[n] == gridDp<mint>(0, 0, n, n, [](int, int) { return true; }, true), "delannoyCentral");
        for (int m = 0; m <= min(n, 12); ++m) { need(delannoy(m, n, pc) == gridDp<mint>(0, 0, m, n, [](int, int) { return true; }, true), "delannoy"); }}
    for (int a = 1; a <= 4; ++a) {
        for (int b = 1; b <= 4; ++b) {
            auto bz = bizleyPaths(a, b, quick ? 4 : 6);
            for (int i = 0; i < int(bz.size()); ++i) {
                context = str({a, b, i});
                need(bz[i] == gridDp<mint>(0, 0, a * i, b * i, [&](int x, int y) { return lng(a) * y <= lng(b) * x; }), "bizleyPaths");}}}
    cout << "PASS exhaustive small counts vs grid and walk DP checks=" << checks << '\n';
    (void)rng;}

// Random obstacles, rectangles, boundary flows, bounds and LGV systems against DP and enumeration.
void randomized(const ModFac &pc, std::mt19937_64 &rng, int rounds, int size) {
    auto rnd = [&](int lo, int hi) { return lo + int(rng() % ulng(hi - lo + 1)); };
    for (int r = 0; r < rounds; ++r) {
        int X = rnd(-1, size), Y = rnd(-1, size), k = rnd(0, 12);
        vector<pair<int, int>> pts(k);
        for (auto &[x, y] : pts) { x = rnd(-2, size + 2); y = rnd(-2, size + 2); }
        if (k && r % 9 == 0) { pts[0] = {0, 0}; }
        if (k && r % 11 == 0) { pts[0] = {X, Y}; }
        if (k > 1 && r % 5 == 0) { pts[1] = pts[0]; }
        set<pair<int, int>> blocked(pts.begin(), pts.end());
        context = "X=" + std::to_string(X) + " Y=" + std::to_string(Y) + " k=" + std::to_string(k);
        need(gridPathsAvoidingPoints(X, Y, pts, pc) == gridDp<mint>(0, 0, X, Y, [&](int x, int y) { return !blocked.count({x, y}); }), "gridPathsAvoidingPoints");
        int x1 = rnd(-3, size + 1), x2 = rnd(-3, size + 3), y1 = rnd(-3, size + 1), y2 = rnd(-3, size + 3);
        context += " rect=" + str({x1, y1, x2, y2});
        need(gridPathsAvoidingRectangle(X, Y, x1, y1, x2, y2, pc) == gridDp<mint>(0, 0, X, Y, [&](int x, int y) { return !(x1 <= x && x < x2 && y1 <= y && y < y2); }), "gridPathsAvoidingRectangle");
        int h = rnd(0, size), w = rnd(0, size);
        vector<mint> left(h), bottom(w);
        for (auto &v : left) { v = mint(rng()); }
        for (auto &v : bottom) { v = mint(rng()); }
        auto [right, top] = gridPathsBoundaryTransfer(left, bottom, pc);
        context = "h=" + std::to_string(h) + " w=" + std::to_string(w);
        if (h == 0 || w == 0) { need(right == left && top == bottom, "boundary transfer pass-through"); }
        else {
            vector<vector<mint>> d(w, vector<mint>(h, 0));
            for (int x = 0; x < w; ++x) { for (int y = 0; y < h; ++y) { d[x][y] = (x ? d[x - 1][y] : left[y]) + (y ? d[x][y - 1] : bottom[x]); } }
            bool ok = int(right.size()) == h && int(top.size()) == w;
            for (int y = 0; y < h && ok; ++y) { ok = right[y] == d[w - 1][y]; }
            for (int x = 0; x < w && ok; ++x) { ok = top[x] == d[x][h - 1]; }
            need(ok, "gridPathsBoundaryTransfer");}}}

// Nondecreasing x with a_i <= x_i < b_i by prefix-sum DP.
template<typename M = mint>
M sequencesDp(const vector<int> &a, const vector<int> &b) {
    int n = int(a.size()), top = 0;
    for (int v : b) { top = max(top, v); }
    vector<M> d(top + 1, 1);
    for (int i = 0; i < n; ++i) {
        vector<M> e(top + 1, 0);
        for (int v = 0; v <= top; ++v) { e[v] = (v ? e[v - 1] : M(0)) + (a[i] <= v && v < b[i] ? d[v] : M(0)); }
        d = e;}
    return n == 0 ? M(1) : d[top];}
void sequences(std::mt19937_64 &rng, int rounds, int N, int M) {
    for (int r = 0; r < rounds; ++r) {
        int n = int(rng() % ulng(N + 1)), m = 1 + int(rng() % ulng(M));
        vector<int> a(n), b(n);
        for (int i = 0; i < n; ++i) {
            if (r % 3 == 0) { a[i] = int(rng() % ulng(m)); b[i] = a[i] + 1 + int(rng() % ulng(m - a[i])); }
            else {
                int c = int(lng(i) * m / max(n, 1)), w = 1 + int(rng() % ulng(max(1, m / 3)));
                a[i] = max(0, min(m - 1, c - int(rng() % ulng(w)))); b[i] = min(m, a[i] + 1 + int(rng() % ulng(2 * w)));}}
        context = "n=" + std::to_string(n) + " m=" + std::to_string(m) + " r=" + std::to_string(r);
        need(increasingSequencesBetweenBounds(a, b) == sequencesDp(a, b), "increasingSequencesBetweenBounds");
        if (r % 4 == 0) { need(increasingSequencesBetweenBounds<ModInt<1000003>>(a, b) == sequencesDp<ModInt<1000003>>(a, b), "increasingSequencesBetweenBounds second prime"); }}}

// LGV: det equals the signed count of vertex-disjoint path systems over every endpoint permutation.
void lgv(const ModFac &pc, std::mt19937_64 &rng, int rounds) {
    for (int r = 0; r < rounds; ++r) {
        int k = 1 + int(rng() % 3), S = 4;
        vector<pair<int, int>> s(k), t(k);
        for (auto &[x, y] : s) { x = int(rng() % 3); y = int(rng() % 3); }
        for (auto &[x, y] : t) { x = 1 + int(rng() % S); y = 1 + int(rng() % S); }
        auto paths = [&](pair<int, int> a, pair<int, int> b) {
            vector<vector<pair<int, int>>> res;
            vector<pair<int, int>> cur{a};
            auto rec = [&](auto &&self, pair<int, int> p) -> void {
                if (p == b) { res.pb(cur); return; }
                if (p.first < b.first) { cur.pb({p.first + 1, p.second}); self(self, cur.back()); cur.pop_back(); }
                if (p.second < b.second) { cur.pb({p.first, p.second + 1}); self(self, cur.back()); cur.pop_back(); }};
            if (a.first <= b.first && a.second <= b.second) { rec(rec, a); }
            return res;};
        vector<int> perm(k);
        iota(perm.begin(), perm.end(), 0);
        lng total = 0;
        do {
            int sign = 1;
            for (int i = 0; i < k; ++i) { for (int j = i + 1; j < k; ++j) { if (perm[i] > perm[j]) { sign = -sign; } } }
            vector<vector<vector<pair<int, int>>>> ps(k);
            for (int i = 0; i < k; ++i) { ps[i] = paths(s[i], t[perm[i]]); }
            vector<int> idx(k, 0);
            auto rec = [&](auto &&self, int i, set<pair<int, int>> &used) -> void {
                if (i == k) { total += sign; return; }
                for (auto &p : ps[i]) {
                    bool ok = true;
                    for (auto &q : p) { if (used.count(q)) { ok = false; break; } }
                    if (!ok) { continue; }
                    for (auto &q : p) { used.insert(q); }
                    self(self, i + 1, used);
                    for (auto &q : p) { used.erase(q); }}};
            set<pair<int, int>> used;
            rec(rec, 0, used);} while (std::next_permutation(perm.begin(), perm.end()));
        context = "k=" + std::to_string(k) + " r=" + std::to_string(r);
        need(nonIntersectingPathsDeterminant(s, t, pc) == mint(total), "nonIntersectingPathsDeterminant");}}

int main(int argc, char **argv) {
    string mode = "full", invalid;
    ulng seed = 20261019;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else if (arg == "--oracle") {
            // Rows of named counts; output one residue mod 998244353 per row.
            ModFac pc(200000);
            string op;
            while (cin >> op) {
                if (op == "motzkin") { int n; cin >> n; cout << motzkinTable(n)[n] << ' ' << motzkinPaths(n, 0, 0, pc) << '\n'; }
                else if (op == "schroder") { int n; cin >> n; cout << schroderLarge(n)[n] << ' ' << schroderSmall(n)[n] << '\n'; }
                else if (op == "delannoy") { int n; cin >> n; cout << delannoyCentral(n)[n] << ' ' << delannoy(n, n, pc) << '\n'; }
                else if (op == "narayana") { int n, k; cin >> n >> k; cout << narayana(n, k, pc) << '\n'; }
                else if (op == "catconv") { int n, k; cin >> n >> k; cout << catalanConvolution(n, k, pc) << '\n'; }
                else if (op == "ballot") { int c, d, mu, st; cin >> c >> d >> mu >> st; cout << ballotGeneral(c, d, mu, st, pc) << '\n'; }
                else if (op == "bizley") { int a, b, n; cin >> a >> b >> n; cout << bizleyPaths(a, b, n)[n] << '\n'; }
                else if (op == "dyck") { int n, a, b; cin >> n >> a >> b; cout << dyckPaths(n, a, b, pc) << '\n'; }
                else if (op == "strip") { int x1, y1, x2, y2, lo, hi; cin >> x1 >> y1 >> x2 >> y2 >> lo >> hi; cout << pathsBetweenLines(x1, y1, x2, y2, lo, hi, pc) << '\n'; }
                else if (op == "line") { int a, b, c, d, mu; cin >> a >> b >> c >> d >> mu; cout << pathsBelowLine(a, b, c, d, mu, pc) << '\n'; }}
            return 0;}}
    if (!invalid.empty()) {
        ModFac pc(20);
        if (invalid == "grid-table") { gridPaths(15, 15, pc); }
        else if (invalid == "dyck-negative") { dyckPaths(-1, 0, 0, pc); }
        else if (invalid == "ballot-negative") { ballotGeneral(3, -1, 1, false, pc); }
        else if (invalid == "convolution-table") { catalanConvolution(10, 5, pc); }
        else if (invalid == "line-mu") { pathsBelowLine(0, 0, 3, 1, -1, pc); }
        else if (invalid == "strip-order") { pathsBetweenLines(0, 0, 2, 2, 1, 0, pc); }
        else if (invalid == "motzkin-table") { motzkinPaths(21, 0, 0, pc); }
        else if (invalid == "transfer-table") { gridPathsBoundaryTransfer(vector<mint>(15), vector<mint>(15), pc); }
        else if (invalid == "sequences-size") { increasingSequencesBetweenBounds(vector<int>{0, 1}, vector<int>{3}); }
        else if (invalid == "sequences-negative") { increasingSequencesBetweenBounds(vector<int>{-1}, vector<int>{3}); }
        else if (invalid == "bizley-zero") { bizleyPaths(0, 1, 3); }
        else if (invalid == "lgv-size") { nonIntersectingPathsDeterminant(vector<pair<int, int>>{{0, 0}}, vector<pair<int, int>>{}, pc); }
        else if (invalid == "table-negative") { motzkinTable(-1); }
        else { return 2; }
        return 0;}
    bool quick = mode == "quick", stress = mode == "stress";
    std::mt19937_64 rng(seed);
    ModFac pc(20000);
    small(pc, rng, quick);
    randomized(pc, rng, quick ? 200 : stress ? 20000 : 2000, quick ? 8 : 14);
    cout << "PASS obstacles, rectangles and boundary flows vs DP checks=" << checks << '\n';
    sequences(rng, quick ? 300 : stress ? 30000 : 3000, 12, 12);
    sequences(rng, quick ? 4 : stress ? 60 : 12, quick ? 200 : 1500, quick ? 200 : 1500);
    cout << "PASS increasingSequencesBetweenBounds vs prefix-sum DP checks=" << checks << '\n';
    lgv(pc, rng, quick ? 30 : stress ? 600 : 150);
    cout << "PASS LGV determinant vs signed enumeration of disjoint systems checks=" << checks << '\n';
    // Any ring (ulng wraps modulo 2^64) and a second prime for the recurrence tables.
    using P2 = ModInt<1000003>;
    auto tri = catalanTriangle<ulng>(70);
    auto m2 = motzkinTable<P2>(80), s2 = schroderLarge<P2>(80), q2 = schroderSmall<P2>(80), d2 = delannoyCentral<P2>(80);
    for (int i = 0; i <= 70; ++i) {
        context = str({i});
        need(tri[i][i / 2] == gridDp<ulng>(0, 0, i, i / 2, [](int x, int y) { return x >= y; }), "catalanTriangle ulng");}
    for (int n = 0; n <= 80; ++n) {
        context = str({n});
        need(m2[n] == walkDp<P2>(n, 0, 0, {-1, 0, 1}, 100), "motzkinTable second prime");
        need(s2[n] == gridDp<P2>(0, 0, n, n, [](int x, int y) { return y <= x; }, true) && q2[n] * 2 == s2[n] + P2(n == 0), "schroder second prime");
        need(d2[n] == gridDp<P2>(0, 0, n, n, [](int, int) { return true; }, true), "delannoyCentral second prime");}
    cout << "PASS second prime and any-ring tables checks=" << checks << '\n';
    // The 64-bit prime 2^64 - 59 through the convolution-based paths.
    using W = ModInt64<18446744073709551557ULL>;
    for (int r = 0; r < (quick ? 20 : 120); ++r) {
        int n = 1 + int(rng() % 80), m = 2 + int(rng() % 80);
        vector<int> a(n), b(n);
        for (int i = 0; i < n; ++i) { a[i] = int(rng() % ulng(m - 1)); b[i] = a[i] + 1 + int(rng() % ulng(m - a[i])); }
        context = "64-bit n=" + std::to_string(n) + " m=" + std::to_string(m);
        need(increasingSequencesBetweenBounds<W>(a, b) == sequencesDp<W>(a, b), "increasingSequencesBetweenBounds modulo 2^64 - 59");}
    for (int a = 1; a <= 3; ++a) {
        for (int b = 1; b <= 3; ++b) {
            auto bz = bizleyPaths<W>(a, b, 5);
            for (int i = 0; i <= 5; ++i) {
                context = str({a, b, i});
                need(bz[i] == gridDp<W>(0, 0, a * i, b * i, [&](int x, int y) { return lng(a) * y <= lng(b) * x; }), "bizleyPaths modulo 2^64 - 59");}}}
    PrimeCombinatorics<W> pw(400);
    vector<W> left(150), bottom(170);
    for (auto &v : left) { v = W(rng()); }
    for (auto &v : bottom) { v = W(rng()); }
    auto [right, top] = gridPathsBoundaryTransfer(left, bottom, pw);
    vector<vector<W>> d(170, vector<W>(150, 0));
    for (int x = 0; x < 170; ++x) { for (int y = 0; y < 150; ++y) { d[x][y] = (x ? d[x - 1][y] : left[y]) + (y ? d[x][y - 1] : bottom[x]); } }
    bool ok = true;
    for (int y = 0; y < 150; ++y) { ok = ok && right[y] == d[169][y]; }
    for (int x = 0; x < 170; ++x) { ok = ok && top[x] == d[x][149]; }
    context = "64-bit transfer";
    need(ok, "gridPathsBoundaryTransfer modulo 2^64 - 59");
    cout << "PASS 64-bit prime through the convolution paths checks=" << checks << '\n';}
