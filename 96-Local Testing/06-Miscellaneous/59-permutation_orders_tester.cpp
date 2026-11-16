#include "../../06-Miscellaneous/59-permutation_orders.hpp"

ulng seed = 20260927;
string context;
void check(bool ok, string_view s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << s << '\n'; std::exit(1); }}
string show(const vector<int> &a) {
    string s = "[";
    for (size_t i = 0; i < min(a.size(), size_t(40)); ++i) { s += std::to_string(a[i]) + ','; }
    return s + (a.size() > 40 ? "...]" : "]");}
void checkVector(const vector<int> &a, const vector<int> &b, string_view operation) {
    if (a != b) { check(false, string(operation) + " expected=" + show(b) + " actual=" + show(a)); }}
void checkCount(bool ok, string_view name, int n, ulng expected, ulng actual, string_view operation) {
    if (!ok) { context = string(name) + " n=" + std::to_string(n) + " expected=" + std::to_string(expected) + " actual=" + std::to_string(actual); check(false, operation); }}
ulng factorial(int n) {
    ulng f = 1;
    for (int i = 2; i <= n; ++i) { f *= ulng(i); }
    return f;}
// Exactly two neighbouring positions differ, and they are swapped.
bool adjacent(const vector<int> &a, const vector<int> &b) {
    int first = -1, differ = 0;
    for (int i = 0; i < int(a.size()); ++i) {
        if (a[i] != b[i]) { differ += 1; first = first < 0 ? i : first; }}
    return a.size() == b.size() && differ == 2 && first + 1 < int(a.size()) && a[first] == b[first + 1] && a[first + 1] == b[first];}
// The recursive SJT list: n - 1 is inserted into each permutation of n - 1 elements, right to left under
// even-indexed parents and left to right under odd ones.
vector<vector<int>> extendList(const vector<vector<int>> &list, int m) {
    vector<vector<int>> next;
    for (size_t i = 0; i < list.size(); ++i) {
        for (int s = 0; s <= m; ++s) {
            vector<int> q = list[i]; q.insert(q.begin() + (i % 2 ? s : m - s), m);
            next.push_back(std::move(q));}}
    return next;}
// Knuth's Algorithm P (TAOCP 7.2.1.2, plain changes): Even's direction o[j] and sweep counter c[j] per level j, where
// level j moves the value j - 1; O(1) amortized per step, swapped the left position of the last swap.
struct PlainChanges {
    int n, swapped = -1; vector<int> a, c, o;
    explicit PlainChanges(int size) : n(size), a(size), c(size + 1), o(size + 1, 1) { iota(a.begin(), a.end(), 0); }
    bool step() {
        int j = n, s = 0;
        for (; j > 1; --j) {
            int q = c[j] + o[j];
            if (0 <= q && q < j) {
                int x = j - c[j] + s - 1, y = j - q + s - 1;
                std::swap(a[x], a[y]); swapped = min(x, y); c[j] = q; return true;}
            s += q == j; o[j] = -o[j];}
        return false;}
};
// The state whose level j sits at offset d[j] in its sweep: rank R_j = R_{j-1} j + d[j], leftward (o = 1) iff R_{j-1} is
// even. rank is exact for n <= 20 and keeps its parity beyond. Insertions at offsets 0 and j - 1 cost O(1).
PlainChanges build(const vector<int> &d, ulng &rank) {
    int n = int(d.size()) - 1; PlainChanges s(n); std::deque<int> q; rank = 0;
    for (int j = 1; j <= n; ++j) {
        bool even = rank % 2 == 0;
        s.o[j] = even ? 1 : -1; s.c[j] = even ? d[j] : j - 1 - d[j];
        q.insert(q.begin() + (j - 1 - s.c[j]), j - 1);
        rank = rank * ulng(j) + ulng(d[j]);}
    s.a.assign(q.begin(), q.end()); return s;}
PlainChanges fromRank(int n, ulng r) {
    vector<int> d(n + 1); ulng rank = 0;
    for (int j = n; j > 0; --j) { d[j] = int(r % ulng(j)); r /= ulng(j); }
    return build(d, rank);}
// forEach's visitor contract: identity with i = -1, then p is its predecessor swapped at i, i + 1 < n; compared whole
// every call for n <= 64, else at i, i + 1 and whole every n calls and at the stop call. name is a literal.
struct Visits {
    string_view name; int n; ulng stop, calls = 0; vector<int> mirror;
    Visits(string_view label, int size, ulng last) : name(label), n(size), stop(last), mirror(size) { iota(mirror.begin(), mirror.end(), 0); }
    void require(bool ok, const vector<int> &p, string_view operation) const {
        if (!ok) { context = string(name) + " n=" + std::to_string(n) + " call=" + std::to_string(calls) + " p=" + show(p); check(false, operation); }}
    bool see(const vector<int> &p, int i) {
        require(calls ? 0 <= i && i < n - 1 : i == -1, p, "forEach i = -1 on the first call, else 0 <= i < n - 1");
        if (calls) { std::swap(mirror[i], mirror[i + 1]); }
        bool whole = n <= 64 || calls % ulng(n) == 0 || calls == stop;
        require(int(p.size()) == n && (whole ? p == mirror : p[i] == mirror[i] && p[i + 1] == mirror[i + 1]), p,
                "forEach p is the identity, then the previous permutation with positions i and i + 1 swapped");
        return calls++ != stop;}
};
// A visitor (an lvalue functor) returning false at call t ends the enumeration after t + 1 calls, at expected when given,
// and makes forEachTrotterJohnson return false, at the last permutation too.
void foreachStop(int n, ulng t, const vector<int> *expected) {
    ulng calls = 0;
    auto visit = [&](const vector<int> &p, int) {
        if (calls == t && expected && p != *expected) { checkCount(false, "forEach stop", n, t, calls, "forEach permutation at the stop call"); }
        return calls++ != t;};
    bool done = forEachTrotterJohnson(n, visit);
    checkCount(!done && calls == t + 1, "forEach stop", n, t + 1, calls, "forEach stopped at call t: t + 1 calls and false");}
// forEachTrotterJohnson visits the recursive SJT list in order and returns true after n! calls; stops at every call
// when every, else at the ends and 64 random calls.
void foreachList(const vector<vector<int>> &list, int n, bool every) {
    ulng total = list.size(); Visits v("forEach list", n, ~ulng(0));
    bool done = forEachTrotterJohnson(n, [&](auto &p, auto i) {
        static_assert(std::is_same_v<decltype(p), const vector<int> &> && std::is_same_v<decltype(i), int>, "visit(const vector<int> &, int)");
        v.require(v.calls < total && p == list[v.calls], p, "forEach order against the recursive SJT list");
        return v.see(p, i);});
    checkCount(done && v.calls == total, "forEach list", n, total, v.calls, "forEach returns true after n! calls");
    vector<ulng> stops{0, 1, total - 2, total - 1};
    std::mt19937_64 rng(seed + ulng(n));
    if (every) { stops.resize(total); iota(stops.begin(), stops.end(), ulng(0)); }
    else {
        for (int k = 0; k < 64; ++k) { stops.push_back(rng() % total); }}
    for (ulng t : stops) { foreachStop(n, t, &list[t]); }}
void exhaustive(int limit) {
    vector<vector<int>> list{{}};
    for (int n = 0; n <= limit; ++n) {
        if (n) { list = extendList(list, n - 1); }
        ulng total = factorial(n); vector<int> id(n); iota(id.begin(), id.end(), 0);
        context = "exhaustive n=" + std::to_string(n);
        vector<vector<int>> sorted = list; sort(sorted.begin(), sorted.end());
        check(list.size() == total && unique(sorted.begin(), sorted.end()) == sorted.end() && list[0] == id, "oracle list: n! distinct, identity first");
        PlainChanges walk(n); vector<int> q = id;
        for (ulng r = 0; r < total; ++r) {
            const vector<int> &p = list[r]; bool more = r + 1 < total;
            context = "exhaustive n=" + std::to_string(n) + " rank=" + std::to_string(r) + " p=" + show(p);
            check(!more || adjacent(p, list[r + 1]), "oracle list: consecutive permutations one adjacent transposition apart");
            checkVector(walk.a, p, "Algorithm P walk from identity follows the list");
            check(walk.step() == more, "Algorithm P stops after n! - 1 steps");
            PlainChanges state = fromRank(n, r);
            checkVector(state.a, p, "oracle decode of rank digits");
            check(state.step() == more && (!more || state.a == list[r + 1]), "Algorithm P from decoded state steps to the next entry");
            check(trotterJohnsonRank(p) == r, "rank expected=" + std::to_string(r) + " actual=" + std::to_string(trotterJohnsonRank(p)));
            vector<int> out(size_t(n) + 3, -7);
            check(trotterJohnsonUnrank(n, r, out), "unrank exists"); checkVector(out, p, "unrank");
            vector<int> next = p;
            check(nextTrotterJohnson(next) == more, "next flag"); checkVector(next, list[more ? r + 1 : 0], "next");
            checkVector(q, p, "sequential next walk");
            check(nextTrotterJohnson(q) == more, "sequential next flag");}
        foreachList(list, n, n <= 7);
        context = "exhaustive n=" + std::to_string(n);
        checkVector(q, id, "next wraps to identity after n! steps");
        for (ulng r : {total, total + 1, total * 7 + 3, ~ulng(0)}) {
            vector<int> out{-42, 5}; check(!trotterJohnsonUnrank(n, r, out), "unrank absent rank=" + std::to_string(r));
            checkVector(out, {-42, 5}, "unrank absence leaves out");}}}
// Random n in [9, 20]: unrank/rank round trips and 1000-step next walks against rank + 1 and Algorithm P.
void randomized(std::mt19937_64 &rng, int cases) {
    for (int t = 0; t < cases; ++t) {
        int n = 9 + int(rng() % 12); ulng total = factorial(n), r = rng() % total;
        if (t % 5 == 1) { r = total - 1 - rng() % 1000; }
        if (t % 5 == 2) { r = rng() % 1000; }
        context = "random n=" + std::to_string(n) + " rank=" + std::to_string(r);
        vector<int> p{-1};
        check(trotterJohnsonUnrank(n, r, p) && isPermutation(p) && int(p.size()) == n, "unrank exists");
        PlainChanges state = fromRank(n, r);
        checkVector(p, state.a, "unrank against oracle decode");
        check(trotterJohnsonRank(p) == r, "rank round trip actual=" + std::to_string(trotterJohnsonRank(p)));
        for (ulng bad : {total, total + rng() % total, ~ulng(0), ~ulng(0) - rng() % 1000}) {
            vector<int> out = p; check(!trotterJohnsonUnrank(n, bad, out), "unrank absent rank=" + std::to_string(bad));
            checkVector(out, p, "unrank absence leaves out");}
        for (int s = 0; s < 1000; ++s) {
            context = "random n=" + std::to_string(n) + " walk from rank=" + std::to_string(r) + " step=" + std::to_string(s);
            vector<int> before = p; r = (r + 1) % total;
            bool ok = nextTrotterJohnson(p), expected = state.step();
            check(ok == (r != 0) && ok == expected, "next flag");
            if (!ok) { state = PlainChanges(n); }
            checkVector(p, state.a, "next against Algorithm P");
            check(trotterJohnsonRank(p) == r, "next is rank + 1 mod n! expected=" + std::to_string(r));
            check(!ok || adjacent(before, p), "next one adjacent transposition");}}
    for (int n : {19, 20}) {
        ulng total = factorial(n); vector<int> id(n), last, out{-42}; iota(id.begin(), id.end(), 0);
        vector<int> rev(id.rbegin(), id.rend());
        context = "boundary n=" + std::to_string(n);
        check(trotterJohnsonRank(id) == 0 && trotterJohnsonUnrank(n, 0, out) && out == id, "identity rank 0");
        check(trotterJohnsonUnrank(n, total - 1, last) && trotterJohnsonRank(last) == total - 1, "last rank n! - 1");
        checkVector(last, fromRank(n, total - 1).a, "last against oracle decode");
        check(trotterJohnsonUnrank(n, trotterJohnsonRank(rev), out) && out == rev, "reversed rank round trip");
        check(!nextTrotterJohnson(last) && last == id, "last wraps to identity");}}
// Long next walks against Algorithm P from the identity and from cascade states: random low levels, middle levels at a
// sweep end (mostly the last offset, so a step cascades through them), top two levels near the end of theirs.
void walk(PlainChanges state, int steps, const string &name) {
    vector<int> p = state.a; int n = state.n;
    for (int s = 0; s < steps; ++s) {
        context = name + " n=" + std::to_string(n) + " step=" + std::to_string(s);
        bool expected = state.step(), ok = nextTrotterJohnson(p);
        check(ok == expected, "next flag against Algorithm P");
        if (!ok) { state = PlainChanges(n); }
        check(p == state.a, "next against Algorithm P");}}
void longWalks(std::mt19937_64 &rng, int identityN, int identitySteps, const vector<array<int, 3>> &plan) {
    walk(PlainChanges(identityN), identitySteps, "identity walk");
    for (auto [n, cases, steps] : plan) {
        for (int t = 0; t < cases; ++t) {
            int k = 1 + int(rng() % 8); vector<int> d(n + 1); ulng rank = 0;
            for (int j = 1; j <= n; ++j) {
                if (j <= k) { d[j] = int(rng() % ulng(j)); }
                else if (j >= n - 1) { d[j] = max(0, j - 1 - int(rng() % ulng(steps / 2 + 1))); }
                else { d[j] = rng() % 8 ? j - 1 : 0; }}
            if (t == 0) {
                for (int j = 1; j <= n; ++j) { d[j] = j - 1; }}
            PlainChanges state = build(d, rank);
            context = "cascade state n=" + std::to_string(n) + " k=" + std::to_string(k);
            check(isPermutation(state.a), "oracle state is a permutation");
            if (t == 0) {
                vector<int> p = state.a, id(n); iota(id.begin(), id.end(), 0);
                check(!nextTrotterJohnson(p) && p == id, "last permutation wraps to identity");}
            walk(state, steps, "cascade walk k=" + std::to_string(k));}}}
// The first steps calls (all n! when steps >= n!) against Algorithm P's swap index, next and rank on the first checked
// calls; the header's loop passes n - v levels when it moves value v, at most 2 calls + n in all.
void foreachPrefix(int n, ulng steps, ulng checked) {
    ulng total = n <= 20 ? factorial(n) : ~ulng(0), work = 0; bool all = steps >= total;
    Visits v("forEach prefix", n, all ? ~ulng(0) : steps - 1); PlainChanges walk(n); vector<int> q = walk.a;
    bool done = forEachTrotterJohnson(n, [&](const vector<int> &p, int i) {
        if (v.calls) {
            v.require(walk.step() && walk.swapped == i, p, "forEach swap index against Algorithm P");
            work += ulng(n - max(p[i], p[i + 1]));}
        if (v.calls < checked) {
            if (v.calls) { v.require(nextTrotterJohnson(q) && q == p, p, "forEach against nextTrotterJohnson"); }
            if (n <= 20) { v.require(trotterJohnsonRank(p) == v.calls, p, "forEach rank equals the call index"); }}
        return v.see(p, i);});
    work += all ? ulng(n - 1) : 0;
    checkCount(done == all && v.calls == min(steps, total), "forEach prefix", n, min(steps, total), v.calls, "forEach call count and result");
    checkCount(!all || !walk.step(), "forEach prefix", n, total, v.calls, "forEach ends with Algorithm P");
    checkCount(work <= 2 * v.calls + ulng(n), "forEach work", n, 2 * v.calls + ulng(n), work, "forEach loop work at most 2 calls + n");}
void foreachLarge(std::mt19937_64 &rng, const string &mode) {
    bool quick = mode == "quick", full = mode == "full"; ulng all = ~ulng(0); vector<int> out;
    if (quick) {
        foreachPrefix(10, 40000, 40000); foreachPrefix(25, 40000, 40000); foreachPrefix(100000, 300000, 0);}
    else {
        foreachPrefix(10, all, 1000000);
        for (int n : {11, 12}) { foreachPrefix(n, full ? 1000000 : 3000000, 1000000); }
        foreachPrefix(25, full ? 3000000 : 10000000, 1000000);
        foreachPrefix(1000000, full ? 4000000 : 10000000, 0);
        for (ulng t : {factorial(10) - 2, factorial(10) - 1}) { trotterJohnsonUnrank(10, t, out); foreachStop(10, t, &out); }}
    for (int k = 0; k < 20; ++k) {
        ulng t = rng() % (quick ? 20000 : 1000000);
        trotterJohnsonUnrank(20, t, out); foreachStop(20, t, &out);}
    vector<int> id(100000); iota(id.begin(), id.end(), 0);
    foreachStop(100000, 0, &id); foreachStop(100000, 1, nullptr);}
void oracle() {
    char op;
    while (cin >> op) {
        int n; cin >> n;
        if (op == 'F') {
            ulng t, calls = 0; cin >> t; vector<int> moves;
            bool done = forEachTrotterJohnson(n, [&](const vector<int> &, int i) { moves.push_back(i); return calls++ != t; });
            cout << done << ' ' << calls;
            for (int i : moves) { cout << ' ' << i; }
            cout << '\n'; continue;}
        if (op == 'U') {
            ulng r; cin >> r; vector<int> out{-42}; bool ok = trotterJohnsonUnrank(n, r, out);
            cout << ok;
            for (int x : out) { cout << ' ' << x; }
            cout << '\n'; continue;}
        vector<int> p(n);
        for (int &x : p) { cin >> x; }
        if (op == 'R') { cout << trotterJohnsonRank(p) << '\n'; continue; }
        bool ok = nextTrotterJohnson(p); cout << ok;
        for (int x : p) { cout << ' ' << x; }
        cout << '\n';}
    check(cin.eof(), "Python input protocol");}
int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--oracle") { oracle(); return 0; }
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i]; vector<int> out, q{0, 0}, big(21); iota(big.begin(), big.end(), 0);
            if (p == "rank-size") { trotterJohnsonRank(big); }
            else if (p == "rank-invalid") { trotterJohnsonRank({0, 2}); }
            else if (p == "unrank-negative") { trotterJohnsonUnrank(-1, 0, out); }
            else if (p == "unrank-size") { trotterJohnsonUnrank(21, 0, out); }
            else if (p == "next-invalid") { nextTrotterJohnson(q); }
            else if (p == "foreach-negative") { forEachTrotterJohnson(-1, [](const vector<int> &, int) { return true; }); }
            return 0;}}
    std::mt19937_64 rng(seed);
    bool quick = mode == "quick", full = mode == "full";
    exhaustive(quick ? 6 : full ? 8 : 9);
    cout << "PASS rank, unrank, next and forEach (stops at every call through n=7) against the recursive SJT list and Algorithm P through n=" << (quick ? 6 : full ? 8 : 9) << "\n";
    randomized(rng, quick ? 20 : full ? 300 : 1000);
    cout << "PASS random n in [9, 20]: round trips, absent ranks and 1000-step walks against rank + 1 and Algorithm P\n";
    if (quick) { longWalks(rng, 200, 1000, {{{3000, 2, 40}, {25, 20, 1000}}}); }
    else if (full) { longWalks(rng, 1000, 4000, {{{25, 100, 3000}, {100000, 3, 40}, {1000000, 2, 6}}}); }
    else { longWalks(rng, 3000, 9000, {{{25, 400, 3000}, {100000, 6, 40}, {1000000, 3, 8}}}); }
    cout << "PASS long next walks against Algorithm P from the identity and from cascade states up to n=" << (quick ? 3000 : 1000000) << "\n";
    foreachLarge(rng, mode);
    cout << "PASS forEach prefixes against Algorithm P, next and rank up to n=" << (quick ? 100000 : 1000000) << ", early stops and loop work\n";}
