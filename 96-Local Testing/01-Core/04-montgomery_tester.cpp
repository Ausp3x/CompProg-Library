#include "../../01-Core/04-montgomery.hpp"

ulng seed;
std::mt19937_64 rng;
string phase;
ulll ctx_m, ctx_a, ctx_b, ctx_x;
int checks = 0;

string decimal(ulll x) {
    if (!x) { return "0"; }
    string s;
    while (x) { s += char('0' + x % 10); x /= 10; }
    reverse(s.begin(), s.end()); return s;}
void checkEqual(ulll got, ulll expected, string_view operation) {
    ++checks;
    if (got != expected) {
        cerr << "FAIL seed=" << seed << " phase=" << phase << " operation=" << operation
             << " smallest-known-reproducer: m=" << decimal(ctx_m) << " a=" << decimal(ctx_a)
             << " b=" << decimal(ctx_b) << " x=" << decimal(ctx_x)
             << " expected=" << decimal(expected) << " actual=" << decimal(got) << '\n';
        std::exit(1); }}

template<typename M>
struct Oracle {
    using T = decltype(M().mod);
    using W = typename M::Wide;
    T mod, rinv, r;

    Oracle(T m) : mod(m), rinv(1 % m), r((W(1) << M::BITS) % m) {
        // Repeated exact division by two in Z/mZ, independent of REDC/inv.
        for (int i = 0; i < M::BITS; i++) {
            rinv = T((W(rinv) + (rinv & 1 ? mod : 0)) / 2); }}

    T red(W x) const { return T((x % mod) * rinv % mod); }
    T init(T a) const { return T(W(a) * r % mod); }
    T mul(T a, T b) const { return T(W(a) * b % mod); }
    T pow(T a, ulng e) const {
        T out = 1 % mod;
        for (; e; e >>= 1) {
            if (e & 1) { out = mul(out, a); }
            a = mul(a, a); }
        return out;}
};

template<typename M>
void pairCase(const M &m, const Oracle<M> &o, decltype(M().mod) a, decltype(M().mod) b, bool powers) {
    using T = decltype(M().mod); using W = typename M::Wide;
    ctx_m = m.mod; ctx_a = a; ctx_b = b;
    T x = m.init(a), y = m.init(b);
    checkEqual(x, o.init(a), "init"); checkEqual(m.get(x), a % m.mod, "get/init");
    checkEqual(m.mul(x, y), o.init(o.mul(a, b)), "mul");
    checkEqual(m.get(m.mul(x, y)), o.mul(a, b), "decoded product");
    if (powers) {
        for (ulng e : {ulng(0), ulng(1), ulng(2), ulng(63), ~ulng(0)}) {
            ctx_b = e;
            checkEqual(m.pow(a, e), o.pow(a, e), "pow");
            checkEqual(m.get(m.powMont(x, e)), o.pow(a, e), "powMont"); }}
    if (m.mod < (T(1) << (M::BITS - 2))) {
        T lx = x + m.mod, ly = y + m.mod, z = m.mulLazy(lx, ly);
        ctx_a = lx; ctx_b = ly;
        checkEqual(z < 2 * m.mod, true, "lazy output bound");
        checkEqual(m.normalize(z), o.red(W(lx) * ly), "mulLazy");
        checkEqual(m.normalize(m.mulLazy(z, lx)), o.red(W(z) * lx), "lazy chain"); }}

template<typename M>
void modulusCase(decltype(M().mod) mod, bool bulk) {
    using T = decltype(M().mod); using W = typename M::Wide;
    constexpr T TOP = ~T(0); constexpr W R = W(1) << M::BITS;
    M m(mod); Oracle<M> o(mod);
    ctx_m = mod;
    checkEqual(W(o.r) * o.rinv % mod, 1 % mod, "oracle radix inverse identity");
    checkEqual(T(mod * m.inv), TOP, "negative inverse");
    checkEqual(m.rsq, W(o.r) * o.r % mod, "rsq");
    M copied = m; M moved = std::move(copied); copied = M(); copied = moved;
    checkEqual(copied.pow(TOP, 3), o.pow(TOP, 3), "copy/move/assignment");
    vector<T> values = {0, 1, T(mod / 2), T(mod - 1), mod, TOP};
    for (T a : values) { for (T b : values) { pairCase(m, o, a, b, b == 0); }}
    vector<W> dividends = {0, 1, W(mod - 1), W(mod), R - 1, R * mod - 1, R * mod / 2};
    if (mod > 1) { dividends.push_back(R); }
    for (W x : dividends) {
        ctx_x = x; checkEqual(m.red(x), o.red(x), "red boundary");
        if (mod < (T(1) << (M::BITS - 1))) {
            T lazy = m.redLazy(x);
            checkEqual(lazy < 2 * mod, true, "redLazy bound");
            checkEqual(m.normalize(lazy), o.red(x), "redLazy boundary"); }}
    if (!bulk) { return; }
    string saved_phase = phase;
    for (int n : {0, 1, 2, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 65, 257}) {
        for (int offset = 0; offset < 8; offset++) {
            phase = saved_phase + " bulk n=" + std::to_string(n) + " offset=" + std::to_string(offset);
            vector<T> raw(n + 16), av(n + 16), bv(n + 16), out(n + 16, TOP), expected(n);
            T *a = av.data() + offset, *b = bv.data() + offset, *c = out.data() + offset;
            for (int i = 0; i < n; i++) { raw[i + offset] = T(rng()); b[i] = T(rng() % mod); }
            m.init(raw.data() + offset, a, n);
            for (int i = 0; i < n; i++) {
                ctx_a = raw[i + offset]; ctx_b = i;
                checkEqual(a[i], o.init(raw[i + offset]), "bulk init"); expected[i] = o.red(W(a[i]) * b[i]); }
            m.mul(a, b, c, n);
            for (int i = 0; i < n; i++) { ctx_b = i; checkEqual(c[i], expected[i], "bulk mul"); }
            checkEqual(out[offset + n], TOP, "bulk tail guard");
            if (offset) { checkEqual(out[offset - 1], TOP, "bulk prefix guard"); }
            m.mul(a, b, a, n);
            for (int i = 0; i < n; i++) { checkEqual(a[i], expected[i], "bulk alias left"); }
            m.init(raw.data() + offset, a, n); m.mul(a, b, b, n);
            for (int i = 0; i < n; i++) { checkEqual(b[i], expected[i], "bulk alias right"); }
            for (int i = 0; i < n; i++) { expected[i] = o.red(W(a[i]) * a[i]); }
            m.mul(a, a, a, n);
            for (int i = 0; i < n; i++) { checkEqual(a[i], expected[i], "bulk all alias"); }
            for (int i = 0; i < n; i++) { expected[i] = o.init(raw[i + offset]); }
            m.init(raw.data() + offset, raw.data() + offset, n);
            for (int i = 0; i < n; i++) { checkEqual(raw[i + offset], expected[i], "bulk init alias"); }
            for (int i = 0; i < n; i++) { expected[i] = o.red(raw[i + offset]); }
            m.get(raw.data() + offset, c, n);
            for (int i = 0; i < n; i++) { checkEqual(c[i], expected[i], "bulk get separate"); }
            checkEqual(out[offset + n], TOP, "bulk get tail guard");
            m.get(raw.data() + offset, raw.data() + offset, n);
            for (int i = 0; i < n; i++) { checkEqual(raw[i + offset], expected[i], "bulk get alias"); }
            if (mod < (T(1) << (M::BITS - 2))) {
                for (int i = 0; i < n; i++) { a[i] = T(rng() % (2 * mod)); b[i] = T(rng() % (2 * mod)); }
                for (int i = 0; i < n; i++) { expected[i] = o.red(W(a[i]) * b[i]); }
                m.mulLazy(a, b, c, n);
                for (int i = 0; i < n; i++) {
                    checkEqual(c[i] < 2 * mod, true, "bulk lazy bound");
                    checkEqual(m.normalize(c[i]), expected[i], "bulk lazy"); }
                checkEqual(out[offset + n], TOP, "bulk lazy tail guard");
                m.mulLazy(a, b, a, n);
                for (int i = 0; i < n; i++) { checkEqual(m.normalize(a[i]), expected[i], "bulk lazy left alias"); }
                for (int i = 0; i < n; i++) { expected[i] = o.red(W(a[i]) * b[i]); }
                m.mulLazy(a, b, b, n);
                for (int i = 0; i < n; i++) { checkEqual(m.normalize(b[i]), expected[i], "bulk lazy right alias"); }
                for (int i = 0; i < n; i++) { expected[i] = o.red(W(a[i]) * a[i]); }
                m.mulLazy(a, a, a, n);
                for (int i = 0; i < n; i++) { checkEqual(m.normalize(a[i]), expected[i], "bulk lazy all alias"); }}}}
    phase = saved_phase;
    m.init(nullptr, nullptr, 0); m.get(nullptr, nullptr, 0); m.mul(nullptr, nullptr, nullptr, 0);
    if (mod < (T(1) << (M::BITS - 2))) { m.mulLazy(nullptr, nullptr, nullptr, 0); }}

template<typename M>
void run(string mode) {
    using T = decltype(M().mod); using W = typename M::Wide;
    constexpr T TOP = ~T(0);
    phase = std::to_string(M::BITS) + "-boundary";
    checkEqual(M().mod, 1, "default modulus");
    vector<T> mods = {1, 3, 5, 7, 17, 998244353, 1000000007, TOP, T(TOP - 2)};
    for (int i = 2; i < M::BITS; i++) {
        mods.push_back(T((T(1) << i) - 1)); mods.push_back(T((T(1) << i) + 1)); }
    sort(mods.begin(), mods.end()); mods.erase(unique(mods.begin(), mods.end()), mods.end());
    for (T mod : mods) { modulusCase<M>(mod, mod == 1 || mod == 3 || mod == 998244353 || mod >= TOP - 2
                                               || mod == (T(1) << (M::BITS - 2)) - 1
                                               || mod == (T(1) << (M::BITS - 1)) - 1); }
    M regression(TOP);
    ctx_m = TOP; ctx_a = ctx_b = TOP - 1; ctx_x = W(TOP - 1) * (TOP - 1);
    checkEqual(regression.mul(TOP - 1, TOP - 1), 1, "carry beyond double word");
    checkEqual(regression.red((W(TOP) << M::BITS) - 1), TOP - 1, "max REDC dividend");
    cout << "PASS " << phase << " conversion/power/carry/bulk aliases and offsets\n";
    phase = std::to_string(M::BITS) + "-exhaustive";
    int bound = mode == "quick" ? 17 : mode == "full" ? 65 : 129;
    for (int mod = 1; mod <= bound; mod += 2) {
        M m(mod); Oracle<M> o(mod); ctx_m = mod;
        for (int a = 0; a < 2 * mod; a++) {
            ctx_a = a; checkEqual(m.normalize(a), a % mod, "normalize exhaustive");
            for (int b = 0; b < 2 * mod; b++) {
                ctx_b = b;
                T x = m.mulLazy(a, b);
                checkEqual(x < T(2 * mod), true, "lazy exhaustive bound");
                checkEqual(m.normalize(x), o.red(W(a) * b), "lazy exhaustive");
                if (a < mod && b < mod) { checkEqual(m.mul(a, b), o.red(W(a) * b), "canonical exhaustive"); }}}
        for (int x = 0; x < 16 * mod; x++) { ctx_x = x; checkEqual(m.red(x), o.red(x), "red exhaustive"); }}
    cout << "PASS " << phase << " odd moduli <=" << bound << "\n";
    phase = std::to_string(M::BITS) + "-random";
    int count = mode == "quick" ? 500 : mode == "full" ? 10000 : 100000;
    for (int i = 0; i < count; i++) {
        T mod = T(rng()) | 1;
        M m(mod); Oracle<M> o(mod);
        pairCase(m, o, T(rng()), T(rng()), i % 41 == 0);
        W x = (W(T(rng() % mod)) << M::BITS) | T(rng()); ctx_x = x;
        checkEqual(m.red(x), o.red(x), "red random full domain");
        if (mod < (T(1) << (M::BITS - 1))) {
            T lazy = m.redLazy(x);
            checkEqual(lazy < 2 * mod, true, "redLazy random bound");
            checkEqual(m.normalize(lazy), o.red(x), "redLazy random"); }}
    cout << "PASS " << phase << " cases=" << count << '\n';}

template<typename M>
void death(const string &name) {
    using T = decltype(M().mod); using W = typename M::Wide;
    M m(17);
    T a[8]{}, b[8]{}, c[8]{};
    if (name == "zero") { M invalid(0); }
    else if (name == "even") { M invalid(2); }
    else if (name == "red") { m.red(W(m.mod) << M::BITS); }
    else if (name == "mul-a") { m.mul(17, 0); }
    else if (name == "mul-b") { m.mul(0, 17); }
    else if (name == "get") { m.get(17); }
    else if (name == "powMont") { m.powMont(17, 0); }
    else if (name == "lazy-mod") { M((T(1) << (M::BITS - 1)) + 1).redLazy(0); }
    else if (name == "lazy-red") { m.redLazy(W(m.mod) << M::BITS); }
    else if (name == "normalize-mod") { M((T(1) << (M::BITS - 1)) + 1).normalize(0); }
    else if (name == "normalize") { m.normalize(34); }
    else if (name == "lazy-mul-mod") { M((T(1) << (M::BITS - 2)) + 1).mulLazy(0, 0); }
    else if (name == "lazy-mul-a") { m.mulLazy(34, 0); }
    else if (name == "lazy-mul-b") { m.mulLazy(0, 34); }
    else if (name == "bulk-mul-n") { m.mul(a, b, c, -1); }
    else if (name == "bulk-mul-null-a") { m.mul(nullptr, b, c, 1); }
    else if (name == "bulk-mul-null-b") { m.mul(a, nullptr, c, 1); }
    else if (name == "bulk-mul-null-c") { m.mul(a, b, nullptr, 1); }
    else if (name == "bulk-mul-a") { a[7] = 17; m.mul(a, b, c, 8); }
    else if (name == "bulk-mul-b") { b[7] = 17; m.mul(a, b, c, 8); }
    else if (name == "bulk-init-n") { m.init(a, c, -1); }
    else if (name == "bulk-init-null-a") { m.init(nullptr, c, 1); }
    else if (name == "bulk-init-null-c") { m.init(a, nullptr, 1); }
    else if (name == "bulk-get-n") { m.get(a, c, -1); }
    else if (name == "bulk-get-null-a") { m.get(nullptr, c, 1); }
    else if (name == "bulk-get-null-c") { m.get(a, nullptr, 1); }
    else if (name == "bulk-get-a") { a[7] = 17; m.get(a, c, 8); }
    else if (name == "bulk-lazy-n") { m.mulLazy(a, b, c, -1); }
    else if (name == "bulk-lazy-null-a") { m.mulLazy(nullptr, b, c, 1); }
    else if (name == "bulk-lazy-null-b") { m.mulLazy(a, nullptr, c, 1); }
    else if (name == "bulk-lazy-null-c") { m.mulLazy(a, b, nullptr, 1); }
    else if (name == "bulk-lazy-mod") { M((T(1) << (M::BITS - 2)) + 1).mulLazy(a, b, c, 0); }
    else if (name == "bulk-lazy-a") { a[7] = 34; m.mulLazy(a, b, c, 8); }
    else if (name == "bulk-lazy-b") { b[7] = 34; m.mulLazy(a, b, c, 8); }
    else { cerr << "Unknown death case " << name << '\n'; std::exit(2); }
    cerr << "Precondition was not rejected: " << name << '\n'; std::exit(1);}

int main(int argc, char **argv) {
    string mode = "full"; seed = 42;
    for (int i = 1; i < argc; i++) {
        string arg = argv[i];
        if (arg == "--mode" && i + 1 < argc) { mode = argv[++i]; }
        else if (arg == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }
        else if (arg == "--death32" && i + 1 < argc) { death<Montgomery32>(argv[++i]); }
        else if (arg == "--death64" && i + 1 < argc) { death<Montgomery64>(argv[++i]); }
        else { cerr << "Invalid argument " << arg << '\n'; return 2; }}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    rng.seed(seed);
    static_assert(std::is_same_v<Montgomery, Montgomery64>);
    run<Montgomery32>(mode); run<Montgomery64>(mode);
    cout << "PASS Montgomery seed=" << seed << " checks=" << checks << '\n';}
