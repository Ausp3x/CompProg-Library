#include "../../02-Data Structures/01-dsu.hpp"

// Feature map: graph subsets + union histories -> find/unite/size/count/groups,
// loops/duplicate unions; random label oracle -> state transitions and copies;
// balanced merges -> depth and compression; --invalid -> each bounds family.
// quick: graphs n<=4/history depth 3/35 random trials/1024 balanced vertices;
// full: n<=6/depth 5/150 trials/65536 vertices; stress: full + depth 6/600 trials.
ulng seed = 20260927;
string mode = "quick", context;
vector<pair<int,int>> history;

[[noreturn]] void fail(const string &op, const string &expected, const string &actual) {
    cerr << "FAIL DSU seed=" << seed << " mode=" << mode << " case=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual
         << " unions=";
    for (auto [u,v] : history) { cerr << '(' << u << ',' << v << ')'; }
    cerr << '\n'; std::exit(1);
}
template<typename T> void check(const T &actual, const T &expected, const string &op) {
    if (actual != expected) {
        std::ostringstream a, e; a << actual; e << expected; fail(op,e.str(),a.str());}
}
string show(const vector<vector<int>> &a) {
    std::ostringstream out;
    for (const auto &v : a) { out << '['; for (int x : v) { out << x << ','; } out << ']'; }
    return out.str();
}
bool join(vector<int> &a, int u, int v) {
    int x = a[u], y = a[v];
    if (x == y) { return false; }
    for (int &z : a) { if (z == y) { z = x; }}
    return true;
}
void verify(DSU &d, const vector<int> &a) {
    int n = int(a.size());
    check(d.n,n,"n");
    vector<vector<int>> expected;
    vector<int> seen(n,-1);
    for (int u = 0; u < n; ++u) {
        int count = 0;
        for (int v = 0; v < n; ++v) {
            bool same = a[u] == a[v]; count += same;
            check(d.isSameSet(u,v),same,"isSameSet("+std::to_string(u)+","+std::to_string(v)+")");}
        check(d.getSize(u),count,"getSize("+std::to_string(u)+")");
        int r = d.findSet(u);
        if (r < 0 || r >= n) { fail("findSet","representative in [0,n)",std::to_string(r)); }
        check(a[r],a[u],"representative membership"); check(d.par[u],r,"full compression");
        check(d.findSet(r),r,"root idempotence");
        if (seen[a[u]] == -1) { seen[a[u]] = int(expected.size()); expected.emplace_back(); }
        expected[seen[a[u]]].push_back(u);}
    check(d.count(),int(expected.size()),"count"); check(d.ncon,d.count(),"legacy ncon");
    check(show(d.groups()),show(expected),"groups");
    check(show(d.groups()),show(expected),"repeated groups");
}
void graphSubsets() {
    int limit = mode == "quick" ? 4 : 6;
    for (int n = 0; n <= limit; ++n) {
        vector<pair<int,int>> edges;
        for (int u = 0; u < n; ++u) { for (int v = u+1; v < n; ++v) { edges.emplace_back(u,v); }}
        for (int mask = 0; mask < (1 << int(edges.size())); ++mask) {
            context = "graph n="+std::to_string(n)+" mask="+std::to_string(mask); history.clear();
            DSU d(n); vector<int> labels(n); iota(labels.begin(),labels.end(),0);
            for (int i = 0; i < int(edges.size()); ++i) {
                if (mask >> i & 1) {
                    auto [u,v] = edges[i]; history.push_back({u,v});
                    check(d.uniteSets(u,v),join(labels,u,v),"uniteSets");}}
            for (int u = 0; u < n; ++u) { check(d.uniteSets(u,u),false,"self union"); }
            verify(d,labels);}}
    cout << "PASS DSU exhaustive graph subsets n<=" << limit << '\n';
}
void unionHistories() {
    int depth = mode == "quick" ? 3 : mode == "full" ? 5 : 6;
    context = "exhaustive n=3 union histories depth="+std::to_string(depth); history.clear();
    auto rec = [&] (auto &&self, DSU d, vector<int> a, int left) -> void {
        verify(d,a);
        if (!left) { return; }
        for (int u = 0; u < 3; ++u) { for (int v = 0; v < 3; ++v) {
            DSU next = d; auto b = a; history.push_back({u,v});
            check(next.uniteSets(u,v),join(b,u,v),"history uniteSets");
            self(self,std::move(next),std::move(b),left-1); history.pop_back();}}};
    rec(rec,DSU(3),vector<int>{0,1,2},depth);
    cout << "PASS DSU exhaustive union histories depth=" << depth << '\n';
}
void randomCases() {
    std::mt19937_64 rng(seed);
    int trials = mode == "quick" ? 35 : mode == "full" ? 150 : 600;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%65); DSU d(n); vector<int> a(n); iota(a.begin(),a.end(),0); history.clear();
        context = "random trial="+std::to_string(trial)+" n="+std::to_string(n); verify(d,a);
        if (!n) { continue; }
        for (int op = 0; op < 200; ++op) {
            int u = int(rng()%n), v = int(rng()%n); history.push_back({u,v});
            check(d.uniteSets(u,v),join(a,u,v),"random uniteSets");
            if (op%7 == 0) { verify(d,a); }}
        DSU copy = d; verify(copy,a); DSU moved = std::move(copy); verify(moved,a);
        copy = DSU(n); vector<int> fresh(n); iota(fresh.begin(),fresh.end(),0); verify(copy,fresh);
        copy = d; verify(copy,a); verify(d,a);}
    cout << "PASS DSU random label oracle/copy/move trials=" << trials << '\n';
}
void balancedTree() {
    int n = mode == "quick" ? 1024 : 65536;
    context = "balanced merges n="+std::to_string(n); history.clear(); DSU d(n);
    for (int step = 1; step < n; step *= 2) {
        for (int u = 0; u < n; u += 2*step) { check(d.uniteSets(u,u+step),true,"balanced merge"); }}
    check(d.count(),1,"balanced count");
    for (int i = n-1; i >= 0; --i) {
        int r = d.findSet(i); check(d.par[i],r,"balanced compression"); check(d.getSize(i),n,"balanced size");}
    auto groups = d.groups(); check(int(groups.size()),1,"balanced groups count");
    check(int(groups[0].size()),n,"balanced members count");
    cout << "PASS DSU balanced merges/compression n=" << n << '\n';
}
int main(int argc, char **argv) {
    string invalid;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (i+1 == argc) { cerr << "missing value\n"; return 2; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else { cerr << "unknown option\n"; return 2; }}
    if (!invalid.empty()) {
        DSU d(2);
        if (invalid == "negative-size") { DSU bad(-1); }
        else if (invalid == "find-negative") { d.findSet(-1); }
        else if (invalid == "find-end") { d.findSet(2); }
        else if (invalid == "empty-find") { DSU().findSet(0); }
        else if (invalid == "union-end") { d.uniteSets(0,2); }
        else if (invalid == "size-end") { d.getSize(2); }
        else if (invalid == "same-end") { d.isSameSet(0,2); }
        else { cerr << "unknown invalid case\n"; return 2; }
        cerr << "precondition did not assert\n"; return 1;}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    cout << "DSU seed=" << seed << " mode=" << mode << '\n';
    graphSubsets(); unionHistories(); randomCases(); balancedTree();
}
