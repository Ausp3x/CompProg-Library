#include "../../02-Data Structures/06-sqrt_decomposition.hpp"

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
    cerr << "] affine-history=";
    for (auto a : history) { cerr << '(' << a[0] << ',' << a[1] << ',' << a[2] << ',' << a[3] << ')'; }
    cerr << '\n'; std::exit(1);}
template<typename A, typename B> void check(const A &actual, const B &expected, const string &op) {
    if (actual != expected) { fail(op,show(expected),show(actual)); }}
vector<int> widths(int n) {
    vector<int> result{0,1,2,max(1,n),n+2,INT_MAX};
    sort(result.begin(),result.end()); result.erase(unique(result.begin(),result.end()),result.end());
    return result;}
template<typename T, typename F>
void verifyGeneric(const SqrtDecomp<T,F> &d, const vector<T> &a, T id, F op) {
    int n = int(a.size()); check(d.n,n,"generic size"); auto values = d.values();
    check(values.size(),a.size(),"generic values size");
    for (int i = 0; i < n; ++i) { check(d.get(i),a[i],"generic get "+show(i)); check(values[i],a[i],"generic values "+show(i)); }
    for (int l = 0; l <= n; ++l) {
        T expected = id;
        for (int r = l; r <= n; ++r) {
            check(d.query(l,r),expected,"generic query("+show(l)+","+show(r)+")");
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
                SqrtDecomp<lng> d(a,b), zero(n,b); verifyGeneric(d,a,lng(0),std::plus<lng>{});
                verifyGeneric(zero,vector<lng>(n),lng(0),std::plus<lng>{});
                for (int i = 0; i < n; ++i) { for (lng x : {-2,0,2}) {
                    auto e = d; auto v = a; v[i] = x; e.set(i,x); verifyGeneric(e,v,lng(0),std::plus<lng>{});
                    e = d; e.setUpdate(i,x); verifyGeneric(e,v,lng(0),std::plus<lng>{});
                    e = d; v = a; v[i] += x; e.opeUpdate(i,x); verifyGeneric(e,v,lng(0),std::plus<lng>{});}}
                auto copy = d; SqrtDecomp<lng> moved(std::move(copy)); verifyGeneric(moved,a,lng(0),std::plus<lng>{});
                copy = d; verifyGeneric(copy,a,lng(0),std::plus<lng>{});
                copy.rebuild(vector<lng>{7,-2,3},2); verifyGeneric(copy,vector<lng>{7,-2,3},lng(0),std::plus<lng>{});
                copy.rebuild({}); verifyGeneric(copy,vector<lng>{},lng(0),std::plus<lng>{});}}}
    cout << "PASS sqrt generic exhaustive n<=" << limit << '\n';}
struct Join {
    bool backwards;
    string operator()(const string &a, const string &b) const { return backwards ? b+a : a+b; }
};
void noncommutative() {
    input.clear(); history.clear();
    for (int n = 0; n <= 11; ++n) { for (int b : widths(n)) { for (bool reverse_order : {false,true}) {
        context = "strings n="+show(n)+" B="+show(b)+" reverse="+show(reverse_order);
        vector<string> a(n);
        for (int i = 0; i < n; ++i) { a[i] = string(1,char('a'+i)); }
        Join op{reverse_order}; SqrtDecomp<string,Join> d(a,b,"",op); verifyGeneric(d,a,string{},op);
        for (int i = 0; i < n; ++i) {
            d.opeUpdate(i,"Z"); a[i] = op(a[i],"Z"); verifyGeneric(d,a,string{},op);
            d.setUpdate(i,"X"); a[i] = "X"; verifyGeneric(d,a,string{},op);}
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
        copy = d; copy.rebuild(vector<lng>{3,5,1,7},3); verifyLazy(copy,vector<lng>{3,5,1,7});
        copy.rebuild({}); verifyLazy(copy,vector<lng>{});}
    cout << "PASS sqrt random vector oracle/copy/move/rebuild trials=" << trials << '\n';}
void regressions() {
    input.clear(); history.clear(); context = "lazy order/alias/wide/modular regressions";
    for (int b : widths(5)) {
        SqrtRangeSum<lng> d(vector<lng>{1,2,3,4,5},b);
        d.assign(0,5,3); d.multiply(0,5,2); d.add(1,4,1); verifyLazy(d,vector<lng>{6,7,7,7,6});
        d.affine(0,5,-1,2); d.set(1,100); verifyLazy(d,vector<lng>{-4,100,-5,-5,-4});
        d.rebuild(d.values(),b); verifyLazy(d,vector<lng>{-4,100,-5,-5,-4});
        d.assign(0,5,d.get(1)); verifyLazy(d,vector<lng>(5,100));}
    SqrtRangeSum<lng> zero(7,3); verifyLazy(zero,vector<lng>(7));
    zero.affine(0,7,2,3); zero.affine(0,7,zero.lazy_mul[0],zero.lazy_add[0]); verifyLazy(zero,vector<lng>(7,9));
    SqrtDecomp<lng> generic(vector<lng>{1,2,3},2); generic.set(0,generic.get(1));
    generic.opeUpdate(2,generic.get(0)); verifyGeneric(generic,vector<lng>{2,2,5},lng(0),std::plus<lng>{});
    lll big = lll(1) << 95; SqrtRangeSum<lll> wide(vector<lll>{big,2*big,-big},2);
    wide.affine(0,3,lll(2),big); wide.set(1,-big); verifyLazy(wide,vector<lll>{3*big,-big,-big});
    SqrtRangeSum<uint> modular(vector<uint>{UINT_MAX,2,3},2);
    modular.affine(0,3,UINT_MAX,uint(1)); verifyLazy(modular,vector<uint>{2,UINT_MAX,UINT_MAX-1});
    modular.add(1,3,uint(5)); verifyLazy(modular,vector<uint>{2,4,3});
    SqrtRangeSum<double> dyadic(vector<double>{0.5,1.25,0.25},2);
    dyadic.affine(0,3,2.0,0.5); verifyLazy(dyadic,vector<double>{1.5,3.0,1.0});
    SqrtDecomp<lng> empty; verifyGeneric(empty,vector<lng>{},lng(0),std::plus<lng>{});
    SqrtRangeSum<lng> empty_sum; verifyLazy(empty_sum,vector<lng>{});
    SqrtDecomp<lng> single({5}); SqrtRangeSum<lng> single_sum({5}), triple{1, 2, 3};
    verifyGeneric(single,vector<lng>{5},lng(0),std::plus<lng>{}); verifyLazy(single_sum,vector<lng>{5}); verifyLazy(triple,vector<lng>{1,2,3});
    static_assert(!std::is_convertible_v<int, SqrtDecomp<lng>> && !std::is_convertible_v<vector<lng>, SqrtDecomp<lng>>);
    static_assert(!std::is_convertible_v<int, SqrtRangeSum<lng>> && !std::is_convertible_v<vector<lng>, SqrtRangeSum<lng>>);
    auto any = [](bool x, bool y) { return x || y; };
    for (int b : {0, 1, 2, 3}) {
        vector<bool> bits{false, true, false, false, true};
        SqrtDecomp<bool, decltype(any)> flags(bits, b, false, any);
        for (int i = 0; i < 5; ++i) { const bool &got = flags.get(i); check(got, bool(bits[i]), "bool get"); }
        for (int l = 0; l <= 5; ++l) {
            for (int r = l; r <= 5; ++r) {
                bool want = false;
                for (int i = l; i < r; ++i) { want = want || bits[i]; }
                check(flags.query(l,r), want, "bool query");}}
        flags.set(4,false); flags.opeUpdate(2,true); check(flags.values() == vector<bool>{false,true,true,false,false}, true, "bool values");}
    cout << "PASS sqrt action order/empty/type/alias/block boundary regressions\n";}
int main(int argc, char **argv) {
    string invalid;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i]; if (i+1 == argc) { return 2; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else { return 2; }}
    if (!invalid.empty()) {
        SqrtDecomp<lng> g(2,1); SqrtRangeSum<lng> d(2,1);
        if (invalid == "generic-negative-size") { SqrtDecomp<lng> bad(-1); }
        else if (invalid == "generic-negative-block") { SqrtDecomp<lng> bad(2,-1); }
        else if (invalid == "generic-get-negative") { g.get(-1); }
        else if (invalid == "generic-get-end") { g.get(2); }
        else if (invalid == "generic-set-end") { g.set(2,1); }
        else if (invalid == "generic-setupdate-end") { g.setUpdate(2,1); }
        else if (invalid == "generic-ope-end") { g.opeUpdate(2,1); }
        else if (invalid == "generic-query-negative") { g.query(-1,1); }
        else if (invalid == "generic-query-reversed") { g.query(1,0); }
        else if (invalid == "generic-query-end") { g.query(0,3); }
        else if (invalid == "generic-rebuild-block") { g.rebuild({},-1); }
        else if (invalid == "lazy-negative-size") { SqrtRangeSum<lng> bad(-1); }
        else if (invalid == "lazy-negative-block") { SqrtRangeSum<lng> bad(2,-1); }
        else if (invalid == "lazy-get-end") { d.get(2); }
        else if (invalid == "lazy-set-end") { d.set(2,1); }
        else if (invalid == "lazy-sum-negative") { d.sum(-1,1); }
        else if (invalid == "lazy-sum-reversed") { d.sum(1,0); }
        else if (invalid == "lazy-sum-end") { d.sum(0,3); }
        else if (invalid == "lazy-affine-negative") { d.affine(-1,1,2,3); }
        else if (invalid == "lazy-affine-reversed") { d.affine(1,0,2,3); }
        else if (invalid == "lazy-affine-end") { d.affine(0,3,2,3); }
        else if (invalid == "lazy-rebuild-block") { d.rebuild({},-1); }
        else { return 2; }
        cerr << "precondition did not assert\n"; return 1;}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    cout << "sqrt seed=" << seed << " mode=" << mode << '\n';
    genericExhaustive(); noncommutative(); lazyExhaustive(); randomCases(); regressions();}
