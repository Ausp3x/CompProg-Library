#include "../../02-Data Structures/10-persistentdsu.hpp"
#include "../../02-Data Structures/01-dsu.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(int x) { return std::to_string(x); }
    string show(bool x) { return x ? "1" : "0"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    // Component labels of the graph on n vertices with the given edges (BFS).
    vector<int> labels(int n, const vector<pair<int, int>> &es) {
        vector<vector<int>> adj(n);
        for (auto [u, v] : es) { adj[u].push_back(v); adj[v].push_back(u); }
        vector<int> c(n, -1);
        int k = 0;
        for (int s = 0; s < n; ++s) {
            if (c[s] >= 0) { continue; }
            vector<int> q{s};
            c[s] = k;
            for (size_t i = 0; i < q.size(); ++i) { for (int y : adj[q[i]]) { if (c[y] < 0) { c[y] = k; q.push_back(y); } } }
            ++k;}
        return c;}

    // findSet must be a member of the component and the same vertex for every member; sizes and counts from labels.
    template<typename F, typename G, typename H>
    void checkState(int n, const vector<int> &c, F find, G size, H same, int cnt, const string &where) {
        int k = 1 + *std::max_element(c.begin(), c.end());
        checkEqual(cnt, k, where + " count");
        vector<int> rep(k, -1), sz(k, 0);
        for (int u = 0; u < n; ++u) { ++sz[c[u]]; }
        for (int u = 0; u < n; ++u) {
            int r = find(u);
            checkEqual(c[r], c[u], where + " findSet member u=" + show(u));
            if (rep[c[u]] < 0) { rep[c[u]] = r; }
            checkEqual(r, rep[c[u]], where + " findSet canonical u=" + show(u));
            checkEqual(size(u), sz[c[u]], where + " getSize u=" + show(u));
            int v = (u * 5 + 1) % n;
            checkEqual(same(u, v), c[u] == c[v], where + " isSameSet u=" + show(u) + " v=" + show(v));}}

    // Partially persistent: every time 0..q checked against BFS labels of the edge prefix; uniteTime by scanning t.
    void partialSmall(Rng &rng, int rounds, int maxN, int maxQ) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 1, maxN)), q = int(rnd(rng, 0, maxQ));
            context = "partial round=" + show(s) + " n=" + show(n) + " q=" + show(q);
            PartiallyPersistentDSU d(n);
            vector<pair<int, int>> es;
            vector<vector<int>> lab{labels(n, es)};
            for (int k = 0; k < q; ++k) {
                int u = int(rnd(rng, 0, n - 1)), v = rng() % 6 ? int(rnd(rng, 0, n - 1)) : u;
                checkEqual(d.uniteSets(u, v), lab.back()[u] != lab.back()[v], "uniteSets return step=" + show(k));
                es.push_back({u, v}); lab.push_back(labels(n, es));}
            checkEqual(d.now, q, "now");
            for (int t = 0; t <= q; ++t) {
                checkState(n, lab[t], [&](int u) { return d.findSet(t, u); }, [&](int u) { return d.getSize(t, u); },
                           [&](int u, int v) { return d.isSameSet(t, u, v); }, d.count(t), "t=" + show(t));}
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) {
                    int want = -1;
                    for (int t = 0; t <= q && want < 0; ++t) { if (lab[t][u] == lab[t][v]) { want = t; } }
                    checkEqual(d.uniteTime(u, v), want, "uniteTime u=" + show(u) + " v=" + show(v));}}}}

    // Persistent: random version trees; each version checked against BFS labels of its ancestor edges.
    void persistentSmall(Rng &rng, int rounds, int maxN, int maxQ) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 1, maxN)), q = int(rnd(rng, 0, maxQ));
            context = "persistent round=" + show(s) + " n=" + show(n) + " q=" + show(q);
            PersistentDSU d(n);
            vector<vector<pair<int, int>>> es(1);
            for (int k = 0; k < q; ++k) {
                int t = rng() % 3 ? int(es.size()) - 1 : int(rnd(rng, 0, lng(es.size()) - 1));
                int u = int(rnd(rng, 0, n - 1)), v = rng() % 6 ? int(rnd(rng, 0, n - 1)) : u;
                checkEqual(d.uniteSets(t, u, v), int(es.size()), "uniteSets version id");
                es.push_back(es[t]); es.back().push_back({u, v});}
            for (int t = 0; t < int(es.size()); ++t) {
                checkState(n, labels(n, es[t]), [&](int u) { return d.findSet(t, u); }, [&](int u) { return d.getSize(t, u); },
                           [&](int u, int v) { return d.isSameSet(t, u, v); }, d.count(t), "version=" + show(t));}}}

    // Large partially persistent: an offline time sweep with a path-compressed DSU (row 01) answers sampled queries,
    // and each uniteTime answer is confirmed connected at its time and disconnected one step earlier.
    void partialLarge(Rng &rng, int n, int q, int samples) {
        context = "partial large n=" + show(n) + " q=" + show(q);
        PartiallyPersistentDSU d(n);
        vector<pair<int, int>> es(q);
        for (auto &[u, v] : es) { u = int(rnd(rng, 0, n - 1)); v = int(rnd(rng, 0, n - 1)); d.uniteSets(u, v); }
        struct Query { int t, u, v, kind, want; };
        vector<Query> qs;
        for (int k = 0; k < samples; ++k) {
            int t = int(rnd(rng, 0, q)), u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            qs.push_back({t, u, v, 0, int(d.isSameSet(t, u, v))});
            qs.push_back({t, u, v, 1, d.getSize(t, u)});
            qs.push_back({t, u, v, 2, d.count(t)});
            int w = d.uniteTime(u, v);
            if (w >= 0) { qs.push_back({w, u, v, 0, 1}); }
            qs.push_back({w > 0 ? w - 1 : q, u, v, 0, w == 0 ? 1 : 0});}
        std::sort(qs.begin(), qs.end(), [](const Query &a, const Query &b) { return a.t < b.t; });
        DSU ref(n);
        size_t j = 0;
        for (int t = 0; t <= q; ++t) {
            if (t) { ref.uniteSets(es[t - 1].first, es[t - 1].second); }
            for (; j < qs.size() && qs[j].t == t; ++j) {
                auto &x = qs[j];
                int got = x.kind == 0 ? int(ref.isSameSet(x.u, x.v)) : x.kind == 1 ? ref.getSize(x.u) : ref.count();
                checkEqual(x.want, got, "sweep kind=" + show(x.kind) + " t=" + show(t) + " u=" + show(x.u) + " v=" + show(x.v));}}
        int depthBound = int(std::bit_width(uint(n))) - 1, maxDepth = 0;
        for (int u = 0; u < n; ++u) {
            int depth = 0;
            for (int x = u; d.tim[x] != INT_MAX; x = d.par[x]) { ++depth; }
            maxDepth = max(maxDepth, depth);}
        checkEqual(maxDepth <= depthBound, true, "depth <= log2(n): " + show(maxDepth));}

    // Large persistent: sampled versions rebuilt from their ancestor edges with a path-compressed DSU (row 01).
    void persistentLarge(Rng &rng, int n, int q, int sampled) {
        context = "persistent large n=" + show(n) + " q=" + show(q);
        PersistentDSU d(n);
        vector<int> parent{-1};
        vector<pair<int, int>> edge{{0, 0}};
        for (int k = 0; k < q; ++k) {
            int t = rng() % 2 ? int(parent.size()) - 1 : int(rnd(rng, 0, lng(parent.size()) - 1));
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            d.uniteSets(t, u, v); parent.push_back(t); edge.push_back({u, v});}
        for (int s = 0; s < sampled; ++s) {
            int t = s == 0 ? int(parent.size()) - 1 : int(rnd(rng, 0, lng(parent.size()) - 1));
            DSU ref(n);
            for (int w = t; w > 0; w = parent[w]) { ref.uniteSets(edge[w].first, edge[w].second); }
            checkEqual(d.count(t), ref.count(), "count version=" + show(t));
            for (int k = 0; k < 300; ++k) {
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                checkEqual(d.isSameSet(t, u, v), ref.isSameSet(u, v), "isSameSet version=" + show(t));
                checkEqual(d.getSize(t, u), ref.getSize(u), "getSize version=" + show(t));}}}

    void edges() {
        context = "edges";
        PartiallyPersistentDSU e;
        checkEqual(e.count(0), 0, "empty partial"); checkEqual(e.now, 0, "empty now");
        PersistentDSU f;
        checkEqual(f.count(0), 0, "empty persistent");
        PartiallyPersistentDSU d(4);
        checkEqual(d.uniteSets(2, 2), false, "self loop"); checkEqual(d.now, 1, "self loop advances time");
        d.uniteSets(0, 1); d.uniteSets(2, 3); d.uniteSets(1, 3);
        checkEqual(d.uniteTime(0, 3), 4, "uniteTime"); checkEqual(d.uniteTime(2, 2), 0, "uniteTime same vertex");
        checkEqual(d.getSize(3, 0), 2, "size at time 3"); checkEqual(d.getSize(4, 0), 4, "size at time 4"); checkEqual(d.count(1), 4, "count at time 1");
        PersistentDSU p(3);
        int v1 = p.uniteSets(0, 0, 1), v2 = p.uniteSets(0, 1, 2), v3 = p.uniteSets(v1, 1, 0);
        checkEqual(p.isSameSet(v1, 0, 1), true, "branch 1"); checkEqual(p.isSameSet(v2, 0, 1), false, "branch 2");
        checkEqual(p.count(v3), 2, "redundant version"); checkEqual(p.findSet(v3, 0), p.findSet(v1, 0), "redundant shares state");
        PersistentDSU deep(1024);
        int dt = 0;
        for (int w = 1; w < 1024; w *= 2) { for (int i = 0; i < 1024; i += 2 * w) { dt = deep.uniteSets(dt, i, i + w); } }
        for (int u = 0; u < 1024; ++u) { checkEqual(deep.findSet(dt, u), 0, "depth-10 tree root u=" + show(u)); }
        checkEqual(deep.getSize(dt, 1023), 1024, "depth-10 tree size");
        PersistentDSU big(1000000000);
        int b1 = big.uniteSets(0, 999999999, 0);
        checkEqual(big.getSize(b1, 0), 2, "huge n"); checkEqual(big.count(b1), 999999999, "huge n count");}

    void invalid(const string &name) {
        PartiallyPersistentDSU d(3);
        PersistentDSU p(3);
        if (name == "partial-time-future") { (void)d.findSet(1, 0); }
        else if (name == "partial-time-negative") { (void)d.findSet(-1, 0); }
        else if (name == "partial-vertex") { (void)d.findSet(0, 3); }
        else if (name == "partial-count") { (void)d.count(1); }
        else if (name == "partial-unitetime") { (void)d.uniteTime(0, 3); }
        else if (name == "partial-size-negative") { PartiallyPersistentDSU bad(-1); }
        else if (name == "persistent-version") { (void)p.findSet(1, 0); }
        else if (name == "persistent-vertex") { (void)p.findSet(0, 3); }
        else if (name == "persistent-unite-version") { p.uniteSets(-1, 0, 1); }
        else if (name == "persistent-count") { (void)p.count(1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
            else if (arg == "--invalid" && i + 1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "persistentdsu seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int rounds = mode == "quick" ? 40 : mode == "full" ? 400 : 3000;
        partialSmall(rng, rounds, 10, 25); persistentSmall(rng, rounds, 10, 30);
        partialSmall(rng, rounds / 20, 40, 120); persistentSmall(rng, rounds / 20, 40, 120);
        if (mode != "quick") { partialLarge(rng, 100000, 200000, 20000); persistentLarge(rng, 100000, 200000, 40); persistentLarge(rng, 200000, 200000, 20); }
        if (mode == "stress") { partialLarge(rng, 150000, 300000, 30000); persistentLarge(rng, 120000, 240000, 30); }
        std::cout << "PASS persistentdsu checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL persistentdsu seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
