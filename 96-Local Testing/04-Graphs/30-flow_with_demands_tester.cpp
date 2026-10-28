#include "../../04-Graphs/30-flow_with_demands.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    std::mt19937_64 rng;
    const lll BIG = lll(1) << 125;
    // libstdc++ debug mode (checked build, -O0) shrinks the large sections.
#ifdef _GLIBCXX_DEBUG
    const int SHRINK = 10;
#else
    const int SHRINK = 1;
#endif
    enum Kind { CIRC, FEAS, MAXV, MINV, COST };
    const array<string, 5> NAMES{"feasibleCirculation", "feasibleFlow", "maxFlowWithLowerBounds", "minFlowWithLowerBounds", "minCostCirculationAdapter"};
    array<lng, 5> feasible{}, infeasible{};

    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string str(lll x) {
        if (x == 0) { return "0"; }
        string res; bool neg = x < 0; ulll y = neg ? ulll(0) - ulll(x) : ulll(x);
        for (; y > 0; y /= 10) { res += char('0' + int(y % 10)); }
        if (neg) { res += '-'; }
        reverse(res.begin(), res.end()); return res;}
    void equal(lll expected, lll actual, const string &operation) {
        ++checks;
        if (expected != actual) { throw std::runtime_error(context + " operation=" + operation + " expected=" + str(expected) + " actual=" + str(actual)); }}
    lll rand(lll lo, lll hi) { return lo + lll(((ulll(rng()) << 64) | rng()) % ulll(hi - lo + 1)); }
    int randInt(int lo, int hi) { return int(rand(lo, hi)); }
    lll mag(lll x) { return x < 0 ? -x : x; }

    // Model network in lll: edges (u, v, lo, hi, cost), supplies b (empty for none), s = t = -1 when n < 2.
    struct Ed { int u, v; lll lo, hi, c; };
    struct Inst { int n = 0; vector<Ed> es; vector<lll> b; int s = -1, t = -1; };
    lll supply(const Inst &in, int v) { return in.b.empty() ? 0 : in.b[v]; }
    string show(const Inst &in) {
        string res = "n=" + std::to_string(in.n) + " s=" + std::to_string(in.s) + " t=" + std::to_string(in.t) + " edges=";
        if (in.es.size() > 30) { return res + std::to_string(in.es.size()); }
        for (auto &e : in.es) { res += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + "," + str(e.lo) + "," + str(e.hi) + "," + str(e.c) + ")"; }
        res += " b=";
        for (lll x : in.b) { res += str(x) + ","; }
        return res;}
    lll weight(const Inst &in) {
        lll res = 0;
        for (auto &e : in.es) { res += mag(e.lo) + mag(e.hi); }
        for (lll x : in.b) { res += mag(x); }
        return res;}
    Inst scale(Inst in, lll k) {
        for (auto &e : in.es) { e.lo *= k; e.hi *= k; }
        for (auto &x : in.b) { x *= k; }
        return in;}

    // Exhaustive integer flows (exact by total unimodularity): circulation feasibility and minimum cost, s-t value range.
    struct Brute { bool circ = false, st = false; lll mx = 0, mn = 0, cost = 0; };
    Brute brute(const Inst &in) {
        Brute res; int m = int(in.es.size());
        vector<lll> f(m);
        auto rec = [&](auto &&self, int i) -> void {
            if (i < m) {
                for (f[i] = in.es[i].lo; f[i] <= in.es[i].hi; ++f[i]) { self(self, i + 1); }
                return;}
            vector<lll> net(in.n);
            lll cost = 0;
            for (int j = 0; j < m; ++j) { net[in.es[j].u] += f[j]; net[in.es[j].v] -= f[j]; cost += in.es[j].c * f[j]; }
            for (int v = 0; v < in.n; ++v) {
                if (v != in.s && v != in.t && net[v] != supply(in, v)) { return; }}
            lll value = in.s < 0 ? 0 : net[in.s] - supply(in, in.s);
            if (value == 0) { res.cost = res.circ ? min(res.cost, cost) : cost; res.circ = true; }
            if (in.s >= 0) { res.mx = res.st ? max(res.mx, value) : value; res.mn = res.st ? min(res.mn, value) : value; res.st = true; }};
        rec(rec, 0);
        return res;}

    // Optimality certificates on a witness: an augmenting path in the residual network, a negative residual cycle (Bellman-Ford).
    bool residualPath(const Inst &in, const vector<lll> &f, int from, int to) {
        vector<vector<int>> adj(in.n);
        for (int i = 0; i < int(in.es.size()); ++i) {
            auto &e = in.es[i];
            if (f[i] < e.hi) { adj[e.u].push_back(e.v); }
            if (f[i] > e.lo) { adj[e.v].push_back(e.u); }}
        vector<char> seen(in.n); vector<int> q{from}; seen[from] = 1;
        for (int i = 0; i < int(q.size()); ++i) {
            for (int v : adj[q[i]]) {
                if (!seen[v]) { seen[v] = 1; q.push_back(v); }}}
        return seen[to];}
    bool negativeCycle(const Inst &in, const vector<lll> &f) {
        struct Arc { int u, v; lll c; };
        vector<Arc> arcs;
        for (int i = 0; i < int(in.es.size()); ++i) {
            auto &e = in.es[i];
            if (f[i] < e.hi) { arcs.push_back({e.u, e.v, e.c}); }
            if (f[i] > e.lo) { arcs.push_back({e.v, e.u, -e.c}); }}
        vector<lll> d(in.n);
        for (int r = 0; r <= in.n; ++r) {
            bool upd = false;
            for (auto [u, v, c] : arcs) {
                if (d[u] + c < d[v]) { d[v] = d[u] + c; upd = true; }}
            if (!upd) { return false; }}
        return true;}

    // Certifies one result: the witness (bounds, conservation, value, cost) and its optimality certificates, or the strict Hoffman cut.
    template<class Cap>
    void verify(const Inst &in, const FlowWithDemandsResult<Cap> &res, Kind kind) {
        int n = in.n, m = int(in.es.size());
        bool st = kind != CIRC && kind != COST, tight = kind == MAXV || kind == MINV;
        string name = NAMES[kind];
        ++(res.ok ? feasible : infeasible)[kind];
        check(int(res.stCut.size()) == (res.ok && tight ? n : 0) && int(res.potentials.size()) == (res.ok && kind == COST ? n : 0), name + " stCut and potentials shape");
        if (!res.ok) {
            auto &cut = res.infeasibleCut;
            check(res.flow.empty() && res.value == 0 && res.cost == 0 && int(cut.size()) == n, name + " infeasible result shape");
            if (st) { check(cut[in.s] == cut[in.t], name + " cut keeps s and t on one side"); }
            lll bs = 0, cap = 0;
            for (int v = 0; v < n; ++v) {
                if (cut[v]) { bs += supply(in, v); }}
            for (auto &e : in.es) {
                if (cut[e.u] && !cut[e.v]) { cap += e.hi; }
                if (!cut[e.u] && cut[e.v]) { cap -= e.lo; }}
            check(bs > cap, name + " Hoffman cut b(S) > hi(out) - lo(in): b(S)=" + str(bs) + " rhs=" + str(cap));
            return;}
        check(int(res.flow.size()) == m && res.infeasibleCut.empty(), name + " feasible result shape");
        vector<lll> f(res.flow.begin(), res.flow.end()), net(n);
        lll cost = 0;
        for (int i = 0; i < m; ++i) {
            auto &e = in.es[i];
            check(e.lo <= f[i] && f[i] <= e.hi, name + " flow within [lo, hi] at edge " + std::to_string(i));
            net[e.u] += f[i]; net[e.v] -= f[i];
            if (kind == COST) { cost += e.c * f[i]; }}
        lll value = st ? net[in.s] - supply(in, in.s) : 0;
        equal(value, res.value, name + " value recomputed from the witness");
        for (int v = 0; v < n; ++v) { equal(supply(in, v) + (st && v == in.s ? value : st && v == in.t ? -value : 0), net[v], name + " conservation at vertex " + std::to_string(v)); }
        equal(cost, res.cost, name + " cost recomputed from the witness");
        if (kind == MAXV) { check(!residualPath(in, f, in.s, in.t), name + " no s->t path in the witness residual"); }
        if (kind == MINV) { check(!residualPath(in, f, in.t, in.s), name + " no t->s path in the witness residual"); }
        if (tight) {
            // value == hi(out) - lo(in) - b(C) for max, lo(out) - hi(in) - b(C) for min: C bounds every feasible value.
            auto &cut = res.stCut;
            check(cut[in.s] && !cut[in.t], name + " stCut holds s and not t");
            lll rhs = 0;
            for (int v = 0; v < n; ++v) {
                if (cut[v]) { rhs -= supply(in, v); }}
            for (auto &e : in.es) {
                if (cut[e.u] && !cut[e.v]) { rhs += kind == MAXV ? e.hi : e.lo; }
                if (!cut[e.u] && cut[e.v]) { rhs -= kind == MAXV ? e.lo : e.hi; }}
            equal(rhs, value, name + " value equals the stCut capacity");}
        if (kind == COST) {
            check(!negativeCycle(in, f), name + " no negative cycle in the witness residual");
            // Complementary slackness under the returned potentials: reduced cost > 0 forces lo, < 0 forces hi.
            auto &p = res.potentials;
            for (int i = 0; i < m; ++i) {
                auto &e = in.es[i];
                lll r = e.c + p[e.u] - p[e.v];
                check((r <= 0 || f[i] == e.lo) && (r >= 0 || f[i] == e.hi), name + " complementary slackness at edge " + std::to_string(i) + " reduced=" + str(r));}}}

    // Tester-side backend: successive shortest paths by Bellman-Ford over explicit residual arcs (given costs are >= 0).
    template<class Cap, class Cost>
    struct BellmanBackend {
        struct Edge { int from, to; Cap cap, flow; Cost cost; };
        int n;
        vector<Edge> es;

        explicit BellmanBackend(int N) : n(N) {}
        int addEdge(int u, int v, Cap cap, Cost cost) { es.push_back({u, v, cap, 0, cost}); return int(es.size()) - 1; }
        Edge getEdge(int i) const { return es[i]; }
        vector<Edge> edges() const { return es; }
        pair<Cap, lll> flow(int s, int t) {
            Cap total = 0;
            lll cost = 0;
            while (true) {
                vector<lll> d(n, BIG); vector<int> par(n, -1); d[s] = 0;
                for (int r = 0; r < n; ++r) {
                    bool upd = false;
                    for (int i = 0; i < int(es.size()); ++i) {
                        auto &e = es[i];
                        if (e.flow < e.cap && d[e.from] < BIG && d[e.from] + e.cost < d[e.to]) { d[e.to] = d[e.from] + e.cost; par[e.to] = 2 * i; upd = true; }
                        if (e.flow > 0 && d[e.to] < BIG && d[e.to] - e.cost < d[e.from]) { d[e.from] = d[e.to] - e.cost; par[e.from] = 2 * i + 1; upd = true; }}
                    if (!upd) { break; }}
                if (d[t] == BIG) { return {total, cost}; }
                Cap f = std::numeric_limits<Cap>::max();
                for (int v = t; v != s; v = par[v] % 2 ? es[par[v] / 2].to : es[par[v] / 2].from) {
                    auto &e = es[par[v] / 2];
                    f = min(f, par[v] % 2 ? e.flow : e.cap - e.flow);}
                for (int v = t; v != s; v = par[v] % 2 ? es[par[v] / 2].to : es[par[v] / 2].from) {
                    auto &e = es[par[v] / 2];
                    e.flow = par[v] % 2 ? e.flow - f : e.flow + f;}
                total += f; cost += lll(f) * d[t];}}
        // Bellman-Ford distances from a virtual source over the residual arcs (no negative cycle once the flow is min-cost).
        vector<Cost> potentials() const {
            vector<lll> d(n);
            for (int r = 0; r < n; ++r) {
                bool upd = false;
                for (auto &e : es) {
                    if (e.flow < e.cap && d[e.from] + e.cost < d[e.to]) { d[e.to] = d[e.from] + e.cost; upd = true; }
                    if (e.flow > 0 && d[e.to] - e.cost < d[e.from]) { d[e.from] = d[e.to] - e.cost; upd = true; }}
                if (!upd) { break; }}
            vector<Cost> res;
            for (lll x : d) { res.push_back(Cost(x)); }
            return res;}
    };

    template<class Cap>
    vector<tuple<int, int, Cap, Cap>> edges4(const Inst &in) {
        vector<tuple<int, int, Cap, Cap>> res;
        for (auto &e : in.es) { res.emplace_back(e.u, e.v, Cap(e.lo), Cap(e.hi)); }
        return res;}
    template<class Cap, class Cost>
    vector<tuple<int, int, Cap, Cap, Cost>> edges5(const Inst &in) {
        vector<tuple<int, int, Cap, Cap, Cost>> res;
        for (auto &e : in.es) { res.emplace_back(e.u, e.v, Cap(e.lo), Cap(e.hi), Cost(e.c)); }
        return res;}
    template<class Cap>
    bool same(const FlowWithDemandsResult<Cap> &a, const FlowWithDemandsResult<Cap> &b) {
        return a.ok == b.ok && a.value == b.value && a.cost == b.cost && a.flow == b.flow && a.infeasibleCut == b.infeasibleCut && a.stCut == b.stCut && a.potentials == b.potentials;}

    // Calls every function as user code does (b omitted when empty), certifies each result, compares with the brute force scaled by k.
    template<class Cap, class Cost>
    array<lll, 6> runCase(const Inst &in, const Brute *br = nullptr, lll k = 1, bool costs = true) {
        ++cases;
        context = "case=" + std::to_string(cases) + " Cap=" + std::to_string(8 * sizeof(Cap)) + "-bit Cost=" + std::to_string(8 * sizeof(Cost)) + "-bit " + show(in);
        check(weight(in) <= lll(std::numeric_limits<Cap>::max()), "tester keeps W within the domain");
        auto es = edges4<Cap>(in);
        vector<Cap> b, zero(in.n);
        for (lll x : in.b) { b.push_back(Cap(x)); }
        if (!b.empty()) { zero = b; }
        auto circ = b.empty() ? feasibleCirculation(in.n, es) : feasibleCirculation(in.n, es, b);
        verify(in, circ, CIRC);
        check(same(circ, feasibleCirculation(in.n, es, zero)), "feasibleCirculation repeated call (explicit zero supplies when none) identical");
        if (br) { check(circ.ok == br->circ, "feasibleCirculation ok equals brute"); }
        array<lll, 6> res{circ.ok, 0, 0, 0, 0, 0};
        if (in.s >= 0) {
            auto mx = b.empty() ? maxFlowWithLowerBounds(in.n, es, in.s, in.t) : maxFlowWithLowerBounds(in.n, es, in.s, in.t, b);
            auto mn = b.empty() ? minFlowWithLowerBounds(in.n, es, in.s, in.t) : minFlowWithLowerBounds(in.n, es, in.s, in.t, b);
            auto any = b.empty() ? feasibleFlow(in.n, es, in.s, in.t) : feasibleFlow(in.n, es, in.s, in.t, b);
            verify(in, mx, MAXV); verify(in, mn, MINV); verify(in, any, FEAS);
            check(same(mx, maxFlowWithLowerBounds(in.n, es, in.s, in.t, zero)) && same(mn, minFlowWithLowerBounds(in.n, es, in.s, in.t, zero))
                && same(any, feasibleFlow(in.n, es, in.s, in.t, zero)), "s-t repeated calls identical");
            check(mx.ok == mn.ok && mn.value <= mx.value, "max and min agree on feasibility, min <= max");
            check(any.ok == mx.ok && (!any.ok || (mn.value <= any.value && any.value <= mx.value)), "feasibleFlow agrees on feasibility, value within [min, max]");
            check(circ.ok == (mx.ok && mn.value <= 0 && 0 <= mx.value), "circulation feasible iff 0 lies in [min, max]");
            if (br) {
                check(mx.ok == br->st, "s-t feasibility equals brute");
                if (mx.ok) { equal(br->mx * k, mx.value, "maxFlowWithLowerBounds value equals brute"); equal(br->mn * k, mn.value, "minFlowWithLowerBounds value equals brute"); }}
            res[1] = mx.ok; res[2] = mx.value; res[3] = mn.value;}
        if (costs) {
            auto es5 = edges5<Cap, Cost>(in);
            auto cost = b.empty() ? minCostCirculationAdapter(in.n, es5) : minCostCirculationAdapter(in.n, es5, b);
            verify(in, cost, COST);
            check(cost.ok == circ.ok, "minCostCirculationAdapter feasibility equals feasibleCirculation");
            check(same(cost, minCostCirculationAdapter(in.n, es5, zero)), "minCostCirculationAdapter repeated call identical");
            if (br && cost.ok) { equal(br->cost * k, cost.cost, "minCostCirculationAdapter cost equals brute"); }
            if (in.n <= 60 && in.es.size() <= 40) {
                auto alt = minCostCirculationAdapter<Cap, Cost, BellmanBackend<Cap, Cost>>(in.n, es5, b);
                verify(in, alt, COST);
                check(alt.ok == cost.ok && alt.cost == cost.cost, "Bellman-Ford backend agrees with MinCostFlow");}
            res[4] = cost.ok; res[5] = cost.cost;}
        return res;}
    array<lll, 6> runTyped(int type, const Inst &in, const Brute *br = nullptr) {
        if (type % 5 == 0) { return runCase<int, int>(in, br); }
        if (type % 5 == 1) { return runCase<lng, lng>(in, br); }
        if (type % 5 == 2) { return runCase<lll, lll>(in, br); }
        if (type % 5 == 3) { return runCase<lng, int>(in, br); }
        return runCase<int, lng>(in, br);}

    // kind 0: random bounds, supplies half the time; 1: a planted witness whose divergence is b; 2: a planted circulation (repair edges 0 -> v).
    Inst randomInst(int n, int m, lll r, lll cr, int kind) {
        Inst in; in.n = n;
        vector<lll> net(n);
        for (int i = 0; i < m && n > 0; ++i) {
            int u = randInt(0, n - 1), v = rng() % 6 == 0 ? u : randInt(0, n - 1);
            if (i > 0 && rng() % 5 == 0) {
                u = in.es[i - 1].u; v = in.es[i - 1].v;
                if (rng() % 2) { swap(u, v); }}
            lll lo = rand(-r, r), hi = rng() % 4 == 0 ? lo : rand(-r, r);
            if (lo > hi) { swap(lo, hi); }
            lll f = rand(lo, hi);
            net[u] += f; net[v] -= f;
            in.es.push_back({u, v, lo, hi, rand(-cr, cr)});}
        if (kind == 1) { in.b = net; }
        else if (kind == 2) {
            for (int v = 1; v < n; ++v) {
                if (net[v] != 0 || rng() % 2) { in.es.push_back({0, v, net[v] - rand(0, r), net[v] + rand(0, r), rand(-cr, cr)}); }}}
        else if (n > 0 && rng() % 2) {
            in.b.assign(n, 0);
            for (int v = 0; v + 1 < n; ++v) { in.b[v] = rand(-r, r); in.b[n - 1] -= in.b[v]; }}
        if (n >= 2) { in.s = randInt(0, n - 1); in.t = (in.s + randInt(1, n - 1)) % n; }
        return in;}
    Inst tiny() {
        while (true) {
            Inst in = randomInst(randInt(0, 4), randInt(0, 4), 3, 3, randInt(0, 2));
            if (in.es.size() <= 4) { return in; }}}

    void handCases() {
        vector<pair<Inst, Brute>> hs = {
            {{2, {{0, 1, 1, 3, 2}}, {}, 0, 1}, {false, true, 3, 1, 0}},
            {{3, {{0, 1, 0, 3, -1}, {1, 2, 0, 3, -1}, {2, 0, 0, 3, -1}}, {}, 0, 1}, {true, true, 3, -3, -9}},
            {{2, {{0, 1, 2, 2, 0}, {1, 0, -1, 5, 1}}, {1, -1}, 0, 1}, {true, true, 2, -4, 1}},
            {{1, {{0, 0, 1, 2, -5}}, {}, -1, -1}, {true, false, 0, 0, -10}},
            {{1, {{0, 0, -3, -1, 2}, {0, 0, 0, 0, 7}}, {0}, -1, -1}, {true, false, 0, 0, -6}},
            {{4, {{0, 1, 0, 2, 1}, {2, 3, 1, 1, 1}}, {}, 0, 1}, {false, false, 0, 0, 0}},
            {{0, {}, {}, -1, -1}, {true, false, 0, 0, 0}},
            {{1, {}, {}, -1, -1}, {true, false, 0, 0, 0}},
            {{2, {{0, 1, -2, 1, 1}, {1, 0, 0, 2, -3}}, {}, 0, 1}, {true, true, 1, -4, -2}},
            {{3, {{0, 1, 0, 4, 1}, {0, 2, 0, 4, 3}, {1, 2, 0, 1, 1}}, {4, -1, -3}, 0, 2}, {true, true, 2, -3, 9}},
            {{2, {{0, 0, 0, 5, 1}, {1, 1, -2, 2, 1}}, {1, -1}, 0, 1}, {false, true, -1, -1, 0}},
            {{2, {}, {}, 1, 0}, {true, true, 0, 0, 0}},
            {{3, {{0, 1, 0, 2, 0}, {0, 1, 1, 3, 0}, {2, 2, -1, 1, -4}}, {}, 1, 0}, {false, true, -1, -5, 0}},
        };
        for (auto &[in, want] : hs) {
            context = "hand " + show(in);
            Brute got = brute(in);
            check(got.circ == want.circ && got.st == want.st && got.mx == want.mx && got.mn == want.mn && got.cost == want.cost, "brute force equals the hand-derived answer");
            for (int type = 0; type < 5; ++type) { runTyped(type, in, &want); }}
        cout << "PASS " << hs.size() << " hand cases (brute force validated; n=0/1, loops, negative bounds, supplies, antiparallel, disconnected, infeasible)\n";}
    // Every two-vertex network with at most `bound` edges: any endpoints, bounds lo <= hi in [-2, 2], cost -2 or 1, three supply vectors.
    void exhaustive() {
        int bound = mode == "quick" ? 1 : 2;
        lng before = cases;
        vector<Ed> opts;
        for (int u = 0; u < 2; ++u) {
            for (int v = 0; v < 2; ++v) {
                for (int lo = -2; lo <= 2; ++lo) {
                    for (int hi = lo; hi <= 2; ++hi) { opts.push_back({u, v, lo, hi, -2}); opts.push_back({u, v, lo, hi, 1}); }}}}
        vector<vector<lll>> bs{{}, {1, -1}, {-2, 2}};
        int type = 0;
        auto rec = [&](auto &&self, Inst &in) -> void {
            for (auto &b : bs) {
                in.b = b;
                Brute br = brute(in);
                runTyped(type++, in, &br);}
            if (int(in.es.size()) == bound) { return; }
            for (auto &e : opts) { in.es.push_back(e); self(self, in); in.es.pop_back(); }};
        Inst in{2, {}, {}, 0, 1};
        rec(rec, in);
        cout << "PASS every two-vertex network with m<=" << bound << " cases=" << cases - before << '\n';}
    void randomSmall() {
        int rounds = mode == "quick" ? 400 : mode == "full" ? 4000 : 20000;
        for (int r = 0; r < rounds; ++r) {
            Inst in = tiny();
            Brute br = brute(in);
            runTyped(r, in, &br);}
        cout << "PASS random tiny networks n<=4 m<=4 against brute force rounds=" << rounds << '\n';}
    // W = MAX boundaries by hand, then tiny brute-forced networks scaled by k = MAX / W (all optima scale by k exactly).
    template<class Cap, class Cost>
    void extremesFor(int rounds) {
        const lll M = std::numeric_limits<Cap>::max(), H = M / 2, Q = M / 4, K = M / 6;
        // lll capacities at W near MAX exceed MinCostFlow's bounded() total-cost limit, so their costed runs use W <= MAX / 8.
        bool wide = sizeof(Cap) == 16;
        vector<pair<Inst, Brute>> hs = {
            {{2, {{0, 1, -H, H, 1}}, {}, 0, 1}, {true, true, H, -H, 0}},
            {{2, {{0, 1, 0, M, -1}}, {}, 0, 1}, {true, true, M, 0, 0}},
            {{2, {{0, 1, -M, 0, 1}}, {}, 1, 0}, {true, true, M, 0, 0}},
            {{2, {{0, 1, H, H, 1}}, {}, 0, 1}, {false, true, H, H, 0}},
            {{2, {{0, 1, 0, 1, 1}}, {H, -H}, 0, 1}, {false, true, 1 - H, -H, 0}},
            {{2, {{1, 0, Q, Q, -1}}, {-Q, Q}, 0, 1}, {true, true, 0, 0, -Q}},
            {{3, {{0, 1, -K, K, -1}, {1, 2, -K, K, -1}, {2, 0, -K, K, -1}}, {}, 0, 1}, {true, true, 2 * K, -2 * K, -3 * K}},
        };
        for (auto &[in, want] : hs) { runCase<Cap, Cost>(in, &want, 1, !wide); }
        for (int r = 0; r < rounds; ++r) {
            Inst raw = tiny();
            lll w = weight(raw);
            if (w == 0) { continue; }
            Brute br = brute(raw);
            runCase<Cap, Cost>(scale(raw, M / w), &br, M / w, !wide);
            if (wide) { runCase<Cap, Cost>(scale(raw, M / 8 / w), &br, M / 8 / w, true); }}
        cout << "PASS extremes Cap=" << 8 * sizeof(Cap) << "-bit Cost=" << 8 * sizeof(Cost) << "-bit: " << hs.size() << " hand cases at W=MAX, " << rounds << " scaled brute-force rounds\n";}
    void medium() {
        int rounds = mode == "quick" ? 60 : mode == "full" ? 600 : 3000;
        for (int r = 0; r < rounds; ++r) {
            int n = randInt(2, 40);
            Inst in = randomInst(n, randInt(0, 4 * n), 1000000, 1000, r % 3);
            auto a = runTyped(r, in), b = runTyped(r + 1, in);
            context = "medium " + show(in);
            check(a == b, "results agree across Cap and Cost types");}
        cout << "PASS medium certified networks n<=40 rounds=" << rounds << '\n';}
    // Uncosted, bounds up to r (randomInst keeps W <= 26 * n * r): random, planted with supplies, planted circulation, forced chain.
    template<class Cap>
    void largeFor(int n, lll r) {
        for (int shape = 0; shape < 4; ++shape) {
            Inst in;
            if (shape < 3) { in = randomInst(n, 4 * n, r, 0, shape); }
            else {
                // A long forced chain closed by one back edge, with random chords: deep residual paths.
                in.n = n; in.s = 0; in.t = n - 1;
                for (int v = 0; v + 1 < n; ++v) {
                    lll lo = rand(1, 5);
                    in.es.push_back({v, v + 1, lo, lo + rand(0, 10), 0});
                    if (rng() % 3 == 0) { in.es.push_back({v + 1, randInt(0, v), -3, 3, 0}); }}
                in.es.push_back({n - 1, 0, 0, 4 * r, 0});}
            runCase<Cap, Cap>(in, nullptr, 1, false);}}
    void large() {
        int n = (mode == "quick" ? 2000 : mode == "full" ? 30000 : 100000) / SHRINK;
        largeFor<lng>(n, lll(1) << 38); largeFor<int>(n, INT_MAX / (32 * n)); largeFor<lll>(n / 4, lll(1) << 100);
        int nc = (mode == "quick" ? 150 : mode == "full" ? 1500 : 3000) / SHRINK, nb = (mode == "quick" ? 100 : mode == "full" ? 300 : 600) / SHRINK;
        for (int shape = 0; shape < 3; ++shape) { runCase<lng, lng>(randomInst(nc, 4 * nc, 20, 100, shape)); }
        // Bounds up to 10^12, so successive shortest paths moves large excesses.
        for (int shape = 0; shape < 3; ++shape) { runCase<lng, lng>(randomInst(nb, 4 * nb, 1000000000000, 1000, shape)); }
        cout << "PASS large certified networks n=" << n << " (Cap lng and int, lll at n=" << n / 4 << "; random, planted with supplies, planted circulation, forced chain), costed n=" << nc
             << " (bounds 20) and n=" << nb << " (bounds 1e12)\n";}
    // Cost precondition boundary, Cap=lll Cost=lng c=2^30: sum |c| * max(|lo|, |hi|) = 2^126 with fixed flows (cost 0)
    // and 2^125 with free flows (values [-2X, 2X], cost -2^124), each in all 24 edge orders.
    void costBound() {
        const lll c = lll(1) << 30;
        for (int shape = 0; shape < 2; ++shape) {
            lll X = lll(1) << (94 - shape), lo = shape ? 0 : X;
            Inst in{2, {{0, 1, lo, X, c}, {1, 0, lo, X, c}, {0, 1, lo, X, -c}, {1, 0, lo, X, -c}}, {}, 0, 1};
            Brute want = shape ? Brute{true, true, 2 * X, -2 * X, -2 * c * X} : Brute{true, true, 0, 0, 0};
            if (!shape) {
                context = "cost bound " + show(in);
                Brute got = brute(in);
                check(got.circ && got.st && got.mx == 0 && got.mn == 0 && got.cost == 0, "brute force equals the hand-derived answer");}
            array<int, 4> ord{0, 1, 2, 3};
            do {
                Inst p = in;
                for (int i = 0; i < 4; ++i) { p.es[i] = in.es[ord[i]]; }
                runCase<lll, lng>(p, &want);}
            while (std::next_permutation(ord.begin(), ord.end()));}
        cout << "PASS cost bound 2^126 (fixed flows) and 2^125 (free flows), Cap=lll Cost=lng, 24 edge orders each\n";}

    void invalid(const string &name) {
        using E4 = vector<tuple<int, int, int, int>>;
        using E5 = vector<tuple<int, int, int, int, int>>;
        const int M = INT_MAX;
        if (name == "size-negative") { feasibleCirculation(-1, E4{}); }
        else if (name == "lo-greater-hi") { feasibleCirculation(2, E4{{0, 1, 2, 1}}); }
        else if (name == "cost-lo-greater-hi") { minCostCirculationAdapter(2, E5{{0, 1, 1, 0, -1}}); }
        else if (name == "supply-size") { maxFlowWithLowerBounds(2, E4{{0, 1, 0, 1}}, 0, 1, vector<int>{0}); }
        else if (name == "supply-sum") { minFlowWithLowerBounds(2, E4{{0, 1, 0, 1}}, 0, 1, vector<int>{1, 0}); }
        else if (name == "cost-supply-sum") { minCostCirculationAdapter(2, E5{}, vector<int>{1, 1}); }
        else if (name == "vertex-range") { feasibleCirculation(2, E4{{0, 2, 0, 1}}); }
        else if (name == "vertex-negative") { minCostCirculationAdapter(2, E5{{-1, 0, 0, 1, 0}}); }
        else if (name == "s-equals-t") { maxFlowWithLowerBounds(2, E4{}, 1, 1); }
        else if (name == "s-range") { maxFlowWithLowerBounds(2, E4{}, -1, 1); }
        else if (name == "t-range") { minFlowWithLowerBounds(2, E4{}, 0, 2); }
        else if (name == "w-overflow") { feasibleCirculation(2, E4{{0, 1, 0, M}, {1, 0, 0, 1}}); }
        else if (name == "w-overflow-negative") { maxFlowWithLowerBounds(2, E4{{0, 1, -M, 0}, {1, 1, -1, -1}}, 0, 1); }
        else if (name == "w-overflow-supply") { minFlowWithLowerBounds(2, E4{{0, 1, 0, 1}}, 0, 1, vector<int>{M, -M}); }
        else if (name == "w-overflow-lll") { feasibleCirculation(1, vector<tuple<int, int, lll, lll>>{{0, 0, std::numeric_limits<lll>::min(), 0}}); }
        else if (name == "w-overflow-cost") { minCostCirculationAdapter(2, vector<tuple<int, int, lng, lng, lng>>{{0, 1, 0, LLONG_MAX, 1}, {1, 0, -1, 0, -1}}); }
        else if (name == "cap-min-cost") { minCostCirculationAdapter(1, E5{{0, 0, INT_MIN, 0, 1}}); }
        else if (name == "cost-min") { minCostCirculationAdapter(2, E5{{0, 1, 0, 1, INT_MIN}}); }
        else if (name == "cost-bound" || name == "cost-bound-exact") {
            // sum |c| * max(|lo|, |hi|) with X = 2^96, c = 2^30: 2^128 (wraps ulll) or 2^127 (exceeds the lll maximum).
            lll X = lll(1) << 96;
            lng c = lng(1) << 30;
            vector<tuple<int, int, lll, lll, lng>> es{{0, 1, X, X, c}, {1, 0, X, X, c}};
            if (name == "cost-bound") { es.emplace_back(0, 1, X, X, -c); es.emplace_back(1, 0, X, X, -c); }
            minCostCirculationAdapter(2, es);}
        else if (name == "cost-min-lng") { minCostCirculationAdapter(2, vector<tuple<int, int, lng, lng, lng>>{{1, 0, 0, 1, LLONG_MIN}}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        rng.seed(seed);
        handCases(); exhaustive(); randomSmall();
        int er = mode == "quick" ? 100 : mode == "full" ? 1000 : 5000;
        extremesFor<int, int>(er); extremesFor<int, lng>(er); extremesFor<lng, lng>(er); extremesFor<lll, lng>(er);
        costBound(); medium(); large();
        context = "coverage";
        for (int k = 0; k < 5; ++k) { check(feasible[k] > 0 && infeasible[k] > 0, NAMES[k] + " returned both feasible and infeasible results"); }
        cout << "PASS flow_with_demands seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL flow_with_demands seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
