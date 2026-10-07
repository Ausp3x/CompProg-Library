#include "../../04-Graphs/01-graph.hpp"

// Repeated BFS kernel used only for representation timing; each result is checked.
template<class G>
lng traverse(const G &g) {
    vector<int> dis(g.n, -1), q; q.reserve(g.n);
    lng sum = 0;
    for (int s = 0; s < g.n; ++s) {
        if (dis[s] != -1) { continue; }
        dis[s] = 0; q.push_back(s);
        for (int i = int(q.size()) - 1; i < int(q.size()); ++i) {
            int u = q[i]; sum += dis[u];
            for (int a : g[u]) {
                int v = g.arcs[a].to;
                if (dis[v] == -1) { dis[v] = dis[u] + 1; q.push_back(v); }}}}
    return sum;}

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    for (int n : {32, 3000, 100000}) {
        for (string shape : {"chain", "star", "random"}) {
            vector<pair<int, int>> edges;
            for (int v = 1; v < n; ++v) { edges.push_back({shape == "star" ? 0 : v - 1, v}); }
            if (shape == "random") {
                for (int i = 0; i < 4 * n; ++i) { edges.push_back({int(rng() % n), int(rng() % n)}); }}
            Graph g(n);
            for (auto [u, v] : edges) { g.addEdge(u, v); }
            CsrGraph csr(g);
            lng expected = traverse(g);
            if (traverse(csr) != expected) { return 1; }
            int repeats = max(1, 200000 / (n + int(edges.size())));
            auto measure = [&] (auto &&f) -> double {
                auto start = std::chrono::steady_clock::now();
                for (int k = 0; k < repeats; ++k) {
                    if (f() != expected) { std::abort(); }}
                return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / repeats;};
            for (int run = 0; run < 5; ++run) {
                double list = measure([&] { return traverse(g); });
                double compact = measure([&] { return traverse(csr); });
                auto start = std::chrono::steady_clock::now();
                CsrGraph rebuilt(g);
                double setup = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                if (traverse(rebuilt) != expected) { return 2; }
                std::cout << n << ' ' << edges.size() << ' ' << shape << ' ' << run << ' '
                          << repeats << ' ' << list << ' ' << compact << ' ' << setup << ' ' << expected << '\n';}}}}
