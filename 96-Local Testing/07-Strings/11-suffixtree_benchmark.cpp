#include "../../07-Strings/11-suffixtree.hpp"

using Clock = std::chrono::steady_clock;
// Rival O(n * log(n)) structure: a sparse table over the same split map.
struct SparseLca {
    const SuffixTree<int> *t;
    SparseTable<Min<int>> table;
    explicit SparseLca(const SuffixTree<int> &tree) : t(&tree), table(tree.split) {}
    int lca(int u, int v) const {
        if (u > v) { swap(u, v); }
        if (v < t->nodes[u].end) { return u; }
        return table.prod(t->nodes[u].l, t->nodes[v].l);}
};
// Rival composition: the LCE through the SuffixArray RMQ, then the jump-pointer weighted ancestor.
int composedLca(const SuffixTree<int> &t, int u, int v) {
    const auto &x = t.nodes[u], &y = t.nodes[v];
    return t.weightedAncestor(u, min({x.depth, y.depth, t.index.lce(t.index.sa[x.l], t.index.sa[y.l])}));}

int reps = 5;
bool first = true;
void emit(const string &w, const string &method, double value, lng sum) {
    cout << (first ? "" : ",") << "{\"workload\":\"" << w << "\",\"method\":\"" << method << "\",\"value\":" << value << ",\"checksum\":" << sum << "}";
    first = false;}
// One warmup, then the median of reps timed runs; setup() runs untimed before each.
template<class Setup, class Run> pair<double, lng> measure(Setup &&setup, Run &&run) {
    vector<double> samples;
    lng sum = 0;
    for (int rep = -1; rep < reps; ++rep) {
        setup();
        auto start = Clock::now();
        sum = run();
        double ms = std::chrono::duration<double, std::milli>(Clock::now() - start).count();
        if (rep >= 0) { samples.push_back(ms); }}
    sort(samples.begin(), samples.end());
    return {samples[reps / 2], sum};}

int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261019;
    reps = argc > 2 ? std::stoi(argv[2]) : 5;
    int n = 1000000, q = 1000000;
    std::mt19937_64 rng(seed);
    vector<pair<string, vector<int>>> workloads;
    for (int sigma : {2, 26}) {
        vector<int> s(n);
        for (int &c : s) { c = int(rng() % sigma); }
        workloads.push_back({"random S=" + std::to_string(sigma), s});}
    vector<int> a{0}, b{1};
    while (int(a.size()) < n) { vector<int> c = a; c.insert(c.end(), b.begin(), b.end()); b = a; a = c; }
    a.resize(n);
    workloads.push_back({"fibonacci", a});
    workloads.push_back({"unary", vector<int>(n, 0)});
    auto none = [] {};
    cout << "[";
    for (auto &[name, s] : workloads) {
        SuffixArray<int> sa(s), copy;
        auto [build, nodes] = measure([&] { copy = sa; }, [&] { return lng(SuffixTree<int>(std::move(copy)).nodes.size()); });
        emit(name, "tree build ms", build, nodes);
        SuffixTree<int> t(sa);
        auto [sparse_build, levels] = measure(none, [&] { return lng(SparseLca(t).table.h); });
        emit(name, "sparse-table build ms", sparse_build, levels);
        SparseLca sparse(t);
        int V = int(t.nodes.size());
        vector<pair<int, int>> far(q), near(q), wa(q);
        for (auto &[u, v] : far) { u = int(rng() % V); v = int(rng() % V); }
        for (auto &[u, v] : near) {
            int r = int(rng() % n);
            u = t.leaf[sa.sa[r]]; v = t.leaf[sa.sa[min(n - 1, r + int(rng() % 64))]];}
        for (auto &[v, len] : wa) { v = int(rng() % V); len = int(rng() % (t.nodes[v].depth + 1)); }
        for (auto *pairs : {&far, &near}) {
            string kind = pairs == &far ? ", random pairs" : ", rank gap < 64";
            auto [x, sx] = measure(none, [&] { lng res = 0; for (auto [u, v] : *pairs) { res += t.lca(u, v); } return res; });
            auto [y, sy] = measure(none, [&] { lng res = 0; for (auto [u, v] : *pairs) { res += sparse.lca(u, v); } return res; });
            auto [z, sz] = measure(none, [&] { lng res = 0; for (auto [u, v] : *pairs) { res += composedLca(t, u, v); } return res; });
            if (sx != sy || sx != sz) { cerr << "lca checksum mismatch " << name << kind << '\n'; return 1; }
            emit(name, "lca ms" + kind, x, sx);
            emit(name, "sparse-table lca ms" + kind, y, sy);
            emit(name, "lce + weightedAncestor ms" + kind, z, sz);}
        auto [batch, sb] = measure(none, [&] { lng res = 0; for (int u : t.weightedAncestors(wa)) { res += u; } return res; });
        auto [online, so] = measure(none, [&] { lng res = 0; for (auto [v, len] : wa) { res += t.weightedAncestor(v, len); } return res; });
        if (sb != so) { cerr << "weighted ancestor checksum mismatch " << name << '\n'; return 1; }
        emit(name, "weightedAncestors batch ms", batch, sb);
        emit(name, "weightedAncestor loop ms", online, so);
        lng block_ints = 0, sparse_ints = 0;
        for (auto &level : t.blocks.v) { block_ints += lng(level.size()); }
        for (auto &level : sparse.table.v) { sparse_ints += lng(level.size()); }
        emit(name, "lca MB (split + window + blocks)", double(4 * (2 * lng(t.split.size()) + block_ints)) / 1e6, V);
        emit(name, "sparse-table MB", double(4 * sparse_ints) / 1e6, V);}
    cout << "]\n";}
