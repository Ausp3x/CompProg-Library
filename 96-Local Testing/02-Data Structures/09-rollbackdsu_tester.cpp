#include "../../02-Data Structures/09-rollbackdsu.hpp"
#include "../../02-Data Structures/01-dsu.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 1000000007;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    // Commutative monoid: (sum mod P, max, count of maxima).
    struct SumMax {
        struct S {
            lng sum, mx, cnt;
            bool operator==(const S &o) const { return sum == o.sum && mx == o.mx && cnt == o.cnt; }
            friend std::ostream &operator<<(std::ostream &os, const S &s) { return os << "{" << s.sum << "," << s.mx << "," << s.cnt << "}"; }
        };
        static S op(const S &a, const S &b) { return {(a.sum + b.sum) % P, max(a.mx, b.mx), a.mx == b.mx ? a.cnt + b.cnt : a.mx > b.mx ? a.cnt : b.cnt}; }
        static S e() { return {0, LLONG_MIN, 0}; }
    };
    struct Or {
        using S = bool;
        static S op(S a, S b) { return a || b; }
        static S e() { return false; }
    };
    SumMax::S value(Rng &rng, SumMax *) { lng x = rnd(rng, -5, 5); return {(x + P) % P, x, 1}; }
    bool value(Rng &rng, Or *) { return rng() % 9 == 0; }

    // Recorded operation: kind 0 unite(u, v), kind 1 addValue(u, x).
    template<typename S> struct Op { int kind, u, v; S x; };

    // Brute force from the record list: components by BFS over unite records, values and edge counts per component.
    template<typename M>
    struct Brute {
        int n, ncomp = 0;
        vector<int> comp;
        vector<typename M::S> value;
        vector<int> size, edges;
        Brute(int N, const vector<typename M::S> &init, const vector<Op<typename M::S>> &rec) : n(N), comp(N, -1) {
            vector<vector<int>> adj(n);
            for (auto &o : rec) { if (o.kind == 0) { adj[o.u].push_back(o.v); adj[o.v].push_back(o.u); } }
            for (int s = 0; s < n; ++s) {
                if (comp[s] >= 0) { continue; }
                vector<int> q{s};
                comp[s] = ncomp;
                for (size_t i = 0; i < q.size(); ++i) { for (int y : adj[q[i]]) { if (comp[y] < 0) { comp[y] = ncomp; q.push_back(y); } } }
                ++ncomp;}
            value.assign(ncomp, M::e()); size.assign(ncomp, 0); edges.assign(ncomp, 0);
            for (int u = 0; u < n; ++u) { value[comp[u]] = M::op(value[comp[u]], init[u]); ++size[comp[u]]; }
            for (auto &o : rec) {
                if (o.kind == 0) { ++edges[comp[o.u]]; }
                else { value[comp[o.u]] = M::op(value[comp[o.u]], o.x); }}}
        vector<vector<int>> groups() const {
            vector<vector<int>> res;
            vector<int> id(ncomp, -1);
            for (int u = 0; u < n; ++u) {
                if (id[comp[u]] < 0) { id[comp[u]] = int(res.size()); res.emplace_back(); }
                res[id[comp[u]]].push_back(u);}
            return res;}
    };

    // Full state check of the base operations against the brute force (findSet is a canonical member of the component).
    template<typename D, typename B>
    void checkBase(const D &d, const B &b, int records) {
        checkEqual(d.count(), b.ncomp, "count");
        checkEqual(d.snapshot(), records, "snapshot");
        checkEqual(d.groups(), b.groups(), "groups");
        vector<int> rep(b.ncomp, -1);
        for (int u = 0; u < b.n; ++u) {
            int r = d.findSet(u);
            checkEqual(b.comp[r], b.comp[u], "findSet member u=" + show(u));
            if (rep[b.comp[u]] < 0) { rep[b.comp[u]] = r; }
            checkEqual(r, rep[b.comp[u]], "findSet canonical u=" + show(u));
            checkEqual(d.getSize(u), b.size[b.comp[u]], "getSize u=" + show(u));}
        for (int k = 0; k < 2 * b.n; ++k) {
            int u = k % b.n, v = (k * 7 + 3) % b.n;
            checkEqual(d.isSameSet(u, v), b.comp[u] == b.comp[v], "isSameSet u=" + show(u) + " v=" + show(v));}}

    // Random unite / undo / rollback histories with every state checked; tokens are taken from snapshot().
    void baseHistories(Rng &rng, int rounds, int maxN, int steps) {
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 1, maxN));
            RollbackDSU d(n);
            vector<Op<int>> rec;
            vector<int> tokens{0};
            for (int k = 0; k < steps; ++k) {
                context = "base round=" + show(s) + " n=" + show(n) + " step=" + show(k);
                int kind = int(rng() % 6);
                if (kind <= 2 || rec.empty()) {
                    int u = int(rnd(rng, 0, n - 1)), v = rng() % 8 ? int(rnd(rng, 0, n - 1)) : u;
                    bool sep = !d.isSameSet(u, v);
                    checkEqual(d.uniteSets(u, v), sep, "uniteSets return");
                    rec.push_back({0, u, v, 0});}
                else if (kind == 3) { d.undo(); rec.pop_back(); }
                else if (kind == 4) { tokens.push_back(d.snapshot()); }
                else {
                    int t = tokens[size_t(rnd(rng, 0, lng(tokens.size()) - 1))];
                    if (t > d.snapshot()) { t = d.snapshot(); }
                    d.rollback(t); rec.resize(size_t(t));}
                vector<SumMax::S> init(n, SumMax::e());
                vector<Op<SumMax::S>> r2;
                for (auto &o : rec) { r2.push_back({0, o.u, o.v, SumMax::e()}); }
                checkBase(d, Brute<SumMax>(n, init, r2), int(rec.size()));}}}

    // Aggregate histories: unite, addValue, undo, rollback; values, edge counts and base operations against the brute force.
    template<typename M>
    void aggregateHistories(Rng &rng, int rounds, int maxN, int steps, const string &name) {
        using S = typename M::S;
        for (int s = 0; s < rounds; ++s) {
            int n = int(rnd(rng, 1, maxN));
            vector<S> init(n);
            for (auto &&x : init) { x = value(rng, (M *)nullptr); }
            RollbackDSUAggregate<M> d(init);
            vector<Op<S>> rec;
            vector<int> tokens{0};
            for (int k = 0; k < steps; ++k) {
                context = name + " aggregate round=" + show(s) + " n=" + show(n) + " step=" + show(k);
                int kind = int(rng() % 7);
                if (kind <= 2 || rec.empty()) {
                    int u = int(rnd(rng, 0, n - 1)), v = rng() % 8 ? int(rnd(rng, 0, n - 1)) : u;
                    bool sep = !d.isSameSet(u, v);
                    checkEqual(d.uniteSets(u, v), sep, "uniteSets return");
                    rec.push_back({0, u, v, M::e()});}
                else if (kind == 3) {
                    int u = int(rnd(rng, 0, n - 1));
                    S x = value(rng, (M *)nullptr);
                    d.addValue(u, x); rec.push_back({1, u, u, x});}
                else if (kind == 4) { d.undo(); rec.pop_back(); }
                else if (kind == 5) { tokens.push_back(d.snapshot()); }
                else {
                    int t = tokens[size_t(rnd(rng, 0, lng(tokens.size()) - 1))];
                    if (t > d.snapshot()) { t = d.snapshot(); }
                    d.rollback(t); rec.resize(size_t(t));}
                Brute<M> b(n, init, rec);
                checkBase(d, b, int(rec.size()));
                for (int u = 0; u < n; ++u) {
                    checkEqual(S(d.componentValue(u)), S(b.value[b.comp[u]]), "componentValue u=" + show(u));
                    checkEqual(d.edgeCount(u), b.edges[b.comp[u]], "edgeCount u=" + show(u));}}
            d.rollback(0);
            for (int u = 0; u < n; ++u) { checkEqual(S(d.componentValue(u)), S(init[u]), "rollback(0) value"); checkEqual(d.edgeCount(u), 0, "rollback(0) edges"); }}}

    // Large: offline-deletion-like add/undo pairs; checkpoints rebuild a path-compressed DSU (row 01) from the live edges.
    void large(Rng &rng, int n, int steps, int checkpoints) {
        context = "large n=" + show(n);
        RollbackDSU d(n);
        vector<pair<int, int>> live;
        int depthBound = int(std::bit_width(uint(n))) - 1;
        for (int k = 0; k < steps; ++k) {
            if (live.empty() || rng() % 3) {
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                d.uniteSets(u, v); live.push_back({u, v});}
            else {
                int cnt = int(rnd(rng, 1, min<lng>(lng(live.size()), 50)));
                d.rollback(d.snapshot() - cnt); live.resize(live.size() - size_t(cnt));}
            if ((k + 1) % (steps / checkpoints) == 0) {
                context = "large n=" + show(n) + " step=" + show(k);
                DSU ref(n);
                for (auto [u, v] : live) { ref.uniteSets(u, v); }
                checkEqual(d.count(), ref.count(), "count");
                checkEqual(d.snapshot(), int(live.size()), "snapshot");
                int maxDepth = 0;
                for (int u = 0; u < n; ++u) {
                    int depth = 0;
                    for (int x = u; d.par[x] != x; x = d.par[x]) { ++depth; }
                    maxDepth = max(maxDepth, depth);
                    if (u % 97 == 0) { checkEqual(d.getSize(u), ref.getSize(u), "getSize u=" + show(u)); }}
                checkEqual(maxDepth <= depthBound, true, "depth <= log2(n): " + show(maxDepth));
                for (int j = 0; j < 2000; ++j) {
                    int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                    checkEqual(d.isSameSet(u, v), ref.isSameSet(u, v), "isSameSet");}}}}

    void edges() {
        context = "edges";
        RollbackDSU e;
        checkEqual(e.count(), 0, "empty count"); checkEqual(e.groups().size(), size_t(0), "empty groups"); checkEqual(e.snapshot(), 0, "empty snapshot");
        e.rollback(0);
        RollbackDSU d(3);
        checkEqual(d.uniteSets(1, 1), false, "self loop"); checkEqual(d.snapshot(), 1, "self loop recorded");
        checkEqual(d.uniteSets(0, 2), true, "merge"); checkEqual(d.uniteSets(2, 0), false, "redundant");
        checkEqual(d.groups(), vector<vector<int>>{{0, 2}, {1}}, "groups order");
        d.undo(); d.undo();
        checkEqual(d.count(), 3, "undo merge"); checkEqual(d.snapshot(), 1, "undo pops records");
        RollbackDSU copy = d;
        copy.uniteSets(0, 1);
        checkEqual(d.count(), 3, "copy independent");
        RollbackDSUAggregate<Or> o(vector<bool>{false, true, false});
        o.uniteSets(0, 2);
        checkEqual(bool(o.componentValue(2)), false, "or before");
        o.addValue(0, true);
        checkEqual(bool(o.componentValue(2)), true, "or after addValue"); checkEqual(o.edgeCount(0), 1, "edge count");
        o.uniteSets(2, 0);
        checkEqual(o.edgeCount(2), 2, "redundant edge counted");
        o.rollback(1);
        checkEqual(bool(o.componentValue(0)), false, "addValue undone"); checkEqual(o.edgeCount(0), 1, "edges after rollback");
        // Runtime state: component sums modulo a member modulus, through unions, addValue and rollback.
        struct ModSum { lng mod; using S = lng; S op(S x, S y) const { return (x + y) % mod; } S e() const { return 0; } };
        for (lng mod : {lng(5), lng(1000000007)}) {
            RollbackDSUAggregate md(vector<lng>{1, 2, 3, 4}, ModSum{mod});
            md.uniteSets(0, 1); md.uniteSets(2, 3); md.addValue(3, mod - 1);
            checkEqual(md.componentValue(1), 3 % mod, "stateful pair sum"); checkEqual(md.componentValue(2), (7 + mod - 1) % mod, "stateful addValue");
            int t = md.snapshot();
            md.uniteSets(1, 2);
            checkEqual(md.componentValue(0), (10 + mod - 1) % mod, "stateful merge");
            md.rollback(t);
            checkEqual(md.componentValue(0), 3 % mod, "stateful rollback");}}

    void invalid(const string &name) {
        RollbackDSU d(3);
        RollbackDSUAggregate<Or> o(vector<bool>{false, true});
        if (name == "find-range") { (void)d.findSet(3); }
        else if (name == "find-negative") { (void)d.findSet(-1); }
        else if (name == "unite-range") { d.uniteSets(0, 3); }
        else if (name == "undo-empty") { d.undo(); }
        else if (name == "rollback-future") { d.uniteSets(0, 1); d.rollback(2); }
        else if (name == "rollback-negative") { d.rollback(-1); }
        else if (name == "size-negative") { RollbackDSU bad(-1); }
        else if (name == "aggregate-undo-empty") { o.undo(); }
        else if (name == "aggregate-rollback-future") { o.rollback(1); }
        else if (name == "aggregate-add-range") { o.addValue(2, true); }
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
        std::cout << "rollbackdsu seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int rounds = mode == "quick" ? 40 : mode == "full" ? 400 : 3000;
        baseHistories(rng, rounds, 12, 60);
        aggregateHistories<SumMax>(rng, rounds, 12, 60, "summax"); aggregateHistories<Or>(rng, rounds / 4, 10, 60, "or");
        baseHistories(rng, rounds / 10, 60, 200);
        if (mode != "quick") { large(rng, 200000, 400000, 20); large(rng, 1 << 16, 300000, 20); }
        if (mode == "stress") { large(rng, 1000000, 3000000, 30); }
        std::cout << "PASS rollbackdsu checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL rollbackdsu seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
