#include "../../02-Data Structures/25-heap_deque.hpp"

// Emits one JSON line per (workload, implementation); checksums must agree within a workload.
namespace {
    using Clock = std::chrono::steady_clock;

    void emit(const string &workload, const string &impl, int n, double sec, ulng sum) {
        std::cout << "{\"workload\":\"" << workload << "\",\"impl\":\"" << impl << "\",\"n\":" << n
                  << ",\"seconds\":" << sec << ",\"checksum\":" << sum << "}\n";}
    template<typename F>
    void timed(const string &workload, const string &impl, int n, F f) {
        auto t0 = Clock::now();
        ulng sum = f();
        emit(workload, impl, n, std::chrono::duration<double>(Clock::now() - t0).count(), sum);}

    // n random pushes, then n pops; the checksum mixes the pop order.
    template<typename H>
    ulng pushPopMeld(const vector<lng> &a) {
        H h;
        int v = 0;
        for (lng x : a) { h.push(v, x); }
        ulng sum = 0;
        while (v) { sum = sum * 31 + ulng(h.top(v)); h.pop(v); }
        return sum;}
    template<int D>
    ulng pushPopDary(const vector<lng> &a) {
        int n = int(a.size());
        DAryHeap<lng, D> h(n);
        for (int i = 0; i < n; ++i) { h.push(i, a[i]); }
        ulng sum = 0;
        while (!h.empty()) { sum = sum * 31 + ulng(h.key(h.top())); h.pop(); }
        return sum;}
    ulng pushPopStd(const vector<lng> &a) {
        std::priority_queue<lng, vector<lng>, std::greater<lng>> q;
        for (lng x : a) { q.push(x); }
        ulng sum = 0;
        while (!q.empty()) { sum = sum * 31 + ulng(q.top()); q.pop(); }
        return sum;}
    ulng pushPopDepq(const vector<lng> &a) {
        DoubleEndedPriorityQueue<lng> q;
        for (lng x : a) { q.push(x); }
        ulng sum = 0;
        while (!q.empty()) { sum = sum * 31 + ulng(q.min()); q.popMin(); }
        return sum;}

    struct Graph { int n; vector<int> start, to; vector<lng> w; };
    Graph randomGraph(int n, int m, ulng seed) {
        std::mt19937_64 rng(seed);
        vector<std::array<lng, 3>> e(m);
        for (auto &[u, v, w] : e) { u = lng(rng() % ulng(n)); v = lng(rng() % ulng(n)); w = lng(rng() % 1000000000) + 1; }
        Graph g{n, vector<int>(n + 1), vector<int>(m), vector<lng>(m)};
        for (auto &x : e) { ++g.start[x[0] + 1]; }
        for (int i = 0; i < n; ++i) { g.start[i + 1] += g.start[i]; }
        vector<int> fill(g.start.begin(), g.start.end() - 1);
        for (auto &[u, v, w] : e) { int k = fill[u]++; g.to[k] = int(v); g.w[k] = w; }
        return g;}
    ulng distanceSum(const vector<lng> &d) {
        ulng sum = 0;
        for (lng x : d) { sum = sum * 31 + ulng(x); }
        return sum;}
    template<int D>
    ulng dijkstraDary(const Graph &g) {
        vector<lng> d(g.n, -1);
        DAryHeap<lng, D> h(g.n);
        h.push(0, 0);
        while (!h.empty()) {
            int u = h.top();
            d[u] = h.key(u); h.pop();
            for (int k = g.start[u]; k < g.start[u + 1]; ++k) {
                int v = g.to[k];
                lng nd = d[u] + g.w[k];
                if (d[v] >= 0) { continue; }
                if (!h.contains(v)) { h.push(v, nd); } else if (nd < h.key(v)) { h.decreaseKey(v, nd); }}}
        return distanceSum(d);}
    // Each vertex is pushed at most once, so node ids stay below n + 1 and vert maps them back.
    template<typename H>
    ulng dijkstraHandles(const Graph &g) {
        vector<lng> d(g.n, -1), best(g.n, -1);
        vector<int> node(g.n), vert(g.n + 1);
        H h;
        int root = 0;
        auto add = [&](int v, lng x) { node[v] = h.push(root, x); vert[node[v]] = v; best[v] = x; };
        add(0, 0);
        while (root) {
            int u = vert[root];
            d[u] = h.top(root); h.pop(root);
            for (int k = g.start[u]; k < g.start[u + 1]; ++k) {
                int v = g.to[k];
                lng nd = d[u] + g.w[k];
                if (d[v] >= 0) { continue; }
                if (best[v] < 0) { add(v, nd); } else if (nd < best[v]) { best[v] = nd; h.decreaseKey(root, node[v], nd); }}}
        return distanceSum(d);}
    ulng dijkstraStd(const Graph &g) {
        vector<lng> d(g.n, -1);
        std::priority_queue<pair<lng, int>, vector<pair<lng, int>>, std::greater<>> q;
        q.push({0, 0});
        while (!q.empty()) {
            auto [du, u] = q.top(); q.pop();
            if (d[u] >= 0) { continue; }
            d[u] = du;
            for (int k = g.start[u]; k < g.start[u + 1]; ++k) {
                if (d[g.to[k]] < 0) { q.push({du + g.w[k], g.to[k]}); }}}
        return distanceSum(d);}

    // n / 16 heaps of 16 keys melded in rounds of pairs, then drained.
    template<typename H>
    ulng meldTournament(const vector<lng> &a) {
        H h;
        vector<int> roots;
        for (int i = 0; i < int(a.size()); i += 16) {
            int v = 0;
            for (int j = i; j < min(i + 16, int(a.size())); ++j) { h.push(v, a[j]); }
            roots.push_back(v);}
        while (roots.size() > 1) {
            vector<int> next;
            for (int i = 0; i + 1 < int(roots.size()); i += 2) { next.push_back(h.meld(roots[i], roots[i + 1])); }
            if (roots.size() % 2) { next.push_back(roots.back()); }
            roots = next;}
        int v = roots.empty() ? 0 : roots[0];
        ulng sum = 0;
        while (v) { sum = sum * 31 + ulng(h.top(v)); h.pop(v); }
        return sum;}
} // namespace

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261013;
    std::mt19937_64 rng(seed);
    for (int n : {1000, 100000, 1000000}) {
        vector<lng> a(n);
        for (auto &x : a) { x = lng(rng() % 1000000000000ULL); }
        string w = "pushpop";
        timed(w, "std::priority_queue", n, [&] { return pushPopStd(a); });
        timed(w, "BinaryHeap", n, [&] { return pushPopDary<2>(a); });
        timed(w, "DAryHeap<4>", n, [&] { return pushPopDary<4>(a); });
        timed(w, "DAryHeap<8>", n, [&] { return pushPopDary<8>(a); });
        timed(w, "LeftistHeap", n, [&] { return pushPopMeld<LeftistHeap<lng>>(a); });
        timed(w, "SkewHeap", n, [&] { return pushPopMeld<SkewHeap<lng>>(a); });
        timed(w, "RandomizedHeap", n, [&] { return pushPopMeld<RandomizedHeap<lng>>(a); });
        timed(w, "BinomialHeap", n, [&] { return pushPopMeld<BinomialHeap<lng>>(a); });
        timed(w, "PairingHeap", n, [&] { return pushPopMeld<PairingHeap<lng>>(a); });
        timed(w, "FibonacciHeap", n, [&] { return pushPopMeld<FibonacciHeap<lng>>(a); });
        timed(w, "DoubleEndedPriorityQueue", n, [&] { return pushPopDepq(a); });
        w = "meld";
        timed(w, "LeftistHeap", n, [&] { return meldTournament<LeftistHeap<lng>>(a); });
        timed(w, "SkewHeap", n, [&] { return meldTournament<SkewHeap<lng>>(a); });
        timed(w, "RandomizedHeap", n, [&] { return meldTournament<RandomizedHeap<lng>>(a); });
        timed(w, "BinomialHeap", n, [&] { return meldTournament<BinomialHeap<lng>>(a); });
        timed(w, "PairingHeap", n, [&] { return meldTournament<PairingHeap<lng>>(a); });
        timed(w, "FibonacciHeap", n, [&] { return meldTournament<FibonacciHeap<lng>>(a); });}
    for (auto [n, m] : {pair{1000, 5000}, pair{200000, 1000000}, pair{200000, 4000000}}) {
        Graph g = randomGraph(n, m, seed ^ ulng(m));
        string w = "dijkstra m=" + std::to_string(m);
        timed(w, "std::priority_queue (lazy)", n, [&] { return dijkstraStd(g); });
        timed(w, "BinaryHeap", n, [&] { return dijkstraDary<2>(g); });
        timed(w, "DAryHeap<4>", n, [&] { return dijkstraDary<4>(g); });
        timed(w, "DAryHeap<8>", n, [&] { return dijkstraDary<8>(g); });
        timed(w, "PairingHeap", n, [&] { return dijkstraHandles<PairingHeap<lng>>(g); });
        timed(w, "FibonacciHeap", n, [&] { return dijkstraHandles<FibonacciHeap<lng>>(g); });}}
