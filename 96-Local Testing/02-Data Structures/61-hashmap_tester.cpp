#include "../../02-Data Structures/61-hashmap.hpp"

namespace {
    ulng seed = 20261009;
    string mode = "full", context;
    lng checks = 0;
    using Rng = std::mt19937_64;
    lng rnd(Rng &rng, lng lo, lng hi) { return lo + lng(rng() % ulng(hi - lo + 1)); }
    template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
    template<typename T, typename U> string show(const pair<T, U> &p) { return "(" + show(p.first) + "," + show(p.second) + ")"; }
    template<typename T> string show(const vector<T> &a) {
        string s = "[";
        for (int i = 0; i < int(a.size()); ++i) { s += (i ? "," : "") + show(a[i]); }
        return s + "]";}
    template<typename T, typename U> void checkEqual(const T &got, const U &want, const string &op) {
        ++checks;
        if (!(got == want)) {
            throw std::runtime_error(context + " operation=" + op + " expected=" + show(want) + " actual=" + show(got));}}
    int steps() { return mode == "quick" ? 3000 : mode == "full" ? 40000 : 300000; }

    // Pathological hashers that force long clusters and wraparound.
    struct ConstHash { size_t operator()(lng) const { return 7; } };
    struct Mod3Hash { size_t operator()(lng x) const { return size_t(((x % 3) + 3) % 3) * 0x9e3779b97f4a7c15ULL; } };
    struct IdHash { size_t operator()(lng x) const { return size_t(x); } };

    // Every live slot is reachable from its home without crossing an empty slot; load stays <= 1/2.
    template<typename Tab, typename Key>
    void structure(const Tab &m, const string &at) {
        int cap = int(m.s.size()), live = 0;
        checkEqual(cap == 0 || std::has_single_bit(uint(cap)), true, at + " power-of-two capacity");
        for (int i = 0; i < cap; ++i) {
            if (m.s[i].t != m.gen) { continue; }
            ++live;
            for (int j = m.home(Tab::key(m.s[i].x)); j != i; j = (j + 1) & (cap - 1)) {
                if (m.s[j].t != m.gen) { checkEqual(j, i, at + " probe gap before slot"); }}}
        checkEqual(live, m.size(), at + " live slots");
        checkEqual(2 * m.size() <= cap, true, at + " load <= 1/2");}

    template<typename H>
    void mapRandom(Rng &rng, const string &name, H hash, lng range, int ops) {
        context = "HashMap<" + name + "> seed=" + show(seed) + " range=" + show(range);
        HashMap<lng, lng, H> m(0, hash);
        map<lng, lng> o;
        for (int step = 0; step < ops; ++step) {
            string at = "step " + show(step);
            lng k = rnd(rng, -range, range), v = rnd(rng, -1000000000000LL, 1000000000000LL);
            int op = int(rnd(rng, 0, 99));
            if (op < 25) { m[k] += v; o[k] += v; }
            else if (op < 40) { checkEqual(m.insert(k, v), o.emplace(k, v).second, at + " insert(" + show(k) + ")"); }
            else if (op < 65) { checkEqual(m.erase(k), o.erase(k) == 1, at + " erase(" + show(k) + ")"); }
            else if (op < 66) { m.clear(); o.clear(); }
            else if (op < 67) {
                int r = int(rnd(rng, 0, 3 * int(o.size()) + 5));
                int cap = int(m.s.size());
                m.reserve(r);
                checkEqual(int(m.s.size()) >= max(cap, 2 * r), true, at + " reserve(" + show(r) + ") capacity");}
            else {
                auto it = o.find(k);
                bool in = it != o.end();
                checkEqual(m.contains(k), in, at + " contains(" + show(k) + ")");
                auto f = m.find(k);
                checkEqual(f == m.end(), !in, at + " find(" + show(k) + ") == end");
                if (in) {
                    checkEqual(*f, *it, at + " *find(" + show(k) + ")");
                    checkEqual(m.at(k), it->second, at + " at(" + show(k) + ")");
                    m.at(k) ^= 1; it->second ^= 1;
                    f->second ^= 2; it->second ^= 2;}
                checkEqual(m.get(k, -7), in ? it->second : -7, at + " get(" + show(k) + ", -7)");
                checkEqual(m.get(k), in ? it->second : 0, at + " get(" + show(k) + ")");}
            checkEqual(m.size(), int(o.size()), at + " size");
            checkEqual(m.empty(), o.empty(), at + " empty");
            if (step % 97 == 0 || ops - step < 3) {
                structure<decltype(m), lng>(m, at);
                vector<pair<lng, lng>> got;
                for (auto &[x, y] : m) { got.push_back({x, y}); }
                sort(got.begin(), got.end());
                checkEqual(show(got), show(vector<pair<lng, lng>>(o.begin(), o.end())), at + " iteration");
                const auto &c = m;
                lng sum = 0;
                int cnt = 0;
                for (auto it = c.begin(); it != c.end(); ++it) { sum += it->second; ++cnt; }
                lng want = 0;
                for (auto &[x, y] : o) { want += y; }
                checkEqual(pair<int, lng>{cnt, sum}, pair<int, lng>{int(o.size()), want}, at + " const iteration");}}
        std::cout << "PASS HashMap<" << name << "> range=" << range << " ops=" << ops << '\n';}

    template<typename H>
    void setRandom(Rng &rng, const string &name, H hash, lng range, int ops) {
        context = "HashSet<" + name + "> seed=" + show(seed) + " range=" + show(range);
        HashSet<lng, H> m(int(rnd(rng, 0, 4)), hash);
        set<lng> o;
        static_assert(std::is_const_v<std::remove_reference_t<decltype(*m.begin())>>, "set iterators are const");
        for (int step = 0; step < ops; ++step) {
            string at = "step " + show(step);
            lng k = rnd(rng, -range, range);
            int op = int(rnd(rng, 0, 99));
            if (op < 45) { checkEqual(m.insert(k), o.insert(k).second, at + " insert(" + show(k) + ")"); }
            else if (op < 75) { checkEqual(m.erase(k), o.erase(k) == 1, at + " erase(" + show(k) + ")"); }
            else if (op < 76) { m.clear(); o.clear(); }
            else if (op < 77) { m.reserve(int(o.size()) + 10); }
            else {
                bool in = o.count(k);
                checkEqual(m.contains(k), in, at + " contains(" + show(k) + ")");
                auto f = m.find(k);
                checkEqual(f == m.end() ? lng(-1) : *f, in ? k : lng(-1), at + " find(" + show(k) + ")");}
            checkEqual(m.size(), int(o.size()), at + " size");
            checkEqual(m.empty(), o.empty(), at + " empty");
            if (step % 97 == 0 || ops - step < 3) {
                structure<decltype(m), lng>(m, at);
                vector<lng> got(m.begin(), m.end());
                sort(got.begin(), got.end());
                checkEqual(show(got), show(vector<lng>(o.begin(), o.end())), at + " iteration");}}
        std::cout << "PASS HashSet<" << name << "> range=" << range << " ops=" << ops << '\n';}

    // Every deletion position in every cluster shape on a tiny table, against std::set.
    void exhaustiveSmall() {
        context = "HashSet<IdHash> exhaustive seed=" + show(seed);
        int cases = 0;
        int lim = mode == "quick" ? 5 : 7;
        // Keys 0..15 with identity hash in an 16-slot table: home = key & 15 for key < 16, so collisions come from k and k + 16.
        for (int mask = 0; mask < (1 << (2 * lim)); ++mask) {
            vector<lng> keys;
            for (int b = 0; b < 2 * lim; ++b) {
                if (mask >> b & 1) { keys.push_back(b < lim ? lng(14 + b) % 16 : lng(14 + b - lim) % 16 + 16); }}
            if (int(keys.size()) > 8) { continue; }
            for (int del = 0; del < int(keys.size()); ++del) {
                HashSet<lng, IdHash> m(8);
                for (lng k : keys) { m.insert(k); }
                checkEqual(int(m.s.size()), 16, "capacity stays 16");
                m.erase(keys[del]);
                for (int i = 0; i < int(keys.size()); ++i) {
                    checkEqual(m.contains(keys[i]), i != del, "mask=" + show(mask) + " erase " + show(keys[del]) + " contains " + show(keys[i]));}
                structure<decltype(m), lng>(m, "mask=" + show(mask));
                ++cases;}}
        std::cout << "PASS exhaustive wraparound erase cases=" << cases << '\n';}

    void semantics(Rng &rng) {
        context = "semantics seed=" + show(seed);
        // Fresh values after erase and clear even though the slot keeps its old storage.
        HashMap<int, vector<int>> m;
        m[1].push_back(5);
        m[2] = {1, 2, 3};
        m.erase(1);
        checkEqual(show(m[1]), string("[]"), "operator[] after erase is V()");
        m.clear();
        checkEqual(show(m[2]), string("[]"), "operator[] after clear is V()");
        checkEqual(m.size(), 1, "size after clear and one access");
        // Version-stamp wraparound.
        HashMap<int, int> w;
        for (int i = 0; i < 50; ++i) { w[i] = i; }
        w.gen = std::numeric_limits<uint>::max() - 1;
        for (auto &x : w.s) { x.t = std::min(x.t, w.gen); }
        for (int r = 0; r < 4; ++r) {
            w.clear();
            checkEqual(w.size(), 0, "size after wrap clear " + show(r));
            for (int i = 0; i < 50; ++i) { checkEqual(w.contains(i), false, "contains after wrap clear " + show(r)); }
            for (int i = 0; i < 20; ++i) { w[i * 3] = i; }
            int cnt = 0;
            for (auto &x : w) { checkEqual(x.second * 3, x.first, "value after wrap clear"); ++cnt; }
            checkEqual(cnt, 20, "iteration after wrap clear " + show(r));}
        checkEqual(w.gen >= 1, true, "generation restarted");
        // Copies are independent snapshots.
        HashMap<lng, lng> a;
        for (int i = 0; i < 100; ++i) { a[i] = i; }
        auto b = a;
        b.erase(5);
        b[200] = 1;
        checkEqual(a.contains(5) && !a.contains(200) && a.size() == 100, true, "copy independence");
        checkEqual(!b.contains(5) && b.get(200) == 1 && b.size() == 100, true, "copy contents");
        // Empty table: no allocation, every query answers absent.
        HashMap<lng, lng> e;
        checkEqual(int(e.s.size()), 0, "default constructs without slots");
        checkEqual(e.contains(0) || e.find(0) != e.end() || e.erase(0) || e.get(0, 9) != 9 || e.begin() != e.end(), false, "empty table queries");
        // reserve guarantees no rehash, so references stay valid.
        HashMap<lng, lng> r(1000);
        int cap = int(r.s.size());
        lng &first = r[123456789];
        for (int i = 0; i < 999; ++i) { r[lng(rng())] = i; }
        checkEqual(int(r.s.size()), cap, "no rehash within reserve");
        first = 77;
        checkEqual(r.get(123456789), 77, "reference stable within reserve");
        // Fixed variants fill to exactly their reserved size.
        HashMapFixed<lng, lng> f(300);
        HashSetFixed<lng> fs(300);
        int fcap = int(f.s.size());
        for (int i = 0; i < 300; ++i) { f[lng(i) << 40] = i; fs.insert(-lng(i) * 1000003); }
        checkEqual(int(f.s.size()) == fcap && f.size() == 300 && fs.size() == 300, true, "fixed fill");
        for (int i = 0; i < 300; ++i) { checkEqual(f.at(lng(i) << 40) == i && fs.contains(-lng(i) * 1000003), true, "fixed lookups"); }
        for (int i = 300; 2 * f.size() < fcap; ++i) { f[lng(i) << 40] = i; }
        checkEqual(int(f.s.size()), fcap, "fixed fills to half its capacity without rehash");
        f.clear();
        checkEqual(f.empty() && int(f.s.size()) == fcap, true, "fixed clear keeps capacity");
        // Generic keys through CustomHash.
        HashMap<string, int> sm;
        map<string, int> so;
        HashSet<pair<int, int>> ps;
        set<pair<int, int>> po;
        for (int i = 0; i < 3000; ++i) {
            string s(size_t(rnd(rng, 0, 3)), 'a');
            for (auto &ch : s) { ch = char('a' + rnd(rng, 0, 2)); }
            sm[s] += i; so[s] += i;
            pair<int, int> p{int(rnd(rng, -5, 5)), int(rnd(rng, -5, 5))};
            if (i % 3) { checkEqual(ps.insert(p), po.insert(p).second, "pair insert"); }
            else { checkEqual(ps.erase(p), po.erase(p) == 1, "pair erase"); }}
        for (auto &[s, v] : so) { checkEqual(sm.at(s), v, "string key " + s); }
        checkEqual(sm.size() == int(so.size()) && ps.size() == int(po.size()), true, "generic key sizes");
        // Arguments aliasing stored elements across a rehash: p[p[x]] (DSU idiom), insert(k, p[k']), string keys and values.
        HashMap<int, int> al;
        map<int, int> ao;
        al[0] = 1; ao[0] = 1;
        for (int i = 1; i < 2000; ++i) {
            al[i] = i + 1; ao[i] = i + 1;
            int &r = al[al[i]];
            int &ro = ao[ao[i]];
            r += i; ro += i;
            checkEqual(al.insert(-i, al[i - 1]), ao.emplace(-i, ao[i - 1]).second, "insert aliasing value");}
        for (auto &[k, v] : ao) { checkEqual(al.get(k, -7), v, "aliasing contents key " + show(k)); }
        checkEqual(al.size(), int(ao.size()), "aliasing size");
        HashMap<string, string> sk;
        HashSet<string> ss(0, CustomHash(seed));
        set<string> so2;
        sk["seed"] = "v";
        ss.insert(string(30, 'a')); so2.insert(string(30, 'a'));
        for (int i = 0; i < 300; ++i) {
            string k = string(30, char('a' + i % 26)) + show(i);
            sk[k] = k;
            (void)sk[sk.at(k)];
            const string &b = *ss.begin();
            so2.insert(b + show(i));
            checkEqual(ss.insert(b + show(i)), true, "set insert from aliased element");}
        checkEqual(sk.size() == 301 && ss.size() == int(so2.size()), true, "string aliasing sizes");
        // Iterator converts to const_iterator.
        HashMap<int, int>::const_iterator ci = al.find(0);
        checkEqual(ci->first, 0, "iterator to const_iterator");
        // Extreme integer keys.
        HashMap<ulng, int> ex;
        vector<ulng> ks{0, 1, ~0ULL, 1ULL << 63, (1ULL << 63) - 1};
        for (int i = 0; i < int(ks.size()); ++i) { ex[ks[i]] = i; }
        for (int i = 0; i < int(ks.size()); ++i) { checkEqual(ex.at(ks[i]), i, "extreme key " + show(ks[i])); }
        std::cout << "PASS value reset, generation wraparound, copies, reserve, fixed variants, generic and extreme keys\n";}

    // Structured keys (multiples of 2^20, consecutive) under the seeded hash keep short probe runs.
    void adversarial() {
        context = "adversarial seed=" + show(seed);
        int n = mode == "quick" ? 1 << 14 : 1 << 18;
        for (int kind = 0; kind < 3; ++kind) {
            HashMap<lng, int> m(0, CustomHash(seed + ulng(kind)));
            auto keyOf = [&](int i) { return kind == 0 ? lng(i) << 20 : kind == 1 ? lng(i) : lng(i) * 1000000007LL; };
            for (int i = 0; i < n; ++i) { m[keyOf(i)] = i; }
            lng probes = 0;
            int cap = int(m.s.size());
            for (int i = 0; i < n; ++i) {
                int j = m.home(keyOf(i));
                while (m.s[j].x.first != keyOf(i)) { j = (j + 1) & (cap - 1); ++probes; }}
            checkEqual(probes < 2LL * n, true, "kind=" + show(kind) + " mean extra probes " + show(double(probes) / n) + " < 2");
            for (int i = 0; i < n; i += 2) { m.erase(keyOf(i)); }
            for (int i = 0; i < n; ++i) { checkEqual(m.get(keyOf(i), -1), i % 2 ? i : -1, "kind=" + show(kind) + " after half erase"); }}
        std::cout << "PASS structured keys n=" << n << '\n';}

    void invalid(const string &name) {
        HashMap<int, int> m;
        m[1] = 2;
        HashMapFixed<int, int> f(2);
        f[1] = 1; f[2] = 2;
        HashSetFixed<int> fs(1);
        fs.insert(1); fs.insert(2);
        if (name == "at-missing") { (void)m.at(5); }
        else if (name == "at-missing-const") { const auto &c = m; (void)c.at(5); }
        else if (name == "fixed-overflow") { f[3] = 3; }
        else if (name == "fixed-insert-overflow") { f.insert(3, 3); }
        else if (name == "fixed-set-overflow") { fs.insert(3); }
        else if (name == "fixed-zero") { HashMapFixed<int, int> z; z[0] = 0; }
        else if (name == "reserve-negative") { m.reserve(-1); }
        else if (name == "reserve-huge") { m.reserve(1 << 30); }
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
        std::cout << "hashmap seed=" << seed << " mode=" << mode << '\n';
        if (!probe.empty()) { invalid(probe); }
        Rng rng(seed);
        int s = steps();
        for (lng range : {3LL, 40LL, 2000LL, 1000000000000000000LL}) {
            mapRandom(rng, "CustomHash", CustomHash(rng()), range, s);
            setRandom(rng, "CustomHash", CustomHash(rng()), range, s);}
        for (lng range : {3LL, 60LL, 500LL}) {
            mapRandom(rng, "ConstHash", ConstHash(), range, s / 4);
            setRandom(rng, "ConstHash", ConstHash(), range, s / 4);
            mapRandom(rng, "Mod3Hash", Mod3Hash(), range, s / 2);
            mapRandom(rng, "IdHash", IdHash(), range, s / 2);
            setRandom(rng, "IdHash", IdHash(), range, s / 2);}
        exhaustiveSmall();
        semantics(rng);
        adversarial();
        std::cout << "PASS hashmap checks=" << checks << '\n'; return 0;} catch (const std::exception &e) {
        std::cerr << "FAIL hashmap seed=" << seed << " mode=" << mode << " " << e.what() << '\n'; return 1;}}
