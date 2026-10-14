#include "../../02-Data Structures/33-offline_rectangle_queries.hpp"

namespace {
    ulng seed = 20261014;
    string mode = "full", context;
    lng checks = 0, adds = 0;
    constexpr ulng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) {
        ulng span = ulng(hi) - ulng(lo);
        return lng(ulng(lo) + (span == ~0ULL ? ulng(rng()) : ulng(rng()) % (span + 1)));}
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Integers mod P built from any lng; += counts the Fenwick additions.
    struct Mod {
        ulng v = 0;
        Mod() = default;
        Mod(lng x) : v(ulng(x % lng(P) + lng(P)) % P) {}
        Mod operator+(const Mod &o) const { Mod r; r.v = (v + o.v) % P; return r; }
        Mod operator-(const Mod &o) const { Mod r; r.v = (v + P - o.v) % P; return r; }
        Mod operator*(const Mod &o) const { Mod r; r.v = v * o.v % P; return r; }
        Mod &operator+=(const Mod &o) { ++adds; v = (v + o.v) % P; return *this; }
        bool operator==(const Mod &o) const { return v == o.v; }
        friend ostream &operator<<(ostream &os, const Mod &m) { return os << m.v; }
    };
    Mod fromWide(lll x) { return Mod(lng(x % lll(P))); }
    // Exact integers whose += records the largest magnitude a Fenwick cell reaches.
    lll peak = 0;
    struct Peak {
        lll v = 0;
        Peak() = default;
        Peak(lng x) : v(x) {}
        Peak operator+(const Peak &o) const { Peak r; r.v = v + o.v; return r; }
        Peak operator-(const Peak &o) const { Peak r; r.v = v - o.v; return r; }
        Peak operator*(const Peak &o) const { Peak r; r.v = v * o.v; return r; }
        Peak &operator+=(const Peak &o) { v += o.v; peak = max(peak, v < 0 ? -v : v); return *this; }
        bool operator==(const Peak &o) const { return v == o.v; }
        friend ostream &operator<<(ostream &os, const Peak &m) { return os << lng(m.v); }
    };
    Peak fromPeak(lll x) { Peak r; r.v = x; return r; }
    ulng wrap(lll x) { return ulng(x); }

    struct Ev { int kind; lng x1, y1, x2, y2, w; };  // 0 point update, 1 rectangle update, 2 rectangle query, 3 point query
    lll overlap(lng a1, lng a2, lng b1, lng b2) { return max(lll(0), lll(min(a2, b2)) - lll(max(a1, b1))); }

    // Answers by the definition: each earlier update contributes w times the lattice points shared with the query.
    template<typename T>
    vector<T> brute(const vector<Ev> &ev, T (*conv)(lll)) {
        vector<T> res;
        for (size_t j = 0; j < ev.size(); ++j) {
            if (ev[j].kind < 2) { continue; }
            const Ev &q = ev[j];
            lng qx2 = q.kind == 3 ? q.x1 + 1 : q.x2, qy2 = q.kind == 3 ? q.y1 + 1 : q.y2;
            T s = conv(0);
            for (size_t i = 0; i < j; ++i) {
                const Ev &u = ev[i];
                if (u.kind > 1) { continue; }
                lng ux2 = u.kind == 0 ? u.x1 + 1 : u.x2, uy2 = u.kind == 0 ? u.y1 + 1 : u.y2;
                lll ox = overlap(u.x1, ux2, q.x1, qx2), oy = overlap(u.y1, uy2, q.y1, qy2);
                if (ox && oy) { s = s + conv(lll(u.w)) * conv(ox) * conv(oy); }}
            res.push_back(s);}
        return res;}

    // Lattice simulation on a small grid validates the brute formula itself.
    vector<lng> grid(const vector<Ev> &ev, lng lo, lng hi) {
        int w = int(hi - lo);
        vector<vector<lng>> g(w, vector<lng>(w, 0));
        vector<lng> res;
        for (const Ev &e : ev) {
            lng x2 = e.kind % 3 == 0 ? e.x1 + 1 : e.x2, y2 = e.kind % 3 == 0 ? e.y1 + 1 : e.y2;
            lng s = 0;
            for (lng x = e.x1; x < x2; ++x) {
                for (lng y = e.y1; y < y2; ++y) {
                    if (e.kind < 2) { g[x - lo][y - lo] += e.w; }
                    else { s += g[x - lo][y - lo]; }}}
            if (e.kind >= 2) { res.push_back(s); }}
        return res;}

    template<typename T>
    vector<T> run(const vector<Ev> &ev, T (*conv)(lll)) {
        OfflineRectangleSweep<T> s;
        int next = 0;
        for (const Ev &e : ev) {
            if (e.kind == 0) { s.addPoint(e.x1, e.y1, conv(e.w)); }
            else if (e.kind == 1) { s.addRectangle(e.x1, e.y1, e.x2, e.y2, conv(e.w)); }
            else {
                int id = e.kind == 2 ? s.addQuery(e.x1, e.y1, e.x2, e.y2) : s.addQuery(e.x1, e.y1);
                checkEqual(id, next++, "query id");}}
        vector<T> res = s.solve();
        checkEqual(s.solve() == res, true, "solve repeatable");
        return res;}

    // kinds: bit k allows kind k; layout 0 random, 1 updates first, 2 queries first, 3 alternating runs of length 1..3.
    vector<Ev> generate(Rng &rng, int n, int kinds, int layout, lng lo, lng hi, lng wmax) {
        vector<int> allowed;
        for (int k = 0; k < 4; ++k) { if (kinds >> k & 1) { allowed.push_back(k); } }
        vector<Ev> ev;
        for (int i = 0; i < n; ++i) {
            int k = allowed[rng() % allowed.size()];
            Ev e{k, rnd(rng, lo, hi - 1), rnd(rng, lo, hi - 1), 0, 0, rnd(rng, -wmax, wmax)};
            if (k == 1 || k == 2) {
                e.x2 = rnd(rng, lo, hi); e.y2 = rnd(rng, lo, hi);
                if (e.x1 > e.x2) { swap(e.x1, e.x2); }
                if (e.y1 > e.y2) { swap(e.y1, e.y2); }}
            ev.push_back(e);}
        if (layout == 1 || layout == 2) {
            std::stable_partition(ev.begin(), ev.end(), [&](const Ev &e) { return (e.kind < 2) == (layout == 1); });}
        else if (layout == 3) {
            vector<Ev> u, q, out;
            for (auto &e : ev) { (e.kind < 2 ? u : q).push_back(e); }
            size_t a = 0, b = 0;
            for (bool upd = true; a < u.size() || b < q.size(); upd = !upd) {
                for (int c = int(rnd(rng, 1, 3)); c > 0; --c) {
                    if (upd && a < u.size()) { out.push_back(u[a++]); }
                    else if (!upd && b < q.size()) { out.push_back(q[b++]); }}}
            ev = out;}
        return ev;}

    const int MIXES[] = {0b0101, 0b1010, 0b0110, 0b1001, 0b1111, 0b0111, 0b1110, 0b1011, 0b1101, 0b0001, 0b0100, 0b0011};

    void small(Rng &rng, int rounds) {
        for (int s = 0; s < rounds; ++s) {
            int kinds = MIXES[s % 12], layout = int(rng() % 4), n = int(rnd(rng, 0, 30));
            vector<Ev> ev = generate(rng, n, kinds, layout, -3, 4, 5);
            context = "small round=" + show(s) + " kinds=" + show(kinds) + " layout=" + show(layout) + " n=" + show(n);
            vector<lng> g = grid(ev, -3, 4), b = brute<lng>(ev, [](lll x) { return lng(x); }), r = run<lng>(ev, [](lll x) { return lng(x); });
            checkEqual(b == g, true, "brute equals grid");
            for (size_t i = 0; i < g.size(); ++i) { checkEqual(r[i], g[i], "answer " + show(i)); }}}

    template<typename T>
    void compare(const vector<Ev> &ev, T (*conv)(lll), const string &name) {
        vector<T> b = brute<T>(ev, conv), r = run<T>(ev, conv);
        checkEqual(r.size(), b.size(), name + " count");
        for (size_t i = 0; i < b.size(); ++i) { checkEqual(r[i], b[i], name + " answer " + show(i)); }}

    void randomLarge(Rng &rng, int rounds, int n) {
        for (int s = 0; s < rounds; ++s) {
            int kinds = MIXES[s % 12], layout = int(rng() % 4);
            context = "random round=" + show(s) + " kinds=" + show(kinds) + " layout=" + show(layout);
            vector<Ev> ev = generate(rng, int(rnd(rng, 1, n)), kinds, layout, -1000, 1000, 1000);
            compare<lng>(ev, [](lll x) { return lng(x); }, "lng");
            ev = generate(rng, int(rnd(rng, 1, n)), kinds, layout, INT64_MIN, INT64_MAX, INT64_MAX);
            compare<ulng>(ev, wrap, "ulng extreme");
            compare<Mod>(ev, fromWide, "mod extreme");
            if (!(kinds & 2) || !(kinds & 4)) { compare<lng>(generate(rng, int(rnd(rng, 1, n)), kinds, layout, INT64_MIN, INT64_MAX, 1 << 20), [](lll x) { return lng(x); }, "lng extreme"); }}}

    // Static input costs one sweep; time-ordered input at most ceil(log2(runs)) sweeps.
    void work(Rng &rng, int n) {
        for (int kinds : {0b0101, 0b1010, 0b0110}) {
            for (int layout : {1, 3}) {
                vector<Ev> ev = generate(rng, n, kinds, layout, -1000000, 1000000, 1000);
                lng items = 0, runs = 1, ys = 0;
                for (size_t i = 0; i < ev.size(); ++i) {
                    items += 4;
                    runs += i && (ev[i].kind < 2) != (ev[i - 1].kind < 2);}
                ys = items;
                lng depth = layout == 1 ? 1 : int(std::bit_width(ulng(runs - 1))), K = kinds == 0b0110 ? 4 : 1;
                context = "work kinds=" + show(kinds) + " layout=" + show(layout) + " runs=" + show(runs);
                adds = 0;
                vector<Mod> r = run<Mod>(ev, fromWide);
                lng bound = 2 * K * items * (int(std::bit_width(ulng(ys))) + 1) * depth;
                checkEqual(adds <= bound, true, "Fenwick additions " + show(adds) + " bound " + show(bound));
                if (n <= 8000 && K == 4 && layout == 3) { checkEqual(r == brute<Mod>(ev, fromWide), true, "answers"); }}}}

    // Cells hold partial sums of the current sweep only: unit point weights keep every cell within the point count.
    void magnitude(Rng &rng, int n) {
        for (int layout : {1, 3}) {
            vector<Ev> ev = generate(rng, n, 0b0101, layout, -50, 50, 0);
            int points = 0;
            for (Ev &e : ev) { if (e.kind == 0) { e.w = 1; ++points; } }
            context = "magnitude layout=" + show(layout) + " points=" + show(points);
            peak = 0;
            vector<Peak> r = run<Peak>(ev, fromPeak);
            checkEqual(peak <= lll(points), true, "Fenwick cell magnitude " + show(lng(peak)));
            checkEqual(r == brute<Peak>(ev, fromPeak), true, "answers");}}

    void edges() {
        context = "edges";
        OfflineRectangleSweep<lng> e;
        checkEqual(e.solve().size(), size_t(0), "empty");
        e.addPoint(0, 0, 5);
        checkEqual(e.solve().size(), size_t(0), "updates only");
        OfflineRectangleSweep<lng> q;
        q.addQuery(-5, -5, 5, 5); q.addPoint(0, 0, 7); q.addQuery(-5, -5, 5, 5); q.addQuery(0, 0);
        checkEqual(q.solve(), vector<lng>{0, 7, 7}, "query before update sees nothing");
        OfflineRectangleSweep<lng> z;
        z.addRectangle(2, 2, 2, 9, 4); z.addRectangle(1, 1, 3, 1, 4); z.addQuery(0, 0, 10, 10); z.addQuery(2, 5);
        checkEqual(z.solve(), vector<lng>{0, 0}, "empty rectangles");
        OfflineRectangleSweep<lng> h;
        h.addPoint(INT64_MAX - 1, INT64_MIN, 3); h.addQuery(INT64_MIN, INT64_MIN, INT64_MAX, INT64_MAX); h.addQuery(INT64_MAX - 1, INT64_MIN);
        h.addPoint(5, 5, 2); h.addQuery(5, 5, 6, 6); h.addQuery(5, 5, 5, 6);
        checkEqual(h.solve(), vector<lng>{3, 3, 2, 0}, "extreme coordinates and half-open edges");
        OfflineRectangleSweep<lng> c = h;
        c.addQuery(0, 0, 10, 10);
        checkEqual(h.solve().size(), size_t(4), "copy independent"); checkEqual(c.solve().back(), lng(2), "copy");}

    void invalid(const string &name) {
        OfflineRectangleSweep<lng> s;
        if (name == "rectangle-reversed-x") { s.addRectangle(3, 0, 2, 5, 1); }
        else if (name == "rectangle-reversed-y") { s.addRectangle(0, 3, 5, 2, 1); }
        else if (name == "query-reversed") { (void)s.addQuery(0, 3, 5, 2); }
        else if (name == "point-max-x") { s.addPoint(INT64_MAX, 0, 1); }
        else if (name == "point-query-max-y") { (void)s.addQuery(0, INT64_MAX); }
        else if (name == "point-max-y") { s.addPoint(0, INT64_MAX, 1); }
        else if (name == "point-query-max-x") { (void)s.addQuery(INT64_MAX, 0); }
        else if (name == "query-reversed-x") { (void)s.addQuery(3, 0, 2, 5); }
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
        std::cout << "offline_rectangle_queries seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        small(rng, mode == "quick" ? 600 : mode == "full" ? 30000 : 200000);
        randomLarge(rng, mode == "quick" ? 24 : mode == "full" ? 480 : 2400, mode == "quick" ? 200 : 800);
        work(rng, mode == "quick" ? 2000 : mode == "full" ? 8000 : 100000);
        magnitude(rng, mode == "quick" ? 600 : 3000);
        std::cout << "PASS offline_rectangle_queries checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL offline_rectangle_queries seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
