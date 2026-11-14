#include "../../04-Graphs/03-toposort.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    constexpr int UNLIMITED = std::numeric_limits<int>::max();
#ifdef _GLIBCXX_DEBUG
    constexpr bool DEBUG_HEAP = true;
#else
    constexpr bool DEBUG_HEAP = false;
#endif
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const Graph &g) {
        std::ostringstream out; out << "n=" << g.n << " edges=";
        for (auto e : g.edges) { out << '(' << e.u << ',' << e.v << ',' << e.w << ')'; }
        return out.str();}
    // Expected results; count -1 means more valid orders than any limit a test asks for, orders sorted.
    struct Expect {
        bool acyclic = true;
        lng count = 0;
        vector<int> smallest, largest, earliest;
        vector<vector<int>> layers, orders;
    };
    template<class G>
    bool valid(const G &g, const vector<int> &order) {
        if (int(order.size()) != g.n) { return false; }
        vector<int> position(g.n, -1);
        for (int i = 0; i < g.n; ++i) {
            int u = order[i];
            if (u < 0 || u >= g.n || position[u] != -1) { return false; }
            position[u] = i;}
        for (auto e : g.edges) { if (position[e.u] >= position[e.v]) { return false; }}
        return true;}
    template<class G>
    void witness(const G &g, const CycleWitness &c, const string &name) {
        check(!c.arcs.empty(), name + " present cycle certificate");
        check(c.vertices.size() == c.arcs.size() + 1 && c.vertices.front() == c.vertices.back(), name + " closed cycle lengths");
        set<int> vertices, ids;
        for (int i = 0; i < int(c.arcs.size()); ++i) {
            int a = c.arcs[i]; check(0 <= a && a < int(g.arcs.size()), name + " cycle arc domain");
            check(g.arcs[a].from == c.vertices[i] && g.arcs[a].to == c.vertices[i + 1], name + " cycle arc orientation");
            check(vertices.insert(c.vertices[i]).second && ids.insert(g.arcs[a].id).second, name + " simple cycle vertices/edge IDs");}}
    template<class G>
    void result(const G &g, const TopologicalResult &r, const Expect &x, const vector<int> *exact, const string &name) {
        check(r.acyclic == x.acyclic, name + " acyclic against permutation existence");
        check(r.unique == (x.count == 1), name + " unique against valid permutation count");
        if (!x.acyclic) {
            check(r.order.empty(), name + " cyclic order sentinel");
            witness(g, r.cycle, name); return;}
        check(r.cycle.vertices.empty() && r.cycle.arcs.empty(), name + " DAG empty cycle");
        check(valid(g, r.order), name + " permutation with every arc forward");
        if (exact) { check(r.order == *exact, name + " order equals the enumerated extreme"); }}
    template<class G>
    void generations(const G &g, const Expect &x) {
        auto r = topologicalGenerations(g);
        check(r.acyclic == x.acyclic, "topologicalGenerations acyclic");
        if (!x.acyclic) {
            check(r.layers.empty(), "topologicalGenerations cyclic sentinel");
            witness(g, r.cycle, "topologicalGenerations"); return;}
        check(r.cycle.vertices.empty() && r.cycle.arcs.empty(), "topologicalGenerations DAG empty cycle");
        check(r.layers == x.layers, "topologicalGenerations longest-incoming-path layers");}
    template<class G>
    void allSorts(const G &g, const Expect &x, const vector<int> &first) {
        check(allTopologicalSorts(g, 0).empty(), "allTopologicalSorts limit 0 empty");
        if (!x.acyclic) {
            check(allTopologicalSorts(g, 1).empty() && allTopologicalSorts(g, UNLIMITED).empty(), "allTopologicalSorts empty on a cycle");
            return;}
        auto some = [&](int limit) {
            auto r = allTopologicalSorts(g, limit);
            check(r.size() == size_t(x.count < 0 ? limit : min<lng>(x.count, limit)), "allTopologicalSorts min(limit, total) orders");
            check(r.front() == first, "allTopologicalSorts first order is topologicalSort's");
            // Valid and distinct makes each a member of the complete enumeration.
            for (const auto &o : r) { check(valid(g, o), "allTopologicalSorts valid order"); }
            sort(r.begin(), r.end());
            check(std::adjacent_find(r.begin(), r.end()) == r.end(), "allTopologicalSorts distinct orders");
            return r;};
        some(1);
        if (x.count < 0) { some(3); return; }
        check(some(UNLIMITED) == x.orders, "allTopologicalSorts equals enumerated set");
        some(int(x.count));
        if (x.count >= 2) { some(int(x.count - 1)); }}
    template<class G>
    void graphCase(const G &g, const Expect &x) {
        auto fifo = topologicalSort(g);
        result(g, fifo, x, nullptr, "topologicalSort");
        result(g, topologicalSortSmallest(g), x, &x.smallest, "topologicalSortSmallest");
        result(g, topologicalSortLargest(g), x, &x.largest, "topologicalSortLargest");
        result(g, topologicalSortEarliest(g), x, &x.earliest, "topologicalSortEarliest");
        auto by_dfs = topologicalSortDfs(g);
        result(g, by_dfs, x, nullptr, "topologicalSortDfs");
        if (fifo.acyclic && g.n <= 2000) {
            // Recursive DFS, roots in increasing order and arcs in adjacency order.
            vector<int> post; vector<char> seen(g.n);
            auto visit = [&](auto &&self, int u) -> void {
                seen[u] = 1;
                for (int a : g[u]) { if (!seen[g.arcs[a].to]) { self(self, g.arcs[a].to); }}
                post.push_back(u);};
            for (int u = 0; u < g.n; ++u) { if (!seen[u]) { visit(visit, u); }}
            reverse(post.begin(), post.end());
            check(by_dfs.order == post, "topologicalSortDfs reverse recursive finishing order");}
        generations(g, x); allSorts(g, x, fifo.order);}
    // Enumerating vertex permutations does not use indegrees, heaps or DFS states.
    Expect brute(const Graph &g) {
        Expect x; vector<int> permutation(g.n), best; iota(permutation.begin(), permutation.end(), 0);
        do {
            vector<int> position(g.n);
            for (int i = 0; i < g.n; ++i) { position[permutation[i]] = i; }
            bool ok = true;
            for (auto e : g.edges) { if (position[e.u] >= position[e.v]) { ok = false; break; }}
            if (!ok) { continue; }
            x.orders.push_back(permutation);
            if (x.orders.size() == 1 || position < best) { best = position; x.earliest = permutation; }} while (std::next_permutation(permutation.begin(), permutation.end()));
        x.count = lng(x.orders.size()); x.acyclic = x.count > 0;
        if (!x.acyclic) { return x; }
        x.smallest = x.orders.front(); x.largest = x.orders.back();
        // Longest incoming path by n rounds of relaxation over the arcs, independent of any order.
        vector<int> level(g.n);
        for (int round = 0; round < g.n; ++round) { for (auto e : g.edges) { level[e.v] = max(level[e.v], level[e.u] + 1); }}
        x.layers.resize(g.n ? size_t(*std::max_element(level.begin(), level.end()) + 1) : 0);
        for (int u = 0; u < g.n; ++u) { x.layers[level[u]].push_back(u); }
        return x;}
    void runCase(const Graph &g) {
        ++cases; context = show(g);
        auto x = brute(g);
        graphCase(g, x); graphCase(CsrGraph(g), x);}
    void exhaustive() {
        // The brute force itself against a hand-solved fixture: 2 -> 0 with 1 free.
        Graph hand(3, true); hand.addEdge(2, 0); auto x = brute(hand);
        context = "brute force fixture";
        check(x.count == 3 && x.smallest == vector<int>{1, 2, 0} && x.largest == vector<int>{2, 1, 0}
              && x.earliest == vector<int>{2, 0, 1} && x.layers == vector<vector<int>>{{1, 2}, {0}}, "brute force hand-solved oracle");
        for (bool loops : {false, true}) {
            int bound = loops ? (mode == "quick" ? 2 : mode == "full" ? 3 : 4) : (mode == "quick" ? 3 : 4);
            for (int n = 0; n <= bound; ++n) {
                vector<pair<int, int>> possible;
                for (int u = 0; u < n; ++u) { for (int v = 0; v < n; ++v) {
                    if (loops || u != v) { possible.emplace_back(u, v); }}}
                for (int mask = 0; mask < (1 << int(possible.size())); ++mask) {
                    Graph g(n, true);
                    for (int i = 0; i < int(possible.size()); ++i) {
                        if ((mask >> i) & 1) { g.addEdge(possible[i].first, possible[i].second); }}
                    runCase(g);}}}
        cout << "PASS exhaustive directed graphs with permutation oracle cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 100 : mode == "full" ? 1200 : 10000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 7), m = int(rng() % 25); Graph g(n, true);
            vector<int> permutation(n); iota(permutation.begin(), permutation.end(), 0);
            std::shuffle(permutation.begin(), permutation.end(), rng);
            for (int i = 0; i < m; ++i) {
                int u = int(rng() % n), v = int(rng() % n);
                if (trial % 2 == 0) { if (u == v) { continue; } if (u > v) { swap(u, v); }}
                g.addEdge(permutation[u], permutation[v], i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max());}
            runCase(g);
            if (!g.edges.empty()) {
                auto e = g.edges[rng() % g.edges.size()]; g.addEdge(e.u, e.v, 0);
                runCase(g);}}
        cout << "PASS random relabeled DAGs/general digraphs, duplicate arcs, extreme ignored weights rounds=" << rounds << '\n';}
    // A chain through order: the only order, one vertex per layer.
    Expect line(const vector<int> &order) {
        Expect x; x.count = 1; x.smallest = x.largest = x.earliest = order; x.orders = {order};
        for (int u : order) { x.layers.push_back({u}); }
        return x;}
    // Chain 0 -> 1 -> ... with skip arcs i - 2 -> i and the link h - 1 -> h dropped, h = n / 2 >= 2:
    // exactly the identity and the identity with h - 1 and h swapped.
    Expect gapExpect(int n) {
        int h = n / 2; Expect x; x.count = 2;
        vector<int> id(n), swapped; iota(id.begin(), id.end(), 0); swapped = id; swap(swapped[h - 1], swapped[h]);
        x.smallest = x.earliest = id; x.largest = swapped; x.orders = {id, swapped};
        for (int u = 0; u < n; ++u) {
            if (u == h) { x.layers.back().push_back(u); }
            else { x.layers.push_back({u}); }}
        return x;}
    Graph gapGraph(int n, bool linked) {
        Graph g(n, true);
        for (int i = 1; i < n; ++i) {
            if (i != n / 2 || linked) { g.addEdge(i - 1, i); }
            if (i >= 2) { g.addEdge(i - 2, i); }}
        return g;}
    // Even n: odd vertices free, even vertices a chain n - 2 -> n - 4 -> ... -> 0; Smallest and Earliest differ.
    Graph zigzagGraph(int n) {
        Graph g(n, true);
        for (int u = n - 2; u >= 2; u -= 2) { g.addEdge(u, u - 2); }
        return g;}
    Expect zigzagExpect(int n) {
        Expect x; x.count = -1;
        for (int u = 1; u <= n - 3; u += 2) { x.smallest.push_back(u); }
        for (int u = n - 2; u >= 0; u -= 2) { x.smallest.push_back(u); x.earliest.push_back(u); }
        x.smallest.push_back(n - 1);
        for (int u = 1; u < n; u += 2) { x.earliest.push_back(u); }
        for (int u = n - 1; u >= 0; --u) { x.largest.push_back(u); }
        x.layers.push_back({});
        for (int u = 1; u < n; u += 2) { x.layers[0].push_back(u); }
        x.layers[0].push_back(n - 2); sort(x.layers[0].begin(), x.layers[0].end());
        for (int u = n - 4; u >= 0; u -= 2) { x.layers.push_back({u}); }
        return x;}
    void closedForms() {
        context = "closed forms against brute force n=8";
        auto same = [](const Expect &a, const Expect &b, bool counted) {
            return a.acyclic == b.acyclic && a.smallest == b.smallest && a.largest == b.largest && a.earliest == b.earliest
                   && a.layers == b.layers && (!counted || (a.count == b.count && a.orders == b.orders));};
        check(same(brute(gapGraph(8, false)), gapExpect(8), true), "gap closed form");
        check(same(brute(gapGraph(8, true)), line({0, 1, 2, 3, 4, 5, 6, 7}), true), "skip-arc chain closed form");
        check(same(brute(zigzagGraph(8)), zigzagExpect(8), false), "zigzag closed form");
        check(brute(zigzagGraph(8)).count == 1680, "zigzag order count 8!/4!");}
    void boundaries() {
        closedForms();
        int n = mode == "quick" ? 10000 : mode == "full" ? 200000 : 500000;
        Graph g(n, true); vector<int> least(n); iota(least.begin(), least.end(), 0);
        for (int i = 1; i < n; ++i) { g.addEdge(i - 1, i); }
        context = "chain n=" + std::to_string(n);
        auto chain = line(least);
        graphCase(g, chain); graphCase(CsrGraph(g), chain);
        Graph down(n, true); vector<int> greatest(least.rbegin(), least.rend());
        for (int i = 1; i < n; ++i) { down.addEdge(i, i - 1); }
        context = "descending chain n=" + std::to_string(n);
        auto descending = line(greatest);
        graphCase(down, descending); graphCase(CsrGraph(down), descending);
        context = "chain with skip arcs and one missing link n=" + std::to_string(n);
        auto gap = gapGraph(n, false), linked = gapGraph(n, true);
        graphCase(gap, gapExpect(n)); graphCase(CsrGraph(gap), gapExpect(n));
        context = "chain with skip arcs n=" + std::to_string(n);
        graphCase(linked, chain);
        g.addEdge(n - 1, 0); context = "closed chain n=" + std::to_string(n);
        Expect cyclic; cyclic.acyclic = false;
        graphCase(g, cyclic); graphCase(CsrGraph(g), cyclic);
        // Checked libstdc++ validates the entire heap at every pop; bound the all-ready
        // fixtures separately so that their debug cost stays practical.
        int width = mode == "quick" ? 500 : mode == "full" ? 2000 : 5000, zigzag = DEBUG_HEAP ? width : n;
        Graph empty(width, true); least.resize(width);
        context = "edgeless width=" + std::to_string(width);
        Expect flat; flat.count = -1; flat.smallest = flat.earliest = least; flat.layers = {least};
        flat.largest.assign(least.rbegin(), least.rend());
        graphCase(empty, flat);
        context = "zigzag n=" + std::to_string(zigzag);
        Graph zig = zigzagGraph(zigzag);
        graphCase(zig, zigzagExpect(zigzag)); graphCase(CsrGraph(zig), zigzagExpect(zigzag));
        Graph fixture(7, true); fixture.addEdge(5, 0); fixture.addEdge(3, 1); fixture.addEdge(3, 2);
        fixture.addEdge(3, 2); runCase(fixture);
        auto copied = topologicalSortSmallest(fixture); auto moved = std::move(copied);
        check(moved.order == topologicalSortSmallest(fixture).order, "result copy/move/repeated-call");
        fixture.addEdge(2, 3); runCase(fixture);
        cout << "PASS iterative chains/cycle n=" << n << " edgeless width=" << width << " zigzag n=" << zigzag
             << ", heap extremes, generations, limited enumeration\n";}
    void enumerations() {
        context = "antichain n=20 limit=1000";
        Graph wide(20, true); auto r = allTopologicalSorts(wide, 1000);
        vector<int> id(20); iota(id.begin(), id.end(), 0);
        check(r.size() == 1000 && r.front() == id, "allTopologicalSorts exactly limit orders, first FIFO");
        for (const auto &o : r) { check(valid(wide, o), "allTopologicalSorts antichain permutation"); }
        sort(r.begin(), r.end());
        check(std::adjacent_find(r.begin(), r.end()) == r.end(), "allTopologicalSorts antichain distinct");
        // Two disjoint chains of a vertices interleave in binomial(2a, a) ways.
        int a = mode == "quick" ? 6 : mode == "full" ? 8 : 9; lng binomial = 1;
        for (int i = 1; i <= a; ++i) { binomial = binomial * (a + i) / i; }
        Graph two(2 * a, true);
        for (int i = 1; i < a; ++i) { two.addEdge(i - 1, i); two.addEdge(a + i - 1, a + i); }
        context = "two chains a=" + std::to_string(a);
        auto check_all = [&](const auto &g) {
            auto all = allTopologicalSorts(g, UNLIMITED);
            check(lng(all.size()) == binomial && all.front() == topologicalSort(g).order, "allTopologicalSorts binomial count, first FIFO");
            for (const auto &o : all) { check(valid(g, o), "allTopologicalSorts interleaving valid"); }
            sort(all.begin(), all.end());
            check(std::adjacent_find(all.begin(), all.end()) == all.end(), "allTopologicalSorts interleavings distinct");};
        check_all(two); check_all(CsrGraph(two));
        Graph ten(20, true);
        for (int i = 1; i < 10; ++i) { ten.addEdge(i - 1, i); ten.addEdge(9 + i, 10 + i); }
        context = "two chains a=10 limit=1000";
        r = allTopologicalSorts(ten, 1000);
        check(r.size() == 1000, "allTopologicalSorts exactly limit of 184756 orders");
        for (const auto &o : r) { check(valid(ten, o), "allTopologicalSorts limited interleaving valid"); }
        sort(r.begin(), r.end());
        check(std::adjacent_find(r.begin(), r.end()) == r.end(), "allTopologicalSorts limited interleavings distinct");
        // Free vertices beside a transitive tournament with doubled arcs: dense in-lists and parallel arcs.
        // A chain of c vertices (all c(c-1)/2 arcs, some doubled) beside f free vertices has (c+1)...(c+f) orders.
        auto tournament = [&](int c, int f, int limit) {
            Graph dense(c + f, true);
            for (int u = 0; u < c; ++u) {
                for (int v = u + 1; v < c; ++v) { dense.addEdge(v, u); if ((u + v) % 7 == 0) { dense.addEdge(v, u); }}}
            context = "tournament c=" + std::to_string(c) + " f=" + std::to_string(f);
            auto all = allTopologicalSorts(dense, limit);
            lng total = 1;
            for (int i = 1; i <= f; ++i) { total = min<lng>(total * (c + i), INT_MAX); }
            check(lng(all.size()) == min<lng>(limit, total) && all.front() == topologicalSort(dense).order, "allTopologicalSorts dense count, first FIFO");
            for (const auto &o : all) { check(valid(dense, o), "allTopologicalSorts dense valid"); }
            sort(all.begin(), all.end());
            check(std::adjacent_find(all.begin(), all.end()) == all.end(), "allTopologicalSorts dense distinct");};
        tournament(mode == "quick" ? 8 : 30, 3, UNLIMITED);
        if (mode != "quick") { tournament(300, 8, 1000); }
        cout << "PASS allTopologicalSorts antichain limit 1000, two chains a=" << a << " all " << binomial << " orders, dense tournaments\n";}
    void invalid(const string &name) {
        Graph g(2);
        if (name == "kahn-undirected") { topologicalSort(g); }
        else if (name == "lexicographic-undirected") { topologicalSortSmallest(g); }
        else if (name == "largest-undirected") { topologicalSortLargest(g); }
        else if (name == "earliest-undirected") { topologicalSortEarliest(g); }
        else if (name == "dfs-undirected") { topologicalSortDfs(g); }
        else if (name == "generations-undirected") { topologicalGenerations(g); }
        else if (name == "all-undirected") { allTopologicalSorts(g, 1); }
        else if (name == "all-negative-limit") { allTopologicalSorts(Graph(2, true), -1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); boundaries(); enumerations();
        cout << "PASS topological seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL topological seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
