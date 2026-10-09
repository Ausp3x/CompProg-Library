#include "../../04-Graphs/23-assignment.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    string str(lll x) {
        if (x == 0) { return "0"; }
        string s; bool neg = x < 0;
        while (x != 0) { int d = int(x % 10); s += char('0' + (d < 0 ? -d : d)); x /= 10; }
        if (neg) { s += '-'; }
        std::reverse(s.begin(), s.end()); return s;}
    template<class T>
    using Matrix = vector<vector<T>>;
    using Mask = vector<vector<uint8_t>>;
    template<class T>
    string show(const Matrix<T> &a, const Mask &ok, bool maximize) {
        std::ostringstream out; out << "max=" << maximize << " a=";
        for (int i = 0; i < int(a.size()); ++i) {
            out << '[';
            for (int j = 0; j < int(a[i].size()); ++j) { out << str(a[i][j]) << (ok.empty() || ok[i][j] ? "" : "x") << ' '; }
            out << ']';}
        return out.str();}
    // Every injective assignment of the smaller side, as (exact cost, row -> col); the oracle for dense operations.
    template<class T>
    vector<pair<lll, vector<int>>> enumerate(const Matrix<T> &a, const Mask &ok) {
        int R = int(a.size()), C = R ? int(a[0].size()) : 0;
        vector<pair<lll, vector<int>>> all;
        vector<int> row(R, -1); vector<uint8_t> used(C);
        int need = min(R, C);
        auto rec = [&](auto &&rec, int i, int got, lll s) -> void {
            if (i == R) { if (got == need) { all.emplace_back(s, row); } return; }
            if (R - i > need - got) { rec(rec, i + 1, got, s); }
            for (int j = 0; j < C; ++j) {
                if (used[j] || (!ok.empty() && !ok[i][j])) { continue; }
                used[j] = 1; row[i] = j; rec(rec, i + 1, got + 1, s + a[i][j]); row[i] = -1; used[j] = 0;}};
        rec(rec, 0, 0, 0);
        return all;}
    template<class T>
    void denseCase(const Matrix<T> &a, const Mask &ok, bool maximize, int k) {
        ++cases; context = show(a, ok, maximize);
        int R = int(a.size()), C = R ? int(a[0].size()) : 0;
        auto all = enumerate(a, ok);
        sort(all.begin(), all.end(), [&](auto &x, auto &y) { return maximize ? x.first > y.first : x.first < y.first; });
        auto res = hungarian(a, maximize, ok);
        check(res.feasible == !all.empty(), "feasibility versus enumeration");
        check(int(res.row.size()) == R && int(res.col.size()) == C && int(res.u.size()) == R && int(res.v.size()) == C, "result shapes");
        if (res.feasible) {
            check(lll(res.cost) == all[0].first, "optimal cost " + str(res.cost) + " expected " + str(all[0].first));
            lll s = 0, dual = 0; int cnt = 0;
            for (int i = 0; i < R; ++i) {
                if (res.row[i] == -1) { continue; }
                int j = res.row[i]; ++cnt; s += a[i][j];
                check(res.col[j] == i && (ok.empty() || ok[i][j]), "assignment consistent and allowed");
                check(lll(res.u[i]) + res.v[j] == lll(a[i][j]), "complementary slackness on assigned pairs");}
            check(cnt == min(R, C) && s == lll(res.cost), "assignment size and cost");
            for (int i = 0; i < R; ++i) {
                dual += res.u[i];
                for (int j = 0; j < C; ++j) {
                    if (!ok.empty() && !ok[i][j]) { continue; }
                    lll slack = lll(a[i][j]) - res.u[i] - res.v[j];
                    check(maximize ? slack <= 0 : slack >= 0, "dual feasibility");}}
            for (int j = 0; j < C; ++j) { dual += res.v[j]; }
            check(dual == lll(res.cost), "dual objective equals cost");
            for (int i = 0; i < R && R > C; ++i) { check(res.row[i] == -1 ? res.u[i] == 0 : maximize ? res.u[i] >= 0 : res.u[i] <= 0, "larger side potential sign, rows"); }
            for (int j = 0; j < C && R <= C; ++j) { check(res.col[j] == -1 ? res.v[j] == 0 : maximize ? res.v[j] >= 0 : res.v[j] <= 0, "larger side potential sign, cols"); }}
        check(kBestAssignments(a, 0, maximize, ok).empty(), "k = 0 yields nothing");
        auto best = kBestAssignments(a, k, maximize, ok);
        check(int(best.size()) == min(k, int(all.size())), "k-best count");
        set<vector<int>> seen;
        for (int t = 0; t < int(best.size()); ++t) {
            auto &[cost, row] = best[t];
            check(lll(cost) == all[t].first, "k-best cost at rank " + std::to_string(t) + " " + str(cost) + " expected " + str(all[t].first));
            check(seen.insert(row).second, "k-best assignments distinct");
            lll s = 0; vector<uint8_t> used(C); int cnt = 0;
            for (int i = 0; i < R; ++i) {
                if (row[i] == -1) { continue; }
                check(0 <= row[i] && row[i] < C && !used[row[i]] && (ok.empty() || ok[i][row[i]]), "k-best assignment valid");
                used[row[i]] = 1; s += a[i][row[i]]; ++cnt;}
            check(cnt == min(R, C) && s == lll(cost), "k-best assignment cost");}}
    template<class T>
    void sparseCase(int nl, int nr, const vector<tuple<int, int, T>> &es, bool maximize) {
        ++cases;
        std::ostringstream out; out << "sparse nl=" << nl << " nr=" << nr << " max=" << maximize << " edges=";
        for (auto [u, v, w] : es) { out << u << '-' << v << ':' << str(w) << ' '; }
        context = out.str();
        int E = int(es.size());
        vector<lll> bestBy(min(nl, nr) + 1);
        vector<uint8_t> has(bestBy.size());
        vector<uint8_t> ul(nl), ur(nr);
        auto rec = [&](auto &&rec, int e, int size, lll s) -> void {
            if (e == E) {
                if (!has[size] || (maximize ? s > bestBy[size] : s < bestBy[size])) { bestBy[size] = s; has[size] = 1; }
                return;}
            rec(rec, e + 1, size, s);
            auto [u, v, w] = es[e];
            if (!ul[u] && !ur[v]) { ul[u] = ur[v] = 1; rec(rec, e + 1, size + 1, s + w); ul[u] = ur[v] = 0; }};
        rec(rec, 0, 0, 0);
        int top = 0;
        for (int s = 0; s < int(has.size()); ++s) {
            if (has[s]) { top = s; }}
        int anySize = 0;
        for (int s = 0; s <= top; ++s) {
            if (maximize ? bestBy[s] > bestBy[anySize] : bestBy[s] < bestBy[anySize]) { anySize = s; }}
        for (bool card : {true, false}) {
            auto res = linearSumAssignment(nl, nr, es, maximize, card);
            vector<uint8_t> a(nl), b(nr); lll s = 0;
            for (int i = 0; i < int(res.edges.size()); ++i) {
                int e = res.edges[i]; auto [u, v, w] = es[e];
                check(i == 0 || res.edges[i - 1] < e, "sparse edges ascending");
                check(!a[u] && !b[v], "sparse result is a matching");
                a[u] = b[v] = 1; s += w;}
            check(s == lll(res.cost), "sparse cost equals edge sum");
            int size = int(res.edges.size());
            check(int(res.curve.size()) == size + 1 && res.curve.back() == res.cost, "curve shape");
            for (int j = 0; j <= size; ++j) { check(lll(res.curve[j]) == bestBy[j], "curve value at " + std::to_string(j)); }
            if (card) { check(size == top && lll(res.cost) == bestBy[top], "max-cardinality optimum"); }
            else { check(lll(res.cost) == bestBy[anySize] && size == anySize, "unrestricted optimum with fewest edges"); }}}
    template<class T>
    void randomDense(std::mt19937_64 &rng, int rounds, T bound, int maxSide) {
        for (int t = 0; t < rounds; ++t) {
            int R = int(rng() % (maxSide + 1)), C = int(rng() % (maxSide + 1));
            if (R == 0) { C = 0; }
            Matrix<T> a(R, vector<T>(C));
            for (auto &row : a) {
                for (T &x : row) {
                    int kind = int(rng() % 4);
                    x = kind == 0 ? bound : kind == 1 ? T(-bound) : T(lll(rng() % (2 * ulng(min<lll>(bound, lll(1) << 62)) + 1)) - min<lll>(bound, lll(1) << 62));}}
            Mask ok;
            if (rng() % 2) {
                ok.assign(R, vector<uint8_t>(C));
                for (auto &row : ok) { for (auto &x : row) { x = rng() % 4 != 0; } }}
            denseCase(a, ok, rng() % 2, 1 + int(rng() % 30));}}
    template<class T>
    void randomSparse(std::mt19937_64 &rng, int rounds, T bound) {
        for (int t = 0; t < rounds; ++t) {
            int nl = int(rng() % 6), nr = int(rng() % 6), E = nl && nr ? int(rng() % 11) : 0;
            vector<tuple<int, int, T>> es;
            for (int e = 0; e < E; ++e) {
                int kind = int(rng() % 4);
                T w = kind == 0 ? bound : kind == 1 ? T(-bound) : T(lll(rng() % (2 * ulng(min<lll>(bound, lll(1) << 62)) + 1)) - min<lll>(bound, lll(1) << 62));
                es.emplace_back(int(rng() % nl), int(rng() % nr), w);}
            sparseCase(nl, nr, es, rng() % 2);}}
    void smallCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 300 : mode == "full" ? 3000 : 20000;
        randomDense<int>(rng, rounds, 9, 5);
        randomDense<lng>(rng, rounds, std::numeric_limits<lng>::max() / 100, 5);
        randomDense<lll>(rng, rounds / 4, std::numeric_limits<lll>::max() / 100, 4);
        randomDense<int>(rng, rounds / 4, 2, 6);
        randomSparse<int>(rng, rounds, 9);
        randomSparse<lng>(rng, rounds, std::numeric_limits<lng>::max() / 200);
        cout << "PASS random dense/k-best/sparse at domain bounds rounds=" << rounds << '\n';}
    void exhaustiveTiny() {
        for (int R = 0; R <= 3; ++R) {
            for (int C = 0; C <= 3; ++C) {
                if (R == 0 && C > 0) { continue; }
                int cells = R * C;
                for (int mask = 0; mask < (1 << cells); ++mask) {
                    Matrix<int> a(R, vector<int>(C)); Mask ok(R, vector<uint8_t>(C, 1));
                    for (int i = 0; i < cells; ++i) { a[i / C][i % C] = (i * 7 + mask) % 5 - 2; ok[i / C][i % C] = mask >> i & 1; }
                    for (bool maximize : {false, true}) { denseCase(a, ok, maximize, 100); }}}}
        cout << "PASS every forbidden pattern up to 3x3 cases=" << cases << '\n';}
    void legacy() {
        context = "legacy";
        Hungarian<lng> h(2, 3);
        h.addEdge(0, 0, 5); h.addEdge(0, 1, 1); h.addEdge(1, 0, 2); h.addEdge(1, 1, 7); h.addEdge(0, 2, 9); h.addEdge(1, 2, 9);
        check(h.solve() == 3 && h.getAssignment() == vector<int>{1, 0}, "legacy 2x3 minimum");
        h.addEdge(1, 0, 8);
        check(h.solve() == 9 && h.getAssignment() == vector<int>{1, 0}, "legacy re-solve after update");
        Hungarian<lng> unset(2, 3);
        unset.addEdge(0, 0, 5); unset.addEdge(1, 1, 4);
        check(unset.solve() == 0 && unset.getAssignment()[0] != 0 && unset.getAssignment()[1] != 1, "legacy unset pairs cost 0");
        Hungarian<lng> tall(3, 2);
        tall.addEdge(0, 0, 4); tall.addEdge(1, 1, -3); tall.addEdge(2, 0, 1);
        check(tall.solve() == -2, "legacy rectangular rows > cols");
        vector<Hungarian<lng>> grown;
        for (int i = 0; i < 3; ++i) { grown.emplace_back(2, 2); }
        auto copied = grown;
        check(!copied[0].res.feasible && copied[0].res.cost == 0 && copied[2].solve() == 0, "copy before solve (initialised result)");
        cout << "PASS legacy adapter\n";}
    void large() {
        std::mt19937_64 rng(seed ^ 0x1a);
        int n = mode == "quick" ? 60 : mode == "full" ? 150 : 400;
        int big = mode == "quick" ? 500 : mode == "full" ? 2000 : 8000;
#ifdef _GLIBCXX_DEBUG
        n = min(n, 30); big = min(big, 200); // Debug STL re-validates the whole heap on every push and pop; domains are unchanged.
#endif
        Matrix<lng> a(n, vector<lng>(n + n / 3));
        for (auto &row : a) { for (lng &x : row) { x = lng(rng() % 2000001) - 1000000; } }
        context = "large dense n=" + std::to_string(n);
        for (bool maximize : {false, true}) {
            auto res = hungarian(a, maximize);
            vector<tuple<int, int, lng>> es;
            for (int i = 0; i < n; ++i) { for (int j = 0; j < int(a[i].size()); ++j) { es.emplace_back(i, j, a[i][j]); } }
            auto sp = linearSumAssignment(n, int(a[0].size()), es, maximize);
            check(res.feasible && sp.cost == res.cost && int(sp.edges.size()) == n, "dense Hungarian equals sparse SSP");
            lll dual = 0;
            for (lng x : res.u) { dual += x; }
            for (lng x : res.v) { dual += x; }
            check(dual == res.cost, "large dual objective");
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < int(a[i].size()); ++j) {
                    lll slack = lll(a[i][j]) - res.u[i] - res.v[j];
                    check(maximize ? slack <= 0 : slack >= 0, "large dual feasibility");}}
            for (int j = 1; j + 1 < int(sp.curve.size()); ++j) {
                lll d1 = lll(sp.curve[j]) - sp.curve[j - 1], d2 = lll(sp.curve[j + 1]) - sp.curve[j];
                check(maximize ? d2 <= d1 : d2 >= d1, "cost curve convexity");}
            auto tr = a;
            Matrix<lng> t(tr[0].size(), vector<lng>(n));
            for (int i = 0; i < n; ++i) { for (int j = 0; j < int(tr[i].size()); ++j) { t[j][i] = tr[i][j]; } }
            check(hungarian(t, maximize).cost == res.cost, "transpose invariance");}
        vector<tuple<int, int, lng>> es;
        for (int u = 0; u < big; ++u) {
            for (int d = 0; d < 3; ++d) { es.emplace_back(u, int((u + rng() % 5) % big), lng(rng() % 1000)); }}
        context = "large sparse n=" + std::to_string(big);
        auto sp = linearSumAssignment(big, big, es);
        auto any = linearSumAssignment(big, big, es, false, false);
        check(any.edges.empty() && any.cost == 0 && any.curve.size() == 1, "nonnegative costs: unrestricted optimum is empty");
        vector<uint8_t> ul(big), ur(big);
        for (int e : sp.edges) { auto [u, v, w] = es[e]; check(!ul[u] && !ur[v], "large sparse matching"); ul[u] = ur[v] = 1; }
        for (int j = 1; j + 1 < int(sp.curve.size()); j += 97) {
            check(lll(sp.curve[j + 1]) - sp.curve[j] >= lll(sp.curve[j]) - sp.curve[j - 1], "large curve convexity");}
        int kn = mode == "quick" ? 6 : 8;
        Matrix<int> small(kn, vector<int>(kn));
        for (auto &row : small) { for (int &x : row) { x = int(rng() % 7); } }
        denseCase(small, {}, false, mode == "quick" ? 50 : 400);
        cout << "PASS large dense n=" << n << ", sparse n=" << big << ", k-best " << kn << "x" << kn << '\n';}
    void invalid(const string &name) {
        if (name == "ragged") { hungarian(Matrix<lng>{{1, 2}, {3}}); }
        else if (name == "mask-shape") { hungarian(Matrix<lng>{{1, 2}}, false, Mask{{1}}); }
        else if (name == "k-negative") { kBestAssignments(Matrix<lng>{{1}}, -1); }
        else if (name == "sparse-range") { linearSumAssignment<lng>(1, 1, {{0, 1, 5}}); }
        else if (name == "legacy-range") { Hungarian<lng> h(1, 1); h.addEdge(1, 0, 1); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustiveTiny(); smallCases(); legacy(); large();
        cout << "PASS assignment seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL assignment seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
