#include "../../02-Data Structures/06-sqrt_decomposition.hpp"
#include "../../02-Data Structures/00-monoids.hpp"

ulng seed = 20260927;
string mode = "quick", context;
vector<lng> input;
vector<array<lng,4>> history;

string show(lll x) {
    if (!x) { return "0"; }
    bool neg = x < 0; ulll u = neg ? ulll(-(x+1))+1 : ulll(x); string s;
    while (u) { s.push_back(char('0'+u%10)); u /= 10; }
    if (neg) { s.push_back('-'); }
    reverse(s.begin(),s.end()); return s;}
template<typename T> string show(const T &x) { std::ostringstream s; s << x; return s.str(); }
[[noreturn]] void fail(const string &op, const string &expected, const string &actual) {
    cerr << "FAIL sqrt seed=" << seed << " mode=" << mode << " case=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual << " input=[";
    for (lng x : input) { cerr << x << ','; }
    cerr << "] history=";
    for (auto a : history) { cerr << '(' << a[0] << ',' << a[1] << ',' << a[2] << ',' << a[3] << ')'; }
    cerr << '\n'; std::exit(1);}
template<typename A, typename B> void check(const A &actual, const B &expected, const string &op) {
    if (actual != expected) { fail(op,show(expected),show(actual)); }}
vector<int> widths(int n) {
    vector<int> result{0,1,2,max(1,n),n+2,INT_MAX};
    sort(result.begin(),result.end()); result.erase(unique(result.begin(),result.end()),result.end());
    return result;}
template<typename M, typename T, typename F>
void verifyGeneric(const SqrtDecomp<M> &d, const vector<T> &a, T id, F op) {
    int n = int(a.size()); check(d.n,n,"generic size"); auto values = d.values();
    check(values.size(),a.size(),"generic values size");
    for (int i = 0; i < n; ++i) { check(d.get(i),a[i],"generic get "+show(i)); check(values[i],a[i],"generic values "+show(i)); }
    for (int l = 0; l <= n; ++l) {
        T expected = id;
        for (int r = l; r <= n; ++r) {
            check(d.prod(l,r),expected,"generic prod("+show(l)+","+show(r)+")");
            if (r < n) { expected = op(expected,a[r]); }}}}
template<typename T> void verifyLazy(const SqrtRangeSum<T> &d, const vector<T> &a) {
    int n = int(a.size()); check(d.n,n,"lazy size"); auto values = d.values();
    check(values.size(),a.size(),"lazy values size");
    for (int i = 0; i < n; ++i) { check(d.get(i),a[i],"lazy get "+show(i)); check(values[i],a[i],"lazy values "+show(i)); }
    for (int l = 0; l <= n; ++l) {
        T expected = T(0);
        for (int r = l; r <= n; ++r) {
            check(d.sum(l,r),expected,"lazy sum("+show(l)+","+show(r)+")");
            if (r < n) { expected = expected+a[r]; }}}}
void genericExhaustive() {
    int limit = mode == "quick" ? 3 : 5;
    for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask;
            for (lng &x : a) { x = code%3-1; code /= 3; }
            input = a; history.clear();
            for (int b : widths(n)) {
                context = "generic n="+show(n)+" mask="+show(mask)+" B="+show(b);
                SqrtDecomp<Sum<lng>> d(a,b), zero(n,b); verifyGeneric(d,a,lng(0),std::plus<lng>{});
                verifyGeneric(zero,vector<lng>(n),lng(0),std::plus<lng>{});
                for (int i = 0; i < n; ++i) { for (lng x : {-2,0,2}) {
                    auto e = d; auto v = a; v[i] = x; e.set(i,x); verifyGeneric(e,v,lng(0),std::plus<lng>{});
                    e = d; v = a; v[i] += x; e.apply(i,x); verifyGeneric(e,v,lng(0),std::plus<lng>{});}}
                auto copy = d; SqrtDecomp<Sum<lng>> moved(std::move(copy)); verifyGeneric(moved,a,lng(0),std::plus<lng>{});
                copy = d; verifyGeneric(copy,a,lng(0),std::plus<lng>{});
                copy.rebuild(vector<lng>{7,-2,3},2); verifyGeneric(copy,vector<lng>{7,-2,3},lng(0),std::plus<lng>{});
                copy.rebuild({}); verifyGeneric(copy,vector<lng>{},lng(0),std::plus<lng>{});}}}
    cout << "PASS sqrt generic exhaustive n<=" << limit << '\n';}
// Stateful monoid: the member flag picks the concatenation order.
struct Join {
    bool backwards;
    using S = string;
    S op(const S &a, const S &b) const { return backwards ? b+a : a+b; }
    S e() const { return ""; }
};
void noncommutative() {
    input.clear(); history.clear();
    for (int n = 0; n <= 11; ++n) { for (int b : widths(n)) { for (bool reverse_order : {false,true}) {
        context = "strings n="+show(n)+" B="+show(b)+" reverse="+show(reverse_order);
        vector<string> a(n);
        for (int i = 0; i < n; ++i) { a[i] = string(1,char('a'+i)); }
        auto op = [reverse_order](const string &x, const string &y) { return reverse_order ? y+x : x+y; };
        SqrtDecomp d(a,b,Join{reverse_order}); verifyGeneric(d,a,string{},op);
        for (int i = 0; i < n; ++i) {
            d.apply(i,"Z"); a[i] = op(a[i],"Z"); verifyGeneric(d,a,string{},op);
            d.set(i,"X"); a[i] = "X"; verifyGeneric(d,a,string{},op);}
        d.rebuild(vector<string>{"ab","c","de"},2); verifyGeneric(d,vector<string>{"ab","c","de"},string{},op);}}}
    cout << "PASS sqrt noncommutative/stateful fold/rebuild\n";}
void lazyExhaustive() {
    int limit = mode == "quick" ? 2 : 3, depth = mode == "quick" ? 1 : 2;
    const vector<pair<lng,lng>> actions{{-1,1},{0,2},{1,-1},{2,0}};
    for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask;
            for (lng &x : a) { x = code%3-1; code /= 3; }
            input = a;
            for (int b = 1; b <= max(n,1); ++b) {
                context = "lazy exhaustive n="+show(n)+" mask="+show(mask)+" B="+show(b); history.clear();
                auto visit = [&] (auto &&self, SqrtRangeSum<lng> d, vector<lng> v, int left) -> void {
                    verifyLazy(d,v);
                    if (!left) { return; }
                    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) { for (auto [mul,add] : actions) {
                        auto e = d; auto w = v; e.affine(l,r,mul,add);
                        for (int i = l; i < r; ++i) { w[i] = mul*w[i]+add; }
                        history.push_back({l,r,mul,add}); self(self,std::move(e),std::move(w),left-1); history.pop_back();}}}};
                visit(visit,SqrtRangeSum<lng>(a,b),a,depth);}}}
    cout << "PASS sqrt lazy exhaustive n<=" << limit << " depth=" << depth << '\n';}
void randomCases() {
    std::mt19937_64 rng(seed);
    int trials = mode == "quick" ? 20 : mode == "full" ? 100 : 400;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%65), b = int(rng()%(n+4)); vector<lng> a(n);
        for (lng &x : a) { x = lng(rng()%21)-10; }
        input = a; history.clear(); context = "random trial="+show(trial)+" n="+show(n)+" B="+show(b);
        SqrtRangeSum<lng> d(a,b); verifyLazy(d,a);
        for (int operation = 0; operation < 200; ++operation) {
            int l = int(rng()%(n+1)), r = int(rng()%(n+1)); if (l > r) { swap(l,r); }
            lng mul = lng(rng()%3)-1, add = lng(rng()%11)-5;
            switch (operation%5) {
                case 0: d.affine(l,r,mul,add); break;
                case 1: mul = 1; d.add(l,r,add); break;
                case 2: mul = 0; d.assign(l,r,add); break;
                case 3: add = 0; d.multiply(l,r,mul); break;
                default:
                    mul = 0;
                    if (n) { l = int(rng()%n); r = l+1; d.set(l,add); }
                    else { l = r = 0; d.assign(0,0,add); }}
            for (int i = l; i < r; ++i) { a[i] = mul*a[i]+add; }
            history.push_back({l,r,mul,add});
            int ql = int(rng()%(n+1)), qr = int(rng()%(n+1)); if (ql > qr) { swap(ql,qr); }
            check(d.sum(ql,qr),accumulate(a.begin()+ql,a.begin()+qr,lng(0)),"random sum("+show(ql)+","+show(qr)+")");
            if (operation%19 == 0) { verifyLazy(d,a); }}
        auto copy = d; verifyLazy(copy,a); auto moved = std::move(copy); verifyLazy(moved,a); verifyLazy(d,a);
        copy = d; verifyLazy(copy,a); copy = SqrtRangeSum<lng>(vector<lng>{3,5,1,7},3); verifyLazy(copy,vector<lng>{3,5,1,7});
        copy = SqrtRangeSum<lng>(vector<lng>{}); verifyLazy(copy,vector<lng>{});}
    cout << "PASS sqrt random vector oracle/copy/move/reconstruction trials=" << trials << '\n';}
void regressions() {
    input.clear(); history.clear(); context = "lazy order/alias/wide/modular regressions";
    for (int b : widths(5)) {
        SqrtRangeSum<lng> d(vector<lng>{1,2,3,4,5},b);
        d.assign(0,5,3); d.multiply(0,5,2); d.add(1,4,1); verifyLazy(d,vector<lng>{6,7,7,7,6});
        d.affine(0,5,-1,2); d.set(1,100); verifyLazy(d,vector<lng>{-4,100,-5,-5,-4});
        d = SqrtRangeSum<lng>(d.values(),b); verifyLazy(d,vector<lng>{-4,100,-5,-5,-4});
        d.assign(0,5,d.get(1)); verifyLazy(d,vector<lng>(5,100));}
    SqrtRangeSum<lng> zero(7,3); verifyLazy(zero,vector<lng>(7));
    zero.affine(0,7,2,3); zero.affine(0,7,zero.lazy_mul[0],zero.lazy_add[0]); verifyLazy(zero,vector<lng>(7,9));
    SqrtDecomp<Sum<lng>> generic(vector<lng>{1,2,3},2); generic.set(0,generic.get(1));
    generic.apply(2,generic.get(0)); verifyGeneric(generic,vector<lng>{2,2,5},lng(0),std::plus<lng>{});
    lll big = lll(1) << 95; SqrtRangeSum<lll> wide(vector<lll>{big,2*big,-big},2);
    wide.affine(0,3,lll(2),big); wide.set(1,-big); verifyLazy(wide,vector<lll>{3*big,-big,-big});
    SqrtRangeSum<uint> modular(vector<uint>{UINT_MAX,2,3},2);
    modular.affine(0,3,UINT_MAX,uint(1)); verifyLazy(modular,vector<uint>{2,UINT_MAX,UINT_MAX-1});
    modular.add(1,3,uint(5)); verifyLazy(modular,vector<uint>{2,4,3});
    SqrtRangeSum<double> dyadic(vector<double>{0.5,1.25,0.25},2);
    dyadic.affine(0,3,2.0,0.5); verifyLazy(dyadic,vector<double>{1.5,3.0,1.0});
    SqrtDecomp<Sum<lng>> empty; verifyGeneric(empty,vector<lng>{},lng(0),std::plus<lng>{});
    SqrtRangeSum<lng> empty_sum; verifyLazy(empty_sum,vector<lng>{});
    SqrtDecomp<Sum<lng>> single({5}); SqrtRangeSum<lng> single_sum({5}), triple{1, 2, 3};
    verifyGeneric(single,vector<lng>{5},lng(0),std::plus<lng>{}); verifyLazy(single_sum,vector<lng>{5}); verifyLazy(triple,vector<lng>{1,2,3});
    static_assert(!std::is_convertible_v<int, SqrtDecomp<Sum<lng>>> && !std::is_convertible_v<vector<lng>, SqrtDecomp<Sum<lng>>>);
    static_assert(!std::is_convertible_v<int, SqrtRangeSum<lng>> && !std::is_convertible_v<vector<lng>, SqrtRangeSum<lng>>);
    auto any = [](bool x, bool y) { return x || y; };
    for (int b : {0, 1, 2, 3}) {
        vector<bool> bits{false, true, false, false, true};
        SqrtDecomp flags(bits, b, Monoid{false, any});
        for (int i = 0; i < 5; ++i) { const bool &got = flags.get(i); check(got, bool(bits[i]), "bool get"); }
        for (int l = 0; l <= 5; ++l) {
            for (int r = l; r <= 5; ++r) {
                bool want = false;
                for (int i = l; i < r; ++i) { want = want || bits[i]; }
                check(flags.prod(l,r), want, "bool prod");}}
        flags.set(4,false); flags.apply(2,true); check(flags.values() == vector<bool>{false,true,true,false,false}, true, "bool values");}
    cout << "PASS sqrt action order/empty/type/alias/block boundary regressions\n";}
int rootFloor(lng n) {
    int k = 0;
    while (lng(k+1)*(k+1) <= n) { ++k; }
    return k;}
template<typename T> void checkVector(const vector<T> &actual, const vector<T> &expected, const string &op) {
    check(actual.size(),expected.size(),op+" size");
    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) { fail(op+" "+show(i),show(expected[i]),show(actual[i])); }}}
template<typename T, typename V> T existingOr(std::mt19937_64 &g, const vector<T> &a, V value) { return !a.empty() && g()%2 ? a[g()%a.size()] : value(g); }
// Lexicographic pairs under componentwise addition: an ordered group that is not arithmetic.
struct Lex {
    lng a = 0, b = 0;
    Lex() = default;
    explicit Lex(lng x) : a(x), b(x) {}
    Lex(lng x, lng y) : a(x), b(y) {}
    Lex operator+(const Lex &o) const { return {a+o.a,b+o.b}; }
    Lex &operator+=(const Lex &o) { a += o.a; b += o.b; return *this; }
    bool operator<(const Lex &o) const { return a != o.a ? a < o.a : b < o.b; }
    bool operator==(const Lex &o) const = default;
    friend std::ostream &operator<<(std::ostream &os, const Lex &x) { return os << '(' << x.a << ';' << x.b << ')'; }
};
int sortedWidth(int n, int b) { return b ? b : max(1,int(std::floor(std::sqrt(static_cast<long double>(n)*std::log2(static_cast<long double>(n)+1))))); }
// Linear-scan oracle for one (l, r, x); maxLess must leave res untouched when it returns false.
template<typename T> void checkSortedQuery(const SortedBlocks<T> &d, const vector<T> &a, int l, int r, const T &x, const T &sentinel) {
    array<int,5> want{}; bool found = false; T best = sentinel;
    for (int i = l; i < r; ++i) {
        want[0] += a[i] < x; want[1] += !(x < a[i]); want[2] += !(a[i] < x); want[3] += x < a[i]; want[4] += !(a[i] < x) && !(x < a[i]);
        if (a[i] < x && (!found || best < a[i])) { best = a[i]; found = true; }}
    T res = sentinel; bool got = d.maxLess(l,r,x,res);
    array<int,5> counts{d.countLess(l,r,x),d.countLeq(l,r,x),d.countGeq(l,r,x),d.countGreater(l,r,x),d.countEqual(l,r,x)};
    T y = x; bool got_alias = d.maxLess(l,r,y,y);
    if (counts != want || got != found || res != best || got_alias != found || y != (found ? best : x)) {
        string where = "("+show(l)+","+show(r)+","+show(x)+")";
        const array<string,5> names{"countLess","countLeq","countGeq","countGreater","countEqual"};
        for (int k = 0; k < 5; ++k) { check(counts[k],want[k],names[k]+where); }
        check(got,found,"maxLess found"+where); check(res,best,"maxLess value"+where);
        check(got_alias,found,"maxLess aliased found"+where); check(y,found ? best : x,"maxLess aliased value"+where);}}
template<typename T> void verifySorted(const SortedBlocks<T> &d, const vector<T> &a, const vector<T> &xs, const T &sentinel) {
    int n = int(a.size()); check(d.n,n,"sorted size"); checkVector(d.values(),a,"sorted values");
    for (int i = 0; i < n; ++i) {
        if (d.get(i) != a[i]) { fail("sorted get "+show(i),show(a[i]),show(d.get(i))); }}
    for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) { for (const T &x : xs) { checkSortedQuery(d,a,l,r,x,sentinel); }}}}
void sortedExhaustive() {
    int limit = mode == "quick" ? 3 : 5, deep = mode == "quick" ? 3 : 5;
    auto queries = [](const vector<lng> &v) {
        lng lo = 0, hi = 0; vector<lng> xs;
        if (!v.empty()) { lo = *std::min_element(v.begin(),v.end()); hi = *std::max_element(v.begin(),v.end()); }
        for (lng x = lo-1; x <= hi+1; ++x) { xs.push_back(x); }
        return xs;};
    // Every add of -1/+1 and assign of 0/1 on every nonempty range (and one empty range) at each step.
    auto visit = [&](auto &&self, const SortedBlocks<lng> &d, const vector<lng> &v, int left) -> void {
        verifySorted(d,v,queries(v),lng(99));
        if (!left) { return; }
        int n = int(v.size());
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
            if (l == r && l) { continue; }
            for (int op = 0; op < 4; ++op) {
                lng x = op < 2 ? 2*op-1 : op-2; auto e = d; auto w = v;
                if (op < 2) { e.add(l,r,x); }
                else { e.assign(l,r,x); }
                for (int i = l; i < r; ++i) { w[i] = op < 2 ? w[i]+x : x; }
                history.push_back({op/2,l,r,x}); self(self,e,w,left-1); history.pop_back();}}}};
    for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask;
            for (lng &x : a) { x = code%3-1; code /= 3; }
            input = a;
            for (int b : widths(n)) {
                context = "sorted exhaustive n="+show(n)+" mask="+show(mask)+" B="+show(b); history.clear();
                SortedBlocks<lng> d(a,b); check(d.width,sortedWidth(n,b),"sorted width"); visit(visit,d,a,1);}}}
    for (int n = 0; n <= deep; ++n) {
        vector<lng> a{1,-1,0,1,-1}; a.resize(n); input = a;
        int depth = mode == "quick" ? 2 : n <= 3 ? 3 : 2;
        for (int b : widths(n)) {
            context = "sorted histories n="+show(n)+" B="+show(b)+" depth="+show(depth); history.clear();
            visit(visit,SortedBlocks<lng>(a,b),a,depth);}}
    context = "sorted default"; input.clear(); history.clear();
    SortedBlocks<lng> none; check(none.width,1,"sorted default width"); verifySorted(none,vector<lng>{},vector<lng>{0},lng(99));
    static_assert(!std::is_convertible_v<vector<lng>, SortedBlocks<lng>>);
    cout << "PASS sqrt sorted-blocks exhaustive n<=" << limit << " histories n<=" << deep << '\n';}
template<typename T, typename V, typename D, typename Q>
void sortedRandom(const string &name, ulng salt, int maxn, const T &sentinel, V value, D delta, Q query) {
    std::mt19937_64 rng(seed*0x9E3779B97F4A7C15ULL+salt);
    int trials = mode == "quick" ? 10 : mode == "full" ? 60 : 240;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%ulng(maxn+1)), b = rng()%6 == 0 ? INT_MAX : int(rng()%ulng(n+3));
        vector<T> a(n);
        for (T &x : a) { x = value(rng); }
        input.clear(); history.clear(); context = "sorted random "+name+" trial="+show(trial)+" n="+show(n)+" B="+show(b);
        SortedBlocks<T> d(a,b); check(d.width,sortedWidth(n,b),"sorted width"); checkVector(d.values(),a,"sorted values");
        // Endpoints land on block boundaries a third of the time, so whole-block paths run often.
        auto pick = [&]() {
            int p = int(rng()%ulng(n+1));
            if (rng()%3 == 0 && d.width <= n) { p = min(n,int(rng()%ulng(n/d.width+2))*d.width); }
            return p;};
        auto probe = [&]() {
            int l = pick(), r = pick();
            if (l > r) { swap(l,r); }
            checkSortedQuery(d,a,l,r,query(rng,a),sentinel);};
        for (int operation = 0; operation < 300; ++operation) {
            int l = pick(), r = pick();
            if (l > r) { swap(l,r); }
            bool adding = rng()%2;
            if (adding) {
                T x = delta(rng,a,l,r); d.add(l,r,x);
                for (int i = l; i < r; ++i) { a[i] = a[i]+x; }}
            else {
                T x = value(rng); d.assign(l,r,x);
                for (int i = l; i < r; ++i) { a[i] = x; }}
            history.push_back({adding ? 0 : 1,l,r,0}); checkVector(d.values(),a,"sorted values");
            for (int k = 0; k < 4; ++k) { probe(); }}
        for (int k = 0; k < 4*n+4; ++k) { probe(); }
        auto copy = d; checkVector(copy.values(),a,"sorted copy"); auto moved = std::move(copy); checkVector(moved.values(),a,"sorted move");}
    cout << "PASS sqrt sorted-blocks random " << name << " trials=" << trials << '\n';}
void sortedRandomCases() {
    int maxn = mode == "quick" ? 64 : 200;
    auto ints = [](std::mt19937_64 &g) { return lng(g()%21)-10; };
    sortedRandom<lng>("int64-small",1,maxn,lng(1000),ints,[](std::mt19937_64 &g, const vector<lng> &, int, int) { return lng(g()%7)-3; },
        [&](std::mt19937_64 &g, const vector<lng> &a) { lng x = existingOr(g,a,ints); return x+lng(g()%3)-1; });
    // Values stay in [-L, L], so every pending add tag (a difference of two values) stays representable.
    const lng L = LLONG_MAX/2;
    auto wide = [L](std::mt19937_64 &g) {
        ulng kind = g()%3, k = g()%4, r = g()%ulng(2*L+1);
        return kind == 0 ? L-lng(k) : kind == 1 ? lng(k)-L : lng(r)-L;};
    auto wide_delta = [L](std::mt19937_64 &g, const vector<lng> &a, int l, int r) {
        lng mn = 0, mx = 0;
        if (l < r) { mn = *std::min_element(a.begin()+l,a.begin()+r); mx = *std::max_element(a.begin()+l,a.begin()+r); }
        lng lo = -L-mn, hi = L-mx; ulng kind = g()%4, k = g();
        return kind == 0 ? hi : kind == 1 ? lo : kind == 2 ? lo+lng(k%ulng(hi-lo+1)) : max(lo,min(hi,lng(k%7)-3));};
    auto wide_query = [&](std::mt19937_64 &g, const vector<lng> &a) {
        ulng kind = g()%6; lng x = existingOr(g,a,wide), k = lng(g()%3)-1;
        return kind == 0 ? lng(LLONG_MIN) : kind == 1 ? lng(LLONG_MAX) : kind == 2 ? (k < 0 ? -L-1 : L+1) : x+k;};
    sortedRandom<lng>("int64-wide",2,maxn,lng(7),wide,wide_delta,wide_query);
    auto dyadic = [](std::mt19937_64 &g) { return double(int(g()%41)-20)/8; };
    sortedRandom<double>("double-dyadic",3,maxn,1e9,dyadic,[](std::mt19937_64 &g, const vector<double> &, int, int) { return double(int(g()%17)-8)/4; },
        [&](std::mt19937_64 &g, const vector<double> &a) { double x = existingOr(g,a,dyadic); return x+double(int(g()%3)-1)/8; });
    auto lex = [](std::mt19937_64 &g) { return Lex{lng(g()%5)-2,lng(g()%5)-2}; };
    sortedRandom<Lex>("lexicographic",4,maxn,Lex{1000,1000},lex,[](std::mt19937_64 &g, const vector<Lex> &, int, int) { return Lex{lng(g()%3)-1,lng(g()%3)-1}; },
        [&](std::mt19937_64 &g, const vector<Lex> &a) { Lex x = existingOr(g,a,lex); return x+Lex{lng(g()%3)-1,lng(g()%3)-1}; });}
void sortedExtremes() {
    const lng MN = LLONG_MIN, MX = LLONG_MAX;
    const vector<lng> xs{MN,MN+1,MN+3,MN+7,MN+8,-8,-7,-6,-5,-3,-2,-1,0,1,2,6,MX-1,MX};
    input.clear();
    for (int b : widths(7)) {
        context = "sorted int64 extremes assign B="+show(b); history.clear();
        vector<lng> a{MN,MX,MN,0,MX,-1,1}; SortedBlocks<lng> d(a,b); verifySorted(d,a,xs,lng(42));
        auto assign = [&](int l, int r, lng x) {
            d.assign(l,r,x); history.push_back({1,l,r,x});
            for (int i = l; i < r; ++i) { a[i] = x; }
            verifySorted(d,a,xs,lng(42));};
        assign(1,4,MX); assign(0,7,MN); assign(2,5,MX); assign(3,3,0); assign(6,7,MN+1); assign(0,7,d.get(6));}
    for (int b : widths(5)) {
        context = "sorted int64 extremes add B="+show(b); history.clear();
        vector<lng> a{MN,MN+1,MN,MN+7,MN+3}; SortedBlocks<lng> d(a,b); verifySorted(d,a,xs,lng(42));
        auto add = [&](int l, int r, lng x) {
            d.add(l,r,x); history.push_back({0,l,r,x});
            for (int i = l; i < r; ++i) { a[i] += x; }
            verifySorted(d,a,xs,lng(42));};
        add(0,5,MX); add(0,5,MN+1); add(0,5,MX-7); add(1,3,5); add(0,5,2); add(2,2,MX);}
    cout << "PASS sqrt sorted-blocks int64 extremes\n";}
// Minimal ordered type: only T(int), copies, + and <; no default constructor, no +=, no ==.
struct Bare {
    lng a;
    explicit Bare(int x) : a(x) {}
    Bare operator+(const Bare &o) const { Bare res(0); res.a = a+o.a; return res; }
    bool operator<(const Bare &o) const { return a < o.a; }
};
void checkBareQuery(const SortedBlocks<Bare> &d, const vector<lng> &a, int l, int r, lng x) {
    array<int,5> want{}; bool found = false; lng best = 777;
    for (int i = l; i < r; ++i) {
        want[0] += a[i] < x; want[1] += a[i] <= x; want[2] += a[i] >= x; want[3] += a[i] > x; want[4] += a[i] == x;
        if (a[i] < x && (!found || best < a[i])) { best = a[i]; found = true; }}
    Bare res(777), y{int(x)}, q{int(x)};
    array<int,5> counts{d.countLess(l,r,q),d.countLeq(l,r,q),d.countGeq(l,r,q),d.countGreater(l,r,q),d.countEqual(l,r,q)};
    bool got = d.maxLess(l,r,q,res), got_alias = d.maxLess(l,r,y,y);
    if (counts != want || got != found || res.a != best || got_alias != found || y.a != (found ? best : x)) {
        string where = "("+show(l)+","+show(r)+","+show(x)+")";
        const array<string,5> names{"countLess","countLeq","countGeq","countGreater","countEqual"};
        for (int k = 0; k < 5; ++k) { check(counts[k],want[k],"bare "+names[k]+where); }
        check(got,found,"bare maxLess found"+where); check(res.a,best,"bare maxLess value"+where);
        check(got_alias,found,"bare maxLess aliased found"+where); check(y.a,found ? best : x,"bare maxLess aliased value"+where);}}
void sortedBare() {
    std::mt19937_64 rng(seed+13);
    int trials = mode == "quick" ? 20 : mode == "full" ? 80 : 320;
    auto project = [](const vector<Bare> &v) {
        vector<lng> res;
        for (const Bare &x : v) { res.push_back(x.a); }
        return res;};
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%17), b = rng()%6 == 0 ? INT_MAX : int(rng()%ulng(n+3));
        vector<lng> a(n); vector<Bare> init;
        for (lng &x : a) { x = lng(rng()%11)-5; init.push_back(Bare(int(x))); }
        input = a; history.clear(); context = "sorted bare trial="+show(trial)+" n="+show(n)+" B="+show(b);
        SortedBlocks<Bare> d(init,b); check(d.width,sortedWidth(n,b),"bare width");
        auto verify = [&](const SortedBlocks<Bare> &e) {
            checkVector(project(e.values()),a,"bare values");
            for (int i = 0; i < n; ++i) { check(e.get(i).a,a[i],"bare get "+show(i)); }
            for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) {
                for (int k = 0; k < 2; ++k) {
                    lng x = existingOr(rng,a,[](std::mt19937_64 &g) { return lng(g()%11)-5; }); x += lng(rng()%3)-1;
                    checkBareQuery(e,a,l,r,x);}
                checkBareQuery(e,a,l,r,(l+r)%2 ? 1000 : -1000);}}};
        verify(d);
        for (int operation = 0; operation < 40; ++operation) {
            int l = int(rng()%ulng(n+1)), r = int(rng()%ulng(n+1));
            if (l > r) { swap(l,r); }
            bool adding = rng()%2; int x = adding ? int(rng()%7)-3 : int(rng()%11)-5;
            if (adding) { d.add(l,r,Bare(x)); }
            else { d.assign(l,r,Bare(x)); }
            for (int i = l; i < r; ++i) { a[i] = adding ? a[i]+x : x; }
            history.push_back({adding ? 0 : 1,l,r,x}); verify(d);}
        SortedBlocks<Bare> copy(d); verify(copy); copy = SortedBlocks<Bare>(vector<Bare>{Bare(1)},1); copy = d; verify(copy);
        SortedBlocks<Bare> moved(std::move(copy)); verify(moved);}
    SortedBlocks<Bare> none; check(none.n,0,"bare default size"); check(none.countLess(0,0,Bare(0)),0,"bare empty count");
    cout << "PASS sqrt sorted-blocks minimal type (T(int), copy, +, <) with aliased maxLess trials=" << trials << '\n';}
// Instrumented block: logs every call with its global range; the proto's log and tag must reach every copy.
struct Probe {
    vector<array<int,4>> *log = nullptr;
    int tag = 0, l = 0, r = 0;
    void build(int lo, int hi) { l = lo; r = hi; log->push_back({0,lo,hi,tag}); }
    void updateAll(int x) { log->push_back({1,l,r,x}); }
    void updatePart(int lo, int hi, int x) { log->push_back({2,lo,hi,x}); }
    string queryAll() { log->push_back({3,l,r,0}); return "<"+show(l)+","+show(r)+">"; }
    string queryPart(int lo, int hi) { log->push_back({4,lo,hi,0}); return "["+show(lo)+","+show(hi)+")"; }
};
string showLog(const vector<array<int,4>> &log) {
    string s;
    for (auto [kind,l,r,x] : log) { s += "("+show(kind)+","+show(l)+","+show(r)+","+show(x)+")"; }
    return s;}
int decompWidth(int n, int b) { return b ? b : max(1,rootFloor(n)); }
// Expected call sequence for (n, B) on the given ranges, computed in 64 bits; n / width stays small, so the blocks are few.
template<typename M> void decompCalls(int n, int b, const vector<pair<int,int>> &ranges, M mon, bool backwards, const string &name) {
    context = "decomp calls "+name+" n="+show(n)+" B="+show(b);
    int w = decompWidth(n,b); vector<array<int,4>> log, want;
    for (lng s = 0; s < n; s += w) { want.push_back({0,int(s),int(min(lng(n),s+w)),42}); }
    BlockDecomp<Probe,M> d(n,b,Probe{&log,42},mon);
    check(d.n,n,"decomp size"); check(d.width,w,"decomp width"); check(d.blks.size(),want.size(),"decomp block count");
    check(showLog(log),showLog(want),"decomp build calls");
    for (size_t bi = 0; bi < want.size(); ++bi) { check(d.blks[bi].log == &log && d.blks[bi].tag == 42,true,"decomp proto copy "+show(bi)); }
    for (auto [l,r] : ranges) {
        int x = int((31*lng(l)+r)%1000003); string expected, where = "("+show(l)+","+show(r)+")"; want.clear();
        for (lng s = 0; s < n; s += w) {
            lng e = min(lng(n),s+w), lo = max(lng(l),s), hi = min(lng(r),e);
            if (lo < hi) { want.push_back({lo == s && hi == e ? 1 : 2,int(lo),int(hi),x}); }}
        log.clear(); d.apply(l,r,x); check(showLog(log),showLog(want),"decomp apply"+where+" calls");
        for (auto &call : want) {
            string piece = call[0] == 1 ? "<"+show(call[1])+","+show(call[2])+">" : "["+show(call[1])+","+show(call[2])+")";
            expected = backwards ? piece+expected : expected+piece; call[0] += 2; call[3] = 0;}
        log.clear(); string got = d.prod(l,r);
        check(showLog(log),showLog(want),"decomp prod"+where+" calls"); check(got,expected,"decomp prod"+where);}}
template<typename M> void decompCallsWith(M mon, bool backwards, const string &name) {
    int limit = mode == "quick" ? 7 : 12;
    input.clear(); history.clear();
    for (int n = 0; n <= limit; ++n) {
        vector<pair<int,int>> ranges;
        for (int l = 0; l <= n; ++l) { for (int r = l; r <= n; ++r) { ranges.push_back({l,r}); }}
        for (int b : widths(n)) { decompCalls(n,b,ranges,mon,backwards,name); }}
    // Near INT_MAX: block ends must not overflow when width is close to n.
    const int MX = INT_MAX, H = 1 << 30;
    vector<pair<int,int>> ends{{0,MX},{0,0},{MX,MX},{MX-3,MX},{H-1,H+1},{H,MX},{H,MX-1},{5,MX-1},{0,H},{MX-1,MX},{1,2},{0,MX-2},{MX-5,MX-2},{H-1,MX-2},{2*(H-1)-1,2*(H-1)}};
    for (auto [n,b] : vector<pair<int,int>>{{MX,H},{MX,H+1},{MX,MX-1},{MX,MX},{MX-1,H},{MX,H-1},{MX-2,H-1}}) {
        vector<pair<int,int>> fit;
        for (auto [l,r] : ends) {
            if (r <= n) { fit.push_back({l,r}); }}
        decompCalls(n,b,fit,mon,backwards,name);}
    cout << "PASS sqrt block-decomp call sequence " << name << " n<=" << limit << " and n near INT_MAX\n";}
// Range-add range-sum block over caller data reached through the proto's pointer; partial ranges must be proper and nonempty.
struct AddSum {
    vector<lng> *a = nullptr;
    int l = 0, r = 0; lng tag = 0, sum = 0;
    void build(int lo, int hi) {
        l = lo; r = hi; sum = 0;
        for (int i = lo; i < hi; ++i) { sum += (*a)[i]; }}
    void inside(int lo, int hi) const {
        if (!(l <= lo && lo < hi && hi <= r) || (lo == l && hi == r)) { fail("decomp part range","proper part of ["+show(l)+","+show(r)+")","["+show(lo)+","+show(hi)+")"); }}
    void updateAll(lng x) { tag += x; sum += x*(r-l); }
    void updatePart(int lo, int hi, lng x) {
        inside(lo,hi);
        for (int i = lo; i < hi; ++i) { (*a)[i] += x; }
        sum += x*(hi-lo);}
    lng queryAll() const { return sum; }
    lng queryPart(int lo, int hi) const {
        inside(lo,hi); lng res = 0;
        for (int i = lo; i < hi; ++i) { res += (*a)[i]+tag; }
        return res;}
};
void decompCases() {
    auto cat = [](const string &x, const string &y) { return x+y; };
    decompCallsWith(Monoid<string,decltype(cat)>{string{},cat},false,"Monoid-concat");
    decompCallsWith(Join{false},false,"Join"); decompCallsWith(Join{true},true,"Join-backwards");
    std::mt19937_64 rng(seed+3);
    int trials = mode == "quick" ? 20 : mode == "full" ? 100 : 400, maxn = mode == "quick" ? 64 : 200;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%ulng(maxn+1)), b = rng()%6 == 0 ? INT_MAX : int(rng()%ulng(n+3));
        vector<lng> data(n);
        for (lng &x : data) { x = lng(rng()%21)-10; }
        vector<lng> a = data; input = a; history.clear(); context = "decomp random trial="+show(trial)+" n="+show(n)+" B="+show(b);
        BlockDecomp<AddSum,Sum<lng>> d(n,b,AddSum{&data}); int w = decompWidth(n,b); check(d.width,w,"decomp width");
        for (int operation = 0; operation < 300; ++operation) {
            int l = int(rng()%ulng(n+1)), r = int(rng()%ulng(n+1));
            if (l > r) { swap(l,r); }
            lng x = lng(rng()%11)-5; d.apply(l,r,x); history.push_back({0,l,r,x});
            for (int i = l; i < r; ++i) { a[i] += x; }
            int ql = int(rng()%ulng(n+1)), qr = int(rng()%ulng(n+1));
            if (ql > qr) { swap(ql,qr); }
            check(d.prod(ql,qr),accumulate(a.begin()+ql,a.begin()+qr,lng(0)),"decomp prod("+show(ql)+","+show(qr)+")");
            if (operation%50 == 0) {
                for (int i = 0; i < n; ++i) { check(data[i]+d.blks[size_t(i/w)].tag,a[i],"decomp element "+show(i)); }}}}
    context = "decomp empty"; input.clear(); history.clear();
    BlockDecomp<Probe,Join> none; check(none.n,0,"decomp empty size"); check(none.blks.size(),size_t(0),"decomp empty blocks");
    none.apply(0,0,1); check(none.prod(0,0),string{},"decomp empty prod");
    BlockDecomp<AddSum,Sum<lng>> zero(0,5); zero.apply(0,0,lng(3)); check(zero.prod(0,0),lng(0),"decomp empty sum");
    static_assert(!std::is_convertible_v<int, BlockDecomp<AddSum,Sum<lng>>>);
    cout << "PASS sqrt block-decomp range-add sum/empty trials=" << trials << '\n';}
template<typename T> string showSizes(const BlockList<T> &d) {
    string s;
    for (const auto &blk : d.blks) { s += show(blk.size())+","; }
    return s;}
// Contract widths: B = 0 refreshes to max(1, floor(sqrt(n))) when n leaves [width^2 / 4, 4 * width^2].
int listWidth(int n, int w, bool adaptive) { return adaptive && !(4*lng(n) >= lng(w)*w && lng(n) <= 4*lng(w)*w) ? max(1,rootFloor(n)) : w; }
// Widths above INT_MAX / 2 check that 2 * width is not formed in int.
vector<int> listWidths(int n) {
    vector<int> res = widths(n); res.push_back(3); res.push_back(INT_MAX/2+1);
    sort(res.begin(),res.end()); res.erase(unique(res.begin(),res.end()),res.end());
    return res;}
template<typename T, typename K> void listCheck(const BlockList<T> &d, const vector<lng> &a, int w, bool adaptive, K key, bool contents) {
    int n = int(a.size()); check(d.size(),n,"list size"); check(d.n,n,"list n"); check(d.width,w,"list width"); check(d.adaptive,adaptive,"list adaptive");
    lng total = 0, prev = -1;
    for (const auto &blk : d.blks) {
        lng s = lng(blk.size());
        if (!s || s > 2*lng(w) || (prev >= 0 && prev+s < w)) { fail("list invariant","nonempty blocks <= 2*width, pairs >= width="+show(w),showSizes(d)); }
        total += s; prev = s;}
    check(total,lng(n),"list total");
    if (!contents) { return; }
    int i = 0;
    for (const auto &blk : d.blks) { for (const T &x : blk) {
        if (key(x) != a[size_t(i)]) { fail("list element "+show(i),show(a[size_t(i)]),show(key(x))); }
        ++i;}}}
// After a reshape every block but the last holds exactly width elements.
template<typename T> void listLayout(const BlockList<T> &d, int w) {
    for (size_t bi = 0; bi < d.blks.size(); ++bi) {
        size_t s = d.blks[bi].size();
        if (bi+1 < d.blks.size() ? s != size_t(w) : s < 1 || s > size_t(w)) { fail("list layout "+show(bi),"width "+show(w),showSizes(d)); }}}
void listExhaustive() {
    int depth = mode == "quick" ? 3 : 4, stamp = 100;
    auto id = [](int x) { return lng(x); };
    auto visit = [&](auto &&self, const BlockList<int> &d, const vector<lng> &a, int w, bool adaptive, int left) -> void {
        listCheck(d,a,w,adaptive,id,true);
        int n = int(a.size());
        for (int i = 0; i < n; ++i) {
            if (d.at(i) != a[size_t(i)]) { fail("list at "+show(i),show(a[size_t(i)]),show(d.at(i))); }}
        if (!left) { return; }
        for (int i = 0; i <= n; ++i) {
            auto e = d; auto v = a; int x = ++stamp; int nw = listWidth(n+1,w,adaptive);
            e.insert(i,x); v.insert(v.begin()+i,x); history.push_back({0,i,x,0});
            if (nw != w) { listLayout(e,nw); }
            self(self,e,v,nw,adaptive,left-1); history.pop_back();}
        for (int i = 0; i < n; ++i) {
            auto e = d; auto v = a; int nw = listWidth(n-1,w,adaptive);
            e.erase(i); v.erase(v.begin()+i); history.push_back({1,i,0,0});
            if (nw != w) { listLayout(e,nw); }
            self(self,e,v,nw,adaptive,left-1); history.pop_back();
            e = d; v = a; e.at(i) = -i-1; v[size_t(i)] = -i-1; history.push_back({2,i,-i-1,0});
            self(self,e,v,w,adaptive,left-1); history.pop_back();}};
    for (int n = 0; n <= 3; ++n) { for (int b : listWidths(n)) {
        context = "list exhaustive n="+show(n)+" B="+show(b)+" depth="+show(depth); history.clear();
        vector<lng> a(n); vector<int> init(n);
        std::iota(a.begin(),a.end(),lng(0)); std::iota(init.begin(),init.end(),0); input = a;
        BlockList<int> d(init,b); int w = b ? b : max(1,rootFloor(n)); listLayout(d,w); visit(visit,d,a,w,b == 0,depth);}}
    cout << "PASS sqrt block-list exhaustive histories n<=3 depth=" << depth << '\n';}
template<typename T, typename Make, typename Key>
void listRandom(const string &name, ulng salt, int maxn, Make make, Key key) {
    std::mt19937_64 rng(seed*0x9E3779B97F4A7C15ULL+salt);
    int trials = mode == "quick" ? 10 : mode == "full" ? 50 : 200;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%ulng(maxn+1)), b = rng()%3 == 0 ? 0 : int(rng()%ulng(n+3));
        if (rng()%8 == 0) { b = rng()%2 ? INT_MAX : INT_MAX/2+1; }
        vector<lng> a(n); vector<T> init;
        for (lng &x : a) { x = lng(rng()%1000); init.push_back(make(x)); }
        input.clear(); history.clear(); context = "list random "+name+" trial="+show(trial)+" n="+show(n)+" B="+show(b);
        bool adaptive = b == 0; int w = b ? b : max(1,rootFloor(n));
        BlockList<T> d(std::move(init),b); listCheck(d,a,w,adaptive,key,true); listLayout(d,w);
        for (int operation = 0; operation < 600; ++operation) {
            int kind = int(rng()%8), size = int(a.size()), i = 0; lng x = lng(rng()%1000)+1000;
            if (!size || kind < (operation < 300 ? 4 : 2)) {
                ulng where = rng()%3; i = where == 0 ? 0 : where == 1 ? size : int(rng()%ulng(size+1));
                int nw = listWidth(size+1,w,adaptive); d.insert(i,make(x)); a.insert(a.begin()+i,x);
                if (nw != w) { listLayout(d,nw); }
                w = nw;}
            else if (kind < 6) {
                i = int(rng()%ulng(size)); int nw = listWidth(size-1,w,adaptive); d.erase(i); a.erase(a.begin()+i);
                if (nw != w) { listLayout(d,nw); }
                w = nw;}
            else if (kind == 6) { i = int(rng()%ulng(size)); d.at(i) = make(x); a[size_t(i)] = x; }
            else {
                const BlockList<T> &c = d; i = int(rng()%ulng(size));
                if (key(c.at(i)) != a[size_t(i)]) { fail("list const at "+show(i),show(a[size_t(i)]),show(key(c.at(i)))); }}
            history.push_back({kind,i,x,0}); listCheck(d,a,w,adaptive,key,operation%16 == 0);
            if (!a.empty()) {
                int j = int(rng()%a.size());
                if (key(d.at(j)) != a[size_t(j)]) { fail("list at "+show(j),show(a[size_t(j)]),show(key(d.at(j)))); }}}
        listCheck(d,a,w,adaptive,key,true); BlockList<T> moved(std::move(d)); listCheck(moved,a,w,adaptive,key,true);}
    cout << "PASS sqrt block-list random " << name << " trials=" << trials << '\n';}
// Move-only element without a default constructor: splits, merges and reshapes may only move.
struct NoDef {
    lng v;
    explicit NoDef(int x) : v(x) {}
    NoDef(const NoDef &) = delete;
    NoDef(NoDef &&) = default;
    NoDef &operator=(const NoDef &) = delete;
    NoDef &operator=(NoDef &&) = default;
};
// Inserts at the front, back and middle up to top elements, then erases back to empty, checking width and invariants each step.
template<typename T, typename Make, typename Key>
void listGrowth(const string &name, const vector<pair<int,int>> &plans, Make make, Key key) {
    std::mt19937_64 rng(seed+11);
    for (auto [b,top] : plans) {
        context = "list growth "+name+" B="+show(b)+" top="+show(top); input.clear(); history.clear();
        BlockList<T> d({},b); vector<lng> a; bool adaptive = b == 0; int w = b ? b : 1;
        for (int step = 0; step < 2*top; ++step) {
            int size = int(a.size()), pos = step%3 == 0 ? 0 : step%3 == 1 ? size-(step >= top) : int(rng()%ulng(size+(step < top)));
            int nw = listWidth(size+(step < top ? 1 : -1),w,adaptive);
            if (step < top) { d.insert(pos,make(step)); a.insert(a.begin()+pos,step); }
            else { d.erase(pos); a.erase(a.begin()+pos); }
            if (nw != w) { listLayout(d,nw); }
            w = nw; listCheck(d,a,w,adaptive,key,step%512 == 0 || step+1 == 2*top);
            if (step+1 == top && adaptive) { check(rootFloor(lng(top))/2 <= d.width && d.width <= 2*rootFloor(lng(top)),true,"list width near sqrt(n)"); }}
        check(d.blks.size(),size_t(0),"list blocks after shrink");}
    cout << "PASS sqrt block-list growth/shrink " << name << '\n';}
void listCases() {
    listExhaustive();
    int maxn = mode == "quick" ? 64 : 200;
    listRandom<int>("int",5,maxn,[](lng x) { return int(x); },[](int x) { return lng(x); });
    listRandom<string>("string",6,maxn,[](lng x) { return string(x%3 ? 24 : 0,'#')+"v"+std::to_string(x); },
        [](const string &s) { return lng(std::stoll(s.substr(s.find('v')+1))); });
    listRandom<std::unique_ptr<lng>>("unique_ptr",7,maxn,[](lng x) { return std::make_unique<lng>(x); },[](const std::unique_ptr<lng> &p) { return *p; });
    listRandom<NoDef>("move-only-no-default",8,maxn,[](lng x) { return NoDef(int(x)); },[](const NoDef &x) { return x.v; });
    bool quick = mode == "quick";
    listGrowth<int>("int",quick ? vector<pair<int,int>>{{0,2000},{5,500},{INT_MAX,300}} : vector<pair<int,int>>{{0,10000},{1,1500},{3,3000},{64,10000},{INT_MAX,2000},{INT_MAX/2+1,2000}},
        [](int x) { return x; },[](int x) { return lng(x); });
    listGrowth<NoDef>("move-only-no-default",quick ? vector<pair<int,int>>{{0,1000},{3,300}} : vector<pair<int,int>>{{0,4000},{2,1500},{INT_MAX,1000}},
        [](int x) { return NoDef(x); },[](const NoDef &x) { return x.v; });
    context = "list types"; input.clear(); history.clear();
    BlockList<string> s(vector<string>{"a","b","c"},2); s.at(1) += "z"; s.insert(3,"d"); s.erase(0);
    string text(40,'q'); s.insert(1,std::move(text)); const auto &cs = s;
    check(cs.at(0)+"|"+cs.at(1)+"|"+cs.at(2)+"|"+cs.at(3),string("bz|"+string(40,'q')+"|c|d"),"list string contents");
    static_assert(std::is_same_v<decltype(s.at(0)), string &> && std::is_same_v<decltype(cs.at(0)), const string &>);
    BlockList<std::unique_ptr<lng>> u; u.insert(0,std::make_unique<lng>(5)); u.insert(0,std::make_unique<lng>(9)); *u.at(1) += 1;
    check(*u.at(0)*10+*u.at(1),lng(96),"list unique_ptr contents"); u.erase(0); u.erase(0); check(u.size(),0,"list unique_ptr empty");
    BlockList<int> none; check(none.size(),0,"list default size"); check(none.blks.empty(),true,"list default blocks");
    static_assert(!std::is_convertible_v<vector<int>, BlockList<int>>);
    cout << "PASS sqrt block-list growth/shrink/string/move-only\n";}
// Operation counters: a whole-range query must use full blocks, so its work stays O(n / k + k), not O(n).
struct CountSum {
    using S = lng;
    static inline lng ops = 0;
    S op(S a, S b) const { ++ops; return a + b; }
    S e() const { return 0; }
};
struct CountRing {
    lng a;
    CountRing(lng x = 0) : a(x) {}
    static inline lng adds = 0;
    CountRing operator+(const CountRing &o) const { ++adds; return a + o.a; }
    CountRing operator*(const CountRing &o) const { return a * o.a; }
    bool operator==(const CountRing &o) const { return a == o.a; }
};
struct CountLess {
    lng a;
    CountLess(lng x) : a(x) {}
    static inline lng cmps = 0;
    CountLess operator+(const CountLess &o) const { return a + o.a; }
    bool operator<(const CountLess &o) const { ++cmps; return a < o.a; }
};
void costCounts() {
    context = "cost counts";
    int n = 10000;
    vector<lng> a(n); std::iota(a.begin(), a.end(), 0);
    lng total = lng(n) * (n - 1) / 2;
    SqrtDecomp<CountSum> g(a);
    CountSum::ops = 0; check(g.prod(0, n), total, "counted prod");
    check(CountSum::ops <= n / g.width + 2 * g.width + 2, true, "prod uses full blocks ops=" + std::to_string(CountSum::ops));
    SqrtRangeSum<CountRing> d(vector<CountRing>(a.begin(), a.end()));
    CountRing::adds = 0; check(d.sum(0, n).a, total, "counted sum");
    check(CountRing::adds <= n / d.m + 4 * d.m + 4, true, "sum uses full blocks adds=" + std::to_string(CountRing::adds));
    SortedBlocks<CountLess> sb(vector<CountLess>(a.begin(), a.end()));
    int k = sb.width, bound = (n / k + 1) * (std::bit_width(unsigned(k)) + 2) + 2 * k + 4;
    CountLess::cmps = 0; check(sb.countLess(0, n, CountLess(n / 2)), n / 2, "counted countLess");
    check(CountLess::cmps <= bound, true, "countLess uses full blocks cmps=" + std::to_string(CountLess::cmps));
    cout << "PASS sqrt full-block operation counts\n";}

struct Reached : std::runtime_error {
    Reached() : std::runtime_error("operation reached past the size assert") {}
};
void boolListAndLimits() {
    context = "BlockList<bool> bit proxies";
    for (int b : {0, 1, 2}) {
        BlockList<bool> bits(vector<bool>{true, false, true, false}, b);
        bits.at(1) = true; bits.insert(4, true); bits.erase(0);
        const BlockList<bool> &view = bits;
        vector<bool> got;
        for (int i = 0; i < view.size(); ++i) { got.push_back(view.at(i)); }
        checkVector(got, vector<bool>{true, true, false, true}, "bool list write through at, insert, erase, const read B=" + show(b));}
    if (mode == "quick") { return; }
    // INT_MAX packed bools (256 MiB) are the largest admitted input: the size assert passes, and the throwing op stops the block fold at once.
    context = "largest admitted SqrtDecomp size";
    bool reached = false;
    try { SqrtDecomp big(vector<bool>(INT_MAX), 0, Monoid{false, [](bool, bool) -> bool { throw Reached(); }}); } catch (const Reached &) { reached = true; }
    check(reached, true, "SqrtDecomp of INT_MAX elements passes its assert");
    cout << "PASS sqrt BlockList<bool> proxies and the largest admitted SqrtDecomp size\n";}

int main(int argc, char **argv) {
    string invalid;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i]; if (i+1 == argc) { return 2; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else { return 2; }}
    if (!invalid.empty()) {
        SqrtDecomp<Sum<lng>> g(2,1); SqrtRangeSum<lng> d(2,1); SortedBlocks<lng> sb(vector<lng>{1,2},1); lng res = 0;
        vector<lng> data(2); BlockDecomp<AddSum,Sum<lng>> bd(2,1,AddSum{&data}); BlockList<int> bl(vector<int>{1,2},1);
        if (invalid == "generic-negative-size") { SqrtDecomp<Sum<lng>> bad(-1); }
        else if (invalid == "generic-negative-block") { SqrtDecomp<Sum<lng>> bad(2,-1); }
        else if (invalid == "generic-get-negative") { g.get(-1); }
        else if (invalid == "generic-get-end") { g.get(2); }
        else if (invalid == "generic-set-end") { g.set(2,1); }
        else if (invalid == "generic-apply-end") { g.apply(2,1); }
        else if (invalid == "generic-prod-negative") { g.prod(-1,1); }
        else if (invalid == "generic-prod-reversed") { g.prod(1,0); }
        else if (invalid == "generic-prod-end") { g.prod(0,3); }
        else if (invalid == "generic-rebuild-block") { g.rebuild({},-1); }
        else if (invalid == "lazy-negative-size") { SqrtRangeSum<lng> bad(-1); }
        else if (invalid == "lazy-negative-block") { SqrtRangeSum<lng> bad(2,-1); }
        else if (invalid == "lazy-get-end") { d.get(2); }
        else if (invalid == "lazy-get-past-end") { d.get(3); }
        else if (invalid == "lazy-set-end") { d.set(2,1); }
        else if (invalid == "lazy-sum-negative") { d.sum(-1,1); }
        else if (invalid == "lazy-sum-reversed") { d.sum(1,0); }
        else if (invalid == "lazy-sum-end") { d.sum(0,3); }
        else if (invalid == "lazy-affine-negative") { d.affine(-1,1,2,3); }
        else if (invalid == "lazy-affine-reversed") { d.affine(1,0,2,3); }
        else if (invalid == "lazy-affine-end") { d.affine(0,3,2,3); }
        else if (invalid == "lazy-vector-negative-block") { SqrtRangeSum<lng> bad(vector<lng>{},-1); }
        else if (invalid == "sorted-negative-block") { SortedBlocks<lng> bad(vector<lng>{},-1); }
        else if (invalid == "sorted-get-negative") { sb.get(-1); }
        else if (invalid == "sorted-get-end") { sb.get(2); }
        else if (invalid == "sorted-get-past-end") { sb.get(3); }
        else if (invalid == "sorted-add-negative") { sb.add(-1,1,1); }
        else if (invalid == "sorted-add-reversed") { sb.add(1,0,1); }
        else if (invalid == "sorted-add-end") { sb.add(0,3,1); }
        else if (invalid == "sorted-assign-negative") { sb.assign(-1,1,1); }
        else if (invalid == "sorted-assign-reversed") { sb.assign(1,0,1); }
        else if (invalid == "sorted-assign-end") { sb.assign(0,3,1); }
        else if (invalid == "sorted-countless-end") { sb.countLess(0,3,1); }
        else if (invalid == "sorted-countleq-reversed") { sb.countLeq(1,0,1); }
        else if (invalid == "sorted-countgeq-reversed") { sb.countGeq(1,0,1); }
        else if (invalid == "sorted-countgeq-end") { sb.countGeq(0,3,1); }
        else if (invalid == "sorted-countgreater-reversed") { sb.countGreater(1,0,1); }
        else if (invalid == "sorted-countgreater-end") { sb.countGreater(0,3,1); }
        else if (invalid == "sorted-countequal-reversed") { sb.countEqual(1,0,1); }
        else if (invalid == "sorted-countequal-end") { sb.countEqual(0,3,1); }
        else if (invalid == "sorted-maxless-negative") { sb.maxLess(-1,1,1,res); }
        else if (invalid == "sorted-maxless-end") { sb.maxLess(0,3,1,res); }
        else if (invalid == "decomp-negative-size") { BlockDecomp<AddSum,Sum<lng>> bad(-1); }
        else if (invalid == "decomp-negative-block") { BlockDecomp<AddSum,Sum<lng>> bad(2,-1,AddSum{&data}); }
        else if (invalid == "decomp-apply-negative") { bd.apply(-1,1,lng(1)); }
        else if (invalid == "decomp-apply-reversed") { bd.apply(1,0,lng(1)); }
        else if (invalid == "decomp-apply-end") { bd.apply(0,3,lng(1)); }
        else if (invalid == "decomp-prod-negative") { bd.prod(-1,1); }
        else if (invalid == "decomp-prod-reversed") { bd.prod(1,0); }
        else if (invalid == "decomp-prod-end") { bd.prod(0,3); }
        else if (invalid == "list-negative-block") { BlockList<int> bad(vector<int>{},-1); }
        else if (invalid == "list-insert-negative") { bl.insert(-1,1); }
        else if (invalid == "list-insert-end") { bl.insert(3,1); }
        else if (invalid == "list-erase-negative") { bl.erase(-1); }
        else if (invalid == "list-erase-end") { bl.erase(2); }
        else if (invalid == "list-erase-empty") { BlockList<int> none; none.erase(0); }
        else if (invalid == "list-at-negative") { bl.at(-1); }
        else if (invalid == "list-at-end") { bl.at(2); }
        else if (invalid == "list-const-at-end") { const BlockList<int> &c = bl; c.at(2); }
        else if (invalid == "generic-max-size") { SqrtDecomp bad(vector<bool>(size_t(INT_MAX) + 1), 0, Monoid{false, [](bool x, bool y) { return x || y; }}); }
        else { return 2; }
        cerr << "precondition did not assert\n"; return 1;}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    cout << "sqrt seed=" << seed << " mode=" << mode << '\n';
    genericExhaustive(); noncommutative(); lazyExhaustive(); randomCases(); regressions();
    sortedExhaustive(); sortedRandomCases(); sortedExtremes(); sortedBare(); decompCases(); listCases(); costCounts(); boolListAndLimits();}
