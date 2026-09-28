#include "../../04-Graphs/05-shortest_path.hpp"
#include "../../04-Graphs/06-mst.hpp"

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20260927;
    std::mt19937_64 rng(seed);
    for (int n : {24, 200, 800}) {
        for (string shape : {"chain", "sparse", "dense", "disconnected"}) {
            Graph g(n);
            if (shape == "dense") {
                for (int u = 0; u < n; ++u) { for (int v = u + 1; v < n; ++v) {
                    g.addEdge(u, v, lng(rng() % 1000001)); }}}
            else {
                for (int v = 1; v < n; ++v) {
                    if (shape != "disconnected" || v % 8) { g.addEdge(v - 1, v, lng(rng() % 1000001)); }}
                if (shape == "sparse") {
                    for (int i = 0; i < 4 * n; ++i) {
                        g.addEdge(int(rng() % n), int(rng() % n), lng(rng() % 1000001)); }}}
            DenseGraph dense(g);
            auto reference = dijkstra(g, 0); auto expected = kruskal(g);
            auto check_path = [&] (const auto &result) -> lng {
                if (result.reachable != reference.reachable || result.dist != reference.dist) { std::abort(); }
                lll checksum = 0;
                for (int u = 0; u < n; ++u) { if (result.reachable[u]) { checksum += result.dist[u]; }}
                return lng(checksum); // n<=800 and weights<=10^6, so checksum fits lng.
            };
            auto check_mst = [&] (const auto &result) -> lng {
                if (result.weight != expected.weight || result.components != expected.components ||
                    int(result.edges.size()) != n - expected.components) { std::abort(); }
                return lng(result.weight);
            };
            check_path(dijkstraDense(dense, 0)); check_mst(primSparse(g)); check_mst(primDense(dense));
            int repeats = n == 24 ? 200 : n == 200 ? 8 : 1;
            auto measure = [&] (auto &&operation, auto &&check) -> pair<double, lng> {
                lng checksum = 0; auto start = std::chrono::steady_clock::now();
                for (int k = 0; k < repeats; ++k) { checksum += check(operation()); }
                double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                return {ms / repeats, checksum};
            };
            for (int run = 0; run < 5; ++run) {
                auto a = measure([&] { return dijkstra(g, 0); }, check_path);
                auto b = measure([&] { return dijkstraDense(dense, 0); }, check_path);
                auto c = measure([&] { return primSparse(g); }, check_mst);
                auto d = measure([&] { return primDense(dense); }, check_mst);
                auto start = std::chrono::steady_clock::now(); DenseGraph rebuilt(g);
                double setup = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                check_path(dijkstraDense(rebuilt, 0));
                std::cout << n << ' ' << g.edges.size() << ' ' << shape << ' ' << run << ' ' << repeats
                          << ' ' << a.first << ' ' << b.first << ' ' << c.first << ' ' << d.first
                          << ' ' << setup << ' ' << a.second << ' ' << c.second << '\n';}}}
}
