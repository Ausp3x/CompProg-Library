#include "../../02-Data Structures/04-sparsetable.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    int checks = 0;
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (got != want) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    // Semigroups with S and op only: the tables never call e.
    struct Minimum { using S = lng; static S op(S a, S b) { return std::min(a, b); } };
    struct Maximum { using S = lng; static S op(S a, S b) { return std::max(a, b); } };
    struct Add { using S = lng; static S op(S a, S b) { return a + b; } };
    struct Join { using S = string; static S op(const S &a, const S &b) { return a + b; } };
    using Table = SparseTable<Minimum>;

    // Direct scan folds from the start outward (left to right for maxRight); each pred call is counted and must see a fold of a nonempty range.
    template<typename M, typename G>
    void scanCheck(const SparseTable<M> &t, const vector<typename M::S> &a, const G &pred, const string &op, bool right) {
        using S = typename M::S;
        int n = int(a.size()), bound = int(std::bit_width(uint(n)));
        for (int s = 0; s <= n; ++s) {
            vector<S> seen; int len = -1, steps = right ? n - s : s;
            for (int k = 0; k < steps; ++k) {
                const S &x = a[right ? s + k : s - 1 - k];
                seen.push_back(seen.empty() ? x : right ? t.m.op(seen.back(), x) : t.m.op(x, seen.back()));
                if (len < 0 && !pred(seen.back())) { len = k; }}
            if (len < 0) { len = steps; }
            int calls = 0;
            string at = op + (right ? " maxRight(" : " minLeft(") + show(s) + ")";
            auto counted = [&](const S &x) {
                ++calls;
                if constexpr (std::equality_comparable<S>) {
                    if (std::find(seen.begin(), seen.end(), x) == seen.end()) { throw std::runtime_error(context + " operation=" + at + " pred argument is no fold of a nonempty range"); }}
                return pred(x);};
            checkEqual(right ? t.maxRight(s, counted) : t.minLeft(s, counted), right ? s + len : s - len, at);
            checkEqual(calls <= bound, true, at + " pred calls=" + show(calls) + " <= bit_width(n)");}}
    template<typename M, typename G>
    void bothScans(const SparseTable<M> &t, const vector<typename M::S> &a, const G &pred, const string &op) {
        scanCheck(t, a, pred, op, true); scanCheck(t, a, pred, op, false);}

    void numeric(const vector<lng> &a, bool sums = true) {
        int n = int(a.size()); context = "array=" + show(a);
        Table t(a); SparseTable<Maximum> mx(a);
        SparseTable<Add> sum(a);
        checkEqual(t.n, n, "size"); checkEqual(t.h, int(std::bit_width(uint(n))), "height");
        for (int l = 0; l < n; ++l) {
            lng low = a[l], high = a[l], total = 0;
            for (int r = l + 1; r <= n; ++r) {
                low = std::min(low, a[r - 1]); high = std::max(high, a[r - 1]);
                string range = "(" + show(l) + "," + show(r) + ")";
                checkEqual(t.prod(l, r), low, "prod" + range);
                checkEqual(t.associativeProd(l, r), low, "associativeProd" + range);
                checkEqual(mx.prod(l, r), high, "maximum prod" + range);
                if (sums) {
                    total += a[r - 1];
                    checkEqual(sum.associativeProd(l, r), total, "associative sum associativeProd" + range);}}}
        Table copied = t, moved = std::move(copied);
        if (n) { checkEqual(moved.prod(0, n), *std::min_element(a.begin(), a.end()), "copy/move"); }
        else {
            std::ostringstream s; s << t;
            checkEqual(s.str(), string("[]"), "empty output");}
        std::ostringstream got, want;
        got << t;
        if (!n) { want << "[]"; }
        else {
            want << "\n[\n";
            for (int h = t.h - 1; h >= 0; --h) {
                want << "  [";
                for (int l = 0; l <= n - (1 << h); ++l) {
                    lng low = a[l];
                    for (int r = l + 1; r < l + (1 << h); ++r) { low = std::min(low, a[r]); }
                    want << "[" << l << ", " << l + (1 << h) - 1 << "]: " << low
                         << (l < n - (1 << h) ? ", " : "");}
                want << "]" << (h ? ",\n" : "\n");}
            want << "]\n";}
        checkEqual(got.str(), want.str(), "legacy diagnostic output");}

    // Sums run on a shifted to nonnegative values so sum thresholds are monotone; min/max thresholds run on a itself.
    void numericSearches(const vector<lng> &a, const vector<lng> &caps, const vector<lng> &limits) {
        vector<lng> b(a);
        lng base = a.empty() ? 0 : *std::min_element(a.begin(), a.end());
        for (lng &x : b) { x -= base; }
        context = "search array=" + show(a) + " shifted=" + show(b);
        SparseTable<Add> sum(b); Table low(a); SparseTable<Maximum> high(a);
        for (lng c : caps) {
            bothScans(sum, b, [c](lng x) { return x <= c; }, "sum<=" + show(c));
            bothScans(sum, b, [c](lng x) { return x < c; }, "sum<" + show(c));}
        for (lng y : limits) {
            bothScans(low, a, [y](lng x) { return x >= y; }, "min>=" + show(y));
            bothScans(high, a, [y](lng x) { return x <= y; }, "max<=" + show(y));}
        bothScans(low, a, [](lng) { return false; }, "always false");
        bothScans(low, a, [](lng) { return true; }, "always true");}

    vector<lng> randomCaps(std::mt19937_64 &rng, const vector<lng> &a) {
        lng base = a.empty() ? 0 : *std::min_element(a.begin(), a.end()), total = 0;
        for (lng x : a) { total += x - base; }
        vector<lng> caps{-1, 0, total - 1, total, total + 1};
        for (int i = 0; i < 6; ++i) { caps.push_back(lng(rng() % ulng(total + 1))); }
        return caps;}
    vector<lng> randomLimits(std::mt19937_64 &rng, const vector<lng> &a) {
        vector<lng> limits{std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()};
        for (int i = 0; i < 4 && !a.empty(); ++i) { limits.push_back(a[rng() % a.size()]); }
        for (int i = 0; i < 2; ++i) { limits.push_back(lng(rng() % 2000000001) - 1000000000); }
        return limits;}

    void exhaustive() {
        int limit = mode == "quick" ? 4 : mode == "full" ? 7 : 9;
        for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &v : a) { v = x % 3 - 1; x /= 3; }
                numeric(a);
                vector<lng> caps;
                for (lng c = -1; c <= 2 * n + 1; ++c) { caps.push_back(c); }
                numericSearches(a, caps, {-2, -1, 0, 1, 2});}}
        numeric({std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()});
        numeric({17});
        std::cout << "PASS bounded exhaustive idempotent/associative/legacy/output tests and maxRight/minLeft threshold scans\n";}

    void randomized(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 80 : 300;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = int(rng() % 150); vector<lng> a(n);
            for (lng &x : a) { x = lng(rng() % 2000000001) - 1000000000; }
            numeric(a); numericSearches(a, randomCaps(rng, a), randomLimits(rng, a));
            if (n) {
                context = "trial=" + show(trial) + " original=" + show(a);
                Table t(a); lng low = *std::min_element(a.begin(), a.end());
                a.assign(n, 7000000001LL);
                checkEqual(t.prod(0, n), low, "input ownership");}}
        for (int n : {1, 2, 3, 4, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129}) {
            vector<lng> a(n); std::iota(a.begin(), a.end(), lng(-n)); numeric(a);
            std::reverse(a.begin(), a.end()); numeric(a);
            for (lng &x : a) { x = lng(rng() % 101) - 50; }
            numericSearches(a, randomCaps(rng, a), randomLimits(rng, a));
            for (lng &x : a) { x = lng(rng() % 2000000001) - 1000000000; }
            numericSearches(a, randomCaps(rng, a), randomLimits(rng, a));}
        std::cout << "PASS seeded arrays, input ownership, power-of-two boundaries and random maxRight/minLeft\n";}

    struct Affine {
        int a, b;
        int eval(int x) const { return (a * x + b) % 97; }
    };
    struct Compose {
        using S = Affine;
        static S op(Affine x, Affine y) { return {y.a * x.a % 97, (y.a * x.b + y.b) % 97}; }
    };
    // Affine maps tagged with their range; op rejects nonadjacent operands, so search operand order and contiguity are checked.
    struct Piece {
        Affine f; int l, r;
        Piece() = delete;
        Piece(Affine f, int l, int r) : f(f), l(l), r(r) {}
        bool operator==(const Piece &o) const { return f.a == o.f.a && f.b == o.f.b && l == o.l && r == o.r; }
    };
    struct Chain {
        using S = Piece;
        static S op(const Piece &x, const Piece &y) {
            if (x.r != y.l) { throw std::runtime_error(context + " nonadjacent operands [" + show(x.l) + "," + show(x.r) + ")[" + show(y.l) + "," + show(y.r) + ")"); }
            return Piece(Compose::op(x.f, y.f), x.l, y.r);}
    };
    struct Ends {
        int first, last;
        Ends() = delete;
        Ends(int first, int last) : first(first), last(last) {}
    };
    // Rectangular band: associative/idempotent, explicitly noncommutative.
    struct Band { using S = Ends; static S op(Ends a, Ends b) { return Ends(a.first, b.last); } };
    // Runtime state: index of the leftmost smallest key in a caller's array (idempotent, associative).
    struct ArgminByKey {
        vector<lng> key;
        using S = int;
        S op(S i, S j) const { return key[j] < key[i] || (key[j] == key[i] && j < i) ? j : i; }
    };

    void noncommutative(std::mt19937_64 &rng) {
        int rounds = mode == "quick" ? 10 : mode == "full" ? 70 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 45);
            vector<string> a(n); vector<Affine> affine(n); vector<Ends> ends;
            for (int i = 0; i < n; ++i) {
                a[i] = string(1, char('a' + rng() % 5)); affine[i] = {int(rng() % 97), int(rng() % 97)};
                ends.emplace_back(i, n - i);}
            SparseTable<Join> text(a); SparseTable<Compose> composed(affine); SparseTable<Band> band(ends);
            context = "trial=" + show(trial) + " strings=" + show(a);
            vector<string> bits(n); vector<Piece> pieces;
            for (int i = 0; i < n; ++i) { bits[i] = string(1, char('a' + rng() % 2)); pieces.emplace_back(affine[i], i, i + 1); }
            SparseTable<Join> two(bits); SparseTable<Chain> chain(pieces);
            string whole, other;
            for (const string &c : bits) { whole += c; other += char('a' + rng() % 2); }
            string flipped = whole;
            char &c = flipped[rng() % n];
            c = c == 'a' ? 'b' : 'a';
            for (const string &target : {whole, flipped, other}) {
                scanCheck(two, bits, [&](const string &x) { return target.starts_with(x); }, "prefix of " + target, true);
                scanCheck(two, bits, [&](const string &x) { return target.ends_with(x); }, "suffix of " + target, false);}
            for (int k : {0, 1, 2, 3, n / 2, n}) {
                bothScans(text, a, [k](const string &x) { return int(x.size()) <= k; }, "length<=" + show(k));
                bothScans(chain, pieces, [k](const Piece &x) { return x.r - x.l <= k; }, "affine length<=" + show(k));
                bothScans(band, ends, [k](const Ends &x) { return x.first >= k; }, "nondefault l>=" + show(k));
                bothScans(band, ends, [n, k](const Ends &x) { return x.last >= n - k + 1; }, "nondefault r<=" + show(k));}
            for (int l = 0; l < n; ++l) {
                string joined;
                for (int r = l + 1; r <= n; ++r) {
                    joined += a[r - 1]; string range = "(" + show(l) + "," + show(r) + ")";
                    checkEqual(text.associativeProd(l, r), joined, "ordered concatenation" + range);
                    for (int x = 0; x < 3; ++x) {
                        int result = x;
                        for (int i = l; i < r; ++i) { result = affine[i].eval(result); }
                        checkEqual(composed.associativeProd(l, r).eval(x), result, "affine associativeProd" + range + " x=" + show(x));}
                    checkEqual(band.prod(l, r).first, l, "idempotent noncommutative first" + range);
                    checkEqual(band.prod(l, r).last, n - r + 1, "idempotent noncommutative last" + range);
                    checkEqual(band.associativeProd(l, r).first, l, "nondefault payload associativeProd first" + range);
                    checkEqual(band.associativeProd(l, r).last, n - r + 1, "nondefault payload associativeProd last" + range);}}}
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 45);
            vector<lng> key(n); vector<int> ids(n);
            for (int i = 0; i < n; ++i) { key[i] = lng(rng() % 7); ids[i] = i; }
            SparseTable arg(ids, ArgminByKey{key});
            context = "stateful argmin keys=" + show(key);
            for (lng y = 0; y <= 7; ++y) { bothScans(arg, ids, [&key, y](int i) { return key[i] >= y; }, "stateful min key>=" + show(y)); }
            for (int l = 0; l < n; ++l) {
                int best = l;
                for (int r = l + 1; r <= n; ++r) {
                    if (key[r - 1] < key[best]) { best = r - 1; }
                    checkEqual(arg.prod(l, r), best, "stateful prod(" + show(l) + "," + show(r) + ")");
                    checkEqual(arg.associativeProd(l, r), best, "stateful associativeProd(" + show(l) + "," + show(r) + ")");}}}
        std::cout << "PASS strings/affine, idempotent noncommutative generic payloads and stateful monoid, with ordered maxRight/minLeft\n";}

    struct CountedAdd { using S = lng; int *calls; S op(S a, S b) const { ++*calls; return a + b; } };
    struct And { using S = bool; static S op(S a, S b) { return a && b; } };
    // associativeProd joins the popcount(r - l) blocks of the binary decomposition, the fewest, so it calls op popcount(r - l) - 1 times.
    void opCount(std::mt19937_64 &rng) {
        int calls = 0, most = mode == "quick" ? 80 : 300;
        for (int n : {0, 1, 2, 3, 7, 8, 9, 63, 64, 65, most}) {
            vector<lng> a(n); for (lng &x : a) { x = lng(rng() % 1000); }
            SparseTable<CountedAdd> t(a, CountedAdd{&calls});
            context = "op count n=" + show(n);
            checkEqual(int(t.v.size()), t.h, "levels == h");
            for (int l = 0; l < n; ++l) {
                lng sum = 0;
                for (int r = l + 1; r <= n; ++r) {
                    sum += a[r - 1]; calls = 0;
                    string range = "(" + show(l) + "," + show(r) + ")";
                    checkEqual(t.associativeProd(l, r), sum, "counted associativeProd" + range);
                    checkEqual(calls, std::popcount(uint(r - l)) - 1, "associativeProd op calls" + range);}}}
        vector<lng> big((1 << 16) + 3); for (lng &x : big) { x = lng(rng() % 1000000); }
        SparseTable<Minimum> t(big);
        context = "large n=" + show(int(big.size()));
        checkEqual(int(t.v.size()), t.h, "levels == h");
        for (int q = 0; q < (mode == "quick" ? 20 : 200); ++q) {
            int l = int(rng() % big.size()), r = l + 1 + int(rng() % (big.size() - size_t(l)));
            lng low = *std::min_element(big.begin() + l, big.begin() + r);
            string range = "(" + show(l) + "," + show(r) + ")";
            checkEqual(t.prod(l, r), low, "large prod" + range); checkEqual(t.associativeProd(l, r), low, "large associativeProd" + range);}
        std::cout << "PASS associativeProd op calls == popcount(r - l) - 1, levels == h, n=2^16+3\n";}

    // Every pred argument must be a Piece [s, r') or [l', s) of positive length; Chain throws on any other combination.
    void searchDiscipline() {
        for (int n : {0, 1, 2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128, 129}) {
            vector<Piece> pieces;
            for (int i = 0; i < n; ++i) { pieces.emplace_back(Affine{(3 * i + 1) % 97, (5 * i + 2) % 97}, i, i + 1); }
            SparseTable<Chain> chain(pieces);
            context = "pred discipline n=" + show(n);
            for (int k : {0, 1, 2, 5, 8, 33, 64, 129}) { bothScans(chain, pieces, [k](const Piece &x) { return x.r - x.l <= k; }, "length<=" + show(k)); }
            bothScans(chain, pieces, [](const Piece &x) { return x.l < x.r; }, "nonempty");}
        std::cout << "PASS maxRight/minLeft pred-call discipline (calls <= bit_width(n), nonempty folds only)\n";}

    struct Gcd { using S = lng; static S op(S a, S b) { return std::gcd(a, b); } };
    struct Cell {
        lng x;
        Cell() = delete;
        explicit Cell(lng x) : x(x) {}
    };
    struct CellMin { using S = Cell; static S op(const Cell &a, const Cell &b) { return a.x < b.x ? a : b; } };

    void grid(const vector<vector<lng>> &a) {
        int n = int(a.size()), m = n ? int(a[0].size()) : 0;
        context = "grid=" + show(a);
        SparseTable2D<Minimum> low(a); SparseTable2D<Maximum> high(a); SparseTable2D<Gcd> div(a);
        checkEqual(low.rows, n, "2D rows"); checkEqual(low.cols, m, "2D columns");
        for (int x1 = 0; x1 < n; ++x1) {
            vector<lng> mn(a[x1]), mx(a[x1]), g(a[x1]);
            for (int x2 = x1 + 1; x2 <= n; ++x2) {
                if (x2 > x1 + 1) {
                    for (int y = 0; y < m; ++y) {
                        mn[y] = std::min(mn[y], a[x2 - 1][y]); mx[y] = std::max(mx[y], a[x2 - 1][y]); g[y] = std::gcd(g[y], a[x2 - 1][y]);}}
                for (int y1 = 0; y1 < m; ++y1) {
                    lng lo = mn[y1], hi = mx[y1], d = g[y1];
                    for (int y2 = y1 + 1; y2 <= m; ++y2) {
                        lo = std::min(lo, mn[y2 - 1]); hi = std::max(hi, mx[y2 - 1]); d = std::gcd(d, g[y2 - 1]);
                        string rect = "[" + show(x1) + "," + show(x2) + ")x[" + show(y1) + "," + show(y2) + ")";
                        checkEqual(low.prod(x1, y1, x2, y2), lo, "2D min" + rect);
                        checkEqual(high.prod(x1, y1, x2, y2), hi, "2D max" + rect);
                        checkEqual(div.prod(x1, y1, x2, y2), d, "2D gcd" + rect);}}}}
        SparseTable2D copied = low, moved = std::move(copied);
        if (n && m) { checkEqual(moved.prod(0, 0, n, m), low.prod(0, 0, n, m), "2D copy/move"); }}

    void grids(std::mt19937_64 &rng) {
        int cells = mode == "quick" ? 4 : mode == "full" ? 8 : 9;
        for (int n = 0; n <= cells; ++n) {
            for (int m = 0; m <= (n ? cells / n : 3); ++m) {
                int count = 1;
                for (int i = 0; i < n * m; ++i) { count *= 3; }
                for (int code = 0; code < count; ++code) {
                    vector<vector<lng>> a(n, vector<lng>(m)); int x = code;
                    for (auto &row : a) { for (lng &v : row) { v = x % 3 - 1; x /= 3; }}
                    grid(a);}}}
        int rounds = mode == "quick" ? 5 : mode == "full" ? 40 : 150;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 20), m = 1 + int(rng() % 20);
            vector<vector<lng>> a(n, vector<lng>(m));
            for (auto &row : a) { for (lng &v : row) { v = lng(rng() % 2000000001) - 1000000000; }}
            grid(a);
            SparseTable2D<Minimum> t(a); lng low = a[0][0];
            for (const auto &row : a) { low = std::min(low, *std::min_element(row.begin(), row.end())); }
            for (auto &row : a) { fill(row.begin(), row.end(), 7000000001LL); }
            checkEqual(t.prod(0, 0, n, m), low, "2D input ownership");}
        for (auto [n, m] : vector<pair<int, int>>{{1, 1}, {1, 33}, {33, 1}, {8, 9}, {16, 16}, {17, 15}, {2, 64}}) {
            vector<vector<lng>> a(n, vector<lng>(m));
            for (int i = 0; i < n; ++i) { for (int j = 0; j < m; ++j) { a[i][j] = lng((i * 37 + j * 91) % 101) - 50; }}
            grid(a);}
        context = "2D extremes";
        vector<vector<lng>> extreme{{std::numeric_limits<lng>::min(), std::numeric_limits<lng>::max()}, {0, -1}};
        SparseTable2D<Minimum> extremeLow(extreme); SparseTable2D<Maximum> extremeHigh(extreme);
        checkEqual(extremeLow.prod(0, 0, 2, 2), std::numeric_limits<lng>::min(), "2D extreme min");
        checkEqual(extremeHigh.prod(0, 0, 2, 2), std::numeric_limits<lng>::max(), "2D extreme max");
        checkEqual(extremeHigh.prod(1, 0, 2, 2), lng(0), "2D extreme row max");
        context = "2D nondefault payload and copy-only callable";
        SparseTable2D<CellMin> cell(vector<vector<Cell>>{{Cell(5), Cell(2)}, {Cell(4), Cell(9)}});
        checkEqual(cell.prod(0, 0, 2, 2).x, lng(2), "2D nondefault min"); checkEqual(cell.prod(1, 0, 2, 2).x, lng(4), "2D nondefault row");
        vector<lng> key{5, 1, 4, 0};
        SparseTable2D argGrid(vector<vector<int>>{{0, 1}, {2, 3}}, ArgminByKey{key});
        checkEqual(argGrid.prod(0, 0, 1, 2), 1, "2D stateful row"); checkEqual(argGrid.prod(0, 0, 2, 2), 3, "2D stateful grid");
        std::cout << "PASS 2D exhaustive grids n*m<=" << cells << ", seeded grids, boundaries and generic payloads\n";}

    void invalid(const string &name) {
        Table t(vector<lng>{3, 1, 2});
        if (name == "prod-empty") { (void)t.prod(1, 1); }
        else if (name == "prod-negative") { (void)t.prod(-1, 2); }
        else if (name == "prod-end") { (void)t.prod(0, 4); }
        else if (name == "prod-reversed") { (void)t.prod(2, 1); }
        else if (name == "associative-prod-empty") { (void)t.associativeProd(0, 0); }
        else if (name == "associative-prod-negative") { (void)t.associativeProd(-1, 2); }
        else if (name == "associative-prod-end") { (void)t.associativeProd(0, 4); }
        else if (name == "associative-prod-reversed") { (void)t.associativeProd(2, 1); }
        else if (name == "empty-table") { Table e(vector<lng>{}); (void)e.prod(0, 1); }
        else if (name == "max-right-negative") { (void)t.maxRight(-1, [](lng) { return true; }); }
        else if (name == "max-right-end") { (void)t.maxRight(4, [](lng) { return true; }); }
        else if (name == "min-left-negative") { (void)t.minLeft(-1, [](lng) { return true; }); }
        else if (name == "min-left-end") { (void)t.minLeft(4, [](lng) { return true; }); }
        else if (name == "max-right-empty-table") { Table e(vector<lng>{}); (void)e.maxRight(1, [](lng) { return true; }); }
        else if (name == "grid-empty-rows") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(1, 0, 1, 2); }
        else if (name == "grid-empty-columns") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(0, 1, 2, 1); }
        else if (name == "grid-row-end") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(0, 0, 3, 1); }
        else if (name == "oversize") { SparseTable<And> e(vector<bool>(size_t(INT_MAX) + 1)); }
        else if (name == "grid-oversize") { SparseTable2D<And> g(vector<vector<bool>>(2, vector<bool>(size_t(INT_MAX / 2) + 1))); }
        else if (name == "grid-reversed-rows") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(1, 0, 0, 1); }
        else if (name == "grid-reversed-columns") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(0, 1, 1, 0); }
        else if (name == "grid-column-negative") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3, 4}}); (void)g.prod(0, -1, 1, 1); }
        else if (name == "grid-ragged-empty-first") { SparseTable2D<Minimum> g(vector<vector<lng>>{{}, {1, 2}}); }
        else if (name == "grid-ragged") { SparseTable2D<Minimum> g(vector<vector<lng>>{{1, 2}, {3}}); }
        else if (name == "grid-zero-width") { SparseTable2D<Minimum> g(vector<vector<lng>>(2)); (void)g.prod(0, 0, 1, 1); }
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
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "sparsetable seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        std::mt19937_64 rng(seed);
        exhaustive(); randomized(rng); noncommutative(rng); searchDiscipline(); opCount(rng); grids(rng);
        std::cout << "PASS sparsetable checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL sparsetable seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
