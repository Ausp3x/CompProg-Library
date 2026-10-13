#include "../../02-Data Structures/25-heap_deque.hpp"

namespace {
    ulng seed = 20261013;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    string show(const pair<lng, int> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) { string s = "["; for (auto &x : v) { s += show(x) + ","; } return s + "]"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    void check(bool ok, const string &op) { checkEqual(ok, true, op); }

    // Ids 0..n-1 against a key array with presence flags; top must carry a C-minimum key.
    template<int D, typename C>
    void daryRandom(Rng &rng, int steps, int n, lng span, bool build, const string &name) {
        C cmp;
        vector<lng> init(n);
        for (auto &x : init) { x = rnd(rng, -span, span); }
        DAryHeap<lng, D, C> h = build ? DAryHeap<lng, D, C>(init) : DAryHeap<lng, D, C>(n);
        vector<lng> key = init;
        vector<char> in(n, build);
        std::multiset<lng, C> keys;
        if (build) { keys.insert(init.begin(), init.end()); }
        for (int s = 0; s < steps; ++s) {
            int i = int(rnd(rng, 0, n - 1)), op = int(rng() % 10);
            context = name + " D=" + show(D) + " step=" + show(s) + " op=" + show(op) + " id=" + show(i);
            if (op < 3 && !in[i]) { lng x = rnd(rng, -span, span); h.push(i, x); key[i] = x; in[i] = 1; keys.insert(x); }
            else if (op < 5 && !keys.empty()) { int t = h.top(); h.pop(); in[t] = 0; keys.erase(keys.find(key[t])); }
            else if (op < 7 && in[i]) {
                lng x = cmp(key[i], key[i] - 1) ? key[i] + rnd(rng, 0, 9) : key[i] - rnd(rng, 0, 9);
                h.decreaseKey(i, x); keys.erase(keys.find(key[i])); keys.insert(x); key[i] = x;}
            else if (op < 8 && in[i]) { h.erase(i); in[i] = 0; keys.erase(keys.find(key[i])); }
            checkEqual(h.size(), int(keys.size()), "size"); checkEqual(h.empty(), keys.empty(), "empty");
            checkEqual(h.contains(i), bool(in[i]), "contains");
            if (!keys.empty()) {
                int t = h.top();
                check(in[t], "top present");
                checkEqual(h.key(t), key[t], "top key current");
                checkEqual(key[t], *keys.begin(), "top is a C-minimum");}}}

    template<typename C>
    void erasableRandom(Rng &rng, int steps, lng span, const string &name) {
        vector<lng> init(rnd(rng, 0, 30));
        for (auto &x : init) { x = rnd(rng, -span, span); }
        ErasableHeap<lng, C> h(init);
        std::multiset<lng, C> ref(init.begin(), init.end());
        for (int s = 0; s < steps; ++s) {
            int op = int(rng() % 10);
            context = name + " step=" + show(s) + " op=" + show(op) + " size=" + show(ref.size());
            if (op < 4 || ref.empty()) { lng x = rnd(rng, -span, span); h.push(x); ref.insert(x); }
            else if (op < 7) {
                auto it = std::next(ref.begin(), lng(rng() % ref.size()));
                h.erase(*it); ref.erase(it);}
            else { h.pop(); ref.erase(ref.begin()); }
            checkEqual(h.size(), int(ref.size()), "size"); checkEqual(h.empty(), ref.empty(), "empty");
            if (!ref.empty()) { checkEqual(h.top(), *ref.begin(), "top"); }}}

    // K heaps in one pool against multisets: push, pop, top, meld, size and, when the heap has it, applyAdd.
    template<typename H, typename T, typename C>
    void meldPlain(Rng &rng, int steps, auto gen, const string &name, H h = H()) {
        constexpr int K = 4;
        int root[K] = {};
        std::multiset<T, C> ref[K];
        for (int s = 0; s < steps; ++s) {
            int i = int(rng() % K), j = int((i + 1 + rng() % (K - 1)) % K), op = int(rng() % 12);
            context = name + " step=" + show(s) + " op=" + show(op) + " heap=" + show(i) + " size=" + show(ref[i].size());
            if (ref[i].size() > 300) { op = 5; }
            if (op < 5) { T x = gen(rng); h.push(root[i], x); ref[i].insert(x); }
            else if (op < 8 && !ref[i].empty()) { h.pop(root[i]); ref[i].erase(ref[i].begin()); }
            else if (op == 8) {
                root[i] = h.meld(root[i], root[j]); root[j] = 0;
                ref[i].insert(ref[j].begin(), ref[j].end()); ref[j].clear();}
            else if (op == 9) {
                if constexpr (requires { h.applyAdd(root[i], T()); }) {
                    T d = gen(rng);
                    h.applyAdd(root[i], d);
                    std::multiset<T, C> next;
                    for (T x : ref[i]) { next.insert(x + d); }
                    ref[i] = next;}}
            checkEqual(h.size(root[i]), int(ref[i].size()), "size");
            checkEqual(root[i] == 0, ref[i].empty(), "empty root");
            if (!ref[i].empty()) { checkEqual(h.top(root[i]), *ref[i].begin(), "top"); }}
        // Drain every heap in order.
        for (int i = 0; i < K; ++i) {
            context = name + " drain heap=" + show(i);
            for (; !ref[i].empty(); ref[i].erase(ref[i].begin())) {
                checkEqual(h.top(root[i]), *ref[i].begin(), "drain top");
                h.pop(root[i]);}
            checkEqual(root[i], 0, "drained root");}}

    // Handle heaps: nodes map to (heap, key); decreaseKey and erase hit random live handles, pop removes the root node.
    template<typename H>
    void meldHandles(Rng &rng, int steps, lng span, const string &name) {
        constexpr int K = 4;
        H h;
        int root[K] = {};
        vector<std::map<int, lng>> live(K);
        auto best = [&](int i) { lng res = std::numeric_limits<lng>::max(); for (auto &[u, k] : live[i]) { res = min(res, k); } return res; };
        for (int s = 0; s < steps; ++s) {
            int i = int(rng() % K), j = int((i + 1 + rng() % (K - 1)) % K), op = int(rng() % 14);
            context = name + " step=" + show(s) + " op=" + show(op) + " heap=" + show(i) + " size=" + show(live[i].size());
            if (live[i].size() > 300) { op = 5; }
            if (op < 5) { lng x = rnd(rng, -span, span); int u = h.push(root[i], x); check(!live[i].count(u), "fresh handle"); live[i][u] = x; }
            else if (op < 7 && !live[i].empty()) {
                checkEqual(live[i].count(root[i]), size_t(1), "root is live");
                checkEqual(live[i][root[i]], best(i), "root key minimum");
                live[i].erase(root[i]); h.pop(root[i]);}
            else if (op == 7) { root[i] = h.meld(root[i], root[j]); root[j] = 0; live[i].insert(live[j].begin(), live[j].end()); live[j].clear(); }
            else if (op < 11 && !live[i].empty()) {
                auto it = std::next(live[i].begin(), lng(rng() % live[i].size()));
                lng x = it->second - rnd(rng, 0, op == 8 ? 0 : 2 * span);
                h.decreaseKey(root[i], it->first, x); it->second = x;}
            else if (op < 13 && !live[i].empty()) {
                auto it = std::next(live[i].begin(), lng(rng() % live[i].size()));
                h.erase(root[i], it->first); live[i].erase(it);}
            checkEqual(h.size(root[i]), int(live[i].size()), "size");
            checkEqual(root[i] == 0, live[i].empty(), "empty root");
            if (!live[i].empty()) { checkEqual(h.top(root[i]), best(i), "top"); }}}

    // Leftist null-path lengths, binomial degree order, Fibonacci degree bound: the facts behind the bounds.
    void shapes(Rng &rng) {
        context = "shapes";
        int n = mode == "quick" ? 3000 : 60000;
        LeftistHeap<lng> lh;
        int lr = 0;
        for (int i = 0; i < n; ++i) { lh.push(lr, rnd(rng, 0, 1000000)); if (i % 3 == 2) { lh.pop(lr); }}
        for (vector<int> st = {lr}; !st.empty();) {
            int v = st.back(); st.pop_back();
            auto &x = lh.t[v];
            check(lh.t[x.l].s >= lh.t[x.r].s && x.s == lh.t[x.r].s + 1 && x.sz == lh.t[x.l].sz + lh.t[x.r].sz + 1, "leftist rank node=" + show(v));
            for (int c : {x.l, x.r}) {
                if (c) { check(!(lh.t[c].key < x.key), "leftist heap order"); st.push_back(c); }}}
        check((1 << lh.t[lr].s) <= lh.size(lr) + 1, "leftist right spine");
        BinomialHeap<lng> bh;
        int br = 0;
        for (int i = 0; i < n; ++i) { bh.push(br, rnd(rng, 0, 1000000)); if (i % 3 == 2) { bh.pop(br); }}
        int total = 0;
        for (int x = br, last = -1; x; x = bh.t[x].sib) {
            check(bh.t[x].deg > last, "binomial degrees increase");
            last = bh.t[x].deg; total += 1 << last;}
        checkEqual(total, bh.size(br), "binomial sizes are powers of two");
        FibonacciHeap<lng> fh;
        int fr = 0;
        for (int i = 0; i < n; ++i) { fh.push(fr, rnd(rng, 0, 1000000)); }
        fh.pop(fr);
        int roots = 0, maxdeg = 0;
        std::set<int> degs;
        int x = fr;
        do { ++roots; degs.insert(fh.t[x].deg); maxdeg = max(maxdeg, fh.t[x].deg); x = fh.t[x].rgt; } while (x != fr);
        checkEqual(int(degs.size()), roots, "fibonacci root degrees distinct after pop");
        check(maxdeg <= int(1.4405 * std::log2(double(n)) + 2), "fibonacci degree bound");}

    // Forces cascading cuts: pops build deep trees, then decreases hit nodes whose parents are already marked.
    void fibonacciCascade(Rng &rng) {
        context = "fibonacci cascade";
        FibonacciHeap<lng> h;
        int r = 0, n = mode == "quick" ? 2000 : 20000;
        vector<int> alive, at(n + 1);
        vector<lng> key(n + 1);
        auto drop = [&](int u) { at[alive.back()] = at[u]; alive[at[u]] = alive.back(); alive.pop_back(); };
        for (int i = 0; i < n; ++i) { int u = h.push(r, 10 * lng(i)); key[u] = 10 * lng(i); at[u] = int(alive.size()); alive.push_back(u); }
        drop(r); h.pop(r);
        for (int round = 0; round < 3 * n && !alive.empty(); ++round) {
            int u = alive[rng() % alive.size()];
            key[u] -= rnd(rng, 1, 5);
            h.decreaseKey(r, u, key[u]);
            if (round % 7 == 0) { drop(r); h.pop(r); }
            checkEqual(h.size(r), int(alive.size()), "size after cascades");
            if (round % 64 == 0 && !alive.empty()) {
                lng want = std::numeric_limits<lng>::max();
                for (int w : alive) { want = min(want, key[w]); }
                checkEqual(h.top(r), want, "top after cascades");}}}

    template<typename C>
    void depqRandom(Rng &rng, int steps, lng span, const string &name) {
        C cmp;
        vector<lng> init(rnd(rng, 0, 40));
        for (auto &x : init) { x = rnd(rng, -span, span); }
        DoubleEndedPriorityQueue<lng, C> q(init);
        std::multiset<lng, C> ref(init.begin(), init.end());
        for (int s = 0; s < steps; ++s) {
            int op = int(rng() % 10);
            context = name + " step=" + show(s) + " op=" + show(op) + " size=" + show(ref.size());
            if (ref.size() > 500) { op = 9; }
            if (op < 5 || ref.empty()) { lng x = rnd(rng, -span, span); q.push(x); ref.insert(x); }
            else if (op < 7 || (op == 9 && rng() % 2)) { q.popMin(); ref.erase(ref.begin()); }
            else { q.popMax(); ref.erase(std::prev(ref.end())); }
            checkEqual(q.size(), int(ref.size()), "size"); checkEqual(q.empty(), ref.empty(), "empty");
            if (!ref.empty()) { checkEqual(q.min(), *ref.begin(), "min"); checkEqual(q.max(), *ref.rbegin(), "max"); }
            if (s % 50 == 0) {
                int n = q.size();
                for (int i = 2; i < n; ++i) {
                    int p = (i / 2 - 1) / 2 * 2;
                    check(!cmp(q.a[i], q.a[p]) && !cmp(q.a[p + 1], q.a[i]), "interval nested i=" + show(i));
                    if (i % 2 == 1) { check(!cmp(q.a[i], q.a[i - 1]), "interval ordered i=" + show(i)); }}}}}

    template<typename C>
    void prioritySumRandom(Rng &rng, int steps, lng span, const string &name) {
        C cmp;
        PrioritySum<lng, C> ps(int(rnd(rng, 0, 5)));
        int k = ps.k;
        vector<lng> ref;
        for (int s = 0; s < steps; ++s) {
            int op = int(rng() % 10);
            context = name + " step=" + show(s) + " op=" + show(op) + " size=" + show(ref.size()) + " k=" + show(k);
            if (ref.size() > 200) { op = 5; }
            if (op < 5 || ref.empty()) { lng x = rnd(rng, -span, span); ps.insert(x); ref.push_back(x); }
            else if (op < 8) { int i = int(rng() % ref.size()); ps.erase(ref[i]); ref.erase(ref.begin() + i); }
            else { k = int(rnd(rng, 0, lng(ref.size()) + 2)); ps.setK(k); }
            vector<lng> v = ref;
            sort(v.begin(), v.end(), [&](lng a, lng b) { return cmp(b, a); });
            int kept = min(k, int(v.size()));
            lng in = std::accumulate(v.begin(), v.begin() + kept, 0LL), out = std::accumulate(v.begin() + kept, v.end(), 0LL);
            checkEqual(ps.size(), int(v.size()), "size");
            checkEqual(ps.sum(), in, "sum"); checkEqual(ps.sumRest(), out, "sumRest");
            if (kept) { checkEqual(ps.boundary(), v[kept - 1], "boundary"); }}}

    // Running median from PrioritySum: k = (size + 1) / 2 keeps the upper half, so boundary is sorted[size / 2].
    void median(Rng &rng) {
        context = "median";
        PrioritySum<lng> ps;
        vector<lng> v;
        for (int s = 0; s < 2000; ++s) {
            lng x = rnd(rng, -50, 50);
            ps.insert(x); v.push_back(x);
            ps.setK((ps.size() + 1) / 2);
            vector<lng> w = v;
            sort(w.begin(), w.end());
            checkEqual(ps.boundary(), w[w.size() / 2], "median after " + show(s + 1));}}

    void edges() {
        context = "edges";
        DAryHeap<lng> empty;
        checkEqual(empty.size(), 0, "dary empty size"); checkEqual(empty.empty(), true, "dary empty");
        DAryHeap<lng, 3> built(vector<lng>{});
        checkEqual(built.size(), 0, "dary empty build");
        BinaryHeap<pair<lng, int>> bp(2);
        bp.push(0, {5, 1}); bp.push(1, {5, 0});
        checkEqual(bp.top(), 1, "pair keys");
        DAryHeap<lng> copy(3);
        copy.push(0, 4);
        DAryHeap<lng> other = copy;
        other.pop();
        checkEqual(copy.size(), 1, "dary copy independence");
        ErasableHeap<lng> eh({2, 2, 2});
        eh.erase(2); eh.erase(2);
        checkEqual(eh.size(), 1, "erasable duplicates"); checkEqual(eh.top(), lng(2), "erasable duplicate top");
        eh.push(1); eh.erase(1); eh.erase(2);
        checkEqual(eh.empty(), true, "erasable all erased");
        LeftistHeap<pair<lng, int>> lp;
        SkewHeap<pair<lng, int>, std::greater<pair<lng, int>>> sp;
        int a = 0, b = 0;
        lp.push(a, {3, 1}); lp.push(a, {1, 2}); sp.push(b, {3, 1}); sp.push(b, {1, 2});
        checkEqual(lp.top(a), pair<lng, int>{1, 2}, "leftist pair"); checkEqual(sp.top(b), pair<lng, int>{3, 1}, "skew pair greater");
        checkEqual(LeftistHeap<pair<lng, int>>::ADD, false, "pair keys skip lazy tags");
        LeftistHeap<lng> lh;
        int z = 0;
        checkEqual(lh.size(z), 0, "meldable empty size"); checkEqual(lh.meld(0, 0), 0, "meld empties");
        lh.applyAdd(z, 5);
        checkEqual(z, 0, "applyAdd on empty");
        DoubleEndedPriorityQueue<lng> q({7});
        checkEqual(q.min(), lng(7), "depq single min"); checkEqual(q.max(), lng(7), "depq single max");
        q.popMax();
        checkEqual(q.empty(), true, "depq single pop");
        PrioritySum<lng> ps(3);
        checkEqual(ps.sum(), lng(0), "empty sum"); checkEqual(ps.size(), 0, "empty size");
        ps.insert(4);
        checkEqual(ps.boundary(), lng(4), "fewer than k");
        ps.setK(0);
        checkEqual(ps.sum(), lng(0), "k zero sum"); checkEqual(ps.sumRest(), lng(4), "k zero rest");
        // Arguments that alias the structure's own storage (review regressions).
        PrioritySum<lng> pa(2);
        pa.insert(5); pa.insert(7); pa.erase(pa.boundary());
        checkEqual(pa.sum(), lng(7), "erase(boundary()) sum"); checkEqual(pa.size(), 1, "erase(boundary()) size");
        PrioritySum<lng> pb(1);
        pb.insert(5); pb.insert(pb.boundary()); pb.insert(pb.boundary());
        checkEqual(pb.sum(), lng(5), "insert(boundary()) sum"); checkEqual(pb.sumRest(), lng(10), "insert(boundary()) rest");
        LeftistHeap<lng> la; SkewHeap<lng> sa;
        int lv = 0, sv = 0;
        for (lng x : {3, 10}) { la.push(lv, x); sa.push(sv, x); }
        la.applyAdd(lv, la.top(lv)); sa.applyAdd(sv, sa.top(sv));
        la.pop(lv); sa.pop(sv);
        checkEqual(la.top(lv), lng(13), "leftist applyAdd(top)"); checkEqual(sa.top(sv), lng(13), "skew applyAdd(top)");
        ErasableHeap<lng> ea({4, 4, 9});
        ea.erase(ea.top()); ea.push(ea.top());
        checkEqual(ea.size(), 3, "erasable aliasing size"); checkEqual(ea.top(), lng(4), "erasable aliasing top");
        DoubleEndedPriorityQueue<lng> qa({8, 2});
        for (int i = 0; i < 40; ++i) { qa.push(qa.min()); qa.push(qa.max()); }
        checkEqual(qa.size(), 82, "depq aliasing size"); checkEqual(qa.min(), lng(2), "depq aliasing min"); checkEqual(qa.max(), lng(8), "depq aliasing max");
        // Same seed gives the same randomized meld shapes.
        RandomizedHeap<lng> x(9), y(9);
        int rx = 0, ry = 0;
        for (int i = 0; i < 100; ++i) { x.push(rx, i * 37 % 101); y.push(ry, i * 37 % 101); }
        bool same = rx == ry;
        for (int i = 0; i < int(x.t.size()); ++i) { same = same && x.t[i].l == y.t[i].l && x.t[i].r == y.t[i].r; }
        checkEqual(same, true, "randomized seed reproducible");}

    void invalid(const string &name) {
        DAryHeap<lng> h(3);
        h.push(0, 5);
        ErasableHeap<lng> eh;
        LeftistHeap<lng> lh; SkewHeap<lng> sh; RandomizedHeap<lng> rh; BinomialHeap<lng> bh; PairingHeap<lng> ph; FibonacciHeap<lng> fh;
        int v = 0;
        DoubleEndedPriorityQueue<lng> q;
        PrioritySum<lng> ps(1);
        if (name == "dary-size-negative") { DAryHeap<lng> bad(-1); }
        else if (name == "dary-push-present") { h.push(0, 1); }
        else if (name == "dary-push-range") { h.push(3, 1); }
        else if (name == "dary-top-empty") { DAryHeap<lng> e(2); (void)e.top(); }
        else if (name == "dary-pop-empty") { DAryHeap<lng> e(2); e.pop(); }
        else if (name == "dary-decrease-absent") { h.decreaseKey(1, 0); }
        else if (name == "dary-decrease-greater") { h.decreaseKey(0, 6); }
        else if (name == "dary-erase-absent") { h.erase(2); }
        else if (name == "dary-contains-range") { (void)h.contains(-1); }
        else if (name == "dary-key-absent") { (void)h.key(1); }
        else if (name == "erasable-top-empty") { (void)eh.top(); }
        else if (name == "erasable-pop-empty") { eh.pop(); }
        else if (name == "leftist-top-empty") { (void)lh.top(v); }
        else if (name == "leftist-pop-empty") { lh.pop(v); }
        else if (name == "skew-pop-empty") { sh.pop(v); }
        else if (name == "randomized-top-empty") { (void)rh.top(v); }
        else if (name == "binomial-pop-empty") { bh.pop(v); }
        else if (name == "binomial-top-empty") { (void)bh.top(v); }
        else if (name == "pairing-decrease-greater") { int u = ph.push(v, 5); ph.decreaseKey(v, u, 6); }
        else if (name == "pairing-erase-empty") { ph.erase(v, 1); }
        else if (name == "pairing-pop-empty") { ph.pop(v); }
        else if (name == "fibonacci-decrease-greater") { int u = fh.push(v, 5); fh.decreaseKey(v, u, 6); }
        else if (name == "fibonacci-erase-null") { fh.push(v, 5); fh.erase(v, 0); }
        else if (name == "fibonacci-top-empty") { (void)fh.top(v); }
        else if (name == "depq-popmin-empty") { q.popMin(); }
        else if (name == "depq-popmax-empty") { q.popMax(); }
        else if (name == "depq-min-empty") { (void)q.min(); }
        else if (name == "depq-max-empty") { (void)q.max(); }
        else if (name == "prioritysum-boundary-empty") { (void)ps.boundary(); }
        else if (name == "prioritysum-negative-k") { PrioritySum<lng> bad(-1); }
        else if (name == "prioritysum-setk-negative") { ps.setK(-2); }
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
        std::cout << "heap_deque seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int steps = mode == "quick" ? 3000 : mode == "full" ? 60000 : 120000;
        edges();
        for (bool build : {false, true}) {
            for (lng span : {2LL, 1000000LL}) {
                daryRandom<2, std::less<lng>>(rng, steps / 4, 40, span, build, "dary less");
                daryRandom<3, std::greater<lng>>(rng, steps / 4, 40, span, build, "dary greater");
                daryRandom<4, std::less<lng>>(rng, steps / 4, 7, span, build, "dary small");
                daryRandom<8, std::less<lng>>(rng, steps / 4, 200, span, build, "dary wide");}}
        for (lng span : {3LL, 1000000LL}) {
            erasableRandom<std::less<lng>>(rng, steps, span, "erasable less");
            erasableRandom<std::greater<lng>>(rng, steps / 2, span, "erasable greater");
            auto gen = [span](Rng &g) { return rnd(g, -span, span); };
            meldPlain<LeftistHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "leftist");
            meldPlain<SkewHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "skew");
            meldPlain<RandomizedHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "randomized", RandomizedHeap<lng>(seed));
            meldPlain<BinomialHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "binomial");
            meldPlain<PairingHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "pairing");
            meldPlain<FibonacciHeap<lng>, lng, std::less<lng>>(rng, steps, gen, "fibonacci");
            meldPlain<LeftistHeap<lng, std::greater<lng>>, lng, std::greater<lng>>(rng, steps / 2, gen, "leftist greater");
            meldPlain<SkewHeap<lng, std::greater<lng>>, lng, std::greater<lng>>(rng, steps / 2, gen, "skew greater");
            meldPlain<BinomialHeap<lng, std::greater<lng>>, lng, std::greater<lng>>(rng, steps / 2, gen, "binomial greater");
            meldPlain<FibonacciHeap<lng, std::greater<lng>>, lng, std::greater<lng>>(rng, steps / 2, gen, "fibonacci greater");
            meldHandles<PairingHeap<lng>>(rng, steps, span, "pairing handles");
            meldHandles<FibonacciHeap<lng>>(rng, steps, span, "fibonacci handles");
            depqRandom<std::less<lng>>(rng, steps, span, "depq less");
            depqRandom<std::greater<lng>>(rng, steps / 2, span, "depq greater");
            prioritySumRandom<std::less<lng>>(rng, steps / 4, span, "prioritysum largest");
            prioritySumRandom<std::greater<lng>>(rng, steps / 4, span, "prioritysum smallest");}
        auto pairGen = [](Rng &g) { return pair<lng, int>{rnd(g, -5, 5), int(rnd(g, 0, 3))}; };
        meldPlain<LeftistHeap<pair<lng, int>>, pair<lng, int>, std::less<pair<lng, int>>>(rng, steps / 4, pairGen, "leftist pairs");
        meldPlain<PairingHeap<pair<lng, int>>, pair<lng, int>, std::less<pair<lng, int>>>(rng, steps / 4, pairGen, "pairing pairs");
        shapes(rng);
        fibonacciCascade(rng);
        median(rng);
        std::cout << "PASS heap_deque checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL heap_deque seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
