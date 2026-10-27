#include "../../02-Data Structures/21-lichao.hpp"

namespace {
    ulng seed = 20261028;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    using Line = LiChaoTree<>::Line;
    constexpr lng LMIN = std::numeric_limits<lng>::min(), LMAX = std::numeric_limits<lng>::max();
    constexpr lng E9 = 1000000000, E18 = 1000000000000000000;
    lng rnd(Rng &rng, lng lo, lng hi) {
        ulng w = ulng(hi) - ulng(lo);
        return lng(ulng(lo) + (w == ~0ULL ? rng() : rng() % (w + 1)));}

    // a (x - p)^2 + c with one a per tree; the reference evaluates it in __int128.
    struct Para {
        lng a, p, c;
        lng eval(lng x) const { return a * (x - p) * (x - p) + c; }
    };
    // 0.5 (x - p)^2 + c with integer p and half-integer c, so every value on the tested domains is an exact double.
    struct Half {
        double p, c;
        double eval(lng x) const { double d = double(x) - p; return 0.5 * d * d + c; }
    };

    string show(const Line &f) { return "{m=" + std::to_string(f.m) + ",b=" + std::to_string(f.b) + "}"; }
    string show(const Para &f) { return "{a=" + std::to_string(f.a) + ",p=" + std::to_string(f.p) + ",c=" + std::to_string(f.c) + "}"; }
    string show(double x) { std::ostringstream s; s << std::setprecision(17) << x; return s.str(); }
    string show(const Half &f) { return "{p=" + show(f.p) + ",c=" + show(f.c) + "}"; }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    template<typename T, typename U> void checkQuery(const T &got, const U &want, const string &op, lng x) {
        if (got == want) { ++checks; }
        else { checkEqual(got, want, op + " query x=" + show(x)); }}

    // Independent references; a value outside (LMIN, LMAX) or an overflowing product means the generator left the documented domain.
    template<typename F> lng exact(lll v, const F &f, lng x) {
        if (v <= LMIN || v >= LMAX) { throw std::runtime_error(context + " generator left the value domain: " + show(f) + " x=" + show(x)); }
        return lng(v);}
    lng ref(const Line &f, lng x) {
        lll p = lll(f.m) * x;
        if (p < LMIN || p > LMAX) { throw std::runtime_error(context + " generator overflows m*x: " + show(f) + " x=" + show(x)); }
        return exact(p + f.b, f, x);}
    lng ref(const Para &f, lng x) { return exact(lll(f.a) * (lll(x) - f.p) * (lll(x) - f.p) + f.c, f, x); }
    double ref(const Half &f, lng x) { double d = double(x) - f.p; return d * d / 2 + f.c; }
    template<typename F> using Val = decltype(ref(std::declval<const F &>(), lng()));
    template<bool MAX, typename T> constexpr T none() { return MAX ? std::numeric_limits<T>::lowest() : std::numeric_limits<T>::max(); }
    template<bool MAX, typename T> bool improves(const T &y, const T &res) { return MAX ? res < y : y < res; }

    // A stored function: a line covers everything, a segment covers [l, r).
    template<typename F> struct Item {
        F f;
        bool line;
        lng l, r;
        bool covers(lng x) const { return line || (l <= x && x < r); }
    };
    template<typename F> string describe(const Item<F> &it) { return it.line ? "addLine f=" + show(it.f) : "addSegment l=" + show(it.l) + " r=" + show(it.r) + " f=" + show(it.f); }
    template<bool MAX, typename F> Val<F> best(const vector<Item<F>> &v, lng x) {
        Val<F> res = none<MAX, Val<F>>();
        // A raw range keeps the checked build's debug iterators out of the brute force's inner loop.
        for (const Item<F> *it = v.data(), *e = it + v.size(); it != e; ++it) {
            if (!it->covers(x)) { continue; }
            Val<F> y = ref(it->f, x);
            if (improves<MAX>(y, res)) { res = y; }}
        return res;}

    // A domain [lo, hi) with a generator whose functions stay exact on what they cover; free: segment ends may move anywhere.
    template<typename F> struct Scene {
        string name;
        lng lo, hi;
        bool free;
        std::function<Item<F>(Rng &, bool)> gen;
        vector<lng> extra;
    };
    template<typename F> void validate(const Scene<F> &sc, const Item<F> &it) {
        lng l = it.line ? sc.lo : max(it.l, sc.lo), r = it.line ? sc.hi : min(it.r, sc.hi);
        if (l >= r) { return; }
        (void)ref(it.f, l); (void)ref(it.f, r - 1);
        if constexpr (std::is_same_v<F, Para>) { (void)ref(it.f, std::clamp(it.f.p, l, r - 1)); }}
    template<typename F> Item<F> draw(Rng &rng, const Scene<F> &sc, bool line) {
        Item<F> it = sc.gen(rng, line);
        validate(sc, it);
        return it;}
    template<typename F> vector<lng> probes(Rng &rng, const Scene<F> &sc, const vector<Item<F>> &v, int q) {
        vector<lng> xs;
        if (ulng(sc.hi) - ulng(sc.lo) <= 64) {
            for (lng x = sc.lo; x < sc.hi; ++x) { xs.push_back(x); }
            return xs;}
        auto add = [&](lng x) { if (sc.lo <= x && x < sc.hi) { xs.push_back(x); }};
        for (int i = 0; i < q; ++i) { xs.push_back(rnd(rng, sc.lo, sc.hi - 1)); }
        add(sc.lo); add(sc.hi - 1); add(std::midpoint(sc.lo, sc.hi)); add(std::midpoint(sc.lo, sc.hi) - 1);
        for (size_t i = v.size() > 3 ? v.size() - 3 : 0; i < v.size(); ++i) {
            if (v[i].line) { continue; }
            add(v[i].l); add(v[i].r);
            if (v[i].l > LMIN) { add(v[i].l - 1); }
            if (v[i].r > LMIN) { add(v[i].r - 1); }}
        return xs;}

    lng segEnd(Rng &rng, lng lo, lng hi) { return rng() % 3 ? rnd(rng, lo, hi) : rng() % 2 ? lo : hi; }
    Scene<Line> tinyLines(Rng &rng, lng len) {
        lng lo = rnd(rng, -20, 20), hi = lo + len;
        return {"lines[" + show(lo) + "," + show(hi) + ")", lo, hi, true, [lo, hi](Rng &g, bool line) {
            lng l = rnd(g, lo - 3, hi + 3);
            return Item<Line>{{rnd(g, -4, 4), rnd(g, -30, 30)}, line, l, g() % 3 ? rnd(g, lo - 3, hi + 3) : l + rnd(g, 0, 3)};}, {}};}
    // Library Checker bounds: |m| <= 1e9, |x| <= 1e9, |b| <= 1e18; a quarter of the functions are small, for ties and equal slopes.
    Scene<Line> checkerLines() {
        lng lo = -E9, hi = E9 + 1;
        return {"checker[-1e9,1e9]", lo, hi, true, [lo, hi](Rng &g, bool line) {
            Line f = g() % 4 ? Line{rnd(g, -E9, E9), rnd(g, -E18, E18)} : Line{rnd(g, -2, 2), rnd(g, -5, 5)};
            lng l = segEnd(g, lo - 5, hi + 5);
            return Item<Line>{f, line, l, g() % 3 ? segEnd(g, lo - 5, hi + 5) : l + rnd(g, 0, 100)};}, {}};}
    // The whole lng range: lines are constants, segments keep m * x + b exact on the part they cover.
    Scene<Line> wideLines() {
        constexpr lng P60 = lng(1) << 60, P62 = lng(1) << 62;
        return {"wide[LMIN,LMAX)", LMIN, LMAX, false, [](Rng &g, bool line) {
            if (line) { return Item<Line>{{0, rnd(g, -P62, P62)}, true, 0, 0}; }
            int k = int(g() % 4);
            if (k == 0) {
                lng l = rnd(g, -P60, P60);
                return Item<Line>{{rnd(g, -3, 3), rnd(g, -P60, P60)}, false, l, g() % 2 ? rnd(g, -P60, P60) : l + rnd(g, 0, 10)};}
            if (k == 1) {
                lng l = g() % 3 ? rnd(g, LMIN, -1) : LMIN;
                return Item<Line>{{1, rnd(g, 1, P62)}, false, l, rnd(g, l, 0)};}
            if (k == 2) {
                lng l = rnd(g, 1, LMAX);
                return Item<Line>{{-1, rnd(g, 0, P62)}, false, l, g() % 3 ? rnd(g, l, LMAX) : LMAX};}
            return Item<Line>{{0, rnd(g, -P62, P62)}, false, g() % 4 ? rnd(g, LMIN, LMAX) : LMIN, g() % 4 ? rnd(g, LMIN, LMAX) : LMAX};}, {LMIN, LMIN + 1, -1, 0, 1, LMAX - 1, LMAX}};}
    // [LMIN, LMIN + len): slopes 0 and 1 only (-x overflows at LMIN); b = d - LMIN is written LMAX + d + 1.
    Scene<Line> lowLines(lng len) {
        lng lo = LMIN, hi = LMIN + len;
        return {"low[LMIN,LMIN+" + show(len) + ")", lo, hi, true, [lo, hi](Rng &g, bool line) {
            Line f = g() % 2 ? Line{0, rnd(g, -100, 100)} : Line{1, LMAX + rnd(g, -100, -1) + 1};
            return Item<Line>{f, line, rnd(g, lo, hi + 3), rnd(g, lo, hi + 3)};}, {}};}
    // [LMAX - len, LMAX): slopes -1, 0, 1 around c = LMAX - 1; b = d - c needs d >= -1.
    Scene<Line> highLines(lng len) {
        lng lo = LMAX - len, hi = LMAX, c = LMAX - 1;
        return {"high[LMAX-" + show(len) + ",LMAX)", lo, hi, true, [lo, hi, c](Rng &g, bool line) {
            int k = int(g() % 3);
            Line f = k == 0 ? Line{0, rnd(g, -100, 100)} : k == 1 ? Line{1, rnd(g, -1, 100) - c} : Line{-1, c + rnd(g, -100, 1)};
            return Item<Line>{f, line, rnd(g, lo - 3, hi), rnd(g, lo - 3, hi)};}, {}};}
    Scene<Para> parabolas(lng lo, lng hi, lng a) {
        return {"parabolas a=" + show(a) + " [" + show(lo) + "," + show(hi) + ")", lo, hi, true, [lo, hi, a](Rng &g, bool line) {
            lng l = rnd(g, lo - 3, hi + 3);
            return Item<Para>{{a, rnd(g, lo - 100, hi + 100), rnd(g, -10000000, 10000000)}, line, l, g() % 3 ? rnd(g, lo - 3, hi + 3) : l + rnd(g, 0, 5)};}, {}};}
    Scene<Half> halves(lng lo, lng hi) {
        return {"double parabolas [" + show(lo) + "," + show(hi) + ")", lo, hi, true, [lo, hi](Rng &g, bool line) {
            lng l = rnd(g, lo - 3, hi + 3);
            return Item<Half>{{double(rnd(g, lo - 100, hi + 100)), double(rnd(g, -4000000, 4000000)) / 2}, line, l, g() % 3 ? rnd(g, lo - 3, hi + 3) : l + rnd(g, 0, 5)};}, {}};}

    struct Sizes { int steps, q, cap, trees, per; };

    // Random histories of addLine, addSegment, merge and (with UNDO) undo, including undo back to empty; oracle: the stored list.
    template<bool MAX, bool UNDO, typename F>
    void treeRandom(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        using Tree = LiChaoTree<MAX, UNDO, F>;
        string name = "LiChaoTree<MAX=" + show(MAX) + ",UNDO=" + show(UNDO) + "> " + sc.name;
        Tree t(sc.lo, sc.hi);
        vector<Item<F>> cur;
        vector<size_t> saves;
        auto check = [&](const Tree &u, const vector<Item<F>> &v, const string &op) {
            for (lng x : probes(rng, sc, v, z.q)) { checkQuery(u.query(x), best<MAX>(v, x), op, x); }};
        auto insert = [&](Tree &u, vector<Item<F>> &v) {
            Item<F> it = draw(rng, sc, rng() % 2 == 0);
            if (it.line) { u.addLine(it.f); }
            else { u.addSegment(it.l, it.r, it.f); }
            v.push_back(it);
            return describe(it);};
        for (int s = 0; s < z.steps; ++s) {
            context = name + " step=" + show(s) + " size=" + show(cur.size());
            int k = int(rnd(rng, 0, 19));
            string op;
            if (k >= 16 && UNDO && !saves.empty()) {
                if constexpr (UNDO) {
                    size_t times = rng() % 8 ? 1 : saves.size();
                    for (size_t i = 0; i < times; ++i) {
                        t.undo();
                        cur.resize(saves.back());
                        saves.pop_back();}
                    op = "undo x" + show(times);}}
            else if (k == 19 || int(cur.size()) > z.cap) {
                if constexpr (UNDO) {
                    while (!saves.empty()) { t.undo(); saves.pop_back(); }
                    cur.clear();
                    op = "undo to empty";}
                else {
                    t = Tree(sc.lo, sc.hi);
                    cur.clear();
                    op = "reset by assignment";}}
            else if (k >= 13) {
                Tree o(sc.lo, sc.hi);
                vector<Item<F>> ov;
                int m = int(rnd(rng, 0, 5));
                for (int i = 0; i < m; ++i) { insert(o, ov); }
                if constexpr (UNDO) {
                    if (rng() % 2 && !ov.empty()) { o.undo(); ov.pop_back(); }}
                Tree before = o;
                saves.push_back(cur.size());
                t.merge(o);
                cur.insert(cur.end(), ov.begin(), ov.end());
                op = "merge of " + show(m) + " calls";
                checkEqual(o.t == before.t && o.fs.size() == before.fs.size(), true, op + " leaves the argument's nodes unchanged");
                check(o, ov, op + " argument");}
            else {
                saves.push_back(cur.size());
                op = insert(t, cur);}
            check(t, cur, op);}
        if constexpr (UNDO) {
            context = name + " final undo";
            while (!saves.empty()) { t.undo(); saves.pop_back(); }
            cur.clear();
            check(t, cur, "undo all");
            checkEqual(t.t.size() == 3 && t.fs.empty() && t.his.empty(), true, "undo all restores the empty tree");
            insert(t, cur);
            check(t, cur, "insert after undo all");}}

    // Small-to-large merging: always merge the tree with fewer calls into the larger after std::swap; oracle: the union.
    template<bool MAX, bool UNDO, typename F>
    void mergeChain(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        using Tree = LiChaoTree<MAX, UNDO, F>;
        string name = "merge chain<MAX=" + show(MAX) + ",UNDO=" + show(UNDO) + "> " + sc.name;
        vector<Tree> ts;
        vector<vector<Item<F>>> vs;
        for (int i = 0; i < z.trees; ++i) {
            ts.emplace_back(sc.lo, sc.hi);
            vs.emplace_back();
            int m = int(rnd(rng, 0, z.per));
            for (int j = 0; j < m; ++j) {
                Item<F> it = draw(rng, sc, rng() % 2 == 0);
                if (it.line) { ts.back().addLine(it.f); }
                else { ts.back().addSegment(it.l, it.r, it.f); }
                vs.back().push_back(it);}}
        while (ts.size() > 1) {
            context = name + " trees=" + show(ts.size());
            size_t a = size_t(rnd(rng, 0, lng(ts.size()) - 1)), b = size_t(rnd(rng, 0, lng(ts.size()) - 2));
            b += b >= a;
            if (vs[a].size() < vs[b].size()) { std::swap(ts[a], ts[b]); std::swap(vs[a], vs[b]); }
            if constexpr (UNDO) {
                if (rng() % 3 == 0) {
                    ts[a].merge(ts[b]);
                    ts[a].undo();
                    for (lng x : probes(rng, sc, vs[a], z.q)) { checkQuery(ts[a].query(x), best<MAX>(vs[a], x), "merge then undo", x); }}}
            ts[a].merge(ts[b]);
            vs[a].insert(vs[a].end(), vs[b].begin(), vs[b].end());
            for (lng x : probes(rng, sc, vs[a], z.q)) { checkQuery(ts[a].query(x), best<MAX>(vs[a], x), "merge", x); }
            ts.erase(ts.begin() + lng(b));
            vs.erase(vs.begin() + lng(b));}}

    // reserve(nodes) leaves the stored nodes alone and holds at least 3 * nodes ints; nodes below the current count must not shrink t.
    template<typename P> void reserveChecked(P &t, int nodes) {
        vector<int> before = t.t;
        t.reserve(nodes);
        checkEqual(t.t.capacity() >= 3 * size_t(nodes), true, "reserve(" + show(nodes) + ") capacity=" + show(t.t.capacity()));
        checkEqual(t.t == before, true, "reserve(" + show(nodes) + ") keeps the nodes");}

    // Versions branch from the latest or a random older one; oracle: the parent chain of stored functions; answers recorded at creation must still hold at the end.
    // reserve runs before the first update and between updates (1/4 of steps, up to past the current node count or below it).
    template<bool MAX, typename F>
    void perRandom(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        string name = "PerLiChaoTree<MAX=" + show(MAX) + "> " + sc.name;
        PerLiChaoTree<MAX, F> t(sc.lo, sc.hi);
        context = name + " reserve";
        if (rng() % 2) { reserveChecked(t, int(rnd(rng, 0, 64 * z.steps))); }
        vector<int> par{-1};
        vector<Item<F>> its(1);
        vector<vector<pair<lng, Val<F>>>> rec(1);
        auto brute = [&](int v, lng x) {
            Val<F> res = none<MAX, Val<F>>();
            for (; v > 0; v = par[size_t(v)]) {
                if (its[size_t(v)].covers(x)) {
                    Val<F> y = ref(its[size_t(v)].f, x);
                    if (improves<MAX>(y, res)) { res = y; }}}
            return res;};
        context = name + " version 0";
        for (lng x : probes(rng, sc, vector<Item<F>>{}, z.q)) {
            checkQuery(t.query(0, x), none<MAX, Val<F>>(), "empty version", x);
            rec[0].emplace_back(x, none<MAX, Val<F>>());}
        for (int s = 0; s < 2 * z.steps; ++s) {
            int n = int(par.size());
            int ver = rng() % 2 ? n - 1 : int(rnd(rng, 0, n - 1));
            Item<F> it = draw(rng, sc, rng() % 2 == 0);
            context = name + " step=" + show(s) + " base=" + show(ver) + " " + describe(it);
            if (rng() % 4 == 0) { reserveChecked(t, int(rnd(rng, 0, lng(t.t.size() / 3) + 256))); }
            int id = it.line ? t.addLine(ver, it.f) : t.addSegment(ver, it.l, it.r, it.f);
            checkEqual(id, n, "returned version");
            par.push_back(ver);
            its.push_back(it);
            rec.emplace_back();
            for (lng x : probes(rng, sc, vector<Item<F>>{it}, z.q)) {
                Val<F> want = brute(n, x);
                checkQuery(t.query(n, x), want, "new version", x);
                rec.back().emplace_back(x, want);}
            int old = int(rnd(rng, 0, n - 1));
            for (lng x : probes(rng, sc, vector<Item<F>>{its[size_t(old)]}, 2)) { checkQuery(t.query(old, x), brute(old, x), "older version " + show(old), x); }}
        context = name + " final";
        for (size_t v = 0; v < rec.size(); ++v) {
            for (auto &[x, want] : rec[v]) { checkEqual(t.query(int(v), x), want, "version " + show(v) + " unchanged at x=" + show(x)); }}}

    // A compressed answer: the optimal value, and an id naming a live stored function covering x that attains it, or {NONE, -1}.
    template<bool MAX, typename F> void checkCompressed(const pair<Val<F>, int> &got, const vector<Item<F>> &cur, lng x, const string &op) {
        auto [val, id] = got;
        Val<F> want = best<MAX>(cur, x);
        bool live = 0 <= id && id < int(cur.size());
        if (want == none<MAX, Val<F>>() ? val == want && id == -1 : val == want && live && cur[size_t(id)].covers(x) && ref(cur[size_t(id)].f, x) == want) { ++checks; return; }
        throw std::runtime_error(context + " operation=" + op + " compressed query x=" + show(x) + " expected=" + show(want) + (want == none<MAX, Val<F>>() ? " id -1" : " with a live covering optimal id")
            + " actual=" + show(got) + (live ? " id function " + describe(cur[size_t(id)]) : ""));}

    // Compressed points: unsorted with duplicates, possibly none; the answer's id must name a live stored function covering x that attains the optimum.
    template<bool MAX, bool UNDO, typename F>
    void compressedRandom(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        using Tree = LiChaoTreeCompressed<MAX, UNDO, F>;
        string name = "LiChaoTreeCompressed<MAX=" + show(MAX) + ",UNDO=" + show(UNDO) + "> " + sc.name;
        int m = rng() % 8 == 0 ? 0 : int(rnd(rng, 1, 48));
        vector<lng> pts;
        for (int i = 0; i < m; ++i) { pts.push_back(i > 0 && rng() % 4 == 0 ? pts[size_t(rnd(rng, 0, i - 1))] : rnd(rng, sc.lo, sc.hi - 1)); }
        if (m > 0) {
            pts.push_back(sc.lo); pts.push_back(sc.hi - 1);
            pts.insert(pts.end(), sc.extra.begin(), sc.extra.end());}
        std::shuffle(pts.begin(), pts.end(), rng);
        Tree t(pts);
        sort(pts.begin(), pts.end());
        pts.erase(unique(pts.begin(), pts.end()), pts.end());
        vector<Item<F>> cur;
        vector<size_t> saves;
        auto check = [&](const string &op) {
            vector<lng> xs;
            if (pts.size() <= 32) { xs = pts; }
            else {
                for (int i = 0; i < z.q; ++i) { xs.push_back(pts[size_t(rnd(rng, 0, lng(pts.size()) - 1))]); }
                xs.push_back(pts.front()); xs.push_back(pts.back());}
            for (lng x : xs) { checkCompressed<MAX>(t.query(x), cur, x, op); }};
        for (int s = 0; s < z.steps; ++s) {
            context = name + " points=" + show(pts.size()) + " step=" + show(s) + " size=" + show(cur.size());
            int k = int(rnd(rng, 0, 19));
            string op;
            if (k >= 16 && UNDO && !saves.empty()) {
                if constexpr (UNDO) {
                    size_t times = rng() % 8 ? 1 : saves.size();
                    for (size_t i = 0; i < times; ++i) {
                        t.undo();
                        cur.resize(saves.back());
                        saves.pop_back();}
                    op = "undo x" + show(times);}}
            else if (int(cur.size()) > z.cap) {
                if constexpr (UNDO) {
                    while (!saves.empty()) { t.undo(); saves.pop_back(); }
                    op = "undo to empty";}
                else {
                    t = Tree(pts);
                    op = "reset by assignment";}
                cur.clear();}
            else {
                Item<F> it = draw(rng, sc, rng() % 2 == 0);
                if (sc.free && !pts.empty() && !it.line) {
                    auto onPoint = [&](lng &e) {
                        lng p = pts[size_t(rnd(rng, 0, lng(pts.size()) - 1))];
                        if (rng() % 2) { e = p; }
                        else if (rng() % 2 && p > LMIN) { e = p - 1; }
                        else if (p < LMAX) { e = p + 1; }};
                    if (rng() % 2) { onPoint(it.l); }
                    if (rng() % 2) { onPoint(it.r); }
                    validate(sc, it);}
                if (it.line) { t.addLine(it.f); }
                else { t.addSegment(it.l, it.r, it.f); }
                saves.push_back(cur.size());
                cur.push_back(it);
                op = describe(it) + " id=" + show(cur.size() - 1);}
            if (!pts.empty()) { check(op); }}
        if constexpr (UNDO) {
            context = name + " final undo";
            while (!saves.empty()) { t.undo(); saves.pop_back(); }
            cur.clear();
            checkEqual(t.fs.empty() && t.his.empty(), true, "undo all empties the log");
            if (!pts.empty()) { check("undo all"); }}}

    template<bool MAX, typename F> void suite(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        treeRandom<MAX, true, F>(rng, sc, z); treeRandom<MAX, false, F>(rng, sc, z);
        mergeChain<MAX, true, F>(rng, sc, z); mergeChain<MAX, false, F>(rng, sc, z);
        perRandom<MAX, F>(rng, sc, z);
        compressedRandom<MAX, true, F>(rng, sc, z); compressedRandom<MAX, false, F>(rng, sc, z);}
    void report(const string &name) { std::cout << "ok " << name << " checks=" << checks << std::endl; }
    template<typename F> void suites(Rng &rng, const Scene<F> &sc, const Sizes &z) {
        suite<false, F>(rng, sc, z);
        suite<true, F>(rng, sc, z);
        report(sc.name);}

    // Every point of a 256-point domain against an incrementally maintained envelope array, Library Checker magnitudes.
    template<bool MAX> void largeDense(Rng &rng, int n) {
        constexpr lng lo = -128, hi = 128;
        LiChaoTree<MAX> t(lo, hi);
        vector<lng> want(size_t(hi - lo), none<MAX, lng>());
        for (int i = 0; i < n; ++i) {
            context = "large dense MAX=" + show(MAX) + " n=" + show(n) + " i=" + show(i);
            Line f{rnd(rng, -E9, E9), rnd(rng, -E18, E18)};
            bool line = rng() % 4 == 0;
            lng l = line ? lo : rnd(rng, lo - 2, hi + 2), r = line ? hi : rnd(rng, lo - 2, hi + 2);
            if (line) { t.addLine(f); }
            else { t.addSegment(l, r, f); }
            for (lng x = max(l, lo); x < min(r, hi); ++x) {
                lng y = ref(f, x);
                lng &w = want[size_t(x - lo)];
                if (improves<MAX>(y, w)) { w = y; }}
            if ((i + 1) % max(n / 4, 1) == 0) {
                for (lng x = lo; x < hi; ++x) { checkQuery(t.query(x), want[size_t(x - lo)], "dense", x); }}}}

    // n random calls on the Library Checker domain, then a merge of n / 2 calls, its undo and undo of half the calls; sampled points against the list.
    template<bool MAX> void largeSparse(Rng &rng, int n, int q) {
        Scene<Line> sc = checkerLines();
        LiChaoTree<MAX, true> t(sc.lo, sc.hi), o(sc.lo, sc.hi);
        vector<Item<Line>> cur, ov;
        auto fill = [&](LiChaoTree<MAX, true> &u, vector<Item<Line>> &v, int m) {
            for (int i = 0; i < m; ++i) {
                Item<Line> it = draw(rng, sc, rng() % 2 == 0);
                if (it.line) { u.addLine(it.f); }
                else { u.addSegment(it.l, it.r, it.f); }
                v.push_back(it);}};
        auto check = [&](const string &op) {
            for (lng x : probes(rng, sc, cur, q)) { checkQuery(t.query(x), best<MAX>(cur, x), op, x); }};
        context = "large sparse MAX=" + show(MAX) + " n=" + show(n);
        fill(t, cur, n);
        check("insert");
        fill(o, ov, n / 2);
        t.merge(o);
        vector<Item<Line>> keep = cur;
        cur.insert(cur.end(), ov.begin(), ov.end());
        check("merge");
        t.undo();
        cur = keep;
        check("undo merge");
        for (int i = 0; i < n / 2; ++i) { t.undo(); cur.pop_back(); }
        check("undo half");}

    // A long version chain with random branches (5% segments); random (version, x) pairs against the parent chain.
    template<bool MAX> void largePer(Rng &rng, int n, int q) {
        Scene<Line> sc = checkerLines();
        PerLiChaoTree<MAX> t(sc.lo, sc.hi);
        vector<int> par{-1};
        vector<Item<Line>> its(1);
        context = "large persistent MAX=" + show(MAX) + " n=" + show(n);
        // A line update copies one node per level, at most 33 on the 2^31-wide domain; segments (5%) may outgrow the reservation.
        reserveChecked(t, 40 * n);
        const int *data = t.t.data();
        for (int i = 0; i < n; ++i) {
            int ver = rng() % 10 ? i : int(rnd(rng, 0, i));
            Item<Line> it = draw(rng, sc, rng() % 20 != 0);
            int id = it.line ? t.addLine(ver, it.f) : t.addSegment(ver, it.l, it.r, it.f);
            checkEqual(id, i + 1, "returned version");
            par.push_back(ver);
            its.push_back(it);}
        checkEqual(t.t.data() == data || t.t.size() > 120 * size_t(n), true, "reserve(40 n) avoids reallocation within it, size=" + show(t.t.size()));
        for (int k = 0; k < q; ++k) {
            int v = k < 4 ? n - k : int(rnd(rng, 0, n));
            lng x = rnd(rng, sc.lo, sc.hi - 1);
            lng want = none<MAX, lng>();
            for (int u = v; u > 0; u = par[size_t(u)]) {
                if (its[size_t(u)].covers(x)) {
                    lng y = ref(its[size_t(u)].f, x);
                    if (improves<MAX>(y, want)) { want = y; }}}
            checkQuery(t.query(v, x), want, "version " + show(v), x);}}

    // n points (duplicates) and n calls with occasional undo; sampled points, value and id.
    template<bool MAX> void largeCompressed(Rng &rng, int n, int q) {
        Scene<Line> sc = checkerLines();
        vector<lng> pts;
        for (int i = 0; i < n; ++i) { pts.push_back(i > 0 && rng() % 8 == 0 ? pts[size_t(rnd(rng, 0, i - 1))] : rnd(rng, sc.lo, sc.hi - 1)); }
        LiChaoTreeCompressed<MAX, true> t(pts);
        vector<Item<Line>> cur;
        context = "large compressed MAX=" + show(MAX) + " n=" + show(n);
        for (int i = 0; i < n; ++i) {
            if (!cur.empty() && rng() % 10 == 0) {
                t.undo();
                cur.pop_back();
                continue;}
            Item<Line> it = draw(rng, sc, rng() % 2 == 0);
            if (it.line) { t.addLine(it.f); }
            else { t.addSegment(it.l, it.r, it.f); }
            cur.push_back(it);}
        for (int k = 0; k < q; ++k) {
            lng x = pts[size_t(rnd(rng, 0, n - 1))];
            checkCompressed<MAX>(t.query(x), cur, x, "large");}}

    void edges() {
        context = "edges";
        // Line::eval against __int128, including the Library Checker extremes.
        checkEqual(Line{3, -4}.eval(5), lng(11), "Line eval");
        checkEqual(Line{-E9, E18}.eval(E9), lng(0), "Line eval extreme");
        checkEqual(Line{E9, -E18}.eval(-E9), -2 * E18, "Line eval most negative");
        Rng rng(seed ^ 17);
        for (int i = 0; i < 1000; ++i) {
            Line f{rnd(rng, -E9, E9), rnd(rng, -E18, E18)};
            lng x = rnd(rng, -E9, E9);
            checkEqual(f.eval(x), ref(f, x), "Line eval random " + show(f) + " x=" + show(x));}
        // The reference itself on a hand-checked list.
        vector<Item<Line>> hand{{{1, 0}, true, 0, 0}, {{-1, 0}, false, 0, 5}};
        checkEqual(best<false>(hand, 2), lng(-2), "reference min inside the segment");
        checkEqual(best<false>(hand, 5), lng(5), "reference min at the segment end");
        checkEqual(best<true>(hand, -3), lng(-3), "reference max outside the segment");
        checkEqual(best<true>(vector<Item<Line>>{}, 0), LMIN, "reference empty max");
        // Sentinels.
        checkEqual(LiChaoTree<>::NONE, LMAX, "min NONE"); checkEqual(LiChaoTree<true>::NONE, LMIN, "max NONE");
        checkEqual(PerLiChaoTree<>::NONE, LMAX, "persistent min NONE"); checkEqual(PerLiChaoTree<true>::NONE, LMIN, "persistent max NONE");
        checkEqual(LiChaoTreeCompressed<>::NONE, LMAX, "compressed min NONE"); checkEqual(LiChaoTreeCompressed<true>::NONE, LMIN, "compressed max NONE");
        checkEqual(LiChaoTree<false, false, Half>::NONE, std::numeric_limits<double>::max(), "double min NONE");
        checkEqual(LiChaoTree<true, true, Half>::NONE, std::numeric_limits<double>::lowest(), "double max NONE");
        // Empty structures.
        LiChaoTree<> e(-5, 5);
        for (lng x = -5; x < 5; ++x) { checkEqual(e.query(x), LMAX, "empty tree query"); }
        PerLiChaoTree<true> pe(LMIN, LMAX);
        checkEqual(pe.query(0, LMIN), LMIN, "empty persistent query LMIN"); checkEqual(pe.query(0, LMAX - 1), LMIN, "empty persistent query LMAX-1");
        LiChaoTreeCompressed<> ce({4, -2, 4});
        checkEqual(ce.query(4), pair<lng, int>{LMAX, -1}, "empty compressed query");
        // No points: insertions and undo still work.
        LiChaoTreeCompressed<true, true> none0(vector<lng>{});
        none0.addLine({1, 2}); none0.addSegment(0, 5, {3, 4}); none0.undo();
        checkEqual(none0.fs.size(), size_t(1), "no points undo"); none0.undo();
        // Singleton domain at the top of lng; an empty (clipped) segment is still one undo record.
        LiChaoTree<false, true> s(LMAX - 1, LMAX);
        s.addLine({-1, LMAX - 1});
        checkEqual(s.query(LMAX - 1), lng(0), "singleton line");
        s.addSegment(LMAX - 1, LMAX, {0, -3});
        checkEqual(s.query(LMAX - 1), lng(-3), "singleton segment");
        s.addSegment(LMIN, LMAX - 1, {0, -10});
        checkEqual(s.query(LMAX - 1), lng(-3), "segment clipped to nothing");
        s.undo();
        checkEqual(s.query(LMAX - 1), lng(-3), "undo of the empty segment");
        s.undo();
        checkEqual(s.query(LMAX - 1), lng(0), "undo of the segment");
        s.undo();
        checkEqual(s.query(LMAX - 1), LMAX, "undo of the line");
        PerLiChaoTree<> ps(LMIN, LMIN + 1);
        int v1 = ps.addLine(0, {1, 5}), v2 = ps.addSegment(v1, 3, 9, {0, 0}), v3 = ps.addSegment(v1, LMIN, LMIN + 1, {0, LMIN + 2});
        checkEqual(pair<int, int>{v2, v3}, pair<int, int>{2, 3}, "persistent version ids");
        checkEqual(ps.query(v1, LMIN), LMIN + 5, "persistent singleton line");
        checkEqual(ps.query(v2, LMIN), LMIN + 5, "persistent empty segment");
        checkEqual(ps.query(v3, LMIN), LMIN + 2, "persistent singleton segment");
        // Compressed ids: half-open segments by x value, outside and between points, and id reuse after undo.
        LiChaoTreeCompressed<false, true> c({3, 1, 3, 7});
        c.addLine({0, 10});
        c.addSegment(2, 5, {0, 5});
        checkEqual(c.query(3), pair<lng, int>{5, 1}, "compressed segment between points");
        checkEqual(c.query(1), pair<lng, int>{10, 0}, "compressed line");
        c.undo();
        checkEqual(c.query(3), pair<lng, int>{10, 0}, "compressed undo");
        c.addSegment(5, 8, {0, 1});
        checkEqual(c.query(7), pair<lng, int>{1, 1}, "compressed id reused after undo");
        c.addSegment(8, 100, {0, 0}); c.addSegment(-100, 1, {0, -1});
        checkEqual(c.query(1), pair<lng, int>{10, 0}, "compressed segments covering no point");
        c.addSegment(1, 2, {0, -1});
        checkEqual(c.query(1), pair<lng, int>{-1, 4}, "compressed segment on one point");
        // Identical lines.
        LiChaoTree<true> d(0, 4);
        LiChaoTreeCompressed<true> dc({0, 1, 2, 3});
        for (int i = 0; i < 3; ++i) { d.addLine({2, 3}); dc.addLine({2, 3}); }
        for (lng x = 0; x < 4; ++x) {
            checkEqual(d.query(x), 2 * x + 3, "identical lines");
            auto [val, id] = dc.query(x);
            checkEqual(val == 2 * x + 3 && 0 <= id && id < 3, true, "identical lines compressed");}
        // Merges of empty trees, merge then undo.
        LiChaoTree<true, true> a(0, 8), b(0, 8);
        a.merge(b);
        checkEqual(a.query(3), LMIN, "merge of empty trees");
        b.addSegment(2, 5, {1, 0});
        a.merge(b);
        checkEqual(pair<lng, lng>{a.query(3), a.query(6)}, pair<lng, lng>{3, LMIN}, "merge of a segment");
        b.merge(a);
        checkEqual(b.query(4), lng(4), "merge into the source tree");
        a.undo();
        checkEqual(a.query(3), LMIN, "undo merge");
        a.undo();
        checkEqual(a.t.size() == 3 && a.his.empty(), true, "undo the empty merge");}

    void invalid(const string &name) {
        LiChaoTree<false, true> t(0, 10);
        PerLiChaoTree<> p(0, 10);
        LiChaoTreeCompressed<false, true> c(vector<lng>{1, 5, 9});
        if (name == "tree-domain-empty") { LiChaoTree<> u(3, 3); }
        else if (name == "tree-domain-inverted") { LiChaoTree<true> u(4, 3); }
        else if (name == "per-domain-empty") { PerLiChaoTree<> u(LMAX, LMAX); }
        else if (name == "tree-query-below") { (void)t.query(-1); }
        else if (name == "tree-query-above") { (void)t.query(10); }
        else if (name == "per-query-below") { (void)p.query(0, -1); }
        else if (name == "per-query-above") { (void)p.query(0, 10); }
        else if (name == "tree-merge-domain") { LiChaoTree<false, true> u(0, 11); t.merge(u); }
        else if (name == "tree-merge-self") { t.addLine({1, 2}); t.merge(t); }
        else if (name == "tree-undo-empty") { t.undo(); }
        else if (name == "tree-undo-exhausted") { t.addLine({1, 2}); t.undo(); t.undo(); }
        else if (name == "compressed-undo-empty") { c.undo(); }
        else if (name == "compressed-undo-exhausted") { c.addLine({1, 2}); c.undo(); c.undo(); }
        else if (name == "per-addline-version") { (void)p.addLine(1, {1, 2}); }
        else if (name == "per-addline-negative") { (void)p.addLine(-1, {1, 2}); }
        else if (name == "per-addsegment-version") { (void)p.addSegment(1, 0, 5, {1, 2}); }
        else if (name == "per-query-version") { (void)p.addLine(0, {1, 2}); (void)p.query(2, 3); }
        else if (name == "compressed-query-nonpoint") { c.addLine({1, 2}); (void)c.query(4); }
        else if (name == "compressed-query-above") { (void)c.query(10); }
        else if (name == "compressed-query-empty") { LiChaoTreeCompressed<> u(vector<lng>{}); (void)u.query(0); }
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
        std::cout << "lichao seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        bool quick = mode == "quick", full = mode == "full";
        Sizes z = quick ? Sizes{40, 6, 30, 6, 4} : full ? Sizes{300, 10, 100, 24, 8} : Sizes{1000, 16, 250, 64, 12};
        int rounds = quick ? 2 : full ? 10 : 16, large = quick ? 2000 : full ? 200000 : 400000, q = quick ? 30 : full ? 60 : 60;
#ifdef _GLIBCXX_DEBUG
        // Debug lower_bound checks partitioning in O(m) per call, so the compressed tree's point lookups are linear in the checked build.
        int largeCompressedN = min(large, 5000);
#else
        int largeCompressedN = large;
#endif
        edges();
        report("edges");
        suites(rng, tinyLines(rng, 1), z);
        for (int r = 0; r < rounds; ++r) { suites(rng, tinyLines(rng, rnd(rng, 2, 40)), z); }
        suites(rng, checkerLines(), z);
        suites(rng, wideLines(), z);
        suites(rng, lowLines(1), z); suites(rng, lowLines(rnd(rng, 2, 64)), z);
        suites(rng, highLines(1), z); suites(rng, highLines(rnd(rng, 2, 64)), z);
        for (int r = 0; r < max(rounds / 2, 1); ++r) {
            lng lo = rnd(rng, -50, 50);
            suites(rng, parabolas(lo, lo + rnd(rng, 1, 60), rnd(rng, -3, 3)), z);}
        suites(rng, parabolas(-3000, 3000, rnd(rng, 1, 3)), z); suites(rng, parabolas(-3000, 3000, -rnd(rng, 1, 3)), z);
        for (int r = 0; r < max(rounds / 2, 1); ++r) {
            lng lo = rnd(rng, -50, 50);
            suites(rng, halves(lo, lo + rnd(rng, 1, 60)), z);}
        suites(rng, halves(-1000, 1000), z);
        largeDense<false>(rng, large); largeDense<true>(rng, large);
        report("large dense n=" + show(large));
        largeSparse<false>(rng, large, q); largeSparse<true>(rng, large, q);
        report("large sparse n=" + show(large));
        largePer<false>(rng, large / 2, q); largePer<true>(rng, large / 2, q);
        report("large persistent n=" + show(large / 2));
        largeCompressed<false>(rng, largeCompressedN, q); largeCompressed<true>(rng, largeCompressedN, q);
        report("large compressed n=" + show(largeCompressedN));
        std::cout << "PASS lichao checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL lichao seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
