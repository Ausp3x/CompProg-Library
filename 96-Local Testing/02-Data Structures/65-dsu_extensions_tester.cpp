#include "../../02-Data Structures/65-dsu_extensions.hpp"
#include "../../02-Data Structures/00-monoids.hpp"
#include "../../02-Data Structures/10-persistentdsu.hpp"

namespace {
    ulng seed = 20261023;
    string mode = "full", context;
    lng checks = 0;
    constexpr lng P = 1000000007;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, size_t K> string show(const array<T, K> &a) {
        string s = "<";
        for (size_t i = 0; i < K; ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + ">";}
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    // Components of an edge list by BFS: comp[u] and members ascending.
    struct Components {
        vector<int> comp;
        vector<vector<int>> sets;
        Components(int n, const vector<pair<int, int>> &edges) : comp(n, -1) {
            vector<vector<int>> adj(n);
            for (auto [u, v] : edges) { adj[u].push_back(v); adj[v].push_back(u); }
            for (int s = 0; s < n; ++s) {
                if (comp[s] >= 0) { continue; }
                vector<int> q{s};
                comp[s] = int(sets.size());
                for (size_t i = 0; i < q.size(); ++i) {
                    for (int y : adj[q[i]]) {
                        if (comp[y] < 0) { comp[y] = comp[s]; q.push_back(y); }}}
                sort(q.begin(), q.end());
                sets.push_back(q);}}
    };

    void linkedHistories(Rng &rng, int rounds, int maxN, int steps) {
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN));
            LinkedDSU d(n);
            vector<pair<int, int>> edges;
            for (int step = 0; step <= steps; ++step) {
                context = "LinkedDSU n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                Components c(n, edges);
                checkEqual(d.count(), int(c.sets.size()), "count");
                checkEqual(d.groups(), c.sets, "groups");
                for (int u = 0; u < n; ++u) {
                    vector<int> m = d.members(u), sorted = m;
                    sort(sorted.begin(), sorted.end());
                    checkEqual(m[0], u, "members(" + show(u) + ") starts at u");
                    checkEqual(sorted, c.sets[c.comp[u]], "members(" + show(u) + ")");
                    if (m.size() > 1) {
                        vector<int> rot(m.begin() + 1, m.end());
                        rot.push_back(u);
                        checkEqual(d.members(m[1]), rot, "members follow one cycle");}}
                int kind = int(rng() % 8);
                if (kind == 0 && n < maxN + 3) {
                    checkEqual(d.makeSet(), n, "makeSet");
                    ++n;
                    checkEqual(d.members(n - 1), vector<int>{n - 1}, "new singleton");
                    continue;}
                if (!n) { continue; }
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                bool apart = c.comp[u] != c.comp[v];
                if (kind < 4) {
                    vector<pair<int, int>> got, want;
                    for (int x : c.sets[c.comp[u]]) {
                        for (int y : c.sets[c.comp[v]]) { want.push_back({x, y}); }}
                    if (!apart) { want.clear(); }
                    checkEqual(d.unionEnumerate(u, v, [&](int x, int y) { got.push_back({x, y}); }), apart, "unionEnumerate return");
                    sort(got.begin(), got.end());
                    checkEqual(got, want, "unionEnumerate(" + show(u) + "," + show(v) + ") pairs");}
                else { checkEqual(d.uniteSets(u, v), apart, "uniteSets(" + show(u) + "," + show(v) + ")"); }
                edges.push_back({u, v});}}
        std::cout << "PASS LinkedDSU\n";}

    // Commutative monoids: (sum mod P, max, count of maxima), row 00's Or<bool> and a stateful modular sum.
    struct SumMax {
        struct S {
            lng sum, mx, cnt;
            bool operator==(const S &o) const { return sum == o.sum && mx == o.mx && cnt == o.cnt; }
            friend std::ostream &operator<<(std::ostream &os, const S &s) { return os << "{" << s.sum << "," << s.mx << "," << s.cnt << "}"; }
        };
        static S op(const S &a, const S &b) { return {(a.sum + b.sum) % P, max(a.mx, b.mx), a.mx == b.mx ? a.cnt + b.cnt : a.mx > b.mx ? a.cnt : b.cnt}; }
        static S e() { return {0, LLONG_MIN, 0}; }
    };
    struct ModSum {
        lng mod;
        using S = lng;
        S op(S a, S b) const { return (a + b) % mod; }
        S e() const { return 0; }
    };
    SumMax::S value(Rng &rng, const SumMax &) { lng x = rnd(rng, -5, 5); return {(x + P) % P, x, 1}; }
    bool value(Rng &rng, const Or<bool> &) { return rng() % 9 == 0; }
    lng value(Rng &rng, const ModSum &m) { return rnd(rng, 0, m.mod - 1); }

    template<typename M>
    void sumHistories(Rng &rng, int rounds, int maxN, int steps, M mon, const string &name) {
        using S = typename M::S;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN));
            vector<S> own(n);
            for (int u = 0; u < n; ++u) { own[u] = value(rng, mon); }
            ComponentSumDSU<M> d(own, mon);
            vector<pair<int, int>> edges;
            for (int step = 0; step <= steps; ++step) {
                context = "ComponentSumDSU<" + name + "> n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                Components c(n, edges);
                for (int u = 0; u < n; ++u) {
                    S want = mon.e();
                    for (int v : c.sets[c.comp[u]]) { want = mon.op(want, own[v]); }
                    checkEqual(S(d.componentValue(u)), want, "componentValue(" + show(u) + ")");
                    checkEqual(d.getSize(u), int(c.sets[c.comp[u]].size()), "getSize");}
                int kind = int(rng() % 8);
                if (kind == 0 && n < maxN + 3) {
                    S x = value(rng, mon);
                    checkEqual(d.makeSet(x), n, "makeSet");
                    own.push_back(x); ++n;
                    continue;}
                if (!n) { continue; }
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                // Folding into the component equals folding into u's own value, as the monoid commutes.
                if (kind < 3) { S x = value(rng, mon); d.addValue(u, x); own[u] = mon.op(own[u], x); }
                else { checkEqual(d.uniteSets(u, v), c.comp[u] != c.comp[v], "uniteSets(" + show(u) + "," + show(v) + ")"); edges.push_back({u, v}); }}}
        std::cout << "PASS ComponentSumDSU<" << name << ">\n";}

    struct Perm {
        using S = array<int, 5>;
        static S op(const S &a, const S &b) { S c{}; for (int i = 0; i < 5; ++i) { c[i] = b[a[i]]; } return c; }
        static S e() { return {0, 1, 2, 3, 4}; }
        static S inv(const S &a) { S c{}; for (int i = 0; i < 5; ++i) { c[a[i]] = i; } return c; }
    };
    struct ModAdd {
        lng mod;
        using S = lng;
        S op(S a, S b) const { return (a + b) % mod; }
        S e() const { return 0; }
        S inv(S a) const { return (mod - a) % mod; }
    };
    lng value(Rng &rng, const Sum<lng> &) { return rnd(rng, -1000000000, 1000000000); }
    Perm::S value(Rng &rng, const Perm &) { Perm::S a = Perm::e(); std::shuffle(a.begin(), a.end(), rng); return a; }
    lng value(Rng &rng, const ModAdd &m) { return rnd(rng, 0, m.mod - 1); }
    bool value(Rng &rng, const Xor<bool> &) { return rng() & 1; }

    // Brute force: explicit values, apply multiplies every member on the right.
    template<typename G>
    void dualHistories(Rng &rng, int rounds, int maxN, int steps, G g, const string &name) {
        using S = typename G::S;
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN));
            vector<S> val(n);
            for (int u = 0; u < n; ++u) { val[u] = value(rng, g); }
            DualDSU<G> d(val, g);
            vector<pair<int, int>> edges;
            bool every = round % 2 == 0;
            for (int step = 0; step <= steps; ++step) {
                context = "DualDSU<" + name + "> n=" + show(n) + " round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                Components c(n, edges);
                if (every || step == steps) {
                    for (int u = 0; u < n; ++u) {
                        checkEqual(S(d.get(u)), S(val[u]), "get(" + show(u) + ")");
                        int r = d.findSet(u);
                        checkEqual(c.comp[r], c.comp[u], "findSet(" + show(u) + ") in component");
                        for (int k = 0; k < (n <= 12 ? n : 4); ++k) {
                            int v = n <= 12 ? k : int(rnd(rng, 0, n - 1));
                            checkEqual(d.isSameSet(u, v), c.comp[u] == c.comp[v], "isSameSet");}}}
                if (!n) { continue; }
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                if (rng() % 2) {
                    S x = value(rng, g);
                    d.apply(u, x);
                    for (int w : c.sets[c.comp[u]]) { val[w] = g.op(val[w], x); }}
                else { checkEqual(d.uniteSets(u, v), c.comp[u] != c.comp[v], "uniteSets"); edges.push_back({u, v}); }}}
        std::cout << "PASS DualDSU<" << name << ">\n";}

    template<typename K>
    void sparseHistories(Rng &rng, int rounds, int steps, auto key, const string &name) {
        for (int round = 0; round < rounds; ++round) {
            int pool = int(rnd(rng, 1, 14));
            vector<K> keys;
            for (int i = 0; i < pool; ++i) { keys.push_back(key(rng)); }
            sort(keys.begin(), keys.end());
            keys.erase(unique(keys.begin(), keys.end()), keys.end());
            pool = int(keys.size());
            SparseDSU<K> d;
            vector<pair<int, int>> edges;
            for (int step = 0; step <= steps; ++step) {
                context = "SparseDSU<" + name + "> round=" + show(round) + " step=" + show(step) + " seed=" + show(seed);
                Components c(pool, edges);
                vector<char> touched(pool, 0);
                for (auto [a, b] : edges) { touched[a] = touched[b] = 1; }
                for (int i = 0; i < pool; ++i) {
                    K r = d.findSet(keys[i]);
                    int j = int(lower_bound(keys.begin(), keys.end(), r) - keys.begin());
                    checkEqual(j < pool && keys[j] == r && c.comp[j] == c.comp[i], true, "findSet in component");
                    if (!touched[i]) { checkEqual(r == keys[i], true, "untouched key is its own root"); }
                    checkEqual(d.getSize(keys[i]), int(c.sets[c.comp[i]].size()), "getSize");
                    for (int k = 0; k < pool; ++k) { checkEqual(d.isSameSet(keys[i], keys[k]), c.comp[i] == c.comp[k], "isSameSet"); }}
                checkEqual(int(d.key.size()), int(std::count(touched.begin(), touched.end(), 1)), "only united keys allocate");
                int a = int(rnd(rng, 0, pool - 1)), b = int(rnd(rng, 0, pool - 1));
                checkEqual(d.uniteSets(keys[a], keys[b]), c.comp[a] != c.comp[b], "uniteSets");
                edges.push_back({a, b});}}
        std::cout << "PASS SparseDSU<" << name << ">\n";}

    // Brute force per time t: components of the first t unions.
    void forestCases(Rng &rng, int rounds, int maxN, int maxQ) {
        for (int round = 0; round < rounds; ++round) {
            int n = int(rnd(rng, 0, maxN)), q = n ? int(rnd(rng, 0, maxQ)) : 0;
            vector<pair<int, int>> unions(q);
            for (auto &[u, v] : unions) { u = int(rnd(rng, 0, n - 1)); v = int(rnd(rng, 0, n - 1)); }
            context = "MergeHistoryForest n=" + show(n) + " q=" + show(q) + " round=" + show(round) + " seed=" + show(seed);
            MergeHistoryForest f(n, unions);
            vector<Components> at;
            for (int t = 0; t <= q; ++t) { at.emplace_back(n, vector<pair<int, int>>(unions.begin(), unions.begin() + t)); }
            // Node ids: leaves, then one node per merging union in order.
            vector<int> nodeTimes(n, 0);
            vector<vector<int>> nodeSets;
            for (int u = 0; u < n; ++u) { nodeSets.push_back({u}); }
            for (int i = 0; i < q; ++i) {
                auto [u, v] = unions[i];
                if (at[i].comp[u] != at[i].comp[v]) { nodeTimes.push_back(i + 1); nodeSets.push_back(at[i + 1].sets[at[i + 1].comp[u]]); }}
            int m = int(nodeTimes.size());
            checkEqual(int(f.tim.size()), m, "node count");
            vector<int> perm = f.leaves;
            sort(perm.begin(), perm.end());
            vector<int> id(n);
            iota(id.begin(), id.end(), 0);
            checkEqual(perm, id, "leaves is a permutation");
            for (int x = 0; x < m; ++x) {
                checkEqual(f.nodeTime(x), nodeTimes[x], "nodeTime(" + show(x) + ")");
                auto [l, r] = f.leafRange(x);
                vector<int> got(f.leaves.begin() + l, f.leaves.begin() + r);
                sort(got.begin(), got.end());
                checkEqual(got, nodeSets[x], "leafRange(" + show(x) + ")");
                int want = -1, u = nodeSets[x][0];
                for (int t = nodeTimes[x] + 1; t <= q && want < 0; ++t) {
                    if (at[t].sets[at[t].comp[u]].size() > nodeSets[x].size()) { want = t; }}
                checkEqual(f.mergeTime(x), want, "mergeTime(" + show(x) + ")");}
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) {
                    int want = -1;
                    for (int t = 0; t <= q && want < 0; ++t) {
                        if (at[t].comp[u] == at[t].comp[v]) { want = t; }}
                    checkEqual(f.earliestConnectTime(u, v), want, "earliestConnectTime(" + show(u) + "," + show(v) + ")");}
                for (int t : {0, q / 2, q, q + 1, INT_MAX}) {
                    int x = f.componentAt(u, t);
                    const Components &c = at[min(t, q)];
                    auto [l, r] = f.leafRange(x);
                    vector<int> got(f.leaves.begin() + l, f.leaves.begin() + r);
                    sort(got.begin(), got.end());
                    checkEqual(got, c.sets[c.comp[u]], "componentAt(" + show(u) + "," + show(t) + ")");
                    checkEqual(f.nodeTime(x) <= t && (f.mergeTime(x) < 0 || t < f.mergeTime(x)), true, "componentAt lifetime");}}}
        std::cout << "PASS MergeHistoryForest\n";}

    // Forest queries: row 10's uniteTime, and componentAt sorted by time against row 01's DSU replaying the unions.
    void checkForest(Rng &rng, int n, const vector<pair<int, int>> &unions, int queries) {
        int q = int(unions.size());
        PartiallyPersistentDSU p(n);
        for (auto [u, v] : unions) { p.uniteSets(u, v); }
        MergeHistoryForest f(n, unions);
        for (int k = 0; k < queries; ++k) {
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            checkEqual(f.earliestConnectTime(u, v), p.uniteTime(u, v), "earliestConnectTime");}
        vector<pair<int, int>> at(queries);
        for (auto &[t, u] : at) { t = int(rnd(rng, 0, q)); u = int(rnd(rng, 0, n - 1)); }
        sort(at.begin(), at.end());
        DSU replay(n);
        int done = 0;
        for (auto [t, u] : at) {
            for (; done < t; ++done) { replay.uniteSets(unions[done].first, unions[done].second); }
            auto [lo, hi] = f.leafRange(f.componentAt(u, t));
            checkEqual(hi - lo, replay.getSize(u), "componentAt size");
            checkEqual(replay.isSameSet(u, f.leaves[int(rnd(rng, lo, hi - 1))]), true, "componentAt member");}}

    // Deep and shallow merge trees: chains in both orders, a star, a balanced tree, each with redundant unions mixed in.
    void forestShapes(Rng &rng, int n) {
        for (int shape = 0; shape < 4; ++shape) {
            context = "MergeHistoryForest shape=" + show(shape) + " n=" + show(n) + " seed=" + show(seed);
            vector<pair<int, int>> unions;
            if (shape == 0) { for (int i = 0; i + 1 < n; ++i) { unions.push_back({i, i + 1}); } }
            if (shape == 1) { for (int i = n - 1; i > 0; --i) { unions.push_back({i, i - 1}); } }
            if (shape == 2) { for (int i = 1; i < n; ++i) { unions.push_back({i, 0}); } }
            if (shape == 3) { for (int w = 1; w < n; w *= 2) { for (int i = 0; i + w < n; i += 2 * w) { unions.push_back({i, i + w}); } } }
            for (int k = 0; k < n / 4; ++k) {
                int u = int(rnd(rng, 0, n - 1));
                unions.insert(unions.begin() + rnd(rng, 0, int(unions.size())), {u, u});}
            checkForest(rng, n, unions, 4000);}
        std::cout << "PASS MergeHistoryForest shapes n=" << n << '\n';}

    void large(Rng &rng, int n, int q) {
        context = "large LinkedDSU/ComponentSumDSU n=" + show(n) + " seed=" + show(seed);
        LinkedDSU l(n);
        vector<lng> own(n);
        for (lng &x : own) { x = rnd(rng, 0, 1000000); }
        ComponentSumDSU<Sum<lng>> s(own);
        DSU c(n);
        for (int step = 0; step < q; ++step) {
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            if (rng() % 4 == 0) { lng x = rnd(rng, 0, 1000000); s.addValue(u, x); own[u] += x; continue; }
            bool apart = !c.isSameSet(u, v);
            c.uniteSets(u, v);
            checkEqual(l.uniteSets(u, v), apart, "LinkedDSU uniteSets"); checkEqual(s.uniteSets(u, v), apart, "ComponentSumDSU uniteSets");
            if (step % max(1, q / 16) == 0 || step == q - 1) {
                vector<lng> sum(n, 0);
                vector<int> mark(n, -1);
                for (int x = 0; x < n; ++x) { sum[c.findSet(x)] += own[x]; }
                for (int k = 0; k < 64; ++k) {
                    int a = int(rnd(rng, 0, n - 1));
                    checkEqual(s.componentValue(a), sum[c.findSet(a)], "componentValue");
                    if (k >= 4) { continue; }
                    // Distinct members of a's component, as many as its size: exactly the component.
                    vector<int> m = l.members(a);
                    int good = 0;
                    for (int x : m) { good += mark[x] != k && c.isSameSet(x, a); mark[x] = k; }
                    checkEqual(pair(int(m.size()), good), pair(c.getSize(a), c.getSize(a)), "members");}}}

        std::cout << "PASS large LinkedDSU/ComponentSumDSU n=" << n << '\n';
        context = "large MergeHistoryForest n=" + show(n) + " seed=" + show(seed);
        vector<pair<int, int>> unions(q);
        for (auto &[u, v] : unions) { u = int(rnd(rng, 0, n - 1)); v = int(rnd(rng, 0, n - 1)); }
        checkForest(rng, n, unions, 20000);
        std::cout << "PASS large MergeHistoryForest n=" << n << '\n';
        context = "large SparseDSU n=" + show(n) + " seed=" + show(seed);
        vector<lng> pool(n);
        for (lng &x : pool) { x = lng(rng()); }
        sort(pool.begin(), pool.end());
        pool.erase(unique(pool.begin(), pool.end()), pool.end());
        int m = int(pool.size());
        SparseDSU<lng> sp;
        DSU e(m);
        std::unordered_map<lng, int> index;
        for (int i = 0; i < m; ++i) { index[pool[i]] = i; }
        for (int step = 0; step < q; ++step) {
            int a = int(rnd(rng, 0, m - 1)), b = int(rnd(rng, 0, m - 1));
            checkEqual(sp.uniteSets(pool[a], pool[b]), e.uniteSets(a, b), "uniteSets");
            if (step % 64 == 0) {
                int i = int(rnd(rng, 0, m - 1));
                checkEqual(e.isSameSet(index.at(sp.findSet(pool[i])), i), true, "findSet");
                checkEqual(sp.getSize(pool[i]), e.getSize(i), "getSize");}}
        std::cout << "PASS large SparseDSU n=" << n << '\n';}

    // DualDSU at moderate size: explicit member lists as the brute force.
    void dualMedium(Rng &rng, int n, int q) {
        context = "medium DualDSU n=" + show(n) + " seed=" + show(seed);
        vector<lng> val(n);
        for (lng &x : val) { x = rnd(rng, -1000, 1000); }
        DualDSU<Sum<lng>> d(val);
        DSU c(n);
        vector<vector<int>> mem(n);
        for (int u = 0; u < n; ++u) { mem[u] = {u}; }
        for (int step = 0; step < q; ++step) {
            int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
            if (rng() % 2) {
                lng x = rnd(rng, -1000, 1000);
                d.apply(u, x);
                for (int w : mem[c.findSet(u)]) { val[w] += x; }}
            else {
                int a = c.findSet(u), b = c.findSet(v);
                checkEqual(d.uniteSets(u, v), a != b, "uniteSets");
                if (a != b) {
                    c.uniteSets(a, b);
                    int r = c.findSet(a), o = r == a ? b : a;
                    mem[r].insert(mem[r].end(), mem[o].begin(), mem[o].end()); mem[o].clear();}}
            int w = int(rnd(rng, 0, n - 1));
            checkEqual(d.get(w), val[w], "get");}
        std::cout << "PASS medium DualDSU n=" << n << '\n';}

    void edges() {
        context = "edges";
        LinkedDSU z;
        checkEqual(z.makeSet(), 0, "makeSet on empty"); checkEqual(z.members(0), vector<int>{0}, "singleton");
        ComponentSumDSU<Or<bool>> o(vector<bool>{false, true});
        o.uniteSets(0, 0);
        checkEqual(bool(o.componentValue(0)), false, "self union keeps value");
        o.uniteSets(1, 0);
        checkEqual(bool(o.componentValue(0)), true, "or merged");
        for (lng mod : {lng(5), lng(1000000007)}) {
            ComponentSumDSU<ModSum> ms(vector<lng>{1, 2, 3, 4}, ModSum{mod});
            ms.uniteSets(0, 1); ms.addValue(1, mod - 1);
            checkEqual(ms.componentValue(0), (3 + mod - 1) % mod, "stateful sum");
            DualDSU<ModAdd> md(vector<lng>{0, 1, 2}, ModAdd{mod});
            md.apply(0, mod - 1); md.uniteSets(0, 1); md.apply(1, 2 % mod);
            checkEqual(md.get(0), (mod - 1 + 2) % mod, "stateful dual get"); checkEqual(md.get(1), (1 + 2) % mod, "stateful dual merged"); checkEqual(md.get(2), 2 % mod, "untouched");}
        DualDSU<Perm> dp(vector<Perm::S>(2, Perm::e()));
        Perm::S a{1, 0, 2, 3, 4}, b{0, 2, 1, 3, 4};
        dp.apply(0, a); dp.uniteSets(1, 0); dp.apply(0, b);
        checkEqual(dp.get(0), Perm::op(a, b), "noncommutative order"); checkEqual(dp.get(1), b, "merged after apply");
        SparseDSU<string> ss;
        checkEqual(ss.findSet("x"), string("x"), "unknown key"); checkEqual(ss.getSize("x"), 1, "unknown size"); checkEqual(int(ss.key.size()), 0, "queries allocate nothing");
        ss.uniteSets("a", "b");
        checkEqual(ss.isSameSet("b", "a"), true, "string keys");
        SparseDSU<pair<int, int>> sq;
        sq.uniteSets({1, 2}, {INT_MIN, INT_MAX});
        checkEqual(sq.getSize({INT_MIN, INT_MAX}), 2, "pair keys");
        MergeHistoryForest e0(0, {});
        checkEqual(int(e0.tim.size()), 0, "empty forest");
        MergeHistoryForest e1(1, {{0, 0}, {0, 0}});
        checkEqual(e1.earliestConnectTime(0, 0), 0, "self"); checkEqual(e1.mergeTime(0), -1, "never merged"); checkEqual(e1.componentAt(0, 5), 0, "leaf only");
        std::cout << "PASS edges\n";}

    void invalid(const string &name) {
        LinkedDSU l(3);
        ComponentSumDSU<Sum<lng>> s(vector<lng>(3, 0));
        DualDSU<Sum<lng>> d(vector<lng>(3, 0));
        MergeHistoryForest f(3, {{0, 1}});
        if (name == "linked-members-range") { (void)l.members(3); }
        else if (name == "linked-unite-range") { l.uniteSets(0, 3); }
        else if (name == "sum-unite-range") { s.uniteSets(0, 3); }
        else if (name == "sum-add-negative") { s.addValue(-1, 1); }
        else if (name == "dual-find-range") { (void)d.findSet(3); }
        else if (name == "dual-apply-range") { d.apply(3, 1); }
        else if (name == "forest-size") { MergeHistoryForest bad(-1, {}); }
        else if (name == "forest-size-large") { MergeHistoryForest bad((1 << 30) + 1, {}); }
        else if (name == "forest-union-range") { MergeHistoryForest bad(3, {{0, 3}}); }
        else if (name == "forest-connect-range") { (void)f.earliestConnectTime(0, 3); }
        else if (name == "forest-time-negative") { (void)f.componentAt(0, -1); }
        else if (name == "forest-node-range") { (void)f.leafRange(4); }
        else if (name == "forest-node-time-range") { (void)f.nodeTime(-1); }
        else if (name == "forest-merge-time-range") { (void)f.mergeTime(4); }
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
        std::cout << "dsu_extensions seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int rounds = mode == "quick" ? 30 : mode == "full" ? 300 : 2000;
        lng mod = std::array<lng, 3>{7, 1000000007, 998244353}[seed % 3];
        linkedHistories(rng, rounds, 10, 40);
        sumHistories(rng, rounds, 10, 40, SumMax{}, "SumMax");
        sumHistories(rng, rounds / 2, 10, 40, Or<bool>{}, "Or<bool>");
        sumHistories(rng, rounds / 2, 10, 40, ModSum{mod}, "ModSum");
        dualHistories(rng, rounds, 10, 60, Sum<lng>{}, "Sum<lng>");
        dualHistories(rng, rounds, 10, 60, Perm{}, "Perm");
        dualHistories(rng, rounds / 2, 10, 60, Xor<bool>{}, "Xor<bool>");
        dualHistories(rng, rounds / 2, 10, 60, ModAdd{mod}, "ModAdd");
        dualHistories(rng, rounds / 10, 60, 300, Perm{}, "Perm n<=60");
        sparseHistories<lng>(rng, rounds, 30, [](Rng &r) { return lng(r() % 2 ? r() : r() % 8); }, "lng");
        sparseHistories<string>(rng, rounds / 2, 30, [](Rng &r) { return string(1 + r() % 3, char('a' + r() % 3)); }, "string");
        sparseHistories<pair<int, int>>(rng, rounds / 2, 30, [](Rng &r) { return pair<int, int>(int(r() % 3) - 1, int(r() % 4)); }, "pair");
        forestCases(rng, rounds, 9, 16);
        forestCases(rng, rounds / 10, 30, 80);
        forestShapes(rng, mode == "quick" ? 300 : 4096);
        if (mode != "quick") { large(rng, 200000, 400000); large(rng, 1 << 16, 300000); dualMedium(rng, 3000, 30000); }
        if (mode == "stress") { large(rng, 1000000, 3000000); dualMedium(rng, 5000, 100000); }
        std::cout << "PASS dsu_extensions checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL dsu_extensions seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
