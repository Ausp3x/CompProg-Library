#include "../../02-Data Structures/63-offline_deletion.hpp"
#include "../../02-Data Structures/09-rollbackdsu.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0;
    double worstRatio = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(int x) { return std::to_string(x); }
    string show(bool x) { return x ? "1" : "0"; }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    template<typename T> vector<T> sorted(vector<T> v) { sort(v.begin(), v.end()); return v; }
    ulng mix(ulng x) { x += 0x9e3779b97f4a7c15ULL; x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL; x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL; return x ^ (x >> 31); }

    // Components of the graph on n vertices with the given edges (BFS labels).
    vector<int> labels(int n, const vector<pair<int, int>> &es) {
        vector<vector<int>> adj(n);
        for (auto [u, v] : es) { adj[u].push_back(v); adj[v].push_back(u); }
        vector<int> c(n, -1);
        for (int s = 0, k = 0; s < n; ++s) {
            if (c[s] >= 0) { continue; }
            vector<int> q{s};
            c[s] = k;
            for (size_t i = 0; i < q.size(); ++i) { for (int y : adj[q[i]]) { if (c[y] < 0) { c[y] = k; q.push_back(y); } } }
            ++k;}
        return c;}

    // Rank and maximum xor of a multiset by fresh Gaussian elimination (brute force).
    pair<int, ulng> gauss(vector<ulng> v) {
        int rank = 0;
        for (int b = 63; b >= 0; --b) {
            int piv = -1;
            for (int i = rank; i < int(v.size()); ++i) { if (v[i] >> b & 1) { piv = i; break; } }
            if (piv < 0) { continue; }
            swap(v[rank], v[piv]);
            for (int i = 0; i < int(v.size()); ++i) { if (i != rank && (v[i] >> b & 1)) { v[i] ^= v[rank]; } }
            ++rank;}
        ulng mx = 0;
        for (int i = 0; i < rank; ++i) { mx ^= v[i]; }
        return {rank, mx};}

    // Undo-only xor basis: insert records the reduced vector or 0 when dependent.
    struct UndoXorBasis {
        array<ulng, 64> b{};
        vector<int> his;
        void insert(ulng x) {
            for (int k = 63; k >= 0; --k) {
                if (!(x >> k & 1)) { continue; }
                if (!b[k]) { b[k] = x; his.push_back(k); return; }
                x ^= b[k];}
            his.push_back(-1);}
        void undo() { if (his.back() >= 0) { b[his.back()] = 0; } his.pop_back(); }
        pair<int, ulng> query() const {
            int rank = 0;
            ulng mx = 0;
            for (int k = 63; k >= 0; --k) { if (b[k]) { ++rank; mx = max(mx, mx ^ b[k]); } }
            return {rank, mx};}
    };

    struct Interval { int l, r, id; };

    // Engine oracle: the applied multiset at each answered time equals the intervals containing t; LIFO undo, call counts.
    void recorder(Rng &rng, int rounds, int maxT, int maxM) {
        for (int s = 0; s < rounds; ++s) {
            int T = int(rnd(rng, 0, maxT)), m = int(rnd(rng, 0, maxM));
            context = "recorder round=" + show(s) + " T=" + show(T) + " m=" + show(m);
            OfflineDeletion<int> od(T);
            vector<Interval> iv;
            lng decomposition = 0;
            int sz = int(std::bit_ceil(uint(max(T, 1))));
            for (int j = 0; j < m; ++j) {
                int l = int(rnd(rng, 0, T)), r = rng() % 5 ? int(rnd(rng, l, T)) : l;
                int id = rng() % 6 ? j : 0;
                od.addInterval(l, r, id); iv.push_back({l, r, id});
                for (int a = l + sz, b = r + sz; a < b; a >>= 1, b >>= 1) { decomposition += (a & 1) + (b & 1); a += a & 1; }}
            vector<int> asked;
            for (int t = 0; t < T; ++t) { if (rng() % 3 == 0) { od.addQuery(t); asked.push_back(t); if (rng() % 4 == 0) { od.addQuery(t); } } }
            // Large rounds compare count and two multiset hashes with a difference-array sweep; small rounds the exact multiset.
            bool exact = lng(T) * m <= 2000000;
            vector<ulng> dc(T + 1), d1(T + 1), d2(T + 1);
            for (auto &x : iv) {
                dc[x.l] += 1; dc[x.r] -= 1; d1[x.l] += mix(ulng(x.id)); d1[x.r] -= mix(ulng(x.id)); d2[x.l] += mix(~ulng(x.id)); d2[x.r] -= mix(~ulng(x.id));}
            for (int t = 1; t <= T; ++t) { dc[t] += dc[t - 1]; d1[t] += d1[t - 1]; d2[t] += d2[t - 1]; }
            vector<int> applied, answered;
            ulng h1 = 0, h2 = 0;
            lng adds = 0, undos = 0;
            auto run = [&] {
                answered.clear(); adds = undos = 0;
                auto add = [&](int x) { applied.push_back(x); h1 += mix(ulng(x)); h2 += mix(~ulng(x)); ++adds; };
                auto undo = [&] { h1 -= mix(ulng(applied.back())); h2 -= mix(~ulng(applied.back())); applied.pop_back(); ++undos; };
                od.solve(add, undo, [&](int t) {
                    checkEqual(ulng(applied.size()), dc[t], "count at t=" + show(t));
                    checkEqual(pair{h1, h2}, pair{d1[t], d2[t]}, "multiset hash at t=" + show(t));
                    if (exact) {
                        vector<int> want;
                        for (auto &x : iv) { if (x.l <= t && t < x.r) { want.push_back(x.id); } }
                        checkEqual(sorted(applied), sorted(want), "multiset at t=" + show(t));}
                    answered.push_back(t);});
                checkEqual(answered, asked, "answer once per queried time, increasing");
                checkEqual(applied.size(), size_t(0), "all adds undone"); checkEqual(adds, undos, "adds equal undos");
                checkEqual(adds <= decomposition, true, "adds within decomposition " + show(adds) + " <= " + show(decomposition));};
            run();
            lng first = adds;
            run();
            checkEqual(adds, first, "solve twice repeats");}}

    // Only time 0 asked: one add per interval containing 0 (pruning of unasked subtrees).
    void pruning(Rng &rng) {
        for (int T : {1, 2, 7, 64, 1000}) {
            context = "pruning T=" + show(T);
            OfflineDeletion<int> od(T);
            int want = 0;
            for (int j = 0; j < 300; ++j) { int l = int(rnd(rng, 0, T)), r = int(rnd(rng, l, T)); od.addInterval(l, r, j); want += l == 0 && r > 0; }
            od.addQuery(0);
            lng adds = 0;
            od.solve([&](int) { ++adds; }, [] {}, [](int) {});
            checkEqual(adds, lng(want), "adds");
            OfflineDeletion<int> none(T);
            none.addInterval(0, T, 1);
            none.solve([&](int) { throw std::runtime_error("add without queries"); }, [] {}, [](int) {});}}

    // insert/erase events in time order over repeated keys; oracle counts inserts minus erases at times <= t.
    void events(Rng &rng, int rounds, int maxT) {
        for (int s = 0; s < rounds; ++s) {
            int T = int(rnd(rng, 1, maxT));
            context = "events round=" + show(s) + " T=" + show(T);
            OfflineDeletion<int> od(T);
            vector<array<int, 3>> ev;
            map<int, int> live;
            for (int t = 0; t <= T; ++t) {
                for (int k = int(rnd(rng, 0, 3)); k > 0; --k) {
                    int x = int(rnd(rng, 0, 5));
                    if (live[x] > 0 && rng() % 2) { od.erase(t, x); --live[x]; ev.push_back({t, x, -1}); }
                    else { od.insert(t, x); ++live[x]; ev.push_back({t, x, 1}); }}}
            vector<int> asked;
            for (int t = 0; t < T; ++t) { if (rng() % 2) { od.addQuery(t); asked.push_back(t); } }
            vector<int> applied, answered;
            od.solve([&](int x) { applied.push_back(x); }, [&] { applied.pop_back(); }, [&](int t) {
                map<int, int> cnt;
                for (auto [et, x, d] : ev) { if (et <= t) { cnt[x] += d; } }
                vector<int> want;
                for (auto [x, c] : cnt) { for (int k = 0; k < c; ++k) { want.push_back(x); } }
                checkEqual(sorted(applied), want, "multiset at t=" + show(t));
                answered.push_back(t);});
            checkEqual(answered, asked, "answered");
            checkEqual(od.open.empty(), true, "open copies closed at T");}}

    // Clients: RollbackDSU connectivity and an undo-only xor basis against brute force per asked time.
    void clients(Rng &rng, int rounds, int maxT, int n) {
        for (int s = 0; s < rounds; ++s) {
            int T = int(rnd(rng, 1, maxT)), m = int(rnd(rng, 0, 3 * maxT));
            context = "clients round=" + show(s) + " T=" + show(T) + " n=" + show(n);
            OfflineDeletion<pair<int, int>> ge(T);
            OfflineDeletion<ulng> xe(T);
            vector<array<int, 4>> edges;
            vector<array<lng, 3>> vecs;
            for (int j = 0; j < m; ++j) {
                int l = int(rnd(rng, 0, T - 1)), r = int(rnd(rng, l + 1, T));
                int u = int(rnd(rng, 0, n - 1)), v = int(rnd(rng, 0, n - 1));
                ge.addInterval(l, r, {u, v}); edges.push_back({l, r, u, v});
                ulng x = rng() >> rnd(rng, 0, 63);
                if (rng() % 4 == 0 && !vecs.empty()) { x = ulng(vecs[size_t(rnd(rng, 0, lng(vecs.size()) - 1))][2]) ^ (rng() & 1); }
                xe.addInterval(l, r, x); vecs.push_back({l, r, lng(x)});}
            for (int t = 0; t < T; ++t) { ge.addQuery(t); if (t % 2 == 0) { xe.addQuery(t); } }
            RollbackDSU d(n);
            ge.solve([&](const pair<int, int> &e) { d.uniteSets(e.first, e.second); }, [&] { d.undo(); }, [&](int t) {
                vector<pair<int, int>> live;
                for (auto &e : edges) { if (e[0] <= t && t < e[1]) { live.push_back({e[2], e[3]}); } }
                auto c = labels(n, live);
                checkEqual(d.count(), 1 + *std::max_element(c.begin(), c.end()), "dsu count t=" + show(t));
                for (int u = 0; u < n; ++u) { checkEqual(d.isSameSet(u, (u * 3 + 1) % n), c[u] == c[(u * 3 + 1) % n], "dsu isSameSet t=" + show(t)); }});
            checkEqual(d.snapshot(), 0, "dsu restored");
            UndoXorBasis xb;
            xe.solve([&](ulng x) { xb.insert(x); }, [&] { xb.undo(); }, [&](int t) {
                vector<ulng> live;
                for (auto &e : vecs) { if (e[0] <= t && t < e[1]) { live.push_back(ulng(e[2])); } }
                auto [rank, mx] = gauss(live);
                checkEqual(xb.query().first, rank, "xor rank t=" + show(t)); checkEqual(xb.query().second, mx, "xor max t=" + show(t));});}}

    // QueueUndo against a deque of live updates: recorder multiset, call-count bound, and a RollbackDSU client.
    template<bool TYPED>
    void queueUndo(Rng &rng, int rounds, int maxOps, int pattern) {
        for (int s = 0; s < rounds; ++s) {
            int ops = int(rnd(rng, 1, maxOps));
            context = "queueUndo typed=" + show(TYPED) + " pattern=" + show(pattern) + " round=" + show(s) + " ops=" + show(ops);
            vector<int> applied;
            lng calls = 0;
            auto add = [&](const int &x) { applied.push_back(x); ++calls; };
            auto undo = [&] { applied.pop_back(); ++calls; };
            using Q = std::conditional_t<TYPED, QueueUndo<int, decltype(add), decltype(undo)>, QueueUndo<int>>;
            Q q(add, undo);
            deque<int> live;
            int next = 0, peak = 1;
            for (int k = 0; k < ops; ++k) {
                bool push = live.empty() || (pattern == 0 ? rng() % 2 : pattern == 1 ? k < ops / 2 : pattern == 2 ? k % 3 != 2 : pattern == 3 ? (k / 64) % 2 == 0 : k < ops / 2 || k % 2 == 0);
                if (push) { q.push(next); live.push_back(next++); }
                else { q.popFront(); live.pop_front(); }
                peak = max(peak, int(live.size()));
                checkEqual(q.size(), int(live.size()), "size"); checkEqual(q.empty(), live.empty(), "empty");
                if (ops <= 300 || k % max(97, ops / 64) == 0) { checkEqual(sorted(applied), vector<int>(live.begin(), live.end()), "applied multiset step=" + show(k)); }}
            double bound = double(ops) * (2 + 2 * std::log2(double(peak) + 1));
            worstRatio = max(worstRatio, double(calls) / (double(ops) * (1 + std::log2(double(peak) + 1))));
            checkEqual(double(calls) <= 2 * bound, true, "amortized calls " + show(calls) + " <= " + show(2 * bound));}}

    void queueDsu(Rng &rng, int rounds, int n, int ops) {
        for (int s = 0; s < rounds; ++s) {
            context = "queue dsu round=" + show(s);
            RollbackDSU d(n);
            QueueUndo<pair<int, int>> q([&](const pair<int, int> &e) { d.uniteSets(e.first, e.second); }, [&] { d.undo(); });
            deque<pair<int, int>> live;
            for (int k = 0; k < ops; ++k) {
                if (live.empty() || rng() % 5 < 3) { pair<int, int> e{int(rnd(rng, 0, n - 1)), int(rnd(rng, 0, n - 1))}; q.push(e); live.push_back(e); }
                else { q.popFront(); live.pop_front(); }
                auto c = labels(n, vector<pair<int, int>>(live.begin(), live.end()));
                checkEqual(d.count(), 1 + *std::max_element(c.begin(), c.end()), "count step=" + show(k));
                for (int u = 0; u < n; ++u) { checkEqual(d.isSameSet(u, (u + 1) % n), c[u] == c[(u + 1) % n], "isSameSet step=" + show(k)); }}}}

    void edgesCase() {
        context = "edges";
        OfflineDeletion<int> zero(0);
        zero.addInterval(0, 0, 5);
        zero.solve([](int) { throw std::runtime_error("add at T=0"); }, [] {}, [](int) { throw std::runtime_error("answer at T=0"); });
        OfflineDeletion<int> one(1);
        one.addInterval(0, 1, 7); one.addInterval(1, 1, 8); one.insert(1, 9); one.insert(0, 4); one.erase(0, 4); one.addQuery(0);
        vector<int> got;
        one.solve([&](int x) { got.push_back(x); }, [&] { got.pop_back(); }, [&](int t) { checkEqual(t, 0, "time"); checkEqual(got, vector<int>{7}, "T=1 state"); });
        QueueUndo<int> q([](const int &) {}, [] {});
        q.push(1); q.popFront();
        checkEqual(q.empty(), true, "queue empty again");
        struct Unordered { int v; };
        OfflineDeletion<Unordered> u(3);
        u.addInterval(1, 3, {5}); u.addQuery(0); u.addQuery(2);
        vector<int> seen;
        u.solve([&](const Unordered &x) { seen.push_back(x.v); }, [&] { seen.pop_back(); }, [&](int t) { checkEqual(int(seen.size()), t == 2 ? 1 : 0, "X without operator< t=" + show(t)); });}

    void invalid(const string &name) {
        OfflineDeletion<int> od(3);
        QueueUndo<int> q([](const int &) {}, [] {});
        if (name == "interval-reversed") { od.addInterval(2, 1, 0); }
        else if (name == "interval-range") { od.addInterval(0, 4, 0); }
        else if (name == "query-range") { od.addQuery(3); }
        else if (name == "insert-range") { od.insert(4, 0); }
        else if (name == "erase-absent") { od.erase(1, 0); }
        else if (name == "erase-early") { od.insert(2, 0); od.erase(1, 0); }
        else if (name == "time-negative") { OfflineDeletion<int> bad(-1); }
        else if (name == "time-large") { OfflineDeletion<int> bad((1 << 29) + 1); }
        else if (name == "queue-pop-empty") { q.popFront(); }
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
        std::cout << "offline_deletion seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edgesCase();
        pruning(rng);
        int rounds = mode == "quick" ? 40 : mode == "full" ? 400 : 3000;
        recorder(rng, rounds, 40, 60); recorder(rng, min(rounds / 10, 60), 700, 2000);
        events(rng, rounds, 30);
        clients(rng, rounds / 4, 40, 9);
        for (int p = 0; p < 5; ++p) { queueUndo<false>(rng, rounds / 4, 300, p); queueUndo<true>(rng, rounds / 4, 300, p); }
        queueDsu(rng, rounds / 20, 8, 120);
        if (mode != "quick") {
            for (int p = 0; p < 5; ++p) { queueUndo<true>(rng, 3, 200000, p); }
            recorder(rng, 3, 100000, 200000);}
        if (mode == "stress") { for (int p = 0; p < 5; ++p) { queueUndo<true>(rng, 1, 600000, p); } clients(rng, 30, 120, 16); }
        std::cout << "PASS offline_deletion checks=" << checks << " queueUndo worst calls/(ops*(1+log2(peak+1)))=" << worstRatio << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL offline_deletion seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
