#include "../../06-Miscellaneous/34-slope_trick.hpp"

ulng test_seed = 0;
lng checks = 0, applied = 0, sequences = 0, truncated = 0;
string context;
std::mt19937_64 gen;

lng rnd(lng l, lng r) { return l + lng(gen() % ulng(r - l + 1)); }
string number(lll x) {
    bool negative = x < 0; ulll value = negative ? ulll(0) - ulll(x) : ulll(x); string out;
    do { out += char('0' + int(value % 10)); value /= 10; } while (value);
    if (negative) { out += '-'; }
    reverse(out.begin(), out.end()); return out;}
template<class T> string show(const T &x) {
    if constexpr (std::is_floating_point_v<T>) { std::ostringstream out; out << std::setprecision(17) << x; return out.str(); }
    else { return number(lll(x)); }}
template<class T> string show(const pair<T, T> &p) { return "(" + show(p.fi) + "," + show(p.se) + ")"; }
void fail(const string &what, const string &expected, const string &actual) {
    cerr << "FAIL seed=" << test_seed << " smallest known reproducer=" << context << " operation=" << what
         << " expected=" << expected << " actual=" << actual << '\n';
    std::exit(1);}
void check(bool ok, const string &what) { ++checks; if (!ok) { fail(what, "true", "false"); } }
template<class A, class E> void checkEqual(const A &actual, const E &expected, const string &what) {
    ++checks;
    if (!(actual == expected)) { fail(what, show(expected), show(actual)); }}

// Independent oracle: exact values on the integer window [-B, B], +INF outside the domain. Every breakpoint
// and finite domain end stays within [-LIMIT, LIMIT], so beyond the window the function is exactly linear.
constexpr int B = 128, P = 12, LIMIT = B - 4;
constexpr lll INFV = lll(1) << 100;
lll fixv(lll x) { return x >= INFV / 2 ? INFV : x; }
lll addv(lll a, lll b) { return a >= INFV || b >= INFV ? INFV : a + b; }
struct Fn {
    vector<lll> v = vector<lll>(2 * B + 1, 0);
    lll get(int x) const { return v[x + B]; }
    lll at(lll x) const {
        if (x < -B) { lll a = get(-B), b = get(-B + 1); return a >= INFV ? INFV : a + (b - a) * (x + B); }
        if (x > B) { lll a = get(B), b = get(B - 1); return a >= INFV ? INFV : a + (a - b) * (x - B); }
        return get(int(x));}
    template<class F> static Fn make(F g) {
        Fn r;
        for (int x = -B; x <= B; ++x) { r.v[x + B] = fixv(g(x)); }
        return r;}
    lll leftSlope() const { return get(-B + 1) - get(-B); }
    lll rightSlope() const { return get(B) - get(B - 1); }
    pair<lll, lll> domain() const {
        int l = -B, r = B;
        while (l < B && get(l) >= INFV) { ++l; }
        while (r > -B && get(r) >= INFV) { --r; }
        return {l == -B ? -INFV : l, r == B ? INFV : r};}
    bool boundedLeft() const { return domain().fi > -INFV || leftSlope() <= 0; }
    bool boundedRight() const { return domain().se < INFV || rightSlope() >= 0; }
    bool bounded() const { return boundedLeft() && boundedRight(); }
    lll minv() const { lll m = INFV; for (lll y : v) { m = std::min(m, y); } return m; }
    pair<lll, lll> argmin() const {
        lll m = minv(); int l = -B, r = B;
        while (get(l) != m) { ++l; }
        while (get(r) != m) { --r; }
        return {l == -B ? -INFV : l, r == B ? INFV : r};}
    bool convex() const {
        auto [l, r] = domain();
        int a = l == -INFV ? -B : int(l), b = r == INFV ? B : int(r);
        for (int x = -B; x <= B; ++x) { if ((x >= a && x <= b) != (get(x) < INFV)) { return false; } }
        for (int x = a + 1; x < b; ++x) { if (get(x - 1) - 2 * get(x) + get(x + 1) < 0) { return false; } }
        return true;}
    lll extent() const {
        lll e = 0;
        for (int x = -B + 1; x < B; ++x) {
            lll a = get(x - 1), b = get(x), c = get(x + 1);
            if (b < INFV && (a >= INFV || c >= INFV || a - 2 * b + c != 0)) { e = max(e, lll(abs(x))); }}
        return e;}
};
Fn delta() { return Fn::make([](int x) -> lll { return x ? INFV : 0; }); }
Fn bAdd(const Fn &f, auto term) { return Fn::make([&](int x) { return addv(f.get(x), term(lll(x))); }); }
Fn bRestrict(const Fn &f, lll l, lll r) { return Fn::make([&](int x) { return x < l || x > r ? INFV : f.get(x); }); }
Fn bPrefixMin(const Fn &f) {
    Fn r = f;
    for (int x = -B + 1; x <= B; ++x) { r.v[x + B] = std::min(r.v[x + B], r.v[x + B - 1]); }
    return r;}
Fn bSuffixMin(const Fn &f) {
    Fn r = f;
    for (int x = B - 1; x >= -B; --x) { r.v[x + B] = std::min(r.v[x + B], r.v[x + B + 1]); }
    return r;}
Fn bShift(const Fn &f, lll d) { return Fn::make([&](int x) { return f.at(x - d); }); }
Fn bSlide(const Fn &f, lll a, lll b) {
    return Fn::make([&](int x) {
        lll m = INFV;
        for (lll y = x - b; y <= x - a; ++y) { m = std::min(m, f.at(y)); }
        return m;});}
Fn bClamp(const Fn &f, lll s1, lll s2) {
    return Fn::make([&](int x) {
        lll m = INFV;
        for (int y = -B; y <= B; ++y) { if (f.get(y) < INFV) { m = std::min(m, f.get(y) + (x >= y ? s2 : s1) * (x - y)); } }
        return m;});}
Fn bMerge(const Fn &f, const Fn &g) { return Fn::make([&](int x) { return addv(f.get(x), g.get(x)); }); }
Fn bConv(const Fn &f, const Fn &g, lll A) {
    return Fn::make([&](int z) {
        lll m = INFV;
        for (lll x = -A; x <= A; ++x) { m = std::min(m, addv(f.at(x), g.at(z - x))); }
        for (lll x = z - A; x <= z + A; ++x) { m = std::min(m, addv(f.at(x), g.at(z - x))); }
        return m;});}
// Every integer of [-e - 2, e + 2] when dense (small extent or the last step of a sequence), else 20 of them.
bool dense = false;
vector<lll> points(lll e) {
    vector<lll> res;
    if (dense || e <= 16) { for (lll x = -e - 2; x <= e + 2; ++x) { res.push_back(x); } }
    else { for (int i = 0; i < 20; ++i) { res.push_back(rnd(-lng(e) - 2, lng(e) + 2)); } }
    for (lll x : {lll(-B - 37), lll(-B), lll(B), lll(B + 37)}) { res.push_back(x); }
    return res;}

// Positions map to s * x + t, weights and slopes scale by u, values by s * u.
template<class T>
struct Map {
    T s, t; lll u;
    static constexpr T INF = std::numeric_limits<T>::max();
    T pos(lll x) const {
        if (x <= -INFV) { return -INF; }
        if (x >= INFV) { return INF; }
        if constexpr (std::is_floating_point_v<T>) { return s * T(x) + t; } else { return T(lll(s) * x + t); }}
    T val(lll v) const {
        if (v >= INFV) { return INF; }
        if constexpr (std::is_floating_point_v<T>) { return s * T(u * v); } else { return T(lll(s) * u * v); }}
    T len(lll d) const { if constexpr (std::is_floating_point_v<T>) { return s * T(d); } else { return T(lll(s) * d); } }
    T wt(lll w) const { return T(u * w); }
    string name() const { return "map(s=" + show(s) + ",t=" + show(t) + ",u=" + number(u) + ")"; }
};

template<class T>
void verify(const SlopeTrick<T> &st, const Fn &f, lll e, const Map<T> &m) {
    check(f.convex() && f.bounded() && f.extent() <= e, "oracle invariants (convex, bounded, extent)");
    checkEqual(lll(st.size()), f.rightSlope() - f.leftSlope(), "size");
    checkEqual(st.min(), m.val(f.minv()), "min");
    auto [l, r] = f.argmin();
    checkEqual(st.argminRange(), pair<T, T>{m.pos(l), m.pos(r)}, "argminRange");
    for (lll x : points(e)) { checkEqual(st.evaluate(m.pos(x)), m.val(f.at(x)), "evaluate(" + number(x) + ")"); }}

template<class T>
void verify(const WeightedSlopeTrick<T> &st, const Fn &f, lll e, const Map<T> &m) {
    check(f.convex() && f.extent() <= e, "oracle invariants (convex, extent)");
    auto [dl, dr] = f.domain();
    checkEqual(st.domain(), pair<T, T>{m.pos(dl), m.pos(dr)}, "domain");
    for (lll x : points(e)) { checkEqual(st.evaluate(m.pos(x)), m.val(f.at(x)), "evaluate(" + number(x) + ")"); }
    check(st.bounded() == f.bounded(), "bounded state");
    if (f.bounded()) {
        checkEqual(st.min(), m.val(f.minv()), "min");
        auto [l, r] = f.argmin();
        checkEqual(st.argminRange(), pair<T, T>{m.pos(l), m.pos(r)}, "argminRange");}}

template<class T>
lll dualAt(const DualSlopeTrick<T> &d, T x) { DualSlopeTrick<T> c = d; c.shift(-x); T v = c.atZero(); return v == c.INF ? INFV : lll(v); }

void verify(const DualSlopeTrick<lng> &d, const Fn &f, lll e, const Map<lng> &m) {
    check(f.convex() && f.extent() <= e, "oracle invariants (convex, extent)");
    auto [dl, dr] = f.domain();
    check(dl > -INFV && dr < INFV, "oracle domain finite");
    checkEqual(d.domain(), pair<lng, lng>{m.pos(dl), m.pos(dr)}, "domain");
    checkEqual(d.atZero(), m.val(f.at(-lll(m.t))), "atZero");
    vector<lll> xs = {dl - 2, dl - 1, dl, dl + 1, dr - 1, dr, dr + 1, dr + 2};
    if (dense || dr - dl <= 16) { for (lll x = dl; x <= dr; ++x) { xs.push_back(x); } }
    else { for (int i = 0; i < 12; ++i) { xs.push_back(rnd(lng(dl), lng(dr))); } }
    for (lll x : xs) {
        lll want = f.at(x) >= INFV ? INFV : lll(m.val(f.at(x)));
        checkEqual(dualAt(d, m.pos(x)), want, "f(" + number(x) + ") via shift and atZero");}}

// One random unit-slope operation on (st, f); returns false when the window bound would be exceeded.
template<class T>
bool stepSlope(SlopeTrick<T> &st, Fn &f, lll &e, const Map<T> &m, int depth) {
    int op = int(rnd(0, depth ? 8 : 10));
    lll a = rnd(-P, P), b = rnd(-4, 4), c = rnd(-4, 4);
    if (b > c) { std::swap(b, c); }
    auto log = [&](const string &s) { context += s + ";"; ++applied; };
    if (op == 0) { lll k = rnd(-20, 20); st.addConst(m.val(k)); f = bAdd(f, [&](lll) { return k; }); log("addConst(" + number(k) + ")"); }
    if (op == 1) {
        lll k = rnd(-lng(std::min(f.rightSlope(), lll(3))), lng(std::min(-f.leftSlope(), lll(3))));
        st.addLinear(T(k)); st.addConst(T(-m.t * T(k)));
        f = bAdd(f, [&](lll x) { return k * x; }); log("addLinear(" + number(k) + ")");}
    if (op >= 2 && op <= 4) {
        if (std::max(e, lll(abs(lng(a)))) > LIMIT) { return false; }
        e = std::max(e, lll(abs(lng(a))));
        if (op == 2) { st.addXMinusA(m.pos(a)); f = bAdd(f, [&](lll x) { return max(lll(0), x - a); }); log("addXMinusA(" + number(a) + ")"); }
        if (op == 3) { st.addAMinusX(m.pos(a)); f = bAdd(f, [&](lll x) { return max(lll(0), a - x); }); log("addAMinusX(" + number(a) + ")"); }
        if (op == 4) { st.addAbs(m.pos(a)); f = bAdd(f, [&](lll x) { return x > a ? x - a : a - x; }); log("addAbs(" + number(a) + ")"); }}
    if (op == 5) { st.prefixMin(); f = bPrefixMin(f); log("prefixMin"); }
    if (op == 6) { st.suffixMin(); f = bSuffixMin(f); log("suffixMin"); }
    if (op == 7) {
        if (e + abs(lng(b)) > LIMIT) { return false; }
        e += abs(lng(b)); st.shift(m.len(b)); f = bShift(f, b); log("shift(" + number(b) + ")");}
    if (op == 8) {
        if (e + std::max(abs(lng(b)), abs(lng(c))) > LIMIT) { return false; }
        e += std::max(abs(lng(b)), abs(lng(c))); st.slide(m.len(b), m.len(c)); f = bSlide(f, b, c);
        log("slide(" + number(b) + "," + number(c) + ")");}
    if (op == 9) {
        SlopeTrick<T> g; Fn fg; lll eg = 0; string saved = context; context = "";
        for (int i = int(rnd(0, 6)); i > 0; --i) { if (!stepSlope(g, fg, eg, m, depth + 1)) { break; } }
        string inner = context; context = saved;
        if (std::max(e, eg) > LIMIT) { return false; }
        e = std::max(e, eg); f = bMerge(f, fg);
        if (rnd(0, 1)) { st.merge(std::move(g)); } else { st.merge(g); }
        log("merge{" + inner + "}");}
    if (op == 10) { st.merge(st); f = bMerge(f, f); log("merge(self copy)"); }
    return true;}

template<class T>
void randomSlope(int rounds, int maxOps, const Map<T> &m, const string &label) {
    for (int round = 0; round < rounds; ++round) {
        SlopeTrick<T> st; Fn f; lll e = 0; context = label + " " + m.name() + ":"; ++sequences;
        verify(st, f, e, m);
        for (int i = int(rnd(1, maxOps)); i > 0; --i) {
            if (!stepSlope(st, f, e, m, 0)) { ++truncated; break; }
            dense = i == 1; verify(st, f, e, m); dense = false;}}}

template<class T>
bool stepWeighted(WeightedSlopeTrick<T> &st, Fn &f, lll &e, const Map<T> &m, int depth) {
    int op = int(rnd(0, depth ? 9 : 12));
    lll a = rnd(-P, P), w = rnd(0, 6), b = rnd(-4, 4), c = rnd(-4, 4);
    if (b > c) { std::swap(b, c); }
    auto log = [&](const string &s) { context += s + ";"; ++applied; };
    auto grow = [&](lll x) { if (std::max(e, lll(abs(lng(x)))) > LIMIT) { return false; } e = std::max(e, lll(abs(lng(x)))); return true; };
    if (op == 0) { lll k = rnd(-20, 20); st.addConst(m.val(k)); f = bAdd(f, [&](lll) { return k; }); log("addConst(" + number(k) + ")"); }
    if (op == 1) {
        lll k = rnd(-6, 6);
        st.addLinear(m.wt(k)); st.addConst(T(-lll(m.t) * m.wt(k)));
        f = bAdd(f, [&](lll x) { return k * x; }); log("addLinear(" + number(k) + ")");}
    if (op >= 2 && op <= 4) {
        if (!grow(a)) { return false; }
        string args = "(" + number(a) + "," + number(w) + ")";
        if (op == 2) { st.addXMinusA(m.pos(a), m.wt(w)); f = bAdd(f, [&](lll x) { return w * max(lll(0), x - a); }); log("addXMinusA" + args); }
        if (op == 3) { st.addAMinusX(m.pos(a), m.wt(w)); f = bAdd(f, [&](lll x) { return w * max(lll(0), a - x); }); log("addAMinusX" + args); }
        if (op == 4) { st.addAbs(m.pos(a), m.wt(w)); f = bAdd(f, [&](lll x) { return w * (x > a ? x - a : a - x); }); log("addAbs" + args); }}
    if (op == 5) {
        if (!f.boundedLeft()) { return true; }
        st.prefixMin(); f = bPrefixMin(f); log("prefixMin");}
    if (op == 6) {
        if (!f.boundedRight()) { return true; }
        st.suffixMin(); f = bSuffixMin(f); log("suffixMin");}
    if (op == 7) {
        if (e + abs(lng(b)) > LIMIT) { return false; }
        e += abs(lng(b)); st.shift(m.len(b)); f = bShift(f, b); log("shift(" + number(b) + ")");}
    if (op == 8) {
        if (e + std::max(abs(lng(b)), abs(lng(c))) > LIMIT) { return false; }
        e += std::max(abs(lng(b)), abs(lng(c))); st.slide(m.len(b), m.len(c)); f = bSlide(f, b, c);
        log("slide(" + number(b) + "," + number(c) + ")");}
    if (op == 9) {
        auto [dl, dr] = f.domain();
        lll x0 = std::clamp(lll(rnd(-P, P)), std::max(dl, lll(-LIMIT)), std::min(dr, lll(LIMIT)));
        lll l = rnd(0, 3) ? x0 - rnd(0, 6) : -INFV, r = rnd(0, 3) ? x0 + rnd(0, 6) : INFV;
        if ((l > -INFV && !grow(l)) || (r < INFV && !grow(r))) { return false; }
        st.restrictDomain(m.pos(l), m.pos(r)); f = bRestrict(f, l, r);
        log("restrictDomain(" + (l > -INFV ? number(l) : "-INF") + "," + (r < INFV ? number(r) : "INF") + ")");}
    if (op == 10) {
        if (!f.bounded()) { return true; }
        lll s1 = -rnd(0, 3), s2 = rnd(0, 3);
        st.clampSlope(m.wt(s1), m.wt(s2)); f = bClamp(f, s1, s2);
        log("clampSlope(" + number(s1) + "," + number(s2) + ")");}
    if (op == 11 || op == 12) {
        Map<T> mg = m;
        if (op == 12) { mg.t = 0; }
        WeightedSlopeTrick<T> g; Fn fg; lll eg = 0; string saved = context; context = "";
        for (int i = int(rnd(0, 8)); i > 0; --i) { if (!stepWeighted(g, fg, eg, mg, depth + 1)) { break; } }
        string inner = context; context = saved;
        if (op == 11) {
            auto [l1, r1] = f.domain(); auto [l2, r2] = fg.domain();
            if (std::max(l1, l2) > std::min(r1, r2) || std::max(e, eg) > LIMIT) { return true; }
            e = std::max(e, eg); f = bMerge(f, fg);
            if (rnd(0, 1)) { st.merge(std::move(g)); } else { st.merge(g); }
            log("merge{" + inner + "}");}
        else {
            if (!f.bounded() || !fg.bounded()) { return true; }
            if (e + eg > LIMIT) { return false; }
            f = bConv(f, fg, std::max(e, eg) + 1); e += eg;
            if (rnd(0, 1)) { st.minPlusConvolution(std::move(g)); } else { st.minPlusConvolution(g); }
            log("minPlusConvolution{" + inner + "}");}}
    return true;}

template<class T>
void randomWeighted(int rounds, int maxOps, const Map<T> &m, const string &label) {
    for (int round = 0; round < rounds; ++round) {
        WeightedSlopeTrick<T> st; Fn f; lll e = 0; context = label + " " + m.name() + ":"; ++sequences;
        verify(st, f, e, m);
        for (int i = int(rnd(1, maxOps)); i > 0; --i) {
            if (!stepWeighted(st, f, e, m, 0)) { ++truncated; break; }
            dense = i == 1; verify(st, f, e, m); dense = false;}}}

void randomDual(int rounds, int maxOps, const Map<lng> &m) {
    for (int round = 0; round < rounds; ++round) {
        DualSlopeTrick<lng> d; Fn f = delta(); lll e = 0; context = "dual " + m.name() + ":"; ++sequences;
        d.shift(m.t);
        verify(d, f, e, m);
        for (int i = int(rnd(1, maxOps)); i > 0; --i) {
            int op = int(rnd(0, 5));
            lll a = rnd(-P, P), w = rnd(0, 6), b = rnd(-4, 4), c = rnd(-4, 4);
            if (b > c) { std::swap(b, c); }
            if (op == 0) { lll k = rnd(-20, 20); d.addConst(m.val(k)); f = bAdd(f, [&](lll) { return k; }); context += "addConst(" + number(k) + ");"; }
            if (op == 1) {
                lll k = rnd(-6, 6);
                d.addLinear(m.wt(k)); d.addConst(lng(-lll(m.t) * m.wt(k)));
                f = bAdd(f, [&](lll x) { return k * x; }); context += "addLinear(" + number(k) + ");";}
            if (op == 2 || op == 3) {
                if (std::max(e, lll(abs(lng(a)))) > LIMIT) { ++truncated; break; }
                e = std::max(e, lll(abs(lng(a))));
                if (op == 2) { d.addXMinusA(m.pos(a), m.wt(w)); f = bAdd(f, [&](lll x) { return w * max(lll(0), x - a); }); }
                else { d.addAMinusX(m.pos(a), m.wt(w)); f = bAdd(f, [&](lll x) { return w * max(lll(0), a - x); }); }
                context += string(op == 2 ? "addXMinusA(" : "addAMinusX(") + number(a) + "," + number(w) + ");";}
            if (op == 4) {
                if (e + abs(lng(b)) > LIMIT) { ++truncated; break; }
                e += abs(lng(b)); d.shift(lng(b)); f = bShift(f, b); context += "shift(" + number(b) + ");";}
            if (op == 5) {
                if (e + std::max(abs(lng(b)), abs(lng(c))) > LIMIT) { ++truncated; break; }
                e += std::max(abs(lng(b)), abs(lng(c))); d.slide(lng(b), lng(c)); f = bSlide(f, b, c);
                context += "slide(" + number(b) + "," + number(c) + ");";}
            ++applied;
            dense = i == 1; verify(d, f, e, m); dense = false;}}}

// Treap balance guard: a degenerate shape (e.g. duplicates ordered by priority) fails here, not only by timeout.
template<class T> void checkDepth(const WeightedSlopeTrick<T> &w, const string &what) {
    vector<pair<int, int>> stack; int depth = 0;
    if (w.root) { stack.push_back({w.root, 1}); }
    while (!stack.empty()) {
        auto [v, d] = stack.back(); stack.pop_back(); depth = std::max(depth, d);
        for (int c : {w.t[v].l, w.t[v].r}) { if (c) { stack.push_back({c, d + 1}); } }}
    check(depth <= 6 * std::bit_width(unsigned(w.nodes())) + 16, what + " treap depth " + std::to_string(depth) + " for " + std::to_string(w.nodes()) + " nodes");}

// Hand-computed fixtures, independent of the window oracle (and validating it).
void fixtures() {
    context = "fixtures";
    using W = WeightedSlopeTrick<lng>;
    constexpr lng INF = std::numeric_limits<lng>::max();
    SlopeTrick<lng> s;
    checkEqual(s.argminRange(), pair<lng, lng>{-INF, INF}, "empty argminRange");
    checkEqual(s.min(), lng(0), "empty min"); checkEqual(s.evaluate(-5), lng(0), "empty evaluate"); checkEqual(s.size(), 0, "empty size");
    s.addAbs(3); s.addAbs(7); s.addXMinusA(5);
    checkEqual(s.min(), lng(4), "|x-3|+|x-7|+(x-5)+ min"); checkEqual(s.argminRange(), pair<lng, lng>{3, 5}, "argmin [3,5]");
    checkEqual(s.evaluate(0), lng(10), "f(0)=10"); checkEqual(s.evaluate(9), lng(12), "f(9)=12");
    s.addLinear(-1);
    checkEqual(s.min(), lng(-1), "after -x min=-1"); checkEqual(s.argminRange(), pair<lng, lng>{5, 7}, "after -x argmin [5,7]");
    Fn f = bAdd(bAdd(bAdd(Fn(), [](lll x) { return x > 3 ? x - 3 : 3 - x; }), [](lll x) { return x > 7 ? x - 7 : 7 - x; }), [](lll x) { return max(lll(0), x - 5) - x; });
    checkEqual(f.minv(), lll(-1), "oracle fixture min"); checkEqual(f.argmin(), pair<lll, lll>{5, 7}, "oracle fixture argmin");
    checkEqual(f.at(1000), lll(1000 - 3 + 1000 - 7 + 995 - 1000), "oracle extrapolation right");
    checkEqual(f.at(-1000), lll(1003 + 1007 + 1000), "oracle extrapolation left");
    W w;
    checkEqual(w.domain(), pair<lng, lng>{-INF, INF}, "default domain"); checkEqual(w.argminRange(), pair<lng, lng>{-INF, INF}, "empty weighted argmin");
    w.addAbs(3, 2); w.addAbs(7, 1); w.addXMinusA(5, 3);
    checkEqual(w.min(), lng(4), "2|x-3|+|x-7|+3(x-5)+ min"); checkEqual(w.argminRange(), pair<lng, lng>{3, 3}, "argmin 3");
    checkEqual(w.evaluate(10), lng(14 + 3 + 15), "f(10)");
    w.restrictDomain(5, 9);
    checkEqual(w.domain(), pair<lng, lng>{5, 9}, "restricted domain"); checkEqual(w.min(), lng(6), "restricted min");
    checkEqual(w.argminRange(), pair<lng, lng>{5, 5}, "restricted argmin"); checkEqual(w.evaluate(4), INF, "outside domain is INF");
    w.prefixMin();
    checkEqual(w.domain(), pair<lng, lng>{5, INF}, "prefixMin opens right"); checkEqual(w.evaluate(100), lng(6), "flat after prefixMin");
    W e; e.addLinear(2);
    check(!e.bounded(), "2x is unbounded"); checkEqual(e.evaluate(-7), lng(-14), "2x at -7");
    e.restrictDomain(-3, INF);
    checkEqual(e.min(), lng(-6), "2x on [-3,inf) min"); checkEqual(e.argminRange(), pair<lng, lng>{-3, -3}, "2x argmin");
    W h; h.addAbs(0, 5); h.clampSlope(-2, 1);
    checkEqual(h.evaluate(-10), lng(20), "clamp left slope 2"); checkEqual(h.evaluate(10), lng(10), "clamp right slope 1");
    W c1, c2; c1.addAbs(0); c2.restrictDomain(2, 4);
    c1.minPlusConvolution(c2);
    checkEqual(c1.min(), lng(0), "|x| conv [2,4] min"); checkEqual(c1.argminRange(), pair<lng, lng>{2, 4}, "|x| conv [2,4] argmin");
    checkEqual(c1.evaluate(7), lng(3), "|x| conv [2,4] at 7"); checkEqual(c1.domain(), pair<lng, lng>{-INF, INF}, "conv domain");
    const lng Q = (lng(1) << 62) - 10;
    W big; big.addAbs(Q); big.addAbs(-Q);
    checkEqual(big.min(), lng(2 * lll(Q)), "|x-Q|+|x+Q| near 2^62"); checkEqual(big.argminRange(), pair<lng, lng>{-Q, Q}, "extreme argmin");
    checkEqual(big.evaluate(Q), lng(2 * lll(Q)), "extreme evaluate");
    SlopeTrick<lng> sb; sb.addAbs(Q); sb.addAbs(-Q); sb.shift(-5);
    checkEqual(sb.min(), lng(2 * lll(Q)), "unit extreme min"); checkEqual(sb.argminRange(), pair<lng, lng>{-Q - 5, Q - 5}, "unit extreme argmin");
    DualSlopeTrick<lng> d;
    checkEqual(d.domain(), pair<lng, lng>{0, 0}, "dual initial domain"); checkEqual(d.atZero(), lng(0), "dual initial f(0)");
    d.shift(3);
    checkEqual(d.atZero(), INF, "dual f(0) outside domain"); checkEqual(d.domain(), pair<lng, lng>{3, 3}, "dual shifted domain");
    d.slide(-5, 1);
    checkEqual(d.domain(), pair<lng, lng>{-2, 4}, "dual slide domain"); checkEqual(d.atZero(), lng(0), "dual slide f(0)");
    d.addXMinusA(1, lng(1) << 50);
    checkEqual(dualAt(d, lng(4)), lll(3) << 50, "dual huge weight f(4)");
    cout << "PASS fixtures (hand-computed values, oracle extrapolation, INF sentinels, 2^62 extremes)\n";}

// Classic DPs with independent dense-table oracles on [0, V).
void classics(int rounds, int n, int V) {
    for (int round = 0; round < rounds; ++round) {
        int m = int(rnd(1, n)), v = int(rnd(1, V)), D = int(rnd(0, 5)), cost = int(rnd(0, 4));
        vector<lng> a(m), c(m);
        for (int i = 0; i < m; ++i) { a[i] = rnd(0, v - 1); c[i] = rnd(1, 6); }
        context = "classic n=" + std::to_string(m) + " V=" + std::to_string(v) + " D=" + std::to_string(D);
        for (int dir = 0; dir < 2; ++dir) {
            vector<lll> dp(v, 0), du(v, 0);
            WeightedSlopeTrick<lng> w; SlopeTrick<lng> u;
            for (int i = 0; i < m; ++i) {
                if (dir == 0) { for (int x = 1; x < v; ++x) { dp[x] = std::min(dp[x], dp[x - 1]); du[x] = std::min(du[x], du[x - 1]); } w.prefixMin(); u.prefixMin(); }
                else { for (int x = v - 2; x >= 0; --x) { dp[x] = std::min(dp[x], dp[x + 1]); du[x] = std::min(du[x], du[x + 1]); } w.suffixMin(); u.suffixMin(); }
                for (int x = 0; x < v; ++x) { dp[x] += c[i] * abs(x - a[i]); du[x] += abs(x - a[i]); }
                w.addAbs(a[i], c[i]); u.addAbs(a[i]);}
            checkEqual(lll(w.min()), *std::min_element(dp.begin(), dp.end()), "monotone fit weighted min");
            checkEqual(lll(u.min()), *std::min_element(du.begin(), du.end()), "monotone fit unit min");
            for (int x = 0; x < v; ++x) {
                checkEqual(lll(w.evaluate(x)), dp[x], "monotone fit weighted f(" + std::to_string(x) + ")");
                checkEqual(lll(u.evaluate(x)), du[x], "monotone fit unit f(" + std::to_string(x) + ")");}}
        {
            vector<lll> dp(v, 0), dq(v, 0);
            WeightedSlopeTrick<lng> w, q;
            for (int i = 0; i < m; ++i) {
                if (i) {
                    vector<lll> nd(v, INFV), nq(v, INFV);
                    for (int x = 0; x < v; ++x) {
                        for (int y = std::max(0, x - D); y <= std::min(v - 1, x + D); ++y) { nd[x] = std::min(nd[x], dp[y]); }
                        for (int y = 0; y < v; ++y) { nq[x] = std::min(nq[x], dq[y] + lll(cost) * abs(x - y)); }}
                    dp = nd; dq = nq; w.slide(-D, D); q.clampSlope(-cost, cost);}
                for (int x = 0; x < v; ++x) { dp[x] += c[i] * abs(x - a[i]); dq[x] += c[i] * abs(x - a[i]); }
                w.addAbs(a[i], c[i]); q.addAbs(a[i], c[i]);}
            checkEqual(lll(w.min()), *std::min_element(dp.begin(), dp.end()), "bounded-step tracking min");
            checkEqual(lll(q.min()), *std::min_element(dq.begin(), dq.end()), "linear-move-cost tracking min");
            for (int x = 0; x < v; ++x) {
                checkEqual(lll(w.evaluate(x)), dp[x], "bounded-step f(" + std::to_string(x) + ")");
                checkEqual(lll(q.evaluate(x)), dq[x], "linear-move-cost f(" + std::to_string(x) + ")");}}}
    cout << "PASS classic DPs rounds=" << rounds << " (monotone fit both directions, bounded step, linear move cost)\n";}

// APIO 2016 Fireworks: equalize leaf depths by changing edge lengths to any value >= 0 at cost |new - old|.
void fireworks(int rounds, int n) {
    for (int round = 0; round < rounds; ++round) {
        int m = int(rnd(1, n));
        vector<int> par(m, -1); vector<lng> len(m, 0); vector<vector<int>> ch(m);
        for (int i = 1; i < m; ++i) { par[i] = int(rnd(0, i - 1)); len[i] = rnd(0, 8); ch[par[i]].push_back(i); }
        int D = 0;
        for (int i = 1; i < m; ++i) { D = std::max(D, int(len[i])); }
        D = 2 * (m * D + 4);
        context = "fireworks n=" + std::to_string(m);
        vector<vector<lll>> bf(m); vector<WeightedSlopeTrick<lng>> st(m);
        for (int v = m - 1; v >= 0; --v) {
            bf[v].assign(D + 1, ch[v].empty() ? INFV : 0);
            if (ch[v].empty()) { bf[v][0] = 0; st[v].restrictDomain(0, 0); }
            for (int c : ch[v]) {
                for (int x = 0; x <= D; ++x) {
                    lll best = INFV;
                    for (int y = 0; y <= x; ++y) { if (bf[c][y] < INFV) { best = std::min(best, bf[c][y] + abs(x - y - len[c])); } }
                    bf[v][x] = addv(bf[v][x], best);}
                WeightedSlopeTrick<lng> edge; edge.restrictDomain(0, std::numeric_limits<lng>::max()); edge.addAbs(len[c]);
                st[c].minPlusConvolution(std::move(edge));
                st[v].merge(std::move(st[c]));}}
        checkEqual(lll(st[0].min()), *std::min_element(bf[0].begin(), bf[0].end()), "fireworks min");
        checkDepth(st[0], "fireworks");
        for (int x = 0; x <= D; ++x) {
            lll got = st[0].evaluate(x);
            checkEqual(got == std::numeric_limits<lng>::max() ? INFV : got, bf[0][x], "fireworks f(" + std::to_string(x) + ")");}}
    cout << "PASS Fireworks tree DP rounds=" << rounds << " (minPlusConvolution, merge, restrictDomain)\n";}

// Choose exactly K of n items (cost q chosen, p otherwise): base + K smallest (q - p), by sorting.
void chooseK(int rounds, int n, lng magnitude) {
    for (int round = 0; round < rounds; ++round) {
        int m = int(rnd(0, n));
        vector<lng> p(m), q(m), diff(m);
        lll base = 0;
        for (int i = 0; i < m; ++i) { p[i] = rnd(-magnitude, magnitude); q[i] = rnd(-magnitude, magnitude); diff[i] = q[i] - p[i]; base += p[i]; }
        sort(diff.begin(), diff.end());
        vector<lll> want(m + 1, base);
        for (int k = 1; k <= m; ++k) { want[k] = want[k - 1] + diff[k - 1]; }
        context = "chooseK n=" + std::to_string(m) + " magnitude=" + std::to_string(magnitude);
        DualSlopeTrick<lng> d; WeightedSlopeTrick<lng> w; w.restrictDomain(0, 0);
        for (int i = 0; i < m; ++i) {
            lng s = q[i] - p[i];
            d.addLinear(-s); d.slide(0, 1); d.addLinear(s); d.addConst(p[i]);
            w.addLinear(-s); w.slide(0, 1); w.addLinear(s); w.addConst(p[i]);}
        checkEqual(d.domain(), pair<lng, lng>{0, m}, "dual domain [0,n]"); checkEqual(w.domain(), pair<lng, lng>{0, m}, "weighted domain [0,n]");
        checkEqual(lll(w.min()), *std::min_element(want.begin(), want.end()), "weighted min over K");
        for (int k = 0; k <= m; ++k) {
            checkEqual(lll(d.atZero()), want[k], "dual f(" + std::to_string(k) + ") after shifting by -k");
            checkEqual(lll(w.evaluate(k)), want[k], "weighted f(" + std::to_string(k) + ")");
            d.shift(-1);}
        checkEqual(d.atZero(), std::numeric_limits<lng>::max(), "dual f(n + 1) outside domain");}
    cout << "PASS choose-K closed form rounds=" << rounds << " magnitude=" << magnitude << '\n';}

// Weighted median oracle on sorted unit points plus heavy alternating extremes (heap-of-pairs ping-pong pattern).
void pingPong(int n) {
    context = "ping-pong n=" + std::to_string(n);
    const lng M = 1000000, H = 1000000;
    WeightedSlopeTrick<lng> w; vector<lng> pts; lll A = 0, Bw = 0;
    for (int i = 0; i < n; ++i) {
        lng x = rnd(-M + 1, M - 1); pts.push_back(x); w.addAbs(x);
        if (i % 2) { w.addAbs(-M, H); A += H; } else { w.addAbs(M, H); Bw += H; }
        if (i % (n / 16 + 1) == 0 || i == n - 1) {
            vector<lng> s = pts; sort(s.begin(), s.end());
            lll total = lll(s.size()) + A + Bw, need = total, acc = A;
            lng med = -M;
            if (2 * acc < need) { for (lng y : s) { med = y; acc += 1; if (2 * acc >= need) { break; } } if (2 * acc < need) { med = M; } }
            lll value = A * (med + M) + Bw * (M - med);
            for (lng y : s) { value += abs(lll(y) - med); }
            checkEqual(lll(w.min()), value, "ping-pong weighted-median min");}}
    checkDepth(w, "ping-pong");
    cout << "PASS ping-pong weighted median n=" << n << " nodes=" << w.nodes() << '\n';}

// Balance regressions: duplicate keys across merges, merges with copies (shared priorities), moved-from reuse.
void balance(int n, int doublings) {
    context = "balance n=" + std::to_string(n);
    WeightedSlopeTrick<lng> w;
    for (int i = 0; i < n; ++i) { WeightedSlopeTrick<lng> s; s.addXMinusA(0); w.merge(std::move(s)); }
    checkEqual(w.evaluate(5), lng(5) * n, "equal-key merges value"); checkEqual(w.argminRange(), pair<lng, lng>{-w.INF, 0}, "equal-key merges argmin");
    checkDepth(w, "equal-key merges");
    WeightedSlopeTrick<lng> star;
    for (int i = 0; i < n; ++i) {
        WeightedSlopeTrick<lng> leaf, edge; leaf.restrictDomain(0, 0); edge.restrictDomain(0, w.INF); edge.addAbs(5);
        leaf.minPlusConvolution(std::move(edge)); star.merge(std::move(leaf));}
    checkEqual(star.min(), lng(0), "equal-edge star min"); checkEqual(star.evaluate(7), lng(2) * n, "equal-edge star f(7)");
    checkDepth(star, "equal-edge star");
    WeightedSlopeTrick<lng> base, self;
    for (int i = 0; i < 64; ++i) { base.addAbs(rnd(-1000, 1000), rnd(1, 3)); }
    self = base;
    for (int i = 0; i < doublings; ++i) { self.merge(self); }
    for (lng x : {-2000, -7, 0, 13, 2000}) { checkEqual(lll(self.evaluate(x)), lll(base.evaluate(x)) << doublings, "self-merge value"); }
    checkDepth(self, "self-merges");
    WeightedSlopeTrick<lng> walk = base;
    for (int i = 0; i < doublings; ++i) { WeightedSlopeTrick<lng> g = walk; g.shift(1); walk.merge(std::move(g)); }
    for (lng x : {-1500, 0, 1500}) {
        lll want = 0, binom = 1;
        for (int j = 0; j <= doublings; ++j) { want += binom * base.evaluate(x - j); binom = binom * (doublings - j) / (j + 1); }
        checkEqual(lll(walk.evaluate(x)), want, "shifted-copy merges value (binomial sum)");}
    checkDepth(walk, "shifted-copy merges");
    WeightedSlopeTrick<lng> a, b; a.addAbs(3); b.merge(std::move(a));
    checkEqual(a.domain(), pair<lng, lng>{-a.INF, a.INF}, "moved-from is the zero function"); checkEqual(a.evaluate(9), lng(0), "moved-from value");
    a.addAbs(2); b.minPlusConvolution(std::move(a)); a.addAbs(1);
    checkEqual(a.evaluate(5), lng(4), "moved-from reuse"); checkEqual(b.evaluate(10), lng(5), "|x-3| conv |x-2| at 10");
    cout << "PASS balance n=" << n << " doublings=" << doublings << " (equal keys, shared priorities, moved-from reuse)\n";}

// Supporting evidence only: unit SlopeTrick and WeightedSlopeTrick agree on long random sequences.
void crossCheck(int n) {
    context = "cross-check n=" + std::to_string(n);
    SlopeTrick<lng> u; WeightedSlopeTrick<lng> w; int peak = 0;
    for (int i = 0; i < n; ++i) {
        int op = int(rnd(0, 7)); lng a = rnd(-1000000000, 1000000000), b = rnd(-1000, 1000), c = rnd(-1000, 1000);
        if (b > c) { std::swap(b, c); }
        if (op == 0) { u.addXMinusA(a); w.addXMinusA(a); }
        if (op == 1) { u.addAMinusX(a); w.addAMinusX(a); }
        if (op == 2) { u.addAbs(a); w.addAbs(a); }
        if (op == 3 && rnd(0, 9) == 0) { u.prefixMin(); w.prefixMin(); }
        if (op == 4 && rnd(0, 9) == 0) { u.suffixMin(); w.suffixMin(); }
        if (op == 5) { u.shift(b); w.shift(b); }
        if (op == 6) { u.slide(b, c); w.slide(b, c); }
        if (op == 7) { lng k = rnd(-std::min(lng(3), lng(u.right.size())), std::min(lng(3), lng(u.left.size()))); u.addLinear(k); w.addLinear(k); }
        checkEqual(w.min(), u.min(), "cross min"); checkEqual(w.argminRange(), u.argminRange(), "cross argminRange");
        peak = std::max(peak, w.nodes());}
    check(int(w.t.size()) <= peak + 8, "freed nodes are reused (pool within peak live nodes + 8)");
    WeightedSlopeTrick<lng> copy = w; copy.addAbs(5, 7); copy.prefixMin();
    for (lng x : {-5, 0, 77}) { checkEqual(w.evaluate(x), u.evaluate(x), "cross evaluate after copy mutation"); }
    checkDepth(w, "cross-check");
    cout << "PASS cross-check n=" << n << " peak nodes=" << peak << " pool=" << w.t.size() - 1 << '\n';}

int main(int argc, char **argv) {
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "--mode") { mode = argv[++i]; }
        if (arg == "--seed") { test_seed = std::stoull(argv[++i]); }
        if (arg == "--invalid") {
            string probe = argv[++i];
            SlopeTrick<lng> s; WeightedSlopeTrick<lng> w, o; DualSlopeTrick<lng> d;
            if (probe == "slide-reversed") { s.slide(1, 0); }
            if (probe == "linear-range") { s.addAbs(0); s.addLinear(2); }
            if (probe == "weighted-negative-weight") { w.addXMinusA(0, -1); }
            if (probe == "weighted-slide-reversed") { w.slide(1, 0); }
            if (probe == "weighted-empty-domain") { w.restrictDomain(0, 1); w.restrictDomain(2, 3); }
            if (probe == "weighted-disjoint-merge") { w.restrictDomain(0, 1); o.restrictDomain(2, 3); w.merge(o); }
            if (probe == "weighted-min-unbounded") { w.addLinear(1); w.min(); }
            if (probe == "weighted-argmin-unbounded") { w.addLinear(-1); w.argminRange(); }
            if (probe == "weighted-prefix-unbounded") { w.addLinear(1); w.prefixMin(); }
            if (probe == "weighted-suffix-unbounded") { w.addLinear(-1); w.suffixMin(); }
            if (probe == "weighted-clamp-positive") { w.clampSlope(1, 2); }
            if (probe == "weighted-clamp-unbounded") { w.addLinear(1); w.clampSlope(-1, 1); }
            if (probe == "weighted-convolution-unbounded") { o.addLinear(1); w.minPlusConvolution(o); }
            if (probe == "dual-slide-reversed") { d.slide(1, 0); }
            if (probe == "dual-negative-weight") { d.addXMinusA(0, -1); }
            if (probe == "dual-amx-negative-weight") { d.addAMinusX(0, -1); }
            if (probe == "weighted-amx-negative-weight") { w.addAMinusX(0, -1); }
            if (probe == "weighted-abs-negative-weight") { w.addAbs(0, -1); }
            if (probe == "weighted-clamp-negative-right") { w.clampSlope(-2, -1); }
            if (probe == "weighted-convolution-this-unbounded") { w.addLinear(1); w.minPlusConvolution(o); }
            cerr << "probe returned without assertion: " << probe << '\n';
            return 0;}}
    gen.seed(test_seed); rng = Random(test_seed); cout << std::unitbuf;
    int q = mode == "quick" ? 1 : 8;
    int maxOps = mode == "quick" ? 16 : mode == "full" ? 48 : 96;
    fixtures();
    const lng T59 = lng(1) << 59;
    for (auto m : {Map<lng>{1, 0, 1}, Map<lng>{lng(1) << 40, T59, 1}, Map<lng>{lng(1) << 20, -T59, 1}, Map<lng>{3, -(lng(1) << 61), 1}}) {
        randomSlope(40 * q, maxOps, m, "unit");}
    randomSlope(20 * q, maxOps, Map<int>{3, 1 << 28, 1}, "unit int");
    randomSlope(20 * q, maxOps, Map<double>{0.5, 0.25, 1}, "unit double");
    for (auto m : {Map<lng>{1, 0, 1}, Map<lng>{lng(1) << 20, lng(1) << 40, lng(1) << 10}, Map<lng>{lng(1) << 30, -T59, 1}, Map<lng>{1, 0, lng(1) << 40}}) {
        randomWeighted(40 * q, maxOps, m, "weighted");}
    randomWeighted(20 * q, maxOps, Map<int>{3, 1 << 20, 4}, "weighted int");
    for (auto m : {Map<lng>{1, 0, 1}, Map<lng>{1, lng(1) << 40, lng(1) << 19}, Map<lng>{1, -T59, 1}}) { randomDual(30 * q, maxOps, m); }
    cout << "PASS random oracle sequences=" << sequences << " operations=" << applied << " window-truncated=" << truncated << '\n';
    classics(10 * q, mode == "quick" ? 30 : 80, mode == "quick" ? 30 : 60);
    fireworks(10 * q, mode == "quick" ? 8 : 20);
    chooseK(10 * q, mode == "quick" ? 50 : 300, 100);
    chooseK(10 * q, mode == "quick" ? 50 : 1000, 500000000000000LL);
    pingPong(mode == "quick" ? 2000 : mode == "full" ? 50000 : 200000);
    balance(mode == "quick" ? 4000 : 16000, mode == "quick" ? 8 : 12);
    crossCheck(mode == "quick" ? 20000 : mode == "full" ? 200000 : 1000000);
    cout << "PASS 34-slope_trick mode=" << mode << " seed=" << test_seed << " checks=" << checks << '\n';}
