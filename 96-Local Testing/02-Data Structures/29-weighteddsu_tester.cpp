#include "../../02-Data Structures/29-weighteddsu.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

namespace {
    ulng seed = 20261023;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, size_t K> string show(const array<T, K> &a) {
        string s = "<";
        for (size_t i = 0; i < K; ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + ">";}
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    // S5 under composition, a applied first: noncommutative, so any orientation slip changes a diff.
    struct Perm {
        using S = array<int, 5>;
        static S op(const S &a, const S &b) { S c{}; for (int i = 0; i < 5; ++i) { c[i] = b[a[i]]; } return c; }
        static S e() { return {0, 1, 2, 3, 4}; }
        static S inv(const S &a) { S c{}; for (int i = 0; i < 5; ++i) { c[a[i]] = i; } return c; }
    };
    // Runtime state: Z / mod with a member modulus.
    struct ModAdd {
        lng mod;
        using S = lng;
        S op(S a, S b) const { return (a + b) % mod; }
        S e() const { return 0; }
        S inv(S a) const { return (mod - a) % mod; }
    };
    lng value(Rng &rng, const Sum<lng> &) { return rnd(rng, -1000000000, 1000000000); }
    bool value(Rng &rng, const Xor<bool> &) { return rng() & 1; }
    Perm::S value(Rng &rng, const Perm &) { Perm::S a = Perm::e(); std::shuffle(a.begin(), a.end(), rng); return a; }
    lng value(Rng &rng, const ModAdd &m) { return rnd(rng, 0, m.mod - 1); }

    // Accepted relations as a graph: edge a -> b labelled w means x_b = x_a w; contradictions are events at their first vertex.
    template<typename G>
    struct Brute {
        using S = typename G::S;
        G m;
        int n;
        vector<vector<pair<int, S>>> adj;
        vector<int> events;
        Brute(int N, G g) : m(g), n(N), adj(N) {}
        // seen[v] and d[v] = x_u^-1 x_v over accepted relations, by BFS.
        pair<vector<char>, vector<S>> reach(int u) const {
            vector<char> seen(n, 0);
            vector<S> d(n, m.e());
            vector<int> q{u};
            seen[u] = 1;
            for (size_t i = 0; i < q.size(); ++i) {
                int a = q[i];
                for (auto &[b, w] : adj[a]) {
                    if (!seen[b]) { seen[b] = 1; d[b] = m.op(d[a], w); q.push_back(b); }}}
            return {seen, d};}
        bool unite(int u, int v, const S &w) {
            auto [seen, d] = reach(u);
            if (seen[v]) {
                if (!(d[v] == w)) { events.push_back(u); }
                return false;}
            adj[u].push_back({v, w}); adj[v].push_back({u, m.inv(w)});
            return true;}
    };

    // Full state of a weighted DSU against the brute force; findSet must be one member of the whole component.
    template<typename D, typename G>
    void checkWeighted(D &d, const Brute<G> &b) {
        using S = typename G::S;
        int n = b.n, ncomp = 0;
        vector<int> comp(n, -1);
        vector<vector<int>> groups;
        for (int u = 0; u < n; ++u) {
            auto [seen, dist] = b.reach(u);
            if (comp[u] < 0) {
                groups.emplace_back();
                for (int v = 0; v < n; ++v) {
                    if (seen[v]) { comp[v] = ncomp; groups.back().push_back(v); }}
                ++ncomp;}
            int r = d.findSet(u);
            checkEqual(bool(seen[r]), true, "findSet(" + show(u) + ") in component");
            int size = 0, bad = 0;
            for (int v = 0; v < n; ++v) {
                size += seen[v];
                checkEqual(d.isSameSet(u, v), bool(seen[v]), "isSameSet(" + show(u) + "," + show(v) + ")");
                if (seen[v]) {
                    checkEqual(d.findSet(v), r, "findSet(" + show(v) + ") same root");
                    checkEqual(S(d.diff(u, v)), S(dist[v]), "diff(" + show(u) + "," + show(v) + ")");}}
            for (int e : b.events) { bad += seen[e]; }
            auto [rs, rd] = b.reach(r);
            checkEqual(S(d.potential(u)), S(rd[u]), "potential(" + show(u) + ")");
            checkEqual(d.getSize(u), size, "getSize(" + show(u) + ")");
            checkEqual(d.contradictions(u), bad, "contradictions(" + show(u) + ")");}
        checkEqual(d.count(), ncomp, "count");
        checkEqual(d.groups(), groups, "groups");}

    template<typename G>
    void weightedHistories(Rng &rng, int rounds, int maxN, int steps, G g, const string &name) {
        using S = typename G::S;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN));
            bool every = round % 2 == 0;
            context = name + " n=" + show(n) + " round=" + show(round) + " seed=" + show(seed);
            WeightedDSU<G> d(n, g);
            Brute<G> b(n, g);
            checkWeighted(d, b);
            if (!n) { continue; }
            vector<S> x(n);
            for (int u = 0; u < n; ++u) { x[u] = value(rng, g); }
            for (int step = 0; step < steps; ++step) {
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                S w = rng() % 4 ? g.op(g.inv(x[u]), x[v]) : value(rng, g);
                context = name + " n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                checkEqual(d.uniteSets(u, v, w), b.unite(u, v, w), "uniteSets(" + show(u) + "," + show(v) + "," + show(w) + ")");
                if (every || step % 16 == 15) { checkWeighted(d, b); }}
            checkWeighted(d, b);}
        std::cout << "PASS WeightedDSU<" << name << ">\n";}

    // Every relation applied; (vertex, parity) states reachable from (u, 0) by BFS.
    struct ParityBrute {
        int n;
        vector<vector<pair<int, int>>> adj;
        explicit ParityBrute(int N) : n(N), adj(N) {}
        vector<array<char, 2>> reach(int u) const {
            vector<array<char, 2>> seen(n, {0, 0});
            vector<pair<int, int>> q{{u, 0}};
            seen[u][0] = 1;
            for (size_t i = 0; i < q.size(); ++i) {
                auto [a, p] = q[i];
                for (auto [b, w] : adj[a]) {
                    if (!seen[b][p ^ w]) { seen[b][p ^ w] = 1; q.push_back({b, p ^ w}); }}}
            return seen;}
        int parity(int u, int v) const { auto s = reach(u); return s[v][0] ? 0 : s[v][1] ? 1 : -1; }
        bool bipartite(int u) const { return !reach(u)[u][1]; }
        bool unite(int u, int v, int w) {
            bool res = parity(u, v) < 0;
            adj[u].push_back({v, w}); adj[v].push_back({u, w});
            return res;}
    };

    void parityHistories(Rng &rng, int rounds, int maxN, int steps) {
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN));
            ParityDSU d(n);
            ParityBrute b(n);
            vector<int> color(n);
            for (int &c : color) { c = int(rng() & 1); }
            for (int step = 0; step <= steps && n; ++step) {
                context = "ParityDSU n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                for (int u = 0; u < n; ++u) {
                    checkEqual(d.isBipartite(u), b.bipartite(u), "isBipartite(" + show(u) + ")");
                    for (int v = 0; v < n; ++v) { checkEqual(d.parity(u, v), b.parity(u, v), "parity(" + show(u) + "," + show(v) + ")"); }}
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1)), w = rng() % 6 ? color[u] ^ color[v] : int(rng() & 1);
                checkEqual(d.uniteSets(u, v, w), b.unite(u, v, w), "uniteSets(" + show(u) + "," + show(v) + "," + show(w) + ")");}}
        std::cout << "PASS ParityDSU\n";}

    // Rollback: the brute force is rebuilt from the live records after every operation.
    template<typename G>
    void rollbackHistories(Rng &rng, int rounds, int maxN, int steps, G g, const string &name) {
        using S = typename G::S;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 1, maxN)), token = 0;
            RollbackWeightedDSU<G> d(n, g);
            vector<tuple<int, int, S>> rec;
            vector<S> x(n);
            for (int u = 0; u < n; ++u) { x[u] = value(rng, g); }
            for (int step = 0; step < steps; ++step) {
                context = name + " rollback n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                int kind = int(rng() % 10);
                if (kind < 6) {
                    int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                    S w = rng() % 4 ? g.op(g.inv(x[u]), x[v]) : value(rng, g);
                    Brute<G> b(n, g);
                    for (auto &[a, c, y] : rec) { b.unite(a, c, y); }
                    checkEqual(d.uniteSets(u, v, w), b.unite(u, v, w), "uniteSets(" + show(u) + "," + show(v) + "," + show(w) + ")");
                    rec.push_back({u, v, w});}
                else if (kind == 6 && !rec.empty()) { d.undo(); rec.pop_back(); }
                else if (kind == 7) {
                    int t = int(rnd(rng, 0, int(rec.size())));
                    d.rollback(t); rec.resize(t);}
                else if (kind == 8) { token = d.snapshot(); }
                else if (token <= int(rec.size())) { d.rollback(token); rec.resize(token); }
                Brute<G> b(n, g);
                for (auto &[a, c, y] : rec) { b.unite(a, c, y); }
                checkEqual(d.snapshot(), int(rec.size()), "snapshot");
                checkWeighted(d, b);}}
        std::cout << "PASS RollbackWeightedDSU<" << name << ">\n";}

    int depthOf(const RollbackDSU &d, int u) { int k = 0; for (; d.par[u] != u; u = d.par[u]) { ++k; } return k; }

    // Large: hidden potentials x, wrong relations only inside a component; row 01's DSU tracks components and counts.
    void large(Rng &rng, int n, int q) {
        context = "large WeightedDSU n=" + show(n) + " seed=" + show(seed);
        WeightedDSU<Sum<lng>> d(n);
        DSU c(n);
        vector<lng> x(n);
        vector<int> bad(n, 0);
        for (lng &y : x) { y = rnd(rng, -1000000000, 1000000000); }
        for (int step = 0; step < q; ++step) {
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            bool same = c.isSameSet(u, v), wrong = same && rng() % 8 == 0;
            lng w = x[v] - x[u] + (wrong ? rnd(rng, 1, 5) : 0);
            if (wrong) { ++bad[c.findSet(u)]; }
            else if (!same) { int s = bad[c.findSet(u)] + bad[c.findSet(v)]; c.uniteSets(u, v); bad[c.findSet(u)] = s; }
            checkEqual(d.uniteSets(u, v, w), !same, "uniteSets step=" + show(step));
            if (step % 1024 == 0) {
                checkEqual(d.count(), c.count(), "count");
                for (int k = 0; k < 64; ++k) {
                    int a = int(rnd(rng, 0, n - 1)), b = rng() % 2 ? int(rnd(rng, 0, n - 1)) : c.findSet(a);
                    checkEqual(d.isSameSet(a, b), c.isSameSet(a, b), "isSameSet");
                    if (c.isSameSet(a, b)) { checkEqual(d.diff(a, b), x[b] - x[a], "diff"); }
                    checkEqual(d.getSize(a), c.getSize(a), "getSize");
                    checkEqual(d.contradictions(a), bad[c.findSet(a)], "contradictions");}}}

        context = "large RollbackWeightedDSU n=" + show(n) + " seed=" + show(seed);
        RollbackWeightedDSU<Sum<lng>> r(n);
        RollbackDSU rc(n);
        vector<tuple<int, int, lng>> rec;
        int maxDepth = 0, bound = n > 1 ? int(std::bit_width(uint(n))) - 1 : 0;
        for (int step = 0; step < q; ++step) {
            if (rng() % 64 == 0 && !rec.empty()) {
                int t = int(rnd(rng, 0, int(rec.size())));
                r.rollback(t); rc.rollback(t); rec.resize(t);}
            else {
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                lng w = x[v] - x[u] + (rc.isSameSet(u, v) && rng() % 16 == 0 ? 1 : 0);
                checkEqual(r.uniteSets(u, v, w), rc.uniteSets(u, v), "uniteSets step=" + show(step));
                rec.push_back({u, v, w});}
            if (step % 8192 == 0 || step == q - 1) {
                // Replay the live records: wrong relations lie inside components, so x stays a valid potential.
                DSU e(n);
                vector<int> cnt(n, 0);
                for (auto &[u, v, w] : rec) {
                    if (e.isSameSet(u, v)) { cnt[e.findSet(u)] += w != x[v] - x[u]; continue; }
                    int s = cnt[e.findSet(u)] + cnt[e.findSet(v)];
                    e.uniteSets(u, v); cnt[e.findSet(u)] = s;}
                checkEqual(r.count(), e.count(), "count");
                checkEqual(r.snapshot(), int(rec.size()), "snapshot");
                for (int k = 0; k < 2000; ++k) {
                    int a = int(rnd(rng, 0, n - 1)), b = rng() % 2 ? int(rnd(rng, 0, n - 1)) : e.findSet(a);
                    checkEqual(r.isSameSet(a, b), e.isSameSet(a, b), "isSameSet");
                    if (e.isSameSet(a, b)) { checkEqual(r.diff(a, b), x[b] - x[a], "diff"); checkEqual(r.potential(b) - r.potential(a), x[b] - x[a], "potential"); }
                    checkEqual(r.getSize(a), e.getSize(a), "getSize");
                    checkEqual(r.contradictions(a), cnt[e.findSet(a)], "contradictions");
                    maxDepth = max(maxDepth, depthOf(r, a));}}}
        checkEqual(maxDepth <= bound, true, "depth <= floor(log2(n))");

        context = "large ParityDSU n=" + show(n) + " seed=" + show(seed);
        ParityDSU p(n);
        DSU pc(n);
        vector<int> color(n);
        vector<char> odd(n, 0);
        for (int &y : color) { y = int(rng() & 1); }
        for (int step = 0; step < q; ++step) {
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            bool same = pc.isSameSet(u, v), wrong = same && rng() % 64 == 0;
            int w = color[u] ^ color[v] ^ int(wrong);
            if (wrong) { odd[pc.findSet(u)] = 1; }
            else if (!same) { char o = odd[pc.findSet(u)] | odd[pc.findSet(v)]; pc.uniteSets(u, v); odd[pc.findSet(u)] = o; }
            checkEqual(p.uniteSets(u, v, w), !same, "uniteSets step=" + show(step));
            if (step % 1024 == 0) {
                for (int k = 0; k < 64; ++k) {
                    int a = int(rnd(rng, 0, n - 1)), b = int(rnd(rng, 0, n - 1));
                    bool bip = !odd[pc.findSet(a)];
                    checkEqual(p.isBipartite(a), bip, "isBipartite");
                    checkEqual(p.parity(a, b), !pc.isSameSet(a, b) ? -1 : bip ? color[a] ^ color[b] : 0, "parity");}}}}

    void edges() {
        context = "edges";
        WeightedDSU<Sum<lng>> z;
        checkEqual(z.count(), 0, "empty count"); checkEqual(z.groups(), vector<vector<int>>{}, "empty groups");
        RollbackWeightedDSU<Sum<lng>> rz;
        checkEqual(rz.count(), 0, "empty rollback count"); checkEqual(rz.snapshot(), 0, "empty snapshot");
        ParityDSU pz;
        checkEqual(pz.n, 0, "empty parity");
        WeightedDSU<Sum<lng>> d(3);
        checkEqual(d.uniteSets(1, 1, 0), false, "self relation e"); checkEqual(d.contradictions(1), 0, "self relation consistent");
        checkEqual(d.uniteSets(1, 1, 4), false, "self relation w"); checkEqual(d.contradictions(1), 1, "self relation contradicts");
        checkEqual(d.uniteSets(0, 1, 5), true, "merge"); checkEqual(d.diff(0, 1), lng(5), "diff"); checkEqual(d.diff(1, 0), lng(-5), "reverse diff");
        checkEqual(d.contradictions(0), 1, "merged counts");
        WeightedDSU<Sum<lng>> copy = d;
        copy.uniteSets(1, 2, 7);
        checkEqual(d.isSameSet(1, 2), false, "copy independent"); checkEqual(copy.diff(0, 2), lng(12), "copy diff");
        ParityDSU p(2);
        checkEqual(p.uniteSets(0, 0, false), false, "even self-loop"); checkEqual(p.isBipartite(0), true, "even self-loop keeps bipartite");
        checkEqual(p.uniteSets(0, 0, true), false, "odd self-loop"); checkEqual(p.isBipartite(0), false, "odd self-loop");
        checkEqual(p.parity(0, 0), 0, "non-bipartite parity is 0"); checkEqual(p.parity(0, 1), -1, "unrelated"); checkEqual(p.isBipartite(1), true, "other vertex");
        RollbackWeightedDSU<Perm> r(3);
        Perm::S a{1, 0, 2, 3, 4}, b{0, 2, 1, 3, 4};
        r.uniteSets(0, 1, a); r.uniteSets(1, 2, b);
        checkEqual(r.diff(0, 2), Perm::op(a, b), "noncommutative composition"); checkEqual(r.diff(2, 0), Perm::inv(Perm::op(a, b)), "inverse");
        r.uniteSets(0, 2, Perm::op(b, a));
        checkEqual(r.contradictions(0), 1, "swapped order contradicts");
        r.undo();
        checkEqual(r.contradictions(0), 0, "undo restores count");
        for (lng mod : {lng(5), lng(1000000007)}) {
            WeightedDSU<ModAdd> md(4, ModAdd{mod});
            md.uniteSets(0, 1, mod - 1); md.uniteSets(1, 2, 3 % mod);
            checkEqual(md.diff(0, 2), (mod + 2) % mod, "stateful diff"); checkEqual(md.diff(2, 0), (mod - 2) % mod, "stateful inverse");}
        struct Layout { int n, ncon; vector<int> par, siz, bad; vector<lng> pot; };
        checkEqual(sizeof(WeightedDSU<Sum<lng>>), sizeof(Layout), "empty group adds no size");
        std::cout << "PASS edges\n";}

    void invalid(const string &name) {
        WeightedDSU<Sum<lng>> d(3);
        ParityDSU p(3);
        RollbackWeightedDSU<Sum<lng>> r(3);
        if (name == "find-range") { (void)d.findSet(3); }
        else if (name == "find-negative") { (void)d.findSet(-1); }
        else if (name == "unite-range") { d.uniteSets(0, 3, 0); }
        else if (name == "size-negative") { WeightedDSU<Sum<lng>> bad(-1); }
        else if (name == "diff-apart") { (void)d.diff(0, 1); }
        else if (name == "parity-size") { ParityDSU bad(INT_MAX / 2 + 1); }
        else if (name == "parity-size-negative") { ParityDSU bad(-1); }
        else if (name == "parity-range") { (void)p.parity(0, 3); }
        else if (name == "bipartite-range") { (void)p.isBipartite(-1); }
        else if (name == "rollback-undo-empty") { r.undo(); }
        else if (name == "rollback-future") { r.rollback(1); }
        else if (name == "rollback-diff-apart") { (void)r.diff(0, 1); }
        else if (name == "rollback-potential-range") { (void)r.potential(3); }
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
        std::cout << "weighteddsu seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 1000;
        lng mod = std::array<lng, 3>{7, 1000000007, 998244353}[seed % 3];
        weightedHistories(rng, rounds, 12, 60, Sum<lng>{}, "Sum<lng>");
        weightedHistories(rng, rounds, 12, 60, Xor<bool>{}, "Xor<bool>");
        weightedHistories(rng, rounds, 12, 60, Perm{}, "Perm");
        weightedHistories(rng, rounds / 2, 12, 60, ModAdd{mod}, "ModAdd");
        weightedHistories(rng, rounds / 10, 40, 150, Perm{}, "Perm n<=40");
        parityHistories(rng, rounds, 10, 40);
        rollbackHistories(rng, rounds, 10, 50, Sum<lng>{}, "Sum<lng>");
        rollbackHistories(rng, rounds, 10, 50, Perm{}, "Perm");
        rollbackHistories(rng, rounds / 2, 10, 50, Xor<bool>{}, "Xor<bool>");
        if (mode != "quick") { large(rng, 200000, 400000); large(rng, 1 << 16, 300000); }
        if (mode == "stress") { large(rng, 1000000, 3000000); }
        std::cout << "PASS weighteddsu checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL weighteddsu seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
