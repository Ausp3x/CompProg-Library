#define LOCAL
#include "../../01-Core/02-debug.hpp"
#include "../../01-Core/18-bitset.hpp"
#include <cstdlib>
#include <iostream>
#include <limits>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using Bits = std::vector<unsigned char>;
static ulng seed = 42;
static std::string context;
static lng checks = 0;

[[noreturn]] static void fail(const std::string &message) {
    std::cerr << "FAIL seed=" << seed << " context=" << context << '\n' << message << '\n';
    std::exit(1);}

static void require(bool ok, const std::string &message) {
    ++checks;
    if (!ok) { fail(message); }}

template<class A, class B> static void expect(const A &actual, const B &expected, const std::string &operation) {
    ++checks;
    if (actual != expected) {
        std::ostringstream out;
        out << operation << " expected=" << expected << " actual=" << actual;
        fail(out.str());}}

static std::string binary(const Bits &v) {
    std::string result(v.size(), '0');
    for (size_t i = 0; i < v.size(); ++i) { result[v.size() - 1 - i] = char('0' + v[i]); }
    return result;}

static Bitset packed(const Bits &v) { return Bitset(binary(v)); }

static Bits pattern(size_t n, int kind, std::mt19937_64 &rng) {
    Bits v(n);
    for (size_t i = 0; i < n; ++i) {
        v[i] = static_cast<unsigned char>(kind == 1 || (kind == 2 && i % 2 == 0) ||
            (kind == 3 && (i == 0 || i + 1 == n || i % 64 == 63)) ||
            (kind == 4 && rng() % 2) || (kind == 5 && rng() % 97 == 0));}
    return v;}

static Bits shifted(const Bits &v, size_t k, bool left) {
    Bits result(v.size());
    if (k < v.size()) {
        for (size_t i = 0; i < v.size() - k; ++i) {
            result[left ? i + k : i] = v[left ? i : i + k];}}
    return result;}

static Bits rotated(const Bits &v, size_t k, bool left) {
    if (v.empty()) { return v; }
    k %= v.size();
    Bits result(v.size());
    for (size_t i = 0; i < v.size(); ++i) {
        result[left ? (i + k) % v.size() : (i + v.size() - k) % v.size()] = v[i];}
    return result;}

static unsigned char bitOp(int op, unsigned char x, unsigned char y) {
    return static_cast<unsigned char>(op == 0 ? y : op == 1 ? (x && y) : op == 2 ? (x || y) : op == 3 ? (x != y) : (x && !y));}

static Bits combined(const Bits &a, const Bits &b, int op) {
    Bits result(a.size());
    for (size_t i = 0; i < a.size(); ++i) { result[i] = bitOp(op, a[i], b[i]); }
    return result;}

static Bits combinedRange(const Bits &a, size_t l, size_t r, const Bits &b, size_t p, int op) {
    Bits result = a;
    for (size_t i = l; i < r; ++i) { result[i] = bitOp(op, a[i], b[p + i - l]); }
    return result;}

static Bits sub(const Bits &v, size_t l, size_t r) { return Bits(v.begin() + ptrdiff_t(l), v.begin() + ptrdiff_t(r)); }

static void combine(Bitset &a, const Bitset &b, int op) {
    Bitset *result = nullptr;
    if (op == 0) { result = &a.combine<Bitset::Op::Assign>(b); }
    if (op == 1) { result = &a.combine<Bitset::Op::And>(b); }
    if (op == 2) { result = &a.combine<Bitset::Op::Or>(b); }
    if (op == 3) { result = &a.combine<Bitset::Op::Xor>(b); }
    if (op == 4) { result = &a.combine<Bitset::Op::AndNot>(b); }
    require(result == &a, "combine must return receiver reference");}

static void combineShift(Bitset &a, const Bitset &b, int op, size_t k, bool left) {
    Bitset *result = nullptr;
    if (op == 0) { result = &a.combineShift<Bitset::Op::Assign>(b, k, left); }
    if (op == 1) { result = &a.combineShift<Bitset::Op::And>(b, k, left); }
    if (op == 2) { result = &a.combineShift<Bitset::Op::Or>(b, k, left); }
    if (op == 3) { result = &a.combineShift<Bitset::Op::Xor>(b, k, left); }
    if (op == 4) { result = &a.combineShift<Bitset::Op::AndNot>(b, k, left); }
    require(result == &a, "combineShift must return receiver reference");}

static void combineRange(Bitset &a, size_t l, size_t r, const Bitset &b, size_t p, int op) {
    Bitset *result = nullptr;
    if (op == 0) { result = &a.combineRange<Bitset::Op::Assign>(l, r, b, p); }
    if (op == 1) { result = &a.combineRange<Bitset::Op::And>(l, r, b, p); }
    if (op == 2) { result = &a.combineRange<Bitset::Op::Or>(l, r, b, p); }
    if (op == 3) { result = &a.combineRange<Bitset::Op::Xor>(l, r, b, p); }
    if (op == 4) { result = &a.combineRange<Bitset::Op::AndNot>(l, r, b, p); }
    require(result == &a, "combineRange must return receiver reference");}

static void verify(const Bitset &a, const Bits &v, bool scan = true) {
    const size_t n = v.size();
    expect(a.size(), n, "size"); expect(a.n, n, "stored size");
    expect(a.empty(), v.empty(), "empty");
    expect(a.blocks().size(), (n + 63) / 64, "block count");
    expect(a.a.size(), a.blocks().size(), "stored block count");
    if (n % 64) { expect(a.blocks().back() >> (n % 64), ulng(0), "unused tail bits"); }
    size_t count = 0, first = n, last = n;
    for (size_t i = 0; i < n; ++i) {
        if (a.test(i) != bool(v[i]) || a[i] != bool(v[i])) {
            fail("bit=" + std::to_string(i) + " expected=" + binary(v) + " actual=" + a.toString());}
        ++checks;
        if (v[i]) {
            ++count; last = i;
            if (first == n) { first = i;}}}
    expect(a.count(), count, "whole count"); expect(a.count(0, n), count, "whole-range count");
    expect(a.any(), bool(count), "any"); expect(a.none(), !count, "none"); expect(a.all(), count == n, "all");
    expect(a.findFirst(), first, "first"); expect(a.findLast(), last, "last");
    expect(a.findNext(n), n, "next sentinel"); expect(a.findPrev(0), n, "previous sentinel");
    expect(a.toString(), binary(v), "MSB-first string");
    expect(debugString(a), binary(v), "debugString ADL hook");

    if (scan) {
        size_t previous = n;
        for (size_t i = 0; i <= n; ++i) {
            expect(a.findPrev(i), previous, "previous position=" + std::to_string(i));
            if (i < n && v[i]) { previous = i; }}
        size_t next = n;
        for (size_t i = n; i > 0; --i) {
            expect(a.findNext(i - 1), next, "next position=" + std::to_string(i - 1));
            if (v[i - 1]) { next = i - 1; }}}}

static std::vector<size_t> shifts(size_t n) {
    std::vector<size_t> result{0, 1, 2, 31, 32, 63, 64, 65, 127, 128, 129,
        n ? n - 1 : 0, n, n + 1, std::numeric_limits<size_t>::max()};
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;}

static void unary(const Bits &v, bool exhaustive_ranges) {
    const std::string base = context;
    const size_t n = v.size();
    Bitset a = packed(v);
    verify(a, v);
    verify(Bitset(n), Bits(n));
    verify(Bitset(n, true), Bits(n, 1));

    std::vector<ulng> words(a.blocks().begin(), a.blocks().end());
    if (n % 64) { words.back() |= ~ulng(0) << (n % 64); }
    verify(Bitset::fromWords(n, words), v);
    Bits opposite = v;
    for (auto &bit : opposite) { bit = static_cast<unsigned char>(!bit); }
    verify(~a, opposite);

    Bitset b = a;
    b.flip();
    verify(b, opposite);
    b.reset();
    verify(b, Bits(n));
    b.set();
    verify(b, Bits(n, 1));

    b = a;
    b = b;
    verify(b, v);
    Bitset &self = b;
    b = std::move(self);
    verify(b, v);
    Bitset c(std::move(b));
    verify(c, v); verify(b, {});
    b = Bitset(79, true);
    b = std::move(c);
    verify(b, v); verify(c, {});
    c = Bitset(3, true);
    b.swap(c);
    verify(c, v); verify(b, Bits(3, 1));
    c.swap(c);
    verify(c, v);
    require(a == packed(v) && !(a != packed(v)), "equal copied values");
    require(a != Bitset(n + 1), "equality must include size");

    std::vector<size_t> points{0, n / 2, n};
    for (size_t p : {size_t(1), size_t(63), size_t(64), size_t(65), size_t(127), size_t(128), size_t(129)}) {
        if (p <= n) { points.push_back(p); }}
    if (exhaustive_ranges) {
        points.clear();
        for (size_t i = 0; i <= n; ++i) { points.push_back(i); }}
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    for (size_t l : points) {
        for (size_t r : points) {
            if (r < l) { continue; }
            context = base + " ranges l=" + std::to_string(l) + " r=" + std::to_string(r);
            size_t count = 0;
            for (size_t i = l; i < r; ++i) { count += v[i]; }
            expect(a.count(l, r), count, "range count");
            Bits mask(n), reset = v, set = v, flip = v;
            for (size_t i = l; i < r; ++i) { mask[i] = set[i] = 1; reset[i] = 0; flip[i] ^= 1; }
            verify(Bitset::rangeMask(n, l, r), mask, false);
            b = a;
            b.setRange(l, r);
            verify(b, set, false);
            b = a;
            b.setRange(l, r, false);
            verify(b, reset, false);
            b = a;
            b.resetRange(l, r);
            verify(b, reset, false);
            b = a;
            b.flipRange(l, r);
            verify(b, flip, false);}}

    for (size_t k : shifts(n)) {
        context = base + " shifts/rotates k=" + std::to_string(k);
        Bits left = shifted(v, k, true), right = shifted(v, k, false);
        verify(a << k, left, false); verify(a >> k, right, false);
        b = a;
        b <<= k;
        verify(b, left, false);
        b = a;
        b >>= k;
        verify(b, right, false);
        b = a;
        require(&b.rotateLeft(k) == &b, "rotateLeft return");
        verify(b, rotated(v, k, true), false);
        b = a;
        require(&b.rotateRight(k) == &b, "rotateRight return");
        verify(b, rotated(v, k, false), false);
        b = a;
        b.rotateLeft(k).rotateRight(k);
        verify(b, v, false);}
    verify(a, v);
    context = base;}

static void binaryTests(const Bits &v, const Bits &w, bool fused) {
    const std::string base = context;
    const Bitset a = packed(v), b = packed(w);
    Bits meet = combined(v, w, 1), join = combined(v, w, 2), diff = combined(v, w, 3);
    size_t ca = 0, co = 0, cx = 0;
    bool subset = true;
    for (size_t i = 0; i < v.size(); ++i) {
        ca += meet[i]; co += join[i]; cx += diff[i];
        if (v[i] && !w[i]) { subset = false; }}
    expect(a.countAnd(b), ca, "countAnd"); expect(a.countOr(b), co, "countOr"); expect(a.countXor(b), cx, "countXor");
    expect(a.intersects(b), bool(ca), "intersects"); expect(a.isSubsetOf(b), subset, "isSubsetOf");
    expect(a == b, v == w, "operator=="); expect(a != b, v != w, "operator!=");
    verify(a & b, meet, false); verify(a | b, join, false); verify(a ^ b, diff, false);
    verify(a - b, combined(v, w, 4), false);

    Bitset c = a;
    c &= b;
    verify(c, meet, false);
    c = a;
    c |= b;
    verify(c, join, false);
    c = a;
    c ^= b;
    verify(c, diff, false);
    c = a;
    c -= b;
    verify(c, combined(v, w, 4), false);
    c = a;
    c &= c;
    verify(c, v, false);
    c = a;
    c |= c;
    verify(c, v, false);
    c = a;
    c ^= c;
    verify(c, Bits(v.size()), false);
    c = a;
    c -= c;
    verify(c, Bits(v.size()), false);

    for (int op = 0; op < 5; ++op) {
        context = base + " combine op=" + std::to_string(op);
        c = a;
        combine(c, b, op);
        verify(c, combined(v, w, op), false);
        c = a;
        combine(c, c, op);
        verify(c, combined(v, v, op), false);
        if (!fused) { continue; }
        for (size_t k : shifts(v.size())) {
            for (bool left : {false, true}) {
                context = base + " combineShift op=" + std::to_string(op) +
                    " k=" + std::to_string(k) + " left=" + std::to_string(left);
                c = a;
                combineShift(c, b, op, k, left);
                verify(c, combined(v, shifted(w, k, left), op), false);
                c = a;
                combineShift(c, c, op, k, left);
                verify(c, combined(v, shifted(v, k, left), op), false);}}}
    verify(a, v, false); verify(b, w, false);
    context = base;}

static void lifecycle(size_t n) {
    context = "lifecycle n=" + std::to_string(n);
    for (bool value : {false, true}) {
        Bitset a(n, true);
        Bits v(n, 1);
        size_t smaller = n ? n - 1 : 0;
        a.resize(smaller); v.resize(smaller);
        verify(a, v);
        a.resize(n, value); v.resize(n, static_cast<unsigned char>(value));
        verify(a, v);
        a.resize(n, !value);
        verify(a, v);
        a.resize(n + 130, value); v.resize(n + 130, static_cast<unsigned char>(value));
        verify(a, v);
        a.resize(0);
        verify(a, {});
        a.resize(n, value);
        verify(a, Bits(n, static_cast<unsigned char>(value)));
        a.clear();
        verify(a, {});

        for (size_t i = 0; i < n + 2; ++i) { a.pushBack(i % 2 != 0); }
        Bits alternation(n + 2);
        for (size_t i = 0; i < alternation.size(); ++i) { alternation[i] = static_cast<unsigned char>(i % 2); }
        verify(a, alternation);
        for (size_t i = 0; i < n + 2; ++i) { a.popBack(); }
        verify(a, {});}
    if (n) {
        Bitset a(n);
        Bits v(n);
        auto first = a[0];
        a.set(n - 1); v[n - 1] = 1;
        first.flip(); v[0] ^= 1;
        verify(a, v);
        a.setRange(0, n); v.assign(n, 1);
        first = false; v[0] = 0;
        verify(a, v);
        auto last = a[n - 1];
        first = last; v[0] = v[n - 1];
        verify(a, v);
        first = first;
        verify(a, v);
        Bitset::Reference copy = first;
        copy.flip(); v[0] ^= 1;
        verify(a, v);
        std::vector<Bitset::Reference> refs{a[0], a[n - 1]};
        refs[1] = refs[0]; v[n - 1] = v[0];
        refs.push_back(copy);
        refs.back() = !refs.back(); v[0] ^= 1;
        verify(a, v);
        expect(Debug::to_string(a[0]), std::string(v[0] ? "true" : "false"), "debug bit proxy");
        expect(Debug::to_string(a), binary(v), "debug Bitset hook");}}

static void rangeTests(const Bits &v, const Bits &w, std::mt19937_64 &rng) {
    const std::string base = context;
    const size_t n = v.size();
    const Bitset a = packed(v);
    std::vector<size_t> points{0, n / 2, n, rng() % (n + 1), rng() % (n + 1)};
    for (size_t p : {size_t(1), size_t(63), size_t(64), size_t(65), size_t(127), size_t(128), size_t(129), size_t(1023), size_t(1025)}) {
        if (p <= n) { points.push_back(p); points.push_back(n - p); }}
    std::sort(points.begin(), points.end());
    points.erase(std::unique(points.begin(), points.end()), points.end());
    // Every pair of boundary points through 513 bits; longer sets sample 12 pairs so the O(n) oracle stays fast.
    std::vector<std::pair<size_t, size_t>> pairs;
    for (size_t l : points) { for (size_t r : points) { if (l <= r) { pairs.emplace_back(l, r); } } }
    if (n > 513) {
        std::shuffle(pairs.begin(), pairs.end(), rng);
        pairs.resize(std::min<size_t>(pairs.size(), 12));
        pairs.emplace_back(0, n);}
    for (auto [l, r] : pairs) {
        {
            const size_t len = r - l;
            context = base + " slice l=" + std::to_string(l) + " r=" + std::to_string(r);
            verify(a.slice(l, r), sub(v, l, r), false);
            for (int kind = 0; kind < 4; ++kind) {
                // Same-size source, exact-length source, longer source with a random offset, and the receiver itself.
                Bits src = kind == 0 ? w : kind == 3 ? v : pattern(len + (kind == 2 ? rng() % 200 : 0), 4, rng);
                size_t p = rng() % (src.size() - len + 1);
                if (kind == 2 && rng() % 2) { p = src.size() - len; }
                const Bitset s = packed(src);
                for (int op = 0; op < 5; ++op) {
                    context = base + " combineRange kind=" + std::to_string(kind) + " op=" + std::to_string(op) +
                        " l=" + std::to_string(l) + " r=" + std::to_string(r) + " p=" + std::to_string(p);
                    Bitset c = a;
                    combineRange(c, l, r, kind == 3 ? c : s, p, op);
                    verify(c, combinedRange(v, l, r, src, p, op), false);}}}}
    verify(a, v, false);
    context = base;}

static void rangeCombos(int limit) {
    for (int na = 0; na <= limit; ++na) {
        for (uint ma = 0; ma < (uint(1) << na); ++ma) {
            Bits v(na);
            for (int i = 0; i < na; ++i) { v[i] = static_cast<unsigned char>((ma >> i) & 1); }
            const Bitset a = packed(v);
            for (size_t l = 0; l <= v.size(); ++l) {
                for (size_t r = l; r <= v.size(); ++r) {
                    context = "exhaustive slice na=" + std::to_string(na) + " a=" + std::to_string(ma) + " l=" + std::to_string(l) + " r=" + std::to_string(r);
                    verify(a.slice(l, r), sub(v, l, r), false);
                    for (size_t p = 0; p + (r - l) <= v.size(); ++p) {
                        for (int op = 0; op < 5; ++op) {
                            context = "exhaustive aliased combineRange na=" + std::to_string(na) + " a=" + std::to_string(ma) +
                                " l=" + std::to_string(l) + " r=" + std::to_string(r) + " p=" + std::to_string(p) + " op=" + std::to_string(op);
                            Bitset c = a;
                            combineRange(c, l, r, c, p, op);
                            verify(c, combinedRange(v, l, r, v, p, op), false);}}}}
            for (int nb = 0; nb <= limit; ++nb) {
                for (uint mb = 0; mb < (uint(1) << nb); ++mb) {
                    Bits w(nb);
                    for (int i = 0; i < nb; ++i) { w[i] = static_cast<unsigned char>((mb >> i) & 1); }
                    const Bitset b = packed(w);
                    for (size_t l = 0; l <= v.size(); ++l) {
                        for (size_t r = l; r <= v.size(); ++r) {
                            for (size_t p = 0; p + (r - l) <= w.size(); ++p) {
                                for (int op = 0; op < 5; ++op) {
                                    context = "exhaustive combineRange na=" + std::to_string(na) + " nb=" + std::to_string(nb) +
                                        " a=" + std::to_string(ma) + " b=" + std::to_string(mb) + " l=" + std::to_string(l) +
                                        " r=" + std::to_string(r) + " p=" + std::to_string(p) + " op=" + std::to_string(op);
                                    Bitset c = a;
                                    combineRange(c, l, r, b, p, op);
                                    verify(c, combinedRange(v, l, r, w, p, op), false);}}}}}}}}
    std::cout << "PASS exhaustive slice and combineRange through length " << limit << " (all offsets, ops and aliasing)\n";}

static void exhaustive(bool quick) {
    for (int n = 0; n <= (quick ? 3 : 6); ++n) {
        for (uint mask = 0; mask < (uint(1) << n); ++mask) {
            Bits a(n);
            for (int i = 0; i < n; ++i) { a[i] = static_cast<unsigned char>((mask >> i) & 1); }
            context = "exhaustive unary n=" + std::to_string(n) + " mask=" + std::to_string(mask);
            unary(a, true);
            for (uint other = 0; other < (uint(1) << n); ++other) {
                Bits b(n);
                for (int i = 0; i < n; ++i) { b[i] = static_cast<unsigned char>((other >> i) & 1); }
                context = "exhaustive binary n=" + std::to_string(n) + " a=" + std::to_string(mask) + " b=" + std::to_string(other);
                binaryTests(a, b, n <= (quick ? 2 : 4));}}}
    std::cout << "PASS exhaustive small bitsets, range masks, set algebra and fused aliasing\n";}

static void boundaries(bool quick, bool stress) {
    std::mt19937_64 rng(seed ^ 0xD6E8FEB86659FD93ULL);
    std::vector<int> sizes{0, 1, 2, 7, 8, 31, 32, 33, 63, 64, 65, 127, 128, 129, 191,
        192, 193, 255, 256, 257, 511, 512, 513};
    if (!quick) {
        for (int n : {895, 896, 897, 959, 960, 961, 1023, 1024, 1025, 1087, 1088, 1089,
                2047, 2048, 2049, 4095, 4096, 4097, 8191, 8192, 8193}) { sizes.push_back(n); }}
    if (stress) { sizes.insert(sizes.end(), {16383, 16384, 16385, 32767, 32768, 32769}); }
    for (int n : sizes) {
        lifecycle(n);
        for (int kind = 0; kind < (quick ? 4 : 6); ++kind) {
            context = "boundaries n=" + std::to_string(n) + " pattern=" + std::to_string(kind);
            Bits a = pattern(n, kind, rng), b = pattern(n, (kind + 2) % 6, rng);
            unary(a, false); binaryTests(a, b, true); rangeTests(a, b, rng);}}
    std::cout << "PASS word/vector/dispatch boundaries, tails, scan sentinels, huge shifts and rotations\n";}

static void histories(bool quick, bool stress) {
    std::mt19937_64 rng(seed ^ 0x9E3779B97F4A7C15ULL);
    const int rounds = quick ? 8 : stress ? 96 : 32;
    const int steps = quick ? 60 : stress ? 500 : 180;
    const std::vector<int> sizes{0, 1, 2, 63, 64, 65, 127, 128, 129, 255, 256, 257,
        511, 512, 513, 1023, 1024, 1025, 4095, 4096, 4097};
    for (int round = 0; round < rounds; ++round) {
        Bits v = pattern(sizes[rng() % sizes.size()], 4, rng);
        Bitset a = packed(v);
        for (int step = 0; step < steps; ++step) {
            int op = int(rng() % 18);
            // Listed lengths are at most 4097; at most 500 pushes occur per history.
            int n = int(v.size()), i = n ? int(rng() % n) : 0;
            bool value = rng() % 2;
            context = "history round=" + std::to_string(round) + " step=" + std::to_string(step) +
                " op=" + std::to_string(op) + " n=" + std::to_string(n) + " i=" + std::to_string(i);
            if (op == 0) {
                int m = sizes[rng() % sizes.size()];
                a.resize(m, value); v.resize(m, static_cast<unsigned char>(value));}
            else if (op == 1) { a.pushBack(value); v.push_back(static_cast<unsigned char>(value)); }
            else if (op == 2 && n) { a.popBack(); v.pop_back(); }
            else if (op == 3 && n) { a.set(i, value); v[i] = static_cast<unsigned char>(value); }
            else if (op == 4 && n) { a.reset(i); v[i] = 0; }
            else if (op == 5 && n) { a.flip(i); v[i] ^= 1; }
            else if (op == 6 && n) {
                a[i] = value; v[i] = static_cast<unsigned char>(value);
                require(bool(a[i]) == value, "mutable proxy conversion");}
            else if (op == 7 && n) {
                int j = int(rng() % n);
                a[i] = a[j]; v[i] = v[j];
                a[i].flip(); v[i] ^= 1;
                a[i] = a[i];}
            else if (op == 8) {
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (r < l) { std::swap(l, r); }
                a.setRange(l, r, value);
                for (int j = l; j < r; ++j) { v[j] = static_cast<unsigned char>(value); }}
            else if (op == 9) {
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (r < l) { std::swap(l, r); }
                a.flipRange(l, r);
                for (int j = l; j < r; ++j) { v[j] ^= 1; }}
            else if (op == 10) {
                size_t k = rng() % (2 * n + 2);
                if (value) { a <<= k; } else { a >>= k; }
                v = shifted(v, k, value);}
            else if (op == 11) {
                size_t k = rng();
                if (value) { a.rotateLeft(k); } else { a.rotateRight(k); }
                v = rotated(v, k, value);}
            else if (op == 12) {
                int type = int(rng() % 5);
                size_t k = rng() % (n + 70);
                combineShift(a, a, type, k, value); v = combined(v, shifted(v, k, value), type);}
            else if (op == 13) {
                Bits w = pattern(n, 4, rng);
                Bitset b = packed(w);
                int type = int(rng() % 5);
                combine(a, b, type); v = combined(v, w, type);}
            else if (op == 14) { a.clear(); v.clear(); }
            else if (op == 15) {
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (r < l) { std::swap(l, r); }
                int m = r - l + int(rng() % 70), type = int(rng() % 5);
                Bits w = pattern(m, 4, rng);
                Bitset b = packed(w);
                size_t p = rng() % (m - (r - l) + 1);
                combineRange(a, l, r, b, p, type); v = combinedRange(v, l, r, w, p, type);}
            else if (op == 16) {
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (r < l) { std::swap(l, r); }
                a = a.slice(l, r); v = sub(v, l, r);}
            else if (op == 17) {
                int l = int(rng() % (n + 1)), r = int(rng() % (n + 1));
                if (r < l) { std::swap(l, r); }
                size_t p = rng() % (n - (r - l) + 1);
                int type = int(rng() % 5);
                combineRange(a, l, r, a, p, type); v = combinedRange(v, l, r, v, p, type);}
            verify(a, v);}}
    std::cout << "PASS seeded mutation/resize/proxy/copy/alias histories rounds=" << rounds << " steps=" << steps << '\n';}

static void countMatrix() {
    std::mt19937_64 rng(seed ^ 0x94D049BB133111EBULL);
    std::vector<int> sizes{0, 1, 2, 63, 64, 65, 127, 128, 129, 4095, 4096, 4097};
    for (int words : {15, 16, 17, 18, 19}) {
        for (int n : {64 * words - 1, 64 * words, 64 * words + 1}) { sizes.push_back(n); }}
    for (int n : sizes) {
        for (int kind = 0; kind < 6; ++kind) {
            Bits v = pattern(n, kind, rng), w = pattern(n, kind >= 4 ? kind : (kind + 2) % 6, rng);
            Bitset a = packed(v), b = packed(w);
            std::vector<int> prefix(n + 1), meet(n + 1), join(n + 1), diff(n + 1);
            for (int i = 0; i < n; ++i) {
                prefix[i + 1] = prefix[i] + v[i];
                meet[i + 1] = meet[i] + (v[i] && w[i]);
                join[i + 1] = join[i] + (v[i] || w[i]);
                diff[i + 1] = diff[i] + (v[i] != w[i]);}
            context = "count-matrix n=" + std::to_string(n) + " pattern=" + std::to_string(kind);
            const std::string base = context;
            expect(a.count(), size_t(prefix[n]), "count");
            expect(a.countAnd(b), size_t(meet[n]), "countAnd");
            expect(a.countOr(b), size_t(join[n]), "countOr");
            expect(a.countXor(b), size_t(diff[n]), "countXor");
            expect(a.countAnd(a), size_t(prefix[n]), "countAnd self");
            expect(a.countOr(a), size_t(prefix[n]), "countOr self");
            expect(a.countXor(a), size_t(0), "countXor self");
            std::vector<int> points{0, n / 2, n};
            for (int p : {1, 2, 7, 8, 11, 13,
                    31, 32, 63, 64, 65, 66,
                    127, 128, 129}) {
                if (p <= n) { points.push_back(p); points.push_back(n - p); }}
            for (int i = 0; i < 20; ++i) { points.push_back(int(rng() % (n + 1))); }
            std::sort(points.begin(), points.end());
            points.erase(std::unique(points.begin(), points.end()), points.end());
            for (int l : points) {
                for (int r : points) {
                    if (r < l) { continue; }
                    context = base + " l=" + std::to_string(l) + " r=" + std::to_string(r);
                    expect(a.count(l, r), size_t(prefix[r] - prefix[l]), "range count");}}
            context = base;
            verify(a, v, false); verify(b, w, false);}}
    std::cout << "PASS focused whole/fused/range count matrix (15/16/17 interior words, unaligned boundaries)\n";}

static void valueOperatorSmoke() {
    std::mt19937_64 rng(seed ^ 0xBF58476D1CE4E5B9ULL);
    for (int n : {0, 1, 63, 64, 65, 255, 1025, 1089}) {
        for (int kind : {0, 1, 4, 5}) {
            context = "value-operator smoke n=" + std::to_string(n) + " pattern=" + std::to_string(kind);
            const std::string base = context;
            Bits v = pattern(n, kind, rng), w = pattern(n, 4, rng), opposite = v;
            for (auto &bit : opposite) { bit ^= 1; }
            const Bitset a = packed(v), b = packed(w);
            verify(a & b, combined(v, w, 1), false);
            verify(a | b, combined(v, w, 2), false);
            verify(a ^ b, combined(v, w, 3), false);
            verify(a - b, combined(v, w, 4), false);
            verify(~a, opposite, false);

            Bitset moved = a;
            verify(std::move(moved) & b, combined(v, w, 1), false); verify(moved, {});
            moved = a;
            verify(std::move(moved) | b, combined(v, w, 2), false); verify(moved, {});
            moved = a;
            verify(std::move(moved) ^ b, combined(v, w, 3), false); verify(moved, {});
            moved = a;
            verify(std::move(moved) - b, combined(v, w, 4), false); verify(moved, {});
            moved = a;
            verify(~std::move(moved), opposite, false); verify(moved, {});
            for (size_t k : shifts(n)) {
                context = base + " k=" + std::to_string(k);
                verify(a << k, shifted(v, k, true), false);
                verify(a >> k, shifted(v, k, false), false);
                moved = a;
                verify(std::move(moved) << k, shifted(v, k, true), false); verify(moved, {});
                moved = a;
                verify(std::move(moved) >> k, shifted(v, k, false), false); verify(moved, {});}
            verify(a, v, false); verify(b, w, false);}}
    std::cout << "PASS focused nonmutating Boolean/complement/shift value operators (lvalue and rvalue operands)\n";}

static int death(const std::string &name) {
    Bitset a(65), b(64);
    if (name == "test") { (void)a.test(65); }
    else if (name == "const-index") { const Bitset &c = a; (void)c[65]; }
    else if (name == "proxy-index") { a[65] = true; }
    else if (name == "set") { a.set(65); }
    else if (name == "reset") { a.reset(65); }
    else if (name == "flip") { a.flip(65); }
    else if (name == "range-order") { a.setRange(2, 1); }
    else if (name == "range-end") { a.resetRange(0, 66); }
    else if (name == "range-flip") { a.flipRange(65, 66); }
    else if (name == "count-range") { (void)a.count(2, 1); }
    else if (name == "mask-range") { (void)Bitset::rangeMask(65, 0, 66); }
    else if (name == "next") { (void)a.findNext(66); }
    else if (name == "prev") { (void)a.findPrev(66); }
    else if (name == "pop-empty") { Bitset().popBack(); }
    else if (name == "string") { (void)Bitset("010x"); }
    else if (name == "words-short") { const std::vector<ulng> words{1}; (void)Bitset::fromWords(65, words); }
    else if (name == "words-long") { const std::vector<ulng> words{1, 2, 3}; (void)Bitset::fromWords(65, words); }
    else if (name == "and-size") { a &= b; }
    else if (name == "or-size") { a |= b; }
    else if (name == "xor-size") { a ^= b; }
    else if (name == "difference-size") { a -= b; }
    else if (name == "intersects-size") { (void)a.intersects(b); }
    else if (name == "subset-size") { (void)a.isSubsetOf(b); }
    else if (name == "count-and-size") { (void)a.countAnd(b); }
    else if (name == "count-or-size") { (void)a.countOr(b); }
    else if (name == "count-xor-size") { (void)a.countXor(b); }
    else if (name == "assign-size") { a.combine<Bitset::Op::Assign>(b); }
    else if (name == "shift-size") { a.combineShift<Bitset::Op::Or>(b, 1000); }
    else if (name == "combine-range-order") { a.combineRange<Bitset::Op::Or>(2, 1, b); }
    else if (name == "combine-range-end") { a.combineRange<Bitset::Op::Or>(60, 66, b); }
    else if (name == "combine-range-source") { a.combineRange<Bitset::Op::Xor>(0, 65, b); }
    else if (name == "combine-range-offset") { a.combineRange<Bitset::Op::Xor>(0, 1, b, 65); }
    else if (name == "slice-order") { (void)a.slice(2, 1); }
    else if (name == "slice-end") { (void)a.slice(0, 66); }
    else { std::cerr << "Unknown death case: " << name << '\n'; return 2; }
    std::cerr << "Precondition was not rejected: " << name << '\n';
    return 0;}

int main(int argc, char **argv) {
    std::string mode = "full";
    bool counts_only = false;
    for (int i = 1; i < argc; ++i) {
        std::string option = argv[i];
        if (option == "--death" && i + 1 < argc) { return death(argv[++i]); }
        if (option == "--mode" && i + 1 < argc) { mode = argv[++i]; }
        else if (option == "--counts-only") { counts_only = true; }
        else if (option == "--seed" && i + 1 < argc) { seed = std::stoull(argv[++i]); }}
    if (mode != "quick" && mode != "full" && mode != "stress") { return 2; }
    std::cout << std::unitbuf << "seed=" << seed << " mode=" << mode << " counts-only=" << counts_only << '\n';
    countMatrix();
    valueOperatorSmoke();
    if (!counts_only) {
        exhaustive(mode == "quick");
        rangeCombos(mode == "quick" ? 3 : mode == "stress" ? 5 : 4);
        boundaries(mode == "quick", mode == "stress");
        histories(mode == "quick", mode == "stress");}
    std::cout << "PASS non-removable checks=" << checks << '\n';}
