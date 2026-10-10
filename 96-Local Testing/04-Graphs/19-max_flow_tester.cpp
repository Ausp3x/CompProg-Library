#include "../../04-Graphs/19-max_flow.hpp"
#include "../../04-Graphs/01-graph.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    std::mt19937_64 rng;
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

    struct E { int u, v; lll cap, rcap; };
    struct Net { int n, s, t; vector<E> es; };
    string show(const Net &net) {
        string res = "n=" + std::to_string(net.n) + " s=" + std::to_string(net.s) + " t=" + std::to_string(net.t) + " edges=";
        for (auto e : net.es) { res += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + "," + str(e.cap) + "," + str(e.rcap) + ")"; }
        return res;}
    // Max-flow min-cut theorem: brute minimum over every s-side vertex subset.
    lll brute(const Net &net) {
        lll best = -1;
        for (uint mask = 0; mask < (1u << net.n); ++mask) {
            if (!((mask >> net.s) & 1) || ((mask >> net.t) & 1)) { continue; }
            lll cut = 0;
            for (auto e : net.es) {
                bool a = (mask >> e.u) & 1, b = (mask >> e.v) & 1;
                if (a && !b) { cut += e.cap; }
                if (b && !a) { cut += e.rcap; }}
            if (best < 0 || cut < best) { best = cut; }}
        return best;}
    template<class Cap>
    MaxFlow<Cap> build(const Net &net) {
        MaxFlow<Cap> mf(net.n);
        for (int i = 0; i < int(net.es.size()); ++i) {
            auto e = net.es[i];
            equal(i, mf.addEdge(e.u, e.v, Cap(e.cap), Cap(e.rcap)), "addEdge returns sequential id");}
        return mf;}
    // Simple s-t paths along flow directions, per-edge usage within |flow|, amounts summing to max(value, 0).
    template<class Cap>
    void decomposition(const MaxFlow<Cap> &mf, const Net &net, lll value) {
        lll total = 0; vector<lll> used(net.es.size());
        for (auto &[f, ids] : mf.pathDecomposition(net.s, net.t)) {
            check(f > 0 && !ids.empty(), "decomposition amount positive");
            int u = net.s; vector<char> seen(net.n); seen[u] = true;
            for (int id : ids) {
                auto e = mf.getEdge(id);
                int v = e.flow > 0 && e.from == u ? e.to : e.flow < 0 && e.to == u ? e.from : -1;
                check(v >= 0, "decomposition follows flow direction");
                used[id] += f;
                check(used[id] <= (e.flow < 0 ? -lll(e.flow) : lll(e.flow)), "decomposition usage within edge flow");
                check(!seen[v], "decomposition path is simple"); seen[v] = true; u = v;}
            check(u == net.t, "decomposition path ends at t"); total += f;}
        equal(max(value, lll(0)), total, "decomposition amounts sum to flow value");}
    // Feasible conserving flow of the given value; when maximum, an equal-capacity residual cut certifies optimality.
    template<class Cap>
    void certify(const MaxFlow<Cap> &mf, const Net &net, lll value, bool maximum) {
        vector<lll> bal(net.n);
        auto all = mf.edges();
        equal(lll(net.es.size()), lll(all.size()), "edges size");
        for (int i = 0; i < int(net.es.size()); ++i) {
            auto e = mf.getEdge(i); auto x = net.es[i];
            check(e.from == x.u && e.to == x.v && lll(e.cap) == x.cap, "getEdge endpoints and capacity");
            check(all[i].from == e.from && all[i].to == e.to && all[i].cap == e.cap && all[i].flow == e.flow, "edges matches getEdge");
            check(-x.rcap <= e.flow && e.flow <= x.cap, "flow within [-rcap, cap]");
            bal[x.u] -= e.flow; bal[x.v] += e.flow;}
        for (int v = 0; v < net.n; ++v) {
            if (v != net.s && v != net.t) { equal(0, bal[v], "conservation"); }}
        equal(value, -bal[net.s], "net outflow of s equals returned flow");
        equal(value, bal[net.t], "net inflow of t equals returned flow");
        if (!maximum) { return; }
        auto side = mf.minCut(net.s);
        check(int(side.size()) == net.n && side[net.s] && !side[net.t], "minCut separates s from t");
        lll cut = 0; vector<int> expected;
        for (int i = 0; i < int(net.es.size()); ++i) {
            auto x = net.es[i]; bool a = side[x.u], b = side[x.v];
            if (a && !b) { cut += x.cap; }
            if (b && !a) { cut += x.rcap; }
            if ((a && !b && x.cap > 0) || (b && !a && x.rcap > 0)) { expected.push_back(i); }}
        equal(value, cut, "minCut capacity equals flow value");
        check(mf.minCutEdges(net.s) == expected, "minCutEdges exact list");
        decomposition(mf, net, value);}

    Net randomNet(int n, int m, lll hi, bool bidirectional) {
        Net net{n, 0, 0, {}};
        net.s = int(rng() % n); net.t = int(rng() % (n - 1)); net.t += net.t >= net.s;
        for (int i = 0; i < m; ++i) {
            int u = int(rng() % n), v = rng() % 5 == 0 ? u : int(rng() % n);
            if (rng() % 7 == 0) { u = net.s; }
            if (rng() % 7 == 0) { v = net.t; }
            net.es.push_back({u, v, rand(0, hi), bidirectional && rng() % 2 ? rand(0, hi) : 0});}
        return net;}
    template<class Cap>
    void engines(const Net &net, lll best) {
        lll cap = std::numeric_limits<Cap>::max(), supply = 0, want = min(best, cap);
        for (auto e : net.es) { supply += (e.u == net.s ? e.cap : 0) + (e.v == net.s ? e.rcap : 0); }
        for (int engine = 0; engine < 5; ++engine) {
            if (engine == 2 && supply > cap) { continue; }
            auto mf = build<Cap>(net);
            if (engine == 0) { equal(want, mf.flow(net.s, net.t), "flow"); }
            if (engine == 1) { equal(want, mf.capacityScalingDinic(net.s, net.t), "capacityScalingDinic"); }
            if (engine == 2) { equal(best, mf.pushRelabel(net.s, net.t), "pushRelabel"); }
            if (engine >= 3) {
                Cap limit = Cap(rand(0, min(best + 2, cap)));
                lll first = engine == 3 ? mf.flow(net.s, net.t, limit) : mf.capacityScalingDinic(net.s, net.t, limit);
                equal(min(best, lll(limit)), first, "flow limit");
                certify(mf, net, first, false);
                lll rest = supply <= cap && rng() % 2 ? mf.pushRelabel(net.s, net.t) : engine == 3 ? mf.flow(net.s, net.t, Cap(cap - first)) : mf.capacityScalingDinic(net.s, net.t, Cap(cap - first));
                equal(want, first + rest, "resume after limit");}
            certify(mf, net, want, want == best);
            if (want == best) { equal(0, mf.flow(net.s, net.t), "flow after maximum adds nothing"); }}}
    void dynamicCase(Net net) {
        auto mf = build<lng>(net);
        lll value = 0, start = rng() % 4, limit = rand(0, 8);
        value += start == 0 ? mf.flow(net.s, net.t) : start == 1 ? mf.pushRelabel(net.s, net.t)
            : start == 2 ? mf.flow(net.s, net.t, lng(limit)) : mf.capacityScalingDinic(net.s, net.t, lng(limit));
        if (rng() % 3 == 0) { value += mf.pushRelabel(net.s, net.t); }
        int rounds = 1 + int(rng() % 6);
        for (int r = 0; r < rounds; ++r) {
            if (net.es.empty() || rng() % 3 == 0) {
                E e{int(rng() % net.n), int(rng() % net.n), rand(0, 6), rng() % 2 ? rand(0, 6) : 0};
                net.es.push_back(e); mf.addEdge(e.u, e.v, lng(e.cap), lng(e.rcap));
                value += mf.flow(net.s, net.t); context = show(net);
                equal(brute(net), value, "addEdge after flow then flow");}
            else {
                int i = int(rng() % net.es.size()); lll c = rand(0, 7);
                net.es[i].cap = c; context = show(net);
                value += mf.changeCapacity(i, lng(c), net.s, net.t);
                equal(brute(net), value, "changeCapacity keeps a maximum flow");}
            certify(mf, net, value, true);}
        if (net.es.empty()) { return; }
        int i = int(rng() % net.es.size()); lng c = lng(rand(0, 9)), f = lng(rand(-net.es[i].rcap, c));
        mf.changeEdge(i, c, f); auto e = mf.getEdge(i);
        check(e.cap == c && e.flow == f && e.from == net.es[i].u && e.to == net.es[i].v, "changeEdge raw set");}
    // Arbitrary valid flows (s-t paths, t-s paths, cycles through any vertex) set by changeEdge.
    void randomFlowCase() {
        int n = 2 + int(rng() % 6);
        Net net{n, int(rng() % n), 0, {}};
        net.t = int(rng() % (n - 1)); net.t += net.t >= net.s;
        vector<lll> flows; lll value = 0;
        for (int k = int(rng() % 6); k > 0; --k) {
            int kind = int(rng() % 3); lll a = rand(1, 5);
            vector<int> walk{kind == 0 ? net.s : kind == 2 ? net.t : int(rng() % n)};
            for (int len = int(rng() % 4); len > 0; --len) { walk.push_back(int(rng() % n)); }
            walk.push_back(kind == 0 ? net.t : kind == 2 ? net.s : walk[0]);
            value += kind == 0 ? a : kind == 2 ? -a : 0;
            for (int j = 0; j + 1 < int(walk.size()); ++j) {
                if (rng() % 3) { net.es.push_back({walk[j], walk[j + 1], a + rand(0, 3), rand(0, 2)}); flows.push_back(a); }
                else { net.es.push_back({walk[j + 1], walk[j], rand(0, 3), a + rand(0, 2)}); flows.push_back(-a); }}}
        auto mf = build<lng>(net); ++cases; context = show(net) + " arbitrary flow";
        for (int i = 0; i < int(net.es.size()); ++i) { mf.changeEdge(i, lng(net.es[i].cap), lng(flows[i])); }
        certify(mf, net, value, false); decomposition(mf, net, value);}
    void legacyCase(const Net &net) {
        int n = net.n - 1;
        vector<vector<int>> adj(net.n);
        vector<vector<lng>> cap(net.n, vector<lng>(net.n));
        Net simple{net.n, net.s, net.t, {}};
        for (auto e : net.es) {
            adj[e.u].push_back(e.v); cap[e.u][e.v] = lng(e.cap);
            if (rng() % 3 == 0) { adj[e.u].push_back(e.v); }}
        for (int u = 0; u < net.n; ++u) {
            for (int v = 0; v < net.n; ++v) {
                if (std::find(adj[u].begin(), adj[u].end(), v) != adj[u].end()) { simple.es.push_back({u, v, cap[u][v], 0}); }}}
        lll best = brute(simple);
        if (best > std::numeric_limits<lng>::max()) { return; }
        EdmondsKarp ek(n, adj, cap);
        lng b = ek.augmentFlow(net.s, net.t);
        vector<int> dist(net.n, -1); vector<int> q{net.s}; dist[net.s] = 0;
        for (int i = 0; i < int(q.size()); ++i) {
            for (auto e : simple.es) {
                if (e.u == q[i] && e.cap > 0 && dist[e.v] < 0) { dist[e.v] = dist[e.u] + 1; q.push_back(e.v); }}}
        if (dist[net.t] < 0) { equal(0, b, "augmentFlow no path"); }
        else {
            lng low = std::numeric_limits<lng>::max(); int len = 0;
            for (int v = net.t; v != net.s; v = ek.par[v], ++len) {
                check(ek.par[v] >= 0 && ek.ocap[ek.par[v]][v] > 0, "augmentFlow parent arc"); low = min(low, ek.ocap[ek.par[v]][v]);}
            equal(dist[net.t], len, "augmentFlow shortest path");
            equal(low, b, "augmentFlow bottleneck");}
        ek.getMinCut(net.s, net.t);
        equal(best, ek.max_flow, "EdmondsKarp getMaxFlow");
        equal(0, ek.add_flow, "no augmenting path after getMaxFlow");
        check(ek.in_S[net.s] && !ek.in_S[net.t], "getMinCut sides");
        lll cut = 0; vector<pair<int, int>> expected;
        for (auto e : simple.es) {
            if (ek.in_S[e.u] && !ek.in_S[e.v]) { cut += e.cap; expected.emplace_back(e.u, e.v); }}
        equal(best, cut, "cut_set capacity");
        check(ek.cut_set == expected, "cut_set exact ordered pairs");
        vector<lll> bal(net.n);
        for (int u = 0; u < net.n; ++u) {
            for (int v = 0; v < net.n; ++v) {
                equal(lll(ek.ocap[u][v]) + ek.ocap[v][u], lll(ek.ncap[u][v]) + ek.ncap[v][u], "residual pair sum");
                check(ek.ncap[u][v] >= 0, "residual nonnegative");
                bal[v] += ek.ocap[u][v] - ek.ncap[u][v];}}
        for (int v = 0; v < net.n; ++v) {
            if (v != net.s && v != net.t) { equal(0, bal[v], "legacy residual conservation"); }}
        equal(best, ek.getMaxFlow(net.s, net.t), "repeated getMaxFlow");
        equal(0, EdmondsKarp(n, adj, cap).getMaxFlow(net.s, net.s), "legacy s == t");}
    void graphCase(int n, int m, bool directed) {
        Graph g(n, directed);
        for (int i = 0; i < m; ++i) { g.addEdge(int(rng() % n), int(rng() % n), lng(rng() % 5)); }
        Net net{n, 0, n - 1, {}};
        for (auto e : g.edges) { net.es.push_back({e.u, e.v, e.w, directed ? 0 : e.w}); }
        context = show(net) + (directed ? " Graph directed" : " Graph undirected");
        MaxFlow<lng> mf(g); MaxFlow<int> csr{CsrGraph(g)};
        equal(brute(net), mf.flow(0, n - 1), "MaxFlow(Graph) flow");
        equal(brute(net), csr.flow(0, n - 1), "MaxFlow(CsrGraph) flow");
        certify(mf, net, brute(net), true);}

    void smallCases() {
        rng.seed(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 4000 : 20000;
        for (int r = 0; r < rounds; ++r) {
            int n = 2 + int(rng() % 7), m = int(rng() % 18);
            lll hi = r % 4 == 0 ? 1 : r % 4 == 1 ? 6 : r % 4 == 2 ? lll(1) << 40 : lll(1) << 61;
            Net net = randomNet(n, m, hi, r % 3 != 0); ++cases;
            context = show(net); lll best = brute(net);
            engines<lng>(net, best); engines<lll>(net, best);
            if (hi <= 6) { engines<int>(net, best); dynamicCase(net); }
            if (r % 3 == 0) { context = show(net) + " legacy"; legacyCase(net); }
            if (r % 5 == 0) { graphCase(n, m, r % 2); }
            randomFlowCase();}
        cout << "PASS random small networks against brute min cut: Dinic, scaling, push-relabel, limits, dynamic, legacy, Graph rounds=" << rounds << '\n';
        int top = mode == "quick" ? 2 : 3, pairs = top * top, total = 1;
        for (int i = 0; i < pairs; ++i) { total *= 3; }
        for (int mask = 0; mask < total; ++mask) {
            Net net{top, 0, top - 1, {}};
            for (int i = 0, x = mask; i < pairs; ++i, x /= 3) {
                if (x % 3) { net.es.push_back({i / top, i % top, x % 3, 0}); }}
            ++cases; context = show(net); engines<lng>(net, brute(net));}
        cout << "PASS exhaustive looped multiplicity networks n=" << top << " cases=" << total << '\n';}
    void wideCases() {
        Net net{4, 0, 3, {{0, 1, lll(1) << 100, 0}, {0, 2, lll(1) << 100, 0}, {1, 3, lll(1) << 99, lll(1) << 98}, {2, 3, lll(1) << 101, 0}, {1, 2, 5, 7}}};
        context = show(net); lll best = brute(net); engines<lll>(net, best);
        Net big{3, 0, 2, {}};
        for (int i = 0; i < 4; ++i) { big.es.push_back({0, 1, std::numeric_limits<lng>::max() / 2, 0}); big.es.push_back({1, 2, std::numeric_limits<lng>::max(), 0}); }
        context = show(big);
        auto mf = build<lng>(big);
        equal(std::numeric_limits<lng>::max(), mf.flow(0, 2), "flow saturates at Cap max");
        certify(mf, big, std::numeric_limits<lng>::max(), false);
        auto wide = build<lll>(big);
        equal(brute(big), wide.flow(0, 2), "lll flow exceeds lng");
        certify(wide, big, brute(big), true);
        cout << "PASS 128-bit capacities and Cap-max saturation\n";}

    void mediumCases() {
        int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 1500;
        for (int r = 0; r < rounds; ++r) {
            int n = 20 + int(rng() % 180), m = n + int(rng() % (6 * n));
            Net net = randomNet(n, m, r % 3 == 0 ? 1 : r % 3 == 1 ? 20 : 1000000000, r % 2); ++cases;
            context = "medium round=" + std::to_string(r) + " " + show(net);
            lll value = -1;
            for (int engine = 0; engine < 4; ++engine) {
                auto mf = build<lng>(net);
                lll v = engine == 0 ? mf.flow(net.s, net.t) : engine == 1 ? mf.capacityScalingDinic(net.s, net.t) : mf.pushRelabel(net.s, net.t);
                if (engine == 3) { v += mf.pushRelabel(net.s, net.t); }
                certify(mf, net, v, true);
                if (value >= 0) { equal(value, v, "engines agree"); }
                value = v;}}
        cout << "PASS medium certified networks n in [20, 200) rounds=" << rounds << '\n';}
    template<class F>
    void largeEngines(const Net &net, lll expected, F extra) {
        for (int engine = 0; engine < 3; ++engine) {
            auto mf = build<lng>(net);
            lll v = engine == 0 ? mf.flow(net.s, net.t) : engine == 1 ? mf.capacityScalingDinic(net.s, net.t) : mf.pushRelabel(net.s, net.t);
            if (expected >= 0) { equal(expected, v, "large engine value"); }
            certify(mf, net, v, true); extra(mf, v);}}
    void largeCases() {
        int n = mode == "quick" ? 2000 : mode == "full" ? 200000 : 500000;
        Net chain{n, 0, n - 1, {}}; lll low = lll(1) << 62;
        for (int u = 0; u + 1 < n; ++u) { lll c = rand(1, 1000000000); chain.es.push_back({u, u + 1, c, 0}); low = min(low, c); }
        context = "chain n=" + std::to_string(n);
        largeEngines(chain, low, [&](MaxFlow<lng> &mf, lll v) {
            int i = n / 2; lng c = lng(chain.es[i].cap);
            lll d = mf.changeCapacity(i, 0, 0, n - 1); equal(-v, d, "chain cut to zero");
            d = mf.changeCapacity(i, c, 0, n - 1); equal(v, d, "chain restore");});
        Net rnd{n / 4 + 2, 0, n / 4 + 1, {}};
        for (int i = 0; i < 4 * rnd.n; ++i) { rnd.es.push_back({int(rng() % rnd.n), int(rng() % rnd.n), rand(0, 1000000), rng() % 4 ? 0 : rand(0, 1000)}); }
        context = "random n=" + std::to_string(rnd.n);
        largeEngines(rnd, -1, [](MaxFlow<lng> &, lll) {});
        int k = n / 8; Net bip{2 * k + 2, 2 * k, 2 * k + 1, {}};
        for (int i = 0; i < k; ++i) { bip.es.push_back({2 * k, i, 1, 0}); bip.es.push_back({k + i, 2 * k + 1, 1, 0}); }
        for (int i = 0; i < 3 * k; ++i) { bip.es.push_back({int(rng() % k), k + int(rng() % k), 1, 0}); }
        context = "unit bipartite k=" + std::to_string(k);
        largeEngines(bip, -1, [](MaxFlow<lng> &, lll) {});
        int d = n / 4; Net dead{d + 3, 0, d + 2, {{0, d + 1, 1000000000000LL, 0}, {d + 1, d + 2, 1, 0}}};
        for (int i = 1; i <= d; ++i) { dead.es.push_back({0, i, 1000000, 0}); dead.es.push_back({i, i % d + 1, 999999, 0}); }
        context = "dead-end excess return n=" + std::to_string(dead.n);
        largeEngines(dead, 1, [](MaxFlow<lng> &, lll) {});
        cout << "PASS large certified networks n=" << n << " (chain, random, unit bipartite, push-relabel excess return)\n";}

    void invalid(const string &name) {
        MaxFlow<lng> mf(3); mf.addEdge(0, 1, 5);
        vector<vector<int>> adj(3); vector<vector<lng>> cap(3, vector<lng>(3));
        if (name == "negative-size") { MaxFlow<lng> bad(-1); }
        else if (name == "addEdge-range") { mf.addEdge(0, 3, 1); }
        else if (name == "addEdge-negative") { mf.addEdge(0, 1, -1); }
        else if (name == "addEdge-overflow") { mf.addEdge(0, 1, std::numeric_limits<lng>::max(), 1); }
        else if (name == "flow-same") { mf.flow(1, 1); }
        else if (name == "flow-range") { mf.flow(0, 3); }
        else if (name == "flow-limit") { mf.flow(0, 1, -1); }
        else if (name == "scaling-same") { mf.capacityScalingDinic(2, 2); }
        else if (name == "pushrelabel-same") { mf.pushRelabel(0, 0); }
        else if (name == "pushrelabel-supply") { mf.addEdge(0, 2, std::numeric_limits<lng>::max()); mf.pushRelabel(0, 2); }
        else if (name == "getEdge-range") { mf.getEdge(1); }
        else if (name == "changeEdge-flow") { mf.changeEdge(0, 2, 3); }
        else if (name == "changeEdge-reverse") { mf.changeEdge(0, 2, -1); }
        else if (name == "changeCapacity-negative") { mf.changeCapacity(0, -1, 0, 1); }
        else if (name == "changeCapacity-same") { mf.changeCapacity(0, 1, 2, 2); }
        else if (name == "minCut-range") { mf.minCut(3); }
        else if (name == "decomposition-same") { mf.pathDecomposition(1, 1); }
        else if (name == "graph-negative") { Graph g(2, true); g.addEdge(0, 1, -1); MaxFlow<lng> bad(g); }
        else if (name == "legacy-size") { EdmondsKarp(3, adj, cap); }
        else if (name == "legacy-negative") { adj[0].push_back(1); cap[0][1] = -1; EdmondsKarp(2, adj, cap); }
        else if (name == "legacy-pair-overflow") {
            adj[0].push_back(1); adj[1].push_back(0); cap[0][1] = cap[1][0] = std::numeric_limits<lng>::max(); EdmondsKarp(2, adj, cap);}
        else if (name == "legacy-range") { EdmondsKarp(2, adj, cap).getMaxFlow(0, 3); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        smallCases(); wideCases(); mediumCases(); largeCases();
        cout << "PASS max flow seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL max flow seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
