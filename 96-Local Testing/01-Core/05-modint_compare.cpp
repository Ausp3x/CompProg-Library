// A/B rows of 05-modint.hpp for 06-compare.py: every operation on every dispatch class, with predictable, random and worst-case data.
#include "05-modint.hpp"
#include "00-perf.hpp"

constexpr int N = 4096;
static Rows *rows;

template<class T> [[gnu::noinline, gnu::noipa]] void kAdd(T *a, const T *b, int n) { for (int i = 0; i < n; ++i) { a[i] += b[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kSub(T *a, const T *b, int n) { for (int i = 0; i < n; ++i) { a[i] -= b[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kAxpy(T *t, const T *s, T c, int n) { for (int i = 0; i < n; ++i) { t[i] -= s[i] * c; } }
template<class T> [[gnu::noinline, gnu::noipa]] T kSum(const T *a, int n) { T r = 0; for (int i = 0; i < n; ++i) { r += a[i]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] T kSubSum(const T *a, int n) { T r = 0; for (int i = 0; i < n; ++i) { r -= a[i]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] T kAlt(const T *a, int n) { T r = 0; for (int i = 0; i + 1 < n; i += 2) { r += a[i]; r -= a[i + 1]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] void kMul(T *o, const T *a, const T *b, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] * b[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] T kChain(const T *a, int n) { T r = 1; for (int i = 0; i < n; ++i) { r *= a[i]; } return r; }
// A product and a converted source that stay the same through the loop: GCC computes them once when the header lets it see them.
template<class T> [[gnu::noinline, gnu::noipa]] void kHoistMul(T *o, const T *a, T x, T y, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] + x * y; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kHoistCtor(T *o, const T *a, lng k, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] + k; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kHoistCtorInt(T *o, const T *a, int k, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] + k; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kHoistCtorUint(T *o, const T *a, uint k, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] + k; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kDet3(T *o, const T *a, int n) {
    for (int i = 0; i + 9 <= n; i += 9) {
        const T *m = a + i;
        o[i] = m[0] * (m[4] * m[8] - m[5] * m[7]) - m[1] * (m[3] * m[8] - m[5] * m[6]) + m[2] * (m[3] * m[7] - m[4] * m[6]);
        o[i + 1] = m[0] * m[1] * m[2] + m[3] * m[4] * m[5] + m[6] * m[7] * m[8];}}
template<class T> [[gnu::noinline, gnu::noipa]] T kHorner(const T *a, int n, T x) {
    T r = 0;
    for (int i = 0; i + 12 <= n; i += 12) {
        const T *c = a + i;
        r += ((((((((((c[0] * x + c[1]) * x + c[2]) * x + c[3]) * x + c[4]) * x + c[5]) * x + c[6]) * x + c[7]) * x + c[8]) * x + c[9]) * x + c[10]) * x + c[11];}
    return r;}
template<class T> [[gnu::noinline, gnu::noipa]] void kNeg(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = -a[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] T kNegChain(const T *a, int n) { T r = 1; for (int i = 0; i < n; ++i) { r = -r + a[i]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] T kIncChain(const T *a, int n) { T r = 1; for (int i = 0; i < n; ++i) { ++r; r += a[i]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] T kDecChain(const T *a, int n) { T r = 1; for (int i = 0; i < n; ++i) { --r; r += a[i]; } return r; }
template<class T> [[gnu::noinline, gnu::noipa]] void kInc(T *a, int n) { for (int i = 0; i < n; ++i) { ++a[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kDec(T *a, int n) { for (int i = 0; i < n; ++i) { --a[i]; } }
// In place, as a loop over a container writes them: the vectorizer accepts some source forms of an operator and not others.
template<class T> [[gnu::noinline, gnu::noipa]] void kNegIn(T *a, int n) { for (int i = 0; i < n; ++i) { a[i] = -a[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kAddK(T *a, T k, int n) { for (int i = 0; i < n; ++i) { a[i] += k; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kSubK(T *a, T k, int n) { for (int i = 0; i < n; ++i) { a[i] -= k; } }
template<class T, class E> [[gnu::noinline, gnu::noipa]] void kPow(T *o, const T *a, const E *e, int n) { for (int i = 0; i < n; ++i) { o[i] = pow(a[i], e[i]); } }
template<class T, class E> [[gnu::noinline, gnu::noipa]] void kPowDep(T *o, const T *a, const E *e, int n) { T r = a[0]; for (int i = 0; i < n; ++i) { o[i] = r = pow(r + a[i], e[i]); } }
template<class T> [[gnu::noinline, gnu::noipa]] void kPowLiteral(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = pow(a[i], 1000000005ULL); } }
template<class T> [[gnu::noinline, gnu::noipa]] void kInv(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = inv(a[i]); } }
template<class T> [[gnu::noinline, gnu::noipa]] void kInvChain(T *o, const T *a, int n) { T x = a[0]; for (int i = 0; i < n; ++i) { x = inv(x) * a[i]; o[i] = x; } }
template<class T> [[gnu::noinline, gnu::noipa]] bool kBatch(const vector<T> &a, vector<T> &o) { return batchInv(a, o); }
template<class T> [[gnu::noinline, gnu::noipa]] void kSqrt(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = sqrt(a[i]); } }
template<class T, class X> [[gnu::noinline, gnu::noipa]] void kCtor(T *o, const X *x, int n) { for (int i = 0; i < n; ++i) { o[i] = T(x[i]); } }
template<class T> T put(typename T::Word w) { if constexpr (requires { T::from(w); }) { return T::from(w); } else { T r; r.n = w; return r; } } // a canonical word as a value of either form
template<class T> [[gnu::noinline, gnu::noipa]] void kRed(T *o, const typename T::Wide *x, int n) { for (int i = 0; i < n; ++i) { o[i] = put<T>(T::red(x[i])); } }
template<class T> [[gnu::noinline, gnu::noipa]] void kFill(T *o, int n) { for (int i = 0; i < n; ++i) { o[i] = T(); } }
template<class T> [[gnu::noinline, gnu::noipa]] void kFillLiteral(T *o, int n) { for (int i = 0; i < n; ++i) { o[i] = 0; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kAlloc(vector<T> &o, int n) { o = vector<T>(n); }
template<class T> [[gnu::noinline, gnu::noipa]] void kFillOne(T *o, int n) { for (int i = 0; i < n; ++i) { o[i] = 1; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kAffineLiteral(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] * 3 + 1; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kAffineMax(T *o, const T *a, int n) { for (int i = 0; i < n; ++i) { o[i] = a[i] * 3 + 4294967295u; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kCountOne(T *o, const T *a, int n) { int c = 0; for (int i = 0; i < n; ++i) { c += a[i] == 1; } o[0] = c; }
template<class T> [[gnu::noinline, gnu::noipa]] void kRead(T *o, istream &is, int n) { for (int i = 0; i < n; ++i) { is >> o[i]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kWrite(ostream &os, const T *a, int n) { for (int i = 0; i < n; ++i) { os << a[i] << ' '; } }
template<class T> [[gnu::noinline, gnu::noipa]] ulng kVal(const T *a, int n) { ulng s = 0; for (int i = 0; i < n; ++i) { s += a[i].val(); } return s; }
template<class T> [[gnu::noinline, gnu::noipa]] void kRaw(T *o, const typename T::Word *w, int n) { for (int i = 0; i < n; ++i) { o[i] = T::raw(w[i]); } }
template<class T> [[gnu::noinline, gnu::noipa]] ulng kLess(const T *a, const T *b, int n) { ulng c = 0; for (int i = 0; i < n; ++i) { c += a[i] < b[i]; } return c; }
// A stream buffer over a string. Input: a get area of `chunk` bytes refilled from the text (LINE: one line at a time, as a terminal or an
// interactive judge delivers it; 0: no area, every character a virtual call). Output: a put area of `chunk` bytes flushed to `sink`.
struct Chunks : std::streambuf {
    static constexpr size_t LINE = size_t(-1);
    string text, sink; size_t at = 0, chunk; vector<char> area;
    explicit Chunks(size_t c) : chunk(c), area(c == LINE ? 0 : c) {}
    void reset(string t = {}) {
        text = std::move(t); at = 0; sink.clear(); setg(nullptr, nullptr, nullptr);
        if (!area.empty()) { setp(area.data(), area.data() + area.size()); }}
    int_type underflow() override {
        if (at == text.size()) { return traits_type::eof(); }
        if (!chunk) { return traits_type::to_int_type(text[at]); }
        size_t k = chunk == LINE ? std::min(text.find('\n', at), text.size() - 1) + 1 - at : std::min(chunk, text.size() - at);
        setg(text.data() + at, text.data() + at, text.data() + at + k); at += k;
        return traits_type::to_int_type(*gptr());}
    int_type uflow() override { return chunk ? std::streambuf::uflow() : at == text.size() ? traits_type::eof() : traits_type::to_int_type(text[at++]); }
    int_type overflow(int_type c) override {
        if (!area.empty()) { sink.append(pbase(), pptr()); setp(area.data(), area.data() + area.size()); }
        if (!traits_type::eq_int_type(c, traits_type::eof())) { sink += traits_type::to_char_type(c); }
        return traits_type::not_eof(c);}
    int sync() override { overflow(traits_type::eof()); return 0; }
    ulng written() { overflow(traits_type::eof()); return ulng(std::hash<string>{}(sink)); }
};
// 1024 cache lines, more than a 48 KB first-level cache holds: the call that follows finds its tables in the second level.
static vector<unsigned char> spill(1 << 16, 1); static ulng spilled;
[[gnu::always_inline]] inline void evict(int i, ulng &s) { for (int j = i & 63; j < 1 << 16; j += 64) { s += spill[j]; } }
template<class T> [[gnu::noinline, gnu::noipa]] void kEvict(T *o, const T *a, int n) { ulng s = 0; for (int i = 0; i < n; ++i) { evict(i, s); o[i] = a[i]; } spilled = s; }
template<class T> [[gnu::noinline, gnu::noipa]] void kInvEvict(T *o, const T *a, int n) { ulng s = 0; for (int i = 0; i < n; ++i) { evict(i, s); o[i] = inv(a[i]); } spilled = s; }
template<class T> [[gnu::noinline, gnu::noipa]] void kWriteEvict(ostream &os, const T *a, int n) { ulng s = 0; for (int i = 0; i < n; ++i) { evict(i, s); os << a[i] << ' '; } spilled = s; }
template<class T> [[gnu::noinline, gnu::noipa]] void kSqrtEvict(T *o, const T *a, int n) { ulng s = 0; for (int i = 0; i < n; ++i) { evict(i, s); o[i] = sqrt(a[i]); } spilled = s; }
[[gnu::noinline, gnu::noipa]] ulng kPrime(const ulng *p, int n) { ulng c = 0; for (int i = 0; i < n; ++i) { c += modint_detail::isPrime(p[i]); } return c; }
[[gnu::noinline, gnu::noipa]] ulng kPrimeEvict(const ulng *p, int n) { ulng s = 0, c = 0; for (int i = 0; i < n; ++i) { evict(i, s); c += modint_detail::isPrime(p[i]); } spilled = s; return c; }
[[gnu::noinline, gnu::noipa]] ulng kSumEvict(const ulng *p, int n) { ulng s = 0, c = 0; for (int i = 0; i < n; ++i) { evict(i, s); c += p[i]; } spilled = s; return c; }

template<class T> ulng digest(const vector<T> &o) { ulng c = 0; for (const T &x : o) { c = c * 0x100000001b3ULL + x.val(); } return c; }

// Arithmetic, powers, inverses, construction and stream rows of one type; `prime` adds the rows that need a field.
template<class T> void suite(const string &type, bool prime) {
    using W = typename T::Word;
    const W mod = T::mod();
    std::mt19937_64 &rng = rows->rng;
    vector<T> a(N), b(N), o(N); vector<ulng> e(N);
    auto word = [&] { return W(rng() % mod); };
    auto coprime = [&](W x) { return gcd(ulng(x), ulng(mod)) == 1; };
    auto unit = [&] { W x; do { x = word(); } while (!coprime(x)); return x; };
    auto below = [&](ulng k) { W x; do { x = W(1 + rng() % k % (mod - 1)); } while (!coprime(x)); return x; }; // a unit in [1, k]
    auto gen = [&] { for (int i = 0; i < N; ++i) { a[i] = T::raw(word()); b[i] = T::raw(word()); } };
    auto every = [&](W v) { return [&, v] { gen(); for (int i = 0; i < N; i += 7) { a[i] = T::raw(v); } }; };
    auto quarter = [&](W v) { return [&, v] { gen(); for (int i = 0; i < N; ++i) { if (rng() % 4 == 0) { a[i] = T::raw(v); } } }; };
    auto first = [&] { return ulng(o[0].val()); };
    auto all = [&] { return digest(o); };
    auto self = [&] { return digest(a); };
    auto row = [&](const string &name, auto g, auto f, auto c) { rows->run(type, name, N, g, f, c); };
    row("add", gen, [&] { kAdd(a.data(), b.data(), N); }, self);
    row("sub", gen, [&] { kSub(a.data(), b.data(), N); }, self);
    row("axpy", gen, [&] { kAxpy(a.data(), b.data(), b[7], N); }, self);
    row("sum", gen, [&] { o[0] = kSum(a.data(), N); }, first);
    row("subsum", gen, [&] { o[0] = kSubSum(a.data(), N); }, first);
    row("altsum", gen, [&] { o[0] = kAlt(a.data(), N); }, first);
    row("mul", gen, [&] { kMul(o.data(), a.data(), b.data(), N); }, all);
    row("chain", gen, [&] { o[0] = kChain(a.data(), N); }, first);
    row("det3", gen, [&] { kDet3(o.data(), a.data(), N); }, all);
    row("horner12", gen, [&] { o[0] = kHorner(a.data(), N, a[5]); }, first);
    row("hoist-mul", gen, [&] { kHoistMul(o.data(), a.data(), a[3], b[5], N); }, all);
    row("hoist-ctor", gen, [&] { kHoistCtor(o.data(), a.data(), lng(ulng(b[9].val()) >> 1) + 12345, N); }, all);
    row("hoist-ctor-int", gen, [&] { kHoistCtorInt(o.data(), a.data(), int(uint(b[9].val()) >> 2) - 54321, N); }, all);
    row("hoist-ctor-uint", gen, [&] { kHoistCtorUint(o.data(), a.data(), uint(b[9].val()) | 3, N); }, all);
    row("neg", gen, [&] { kNeg(o.data(), a.data(), N); }, all);
    row("neg-zeros", quarter(0), [&] { kNeg(o.data(), a.data(), N); }, all);
    row("negchain", gen, [&] { o[0] = kNegChain(a.data(), N); }, first);
    row("incchain", gen, [&] { o[0] = kIncChain(a.data(), N); }, first);
    row("decchain", gen, [&] { o[0] = kDecChain(a.data(), N); }, first);
    row("neg-inplace", gen, [&] { kNegIn(a.data(), N); }, self);
    row("neg-inplace-zeros", quarter(0), [&] { kNegIn(a.data(), N); }, self);
    row("addk", gen, [&] { kAddK(a.data(), b[7], N); }, self);
    row("subk", gen, [&] { kSubK(a.data(), b[7], N); }, self);
    row("inc", every(mod - 1), [&] { kInc(a.data(), N); }, self);
    row("inc-rand", quarter(mod - 1), [&] { kInc(a.data(), N); }, self);
    row("dec", every(0), [&] { kDec(a.data(), N); }, self);
    row("dec-rand", quarter(0), [&] { kDec(a.data(), N); }, self);
    auto power = [&](const string &name, auto exponent) {
        row("pow-" + name, [&] { gen(); for (int i = 0; i < N; ++i) { e[i] = exponent(i); } }, [&] { kPow(o.data(), a.data(), e.data(), N); }, all);};
    for (ulng x : {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 16, 20, 31, 40, 63, 64, 100, 127, 128, 255, 256, 300, 511, 512, 1000, 1023, 1024, 2047, 2048, 3001, 4095, 65536, 100003, 10000019, 1000000005}) {
        power(std::to_string(x), [&](int) { return x; });}
    power("fermat", [&](int) { return ulng(mod - 2); });
    power("golden", [&](int) { return 0x9E3779B97F4A7C15ULL; });
    power("alt2", [&](int i) { return i & 1 ? 100003ULL : 1000000005ULL; });
    power("below5", [&](int) { return rng() % 5; });
    for (int bits : {4, 5, 6, 7, 8, 9, 10, 11, 12, 16, 20, 64}) { power("rand" + std::to_string(bits), [&](int) { return rng() >> (64 - bits); }); }
    for (int bits : {6, 12, 20, 64}) { // each base the previous power plus an operand: one dependent chain through the calls
        row("pow-dep-rand" + std::to_string(bits), [&] { gen(); for (int i = 0; i < N; ++i) { e[i] = rng() >> (64 - bits); } }, [&] { kPowDep(o.data(), a.data(), e.data(), N); }, all);}
    row("pow-literal", gen, [&] { kPowLiteral(o.data(), a.data(), N); }, all);
    row("inv", [&] { for (T &x : a) { x = T::raw(unit()); } }, [&] { kInv(o.data(), a.data(), N); }, all); // a ring needs units only
    rows->net(type, "inv-evict", N, [&] { for (T &x : a) { x = T::raw(unit()); } }, [&] { kInvEvict(o.data(), a.data(), N); }, [&] { kEvict(o.data(), a.data(), N); }, all);
    row("inv-chain", [&] { for (T &x : a) { x = T::raw(unit()); } }, [&] { kInvChain(o.data(), a.data(), N); }, all); // each inverse waits for the one before it
    row("inv-small", [&] { for (T &x : a) { x = T::raw(below(100000)); } }, [&] { kInv(o.data(), a.data(), N); }, all);
    row("inv-tiny", [&] { for (T &x : a) { x = T::raw(below(15)); } }, [&] { kInv(o.data(), a.data(), N); }, all);
    if (mod > 16384) { // units of [16, 256), [256, 2048) and [2048, 16384), the ranges a table could answer; the last two also with the first-level cache evicted
        auto span = [&](ulng lo, ulng hi) { return [&, lo, hi] { for (T &x : a) { W v; do { v = W(lo + rng() % (hi - lo)); } while (!coprime(v)); x = T::raw(v); } }; };
        row("inv-b8", span(16, 256), [&] { kInv(o.data(), a.data(), N); }, all);
        row("inv-b11", span(256, 2048), [&] { kInv(o.data(), a.data(), N); }, all);
        rows->net(type, "inv-b11-evict", N, span(256, 2048), [&] { kInvEvict(o.data(), a.data(), N); }, [&] { kEvict(o.data(), a.data(), N); }, all);
        row("inv-b14", span(2048, 16384), [&] { kInv(o.data(), a.data(), N); }, all);
        rows->net(type, "inv-b14-evict", N, span(2048, 16384), [&] { kInvEvict(o.data(), a.data(), N); }, [&] { kEvict(o.data(), a.data(), N); }, all);}
    if (prime) { row("sqrt", gen, [&] { kSqrt(o.data(), a.data(), N); }, all); }
    for (int k : {1, 2, 4, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 512, 1024, 2048, 4096}) {
        vector<vector<T>> u(N / k, vector<T>(k)); vector<T> r(k); // N / k vectors: one repeated vector teaches the predictor its inverse
        rows->run(type, "batch-" + std::to_string(k), double(k) * (N / k), [&] { for (auto &v : u) { for (T &x : v) { x = T::raw(unit()); } } },
                  [&] { for (const auto &v : u) { kBatch(v, r); } }, [&] { return digest(r); });}
    auto ctor = [&](const string &name, auto value) {
        vector<decltype(value())> x(N);
        row("ctor-" + name, [&] { for (auto &v : x) { v = value(); } }, [&] { kCtor(o.data(), x.data(), N); }, all);};
    auto wide = [&] { return ulll(rng()) << 64 | rng(); };
    ctor("int8", [&] { return int8_t(rng()); });
    ctor("int16", [&] { return int16_t(rng()); });
    ctor("int", [&] { return int(rng()); });
    ctor("int-pos", [&] { return int(rng() >> 33); });
    ctor("uint", [&] { return uint(rng()); });
    ctor("lng", [&] { return lng(rng()); });
    ctor("lng-pos", [&] { return lng(rng() >> 1); });
    ctor("lng-small", [&] { return lng(rng() >> 24); });
    ctor("lng-diff", [&] { return lng(rng() >> 24) - lng(rng() >> 24); });
    ctor("ulng", [&] { return ulng(rng()); });
    ctor("u128", wide);
    ctor("u128-sum", [&] { return wide() >> 58; });
    ctor("u128-small", [&] { return ulll(rng() >> 34); });
    ctor("u128-mix", [&] { return wide() >> (rng() & 1 ? 70 : 20); });
    ctor("i128", [&] { return lll(wide()); });
    vector<typename T::Wide> dw(N); // the reduction hook on a product of residues and on any double-width dividend
    row("red-product", [&] { for (auto &v : dw) { v = typename T::Wide(word()) * word(); } }, [&] { kRed(o.data(), dw.data(), N); }, all);
    row("red-any", [&] { for (auto &v : dw) { v = static_cast<typename T::Wide>(wide()); } }, [&] { kRed(o.data(), dw.data(), N); }, all);
    row("fill", gen, [&] { kFill(o.data(), N); }, all);
    row("fill-literal", gen, [&] { kFillLiteral(o.data(), N); }, all);
    row("alloc", [&] { o.assign(N, T::raw(mod > 1)); }, [&] { kAlloc(o, N); }, all);
    row("fill-one", gen, [&] { kFillOne(o.data(), N); }, all); // literal operands: a dynamic modulus cannot fold them
    row("affine-literal", gen, [&] { kAffineLiteral(o.data(), a.data(), N); }, all);
    row("affine-max", gen, [&] { kAffineMax(o.data(), a.data(), N); }, all);
    row("count-one", every(mod > 1), [&] { kCountOne(o.data(), a.data(), N); }, first);
    vector<W> words(N); ulng acc = 0;
    row("val", gen, [&] { acc = kVal(a.data(), N); }, [&] { return acc; });
    row("raw", [&] { for (W &x : words) { x = word(); } }, [&] { kRaw(o.data(), words.data(), N); }, all);
    row("less", gen, [&] { acc = kLess(a.data(), b.data(), N); }, [&] { return acc; });
    auto tokens = [&](string &text, int digits, int line) { // N tokens of `digits` digits, one in eight negative, `line` to a line
        text.clear();
        for (int i = 0; i < N; ++i) {
            if (rng() % 8 == 0) { text += '-'; }
            for (int d = 0; d < digits; ++d) { text += char('0' + (d ? rng() % 10 : 1 + rng() % 9)); }
            text += i % line == line - 1 ? '\n' : ' ';}};
    for (int digits : {6, 9, 18, 30, 60}) { // one group of eight digits, two, a full chunk, a second chunk, a fold inside the token
        string text; std::istringstream is;
        row("read-" + std::to_string(digits), [&] { tokens(text, digits, 16); is.clear(); is.str(text); }, [&] { kRead(o.data(), is, N); }, all);}
    // Tokens of nine digits by where the get area ends: chunks of 32, 256 and 4096 bytes, one line of 16 tokens or of one token (the area
    // ends behind each line break), no area at all; a stream tied to an output stream, as cin is by default; and lines of four and of
    // one token of three digits, whose last token starts fewer than eight bytes before the area's end.
    auto chunked = [&](const string &name, size_t chunk, int line, bool tied, int digits = 9) {
        Chunks in(chunk), out(64); string text; std::istream is(&in); std::ostream tie(&out);
        if (tied) { is.tie(&tie); }
        row("read-" + std::to_string(digits) + "-" + name, [&] { tokens(text, digits, line); is.clear(); in.reset(text); out.reset(); }, [&] { kRead(o.data(), is, N); }, all);};
    chunked("chunk32", 32, 16, false); chunked("chunk256", 256, 16, false); chunked("chunk4096", 4096, 16, false);
    chunked("line16", Chunks::LINE, 16, false); chunked("line1", Chunks::LINE, 1, false); chunked("nobuf", 0, 16, false); chunked("tied", 4096, 16, true);
    chunked("line4", Chunks::LINE, 4, false, 3); chunked("line1", Chunks::LINE, 1, false, 3);
    std::ostringstream os; string kept;
    row("write", [&] { gen(); os.str(string()); os.clear(); }, [&] { kWrite(os, a.data(), N); }, [&] { kept = os.str(); return ulng(std::hash<string>{}(kept)); });
    auto shorts = [&] { for (auto &v : a) { ulng p = 10; for (ulng j = rng() % 8; j--;) { p *= 10; } v = T(rng() % p); } os.str(string()); os.clear(); }; // one to eight digits, the length random
    row("write-short", shorts, [&] { kWrite(os, a.data(), N); }, [&] { kept = os.str(); return ulng(std::hash<string>{}(kept)); });
    rows->net(type, "write-evict", N, [&] { gen(); os.str(string()); os.clear(); }, [&] { kWriteEvict(os, a.data(), N); }, [&] { kEvict(o.data(), a.data(), N); },
              [&] { kept = os.str(); return ulng(std::hash<string>{}(kept)); }); // the writer with its tables out of the first-level cache
    // The writer by its put area: 4096 bytes, 16 (never room for 20 digits), none; and a stream tied to another, which each output flushes.
    auto emit = [&](const string &name, size_t area, bool tied) {
        Chunks out(area), other(64); std::ostream cs(&out), tie(&other);
        if (tied) { cs.tie(&tie); }
        row("write-" + name, [&] { gen(); out.reset(); other.reset(); cs.clear(); }, [&] { kWrite(cs, a.data(), N); }, [&] { return out.written(); });};
    emit("area4096", 4096, false); emit("area16", 16, false); emit("nobuf", 0, false); emit("tied", 4096, true);}

// Square roots of one prime modulus: random input, residues only, non-residues only, and residues with the tables evicted before each call.
template<class T> void roots(const string &type) {
    using W = typename T::Word;
    const W mod = T::mod();
    std::mt19937_64 &rng = rows->rng;
    vector<T> a(N), o(N);
    auto word = [&] { return W(rng() % mod); };
    auto residue = [&](T x) { return x.val() == 0 || pow(x, (mod - 1) / 2).val() == 1; };
    auto row = [&](const string &name, auto g) { rows->run(type, name, N, g, [&] { kSqrt(o.data(), a.data(), N); }, [&] { return digest(o); }); };
    row("sqrt-mix", [&] { for (T &x : a) { x = T::raw(word()); } });
    row("sqrt-qr", [&] { for (T &x : a) { T r = T::raw(word()); x = r * r; } });
    row("sqrt-non", [&] { for (T &x : a) { do { x = T::raw(word()); } while (residue(x)); } });
    rows->net(type, "sqrt-qr-evict", N, [&] { for (T &x : a) { T r = T::raw(word()); x = r * r; } }, [&] { kSqrtEvict(o.data(), a.data(), N); },
              [&] { kEvict(o.data(), a.data(), N); }, [&] { return digest(o); });}

// The first `count` primes above 2^bits with p - 1 divisible by 2^s exactly; spread: `count` of them across [2^bits, 2^(bits + 1))
// in random steps, so that their bits differ.
static vector<ulng> primes(int bits, int s, size_t count = 1, bool spread = false) {
    vector<ulng> r; ulng lo = ((ulng(1) << bits) >> s) | 1, step = spread ? std::max<ulng>(lo / count, 1) : 1, x = 88172645463325252ULL;
    for (ulng k = lo; r.size() < count; k += 2) {
        ulng p = (k << s) + 1;
        if (modint_detail::isPrime(p)) { r.push_back(p); x ^= x << 7; x ^= x >> 9; k += 2 * (x % step); }}
    return r;}

// The Montgomery-form types where the header has them, else the canonical ones: against a header without them the rows compare the two forms.
namespace modint_detail { template<ulng M, int ID> struct MontContext; }
template<class T> concept Complete = requires { sizeof(T); };
template<ulng M> using MontOr = std::conditional_t<Complete<modint_detail::MontContext<M, 0>>, modint_detail::Value<modint_detail::MontContext<M, 0>>, ModInt64<M>>;
template<int ID> using DynMontOr = std::conditional_t<Complete<modint_detail::MontContext<0, ID>>, modint_detail::Value<modint_detail::MontContext<0, ID>>, DynModInt64<ID>>;
template<ulng M> void mont64(const string &type) { suite<MontOr<M>>(type, MontOr<M>::isPrime()); }
template<uint M> void fixed32(const string &type) { suite<ModInt<M>>(type, ModInt<M>::isPrime()); }
template<ulng M> void fixed64(const string &type) { suite<ModInt64<M>>(type, ModInt64<M>::isPrime()); }
static void runtime32(const string &type, uint m) { DynModInt<0>::setMod(m); suite<DynModInt<0>>(type, DynModInt<0>::isPrime()); }
static void runtime64(const string &type, ulng m) { DynModInt64<0>::setMod(m); suite<DynModInt64<0>>(type, DynModInt64<0>::isPrime()); }
// A modulus known prime without the test of setMod: prm = 0 skips it, and the flag is set directly.
template<class T> void givenPrime(ulng p) { T::setMod(typename T::Word(p), 0); T::is_prime = true; }

int main(int argc, char **argv) {
    Rows table(argc, argv); rows = &table;
    std::mt19937_64 &rng = table.rng;
    constexpr ulng P62 = (ulng(1) << 62) - 57, P64 = ulng(0) - 59, GOLD = 18446744069414584321ULL, J62 = 4611686018427388039ULL;
    // Static 32-bit words: short and long GCC reciprocal sequences, a Mersenne prime, the top of the word, tiny, composite, a power of two.
    fixed32<998244353>("S32"); fixed32<1000000007>("S32b"); fixed32<469762049>("S32c"); fixed32<1000000009>("S32d"); fixed32<167772161>("S32e");
    fixed32<2147483647>("S32f"); fixed32<4294967291U>("S32q"); fixed32<7>("S32tiny"); fixed32<1000000>("S32comp"); fixed32<1U << 30>("S32pow2");
    // Static 64-bit words: moduli below 2^32, fold moduli (2^62 - 57, 2^64 - 59, Goldilocks), Mersenne61, general reciprocals, even moduli.
    fixed64<7>("S64tiny"); fixed64<998244353>("S64s"); fixed64<1000000007>("S64b"); fixed64<469762049>("S64c"); fixed64<1000000009>("S64d");
    fixed64<167772161>("S64e"); fixed64<2147483647>("S64f"); fixed64<3221225473>("S64n"); fixed64<4294967291>("S64q"); fixed64<1000003>("S64t");
    fixed64<999999999>("S64o"); fixed64<754974721>("S64v"); fixed64<2013265921>("S64w"); fixed64<4000000000ULL>("S64ev32");
    fixed64<1000000000039ULL>("S64g"); fixed64<P62>("S64m"); fixed64<J62>("S64j"); fixed64<(ulng(1) << 63) - 25>("S64k"); fixed64<(ulng(1) << 63) + 29>("S64h"); fixed64<P64>("S64l");
    fixed64<Barrett64::MERSENNE61>("M61"); fixed64<GOLD>("S64gold"); fixed64<1000000000000000000ULL>("S64even"); fixed64<ulng(1) << 40>("S64pow2");
    // Runtime 32-bit words: the default, the top of the word, the worst correction rate, an even modulus, a power of two.
    runtime32("D32", 998244353); runtime32("D32b", 1000000007); runtime32("D32q", 4294967291U); runtime32("D32w", 4294902751U); runtime32("D32even", 156648748); runtime32("D32pow2", 1U << 30);
    // Runtime 64-bit words: below 2^32, general, both sides of 2^62 and 2^63, Mersenne61, Goldilocks, even moduli, powers of two.
    runtime64("D64s", 998244353); runtime64("D64b", 1000000007); runtime64("D64q", 4294967291); runtime64("D64g", 1000000000039ULL); runtime64("D64m", P62);
    runtime64("D64j", J62); runtime64("D64k", (ulng(1) << 63) - 25); runtime64("D64l", P64); runtime64("D64f", Barrett64::MERSENNE61); runtime64("D64gold", GOLD);
    runtime64("D64e", 1000000000000000000ULL); runtime64("D64x", 18446744073709551614ULL); runtime64("D64pow2", ulng(1) << 40); runtime64("D64pow2s", ulng(1) << 20);

    // Montgomery-form storage, odd moduli: below 2^32, general, both sides of 2^62 and 2^63, Mersenne61, Goldilocks; static, then runtime.
    mont64<998244353>("SM64s"); mont64<1000000000039ULL>("SM64g"); mont64<P62>("SM64m"); mont64<J62>("SM64j"); mont64<(ulng(1) << 63) - 25>("SM64k");
    mont64<(ulng(1) << 63) + 29>("SM64h"); mont64<P64>("SM64l"); mont64<Barrett64::MERSENNE61>("SM61"); mont64<GOLD>("SM64gold");
    for (auto [name, m] : {pair<const char *, ulng>{"DM64s", 998244353}, {"DM64g", 1000000000039ULL}, {"DM64m", P62}, {"DM64j", J62}, {"DM64k", (ulng(1) << 63) - 25},
                           {"DM64l", P64}, {"DM64f", Barrett64::MERSENNE61}, {"DM64gold", GOLD}}) {
        DynMontOr<0>::setMod(m); suite<DynMontOr<0>>(name, DynMontOr<0>::isPrime());}
    for (int s : {1, 2, 4, 8, 16, 32}) { DynMontOr<1>::setMod(primes(60, s)[0]); roots<DynMontOr<1>>("DM64-s" + std::to_string(s)); }

    // Square roots by the 2-adic order s of p - 1: static primes near 2^29 and 2^60, then runtime ones.
    roots<ModInt<536871029U>>("S32-s2"); roots<ModInt<536871001U>>("S32-s3"); roots<ModInt<536871089U>>("S32-s4"); roots<ModInt<536871233U>>("S32-s6");
    roots<ModInt<536887553U>>("S32-s8"); roots<ModInt<536896513U>>("S32-s10"); roots<ModInt<536924161U>>("S32-s12"); roots<ModInt<536952833U>>("S32-s14");
    roots<ModInt<537591809U>>("S32-s16"); roots<ModInt<581959681U>>("S32-s20"); roots<ModInt<595591169U>>("S32-s23"); roots<ModInt<1811939329U>>("S32-s26");
    roots<ModInt<998244353>>("S32-mint"); roots<ModInt<1000000007>>("S32-3mod4");
    roots<ModInt64<1152921504606847189ULL>>("S64-s2"); roots<ModInt64<1152921504606847601ULL>>("S64-s4"); roots<ModInt64<1152921504606876929ULL>>("S64-s8");
    roots<ModInt64<1152921504608747521ULL>>("S64-s16"); roots<ModInt64<1152921505194049537ULL>>("S64-s24"); roots<ModInt64<1152921680700506113ULL>>("S64-s32");
    roots<ModInt64<1152937997281263617ULL>>("S64-s40"); roots<ModInt64<998244353>>("S64-mint"); roots<ModInt64<GOLD>>("S64-gold"); roots<ModInt64<P64>>("S64-p64");
    for (int s : {1, 2, 3, 4, 6, 8, 10, 12, 14, 16, 17, 18, 19, 20, 21, 22, 23, 26}) { DynModInt<1>::setMod(uint(primes(29, s)[0])); roots<DynModInt<1>>("D32-s" + std::to_string(s)); }
    for (int s : {10, 12, 14, 16}) { DynModInt<1>::setMod(uint(primes(20, s)[0])); roots<DynModInt<1>>("D32b20-s" + std::to_string(s)); }
    DynModInt<1>::setMod(998244353); roots<DynModInt<1>>("D32-mint");
    for (int s : {1, 2, 4, 8, 16, 18, 20, 22, 24, 32, 40}) { DynModInt64<1>::setMod(primes(60, s)[0]); roots<DynModInt64<1>>("D64-s" + std::to_string(s)); }
    for (int s : {10, 12, 14, 16, 18, 20}) { DynModInt64<1>::setMod(primes(29, s)[0]); roots<DynModInt64<1>>("D64b30-s" + std::to_string(s)); }
    DynModInt64<1>::setMod(998244353); roots<DynModInt64<1>>("D64-mint");
    DynModInt64<1>::setMod(GOLD); roots<DynModInt64<1>>("D64-gold");
    // Bursts: b square roots after each change of the modulus, over 16 primes of one shape, whose exponents a branch predictor
    // learns, or (wide) over 4096, which it cannot.
    auto burst = [&]<class T>(const string &type, int bits, int s, int b, T, int pool = 16) {
        vector<ulng> ps = primes(bits, s, size_t(pool), pool > 16), xs(size_t(pool) * b); ulng acc = 0; int n = pool * b;
        table.run(type, "burst-" + std::to_string(b) + (pool == 16 ? "" : "-wide"), double(n), [&] { for (int j = 0; j < n; ++j) { xs[j] = rng() % ps[j / b]; } }, [&] {
            ulng c = 0;
            for (int j = 0; j < n; ++j) {
                if (j % b == 0) { givenPrime<T>(ps[j / b]); }
                c = c * 31 + sqrt(T::raw(typename T::Word(xs[j]))).val();}
            acc = c;}, [&] { return acc; });};
    for (int b : {1, 16, 64, 100, 256, 1000, 4096}) {
        burst("D32-s20", 29, 20, b, DynModInt<2>()); burst("D32-s6", 29, 6, b, DynModInt<2>());
        burst("D32-s2", 29, 2, b, DynModInt<2>()); burst("D32-s1", 29, 1, b, DynModInt<2>());
        burst("D64-s32", 60, 32, b, DynModInt64<2>()); burst("D64-s12", 60, 12, b, DynModInt64<2>());
        burst("D64-s2", 60, 2, b, DynModInt64<2>()); burst("D64-s1", 60, 1, b, DynModInt64<2>());}
    for (int b : {1, 2, 4}) {
        for (int s : {1, 2, 6}) { burst("D32-s" + std::to_string(s), 29, s, b, DynModInt<2>(), 4096); }
        for (int s : {1, 2, 12, 32}) { burst("D64-s" + std::to_string(s), 60, s, b, DynModInt64<2>(), 4096); }}

    // Primality by input class, and the per-query costs of a judge problem: one setMod, or one setMod and one square root.
    vector<ulng> p30, p61, p64, q(N); ulng count = 0;
    rng.seed(12345);
    while (int(p30.size()) < N) { ulng x = (rng() >> 34) | 1 | (ulng(1) << 29); if (modint_detail::isPrime(x)) { p30.push_back(x); } }
    while (int(p61.size()) < N) { ulng x = (rng() >> 3) | 1 | (ulng(1) << 60); if (modint_detail::isPrime(x)) { p61.push_back(x); } }
    while (int(p64.size()) < N) { ulng x = rng() | 1 | (ulng(1) << 63); if (modint_detail::isPrime(x)) { p64.push_back(x); } }
    auto prime = [&](const string &name, auto g, const vector<ulng> &v) { table.run("prime", name, N, g, [&] { count = kPrime(v.data(), N); }, [&] { return count; }); };
    prime("p30", [] {}, p30); prime("p61", [] {}, p61); prime("p64", [] {}, p64);
    vector<ulng> p16; // primes between 2^10 and 2^16, where the hashed base can exceed the modulus
    for (ulng x = 1025; int(p16.size()) < N; x = x + 2 > 65535 ? 1025 : x + 2) { if (modint_detail::isPrime(x)) { p16.push_back(x); } }
    std::shuffle(p16.begin(), p16.end(), rng); prime("p16", [] {}, p16);
    prime("998244353", [&] { for (ulng &x : q) { x = 998244353; } }, q);
    prime("odd30", [&] { for (ulng &x : q) { x = (rng() >> 34) | 1; } }, q);
    prime("odd64", [&] { for (ulng &x : q) { x = rng() | 1; } }, q);
    prime("semi64", [&] { for (ulng &x : q) { x = p30[rng() % N] * (p30[rng() % N] + 2 * (rng() % 64)) | 1; } }, q);
    prime("any30", [&] { for (ulng &x : q) { x = rng() >> 34; } }, q); prime("any64", [&] { for (ulng &x : q) { x = rng(); } }, q);
    // The same with the first-level cache evicted before each call: the cost of the test's tables when the caller's data fills the cache.
    auto cold = [&](const string &name, auto g, const vector<ulng> &v) {
        table.net("prime", name, N, g, [&] { count = kPrimeEvict(v.data(), N); }, [&] { count = kSumEvict(v.data(), N); }, [&] { return count; });};
    cold("p30-evict", [] {}, p30); cold("p61-evict", [] {}, p61); cold("any64-evict", [&] { for (ulng &x : q) { x = rng(); } }, q);
    auto query = [&](const string &name, auto f) { table.run("query", name, N, [] {}, [&] { ulng c = 0; for (int i = 0; i < N; ++i) { c += f(i); } count = c; }, [&] { return count; }); };
    query("setmod-p30", [&](int i) { DynModInt<3>::setMod(uint(p30[i])); return ulng(DynModInt<3>::is_prime); });
    query("setmod-p30-64", [&](int i) { DynModInt64<3>::setMod(p30[i]); return ulng(DynModInt64<3>::is_prime); });
    query("setmod-p30-given", [&](int i) { DynModInt<3>::setMod(uint(p30[i]), 0); return ulng(DynModInt<3>::mod()); });
    query("setmod-p30-given64", [&](int i) { DynModInt64<3>::setMod(p30[i], 0); return DynModInt64<3>::mod(); });
    query("setmod-p61", [&](int i) { DynModInt64<3>::setMod(p61[i]); return ulng(DynModInt64<3>::is_prime); });
    query("setmod-p61-given", [&](int i) { DynModInt64<3>::setMod(p61[i], 0); return DynModInt64<3>::mod(); });
    query("setmod-pow2-given", [&](int i) { DynModInt<3>::setMod(uint(1) << (i % 31 + 1), 0); return ulng(DynModInt<3>::mod()); });
    query("setmod-pow2-given64", [&](int i) { DynModInt64<3>::setMod(ulng(1) << (i % 63 + 1), 0); return DynModInt64<3>::mod(); });
    query("setmod-m61-given", [&](int i) { DynModInt64<3>::setMod(Barrett64::MERSENNE61, 0); return DynModInt64<3>::mod() + i; });
    // One inverse of a small value after each setMod: a dynamic type's table of small inverses at its worst, refilled for every modulus.
    query("setmod-inv-tiny", [&](int i) { DynModInt<3>::setMod(uint(p30[i]), 0); return ulng(inv(DynModInt<3>::raw(uint(2 + i % 13))).val()); });
    query("setmod-inv-tiny64", [&](int i) { DynModInt64<3>::setMod(p61[i], 0); return inv(DynModInt64<3>::raw(ulng(2 + i % 13))).val(); });
    query("setmod-inv-b11", [&](int i) { DynModInt<3>::setMod(uint(p30[i]), 0); return ulng(inv(DynModInt<3>::raw(uint(257 + i % 1789))).val()); });
    query("setmod-inv-b1164", [&](int i) { DynModInt64<3>::setMod(p61[i], 0); return inv(DynModInt64<3>::raw(ulng(257 + i % 1789))).val(); });
    vector<uint> y(N);
    table.run("query", "sqrtmod-p30", N, [&] { for (int i = 0; i < N; ++i) { y[i] = uint(rng() % p30[i]); } }, [&] {
        ulng c = 0;
        for (int i = 0; i < N; ++i) { DynModInt<3>::setMod(uint(p30[i])); c = c * 31 + sqrt(DynModInt<3>::raw(y[i])).val(); }
        count = c;}, [&] { return count; });
    table.run("query", "sqrtmod-p30-given", N, [&] { for (int i = 0; i < N; ++i) { y[i] = uint(rng() % p30[i]); } }, [&] {
        ulng c = 0;
        for (int i = 0; i < N; ++i) { givenPrime<DynModInt<3>>(p30[i]); c = c * 31 + sqrt(DynModInt<3>::raw(y[i])).val(); }
        count = c;}, [&] { return count; });
    return 0;}
