#include "../../04-Graphs/21-matching_bipartite.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    using Edges = vector<pair<int, int>>;
    string show(int nl, int nr, const Edges &es, const vector<int> &alive) {
        std::ostringstream out; out << "nl=" << nl << " nr=" << nr << " edges=";
        for (int e : alive) { out << e << ':' << es[e].first << '-' << es[e].second << ' '; }
        return out.str();}
    // Every matching of the alive edges, by recursion over left vertices; the oracle for all matching queries.
    struct Brute {
        int best = 0;
        vector<vector<int>> maximum;
        Brute(int nl, int nr, const Edges &es, const vector<int> &alive) {
            vector<vector<int>> by(nl);
            for (int e : alive) { by[es[e].first].push_back(e); }
            vector<int> cur; vector<uint8_t> used(nr);
            vector<vector<int>> all;
            auto rec = [&](auto &&rec, int u) -> void {
                if (u == nl) { all.push_back(cur); return; }
                rec(rec, u + 1);
                for (int e : by[u]) {
                    int v = es[e].second;
                    if (used[v]) { continue; }
                    used[v] = 1; cur.push_back(e); rec(rec, u + 1); cur.pop_back(); used[v] = 0;}};
            rec(rec, 0);
            for (auto &m : all) { best = max(best, int(m.size())); }
            for (auto &m : all) {
                if (int(m.size()) == best) { sort(m.begin(), m.end()); maximum.push_back(m); }}}
    };
    void validMatching(const BipartiteMatching &bm, const vector<int> &alive) {
        set<int> live(alive.begin(), alive.end());
        int count = 0;
        for (int u = 0; u < bm.nl; ++u) {
            if (bm.left[u] == -1) { continue; }
            int e = bm.left[u]; ++count;
            check(live.count(e) && bm.edges[e].first == u && bm.right[bm.edges[e].second] == e, "left/right matched edge consistency");}
        for (int v = 0; v < bm.nr; ++v) {
            check(bm.right[v] == -1 || bm.left[bm.edges[bm.right[v]].first] == bm.right[v], "right side consistency");}
        check(count == bm.size && int(bm.match().size()) == count, "size and match()");
        for (int u = 0; u < bm.nl; ++u) {
            int v = bm.mate(u);
            check(v == (bm.left[u] == -1 ? -1 : bm.edges[bm.left[u]].second) && (v == -1 || bm.edges[bm.right[v]].first == u), "mate consistent with left and right");}
        auto m = bm.match();
        for (int i = 0; i < int(m.size()); ++i) { check(i == 0 || bm.edges[m[i - 1]].first < bm.edges[m[i]].first, "match() in left order"); }}
    void queries(const BipartiteMatching &bm, const vector<int> &alive, const Brute &brute) {
        int nl = bm.nl, nr = bm.nr, V = nl + nr;
        const auto &es = bm.edges;
        auto cover = bm.konigVertexCover();
        vector<uint8_t> inL(nl), inR(nr);
        for (int u : cover.left) { inL[u] = 1; }
        for (int v : cover.right) { inR[v] = 1; }
        check(int(cover.left.size() + cover.right.size()) == brute.best, "Konig cover size equals maximum matching");
        check(std::is_sorted(cover.left.begin(), cover.left.end()) && std::is_sorted(cover.right.begin(), cover.right.end()), "cover sorted");
        for (int e : alive) { check(inL[es[e].first] || inR[es[e].second], "cover covers every edge"); }
        auto ind = bm.maximumIndependentSet();
        check(int(ind.left.size() + ind.right.size()) == V - brute.best, "independent set size");
        vector<uint8_t> jL(nl), jR(nr);
        for (int u : ind.left) { jL[u] = 1; }
        for (int v : ind.right) { jR[v] = 1; }
        for (int e : alive) { check(!(jL[es[e].first] && jR[es[e].second]), "independent set has no edge"); }
        vector<int> ec;
        bool ok = bm.minimumEdgeCover(ec);
        vector<int> deg(V);
        for (int e : alive) { ++deg[es[e].first]; ++deg[nl + es[e].second]; }
        bool coverable = std::all_of(deg.begin(), deg.end(), [](int d) { return d > 0; });
        check(ok == coverable && (ok || ec.empty()), "edge cover existence");
        if (ok) {
            vector<uint8_t> hit(V);
            set<int> live(alive.begin(), alive.end());
            for (int e : ec) { check(live.count(e) > 0, "edge cover uses alive edges"); hit[es[e].first] = hit[nl + es[e].second] = 1; }
            check(std::all_of(hit.begin(), hit.end(), [](uint8_t h) { return h != 0; }), "edge cover covers every vertex");
            int bestCover = INT_MAX;
            if (alive.size() <= 14) {
                for (int mask = 0; mask < (1 << alive.size()); ++mask) {
                    vector<uint8_t> h(V);
                    for (int i = 0; i < int(alive.size()); ++i) {
                        if (mask >> i & 1) { h[es[alive[i]].first] = h[nl + es[alive[i]].second] = 1; }}
                    if (std::all_of(h.begin(), h.end(), [](uint8_t x) { return x != 0; })) { bestCover = min(bestCover, __builtin_popcount(mask)); }}
                check(int(ec.size()) == bestCover, "edge cover minimum by subset enumeration");}}
        auto hall = bm.hallViolator();
        check(hall.empty() == (brute.best == nl), "Hall violator exists iff left side not saturated");
        if (!hall.empty()) {
            set<int> nb;
            for (int e : alive) {
                if (std::binary_search(hall.begin(), hall.end(), es[e].first)) { nb.insert(es[e].second); }}
            check(std::is_sorted(hall.begin(), hall.end()) && nb.size() < hall.size(), "Hall violator has |N(S)| < |S|");}
        auto ess = bm.essentialEdges();
        check(ess.size() == es.size(), "essential size");
        vector<int> some(es.size()), every(es.size());
        for (int e : alive) {
            int c = 0;
            for (auto &m : brute.maximum) { c += std::binary_search(m.begin(), m.end(), e); }
            some[e] = c > 0; every[e] = c == int(brute.maximum.size()) && brute.best > 0;}
        for (int e = 0; e < int(es.size()); ++e) {
            int expected = every[e] ? 2 : some[e] ? 1 : 0;
            check(ess[e] == expected, "essential edge status " + std::to_string(e) + " expected " + std::to_string(expected) + " got " + std::to_string(ess[e]));}
        vector<uint8_t> exposedL(nl), exposedR(nr);
        for (auto &m : brute.maximum) {
            vector<uint8_t> cl(nl), cr(nr);
            for (int e : m) { cl[es[e].first] = cr[es[e].second] = 1; }
            for (int u = 0; u < nl; ++u) { exposedL[u] |= !cl[u]; }
            for (int v = 0; v < nr; ++v) { exposedR[v] |= !cr[v]; }}
        vector<uint8_t> w0L(exposedL), w0R(nr), winfL(nl), winfR(exposedR);
        for (int e : alive) {
            if (exposedL[es[e].first]) { w0R[es[e].second] = 1; }
            if (exposedR[es[e].second]) { winfL[es[e].first] = 1; }}
        auto dm = bm.dulmageMendelsohn();
        int k = dm.k;
        check(k >= 2 && int(dm.left.size()) == nl && int(dm.right.size()) == nr, "DM shape");
        for (int u = 0; u < nl; ++u) {
            check((dm.left[u] == k - 1) == bool(w0L[u]) && (dm.left[u] == 0) == bool(winfL[u]), "DM surplus blocks, left");}
        for (int v = 0; v < nr; ++v) {
            check((dm.right[v] == k - 1) == bool(w0R[v]) && (dm.right[v] == 0) == bool(winfR[v]), "DM surplus blocks, right");}
        for (int e : alive) { check(dm.left[es[e].first] <= dm.right[es[e].second], "DM block order on every edge"); }
        vector<int> parent(V); iota(parent.begin(), parent.end(), 0);
        auto find = [&](auto &&find, int x) -> int { return parent[x] == x ? x : parent[x] = find(find, parent[x]); };
        for (int e : alive) {
            int u = es[e].first, v = es[e].second;
            if (some[e] && !w0L[u] && !winfL[u] && !w0R[v] && !winfR[v]) { parent[find(find, u)] = find(find, nl + v); }}
        vector<int> blockOf(V);
        for (int u = 0; u < nl; ++u) { blockOf[u] = dm.left[u]; }
        for (int v = 0; v < nr; ++v) { blockOf[nl + v] = dm.right[v]; }
        set<int> middle;
        for (int x = 0; x < V; ++x) {
            bool mid = x < nl ? !w0L[x] && !winfL[x] : !w0R[x - nl] && !winfR[x - nl];
            if (!mid) { continue; }
            middle.insert(blockOf[x]);
            check(1 <= blockOf[x] && blockOf[x] <= k - 2, "DM middle block range");
            for (int y = 0; y < V; ++y) {
                bool midY = y < nl ? !w0L[y] && !winfL[y] : !w0R[y - nl] && !winfR[y - nl];
                if (midY) { check((blockOf[x] == blockOf[y]) == (find(find, x) == find(find, y)), "DM middle blocks are allowed-edge components"); }}}
        check(int(middle.size()) == k - 2, "DM block count");}
    BipartiteMatching build(int nl, int nr, const Edges &es, int method) {
        if (method == 2) { return bipartiteMatchingDense(nl, nr, es); }
        BipartiteMatching bm(nl, nr, es);
        if (method == 0) { bm.hopcroftKarp(); }
        else if (method == 1) { bm.kuhn(); }
        else { while (bm.augment()) {} }
        return bm;}
    void bCheck(int nl, int nr, const Edges &es, std::mt19937_64 &rng) {
        vector<int> bl(nl), br(nr);
        for (int &b : bl) { b = int(rng() % 4); }
        for (int &b : br) { b = int(rng() % 4); }
        auto res = bMatching(nl, nr, es, bl, br);
        vector<int> dl(nl), dr(nr);
        check(std::is_sorted(res.begin(), res.end()) && std::adjacent_find(res.begin(), res.end()) == res.end(), "b-matching ascending distinct");
        for (int e : res) { ++dl[es[e].first]; ++dr[es[e].second]; }
        for (int u = 0; u < nl; ++u) { check(dl[u] <= bl[u], "b-matching left capacity"); }
        for (int v = 0; v < nr; ++v) { check(dr[v] <= br[v], "b-matching right capacity"); }
        int best = 0, m = int(es.size());
        for (int mask = 0; mask < (1 << m); ++mask) {
            vector<int> a(nl), b(nr); bool ok = true;
            for (int i = 0; i < m && ok; ++i) {
                if (mask >> i & 1) { ok = ++a[es[i].first] <= bl[es[i].first] && ++b[es[i].second] <= br[es[i].second]; }}
            if (ok) { best = max(best, __builtin_popcount(mask)); }}
        check(int(res.size()) == best, "b-matching maximum by subset enumeration");
        vector<int> ones(nl, 1), onesR(nr, 1);
        check(int(bMatching(nl, nr, es, ones, onesR).size()) == Brute(nl, nr, es, [&] { vector<int> a(es.size()); iota(a.begin(), a.end(), 0); return a; }()).best, "unit b-matching equals matching");}
    void runCase(int nl, int nr, const Edges &es, std::mt19937_64 &rng) {
        ++cases;
        vector<int> alive(es.size()); iota(alive.begin(), alive.end(), 0);
        context = show(nl, nr, es, alive);
        Brute brute(nl, nr, es, alive);
        for (int method = 0; method < 4; ++method) {
            auto bm = build(nl, nr, es, method);
            check(bm.size == brute.best, "maximum size method " + std::to_string(method));
            validMatching(bm, alive);
            check(!BipartiteMatching(bm).augment(), "no augmenting path after maximum");
            if (method == 0) { queries(bm, alive, brute); }}
        if (es.size() <= 12) { bCheck(nl, nr, es, rng); }}
    Edges randomEdges(std::mt19937_64 &rng, int nl, int nr, int m) {
        Edges es;
        for (int i = 0; i < m && nl && nr; ++i) { es.emplace_back(int(rng() % nl), int(rng() % nr)); }
        return es;}
    void exhaustive() {
        for (int nl = 0; nl <= 3; ++nl) {
            for (int nr = 0; nr <= 3; ++nr) {
                int pairs = nl * nr;
                if (mode == "quick" && pairs > 6) { continue; }
                for (int mask = 0; mask < (1 << pairs); ++mask) {
                    Edges es;
                    for (int i = 0; i < pairs; ++i) {
                        if (mask >> i & 1) { es.emplace_back(i / nr, i % nr); }}
                    std::mt19937_64 rng(seed + ulng(mask));
                    runCase(nl, nr, es, rng);}}}
        cout << "PASS exhaustive simple bipartite graphs up to 3x3 cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 4000 : 30000;
        for (int t = 0; t < rounds; ++t) {
            int nl = int(rng() % 7), nr = int(rng() % 7), m = int(rng() % 13);
            runCase(nl, nr, randomEdges(rng, nl, nr, m), rng);}
        cout << "PASS random multigraphs nl,nr<=6 rounds=" << rounds << '\n';}
    void dynamicCases() {
        std::mt19937_64 rng(seed ^ 0xd1);
        int rounds = mode == "quick" ? 60 : mode == "full" ? 600 : 4000;
        for (int t = 0; t < rounds; ++t) {
            int nl = 1 + int(rng() % 5), nr = 1 + int(rng() % 5);
            BipartiteMatching bm(nl, nr);
            vector<int> alive;
            for (int step = 0; step < 20; ++step) {
                if (alive.empty() || rng() % 3) { alive.push_back(bm.addEdge(int(rng() % nl), int(rng() % nr))); }
                else {
                    int i = int(rng() % alive.size());
                    bm.eraseEdge(alive[i]); alive.erase(alive.begin() + i);}
                // Warm starts: augment once, or let Hopcroft-Karp / Kuhn grow the surviving matching.
                if (step % 3 == 0) { bm.augment(); }
                else if (step % 3 == 1) { bm.hopcroftKarp(); }
                else { bm.kuhn(); }
                ++cases; context = show(nl, nr, bm.edges, alive) + " dynamic step=" + std::to_string(step);
                Brute brute(nl, nr, bm.edges, alive);
                check(bm.size == brute.best, "augment, warm Hopcroft-Karp or warm Kuhn restores maximum after an update");
                validMatching(bm, alive);
                if (step % 5 == 4) { queries(bm, alive, brute); }}}
        cout << "PASS dynamic add/erase/augment rounds=" << rounds << '\n';}
    void large() {
        std::mt19937_64 rng(seed ^ 0x1a);
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        Edges es;
        for (int i = 0; i < n; ++i) { es.emplace_back(i, i); if (i + 1 < n) { es.emplace_back(i + 1, i); } }
        std::reverse(es.begin(), es.end());
        context = "path n=" + std::to_string(n);
        BipartiteMatching hk(n, n, es);
        hk.hopcroftKarp();
        check(hk.size == n, "path perfect matching (Hopcroft-Karp)");
        validMatching(hk, [&] { vector<int> a(es.size()); iota(a.begin(), a.end(), 0); return a; }());
        auto ess = hk.essentialEdges();
        for (int e = 0; e < int(es.size()); ++e) { check(ess[e] == (es[e].first == es[e].second ? 2 : 0), "path unique perfect matching forced"); }
        auto dm = hk.dulmageMendelsohn();
        check(dm.k == n + 2, "path DM has n singleton blocks");
        int small = mode == "quick" ? 2000 : 20000;
        Edges ps(es.end() - (2 * small - 1), es.end());
        BipartiteMatching kp(small, small, ps); kp.kuhn();
        check(kp.size == small, "Kuhn long augmenting paths");
        int d = mode == "quick" ? 300 : 1500;
        Edges dense = randomEdges(rng, d, d, d * d / 3);
        auto a = bipartiteMatchingDense(d, d, dense);
        BipartiteMatching b(d, d, dense); b.hopcroftKarp();
        BipartiteMatching c(d, d, dense); c.kuhn();
        context = "dense random d=" + std::to_string(d);
        check(a.size == b.size && b.size == c.size, "dense, Hopcroft-Karp and Kuhn agree");
        vector<int> all(dense.size()); iota(all.begin(), all.end(), 0);
        validMatching(a, all);
        auto cover = b.konigVertexCover();
        check(int(cover.left.size() + cover.right.size()) == b.size, "large cover certificate size");
        vector<uint8_t> inL(d), inR(d);
        for (int u : cover.left) { inL[u] = 1; }
        for (int v : cover.right) { inR[v] = 1; }
        for (auto [u, v] : dense) { check(inL[u] || inR[v], "large cover covers"); }
        int big = mode == "quick" ? 20000 : 200000;
        Edges sparse = randomEdges(rng, big, big, 3 * big);
        BipartiteMatching s1(big, big, sparse); s1.hopcroftKarp();
        auto cs = s1.konigVertexCover();
        context = "sparse random n=" + std::to_string(big);
        check(int(cs.left.size() + cs.right.size()) == s1.size, "sparse cover certificate");
        vector<int> bl(big, 2), br(big, 1);
        auto bm = bMatching(big, big, sparse, bl, br);
        vector<int> dl(big), dr(big);
        for (int e : bm) { check(++dl[sparse[e].first] <= 2 && ++dr[sparse[e].second] <= 1, "large b-matching capacities"); }
        check(int(bm.size()) >= s1.size, "b-matching with larger left capacity at least matching size");
        cout << "PASS large paths, dense d=" << d << ", sparse n=" << big << '\n';}
    void invalid(const string &name) {
        if (name == "edge-range") { BipartiteMatching bm(2, 2, {{0, 2}}); }
        else if (name == "negative-size") { BipartiteMatching bm(-1, 2); }
        else if (name == "erase-twice") { BipartiteMatching bm(2, 2, {{0, 1}}); bm.eraseEdge(0); bm.eraseEdge(0); }
        else if (name == "erase-range") { BipartiteMatching bm(2, 2); bm.eraseEdge(0); }
        else if (name == "cover-not-maximum") { BipartiteMatching bm(1, 1, {{0, 0}}); bm.konigVertexCover(); }
        else if (name == "bmatching-size") { bMatching(2, 2, {{0, 0}}, {1}, {1, 1}); }
        else if (name == "bmatching-negative") { bMatching(1, 1, {{0, 0}}, {-1}, {1}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); dynamicCases(); large();
        cout << "PASS matching seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL matching seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
