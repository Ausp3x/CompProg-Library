#include "../1-Core/05-infint.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";
string context;

[[noreturn]] void fail(const string &expr, int line) {
    cerr << RED << "FAIL: 1-Core_05-infint_tester.cpp:" << line << ": " << expr;
    if (!context.empty()) { cerr << " [" << context << ']'; }
    cerr << RESET << '\n'; std::abort();
}
#define REQUIRE(x) do { if (!(x)) { fail(#x, __LINE__); } } while (0)

template<class F>
void runPhase(int id, int total, const string &name, F &&f) {
    cerr << "    [self-test " << id << '/' << total << "] " << name << " ... " << std::flush;
    auto start = std::chrono::steady_clock::now(); f();
    double elapsed = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    cerr << GREEN << "PASS" << RESET << " (" << std::fixed << std::setprecision(2) << elapsed << "s)\n";}

struct ProtocolProgress {
    int total, done = 0, last = -1, next = 0;
    bool tty;
    string label;
    ProtocolProgress(int total) : total(total) {
        const char *mode = std::getenv("INFINT_TEST_PROGRESS"), *variant = std::getenv("INFINT_TEST_VARIANT");
        tty = mode && string(mode) == "tty"; label = variant ? variant : "protocol"; update(0);}
    void update(int cur) {
        done = cur; int pct = total ? int(100LL * done / total) : 100;
        if (tty) {
            if (pct == last) { return; }
            last = pct; int fill = 32 * pct / 100;
            cerr << '\r' << "    [" << label << " protocol] [" << string(fill, '#')
                 << string(32 - fill, '-') << "] " << std::setw(3) << pct << "% ("
                 << done << '/' << total << ')' << std::flush;
            if (done == total) { cerr << '\n'; }}
        else if (pct >= next || done == total) {
            cerr << "    [" << label << " protocol] " << std::setw(3) << pct << "% ("
                 << done << '/' << total << ")\n";
            next = std::min(100, 10 * (pct / 10 + 1));}}
};

std::mt19937_64 rng(0x243F6A8885A308D3ULL);

iint randomIint(int limbs, bool negative = false) {
    if (limbs <= 0) { return 0; }
    iint a; a.sgn = negative ? -1 : 1; a.n.resize(limbs);
    for (uint &x : a.n) { x = uint(rng()); }
    a.n.back() |= 0x80000000U;
    return a;
}

void trimVec(vector<uint> &a) {
    while (a.size() > 1 && a.back() == 0) { a.pop_back(); }
    if (a.empty()) { a.push_back(0); }
}

vector<uint> schoolMul(const vector<uint> &a, const vector<uint> &b) {
    vector<uint> res(a.size() + b.size(), 0);
    for (int i = 0; i < a.size(); i++) {
        ulng carry = 0;
        for (int j = 0; j < b.size(); j++) {
            ulll cur = ulll(a[i]) * b[j] + res[i + j] + carry;
            res[i + j] = uint(cur); carry = ulng(cur >> 32);}
        for (int j = i + b.size(); carry; j++) {
            ulll cur = ulll(res[j]) + carry; res[j] = uint(cur); carry = ulng(cur >> 32);}}
    trimVec(res);
    return res;
}

void requireCanonical(const iint &a) {
    if (a.is_inf) { REQUIRE(a.n.empty()); return; }
    REQUIRE(!a.n.empty()); REQUIRE(a.n.size() == 1 || a.n.back() != 0);
    REQUIRE(!a.isNil() || a.sgn == 1);
}

void verifyDiv(const iint &a, const iint &b, const pair<iint, iint> &qr) {
    REQUIRE(!b.isNil());
    REQUIRE(qr.first * b + qr.second == a);
    REQUIRE(abs(qr.second) < abs(b));
    REQUIRE(qr.second.isNil() || qr.second.sgn == a.sgn);
    requireCanonical(qr.first); requireCanonical(qr.second);
}

void checkSchoolMul(int al, int bl, bool ones = false) {
    context = "multiply limbs=" + std::to_string(al) + 'x' + std::to_string(bl) + (ones ? " ones" : " random");
    iint a = randomIint(al), b = randomIint(bl, true);
    if (ones) { std::fill(a.n.begin(), a.n.end(), UINT_MAX); std::fill(b.n.begin(), b.n.end(), UINT_MAX); }
    vector<uint> expected = schoolMul(a.n, b.n);
    iint c = a * b; REQUIRE(c.sgn == -1 && c.n == expected); requireCanonical(c);
    c = a; c *= b; REQUIRE(c.sgn == -1 && c.n == expected);
    if (al == bl) {
        expected = schoolMul(a.n, a.n); c = a; c *= c; REQUIRE(c.sgn == 1 && c.n == expected);
        iint same = a; c = a; c *= same; REQUIRE(c.sgn == 1 && c.n == expected);
        same.sgn = -1; c = a; c *= same; REQUIRE(c.sgn == -1 && c.n == expected);}
}

void testConstructionBasesAndIO() {
    context = "integral boundaries";
    for (lng x : {LLONG_MIN, -4'294'967'297LL, -4'294'967'296LL, -1LL, 0LL, 1LL,
                  4'294'967'295LL, 4'294'967'296LL, LLONG_MAX}) {
        iint a = x; REQUIRE(lng(a) == x); requireCanonical(a); iint b; b = x; REQUIRE(b == a);}
    for (ulng x : {0ULL, 1ULL, 0xFFFFFFFFULL, 0x100000000ULL, ULLONG_MAX}) {
        iint a = x; REQUIRE(ulng(a) == x); requireCanonical(a);}
    ulll umax = ~ulll(0); iint u = umax;
    REQUIRE(u.n == vector<uint>({UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX}));
    lll smin = lll(ulll(1) << 127); iint si = smin;
    REQUIRE(si.sgn == -1 && si.n == vector<uint>({0, 0, 0, 0x80000000U}));
    for (uint base : {2U, 8U, 16U}) {
        iint reset("inf"); reset.init(vector<uint>{1}, base, -1);
        REQUIRE(reset == -1 && !reset.is_inf);}
    vector<uint> blocks = {123456789U}; iint reset("inf"); reset.initFromBlks(blocks, 10, -1);
    REQUIRE(reset == -123456789 && !reset.is_inf);
    for (int t = 0; t < 100; t++) {
        iint a = randomIint(1 + rng() % 600, rng() & 1);
        for (uint base : {2U, 3U, 7U, 8U, 10U, 16U, 17U, 36U, 255U, 65'521U, 1'000'000'007U, UINT_MAX}) {
            context = "roundtrip base=" + std::to_string(base) + " t=" + std::to_string(t);
            string s = a.toString(base); iint b(s, base); REQUIRE(a == b);}}
    string dec = "-9"; dec.reserve(20'001);
    for (int i = 2; i <= 20'000; i++) { dec += char('0' + rng() % 10); }
    context = "20k decimal roundtrip"; iint d(dec); REQUIRE(d.toString() == dec);
    string hs = "F"; hs.reserve(20'001);
    for (int i = 1; i < 20'000; i++) { hs += iint::DIG[rng() & 15]; }
    context = "20k hex roundtrip"; iint h(hs, 16); REQUIRE(h.toString(16) == hs);
    string lower = hs; for (char &c : lower) { c = std::tolower(static_cast<unsigned char>(c)); }
    REQUIRE(iint(lower, 16) == h);
    REQUIRE(bin(iint(0)) == "0" && oct(iint(0)) == "0" && hex(iint(0)) == "0");
    REQUIRE(bin(iint(10)) == "1010" && oct(iint(10)) == "12" && hex(iint(10)) == "A");
    REQUIRE(bin(iint(-10)) == "-1010" && oct(iint(-10)) == "-12" && hex(iint(-10)) == "-A");
    REQUIRE(bin(iint("inf")) == "inf" && oct(iint("-inf")) == "-inf" && hex(iint("inf")) == "inf");
    for (int limbs : {1, 3, 4, 5, 7, 8, 9, 31, 32, 33, 257}) for (bool neg : {false, true}) {
        context = "direct bin/oct/hex limbs=" + std::to_string(limbs) + ", neg=" + std::to_string(neg);
        iint v = randomIint(limbs, neg);
        string sb = bin(v), so = oct(v), sh = hex(v);
        REQUIRE(sb == v.toString(2) && so == v.toString(8) && sh == v.toString(16));
        REQUIRE(iint(sb, 2) == v && iint(so, 8) == v && iint(sh, 16) == v);}
    stringstream ss; iint a, b, c;
    ss << iint::SetBase(3) << iint(123456789) << ' ' << std::hex << iint(0xABCDEF) << ' ' << std::oct << iint(01234567);
    ss >> std::dec >> iint::SetBase(3) >> a >> std::hex >> b >> std::oct >> c;
    REQUIRE(a == 123456789 && b == 0xABCDEF && c == 01234567);
    stringstream bs("[12, 34, 0, 56]"); bs >> iint::SetBase(100) >> a;
    REQUIRE(a == iint(12) * 100 * 100 * 100 + iint(34) * 100 * 100 + 56);
    iint unchanged = 42; stringstream empty; empty >> unchanged; REQUIRE(unchanged == 42 && empty.eof());
    context.clear();
}

void testSmallArithmeticBitwiseAndShifts() {
    for (lng A = -120; A <= 120; A++) for (lng B = -120; B <= 120; B++) {
        context = "small A=" + std::to_string(A) + " B=" + std::to_string(B);
        iint a = A, b = B;
        REQUIRE(a + b == A + B); REQUIRE(a - b == A - B); REQUIRE(a * b == A * B);
        REQUIRE((a == b) == (A == B)); REQUIRE((a < b) == (A < B)); REQUIRE((a <= b) == (A <= B));
        REQUIRE((a > b) == (A > B)); REQUIRE((a >= b) == (A >= B));
        REQUIRE((a & b) == (A & B)); REQUIRE((a | b) == (A | B)); REQUIRE((a ^ b) == (A ^ B)); REQUIRE(~a == ~A);
        if (B != 0) {
            auto qr = divMod(a, b); REQUIRE(qr.first == A / B && qr.second == A % B); verifyDiv(a, b, qr);}}
    for (lng A = -1000; A <= 1000; A++) {
        context = "increment/shift A=" + std::to_string(A);
        iint a = A, old = a++; REQUIRE(old == A && a == A + 1); old = a--; REQUIRE(old == A + 1 && a == A);
        REQUIRE(++a == A + 1 && --a == A);
        for (int shf = 0; shf <= 70; shf++) {
            iint l = iint(A) << shf, r = iint(A) >> shf;
            if (shf < 50) { REQUIRE(l == iint(A) * (1LL << shf)); }
            REQUIRE(r == iint(A >> min(shf, 63)));}
        ulng mag = A < 0 ? ulng(-A) : ulng(A);
        REQUIRE(popcount(iint(A)) == __builtin_popcountll(mag));
        if (A != 0) { REQUIRE(ctz(iint(A)) == __builtin_ctzll(mag)); }}
    iint huge = randomIint(4096), z;
    z += huge; REQUIRE(z == huge); z = 0; z -= huge; REQUIRE(z == -huge);
    z = huge; z += z; REQUIRE(z == huge * 2);
    z = huge; z += -huge; REQUIRE(z.isNil());
    z = huge; z -= huge; REQUIRE(z.isNil());
    iint blk = -randomIint(4), blk0 = blk;
    blk.shiftBlocksLeft(3); REQUIRE(blk == (blk0 << 96));
    blk.shiftBlocksRight(3); REQUIRE(blk == blk0);
    blk.shiftBlocksRight(5); REQUIRE(blk.isNil());
    for (auto [a, b] : vector<pair<iint, iint>>{{randomIint(9), randomIint(3)},
                                                 {-randomIint(3), randomIint(9)},
                                                 {randomIint(9), -randomIint(3)}}) {
        iint got = a; got &= b; REQUIRE(got == (a & b));
        got = a; got |= b; REQUIRE(got == (a | b));
        got = a; got ^= b; REQUIRE(got == (a ^ b));
        got = a; got &= got; REQUIRE(got == a);
        got = a; got |= got; REQUIRE(got == a);
        got = a; got ^= got; REQUIRE(got.isNil());}
    iint emptyA, emptyB; REQUIRE((emptyA | emptyB).isNil()); requireCanonical(emptyA | emptyB);
    for (lng x : {-0x100000001LL, -1LL, 0LL, 1LL, 0x100000001LL}) {
        iint a = x; vector<uint> tc = a.convTwosComp();
        REQUIRE(iint::fromTwosComp(std::move(tc), x < 0 ? -1 : 0) == a);}
    REQUIRE(iint::fromTwosComp(vector<uint>{0}, -1) == -(iint(1) << 32));
    REQUIRE(iint::fromTwosComp(vector<uint>{}, -1) == -1);
    context.clear();
}

void testMultiplicationAndNTT() {
    REQUIRE(iint::nttPow(0, 0) == 1 && iint::nttPow(iint::NTT_P, 0) == 1);
    REQUIRE(iint::nttPow(0, 5) == 0 && iint::nttPow(1, ULLONG_MAX) == 1);
    REQUIRE(iint::nttPow(iint::NTT_P + 1, 123) == 1);
    REQUIRE(iint::nttPow(iint::NTT_P - 1, 63) == iint::NTT_P - 1);
    REQUIRE(iint::nttPow(iint::NTT_P - 1, 64) == 1);
    REQUIRE(iint::nttPow(2, 1) == 2 && iint::nttPow(2, 2) == 4 && iint::nttPow(2, 3) == 8);
    for (auto [a, b] : vector<pair<int, int>>{{1, 1}, {16, 16}, {31, 31}, {31, 33}, {32, 32}, {33, 33},
                                               {55, 55}, {56, 56}, {63, 511}, {64, 64}, {64, 255}, {64, 256},
                                               {65, 65}, {103, 103}, {104, 104}, {112, 112}, {127, 127},
                                               {127, 129}, {129, 129}, {187, 187}, {188, 188}, {192, 192},
                                               {224, 224}, {255, 255}, {256, 256}, {257, 257}, {320, 320},
                                               {334, 334}, {335, 335}, {384, 384}, {511, 513}, {590, 590},
                                               {591, 591}, {1023, 1023}, {1024, 1024},
                                               {64, 4096}, {128, 4096}, {512, 4096}}) { checkSchoolMul(a, b); }
    for (int len : {64, 257, 1024}) { checkSchoolMul(len, len, true); }
    iint large = randomIint(2048, true);
    for (iint small : {iint(0xFFFFFFFFULL), iint(0xFFFFFFFFFFFFFFFFULL), iint(-0x100000001LL)}) {
        context = "one/two-limb multiplication";
        REQUIRE(large * small == small * large); REQUIRE((large * small) / small == large);}
    for (int t = 0; t < 100'000; t++) {
        ulll x = (ulll(rng()) << 64) | rng();
        context = "Goldilocks reduction t=" + std::to_string(t);
        REQUIRE(iint::modP(x) == ulng(x % iint::NTT_P));}
    for (int len = 1; len <= 4096; len <<= 1) {
        context = "scalar NTT len=" + std::to_string(len);
        vector<ulng> a(len); for (ulng &x : a) { x = rng() % iint::NTT_P; }
        vector<ulng> orig = a; iint::ntt(a, false); iint::ntt(a, true); REQUIRE(a == orig);}
    context.clear();
}

#ifdef __AVX2__
template<class E, uint MOD>
void testAVXEngine(const string &name) {
    for (int t = 0; t < 20'000; t++) {
        uint a = rng() % MOD, b = rng() % MOD;
        context = name + " Montgomery t=" + std::to_string(t);
        REQUIRE(E::mul(a, E::init(b)) == ulng(a) * b % MOD);}
    alignas(32) uint a[8], bm[8], got[8];
    for (int t = 0; t < 20'000; t++) {
        for (int i = 0; i < 8; i++) { a[i] = rng() % MOD; bm[i] = E::init(rng() % MOD); }
        __m256i va = _mm256_load_si256(reinterpret_cast<const __m256i *>(a));
        __m256i vb = _mm256_load_si256(reinterpret_cast<const __m256i *>(bm));
        _mm256_store_si256(reinterpret_cast<__m256i *>(got), E::vmul(va, vb));
        for (int i = 0; i < 8; i++) { REQUIRE(got[i] == E::mul(a[i], bm[i])); }}
    alignas(32) uint ea[8] = {0, 1, MOD - 1, MOD - 2, MOD / 2, MOD / 2 + 1, 17, MOD - 17};
    alignas(32) uint eb[8] = {0, MOD - 1, 1, MOD - 1, MOD / 2 + 1, MOD / 2, MOD - 17, 17};
    alignas(32) uint add[8], sub[8], mul[8], ebm[8];
    for (int i = 0; i < 8; i++) { ebm[i] = E::init(eb[i]); }
    __m256i va = _mm256_load_si256(reinterpret_cast<const __m256i *>(ea));
    __m256i vb = _mm256_load_si256(reinterpret_cast<const __m256i *>(eb));
    _mm256_store_si256(reinterpret_cast<__m256i *>(add), E::vadd(va, vb));
    _mm256_store_si256(reinterpret_cast<__m256i *>(sub), E::vsub(va, vb));
    _mm256_store_si256(reinterpret_cast<__m256i *>(mul), E::vmul(va, _mm256_load_si256(reinterpret_cast<const __m256i *>(ebm))));
    for (int i = 0; i < 8; i++) {
        REQUIRE(add[i] == (ulng(ea[i]) + eb[i]) % MOD);
        REQUIRE(sub[i] == (ulng(ea[i]) + MOD - eb[i]) % MOD);
        REQUIRE(mul[i] == ulng(ea[i]) * eb[i] % MOD);}
    for (int len = 1; len <= 4096; len <<= 1) {
        context = name + " NTT len=" + std::to_string(len);
        vector<uint> v(len); for (uint &x : v) { x = rng() % MOD; }
        vector<uint> orig = v; E::ntt(v, false); E::ntt(v, true); REQUIRE(v == orig);}
}

void testAVX2() {
    testAVXEngine<FastIintMul::FastNTT<FastIintMul::P1, FastIintMul::G1>, FastIintMul::P1>("P1");
    testAVXEngine<FastIintMul::FastNTT<FastIintMul::P2, FastIintMul::G2>, FastIintMul::P2>("P2");
    testAVXEngine<FastIintMul::FastNTT<FastIintMul::P3, FastIintMul::G3>, FastIintMul::P3>("P3");
    for (int t = 0; t < 100; t++) {
        int al = 64 + rng() % 900, bl = 64 + rng() % 900;
        vector<uint> a = randomIint(al).n, b = randomIint(bl).n, got, expected = schoolMul(a, b);
        context = "direct AVX convolution t=" + std::to_string(t);
        FastIintMul::multiply(a, b, got); trimVec(got); REQUIRE(got == expected);}
    vector<uint> a = randomIint(700).n, same = a, got, expected = schoolMul(a, a);
    FastIintMul::multiply(a, same, got); trimVec(got); REQUIRE(got == expected);
    FastIintMul::multiply(a, a, a); trimVec(a); REQUIRE(a == expected);
    a = randomIint(8193).n; vector<uint> b = randomIint(8193).n; expected = schoolMul(a, b);
    context = "direct AVX convolution len=32768";
    FastIintMul::multiply(a, b, a); trimVec(a); REQUIRE(a == expected);
    context.clear();
}
#endif

void testScalarAndBigDivision() {
    for (iint a : vector<iint>{iint(1), iint(-1), iint(0x100000001ULL),
                               iint(-0x100000001LL), randomIint(300), randomIint(300, true)}) {
        context = "self-alias compound division limbs=" + std::to_string(a.n.size());
        iint q = a; q /= q; REQUIRE(q == 1);
        iint r = a; r %= r; REQUIRE(r.isNil());}
    for (lng a : {-17LL, -7LL, 0LL, 7LL, 17LL}) for (lng b : {-5LL, -3LL, 3LL, 5LL}) {
        context = "signed divModSlow"; REQUIRE(divModSlow(iint(a), iint(b)) == divMod(iint(a), iint(b)));}
    for (int t = 0; t < 100; t++) {
        iint a = randomIint(1 + rng() % 150, rng() & 1);
        for (lng d : {LLONG_MIN, -4'294'967'297LL, -4'294'967'296LL, -2LL, -1LL, 1LL, 2LL,
                      4'294'967'295LL, 4'294'967'296LL, LLONG_MAX}) {
            context = "scalar division d=" + std::to_string(d);
            iint q = a / d, r = a % d; REQUIRE(q * d + r == a); REQUIRE(abs(r) < abs(iint(d)));
            REQUIRE(r.isNil() || r.sgn == a.sgn); REQUIRE(a * d / d == a);}}
    for (int limbs : {7, 8}) for (bool neg : {false, true}) {
        iint a = randomIint(limbs, neg);
        for (ulng d : {2ULL, 1ULL << 31, 1ULL << 32, (1ULL << 32) + 1,
                       1ULL << 33, 1ULL << 63, (1ULL << 63) + 1, ULLONG_MAX}) {
            context = "unsigned scalar division limbs=" + std::to_string(limbs) + ", d=" + std::to_string(d);
            auto expected = divMod(a, iint(d)); iint q = a / d, r = a % d;
            REQUIRE(q == expected.first && r == expected.second); verifyDiv(a, iint(d), {q, r});
            iint cur = a; cur /= d; REQUIRE(cur == q); cur = a; cur %= d; REQUIRE(cur == r);}}
    iint pow2_a = randomIint(600, true);
    for (int k : {0, 1, 31, 32, 33, 127, 4096, 10'000}) {
        iint d = iint(1) << k; context = "power-of-two division k=" + std::to_string(k);
        verifyDiv(pow2_a, d, divMod(pow2_a, d));}
    for (int t = 0; t < 30; t++) {
        int bl = 257 + rng() % 160, al = bl + rng() % (bl + 1);
        iint a = randomIint(al), b = randomIint(bl);
        context = "fast/slow division t=" + std::to_string(t) + " limbs=" + std::to_string(al) + ',' + std::to_string(bl);
        auto slow = divModSlow(a, b), fast = divMod(a, b); REQUIRE(fast == slow); verifyDiv(a, b, fast);
        iint bn = b << __builtin_clz(b.n.back()), bi = iint::newtonRaphsonInv(bn);
        if (a.n.size() <= 2 * b.n.size()) { auto cached = divMod(a, b, &bi); REQUIRE(cached == slow); }
        if (t < 5) {
            iint num; num.n.assign(2 * bn.n.size() + 1, 0); num.n.back() = 1;
            iint exact = divModSlow(num, bn).first;
            REQUIRE(abs(bi - exact) <= 4);}}
    for (int t = 0; t < 10; t++) {
        iint b = randomIint(257 + rng() % 300), q = randomIint(1 + rng() % 900), r = b - 1;
        iint a = b * q + r; context = "crafted fast division t=" + std::to_string(t);
        auto qr = divMod(a, b); REQUIRE(qr.first == q && qr.second == r); verifyDiv(a, b, qr);
        a.sgn = -1; qr = divMod(a, b); REQUIRE(qr.first == -q && qr.second == -r); verifyDiv(a, b, qr);}
    for (uint fill : {0U, UINT_MAX}) {
        iint a; a.n.assign(257, fill); a.n.back() = fill ? UINT_MAX : 0x80000000U;
        iint got = iint::newtonRaphsonInv(a), num; num.n.assign(2 * a.n.size() + 1, 0); num.n.back() = 1;
        iint exact = divModSlow(num, a).first;
        context = "adversarial Newton fill=" + std::to_string(fill);
        REQUIRE(abs(got - exact) <= 4);}
    context.clear();
}

iint slowGcd(iint a, iint b) {
    a.sgn = b.sgn = 1;
    while (!b.isNil()) { auto r = divModSlow(a, b).second; a = std::exchange(b, std::move(r)); }
    return a;
}

void checkHalfGcd(const iint &a0, const iint &b0) {
    iint a = a0, b = b0, A, B, C, D;
    bool reduced = iint::halfGcd(a, b, A, B, C, D);
    context = "halfGcd limbs=" + std::to_string(a0.n.size()) + ',' + std::to_string(b0.n.size());
    REQUIRE(abs(A * D - B * C) == 1);
    iint ta = a0, tb = b0; iint::leftMatMul(A, B, C, D, ta, tb);
    REQUIRE(ta == a && tb == b);
    iint ra = a0, rb = b0, rA, rB, rC, rD;
    REQUIRE(iint::halfGcd(ra, rb, rA, rB, rC, rD) == reduced);
    REQUIRE(reduced == !iint::isIdMat(A, B, C, D));
    REQUIRE(rA == A && rB == B && rC == C && rD == D && ra == a && rb == b);
    iint ma = a0, mb = b0, unusedA, unusedB, unusedC, unusedD;
    REQUIRE(iint::halfGcd<false>(ma, mb, unusedA, unusedB, unusedC, unusedD) == reduced);
    REQUIRE(ma == a && mb == b);
    REQUIRE(a.sgn == 1 && b.sgn == 1 && a >= b);
    REQUIRE(b.isNil() || b.n.size() <= (a0.n.size() + 1) / 2);
}

void testGcdHalfGcdAndNumberTheory() {
    for (auto [a, b] : vector<pair<iint, iint>>{{0, 0}, {0, 37}, {-37, 0}, {37, 37}, {-37, 37}}) {
        iint expected = a.isNil() ? abs(b) : b.isNil() ? abs(a) : abs(a);
        context = "GCD zero/equal identity a=" + a.toString() + ", b=" + b.toString();
        REQUIRE(gcd(a, b) == expected);
        iint x, y, g = exGcd(a, b, x, y);
        REQUIRE(g == expected && a * x + b * y == g);
        REQUIRE(lcm(a, b) == (a.isNil() || b.isNil() ? iint(0) : expected));}
    checkHalfGcd(0, 0); checkHalfGcd(1, 0); checkHalfGcd(1, 1);
    {
        iint a = 0x123456789ABCDEFLL, b = 0xFEDCBA987654321LL, x = a, y = b;
        iint::leftMatMul(UINT_MAX, 1U, 0U, UINT_MAX, x, y);
        REQUIRE(x == a * ulng(UINT_MAX) + b && y == b * ulng(UINT_MAX));
        x = a; y = b; iint::leftMatMul(ULLONG_MAX, 0ULL, 1ULL, ULLONG_MAX, x, y);
        REQUIRE(x == a * ULLONG_MAX && y == a + b * ULLONG_MAX);}
    for (int t = 0; t < 100; t++) {
        iint a = randomIint(1 + rng() % 64), b = randomIint(1 + rng() % 64);
        if (a < b) { std::swap(a, b); }
        iint A = 1, B = 0, C = 0, D = 1;
        for (int step = 0; step < 100 && !b.isNil(); step++) {
            iint pa = a, pb = b, pA = A, pB = B, pC = C, pD = D;
            auto [q, r] = divMod(a, b); iint::leftEucMul(std::move(q), A, B, C, D);
            a = std::exchange(b, std::move(r));
            iint ra = a, rb = b, rA = A, rB = B, rC = C, rD = D;
            iint::backEucMul(rA, rB, rC, rD, ra, rb);
            context = "halfGcd rollback t=" + std::to_string(t) + ", step=" + std::to_string(step);
            REQUIRE(ra == pa && rb == pb && rA == pA && rB == pB && rC == pC && rD == pD);}}
    for (int t = 0; t < 300; t++) {
        int len = 3 + rng() % 62;
        iint a0 = randomIint(len), b0 = randomIint(len);
        if (t % 3 == 0) { std::fill(a0.n.begin(), a0.n.end() - 2, 0); }
        if (t % 3 == 1) { std::fill(b0.n.begin(), b0.n.end() - 2, UINT_MAX); }
        if (a0 < b0) { std::swap(a0, b0); }
        iint a = a0, b = b0, A = 1, B = 0, C = 0, D = 1;
        if (!iint::lehmerHalfGcd(a, b, A, B, C, D)) { continue; }
        iint ea = a0, eb = b0, eA = 1, eB = 0, eC = 0, eD = 1;
        int steps = 0;
        while ((ea != a || eb != b) && steps++ < 128) {
            auto [q, r] = divMod(ea, eb); iint::leftEucMul(std::move(q), eA, eB, eC, eD);
            ea = std::exchange(eb, std::move(r));}
        context = "certified Lehmer batch t=" + std::to_string(t);
        REQUIRE(ea == a && eb == b && eA == A && eB == B && eC == C && eD == D);
        REQUIRE(abs(A * D - B * C) == 1 && a.sgn == 1 && b.sgn == 1 && a >= b);}
    for (auto [al, bl] : vector<pair<int, int>>{{65, 64}, {129, 128}, {300, 290}, {513, 512}, {1024, 1000}}) {
        iint a = randomIint(al), b = randomIint(bl); if (a < b) { std::swap(a, b); } checkHalfGcd(a, b);}
    for (int t = 0; t < 100; t++) {
        int al = 65 + rng() % 2000, bl = 1 + rng() % al;
        iint a = randomIint(al), b = randomIint(bl); if (a < b) { std::swap(a, b); }
        checkHalfGcd(a, b);}
    for (int t = 0; t < 12; t++) {
        iint a = randomIint(260 + rng() % 80, rng() & 1), b = randomIint(250 + rng() % 80, rng() & 1);
        context = "gcd/exGcd t=" + std::to_string(t);
        iint expected = slowGcd(a, b); REQUIRE(gcd(a, b) == expected);
        iint x, y, g = exGcd(a, b, x, y); REQUIRE(g == expected && a * x + b * y == g);
        REQUIRE(lcm(a, b) == abs(a / g * b));}
    iint f0 = 0, f1 = 1;
    for (int i = 0; i < 200'000; i++) { f0 = std::exchange(f1, f0 + f1); }
    context = "Fibonacci half-GCD"; REQUIRE(f0.n.size() > 4096 && f1.n.size() > 4096);
    checkHalfGcd(f1, f0); checkHalfGcd(f1, f1); REQUIRE(gcd(f1, f0) == 1);
    iint x, y, g = exGcd(f1, f0, x, y); REQUIRE(g == 1 && f1 * x + f0 * y == 1);
    for (int a = -100; a <= 100; a++) for (int m = 2; m <= 100; m++) {
        iint ia = a, im = m, iim = -m, iv = inv(ia, im), iiv = inv(ia, iim); int gg = std::gcd(abs(a), m);
        context = "inverse a=" + std::to_string(a) + " mod=" + std::to_string(m);
        if (gg != 1) { REQUIRE(iv == -1 && iiv == -1); }
        else {
            REQUIRE(iv >= 0 && iv < im && ((ia * iv) % im + im) % im == 1);
            REQUIRE(iiv == iv);}}
    for (int a = -10; a <= 10; a++) { REQUIRE(inv(iint(a), iint(1)) == 0); REQUIRE(inv(iint(a), iint(-1)) == 0); }
    for (int a = -12; a <= 12; a++) for (int e = 0; e <= 20; e++) {
        lll expected = 1; for (int i = 0; i < e; i++) { expected *= a; }
        REQUIRE(pow(iint(a), e) == iint(expected));}
    for (int t = 0; t < 20; t++) {
        iint mod = randomIint(257 + rng() % 100), a = randomIint(257 + rng() % 100, rng() & 1), e = rng() % 2000;
        context = "large powMod t=" + std::to_string(t); iint r = powMod(a, e, mod);
        REQUIRE(r >= 0 && r < mod);}
    REQUIRE(sqrt(iint(0)) == 0 && sqrt(iint(1)) == 1 && sqrt(iint(2)) == 1);
    for (int bits : {1, 2, 31, 32, 33, 127, 128, 129, 1024, 8192}) {
        iint a = randomIint((bits + 31) / 32); a >>= 32 * int(a.n.size()) - bits;
        context = "sqrt bits=" + std::to_string(bits);
        iint r = sqrt(a); REQUIRE(r * r <= a && (r + 1) * (r + 1) > a); REQUIRE(sqrt(a * a) == a);}
    for (int bits = 0; bits <= 500; bits++) {
        iint r = iint::rand(bits); if (!bits) { REQUIRE(r.isNil()); }
        else { REQUIRE(r.n.size() * 32 - __builtin_clz(r.n.back()) == bits); }}
    context.clear();
}

void testInfinity() {
    context = "infinity"; iint pi("inf"), ni("-inf"), z = 0, p = 7, n = -7;
    for (const iint &a : vector<iint>{z, p, n, pi, ni}) {
        REQUIRE(+a == a); REQUIRE(static_cast<bool>(a) == !a.isNil()); REQUIRE((!a) == a.isNil());}
    REQUIRE(pi > p && ni < n && pi != ni && pi == iint("INF"));
    REQUIRE(pi + p == pi && ni + p == ni && (pi + ni).isNil());
    REQUIRE((pi - pi).isNil() && pi - ni == pi); REQUIRE(pi * n == ni && ni * n == pi && (pi * z).isNil());
    REQUIRE((p / pi).isNil() && pi / n == ni && pi / ni == -1); REQUIRE((pi % p).isNil() && p % pi == p);
    REQUIRE(abs(ni) == pi && pow(ni, 2) == pi && pow(ni, 3) == ni && sqrt(pi) == pi);
    iint piBlocks = pi, niBlocks = ni;
    piBlocks.shiftBlocksLeft(3).shiftBlocksRight(7);
    niBlocks.shiftBlocksRight(3).shiftBlocksLeft(7);
    REQUIRE(piBlocks == pi && niBlocks == ni);
    for (const iint &a : vector<iint>{pi, ni, z, p, n}) for (const iint &b : vector<iint>{pi, ni, p, n}) {
        auto qr = divMod(a, b), sr = divModSlow(a, b);
        REQUIRE(qr.first == a / b && qr.second == a % b && sr == qr);}
    for (auto [a, b] : vector<pair<iint, iint>>{{pi, ni}, {pi, z}, {pi, n}, {z, ni}, {p, ni}}) {
        iint x, y, g = exGcd(a, b, x, y);
        REQUIRE(g == gcd(a, b) && a * x + b * y == g);}
    REQUIRE(lcm(z, pi).isNil() && lcm(ni, z).isNil());
    context.clear();
}

void runProtocol() {
    int total; if (!(cin >> total)) { return; }
    ProtocolProgress progress(total);
    for (int tc = 0; tc < total; tc++) {
        string op, sa, sb, sc; cin >> op;
        if (op == "add" || op == "sub" || op == "mul" || op == "div" || op == "and" || op == "or" ||
            op == "xor" || op == "cmp" || op == "gcd" || op == "lcm") {
            cin >> sa >> sb; iint a(sa), b(sb);
            if (op == "add") { cout << a + b; }
            else if (op == "sub") { cout << a - b; }
            else if (op == "mul") { cout << a * b; }
            else if (op == "div") { auto [q, r] = divMod(a, b); cout << q << ' ' << r; }
            else if (op == "and") { cout << (a & b); }
            else if (op == "or") { cout << (a | b); }
            else if (op == "xor") { cout << (a ^ b); }
            else if (op == "cmp") { cout << (a < b ? -1 : a > b ? 1 : 0); }
            else if (op == "gcd") { cout << gcd(a, b); }
            else { cout << lcm(a, b); }}
        else if (op == "not" || op == "sqrt" || op == "inc" || op == "dec" || op == "ctz" || op == "popcount") {
            cin >> sa; iint a(sa);
            if (op == "not") { cout << ~a; }
            else if (op == "sqrt") { cout << sqrt(a); }
            else if (op == "inc") { cout << ++a; }
            else if (op == "dec") { cout << --a; }
            else if (op == "ctz") { cout << ctz(a); }
            else { cout << popcount(a); }}
        else if (op == "bin" || op == "oct" || op == "hex") {
            cin >> sa; iint a(sa);
            if (op == "bin") { cout << bin(a); }
            else if (op == "oct") { cout << oct(a); }
            else { cout << hex(a); }}
        else if (op == "shl" || op == "shr" || op == "pow") {
            lng e; cin >> sa >> e; iint a(sa);
            if (op == "shl") { cout << (a << int(e)); }
            else if (op == "shr") { cout << (a >> int(e)); }
            else { cout << pow(a, e); }}
        else if (op == "powmod") { cin >> sa >> sb >> sc; cout << powMod(iint(sa), iint(sb), iint(sc)); }
        else if (op == "inv") { cin >> sa >> sb; cout << inv(iint(sa), iint(sb)); }
        else if (op == "exgcd") { cin >> sa >> sb; iint x, y, g = exGcd(iint(sa), iint(sb), x, y); cout << g << ' ' << x << ' ' << y; }
        else if (op == "tobase") { uint base; cin >> base >> sa; cout << iint(sa).toString(base); }
        else if (op == "frombase") { uint base; cin >> base >> sa; cout << iint(sa, base); }
        else if (op == "newton") {
            cin >> sa; iint a(sa, 16); a <<= __builtin_clz(a.n.back()); cout << iint::newtonRaphsonInv(a).toString(16);}
        else if (op == "halfgcd") {
            cin >> sa >> sb; iint a(sa, 16), b(sb, 16), A, B, C, D; iint::halfGcd(a, b, A, B, C, D);
            cout << A.toString(16) << ' ' << B.toString(16) << ' ' << C.toString(16) << ' ' << D.toString(16);}
        else { fail("unknown protocol operation: " + op, __LINE__); }
        cout << '\n'; progress.update(tc + 1);
    }
}

int main(int argc, char **argv) {
    std::ios_base::sync_with_stdio(false); cin.tie(nullptr);
    if (argc > 1 && string(argv[1]) == "--protocol") { runProtocol(); return 0; }
    #ifdef __AVX2__
    constexpr int PHASES = 7;
    #else
    constexpr int PHASES = 6;
    #endif
    cerr << "    [self-test] RNG seed=0x243F6A8885A308D3\n";
    int phase = 0;
    runPhase(++phase, PHASES, "construction, bases, and I/O", testConstructionBasesAndIO);
    runPhase(++phase, PHASES, "arithmetic, bitwise operations, and shifts", testSmallArithmeticBitwiseAndShifts);
    runPhase(++phase, PHASES, "multiplication and scalar NTT", testMultiplicationAndNTT);
    #ifdef __AVX2__
    runPhase(++phase, PHASES, "AVX2 Montgomery arithmetic, NTT, and convolution", testAVX2);
    #endif
    runPhase(++phase, PHASES, "scalar and large division", testScalarAndBigDivision);
    runPhase(++phase, PHASES, "GCD, half-GCD, and number theory", testGcdHalfGcdAndNumberTheory);
    runPhase(++phase, PHASES, "infinity semantics", testInfinity);
    cout << GREEN << "PASS: 1-Core_05-infint_tester (";
    #ifdef __AVX2__
    cout << "AVX2";
    #else
    cout << "scalar";
    #endif
    cout << ')' << RESET << '\n';
}
