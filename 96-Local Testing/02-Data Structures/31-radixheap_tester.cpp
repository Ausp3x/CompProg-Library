#include "../../02-Data Structures/31-radixheap.hpp"

namespace {
    // Debug-mode push_heap and pop_heap verify the whole heap, so the checked build scales the priority_queue oracle and heap sorts down.
#ifdef _GLIBCXX_DEBUG
    constexpr bool CHECKED = true;
#else
    constexpr bool CHECKED = false;
#endif
    // Instrumented builds run shorter stress histories to stay inside the runner's per-binary timeout.
#if defined(_GLIBCXX_DEBUG) || defined(__SANITIZE_ADDRESS__)
    constexpr bool SLOW = true;
#else
    constexpr bool SLOW = false;
#endif
    ulng seed = 20261102;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename A, typename B> string show(const pair<A, B> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(v[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}
    template<typename T> void checkVec(const vector<T> &got, const vector<T> &want, const string &op) {
        checkEqual(got.size(), want.size(), op + " size");
        for (size_t i = 0; i < got.size(); ++i) {
            if (got[i] != want[i]) { checkEqual(got[i], want[i], op + " at " + show(i)); }}
        ++checks;}
    template<typename K> string typeName() { return std::is_same_v<K, int> ? "int" : std::is_same_v<K, lng> ? "lng" : std::is_same_v<K, uint> ? "uint" : "ulng"; }

    // Offsets from the least K: an order-preserving map written independently of the header's sign-bit flip.
    template<typename K>
    struct Keys {
        using U = std::make_unsigned_t<K>;
        static constexpr int B = std::numeric_limits<U>::digits;
        static constexpr U TOP = std::numeric_limits<U>::max();
        static U off(K x) { return U(U(x) - U(std::numeric_limits<K>::min())); }
        static K at(U o) { return K(U(o + U(std::numeric_limits<K>::min()))); }
        static U below(Rng &rng, U hi) { return hi == TOP ? U(rng()) : U(U(rng()) % U(hi + 1)); }
        // Gap kinds: 0 zero, 1 small, 2 power of two -1/0/+1, 3 uniform up to the top, 4 the top, 5 near the top, 6 near zero, 7 below a power of two.
        static K above(Rng &rng, K lo, int kind) {
            U o = off(lo), room = U(TOP - o), g = 0;
            int j = int(rng() % B);
            if (kind == 1) { g = U(rnd(rng, 1, 16)); }
            else if (kind == 2) { g = U((U(1) << j) + U(rnd(rng, 0, 2)) - U(1)); }
            else if (kind == 3) { g = below(rng, room); }
            else if (kind == 4) { g = room; }
            else if (kind == 5) { g = U(room - min(room, U(rnd(rng, 0, 16)))); }
            else if (kind == 6) {
                U z = off(K(rnd(rng, std::is_signed_v<K> ? -2 : 0, 2)));
                g = z >= o ? U(z - o) : U(0);}
            else if (kind == 7) { g = U(U(rng()) & U((U(1) << j) - U(1))); }
            return at(U(o + min(g, room)));}
    };
    // Profiles: 0 mixed, 1 duplicates, 2 extremes, 3 tiny heap, 4 bucket boundaries.
    int gapKind(Rng &rng, int profile) {
        if (profile == 1) { return rng() % 8 ? 0 : 1; }
        if (profile == 2) { return std::array{0, 3, 4, 5, 6}[rng() % 5]; }
        if (profile == 4) { return std::array{0, 2, 7}[rng() % 3]; }
        return std::array{0, 0, 1, 1, 2, 2, 2, 7, 7, 7, 6, 3, 5, 1, 2, 7}[rng() % 16];}
    int growth(int step) { return std::array{70, 45, 20}[step / 256 % 3]; }

    template<typename K>
    void heapHistory(Rng &rng, int profile, int steps) {
        using KK = Keys<K>;
        RadixHeap<K, int> h;
        std::multiset<pair<K, int>> o;
        K lo = std::numeric_limits<K>::min();
        int cap = profile == 3 ? 3 : 1 << 30, payload = 0;
        bool known = false;
        pair<K, int> t{};
        auto seeTop = [&](const pair<K, int> &got) {
            checkEqual(got.first, o.begin()->first, "top key is the minimum");
            checkEqual(o.find(got) != o.end(), true, "top pair " + show(got) + " is stored");
            t = got; lo = got.first;};
        auto pop = [&]() {
            // A copy's top predicts which pair the original's pop removes.
            if (!known && o.size() <= 64 && rng() % 2) { RadixHeap<K, int> c = h; seeTop(c.top()); }
            else if (!known) { seeTop(h.top()); }
            h.pop(); o.erase(o.find(t));
            known = false;};
        auto state = [&]() {
            checkEqual(h.size(), int(o.size()), "size"); checkEqual(h.empty(), o.empty(), "empty");
            checkEqual(h.last, KK::off(lo), "last");};
        state();
        for (int step = 0; step < steps; ++step) {
            context = "RadixHeap<" + typeName<K>() + "> profile=" + show(profile) + " step=" + show(step) + " size=" + show(o.size()) + " seed=" + show(seed);
            int r = int(rng() % 100), grow = growth(step);
            if (o.empty() || (r < grow && int(o.size()) < cap)) {
                K base = !o.empty() && rng() % 4 == 0 ? o.rbegin()->first : lo;
                K key = KK::above(rng, base, gapKind(rng, profile));
                int val = profile == 1 || rng() % 4 == 0 ? int(rng() % 3) : payload++;
                h.push(key, val); o.emplace(key, val);
                known = false;}
            else if (r < grow + (100 - grow) / 3) { seeTop(h.top()); known = true; }
            else { pop(); }
            state();}
        context = "RadixHeap<" + typeName<K>() + "> profile=" + show(profile) + " drain seed=" + show(seed);
        while (!o.empty()) { pop(); state(); }}

    template<typename K>
    void arrayHistory(Rng &rng, int n, int profile, int steps) {
        using KK = Keys<K>;
        using U = typename KK::U;
        RadixHeapArray<K> h(n);
        std::set<pair<K, int>> o;
        vector<K> ok(n);
        // ids[0, o.size()) are present, the rest absent; where inverts ids.
        vector<int> ids(n), where(n);
        iota(ids.begin(), ids.end(), 0); iota(where.begin(), where.end(), 0);
        K lo = std::numeric_limits<K>::min();
        bool known = false;
        int t = -1;
        auto present = [&](int i) { return where[i] < int(o.size()); };
        auto swapTo = [&](int i, int p) {
            int a = where[i], j = ids[p];
            ids[a] = j; ids[p] = i; where[j] = a; where[i] = p;};
        auto seeTop = [&](int i) {
            checkEqual(0 <= i && i < n && present(i), true, "top id " + show(i) + " is present");
            checkEqual(ok[i], o.begin()->first, "top key is the minimum");
            t = i; lo = ok[i];};
        auto pop = [&]() {
            vector<int> cand;
            for (auto it = o.begin(); !known && it != o.end() && it->first == o.begin()->first && cand.size() <= 64; ++it) { cand.push_back(it->second); }
            if (!known && cand.size() <= 64 && rng() % 2) {
                // Without a preceding top exactly one minimum-key id must vanish.
                h.pop();
                vector<int> gone;
                for (int i : cand) { if (!h.contains(i)) { gone.push_back(i); } }
                checkEqual(gone.size(), size_t(1), "pop removes one minimum-key id");
                t = gone[0]; lo = ok[t];}
            else {
                if (!known) { seeTop(h.top()); }
                h.pop();}
            swapTo(t, int(o.size()) - 1); o.erase({ok[t], t});
            known = false;
            return t;};
        auto checkId = [&](int i) {
            checkEqual(h.contains(i), present(i), "contains(" + show(i) + ")");
            if (present(i)) { checkEqual(h.key(i), ok[i], "key(" + show(i) + ")"); }};
        auto state = [&](int i, bool all) {
            checkEqual(h.size(), int(o.size()), "size"); checkEqual(h.empty(), o.empty(), "empty");
            checkEqual(h.last, KK::off(lo), "last");
            if (i >= 0) { checkId(i); }
            for (int j = 0; all && j < n; ++j) { checkId(j); }};
        state(-1, true);
        for (int step = 0; step < steps; ++step) {
            context = "RadixHeapArray<" + typeName<K>() + "> n=" + show(n) + " profile=" + show(profile) + " step=" + show(step) + " size=" + show(o.size()) + " seed=" + show(seed);
            int r = int(rng() % 100), grow = growth(step), cnt = int(o.size()), i = -1;
            if (!cnt || (r < grow && cnt < n)) {
                i = ids[cnt + int(rng() % ulng(n - cnt))];
                K base = cnt && rng() % 4 == 0 ? o.rbegin()->first : lo;
                K key = KK::above(rng, base, gapKind(rng, profile));
                swapTo(i, cnt); ok[i] = key; o.emplace(key, i);
                h.push(i, key);
                known = false;}
            else if (r < grow + 20) {
                // New key: same, last, uniform in [last, key], one power of two lower, one lower.
                i = ids[rng() % ulng(cnt)];
                U oi = KK::off(ok[i]), d = U(oi - KK::off(lo)), delta = 0;
                int kind = int(rng() % 5);
                if (kind == 1) { delta = d; }
                else if (kind == 2) { delta = KK::below(rng, d); }
                else if (kind == 3) { delta = min(d, U(U(1) << (rng() % KK::B))); }
                else if (kind == 4) { delta = min(d, U(1)); }
                K x = KK::at(U(oi - delta));
                o.erase({ok[i], i}); ok[i] = x; o.emplace(x, i);
                h.decreaseKey(i, x);
                known = false;}
            else if (r < grow + 35) { seeTop(h.top()); known = true; i = t; }
            else { i = pop(); }
            state(i, step % max(1, n / 64) == 0);}
        context = "RadixHeapArray<" + typeName<K>() + "> n=" + show(n) + " profile=" + show(profile) + " drain seed=" + show(seed);
        while (!o.empty()) { state(pop(), false); }
        state(-1, true);}

    // Dijkstra from 0 on a random multigraph with self-loops; oracle: lazy std::priority_queue Dijkstra, itself checked by Bellman-Ford on small graphs.
    template<typename W>
    void dijkstra(Rng &rng, int n, int m, W maxw) {
        using KK = Keys<W>;
        using U = typename KK::U;
        context = "Dijkstra<" + typeName<W>() + "> n=" + show(n) + " m=" + show(m) + " maxw=" + show(maxw) + " seed=" + show(seed);
        vector<int> from(m), to(m), head(n + 1);
        vector<W> w(m);
        for (int e = 0; e < m; ++e) {
            from[e] = int(rnd(rng, 0, n - 1)); to[e] = int(rnd(rng, 0, n - 1)); w[e] = W(KK::below(rng, U(maxw)));
            ++head[from[e] + 1];}
        for (int u = 0; u < n; ++u) { head[u + 1] += head[u]; }
        vector<int> adj(m), fill = head;
        for (int e = 0; e < m; ++e) { adj[fill[from[e]]++] = e; }
        const W INF = std::numeric_limits<W>::max();
        vector<W> want(n, INF), lazy(n, INF), arr(n, INF);
        priority_queue<pair<W, int>, vector<pair<W, int>>, std::greater<>> pq;
        want[0] = 0; pq.emplace(0, 0);
        while (!pq.empty()) {
            auto [d, u] = pq.top();
            pq.pop();
            if (d > want[u]) { continue; }
            for (int k = head[u]; k < head[u + 1]; ++k) {
                int e = adj[k];
                if (d + w[e] < want[to[e]]) { want[to[e]] = d + w[e]; pq.emplace(want[to[e]], to[e]); }}}
        if (n <= 64) {
            vector<W> bf(n, INF);
            bf[0] = 0;
            for (int round = 0; round < n; ++round) {
                for (int e = 0; e < m; ++e) {
                    if (bf[from[e]] != INF && bf[from[e]] + w[e] < bf[to[e]]) { bf[to[e]] = bf[from[e]] + w[e]; }}}
            checkVec(want, bf, "priority_queue oracle vs Bellman-Ford");}
        RadixHeap<W, int> h;
        lazy[0] = 0; h.push(0, 0);
        while (!h.empty()) {
            auto [d, u] = h.top();
            h.pop();
            if (d > lazy[u]) { continue; }
            for (int k = head[u]; k < head[u + 1]; ++k) {
                int e = adj[k];
                if (d + w[e] < lazy[to[e]]) { lazy[to[e]] = d + w[e]; h.push(lazy[to[e]], to[e]); }}}
        checkVec(lazy, want, "RadixHeap Dijkstra");
        RadixHeapArray<W> a(n);
        arr[0] = 0; a.push(0, 0);
        while (!a.empty()) {
            int u = a.top();
            W d = a.key(u);
            a.pop();
            for (int k = head[u]; k < head[u + 1]; ++k) {
                int e = adj[k], v = to[e];
                if (d + w[e] >= arr[v]) { continue; }
                arr[v] = d + w[e];
                if (a.contains(v)) { a.decreaseKey(v, arr[v]); }
                else { a.push(v, arr[v]); }}}
        checkVec(arr, want, "RadixHeapArray Dijkstra");}

    // Heap sort against std::sort; every popped payload or id is seen once and carries its own key.
    template<typename K>
    void heapSort(Rng &rng, int n, bool narrow) {
        using KK = Keys<K>;
        using U = typename KK::U;
        context = "heapSort<" + typeName<K>() + "> n=" + show(n) + " narrow=" + show(narrow) + " seed=" + show(seed);
        U base = KK::below(rng, U(KK::TOP - 1000));
        vector<K> keys(n);
        for (K &x : keys) { x = narrow ? KK::at(U(base + U(rng() % 1000))) : KK::at(U(rng())); }
        vector<K> want = keys, got;
        sort(want.begin(), want.end());
        vector<char> seen(n);
        RadixHeap<K, int> h;
        for (int i = 0; i < n; ++i) { h.push(keys[i], i); }
        while (!h.empty()) {
            auto [x, i] = h.top();
            h.pop();
            if (i < 0 || i >= n || seen[i] || keys[i] != x) { checkEqual(true, false, "RadixHeap payload " + show(i) + " of key " + show(x)); }
            seen[i] = 1; got.push_back(x);}
        checkVec(got, want, "RadixHeap order");
        RadixHeapArray<K> a(n);
        for (int i = 0; i < n; ++i) { a.push(i, keys[i]); }
        got.clear();
        while (!a.empty()) {
            int i = a.top();
            if (i < 0 || i >= n || !seen[i] || a.key(i) != keys[i]) { checkEqual(true, false, "RadixHeapArray id " + show(i)); }
            seen[i] = 0; got.push_back(keys[i]);
            a.pop();
            if (a.contains(i)) { checkEqual(true, false, "popped id still contained"); }}
        checkVec(got, want, "RadixHeapArray order");}

    template<typename K>
    void edgesFor() {
        using KK = Keys<K>;
        using U = typename KK::U;
        using L = std::numeric_limits<K>;
        context = "edges<" + typeName<K>() + ">";
        RadixHeap<K, string> h;
        checkEqual(h.empty(), true, "empty"); checkEqual(h.size(), 0, "size 0"); checkEqual(h.last, KK::off(L::min()), "initial last is the least K");
        // Extremes pushed out of order; long payload strings are moved between buckets.
        vector<K> ex{L::max(), K(0), L::min(), K(1), K(L::max() - 1), K(L::min() + 1), K(L::max() / 2), K(L::max() / 2 + 1), K(2), L::max()};
        for (K x : {K(-1), K(-2), K(L::min() / 2), K(L::min() / 2 - 1)}) { if (std::is_signed_v<K>) { ex.push_back(x); } }
        auto tag = [](K x) { return string(40, 'p') + show(x); };
        for (K x : ex) { h.push(x, tag(x)); }
        checkEqual(h.size(), int(ex.size()), "size after pushes");
        RadixHeap<K, string> copy = h;
        sort(ex.begin(), ex.end());
        auto drain = [&](RadixHeap<K, string> &g, const string &op) {
            for (K x : ex) {
                const pair<K, string> &p = g.top();
                checkEqual(p.first, x, op + " key"); checkEqual(p.second, tag(x), op + " payload");
                g.pop();}
            checkEqual(g.empty(), true, op + " empty");};
        drain(h, "drain");
        checkEqual(copy.size(), int(ex.size()), "copy unaffected by the original");
        drain(copy, "drain copy");
        // After a full drain last stays at the maximum, and a key equal to it is still valid.
        checkEqual(h.last, KK::off(L::max()), "last after drain");
        h.push(L::max(), "again");
        checkEqual(h.top().second, string("again"), "push == last after drain");
        h.pop();
        // A key equal to last pushed between top and pop.
        RadixHeap<K, int> g;
        g.push(K(5), 1); g.push(K(7), 2); g.push(K(5), 1);
        checkEqual(g.top(), pair<K, int>(K(5), 1), "top before push == last");
        g.push(K(5), 3);
        checkEqual(g.size(), 4, "size after push == last");
        vector<int> vals;
        for (int k = 0; k < 3; ++k) {
            checkEqual(g.top().first, K(5), "equal keys first");
            vals.push_back(g.top().second);
            g.pop();}
        sort(vals.begin(), vals.end());
        checkEqual(vals, vector<int>{1, 1, 3}, "duplicate payloads popped");
        checkEqual(g.top(), pair<K, int>(K(7), 2), "then the larger key");
        checkEqual(g.size(), 1, "size after mixed ops");
        // RadixHeapArray: n = 0, default, n = 1 with decreaseKey to the same key and to last, then id reuse.
        RadixHeapArray<K> z(0), d;
        checkEqual(z.size() + d.size(), 0, "n = 0 size"); checkEqual(z.empty() && d.empty(), true, "n = 0 empty");
        RadixHeapArray<K> one(1);
        checkEqual(one.contains(0), false, "fresh id absent");
        one.push(0, L::max());
        checkEqual(one.contains(0), true, "pushed id present"); checkEqual(one.key(0), L::max(), "key max");
        one.decreaseKey(0, L::max());
        checkEqual(one.key(0), L::max(), "decreaseKey to the same key");
        one.decreaseKey(0, K(1));
        one.decreaseKey(0, L::min());
        checkEqual(one.key(0), L::min(), "decreaseKey to last");
        checkEqual(one.top(), 0, "top of one id");
        one.pop();
        checkEqual(one.contains(0), false, "popped id absent"); checkEqual(one.empty(), true, "empty after pop");
        one.push(0, L::min());
        checkEqual(one.key(0), L::min(), "id reuse with key == last");
        // All ids present, copied, then decreased across buckets in the original only.
        int n = 1000;
        RadixHeapArray<K> a(n);
        vector<K> keys(n);
        for (int i = 0; i < n; ++i) { keys[i] = KK::at(U(KK::TOP - U(i) * U(97))); a.push(i, keys[i]); }
        RadixHeapArray<K> b = a;
        for (int i = 0; i < n; i += 3) { keys[i] = KK::at(U(KK::off(keys[i]) >> (i % KK::B))); a.decreaseKey(i, keys[i]); }
        checkEqual(a.size(), n, "all ids present");
        vector<int> order(n);
        iota(order.begin(), order.end(), 0);
        auto check = [&](RadixHeapArray<K> &g, const vector<K> &ks, const string &op) {
            vector<K> want = ks, got;
            sort(want.begin(), want.end());
            while (!g.empty()) {
                int i = g.top();
                got.push_back(g.key(i));
                if (ks[i] != g.key(i)) { checkEqual(g.key(i), ks[i], op + " key(" + show(i) + ")"); }
                g.pop();}
            checkVec(got, want, op);};
        check(a, keys, "all ids drained");
        for (int i = 0; i < n; ++i) { keys[i] = KK::at(U(KK::TOP - U(i) * U(97))); }
        check(b, keys, "copy unaffected by decreaseKey");}

    void invalid(const string &name) {
        RadixHeap<lng, int> h;
        RadixHeapArray<lng> a(4);
        if (name == "heap-push-below-last-after-pop") { h.push(5, 0); h.push(9, 0); h.pop(); h.push(4, 0); }
        else if (name == "heap-push-below-last-after-top") { h.push(5, 0); (void)h.top(); h.push(4, 0); }
        else if (name == "heap-push-below-last-drained") { h.push(5, 0); h.pop(); h.push(4, 0); }
        else if (name == "heap-push-below-last-negative") { RadixHeap<int, int> g; g.push(-5, 0); g.pop(); g.push(-6, 0); }
        else if (name == "heap-push-below-last-unsigned") { RadixHeap<ulng, int> g; g.push(ULLONG_MAX, 0); (void)g.top(); g.push(ULLONG_MAX - 1, 0); }
        else if (name == "heap-top-empty") { (void)h.top(); }
        else if (name == "heap-pop-empty") { h.pop(); }
        else if (name == "heap-pop-drained") { h.push(1, 0); h.pop(); h.pop(); }
        else if (name == "array-push-below-last-after-pop") { a.push(0, 5); a.push(1, 9); a.pop(); a.push(2, 4); }
        else if (name == "array-push-below-last-after-top") { a.push(0, 5); (void)a.top(); a.push(1, 4); }
        else if (name == "array-top-empty") { (void)a.top(); }
        else if (name == "array-pop-empty") { a.pop(); }
        else if (name == "array-contains-range") { (void)a.contains(4); }
        else if (name == "array-contains-negative") { (void)a.contains(-1); }
        else if (name == "array-push-range") { a.push(4, 0); }
        else if (name == "array-key-absent") { (void)a.key(0); }
        else if (name == "array-key-popped") { a.push(0, 1); a.pop(); (void)a.key(0); }
        else if (name == "array-push-present") { a.push(0, 1); a.push(0, 2); }
        else if (name == "array-decrease-absent") { a.decreaseKey(0, 1); }
        else if (name == "array-decrease-above") { a.push(0, 5); a.decreaseKey(0, 6); }
        else if (name == "array-decrease-below-last") { a.push(0, 5); a.push(1, 9); (void)a.top(); a.decreaseKey(1, 4); }
        else if (name == "array-size-negative") { RadixHeapArray<lng> bad(-1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}

    // Other 32/64-bit spellings (long long, unsigned long long), move-only payloads, pop after a decreaseKey to last following top.
    void spellings() {
        context = "spellings";
        RadixHeap<long long, std::unique_ptr<int>> h;
        h.push(LLONG_MAX, std::make_unique<int>(1)); h.push(-3, std::make_unique<int>(2)); h.push(LLONG_MIN, std::make_unique<int>(3));
        vector<pair<long long, int>> got;
        while (!h.empty()) { got.emplace_back(h.top().first, *h.top().second); h.pop(); }
        checkEqual(got == vector<pair<long long, int>>{{LLONG_MIN, 3}, {-3, 2}, {LLONG_MAX, 1}}, true, "long long keys with unique_ptr payloads");
        RadixHeapArray<unsigned long long> a(3);
        a.push(0, 5); a.push(1, 5); a.push(2, 9);
        checkEqual(a.key(a.top()), 5ULL, "top key");
        a.decreaseKey(2, 5);
        int t = a.top();
        a.pop();
        checkEqual(a.contains(t), false, "pop removes the id top returns after decreaseKey to last");
        checkEqual(a.size(), 2, "two ids left");
        RadixHeapArray<long long> b(2);
        b.push(1, -7); b.push(0, LLONG_MIN);
        checkEqual(b.top(), 0, "long long array order");
        std::cout << "PASS spellings\n";}

    static_assert(radixheap_detail::Key<long> && radixheap_detail::Key<unsigned long> && radixheap_detail::Key<long long> && radixheap_detail::Key<unsigned long long>);
    static_assert(!radixheap_detail::Key<wchar_t> && !radixheap_detail::Key<char32_t> && !radixheap_detail::Key<short> && !radixheap_detail::Key<bool> && !radixheap_detail::Key<lll> && !radixheap_detail::Key<const int> && !radixheap_detail::Key<volatile lng>);

    // A cascade through every bucket (n copies of the top key, then one key per bucket boundary) must not retain O(n * b) capacity.
    void memoryBound() {
        context = "memoryBound";
        int n = 1 << 17;
        size_t bound = 4 * size_t(n) + 65 * 4096;
        RadixHeap<ulng, int> h;
        RadixHeapArray<ulng> a(n + 64);
        for (int i = 0; i < n; ++i) { h.push(ULLONG_MAX, i); a.push(i, ULLONG_MAX); }
        for (int j = 63; j >= 1; --j) {
            ulng x = ~0ULL << j;
            h.push(x, -j); a.push(n + j, x);
            checkEqual(h.top().first, x, "cascade top"); h.pop();
            checkEqual(a.key(a.top()), x, "cascade array top"); a.pop();}
        size_t ch = 0, ca = 0;
        for (auto &w : h.v) { ch += w.capacity(); }
        for (auto &w : a.v) { ca += w.capacity(); }
        checkEqual(ch <= bound, true, "RadixHeap retained capacity " + show(ch));
        checkEqual(ca <= bound, true, "RadixHeapArray retained capacity " + show(ca));
        checkEqual(h.size(), n, "cascade size"); checkEqual(a.size(), n, "cascade array size");
        std::cout << "PASS memoryBound\n";}

    template<typename K>
    void suite(Rng &rng, int steps, const vector<int> &ns, int sortN) {
        edgesFor<K>();
        for (int profile = 0; profile < 5; ++profile) {
            heapHistory<K>(rng, profile, steps);
            for (int n : ns) { arrayHistory<K>(rng, n, profile, n <= 3 ? min(steps, 1000) : steps); }}
        heapSort<K>(rng, sortN, false); heapSort<K>(rng, sortN, true);
        std::cout << "PASS " << typeName<K>() << '\n';}
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
        std::cout << "radixheap seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        bool quick = mode == "quick", stress = mode == "stress";
        int steps = quick ? 300 : stress ? (SLOW ? 10000 : 30000) : 4000, sortN = quick ? 10000 : (stress ? 4000000 : 1000000) / (CHECKED ? 10 : 1);
        vector<int> ns = quick ? vector<int>{1, 2, 3, 64, 300} : vector<int>{1, 2, 3, 64, 65, 300, 5000};
        if (stress) { ns.push_back(50000); }
        suite<int>(rng, steps, ns, sortN); suite<lng>(rng, steps, ns, sortN);
        suite<uint>(rng, steps, ns, sortN); suite<ulng>(rng, steps, ns, sortN);
        spellings(); memoryBound();
        for (int n : {1, 2, 5, 64}) {
            for (int m : {0, n / 2, 4 * n}) {
                for (lng maxw : {1LL, 1000LL, 1LL << 40, LLONG_MAX / n}) { dijkstra<lng>(rng, n, m, maxw); }
                for (ulng maxw : {0ULL, 1ULL, 1ULL << 33, ULLONG_MAX / ulng(n)}) { dijkstra<ulng>(rng, n, m, maxw); }}}
        int big = quick || CHECKED ? 2000 : stress ? 1000000 : 200000;
        for (int m : {big / 2, 4 * big}) {
            dijkstra<lng>(rng, big, m, 1000); dijkstra<lng>(rng, big, m, LLONG_MAX / big);
            dijkstra<ulng>(rng, big, m, 1); dijkstra<ulng>(rng, big, m, ULLONG_MAX / ulng(big));}
        std::cout << "PASS Dijkstra\n";
        std::cout << "PASS radixheap checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL radixheap seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
