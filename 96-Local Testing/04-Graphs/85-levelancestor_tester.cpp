#include "../../04-Graphs/85-levelancestor.hpp"

static ulng seed = 20260927;
static lng checks = 0;
static string context;
static const array<lng, 3> HUGE_K{lng(INT_MAX), lng(1) << 40, std::numeric_limits<lng>::max()};

void require(bool ok, const string &op, const string &want = "true", const string &got = "false") {
    ++checks;
    if (!ok) { throw std::runtime_error(context + " operation=" + op + " expected=" + want + " actual=" + got); }}
void expect(int got, int want, const string &op) { require(got == want, op, std::to_string(want), std::to_string(got)); }
void expectAt(int got, int want, const char *op, int u, lng k) {
    ++checks;
    if (got != want) {
        throw std::runtime_error(context + " operation=" + op + " u=" + std::to_string(u) + " k=" + std::to_string(k)
            + " expected=" + std::to_string(want) + " actual=" + std::to_string(got));}}
string describe(const Graph &g, int root, const string &name = "") {
    string s = name + " n=" + std::to_string(g.n) + " root=" + std::to_string(root) + " edges=";
    if (g.edges.size() > 40) { return s + std::to_string(g.edges.size()) + " seed=" + std::to_string(seed); }
    for (auto e : g.edges) { s += "(" + std::to_string(e.u) + "," + std::to_string(e.v) + ")"; }
    return s;}

// BFS rooting with the documented root rule; ancestors by parent walks or an explicit root-path DFS.
struct Oracle {
    int n;
    vector<vector<int>> adj, child;
    vector<int> parent, depth, height;
    explicit Oracle(const Graph &g, int root = -1) : n(g.n), adj(n), child(n), parent(n, -2), depth(n), height(n, 1) {
        for (auto e : g.edges) { adj[e.u].push_back(e.v); adj[e.v].push_back(e.u); }
        vector<int> q;
        auto bfs = [&](int r) {
            parent[r] = -1; q.push_back(r);
            for (size_t i = q.size() - 1; i < q.size(); ++i) {
                int u = q[i];
                for (int v : adj[u]) {
                    if (parent[v] != -2) { continue; }
                    parent[v] = u; depth[v] = depth[u] + 1; child[u].push_back(v); q.push_back(v);}}};
        if (root != -1) { bfs(root); }
        for (int u = 0; u < n; ++u) { if (parent[u] == -2) { bfs(u); }}
        for (size_t i = q.size(); i-- > 0;) {
            int u = q[i];
            if (parent[u] >= 0) { height[parent[u]] = max(height[parent[u]], height[u] + 1); }}}
    int ancestor(int u, lng k) const {
        while (k > 0 && u != -1) { --k; u = parent[u]; }
        return u;}
    vector<int> batch(const vector<pair<int, lng>> &queries) const {
        vector<vector<int>> at(n); vector<int> res(queries.size(), -1), path; vector<pair<int, size_t>> stack;
        for (size_t i = 0; i < queries.size(); ++i) { at[queries[i].first].push_back(int(i)); }
        auto enter = [&](int u) {
            path.push_back(u); stack.push_back({u, 0});
            for (int j : at[u]) {
                lng k = queries[size_t(j)].second;
                if (0 <= k && k < lng(path.size())) { res[size_t(j)] = path[path.size() - 1 - size_t(k)]; }}};
        for (int r = 0; r < n; ++r) {
            if (parent[r] != -1) { continue; }
            enter(r);
            while (!stack.empty()) {
                auto &[u, i] = stack.back();
                if (i == child[u].size()) { stack.pop_back(); path.pop_back(); }
                else { enter(child[u][i++]); }}}
        return res;}
};

// Forest fields, long-path bottoms, ladder tiling and jump pointers, answered by one oracle batch.
void structure(const LevelAncestor &la, const Oracle &o, int root) {
    int n = o.n;
    require(la.n == n && la.parent == o.parent && la.dep == o.depth, "Forest parent/dep against BFS");
    expect(la.root, root == -1 && n ? 0 : root, "preferred root field");
    require(la.bottom.size() == size_t(n) && la.pos.size() == size_t(n) && la.jump_start.size() == size_t(n), "per-vertex array sizes");
    require(la.ladder.size() <= size_t(2 * n), "ladder.size() <= 2n", std::to_string(2 * n), std::to_string(la.ladder.size()));
    size_t total = 0; vector<int> leaves, len(n);
    for (int u = 0; u < n; ++u) {
        if (o.child[u].empty()) { leaves.push_back(u); total += size_t(std::bit_width(uint(o.depth[u]))); }}
    require(la.jumps.size() == total, "jumps.size() = sum of bit_width(dep) over leaves", std::to_string(total), std::to_string(la.jumps.size()));
    vector<pair<int, lng>> q; vector<int> want;
    for (int u = 0; u < n; ++u) {
        int b = la.bottom[u], p = la.pos[u];
        require(0 <= b && b < n && o.child[b].empty(), "bottom is a leaf u=" + std::to_string(u), "leaf", std::to_string(b));
        expect(o.depth[b] - o.depth[u] + 1, o.height[u], "bottom is a deepest leaf u=" + std::to_string(u));
        if (!o.child[u].empty()) {
            int c = *std::find_if(o.child[u].begin(), o.child[u].end(), [&](int v) { return o.height[v] + 1 == o.height[u]; });
            expect(la.bottom[c], b, "long path continues through the first deepest child u=" + std::to_string(u));}
        require(0 <= p && size_t(p) < la.ladder.size() && la.ladder[size_t(p)] == u, "ladder[pos[u]] = u u=" + std::to_string(u));
        expect(p, la.pos[b] + o.depth[b] - o.depth[u], "pos follows the long path u=" + std::to_string(u));
        q.push_back({b, o.depth[b] - o.depth[u]}); want.push_back(u); ++len[size_t(b)];}
    sort(leaves.begin(), leaves.end(), [&](int x, int y) { return la.pos[x] < la.pos[y]; });
    size_t at = 0;
    for (int b : leaves) {
        int size = len[size_t(b)] + min(len[size_t(b)], o.depth[b] - len[size_t(b)] + 1);
        expect(la.pos[b], int(at), "ladders tile in order b=" + std::to_string(b));
        require(at + size_t(size) <= la.ladder.size(), "ladder of b=" + std::to_string(b) + " in range");
        for (int i = 0; i < size; ++i) { q.push_back({b, i}); want.push_back(la.ladder[at + size_t(i)]); }
        at += size_t(size);}
    expect(int(at), int(la.ladder.size()), "ladders cover the ladder array");
    sort(leaves.begin(), leaves.end(), [&](int x, int y) { return pair{la.jump_start[x], o.depth[x]} < pair{la.jump_start[y], o.depth[y]}; });
    at = 0;
    for (int b : leaves) {
        int bits = int(std::bit_width(uint(o.depth[b])));
        expect(la.jump_start[b], int(at), "jump tables tile in order b=" + std::to_string(b));
        for (int h = 0; h < bits; ++h) {
            int j = la.jumps[at + size_t(h)];
            require(0 <= j && size_t(j) < la.ladder.size() && la.pos[size_t(la.ladder[size_t(j)])] == j, "jump is the ancestor's own ladder position b=" + std::to_string(b));
            q.push_back({b, lng(1) << h}); want.push_back(la.ladder[size_t(j)]);}
        at += size_t(bits);}
    require(o.batch(q) == want, "bottoms, ladder segments and jumps are ancestor chains");}

// Every (u, k) with k in [0, n + 1] (or [0, dep + 1] plus n + 1 when wide), huge k, online, CSR and offline orders.
void verify(const Graph &g, int root, bool wide = false, const string &name = "") {
    context = describe(g, root, name); Oracle o(g, root);
    LevelAncestor la(g, root), csr(CsrGraph(g), root); structure(la, o, root);
    vector<pair<int, lng>> queries; vector<int> answers;
    for (int u = 0; u < g.n; ++u) {
        int w = u, top = wide ? o.depth[u] + 1 : g.n + 1;
        for (lng k = 0; k <= top; ++k) {
            expectAt(la.query(u, k), w, "query", u, k); expectAt(csr.query(u, k), w, "CSR query", u, k);
            require(wide || o.ancestor(u, k) == w, "oracle walk self-check"); queries.push_back({u, k}); answers.push_back(w);
            w = w == -1 ? -1 : o.parent[w];}
        for (lng k : HUGE_K) {
            expectAt(la.query(u, k), -1, "huge query", u, k); queries.push_back({u, k}); answers.push_back(-1);}
        if (wide) { expectAt(la.query(u, g.n + 1), -1, "query beyond n", u, g.n + 1); }
    }
    require(o.batch(queries) == answers, "oracle DFS batch against parent walks");
    require(levelAncestorOffline(g, queries, root) == answers, "offline all pairs");
    require(levelAncestorOffline(CsrGraph(g), queries, root) == answers, "offline CSR all pairs");
    std::mt19937_64 rng(seed ^ (ulng(g.n) * 1000003 + ulng(root + 1)));
    vector<size_t> perm(queries.size()); iota(perm.begin(), perm.end(), size_t(0)); std::shuffle(perm.begin(), perm.end(), rng);
    for (size_t i = 0; i < queries.size() / 4; ++i) { perm.push_back(perm[rng() % perm.size()]); }
    vector<pair<int, lng>> mixed; vector<int> expected;
    for (size_t i : perm) { mixed.push_back(queries[i]); expected.push_back(answers[i]); }
    require(levelAncestorOffline(g, mixed, root) == expected, "offline shuffled with duplicates");
    require(levelAncestorOffline(g, {}, root).empty(), "offline empty query list");
}

void exhaustive(const string &mode) {
    int bound = mode == "quick" ? 4 : mode == "full" ? 6 : 7; lng cases = 0;
    const array<lng, 8> forests{1, 1, 2, 7, 38, 291, 2932, 36961};
    for (int n = 0; n <= bound; ++n) {
        vector<pair<int, int>> slots; lng count = 0;
        for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) { slots.push_back({u, v}); }}
        for (uint mask = 0; mask < (1U << slots.size()); ++mask) {
            vector<int> dsu(n); iota(dsu.begin(), dsu.end(), 0); Graph g(n); bool forest = true;
            auto find = [&](int x) { while (dsu[size_t(x)] != x) { x = dsu[size_t(x)] = dsu[size_t(dsu[size_t(x)])]; } return x; };
            for (size_t i = 0; i < slots.size() && forest; ++i) { if (mask >> i & 1U) {
                auto [u, v] = slots[i]; int a = find(u), b = find(v);
                forest = a != b; dsu[size_t(a)] = b; g.addEdge(u, v);}}
            if (!forest) { continue; }
            ++count;
            for (int root = -1; root < n; ++root) { verify(g, root); ++cases; }}
        context = "labeled forest count n=" + std::to_string(n); expect(int(count), int(forests[size_t(n)]), "OEIS A001858");}
    std::cout << "PASS exhaustive labeled forests through n=" << bound << " root-cases=" << cases << '\n';}

void randomized(const string &mode, int count) {
    std::mt19937_64 rng(seed);
    for (int i = 0; i < count; ++i) {
        int n = 1 + int(rng() % (i % 10 ? 40 : 160)), cut = 2 + int(rng() % 6);
        vector<int> perm(n); iota(perm.begin(), perm.end(), 0); std::shuffle(perm.begin(), perm.end(), rng);
        vector<pair<int, int>> edges;
        for (int u = 1; u < n; ++u) {
            if (rng() % ulng(cut)) { edges.push_back({perm[size_t(u)], perm[rng() % ulng(u)]}); }}
        std::shuffle(edges.begin(), edges.end(), rng); Graph g(n);
        for (auto [u, v] : edges) { if (rng() & 1) { swap(u, v); } g.addEdge(u, v); }
        verify(g, int(rng() % ulng(n + 1)) - 1, n > 40);}
    std::cout << "PASS seeded random forests count=" << count << " mode=" << mode << '\n';}

// Parent of u >= 1 in each shape; -2 leaves u isolated. Comb teeth share one length, so deep-child choices tie.
const vector<string> SHAPES{"path", "star", "binary", "caterpillar", "broom", "recursive", "comb", "spider", "path-binary",
    "deep-random", "forest"};
int shapeParent(const string &shape, int u, int n, std::mt19937_64 &rng) {
    int teeth = max(1, int(std::sqrt(double(n)))), tooth = max(1, (n - 1) / teeth), half = n / 2;
    if (shape == "path") { return u - 1; }
    if (shape == "star") { return 0; }
    if (shape == "binary") { return (u - 1) / 2; }
    if (shape == "caterpillar") { return u % 2 ? u - 1 : u - 2; }
    if (shape == "broom") { return u <= half ? u - 1 : half; }
    if (shape == "recursive") { return int(rng() % ulng(u)); }
    if (shape == "comb") { return (u - 1) % tooth ? u - 1 : 0; }
    if (shape == "spider") { return u <= 3 ? 0 : u - 3; }
    if (shape == "path-binary") { return u <= half ? u - 1 : half + (u - half - 1) / 2; }
    if (shape == "deep-random") { return u - 1 - int(rng() % ulng(min(u, 4))); }
    return rng() % 50 ? int(rng() % ulng(u)) : -2;}
Graph shapeGraph(const string &shape, int n, std::mt19937_64 &rng, bool relabel) {
    vector<int> label(n); iota(label.begin(), label.end(), 0);
    if (relabel) { std::shuffle(label.begin(), label.end(), rng); }
    Graph g(n);
    for (int u = 1; u < n; ++u) {
        int p = shapeParent(shape, u, n, rng);
        if (p >= 0) { g.addEdge(label[size_t(u)], label[size_t(p)]); }}
    return g;}

void shapes(const string &mode) {
    std::mt19937_64 rng(seed + 1); int cases = 0;
    vector<int> sizes{2, 3, 4, 5, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128};
    if (mode != "quick") { for (int n : {255, 256, 257, 511, 512, 513}) { sizes.push_back(n); }}
    for (int n : sizes) {
        for (const string &shape : SHAPES) {
            for (bool relabel : {false, true}) {
                Graph g = shapeGraph(shape, n, rng, relabel);
                vector<int> roots{-1, int(rng() % ulng(n))};
                if (!relabel && (n <= 128 || shape == "path")) { for (int r : {0, n - 1, n / 2}) { roots.push_back(r); }}
                for (int r : roots) { verify(g, r, n > 40, shape); ++cases; }}}}
    std::cout << "PASS shapes (path, star, binary, caterpillar, broom, recursive, comb, spider, path-binary, deep-random,"
        " forest) root-cases=" << cases << '\n';}

// Sampled k: uniform in [0, dep + 1], 2^h - 1, 2^h, 2^h + 1, dep - 2^h, dep and huge; checked online, offline and by closed form.
void sampled(const Graph &g, int root, const string &name, int samples, std::mt19937_64 &rng) {
    context = describe(g, root, name); Oracle o(g, root); LevelAncestor la(g, root); structure(la, o, root);
    context = describe(g, root, name);
    vector<pair<int, lng>> queries;
    for (int i = 0; i < samples; ++i) {
        int u = int(rng() % ulng(g.n)), d = o.depth[u], h = int(rng() % ulng(std::bit_width(uint(d)) + 1));
        array<lng, 8> ks{lng(rng() % ulng(d + 2)), (lng(1) << h) - 1, lng(1) << h, (lng(1) << h) + 1,
            max(lng(0), d - (lng(1) << h)), d, d + 1, HUGE_K[rng() % 3]};
        queries.push_back({u, ks[rng() % ks.size()]});}
    vector<int> want = o.batch(queries), off = levelAncestorOffline(g, queries, root);
    for (size_t i = 0; i < queries.size(); ++i) {
        auto [u, k] = queries[i];
        expectAt(la.query(u, k), want[i], "sampled query", u, k); expectAt(off[i], want[i], "sampled offline", u, k);
        if (name == "path" && (root == -1 || root == 0)) { expectAt(want[i], k <= u ? int(u - k) : -1, "closed-form path", u, k); }
        if (name == "path" && root == g.n - 1) { expectAt(want[i], k < g.n - u ? int(u + k) : -1, "closed-form reversed path", u, k); }}
    require(levelAncestorOffline(CsrGraph(g), queries, root) == want, "offline CSR sampled");}

void large(const string &mode) {
    std::mt19937_64 rng(seed + 2); int samples = mode == "quick" ? 20000 : 200000;
    vector<int> sizes{mode == "quick" ? 2000 : 200000};
    if (mode == "stress") { sizes.push_back(500000); }
    for (int n : sizes) {
        Graph path = shapeGraph("path", n, rng, false);
        for (int r : {-1, n - 1, n / 2}) { sampled(path, r, "path", samples, rng); }
        vector<string> names{"recursive"};
        if (mode != "quick") { names = {"caterpillar", "recursive", "deep-random", "path-binary", "comb", "forest"}; }
        int rounds = mode == "stress" && n < 500000 ? 2 : 1;
        if (n == 500000) { names = {"caterpillar", "recursive"}; }
        for (const string &name : names) {
            for (int i = 0; i < rounds; ++i) { sampled(shapeGraph(name, n, rng, true), int(rng() % ulng(n + 1)) - 1, name, samples, rng); }}}
    std::cout << "PASS large sampled n=" << sizes.back() << " samples=" << samples << '\n';}

void boundary() {
    context = "n=0";
    LevelAncestor empty(Graph(0)), csr(CsrGraph(Graph(0)), -1);
    require(empty.n == 0 && empty.ladder.empty() && empty.jumps.empty() && csr.ladder.empty(), "empty forest fields");
    require(levelAncestorOffline(Graph(0), {}).empty() && levelAncestorOffline(Graph(0), {}, -1).empty(), "offline on n=0");
    context = "n=1";
    for (int r : {-1, 0}) {
        LevelAncestor one(Graph(1), r);
        expect(one.query(0, 0), 0, "single vertex k=0"); expect(one.query(0, 1), -1, "single vertex k=1");
        expect(one.query(0, std::numeric_limits<lng>::max()), -1, "single vertex huge k");
        require(one.ladder == vector<int>{0} && one.jumps.empty(), "single vertex ladder");
        require(levelAncestorOffline(Graph(1), {{0, 1}, {0, 0}, {0, std::numeric_limits<lng>::max()}, {0, 0}}, r)
            == vector<int>{-1, 0, -1, 0}, "single vertex offline");}
    context = "many queries on one vertex, path n=60 root=0";
    Graph g(60); for (int u = 1; u < 60; ++u) { g.addEdge(u, u - 1); }
    std::mt19937_64 rng(seed + 3); vector<pair<int, lng>> queries; vector<int> want;
    for (int i = 0; i < 5000; ++i) { lng k = lng(rng() % 64); queries.push_back({59, k}); want.push_back(k <= 59 ? int(59 - k) : -1); }
    require(levelAncestorOffline(g, queries) == want, "offline many queries on one vertex");
    LevelAncestor la(g);
    for (auto [u, k] : queries) { expectAt(la.query(u, k), k <= 59 ? int(59 - k) : -1, "query many on one vertex", u, k); }
    context = "lifetime";
    LevelAncestor snapshot(g); g.addEdge(59, 0);
    expect(snapshot.query(59, 59), 0, "owning snapshot after input mutation");
    LevelAncestor copied = snapshot, moved = std::move(copied); copied = moved;
    expect(copied.query(30, 7), 23, "copy/move assignment"); expect(moved.query(59, 60), -1, "moved object");
    Graph h(4); h.addEdge(0, 1); h.addEdge(2, 3); snapshot = LevelAncestor(h, 3);
    expect(snapshot.query(2, 1), 3, "rebuild by assignment"); expect(snapshot.query(1, 1), 0, "rebuild second component");
    std::cout << "PASS n=0, n=1, many queries on one vertex, snapshot and copy semantics\n";}

void invalid(const string &probe) {
    Graph g(3); g.addEdge(0, 1); g.addEdge(1, 2);
    Graph directed(2, true); directed.addEdge(0, 1);
    if (probe == "directed") { LevelAncestor x(directed); }
    else if (probe == "offline-directed") { (void)levelAncestorOffline(directed, {}); }
    else if (probe == "root-negative") { LevelAncestor x(g, -2); }
    else if (probe == "root-end") { LevelAncestor x(g, 3); }
    else if (probe == "empty-root") { LevelAncestor x(Graph(0), 0); }
    else if (probe == "offline-root-end") { (void)levelAncestorOffline(g, {}, 3); }
    else if (probe == "cycle" || probe == "offline-cycle") {
        g.addEdge(2, 0);
        if (probe == "cycle") { LevelAncestor x(g); }
        else { (void)levelAncestorOffline(g, {{0, 0}}); }}
    else if (probe == "loop") { g.addEdge(1, 1); LevelAncestor x(g); }
    else if (probe == "parallel") { g.addEdge(2, 1); LevelAncestor x(g); }
    else if (probe == "offline-query-negative") { (void)levelAncestorOffline(g, {{0, 0}, {-1, 0}}); }
    else if (probe == "offline-query-end") { (void)levelAncestorOffline(g, {{3, 0}}); }
    else if (probe == "offline-k-negative") { (void)levelAncestorOffline(g, {{2, 1}, {0, -1}}); }
    else {
        LevelAncestor x(g);
        if (probe == "query-negative") { (void)x.query(-1, 0); }
        else if (probe == "query-end") { (void)x.query(3, 0); }
        else if (probe == "k-negative") { (void)x.query(0, -1); }
        else { throw std::runtime_error("unknown invalid probe " + probe); }}
    throw std::runtime_error("precondition accepted " + probe);}

int main(int argc, char **argv) {
    string mode = "full"; std::cout << std::unitbuf;
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 1; }
            else { throw std::runtime_error("unknown argument " + arg); }}
        boundary(); exhaustive(mode); randomized(mode, mode == "quick" ? 60 : mode == "full" ? 600 : 4000);
        shapes(mode); large(mode);
        std::cout << "PASS 85-levelancestor mode=" << mode << " seed=" << seed << " checks=" << checks << '\n';} catch (const std::exception &e) {
        std::cerr << "FAIL 85-levelancestor seed=" << seed << " " << e.what() << '\n'; return 1;}}
