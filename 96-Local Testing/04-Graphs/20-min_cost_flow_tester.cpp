#include "../../04-Graphs/20-min_cost_flow.hpp"
#include "../../04-Graphs/19-max_flow.hpp"
#include "../../04-Graphs/23-assignment.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    std::mt19937_64 rng;
    const lll BIG = lll(1) << 125;
    // libstdc++ debug mode re-validates the whole heap on every push and pop, so the checked build shrinks heap-heavy sections.
#ifdef _GLIBCXX_DEBUG
    const bool DEBUG_HEAP = true;
#else
    const bool DEBUG_HEAP = false;
#endif
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

    // Model edge: pieces (length, unit cost) in nondecreasing unit cost; a linear edge is the single piece (cap, cost).
    struct E { int u, v; bool convex; vector<pair<lll, lll>> pieces; };
    struct Net { int n; vector<E> es; };
    lll capOf(const E &e) {
        lll res = 0;
        for (auto [len, w] : e.pieces) { res += len; }
        return res;}
    lll costOf(const E &e, lll x) {
        lll res = 0;
        for (auto [len, w] : e.pieces) { lll k = min(len, x); res += k * w; x -= k; }
        return res;}
    // Unit cost of the 0-based unit x of the edge (the last piece's beyond the capacity).
    lll unitAt(const E &e, lll x) {
        for (auto [len, w] : e.pieces) {
            if (x < len) { return w; }
            x -= len;}
        return e.pieces.back().second;}
    string show(const Net &net) {
        string res = "n=" + std::to_string(net.n) + " edges=";
        if (net.es.size() > 200) { return res + std::to_string(net.es.size()); }
        for (auto &e : net.es) {
            res += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + (e.convex ? ",convex" : "");
            for (auto [len, w] : e.pieces) { res += ",[" + str(len) + "," + str(w) + "]"; }
            res += ")";}
        return res;}

    // Residual arcs of a model flow x: forward at the next unit's cost, backward at minus the last unit's cost.
    struct R { int u, v; lll cost; int id, dir; };
    vector<R> residual(const Net &net, const vector<lll> &x) {
        vector<R> res;
        for (int i = 0; i < int(net.es.size()); ++i) {
            auto &e = net.es[i];
            if (x[i] < capOf(e)) { res.push_back({e.u, e.v, unitAt(e, x[i]), i, 1}); }
            if (x[i] > 0) { res.push_back({e.v, e.u, -unitAt(e, x[i] - 1), i, -1}); }}
        return res;}
    // Bellman-Ford from src (every vertex at 0 when src < 0); par holds arc indices; neg reports a negative cycle.
    vector<lll> bellman(int n, const vector<R> &arcs, int src, vector<int> &par, bool &neg) {
        vector<lll> d(n, src < 0 ? 0 : BIG);
        par.assign(n, -1); neg = false;
        if (src >= 0) { d[src] = 0; }
        for (int r = 0; r <= n; ++r) {
            bool upd = false;
            for (int i = 0; i < int(arcs.size()); ++i) {
                auto &a = arcs[i];
                if (d[a.u] < BIG && d[a.u] + a.cost < d[a.v]) { d[a.v] = d[a.u] + a.cost; par[a.v] = i; upd = true; }}
            if (!upd) { return d; }}
        neg = true; return d;}
    bool negativeCycle(const Net &net, const vector<lll> &x) {
        vector<int> par; bool neg;
        bellman(net.n, residual(net, x), -1, par, neg); return neg;}
    // Whether adding e beside model flow x closes a negative residual cycle: its first unit cost plus the distance back.
    bool closesNegativeCycle(const Net &net, const vector<lll> &x, const E &e) {
        if (capOf(e) == 0) { return false; }
        vector<int> par; bool neg;
        auto d = bellman(net.n, residual(net, x), e.v, par, neg);
        return neg || (d[e.u] < BIG && e.pieces[0].second + d[e.u] < 0);}

    // Cost families without negative cycles: 0 uniform costs (rejected if a cycle appears), 1 nonnegative bases shifted by potentials, 2 DAG.
    Net randomNet(int n, int m, lll capHi, lll lo, lll hi, int convexPct, int maxPieces, lll lenHi, int kind = -1) {
        while (true) {
            Net net{n, {}};
            int k0 = kind < 0 ? randInt(0, 2) : kind;
            vector<lll> pi(n);
            vector<int> rank(n);
            for (auto &p : pi) { p = rand(lo / 2, hi / 2); }
            iota(rank.begin(), rank.end(), 0); std::shuffle(rank.begin(), rank.end(), rng);
            for (int i = 0; i < m; ++i) {
                int u = randInt(0, n - 1), v = rng() % 6 == 0 ? u : randInt(0, n - 1);
                if (k0 == 2 && rank[u] > rank[v]) { swap(u, v); }
                bool convex = randInt(1, 100) <= convexPct;
                int k = convex ? randInt(1, maxPieces) : 1;
                vector<lll> w(k);
                for (auto &c : w) { c = k0 == 1 ? rand(0, (hi - lo) / 2) + pi[u] - pi[v] : k0 == 2 && u == v ? rand(0, hi) : rand(lo, hi); }
                sort(w.begin(), w.end());
                E e{u, v, convex, {}};
                for (int j = 0; j < k; ++j) { e.pieces.push_back({convex ? rand(1, lenHi) : rand(0, capHi), w[j]}); }
                net.es.push_back(e);}
            if (k0 != 0 || !negativeCycle(net, vector<lll>(m))) { return net; }}}

    template<class Cap, class Cost>
    void add(MinCostFlow<Cap, Cost> &mf, const E &e, int id) {
        int got;
        if (e.convex) {
            vector<pair<Cap, Cost>> p;
            for (auto [len, w] : e.pieces) { p.emplace_back(Cap(len), Cost(w)); }
            got = mf.addConvexEdge(e.u, e.v, p);}
        else { got = mf.addEdge(e.u, e.v, Cap(e.pieces[0].first), Cost(e.pieces[0].second)); }
        equal(id, got, "edge ids are sequential");}
    template<class Cap, class Cost>
    MinCostFlow<Cap, Cost> build(const Net &net) {
        MinCostFlow<Cap, Cost> mf(net.n);
        for (int i = 0; i < int(net.es.size()); ++i) { add(mf, net.es[i], i); }
        return mf;}

    // Min cost of every integral flow per divergence vector (inflow - outflow), by enumerating all edge flows.
    struct Exhaustive {
        int n;
        std::unordered_map<lng, lll> best;
        lng key(const vector<lll> &div) const {
            lng res = 0;
            for (int v = n - 1; v >= 0; --v) {
                if (mag(div[v]) >= 127) { return -1; }
                res = res * 256 + lng(div[v] + 128);}
            return res;}
        explicit Exhaustive(const Net &net) : n(net.n) {
            vector<lll> div(n);
            auto rec = [&](auto &&self, int i, lll cost) -> void {
                if (i == int(net.es.size())) {
                    auto [it, fresh] = best.try_emplace(key(div), cost);
                    if (!fresh && cost < it->second) { it->second = cost; }
                    return;}
                auto &e = net.es[i];
                for (lll x = 0; x <= capOf(e); ++x) {
                    div[e.u] -= x; div[e.v] += x;
                    self(self, i + 1, cost + costOf(e, x));
                    div[e.u] += x; div[e.v] -= x;}};
            rec(rec, 0, 0);}
        lll get(const vector<lll> &div) const {
            auto it = best.find(key(div));
            return it == best.end() ? BIG : it->second;}
        // Relative min costs of 0, 1, ... extra s -> t units, stopping at limit, infeasibility or a unit cost >= below.
        vector<lll> curve(vector<lll> b, int s, int t, lll limit, lll below) const {
            lll base = get(b);
            vector<lll> c{0};
            check(base < BIG, "oracle: current divergence is feasible");
            while (lll(c.size()) - 1 < limit) {
                b[s] -= 1; b[t] += 1;
                lll v = get(b);
                if (v >= BIG || v - base - c.back() >= below) { break; }
                c.push_back(v - base);}
            return c;}
    };
    lll space(const Net &net) {
        lll res = 1;
        for (auto &e : net.es) { res *= capOf(e) + 1; }
        return res;}
    // Unit-step successive shortest paths by Bellman-Ford on the model residual (oracle flow x): relative min costs per extra unit.
    vector<lll> unitCurve(const Net &net, vector<lll> &x, int s, int t, lll limit, lll below) {
        vector<lll> c{0};
        vector<int> par;
        bool neg;
        while (lll(c.size()) - 1 < limit) {
            auto arcs = residual(net, x);
            auto d = bellman(net.n, arcs, s, par, neg);
            check(!neg, "unit oracle: no negative residual cycle");
            if (d[t] >= BIG || d[t] >= below) { break; }
            for (int v = t; v != s; v = arcs[par[v]].u) { x[arcs[par[v]].id] += arcs[par[v]].dir; }
            c.push_back(c.back() + d[t]);}
        return c;}
    vector<pair<lll, lll>> breakpoints(const vector<lll> &c) {
        vector<pair<lll, lll>> res{{0, 0}};
        int F = int(c.size()) - 1;
        for (int f = 1; f <= F; ++f) {
            if (f == F || c[f + 1] - c[f] != c[f] - c[f - 1]) { res.push_back({f, c[f]}); }}
        return res;}

    struct Call { int s, t; lll limit, below; bool slope; };
    string show(const Call &c) {
        return " call=" + string(c.slope ? "slope(" : "flow(") + std::to_string(c.s) + "," + std::to_string(c.t) + ",limit=" + str(c.limit) + ",below=" + str(c.below) + ")";}
    template<class Cap, class Cost>
    Call randomCall(int n, lll lo, lll hi, lll limitHi) {
        Call c{randInt(0, n - 1), randInt(0, n - 2), lll(MinCostFlow<Cap, Cost>::MAX), lll(MinCostFlow<Cap, Cost>::INF), rng() % 2 == 0};
        c.t += c.t >= c.s;
        if (rng() % 3 == 0) { c.limit = rand(0, limitHi); }
        if (rng() % 3 == 0) { c.below = rand(lo, hi); }
        return c;}
    template<class Cap, class Cost>
    vector<pair<lll, lll>> run(MinCostFlow<Cap, Cost> &mf, const Call &c) {
        vector<pair<lll, lll>> res;
        if (c.slope) {
            for (auto [f, w] : mf.slope(c.s, c.t, Cap(c.limit), Cost(c.below))) { res.push_back({f, w}); }}
        else { auto [f, w] = mf.flow(c.s, c.t, Cap(c.limit), Cost(c.below)); res.push_back({f, w}); }
        return res;}
    void compare(const vector<pair<lll, lll>> &got, const vector<pair<lll, lll>> &bp, bool slope) {
        if (!slope) {
            equal(bp.back().first, got[0].first, "flow amount"); equal(bp.back().second, got[0].second, "flow cost");
            return;}
        equal(lll(bp.size()), lll(got.size()), "slope breakpoint count");
        for (int i = 0; i < int(bp.size()); ++i) {
            equal(bp[i].first, got[i].first, "slope breakpoint flow"); equal(bp[i].second, got[i].second, "slope breakpoint cost");}}
    // Breakpoints from (0, 0) with increasing flow, strictly increasing unit costs, all below the bound.
    void slopeShape(const vector<pair<lll, lll>> &sl, lll below) {
        lll last = -BIG;
        check(sl[0].first == 0 && sl[0].second == 0, "slope starts at (0, 0)");
        for (int i = 1; i < int(sl.size()); ++i) {
            lll df = sl[i].first - sl[i - 1].first, dc = sl[i].second - sl[i - 1].second;
            check(df > 0 && dc % df == 0, "slope segment has positive length and an integral unit cost");
            check(last < dc / df && dc / df < below, "slope unit costs strictly increase and stay below the bound");
            last = dc / df;}}
    lll interpolate(const vector<pair<lll, lll>> &sl, lll j) {
        for (int i = 1; i < int(sl.size()); ++i) {
            if (j <= sl[i].first) { return sl[i - 1].second + (sl[i].second - sl[i - 1].second) / (sl[i].first - sl[i - 1].first) * (j - sl[i - 1].first); }}
        return j == 0 ? 0 : BIG;}

    // Library state against the model: edge reads, conservation to the tracked divergence, the summed cost, and potentials
    // within [-3B, 2B] that give every residual arc a nonnegative reduced cost (min cost for the divergence).
    template<class Cap, class Cost>
    vector<lll> certify(MinCostFlow<Cap, Cost> &mf, const Net &net, const vector<lll> &div, lll total) {
        int m = int(net.es.size());
        vector<lll> x(m), bal(net.n);
        auto all = mf.edges();
        lll cost = 0, w = 0, top = 0;
        equal(m, lll(all.size()), "edges size");
        for (int i = 0; i < m; ++i) {
            auto e = mf.getEdge(i); auto &me = net.es[i];
            check(e.from == me.u && e.to == me.v && lll(e.cap) == capOf(me), "getEdge endpoints and capacity");
            check(all[i].from == e.from && all[i].to == e.to && all[i].cap == e.cap && all[i].flow == e.flow && all[i].cost == e.cost, "edges matches getEdge");
            check(0 <= e.flow && e.flow <= e.cap, "flow within [0, cap]");
            equal(unitAt(me, e.flow > 0 ? lll(e.flow) - 1 : 0), e.cost, "getEdge cost is the unit cost of the last unit");
            x[i] = e.flow; bal[me.u] -= e.flow; bal[me.v] += e.flow; cost += costOf(me, e.flow);
            lll c = me.u == me.v || capOf(me) == 0 ? 0 : max(mag(me.pieces[0].second), mag(me.pieces.back().second));
            w += c; top = max(top, c);}
        for (int v = 0; v < net.n; ++v) { equal(div[v], bal[v], "conservation to the tracked divergence"); }
        equal(total, cost, "edge costs sum to the returned costs");
        auto p = mf.potentials();
        lll B = min(w, lll(max(net.n - 1, 0)) * top);
        equal(net.n, lll(p.size()), "potentials size");
        for (int v = 0; v < net.n; ++v) { check(-3 * B <= p[v] && p[v] <= 2 * B, "potentials within [-3B, 2B]"); }
        for (int i = 0; i < m; ++i) {
            auto &me = net.es[i];
            if (x[i] < capOf(me)) { check(unitAt(me, x[i]) + p[me.u] - p[me.v] >= 0, "forward residual reduced cost >= 0"); }
            if (x[i] > 0) { check(lll(p[me.v]) - p[me.u] - unitAt(me, x[i] - 1) >= 0, "backward residual reduced cost >= 0"); }}
        return x;}
    // A call that returned F stopped legitimately: limit reached, t unreachable, or p[t] - p[s] >= below bounds every residual path.
    template<class Cap, class Cost>
    void stopped(MinCostFlow<Cap, Cost> &mf, const Net &net, const vector<lll> &x, const Call &c, lll F) {
        if (F == c.limit) { return; }
        vector<char> seen(net.n);
        vector<int> q{c.s};
        vector<vector<int>> out(net.n);
        for (auto &a : residual(net, x)) { out[a.u].push_back(a.v); }
        seen[c.s] = 1;
        for (int i = 0; i < int(q.size()); ++i) {
            for (int v : out[q[i]]) { if (!seen[v]) { seen[v] = 1; q.push_back(v); }}}
        if (!seen[c.t]) { return; }
        auto p = mf.potentials();
        check(lll(p[c.t]) - p[c.s] >= c.below, "stopped below the bound: every residual s-t path costs >= below");}
    // MaxFlow::pathDecomposition on the min-cost flow's edges: its s-t paths carry the whole cost (flow cycles cost 0).
    template<class Cap, class Cost>
    void recipe(const MinCostFlow<Cap, Cost> &mf, const Net &net, int s, int t, lll cost) {
        MaxFlow<lll> g(net.n);
        lll sum = 0;
        for (auto e : mf.edges()) { int id = g.addEdge(e.from, e.to, e.cap); g.changeEdge(id, e.cap, e.flow); }
        for (auto &[f, ids] : g.pathDecomposition(s, t)) {
            lll c = 0;
            for (int id : ids) { c += net.es[id].pieces[0].second; }
            sum += f * c;}
        equal(cost, sum, "MaxFlow::pathDecomposition paths carry the whole min cost");}

    // Random call sequences (edges added between calls unless a negative residual cycle appears), each against the oracle curve.
    template<class Cap, class Cost>
    void scenario(Net net, int calls, bool exhaustive, lll lo, lll hi, lll capHi, lll lenHi) {
        auto mf = build<Cap, Cost>(net);
        vector<lll> div(net.n), x(net.es.size());
        lll total = 0;
        bool linear = true;
        for (auto &e : net.es) { linear = linear && !e.convex; }
        auto ex = exhaustive ? std::make_unique<Exhaustive>(net) : nullptr;
        for (int k = 0; k < calls; ++k) {
            context = "seed=" + std::to_string(seed) + " " + show(net);
            if (k > 0 && rng() % 3 == 0) {
                auto extra = randomNet(net.n, 1, capHi, lo, hi, 30, 3, lenHi).es[0];
                Net next = net; next.es.push_back(extra);
                if (!closesNegativeCycle(net, certify(mf, net, div, total), extra) && (!exhaustive || space(next) <= 4096)) {
                    add(mf, extra, int(net.es.size()));
                    net = next; x.push_back(0); linear = false;
                    if (exhaustive) { ex = std::make_unique<Exhaustive>(net); }}}
            Call c = randomCall<Cap, Cost>(net.n, lo - 2, hi + 2, 6);
            context = "seed=" + std::to_string(seed) + " " + show(net) + show(c);
            auto curve = exhaustive ? ex->curve(div, c.s, c.t, c.limit, c.below) : unitCurve(net, x, c.s, c.t, c.limit, c.below);
            auto got = run(mf, c);
            compare(got, breakpoints(curve), c.slope);
            lll F = got.back().first;
            div[c.s] -= F; div[c.t] += F; total += got.back().second;
            stopped(mf, net, certify(mf, net, div, total), c, F);
            if (k == 0 && linear && c.limit == lll(MinCostFlow<Cap, Cost>::MAX) && c.below == lll(MinCostFlow<Cap, Cost>::INF)) { recipe(mf, net, c.s, c.t, got.back().second); }
            ++cases;}}
    template<class F>
    void typed(int round, F &&f) {
        switch (round % 5) {
            case 0: f(MinCostFlow<lng, lng>()); break;
            case 1: f(MinCostFlow<int, int>()); break;
            case 2: f(MinCostFlow<lll, lll>()); break;
            case 3: f(MinCostFlow<int, lng>()); break;
            default: f(MinCostFlow<lng, int>()); break;}}

    void smallCases() {
        int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 12000;
        for (int r = 0; r < rounds; ++r) {
            Net net;
            do { net = randomNet(randInt(2, 5), randInt(0, 6), 3, -4, 6, 25, 3, 2); } while (space(net) > 4096);
            typed(r, [&]<class Cap, class Cost>(MinCostFlow<Cap, Cost>) { scenario<Cap, Cost>(net, randInt(1, 4), true, -4, 6, 3, 2); });}
        cout << "PASS small exhaustive networks rounds=" << rounds << '\n';}
    void mediumCases() {
        int rounds = mode == "quick" ? 20 : mode == "full" ? 300 : 1000;
        for (int r = 0; r < rounds; ++r) {
            int n = randInt(6, 24);
            Net net = randomNet(n, randInt(n, 4 * n), 8, -15, 30, 25, 4, 4);
            typed(r, [&]<class Cap, class Cost>(MinCostFlow<Cap, Cost>) { scenario<Cap, Cost>(net, randInt(1, 5), false, -15, 30, 8, 4); });}
        cout << "PASS medium unit-step oracle networks rounds=" << rounds << '\n';}
    // Every network on 3 vertices whose six ordered pairs each take a cap and a cost from small sets; s = 0, t = 2.
    void familyCases() {
        vector<lll> caps = mode == "quick" ? vector<lll>{0, 1} : vector<lll>{0, 1, 2}, costs{-1, 2};
        int k = int(caps.size() * costs.size()), total = 1, runs = 0;
        const int pu[] = {0, 0, 1, 1, 2, 2}, pv[] = {1, 2, 0, 2, 0, 1};
        for (int i = 0; i < 6; ++i) { total *= k; }
        for (int code = 0; code < total; ++code) {
            Net net{3, {}};
            for (int i = 0, c = code; i < 6; ++i, c /= k) { net.es.push_back({pu[i], pv[i], false, {{caps[c % k / 2], costs[c % 2]}}}); }
            if (negativeCycle(net, vector<lll>(6))) { continue; }
            vector<lll> best(13, BIG), c{0};
            int size = int(space(net));
            for (int code2 = 0; code2 < size; ++code2) {
                lll div[3] = {0, 0, 0}, cost = 0;
                for (int i = 0, rest = code2; i < 6; ++i) {
                    int cap = int(capOf(net.es[i])), x = rest % (cap + 1);
                    rest /= cap + 1; div[pu[i]] -= x; div[pv[i]] += x; cost += x * net.es[i].pieces[0].second;}
                if (div[1] == 0 && div[2] >= 0) { best[int(div[2])] = min(best[int(div[2])], cost); }}
            context = "family " + show(net);
            equal(0, best[0], "oracle: zero flow is optimal without negative cycles");
            while (best[c.size()] < BIG) { c.push_back(best[c.size()]); }
            auto mf = build<lng, lng>(net);
            Call call{0, 2, lll(MinCostFlow<lng, lng>::MAX), lll(MinCostFlow<lng, lng>::INF), code % 2 == 0};
            compare(run(mf, call), breakpoints(c), call.slope);
            vector<lll> div{-lll(c.size()) + 1, 0, lll(c.size()) - 1};
            stopped(mf, net, certify(mf, net, div, c.back()), call, lll(c.size()) - 1);
            recipe(mf, net, 0, 2, c.back());
            auto again = mf.flow(0, 2);
            equal(0, again.first, "repeated flow adds nothing"); equal(0, again.second, "repeated flow costs nothing");
            ++runs; ++cases;}
        cout << "PASS exhaustive three-vertex family networks=" << runs << '\n';}

    // Scaling caps by K and costs by L maps every breakpoint (F, C) to (K * F, K * L * C): oracle-checked networks at large magnitudes.
    template<class Cap, class Cost>
    void scaled(lll kHi, lll lHi) {
        Net net;
        do { net = randomNet(randInt(2, 5), randInt(1, 6), 3, -4, 6, 25, 3, 2); } while (space(net) > 4096);
        lll K = rand(1, kHi), L = rand(1, lHi);
        Net big = net;
        for (auto &e : big.es) { for (auto &[len, w] : e.pieces) { len *= K; w *= L; }}
        Call c = randomCall<Cap, Cost>(net.n, -6, 8, 6);
        context = "scaled K=" + str(K) + " L=" + str(L) + " " + show(net) + show(c);
        auto bp = breakpoints(Exhaustive(net).curve(vector<lll>(net.n), c.s, c.t, c.limit, c.below));
        for (auto &[f, w] : bp) { f *= K; w *= K * L; }
        if (c.limit != lll(MinCostFlow<Cap, Cost>::MAX)) { c.limit *= K; }
        if (c.below != lll(MinCostFlow<Cap, Cost>::INF)) { c.below *= L; }
        auto mf = build<Cap, Cost>(big);
        compare(run(mf, c), bp, c.slope);
        vector<lll> div(net.n);
        div[c.s] -= bp.back().first; div[c.t] += bp.back().first;
        stopped(mf, big, certify(mf, big, div, bp.back().second), c, bp.back().first);
        ++cases;}
    void wideCases() {
        int rounds = mode == "quick" ? 60 : mode == "full" ? 600 : 3000;
        for (int r = 0; r < rounds; ++r) {
            if (r % 3 == 0) { scaled<lng, lng>(lll(1) << 40, lll(1) << 50); }
            else if (r % 3 == 1) { scaled<lll, lll>(lll(1) << 62, lll(1) << 54); }
            else { scaled<int, int>(lll(1) << 20, lll(1) << 20); }}
        cout << "PASS scaled networks rounds=" << rounds << " (capacities below 2^64, unit costs below 2^58)\n";}

    // Documented limits under UBSan: |cost| = LIM with cap MAX; chains whose |costs| sum to LIM, under alternating sinks.
    template<class Cap, class Cost>
    void boundCases(bool full) {
        using M = MinCostFlow<Cap, Cost>;
        lll lim = M::LIM, cmax = M::MAX;
        for (lll sign : {1, -1}) {
            Net net{2, {{0, 1, false, {{full ? cmax : 2, sign * lim}}}}};
            auto mf = build<Cap, Cost>(net);
            context = "bound single edge cost=" + str(sign * lim);
            auto [f, c] = mf.flow(0, 1);
            equal(net.es[0].pieces[0].first, f, "flow at MAX"); equal(f * sign * lim, c, "cost at LIM");
            certify(mf, net, {-f, f}, c); ++cases;}
        for (int dag = 0; dag < 2; ++dag) {
            Net net{17, {}};
            for (int i = 0; i < 16; ++i) {
                net.es.push_back({i, i + 1, false, {{2, (dag ? -1 : 1) * (lim / 16)}}});
                net.es.push_back(dag ? E{i, min(i + 2, 16), false, {{1, 0}}} : E{i + 1, i, false, {{1, 0}}});}
            // Loops and zero-capacity edges lie on no residual path, so they stay outside B.
            net.es.push_back({5, 5, false, {{3, lim}}}); net.es.push_back({16, 0, false, {{0, -lim}}});
            auto mf = build<Cap, Cost>(net);
            vector<lll> div(17);
            lll total = 0;
            const Call calls[] = {{0, 16, 1, BIG, false}, {0, 8, 1, BIG, true}, {3, 16, 1, BIG, false}, {16, 0, 2, BIG, true},
                {0, 16, cmax, BIG, true}, {5, 12, cmax, BIG, false}, {12, 2, cmax, BIG, true}, {0, 9, cmax, BIG, false}};
            for (auto c : calls) {
                c.below = lll(M::INF);
                context = "bound chain dag=" + std::to_string(dag) + show(c);
                auto got = run(mf, c);
                if (c.slope) { slopeShape(got, c.below); }
                div[c.s] -= got.back().first; div[c.t] += got.back().first; total += got.back().second;
                stopped(mf, net, certify(mf, net, div, total), c, got.back().first); ++cases;}}}

    // Alternating (0, 2) and (1, 3) calls lower both sinks' potentials by 10 per round unless a new sink re-anchors them.
    void driftCases() {
        const lll MAXL = lll(MinCostFlow<lng, lng>::MAX), INFL = lll(MinCostFlow<lng, lng>::INF);
        Net net{4, {{0, 3, false, {{1000, 0}}}, {0, 2, false, {{1000, 5}}}, {1, 2, false, {{1000, 0}}}, {1, 3, false, {{1000, 5}}}}};
        for (int push = 0; push < 2; ++push) {
            auto mf = build<lng, lng>(net);
            vector<lll> div(4);
            lll total = 0;
            for (int k = 0; k < 300; ++k) {
                Call c{k % 2, k % 2 + 2, push ? 1 : MAXL, push ? INFL : 0, k % 4 < 2};
                context = "drift" + show(c);
                auto got = run(mf, c);
                div[c.s] -= got.back().first; div[c.t] += got.back().first; total += got.back().second;
                stopped(mf, net, certify(mf, net, div, total), c, got.back().first); ++cases;}}
        cout << "PASS alternating-sink drift networks\n";}

    void degenerateCases() {
        context = "degenerate";
        MinCostFlow<lng, lng> none, empty(2), z(2), one(3);
        check(none.potentials().empty() && none.edges().empty(), "zero vertices");
        auto sl = empty.slope(0, 1);
        check(sl.size() == 1 && sl[0].first == 0 && sl[0].second == 0, "no edges: slope is (0, 0)");
        check(empty.flow(1, 0) == pair<lng, lll>{0, 0} && empty.potentials() == vector<lng>{0, 0}, "no edges: flow 0, potentials 0");
        equal(0, z.addConvexEdge(0, 1, {}), "empty pieces: edge id");
        auto e = z.getEdge(0);
        check(e.from == 0 && e.to == 1 && e.cap == 0 && e.flow == 0 && e.cost == 0 && z.flow(0, 1).first == 0, "empty pieces: zero-capacity edge of cost 0");
        one.addEdge(0, 1, 5, 3); one.addEdge(1, 2, 5, -1); one.addEdge(2, 2, 4, 0);
        check(one.flow(0, 2, 0) == pair<lng, lll>{0, 0}, "limit 0 adds nothing");
        check(one.flow(0, 2, 5, 2) == pair<lng, lll>{0, 0}, "below equal to the path cost adds nothing");
        check(one.flow(0, 2, 3, 3) == pair<lng, lll>{3, 6}, "below above the path cost, limited");
        check(one.flow(0, 2) == pair<lng, lll>{2, 4} && one.flow(0, 2) == pair<lng, lll>{0, 0}, "resume to the maximum, then nothing");
        check(one.getEdge(2).flow == 0 && one.flow(2, 0) == pair<lng, lll>{5, -10}, "loop unused; reverse residual flow");
        cases += 4;
        cout << "PASS degenerate networks\n";}

    // Unit-capacity bipartite networks against hungarian (min and max) and linearSumAssignment (cost curve, any size).
    void assignmentCases() {
        int k = mode == "quick" ? 15 : DEBUG_HEAP ? 25 : mode == "full" ? 60 : 120, reps = mode == "quick" ? 3 : 6;
        for (int rep = 0; rep < reps; ++rep) {
            int rows = k, cols = k + randInt(0, 5), s = rows + cols, t = s + 1;
            vector<vector<lng>> a(rows, vector<lng>(cols));
            vector<tuple<int, int, lng>> sparse;
            for (auto &row : a) { for (auto &x : row) { x = lng(rand(-1000000000, 1000000000)); }}
            for (int sign : {1, -1}) {
                Net net{t + 1, {}};
                for (int i = 0; i < rows; ++i) { net.es.push_back({s, i, false, {{1, 0}}}); }
                for (int j = 0; j < cols; ++j) { net.es.push_back({rows + j, t, false, {{1, 0}}}); }
                for (int i = 0; i < rows; ++i) {
                    for (int j = 0; j < cols; ++j) { net.es.push_back({i, rows + j, false, {{1, sign * a[i][j]}}}); }}
                context = "assignment rows=" + std::to_string(rows) + " cols=" + std::to_string(cols) + " sign=" + std::to_string(sign);
                auto mf = build<lng, lng>(net);
                auto [f, c] = mf.flow(s, t);
                equal(rows, f, "assignment size");
                equal(sign * hungarian(a, sign < 0).cost, c, "assignment cost equals hungarian");
                vector<lll> div(t + 1);
                div[s] = -f; div[t] = f;
                certify(mf, net, div, c); ++cases;}
            for (int i = 0; i < rows; ++i) {
                for (int j = 0; j < cols; ++j) { if (rng() % 4 == 0) { sparse.emplace_back(i, j, a[i][j]); }}}
            Net net{t + 1, {}};
            for (int i = 0; i < rows; ++i) { net.es.push_back({s, i, false, {{1, 0}}}); }
            for (int j = 0; j < cols; ++j) { net.es.push_back({rows + j, t, false, {{1, 0}}}); }
            for (auto [i, j, w] : sparse) { net.es.push_back({i, rows + j, false, {{1, w}}}); }
            context = "sparse assignment edges=" + std::to_string(sparse.size());
            auto curve = linearSumAssignment(rows, cols, sparse).curve;
            auto mf = build<lng, lng>(net);
            auto sl = run(mf, Call{s, t, lll(MinCostFlow<lng, lng>::MAX), lll(MinCostFlow<lng, lng>::INF), true});
            slopeShape(sl, BIG);
            equal(lll(curve.size()) - 1, sl.back().first, "slope reaches the maximum matching size");
            for (int j = 0; j < int(curve.size()); ++j) { equal(curve[j], interpolate(sl, j), "slope interpolates the per-size optimum"); }
            auto any = build<lng, lng>(net).flow(s, t, MinCostFlow<lng, lng>::MAX, 0);
            equal(linearSumAssignment(rows, cols, sparse, false, false).cost, any.second, "below 0 gives the optimum of any size");
            cases += 2;}
        cout << "PASS assignment networks k=" << k << '\n';}

    template<class Cap, class Cost>
    vector<pair<lll, lll>> largeCall(MinCostFlow<Cap, Cost> &mf, const Net &net, vector<lll> &div, lll &total, const Call &c) {
        context = "large " + show(net) + show(c);
        auto sl = run(mf, Call{c.s, c.t, c.limit, c.below, true});
        slopeShape(sl, c.below);
        div[c.s] -= sl.back().first; div[c.t] += sl.back().first; total += sl.back().second;
        stopped(mf, net, certify(mf, net, div, total), c, sl.back().first);
        ++cases; return sl;}
    void largeCases() {
        int n = mode == "quick" ? 2000 : DEBUG_HEAP ? 20000 : mode == "full" ? 100000 : 300000;
        const lll MAXL = lll(MinCostFlow<lng, lng>::MAX), INFL = lll(MinCostFlow<lng, lng>::INF);
        // Chain DAG labelled against its order, with negative skips: Kahn order settles the potentials in one round.
        Net chain{n, {}};
        for (int i = 0; i + 1 < n; ++i) {
            chain.es.push_back({n - 1 - i, n - 2 - i, false, {{rand(1, 3), rand(-1000000, 1000000)}}});
            if (i + 2 < n && rng() % 2) { chain.es.push_back({n - 1 - i, n - 3 - i, false, {{1, rand(-1000000, 0)}}}); }}
        {
            auto mf = build<lng, lng>(chain);
            vector<lll> div(n);
            lll total = 0;
            largeCall(mf, chain, div, total, {n - 1, 0, MAXL, INFL, true});}
        // Cyclic sparse network with potential-shifted costs (Bellman-Ford rounds), mixed calls and negative arcs added after flow.
        int rn = DEBUG_HEAP ? min(n / 20, 300) : max(50, n / 20);
        Net rnd = randomNet(rn, 5 * rn, 20, -1000000, 1000000, 10, 6, 10, 1);
        auto mf = build<lng, lng>(rnd);
        vector<lll> div(rn);
        lll total = 0;
        for (int k = 0; k < 8; ++k) {
            if (k % 3 == 2) {
                auto extra = randomNet(rn, 1, 20, -3000000, 1000000, 0, 1, 1, 0).es[0];
                if (!closesNegativeCycle(rnd, certify(mf, rnd, div, total), extra)) { add(mf, extra, int(rnd.es.size())); rnd.es.push_back(extra); }}
            largeCall(mf, rnd, div, total, randomCall<lng, lng>(rn, -2000000, 2000000, 40));}
        // Convex edges against their expansion into parallel linear arcs: identical breakpoints, both certified.
        int cn = DEBUG_HEAP ? min(max(30, n / 250), 100) : max(30, n / 250);
        Net conv = randomNet(cn, 4 * cn, 10, -1000, 1000, 60, 30, 5, 1), flat{cn, {}};
        for (auto &e : conv.es) { for (auto p : e.pieces) { flat.es.push_back({e.u, e.v, false, {p}}); }}
        auto cm = build<lng, lng>(conv), fm = build<lng, lng>(flat);
        vector<lll> cdiv(cn), fdiv(cn);
        lll ctotal = 0, ftotal = 0;
        for (int k = 0; k < 6; ++k) {
            auto c = randomCall<lng, lng>(cn, -2000, 4000, 30);
            auto want = largeCall(fm, flat, fdiv, ftotal, c), got = largeCall(cm, conv, cdiv, ctotal, c);
            context = "convex against parallel arcs " + show(conv) + show(c);
            compare(got, want, true);}
        cout << "PASS large certified networks (reverse-labelled DAG chain n=" << n << ", cyclic potential-shifted n=" << rn << ", convex n=" << cn << ")\n";}

    void invalid(const string &name) {
        using M = MinCostFlow<lng, lng>;
        M mf(3); mf.addEdge(0, 1, 5, 2);
        if (name == "negative-size") { M bad(-1); }
        else if (name == "addEdge-range") { mf.addEdge(0, 3, 1, 0); }
        else if (name == "addEdge-negative") { mf.addEdge(0, 1, -1, 0); }
        else if (name == "addEdge-cost-high") { mf.addEdge(0, 1, 1, M::LIM + 1); }
        else if (name == "addEdge-cost-low") { mf.addEdge(0, 1, 1, -M::LIM - 1); }
        else if (name == "convex-range") { mf.addConvexEdge(3, 0, {{1, 0}}); }
        else if (name == "convex-length") { mf.addConvexEdge(0, 1, {{1, 0}, {0, 1}}); }
        else if (name == "convex-order") { mf.addConvexEdge(0, 1, {{1, 2}, {1, 1}}); }
        else if (name == "convex-overflow") { mf.addConvexEdge(0, 1, {{M::MAX, 0}, {1, 1}}); }
        else if (name == "convex-cost") { mf.addConvexEdge(0, 1, {{1, 0}, {1, M::LIM + 1}}); }
        else if (name == "getEdge-range") { mf.getEdge(1); }
        else if (name == "flow-same") { mf.flow(1, 1); }
        else if (name == "flow-range") { mf.flow(0, 3); }
        else if (name == "flow-limit") { mf.flow(0, 1, -1); }
        else if (name == "slope-same") { mf.slope(2, 2); }
        else if (name == "slope-limit") { mf.slope(0, 1, -1); }
        else if (name == "negative-cycle") { mf.addEdge(1, 2, 1, -3); mf.addEdge(2, 1, 1, 1); mf.flow(0, 2); }
        else if (name == "negative-loop") { mf.addEdge(2, 2, 1, -1); mf.flow(0, 1); }
        else if (name == "negative-cycle-after-flow") { mf.addEdge(1, 2, 9, 1); mf.flow(0, 2); mf.addEdge(2, 1, 1, -2); mf.flow(0, 2); }
        else if (name == "potentials-cycle") { mf.addEdge(1, 2, 1, -3); mf.addEdge(2, 1, 1, 1); mf.potentials(); }
        else if (name == "path-bound") { M big(3); big.addEdge(0, 1, 1, M::LIM); big.addEdge(1, 2, 1, M::LIM); big.flow(0, 2); }
        else if (name == "total-bound") { MinCostFlow<lll, lll> big(2); big.addEdge(0, 1, lll(1) << 100, lll(1) << 30); big.flow(0, 1); }
        else if (name == "total-sum-bound") { MinCostFlow<lll, lll> big(2); big.addEdge(0, 1, lll(1) << 100, lll(1) << 25); big.addEdge(0, 1, lll(1) << 100, lll(1) << 25); big.flow(0, 1); }
        else if (name == "potentials-bound") { M big(3); big.addEdge(0, 1, 1, -M::LIM); big.addEdge(1, 2, 1, -M::LIM); big.potentials(); }
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
        degenerateCases(); driftCases(); familyCases(); smallCases(); mediumCases(); wideCases();
        boundCases<lng, lng>(true); boundCases<int, int>(true); boundCases<lll, lll>(false); boundCases<int, lng>(true);
        cout << "PASS bound networks (LIM costs, MAX capacities, alternating sinks)\n";
        assignmentCases(); largeCases();
        cout << "PASS min cost flow seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL min cost flow seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
