#include "../../02-Data Structures/07-ordered_set.hpp"

namespace {
    ulng seed = 20260927;
    string mode = "full", context;
    lng checks = 0;
    void check(bool ok, const string &operation) {
        ++checks;
        if (!ok) { throw std::runtime_error(context + " operation=" + operation + " expected=true actual=false"); }}
    template<typename T> string repr(const vector<T> &a) {
        std::ostringstream s; s << '[';
        for (int i = 0; i < int(a.size()); ++i) { s << (i ? "," : "") << a[i]; }
        s << ']'; return s.str();}
    struct Order {
        bool descending = false;
        lng width = 1;
        bool operator()(lng a, lng b) const {
            a /= width; b /= width; return descending ? a > b : a < b;}
    };
    template<typename T, typename C>
    void staticCase(const vector<T> &a, const vector<T> &probes, C cmp) {
        SortedVector<T, C> v(a, cmp);
        CoordinateCompression<T, C> c(a, cmp);
        std::multiset<T, C> all(a.begin(), a.end(), cmp);
        std::set<T, C> distinct(a.begin(), a.end(), cmp);
        auto same = [&](const T &x, const T &y) { return !cmp(x, y) && !cmp(y, x); };
        check(v.size() == int(a.size()) && c.size() == int(distinct.size()), "static size");
        check(v.empty() == a.empty() && c.empty() == a.empty(), "static empty");
        vector<int> encoded = c.encode(probes), ranked = c.ranks(probes);
        check(int(ranked.size()) == int(probes.size()) && c.ranks({}).empty(), "bulk ranks size");
        for (int j = 0; j < int(probes.size()); ++j) {
            const T &x = probes[j];
            int less = 0, eq = 0, unique_less = 0, unique_eq = 0;
            for (const T &y : a) { less += cmp(y, x); eq += same(y, x); }
            for (const T &y : distinct) { unique_less += cmp(y, x); unique_eq += same(y, x); }
            check(v.rank(x) == less && v.upperRank(x) == less + eq && v.count(x) == eq, "sorted rank/count");
            check(v.index(x) == (eq ? less : -1), "sorted exact index");
            check(c.rank(x) == unique_less && c.upperRank(x) == unique_less + unique_eq, "compressed rank");
            check(c.count(x) == unique_eq && c.index(x) == (unique_eq ? unique_less : -1), "compressed count/index");
            check(encoded[j] == c.index(x), "batch encoding");
            check(ranked[j] == unique_less, "bulk ranks linear oracle");}
        for (const T &l : probes) {
            for (const T &r : probes) {
                if (cmp(r, l)) { continue; }
                for (bool lc : {false, true}) { for (bool rc : {false, true}) {
                    int start = 0, end = 0, inside = 0;
                    for (const T &y : distinct) {
                        bool after = lc ? !cmp(y, l) : cmp(l, y), before = rc ? !cmp(r, y) : cmp(y, r);
                        start += !after; end += before; inside += after && before;}
                    auto [lo, hi] = c.pointRange(l, r, lc, rc);
                    check(lo == start && hi == max(start, end) && hi - lo == inside, "pointRange linear oracle closed=" + std::to_string(lc) + std::to_string(rc));}}}}
        int i = 0;
        for (const T &x : all) { check(same(*v.findByOrder(i++), x), "sorted select"); }
        i = 0;
        for (const T &x : distinct) { check(same(*c.findByOrder(i), x) && same(c.decode(i), x), "compressed select/decode"); ++i; }
        check(std::equal(v.begin(), v.end(), all.begin(), all.end(), same), "sorted begin/end iteration");
        check(std::equal(c.begin(), c.end(), distinct.begin(), distinct.end(), same), "compressed begin/end iteration");
        for (int k : {-1, v.size(), INT_MAX}) { check(v.findByOrder(k) == v.end(), "sorted missing select"); }
        for (int k : {-1, c.size(), INT_MAX}) { check(c.findByOrder(k) == c.end(), "compressed missing select"); }
        auto copy = v; auto moved = std::move(copy);
        moved.rebuild({}); check(moved.empty() && v.size() == int(a.size()), "sorted copy/move/rebuild");
        auto cc = c; auto cm = std::move(cc);
        cm.rebuild(probes); check(c.size() == int(distinct.size()), "compression copy independence");
        std::set<T, C> replaced(probes.begin(), probes.end(), cmp);
        check(std::equal(cm.begin(), cm.end(), replaced.begin(), replaced.end(), same), "compression rebuild replaces values");
        cm.rebuild({}); check(cm.empty(), "compression empty rebuild");
        v.rebuild(v.v); check(v.size() == int(a.size()), "sorted alias rebuild");
        c.rebuild(c.v); check(c.size() == int(distinct.size()), "compressed alias rebuild");}

    void staticTests(std::mt19937_64 &rng) {
        int limit = mode == "quick" ? 4 : mode == "full" ? 6 : 8;
        for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
            for (int code = 0; code < count; ++code) {
                vector<lng> a(n); int x = code;
                for (lng &y : a) { y = x % 3 - 1; x /= 3; }
                context = "static exhaustive input=" + repr(a);
                staticCase(a, vector<lng>{-2,-1,0,1,2}, std::less<lng>{});}}
        for (Order cmp : {Order{false,1}, Order{true,1}, Order{false,10}, Order{true,10}}) {
            vector<lng> a{10,11,0,3,-1,-11,20,11,0}, probes{-25,-10,-1,0,5,10,15,20,25};
            context = "stateful static comparator descending=" + std::to_string(cmp.descending) + " width=" + std::to_string(cmp.width);
            staticCase(a, probes, cmp);}
        context = "signed extrema";
        staticCase(vector<lng>{LLONG_MIN,LLONG_MAX,0,LLONG_MIN}, vector<lng>{LLONG_MIN,LLONG_MAX,-1,0,1}, std::less<lng>{});
        context = "string keys";
        staticCase(vector<string>{"z","","aa","aa","a"}, vector<string>{"","a","aa","b","z","zz"}, std::less<string>{});
        context = "packed bool keys";
        staticCase(vector<bool>{true,false,true,false,true}, vector<bool>{false,true}, std::less<bool>{});
        context = "composite keys";
        staticCase(vector<pair<int,int>>{{1,2},{1,1},{1,2},{0,9}}, vector<pair<int,int>>{{0,0},{1,1},{1,2},{1,3},{2,0}}, std::less<pair<int,int>>{});
        context = "128-bit keys";
        lll big = lll(LLONG_MAX) + 1, small = lll(LLONG_MIN) - 1;
        staticCase(vector<lll>{big,small,0,0}, vector<lll>{small-1,small,0,big-1,big,big+1}, std::less<lll>{});
        int rounds = mode == "quick" ? 8 : mode == "full" ? 60 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            vector<lng> a(rng() % 80), probes(20);
            for (lng &x : a) { x = lng(rng() % 101) - 50; }
            for (lng &x : probes) { x = lng(rng() % 121) - 60; }
            context = "static random trial=" + std::to_string(trial) + " input=" + repr(a);
            staticCase(a, probes, Order{bool(rng() % 2), lng(1 + rng() % 7)});}
        for (int trial = 0; trial < rounds; ++trial) {
            vector<lng> a(rng() % 101), probes{LLONG_MIN, LLONG_MAX};
            for (lng &x : a) { x = std::bit_cast<lng>(rng()); }
            for (int j = 0; j < 10; ++j) { probes.push_back(j % 2 && !a.empty() ? a[rng() % a.size()] : std::bit_cast<lng>(rng())); }
            context = "static full-width trial=" + std::to_string(trial) + " input=" + repr(a);
            staticCase(a, probes, std::less<lng>{});}
        std::cout << "PASS compression/sorted-vector exhaustive, comparator, rebuild and domain tests\n";}

    using Multi = OrderedMultiSet<lng, Order>;
    using Key = Multi::Key;
    void verifyMulti(const Multi &s, const vector<Key> &live, Order cmp) {
        vector<Key> want = live;
        std::sort(want.begin(), want.end(), [&](Key a, Key b) {
            if (cmp(a.first,b.first)) { return true; }
            if (cmp(b.first,a.first)) { return false; }
            return a.second < b.second;});
        check(s.size() == int(want.size()) && s.empty() == want.empty(), "multiset size/empty");
        auto it = s.begin();
        for (int k = 0; k < int(want.size()); ++k, ++it) {
            check(it != s.end() && *it == want[k], "multiset iteration k=" + std::to_string(k));
            auto selected = s.findByOrder(k);
            check(selected != s.end() && *selected == want[k], "multiset select k=" + std::to_string(k));}
        check(it == s.end(), "multiset iteration end");
        for (int k : {-1, s.size(), INT_MAX}) { check(s.findByOrder(k) == s.end(), "multiset absent select"); }
        for (lng x : {-101LL,-21LL,-1LL,0LL,1LL,2LL,5LL,11LL,21LL,101LL,LLONG_MIN,LLONG_MAX}) {
            int less = 0, eq = 0;
            for (const Key &p : live) { less += cmp(p.first,x); eq += !cmp(p.first,x) && !cmp(x,p.first); }
            check(s.rank(x) == less && s.upperRank(x) == less + eq && s.count(x) == eq, "multiset ranks x=" + std::to_string(x));
            auto lo = s.lowerBound(x), hi = s.upperBound(x);
            check((lo == s.end()) == (less == s.size()), "multiset lower-end");
            check((hi == s.end()) == (less + eq == s.size()), "multiset upper-end");
            if (lo != s.end()) { check(*lo == want[less], "multiset lower value"); }
            if (hi != s.end()) { check(*hi == want[less+eq], "multiset upper value"); }
            auto floor = s.prev(x);
            check(less + eq ? floor != s.end() && *floor == want[less+eq-1] : floor == s.end(), "multiset prev x=" + std::to_string(x));}}
    bool eraseOne(vector<Key> &live, lng x, Order cmp) {
        auto it = std::find_if(live.begin(), live.end(), [&](const Key &p) {
            return !cmp(x,p.first) && !cmp(p.first,x);});
        if (it == live.end()) { return false; }
        live.erase(it); return true;}
    bool eraseKey(vector<Key> &live, const Key &key) {
        auto it = std::find(live.begin(), live.end(), key);
        if (it == live.end()) { return false; }
        live.erase(it); return true;}

    void histories(Multi s, vector<Key> live, vector<Key> past, int left, string history) {
        Order cmp{false,1}; context = "history=" + history;
        verifyMulti(s,live,cmp);
        if (!left) { return; }
        for (int action = 0; action < 5; ++action) {
            Multi next = s; vector<Key> a = live, tokens = past;
            if (action < 2) { Key key = next.insert(action); a.push_back(key); tokens.push_back(key); }
            else if (action < 4) { check(next.eraseOne(action-2) == eraseOne(a,action-2,cmp), "exhaustive eraseOne"); }
            else if (!tokens.empty()) { check(next.erase(tokens.front()) == eraseKey(a,tokens.front()), "exhaustive token erase"); }
            histories(std::move(next), std::move(a), std::move(tokens), left-1, history + char('0'+action));}}

    void dynamicTests(std::mt19937_64 &rng) {
        histories(Multi(Order{false,1}), {}, {}, mode == "quick" ? 3 : mode == "full" ? 5 : 6, "");
        int rounds = mode == "quick" ? 3 : mode == "full" ? 12 : 50;
        int steps = mode == "quick" ? 60 : mode == "full" ? 180 : 500;
        for (int trial = 0; trial < rounds; ++trial) {
            Order cmp{bool(trial % 2), trial % 3 == 0 ? 10 : 1};
            Multi s(cmp); vector<Key> live, past;
            for (int step = 0; step < steps; ++step) {
                lng x = lng(rng() % 61) - 30;
                int op = int(rng() % 7);
                context = "random trial=" + std::to_string(trial) + " step=" + std::to_string(step) + " op=" + std::to_string(op) + " key=" + std::to_string(x);
                if (op < 3) { Key key = s.insert(x); live.push_back(key); past.push_back(key); }
                else if (op == 3) { check(s.eraseOne(x) == eraseOne(live,x,cmp), "random eraseOne"); }
                else if (op == 4 && !past.empty()) {
                    Key key = past[rng() % past.size()]; check(s.erase(key) == eraseKey(live,key), "random token erase");}
                else if (op == 5) {
                    Multi copy = s, moved = std::move(copy); verifyMulti(moved,live,cmp);
                    moved.insert(500); verifyMulti(s,live,cmp);}
                else if (op == 6 && rng() % 5 == 0) {
                    s.clear(); live.clear();
                    for (const Key &key : past) { check(!s.erase(key), "stale tokens after clear"); }}
                verifyMulti(s,live,cmp);}}
        context = "lifetime tokens/rebuild/last representable ID";
        Multi s(vector<lng>{LLONG_MIN,0,LLONG_MAX}, Order{true,1});
        vector<Key> live;
        for (const auto &p : s.tree) { live.push_back(p); }
        verifyMulti(s,live,Order{true,1});
        Key old = s.insert(7); s.rebuild(vector<lng>{7,7,1});
        check(!s.erase(old) && s.count(7) == 2, "rebuild does not recycle IDs");
        Key now = s.insert(7); check(now.second > old.second, "monotone IDs");
        s.clear(); s.next_id = LLONG_MAX - 1;
        Key last = s.insert(7); check(last.second == LLONG_MAX-1 && s.rank(7) == 0 && s.upperRank(7) == 1, "last allowed ID query");
        check(s.erase(last) && !s.erase(last), "last ID erase/stale token");
        std::cout << "PASS multiset exhaustive/random histories, tokens, copies, comparators and limits\n";}

    struct Side {
        Multi s;
        vector<Key> live = {};
        lng next = 1;
    };
    // A token of the other set erases an element with the same ID whose key is equivalent under C.
    bool eraseToken(vector<Key> &live, const Key &key, Order cmp) {
        auto it = std::find_if(live.begin(), live.end(), [&](const Key &p) {
            return p.second == key.second && !cmp(key.first,p.first) && !cmp(p.first,key.first);});
        if (it == live.end()) { return false; }
        live.erase(it); return true;}
    void verifySide(const Side &x, const vector<Key> &tokens, Order cmp, const string &name) {
        string saved = context; context += " set=" + name;
        verifyMulti(x.s, x.live, cmp);
        check(x.s.next_id == x.next, "next_id oracle");
        Multi copy = x.s; vector<Key> live = x.live;
        for (const Key &key : tokens) { check(copy.erase(key) == eraseToken(live, key, cmp), "exact token erase"); }
        check(copy.empty() && live.empty(), "tokens cover contents");
        context = saved;}
    void sideInsert(Side &x, lng k, vector<Key> &tokens) {
        Key key = x.s.insert(k);
        check(key == Key(k, x.next), "insert returns next id");
        ++x.next; x.live.push_back(key); tokens.push_back(key);}
    void sideSplit(Side &a, lng x, Side &b, Order cmp) {
        a.s.split(x, b.s);
        vector<Key> keep, moved;
        for (const Key &key : a.live) { (cmp(x, key.first) ? moved : keep).push_back(key); }
        a.live = std::move(keep); b.live = std::move(moved); b.next = max(b.next, a.next);}
    bool below(const vector<Key> &a, const vector<Key> &b, Order cmp) {
        if (a.empty() || b.empty()) { return true; }
        lng hi = a[0].first, lo = b[0].first;
        for (const Key &key : a) { if (cmp(hi, key.first)) { hi = key.first; } }
        for (const Key &key : b) { if (cmp(key.first, lo)) { lo = key.first; } }
        return cmp(hi, lo);}
    bool sideJoin(Side &a, Side &b, Order cmp) {
        if (!below(a.live, b.live, cmp) && !below(b.live, a.live, cmp)) { return false; }
        a.s.join(b.s);
        a.live.insert(a.live.end(), b.live.begin(), b.live.end()); b.live.clear();
        a.next = max(a.next, b.next);
        check(b.s.next_id >= b.next, "join never lowers other's next_id");
        b.next = b.s.next_id; return true;}

    void pairHistories(const Side &a, const Side &b, const vector<Key> &tokens, Order cmp, int left, const string &history) {
        string prefix = "split/join history descending=" + std::to_string(cmp.descending) + " width=" + std::to_string(cmp.width) + " history=" + history;
        context = prefix; verifySide(a, tokens, cmp, "A"); verifySide(b, tokens, cmp, "B");
        if (!left) { return; }
        for (int action = 0; action < 22; ++action) {
            Side x = a, y = b; vector<Key> t = tokens; string step;
            context = prefix + " action=" + std::to_string(action);
            if (action < 3) { sideInsert(x, action, t); step = "iA" + std::to_string(action); }
            else if (action < 6) { sideInsert(y, action-3, t); step = "iB" + std::to_string(action-3); }
            else if (action < 9) { check(x.s.eraseOne(action-6) == eraseOne(x.live, action-6, cmp), "exhaustive eraseOne A"); step = "eA" + std::to_string(action-6); }
            else if (action == 9) { check(y.s.eraseOne(1) == eraseOne(y.live, 1, cmp), "exhaustive eraseOne B"); step = "eB1"; }
            else if (action < 12) {
                if (t.empty()) { continue; }
                Side &z = action == 10 ? x : y;
                Key key = action == 10 ? t.front() : t.back();
                check(z.s.erase(key) == eraseToken(z.live, key, cmp), "exhaustive token erase"); step = action == 10 ? "tA" : "tB";}
            else if (action < 20) {
                lng k = (action-12) % 4 - 1;
                if (action < 16) { sideSplit(x, k, y, cmp); } else { sideSplit(y, k, x, cmp); }
                step = (action < 16 ? "sA" : "sB") + std::to_string(k);}
            else if (!(action == 20 ? sideJoin(x, y, cmp) : sideJoin(y, x, cmp))) { continue; }
            else { step = action == 20 ? "jA" : "jB"; }
            pairHistories(x, y, t, cmp, left-1, history + step + ' ');}}

    void splitJoinTests(std::mt19937_64 &rng) {
        int deep = mode == "quick" ? 3 : 4, shallow = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
        for (Order cmp : {Order{false,1}, Order{true,1}, Order{false,2}}) {
            pairHistories(Side{Multi(cmp)}, Side{Multi(cmp)}, {}, cmp, cmp.descending || cmp.width > 1 ? shallow : deep, "");}
        int rounds = mode == "quick" ? 2 : mode == "full" ? 6 : 20, n = mode == "quick" ? 300 : 3000;
        for (int trial = 0; trial < rounds; ++trial) {
            Order cmp{bool(trial % 2), trial % 3 == 0 ? 10 : 1};
            Side a{Multi(cmp)}, b{Multi(cmp)}; vector<Key> tokens;
            context = "split/join random trial=" + std::to_string(trial) + " setup";
            for (int i = 0; i < n; ++i) { sideInsert(i % 8 ? a : b, lng(rng() % 4001) - 2000, tokens); }
            for (int i = 0; i < n / 4; ++i) {
                Key key = tokens[rng() % tokens.size()];
                Side &z = rng() % 2 ? a : b;
                check(z.s.erase(key) == eraseToken(z.live, key, cmp), "random token erase");}
            for (int step = 0; step < 16; ++step) {
                int op = int(rng() % 6);
                lng x = lng(rng() % 4401) - 2200;
                context = "split/join random trial=" + std::to_string(trial) + " step=" + std::to_string(step) + " op=" + std::to_string(op) + " key=" + std::to_string(x);
                if (op == 0) { sideSplit(a, x, b, cmp); }
                else if (op == 1) { sideSplit(b, x, a, cmp); }
                else if (op == 2) {
                    Side &from = rng() % 2 ? a : b, &to = &from == &a ? b : a;
                    vector<Key> before(from.s.begin(), from.s.end());
                    sideSplit(from, x, to, cmp);
                    verifySide(a, tokens, cmp, "A"); verifySide(b, tokens, cmp, "B");
                    bool back = rng() % 2;
                    check(back ? sideJoin(from, to, cmp) : sideJoin(to, from, cmp), "split halves are separated");
                    const Side &whole = back ? from : to;
                    check(vector<Key>(whole.s.begin(), whole.s.end()) == before, "split/join round trip");}
                else if (op == 3) {
                    sideSplit(a, x, b, cmp);
                    for (int i = 0; i < 60; ++i) {
                        lng k = lng(rng() % 4401) - 2200; sideInsert(cmp(x, k) ? b : a, k, tokens);}
                    verifySide(a, tokens, cmp, "A"); verifySide(b, tokens, cmp, "B");
                    check(rng() % 2 ? sideJoin(a, b, cmp) : sideJoin(b, a, cmp), "halves stay separated after inserts");}
                else if (op == 4) {
                    for (int i = 0; i < 40; ++i) { sideInsert(rng() % 2 ? a : b, lng(rng() % 4001) - 2000, tokens); }}
                else if (!sideJoin(a, b, cmp)) { sideJoin(b, a, cmp); }
                verifySide(a, tokens, cmp, "A"); verifySide(b, tokens, cmp, "B");}}
        context = "split/join sorted-insertion shape";
        int m = mode == "quick" ? 200 : 1500;
        Side a{Multi(Order{})}, b{Multi(Order{})}; vector<Key> tokens;
        for (int i = 0; i < m; ++i) { sideInsert(a, i / 2, tokens); }
        for (int step = 0; step < 8; ++step) {
            lng x = lng(rng() % (m / 2 + 2)) - 1;
            vector<Key> before(a.s.begin(), a.s.end());
            sideSplit(a, x, b, Order{});
            verifySide(a, tokens, Order{}, "A"); verifySide(b, tokens, Order{}, "B");
            if (step % 2) { check(sideJoin(b, a, Order{}), "sorted shape join below"); std::swap(a, b); }
            else { check(sideJoin(a, b, Order{}), "sorted shape join above"); }
            check(vector<Key>(a.s.begin(), a.s.end()) == before, "sorted shape round trip");}
        context = "split/join last representable ID";
        Multi s, o; s.next_id = LLONG_MAX - 2;
        Key p = s.insert(5), q = s.insert(5);
        s.split(5, o); check(s.size() == 2 && o.empty() && o.next_id == LLONG_MAX, "split keeps the last ID at x");
        s.split(4, o); check(s.empty() && o.size() == 2 && o.erase(q) && o.erase(p) && o.empty(), "moved last IDs stay valid");
        context = "split/join signed extrema";
        Multi e, f; vector<Key> keys{e.insert(LLONG_MAX), e.insert(0), e.insert(LLONG_MIN)};
        e.split(LLONG_MAX, f); check(e.size() == 3 && f.empty(), "split at the maximum moves nothing");
        e.split(LLONG_MIN, f); check(e.size() == 1 && f.size() == 2 && e.begin()->first == LLONG_MIN, "split at the minimum keeps equal keys");
        f.join(e); check(e.empty() && f.size() == 3 && f.begin()->first == LLONG_MIN && f.erase(keys[1]) && f.erase(keys[2]), "join a smaller side");
        context = "insert into both halves, then replace a non-empty other";
        Multi g, h;
        for (lng x : {1,1,2,2,3}) { g.insert(x); }
        g.split(1, h);
        g.insert(1); h.insert(2); h.insert(3); g.join(h);
        check(vector<Key>(g.begin(), g.end()) == vector<Key>{{1,1},{1,2},{1,6},{2,3},{2,4},{2,6},{3,5},{3,7}} && h.empty() && g.next_id == 8, "joined halves keep insertion order");
        check(g.insert(2) == Key(2,8) && g.findByOrder(6)->second == 8, "insert after join takes the max next_id");
        Multi big; big.next_id = 100; Key old = big.insert(7);
        g.split(1, big);
        check(!big.erase(old) && big.size() == 6 && big.next_id == 101 && big.insert(2) == Key(2,101), "split replaces other and keeps its larger next_id");
        std::cout << "PASS multiset split/join exhaustive pair histories, round trips, random, sorted shape and limits\n";}

    using Offline = OrderedMultiSetOffline<lng, Order>;
    // Oracle: the live values in insertion order, scanned linearly; a query value equivalent to a universe key answers with that key's representative.
    void verifyOffline(const Offline &s, const vector<lng> &live, const vector<lng> &universe, Order cmp) {
        auto same = [&](lng a, lng b) { return !cmp(a, b) && !cmp(b, a); };
        vector<lng> want = live;
        std::stable_sort(want.begin(), want.end(), cmp);
        check(s.size() == int(want.size()) && s.empty() == want.empty(), "offline size/empty");
        for (int k = 0; k < int(want.size()); ++k) {
            lng got = s.findByOrder(k);
            check(same(got, want[k]) && std::find(universe.begin(), universe.end(), got) != universe.end(), "offline select k=" + std::to_string(k));}
        vector<lng> probes = universe;
        for (lng x : {-101LL,-21LL,-1LL,0LL,1LL,2LL,5LL,11LL,21LL,101LL,LLONG_MIN,LLONG_MAX}) { probes.push_back(x); }
        for (lng x : probes) {
            int less = 0, eq = 0;
            for (lng y : live) { less += cmp(y, x); eq += same(y, x); }
            check(s.rank(x) == less && s.upperRank(x) == less + eq && s.count(x) == eq, "offline ranks x=" + std::to_string(x));
            lng res = LLONG_MIN + 7;
            bool ok = s.lowerBound(x, res);
            check(ok == (less < s.size()) && (ok ? same(res, want[less]) : res == LLONG_MIN + 7), "offline lowerBound x=" + std::to_string(x));
            res = LLONG_MIN + 7; ok = s.upperBound(x, res);
            check(ok == (less + eq < s.size()) && (ok ? same(res, want[less+eq]) : res == LLONG_MIN + 7), "offline upperBound x=" + std::to_string(x));
            res = LLONG_MIN + 7; ok = s.prev(x, res);
            check(ok == (less + eq > 0) && (ok ? same(res, want[less+eq-1]) : res == LLONG_MIN + 7), "offline prev x=" + std::to_string(x));
            lng alias = x;
            check(s.lowerBound(alias, alias) == (less < s.size()) && (less == s.size() ? alias == x : same(alias, want[less])), "offline aliased query and result");}}
    void offlineHistories(const Offline &s, const vector<lng> &live, const vector<lng> &universe, Order cmp, int left, const string &history) {
        context = "offline universe=" + repr(universe) + " history=" + history;
        verifyOffline(s, live, universe, cmp);
        if (!left) { return; }
        vector<lng> keys = universe;
        keys.push_back(-1); keys.push_back(4);
        for (int action = 0; action <= 2 * int(keys.size()); ++action) {
            Offline next = s; vector<lng> a = live;
            if (action == 2 * int(keys.size())) { next.clear(); a.clear(); }
            else if (action % 2 == 0) {
                lng x = keys[action / 2];
                if (std::none_of(universe.begin(), universe.end(), [&](lng y) { return !cmp(x, y) && !cmp(y, x); })) { continue; }
                next.insert(x); a.push_back(x);}
            else {
                lng x = keys[action / 2];
                auto it = std::find_if(a.begin(), a.end(), [&](lng y) { return !cmp(x, y) && !cmp(y, x); });
                bool want = it != a.end();
                if (want) { a.erase(it); }
                check(next.eraseOne(x) == want, "offline eraseOne");}
            offlineHistories(next, a, universe, cmp, left - 1, history + char('a' + action));}}
    void offlineTests(std::mt19937_64 &rng) {
        int depth = mode == "quick" ? 2 : mode == "full" ? 3 : 4;
        for (int u = 0; u <= 3; ++u) {
            for (lng code = 0; code < lng(std::pow(4, u)); ++code) {
                vector<lng> universe;
                for (lng c = code, j = 0; j < u; ++j, c /= 4) { universe.push_back(c % 4); }
                for (Order cmp : {Order{false,1}, Order{true,1}, Order{false,2}}) {
                    if (u == 3 && cmp.width == 2 && code % 5) { continue; }
                    offlineHistories(Offline(universe, cmp), {}, universe, cmp, depth, "");}}}
        int rounds = mode == "quick" ? 4 : mode == "full" ? 20 : 80, steps = mode == "quick" ? 80 : mode == "full" ? 300 : 800;
        for (int trial = 0; trial < rounds; ++trial) {
            Order cmp{bool(rng() % 2), lng(1 + rng() % 4)};
            vector<lng> universe(rng() % 30);
            for (lng &x : universe) { x = lng(rng() % 41) - 20; }
            Offline s(universe, cmp); vector<lng> live;
            for (int step = 0; step < steps; ++step) {
                context = "offline random trial=" + std::to_string(trial) + " step=" + std::to_string(step) + " universe=" + repr(universe);
                lng x = lng(rng() % 45) - 22;
                bool known = std::any_of(universe.begin(), universe.end(), [&](lng y) { return !cmp(x, y) && !cmp(y, x); });
                if (rng() % 3 && known) { s.insert(x); live.push_back(x); }
                else if (rng() % 50 == 0) { s.clear(); live.clear(); }
                else {
                    auto it = std::find_if(live.begin(), live.end(), [&](lng y) { return !cmp(x, y) && !cmp(y, x); });
                    bool want = it != live.end();
                    if (want) { live.erase(it); }
                    check(s.eraseOne(x) == want, "offline random eraseOne");}
                if (step % 10 == 0) { verifyOffline(s, live, universe, cmp); }}
            verifyOffline(s, live, universe, cmp);
            Offline copy = s;
            if (!universe.empty()) { copy.insert(universe[0]); check(copy.size() == s.size() + 1, "offline copy independence"); }
            else { check(s.empty() && !copy.eraseOne(0) && copy.rank(0) == 0, "offline empty universe"); }
            Offline moved = std::move(s); verifyOffline(moved, live, universe, cmp);}
        context = "offline strings";
        OrderedMultiSetOffline<string> w(vector<string>{"dog", "", "cat", "cat"});
        w.insert("cat"); w.insert(""); w.insert("cat"); w.insert("dog");
        string r;
        check(w.size() == 4 && w.count("cat") == 2 && w.rank("cat") == 1 && w.upperRank("cat") == 3 && w.findByOrder(3) == "dog", "offline string ranks/select");
        check(w.lowerBound("b", r) && r == "cat" && w.prev("b", r) && r == "" && !w.upperBound("dog", r) && !w.eraseOne("bird") && w.eraseOne("cat") && w.count("cat") == 1, "offline string bounds/erase");
        context = "offline bool keys";
        OrderedMultiSetOffline<bool> flags(vector<bool>{true, false, true});
        flags.insert(true); flags.insert(false); flags.insert(true);
        bool b = false;
        check(flags.size() == 3 && flags.count(true) == 2 && flags.rank(true) == 1 && flags.findByOrder(0) == false && flags.findByOrder(2) == true, "offline bool ranks/select");
        check(flags.upperBound(false, b) && b && flags.eraseOne(false) && !flags.eraseOne(false) && !flags.prev(false, b) && b, "offline bool bounds/erase");
        context = "offline extrema keys";
        Offline e(vector<lng>{LLONG_MIN, LLONG_MAX, 0});
        e.insert(LLONG_MAX); e.insert(LLONG_MIN);
        check(e.rank(LLONG_MAX) == 1 && e.upperRank(LLONG_MAX) == 2 && e.findByOrder(0) == LLONG_MIN && e.count(0) == 0, "offline signed extrema");
        std::cout << "PASS offline multiset exhaustive universes and histories, random comparators, strings, copy/move and extrema\n";}

    struct Reached : std::runtime_error {
        Reached() : std::runtime_error("comparator reached past the size assert") {}
    };
    struct Throwing {
        bool operator()(bool, bool) const { throw Reached(); }
    };
    template<typename F> void expectReached(F f, const string &operation) {
        bool reached = false;
        try { f(); } catch (const Reached &) { reached = true; }
        check(reached, operation);}
    // INT_MAX packed bools (256 MiB) are the largest admitted input: the size asserts pass, and the comparator stops the work at once.
    void sizeLimits() {
        if (mode == "quick") { return; }
        context = "largest admitted sizes";
        expectReached([] { SortedVector<bool, Throwing> v(vector<bool>(INT_MAX)); }, "SortedVector of INT_MAX elements passes its assert");
        expectReached([] { OrderedMultiSet<bool, Throwing> m; m.rebuild(vector<bool>(INT_MAX)); }, "rebuild of INT_MAX elements passes its assert");
        std::cout << "PASS largest admitted SortedVector and rebuild sizes\n";}
    struct Box {
        lng value;
        Box() = delete;
        explicit Box(lng x) : value(x) {}
        friend ostream &operator<<(ostream &os, const Box &x) { return os << x.value; }
    };
    struct BoxLess {
        bool operator()(const Box &a, const Box &b) const { return a.value < b.value; }
    };
    void pbdsAndGeneric() {
        context = "unique PBDS adapter";
        OrderedSet<lng, Order> s(Order{true,10}); std::set<lng, Order> ref(Order{true,10});
        for (lng x : {11,1,5,21,20,-1,11,3}) {
            check(s.insert(x).second == ref.insert(x).second, "unique insertion status");}
        int k = 0;
        for (lng x : ref) {
            check(*s.find_by_order(k) == x && s.order_of_key(x) == unsigned(k), "unique rank/select"); ++k;}
        check(s.find_by_order(s.size()) == s.end(), "unique absent select");
        for (lng x : {3,15,44,-1,3}) { check(s.erase(x) == bool(ref.erase(x)), "unique erase equivalence"); }
        context = "non-default keys without equality";
        CoordinateCompression<Box, BoxLess> c(vector<Box>{Box(9),Box(-2),Box(9)});
        check(c.index(Box(-2)) == 0 && c.findByOrder(1)->value == 9 && c.count(Box(9)) == 1, "generic compression");
        OrderedMultiSet<Box, BoxLess> m;
        auto key = m.insert(Box(9)); m.insert(Box(9)); m.insert(Box(-2));
        check(m.count(Box(9)) == 2 && m.findByOrder(0)->first.value == -2 && m.erase(key), "generic multiset");
        context = "string multiset";
        OrderedMultiSet<string> words; auto a = words.insert(""); auto b = words.insert("cat"); words.insert("cat");
        check(words.rank("cat") == 1 && words.count("cat") == 2 && words.erase(b), "string count/rank/token");
        check(words.eraseOne("cat") && !words.eraseOne("cat") && words.erase(a), "string eraseOne");
        OrderedMultiSet<string> tail; words.insert("b"); words.insert("dog"); words.split("b", tail);
        check(words.size() == 1 && tail.size() == 1 && tail.begin()->first == "dog", "string split");
        tail.join(words); check(words.empty() && tail.rank("dog") == 1, "string join below");
        OrderedMultiSet<Box, BoxLess> high; m.split(Box(0), high);
        check(m.size() == 1 && high.size() == 1 && high.begin()->first.value == 9, "generic split");
        high.join(m); check(m.empty() && high.findByOrder(0)->first.value == -2, "generic join below");
        context = "id limit and default bounds";
        OrderedMultiSet<int> ids; ids.next_id = LLONG_MAX - 2; ids.rebuild(vector<int>{5, 6});
        check(ids.size() == 2 && ids.next_id == LLONG_MAX && ids.upperBound(5) != ids.end() && ids.upperBound(5)->first == 6, "rebuild ending at the id limit");
        ids.next_id = lng(1) << 62; ids.rebuild(vector<int>{1, 2, 3});
        check(ids.size() == 3 && ids.count(2) == 1, "rebuild with a large next_id");
        CoordinateCompression<int> pc(vector<int>{1, 3, 5});
        check(pc.pointRange(1, 5) == pair<int, int>(0, 2) && pc.pointRange(3, 3) == pair<int, int>(1, 1), "pointRange default [l, r)");
        context = "PBDS order-statistics map";
        OrderedMap<lng, string, Order> om(Order{true,10}); std::map<lng, string, Order> rm(Order{true,10});
        for (lng x : {11,1,5,21,20,-1,11,3,40}) { string t = std::to_string(x); om[x] += t; rm[x] += t; }
        check(om.size() == rm.size(), "map size");
        k = 0;
        for (const auto &[key, value] : rm) {
            auto it = om.find_by_order(k);
            check(it != om.end() && !Order{true,10}(it->first, key) && !Order{true,10}(key, it->first) && it->second == value, "map select");
            check(om.order_of_key(key) == unsigned(k), "map rank"); ++k;}
        check(om.find_by_order(om.size()) == om.end() && om.erase(5) == bool(rm.erase(5)) && om.size() == rm.size(), "map erase/absent");
        std::cout << "PASS unique PBDS set/map adapters and non-default/string generic keys\n";}

    void invalid(const string &name) {
        OrderedMultiSet<int> s;
        if (name == "id-exhaustion") { s.next_id = LLONG_MAX; s.insert(1); }
        else if (name == "decode-end") { CoordinateCompression<int> c(vector<int>{4, 4, 2}); (void)c.decode(2); }
        else if (name == "decode-past-end") { CoordinateCompression<int> c(vector<int>{4, 4, 2}); (void)c.decode(3); }
        else if (name == "join-self-empty") { s.join(s); }
        else if (name == "decode-negative") { CoordinateCompression<int> c(vector<int>{4}); (void)c.decode(-1); }
        else if (name == "range-order") { CoordinateCompression<int> c(vector<int>{1, 2}); (void)c.pointRange(2, 1); }
        else if (name == "rebuild-id-exhaustion") { s.next_id = LLONG_MAX-1; s.rebuild(vector<int>{1,2}); }
        else if (name == "split-self") { s.insert(1); s.split(0, s); }
        else if (name == "join-self") { s.insert(1); s.join(s); }
        else if (name == "join-overlap") { OrderedMultiSet<int> o; s.insert(1); s.insert(3); o.insert(2); s.join(o); }
        else if (name == "join-equal-above") { OrderedMultiSet<int> o; s.insert(1); s.insert(2); o.insert(2); o.insert(3); s.join(o); }
        else if (name == "join-equal-below") { OrderedMultiSet<int> o; s.insert(2); s.insert(3); o.insert(1); o.insert(2); s.join(o); }
        else if (name == "offline-unknown-insert") { Offline o(vector<lng>{1, 3}); o.insert(2); }
        else if (name == "offline-kth-end") { Offline o(vector<lng>{1, 3}); o.insert(3); (void)o.findByOrder(1); }
        else if (name == "offline-size-limit") { Offline o(vector<lng>{1}); o.n = INT_MAX; o.insert(1); }
        else if (name == "offline-kth-negative") { Offline o(vector<lng>{1}); o.insert(1); (void)o.findByOrder(-1); }
        // INT_MAX + 1 bools (256 MiB) pass the size asserts' mutants; the comparator throws at once instead of sorting or inserting them.
        else if (name == "sorted-max-size") { SortedVector<bool, Throwing> v(vector<bool>(size_t(INT_MAX) + 1)); }
        else if (name == "multiset-rebuild-max-size") { OrderedMultiSet<bool, Throwing> m; m.rebuild(vector<bool>(size_t(INT_MAX) + 1)); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
} // namespace

int main(int argc, char **argv) {
    try {
        string probe;
        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];
            if (arg == "--mode" && i+1 < argc) { mode = argv[++i]; }
            else if (arg == "--seed" && i+1 < argc) { seed = std::stoull(argv[++i]); }
            else if (arg == "--invalid" && i+1 < argc) { probe = argv[++i]; }
            else { throw std::runtime_error("unknown/incomplete argument " + arg); }}
        if (mode != "quick" && mode != "full" && mode != "stress") { throw std::runtime_error("unknown mode " + mode); }
        std::cout << "ordered_set seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        std::mt19937_64 rng(seed);
        staticTests(rng); dynamicTests(rng); splitJoinTests(rng); offlineTests(rng); pbdsAndGeneric(); sizeLimits();
        std::cout << "PASS ordered_set checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL ordered_set seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1;}}
