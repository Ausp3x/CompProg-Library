#include "../../04-Graphs/26-dynamic_connectivity.hpp"
#include "../../04-Graphs/09-bridges_articulation.hpp"
#include "../../02-Data Structures/01-dsu.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    // Plain path-compressed union-find for brute-force component labels.
    struct Labels {
        vector<int> p;
        explicit Labels(int n) : p(n) { iota(p.begin(), p.end(), 0); }
        int f(int x) { while (p[x] != x) { x = p[x] = p[p[x]]; } return x; }
        void u(int a, int b) { p[f(a)] = f(b); }
    };
    struct Interval { int l, r, u, v; };
    struct Value { int l, r, u; lng x; };
    struct Event { int t, u, v; bool insert; };
    // Present edges at t: intervals covering t plus inserted-minus-erased event copies at times <= t.
    vector<pair<int, int>> present(const vector<Interval> &iv, const vector<Event> &ev, int t) {
        vector<pair<int, int>> res;
        for (auto &e : iv) { if (e.l <= t && t < e.r) { res.push_back({e.u, e.v}); }}
        map<pair<int, int>, int> cnt;
        for (auto &e : ev) { if (e.t <= t) { cnt[{min(e.u, e.v), max(e.u, e.v)}] += e.insert ? 1 : -1; }}
        for (auto &[k, c] : cnt) { for (int i = 0; i < c; ++i) { res.push_back(k); }}
        return res;}
    bool bipartiteBrute(int n, const vector<pair<int, int>> &es) {
        vector<vector<int>> adj(n);
        for (auto [u, v] : es) { adj[u].push_back(v); adj[v].push_back(u); }
        vector<int> col(n, -1);
        for (int s = 0; s < n; ++s) {
            if (col[s] != -1) { continue; }
            col[s] = 0; vector<int> q{s};
            for (int i = 0; i < int(q.size()); ++i) {
                for (int y : adj[q[i]]) {
                    if (col[y] == -1) { col[y] = col[q[i]] ^ 1; q.push_back(y); }
                    else if (col[y] == col[q[i]]) { return false; }}}}
        return true;}
    template<class M>
    void offlineRound(std::mt19937_64 &rng, int n, int T, M mon) {
        using S = typename M::S;
        OfflineDynamicConnectivity<M> odc(n, T, mon);
        vector<Interval> iv; vector<Event> ev; vector<Value> vals;
        int k = int(rng() % 8);
        for (int i = 0; i < k; ++i) {
            int l = int(rng() % (T + 1)), r = l + int(rng() % (T + 1 - l)), u = int(rng() % n), v = rng() % 6 ? int(rng() % n) : u;
            iv.push_back({l, r, u, v}); odc.addEdge(l, r, u, v);}
        map<pair<int, int>, int> open;
        for (int t = 0; t <= T; ++t) {
            for (int j = int(rng() % 3); j > 0; --j) {
                if (!open.empty() && rng() % 2) {
                    auto it = std::next(open.begin(), int(rng() % open.size())); auto [a, b] = it->first;
                    if (rng() % 2) { swap(a, b); }
                    ev.push_back({t, a, b, false}); odc.eraseEdge(t, a, b);
                    if (--it->second == 0) { open.erase(it); }}
                else if (t < T) {
                    int u = int(rng() % n), v = rng() % 6 ? int(rng() % n) : u;
                    ev.push_back({t, u, v, true}); odc.insertEdge(t, u, v); ++open[{min(u, v), max(u, v)}];}}}
        for (int i = int(rng() % 6); i > 0; --i) {
            int l = int(rng() % (T + 1)), r = l + int(rng() % (T + 1 - l)), u = int(rng() % n); lng x = lng(rng() % 2001) - 1000;
            vals.push_back({l, r, u, x}); odc.addValue(l, r, u, S(x));}
        vector<int> asked;
        for (int t = 0; t < T; ++t) { if (rng() % 3) { asked.push_back(t); odc.query(t); }}
        vector<int> got;
        odc.run([&](int t, const RollbackDSUAggregate<M> &dsu) {
            got.push_back(t);
            auto es = present(iv, ev, t); Labels lab(n);
            for (auto [u, v] : es) { lab.u(u, v); }
            int comps = 0;
            for (int u = 0; u < n; ++u) { comps += lab.f(u) == u; }
            check(dsu.count() == comps, "run count at t=" + std::to_string(t));
            for (int u = 0; u < n; ++u) {
                S sum = mon.e(); int size = 0, edges = 0;
                for (int v = 0; v < n; ++v) {
                    check(dsu.isSameSet(u, v) == (lab.f(u) == lab.f(v)), "run isSameSet");
                    if (lab.f(v) == lab.f(u)) { ++size; }}
                for (auto &x : vals) { if (x.l <= t && t < x.r && lab.f(x.u) == lab.f(u)) { sum = mon.op(sum, S(x.x)); }}
                for (auto [a, b] : es) { edges += lab.f(a) == lab.f(u); }
                check(dsu.getSize(u) == size && dsu.componentValue(u) == sum && dsu.edgeCount(u) == edges, "run size, componentValue, edgeCount");}});
        check(got == asked, "run answers every queried time once, increasing");
        got.clear();
        odc.bipartiteness([&](int t, bool bip) {
            got.push_back(t); check(bip == bipartiteBrute(n, present(iv, ev, t)), "bipartiteness at t=" + std::to_string(t));});
        check(got == asked, "bipartiteness answers every queried time once, increasing");
        ++cases;}
    void offlineCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 4000 : 25000;
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 6), T = 1 + int(rng() % 10);
            context = "offline round=" + std::to_string(r) + " n=" + std::to_string(n) + " T=" + std::to_string(T);
            if (r % 3 == 0) { offlineRound(rng, n, T, Sum<lng>()); }
            else if (r % 3 == 1) { offlineRound(rng, n, T, Min<lng>()); }
            else { offlineRound(rng, n, T, Sum<int>()); }}
        OfflineDynamicConnectivity<> plain(3, 2); plain.addEdge(0, 1, 0, 2); plain.query(0); plain.query(1);
        vector<int> counts;
        plain.run([&](int, const auto &dsu) { counts.push_back(dsu.count()); });
        check(counts == vector<int>{2, 3}, "default monoid connectivity");
        OfflineDynamicConnectivity<> none(0, 3), timeless(3, 0); none.query(1); int calls = 0;
        none.run([&](int, const auto &dsu) { calls += dsu.count() == 0; });
        none.bipartiteness([&](int, bool bip) { calls += bip; });
        timeless.run([&](int, const auto &) { calls += 10; });
        check(calls == 2, "n = 0 and T = 0");
        IncrementalBridgeConnectivity empty(0);
        check(empty.bridgeCount() == 0 && rangeEdgeComponentCount(Graph(0), {{0, 0}}) == vector<int>{0} && rangeEdgeComponentCount(Graph(3), {{0, 0}}) == vector<int>{3}, "empty incremental and range inputs");
        cout << "PASS offline connectivity, component values (Sum, Min), edge counts, bipartiteness n<=6 T<=10 rounds=" << rounds << '\n';}
    void offlineLarge() {
        std::mt19937_64 rng(seed ^ 0x9e);
        int n = mode == "quick" ? 2000 : mode == "full" ? 50000 : 200000, T = 2 * n;
        context = "offline large n=" + std::to_string(n);
        vector<pair<int, int>> es(T);
        for (auto &[u, v] : es) { u = int(rng() % n); v = int(rng() % n); }
        // Insert-only: edge t appears at time t; prefix DSU counts are the oracle. Erase-only mirrors it.
        OfflineDynamicConnectivity<Sum<lng>> up(n, T), down(n, T);
        for (int t = 0; t < T; ++t) {
            up.insertEdge(t, es[t].first, es[t].second); down.addEdge(0, T - t, es[t].first, es[t].second);
            up.query(t); down.query(t);}
        for (int u = 0; u < n; ++u) { up.addValue(0, T, u, u); }
        vector<int> cu(T), cd(T); vector<lng> vu(T);
        up.run([&](int t, const auto &dsu) { cu[t] = dsu.count(); vu[t] = dsu.componentValue(es[t].first); });
        down.run([&](int t, const auto &dsu) { cd[t] = dsu.count(); });
        DSU d(n); vector<lng> sum(n); iota(sum.begin(), sum.end(), 0);
        for (int t = 0; t < T; ++t) {
            int a = d.findSet(es[t].first), b = d.findSet(es[t].second);
            if (a != b) { d.uniteSets(a, b); sum[d.findSet(a)] = sum[a] + sum[b]; }
            check(cu[t] == d.count() && vu[t] == sum[d.findSet(es[t].first)], "insert-only prefix count and value");
            check(cd[T - 1 - t] == d.count(), "erase-only mirror count");}
        cout << "PASS offline large insert-only and erase-only n=" << n << " T=" << T << '\n';}
    struct BridgeBrute { int bridges = 0; vector<int> comp, two; };
    BridgeBrute bridgeBrute(int n, const vector<pair<int, int>> &es) {
        BridgeBrute res; int m = int(es.size()); vector<char> bridge(m);
        auto labels = [&](int skip, bool nonBridge) {
            Labels lab(n);
            for (int i = 0; i < m; ++i) { if (i != skip && !(nonBridge && bridge[i])) { lab.u(es[i].first, es[i].second); }}
            vector<int> res2(n);
            for (int u = 0; u < n; ++u) { res2[u] = lab.f(u); }
            return res2;};
        for (int i = 0; i < m; ++i) {
            auto l = labels(i, false);
            bridge[i] = l[es[i].first] != l[es[i].second]; res.bridges += bridge[i];}
        res.comp = labels(-1, false); res.two = labels(-1, true);
        return res;}
    void incrementalCases() {
        std::mt19937_64 rng(seed ^ 0x51);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 20000;
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 8), m = int(rng() % 16);
            IncrementalBridgeConnectivity ib(n); vector<pair<int, int>> es;
            context = "incremental round=" + std::to_string(r) + " n=" + std::to_string(n);
            check(ib.bridgeCount() == 0, "empty graph has no bridges");
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = rng() % 7 ? int(rng() % n) : u;
                if (i && rng() % 4 == 0) { std::tie(u, v) = es[rng() % es.size()]; swap(u, v); }
                es.push_back({u, v}); ib.addEdge(u, v);
                auto b = bridgeBrute(n, es);
                check(ib.bridgeCount() == b.bridges, "bridgeCount after " + std::to_string(es.size()) + " edges");
                for (int x = 0; x < n; ++x) {
                    check(b.two[ib.find(x)] == b.two[x], "find stays in its 2-edge-connected component");
                    for (int y = 0; y < n; ++y) {
                        check(ib.twoEdgeConnected(x, y) == (b.two[x] == b.two[y]), "twoEdgeConnected");
                        check(ib.connected(x, y) == (b.comp[x] == b.comp[y]), "connected");
                        check((ib.root(x) == ib.root(y)) == (b.comp[x] == b.comp[y]), "root");}}}
            ++cases;}
        std::mt19937_64 big(seed ^ 0x52);
        int n = mode == "quick" ? 3000 : mode == "full" ? 100000 : 300000;
        IncrementalBridgeConnectivity ib(n); Graph g(n); DSU d(n);
        context = "incremental large n=" + std::to_string(n);
        for (int phase = 0; phase < 4; ++phase) {
            for (int i = 0; i < n / 2; ++i) {
                int u = int(big() % n), v = phase < 2 ? (u + 1 + int(big() % 3)) % n : int(big() % n);
                g.addEdge(u, v); ib.addEdge(u, v); d.uniteSets(u, v);}
            auto low = lowlink(g);
            check(ib.bridgeCount() == int(low.bridges.size()), "large bridgeCount versus Tarjan lowlink phase " + std::to_string(phase));
            for (int t = 0; t < 2000; ++t) {
                int x = int(big() % n), y = big() % 2 ? int(big() % n) : (x + 1) % n;
                check(ib.twoEdgeConnected(x, y) == (low.component[x] == low.component[y]), "large twoEdgeConnected versus lowlink");
                check(ib.connected(x, y) == d.isSameSet(x, y), "large connected versus DSU");}}
        cout << "PASS incremental bridges n<=8 rounds=" << rounds << " (all pairs after each edge), large n=" << n << " versus lowlink\n";}
    void rangeCheck(const Graph &g, const vector<pair<int, int>> &qs) {
        auto res = rangeEdgeComponentCount(g, qs);
        check(res.size() == qs.size() && rangeEdgeComponentCount(CsrGraph(g), qs) == res, "rangeEdgeComponentCount size, CSR agrees");
        for (int j = 0; j < int(qs.size()); ++j) {
            Labels lab(g.n); int comps = g.n;
            for (int i = qs[j].first; i < qs[j].second; ++i) {
                int a = lab.f(g.edges[i].u), b = lab.f(g.edges[i].v);
                if (a != b) { lab.p[a] = b; --comps; }}
            check(res[j] == comps, "rangeEdgeComponentCount [" + std::to_string(qs[j].first) + ", " + std::to_string(qs[j].second) + ")");}}
    void rangeCases() {
        std::mt19937_64 rng(seed ^ 0x33);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 4000 : 25000;
        for (int r = 0; r < rounds; ++r) {
            int n = 1 + int(rng() % 7), m = int(rng() % 15);
            Graph g(n);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = rng() % 6 ? int(rng() % n) : u;
                if (i && rng() % 4 == 0) { u = g.edges[rng() % i].v; v = g.edges[rng() % i].u; }
                g.addEdge(u, v);}
            vector<pair<int, int>> qs;
            for (int l = 0; l <= m; ++l) { for (int rr = l; rr <= m; ++rr) { qs.push_back({l, rr}); }}
            std::shuffle(qs.begin(), qs.end(), rng);
            std::ostringstream out; out << "range n=" << n << " edges=";
            for (auto e : g.edges) { out << e.u << '-' << e.v << ' '; }
            context = out.str(); rangeCheck(g, qs); ++cases;}
        int s = mode == "quick" ? 60 : 250;
        for (int shape = 0; shape < 3; ++shape) {
            int n = shape == 0 ? s / 4 : s; Graph g(n);
            for (int i = 0; i < s; ++i) {
                if (shape == 2) { g.addEdge(i % n, (i + 1) % n); }
                else { g.addEdge(int(rng() % n), int(rng() % n)); }}
            vector<pair<int, int>> qs;
            for (int l = 0; l <= s; ++l) { for (int r = l; r <= s; ++r) { qs.push_back({l, r}); }}
            context = "range medium shape=" + std::to_string(shape) + " m=" + std::to_string(s); rangeCheck(g, qs);}
        int n = mode == "quick" ? 3000 : mode == "full" ? 100000 : 200000, m = 2 * n;
        for (int shape = 0; shape < 3; ++shape) {
            Graph g(n);
            for (int i = 0; i < m; ++i) {
                if (shape == 0) { g.addEdge(int(rng() % n), int(rng() % n)); }
                else if (shape == 1) { g.addEdge(i % (n - 1), i % (n - 1) + 1); }
                else { g.addEdge(0, i % n); }}
            vector<pair<int, int>> qs{{0, m}, {0, 0}, {m, m}, {0, 1}, {m - 1, m}};
            for (int t = 0; t < 120; ++t) { int l = int(rng() % (m + 1)); qs.push_back({l, l + int(rng() % (m + 1 - l))}); }
            context = "range large shape=" + std::to_string(shape) + " n=" + std::to_string(n); rangeCheck(g, qs);}
        cout << "PASS rangeEdgeComponentCount every [l, r) on random multigraphs with loops rounds=" << rounds
             << ", medium all ranges m=" << s << ", large n=" << n << " sampled\n";}
    void invalid(const string &name) {
        OfflineDynamicConnectivity<> odc(3, 4);
        if (name == "odc-vertex") { odc.addEdge(0, 1, 0, 3); }
        else if (name == "odc-time") { odc.addEdge(0, 5, 0, 1); }
        else if (name == "odc-erase") { odc.eraseEdge(1, 0, 1); }
        else if (name == "odc-query") { odc.query(4); }
        else if (name == "odc-value") { odc.addValue(0, 1, -1, 1); }
        else if (name == "ibc-vertex") { IncrementalBridgeConnectivity(2).addEdge(0, 2); }
        else if (name == "range-query") { Graph g(2); g.addEdge(0, 1); rangeEdgeComponentCount(g, {{1, 0}}); }
        else if (name == "range-directed") { rangeEdgeComponentCount(Graph(2, true), {}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        offlineCases(); offlineLarge(); incrementalCases(); rangeCases();
        cout << "PASS dynamic_connectivity seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL dynamic_connectivity seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
