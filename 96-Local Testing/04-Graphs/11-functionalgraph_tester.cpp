#include "../../04-Graphs/11-functionalgraph.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string show(const vector<int> &next) {
        std::ostringstream out; out << "next=[";
        for (int v : next) { out << v << ','; } out << ']'; return out.str();}
    struct Orbit {
        vector<int> path;
        int tail, length, entry;
    };
    Orbit orbit(const vector<int> &next, int u) {
        vector<int> seen(next.size(), -1); vector<int> path;
        while (u != -1 && seen[u] == -1) { seen[u] = int(path.size()); path.push_back(u); u = next[u]; }
        int tail = u == -1 ? int(path.size()) - 1 : seen[u];
        return {path, tail, u == -1 ? 0 : int(path.size()) - tail, path[tail]};}
    int jump(const Orbit &r, ulng k) {
        if (k < r.path.size()) { return r.path[size_t(k)]; }
        return r.length ? r.path[r.tail + int((k - r.tail) % r.length)] : -1;}
    int distance(const Orbit &r, int v) {
        auto it = std::find(r.path.begin(), r.path.end(), v);
        return it == r.path.end() ? -1 : int(it - r.path.begin());}
    int meeting(const vector<int> &next, int u, int v) {
        int n = int(next.size()), t = 0; vector<uint8_t> seen(n * n);
        while (u != -1 && v != -1) {
            if (u == v) { return t; }
            if (seen[u * n + v]) { return -1; }
            seen[u * n + v] = true; u = next[u]; v = next[v]; ++t;}
        return -1;}
    struct Sum {
        lll operator()(lll a, lll b) const { return a + b; }
    };
    struct Concat {
        string operator()(const string &a, const string &b) const { return a + b; }
    };
    using Matrix = array<lng, 4>;
    struct Multiply {
        Matrix operator()(const Matrix &a, const Matrix &b) const {
            constexpr lng MOD = 1000000007;
            return {(a[0] * b[0] + a[1] * b[2]) % MOD,
                    (a[0] * b[1] + a[1] * b[3]) % MOD,
                    (a[2] * b[0] + a[3] * b[2]) % MOD,
                    (a[2] * b[1] + a[3] * b[3]) % MOD};}
    };
    const Matrix IDENTITY{1, 0, 0, 1};
    template<class T>
    struct RefResult { int next; ulng count; T value; };
    template<class T, class Op>
    RefResult<T> bruteFold(const vector<int> &next, const vector<T> &values, int u, ulng count, T id, Op op) {
        ulng used = 0;
        while (used < count && u != -1) { id = op(id, values[u]); u = next[u]; ++used; }
        return {u, used, id};}
    // Independent fixed 64-level reference, without graph decomposition or
    // cycle repetition. Each block carries its actual terminal-truncated size.
    template<class T, class Op>
    struct ReferenceFold {
        array<vector<int>, 64> next;
        array<vector<ulng>, 64> size;
        array<vector<T>, 64> value;
        T identity;
        Op op;
        ReferenceFold(const vector<int> &successor, const vector<T> &values, T identity, Op op)
            : identity(identity), op(op) {
            int n = int(successor.size()); next[0] = successor; value[0] = values; size[0].assign(n, 1);
            for (int j = 1; j < 64; ++j) {
                next[j].resize(n); size[j].resize(n); value[j].resize(n);
                for (int u = 0; u < n; ++u) {
                    int v = next[j - 1][u];
                    next[j][u] = v == -1 ? -1 : next[j - 1][v];
                    size[j][u] = size[j - 1][u] + (v == -1 ? 0 : size[j - 1][v]);
                    value[j][u] = v == -1 ? value[j - 1][u] : op(value[j - 1][u], value[j - 1][v]);}}}
        RefResult<T> fold(int u, ulng count) const {
            RefResult<T> out{u, 0, identity};
            for (int j = 0; j < 64 && out.next != -1; ++j) {
                if ((count >> j) & 1) {
                    int v = out.next; out.value = op(out.value, value[j][v]);
                    out.count += size[j][v]; out.next = next[j][v];}}
            return out;}
    };
    vector<vector<int>> cycles(const vector<Orbit> &orbits) {
        set<vector<int>> unique;
        for (const auto &r : orbits) {
            if (!r.length) { continue; }
            vector<int> cycle(r.path.begin() + r.tail, r.path.end());
            std::rotate(cycle.begin(), std::min_element(cycle.begin(), cycle.end()), cycle.end()); unique.insert(cycle);}
        return {unique.begin(), unique.end()};}
    vector<ulng> counts(int n) {
        return {0, 1, 2, ulng(n), ulng(n + 1), ulng(2 * n + 3),
                (1ULL << 63) - 1, 1ULL << 63, std::numeric_limits<ulng>::max() - 1,
                std::numeric_limits<ulng>::max()};}
    void graphResult(const FunctionalGraph &g, const vector<int> &next, const vector<Orbit> &orbits) {
        int n = int(next.size()); auto expected_cycles = cycles(orbits);
        check(g.n == n && g.successor == next, "owning successor snapshot");
        check(g.cycles == expected_cycles, "all canonical cycles in minimum-vertex order");
        check(int(g.component.size()) == n && int(g.cycle.size()) == n && int(g.entry.size()) == n
            && int(g.depth.size()) == n && int(g.position.size()) == n, "decomposition field lengths");
        check(int(g.up.size()) == max(1, int(std::bit_width(uint(n)))), "compact successor table height");
        for (const auto &row : g.up) { check(int(row.size()) == n, "successor table width"); }
        vector<int> terminals;
        for (int u = 0; u < n; ++u) { if (next[u] == -1) { terminals.push_back(u); }}
        vector<int> tin = g.tin; sort(tin.begin(), tin.end());
        for (int u = 0; u < n; ++u) {
            const auto &r = orbits[u];
            check(g.entry[u] == r.entry && g.depth[u] == r.tail, "terminal/cycle entry and tail depth");
            check(tin[u] == u && g.tin[u] < g.tout[u] && g.tout[u] <= n, "reverse-forest Euler intervals");
            int cid = -1, position = -1;
            for (int c = 0; c < int(expected_cycles.size()); ++c) {
                if (std::find(expected_cycles[c].begin(), expected_cycles[c].end(), r.entry) != expected_cycles[c].end()) {
                    cid = c;
                    auto it = std::find(expected_cycles[c].begin(), expected_cycles[c].end(), u);
                    if (it != expected_cycles[c].end()) { position = int(it - expected_cycles[c].begin()); }}}
            check(g.cycle[u] == cid && g.position[u] == position, "cycle IDs and cycle-only positions");
            int component = cid == -1 ? int(expected_cycles.size()) + int(lower_bound(terminals.begin(), terminals.end(), r.entry) - terminals.begin()) : cid;
            check(g.component[u] == component, "cycle-first and terminal-ordered component IDs");
            for (int j = 0; j < int(g.up.size()); ++j) { check(g.up[j][u] == jump(r, 1ULL << j), "successor doubling table"); }
            for (ulng k : counts(n)) { check(g.jump(u, k) == jump(r, k), "jump u=" + std::to_string(u) + " k=" + std::to_string(k)); }
            auto floyd = functionalOrbit(next, u);
            check(floyd.entry == r.entry && floyd.tail == r.tail && floyd.cycle_length == r.length, "constant-space orbit versus visited sequence");
            for (int v = 0; v < n; ++v) {
                int d = distance(r, v);
                check(g.distance(u, v) == d && g.reachable(u, v) == (d != -1), "distance/reachable u=" + std::to_string(u) + " v=" + std::to_string(v));
                check(g.firstMeeting(u, v) == meeting(next, u, v), "synchronous pair-state meeting u=" + std::to_string(u) + " v=" + std::to_string(v));
                bool same = g.cycle[u] != -1 ? g.cycle[u] == g.cycle[v] : g.cycle[v] == -1 && r.entry == orbits[v].entry;
                check((g.component[u] == g.component[v]) == same, "weak-component partition");
                bool ancestor = r.entry == orbits[v].entry && r.tail >= orbits[v].tail && jump(r, r.tail - orbits[v].tail) == v;
                check((g.tin[v] <= g.tin[u] && g.tin[u] < g.tout[v]) == ancestor, "Euler intervals encode reverse-tree ancestry");}}
        vector<ulng> ks = counts(n);
        for (int k = 3; k <= 2 * n + 3; ++k) { ks.push_back(ulng(k)); }
        for (ulng k : ks) {
            auto all = g.jumpAll(k); check(int(all.size()) == n, "jumpAll size");
            for (int u = 0; u < n; ++u) { check(all[u] == jump(orbits[u], k), "jumpAll u=" + std::to_string(u) + " k=" + std::to_string(k)); }}}
    // Step-by-step walk; past cap steps with pred still true, every further cycle lap adds nothing.
    template<class T, class Op, class P>
    ulng bruteStep(const vector<int> &next, const vector<T> &values, int u, T acc, Op op, P pred, ulng limit, ulng cap) {
        ulng k = 0;
        while (k < limit && u != -1) {
            T value = op(acc, values[u]);
            if (!pred(value)) { break; }
            acc = value; ++k; u = next[u];
            if (k > cap) { return limit; }}
        return k;}
    void stepResult(const vector<int> &next, const vector<string> &words) {
        int n = int(next.size()); vector<lll> weights(n);
        for (int u = 0; u < n; ++u) { weights[u] = (7 * u + 3) % 3; }
        FunctionalGraph g(next); FunctionalGraphFold<string, Concat> text(g, words, string{}, Concat{});
        FunctionalGraphFold<lll, Sum> sum(g, weights, lll(0), Sum{});
        vector<ulng> limits{0, 1, 2, ulng(n), ulng(2 * n + 3), std::numeric_limits<ulng>::max()};
        for (int u = 0; u < n; ++u) {
            for (ulng limit : limits) {
                for (int target : {0, 1, 2, 5, n, 3 * n}) {
                    auto pred = [&](lll x) { return x <= target; };
                    ulng cap = ulng(n) * ulng(target + 2) + ulng(n);
                    check(sum.maxStep(u, pred, limit) == bruteStep(next, weights, u, lll(0), Sum{}, pred, limit, cap),
                          "maxStep sum u=" + std::to_string(u) + " limit=" + std::to_string(limit) + " target=" + std::to_string(target));}
                for (ulng count : {ulng(0), ulng(1), ulng(n), ulng(2 * n + 3)}) {
                    string goal = text.fold(u, count).value;
                    auto pred = [&](const string &x) { return goal.compare(0, x.size(), x) == 0 && x.size() <= goal.size(); };
                    ulng cap = ulng(n) * ulng(goal.size() + 2) + ulng(n);
                    check(text.maxStep(u, pred, limit) == bruteStep(next, words, u, string{}, Concat{}, pred, limit, cap),
                          "maxStep ordered prefix u=" + std::to_string(u) + " limit=" + std::to_string(limit));}}}}
    template<class T, class Op>
    void aggregateResult(const FunctionalGraph &g, const vector<int> &next, const vector<Orbit> &orbits,
                         const vector<T> &values, T id, Op op) {
        auto expected_cycles = cycles(orbits); vector<T> expected;
        for (const auto &cycle : expected_cycles) {
            T value = id; for (int u : cycle) { value = op(value, values[u]); } expected.push_back(value);}
        check(g.cycleAggregates(values, id, op) == expected, "cycle aggregates in canonical successor order");
        FunctionalGraphFold<T, Op> folded(g, values, id, op);
        for (int u = 0; u < g.n; ++u) {
            const auto &r = orbits[u]; T value = id;
            if (r.length) { for (int i = r.tail; i < int(r.path.size()); ++i) { value = op(value, values[r.path[i]]); }}
            auto a = g.cycleAggregate(u, values, id, op), b = folded.cycleAggregate(u);
            check(a.first == bool(r.length) && a.second == value && b == a, "cycle rotation starts at orbit entry");
            for (ulng count = 0; count <= ulng(2 * g.n + 3); ++count) {
                auto ref = bruteFold(next, values, u, count, id, op); auto actual = folded.fold(u, count);
                check(actual.next == ref.next && actual.count == ref.count && actual.value == ref.value,
                      "ordered explicit-walk fold u=" + std::to_string(u) + " count=" + std::to_string(count));
                if (count <= ulng(g.n) && (r.length || count <= ulng(r.tail + 1))) {
                    auto segment = folded.segment(u, int(count));
                    check(segment.next == ref.next && segment.count == ref.count && segment.value == ref.value,
                          "bounded segment u=" + std::to_string(u) + " count=" + std::to_string(count));}}}}
    void runCase(const vector<int> &next, bool matrix_huge = false) {
        ++cases; context = show(next); int n = int(next.size());
        vector<Orbit> orbits; for (int u = 0; u < n; ++u) { orbits.push_back(orbit(next, u)); }
        FunctionalGraph g(next); graphResult(g, next, orbits);
        Graph graph(n, true);
        for (int u = 0; u < n; ++u) { if (next[u] != -1) {
            graph.addEdge(u, next[u], u % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max());}}
        graphResult(FunctionalGraph(graph), next, orbits); graphResult(FunctionalGraph(CsrGraph(graph)), next, orbits);
        check(FunctionalGraph::successors(graph) == next && FunctionalGraph::successors(CsrGraph(graph)) == next, "explicit successor adapters");
        vector<string> words(n); vector<Matrix> matrices(n); vector<lll> weights(n);
        for (int u = 0; u < n; ++u) {
            words[u] = u % 3 ? "[" + std::to_string(u) + "]" : "";
            matrices[u] = {1 + u, 1, 1 + u % 2, 2 + u}; weights[u] = u % 2 ? -(u + 1) : u + 1;}
        aggregateResult(g, next, orbits, words, string{}, Concat{});
        aggregateResult(g, next, orbits, matrices, IDENTITY, Multiply{});
        if (matrix_huge || n <= 4) { stepResult(next, words); }
        FunctionalGraphFold<lll, Sum> sum(g, weights, lll(0), Sum{}); ReferenceFold<lll, Sum> ref(next, weights, 0, Sum{});
        for (int u = 0; u < n; ++u) { for (ulng count : counts(n)) {
            auto a = sum.fold(u, count); auto b = ref.fold(u, count);
            check(a.next == b.next && a.count == b.count && a.value == b.value,
                  "64-level exact sum fold u=" + std::to_string(u) + " count=" + std::to_string(count));}}
        if (matrix_huge) {
            FunctionalGraphFold<Matrix, Multiply> folded(g, matrices, IDENTITY, Multiply{});
            ReferenceFold<Matrix, Multiply> reference(next, matrices, IDENTITY, Multiply{});
            for (int u = 0; u < n; ++u) { for (ulng count : counts(n)) {
                auto a = folded.fold(u, count); auto b = reference.fold(u, count);
                check(a.next == b.next && a.count == b.count && a.value == b.value, "64-level noncommutative matrix fold");}}}
        check(g.successor == next, "all operations preserve source graph");}
    void exhaustive() {
        int bound = mode == "quick" ? 3 : mode == "full" ? 5 : 6;
        for (int n = 0; n <= bound; ++n) {
            int total = 1; for (int u = 0; u < n; ++u) { total *= n + 1; }
            for (int code = 0; code < total; ++code) {
                vector<int> next(n); int value = code;
                for (int &v : next) { v = value % (n + 1) - 1; value /= n + 1; }
                runCase(next);}}
        cout << "PASS exhaustive partial successor maps n<=" << bound << " cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed); int rounds = mode == "quick" ? 80 : mode == "full" ? 700 : 5000;
        for (int trial = 0; trial < rounds; ++trial) {
            int n = 1 + int(rng() % 13); vector<int> next(n);
            for (int &v : next) { v = int(rng() % (n + 1)) - 1; }
            if (trial % 3 == 0) {
                iota(next.begin(), next.end(), 0); std::shuffle(next.begin(), next.end(), rng);}
            runCase(next, true);}
        cout << "PASS random partial maps/permutations and huge noncommutative matrix folds rounds=" << rounds << '\n';}
    void boundaries() {
        FunctionalGraph empty; context = "default empty graph";
        check(empty.n == 0 && empty.successor.empty() && empty.cycles.empty(), "default constructor");
        int n = mode == "quick" ? 20000 : mode == "full" ? 200000 : 500000;
        vector<int> next(n); iota(next.begin(), next.end(), 1); next.back() = -1;
        {
            FunctionalGraph g(next); context = "terminating chain n=" + std::to_string(n);
            check(g.entry[0] == n - 1 && g.depth[0] == n - 1 && g.cycles.empty(), "large terminal decomposition");
            auto r = functionalOrbit(next, 0); check(r.entry == n - 1 && r.tail == n - 1 && !r.cycle_length, "large constant-space orbit");
            check(g.jump(0, n - 1) == n - 1 && g.jump(0, n) == -1 && g.jump(0, std::numeric_limits<ulng>::max()) == -1, "large terminal jumps");
            check(g.distance(0, n - 1) == n - 1 && g.distance(n - 1, 0) == -1, "large terminal distances");
            check(g.firstMeeting(0, 1) == -1 && g.firstMeeting(n - 1, n - 1) == 0, "termination is never a meeting");
            FunctionalGraphFold<lll, Sum> fold(g, vector<lll>(n, 1), 0, Sum{});
            auto a = fold.fold(0, std::numeric_limits<ulng>::max());
            check(a.next == -1 && a.count == ulng(n) && a.value == n, "large fold consumes terminal once");
            auto all = g.jumpAll(ulng(n / 2)); bool ok = true;
            for (int u = 0; u < n; ++u) { ok = ok && all[u] == (u + n / 2 < n ? u + n / 2 : -1); }
            check(ok, "large terminal jumpAll");
            check(fold.maxStep(0, [](lll) { return true; }) == ulng(n) && fold.maxStep(0, [&](lll x) { return x <= n / 3; }) == ulng(n / 3),
                  "large terminal maxStep"); }
        next.back() = 0;
        {
            FunctionalGraph g(next); context = "cycle n=" + std::to_string(n);
            check(g.cycles.size() == 1 && int(g.cycles[0].size()) == n, "large cycle decomposition");
            ulng count = std::numeric_limits<ulng>::max();
            check(g.jump(0, count) == int(count % n) && g.distance(n - 1, 0) == 1 && g.firstMeeting(0, 1) == -1, "large cycle queries");
            auto r = functionalOrbit(next, 0); check(r.entry == 0 && r.tail == 0 && r.cycle_length == n, "large constant-space cycle orbit");
            FunctionalGraphFold<lll, Sum> fold(g, vector<lll>(n, 1), 0, Sum{}); auto a = fold.fold(0, count);
            check(a.next == int(count % n) && a.count == count && a.value == lll(count), "large full-range cycle fold");
            auto all = g.jumpAll(count); bool ok = true;
            for (int u = 0; u < n; ++u) { ok = ok && all[u] == int((u + count % n) % n); }
            check(ok, "large cycle full-range jumpAll");
            check(fold.maxStep(1, [](lll x) { return x <= lll(1) << 62; }) == 1ULL << 62 && fold.maxStep(1, [](lll) { return true; }) == count,
                  "large cycle full-range maxStep"); }
        next.back() = n / 2;
        {
            FunctionalGraph g(next); context = "long tail into cycle n=" + std::to_string(n);
            check(g.firstMeeting(0, n / 2) == n / 2, "different depths with aligned cycle phase");
            check(g.distance(0, n - 1) == n - 1 && g.distance(n - 1, 0) == -1, "tail/cycle reachability"); }
        vector<int> fixture{2, 2, 3, 4, 3, -1, 5}; runCase(fixture, true);
        FunctionalGraph original(fixture), copy = original; auto moved = std::move(copy);
        fixture[2] = -1; original = FunctionalGraph(fixture);
        context = "copy/move/reassignment and external successor mutation";
        check(moved.jump(0, 3) == 4 && original.jump(0, 3) == -1, "graph snapshots and rebuild");
        vector<string> words(moved.n, "x"); FunctionalGraphFold<string, Concat> folded(moved, words, "", Concat{});
        words[0] = "bad"; auto fold_copy = folded; auto fold_moved = std::move(fold_copy);
        folded = FunctionalGraphFold<string, Concat>(original, words, "", Concat{});
        check(fold_moved.fold(0, 3).value == "xxx" && folded.fold(0, 3).value == "badx", "fold label/graph ownership and assignment");
        cout << "PASS stack-safe terminal chains/cycles/tails n=" << n << ", copy/move/snapshot/rebuild\n";}
    void invalid(const string &name) {
        FunctionalGraph g(vector<int>{1, -1}); vector<int> next{1, -1}; vector<string> words{"a", "b"};
        FunctionalGraphFold<string, Concat> fold(g, words, "", Concat{});
        if (name == "successor-negative") { FunctionalGraph bad(vector<int>{-2}); }
        else if (name == "successor-high") { FunctionalGraph bad(vector<int>{1}); }
        else if (name == "undirected") { FunctionalGraph bad(Graph(2)); }
        else if (name == "two-successors" || name == "parallel-successors" || name == "csr-parallel") {
            Graph graph(2, true); graph.addEdge(0, 1); graph.addEdge(0, name == "two-successors" ? 0 : 1);
            if (name == "csr-parallel") { FunctionalGraph bad{CsrGraph(graph)}; }
            else { FunctionalGraph bad(graph); }}
        else if (name == "csr-undirected") { FunctionalGraph bad{CsrGraph(Graph(2))}; }
        else if (name == "jump-negative") { g.jump(-1, 0); }
        else if (name == "jump-high") { g.jump(2, 0); }
        else if (name == "distance-negative") { g.distance(-1, 0); }
        else if (name == "distance-high") { g.distance(0, 2); }
        else if (name == "reachable-high") { g.reachable(2, 0); }
        else if (name == "meeting-negative") { g.firstMeeting(0, -1); }
        else if (name == "meeting-high") { g.firstMeeting(2, 0); }
        else if (name == "empty-jump") { FunctionalGraph(vector<int>{}).jump(0, 0); }
        else if (name == "orbit-negative") { functionalOrbit(next, -1); }
        else if (name == "orbit-high") { functionalOrbit(next, 2); }
        else if (name == "aggregate-size") { g.cycleAggregates(vector<string>{"a"}, string{}, Concat{}); }
        else if (name == "rotated-size") { g.cycleAggregate(0, vector<string>{"a"}, string{}, Concat{}); }
        else if (name == "rotated-high") { g.cycleAggregate(2, words, string{}, Concat{}); }
        else if (name == "fold-size") { FunctionalGraphFold<string, Concat> bad(g, vector<string>{"a"}, "", Concat{}); }
        else if (name == "fold-negative") { fold.fold(-1, 0); }
        else if (name == "fold-high") { fold.fold(2, 0); }
        else if (name == "fold-cycle-high") { fold.cycleAggregate(2); }
        else if (name == "segment-negative") { fold.segment(0, -1); }
        else if (name == "segment-high") { fold.segment(0, 3); }
        else if (name == "segment-unavailable") { fold.segment(1, 2); }
        else if (name == "step-high") { fold.maxStep(2, [](const string &) { return true; }); }
        else if (name == "step-identity") { fold.maxStep(0, [](const string &x) { return !x.empty(); }); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); boundaries();
        cout << "PASS functional graph seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';} catch (const std::exception &e) {
        cerr << "FAIL functional graph seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
