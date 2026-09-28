#include "../1-Core/06-infintmini.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";
string context;

[[noreturn]] void fail(const string &expr, int line) {
    cerr << RED << "FAIL: 1-Core_06-infintmini_tester.cpp:" << line << ": " << expr;
    if (!context.empty()) { cerr << " [" << context << ']'; }
    cerr << RESET << '\n'; std::abort();}
#define REQUIRE(x) do { if (!(x)) { fail(#x, __LINE__); } } while (0)

template<class F>
void runPhase(int id, int total, const string &name, F &&f) {
    cerr << "    [self-test " << id << '/' << total << "] " << name << " ... " << std::flush;
    auto start = std::chrono::steady_clock::now(); f();
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    cerr << GREEN << "PASS" << RESET << " (" << std::fixed << std::setprecision(2) << elapsed << "s)\n";}

struct ProtocolProgress {
    int total, last = -1, next = 0;
    bool tty;
    ProtocolProgress(int total) : total(total) {
        const char *mode = std::getenv("INFINT_TEST_PROGRESS");
        tty = mode && string(mode) == "tty"; update(0);}
    void update(int done) {
        int pct = total ? int(100LL * done / total) : 100;
        if (tty) {
            if (pct == last) { return; }
            last = pct; int fill = 32 * pct / 100;
            cerr << '\r' << "    [mini protocol] [" << string(fill, '#')
                 << string(32 - fill, '-') << "] " << std::setw(3) << pct << "% ("
                 << done << '/' << total << ')' << std::flush;
            if (done == total) { cerr << '\n'; }}
        else if (pct >= next || done == total) {
            cerr << "    [mini protocol] " << std::setw(3) << pct << "% ("
                 << done << '/' << total << ")\n";
            next = std::min(100, 10 * (pct / 10 + 1));}}
};

std::mt19937_64 rng(0x6A09E667F3BCC909ULL);

void requireCanonical(const iint &a) {
    REQUIRE(a.sgn == 1 || a.sgn == -1);
    if (a.is_inf) { REQUIRE(a.n.empty()); return; }
    REQUIRE(!a.n.empty());
    for (uint x : a.n) { REQUIRE(x < iint::BASE); }
    REQUIRE(a.n.size() == 1 || a.n.back() != 0);
    REQUIRE(!a.isNil() || a.sgn == 1);}

iint randomIint(int len, bool neg = false) {
    iint a; a.n.resize(max(len, 1));
    for (uint &x : a.n) { x = rng() % iint::BASE; }
    a.n.back() = 1 + rng() % (iint::BASE - 1); a.sgn = neg ? -1 : 1;
    return a;}

vector<uint> schoolMul(const vector<uint> &a, const vector<uint> &b) {
    vector<uint> res(a.size() + b.size(), 0);
    for (int i = 0; i < a.size(); i++) {
        ulng carry = 0;
        for (int j = 0; j < b.size(); j++) {
            ulng cur = res[i + j] + ulng(a[i]) * b[j] + carry;
            res[i + j] = uint(cur % iint::BASE); carry = cur / iint::BASE;}
        res[i + b.size()] = uint(carry);}
    while (res.size() > 1 && res.back() == 0) { res.pop_back(); }
    return res;}

void verifyDiv(const iint &a, const iint &b) {
    auto [q, r] = divMod(a, b);
    REQUIRE(q * b + r == a); REQUIRE(r.isNil() || abs(r) < abs(b));
    REQUIRE(r.isNil() || r.sgn == a.sgn);
    requireCanonical(q); requireCanonical(r);}

void selfTest() {
    constexpr int PHASES = 8;
    runPhase(1, PHASES, "construction, bases, and I/O", [] {
    context = "construction and base I/O";
    REQUIRE(iint().toString() == "0"); REQUIRE(iint("--00000123") == 123);
    REQUIRE(iint("").isNil() && iint("+").isNil() && iint("-").isNil() && iint("+-+-").isNil());
    REQUIRE(iint("+00000123") == 123 && iint("-+-123") == 123);
    REQUIRE(iint("-0").sgn == 1); REQUIRE(lng(iint(LLONG_MIN)) == LLONG_MIN);
    REQUIRE(lng(iint(LLONG_MAX)) == LLONG_MAX);
    REQUIRE(ulng(iint(ULLONG_MAX)) == ULLONG_MAX && ulng(iint(-1)) == ULLONG_MAX);
    REQUIRE(ulng(iint("inf")) == ULLONG_MAX && ulng(iint("-inf")) == 0);
    REQUIRE(iint("INF") == iint("inf") && iint("-INF") == iint("-inf"));
    REQUIRE(iint("inf").toString() == "inf" && iint("-inf").toString() == "-inf");
    lll lll_max = lll((ulll(1) << 127) - 1), lll_min = -lll_max - 1;
    ulll ulll_max = ~ulll(0);
    REQUIRE(iint(lll_max).toString() == "170141183460469231731687303715884105727");
    REQUIRE(iint(lll_min).toString() == "-170141183460469231731687303715884105728");
    REQUIRE(iint(ulll_max).toString() == "340282366920938463463374607431768211455");
    iint assigned_wide; assigned_wide = lll_min;
    REQUIRE(assigned_wide.toString() == "-170141183460469231731687303715884105728");
    assigned_wide = ulll_max;
    REQUIRE(assigned_wide.toString() == "340282366920938463463374607431768211455");
    REQUIRE(iint("ABCDEF", 16).toString(16) == "ABCDEF");
    REQUIRE(iint("-101010", 2) == -42); REQUIRE(iint("ZZZZ", 36).toString(36) == "ZZZZ");
    REQUIRE(iint("-abcdef", 16) == iint("-ABCDEF", 16));
    iint assigned; assigned = std::string_view("-12345678901234567890");
    REQUIRE(assigned == iint("-12345678901234567890"));
    REQUIRE(ulng(iint("18446744073709551616")) == 0);
    REQUIRE(ulng(iint("18446744073709551617")) == 1);
    for (int tc = 0; tc < 100; tc++) {
        iint a = randomIint(1 + rng() % 80, rng() & 1);
        for (uint base = 2; base <= 36; base++) {
            context = "base roundtrip tc=" + std::to_string(tc) + ", base=" + std::to_string(base);
            REQUIRE(iint(a.toString(base), base) == a); }}
    stringstream out; out << iint::SetBase(16) << iint(255); REQUIRE(out.str() == "FF");
    stringstream in("-FF"); iint stream_a; in >> iint::SetBase(16) >> stream_a; REQUIRE(stream_a == -255);
    stringstream custom_out;
    custom_out << iint::SetBase(2) << iint(42) << ' ' << iint::SetBase(36) << iint(1295);
    REQUIRE(custom_out.str() == "101010 ZZ");
    stringstream custom_in("101010 ZZ"); iint stream_b;
    custom_in >> iint::SetBase(2) >> stream_a >> iint::SetBase(36) >> stream_b;
    REQUIRE(stream_a == 42 && stream_b == 1295);
    stringstream std_out; std_out << std::hex << iint(255) << ' ' << std::oct << iint(511);
    REQUIRE(std_out.str() == "FF 777");
    stringstream std_in("FF 777"); std_in >> std::hex >> stream_a; REQUIRE(stream_a == 255);
    std_in >> std::oct >> stream_a; REQUIRE(stream_a == 511);
    stringstream default_in("+00123 -0 INF -inf");
    iint in_a, in_b, in_c, in_d;
    default_in >> in_a >> in_b >> in_c >> in_d;
    REQUIRE(in_a == 123 && in_b == 0 && in_c == iint("inf") && in_d == iint("-inf"));
    stringstream special_out;
    special_out << iint(0) << ' ' << iint("inf") << ' ' << iint("-inf");
    REQUIRE(special_out.str() == "0 inf -inf");
    stringstream failed_in; iint unchanged = 77;
    failed_in >> unchanged; REQUIRE(failed_in.fail() && unchanged == 77);
    iint empty_zero; empty_zero.n.clear(); iint canonical_zero = 0;
    REQUIRE(empty_zero.isNil() && !empty_zero && empty_zero == canonical_zero);
    REQUIRE(magCmp(empty_zero, canonical_zero) == std::strong_ordering::equal);
    REQUIRE((empty_zero <=> canonical_zero) == std::strong_ordering::equal);
    });

    runPhase(2, PHASES, "representation, unary operations, and magnitude helpers", [] {
    context = "trim empty zero";
    iint empty; empty.sgn = -1; empty.n.clear(); empty.trim();
    REQUIRE(empty == 0); requireCanonical(empty);
    context = "trim redundant zero limbs";
    iint all_zero; all_zero.sgn = -1; all_zero.n = {0, 0, 0}; all_zero.trim();
    REQUIRE(all_zero == 0); requireCanonical(all_zero);
    iint padded; padded.sgn = -1; padded.n = {7, 0, 0}; padded.trim();
    REQUIRE(padded == -7); requireCanonical(padded);
    context = "trim infinity payload";
    iint stale_inf; stale_inf.sgn = -1; stale_inf.is_inf = true; stale_inf.n = {1, 2, 3};
    stale_inf.trim(); REQUIRE(stale_inf == iint("-inf")); requireCanonical(stale_inf);

    context = "unary and boolean operations";
    const iint value = -123;
    REQUIRE(+value == -123 && -value == 123);
    REQUIRE(-iint(123) == -123 && -iint(-123) == 123);
    REQUIRE(static_cast<bool>(iint(1)) && !static_cast<bool>(iint(0)));

    context = "addMagSmall carry chain";
    iint a; a.n = {iint::BASE - 1, iint::BASE - 1}; a.addMagSmall(1);
    REQUIRE(a.n == vector<uint>({0, 0, 1})); requireCanonical(a);
    context = "addMag resize and carry";
    a = 0; a.n = {iint::BASE - 1};
    a.addMag(vector<uint>{1, iint::BASE - 1});
    REQUIRE(a.n == vector<uint>({0, 0, 1})); requireCanonical(a);
    context = "subMag borrow chain";
    a = 0; a.n = {0, 0, 2}; a.subMag(vector<uint>{1});
    REQUIRE(a.n == vector<uint>({iint::BASE - 1, iint::BASE - 1, 1})); requireCanonical(a);
    context = "rsubMag borrow chain";
    a = 0; a.n = {1, 1}; a.rsubMag(vector<uint>{0, 0, 1});
    REQUIRE(a.n == vector<uint>({iint::BASE - 1, iint::BASE - 2})); requireCanonical(a);
    context = "mulMagSmall carry chain";
    a = 0; a.n = {iint::BASE - 1, iint::BASE - 1}; a.mulMagSmall(2);
    REQUIRE(a.n == vector<uint>({iint::BASE - 2, iint::BASE - 1, 1})); requireCanonical(a);
    context = "divMagSmall quotient and remainder";
    a = iint("123456789"); uint rem = a.divMagSmall(iint::BASE);
    REQUIRE(a == 1234 && rem == 56789); requireCanonical(a);
    });

    runPhase(3, PHASES, "small and scalar arithmetic", [] {
    context = "small arithmetic";
    for (lng x = -100; x <= 100; x++) {
        for (lng y = -100; y <= 100; y++) {
            context = "small arithmetic x=" + std::to_string(x) + ", y=" + std::to_string(y);
            iint a = x, b = y;
            REQUIRE(a + b == x + y); REQUIRE(a - b == x - y); REQUIRE(a * b == x * y);
            REQUIRE((a < b) == (x < y)); REQUIRE((a == b) == (x == y));
            if (y) {
                auto [q, r] = divMod(a, b); REQUIRE(q == x / y && r == x % y); verifyDiv(a, b);}}}
    for (lng b : {LLONG_MIN, -4'294'967'297LL, -100'001LL, -1LL, 1LL, 100'001LL,
                  4'294'967'297LL, LLONG_MAX}) {
        iint a = randomIint(200, rng() & 1), bb = b;
        REQUIRE(a * b == a * bb); REQUIRE(b * a == bb * a);
        REQUIRE(a / b == divMod(a, bb).first); REQUIRE(a % b == divMod(a, bb).second);
        iint cur = a; cur *= b; REQUIRE(cur == a * bb);
        cur = a; cur /= b; REQUIRE(cur == divMod(a, bb).first);
        cur = a; cur %= b; REQUIRE(cur == divMod(a, bb).second);}
    for (int len : {199, 200, 201}) {
        iint a = randomIint(len, rng() & 1);
        for (ulng b : {100'001ULL, ULLONG_MAX / iint::BASE, ULLONG_MAX / iint::BASE + 1,
                       1ULL << 63, ULLONG_MAX}) {
            context = "unsigned scalar len=" + std::to_string(len) + ", b=" + std::to_string(b);
            iint bb = b; REQUIRE(a * b == a * bb); REQUIRE(a / b == divMod(a, bb).first);
            REQUIRE(a % b == divMod(a, bb).second);}}
    context = "independent unsigned scalar multiplication";
    iint scalar_a = randomIint(80, true), scalar_b = ULLONG_MAX, scalar_res = scalar_a;
    vector<uint> scalar_exp = schoolMul(scalar_a.n, scalar_b.n);
    scalar_res *= ULLONG_MAX;
    REQUIRE(scalar_res.sgn == -1 && scalar_res.n == scalar_exp); requireCanonical(scalar_res);
    iint scalar_zero = -1234567; scalar_zero *= 0;
    REQUIRE(scalar_zero == 0); requireCanonical(scalar_zero);
    scalar_zero /= -7; REQUIRE(scalar_zero == 0);
    scalar_zero %= -7; REQUIRE(scalar_zero == 0); requireCanonical(scalar_zero);
    iint alias = iint("-123456789012345678901234567890"), original = alias;
    alias += alias; REQUIRE(alias == original * 2);
    alias = original; alias -= alias; REQUIRE(alias.isNil()); requireCanonical(alias);
    });

    runPhase(4, PHASES, "Goldilocks reduction and NTT", [] {
    context = "Goldilocks reduction";
    for (ulll x : vector<ulll>{0, iint::NTT_P - 1, iint::NTT_P, iint::NTT_P + 1,
                               ULLONG_MAX, ulll(iint::NTT_P - 1) * (iint::NTT_P - 1), ~ulll(0)}) {
        REQUIRE(iint::modP(x) == ulng(x % iint::NTT_P));}
    for (int tc = 0; tc < 20'000; tc++) {
        ulll x = (ulll(rng()) << 64) | rng(); REQUIRE(iint::modP(x) == ulng(x % iint::NTT_P));}
    context = "nttPow edge cases";
    REQUIRE(iint::nttPow(0, 0) == 1 && iint::nttPow(iint::NTT_G, 0) == 1);
    REQUIRE(iint::nttPow(0, 7) == 0 && iint::nttPow(2, 10) == 1024);
    REQUIRE(iint::nttPow(iint::NTT_P + 1, 17) == 1);
    REQUIRE(iint::nttPow(iint::NTT_G, iint::NTT_P - 1) == 1);
    for (int len = 1; len <= 4096; len <<= 1) {
        context = "NTT inverse length=" + std::to_string(len);
        vector<ulng> a(len); for (ulng &x : a) { x = rng() % iint::NTT_P; }
        vector<ulng> b = a; iint::ntt(b, false); iint::ntt(b, true); REQUIRE(a == b);}
    });

    runPhase(5, PHASES, "naive and NTT multiplication", [] {
    for (auto [al, bl] : vector<pair<int, int>>{{1, 1}, {31, 33}, {63, 65}, {127, 129},
                                                 {255, 255}, {256, 256}, {300, 301}, {600, 600},
                                                 {64, 1200}, {200, 1200}}) {
        context = "multiply limbs=" + std::to_string(al) + "x" + std::to_string(bl);
        iint a = randomIint(al), b = randomIint(bl, true); vector<uint> exp = schoolMul(a.n, b.n);
        iint c = a * b; REQUIRE(c.sgn == -1 && c.n == exp); requireCanonical(c);
        if (al == bl) { exp = schoolMul(a.n, a.n); c = a; c *= c; REQUIRE(c.sgn == 1 && c.n == exp); }}
    context = "all-max-limb square";
    iint max_limb; max_limb.n.assign(300, iint::BASE - 1);
    REQUIRE((max_limb * max_limb).n == schoolMul(max_limb.n, max_limb.n));
    {
        constexpr int AL = 1000, BL = 1001;
        int ntt_len = int(std::bit_ceil(uint(AL + BL - 1)));
        REQUIRE(ulng(AL) * BL > 30ULL * ntt_len * std::countr_zero(uint(ntt_len)));
        context = "forced NTT product limbs=" + std::to_string(AL) + "x" + std::to_string(BL);
        iint a = randomIint(AL), b = randomIint(BL, true);
        vector<uint> exp = schoolMul(a.n, b.n);
        iint c = a; c *= b;
        REQUIRE(c.sgn == -1 && c.n == exp); requireCanonical(c);}
    {
        constexpr int LEN = 900;
        int ntt_len = int(std::bit_ceil(uint(2 * LEN - 1)));
        REQUIRE(ulng(LEN) * LEN > 20ULL * ntt_len * std::countr_zero(uint(ntt_len)));
        context = "forced NTT alias square limbs=" + std::to_string(LEN);
        iint a = randomIint(LEN, true);
        vector<uint> exp = schoolMul(a.n, a.n);
        a *= a;
        REQUIRE(a.sgn == 1 && a.n == exp); requireCanonical(a);}
    });

    runPhase(6, PHASES, "scalar and Knuth division", [] {
    context = "division invariants";
    {
        iint a("759028842393192494218867898051"), b("619397960225983");
        auto [q, r] = divMod(a, b);
        REQUIRE(q == iint("1225429999989451") && r == iint("175611291792718"));
        auto [slow_q, slow_r] = divModSlow(a, b); REQUIRE(slow_q == q && slow_r == r);
        std::tie(q, r) = divMod(iint("500005000000000"), iint("5000099999"));
        REQUIRE(q == 99999 && r == 199999);}
    for (int tc = 0; tc < 500; tc++) {
        iint a = randomIint(1 + rng() % 240, rng() & 1);
        iint b = randomIint(1 + rng() % 120, rng() & 1); verifyDiv(a, b);
        if (tc < 50) {
            auto [q, r] = divMod(a, b); REQUIRE(a / b == q && a % b == r);
            iint cur = a; cur /= b; REQUIRE(cur == q);
            cur = a; cur %= b; REQUIRE(cur == r);}}
    iint div_alias = randomIint(100, true), div_copy = div_alias;
    div_alias /= div_alias; REQUIRE(div_alias == 1);
    div_alias = div_copy; div_alias %= div_alias; REQUIRE(div_alias.isNil()); requireCanonical(div_alias);
    REQUIRE(div_copy / iint(-1) == -div_copy && (div_copy % iint(-1)).isNil());
    iint div_empty; div_empty.n.clear();
    auto [empty_q, empty_r] = divMod(div_empty, 1);
    requireCanonical(empty_q); requireCanonical(empty_r);
    });

    runPhase(7, PHASES, "two- and three-limb divisor fast paths", [] {
    auto testFastDivisor = [&](ulng d, int limbs) {
        iint divisor = d; REQUIRE(int(divisor.n.size()) == limbs);
        iint q_abs("123456789012345678901234567890"), rem_abs = d - 1;
        iint mag = q_abs * divisor + rem_abs;
        for (int a_sgn : {-1, 1}) {
            for (int b_sgn : {-1, 1}) {
                context = std::to_string(limbs) + "-limb divisor, dividend sign=" +
                          std::to_string(a_sgn) + ", divisor sign=" + std::to_string(b_sgn);
                iint a = mag, b = divisor, exp_q = q_abs, exp_r = rem_abs;
                if (a_sgn == -1) { a.sgn = -1; exp_r.sgn = -1; }
                if (b_sgn == -1) { b.sgn = -1; }
                if (a_sgn != b_sgn) { exp_q.sgn = -1; }
                iint got_q = a, got_r = a;
                got_q /= b; got_r %= b;
                REQUIRE(got_q == exp_q && got_r == exp_r);
                REQUIRE(a / b == exp_q && a % b == exp_r);
                REQUIRE(got_q * b + got_r == a);
                requireCanonical(got_q); requireCanonical(got_r);}}};
    ulng d2 = 99'999ULL * iint::BASE + 99'937;
    ulng d3 = (90'000ULL * iint::BASE + 12'345) * iint::BASE + 67'890;
    testFastDivisor(d2, 2); testFastDivisor(d3, 3);
    });

    runPhase(8, PHASES, "infinity semantics", [] {
    context = "infinity";
    iint pinf("inf"), ninf("-inf"), zero = 0;
    requireCanonical(pinf); requireCanonical(ninf);
    iint inf_helper = pinf; inf_helper.addMagSmall(7); REQUIRE(inf_helper == pinf); requireCanonical(inf_helper);
    REQUIRE(static_cast<bool>(pinf) && static_cast<bool>(ninf));
    REQUIRE(magCmp(pinf, 1) == std::strong_ordering::greater);
    REQUIRE(magCmp(1, ninf) == std::strong_ordering::less);
    REQUIRE(magCmp(pinf, ninf) == std::strong_ordering::equal);
    REQUIRE(pinf > iint("999999999999999999999999") && ninf < -999);
    REQUIRE(pinf == iint("inf") && ninf == iint("-inf") && pinf != ninf);
    REQUIRE((pinf <=> ninf) == std::strong_ordering::greater);
    REQUIRE(+pinf == pinf && -pinf == ninf && -iint("-inf") == pinf);
    REQUIRE(abs(pinf) == pinf && abs(ninf) == pinf);
    REQUIRE(pinf + pinf == pinf && ninf + ninf == ninf);
    REQUIRE(pinf + ninf == 0 && pinf - pinf == 0);
    REQUIRE(ninf + pinf == 0 && ninf - ninf == 0);
    REQUIRE(pinf - ninf == pinf && ninf - pinf == ninf);
    REQUIRE(pinf + 7 == pinf && ninf - 7 == ninf);
    REQUIRE(iint(7) + pinf == pinf && iint(7) + ninf == ninf);
    REQUIRE(iint(7) - pinf == ninf && iint(7) - ninf == pinf);
    REQUIRE(zero * pinf == 0 && pinf * zero == 0);
    REQUIRE(pinf * ninf == ninf && ninf * ninf == pinf);
    REQUIRE(iint(7) * pinf == pinf && iint(-7) * pinf == ninf);
    REQUIRE(pinf * -7 == ninf && ninf * -7 == pinf);
    auto [q, r] = divMod(pinf, -2); REQUIRE(q == ninf && r == 0);
    std::tie(q, r) = divMod(ninf, -2); REQUIRE(q == pinf && r == 0);
    std::tie(q, r) = divMod(pinf, pinf); REQUIRE(q == 1 && r == 0);
    std::tie(q, r) = divMod(pinf, ninf); REQUIRE(q == -1 && r == 0);
    std::tie(q, r) = divMod(ninf, pinf); REQUIRE(q == -1 && r == 0);
    std::tie(q, r) = divMod(ninf, ninf); REQUIRE(q == 1 && r == 0);
    std::tie(q, r) = divMod(-7, pinf); REQUIRE(q == 0 && r == -7);
    REQUIRE(pinf / -2 == ninf && pinf % -2 == 0);
    REQUIRE(ninf / -2 == pinf && ninf % -2 == 0);
    REQUIRE(pinf / pinf == 1 && pinf % pinf == 0);
    REQUIRE(pinf / ninf == -1 && pinf % ninf == 0);
    REQUIRE(ninf / pinf == -1 && ninf % pinf == 0);
    REQUIRE(ninf / ninf == 1 && ninf % ninf == 0);
    REQUIRE(iint(-7) / pinf == 0 && iint(-7) % pinf == -7);
    REQUIRE(iint(7) / ninf == 0 && iint(7) % ninf == 7);
    context.clear();
    });}

void runProtocolCase() {
    string op, sa, sb; cin >> op;
    if (op == "frombase") {
        uint base; cin >> base >> sa; cout << iint(sa, base) << '\n'; return;}
    cin >> sa; iint a(sa);
    if (op == "tobase") {
        uint base; cin >> base; cout << a.toString(base) << '\n'; return;}
    if (op == "alias") { a *= a; cout << a << '\n'; return; }
    cin >> sb; iint b(sb);
    if (op == "smul") { cout << a * std::stoll(sb) << '\n'; return; }
    if (op == "sdiv") { cout << a / std::stoll(sb) << '\n'; return; }
    if (op == "smod") { cout << a % std::stoll(sb) << '\n'; return; }
    if (op == "add") { cout << a + b; }
    else if (op == "sub") { cout << a - b; }
    else if (op == "mul") { cout << a * b; }
    else if (op == "div") { auto [q, r] = divMod(a, b); cout << q << ' ' << r; }
    else if (op == "cmp") { cout << (a > b) - (a < b); }
    else { fail("unknown command " + op, __LINE__); }
    cout << '\n';}

void runProtocol() {
    int total; if (!(cin >> total)) { return; }
    ProtocolProgress progress(total);
    for (int tc = 0; tc < total; tc++) {
        runProtocolCase(); progress.update(tc + 1);}}

int main(int argc, char **argv) {
    ios_base::sync_with_stdio(false); cin.tie(nullptr);
    if (argc > 1 && string(argv[1]) == "--protocol") { runProtocol(); return 0; }
    cerr << "    [self-test] RNG seed=0x6A09E667F3BCC909\n";
    selfTest();
    cout << GREEN << "PASS: 1-Core_06-infintmini_tester" << RESET << '\n';
    return 0;}
