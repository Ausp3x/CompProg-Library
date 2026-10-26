// Independent oracles for 07-infint.hpp: schoolbook limb products, identities checked with oracle products, two's-complement bit models, sieves and native 128-bit arithmetic; --protocol serves the Python big-integer oracle.
#include "../../01-Core/07-infint.hpp"
#include <cctype>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <format>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <string>
#include <vector>

static ulng seed = 42;
static std::string mode = "full";
static std::string context;
static lng checks = 0;
static std::mt19937_64 rng(42);
using W = infint_detail::W;

ulng otherUnitSum(const InfInt &a);

[[noreturn]] static void fail(const std::string &message) {
    std::cerr << "FAIL seed=" << seed << " mode=" << mode << " context=" << context << '\n' << message << '\n';
    std::exit(1);}
static void require(bool ok, const std::string &message) { ++checks; if (!ok) { fail(message); } }
template<typename T> static std::ostream &operator<<(std::ostream &os, const vector<T> &v) { os << '['; for (size_t i = 0; i < v.size(); ++i) { os << (i ? ", " : "") << v[i]; } return os << ']'; }
template<typename A, typename B> static std::ostream &operator<<(std::ostream &os, const pair<A, B> &p) { return os << '(' << p.first << ", " << p.second << ')'; }
template<class A, class B> static void expect(const A &actual, const B &expected, const std::string &operation) {
    ++checks;
    if (!(actual == expected)) {
        std::ostringstream out; out << operation << "\n expected=" << expected << "\n actual=" << actual;
        fail(out.str());}}
static int ri(int k) { return int(rng() % ulng(k)); }
static bool full() { return mode != "quick"; }
static bool stress() { return mode == "stress"; }
static InfInt randBits(int bits, bool neg = false) { InfInt r = InfInt::sampleBits(bits, rng); if (neg) { r = -r; } return r; }
static InfInt randSigned(int bits) { return randBits(1 + ri(max(bits, 1)), ri(2) == 1); }
static vector<uint> schoolLimbs(const vector<uint> &a, const vector<uint> &b) {
    vector<uint> r(a.size() + b.size(), 0);
    for (size_t i = 0; i < a.size(); ++i) {
        ulng c = 0;
        for (size_t j = 0; j < b.size(); ++j) { ulll t = ulll(a[i]) * b[j] + r[i + j] + c; r[i + j] = uint(t); c = ulng(t >> 32); }
        for (size_t j = i + b.size(); c; ++j) { ulng t = r[j] + c; r[j] = uint(t); c = t >> 32; }}
    while (r.size() > 1 && r.back() == 0) { r.pop_back(); }
    return r;}
// Oracle product through limb schoolbook arithmetic only.
static InfInt omul(const InfInt &a, const InfInt &b) {
    if (a.isNil() || b.isNil()) { return 0; }
    return InfInt::fromLimbs(schoolLimbs(a.toLimbs(), b.toLimbs()), a.sgn * b.sgn);}
static vector<W> wordsOf(const InfInt &a) { vector<W> w(a.n.begin(), a.n.end()); w.resize(size_t(infint_detail::norm(w.data(), int(w.size())))); return w; }
static InfInt ofWords(const vector<W> &w) { return InfInt::fromLimbs(w); }
static InfInt egcd(InfInt a, InfInt b) { a = abs(a); b = abs(b); while (!b.isNil()) { InfInt r = divMod(a, b).second; a = std::move(b); b = std::move(r); } return a; }
static lng floorDiv(lng a, lng b) { lng q = a / b; if ((a % b != 0) && ((a < 0) != (b < 0))) { --q; } return q; }
static int sz(int small, int big) { return full() ? big : small; }
static std::string lowered(std::string s) { for (char &c : s) { c = char(std::tolower(uint8_t(c))); } return s; }
// Pattern words for arena frames: salt and index, so a frame overwritten by another frame or a lost copy shows.
static void pattern(W *p, size_t n, W salt) { for (size_t i = 0; i < n; ++i) { p[i] = (salt * 0x9E3779B97F4A7C15ULL) ^ i; } }
static bool intact(const W *p, size_t n, W salt) { for (size_t i = 0; i < n; ++i) { if (p[i] != ((salt * 0x9E3779B97F4A7C15ULL) ^ i)) { return false; } } return true; }
static bool disjoint(const W *p, size_t n, const W *q, size_t m) { auto a = uintptr_t(p), b = uintptr_t(q); return a + 8 * n <= b || b + 8 * m <= a; }

// Words inline-to-heap transitions against a vector model, InfInt values across the inline limit and nested arena frames.
static void storage() {
    context = "Words";
    using infint_detail::Words;
    require(sizeof(InfInt) == 56, "sizeof(InfInt) == 56");
    auto same = [](const Words &x, const vector<W> &v) { return x.size() == int(v.size()) && std::equal(x.begin(), x.end(), v.begin(), v.end()); };
    Words w; vector<W> model;
    require(w.empty() && w.size() == 0 && w.data() == w.buf, "default Words is empty and inline");
    for (int i = 0; i < 40; ++i) {
        W v = rng(); w.push_back(v); model.push_back(v);
        require(same(w, model) && w.back() == v && (w.data() == w.buf) == (i < Words::INLINE), "push_back to " + std::to_string(i + 1) + " words");
        Words c = w; require(c == w && same(c, model) && c.data() != w.data() && (c.data() == c.buf) == (i < Words::INLINE), "copy construction");
        Words m = std::move(c); require(same(m, model) && c.empty() && c.data() == c.buf, "move construction empties the source");
        c.push_back(1); require(c.size() == 1 && c[0] == 1, "a moved-from Words is reusable");
        Words h; h.resize(9, 3); h = std::move(m); require(same(h, model) && m.empty(), "move assignment over heap storage");
        Words in; in.push_back(5); in = h; require(same(in, model), "copy assignment over inline storage");
        Words big; big.assign(50, 7); big = in; require(same(big, model), "copy assignment over heap storage");
        Words &alias = in; in = alias; require(same(in, model), "self copy assignment"); in = std::move(alias); require(same(in, model), "self move assignment");}
    while (w.size() > 3) { w.pop_back(); model.pop_back(); }
    { Words small = w; require(same(small, model) && small.data() == small.buf, "a copy of three heap words is inline");
      const W *heap = w.data(); Words moved = std::move(w); require(same(moved, model) && moved.data() == heap && w.empty() && w.data() == w.buf, "a move steals heap storage");
      Words r; r.setSize(3); r[0] = r[1] = r[2] = 7; r.resize(6); require(r.size() == 6 && r[2] == 7 && r[3] == 0 && r[5] == 0, "resize zero-fills");
      r.resize(2); r.resize(5, 9); require(r[1] == 7 && r[2] == 9 && r[4] == 9, "resize fills the new words"); r.reserve(100); require(r.size() == 5 && r[0] == 7 && r[4] == 9, "reserve keeps the words");
      Words s; s.assign(5, 9); s[0] = 7; s[1] = 7; require(s == r && !(s == small), "operator=="); r.clear(); require(r.empty(), "clear"); }
    context = "InfInt inline and heap words";
    for (int words = 1; words <= 9; ++words) {
        InfInt a = randBits(64 * words, ri(2) == 1), b = randBits(64 * (10 - words)), a0 = a, b0 = b;
        expect(a.n.size(), words, "word count"); require((a.n.data() == a.n.buf) == (words <= 4), "inline up to four words");
        std::swap(a, b); expect(a, b0, "swap"); expect(b, a0, "swap back"); std::swap(a, b);
        InfInt c = a; c = b; expect(c, b0, "copy assignment across storage"); c = std::move(a); expect(c, a0, "move assignment across storage"); a = c;
        expect(omul(a + b - b, InfInt(1)), a0, "arithmetic across the inline limit"); expect((a << 64) >> 64, a0, "shift across the inline limit");}
    context = "moved-from values";
    for (InfInt src : {InfInt(-5), InfInt("inf"), InfInt("-inf"), -randBits(64 * 9), randBits(64 * 3), InfInt(0)}) {
        InfInt a = src, b = std::move(a);
        require(a.isNil() && a.sgn == 1 && !a.is_inf && a.n.empty(), "a moved-from InfInt is zero");
        require(b.is_inf == src.is_inf && b.sgn == src.sgn && b.n == src.n, "moved-to value");
        ++a; expect(a, InfInt(1), "++ on a moved-from value");
        a = src; b = std::move(a); require(a.isNil() && a.sgn == 1 && !a.is_inf && a.n.empty(), "move assignment resets the source");
        a.setBit(3); expect(a, InfInt(8), "setBit on a moved-from value");
        a = src; b = std::move(a); a.flipBit(0); expect(a, InfInt(1), "flipBit on a moved-from value");
        a = src; b = std::move(a); require(a == InfInt(0) && !(a < InfInt(0)) && a + src == b, "comparison and sum on a moved-from value");}
    context = "Scratch";
    using infint_detail::Scratch;
    {   Scratch s1(1); pattern(s1, 1, 1);
        {   Scratch s2(1000); pattern(s2, 1000, 2);
            {   Scratch s3(1000000); pattern(s3, 1000000, 3);
                {   Scratch s4(3000000); pattern(s4, 3000000, 4);
                    require(intact(s4, 3000000, 4) && intact(s3, 1000000, 3) && intact(s2, 1000, 2) && intact(s1, 1, 1), "nested frames of 1, 1000, 10^6 and 3 10^6 words");
                    require(disjoint(s4, 3000000, s3, 1000000) && disjoint(s3, 1000000, s2, 1000) && disjoint(s2, 1000, s1, 1), "nested frames are disjoint");}
                require(intact(s3, 1000000, 3) && intact(s2, 1000, 2) && intact(s1, 1, 1), "outer frames after the 3 10^6-word frame is released");
                { Scratch s5(3000000); pattern(s5, 3000000, 5); require(disjoint(s5, 3000000, s3, 1000000), "a reused block is disjoint from live frames"); }
                { Scratch s6(10); pattern(s6, 10, 6); require(disjoint(s6, 10, s3, 1000000) && disjoint(s6, 10, s2, 1000), "a new frame is disjoint from live frames"); }
                require(intact(s3, 1000000, 3) && intact(s2, 1000, 2), "outer frames after reuse");}
            require(intact(s2, 1000, 2) && intact(s1, 1, 1), "outer frames after the 10^6-word frame is released");}
        require(intact(s1, 1, 1), "outermost frame");}
    { Scratch after(100); pattern(after, 100, 9); require(intact(after, 100, 9), "a frame after the nested frames are released"); }
    context = "construction";}

static void construction() {
    storage();
    context = "construction";
    for (lng x : {LLONG_MIN, -4294967297LL, -4294967296LL, -1LL, 0LL, 1LL, 4294967295LL, 4294967296LL, LLONG_MAX}) {
        InfInt a = x; lng y = 0; require(a.tryCast(y) && y == x && lng(a) == x, "lng round trip"); InfInt b; b = x; expect(b, a, "assignment");
        expect(a.toString(), std::to_string(x), "decimal of lng"); expect(InfInt(std::to_string(x)), a, "parse decimal");}
    for (ulng x : {0ULL, 1ULL, 0xFFFFFFFFULL, 0x100000000ULL, ULLONG_MAX}) { InfInt a = x; ulng y = 1; require(a.tryCast(y) && y == x && ulng(a) == x, "ulng round trip"); }
    { InfInt u = ~ulll(0); expect(u.toLimbs(), vector<uint>({UINT_MAX, UINT_MAX, UINT_MAX, UINT_MAX}), "ulll limbs"); require(u.n.size() == 2 && u.n[0] == ~W(0) && u.n[1] == ~W(0), "ulll words"); ulll back = 0; require(u.tryCast(back) && back == ~ulll(0), "ulll round trip");
      InfInt s = lll(ulll(1) << 127); require(s.sgn == -1 && s.toLimbs() == vector<uint>({0, 0, 0, 0x80000000u}) && s.n.size() == 2 && s.n[0] == 0 && s.n[1] == W(1) << 63, "lll min limbs"); lll b2 = 0; require(s.tryCast(b2) && b2 == lll(ulll(1) << 127), "lll min round trip");
      int i = 0; require(!s.tryCast(i) && !u.tryCast(i), "narrow fails"); require(InfInt(INT_MIN).tryCast(i) && i == INT_MIN, "int min"); require(!InfInt(lng(INT_MIN) - 1).tryCast(i), "below int min");
      uint8_t c = 0; require(InfInt(255).tryCast(c) && c == 255 && !InfInt(256).tryCast(c) && !InfInt(-1).tryCast(c), "uint8 bounds"); int8_t d = 0; require(InfInt(-128).tryCast(d) && d == -128 && !InfInt(-129).tryCast(d), "int8 bounds");
      InfInt inf("inf"); require(!inf.tryCast(i) && lng(inf) == LLONG_MAX && lng(-inf) == LLONG_MIN && ulng(inf) == ULLONG_MAX && ulng(-inf) == 0, "sentinel casts");
      expect(lng(InfInt(ulll(1) << 64)), 0LL, "wrap lng"); expect(ulng(InfInt(-1)), ULLONG_MAX, "wrap ulng"); expect(lng(InfInt("-9223372036854775809")), LLONG_MAX, "wrap below min");}
    for (int t = 0; t < sz(100, 1000); ++t) {
        InfInt a = randSigned(300);
        vector<uint> l32 = a.toLimbs(); vector<ulng> l64 = a.toLimbs<ulng>();
        expect(InfInt::fromLimbs(l32, a.sgn), a, "limbs 32 round trip"); expect(InfInt::fromLimbs(l64, a.sgn), a, "limbs 64 round trip");
        vector<uint> padded = l32; padded.push_back(0); padded.push_back(0); expect(InfInt::fromLimbs(padded, a.sgn), a, "padded limbs");
        for (uint base : {2u, 3u, 7u, 8u, 10u, 16u, 32u, 36u}) {
            std::string s = a.toString(base); expect(InfInt(s, base), a, "string round trip base " + std::to_string(base));
            std::string lower = s; for (char &c : lower) { c = char(std::tolower(c)); } expect(InfInt(lower, base), a, "lowercase parse");
            InfInt c = a; require(c.read("#" + s, base) == false && c == a, "bad char keeps value");}
        for (uint base : {2u, 10u, 1000u, 65536u, 1000000007u, 4294967295u}) {
            vector<uint> d = abs(a).toDigits(base); for (uint x : d) { require(x < base, "digit range"); } require(d.size() == 1 || d[0] != 0, "no leading zero digit");
            expect(InfInt(d, base, a.sgn), a, "digits round trip base " + std::to_string(base));}}
    { InfInt t; const W five[] = {5, 0, 0}; t.n.assign(five, 3); t.sgn = -1; t.trim(); require(t.n.size() == 1 && t.n[0] == 5 && t == -5, "trim strips zero words"); t.n.assign(2, 0); t.sgn = -1; t.trim(); require(t.n.size() == 1 && t.n[0] == 0 && t.sgn == 1 && t.isNil(), "trim resets negative zero"); t.n.clear(); t.trim(); require(t.n.size() == 1 && t.n[0] == 0, "trim fills empty");
      InfInt p = 7; expect(p++, InfInt(7), "postfix increment value"); expect(p, InfInt(8), "postfix increment effect"); expect(p--, InfInt(8), "postfix decrement value"); expect(p, InfInt(7), "postfix decrement effect"); InfInt q = -1; expect(q++, InfInt(-1), "postfix through zero"); require(q.isNil() && q.sgn == 1, "zero after increment");
      require(InfInt::getBaseId() == InfInt::getBaseId() && InfInt::getBaseId() >= 0, "stable base id"); }
    { InfInt z; require(z.isNil() && !z && z.toString() == "0" && z.toDigits(10) == vector<uint>{0} && z.toLimbs() == vector<uint>{0}, "zero forms"); InfInt z2 = 0; require(z == z2 && std::hash<InfInt>{}(z) == std::hash<InfInt>{}(z2), "zero hash");
      expect(InfInt("-0"), z, "-0"); expect(InfInt("000123"), InfInt(123), "leading zeros"); expect(InfInt("+123"), InfInt(123), "plus sign");
      InfInt keep = 5; for (const char *bad : {"", "+", "-", "12a", "1 2", "--1", " 1", "1.0", "in", "infinity"}) { require(!keep.read(bad) && keep == 5, std::string("reject ") + bad); }
      require(!keep.read("129", 9) && keep.read("128", 9) && keep == 107, "digit below base");
      require(!keep.read("102", 2) && !keep.read("18", 8) && !keep.read("1W", 32) && keep.read("1V", 32) && keep == 63 && keep.read("-0Fa", 16) && keep == -250, "power-of-two bases");
      for (int len : {1, 7, 8, 9, 15, 16, 17, 31, 32, 33, 40, 63, 64, 65, 100, 4097}) {
          if (len > 100 && !full()) { continue; }
          std::string h; InfInt acc = 0; for (int i = 0; i < len; ++i) { char c = "0123456789abcdefABCDEF"[ri(22)]; h += c; acc = acc * 16 + InfInt::DIGIT_VALUE[size_t(uint8_t(c))]; }
          expect(InfInt(h, 16), acc, "hex parse of length " + std::to_string(len));
          for (int pos = 0; pos < len; pos += full() ? (len <= 100 ? 1 : 97) : max(1, len / 2)) { for (int bad = 0; bad < 256; ++bad) { if (std::isxdigit(bad) || (pos == 0 && (bad == '+' || bad == '-'))) { continue; } std::string w = h; w[size_t(pos)] = char(bad); InfInt k = 5; require(!k.read(w, 16) && k == 5, "byte " + std::to_string(bad) + " kept out at " + std::to_string(pos) + " of " + std::to_string(len)); } }
          InfInt si("inf"); require(!si.read(h + "g", 16) && si.is_inf, "rejected hex keeps the sentinel");} require(keep.read("INF") && keep.is_inf && keep.sgn == 1 && keep.read("-Inf") && keep.sgn == -1, "inf tokens");
      expect(InfInt("ZZ", 36), InfInt(1295), "base 36"); expect(InfInt(1295).toString(36), std::string("ZZ"), "print base 36"); expect(InfInt("-ff", 16), InfInt(-255), "hex lowercase");}
    for (int words : {64, 65, 300, 1023, 1024, 1025, 2500}) {
        if (!full() && words > 300) { continue; }
        context = "radix round trip words " + std::to_string(words);
        InfInt a = randBits(64 * words - ri(64), ri(2)); if (words == 65) { a = (InfInt(1) << (64 * words)) - 1; }
        for (uint base : {10u, 16u, 3u, 36u}) { std::string s = a.toString(base); expect(InfInt(s, base), a, "large round trip"); expect(int(s.size()) > 0, true, "nonempty"); }
        expect(InfInt(abs(a).toDigits(1000000000u), 1000000000u, a.sgn), a, "large digits round trip");}
    for (int t = 0; t < sz(50, 500); ++t) {
        ulng v = rng() >> ri(64); InfInt a = v; expect(a.toDouble(), double(v), "toDouble below 2^64"); expect(InfInt::fromDouble(double(v)), InfInt(ulng(double(v))), "fromDouble below 2^64 (truncated)");
        int e = ri(900); InfInt big = (InfInt(v) << e); double d = big.toDouble(); require(d == std::ldexp(double(v), e), "toDouble exact scaling"); expect(InfInt::fromDouble(d), InfInt(ulng(double(v))) << e, "fromDouble scaled");
        expect(InfInt::fromDouble(-d), -(InfInt(ulng(double(v))) << e), "fromDouble negative");}
    { InfInt t = (InfInt(1) << 53) + 1; expect(t.toDouble(), 9007199254740992.0, "tie to even"); t += 2; expect(t.toDouble(), 9007199254740996.0, "tie to even up"); t = (InfInt(1) << 100) + 1; expect(t.toDouble(), std::ldexp(1.0, 100), "sticky below"); t = (InfInt(1) << 1024); require(std::isinf(t.toDouble()) && t.toDouble() > 0, "overflow to inf"); require(std::isinf((-t).toDouble()), "negative overflow");
      expect(InfInt::fromDouble(0.0), InfInt(0), "fromDouble zero"); expect(InfInt::fromDouble(-0.999), InfInt(0), "fromDouble fraction"); expect(InfInt::fromDouble(-2.5), InfInt(-2), "fromDouble truncation"); require(InfInt::fromDouble(INFINITY).is_inf && InfInt::fromDouble(-INFINITY).sgn == -1, "fromDouble infinity"); expect(InfInt("inf").toDouble(), INFINITY, "inf toDouble");
      expect(InfInt::fromDouble(1e300).toString(), std::string("1000000000000000052504760255204420248704468581108159154915854115511802457988908195786371375080447864043704443832883878176942523235360430575644792184786706982848387200926575803737830233794788090059368953234970799945081119038967640880074652742780142494579258788820056842838115669472196386865459400540160"), "1e300 exact");}
    context = "";}

// A random std::format spec [[fill]align][sign][#][0][width][type] with a literal width of 1-60 or none.
static std::string randSpec() {
    static const char fills[] = " *0#+-x<^_~", *types = "dbBoxX";
    std::string s; int a = ri(3);
    if (a == 2) { s += fills[ri(int(sizeof fills) - 1)]; }
    if (a) { s += "<>^"[ri(3)]; }
    if (int g = ri(5); g >= 2) { s += " +-"[g - 2]; }
    if (ri(2)) { s += '#'; }
    if (ri(3) == 0) { s += '0'; }
    if (ri(3)) { s += std::to_string(1 + ri(60)); }
    if (int t = ri(7); t) { s += types[t - 1]; }
    return s;}
template<typename T> static std::string fmt(const std::string &spec, const T &x) { return std::vformat("{:" + spec + "}", std::make_format_args(x)); }
template<typename T> static bool rejects(const std::string &spec, const T &x) { try { (void)fmt(spec, x); } catch (const std::format_error &) { return true; } return false; }

// toChars against canonical digit strings, read-back, std::to_chars and exact-fit buffers; toString(base, lower); std::format against
// the native integer formatter of libstdc++ (lng, lll, ulll), literal format strings, the sentinel and rejected specs.
static void textOutput() {
    context = "toChars";
    InfInt one = 1;
    vector<InfInt> vals = {0, 1, -1, InfInt("inf"), InfInt("-inf"), InfInt(~ulng(0)), -InfInt(~ulng(0)), one << 64, -(one << 64), (one << 256) - 1, -(one << 256), InfInt(LLONG_MIN), InfInt(35), InfInt(-36)};
    for (int t = 0; t < sz(40, 200); ++t) { vals.push_back(randSigned(t % 4 == 0 ? 5000 : t % 4 == 1 ? 64 : t % 4 == 2 ? 300 : 1500)); }
    for (const InfInt &v : vals) {
        bool neg = v.sgn == -1 && !v.isNil(); InfInt mag = abs(v); ulng small = 0; bool word = !v.is_inf && mag.tryCast(small);
        for (uint base = 2; base <= 36; ++base) {
            std::string upper = v.toString(base);
            for (bool lower : {false, true}) {
                std::string s = v.toString(base, lower); int len = int(s.size());
                expect(s, lower ? lowered(upper) : upper, "toString(base, lower) is the lowercased toString(base)");
                if (v.is_inf) { expect(s, std::string(v.sgn == 1 ? "inf" : "-inf"), "sentinel text"); }
                else {
                    bool ok = len > neg && (!neg || s[0] == '-') && (s[size_t(neg)] != '0' || len == 1);
                    for (int i = neg; i < len && ok; ++i) {
                        char c = s[size_t(i)]; uint d = c >= '0' && c <= '9' ? uint(c - '0') : lower && c >= 'a' && c <= 'z' ? uint(c - 'a' + 10) : !lower && c >= 'A' && c <= 'Z' ? uint(c - 'A' + 10) : 99;
                        ok = d < base;}
                    require(ok, "canonical digits base " + std::to_string(base) + ": " + s.substr(0, 80)); expect(InfInt(s, base), v, "read back base " + std::to_string(base));
                    if (word) {
                        char nb[80]; std::string ref = (neg ? "-" : "") + std::string(nb, std::to_chars(nb, nb + 80, small, int(base)).ptr);
                        if (!lower) { for (char &c : ref) { c = char(std::toupper(uint8_t(c))); } }
                        expect(s, ref, "std::to_chars of one word base " + std::to_string(base));}}
                std::string buf(size_t(len) + 8, '\x7f');
                auto [p, ec] = v.toChars(buf.data(), buf.data() + len, base, lower);
                require(ec == std::errc() && p == buf.data() + len && buf.compare(0, size_t(len), s) == 0 && buf.substr(size_t(len)) == std::string(8, '\x7f'), "exact-fit toChars base " + std::to_string(base));
                std::string shortBuf(size_t(len) + 8, '\x7f');
                auto [q, ec2] = v.toChars(shortBuf.data(), shortBuf.data() + len - 1, base, lower);
                require(ec2 == std::errc::value_too_large && q == shortBuf.data() + len - 1 && shortBuf == std::string(size_t(len) + 8, '\x7f'), "toChars one byte short base " + std::to_string(base));
                auto [e, ec3] = v.toChars(shortBuf.data(), shortBuf.data(), base, lower); require(ec3 == std::errc::value_too_large && e == shortBuf.data() && shortBuf == std::string(size_t(len) + 8, '\x7f'), "toChars into an empty range");
                int bound = v.charsBound(base); require(bound >= len, "charsBound(" + std::to_string(base) + ") = " + std::to_string(bound) + " below length " + std::to_string(len));
                if (std::has_single_bit(base) && !v.is_inf) { expect(bound, len + !neg, "charsBound exact (digits plus a sign slot) in base " + std::to_string(base)); }
                if (v.is_inf) { expect(bound, 4, "sentinel charsBound"); }}}}
    context = "format";
    for (int t = 0; t < sz(500, 5000); ++t) {
        std::string spec = randSpec();
        lll x = t % 3 == 0 ? lll(rng() >> ri(64)) : lll(((ulll(rng()) << 64) | rng()) >> (1 + ri(127))); if (ri(2)) { x = -x; } if (t == 1) { x = lll(ulll(1) << 127); } if (t == 2) { x = 0; }
        InfInt X = x; expect(fmt(spec, X), fmt(spec, x), "std::format(\"{:" + spec + "}\") of " + X.toString() + " against lll");
        lng y = lng(x); InfInt Y = y; expect(fmt(spec, Y), fmt(spec, y), "std::format(\"{:" + spec + "}\") of " + Y.toString() + " against lng");
        ulll u = ulll(x); UInt<2> U = u; expect(fmt(spec, U), fmt(spec, u), "std::format(\"{:" + spec + "}\") of UInt<2> " + InfInt(U).toString() + " against ulll");
        UInt<4> V(X); expect(fmt(spec, V), fmt(spec, InfInt(V)), "UInt<4> formats as its InfInt value");}
    InfInt a = 255, b = -42, z = 0, eight = 8, pi("inf"), ni("-inf"), big = (one << 300) - 1; u256 m1 = u256(InfInt(-1)); UInt<1> seven = 7; UInt<2> e8 = 8;
    expect(std::format("{:#x}", a), std::string("0xff"), "{:#x}"); expect(std::format("{:#X}", a), std::string("0XFF"), "{:#X}"); expect(std::format("{:#b}", eight), std::string("0b1000"), "{:#b}"); expect(std::format("{:#B}", eight), std::string("0B1000"), "{:#B}");
    expect(std::format("{:#o}", eight), std::string("010"), "{:#o}"); expect(std::format("{:#o}", z), std::string("0"), "{:#o} of zero"); expect(std::format("{:*^9}", b), std::string("***-42***"), "{:*^9}"); expect(std::format("{:+08}", eight), std::string("+0000008"), "{:+08}");
    expect(std::format("{:#010X}", a), std::string("0X000000FF"), "{:#010X}"); expect(std::format("{:<05}", eight), std::string("8    "), "{:<05}"); expect(std::format("{: }|{:-}|{}", eight, eight, b), std::string(" 8|8|-42"), "signs");
    expect(std::format("{}", big), big.toString(), "{} of 300 bits"); expect(std::format("{:X}", big), big.toString(16), "{:X} of 300 bits"); expect(std::format("{:x}", m1), std::string(64, 'f'), "u256 {:x}"); expect(std::format("{:>4}|{}", seven, e8), std::string("   7|8"), "UInt<1>, UInt<2>");
    expect(std::format("{}|{}|{:x}|{:>6}|{:<6}|", pi, ni, pi, pi, ni), std::string("inf|-inf|inf|   inf|-inf  |"), "sentinel format");
    for (const char *bad : {"q", ".3", "L", "c", "{}", "5.2", "#q", "=5", "dd", "+-5", "##x", "Ld", "s", "e", "00", "65536", "9999999999", "2147483648"}) { require(rejects(bad, a) && rejects(bad, m1), std::string("format_error for spec ") + bad); }
    context = "";}

static void smallExhaustive() {
    context = "small exhaustive";
    const int R = full() ? 40 : 16;
    for (lng a = -R; a <= R; ++a) {
        InfInt A = a;
        require(bitLength(A) == (a == 0 ? 0 : 64 - std::countl_zero(ulng(a < 0 ? -a : a))), "bitLength");
        expect(~A, InfInt(~a), "not"); expect(-A, InfInt(-a), "neg"); expect(abs(A), InfInt(a < 0 ? -a : a), "abs");
        require(popcount(A) == std::popcount(ulng(a < 0 ? -a : a)), "popcount magnitude"); require(ctz(A) == (a == 0 ? 0 : std::countr_zero(ulng(a < 0 ? -a : a))), "ctz");
        for (int s = 0; s <= 70; ++s) { expect(A << s, InfInt(lll(a) << s), "shl"); expect(A >> s, InfInt(a >= 0 ? (s >= 63 ? 0 : a >> s) : (s >= 63 ? -1 : a >> s)), "shr floor"); }
        for (int i = 0; i < 70; ++i) {
            bool bit = i >= 63 ? a < 0 : ((a >> i) & 1); require(testBit(A, i) == bit, "testBit");
            InfInt s = A; s.setBit(i); expect(s, InfInt(a | (i < 63 ? lng(1) << i : 0)) + (i >= 63 && a >= 0 ? InfInt(1) << i : InfInt(0)), "setBit");
            InfInt c = A; c.clearBit(i); expect(c, i < 63 ? InfInt(a & ~(lng(1) << i)) : (a < 0 ? A - (InfInt(1) << i) : A), "clearBit");
            InfInt f = A; f.flipBit(i); expect(f, bit ? c : s, "flipBit");
            int s1 = -1, s0 = -1; for (int j = i; j < 70 && (s1 < 0 || s0 < 0); ++j) { bool bj = j >= 63 ? a < 0 : ((a >> j) & 1); if (bj && s1 < 0) { s1 = j; } if (!bj && s0 < 0) { s0 = j; } }
            require(scan1(A, i) == s1, "scan1 " + std::to_string(a) + " " + std::to_string(i)); if (a >= 0) { require(scan0(A, i) == s0, "scan0 nonneg"); } else { require(scan0(A, i) == (s0 >= 63 ? -1 : s0), "scan0 negative"); }}
        for (lng b = -R; b <= R; ++b) {
            InfInt B = b;
            expect(A + B, InfInt(a + b), "add"); expect(A - B, InfInt(a - b), "sub"); expect(A * B, InfInt(a * b), "mul"); expect(A * b, InfInt(a * b), "mul scalar"); expect(b * A, InfInt(a * b), "scalar mul");
            expect(A & B, InfInt(a & b), "and"); expect(A | B, InfInt(a | b), "or"); expect(A ^ B, InfInt(a ^ b), "xor");
            require((A <=> B) == (a <=> b) && (A == B) == (a == b), "compare"); require(magCmp(A, B) == (std::abs(a) <=> std::abs(b)), "magCmp");
            if (b != 0) {
                expect(A / B, InfInt(a / b), "div"); expect(A % B, InfInt(a % b), "rem"); expect(A / b, InfInt(a / b), "div scalar"); expect(A % b, InfInt(a % b), "rem scalar");
                auto [q, r] = divMod(A, B); expect(q, InfInt(a / b), "divMod q"); expect(r, InfInt(a % b), "divMod r"); auto [qs, rs] = divModSlow(A, B); expect(qs, q, "slow q"); expect(rs, r, "slow r");
                lng fq = floorDiv(a, b), fr = a - fq * b; expect(divModFloor(A, B), pair<InfInt, InfInt>(fq, fr), "divModFloor");
                lng cq = -floorDiv(-a, b), cr = a - cq * b; expect(divModCeil(A, B), pair<InfInt, InfInt>(cq, cr), "divModCeil");
                lng er = ((a % b) + std::abs(b)) % std::abs(b), eq = (a - er) / b; expect(divModEuclid(A, B), pair<InfInt, InfInt>(eq, er), "divModEuclid");
                if (a % b == 0) { expect(divExact(A, B), InfInt(a / b), "divExact"); }
                lng g = std::gcd(a, b); expect(gcd(A, B), InfInt(g), "gcd"); expect(lcm(A, B), InfInt(g ? std::abs(a / g * b) : 0), "lcm");
                InfInt x, y; expect(exGcd(A, B, x, y), InfInt(g), "exGcd g"); expect(A * x + B * y, InfInt(g), "bezout");
                if (b != 0) { InfInt iv = inv(A, B); lng m = std::abs(b); if (m == 1) { expect(iv, InfInt(0), "inv mod 1"); } else if (std::gcd(a, m) != 1) { expect(iv, InfInt(-1), "no inverse"); } else { require(iv.sgn == 1 && iv < InfInt(m) && divModEuclid(iv * A, InfInt(m)).second == 1, "inverse"); } }
                if (b > 0 && (b & 1)) {
                    int j = jacobi(A, B), o = 0; lng am = ((a % b) + b) % b;
                    if (std::gcd(am, b) == 1) { o = -1; for (lng t = 0; t < b; ++t) { if (t * t % b == am) { o = 1; } } }
                    if (b == 1) { o = 1; }
                    bool prime = b > 1; for (lng d = 2; d * d <= b; ++d) { if (b % d == 0) { prime = false; } }
                    if (prime || b == 1) { require(j == o, "jacobi prime " + std::to_string(a) + "/" + std::to_string(b)); }}
                if (b >= 2 && a != 0) { InfInt rest = A; int cnt = removeFactor(rest, B); require(cnt >= 0 && rest * pow(B, cnt) == A && (rest % B).isNil() == false, "removeFactor"); }}
            if (b >= 0 && b <= 12) { lll p = 1; for (int i = 0; i < b; ++i) { p *= a; } expect(pow(A, b), InfInt(p), "pow"); }}}
    context = "";}

// Oracle sum through limb schoolbook addition and subtraction of magnitudes only.
static InfInt oadd(const InfInt &a, const InfInt &b) {
    if (a.isNil()) { return b; }
    if (b.isNil()) { return a; }
    vector<uint> x = a.toLimbs(), y = b.toLimbs(); int s = a.sgn; bool same = a.sgn == b.sgn;
    auto below = [](const vector<uint> &p, const vector<uint> &q) {
        if (p.size() != q.size()) { return p.size() < q.size(); }
        for (size_t i = p.size(); i-- > 0;) { if (p[i] != q[i]) { return p[i] < q[i]; } }
        return false;};
    if (!same && below(x, y)) { std::swap(x, y); s = b.sgn; }
    x.resize(max(x.size(), y.size()) + 1, 0); ulng c = 0;
    for (size_t i = 0; i < x.size(); ++i) {
        ulng t = (i < y.size() ? y[i] : 0) + c;
        if (same) { ulng v = x[i] + t; x[i] = uint(v); c = v >> 32; }
        else { c = ulng(x[i]) < t; x[i] = uint(ulng(x[i]) - t); }}
    return InfInt::fromLimbs(x, s);}

// + and - on lvalues (one-pass combine), temporaries (compound forms) and chains, against the limb oracle: carries and borrows across
// the top word at 1-6 words (the four-word inline limit included), a - a, a + ~a, a + a.
static void addition() {
    context = "addition oracle";
    for (int t = 0; t < sz(300, 3000); ++t) { lng x = lng(rng() >> ri(64)) * (ri(2) ? -1 : 1), y = lng(rng() >> ri(64)) * (ri(2) ? -1 : 1); expect(oadd(InfInt(x), InfInt(y)), InfInt(lll(x) + y), "limb oracle against 128-bit sums"); }
    vector<InfInt> vals = {0, 1, -1, 2, -2};
    for (int k = 1; k <= 6; ++k) { InfInt p = InfInt(1) << (64 * k); for (const InfInt &v : {p - 1, p, p + 1, p - 2, (p >> 1) - 1}) { vals.push_back(v); vals.push_back(-v); } }
    for (int t = 0; t < sz(20, 60); ++t) { vals.push_back(randSigned(t % 2 ? 64 * 5 : 400)); }
    for (size_t i = 0; i < vals.size(); ++i) {
        const InfInt &a = vals[i]; context = "addition a=" + a.toString(16);
        expect(a + ~a, InfInt(-1), "a + ~a"); expect(a - a, InfInt(0), "a - a"); expect(a + a, omul(a, InfInt(2)), "a + a"); expect(InfInt(a) - a, InfInt(0), "temporary - itself"); expect(InfInt(a) + a, omul(a, InfInt(2)), "temporary + itself");
        for (size_t j = 0; j < vals.size(); ++j) {
            const InfInt &b = vals[j]; InfInt s = oadd(a, b), d = oadd(a, -b), r = a + b; std::string what = " b=" + b.toString(16);
            expect(r, s, "lvalue + lvalue" + what); require(r.isNil() ? r.sgn == 1 : r.n.back() != 0, "canonical sum" + what);
            expect(a - b, d, "lvalue - lvalue" + what); expect(InfInt(a) + b, s, "temporary + lvalue" + what); expect(InfInt(a) - b, d, "temporary - lvalue" + what);
            expect(a + InfInt(b), s, "lvalue + temporary" + what); expect(a - InfInt(b), d, "lvalue - temporary" + what);
            InfInt c = a; c += b; expect(c, s, "+=" + what); c = a; c -= b; expect(c, d, "-=" + what);
            expect((a + b) + a, oadd(s, a), "(a + b) + a" + what); expect((a - b) - a, oadd(d, -a), "(a - b) - a" + what); expect(a + (b - a), oadd(a, oadd(b, -a)), "a + (b - a)" + what);}}
    context = "";}

static void multiplication() {
    context = "multiplication";
    using namespace infint_detail;
    vector<int> sizes = {1, 2, 3, 5, 7, 8, 15, 16, 17, 23, 24, 25, 31, 32, 33, 47, 48, 64, 95, 96, 97, 127, 128, 129, 200, 255, 256, 257};
    for (int t : {FFT - 1, FFT, FFT + 1, FFT_SQR - 1, FFT_SQR, FFT_SQR + 1, TOOM3 - 1, TOOM3, TOOM3 + 1, TOOM3_SQR, 2 * FFT + 3}) { if (full() || t <= 300) { sizes.push_back(t); } }
    if (full()) { sizes.push_back(3000); sizes.push_back(5001); }
    if (stress()) { sizes.push_back(20000); }
    for (int n : sizes) {
        for (int m : {1, 2, n / 2, n - 1, n, 2 * n + 1}) {
            if (m < 1 || (!full() && m > 300)) { continue; }
            for (int pattern = 0; pattern < 2; ++pattern) {
                context = "multiply words " + std::to_string(n) + "x" + std::to_string(m) + (pattern ? " ones" : " random");
                InfInt a = randBits(64 * n - ri(64), ri(2)), b = randBits(64 * m - ri(64), ri(2));
                if (pattern) { a = (InfInt(1) << (64 * n)) - 1; b = (InfInt(1) << (64 * m)) - 1; }
                InfInt e = omul(a, b);
                expect(a * b, e, "product"); expect(b * a, e, "product commuted");
                InfInt c = a; c *= b; expect(c, e, "compound product");
                if (n == m) { InfInt sq = omul(a, a); expect(a * a, sq, "square"); InfInt d = a; d *= d; expect(d, sq, "self square"); InfInt same = a; d = a; d *= same; expect(d, sq, "equal operand square"); }
                vector<W> x = wordsOf(abs(a)), y = wordsOf(abs(b)); if (x.size() < y.size()) { std::swap(x, y); }
                int nx = int(x.size()), ny = int(y.size()); vector<W> r(size_t(nx + ny), 0);
                auto check = [&](const char *name) { expect(ofWords(r), abs(e), name); };
                mulBasecase(r.data(), x.data(), nx, y.data(), ny); check("mulBasecase");
                if (ny > nx / 2) { std::fill(r.begin(), r.end(), 0); karatsuba(r.data(), x.data(), nx, y.data(), ny); check("karatsuba"); }
                if (nx == ny && nx >= 6) { toom3(r.data(), x.data(), y.data(), nx); check("toom3"); toom3Square(r.data(), x.data(), nx); expect(ofWords(r), omul(abs(a), abs(a)), "toom3Square"); karatsubaSquare(r.data(), x.data(), nx); expect(ofWords(r), omul(abs(a), abs(a)), "karatsubaSquare"); squareBasecase(r.data(), x.data(), nx); expect(ofWords(r), omul(abs(a), abs(a)), "squareBasecase"); }
                if (nx >= 2 * ny) { for (int ch : {1, 2, ny / 2 + 1, ny - 1, ny, ny + 1, int(std::bit_ceil(uint(nx + ny)) / 2) - ny, nx / 3 + 1, nx - 1, nx, nx + 5}) { if (ch >= 1) { unbalancedMul(r.data(), x.data(), nx, y.data(), ny, ch); check(("unbalancedMul chunks of " + std::to_string(ch)).c_str()); } } }
                if (nx + ny <= 20000) {
                    transformMul(r.data(), x.data(), nx, y.data(), ny); check("transformMul");
                    schonhageStrassen(r.data(), x.data(), nx, y.data(), ny); check("schonhageStrassen");
                    if (nx == ny) { schonhageStrassen(r.data(), x.data(), nx, x.data(), nx); expect(ofWords(r), omul(abs(a), abs(a)), "schonhageStrassen square"); }}
                if (nx == ny) { vector<W> tr(size_t(nx), 0); truncatedMul(tr.data(), x.data(), y.data(), nx); vector<W> low = wordsOf(abs(e)); low.resize(size_t(nx), 0); low.resize(size_t(norm(low.data(), nx))); expect(ofWords(tr), ofWords(low), "truncatedMul"); }}}}
    vector<pair<int, int>> wide = {{2 * FFT + 100, FFT + 1}, {4 * FFT + 3, FFT}};
    if (full()) { for (auto p : {pair{16391, 1025}, {12000, 700}, {9000, 2100}, {4100, 600}, {2600, 1300}, {1300, 1000}, {7000, 2100}, {16391, 2100}, {20000, 2048}}) { wide.push_back(p); } }
    for (auto [n, m] : wide) {
        context = "unbalanced transform product " + std::to_string(n) + "x" + std::to_string(m);
        InfInt a = randBits(64 * n - ri(64)), b = randBits(64 * m - ri(64)); InfInt e = omul(a, b); expect(a * b, e, "product"); expect(b * a, e, "product commuted");}
    for (int t = 0; t < sz(60, 400); ++t) {
        int na = 1 + ri(full() ? 300 : 60), nb = 1 + ri(full() ? 300 : 60), lo = ri(na + nb), cnt = 1 + ri(na + nb + 2 - lo); bool ones = t % 5 == 0;
        InfInt a = ones ? (InfInt(1) << (64 * na)) - 1 : randBits(64 * na), b = ones ? (InfInt(1) << (64 * nb)) - 1 : randBits(64 * nb);
        vector<W> x = wordsOf(a), y = wordsOf(b); x.resize(size_t(na), 0); y.resize(size_t(nb), 0);
        vector<W> r(size_t(cnt), 0); infint_detail::middleProduct(r.data(), x.data(), na, y.data(), nb, lo, cnt);
        InfInt exact = omul(a, b) >> (64 * lo); exact &= (InfInt(1) << (64 * cnt)) - 1;
        InfInt diff = divModEuclid(exact - ofWords(r), InfInt(1) << (64 * cnt)).second;
        require(diff < (InfInt(min(na, nb)) << 64), "middleProduct error bound na=" + std::to_string(na) + " nb=" + std::to_string(nb) + " lo=" + std::to_string(lo));}
    for (int k = 0; k < 3; ++k) {
        context = "transform backend " + std::to_string(k);
        for (int t = 0; t < sz(12, 60); ++t) {
            int n = 1 + ri(full() ? 2000 : 200), m = 1 + ri(full() ? 2000 : 200); if (n < m) { std::swap(n, m); }
            bool same = t % 3 == 0, ones = t % 4 == 0;
            InfInt a = ones ? (InfInt(1) << (64 * n)) - 1 : randBits(64 * n), b = same ? a : (ones ? (InfInt(1) << (64 * m)) - 1 : randBits(64 * m)); if (same) { m = n; }
            vector<W> x = wordsOf(a), y = wordsOf(b); x.resize(size_t(n), 0); y.resize(size_t(m), 0);
            vector<uint> la(size_t(2 * n), 0), lb(size_t(2 * m), 0);
            for (int i = 0; i < n; ++i) { la[size_t(2 * i)] = uint(x[size_t(i)]); la[size_t(2 * i) + 1] = uint(x[size_t(i)] >> 32); }
            for (int i = 0; i < m; ++i) { lb[size_t(2 * i)] = uint(y[size_t(i)]); lb[size_t(2 * i) + 1] = uint(y[size_t(i)] >> 32); }
            int L = int(std::bit_ceil(uint(2 * (n + m)))); vector<W> lo(static_cast<size_t>(L)), hi(static_cast<size_t>(L)); const uint *pb = same ? la.data() : lb.data();
            if (k == 0) { infint_detail::ntt3Cyclic(lo.data(), hi.data(), la.data(), 2 * n, pb, 2 * m, L); }
            else if (k == 1) { infint_detail::ntt64Cyclic(lo.data(), hi.data(), la.data(), 2 * n, pb, 2 * m, L); }
            else { infint_detail::fftCyclic(lo.data(), hi.data(), la.data(), 2 * n, pb, 2 * m, L); }
            vector<W> r(size_t(n + m), 0); ulll carry = 0;
            for (int i = 0; i < n + m; ++i) {
                ulll c0 = lo[size_t(2 * i)] + (ulll(hi[size_t(2 * i)]) << 64) + carry; W low = uint(c0); carry = c0 >> 32;
                ulll c1 = lo[size_t(2 * i) + 1] + (ulll(hi[size_t(2 * i) + 1]) << 64) + carry; r[size_t(i)] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}
            expect(ofWords(r), omul(a, b), "cyclic backend product");}}
    if (full()) {
        for (auto [n, m] : {pair{16384, 16384}, pair{32768, 16384}}) {
            context = "double FFT limit " + std::to_string(n) + "x" + std::to_string(m);
            InfInt a = (InfInt(1) << (64 * n)) - 1, b = (InfInt(1) << (64 * m)) - 1;
            vector<W> x = wordsOf(a), y = wordsOf(b); vector<uint> la(size_t(2 * n)), lb(size_t(2 * m));
            for (int i = 0; i < n; ++i) { la[size_t(2 * i)] = uint(x[size_t(i)]); la[size_t(2 * i) + 1] = uint(x[size_t(i)] >> 32); }
            for (int i = 0; i < m; ++i) { lb[size_t(2 * i)] = uint(y[size_t(i)]); lb[size_t(2 * i) + 1] = uint(y[size_t(i)] >> 32); }
            int len = int(std::bit_ceil(uint(2 * (n + m)))); vector<W> lo(static_cast<size_t>(len)), hi(static_cast<size_t>(len));
            infint_detail::fftCyclic(lo.data(), hi.data(), la.data(), 2 * n, lb.data(), 2 * m, len);
            vector<W> r(size_t(n + m)); ulll carry = 0;
            for (int i = 0; i < n + m; ++i) {
                ulll c0 = lo[size_t(2 * i)] + (ulll(hi[size_t(2 * i)]) << 64) + carry; W low = uint(c0); carry = c0 >> 32;
                ulll c1 = lo[size_t(2 * i) + 1] + (ulll(hi[size_t(2 * i) + 1]) << 64) + carry; r[size_t(i)] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}
            expect(ofWords(r), omul(a, b), "all-ones double FFT at the exactness limit");}}
    context = "scalar products";
    for (int t = 0; t < sz(200, 2000); ++t) {
        InfInt a = randSigned(500); ulng u = rng() >> ri(64); lng s = lng(rng() >> ri(64)) * (ri(2) ? -1 : 1);
        expect(a * u, omul(a, InfInt(u)), "times ulng"); expect(a * s, omul(a, InfInt(s)), "times lng"); expect(a * 0, InfInt(0), "times zero"); expect(a * 1, a, "times one"); expect(a * -1, -a, "times minus one");
        int k = ri(64); expect(a * (ulng(1) << k), a << k, "times power of two");}
    context = "";}

static void division() {
    context = "division";
    using namespace infint_detail;
    vector<pair<int, int>> shapes = {{1, 1}, {2, 1}, {2, 2}, {3, 2}, {5, 3}, {8, 7}, {16, 8}, {39, 38}, {40, 39}, {41, 40}, {80, 40}, {81, 41}, {120, 60}, {200, 39}, {200, 41}, {300, 150}};
    if (full()) { for (auto p : {pair{1100, 1024}, {2048, 1023}, {2100, 1025}, {3000, 1500}, {5000, 2500}, {1030, 1030}, {100, 1}, {4000, 40}}) { shapes.push_back(p); } }
    if (stress()) { shapes.push_back({12000, 6000}); shapes.push_back({9000, 1100}); }
    for (int t = 0; t < sz(60, 400); ++t) {
        static const int edges[] = {0, 1, 31, 32, 33, 63, 64, 65}; int k = t < 8 ? edges[t] : ri(full() ? 3000 : 400); context = "divide by 2^" + std::to_string(k);
        InfInt b = InfInt(1) << k; if (ri(2)) { b = -b; }
        for (InfInt a : {randSigned(full() ? 6000 : 800), InfInt(0), b, -b, b - 1, randBits(k + 1)}) {
            auto [q, r] = divMod(a, b); auto [sq, sr] = divModSlow(a, b);
            expect(q, sq, "quotient"); expect(r, sr, "remainder"); expect(q * b + r, a, "identity"); require(magCmp(r, b) < 0 && (r.isNil() || r.sgn == a.sgn), "remainder range and sign");}}
    for (auto [qn, bn] : shapes) {
        for (int pattern = 0; pattern < 3; ++pattern) {
            context = "divide quotient " + std::to_string(qn) + " divisor " + std::to_string(bn) + " pattern " + std::to_string(pattern);
            InfInt b = pattern == 1 ? (InfInt(1) << (64 * bn)) - 1 : randBits(64 * bn - (pattern == 2 ? 1 + ri(63) : 0)), q = randBits(64 * qn - ri(64));
            for (InfInt r : {InfInt(0), InfInt(1), b / 2, b - 1}) {
                if (magCmp(r, b) >= 0) { continue; }
                for (int signs = 0; signs < 4; ++signs) {
                    InfInt bs = signs & 1 ? -b : b, a = omul(q, b) + r; if (signs & 2) { a = -a; }
                    InfInt eq = signs == 1 || signs == 2 ? -q : q, er = signs & 2 ? -r : r;
                    auto [dq, dr] = divMod(a, bs); expect(dq, eq, "divMod quotient"); expect(dr, er, "divMod remainder");
                    expect(a / bs, eq, "quotient"); expect(a % bs, er, "remainder"); InfInt c = a; c /= bs; expect(c, eq, "compound quotient"); c = a; c %= bs; expect(c, er, "compound remainder");
                    if (signs == 0 && r.isNil()) { expect(divExact(a, b), q, "divExact"); expect(divExact(-a, b), -q, "divExact negative"); expect(divExact(a, -b), -q, "divExact negative divisor"); }
                    if (signs == 0 && (pattern != 1 || qn + bn < 400)) { auto [sq, sr] = divModSlow(a, b); expect(sq, q, "divModSlow quotient"); expect(sr, r, "divModSlow remainder"); }
                    if (signs == 0 && bn >= 2 && pattern != 2) {
                        vector<W> x = wordsOf(a), y = wordsOf(b); int nx = int(x.size()), ny = int(y.size());
                        if (nx >= ny) {
                            vector<W> qq(size_t(nx - ny + 1), 0), rr(size_t(ny), 0);
                            divBasecase(qq.data(), rr.data(), x.data(), nx, y.data(), ny); expect(ofWords(qq), q, "divBasecase q"); expect(ofWords(rr), r, "divBasecase r");
                            if (ny >= 3) { divBurnikelZiegler(qq.data(), rr.data(), x.data(), nx, y.data(), ny); expect(ofWords(qq), q, "burnikelZiegler q"); expect(ofWords(rr), r, "burnikelZiegler r"); }
                            vector<W> iv(size_t(ny) + 1); reciprocal(iv.data(), y.data(), ny);
                            InfInt inv = ofWords(iv), B2 = InfInt(1) << (128 * ny); require(omul(b, inv) <= B2 && omul(b, inv + 1) > B2, "reciprocal exact floor");
                            divNewton(qq.data(), rr.data(), x.data(), nx, y.data(), ny, iv.data()); expect(ofWords(qq), q, "divNewton q"); expect(ofWords(rr), r, "divNewton r");}}}}}}
    context = "newtonRaphsonInv";
    for (int limbs : {1, 2, 3, 4, 7, 8, 33, 64, 65, 100}) {
        InfInt a = randBits(32 * limbs); a.setBit(32 * limbs - 1);
        InfInt inv = newtonRaphsonInv(a), B = InfInt(1) << (64 * limbs);
        require(omul(a, inv) <= B && omul(a, inv + 1) > B, "newtonRaphsonInv floor limbs=" + std::to_string(limbs));}
    context = "hensel";
    for (int t = 0; t < sz(60, 400); ++t) {
        int bn = 1 + ri(full() ? 120 : 30), qn = 1 + ri(full() ? 200 : 60);
        InfInt b = randBits(64 * bn - ri(64)); b.n[0] |= 1; InfInt q = randBits(64 * qn - ri(64)), a = omul(q, b);
        vector<W> x = wordsOf(a), y = wordsOf(b); vector<W> qq(x.size() - y.size() + 1);
        divExactHensel(qq.data(), x.data(), int(x.size()), y.data(), int(y.size())); expect(ofWords(qq), q, "divExactHensel");
        int sh = ri(70); expect(divExact(a << sh, b << sh), q, "divExact shifted"); expect(divExact(-(a << sh), b << sh), -q, "divExact shifted negative");}
    context = "scalar division";
    for (int t = 0; t < sz(200, 2000); ++t) {
        InfInt a = randSigned(700); ulng u = (rng() >> ri(64)) | 1; lng s = (lng(rng() >> ri(63)) | 1) * (ri(2) ? -1 : 1);
        auto [q, r] = divMod(a, InfInt(u)); expect(a / u, q, "div ulng"); expect(a % u, r, "rem ulng"); expect(omul(q, InfInt(u)) + r, a, "scalar identity");
        auto [qs, rs] = divMod(a, InfInt(s)); expect(a / s, qs, "div lng"); expect(a % s, rs, "rem lng");
        int k = 1 + ri(100); InfInt p = InfInt(1) << k; auto [qp, rp] = divMod(a, p); expect(a / (ulng(1) << min(k, 63)), k <= 63 ? qp : InfInt(a) / (ulng(1) << 63), "div power of two"); expect(a % p, rp, "rem power of two");}
    context = "remWord";
    for (int t = 0; t < sz(300, 3000); ++t) {
        int n = 1 + ri(t % 10 == 0 ? 200 : 8); vector<W> a(static_cast<size_t>(n)); for (W &x : a) { x = rng() >> ri(64); } if (t % 7 == 0) { a.back() = 0; }
        W d = t % 5 == 0 ? rng() | (W(1) << 63) : t % 5 == 1 ? W(1 + ri(16)) : t % 5 == 2 ? (rng() >> ri(64)) | 1 : t % 5 == 3 ? W(1) << ri(64) : ~W(0) - W(ri(3));
        vector<W> q(static_cast<size_t>(n)); W r = infint_detail::divWord(q.data(), a.data(), n, d); ulll h = 0;
        for (int i = n - 1; i >= 0; --i) { h = ((h << 64) | a[size_t(i)]) % d; }
        expect(infint_detail::remWord(a.data(), n, d), r, "remWord equals divWord's remainder, divisor " + std::to_string(d)); expect(r, W(h), "remainder against 128-bit Horner");
        expect(omul(ofWords(q), InfInt(d)) + InfInt(r), ofWords(a), "divWord identity");}
    context = "rounded division";
    for (int t = 0; t < sz(200, 2000); ++t) {
        InfInt a = randSigned(600), b = randSigned(300); if (b.isNil()) { b = 7; }
        auto [fq, fr] = divModFloor(a, b); require(omul(fq, b) + fr == a && (fr.isNil() || fr.sgn == b.sgn) && magCmp(fr, b) < 0, "floor identity");
        auto [cq, cr] = divModCeil(a, b); require(omul(cq, b) + cr == a && (cr.isNil() || cr.sgn == -b.sgn) && magCmp(cr, b) < 0, "ceil identity");
        auto [eq, er] = divModEuclid(a, b); require(omul(eq, b) + er == a && er.sgn == 1 && magCmp(er, b) < 0, "euclid identity");
        InfInt f = randSigned(40); if (magCmp(f, 1) <= 0) { f = 3; } int cnt = ri(6); InfInt x = omul(a.isNil() ? InfInt(1) : a, pow(f, cnt)); if ((a % f).isNil()) { continue; }
        InfInt y = x; expect(removeFactor(y, f), cnt, "removeFactor count"); expect(y, a.isNil() ? InfInt(1) : a, "removeFactor rest");}
    context = "";}

static void gcdFamily() {
    context = "gcd";
    using namespace infint_detail;
    vector<int> sizes = {1, 2, 3, 4, 5, 8, 9, 16, 33, 64, 100, 160, 161, 320, 321, 400};
    if (full()) { sizes.push_back(700); sizes.push_back(1300); sizes.push_back(2500); }
    if (stress()) { sizes.push_back(6000); }
    if (full()) { sizes.push_back(4500); }
    for (int limbs : sizes) {
        for (int t = 0; t < (limbs >= 4500 ? 1 : sz(2, 4)); ++t) {
            context = "gcd limbs " + std::to_string(limbs) + " trial " + std::to_string(t);
            InfInt g = randBits(1 + ri(32 * limbs)), x = randBits(32 * limbs - ri(32)), y = randBits(32 * limbs - ri(max(1, 32 * limbs / 2)));
            if (t == 1) { y = x + 1; }
            InfInt a = omul(g, x), b = omul(g, y), e = egcd(a, b);
            if (ri(2)) { a = -a; } if (ri(2)) { b = -b; }
            expect(gcd(a, b), e, "gcd"); expect(gcd(b, a), e, "gcd commuted"); expect(gcd(a, InfInt(0)), abs(a), "gcd with zero"); expect(gcd(InfInt(0), InfInt(0)), InfInt(0), "gcd zeros");
            require((e % g).isNil(), "gcd divisible by g");
            InfInt cx, cy, gg = exGcd(a, b, cx, cy); expect(gg, e, "exGcd g"); expect(omul(a, cx) + omul(b, cy), e, "bezout identity");
            if (!a.isNil() && !b.isNil()) { require(magCmp(cx, b / e) <= 0 && magCmp(cy, a / e) <= 0, "cofactor bounds"); }
            if (!e.isNil()) { expect(lcm(a, b), abs(omul(a / e, b)), "lcm"); }
            if (!b.isNil() && !a.isNil()) { InfInt m = abs(b), iv = inv(a, m); if (e == 1) { require(iv.sgn == 1 && iv < m && divModEuclid(omul(iv, a), m).second == 1, "modular inverse"); } else { expect(iv, InfInt(-1), "inverse sentinel"); } }
            vector<W> wa = wordsOf(a), wb = wordsOf(b);
            if (!wa.empty() && !wb.empty()) {
                vector<W> ca = wa, cb = wb; int len = gcdBinary(ca.data(), int(ca.size()), cb.data(), int(cb.size())); ca.resize(size_t(len)); expect(ofWords(ca), e, "gcdBinary");}
            InfInt A, B, C, D, ha = abs(a), hb = abs(b); if (ha < hb) { std::swap(ha, hb); }
            InfInt oa = ha, ob = hb;
            if (InfInt::halfGcd<true>(ha, hb, A, B, C, D)) {
                expect(omul(A, oa) + omul(B, ob), ha, "halfGcd matrix row 1"); expect(omul(C, oa) + omul(D, ob), hb, "halfGcd matrix row 2");
                require(abs(omul(A, D) - omul(B, C)) == 1, "halfGcd unimodular"); require(ha >= hb && hb.sgn == 1, "halfGcd ordered");
                require(hb.isNil() || int(hb.n.size()) <= (int(oa.n.size()) + 1) / 2, "halfGcd halves"); expect(egcd(ha, hb), e, "halfGcd keeps gcd");}
            ha = abs(a); hb = abs(b); if (ha < hb) { std::swap(ha, hb); } oa = ha; ob = hb; A = 1; B = 0; C = 0; D = 1;
            if (InfInt::lehmerHalfGcd<true>(ha, hb, A, B, C, D)) { expect(omul(A, oa) + omul(B, ob), ha, "lehmer row 1"); expect(omul(C, oa) + omul(D, ob), hb, "lehmer row 2"); expect(egcd(ha, hb), e, "lehmer keeps gcd"); require(ha.sgn == 1 && hb.sgn == 1, "lehmer nonnegative"); }}}
    context = "gcd small values";
    for (int t = 0; t < sz(300, 3000); ++t) { ulng x = rng() >> ri(64), y = rng() >> ri(64); expect(gcd(InfInt(x), InfInt(y)), InfInt(std::gcd(x, y)), "gcd 64-bit"); InfInt cx, cy; expect(exGcd(InfInt(x), -InfInt(y), cx, cy), InfInt(std::gcd(x, y)), "exGcd 64-bit"); expect(omul(InfInt(x), cx) - omul(InfInt(y), cy), InfInt(std::gcd(x, y)), "bezout 64-bit"); }
    context = "";}

static void modularPower() {
    context = "powMod";
    auto refPow = [](InfInt a, InfInt e, const InfInt &m) { InfInt r = 1; a = divModEuclid(a, m).second; while (!e.isNil()) { if (e.n[0] & 1) { r = divModEuclid(omul(r, a), m).second; } a = divModEuclid(omul(a, a), m).second; e >>= 1; } return r; };
    vector<int> mbits = {2, 31, 32, 33, 63, 64, 65, 127, 128, 129, 200, 500}; if (full()) { mbits.push_back(1030); mbits.push_back(4000); }
    for (int mb : mbits) {
        for (int t = 0; t < sz(3, 6); ++t) {
            InfInt m = randBits(mb); if (t % 2) { m.n[0] |= 1; } else { m.n[0] &= ~W(1); m.trim(); if (m.isNil()) { m = 2; } }
            int eb = t < 3 ? 1 + ri(70) : 1 + ri(full() ? 600 : 200); InfInt e = randBits(eb), a = randSigned(mb + 30);
            context = "powMod modulus bits " + std::to_string(mb) + " exponent bits " + std::to_string(eb);
            expect(powMod(a, e, m), refPow(a, e, m), "windowed power"); expect(powMod(a, e, -m), refPow(a, e, m), "negative modulus");
            InfInt ones = (InfInt(1) << eb) - 1; expect(powMod(a, ones, m), refPow(a, ones, m), "all-ones exponent");
            InfInt sparse = (InfInt(1) << (eb - 1)) + 1; expect(powMod(a, sparse, m), refPow(a, sparse, m), "sparse exponent");
            expect(powMod(a, InfInt(0), m), m == 1 ? InfInt(0) : InfInt(1), "zero exponent"); expect(powMod(a, InfInt(1), m), divModEuclid(a, m).second, "unit exponent");
            InfInt iv = inv(a, m); if (iv == -1) { expect(powMod(a, -e, m), InfInt(-1), "negative exponent sentinel"); } else { expect(powMod(a, -e, m), refPow(iv, e, m), "negative exponent"); }}}
    if (full()) {
        context = "powMod cached reciprocal";
        InfInt m = randBits(64 * 800); m.n[0] &= ~W(1); InfInt e = randBits(64), a = randSigned(64 * 801);
        expect(powMod(a, e, m), refPow(a, e, m), "even modulus above NEWTON_DIV words");}
    expect(powMod(InfInt(3), InfInt(-2), InfInt(7)), InfInt(4), "3^-2 mod 7"); expect(powMod(InfInt(0), InfInt(0), InfInt(17)), InfInt(1), "0^0");
    context = "";}

static void roots() {
    context = "roots";
    for (int t = 0; t < sz(200, 1500); ++t) {
        int bits = 1 + ri(t % 10 == 0 ? (full() ? 6000 : 1500) : 300);
        InfInt a = randBits(bits); if (t % 7 == 0) { a = omul(a, a) + (ri(2) ? 0 : 1); }
        if (t % 5 == 0) { a = (InfInt(1) << bits) - 1; if (t % 10 == 0) { a = omul(a, a); } }
        auto [s, r] = isqrtRem(a); require(omul(s, s) <= a && omul(s + 1, s + 1) > a && omul(s, s) + r == a, "isqrtRem identity"); expect(sqrt(a), s, "sqrt");
        expect(isPerfectSquare(a), r.isNil(), "isPerfectSquare"); expect(isPerfectSquare(omul(s, s)), true, "square of root"); expect(isPerfectSquare(-a - 1), false, "negative square");
        int k = 3 + ri(9); auto [ks, kr] = kthRootRem(a, k); require(pow(ks, k) <= a && pow(ks + 1, k) > a && pow(ks, k) + kr == a, "kthRootRem identity"); expect(kthRootRem(a, k).first, ks, "kthRootRem first");
        if (k & 1) { auto [ns, nr] = kthRootRem(-a, k); expect(ns, -ks, "odd root negative"); expect(nr, -kr, "odd root remainder"); }
        expect(kthRootRem(a, 1).first, a, "first root"); expect(kthRootRem(a, 2), isqrtRem(a), "second root");
        InfInt base = randBits(1 + ri(60)) + 2; int e = 2 + ri(7); InfInt p = pow(base, e);
        expect(isPerfectPower(p), true, "perfect power"); expect(isPerfectPower(p + 1), bitLength(p + 1) < 3 || isPerfectPower(p + 1), "perfect power plus one (sanity)");
        if (e & 1) { expect(isPerfectPower(-p), true, "negative odd power"); } else { expect(isPerfectPower(-p), isPerfectPower(-p), "negative even power well-defined"); }}
    for (int x = -300; x <= 300; ++x) {
        bool pp = x == 0 || x == 1 || x == -1; for (int b = 2; b * b <= std::abs(x) && !pp; ++b) { for (lng p = lng(b) * b; std::abs(p) <= std::abs(x); p *= b) { if (p == x || (x < 0 && p == -x && false)) { pp = true; } } }
        if (x < 0) { pp = false; for (int b = 2; b * b * b <= -x; ++b) { for (lng p = lng(b) * b * b; p <= -x; p *= lng(b) * b) { if (p == -x) { pp = true; } } } if (x == -1) { pp = true; } }
        expect(isPerfectPower(InfInt(x)), pp, "isPerfectPower " + std::to_string(x));
        if (x >= 0) { int s = int(std::sqrt(double(x))); while (s * s > x) { --s; } while ((s + 1) * (s + 1) <= x) { ++s; } expect(isPerfectSquare(InfInt(x)), s * s == x, "isPerfectSquare small"); }}
    expect(isPerfectPower(InfInt(1) << 1000), true, "2^1000"); expect(isPerfectPower((InfInt(1) << 1000) + 1), false, "2^1000 + 1"); expect(isPerfectPower(pow(InfInt(3), 101)), true, "3^101"); expect(isPerfectPower(pow(InfInt(3), 101) * 5), false, "3^101 * 5");
    context = "";}

static void numberTheory() {
    context = "primality small";
    const int LIM = full() ? 200000 : 20000;
    vector<bool> comp(size_t(LIM) + 1, false);
    for (int i = 2; i <= LIM; ++i) { if (!comp[size_t(i)]) { for (lng j = lng(i) * i; j <= LIM; j += i) { comp[size_t(j)] = true; } } }
    for (int i = -5; i <= LIM; ++i) { expect(isProbablePrime(InfInt(i)), i >= 2 && !comp[size_t(i)], "isProbablePrime " + std::to_string(i)); }
    for (ulng v : {561ULL, 1105ULL, 1729ULL, 2047ULL, 3277ULL, 4033ULL, 4681ULL, 8321ULL, 15841ULL, 29341ULL, 3215031751ULL, 3825123056546413051ULL, 18446744073709551557ULL, 18446744073709551615ULL}) { bool prime = v > 1; for (ulng d = 2; d * d <= v && d < 3000000; ++d) { if (v % d == 0) { prime = false; break; } } if (v >= 9000000000000ULL) { prime = v == 18446744073709551557ULL; } expect(isProbablePrime(InfInt(v)), prime, "pseudoprime " + std::to_string(v)); }
    context = "primality large";
    for (int e : {89, 107, 127, 521, 607}) { expect(isProbablePrime((InfInt(1) << e) - 1), true, "Mersenne prime"); }
    for (int e : {67, 101, 257, 509}) { expect(isProbablePrime((InfInt(1) << e) - 1), false, "Mersenne composite"); }
    expect(isProbablePrime(InfInt("170141183460469231731687303715884105727")), true, "2^127-1 decimal"); expect(isProbablePrime(InfInt("340282366920938463463374607431768211457")), false, "Fermat F7");
    for (int t = 0; t < sz(4, 20); ++t) {
        InfInt p = randBits(80 + ri(200)); p.n[0] |= 1; while (!isProbablePrime(p)) { p += 2; }
        InfInt q = randBits(80 + ri(200)); q.n[0] |= 1; while (!isProbablePrime(q)) { q += 2; }
        expect(isProbablePrime(omul(p, q)), false, "product of two primes"); expect(isProbablePrime(omul(p, p)), false, "prime square"); expect(isProbablePrime(p), true, "found prime");
        expect(isProbablePrime(omul(p, q) * 2 + 1), isProbablePrime(omul(p, q) * 2 + 1), "well-defined");
        for (int i = 0; i < 20; ++i) { InfInt a = randBits(bitLength(p) - 1); int j = jacobi(a, p); InfInt eu = powMod(a, (p - 1) >> 1, p); expect(InfInt(j == -1 ? p - 1 : j), eu, "jacobi Euler criterion"); }
        InfInt n = omul(p, q); for (int i = 0; i < 10; ++i) { InfInt a = randBits(bitLength(n) - 1), b = randBits(bitLength(n) - 1); expect(jacobi(omul(a, b), n), jacobi(a, n) * jacobi(b, n), "jacobi multiplicative"); expect(jacobi(a, n), jacobi(a, p) * jacobi(a, q), "jacobi factors"); expect(jacobi(a + n, n), jacobi(a, n), "jacobi periodic"); expect(jacobi(-a, n), jacobi(a, n) * ((n.n[0] & 3) == 1 ? 1 : -1), "jacobi negative"); }}
    context = "kronecker";
    for (int a = -60; a <= 60; ++a) {
        for (int n = -60; n <= 60; ++n) {
            int expected;
            if (n == 0) { expected = std::abs(a) == 1; }
            else {
                int m = std::abs(n), res = (n < 0 && a < 0) ? -1 : 1; int t = 0; while (m % 2 == 0) { m /= 2; ++t; }
                if (t && a % 2 == 0) { expected = 0; }
                else { int a8 = ((a % 8) + 8) % 8; if ((t & 1) && (a8 == 3 || a8 == 5)) { res = -res; } expected = res * jacobi(InfInt(a), InfInt(m)); }}
            expect(kronecker(InfInt(a), InfInt(n)), expected, "kronecker " + std::to_string(a) + " " + std::to_string(n));}}
    for (int a = -30; a <= 30; ++a) { for (int n = 1; n <= 61; n += 2) { int am = ((a % n) + n) % n; int j = 1, x = am, y = n; while (x) { int tz = std::countr_zero(uint(x)); x >>= tz; if ((tz & 1) && (y % 8 == 3 || y % 8 == 5)) { j = -j; } if (x % 4 == 3 && y % 4 == 3) { j = -j; } std::swap(x, y); x %= y; } if (y != 1) { j = 0; } expect(jacobi(InfInt(a), InfInt(n)), j, "jacobi small"); } }
    context = "";}

static void bits() {
    context = "bits model";
    for (int t = 0; t < sz(200, 2000); ++t) {
        int bits = 1 + ri(300); InfInt a = randSigned(bits); const int M = bits + 70; InfInt mag = abs(a);
        vector<bool> model(size_t(M), 0);
        for (int i = 0; i < M; ++i) { model[size_t(i)] = i / 64 < mag.n.size() && ((mag.n[i / 64] >> (i % 64)) & 1); }
        int pc = 0; for (int i = 0; i < M; ++i) { pc += model[size_t(i)]; }
        if (a.sgn == -1) { bool carry = true; for (int i = 0; i < M; ++i) { bool v = !model[size_t(i)]; if (carry) { carry = v; v = !v; } model[size_t(i)] = v; } }
        expect(popcount(a), pc, "popcount"); expect(bitLength(a), a.isNil() ? 0 : bitLength(mag), "bitLength");
        if (!a.isNil()) { int c = 0; while (!model[size_t(c)]) { ++c; } expect(ctz(a), c, "ctz"); }
        for (int q = 0; q < 12; ++q) {
            int i = ri(M), e1 = -1, e0 = -1;
            for (int j = i; j < M; ++j) { if (model[size_t(j)] && e1 < 0) { e1 = j; } if (!model[size_t(j)] && e0 < 0) { e0 = j; } }
            expect(testBit(a, i), bool(model[size_t(i)]), "testBit"); expect(scan1(a, i), e1, "scan1"); expect(scan0(a, i), e0, "scan0");
            InfInt s = a; s.setBit(i); expect(s, a | (InfInt(1) << i), "setBit"); InfInt c = a; c.clearBit(i); expect(c, a & ~(InfInt(1) << i), "clearBit"); InfInt f = a; f.flipBit(i); expect(f, a ^ (InfInt(1) << i), "flipBit");
            expect(testBit(s, i), true, "set then test"); expect(testBit(c, i), false, "clear then test");}
        InfInt b = randSigned(bits);
        expect(a & b, ~(~a | ~b), "de Morgan and"); expect(a ^ b, (a | b) & ~(a & b), "xor identity"); expect(a | b, ~(~a & ~b), "de Morgan or"); expect(a ^ a, InfInt(0), "self xor"); expect(a & a, a, "self and"); expect(~~a, a, "double not"); expect(~a, -a - 1, "not is -a-1");
        InfInt c = a; c &= b; expect(c, a & b, "and assign"); c = a; c |= b; expect(c, a | b, "or assign"); c = a; c ^= b; expect(c, a ^ b, "xor assign");
        int s = ri(200); expect((a << s) >> s, a, "shift round trip"); expect(a << s, omul(a, InfInt(1) << s), "shift is product"); expect(a >> s, divModFloor(a, InfInt(1) << s).first, "shift is floor division");}
    context = "";}

// Stream buffer with a 7-byte get area, so token reads cross underflow boundaries.
struct ChunkBuf : std::streambuf {
    std::string src; size_t pos = 0; char area[7];
    explicit ChunkBuf(std::string s) : src(std::move(s)) {}
    int_type underflow() override {
        if (gptr() < egptr()) { return traits_type::to_int_type(*gptr()); }
        if (pos >= src.size()) { return traits_type::eof(); }
        size_t k = min(sizeof area, src.size() - pos); std::memcpy(area, src.data() + pos, k); pos += k;
        setg(area, area, area + k);
        return traits_type::to_int_type(*gptr());}
};

static void samplingHashStreams() {
    context = "sampling";
    std::mt19937_64 g(seed); std::mt19937 g32{uint(seed)};
    for (int bits : {1, 2, 31, 32, 33, 64, 65, 100, 1000}) { for (int t = 0; t < 20; ++t) { InfInt x = InfInt::sampleBits(bits, g); expect(bitLength(x), bits, "sampleBits length"); InfInt y = InfInt::sampleBits(bits, g32); expect(bitLength(y), bits, "sampleBits 32-bit generator"); } }
    expect(InfInt::sampleBits(0, g), InfInt(0), "sampleBits zero");
    for (int t = 0; t < 200; ++t) { InfInt bound = randBits(1 + ri(200)); InfInt x = sample(bound, g); require(x.sgn == 1 && x < bound, "sample range"); InfInt y = sample(bound, g32); require(y.sgn == 1 && y < bound, "sample range 32"); }
    { const int B = 97, N = full() ? 200000 : 20000; vector<int> cnt(B); for (int i = 0; i < N; ++i) { ++cnt[size_t(ulng(sample(InfInt(B), g)))]; } double ex = double(N) / B, sd = std::sqrt(ex); for (int c : cnt) { require(std::fabs(c - ex) < 5 * sd + 1, "sample uniformity"); } }
    { std::mt19937_64 g1(7), g2(7); InfInt x = sample(InfInt("123456789123456789123456789"), g1), y = sample(InfInt("123456789123456789123456789"), g2); expect(x, y, "reproducible"); }
    context = "hash";
    { std::hash<InfInt> h; unordered_set<size_t> seen; int coll = 0; for (int t = 0; t < sz(20000, 200000); ++t) { InfInt a = (InfInt(t) << 200) | randBits(200); if (ri(2)) { a = -a; } if (!seen.insert(h(a)).second) { ++coll; } } require(coll <= 2, "hash collisions " + std::to_string(coll)); InfInt a = randSigned(100), b = a; require(h(a) == h(b) && h(a) != h(-a) && h(InfInt("inf")) != h(InfInt("-inf")), "hash equal values"); require(h(InfInt::fromLimbs(vector<uint>{5, 0, 0})) == h(InfInt(5)), "hash trimmed"); std::hash<u256> hu; require(hu(u256(5)) == hu(u256(InfInt(5))) && hu(u256(5)) != hu(u256(6)), "u256 hash"); }
    context = "streams";
    { std::istringstream in("  123  -456 +0 inf -INF 7"); InfInt a, b, c, d, e, f; in >> a >> b >> c >> d >> e >> f; require(a == 123 && b == -456 && c == 0 && d.is_inf && d.sgn == 1 && e.is_inf && e.sgn == -1 && f == 7, "stream input"); require(!(in >> a) && a == 123, "eof keeps value");
      std::istringstream bad("12x"); InfInt k = 9; bad >> k; require(bad.fail() && k == 9, "failbit keeps value");
      std::istringstream hx("ff -10 11"); InfInt p, q, r; hx >> InfInt::SetBase(16) >> p >> q >> InfInt::SetBase(2) >> r; require(p == 255 && q == -16 && r == 3, "SetBase input");
      std::istringstream flag("ff 17"); flag >> std::hex >> p >> std::dec >> q; require(p == 255 && q == 17, "std::hex input");
      std::ostringstream out; out << InfInt(255) << ' ' << InfInt::SetBase(16) << InfInt(-255) << ' ' << InfInt::SetBase(2) << InfInt(5) << ' ' << InfInt("inf") << ' ' << InfInt::SetBase(10) << std::hex << InfInt(255) << std::dec << ' ' << InfInt(-1); expect(out.str(), std::string("255 -FF 101 inf FF -1"), "stream output");
      std::ostringstream big; InfInt huge = (InfInt(1) << 20000) - 3; big << huge; expect(InfInt(big.str()), huge, "large stream round trip"); }
    context = "stream tokens";
    { auto token = [&](int len, uint base) { std::string s(1, InfInt::DIGITS[1 + ri(int(base) - 1)]); while (int(s.size()) < len) { s += InfInt::DIGITS[ri(int(base))]; } return s; };
      const std::string spaces = " \t\n\v\f\r";
      for (int len : {1, 6, 7, 8, 14, 15, 4095, 4096, 4097, 8192, 8193, 70000}) {
          std::string dec = token(len, 10), neg = "-" + token(len, 10), hex = token(len, 16), text;
          vector<std::string> parts = {dec, neg, hex, "5"};
          for (size_t i = 0; i < parts.size(); ++i) { text += parts[i]; if (i + 1 < parts.size()) { for (int k = 0, r = 1 + ri(4); k < r; ++k) { text += spaces[size_t(ri(6))]; } } }
          auto check = [&](std::istream &in, const char *how) {
              InfInt a, b, c, d; in >> a >> b >> InfInt::SetBase(16) >> c >> InfInt::SetBase(10) >> d;
              std::string what = std::string(how) + " token length " + std::to_string(len);
              require(!in.fail() && in.eof(), what + ": stream state (eofbit at the last token, no failbit)");
              expect(a.toString(), dec, what + " decimal"); expect(b.toString(), neg, what + " negative"); expect(c.toString(16), hex, what + " hex"); expect(d, InfInt(5), what + " last token");};
          std::istringstream ss(text); check(ss, "istringstream");
          ChunkBuf cb(text); std::istream cs(&cb); check(cs, "7-byte stream buffer");}
      for (const char *src : {"", "   ", " \n\t\r\v\f"}) {
          std::istringstream e(src); InfInt k = 5; e >> k; require(e.fail() && e.eof() && k == 5, "no token: failbit and eofbit, value kept");
          ChunkBuf cb(src); std::istream ce(&cb); ce >> k; require(ce.fail() && ce.eof() && k == 5, "no token through the 7-byte buffer");}
      for (const std::string &bad : {std::string("12x 34"), std::string("- 7"), std::string("+"), std::string("0x10"), token(5000, 10) + "g 1", std::string("1-2")}) {
          std::istringstream m(bad); InfInt k = 5; m >> k; require(m.fail() && k == 5, "malformed token keeps the value: " + bad.substr(0, 40));
          ChunkBuf cb(bad); std::istream cm(&cb); cm >> k; require(cm.fail() && k == 5, "malformed token through the 7-byte buffer: " + bad.substr(0, 40));}
      { std::istringstream wd("12345 6"); InfInt x, y, z; wd >> std::setw(3) >> x >> y >> z; require(x == 123 && y == 45 && z == 6 && !wd.fail(), "setw limits one token like a string extraction"); }
      { std::istringstream in("12 34"); InfInt a, b; in >> a >> b; require(a == 12 && b == 34 && in.eof() && !in.fail(), "token at end of input without a newline");
        std::istringstream nl("12\n"); nl >> a; require(a == 12 && !nl.eof() && !nl.fail(), "token before a newline leaves eofbit clear"); }
      { std::ostringstream o; o << std::setw(10) << InfInt(-42) << '|' << std::setfill('*') << std::left << std::setw(8) << InfInt(255) << '|' << std::right << std::setw(6) << std::hex << InfInt(255) << std::dec << '|' << std::setw(2) << InfInt(12345) << '|' << InfInt(7) << '|' << std::setw(5) << InfInt("inf");
        expect(o.str(), std::string("       -42|255*****|****FF|12345|7|**inf"), "stream width, fill and alignment");
        InfInt huge = (InfInt(1) << 9000) - 1; std::ostringstream w; w << huge; expect(w.str(), huge.toString(), "stream output without width is the text");
        std::ostringstream wide; wide << std::setw(3000) << std::setfill('.') << huge; expect(wide.str(), std::string(3000 - huge.toString().size(), '.') + huge.toString(), "stream width wider than a long value"); } }
    context = "";}

static void infinity() {
    context = "infinity";
    InfInt pi("inf"), ni("-inf"), z = 0, p = 7, n = -7;
    for (const InfInt &a : {z, p, n, pi, ni}) { require(+a == a && bool(a) == !a.isNil(), "unary plus and bool"); }
    require(pi > p && ni < n && pi != ni && pi == InfInt("INF") && pi > ni && !(pi < pi) && pi == pi && -pi == ni && abs(ni) == pi, "ordering");
    require(pi + p == pi && ni + p == ni && (pi + ni).isNil() && pi + pi == pi && (pi - pi).isNil() && pi - ni == pi && p - pi == ni, "additive");
    require(pi * n == ni && ni * n == pi && (pi * z).isNil() && pi * pi == pi && pi * -3 == ni, "multiplicative");
    require((p / pi).isNil() && pi / n == ni && pi / ni == -1 && pi / pi == 1 && (pi % p).isNil() && p % pi == p && pi / 3 == pi && (pi % 3).isNil(), "division");
    for (const InfInt &a : {pi, ni, z, p, n}) { for (const InfInt &b : {pi, ni, p, n}) { auto qr = divMod(a, b), sr = divModSlow(a, b); require(qr.first == a / b && qr.second == a % b && sr == qr, "divMod sentinel"); } }
    require(pow(ni, 2) == pi && pow(ni, 3) == ni && pow(pi, 0) == 1 && sqrt(pi) == pi, "powers");
    for (auto [a, b] : vector<pair<InfInt, InfInt>>{{pi, ni}, {pi, z}, {pi, n}, {z, ni}, {p, ni}, {pi, pi}}) { InfInt x, y, g = exGcd(a, b, x, y); require(g == gcd(a, b) && a * x + b * y == g, "exGcd sentinel"); }
    require(lcm(z, pi).isNil() && lcm(ni, z).isNil() && lcm(p, pi) == pi, "lcm sentinel"); require((++pi) == InfInt("inf") && (--ni) == InfInt("-inf") && (pi << 5) == pi && (ni >> 5) == ni, "increments and shifts");
    require(pi.toString() == "inf" && ni.toString(16) == "-inf" && pi.toDouble() == INFINITY && !isnan(ni.toDouble()), "conversions"); require(std::hash<InfInt>{}(pi) != std::hash<InfInt>{}(p), "hash");
    InfInt copy = pi; copy = 5; require(!copy.is_inf && copy == 5, "assignment clears"); copy = pi; require(copy.read("12") && copy == 12, "read clears");
    copy = pi; require(copy.read("123456789012345678901234567890") && copy == InfInt("123456789012345678901234567890"), "long decimal read clears the sentinel");
    copy = ni; std::istringstream in("100000000000000000000"); in >> copy; require(copy == InfInt("100000000000000000000"), "stream read clears the sentinel");
    copy = pi; require(copy.read("ff", 16) && copy == 255, "hex read clears the sentinel");
    expect(InfInt(24171).toString(36), std::string("INF"), "INF is a base-36 numeral"); expect(InfInt("INF", 36), InfInt(24171), "base-36 INF parses as a number"); expect(InfInt("inf", 24), InfInt(18 * 576 + 23 * 24 + 15), "base-24 inf parses as a number");
    require(InfInt("inf", 23).is_inf && InfInt("-INF", 2).is_inf && InfInt("Inf", 10).sgn == 1, "inf token in bases up to 23");
    require(!isqrtRem(InfInt(0)).first.is_inf && sqrt(pi) == pi, "sqrt sentinel");
    context = "";}

template<int N> static void uintType() {
    context = "UInt<" + std::to_string(N) + ">";
    InfInt M = InfInt(1) << (64 * N);
    auto wrap = [&](const InfInt &x) { return divModEuclid(x, M).second; };
    auto toU = [&](const InfInt &x) { return UInt<N>(x); };
    for (int t = 0; t < sz(300, 3000); ++t) {
        InfInt a = randSigned(64 * N + 10), b = randSigned(64 * N + 10); if (t % 3 == 0) { b = randBits(1 + ri(64)); } if (t % 11 == 0) { a = M - 1; }
        UInt<N> x = toU(a), y = toU(b);
        expect(InfInt(x), wrap(a), "conversion"); expect(InfInt(x + y), wrap(a + b), "add"); expect(InfInt(x - y), wrap(a - b), "sub"); expect(InfInt(x * y), wrap(omul(a, b)), "mul");
        expect(InfInt(-x), wrap(-a), "neg"); expect(InfInt(~x), wrap(~a), "not"); expect(InfInt(x & y), wrap(a) & wrap(b), "and"); expect(InfInt(x | y), wrap(a) | wrap(b), "or"); expect(InfInt(x ^ y), wrap(a) ^ wrap(b), "xor");
        expect(InfInt(mulHigh(x, y)), omul(wrap(a), wrap(b)) >> (64 * N), "mulHigh");
        if (!y) { y = 1; b = 1; }
        auto [q, r] = divMod(x, y); expect(InfInt(q), wrap(a) / wrap(b), "div"); expect(InfInt(r), wrap(a) % wrap(b), "mod"); expect(InfInt(x / y), InfInt(q), "operator/"); expect(InfInt(x % y), InfInt(r), "operator%");
        int s = ri(64 * N); expect(InfInt(x << s), wrap(wrap(a) << s), "shl"); expect(InfInt(x >> s), wrap(a) >> s, "shr");
        ulng d = (rng() >> ri(64)) | 1; UInt<N> z = x; ulng rem = z.divWord(d); expect(InfInt(z), wrap(a) / d, "divWord quotient"); expect(InfInt(rem), wrap(a) % d, "divWord remainder");
        require((x <=> y) == (wrap(a) <=> wrap(b)) && (x == y) == (wrap(a) == wrap(b)), "compare");
        expect(bitLength(x), bitLength(wrap(a)), "bitLength"); expect(popcount(x), popcount(wrap(a)), "popcount"); expect(ctz(x), ctz(wrap(a)), "ctz");
        uint base = 2 + ri(35); expect(x.toString(base), wrap(a).toString(base), "toString"); expect(x.toString(base, true), lowered(x.toString(base)), "toString lower"); expect(x.toString(base, true), wrap(a).toString(base, true), "toString lower against InfInt"); expect(UInt<N>(x.toString(base), base), x, "string round trip"); expect(ulng(x), ulng(wrap(a)), "low word");
        UInt<N> c = x; c += y; expect(c, x + y, "+="); c = x; c -= y; expect(c, x - y, "-="); c = x; c *= y; expect(c, x * y, "*="); c = x; c /= y; expect(c, x / y, "/="); c = x; c %= y; expect(c, x % y, "%="); c = x; c <<= s; expect(c, x << s, "<<="); c = x; c >>= s; expect(c, x >> s, ">>="); c = x; c &= y; expect(c, x & y, "&="); c = x; c |= y; expect(c, x | y, "|="); c = x; c ^= y; expect(c, x ^ y, "^=");
        c = x; c *= c; expect(c, x * x, "self square"); c = x; c += c; expect(c, x + x, "self add"); c = x; c -= c; expect(InfInt(c), InfInt(0), "self sub");}
    { UInt<N> a = lng(-1); expect(InfInt(a), M - 1, "negative integral"); UInt<N> b = ulll(1) << 70; expect(InfInt(b), N >= 2 ? InfInt(1) << 70 : InfInt(0), "128-bit integral"); UInt<N> c = lll(-5); expect(InfInt(c), M - 5, "negative 128-bit"); UInt<N> d = 7; expect(InfInt(d), InfInt(7), "small"); std::ostringstream o; o << d; expect(o.str(), std::string("7"), "stream"); }
    if constexpr (N == 2) { for (int t = 0; t < sz(300, 3000); ++t) { ulll a = (ulll(rng()) << 64 | rng()) >> ri(128), b = ((ulll(rng()) << 64 | rng()) >> ri(128)) | 1; UInt<2> x = a, y = b; require(ulng(x + y) == ulng(a + b) && ulng((x + y) >> 64) == ulng((a + b) >> 64), "ulll add"); require(ulng(x * y) == ulng(a * b) && ulng((x * y) >> 64) == ulng((a * b) >> 64), "ulll mul"); require(ulng(x / y) == ulng(a / b) && ulng((x / y) >> 64) == ulng((a / b) >> 64), "ulll div"); require(ulng(x % y) == ulng(a % b) && ulng((x % y) >> 64) == ulng((a % b) >> 64), "ulll mod"); } }
    context = "";}

// UInt<N> stream input: decimal, SetBase(16) and std::hex round trips, negative tokens wrap, the sentinel token fails and keeps the value.
template<int N> static void uintStreams() {
    context = "UInt<" + std::to_string(N) + "> streams";
    for (int t = 0; t < sz(50, 300); ++t) {
        InfInt v = randSigned(64 * N + 20); UInt<N> e(v), x, y;
        std::stringstream dec; dec << v << ' ' << -v << '\n' << e; UInt<N> z; dec >> x >> y >> z;
        require(!dec.fail(), "decimal tokens read"); expect(x, e, "decimal token wraps like UInt(InfInt)"); expect(y, UInt<N>(-v), "negated token wraps"); expect(z, e, "UInt output read back");
        std::stringstream hs; hs << InfInt::SetBase(16) << v << ' ' << e; hs >> x >> y; require(!hs.fail(), "SetBase(16) tokens read"); expect(x, e, "SetBase(16) token"); expect(y, e, "SetBase(16) UInt round trip");
        std::stringstream hx; hx << std::hex << v << ' ' << e; hx >> x >> y; require(!hx.fail(), "std::hex tokens read"); expect(x, e, "std::hex token"); expect(y, e, "std::hex UInt round trip");}
    { std::istringstream in("-1 inf 5 -INF 6 12x 7"); UInt<N> x = 9;
      in >> x; expect(InfInt(x), (InfInt(1) << (64 * N)) - 1, "-1 wraps to all ones");
      in >> x; require(in.fail() && InfInt(x) == (InfInt(1) << (64 * N)) - 1, "inf token sets failbit and keeps the value"); in.clear(); in >> x; require(!in.fail() && x == UInt<N>(5), "token after inf");
      in >> x; require(in.fail() && x == UInt<N>(5), "-INF token"); in.clear(); in >> x; require(x == UInt<N>(6), "token after -INF");
      in >> x; require(in.fail() && x == UInt<N>(6), "malformed token keeps the value"); in.clear(); in >> x; require(x == UInt<N>(7) && in.eof(), "token after a malformed one");}
    context = "";}

template<int M, int N> static void uintConvert() {
    InfInt mod = InfInt(1) << (64 * N);
    for (int t = 0; t < sz(30, 200); ++t) {
        InfInt v = t == 0 ? InfInt(-1) : t == 1 ? InfInt(0) : randSigned(64 * max(M, N) + 10);
        UInt<M> a(v); UInt<N> b(a); InfInt va(a); std::string what = "UInt<" + std::to_string(M) + "> -> UInt<" + std::to_string(N) + ">";
        expect(b, UInt<N>(va), what + " against UInt<N>(InfInt)");
        expect(InfInt(b), N >= M ? va : divModEuclid(va, mod).second, what + (N >= M ? " keeps the value" : " is the value modulo 2^(64 N)"));}}
template<int M> static void uintConvertFrom() { uintConvert<M, 1>(); uintConvert<M, 2>(); uintConvert<M, 4>(); uintConvert<M, 8>(); }
static void uintConversions() {
    context = "UInt<M> -> UInt<N>";
    uintConvertFrom<1>(); uintConvertFrom<2>(); uintConvertFrom<4>(); uintConvertFrom<8>();
    uintStreams<1>(); uintStreams<2>(); uintStreams<4>();
    context = "";}

static void aliasing() {
    context = "aliasing";
    for (int t = 0; t < sz(50, 300); ++t) {
        InfInt a = randSigned(400), e;
        InfInt c = a; c += c; expect(c, omul(a, InfInt(2)), "a += a"); c = a; c -= c; expect(c, InfInt(0), "a -= a"); c = a; c *= c; expect(c, omul(a, a), "a *= a");
        if (!a.isNil()) { c = a; c /= c; expect(c, InfInt(1), "a /= a"); c = a; c %= c; expect(c, InfInt(0), "a %= a"); }
        c = a; c &= c; expect(c, a, "a &= a"); c = a; c |= c; expect(c, a, "a |= a"); c = a; c ^= c; expect(c, InfInt(0), "a ^= a");
        { InfInt x("123456789012345678901234567890123"), y(-6); expect(removeFactor(x, x), 1, "removeFactor(x, x) count"); expect(x, InfInt(1), "removeFactor(x, x) rest"); expect(removeFactor(y, y), 1, "removeFactor(y, y) count"); expect(y, InfInt(1), "removeFactor(y, y) rest"); }
        InfInt x, y; expect(exGcd(a, a, x, y), abs(a), "exGcd(a, a)"); expect(gcd(a, a), abs(a), "gcd(a, a)"); if (!a.isNil()) { expect(divMod(a, a), pair<InfInt, InfInt>(1, 0), "divMod(a, a)"); }
        InfInt m = abs(a) + 2; InfInt r = powMod(m, m, m); expect(r, InfInt(0), "powMod(m, m, m)"); require(InfInt(5).read("17") && c.read("x") == false, "read alias");
        c = a; c = c * c; expect(c, omul(a, a), "c = c * c"); c = a; c = c + c; expect(c, a + a, "c = c + c"); c = a; c = -std::move(c); expect(c, -a, "negate moved");
        { const InfInt b0 = randSigned(t % 25 == 0 ? 64 * 1100 : 300), b = b0.isNil() ? InfInt(3) : b0, big = t % 25 == 0 ? randSigned(64 * 2300) : a;
          vector<InfInt> vals = {a, b, omul(a, b) + 1, big}; const vector<InfInt> keep = vals;
          for (auto [i, j] : {pair{0, 1}, {2, 1}, {2, 0}, {1, 1}, {0, 0}, {3, 1}, {3, 3}, {2, 2}}) {
              const InfInt &num = vals[size_t(i)], &den = vals[size_t(j)]; if (den.isNil()) { continue; }
              auto [dq, dr] = divMod(num, den); require(omul(dq, den) + dr == num && magCmp(dr, den) < 0 && (dr.isNil() || dr.sgn == num.sgn), "divMod of vector elements " + std::to_string(i) + ", " + std::to_string(j));
              expect(divModSlow(num, den), pair<InfInt, InfInt>(dq, dr), "divModSlow of vector elements"); require(vals == keep, "divMod leaves its operands unchanged");}}
        std::vector<InfInt> v(3, a); v[1] = v[0] + v[2]; expect(v[1], a + a, "vector elements");}
    context = "";}

static void invalid(const std::string &which) {
    InfInt a = 5, z = 0, inf("inf"), neg = -5;
    if (which == "div-zero") { InfInt r = a / z; (void)r; }
    else if (which == "rem-zero") { InfInt r = a % z; (void)r; }
    else if (which == "divmod-zero") { auto r = divMod(a, z); (void)r; }
    else if (which == "divmodslow-zero") { auto r = divModSlow(a, z); (void)r; }
    else if (which == "scalar-div-zero") { InfInt r = a / 0; (void)r; }
    else if (which == "divexact-zero") { InfInt r = divExact(a, z); (void)r; }
    else if (which == "divexact-inexact") { InfInt r = divExact(InfInt("123456789012345678901234567890123"), InfInt("98765432109876543210987654321")); (void)r; }
    else if (which == "divfloor-inf") { auto r = divModFloor(inf, a); (void)r; }
    else if (which == "shift-negative") { a <<= -1; }
    else if (which == "sqrt-negative") { InfInt r = sqrt(neg); (void)r; }
    else if (which == "root-even-negative") { auto r = kthRootRem(neg, 4); (void)r; }
    else if (which == "root-zero-k") { auto r = kthRootRem(a, 0); (void)r; }
    else if (which == "jacobi-even") { int r = jacobi(a, InfInt(10)); (void)r; }
    else if (which == "inv-zero-mod") { InfInt r = inv(a, z); (void)r; }
    else if (which == "powmod-zero-mod") { InfInt r = powMod(a, a, z); (void)r; }
    else if (which == "pow-negative") { InfInt r = pow(a, -1); (void)r; }
    else if (which == "read-base") { a.read("1", 1); }
    else if (which == "tostring-base") { std::string s = a.toString(37); (void)s; }
    else if (which == "todigits-base") { auto d = a.toDigits(1); (void)d; }
    else if (which == "ctor-string") { InfInt r("12x"); (void)r; }
    else if (which == "ctor-digits") { InfInt r(vector<uint>{1, 10}, 10); (void)r; }
    else if (which == "fromlimbs-sign") { InfInt r = InfInt::fromLimbs(vector<uint>{1}, 2); (void)r; }
    else if (which == "fromdouble-nan") { InfInt r = InfInt::fromDouble(NAN); (void)r; }
    else if (which == "bitwise-inf") { InfInt r = inf & a; (void)r; }
    else if (which == "testbit-negative-index") { bool b = testBit(a, -1); (void)b; }
    else if (which == "popcount-inf") { int r = popcount(inf); (void)r; }
    else if (which == "sample-zero") { std::mt19937_64 g; InfInt r = sample(z, g); (void)r; }
    else if (which == "removefactor-one") { InfInt x = a; removeFactor(x, InfInt(1)); }
    else if (which == "newton-unnormalized") { InfInt r = newtonRaphsonInv(a); (void)r; }
    else if (which == "uint-shift") { u256 x = 1; x <<= 256; }
    else if (which == "uint-divword") { u256 x = 1; x.divWord(0); }
    else if (which == "uint-div-zero") { u256 x = 1, y; u256 r = x / y; (void)r; }
    else if (which == "uint-from-inf") { u256 x(inf); (void)x; }
    else if (which == "isprime-inf") { bool r = isProbablePrime(inf); (void)r; }
    else if (which == "tolimbs-inf") { auto r = inf.toLimbs(); (void)r; }
    else if (which == "tochars-base") { char buf[64]; auto r = a.toChars(buf, buf + 64, 37); (void)r; }
    else if (which == "charsbound-base") { int r = a.charsBound(1); (void)r; }
    else if (which == "rshift-negative") { a >>= -1; }
    else if (which == "setbit-negative") { a.setBit(-1); }
    else if (which == "clearbit-negative") { a.clearBit(-1); }
    else if (which == "scan0-negative") { int r = scan0(a, -1); (void)r; }
    else if (which == "scan1-negative") { int r = scan1(a, -1); (void)r; }
    else if (which == "not-inf") { InfInt r = ~inf; (void)r; }
    else if (which == "ctz-inf") { int r = ctz(inf); (void)r; }
    else if (which == "bitlength-inf") { int r = bitLength(inf); (void)r; }
    else if (which == "issquare-inf") { bool r = isPerfectSquare(inf); (void)r; }
    else if (which == "kronecker-inf") { int r = kronecker(inf, a); (void)r; }
    else if (which == "todigits-inf") { auto d = inf.toDigits(10); (void)d; }
    else if (which == "root-inf") { auto r = kthRootRem(inf, 3); (void)r; }
    else if (which == "uint-rshift") { u256 x = 1; x >>= 256; }
    else if (which == "removefactor-zero") { InfInt x = z; removeFactor(x, a); }
    else if (which == "uint-ctor-string") { u256 x("12x", 10); (void)x; }
    else if (which == "bitlength-words") { InfInt x = 1; x.n.setSize(1 << 25); int r = bitLength(x); (void)r; }
    else if (which == "popcount-words") { InfInt x = 1; x.n.setSize(1 << 25); int r = popcount(x); (void)r; }
    else { fail("unknown invalid case " + which); }
    std::cerr << "precondition case " << which << " did not abort\n";
    std::exit(2);}

// Text protocol for the Python oracle: one command per line, one result line each; all numbers decimal unless the command says otherwise.
static void protocol() {
    std::ios_base::sync_with_stdio(false); std::cin.tie(nullptr);
    std::string op;
    auto out = [](const auto &x) { std::cout << x; };
    while (std::cin >> op) {
        std::string sa, sb, sc;
        if (op == "add" || op == "sub" || op == "mul" || op == "div" || op == "and" || op == "or" || op == "xor" || op == "cmp" || op == "gcd" || op == "lcm" || op == "divfloor" || op == "divceil" || op == "diveuclid" || op == "divexact" || op == "exgcd" || op == "inv" || op == "jacobi" || op == "kronecker" || op == "divslow" || op == "removefactor") {
            std::cin >> sa >> sb; InfInt a(sa), b(sb);
            if (op == "add") { out(a + b); } else if (op == "sub") { out(a - b); } else if (op == "mul") { out(a * b); }
            else if (op == "div") { auto [q, r] = divMod(a, b); std::cout << q << ' ' << r; } else if (op == "divslow") { auto [q, r] = divModSlow(a, b); std::cout << q << ' ' << r; }
            else if (op == "divfloor") { auto [q, r] = divModFloor(a, b); std::cout << q << ' ' << r; } else if (op == "divceil") { auto [q, r] = divModCeil(a, b); std::cout << q << ' ' << r; } else if (op == "diveuclid") { auto [q, r] = divModEuclid(a, b); std::cout << q << ' ' << r; }
            else if (op == "divexact") { out(divExact(a, b)); } else if (op == "and") { out(a & b); } else if (op == "or") { out(a | b); } else if (op == "xor") { out(a ^ b); }
            else if (op == "cmp") { out(a < b ? -1 : a > b ? 1 : 0); } else if (op == "gcd") { out(gcd(a, b)); } else if (op == "lcm") { out(lcm(a, b)); }
            else if (op == "exgcd") { InfInt x, y, g = exGcd(a, b, x, y); std::cout << g << ' ' << x << ' ' << y; } else if (op == "inv") { out(inv(a, b)); }
            else if (op == "jacobi") { out(jacobi(a, b)); } else if (op == "kronecker") { out(kronecker(a, b)); } else if (op == "removefactor") { int c = removeFactor(a, b); std::cout << c << ' ' << a; }}
        else if (op == "not" || op == "sqrt" || op == "issquare" || op == "ispower" || op == "isprime" || op == "bitlength" || op == "ctz" || op == "popcount" || op == "todouble" || op == "hash" || op == "neg" || op == "inc" || op == "dec") {
            std::cin >> sa; InfInt a(sa);
            if (op == "not") { out(~a); } else if (op == "sqrt") { auto [s, r] = isqrtRem(a); std::cout << s << ' ' << r; } else if (op == "issquare") { out(int(isPerfectSquare(a))); } else if (op == "ispower") { out(int(isPerfectPower(a))); }
            else if (op == "isprime") { out(int(isProbablePrime(a))); } else if (op == "bitlength") { out(bitLength(a)); } else if (op == "ctz") { out(ctz(a)); } else if (op == "popcount") { out(popcount(a)); }
            else if (op == "todouble") { char buf[64]; std::snprintf(buf, sizeof buf, "%.17g", a.toDouble()); out(buf); } else if (op == "hash") { out(std::hash<InfInt>{}(a)); } else if (op == "neg") { out(-a); } else if (op == "inc") { out(++a); } else { out(--a); }}
        else if (op == "shl" || op == "shr" || op == "pow" || op == "kthroot" || op == "testbit" || op == "setbit" || op == "clearbit" || op == "flipbit" || op == "scan0" || op == "scan1" || op == "tobase" || op == "todigits" || op == "muls" || op == "divs") {
            lng k; std::cin >> sa >> k; InfInt a(sa);
            if (op == "shl") { out(a << int(k)); } else if (op == "shr") { out(a >> int(k)); } else if (op == "pow") { out(pow(a, k)); } else if (op == "kthroot") { auto [s, r] = kthRootRem(a, int(k)); std::cout << s << ' ' << r; }
            else if (op == "testbit") { out(int(testBit(a, int(k)))); } else if (op == "setbit") { out(a.setBit(int(k))); } else if (op == "clearbit") { out(a.clearBit(int(k))); } else if (op == "flipbit") { out(a.flipBit(int(k))); }
            else if (op == "scan0") { out(scan0(a, int(k))); } else if (op == "scan1") { out(scan1(a, int(k))); } else if (op == "tobase") { out(a.toString(uint(k))); }
            else if (op == "todigits") { vector<uint> d = a.toDigits(uint(k)); for (size_t i = 0; i < d.size(); ++i) { std::cout << (i ? " " : "") << d[i]; } }
            else if (op == "muls") { out(a * k); } else { auto [q, r] = divMod(a, InfInt(k)); std::cout << a / k << ' ' << a % k << ' ' << q << ' ' << r; }}
        else if (op == "addchain") {   // sums of temporaries: (a + b) + c, (a - b) - c, a + (b - c), moved a - b
            std::cin >> sa >> sb >> sc; InfInt a(sa), b(sb), c(sc), m = a; std::cout << (a + b) + c << ' ' << (a - b) - c << ' ' << a + (b - c) << ' ' << std::move(m) - b;}
        else if (op == "tochars") {
            lng b, low; std::cin >> b >> low >> sa; InfInt a(sa); std::string buf(size_t(a.charsBound(uint(b))), '\0');
            auto [p, ec] = a.toChars(buf.data(), buf.data() + buf.size(), uint(b), low != 0); std::cout << (ec == std::errc() ? std::string(buf.data(), p) : std::string("error")) << ' ' << buf.size();}
        else if (op == "format") {   // format <value> <spec>: the spec is the rest of the line after one space (it may hold spaces or be empty)
            std::cin >> sa; std::cin.get(); std::string spec; std::getline(std::cin, spec); InfInt a(sa);
            try { std::cout << fmt(spec, a); } catch (const std::format_error &) { std::cout << "format_error"; }}
        else if (op == "frombase") { lng b; std::cin >> b >> sa; InfInt a; std::cout << (a.read(sa, uint(b)) ? a.toString() : std::string("invalid")); }
        else if (op == "fromdigits") { lng b, k; std::cin >> b >> k; vector<uint> d(size_t(k), 0); for (uint &x : d) { std::cin >> x; } out(InfInt(d, uint(b))); }
        else if (op == "powmod") { std::cin >> sa >> sb >> sc; out(powMod(InfInt(sa), InfInt(sb), InfInt(sc))); }
        else if (op == "fromdouble") { double d; std::cin >> d; out(InfInt::fromDouble(d)); }
        else if (op == "trycast") { std::cin >> sa >> sb; InfInt a(sb); if (sa == "lng") { lng v; std::cout << (a.tryCast(v) ? "1 " + std::to_string(v) : "0"); } else if (sa == "ulng") { ulng v; std::cout << (a.tryCast(v) ? "1 " + std::to_string(v) : "0"); } else if (sa == "int") { int v; std::cout << (a.tryCast(v) ? "1 " + std::to_string(v) : "0"); } else if (sa == "lll") { lll v; std::cout << (a.tryCast(v) ? "1 " + InfInt(v).toString() : "0"); } else { ulll v; std::cout << (a.tryCast(v) ? "1 " + InfInt(v).toString() : "0"); } }
        else if (op == "limbs") { std::cin >> sa; InfInt a(sa); vector<uint> l = a.toLimbs(); vector<ulng> w = a.toLimbs<ulng>(); for (uint x : l) { std::cout << x << ' '; } std::cout << '|'; for (ulng x : w) { std::cout << ' ' << x; } std::cout << " | " << InfInt::fromLimbs(l, a.sgn) << ' ' << InfInt::fromLimbs(w, a.sgn); }
        else if (op == "sample") { std::cin >> sa; lng s; std::cin >> s; std::mt19937_64 g{ulng(s)}; std::cout << sample(InfInt(sa), g) << ' ' << bitLength(InfInt::sampleBits(int(s % 500 + 1), g)); }
        else if (op == "newton") { std::cin >> sa; InfInt a(sa, 16); a <<= 31 - (bitLength(a) - 1) % 32; out(newtonRaphsonInv(a).toString(16)); }
        else if (op == "mulk" || op == "divk" || op == "gcdk") {
            std::string kind; std::cin >> kind >> sa >> sb; InfInt a(sa), b(sb);
            vector<W> x = wordsOf(a), y = wordsOf(b);
            if (op == "mulk") {
                int n = int(x.size()), m = int(y.size()); vector<W> r(size_t(n + m), 0);
                if (kind == "basecase") { infint_detail::mulBasecase(r.data(), x.data(), n, y.data(), m); } else if (kind == "karatsuba") { infint_detail::karatsuba(r.data(), x.data(), n, y.data(), m); }
                else if (kind == "toom3") { infint_detail::toom3(r.data(), x.data(), y.data(), n); } else if (kind == "unbalanced") { infint_detail::unbalancedMul(r.data(), x.data(), n, y.data(), m, m); }
                else if (kind == "transform") { infint_detail::transformMul(r.data(), x.data(), n, y.data(), m); } else if (kind == "ssa") { infint_detail::schonhageStrassen(r.data(), x.data(), n, y.data(), m); } else if (kind == "square") { infint_detail::square(r.data(), x.data(), n); }
                else if (kind == "toom3square") { infint_detail::toom3Square(r.data(), x.data(), n); } else if (kind == "karatsubasquare") { infint_detail::karatsubaSquare(r.data(), x.data(), n); }
                else if (kind == "truncated") { r.assign(size_t(n), 0); infint_detail::truncatedMul(r.data(), x.data(), y.data(), n); }
                else { infint_detail::mul(r.data(), x.data(), n, y.data(), m); }
                out(ofWords(r));}
            else if (op == "divk") {
                int n = int(x.size()), m = int(y.size()); vector<W> q(size_t(n - m + 1), 0), r(size_t(m), 0);
                if (kind == "basecase") { infint_detail::divBasecase(q.data(), r.data(), x.data(), n, y.data(), m); } else if (kind == "bz") { infint_detail::divBurnikelZiegler(q.data(), r.data(), x.data(), n, y.data(), m); }
                else if (kind == "newton") { vector<W> iv(size_t(m) + 1); infint_detail::reciprocal(iv.data(), y.data(), m); infint_detail::divNewton(q.data(), r.data(), x.data(), n, y.data(), m, iv.data()); }
                else if (kind == "hensel") { infint_detail::divExactHensel(q.data(), x.data(), n, y.data(), m); r.assign(1, 0); } else { infint_detail::divRem(q.data(), r.data(), x.data(), n, y.data(), m); }
                std::cout << ofWords(q) << ' ' << ofWords(r);}
            else {
                if (kind == "binary") { int len = infint_detail::gcdBinary(x.data(), int(x.size()), y.data(), int(y.size())); x.resize(size_t(len)); out(ofWords(x)); }
                else { InfInt A = 1, B = 0, C = 0, D = 1; InfInt ha = abs(a), hb = abs(b); if (ha < hb) { std::swap(ha, hb); } bool ch = kind == "lehmer" ? InfInt::lehmerHalfGcd<true>(ha, hb, A, B, C, D) : InfInt::halfGcd<true>(ha, hb, A, B, C, D); std::cout << int(ch) << ' ' << ha << ' ' << hb << ' ' << A << ' ' << B << ' ' << C << ' ' << D; }}}
        else if (op == "u256") {
            std::string what; std::cin >> what >> sa >> sb; u256 x{InfInt(sa)}, y{InfInt(sb)}; lng k = lng(InfInt(sb));
            if (what == "add") { out(InfInt(x + y)); } else if (what == "sub") { out(InfInt(x - y)); } else if (what == "mul") { out(InfInt(x * y)); } else if (what == "div") { out(InfInt(x / y)); } else if (what == "mod") { out(InfInt(x % y)); }
            else if (what == "mulhigh") { out(InfInt(mulHigh(x, y))); } else if (what == "and") { out(InfInt(x & y)); } else if (what == "or") { out(InfInt(x | y)); } else if (what == "xor") { out(InfInt(x ^ y)); } else if (what == "not") { out(InfInt(~x)); }
            else if (what == "shl") { out(InfInt(x << int(k))); } else if (what == "shr") { out(InfInt(x >> int(k))); } else if (what == "cmp") { out(x < y ? -1 : x > y ? 1 : 0); }
            else if (what == "divword") { ulng r = x.divWord(ulng(InfInt(sb))); std::cout << InfInt(x) << ' ' << r; } else if (what == "tostr") { out(x.toString(uint(k))); } else { out(InfInt(u256(sa, 10))); }}
        else { std::cerr << "unknown protocol command " << op << '\n'; std::exit(3); }
        std::cout << '\n';}}

int main(int argc, char **argv) {
    std::string invalidCase; bool proto = false;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
        else if (arg == "--seed" && i + 1 < argc) { seed = std::strtoull(argv[++i], nullptr, 10); }
        else if (arg == "--invalid" && i + 1 < argc) { invalidCase = argv[++i]; }
        else if (arg == "--protocol") { proto = true; }
        else { std::cerr << "usage: --mode quick|full|stress --seed N [--invalid case] [--protocol]\n"; return 2; }}
    rng.seed(seed);
    if (proto) { protocol(); return 0; }
    if (!invalidCase.empty()) { invalid(invalidCase); return 2; }
    auto start = std::chrono::steady_clock::now();
    auto phase = [&](const char *name, auto &&f) {
        auto t0 = std::chrono::steady_clock::now(); lng before = checks; f();
        std::cout << "phase " << name << " checks=" << checks - before << " time=" << std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() << "s\n" << std::flush;};
    phase("construction", construction); phase("text output", textOutput); phase("small exhaustive", smallExhaustive); phase("addition", addition); phase("multiplication", multiplication); phase("division", division); phase("gcd", gcdFamily); phase("powMod", modularPower);
    phase("roots", roots); phase("number theory", numberTheory); phase("bits", bits); phase("sampling, hash, streams", samplingHashStreams); phase("infinity", infinity);
    phase("UInt<1>", uintType<1>); phase("UInt<2>", uintType<2>); phase("UInt<4>", uintType<4>); phase("UInt conversions and streams", uintConversions); phase("aliasing", aliasing);
    require(otherUnitSum(InfInt("123456789012345678901234567890")) == otherUnitSum(InfInt("123456789012345678901234567890")) && otherUnitSum(InfInt(7)) == 9, "second translation unit");
    double sec = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
    std::cout << "PASS mode=" << mode << " seed=" << seed << " checks=" << checks << " time=" << sec << "s\n";
    return 0;}
