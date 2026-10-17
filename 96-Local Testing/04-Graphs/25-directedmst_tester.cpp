#include "../../04-Graphs/25-directedmst.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " arcs=";
        for (auto e : g.edges) { out << e.u << "->" << e.v << ':' << e.w << ' '; }
        return out.str();}
    string str(lll x) {
        if (x == 0) { return "0"; }
        string s; bool neg = x < 0; ulll y = neg ? ulll(0) - ulll(x) : ulll(x);
        while (y) { s += char('0' + int(y % 10)); y /= 10; }
        if (neg) { s += '-'; }
        return string(s.rbegin(), s.rend());}
    // Exhaustive: every in-arc choice per non-root vertex (or none for branchings), acyclic by parent walks.
    struct Brute { bool ok = false; lll best = 0; };
    Brute bruteArcs(int n, const vector<DirectedMst::Arc> &arcs, int root, bool branching) {
        vector<vector<int>> in(n);
        for (int i = 0; i < int(arcs.size()); ++i) {
            if (arcs[i].from != arcs[i].to) { in[arcs[i].to].push_back(i); }}
        vector<int> pick(n, -1); Brute res;
        auto rec = [&](auto &&self, int v) -> void {
            if (v == n) {
                for (int s = 0; s < n; ++s) {
                    int x = s;
                    for (int step = 0; step <= n && pick[x] != -1; ++step) { x = arcs[pick[x]].from; }
                    if (pick[x] != -1 || (!branching && x != root)) { return; }}
                lll w = 0;
                for (int x = 0; x < n; ++x) { if (pick[x] != -1) { w += arcs[pick[x]].w; }}
                if (!res.ok || (branching ? w > res.best : w < res.best)) { res.ok = true; res.best = w; }
                return;}
            if (v == root && !branching) { pick[v] = -1; self(self, v + 1); return; }
            if (branching) { pick[v] = -1; self(self, v + 1); }
            for (int i : in[v]) { pick[v] = i; self(self, v + 1); }
            pick[v] = -1;};
        rec(rec, 0);
        return res;}
    Brute bruteArborescence(const Graph &g, int root, bool branching) {
        vector<DirectedMst::Arc> arcs;
        for (int i = 0; i < int(g.edges.size()); ++i) { arcs.push_back({g.edges[i].u, g.edges[i].v, i, lll(g.edges[i].w)}); }
        return bruteArcs(g.n, arcs, root, branching);}
    // Independent O(V * E) Chu-Liu/Edmonds with explicit contraction; returns false when some vertex is unreachable.
    bool chuLiu(const Graph &g, int root, lll &out) {
        struct E { int u, v; lll w; };
        vector<E> es;
        for (auto e : g.edges) { es.push_back({e.u, e.v, lll(e.w)}); }
        int n = g.n; out = 0;
        while (true) {
            vector<lll> in(n, 0); vector<int> pre(n, -1);
            for (auto &e : es) {
                if (e.u != e.v && (pre[e.v] == -1 || e.w < in[e.v])) { in[e.v] = e.w; pre[e.v] = e.u; }}
            for (int v = 0; v < n; ++v) { if (v != root && pre[v] == -1) { return false; }}
            int cnt = 0; vector<int> id(n, -1), vis(n, -1); in[root] = 0;
            for (int v = 0; v < n; ++v) {
                out += in[v]; int x = v;
                while (vis[x] != v && id[x] == -1 && x != root) { vis[x] = v; x = pre[x]; }
                if (x != root && id[x] == -1) {
                    for (int y = pre[x]; y != x; y = pre[y]) { id[y] = cnt; }
                    id[x] = cnt++;}}
            if (!cnt) { return true; }
            for (int v = 0; v < n; ++v) { if (id[v] == -1) { id[v] = cnt++; }}
            for (auto &e : es) {
                int v = e.v; e.u = id[e.u]; e.v = id[e.v];
                if (e.u != e.v) { e.w -= in[v]; }}
            n = cnt; root = id[root];}}
    // Witness: parent arcs enter their vertex, walks reach the root (or a root for branchings), weight is the arc sum.
    void validate(const Graph &g, const DirectedMstResult &r, int root, bool branching, const string &name) {
        int n = g.n;
        check(int(r.parentEdge.size()) == n, name + " parentEdge size");
        lll w = 0;
        for (int v = 0; v < n; ++v) {
            int e = r.parentEdge[v];
            if (v == root && !branching) { check(e == -1, name + " root has no parent"); continue; }
            if (!branching) { check(e != -1, name + " non-root has a parent"); }
            if (e == -1) { continue; }
            check(0 <= e && e < int(g.edges.size()) && g.edges[e].v == v && g.edges[e].u != v, name + " parent arc enters vertex");
            if (branching) { check(g.edges[e].w > 0, name + " branching uses positive arcs only"); }
            w += g.edges[e].w;}
        for (int s = 0; s < n; ++s) {
            int x = s, step = 0;
            for (; step <= n && r.parentEdge[x] != -1; ++step) { x = g.edges[r.parentEdge[x]].u; }
            check(step <= n && (branching || x == root), name + " parent walk is acyclic");}
        check(w == r.weight, name + " weight is the parent arc sum");}
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        DirectedMst d(g); CsrGraph c(g); DirectedMst dc(c);
        for (int root = 0; root < g.n; ++root) {
            auto r = d.arborescence(root); auto b = bruteArborescence(g, root, false);
            context = show(g) + " root=" + std::to_string(root);
            check(r.ok == b.ok, "arborescence ok equals existence");
            if (r.ok) {
                check(r.weight == b.best, "arborescence weight " + str(r.weight) + " brute " + str(b.best));
                validate(g, r, root, false, "arborescence");}
            else { check(r.weight == 0 && r.parentEdge.empty(), "failure leaves weight 0 and empty parentEdge"); }
            auto rc = dc.arborescence(root);
            check(rc.ok == r.ok && rc.weight == r.weight && rc.parentEdge == r.parentEdge, "CSR build agrees");
            check(d.arborescence(root).parentEdge == r.parentEdge, "repeated query is stable");}
        context = show(g);
        auto mb = maximumBranching(g); auto bb = bruteArborescence(g, -1, true);
        check(mb.ok && mb.weight == bb.best, "maximumBranching weight " + str(mb.weight) + " brute " + str(bb.best));
        validate(g, mb, -1, true, "maximumBranching");}
    lng weightOf(std::mt19937_64 &rng, int kind) {
        if (kind == 0) { return lng(rng() % 7) - 3; }
        if (kind == 1) { return lng(rng() % 1000); }
        lng big[] = {LLONG_MIN, LLONG_MIN + 1, -1, 0, 1, LLONG_MAX - 1, LLONG_MAX};
        return rng() % 3 ? big[rng() % 7] : lng(rng());}
    void exhaustive() {
        int bound = mode == "quick" ? 3 : 4;
        std::mt19937_64 rng(seed ^ 0x5a);
        for (int n = 0; n <= bound; ++n) {
            int arcs = n * (n - 1);
            for (int mask = 0; mask < (1 << arcs); ++mask) {
                Graph g(n, true); int i = 0;
                for (int u = 0; u < n; ++u) {
                    for (int v = 0; v < n; ++v) {
                        if (u != v && (mask >> i++ & 1)) { g.addEdge(u, v, weightOf(rng, mask % 3)); }}}
                runCase(g);}}
        cout << "PASS every simple digraph n<=" << bound << " (three weight domains) cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 400 : mode == "full" ? 5000 : 30000;
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 6), m = int(rng() % (2 * n + 4));
            Graph g(n, true);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = rng() % 8 == 0 ? u : int(rng() % n);
                if (t % 2 && i > 0 && rng() % 3 == 0) { u = g.edges.back().u; v = g.edges.back().v; }
                g.addEdge(u, v, weightOf(rng, t % 3));}
            runCase(g);}
        cout << "PASS random multigraphs with loops and parallel arcs n<=6 rounds=" << rounds << '\n';}
    // Builder: lll arc lists up to 2^64 - 1 in magnitude with IDs 1000 + i, against exhaustive search over the arcs.
    void builder() {
        std::mt19937_64 rng(seed ^ 0x81);
        int rounds = mode == "quick" ? 500 : mode == "full" ? 5000 : 20000;
        lll lim = (lll(1) << 64) - 1, pool[] = {0, 1, -1, lll(LLONG_MAX), -lll(LLONG_MAX), lll(1) << 63, -(lll(1) << 63), lim, -lim};
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 5), m = int(rng() % (2 * n + 3));
            vector<DirectedMst::Arc> arcs;
            for (int i = 0; i < m; ++i) { arcs.push_back({int(rng() % n), int(rng() % n), 1000 + i, rng() % 3 ? pool[rng() % 9] : lll(lng(rng() % 21) - 10)}); }
            std::ostringstream out; out << "builder n=" << n << " arcs=";
            for (auto &a : arcs) { out << a.from << "->" << a.to << ':' << str(a.w) << ' '; }
            DirectedMst d(n, arcs);
            for (int root = 0; root < n; ++root) {
                context = out.str() + " root=" + std::to_string(root);
                auto r = d.arborescence(root); auto b = bruteArcs(n, arcs, root, false);
                check(r.ok == b.ok && (!r.ok || r.weight == b.best), "builder arborescence " + str(r.weight) + " brute " + str(b.best));
                if (!r.ok) { continue; }
                lll w = 0;
                for (int v = 0; v < n; ++v) {
                    int e = r.parentEdge[v] - 1000;
                    check(v == root ? r.parentEdge[v] == -1 : 0 <= e && e < m && arcs[e].to == v && arcs[e].from != v, "builder parent IDs");
                    if (v != root) { w += arcs[e].w; }}
                check(w == r.weight, "builder weight is the arc sum");}
            ++cases;}
        lll big = lll(1) << 63; context = "review instance";
        auto r = DirectedMst(3, {{0, 2, 0, big}, {2, 1, 1, big}, {1, 2, 2, -big}}).arborescence(0);
        check(r.ok && r.weight == 2 * big && r.parentEdge == vector<int>{-1, 1, 0}, "three-vertex ring instance at 2^63");
        cout << "PASS builder arc lists n<=5 with |w| up to 2^64 - 1 and custom IDs rounds=" << rounds << '\n';}
    void medium() {
        std::mt19937_64 rng(seed ^ 0x77);
        int rounds = mode == "quick" ? 20 : mode == "full" ? 200 : 600;
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 60), m = int(rng() % (6 * n + 1));
            Graph g(n, true);
            for (int i = 0; i < m; ++i) { g.addEdge(int(rng() % n), int(rng() % n), weightOf(rng, t % 3)); }
            context = show(g); DirectedMst d(g);
            for (int root = 0; root < n; ++root) {
                lll w = 0; bool ok = chuLiu(g, root, w); auto r = d.arborescence(root);
                check(r.ok == ok && (!ok || r.weight == w), "medium arborescence versus Chu-Liu/Edmonds root " + std::to_string(root));
                if (ok) { validate(g, r, root, false, "medium arborescence"); }}}
        cout << "PASS medium random multigraphs n<=60 versus O(VE) Chu-Liu/Edmonds, every root rounds=" << rounds << '\n';}
    void large() {
        std::mt19937_64 rng(seed ^ 0x3c);
        int n = mode == "quick" ? 3000 : mode == "full" ? 100000 : 300000;
        Graph g(n, true);
        for (int v = 1; v < n; ++v) { g.addEdge(int(rng() % v), v, lng(rng() % 1000000000)); }
        for (int i = 0; i < 2 * n; ++i) { g.addEdge(int(rng() % n), int(rng() % n), lng(rng() % 1000000000)); }
        context = "large random n=" + std::to_string(n);
        DirectedMst d(g);
        for (int root : {0, n / 2, n - 1}) {
            auto r = d.arborescence(root);
            if (root == 0) { check(r.ok, "large root 0 reaches every vertex"); }
            if (r.ok) { validate(g, r, root, false, "large arborescence"); }}
        auto mb = maximumBranching(g); validate(g, mb, -1, true, "large maximumBranching");
        int s = mode == "quick" ? 600 : 3000;
        Graph h(s, true);
        for (int v = 1; v < s; ++v) { h.addEdge(v - 1, v, lng(rng() % 100)); }
        for (int i = 0; i < 4 * s; ++i) { h.addEdge(int(rng() % s), int(rng() % s), lng(rng() % 200) - 50); }
        context = "chain plus random n=" + std::to_string(s);
        lll w = 0; check(chuLiu(h, 0, w), "chain root reaches all");
        auto r = DirectedMst(h).arborescence(0);
        check(r.ok && r.weight == w, "large arborescence equals Chu-Liu/Edmonds");
        Graph cycle(n, true);
        for (int v = 0; v < n; ++v) { cycle.addEdge(v, (v + 1) % n, 1); }
        auto rc = DirectedMst(cycle).arborescence(n / 3);
        check(rc.ok && rc.weight == n - 1 && rc.parentEdge[n / 3] == -1, "directed cycle: drop the arc into the root");
        cout << "PASS large n=" << n << " roots and branching, chain/random n=" << s << ", cycle of " << n << '\n';}
    void invalid(const string &name) {
        Graph g(2, true); g.addEdge(0, 1);
        if (name == "undirected") { DirectedMst d(Graph(2)); }
        else if (name == "root-range") { DirectedMst(g).arborescence(2); }
        else if (name == "branching-undirected") { maximumBranching(Graph(2)); }
        else if (name == "arc-range") { DirectedMst(2, {{0, 2, 0, 1}}); }
        else if (name == "arc-weight") { DirectedMst(2, {{0, 1, 0, DirectedMst::INF}}); }
        else if (name == "arc-weight-64") { DirectedMst(2, {{0, 1, 0, -(lll(1) << 64)}}); }
        else if (name == "negative-n") { DirectedMst(-1, {}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); builder(); medium(); large();
        cout << "PASS directedmst seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL directedmst seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
