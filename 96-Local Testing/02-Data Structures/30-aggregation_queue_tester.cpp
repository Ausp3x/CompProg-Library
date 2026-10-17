#include "../../02-Data Structures/30-aggregation_queue.hpp"

namespace {
    ulng seed = 20261010;
    string mode = "full", context;
    lng checks = 0, ops = 0;
    constexpr lng P = 998244353;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}

    // Affine maps x -> a x + b mod P; op(p, q) applies p first (Library Checker composite order); ops counts calls.
    struct Affine {
        using S = pair<lng, lng>;
        static S op(const S &p, const S &q) { ++ops; return {q.first * p.first % P, (q.first * p.second + q.second) % P}; }
        static S e() { return {1, 0}; }
    };
    struct Concat {
        using S = string;
        static S op(const S &p, const S &q) { return p + q; }
        static S e() { return ""; }
    };
    template<typename M> typename M::S naive(const std::deque<typename M::S> &q) {
        typename M::S res = M::e();
        for (auto &x : q) { res = M::op(res, x); }
        return res;}
    Affine::S value(Rng &rng, Affine *) { return {rnd(rng, 0, P - 1), rnd(rng, 0, P - 1)}; }
    string value(Rng &rng, Concat *) { return string(1, char('a' + rnd(rng, 0, 25))) + (rng() % 4 ? "" : "z"); }

    template<typename M>
    void queueRandom(Rng &rng, int steps, const string &name) {
        SWAGQueue<M> q; std::deque<typename M::S> ref;
        for (int s = 0; s < steps; ++s) {
            context = name + " queue step=" + show(s) + " size=" + show(ref.size());
            int t = int(rnd(rng, 0, 9));
            if (t < 5 || ref.empty()) { auto x = value(rng, (M *)nullptr); q.push(x); ref.push_back(x); }
            else if (t < 9) { q.pop(); ref.pop_front(); }
            else { q.clear(); ref.clear(); }
            checkEqual(q.size(), int(ref.size()), "size"); checkEqual(q.empty(), ref.empty(), "empty");
            checkEqual(q.allProd(), naive<M>(ref), "allProd");
            if (!ref.empty()) { checkEqual(q.front(), ref.front(), "front"); checkEqual(q.back(), ref.back(), "back"); }}}
    template<typename M>
    void dequeRandom(Rng &rng, int steps, const string &name) {
        SWAGDeque<M> q; std::deque<typename M::S> ref;
        for (int s = 0; s < steps; ++s) {
            context = name + " deque step=" + show(s) + " size=" + show(ref.size());
            int t = int(rnd(rng, 0, 20));
            auto x = value(rng, (M *)nullptr);
            if (t < 5 || (ref.empty() && t < 20)) { q.pushBack(x); ref.push_back(x); }
            else if (t < 10) { q.pushFront(x); ref.push_front(x); }
            else if (t < 15) { q.popFront(); ref.pop_front(); }
            else if (t < 20) { q.popBack(); ref.pop_back(); }
            else { q.clear(); ref.clear(); }
            checkEqual(q.size(), int(ref.size()), "size"); checkEqual(q.empty(), ref.empty(), "empty");
            checkEqual(q.allProd(), naive<M>(ref), "allProd");
            if (!ref.empty()) { checkEqual(q.front(), ref.front(), "front"); checkEqual(q.back(), ref.back(), "back"); }}}
    template<typename M>
    void stackRandom(Rng &rng, int steps, const string &name) {
        SWAGStack<M> q; std::deque<typename M::S> ref;
        for (int s = 0; s < steps; ++s) {
            context = name + " stack step=" + show(s) + " size=" + show(ref.size());
            int t = int(rnd(rng, 0, 9));
            if (t < 5 || ref.empty()) { auto x = value(rng, (M *)nullptr); q.push(x); ref.push_back(x); }
            else if (t < 9) { q.pop(); ref.pop_back(); }
            else { q.clear(); ref.clear(); }
            checkEqual(q.size(), int(ref.size()), "size"); checkEqual(q.empty(), ref.empty(), "empty");
            checkEqual(q.allProd(), naive<M>(ref), "allProd");
            if (!ref.empty()) { checkEqual(q.top(), ref.back(), "top"); }}}

    // Monoid-call counts stay linear on patterns that force rebuilds on alternating sides.
    void amortized(int n) {
        Rng rng(seed ^ 7);
        auto run = [&](const string &name, auto body) {
            context = "amortized " + name + " n=" + show(n);
            ops = 0; lng calls = body();
            if (ops > 8 * calls + 8) { throw std::runtime_error(context + " op calls=" + show(ops) + " operations=" + show(calls)); }
            ++checks;};
        run("deque alternate", [&] {
            SWAGDeque<Affine> q; std::deque<Affine::S> ref; lng c = 0;
            for (int i = 0; i < n; ++i) { auto x = value(rng, (Affine *)nullptr); q.pushBack(x); ref.push_back(x); ++c; }
            for (int i = 0; !ref.empty(); ++i, ++c) {
                if (i & 1) { q.popBack(); ref.pop_back(); } else { q.popFront(); ref.pop_front(); }
                if (i % 997 == 0) { lng o = ops; checkEqual(q.allProd(), naive<Affine>(ref), "allProd"); ops = o + 1; }}
            return c;});
        run("deque refill", [&] {
            SWAGDeque<Affine> q; lng c = 0;
            for (int r = 0; r < 4; ++r) {
                for (int i = 0; i < n; ++i, ++c) { r & 1 ? q.pushFront({1, i}) : q.pushBack({1, i}); }
                while (!q.empty()) { r & 1 ? q.popFront() : q.popBack(); ++c; }}
            return c;});
        run("queue", [&] {
            SWAGQueue<Affine> q; lng c = 0;
            for (int i = 0; i < 3 * n; ++i, ++c) { q.push({1, i}); if (i % 3 == 2) { q.pop(); q.pop(); c += 2; }}
            return c;});}

    void edges() {
        context = "edges";
        SWAGQueue<Concat> q; SWAGDeque<Concat> d; SWAGStack<Concat> s;
        checkEqual(q.allProd(), string(), "queue empty allProd"); checkEqual(d.allProd(), string(), "deque empty allProd"); checkEqual(s.allProd(), string(), "stack empty allProd");
        checkEqual(q.size(), 0, "queue empty size"); checkEqual(d.empty(), true, "deque empty");
        q.push("a"); q.push("b"); q.pop(); q.push("c");
        SWAGQueue<Concat> copy = q;
        q.push("d");
        checkEqual(copy.allProd(), string("bc"), "queue copy"); checkEqual(q.allProd(), string("bcd"), "queue after copy");
        d.pushBack("x"); d.popFront(); d.pushFront("y"); d.popBack();
        checkEqual(d.allProd(), string(), "deque single side switch");
        d.pushFront("m"); d.pushBack("n"); d.pushFront("l");
        checkEqual(d.allProd(), string("lmn"), "deque mixed"); checkEqual(d.front(), string("l"), "deque front"); checkEqual(d.back(), string("n"), "deque back");
        s.push("p"); s.push("q");
        SWAGStack<Concat> scopy = std::move(s);
        checkEqual(scopy.allProd(), string("pq"), "stack move"); checkEqual(scopy.top(), string("q"), "stack top");
        scopy.clear(); scopy.push("r");
        checkEqual(scopy.allProd(), string("r"), "stack reuse");
        // Pushing a reference into the structure's own storage (review regression: push read x after a reallocation).
        SWAGQueue<Concat> alias; SWAGDeque<Concat> dalias; SWAGStack<Concat> salias;
        alias.push("a"); dalias.pushBack("a"); salias.push("a");
        string want = "a";
        for (int i = 0; i < 200; ++i) {
            alias.push(alias.back()); dalias.pushBack(dalias.back()); dalias.pushFront(dalias.front()); salias.push(salias.top()); want += "a";}
        checkEqual(alias.allProd(), want, "queue aliasing push"); checkEqual(salias.allProd(), want, "stack aliasing push");
        checkEqual(dalias.allProd(), want + string(200, 'a'), "deque aliasing push");
        // Copy-list initialization of a default structure inside an aggregate.
        struct Holder { SWAGQueue<Concat> q; SWAGStack<Concat> s; int x; };
        Holder h{};
        SWAGDeque<Concat> d0 = {};
        checkEqual(h.q.allProd() + h.s.allProd() + d0.allProd(), string(), "aggregate member default construction");
        // Runtime state: ordered affine composites modulo a member modulus in all three structures.
        struct ModAffine {
            lng mod;
            using S = pair<lng, lng>;
            S op(const S &f, const S &g) const { return {g.first * f.first % mod, (g.first * f.second + g.second) % mod}; }
            S e() const { return {1, 0}; }
        };
        for (lng mod : {lng(7), lng(1000003)}) {
            ModAffine m{mod};
            SWAGQueue sq(m); SWAGDeque sd(m); SWAGStack ss(m);
            std::deque<pair<lng, lng>> rq, rd; vector<pair<lng, lng>> rs;
            auto naiveMod = [&](const auto &c) { pair<lng, lng> res{1, 0}; for (auto &f : c) { res = {f.first * res.first % mod, (f.first * res.second + f.second) % mod}; } return res; };
            for (int i = 0; i < 300; ++i) {
                pair<lng, lng> f{(i * 37 + 3) % mod, (i * 11 + 5) % mod};
                if (i % 3 == 2) { sq.pop(); rq.pop_front(); } else { sq.push(f); rq.push_back(f); }
                if (i % 4 == 0) { sd.pushFront(f); rd.push_front(f); } else if (i % 4 == 3 && !rd.empty()) { sd.popBack(); rd.pop_back(); } else { sd.pushBack(f); rd.push_back(f); }
                if (i % 5 == 4) { ss.pop(); rs.pop_back(); } else { ss.push(f); rs.push_back(f); }
                checkEqual(sq.allProd(), naiveMod(rq), "stateful queue"); checkEqual(sd.allProd(), naiveMod(rd), "stateful deque"); checkEqual(ss.allProd(), naiveMod(rs), "stateful stack");}}}

    void invalid(const string &name) {
        SWAGQueue<Concat> q; SWAGDeque<Concat> d; SWAGStack<Concat> s;
        if (name == "queue-pop-empty") { q.pop(); }
        else if (name == "queue-front-empty") { (void)q.front(); }
        else if (name == "queue-back-empty") { (void)q.back(); }
        else if (name == "deque-pop-front-empty") { d.popFront(); }
        else if (name == "deque-pop-back-empty") { d.popBack(); }
        else if (name == "deque-front-empty") { (void)d.front(); }
        else if (name == "deque-back-empty") { (void)d.back(); }
        else if (name == "stack-pop-empty") { s.pop(); }
        else if (name == "stack-top-empty") { (void)s.top(); }
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
        std::cout << "aggregation_queue seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int steps = mode == "quick" ? 2000 : mode == "full" ? 100000 : 600000;
        edges();
        queueRandom<Affine>(rng, steps, "affine"); queueRandom<Concat>(rng, steps / 10, "concat");
        dequeRandom<Affine>(rng, steps, "affine"); dequeRandom<Concat>(rng, steps / 10, "concat");
        stackRandom<Affine>(rng, steps, "affine"); stackRandom<Concat>(rng, steps / 10, "concat");
        amortized(mode == "quick" ? 2000 : mode == "full" ? 200000 : 1000000);
        std::cout << "PASS aggregation_queue checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL aggregation_queue seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
