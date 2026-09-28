#include "../../01-Core/03-barrett.hpp"

int checks = 0;
ulng seed;
string mode;

string hex128(ulll x) {
    std::ostringstream out; out << "0x" << std::hex << ulng(x >> 64)
        << std::setw(16) << std::setfill('0') << ulng(x); return out.str(); }
void check(ulll got, ulll expected, string_view op, ulll m = 0, ulll a = 0, ulll b = 0) {
    ++checks; if (got == expected) { return; }
    cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << op
         << " mod=" << hex128(m) << " a=" << hex128(a) << " b=" << hex128(b)
         << " expected=" << hex128(expected) << " actual=" << hex128(got)
         << "\nsmallest known reproducer: first failing input in deterministic ordered corpus\n";
    std::exit(1); }
void passed(string_view name) { cout << "PASS " << name << " checks=" << checks << '\n'; }

ulng refPow(ulng a, ulng e, ulng m) {
    ulng r = 1 % m;
    for (int i = 63; i >= 0; --i) {
        r = ulng(ulll(r) * r % m);
        if ((e >> i) & 1) { r = ulng(ulll(r) * a % m); }}
    return r; }

template<class Reducer, class Word, class Wide>
void features(std::mt19937_64 &rng) {
    constexpr int W = std::numeric_limits<Word>::digits;
    constexpr Word MAX = std::numeric_limits<Word>::max();
    vector<Word> mods = {1, 2, 3, 4, 5, 7, 10, 127, 255, 257, 65521, 65535,
                         65537, Word(998244353), Word(1000000007), MAX, Word(MAX - 1), Word(MAX - 2)};
    for (int i = 1; i < W; ++i) {
        Word p = Word(1) << i; mods.push_back(p);
        mods.push_back(p - 1); mods.push_back(p + 1); }
    for (int i = 0; i < (mode == "quick" ? 20 : 200); ++i) {
        Word m = Word(rng()); if (m) { mods.push_back(m); }}
    sort(mods.begin(), mods.end()); mods.erase(unique(mods.begin(), mods.end()), mods.end());
    constexpr Wide TOP = ~Wide(0);
    for (Word m : mods) {
        Reducer red(m), copy = red, moved = std::move(copy);
        vector<Wide> values = {0, 1, Wide(m - 1), Wide(m), Wide(m) + 1, 2 * Wide(m) - 1,
                              Wide(MAX), Wide(MAX) + 1, Wide(MAX) * MAX, TOP, TOP - 1,
                              Wide(m) * MAX, Wide(m) * MAX - 1};
        for (int i = 0; i < (mode == "quick" ? 6 : 40); ++i) {
            values.push_back((Wide(Word(rng())) << W) | Word(rng())); }
        for (Wide x : values) {
            check(red.reduce(x), x % m, "wide-reduce", m, x);
            check(moved.reduce(x), x % m, "copy-move", m, x); }
        vector<Word> words = {0, 1, Word(m - 1), m, Word(m + 1), Word(MAX / 2), MAX};
        for (Word a : words) { for (Word b : words) {
            check(red.mul(a, b), Wide(a) * b % m, "mul-boundary", m, a, b);
            auto fixed = red.multiplier(b), copied = fixed; auto moved_fixed = std::move(copied);
            check(fixed.mul(a), Wide(a) * b % m, "fixed-mul-boundary", m, a, b);
            check(moved_fixed.mul(a), Wide(a) * b % m, "fixed-copy-move", m, a, b); }}
        for (ulng e : {ulng(0), ulng(1), ulng(2), ulng(63), ulng(64), ulng(65), ~ulng(0)}) {
            for (Word a : {Word(0), Word(1), Word(m - 1), MAX}) {
                check(red.pow(a, e), refPow(a, e, m), "pow", m, a, e); }}
        auto detached = Reducer(m).multiplier(MAX);
        check(detached.mul(MAX), Wide(MAX) * MAX % m, "fixed-detached-lifetime", m, MAX, MAX);
    }
    passed(W == 32 ? "32-bit modulus/dividend boundaries, pow, copy/move, fixed" :
                      "64-bit modulus/dividend boundaries, pow, copy/move, fixed");
    int count = mode == "quick" ? 2000 : mode == "full" ? 60000 : 600000;
    for (int i = 0; i < count; ++i) {
        Word m = Word(rng()); if (!m) { m = 1; }
        Reducer red(m);
        Word a = Word(rng()), b = Word(rng()); Wide x = (Wide(a) << W) | b;
        check(red.reduce(x), x % m, "random-wide-reduce", m, x);
        check(red.mul(a, b), Wide(a) * b % m, "random-mul", m, a, b);
        check(red.multiplier(b).mul(a), Wide(a) * b % m, "random-fixed", m, a, b);
        if (i % 101 == 0) {
            ulng e = rng(); check(red.pow(a, e), refPow(a, e, m), "random-pow", m, a, e); }
    }
    passed(W == 32 ? "32-bit seeded random" : "64-bit seeded random");
    int max_mod = mode == "quick" ? 32 : mode == "full" ? 128 : 256;
    int max_x = mode == "quick" ? 512 : mode == "full" ? 16384 : 65536;
    for (int m = 1; m <= max_mod; ++m) {
        Reducer red{Word(m)};
        for (int x = 0; x < max_x; ++x) { check(red.reduce(Wide(x)), x % m, "exhaustive-small", m, x); }
        for (int b = 0; b < max_mod; ++b) {
            auto fixed = red.multiplier(Word(b));
            for (int a = 0; a < max_mod; ++a) {
                check(fixed.mul(Word(a)), a * b % m, "exhaustive-small-fixed", m, a, b); }}
    }
    passed(W == 32 ? "32-bit exhaustive small dividends and fixed factors" :
                      "64-bit exhaustive small dividends and fixed factors");
}

template<class Reducer, class Word, class Wide>
void batches(std::mt19937_64 &rng) {
    constexpr int W = std::numeric_limits<Word>::digits;
    constexpr Word MAX = std::numeric_limits<Word>::max();
    vector<Word> mods = {1, 2, 3, 5, 17, 65537, Word(998244353), Word(1000000007), Word(2147483647),
                         Word(Word(1) << (W - 1)), Word((Word(1) << (W - 1)) + 1), MAX - 1, MAX};
    if constexpr (W == 64) { mods.push_back(Word(2305843009213693951ULL)); }
    vector<int> sizes; for (int i = 0; i <= 65; ++i) { sizes.push_back(i); }
    for (int n : {127, 128, 129, 255, 256, 257, 4095, 4096, 4097}) { sizes.push_back(n); }
    for (Word m : mods) {
        Reducer red(m); red.mul(nullptr, nullptr, nullptr, 0);
        auto fixed = red.multiplier(MAX); fixed.mul(nullptr, nullptr, 0);
        for (int n : sizes) { for (int offset = 0; offset < (mode == "quick" ? 2 : 8); ++offset) {
            vector<Word> a(n + 20, MAX), b(n + 20, MAX), out(n + 20, MAX), expected(n);
            for (int i = 0; i < n; ++i) {
                a[offset + i] = Word(rng()); b[offset + i] = Word(rng());
                expected[i] = Word(Wide(a[offset + i]) * b[offset + i] % m); }
            red.mul(a.data() + offset, b.data() + offset, out.data() + offset, n);
            for (int i = 0; i < n; ++i) {
                check(out[offset + i], expected[i], "batch", m, a[offset + i], b[offset + i]); }
            for (int i = 0; i < offset; ++i) { check(out[i], MAX, "batch-leading-guard", m, n, offset); }
            for (int i = offset + n; i < int(out.size()); ++i) { check(out[i], MAX, "batch-trailing-guard", m, n, offset); }
            auto left = a, right = b, both = a;
            red.mul(left.data() + offset, b.data() + offset, left.data() + offset, n);
            red.mul(a.data() + offset, right.data() + offset, right.data() + offset, n);
            red.mul(both.data() + offset, both.data() + offset, both.data() + offset, n);
            fixed.mul(a.data() + offset, out.data() + offset, n);
            auto fixed_inplace = a; fixed.mul(fixed_inplace.data() + offset, fixed_inplace.data() + offset, n);
            for (int i = 0; i < n; ++i) {
                Word x = a[offset + i];
                check(left[offset + i], expected[i], "batch-alias-left", m, x, b[offset + i]);
                check(right[offset + i], expected[i], "batch-alias-right", m, x, b[offset + i]);
                check(both[offset + i], Wide(x) * x % m, "batch-alias-all", m, x, x);
                check(out[offset + i], Wide(x) * MAX % m, "fixed-batch", m, x, MAX);
                check(fixed_inplace[offset + i], Wide(x) * MAX % m, "fixed-batch-alias", m, x, MAX); }
        }}
    }
    passed(W == 32 ? "32-bit batches: size/unaligned/alias/guards/fixed" :
                      "64-bit batches: size/unaligned/alias/guards/fixed");
}

void highProducts(std::mt19937_64 &rng) {
    vector<ulll> boundary = {0, 1, (ulll(1) << 64) - 1, ulll(1) << 64,
                             (ulll(1) << 64) + 1, ~ulll(0), ~ulll(0) - 1,
                             ulll(1) << 127, (ulll(1) << 127) - 1};
    auto test = [] (ulll a, ulll b) {
        ulll hi = 0, lo = 0;
        for (int i = 127; i >= 0; --i) {
            hi = (hi << 1) | (lo >> 127); lo <<= 1;
            if ((b >> i) & 1) { ulll old = lo; lo += a; hi += lo < old; }}
        check(Barrett64::highProduct(a, b), hi, "high128-product", 0, a, b); };
    for (ulll a : boundary) { for (ulll b : boundary) { test(a, b); }}
    for (int i = 0; i < (mode == "quick" ? 200 : mode == "full" ? 10000 : 100000); ++i) {
        test((ulll(rng()) << 64) | rng(), (ulll(rng()) << 64) | rng()); }
    passed("exact high128 carries vs bitwise 256-bit product");
}

int main(int argc, char **argv) {
    seed = 20260927; mode = "quick";
    if (argc > 1 && string(argv[1]) == "--death") {
        string kind = argv[2];
        uint a = 1; ulng b = 1;
        if (kind == "mod32") { Barrett32 bad(0); }
        else if (kind == "mod64") { Barrett64 bad(0); }
        else if (kind == "n32") { Barrett32(3).mul(&a, &a, &a, -1); }
        else if (kind == "n64") { Barrett64(3).mul(&b, &b, &b, -1); }
        else if (kind == "null32-a") { Barrett32(3).mul(nullptr, &a, &a, 1); }
        else if (kind == "null32-b") { Barrett32(3).mul(&a, nullptr, &a, 1); }
        else if (kind == "null32-out") { Barrett32(3).mul(&a, &a, nullptr, 1); }
        else if (kind == "null64-a") { Barrett64(3).mul(nullptr, &b, &b, 1); }
        else if (kind == "null64-b") { Barrett64(3).mul(&b, nullptr, &b, 1); }
        else if (kind == "null64-out") { Barrett64(3).mul(&b, &b, nullptr, 1); }
        else if (kind == "fixed32-n") { Barrett32(3).multiplier(2).mul(&a, &a, -1); }
        else if (kind == "fixed64-n") { Barrett64(3).multiplier(2).mul(&b, &b, -1); }
        else if (kind == "fixed32-null-a") { Barrett32(3).multiplier(2).mul(nullptr, &a, 1); }
        else if (kind == "fixed32-null-out") { Barrett32(3).multiplier(2).mul(&a, nullptr, 1); }
        else if (kind == "fixed64-null-a") { Barrett64(3).multiplier(2).mul(nullptr, &b, 1); }
        else if (kind == "fixed64-null-out") { Barrett64(3).multiplier(2).mul(&b, nullptr, 1); }
        else { cerr << "unknown death case\n"; return 2; }
        return 3;
    }
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        else if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); }
        else { return 2; }}
    cout << "Barrett seed=" << seed << " mode=" << mode << '\n';
    std::mt19937_64 rng(seed); highProducts(rng);
    features<Barrett32, uint, ulng>(rng); features<Barrett64, ulng, ulll>(rng);
    batches<Barrett32, uint, ulng>(rng); batches<Barrett64, ulng, ulll>(rng);
    check(Barrett(17).mul(123, 456), 123 * 456 % 17, "Barrett-alias");
    passed("all Barrett features");
}
