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
        vector<int> encoded = c.encode(probes);
        for (int j = 0; j < int(probes.size()); ++j) {
            const T &x = probes[j];
            int less = 0, eq = 0, unique_less = 0, unique_eq = 0;
            for (const T &y : a) { less += cmp(y, x); eq += same(y, x); }
            for (const T &y : distinct) { unique_less += cmp(y, x); unique_eq += same(y, x); }
            check(v.rank(x) == less && v.upperRank(x) == less + eq && v.count(x) == eq, "sorted rank/count");
            check(v.index(x) == (eq ? less : -1), "sorted exact index");
            check(c.rank(x) == unique_less && c.upperRank(x) == unique_less + unique_eq, "compressed rank");
            check(c.count(x) == unique_eq && c.index(x) == (unique_eq ? unique_less : -1), "compressed count/index");
            check(encoded[j] == c.index(x), "batch encoding"); }
        int i = 0;
        for (const T &x : all) { check(same(*v.findByOrder(i++), x), "sorted select"); }
        i = 0;
        for (const T &x : distinct) { check(same(*c.findByOrder(i++), x), "compressed select"); }
        for (int k : {-1, v.size(), INT_MAX}) { check(v.findByOrder(k) == v.end(), "sorted missing select"); }
        for (int k : {-1, c.size(), INT_MAX}) { check(c.findByOrder(k) == c.end(), "compressed missing select"); }
        auto copy = v; auto moved = std::move(copy);
        moved.rebuild({}); check(moved.empty() && v.size() == int(a.size()), "sorted copy/move/rebuild");
        auto cc = c; auto cm = std::move(cc);
        cm.rebuild(probes); check(c.size() == int(distinct.size()), "compression copy independence");
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
                staticCase(a, vector<lng>{-2,-1,0,1,2}, std::less<lng>{}); }}
        for (Order cmp : {Order{false,1}, Order{true,1}, Order{false,10}, Order{true,10}}) {
            vector<lng> a{10,11,0,3,-1,-11,20,11,0}, probes{-25,-10,-1,0,5,10,15,20,25};
            context = "stateful static comparator descending=" + std::to_string(cmp.descending) + " width=" + std::to_string(cmp.width);
            staticCase(a, probes, cmp); }
        context = "signed extrema";
        staticCase(vector<lng>{LLONG_MIN,LLONG_MAX,0,LLONG_MIN}, vector<lng>{LLONG_MIN,LLONG_MAX,-1,0,1}, std::less<lng>{});
        context = "string keys";
        staticCase(vector<string>{"z","","aa","aa","a"}, vector<string>{"","a","aa","b","z","zz"}, std::less<string>{});
        context = "packed bool keys";
        staticCase(vector<bool>{true,false,true,false,true}, vector<bool>{false,true}, std::less<bool>{});
        int rounds = mode == "quick" ? 8 : mode == "full" ? 60 : 250;
        for (int trial = 0; trial < rounds; ++trial) {
            vector<lng> a(rng() % 80), probes(20);
            for (lng &x : a) { x = lng(rng() % 101) - 50; }
            for (lng &x : probes) { x = lng(rng() % 121) - 60; }
            context = "static random trial=" + std::to_string(trial) + " input=" + repr(a);
            staticCase(a, probes, Order{bool(rng() % 2), lng(1 + rng() % 7)}); }
        std::cout << "PASS compression/sorted-vector exhaustive, comparator, rebuild and domain tests\n";}

    using Multi = OrderedMultiSet<lng, Order>;
    using Key = Multi::Key;
    void verifyMulti(const Multi &s, const vector<Key> &live, Order cmp) {
        vector<Key> want = live;
        std::sort(want.begin(), want.end(), [&](Key a, Key b) {
            if (cmp(a.first,b.first)) { return true; }
            if (cmp(b.first,a.first)) { return false; }
            return a.second < b.second; });
        check(s.size() == int(want.size()) && s.empty() == want.empty(), "multiset size/empty");
        auto it = s.begin();
        for (int k = 0; k < int(want.size()); ++k, ++it) {
            check(it != s.end() && *it == want[k], "multiset iteration k=" + std::to_string(k));
            auto selected = s.findByOrder(k);
            check(selected != s.end() && *selected == want[k], "multiset select k=" + std::to_string(k)); }
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
            if (hi != s.end()) { check(*hi == want[less+eq], "multiset upper value"); }} }
    bool eraseOne(vector<Key> &live, lng x, Order cmp) {
        auto it = std::find_if(live.begin(), live.end(), [&](const Key &p) {
            return !cmp(x,p.first) && !cmp(p.first,x); });
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
            histories(std::move(next), std::move(a), std::move(tokens), left-1, history + char('0'+action)); }}

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
                    Key key = past[rng() % past.size()]; check(s.erase(key) == eraseKey(live,key), "random token erase"); }
                else if (op == 5) {
                    Multi copy = s, moved = std::move(copy); verifyMulti(moved,live,cmp);
                    moved.insert(500); verifyMulti(s,live,cmp); }
                else if (op == 6 && rng() % 5 == 0) {
                    s.clear(); live.clear();
                    for (const Key &key : past) { check(!s.erase(key), "stale tokens after clear"); }}
                verifyMulti(s,live,cmp); }}
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
            check(s.insert(x).second == ref.insert(x).second, "unique insertion status"); }
        int k = 0;
        for (lng x : ref) {
            check(*s.find_by_order(k) == x && s.order_of_key(x) == unsigned(k), "unique rank/select"); ++k; }
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
        std::cout << "PASS unique PBDS adapter and non-default/string generic keys\n";}

    void invalid(const string &name) {
        OrderedMultiSet<int> s;
        if (name == "id-exhaustion") { s.next_id = LLONG_MAX; s.insert(1); }
        else if (name == "rebuild-id-exhaustion") { s.next_id = LLONG_MAX-1; s.rebuild(vector<int>{1,2}); }
        else { throw std::runtime_error("unknown invalid probe " + name); }
        throw std::runtime_error("invalid precondition survived " + name);}
}

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
        staticTests(rng); dynamicTests(rng); pbdsAndGeneric();
        std::cout << "PASS ordered_set checks=" << checks << '\n'; return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL ordered_set seed=" << seed << " mode=" << mode << ' ' << e.what() << '\n'; return 1; }
}
