#include "../../04-Graphs/17-treeisomorphism.hpp"
#include "../../01-Core/05-modint.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    std::mt19937_64 rng;
#if defined(_GLIBCXX_DEBUG) || defined(__SANITIZE_ADDRESS__)
    constexpr bool CHECKED = true;
#else
    constexpr bool CHECKED = false;
#endif
#ifdef __SANITIZE_ADDRESS__
    constexpr int LARGE = 200000;
#else
    constexpr int LARGE = 50000;
#endif
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    int rnd(int n) { return int(rng() % ulng(n)); }

    Graph build(int n, const vector<pair<int, int>> &es) {
        Graph g(n);
        for (auto [u, v] : es) { g.addEdge(u, v); }
        return g;}
    vector<pair<int, int>> prufer(int n, const vector<int> &seq) {
        vector<int> deg(n, 1);
        for (int x : seq) { ++deg[x]; }
        vector<pair<int, int>> es;
        for (int x : seq) {
            int leaf = 0;
            while (deg[leaf] != 1) { ++leaf; }
            es.push_back({leaf, x}); --deg[leaf]; --deg[x];}
        int a = -1;
        for (int v = 0; v < n; ++v) {
            if (deg[v] == 1) { if (a == -1) { a = v; } else { es.push_back({a, v}); }}}
        return es;}
    // Random tree in one of six shapes, relabelled, edges shuffled.
    vector<pair<int, int>> shaped(int n, int shape) {
        vector<int> label(n);
        iota(label.begin(), label.end(), 0);
        std::shuffle(label.begin(), label.end(), rng);
        vector<pair<int, int>> es;
        for (int i = 1; i < n; ++i) {
            int p = shape == 0 ? rnd(i) : shape == 1 ? i - 1 : shape == 2 ? 0 : shape == 3 ? (i % 2 ? i - 1 : max(0, i - 2))
                : shape == 4 ? (i - 1) / 2 : rnd(max(1, min(i, 3)));
            es.push_back(rng() & 1 ? pair(label[p], label[i]) : pair(label[i], label[p]));}
        std::shuffle(es.begin(), es.end(), rng);
        return es;}
    vector<pair<int, int>> relabel(int n, const vector<pair<int, int>> &es, vector<int> &perm) {
        perm.resize(n);
        iota(perm.begin(), perm.end(), 0);
        std::shuffle(perm.begin(), perm.end(), rng);
        vector<pair<int, int>> res;
        for (auto [u, v] : es) { res.push_back({perm[u], perm[v]}); }
        std::shuffle(res.begin(), res.end(), rng);
        return res;}
    string show(int n, const vector<pair<int, int>> &es) {
        std::ostringstream out; out << "n=" << n << " edges=";
        for (auto [u, v] : es) { out << u << '-' << v << ' '; }
        return out.str();}

    // Oracle: naive AHU strings, every child string sorted, recomputed per root.
    struct Naive {
        int n;
        vector<vector<int>> adj;
        Naive(int n, const vector<pair<int, int>> &es) : n(n), adj(n) {
            for (auto [u, v] : es) { adj[u].push_back(v); adj[v].push_back(u); }}
        // sub[v]: string of v's subtree with the tree rooted at r.
        vector<string> subtrees(int r) const {
            vector<string> sub(n);
            vector<int> order{r}, par(n, -1);
            for (int i = 0; i < int(order.size()); ++i) {
                for (int v : adj[order[i]]) {
                    if (v != par[order[i]]) { par[v] = order[i]; order.push_back(v); }}}
            for (int i = n - 1; i >= 0; --i) {
                int u = order[i];
                vector<string> kids;
                for (int v : adj[u]) {
                    if (v != par[u]) { kids.push_back(sub[v]); }}
                sort(kids.begin(), kids.end());
                sub[u] = "(";
                for (auto &s : kids) { sub[u] += s; }
                sub[u] += ")";}
            return sub;}
        string rooted(int r) const { return subtrees(r)[r]; }
        string unrooted() const {
            string best;
            for (int r = 0; r < n; ++r) {
                string s = rooted(r);
                if (r == 0 || s < best) { best = s; }}
            return best;}
    };

    template<class T>
    bool samePartition(const vector<T> &ids, const vector<string> &keys) {
        map<T, string> a; map<string, T> b;
        for (int i = 0; i < int(ids.size()); ++i) {
            if (a.emplace(ids[i], keys[i]).fi->se != keys[i] || b.emplace(keys[i], ids[i]).fi->se != ids[i]) { return false; }}
        return true;}

    // Mapping validity: bijection onto h, edges onto edges, optional fixed root.
    bool validMap(int n, const vector<pair<int, int>> &g, const vector<pair<int, int>> &h, const vector<int> &f, int r, int s) {
        if (int(f.size()) != n) { return false; }
        vector<char> used(n);
        for (int x : f) {
            if (x < 0 || x >= n || used[x]) { return false; }
            used[x] = 1;}
        set<pair<int, int>> eh;
        for (auto [u, v] : h) { eh.insert({min(u, v), max(u, v)}); }
        for (auto [u, v] : g) {
            if (!eh.count({min(f[u], f[v]), max(f[u], f[v])})) { return false; }}
        return r == -1 || f[r] == s;}

    lng bruteAut(int n, const vector<pair<int, int>> &es, int root) {
        set<pair<int, int>> e;
        for (auto [u, v] : es) { e.insert({min(u, v), max(u, v)}); }
        vector<int> p(n);
        iota(p.begin(), p.end(), 0);
        lng res = 0;
        for (bool more = true; more; more = std::next_permutation(p.begin(), p.end())) {
            bool ok = root == -1 || p[root] == root;
            for (auto [u, v] : es) { ok = ok && e.count({min(p[u], p[v]), max(p[u], p[v])}); }
            res += ok;}
        return res;}

    // Classification ids: contiguous, equal iff naive strings equal, ordered by (height, sorted child ids).
    void verifyClasses(const Graph &g, const Naive &nv, int r) {
        auto id = rootedTreeClassification(g, r);
        auto sub = nv.subtrees(r);
        check(samePartition(id, sub), "rootedTreeClassification partition");
        int k = *std::max_element(id.begin(), id.end()) + 1;
        vector<char> seen(k);
        for (int x : id) { seen[x] = 1; }
        check(std::ranges::all_of(seen, [](char c) { return c; }), "ids contiguous");
        vector<int> par(g.n, -1), order{r}, height(g.n);
        for (int i = 0; i < int(order.size()); ++i) {
            for (int v : nv.adj[order[i]]) {
                if (v != par[order[i]]) { par[v] = order[i]; order.push_back(v); }}}
        vector<vector<int>> kids(g.n);
        for (int i = g.n - 1; i > 0; --i) { int v = order[i]; height[par[v]] = max(height[par[v]], height[v] + 1); kids[par[v]].push_back(id[v]); }
        vector<pair<int, vector<int>>> key(g.n);
        for (int v = 0; v < g.n; ++v) { sort(kids[v].begin(), kids[v].end()); key[v] = {height[v], kids[v]}; }
        for (int u = 0; u < g.n; ++u) {
            for (int v = 0; v < g.n; ++v) { check((id[u] < id[v]) == (key[u] < key[v]), "canonical id order"); }}
        auto h1 = rootedTreeHash(g, r, seed), h2 = rootedTreeHash(CsrGraph(g), r, seed ^ 0x9e37);
        check(samePartition(h1, sub) && samePartition(h2, sub), "rootedTreeHash partition");}

    void exhaustive() {
        int bound = mode == "quick" ? 6 : mode == "full" || CHECKED ? 7 : 8;
        for (int n = 1; n <= bound; ++n) {
            map<string, string> unrootedA, unrootedB, rootedA, rootedB;
            map<string, lng> autCount;
            map<string, ulng> rootHash;
            vector<int> seq(max(0, n - 2), 0);
            vector<vector<pair<int, int>>> all;
            while (true) {
                auto es = prufer(n, seq);
                Graph g = build(n, es);
                Naive nv(n, es);
                ++cases; context = "exhaustive " + show(n, es);
                all.push_back(es);
                string nu = nv.unrooted(), cu = treeCanonicalString(g);
                check(int(cu.size()) == 2 * n, "canonical length");
                check(unrootedA.emplace(nu, cu).fi->se == cu && unrootedB.emplace(cu, nu).fi->se == nu, "treeCanonicalString unrooted iff isomorphic");
                vector<string> rootedNaive(n);
                for (int r = 0; r < n; ++r) {
                    rootedNaive[r] = nv.rooted(r);
                    string cr = treeCanonicalString(g, r);
                    check(rootedA.emplace(rootedNaive[r], cr).fi->se == cr && rootedB.emplace(cr, rootedNaive[r]).fi->se == rootedNaive[r], "treeCanonicalString rooted iff isomorphic");
                    ulng h = rootedTreeHash(g, r, seed)[r];
                    check(rootHash.emplace(rootedNaive[r], h).fi->se == h, "rootedTreeHash comparable across trees");
                    if (n <= 6 || r == 0) { verifyClasses(g, nv, r); }}
                check(samePartition(allRootsClassification(g), rootedNaive), "allRootsClassification orbits");
                if (n <= 6 || rng() % 16 == 0) {
                    lng a = bruteAut(n, es, -1);
                    check(lng(automorphismCount<lll>(g)) == a, "automorphismCount unrooted");
                    check(autCount.emplace(nu, a).fi->se == a, "automorphism count is an invariant");
                    int r = rnd(n);
                    check(lng(automorphismCount<lll>(CsrGraph(g), r)) == bruteAut(n, es, r), "automorphismCount rooted");}
                int j = n - 3;
                while (j >= 0 && seq[j] == n - 1) { seq[j--] = 0; }
                if (j < 0) { break; }
                ++seq[j];}
            // Pairs: random pairs of labelled trees, mapping exists iff naive strings agree.
            int pairs = mode == "quick" ? 300 : 3000;
            for (int t = 0; t < pairs && n >= 1; ++t) {
                auto &a = all[rnd(int(all.size()))], &b = rng() % 2 ? all[rnd(int(all.size()))] : a;
                vector<int> perm;
                auto c = rng() % 2 ? relabel(n, b, perm) : b;
                Naive na(n, a), nc(n, c);
                context = "pair " + show(n, a) + " | " + show(n, c);
                auto f = unrootedTreeIsomorphism(build(n, a), CsrGraph(build(n, c)));
                check(f.empty() != (na.unrooted() == nc.unrooted()), "unrootedTreeIsomorphism decision");
                check(f.empty() || validMap(n, a, c, f, -1, -1), "unrootedTreeIsomorphism mapping");
                int r = rnd(n), s = rnd(n);
                auto fr = rootedTreeIsomorphism(build(n, a), r, build(n, c), s);
                check(fr.empty() != (na.rooted(r) == nc.rooted(s)), "rootedTreeIsomorphism decision");
                check(fr.empty() || validMap(n, a, c, fr, r, s), "rootedTreeIsomorphism mapping");}}
        cout << "PASS every labelled tree n<=" << bound << " (all roots), random pairs\n";}

    // Symmetric families: copies of one rooted tree on a center or across a central edge.
    void randomCases() {
        int rounds = mode == "quick" ? 200 : mode == "full" || CHECKED ? 2000 : 10000;
        // Saved regression: child hash sums agree here ({1 leaf, 3 leaves} against {2 leaves, 2 leaves}); products differ.
        Graph a = build(7, {{0, 1}, {1, 2}, {0, 3}, {3, 4}, {3, 5}, {3, 6}}), b = build(7, {{0, 1}, {1, 2}, {1, 3}, {0, 4}, {4, 5}, {4, 6}});
        context = "regression additive hash collision";
        check(rootedTreeHash(a, 0, seed)[0] != rootedTreeHash(b, 0, seed)[0] && rootedTreeIsomorphism(a, 0, b, 0).empty(), "rooted hash separates the sum-collision pair");
        for (int t = 0; t < rounds; ++t) {
            int base = 1 + rnd(t % 4 == 0 ? 40 : 8), copies = 1 + rnd(4), kind = rnd(3);
            auto es = shaped(base, rnd(6));
            int root = rnd(base);
            vector<pair<int, int>> all;
            int n = kind == 0 ? base : kind == 1 ? copies * base + 1 : 2 * base;
            for (int c = 0; c < (kind == 0 ? 1 : kind == 1 ? copies : 2); ++c) {
                for (auto [u, v] : es) { all.push_back({u + c * base, v + c * base}); }
                if (kind == 1) { all.push_back({copies * base, root + c * base}); }}
            if (kind == 2) { all.push_back({root, root + base}); }
            vector<int> perm;
            auto shuffled = relabel(n, all, perm);
            ++cases; context = "random kind=" + std::to_string(kind) + " " + show(n, shuffled);
            Graph g = build(n, shuffled);
            Naive nv(n, shuffled);
            int r = rnd(n);
            verifyClasses(g, nv, r);
            vector<string> rooted(n);
            for (int v = 0; v < n; ++v) { rooted[v] = nv.rooted(v); }
            check(samePartition(allRootsClassification(g), rooted), "allRootsClassification orbits");
            vector<int> p2;
            auto other = relabel(n, shuffled, p2);
            auto f = unrootedTreeIsomorphism(g, build(n, other));
            check(validMap(n, shuffled, other, f, -1, -1), "unrootedTreeIsomorphism on a relabelled copy");
            auto bigger = other;
            bigger.push_back({rnd(n), n});
            check(unrootedTreeIsomorphism(g, build(n + 1, bigger)).empty() && rootedTreeIsomorphism(g, r, build(n + 1, bigger), rnd(n + 1)).empty(), "sizes differ: no mapping");
            check(treeCanonicalString(g) == treeCanonicalString(build(n, other)), "canonical string relabel invariant");
            check(automorphismCount<mint>(g) == automorphismCount<mint>(build(n, other)), "automorphism count relabel invariant");
            if (n <= 9) { check(lng(automorphismCount<lll>(g)) == bruteAut(n, shuffled, -1), "automorphismCount symmetric family"); }}
        cout << "PASS symmetric families rounds=" << rounds << '\n';}

    void large() {
        int n = mode == "quick" ? 20000 : CHECKED ? min(LARGE, mode == "full" ? 200000 : 1000000) : mode == "full" ? 200000 : 1000000;
        vector<pair<int, int>> path;
        for (int v = 1; v < n; ++v) { path.push_back({v - 1, v}); }
        Graph p = build(n, path);
        context = "large path n=" + std::to_string(n);
        auto id = rootedTreeClassification(p, 0);
        for (int v = 0; v < n; ++v) { check(id[v] == n - 1 - v, "path ids are heights"); }
        auto orb = allRootsClassification(p);
        for (int v = 0; v < n; ++v) { check(orb[v] == orb[n - 1 - v], "path orbits pair mirror vertices"); }
        check(int(set<int>(orb.begin(), orb.begin() + (n + 1) / 2).size()) == (n + 1) / 2, "path half has distinct orbits");
        check(automorphismCount<mint>(p) == mint(2) && automorphismCount<mint>(p, 0) == mint(1), "path automorphisms");
        Graph star(n);
        for (int v = 1; v < n; ++v) { star.addEdge(0, v); }
        mint fact = 1;
        for (int i = 2; i < n; ++i) { fact *= i; }
        check(automorphismCount<mint>(star) == fact && automorphismCount<mint>(star, 1) == fact / (n - 1), "star automorphisms");
        int d = int(std::bit_width(uint(n))) - 1, m = (1 << d) - 1, moved_pairs = 0;
        Graph bin(m);
        for (int v = 1; v < m; ++v) { bin.addEdge((v - 1) / 2, v); }
        context = "large complete binary tree n=" + std::to_string(m);
        check(automorphismCount<mint>(bin) == pow(mint(2), (1 << (d - 1)) - 1), "complete binary automorphisms");
        auto bid = rootedTreeClassification(bin, 0);
        for (int v = 1; v < m; ++v) { check(bid[v] == bid[(v - 1) / 2] - 1, "binary ids by depth"); }
        for (int shape = 0; shape < 6; ++shape) {
            auto es = shaped(n, shape);
            vector<int> perm;
            auto other = relabel(n, es, perm);
            Graph g = build(n, es), h = build(n, other);
            context = "large shape=" + std::to_string(shape) + " n=" + std::to_string(n);
            auto f = unrootedTreeIsomorphism(g, h);
            check(validMap(n, es, other, f, -1, -1), "large unrootedTreeIsomorphism mapping");
            auto fr = rootedTreeIsomorphism(g, 0, h, perm[0]);
            check(validMap(n, es, other, fr, 0, perm[0]), "large rootedTreeIsomorphism mapping");
            check(treeCanonicalString(g) == treeCanonicalString(h) && treeCanonicalString(g, 0) == treeCanonicalString(h, perm[0]), "large canonical strings");
            check(rootedTreeHash(g, 0, seed)[0] == rootedTreeHash(h, perm[0], seed)[perm[0]], "large rooted hash");
            check(automorphismCount<mint>(g) == automorphismCount<mint>(h), "large automorphism invariant");
            auto og = allRootsClassification(g), oh = allRootsClassification(h);
            map<int, int> link;
            for (int v = 0; v < n; ++v) { check(link.emplace(og[v], oh[f[v]]).fi->se == oh[f[v]], "orbits map onto orbits"); }
            // Moving a leaf off a vertex of degree >= 3 onto another leaf changes the degree multiset.
            vector<int> deg(n);
            for (auto [u, v] : es) { ++deg[u]; ++deg[v]; }
            int x = -1, q = -1;
            for (int i = 0; i < n - 1 && x == -1; ++i) {
                auto [u, v] = es[i];
                if (deg[u] == 1 && deg[v] >= 3) { x = i; }
                if (deg[v] == 1 && deg[u] >= 3) { x = i; std::swap(es[i].fi, es[i].se); }}
            for (int v = 0; v < n && x != -1; ++v) {
                if (deg[v] == 1 && v != es[x].fi) { q = v; break; }}
            if (x == -1 || q == -1) { continue; }
            es[x].se = q;
            Graph moved = build(n, es);
            check(unrootedTreeIsomorphism(moved, h).empty() && treeCanonicalString(moved) != treeCanonicalString(h), "large non-isomorphic pair");
            ++moved_pairs;}
        cout << "PASS large path/star/binary/six shapes n=" << n << " non-isomorphic pairs=" << moved_pairs << '\n';}

    void invalid(const string &name) {
        Graph g = build(3, {{0, 1}, {1, 2}}), cyc = build(3, {{0, 1}, {1, 2}, {2, 0}}), split = build(4, {{0, 1}, {1, 2}, {2, 0}});
        Graph dir(2, true); dir.addEdge(0, 1);
        if (name == "directed") { rootedTreeClassification(dir, 0); }
        else if (name == "root-range") { rootedTreeClassification(g, 3); }
        else if (name == "cycle") { treeCanonicalString(cyc); }
        else if (name == "disconnected") { allRootsClassification(split); }
        else if (name == "empty") { treeCanonicalString(Graph(0)); }
        else if (name == "hash-root") { rootedTreeHash(g, -1, 1); }
        else if (name == "iso-root") { rootedTreeIsomorphism(g, 0, g, 5); }
        else if (name == "automorphism-root") { automorphismCount<lng>(g, -2); }
        else if (name == "unrooted-cycle") { unrootedTreeIsomorphism(g, cyc); }
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
        exhaustive(); randomCases(); large();
        cout << "PASS treeiso seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL treeiso seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
