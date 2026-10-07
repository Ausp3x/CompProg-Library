#include "../../02-Data Structures/02-fenwick.hpp"

ulng seed = 20260927;
string mode = "quick", context;
vector<lng> input;
vector<pair<int,lng>> history;

string number(lll x) {
    if (!x) { return "0"; }
    bool neg = x < 0;
    ulll u = neg ? ulll(-(x+1))+1 : ulll(x);
    string s;
    while (u) { s.push_back(char('0'+u%10)); u /= 10; }
    if (neg) { s.push_back('-'); }
    reverse(s.begin(),s.end()); return s;}
[[noreturn]] void fail(const string &op, const string &expected, const string &actual) {
    cerr << "FAIL Fenwick seed=" << seed << " mode=" << mode << " case=" << context
         << " operation=" << op << " expected=" << expected << " actual=" << actual << " input=[";
    for (lng x : input) { cerr << x << ','; }
    cerr << "] updates=";
    for (auto [i,x] : history) { cerr << '(' << i << ',' << x << ')'; }
    cerr << '\n'; std::exit(1);}
template<typename A, typename B> void check(A actual, B expected, const string &op) {
    if (actual != expected) { fail(op,number(lll(expected)),number(lll(actual))); }}
void verify(const Fenwick<lng> &f, const vector<lng> &a) {
    int n = int(a.size()); check(f.n,n,"n");
    vector<lng> values = f.values(); check(int(values.size()),n,"values size");
    for (int i = 0; i < n; ++i) {
        check(f.get(i),a[i],"get("+std::to_string(i)+")"); check(values[i],a[i],"values["+std::to_string(i)+"]");}
    vector<lng> p(n+1);
    for (int i = 0; i < n; ++i) { p[i+1] = p[i]+a[i]; }
    for (int r = 0; r <= n; ++r) {
        check(f.prefixSum(r),p[r],"prefixSum("+std::to_string(r)+")");
        check(f.prefixSum1(r),p[r],"prefixSum1");
        for (int l = 0; l <= r; ++l) {
            string op = "sum("+std::to_string(l)+","+std::to_string(r)+")";
            check(f.sum(l,r),p[r]-p[l],op); check(f.sum1(l+1,r),p[r]-p[l],op+" one-based");}}}
int search(const vector<lng> &a, lng target) {
    if (target <= 0) { return 0; }
    lng sum = 0;
    for (int i = 0; i < int(a.size()); ++i) { sum += a[i]; if (sum >= target) { return i; }}
    return int(a.size());}
void verifySearch(const Fenwick<lng> &f, const vector<lng> &a, lng target) {
    int expected = search(a,target);
    string op = "lowerBound("+std::to_string(target)+")";
    check(f.lowerBound(target),expected,op);
    check(f.lowerBound1(target),expected == int(a.size()) ? 0 : expected+1,op+" one-based");
    int n = int(a.size()), upper = 0;
    for (lng sum = 0; upper < n && sum + a[upper] <= target; ++upper) { sum += a[upper]; }
    check(f.upperBound(target),target < 0 ? 0 : upper,"upperBound("+std::to_string(target)+")");}
void verifyPredicates(const Fenwick<lng> &f, const vector<lng> &a, lng cap) {
    int n = int(a.size());
    for (int kind = 0; kind < 3; ++kind) {
        auto pred = [cap,kind](lng s) { return kind == 0 ? s <= cap : kind == 1 ? s < cap : 0 <= s && s <= cap; };
        if (!pred(0)) { continue; }
        string tag = (kind == 0 ? " cap<=" : kind == 1 ? " cap<" : " 0<=s<=cap") + std::to_string(cap);
        for (int l = 0; l <= n; ++l) {
            int r = l; lng sum = 0;
            while (r < n && pred(sum + a[r])) { sum += a[r++]; }
            check(f.maxRight(l,pred),r,"maxRight("+std::to_string(l)+")"+tag);}
        for (int r = 0; r <= n; ++r) {
            int l = r; lng sum = 0;
            while (l > 0 && pred(sum + a[l-1])) { sum += a[--l]; }
            check(f.minLeft(r,pred),l,"minLeft("+std::to_string(r)+")"+tag);}}}
void exhaustive() {
    int limit = mode == "quick" ? 4 : 6;
    for (int n = 0, count = 1; n <= limit; ++n, count *= 3) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask;
            for (lng &x : a) { x = code%3-1; code /= 3; }
            context = "signed n="+std::to_string(n)+" ternary="+std::to_string(mask); input = a; history.clear();
            Fenwick<lng> f(a), g(n);
            for (int i = 0; i < n; ++i) { g.add(i,a[i]); }
            verify(f,a); verify(g,a);
            for (int i = 0; i < n; ++i) { for (lng delta : {-2,0,2}) {
                auto b = a; auto h = f; b[i] += delta; history = {{i,delta}};
                h.add(i,delta); verify(h,b); h = f; h.add1(i+1,delta); verify(h,b);
                b[i] = delta; h = f; h.set(i,delta); verify(h,b);}}}}
    cout << "PASS Fenwick exhaustive signed vectors n<=" << limit << '\n';
    limit = mode == "quick" ? 4 : 7;
    for (int n = 0, count = 1; n <= limit; ++n, count *= 4) {
        for (int mask = 0; mask < count; ++mask) {
            vector<lng> a(n); int code = mask; lng total = 0;
            for (lng &x : a) { x = code%4; total += x; code /= 4; }
            context = "frequencies n="+std::to_string(n)+" quaternary="+std::to_string(mask); input = a; history.clear();
            Fenwick<lng> f(a);
            for (lng x = -1; x <= total+1; ++x) { verifySearch(f,a,x); verifyPredicates(f,a,x); }}}
    cout << "PASS Fenwick exhaustive nonnegative frequency search n<=" << limit << '\n';}
void randomCases() {
    std::mt19937_64 rng(seed);
    int trials = mode == "quick" ? 35 : mode == "full" ? 150 : 600;
    for (int trial = 0; trial < trials; ++trial) {
        int n = int(rng()%129); vector<lng> a(n), freq(n);
        for (lng &x : a) { x = lng(rng()%2001)-1000; }
        for (lng &x : freq) { x = lng(rng()%11); }
        context = "random signed trial="+std::to_string(trial)+" n="+std::to_string(n); input = a; history.clear();
        Fenwick<lng> f(a); verify(f,a);
        for (int op = 0; op < 300 && n; ++op) {
            int i = int(rng()%n); lng delta = lng(rng()%2001)-1000;
            history.push_back({i,delta}); a[i] += delta;
            if (op&1) { f.add1(i+1,delta); } else { f.add(i,delta); }
            int l = int(rng()%(n+1)), r = int(rng()%(n+1)); if (l > r) { swap(l,r); }
            lng sum = accumulate(a.begin()+l,a.begin()+r,lng(0));
            check(f.sum(l,r),sum,"random sum("+std::to_string(l)+","+std::to_string(r)+")");}
        verify(f,a); auto copy = f; verify(copy,a); Fenwick<lng> moved(std::move(copy)); verify(moved,a);
        copy = Fenwick<lng>(n); verify(copy,vector<lng>(n)); copy = moved; verify(copy,a); verify(f,a);
        context = "random frequencies trial="+std::to_string(trial)+" n="+std::to_string(n); input = freq; history.clear();
        Fenwick<lng> g(freq);
        for (int op = 0; op < 300; ++op) {
            if (n) {
                int i = int(rng()%n); lng value = lng(rng()%11), delta = value-freq[i];
                history.push_back({i,delta}); freq[i] = value; g.add(i,delta);}
            lng total = accumulate(freq.begin(),freq.end(),lng(0));
            verifySearch(g,freq,lng(rng()%ulng(total+3))-1);
            verifySearch(g,freq,total+1);
            if (op%25 == 0) { verifyPredicates(g,freq,lng(rng()%ulng(total+3))-1); }}
        verify(g,freq);}
    cout << "PASS Fenwick random vector oracle/copy/move trials=" << trials << '\n';}
struct Counted {
    static inline int additions = 0;
    lng x;
    explicit Counted(lng value = 0) : x(value) {}
    Counted &operator+=(Counted other) { ++additions; x += other.x; return *this; }
    friend Counted operator-(Counted a, Counted b) { return Counted(a.x-b.x); }
};
void boundaries() {
    int max_power = mode == "quick" ? 10 : mode == "full" ? 16 : 18;
    history.clear(); input.clear();
    for (int k = 0; k <= max_power; ++k) { for (int offset : {-1,0,1}) {
        int n = (1 << k)+offset; vector<lng> a(n), p(n);
        for (int i = 0; i < n; ++i) { a[i] = i%7 == 0 ? 3 : 0; p[i] = a[i]+(i ? p[i-1] : 0); }
        context = "power boundary n="+std::to_string(n)+" a[i]=(i%7==0?3:0)"; Fenwick<lng> f(a);
        lng total = n ? p.back() : 0;
        for (int j = 0; j < 100; ++j) {
            lng target = (total+2)*j/99;
            int expected = target <= 0 ? 0 : int(lower_bound(p.begin(),p.end(),target)-p.begin());
            check(f.lowerBound(target),expected,"power boundary search target="+std::to_string(target));}
        check(f.prefixSum(n),total,"power boundary sum");}}
    context = "linear construction operation counter";
    vector<Counted> a(10000,Counted(1)); Counted::additions = 0; Fenwick<Counted> f(a);
    if (Counted::additions > int(a.size())) { fail("build additions","<=n",std::to_string(Counted::additions)); }
    check(f.sum(0,10000).x,10000,"unordered additive type");
    f.add(9999,Counted(-1)); check(f.sum(9999,10000).x,0,"generic update");
    cout << "PASS Fenwick power boundaries and linear build operations\n";}
void typeCases() {
    context = "type/alias regressions"; input.clear(); history.clear();
    Fenwick<int> small(vector<int>{INT_MIN,INT_MAX});
    check(small.sum(0,2),-1,"signed extrema cancellation");
    Fenwick<lng> wide(vector<lng>{0,LLONG_MAX,0});
    check(wide.lowerBound(LLONG_MAX),1,"LLONG_MAX frequency");
    check(wide.lowerBound(LLONG_MIN),0,"LLONG_MIN target");
    wide.add(1,-LLONG_MAX); check(wide.lowerBound(1),3,"frequency removal");
    lll big = lll(1) << 100;
    Fenwick<lll> huge(vector<lll>{big,0,big});
    check(huge.sum(0,3),2*big,"128-bit sum"); check(huge.lowerBound(big+1),2,"128-bit search");
    Fenwick<uint> modular(vector<uint>{UINT_MAX,1,5});
    check(modular.sum(0,3),uint(5),"unsigned modular sum");
    modular.add(1,UINT_MAX); check(modular.sum(1,3),uint(5),"unsigned modular update/range");
    Fenwick<double> dyadic(vector<double>{0.5,1.25,0.25});
    if (dyadic.sum(1,3) != 1.5 || dyadic.lowerBound(1.5) != 1) { fail("dyadic exact sums/search","1.5,1","wrong"); }
    Fenwick<lng> alias(vector<lng>{2,3,4,5});
    alias.add(0,alias.v[0]); verify(alias,vector<lng>{4,3,4,5});
    alias.add1(2,alias.v[1]); verify(alias,vector<lng>{4,10,4,5});
    Fenwick<lng> empty; check(empty.n,0,"default empty"); verify(empty,{}); verifySearch(empty,{},1); verifyPredicates(empty,{},0);
    vector<lng> skip(17); skip[0] = skip[1] = skip[16] = 1; Fenwick<lng> sf(skip);
    check(sf.maxRight(17,[](lng s) { return s == 0; }),17,"maxRight skips cells before l");
    verifyPredicates(sf,skip,0); verifyPredicates(sf,skip,1);
    vector<uint> wrap{3,0,5,1,0,2,7,1}; Fenwick<uint> uf(wrap);
    for (uint cap : {0u,1u,3u,6u,20u}) {
        auto pred = [cap](uint s) { return s <= cap; };
        for (int l = 0; l <= 8; ++l) {
            int r = l; uint sum = 0;
            while (r < 8 && pred(sum + wrap[r])) { sum += wrap[r++]; }
            check(uf.maxRight(l,pred),r,"wrapping uint maxRight("+std::to_string(l)+")");}
        for (int r = 0; r <= 8; ++r) {
            int l = r; uint sum = 0;
            while (l > 0 && pred(sum + wrap[l-1])) { sum += wrap[--l]; }
            check(uf.minLeft(r,pred),l,"wrapping uint minLeft("+std::to_string(r)+")");}}
    Fenwick<lng> single({7}), triple{1,2,3}, sized(lng(3));
    verify(single,vector<lng>{7}); verify(triple,vector<lng>{1,2,3}); verify(sized,vector<lng>(3));
    check(huge.upperBound(big),2,"128-bit upperBound"); check(wide.upperBound(LLONG_MAX),3,"LLONG_MAX upperBound");
    huge.set(1,big); check(huge.sum(0,3),3*big,"128-bit set");
    cout << "PASS Fenwick types/extrema/argument aliasing\n";}
int main(int argc, char **argv) {
    string invalid;
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i]; if (i+1 == argc) { return 2; }
        if (arg == "--seed") { seed = std::stoull(argv[++i]); }
        else if (arg == "--mode") { mode = argv[++i]; }
        else if (arg == "--invalid") { invalid = argv[++i]; }
        else { return 2; }}
    if (!invalid.empty()) {
        Fenwick<lng> f(2);
        if (invalid == "negative-size") { Fenwick<lng> bad(-1); }
        else if (invalid == "add-negative") { f.add(-1,1); }
        else if (invalid == "add-end") { f.add(2,1); }
        else if (invalid == "prefix-negative") { f.prefixSum(-1); }
        else if (invalid == "prefix-end") { f.prefixSum(3); }
        else if (invalid == "sum-negative") { f.sum(-1,1); }
        else if (invalid == "sum-reversed") { f.sum(1,0); }
        else if (invalid == "sum-end") { f.sum(0,3); }
        else if (invalid == "add1-zero") { f.add1(0,1); }
        else if (invalid == "add1-end") { f.add1(3,1); }
        else if (invalid == "sum1-zero") { f.sum1(0,1); }
        else if (invalid == "sum1-reversed") { f.sum1(2,0); }
        else if (invalid == "sum1-end") { f.sum1(1,3); }
        else if (invalid == "get-end") { f.get(2); }
        else if (invalid == "set-end") { f.set(2,1); }
        else if (invalid == "max-right-end") { f.maxRight(3,[](lng) { return true; }); }
        else if (invalid == "max-right-false") { f.maxRight(0,[](lng) { return false; }); }
        else if (invalid == "min-left-end") { f.minLeft(3,[](lng) { return true; }); }
        else if (invalid == "min-left-false") { f.minLeft(0,[](lng) { return false; }); }
        else { return 2; }
        cerr << "precondition did not assert\n"; return 1;}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    cout << "Fenwick seed=" << seed << " mode=" << mode << '\n';
    exhaustive(); randomCases(); boundaries(); typeCases();}
