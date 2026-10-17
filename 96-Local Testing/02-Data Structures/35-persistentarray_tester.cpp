#include "../../02-Data Structures/35-persistentarray.hpp"

namespace {
    ulng seed = 20261017;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T> string show(const vector<T> &v) {
        string s = "[";
        for (size_t i = 0; i < v.size(); ++i) { s += (i ? "," : "") + show(T(v[i])); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) { throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got)); }}

    int value(Rng &rng, int *) { return int(rnd(rng, -1000000000, 1000000000)); }
    lng value(Rng &rng, lng *) { return lng(rng()); }
    string value(Rng &rng, string *) { return string(size_t(rnd(rng, 0, 40)), char('a' + rnd(rng, 0, 25))); }
    bool value(Rng &rng, bool *) { return rng() & 1; }

    // Snapshot oracle: a full copy per version; sets branch from uniformly random, latest or root versions.
    template<typename T, int B>
    void branching(Rng &rng, int n, int sets, bool fill, const string &name) {
        context = name + " B=" + show(B) + " n=" + show(n) + (fill ? " fill" : " vector");
        vector<vector<T>> snap(1, vector<T>(n));
        T x0 = value(rng, (T *)nullptr);
        if (fill) { for (auto &&x : snap[0]) { x = x0; } }
        else { for (auto &&x : snap[0]) { x = value(rng, (T *)nullptr); } }
        PersistentArray<T, B> a = fill ? PersistentArray<T, B>(n, x0) : PersistentArray<T, B>(snap[0]);
        int K = 1 << B, h = 0;
        while ((lng(K) << (B * h)) < n) { ++h; }
        checkEqual(a.h, h, "height");
        checkEqual(a.versionCount(), 1, "initial versionCount");
        size_t ch0 = a.ch.size(), val0 = a.val.size();
        if (fill) { checkEqual(ch0, size_t(K), "one shared internal node"); checkEqual(val0, size_t(K), "one shared leaf chunk"); }
        for (int s = 0; s < sets && n; ++s) {
            int kind = int(rng() % 3), v = kind == 0 ? int(snap.size()) - 1 : kind == 1 ? 0 : int(rnd(rng, 0, lng(snap.size()) - 1));
            int p = int(rnd(rng, 0, n - 1));
            T x = rng() % 4 ? value(rng, (T *)nullptr) : T(a.get(int(rnd(rng, 0, lng(snap.size()) - 1)), int(rnd(rng, 0, n - 1))));
            if (rng() % 8 == 0) {
                int w = int(rnd(rng, 0, lng(snap.size()) - 1)), q = int(rnd(rng, 0, n - 1));
                checkEqual(a.set(v, p, a.get(w, q)), int(snap.size()), "aliased set id");
                x = snap[w][q];}
            else { checkEqual(a.set(v, p, x), int(snap.size()), "set id"); }
            snap.push_back(snap[v]); snap.back()[p] = x;
            checkEqual(a.ch.size(), ch0 + size_t(s + 1) * h * K, "internal nodes per set");
            checkEqual(a.val.size(), val0 + size_t(s + 1) * K, "leaf chunks per set");
            if (rng() % 4 == 0) {
                int w = int(rnd(rng, 0, lng(snap.size()) - 1));
                for (int k = 0; k < 4; ++k) { int q = int(rnd(rng, 0, n - 1)); checkEqual(T(a.get(w, q)), T(snap[w][q]), "get version=" + show(w) + " p=" + show(q)); }}}
        checkEqual(a.versionCount(), int(snap.size()), "versionCount");
        for (int w = 0; w < int(snap.size()); ++w) {
            if (n <= 64 || w % 37 == 0 || w + 1 == int(snap.size())) {
                checkEqual(a.toVector(w), snap[w], "toVector version=" + show(w));
                for (int p = 0; p < n; ++p) { checkEqual(T(a.get(w, p)), T(snap[w][p]), "get version=" + show(w) + " p=" + show(p)); }}}}

    // Large histories: offline DFS over the version tree with one mutable array answers sampled gets.
    template<int B>
    void large(Rng &rng, int n, int sets, int gets) {
        context = "large B=" + show(B) + " n=" + show(n) + " sets=" + show(sets);
        vector<lng> base(n);
        for (auto &x : base) { x = lng(rng()); }
        PersistentArray<lng, B> a(base);
        vector<int> parent(1, -1), pos(1, 0);
        vector<lng> val(1, 0);
        for (int s = 0; s < sets; ++s) {
            int kind = int(rng() % 4), v = kind <= 1 ? int(parent.size()) - 1 : int(rnd(rng, 0, lng(parent.size()) - 1));
            int p = int(rnd(rng, 0, n - 1));
            lng x = lng(rng());
            a.set(v, p, x);
            parent.push_back(v); pos.push_back(p); val.push_back(x);}
        int V = int(parent.size());
        vector<vector<int>> kids(V);
        for (int w = 1; w < V; ++w) { kids[parent[w]].push_back(w); }
        vector<vector<pair<int, int>>> at(V);
        vector<lng> want(gets);
        for (int g = 0; g < gets; ++g) { at[int(rnd(rng, 0, V - 1))].push_back({int(rnd(rng, 0, n - 1)), g}); }
        vector<lng> cur = base, saved(V);
        vector<pair<int, int>> st{{0, 0}};
        while (!st.empty()) {
            auto &[w, k] = st.back();
            if (k == 0) {
                if (w) { saved[w] = cur[pos[w]]; cur[pos[w]] = val[w]; }
                for (auto [p, g] : at[w]) { want[g] = cur[p]; }}
            if (k < int(kids[w].size())) { int c = kids[w][k++]; st.push_back({c, 0}); }
            else { if (w) { cur[pos[w]] = saved[w]; } st.pop_back(); }}
        for (int w = 0; w < V; ++w) {
            for (auto [p, g] : at[w]) { checkEqual(a.get(w, p), want[g], "get version=" + show(w) + " p=" + show(p)); }}
        checkEqual(a.toVector(V - 1)[pos[V - 1]], val[V - 1], "toVector latest");}

    void edges() {
        context = "edges";
        PersistentArray<int> e(vector<int>{});
        checkEqual(e.toVector(0).size(), size_t(0), "empty toVector"); checkEqual(e.versionCount(), 1, "empty versions");
        PersistentArray<int> f(0, 7);
        checkEqual(f.toVector(0).size(), size_t(0), "empty fill");
        PersistentArray<string, 1> s(vector<string>{"a", "b", "c"});
        int v1 = s.set(0, 1, "x"), v2 = s.set(0, 1, "y"), v3 = s.set(v1, 2, s.get(v2, 1));
        checkEqual(s.toVector(v3), vector<string>{"a", "x", "y"}, "branch and aliased value");
        checkEqual(s.toVector(0), vector<string>{"a", "b", "c"}, "root unchanged");
        PersistentArray<string, 1> copy = s;
        copy.set(v3, 0, "z");
        checkEqual(copy.versionCount(), s.versionCount() + 1, "copy independent");
        PersistentArray<int, 8> wide(1000, -1);
        checkEqual(wide.h, 1, "B=8 height");
        int w1 = wide.set(0, 999, 5);
        checkEqual(wide.get(w1, 999), 5, "B=8 set"); checkEqual(wide.get(0, 999), -1, "B=8 root");
        PersistentArray<int, 1> big(INT_MAX, 3);
        int b1 = big.set(0, INT_MAX - 1, 4);
        checkEqual(big.get(b1, INT_MAX - 1), 4, "n = INT_MAX last"); checkEqual(big.get(b1, 0), 3, "n = INT_MAX first");
        checkEqual(big.h, 30, "n = INT_MAX height");}

    void invalid(const string &name) {
        PersistentArray<int> a(vector<int>{3, 1, 4});
        if (name == "get-version") { (void)a.get(1, 0); }
        else if (name == "get-range") { (void)a.get(0, 3); }
        else if (name == "get-negative") { (void)a.get(0, -1); }
        else if (name == "set-version") { a.set(-1, 0, 1); }
        else if (name == "set-range") { a.set(0, 3, 1); }
        else if (name == "tovector-version") { (void)a.toVector(1); }
        else if (name == "fill-negative") { PersistentArray<int> b(-1, 0); }
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
        std::cout << "persistentarray seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        edges();
        int small = mode == "quick" ? 40 : mode == "full" ? 140 : 300;
        for (int n = 0; n <= small; ++n) {
            for (bool fill : {false, true}) {
                branching<int, 1>(rng, n, 30, fill, "int"); branching<int, 2>(rng, n, 30, fill, "int");
                branching<int, 3>(rng, n, 30, fill, "int"); branching<lng, 4>(rng, n, 30, fill, "lng");
                branching<string, 3>(rng, n, 10, fill, "string"); branching<bool, 2>(rng, n, 20, fill, "bool");}}
        for (int n : {255, 256, 257, 4095, 4096, 4097, 32768, 32769}) {
            branching<int, 3>(rng, n, 200, false, "int"); branching<int, 4>(rng, n, 200, true, "int"); branching<int, 8>(rng, n, 100, false, "int");}
        if (mode != "quick") { large<3>(rng, 100000, 200000, 200000); large<1>(rng, 65536, 100000, 100000); large<4>(rng, 100003, 100000, 100000); }
        if (mode == "stress") { large<3>(rng, 1000000, 1000000, 1000000); large<2>(rng, 1 << 20, 500000, 500000); }
        std::cout << "PASS persistentarray checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL persistentarray seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
