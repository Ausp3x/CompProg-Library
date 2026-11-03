#include "../../04-Graphs/13-shortest_path_advanced.hpp"
#include <sys/resource.h>

// Usage: benchmark kind n w seed reps; lines "graph n m", "run engine rep seconds" (rep -1 is the warmup), "checksum c", "memory kb".
namespace {
    // T: O(n + m), M: O(n + m); kinds potential, chain and antififo, all free of negative cycles.
    Graph build(const string &kind, int n, ulng w, std::mt19937_64 &rng) {
        Graph g(n, true);
        auto pick = [&](ulng k) { return ulng(rng() % k); };
        if (kind == "potential") {
            vector<lng> h(n);
            for (auto &x : h) { x = lng(pick(2 * w + 1)) - lng(w); }
            for (int i = 0; i < 3 * n; ++i) {
                int u = int(pick(ulng(n))), v = int(pick(ulng(n)));
                g.addEdge(u, v, h[u] - h[v] + lng(pick(3)));}}
        else if (kind == "chain") {
            for (int v = n - 1; v > 0; --v) { g.addEdge(v - 1, v, -1); }
            while (int(g.arcs.size()) < 3 * n - 1) {
                int u = int(pick(ulng(n))), v = int(pick(ulng(n)));
                if (u > v) { swap(u, v); }
                if (u < v) { g.addEdge(u, v, -(v - u) + lng(pick(3))); }}}
        else {
            // Star from 0 in descending order, then the negative chain 1 -> ... -> n - 1 in reverse arc order, then never-relaxing filler.
            for (int v = n - 1; v > 0; --v) { g.addEdge(0, v, 0); }
            for (int v = n - 1; v > 1; --v) { g.addEdge(v - 1, v, -lng(1 + pick(w))); }
            lng huge = lng(n) * lng(w);
            for (int i = 0; i < n; ++i) { g.addEdge(1 + int(pick(ulng(n - 1))), 1 + int(pick(ulng(n - 1))), huge + lng(pick(w))); }}
        return g;}

    // T: O(n + m), M: O(1); exits unless every reachable arc is feasible and every reachable non-source vertex has a tight parent arc.
    void certify(const Graph &g, const ShortestPathResult &r) {
        bool ok = r.reachable[0] && r.dist[0] == 0;
        for (const auto &e : g.arcs) { if (r.reachable[e.from]) { ok &= r.reachable[e.to] && r.dist[e.to] <= r.dist[e.from] + e.w; }}
        for (int v = 0; v < g.n; ++v) {
            ok &= !r.negative[v];
            if (v != 0 && r.reachable[v]) {
                int a = r.parent_arc[v];
                ok &= a != -1 && g.arcs[a].to == v && r.reachable[g.arcs[a].from] && r.dist[g.arcs[a].from] + g.arcs[a].w == r.dist[v];}}
        if (!ok) { cerr << "certificate failed\n"; std::exit(1); }}
} // namespace

int main(int argc, char **argv) {
    if (argc != 6) { cerr << "usage: benchmark kind n w seed reps\n"; return 2; }
    string kind = argv[1];
    int n = std::stoi(argv[2]), reps = std::stoi(argv[5]);
    ulng w = std::stoull(argv[3]);
    std::mt19937_64 rng(std::stoull(argv[4]));
    Graph g = build(kind, n, w, rng);
    cout << "graph " << g.n << ' ' << g.arcs.size() << '\n';
    const char *names[] = {"negativeSsspScaling", "spfa", "bellmanFord"};
    auto engine = [&](int k) {
        if (k == 0) { return negativeSsspScaling(g, {0}); }
        if (k == 1) { return spfa(g, {0}); }
        return bellmanFord(g, {0});};
    ShortestPathResult ref;
    for (int rep = -1; rep < reps; ++rep) {
        for (int k = 0; k < 3; ++k) {
            auto start = std::chrono::steady_clock::now();
            auto res = engine(k);
            double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
            if (rep == -1 && k == 0) { certify(g, res); ref = res; }
            if (res.reachable != ref.reachable || res.dist != ref.dist) { cerr << "mismatch " << names[k] << " rep " << rep << '\n'; return 1; }
            cout << "run " << names[k] << ' ' << rep << ' ' << s << std::endl;}}
    lll sum = 0; int reached = 0;
    for (int v = 0; v < n; ++v) {
        if (ref.reachable[v]) { sum += ref.dist[v]; ++reached; }}
    cout << "checksum " << lng(sum % 1000000007) << ' ' << reached << '\n';
    rusage use{}; getrusage(RUSAGE_SELF, &use);
    cout << "memory " << use.ru_maxrss << '\n';}
