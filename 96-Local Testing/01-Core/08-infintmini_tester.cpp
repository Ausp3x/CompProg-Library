// Oracles for 08-infintmini.hpp: a base-1e5 schoolbook, division by construction and identities; --protocol serves the Python oracle.
#include "../../01-Core/08-infintmini.hpp"

static ulng seed = 42;
static string mode = "full";
static string context;
static lng checks = 0;
static std::mt19937_64 rng(42);

int otherUnit(const InfIntMini &a);

[[noreturn]] static void fail(const string &message) {
    cerr << "FAIL seed=" << seed << " mode=" << mode << " context=" << context << '\n' << message << '\n';
    std::exit(1);}
template<typename A, typename B> static void expect(const A &actual, const B &expected, const string &what) {
    ++checks;
    if (!(actual == expected)) {
        std::ostringstream os; os << what << "\n expected=" << expected << "\n actual=" << actual;
        fail(os.str());}}
static void require(bool ok, const string &what) { ++checks; if (!ok) { fail(what); } }
static bool full() { return mode != "quick"; }
static int ri(int k) { return int(rng() % ulng(k)); }
static InfIntMini randLimbs(int len, bool neg = false) {
    InfIntMini r; r.n.resize(size_t(len));
    for (uint &x : r.n) { x = uint(rng() % 100000); }
    r.n.back() = max(r.n.back(), 1u + uint(rng() % 99999));
    r.sgn = neg ? -1 : 1;
    return r;}
// Base-1e5 schoolbook product with per-row carries, independent of the header's kernels.
static InfIntMini school(const InfIntMini &a, const InfIntMini &b) {
    vector<uint> r(a.n.size() + b.n.size(), 0);
    for (size_t i = 0; i < a.n.size(); i++) {
        ulng c = 0;
        for (size_t j = 0; j < b.n.size(); j++) { c += ulng(a.n[i]) * b.n[j] + r[i + j]; r[i + j] = uint(c % 100000); c /= 100000; }
        for (size_t j = i + b.n.size(); c; j++) { c += r[j]; r[j] = uint(c % 100000); c /= 100000; }}
    InfIntMini res; res.n = r; res.sgn = a.sgn * b.sgn; res.trim();
    return res;}
static InfIntMini schoolAdd(InfIntMini a, const InfIntMini &b) {
    a.n.resize(max(a.n.size(), b.n.size()) + 1, 0);
    uint c = 0;
    for (size_t i = 0; i < a.n.size(); i++) { c += a.n[i] + (i < b.n.size() ? b.n[i] : 0); a.n[i] = c % 100000; c /= 100000; }
    a.trim();
    return a;}

static void construction() {
    context = "construction";
    InfIntMini z;
    require(z.isNil() && !bool(z) && !z && !!InfIntMini(-3) && z.toString() == "0" && z.sgn == 1 && lng(z) == 0 && ulng(z) == 0, "default is zero; ! through explicit bool");
    expect(InfIntMini(true).toString(), string("1"), "bool true");
    expect(InfIntMini(lll(ulll(1) << 127)).toString(), string("-170141183460469231731687303715884105728"), "lll min");
    expect(InfIntMini(~ulll(0)).toString(), string("340282366920938463463374607431768211455"), "ulll max");
    expect(InfIntMini(INT8_MIN).toString(), string("-128"), "int8 min");
    InfIntMini t; t.n = {5, 0, 0}; t.sgn = -1; t.trim();
    require(t.n == vector<uint>{5} && t == InfIntMini(-5), "trim strips zero limbs");
    t.n = {0, 0}; t.sgn = -1; t.trim();
    require(t.n == vector<uint>{0} && t.sgn == 1 && t.isNil(), "trim resets negative zero");
    t.n.clear(); t.trim();
    require(t.n == vector<uint>{0}, "trim fills empty");
    InfIntMini inf("inf"), ninf("-INF");
    require(inf.is_inf && inf.sgn == 1 && ninf.is_inf && ninf.sgn == -1 && inf.n.empty(), "sentinel tokens");
    require(lng(inf) == LLONG_MAX && lng(ninf) == LLONG_MIN && ulng(inf) == ULLONG_MAX && ulng(ninf) == 0 && bool(inf) && !inf.isNil(), "sentinel casts");
    InfIntMini c = 777;
    for (string bad : {"", "+", "-", "--1", "1-", " 1", "1 ", "12x", "infinity", "0x10", "1.5"}) { require(!c.read(bad) && c == InfIntMini(777), "malformed keeps the value: '" + bad + "'"); }
    require(c.read("inf", 24) && !c.is_inf && c == InfIntMini(10935), "inf is a base-24 number");
    require(c.read("INF", 36) && !c.is_inf && c == InfIntMini(24171) && c.read("inf", 23) && c.is_inf, "INF is a base-36 number and the sentinel in base 23");
    require(c.read("-000", 10) && c.isNil() && c.sgn == 1, "negative zero parses as zero");
    c = InfIntMini("inf"); require(c.read("12") && !c.is_inf && c == InfIntMini(12), "read replaces the sentinel");
    c = 5; c = InfIntMini("inf"); c = -3; require(!c.is_inf && c == InfIntMini(-3), "assignment clears the sentinel");}

static void arithmetic() {
    context = "arithmetic";
    for (int t = 0; t < (full() ? 400 : 60); t++) {
        int la = 1 + ri(t % 5 == 0 ? 600 : 30), lb = 1 + ri(t % 7 == 0 ? 600 : 30);
        InfIntMini a = randLimbs(la, ri(2)), b = randLimbs(lb, ri(2)), x = a;
        x += x; expect(x, a + a, "aliased +=");
        x = a; x -= x; require(x.isNil() && x.sgn == 1, "aliased -=");
        x = a; x *= x; expect(x, school(a, a), "aliased *= squares");
        x = a; x /= x; expect(x, InfIntMini(1), "aliased /=");
        x = a; x %= x; require(x.isNil(), "aliased %=");
        expect(a * b, school(a, b), "product against schoolbook");
        expect(a + b - b, a, "add then subtract");
        InfIntMini q = randLimbs(1 + ri(40), ri(2)), m = abs(b), r = randLimbs(1 + ri(lb), false);
        while (magCmp(r, m) >= 0) { r.n.pop_back(); r.trim(); }
        InfIntMini n = schoolAdd(abs(school(q, m)), r);
        auto [qq, rr] = divMod(n, m);
        expect(qq, abs(q), "quotient by construction"); expect(rr, r, "remainder by construction");}
    // Adversarial long-division windows: divisor top limbs 1 and 99999, quotient digits 99999 and remainders b - 1.
    for (int m : {4, 5, 9, 33}) {
        for (uint top : {1u, 99999u}) {
            InfIntMini b = randLimbs(m); b.n.back() = top; b.n[size_t(m - 2)] = top == 1 ? 0 : 99999;
            InfIntMini q = randLimbs(8); for (uint &x : q.n) { x = 99999; }
            InfIntMini r = b - InfIntMini(1), n = school(q, b) + r;
            auto [qq, rr] = divMod(n, b);
            expect(qq, q, "adversarial quotient m=" + std::to_string(m)); expect(rr, r, "adversarial remainder m=" + std::to_string(m));}}
    // Product backends on both sides of the cost threshold and in the transform range.
    for (auto [la, lb] : vector<pair<int, int>>{{1, 1}, {1, 400}, {40, 40}, {382, 382}, {383, 383}, {400, 400}, {512, 512}, {2000, 7}, {5000, 300}, {5000, 900}, {300, 5000}}) {
        InfIntMini a = randLimbs(la, true), b = randLimbs(lb);
        expect(a * b, school(a, b), "backend product " + std::to_string(la) + "x" + std::to_string(lb));}
    if (full()) {
        InfIntMini a = randLimbs(1 << 15), b = randLimbs(1 << 14);
        expect(a * b, school(a, b), "large NTT product");
        InfIntMini m9; m9.n.assign(1 << 14, 99999);
        expect(m9 * m9, school(m9, m9), "all-99999 NTT square");}
    expect(otherUnit(InfIntMini(41)), 42, "second translation unit");}

static void streams() {
    context = "streams";
    InfIntMini a("-123456789012345678901234567890"), b;
    std::ostringstream os; os << a << ' ' << InfIntMini::SetBase{36} << a << ' ' << std::hex << a << ' ' << std::oct << a;
    expect(os.str(), "-123456789012345678901234567890 " + a.toString(36) + " " + a.toString(16) + " " + a.toString(8), "stream bases");
    std::istringstream is("-1Z  zz 777 12x 5");
    is >> InfIntMini::SetBase{36} >> b; expect(b, InfIntMini(-71), "SetBase read");
    is >> b; expect(b, InfIntMini(1295), "lowercase read");
    is >> InfIntMini::SetBase{10} >> std::oct >> b; expect(b, InfIntMini(511), "oct flag read");
    is >> std::dec >> b; require(is.fail() && b == InfIntMini(511), "malformed token sets failbit and keeps the value");
    is.clear(); is >> b; expect(b, InfIntMini(5), "after clear");
    is >> b; require(is.fail() && b == InfIntMini(5), "end of input keeps the value");
    require(InfIntMini::getBaseId() == InfIntMini::getBaseId(), "stable iword index");}

static int protocol() {
    std::ios::sync_with_stdio(false);
    string op;
    auto cmp = [](std::strong_ordering o) { return o < 0 ? -1 : o > 0 ? 1 : 0; };
    while (cin >> op) {
        if (op == "ar") {
            string x, y; cin >> x >> y;
            InfIntMini a(x), b(y), s = a, d = a, p = a;
            s += b; d -= b; p *= b;
            if (!(s == a + b && d == a - b && p == a * b)) { cout << "MISMATCH compound\n"; continue; }
            cout << s << ' ' << d << ' ' << p << ' ';
            if (b.isNil()) { cout << "- - "; }
            else {
                auto [q, r] = divMod(a, b);
                InfIntMini q2 = a, r2 = a; q2 /= b; r2 %= b;
                if (!(q == q2 && r == r2 && q == a / b && r == a % b)) { cout << "MISMATCH division\n"; continue; }
                cout << q << ' ' << r << ' ';}
            cout << cmp(a <=> b) << ' ' << (a == b) << ' ' << cmp(magCmp(a, b)) << ' ' << -a << ' ' << +a << ' ' << abs(a) << '\n';}
        else if (op == "rd") {
            string s; uint base; cin >> s >> base;
            if (s == "~") { s.clear(); }
            InfIntMini a = 777; bool ok = a.read(s, base);
            cout << ok << ' ' << a << ' ' << a.toString(base) << '\n';}
        else if (op == "tb") {
            string s; uint base; cin >> s >> base;
            InfIntMini a(s); string t = a.toString(base);
            cout << t << ' ' << InfIntMini(t, base) << '\n';}
        else if (op == "cs") {
            string s; cin >> s;
            InfIntMini a(s);
            cout << lng(a) << ' ' << ulng(a) << ' ' << bool(a) << ' ' << a.isNil() << '\n';}
        else if (op == "ct") {
            string k, s; cin >> k >> s;
            bool neg = s[0] == '-'; ulll u = 0;
            for (char c : s.substr(neg)) { u = 10 * u + ulll(c - '0'); }
            ulll v = neg ? ulll(0) - u : u;
            InfIntMini r, w = 9;
            auto put = [&](auto x) { r = InfIntMini(x); w = x; };
            if (k == "i8") { put(int8_t(v)); }
            else if (k == "u8") { put(uint8_t(v)); }
            else if (k == "i16") { put(int16_t(v)); }
            else if (k == "u16") { put(uint16_t(v)); }
            else if (k == "i32") { put(int32_t(v)); }
            else if (k == "u32") { put(uint32_t(v)); }
            else if (k == "i64") { put(lng(v)); }
            else if (k == "u64") { put(ulng(v)); }
            else if (k == "i128") { put(lll(v)); }
            else if (k == "u128") { put(v); }
            else { put(v != 0); }
            cout << r << ' ' << w << '\n';}
        else if (op == "pw") {
            string s; lng e; cin >> s >> e;
            cout << pow(InfIntMini(s), e) << '\n';}
        else if (op == "gd") {
            string x, y; cin >> x >> y;
            cout << gcd(InfIntMini(x), InfIntMini(y)) << '\n';}
        else if (op == "sc") {
            string k, x; lng v; cin >> k >> x >> v;
            InfIntMini a(x), p = a, q = a, r = a;
            auto run = [&](auto s) {
                p *= s; cout << p << ' ' << a * s << ' ';
                if (s == 0) { cout << "- - - -\n"; return; }
                q /= s; r %= s;
                cout << q << ' ' << a / s << ' ' << r << ' ' << a % s << '\n';};
            if (k == "i32") { run(int32_t(v)); }
            else if (k == "u32") { run(uint32_t(v)); }
            else if (k == "u64") { run(ulng(v)); }
            else { run(v); }}
        else if (op == "io") {
            string s, f; uint base; cin >> s >> base >> f;
            InfIntMini a(s), b = 1;
            std::ostringstream os;
            if (f == "hex") { os << std::hex << a; }
            else if (f == "oct") { os << std::oct << a; }
            else { os << InfIntMini::SetBase{base} << a; }
            std::istringstream is(os.str());
            if (f == "hex") { is >> std::hex >> b; }
            else if (f == "oct") { is >> std::oct >> b; }
            else { is >> InfIntMini::SetBase{base} >> b; }
            cout << os.str() << ' ' << b << '\n';}
        else { cout << "UNKNOWN " << op << '\n'; }}
    return 0;}

static int invalid(const string &name) {
    InfIntMini a = 5, z = 0;
    if (name == "div-zero") { a /= z; }
    else if (name == "mod-zero") { a %= z; }
    else if (name == "divmod-zero") { (void)divMod(a, z); }
    else if (name == "scalar-div-zero") { a = a / 0; }
    else if (name == "read-base") { (void)a.read("1", 1); }
    else if (name == "tostring-base") { (void)a.toString(37); }
    else if (name == "ctor-string") { InfIntMini b("12x"); (void)b; }
    else if (name == "pow-negative") { (void)pow(a, -1); }
    else { cerr << "unknown invalid case\n"; return 2; }
    cerr << "precondition was not caught\n";
    return 3;}

int main(int argc, char **argv) {
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--protocol") { return protocol(); }
        if (arg == "--invalid" && i + 1 < argc) { return invalid(argv[i + 1]); }
        if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
        else if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }}
    rng.seed(seed);
    construction(); arithmetic(); streams();
    cout << "self-tests passed: " << checks << " checks\n";
    return 0;}
