#include "../../04-Graphs/24-dominatortree.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g, int root) {
        std::ostringstream out; out << "n=" << g.n << " root=" << root << " arcs=";
        for (auto e : g.edges) { out << e.u << "->" << e.v << ' '; }
        return out.str();}
    // Dataflow fixpoint Dom(v) = {v} + intersection of Dom(p) over reachable predecessors; independent of both algorithms.
    vector<ulng> dominators(const Graph &g, int root) {
        int n = g.n;
        vector<uint8_t> reach(n);
        vector<int> queue{root}; reach[root] = 1;
        for (int i = 0; i < int(queue.size()); ++i) {
            for (int a : g[queue[i]]) {
                if (!reach[g.arcs[a].to]) { reach[g.arcs[a].to] = 1; queue.push_back(g.arcs[a].to); }}}
        ulng all = n == 64 ? ~0ULL : (1ULL << n) - 1;
        vector<ulng> dom(n, 0);
        for (int v = 0; v < n; ++v) { dom[v] = reach[v] ? (v == root ? 1ULL << root : all) : 0; }
        for (bool changed = true; changed;) {
            changed = false;
            for (int v = 0; v < n; ++v) {
                if (!reach[v] || v == root) { continue; }
                ulng s = all;
                for (auto e : g.edges) {
                    if (e.v == v && reach[e.u]) { s &= dom[e.u]; }}
                s |= 1ULL << v;
                if (s != dom[v]) { dom[v] = s; changed = true; }}}
        return dom;}
    void verify(const Graph &g, const DominatorTree &t, const vector<ulng> &dom, int root, const string &name) {
        int n = g.n;
        check(t.root == root && int(t.idom.size()) == n, name + " shape");
        for (int v = 0; v < n; ++v) {
            if (!dom[v]) {
                check(t.idom[v] == -1 && t.tin[v] == -1 && t.immediateDominator(v) == -1, name + " unreachable vertex");
                for (int u = 0; u < n; ++u) { check(!t.dominates(u, v) && !t.dominates(v, u), name + " unreachable dominates nothing"); }
                continue;}
            int expected = -1;
            for (int d = 0; d < n; ++d) {
                if (d != v && (dom[v] >> d & 1) && __builtin_popcountll(dom[d]) == __builtin_popcountll(dom[v]) - 1) { expected = d; }}
            check(t.idom[v] == expected && t.immediateDominator(v) == expected, name + " idom of " + std::to_string(v));
            for (int u = 0; u < n; ++u) { check(t.dominates(u, v) == bool(dom[v] >> u & 1), name + " dominates(" + std::to_string(u) + "," + std::to_string(v) + ")"); }
            check(t.tout[v] - t.tin[v] == [&] { int c = 0; for (int w = 0; w < n; ++w) { c += int(dom[w] >> v & 1); } return c; }(), name + " subtree size is dominatee count");}
        auto df = t.dominanceFrontier(g);
        for (int v = 0; v < n; ++v) {
            set<int> expected;
            if (dom[v]) {
                for (auto e : g.edges) {
                    int w = e.v;
                    if (dom[e.u] && (dom[e.u] >> v & 1) && !(w != v && (dom[w] >> v & 1))) { expected.insert(w); }}}
            check(set<int>(df[v].begin(), df[v].end()) == expected && df[v].size() == expected.size(), name + " dominance frontier of " + std::to_string(v));}}
    void runCase(const Graph &g, int root) {
        ++cases; context = show(g, root);
        auto dom = dominators(g, root);
        verify(g, dominatorTree(g, root), dom, root, "Lengauer-Tarjan");
        verify(g, dominatorTreeSimple(g, root), dom, root, "vertex deletion");
        CsrGraph c(g);
        verify(g, dominatorTree(c, root), dom, root, "Lengauer-Tarjan CSR");
        check(dominatorTreeSimple(c, root).idom == dominatorTree(g, root).idom, "CSR simple agrees");}
    void exhaustive() {
        int bound = mode == "quick" ? 3 : 4;
        for (int n = 1; n <= bound; ++n) {
            int arcs = n * n;
            for (int mask = 0; mask < (1 << arcs); ++mask) {
                Graph g(n, true);
                for (int i = 0; i < arcs; ++i) {
                    if (mask >> i & 1) { g.addEdge(i / n, i % n); }}
                for (int root = 0; root < n; ++root) { runCase(g, root); }}}
        cout << "PASS every looped digraph n<=" << bound << " with every root cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 4000 : 30000;
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 14), m = int(rng() % (3 * n + 3));
            Graph g(n, true);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = t % 3 == 0 ? int(rng() % n) : min(n - 1, u + 1 + int(rng() % 3));
                if (t % 3 == 1 && rng() % 4 == 0) { v = int(rng() % (u + 1)); }
                g.addEdge(u, v);}
            runCase(g, int(rng() % n));}
        cout << "PASS random multigraphs n<=14 rounds=" << rounds << '\n';}
    void large() {
        std::mt19937_64 rng(seed ^ 0x1a);
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 1000000;
        Graph chain(n, true);
        for (int v = 1; v < n; ++v) { chain.addEdge(v - 1, v); }
        context = "chain n=" + std::to_string(n);
        auto t = dominatorTree(chain, 0);
        for (int v = 1; v < n; ++v) { check(t.idom[v] == v - 1, "chain idom"); }
        check(t.dominates(0, n - 1) && !t.dominates(n - 1, 0), "chain dominates");
        for (int v = 2; v < n; v += 2) { chain.addEdge(v - 2, v); }
        auto ladder = dominatorTree(chain, 0);
        for (int v = 1; v < n; ++v) { check(ladder.idom[v] == (v % 2 ? v - 1 : v - 2), "skip-ladder idom"); }
        chain.addEdge(n - 1, 0);
        auto df = dominatorTree(chain, 0).dominanceFrontier(chain);
        check(df[n - 1].size() == 1 && df[n - 1][0] == 0, "back edge frontier");
        int s = mode == "quick" ? 400 : mode == "full" ? 3000 : 6000;
        Graph g(s, true);
        for (int i = 0; i < 4 * s; ++i) {
            int u = int(rng() % s);
            g.addEdge(u, rng() % 5 ? min(s - 1, u + 1 + int(rng() % 8)) : int(rng() % s));}
        context = "random n=" + std::to_string(s);
        auto a = dominatorTree(g, 0), b = dominatorTreeSimple(g, 0);
        check(a.idom == b.idom, "Lengauer-Tarjan equals vertex deletion on a large graph");
        Graph star(n, true);
        for (int v = 1; v < n; ++v) { star.addEdge(0, v); star.addEdge(v, v - 1 > 0 ? v - 1 : 1); }
        auto st = dominatorTree(star, 0);
        for (int v = 1; v < n; ++v) { check(st.idom[v] == 0, "star with back arcs: root dominates all"); }
        cout << "PASS large chain/ladder/star n=" << n << " and random n=" << s << '\n';}
    void invalid(const string &name) {
        Graph g(2, true); g.addEdge(0, 1);
        if (name == "undirected") { dominatorTree(Graph(2), 0); }
        else if (name == "root-range") { dominatorTree(g, 2); }
        else if (name == "simple-root") { dominatorTreeSimple(g, -1); }
        else if (name == "frontier-size") { dominatorTree(g, 0).dominanceFrontier(Graph(3, true)); }
        else if (name == "query-range") { dominatorTree(g, 0).dominates(0, 2); }
        else if (name == "tree-root") { DominatorTree(0, vector<int>{1, -1}); }
        else if (name == "tree-range") { DominatorTree(0, vector<int>{-1, 5}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); large();
        cout << "PASS dominator seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL dominator seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
