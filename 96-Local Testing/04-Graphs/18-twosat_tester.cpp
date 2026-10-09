#include "../../04-Graphs/18-twosat.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0, cases = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    bool truth(int x, ulng mask) { return x >= 0 ? (mask >> x & 1) : !(mask >> ~x & 1); }
    // One semantic constraint over user variables, evaluated without the clause encoding.
    struct Constraint { int kind, a, b; vector<int> lits; };
    bool holds(const Constraint &c, ulng mask) {
        bool x = truth(c.a, mask), y = truth(c.b, mask);
        if (c.kind == 0) { return x || y; }
        if (c.kind == 1) { return !x || y; }
        if (c.kind == 2) { return x != y; }
        if (c.kind == 3) { return x == y; }
        if (c.kind == 4) { return x; }
        int count = 0;
        for (int l : c.lits) { count += truth(l, mask); }
        return count <= 1;}
    string show(int n, const vector<Constraint> &cs) {
        std::ostringstream out; out << "n=" << n << " constraints=";
        for (auto &c : cs) {
            out << '(' << c.kind << ':' << c.a << ',' << c.b;
            for (int l : c.lits) { out << ',' << l; }
            out << ')';}
        return out.str();}
    void apply(TwoSat &s, const Constraint &c, int style) {
        if (c.kind == 0) {
            if (style) { s.addClause(c.a >= 0 ? c.a : ~c.a, c.a < 0, c.b >= 0 ? c.b : ~c.b, c.b < 0); }
            else { s.addClause(c.a, c.b); }}
        else if (c.kind == 1) { s.addImplication(c.a, c.b); }
        else if (c.kind == 2) { s.addXor(c.a, c.b); }
        else if (c.kind == 3) { s.addEquivalent(c.a, c.b); }
        else if (c.kind == 4) { s.setValue(c.a); }
        else { s.addAtMostOne(c.lits); }}
    bool clauseHolds(const TwoSat &s, ulng mask) {
        for (auto [a, b] : s.clauses) {
            if (!truth(a, mask) && !truth(b, mask)) { return false; }}
        return true;}
    void verify(int n, const vector<Constraint> &cs, int style) {
        ++cases; context = show(n, cs) + " style=" + std::to_string(style);
        TwoSat s(n);
        for (auto &c : cs) { apply(s, c, style); }
        check(int(s.clauses.size()) >= 0 && s.n >= n, "auxiliary variables only appended");
        vector<ulng> user;
        for (ulng mask = 0; mask < (1ULL << n); ++mask) {
            if (std::all_of(cs.begin(), cs.end(), [&](const Constraint &c) { return holds(c, mask); })) { user.push_back(mask); }}
        int total = s.n; vector<ulng> full;
        if (total <= 16) {
            for (ulng mask = 0; mask < (1ULL << total); ++mask) {
                if (clauseHolds(s, mask)) { full.push_back(mask); }}
            set<ulng> projected;
            for (ulng mask : full) { projected.insert(mask & ((1ULL << n) - 1)); }
            check(projected == set<ulng>(user.begin(), user.end()), "encoding projects to the semantic solution set");}
        bool sat = s.satisfiable();
        check(sat == !user.empty(), "satisfiable versus brute force");
        check(s.solve() == sat && s.satisfiable() == sat, "repeated solve and legacy solve");
        if (sat) {
            check(int(s.answer().size()) == total && s.ans == s.answer(), "answer size and legacy ans field");
            ulng mask = 0;
            for (int x = 0; x < total; ++x) { mask |= ulng(s.answer()[x]) << x; }
            check(clauseHolds(s, mask), "answer satisfies every clause");
            check(std::all_of(cs.begin(), cs.end(), [&](const Constraint &c) { return holds(c, mask); }), "answer satisfies semantics");
            check(s.unsatWitness().literals.empty() && s.unsatWitness().clauses.empty(), "no witness when satisfiable");
            auto forced = s.forcedLiterals();
            vector<int> expected;
            int limit = total <= 16 ? total : n;
            for (int x = 0; x < limit; ++x) {
                bool allTrue = true, allFalse = true;
                for (ulng m : (total <= 16 ? full : user)) { allTrue &= truth(x, m); allFalse &= !truth(x, m); }
                if (allTrue) { expected.push_back(x); }
                if (allFalse) { expected.push_back(~x); }}
            if (limit < total) { std::erase_if(forced, [&](int l) { return (l >= 0 ? l : ~l) >= n; }); }
            check(forced == expected, "forced literals equal the intersection of all solutions");}
        else {
            check(s.answer().empty(), "empty answer when unsatisfiable");
            auto w = s.unsatWitness();
            int k = int(w.clauses.size());
            check(k >= 2 && int(w.literals.size()) == k + 1, "witness shape");
            int x = w.literals[0];
            check(0 <= x && x < total && w.literals.back() == x, "witness closes at a positive literal");
            check(std::find(w.literals.begin(), w.literals.end(), ~x) != w.literals.end(), "witness passes the complement");
            for (int i = 0; i < k; ++i) {
                int c = w.clauses[i];
                check(0 <= c && c < int(s.clauses.size()), "witness clause index");
                auto [a, b] = s.clauses[c]; int l = w.literals[i], r = w.literals[i + 1];
                check((a == ~l && b == r) || (b == ~l && a == r), "witness step is an implication of its clause");}}}
    Constraint randomConstraint(std::mt19937_64 &rng, int n) {
        auto lit = [&]() { int x = int(rng() % n); return rng() % 2 ? x : ~x; };
        Constraint c{int(rng() % 6), lit(), lit(), {}};
        if (c.kind == 5) {
            int k = int(rng() % 6);
            for (int i = 0; i < k; ++i) { c.lits.push_back(lit()); }}
        return c;}
    void exhaustive() {
        int bound = mode == "stress" ? 3 : 2;
        for (int n = 1; n <= bound; ++n) {
            vector<pair<int, int>> possible;
            for (int a = -n; a < n; ++a) { for (int b = a; b < n; ++b) { possible.emplace_back(a, b); }}
            for (ulng mask = 0; mask < (1ULL << possible.size()); ++mask) {
                vector<Constraint> cs;
                for (int i = 0; i < int(possible.size()); ++i) {
                    if (mask >> i & 1) { cs.push_back({0, possible[i].first, possible[i].second, {}}); }}
                for (int style = 0; style < (n <= 2 ? 2 : 1); ++style) { verify(n, cs, n <= 2 ? style : int(mask % 2)); }}}
        verify(0, {}, 0);
        cout << "PASS exhaustive clause sets n<=" << bound << " cases=" << cases << '\n';}
    void randomCases() {
        std::mt19937_64 rng(seed);
        int rounds = mode == "quick" ? 400 : mode == "full" ? 6000 : 40000;
        for (int t = 0; t < rounds; ++t) {
            int n = 1 + int(rng() % 8), m = int(rng() % (3 * n + 2));
            vector<Constraint> cs;
            for (int i = 0; i < m; ++i) { cs.push_back(randomConstraint(rng, n)); }
            verify(n, cs, t % 2);}
        cout << "PASS random mixed constraints rounds=" << rounds << '\n';}
    void atMostOneCases() {
        std::mt19937_64 rng(seed ^ 0x5eed);
        for (int n = 1; n <= 6; ++n) {
            for (int t = 0; t < (mode == "quick" ? 40 : 300); ++t) {
                vector<Constraint> cs;
                for (int i = 0; i < 1 + int(rng() % 3); ++i) {
                    Constraint c{5, 0, 0, {}};
                    for (int k = int(rng() % (n + 3)); k > 0; --k) { int x = int(rng() % n); c.lits.push_back(rng() % 2 ? x : ~x); }
                    cs.push_back(c);}
                if (rng() % 2) { cs.push_back(randomConstraint(rng, n)); }
                verify(n, cs, 0);}}
        cout << "PASS at-most-one with repeated and complementary literals\n";}
    void large() {
        int n = mode == "quick" ? 20000 : mode == "full" ? 400000 : 1000000;
        context = "chain n=" + std::to_string(n);
        TwoSat s(n);
        for (int x = 0; x + 1 < n; ++x) { s.addImplication(x, x + 1); }
        check(s.satisfiable(), "large chain satisfiable");
        s.setValue(0);
        check(s.satisfiable() && std::all_of(s.ans.begin(), s.ans.end(), [](bool b) { return b; }), "forced chain all true");
        s.addImplication(n - 1, ~0);
        check(!s.satisfiable(), "chain closed through ~x0 is unsatisfiable");
        auto w = s.unsatWitness();
        check(w.literals.front() == 0 && w.literals.back() == 0 && int(w.clauses.size()) >= n, "long witness");
        for (int i = 0; i + 1 < int(w.literals.size()); ++i) {
            auto [a, b] = s.clauses[w.clauses[i]];
            check((a == ~w.literals[i] && b == w.literals[i + 1]) || (b == ~w.literals[i] && a == w.literals[i + 1]), "long witness steps");}
        int f = mode == "quick" ? 3000 : 20000;
        context = "forced chain n=" + std::to_string(f);
        TwoSat g(f);
        for (int x = 0; x + 1 < f; ++x) { g.addImplication(x, x + 1); }
        g.addImplication(f / 2, ~(f / 2));
        auto forced = g.forcedLiterals();
        check(int(forced.size()) == f / 2 + 1, "forced prefix size");
        for (int x = 0; x <= f / 2; ++x) { check(forced[x] == ~x, "chain prefix forced false"); }
        TwoSat big(n);
        vector<int> lits(n); iota(lits.begin(), lits.end(), 0);
        big.addAtMostOne(lits); big.setValue(n - 1);
        context = "at-most-one n=" + std::to_string(n);
        check(big.satisfiable() && big.n == 2 * n - 2 && big.ans[n - 1] && !big.ans[0] && !big.ans[n - 2], "large at-most-one");
        cout << "PASS large chains, witness, forced prefix and at-most-one n=" << n << '\n';}
    void legacy() {
        context = "legacy";
        TwoSat s(2);
        s.addClause(0, true, 1, false); s.addClause(0, false, 0, false);
        check(s.solve() && s.ans[0] && s.ans[1], "legacy negation flags");
        s.addClause(1, true, 1, true);
        check(!s.solve() && s.ans.empty(), "legacy re-solve after adding clauses");
        TwoSat copy = s; copy.clauses.pop_back();
        check(copy.solve() && !s.solve(), "copies are independent");
        cout << "PASS legacy adapter, re-solve and copy\n";}
    void invalid(const string &name) {
        TwoSat s(2);
        if (name == "literal-high") { s.addClause(2, 0); }
        else if (name == "literal-low") { s.addClause(0, ~2); }
        else if (name == "negative-size") { TwoSat bad(-1); }
        else if (name == "forced-unsat") { s.setValue(0); s.setValue(~0); s.forcedLiterals(); }
        else if (name == "at-most-one-range") { s.addAtMostOne({0, 5}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }}
} // namespace

int main(int argc, char **argv) {
    try {
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode") { mode = argv[++i]; }
            else if (arg == "--seed") { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid") { invalid(argv[++i]); return 2; }}
        exhaustive(); randomCases(); atMostOneCases(); large(); legacy();
        cout << "PASS TwoSat seed=" << seed << " mode=" << mode << " cases=" << cases << " checks=" << checks << '\n';}
    catch (const std::exception &e) {
        cerr << "FAIL TwoSat seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
