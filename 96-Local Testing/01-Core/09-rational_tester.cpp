// Oracles for 09-rational.hpp: identities, a 128-bit cross-product sort and aliasing checks; --protocol serves the Python fractions oracle.
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/09-rational.hpp"
#include "../../01-Core/05-modint.hpp"
#include "../../01-Core/06-modintmini.hpp"

using Big = Rational<InfInt>;
using Wide = Rational<lll>;
static ulng seed = 42;
static string mode = "full";
static string context;
static lng checks = 0;
static std::mt19937_64 rng(42);

int otherUnit(const Frac &x);

[[noreturn]] static void fail(const string &message) {
    cerr << "FAIL seed=" << seed << " mode=" << mode << " context=" << context << '\n' << message << '\n';
    std::exit(1);}
template<typename A, typename B> static void expect(const A &actual, const B &expected, const string &what) {
    ++checks;
    if (!(actual == expected)) {
        std::ostringstream os; os << what << "\n expected=" << expected << "\n actual=" << actual;
        fail(os.str());}}
static void require(bool ok, const string &what) { ++checks; if (!ok) { fail(what); } }
template<typename T> static T parseT(const string &s) {
    if constexpr (Rational<T>::BOUNDED) {
        bool neg = s[0] == '-'; ulll u = 0;
        for (char c : s.substr(neg)) { u = 10 * u + ulll(c - '0'); }
        return T(neg ? ulll(0) - u : u);}
    else { return T(s); }}
template<typename R> static R parseR(const string &s) { R r; if (!r.read(s)) { fail("protocol input rejected: " + s); } return r; }
static string str(const auto &x) { return x.toString(); }

static void identities() {
    context = "identities";
    Frac z;
    require(z.num == 0 && z.den == 1 && z.sign() == 0 && z.isInteger(), "default is 0/1");
    Frac a(6, -4), b(-3, 2), c = 5;
    require(a == b && a.num == -3 && a.den == 2 && c.den == 1, "normalized construction");
    require(Frac(0, -7) == z && Frac(LLONG_MIN, LLONG_MIN) == Frac(1) && Frac(2, LLONG_MIN) == Frac(-1, lng(1) << 62), "zero and minimum normalization");
    require(a + 1 == Frac(-1, 2) && 1 - a == Frac(5, 2) && 2 * a == Frac(-3) && a / 3 == Frac(-1, 2) && a < 0 && 1 > a && a != 1, "mixed integer operands");
    Big g = 5; g += Big(1, 3);
    require(g == Big(InfInt(16), InfInt(3)) && g > 5 && str(g) == "16/3", "InfInt implicit integral constructor");
    Frac x = Frac(3, 7);
    x += x; expect(x, Frac(6, 7), "aliased +="); x -= x; expect(x, Frac(0), "aliased -=");
    x = Frac(3, 7); x *= x; expect(x, Frac(9, 49), "aliased *="); x /= x; expect(x, Frac(1), "aliased /=");
    x = Frac(-3, 7);
    require(tryAdd(x, x, x) && x == Frac(-6, 7) && tryMul(x, x, x) && x == Frac(36, 49) && tryDiv(x, x, x) && x == Frac(1) && trySub(x, x, x) && x == Frac(0), "aliased try forms");
    Frac big(LLONG_MAX), keep(5, 3);
    require(!tryAdd(big, Frac(1), keep) && keep == Frac(5, 3) && !tryMul(big, Frac(2), keep) && keep == Frac(5, 3), "failed try leaves the result unchanged");
    require(!trySub(Frac(LLONG_MIN), Frac(1), keep) && trySub(Frac(-1), Frac(LLONG_MAX), keep) && keep == Frac(LLONG_MIN), "subtraction bounds");
    require(tryDiv(Frac(LLONG_MIN), Frac(LLONG_MIN), keep) && keep == Frac(1) && !tryDiv(Frac(1), Frac(LLONG_MIN), keep), "division by the minimum");
    Frac m; m.num = 6; m.den = -4; m.normalize();
    require(m == Frac(-3, 2), "normalize after a direct edit");
    require(Frac::raw(-3, 2) == Frac(-3, 2) && Big::raw(InfInt(5), InfInt(7)) == Big(5) / Big(7), "raw");
    require(Wide(Frac(-5, 3)) == Wide(-5, 3) && Frac(Wide(lll(7), lll(9))) == Frac(7, 9) && Big(Frac(LLONG_MIN, 3)).num == InfInt(LLONG_MIN), "converting constructors");
    require(Frac(Big(InfInt(-8), InfInt(12))) == Frac(-2, 3) && Wide(Big(InfInt("-170141183460469231731687303715884105728"))).num == lll(ulll(1) << 127), "narrowing conversions");
    expect(pow(Frac(-2, 3), -3), Frac(-27, 8), "negative power"); expect(pow(Frac(5), 0), Frac(1), "zeroth power"); expect(pow(Frac(0), 0), Frac(1), "0^0");
    expect(pow(Frac(LLONG_MIN), 1), Frac(LLONG_MIN), "minimum to the first power"); expect(pow(Frac(-1), LLONG_MIN), Frac(1), "minimum exponent");
    expect(inv(Frac(-3, 5)), Frac(-5, 3), "inverse");
    expect(Frac(7, 2).toDecimal(0), string("4"), "round half away at k = 0"); expect(Frac(-1, 1000).toDecimal(2), string("0.00"), "zero has no sign");
    expect(Frac(-999, 1000).toDecimal(2), string("-1.00"), "carry into the integer part"); expect(Frac(5, 2).toDecimal(3), string("2.500"), "exact");
    expect(Frac(9999, 1000).toDecimal(2), string("10.00"), "carry into a new digit"); expect(Frac(-99995, 10000).toDecimal(3), string("-10.000"), "negative carry into a new digit");
    expect(Frac(19, 2).toDecimal(0), string("10"), "carry into a new digit at k = 0"); expect(Wide(lll(-1), lll(3)).toDecimal(0), string("0"), "-1/3 rounds to unsigned 0");
    expect(Big(InfInt(1), InfInt(3)).toDecimal(30), string("0.333333333333333333333333333333"), "InfInt decimal");
    expect(Frac(5, 2).toString(2), string("101/10"), "binary"); expect(Frac(-255).toString(16), string("-FF"), "hex integer");
    Frac rd = 9; require(rd.read("512e-20") && rd == Frac(1, 195312500000000000) && rd.read("0.00000095367431640625") && rd == Frac(1, lng(1) << 20), "base primes cancel before the denominator forms");
    require(!rd.read("1e-19") && rd == Frac(1, lng(1) << 20) && Rational<lll>().read("1e-19"), "a reduced denominator above MAX(lng) fails only in Frac");
    expect(Frac(5, 3).toMod<mint>(), mint(5) / mint(3), "toMod mint"); expect(Big(InfInt(-5), InfInt(3)).toMod<ModIntMini<1000000007>>().val(), (ModIntMini<1000000007>(-5) / ModIntMini<1000000007>(3)).val(), "toMod InfInt to a mini");
    std::istringstream is("-3/6 1.25e1 0.5/2 7"); Frac r = 9, s = 9, t = 9;
    is >> r >> s; require(r == Frac(-1, 2) && s == Frac(25, 2) && !is.fail(), "stream read");
    is >> t; require(is.fail() && t == Frac(9), "malformed token sets failbit and keeps the value");
    is.clear(); is >> t; require(t == Frac(7), "after clear");
    is >> t; require(is.fail() && t == Frac(7), "end of input keeps the value");
    std::ostringstream os; os << Frac(-1, 2) << ' ' << Wide(lll(1) << 100) << ' ' << Big(InfInt(2), InfInt(4));
    expect(os.str(), string("-1/2 1267650600228229401496703205376 1/2"), "stream write");
    require(std::hash<Frac>()(Frac(2, 4)) == std::hash<Frac>()(Frac(1, 2)) && std::hash<Big>()(Big(2) / Big(4)) == std::hash<Big>()(Big(1) / Big(2)), "hash of equal values");
    std::unordered_set<Frac> seen;
    for (int i = 1; i <= 50; i++) { seen.insert(Frac(i, 2 * i)); seen.insert(Frac(i, 1)); }
    expect(int(seen.size()), 51, "unordered_set with hash");
    // Sort against an independent 128-bit cross-product comparator.
    vector<Frac> v;
    for (int i = 0; i < (mode == "quick" ? 300 : 3000); i++) { v.push_back(Frac(lng(rng() >> int(rng() % 64)) * (rng() & 1 ? 1 : -1), lng(rng() >> int(1 + rng() % 63)) | 1)); }
    vector<Frac> w = v;
    sort(v.begin(), v.end());
    sort(w.begin(), w.end(), [](const Frac &p, const Frac &q) { return lll(p.num) * q.den < lll(q.num) * p.den; });
    require(v == w, "sort agrees with 128-bit cross products");
    expect(otherUnit(Frac(1, 3)), 4, "second translation unit");}

template<typename R> static void arithmeticLine(const string &x, const string &y) {
    R a = parseR<R>(x), b = parseR<R>(y), res, c = a;
    auto out = [&](auto attempt, auto op) {
        res = R(5, 7);
        if (!attempt()) { cout << (res == R(5, 7) ? "F " : "CHANGED "); return; }
        c = a; op(c);
        if (!(c == res)) { cout << "MISMATCH "; return; }
        cout << res << ' ';};
    out([&] { return tryAdd(a, b, res); }, [&](R &p) { p += b; });
    out([&] { return trySub(a, b, res); }, [&](R &p) { p -= b; });
    out([&] { return tryMul(a, b, res); }, [&](R &p) { p *= b; });
    if (b.num == 0) { cout << "Z "; }
    else { out([&] { return tryDiv(a, b, res); }, [&](R &p) { p /= b; }); }
    auto o = a <=> b;
    cout << (o < 0 ? -1 : o > 0 ? 1 : 0) << ' ' << (a == b) << '\n';}

template<typename R> static void unaryLine(const string &x) {
    using T = decltype(R().num);
    R a = parseR<R>(x);
    bool minimum = false;
    if constexpr (R::BOUNDED) { minimum = a.num == std::numeric_limits<T>::min(); }
    cout << (minimum ? "F" : str(-a)) << ' ' << (minimum ? "F" : str(abs(a))) << ' ' << (a.num == 0 ? "Z" : minimum ? "F" : str(inv(a))) << ' ';
    cout << a.sign() << ' ' << a.isInteger() << ' ' << R(floor(a)) << ' ' << R(ceil(a)) << ' ' << R(trunc(a)) << ' ' << R(round(a)) << ' ' << fracPart(a) << ' ';
    cout << (std::hash<R>()(a) == std::hash<R>()(R(a.num, a.den))) << ' ' << (+a == a) << '\n';}

template<typename R> static void floatLine(const string &x) {
    R a = parseR<R>(x); int e = 0;
    double d = a.toDouble(); long double ld = a.toLongDouble();
    if (std::isinf(d)) { cout << (d < 0 ? "-inf " : "inf "); }
    else { double v = std::frexp(d, &e); cout << lng(std::ldexp(v, 53)) << ' ' << e << ' '; }
    if (std::isinf(ld)) { cout << (ld < 0 ? "-inf " : "inf "); }
    else { long double w = std::frexp(ld, &e); cout << (w < 0 ? "-" : "") << ulng(std::ldexp(std::fabs(w), 64)) << ' ' << e << ' '; }
    cout << std::signbit(d) << '\n';}

template<typename R> static void modLine(const string &x) {
    using T = decltype(R().num);
    R a = parseR<R>(x);
    auto unit = [&](ulng p) { return !(a.den % T(p) == 0); };
    cout << (unit(998244353) ? std::to_string(a.template toMod<mint>().val()) : "-") << ' ';
    cout << (unit((1ULL << 61) - 1) ? std::to_string(a.template toMod<ModInt61>().val()) : "-") << ' ';
    cout << (unit(1000000007) ? std::to_string(a.template toMod<ModIntMini<1000000007>>().val()) : "-") << ' ';
    DynModInt64<7>::setMod(1000000000000ULL);
    bool coprime = !(a.den % T(2) == 0) && !(a.den % T(5) == 0);
    cout << (coprime ? std::to_string(a.template toMod<DynModInt64<7>>().val()) : "-") << '\n';}

template<typename R> static void dispatch(const string &op, std::istream &in) {
    using T = decltype(R().num);
    if (op == "ar") { string x, y; in >> x >> y; arithmeticLine<R>(x, y); }
    else if (op == "un") { string x; in >> x; unaryLine<R>(x); }
    else if (op == "pw") { string x; lng e; in >> x >> e; cout << pow(parseR<R>(x), e) << '\n'; }
    else if (op == "fl") { string x; in >> x; floatLine<R>(x); }
    else if (op == "fd") { string h; in >> h; cout << R::fromDouble(std::strtold(h.c_str(), nullptr)) << '\n'; }
    else if (op == "rd") {
        uint base; string s; in >> base >> s;
        if (s == "~") { s.clear(); }
        R a(7, 3); bool ok = a.read(s, base);
        cout << ok << ' ' << a << '\n';}
    else if (op == "ts") { uint base; string x; in >> base >> x; cout << parseR<R>(x).toString(base) << '\n'; }
    else if (op == "dc") { string x; int k; in >> x >> k; cout << parseR<R>(x).toDecimal(k) << '\n'; }
    else if (op == "md") { string x; in >> x; modLine<R>(x); }
    else if (op == "cv") {
        string u, x; in >> u >> x;
        R a = parseR<R>(x);
        if (u == "l") { cout << Frac(a) << '\n'; }
        else if (u == "L") { cout << Wide(a) << '\n'; }
        else { cout << Big(a) << '\n'; }}
    else if (op == "nm") { string n, d; in >> n >> d; cout << R(parseT<T>(n), parseT<T>(d)) << '\n'; }
    else if (op == "rw") { string n, d; in >> n >> d; cout << R::raw(parseT<T>(n), parseT<T>(d)) << '\n'; }
    else { cout << "UNKNOWN " << op << '\n'; }}

static int protocol() {
    std::ios::sync_with_stdio(false);
    string op, t;
    while (cin >> op >> t) {
        if (t == "l") { dispatch<Frac>(op, cin); }
        else if (t == "L") { dispatch<Wide>(op, cin); }
        else { dispatch<Big>(op, cin); }
        cout.flush();}
    return 0;}

static int invalid(const string &name) {
    Frac a(3, 2);
    if (name == "den-zero") { Frac b(1, 0); (void)b; }
    else if (name == "ctor-overflow") { Frac b(1, LLONG_MIN); (void)b; }
    else if (name == "raw-den") { (void)Frac::raw(1, 0); }
    else if (name == "inv-zero") { (void)inv(Frac(0)); }
    else if (name == "inv-overflow") { (void)inv(Frac(LLONG_MIN)); }
    else if (name == "pow-zero-negative") { (void)pow(Frac(0), -1); }
    else if (name == "pow-overflow") { (void)pow(Frac(3), 40); }
    else if (name == "div-zero") { a /= Frac(0); }
    else if (name == "add-overflow") { a = Frac(LLONG_MAX) + Frac(1); }
    else if (name == "mul-overflow") { a = Frac(1, LLONG_MAX) * Frac(1, 2); }
    else if (name == "neg-min") { a = -Frac(LLONG_MIN); }
    else if (name == "abs-min") { a = abs(Wide(lll(ulll(1) << 127))) == Wide(0) ? a : a; }
    else if (name == "fromdouble-nan") { (void)Frac::fromDouble(std::nan("")); }
    else if (name == "fromdouble-range") { (void)Frac::fromDouble(1e-30); }
    else if (name == "read-base") { (void)a.read("1", 37); }
    else if (name == "tostring-base") { (void)a.toString(1); }
    else if (name == "todecimal-negative") { (void)a.toDecimal(-1); }
    else if (name == "cast-narrow") { Frac b(Wide(lll(1) << 100)); (void)b; }
    else if (name == "infint-inf") { Big b(InfInt("inf")); (void)b; }
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
    identities();
    cout << "self-tests passed: " << checks << " checks\n";
    return 0;}
