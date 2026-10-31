#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef __linux__
#include <sys/mman.h>
#endif

// Word kernels on little-endian base 2^64 words; n, m count words, r never aliases an input unless stated.
namespace infint_detail {
    using W = ulng;
    using WW = ulll;
    static_assert(std::endian::native == std::endian::little);
    constexpr int KARATSUBA = 28, KARATSUBA_SQR = 40, TOOM3 = 128, TOOM3_SQR = 128, TRUNC = 32;
#ifdef __AVX2__
    constexpr int FFT = 96, FFT_SQR = 80, NEWTON_DIV = 160, NEWTON_LONG = 96, CACHED_DIV = 96, CACHED_MOD = 64, SCALED_TREE = 80, CYCLIC_REM = 24, CYCLIC_REM_LOG = 2, UNBALANCED_FFT = 5, UNBALANCED_FFT_BIG = 7, BINARY_GCD = 96;
#else
    constexpr int FFT = 640, FFT_SQR = 512, NEWTON_DIV = 768, NEWTON_LONG = 192, CACHED_DIV = 192, CACHED_MOD = 192, SCALED_TREE = 1 << 30, CYCLIC_REM = 48, CYCLIC_REM_LOG = 16, UNBALANCED_FFT = 32, UNBALANCED_FFT_BIG = 32, BINARY_GCD = 48;
#endif
    constexpr int BURNIKEL = 48, KNUTH_Q = 8, KNUTH_FRESH = 6144, KNUTH_CACHED = 2048, FRESH_CYCLIC = 3, HENSEL = 64, HALFGCD = 2048, RADIX = 64, MONTGOMERY_POW = 2;
    constexpr size_t HUGE = size_t(1) << 21;
    // T: O(1), M: O(1); quotient words below which Knuth D beats a quotient block of an m-word divisor, with a prepared (cached) or a fresh context.
    constexpr int knuthGate(int m, bool cached) { return max(KNUTH_Q, (cached ? KNUTH_CACHED : KNUTH_FRESH) / m); }
    // T: O(1), M: O(1); block words from which the cyclic transform product of an m-word operand beats the linear product: the transform's cost per word grows with log m.
    constexpr int cyclicGate(int m) { return CYCLIC_REM + CYCLIC_REM_LOG * max(0, std::bit_width(uint(m)) - 8); }
    // The scaled remainder tree of the print: leaves of RADIX = 2^SCALED_LEAF blocks, SCALED_GUARD guard words below every fraction.
    constexpr int SCALED_LEAF = std::countr_zero(uint(RADIX)), SCALED_GUARD = 2;

    // T: O(1), M: O(n); storage of n words: 2 MiB-aligned multiples of 2 MiB advised as transparent huge pages on Linux from 2 MiB up, 64-byte aligned below.
    inline W *hugeAlloc(size_t n) {
        size_t bytes = max(8 * n, size_t(8)); bool big = bytes >= HUGE;
        if (big) { bytes = (bytes + HUGE - 1) & ~(HUGE - 1); }
        W *p = static_cast<W *>(::operator new(bytes, std::align_val_t(big ? HUGE : 64)));
#ifdef __linux__
        if (big) { madvise(p, bytes, MADV_HUGEPAGE); }
#endif
        return p;}
    // T: O(1), M: O(1); releases the storage of n words from hugeAlloc.
    inline void hugeFree(W *p, size_t n) { ::operator delete(p, std::align_val_t(8 * n >= HUGE ? HUGE : 64)); }
    // S: O(n), U: O(n) per growth, Q: O(1), M: O(n); a table of T in huge-page storage that grows to the largest size used and lives for the process (roots, transform scratch).
    template<typename T> struct HugeVec {
        T *p = nullptr; int n = 0;
        T &operator[](int i) { return p[i]; }
        void resize(int m) {
            if (m <= n) { return; }
            T *q = reinterpret_cast<T *>(hugeAlloc((size_t(m) * sizeof(T) + 7) / 8));
            if (n) { std::copy(p, p + n, q); hugeFree(reinterpret_cast<W *>(p), (size_t(n) * sizeof(T) + 7) / 8); }
            p = q; n = m;}
    };
    // S: O(1), U: O(1) amortized (a new block maps and doubles), Q: O(1), M: O(peak live words); thread-local bump arena of huge-page blocks: Scratch frames release in LIFO order, blocks double, the largest emptied block stays as the spare.
    struct Arena {
        struct Block { Block *prev; W *beg, *end; };
        Block *cur = nullptr, *spare = nullptr; W *top = nullptr;
        ~Arena() { drop(spare); while (cur) { drop(std::exchange(cur, cur->prev)); } }
        static Block *make(size_t n) { n = max(n, HUGE / 8); W *p = hugeAlloc(n); return new Block{nullptr, p, p + n}; }
        static void drop(Block *b) { if (b) { hugeFree(b->beg, size_t(b->end - b->beg)); delete b; } }
        W *alloc(size_t n) {
            if (!cur || size_t(cur->end - top) < n) { push(n); }
            return std::exchange(top, top + n);}
        void push(size_t n) {
            Block *b = spare && size_t(spare->end - spare->beg) >= n ? std::exchange(spare, nullptr) : make(max(n, cur ? 2 * size_t(cur->end - cur->beg) : size_t(0)));
            b->prev = cur; cur = b; top = b->beg;}
        void release(Block *blk, W *mark) {
            while (cur != blk) {
                Block *b = std::exchange(cur, cur->prev);
                if (spare && spare->end - spare->beg >= b->end - b->beg) { drop(b); } else { drop(std::exchange(spare, b)); }}
            top = mark;}
    };
    // T: O(1), M: O(1); the calling thread's arena.
    inline Arena &arena() { thread_local Arena a; return a; }
    constexpr W POISON = 0xABABABABABABABABull;
    // T: O(1), M: O(n); one arena frame of n uninitialized words; INFINT_SCRATCH_HEAP (sanitizer test builds) takes each frame from the heap filled with POISON, so a kernel relying on zeroed scratch fails there.
    struct Scratch {
#ifdef INFINT_SCRATCH_HEAP
        W *p; size_t cap;
        explicit Scratch(size_t n) : p(new W[max(n, size_t(1))]), cap(n) { std::fill(p, p + max(n, size_t(1)), POISON); }
        ~Scratch() { delete[] p; }
#else
        Arena &a; Arena::Block *blk; W *mark, *p; size_t cap;
        explicit Scratch(size_t n) : a(arena()), blk(a.cur), mark(a.top), p(a.alloc(n)), cap(n) {}
        ~Scratch() { a.release(blk, mark); }
#endif
        Scratch(const Scratch &) = delete;
        Scratch &operator=(const Scratch &) = delete;
        operator W *() const { return p; }
    };

    inline void mul(W *r, const W *a, int n, const W *b, int m);
    inline void square(W *r, const W *a, int n);
    inline W divWord(W *q, const W *a, int n, W d);

    // T: O(n), M: O(1); the length of a without leading zero words.
    inline int norm(const W *a, int n) { while (n > 0 && a[n - 1] == 0) { --n; } return n; }
    // T: O(n), M: O(1); three-way comparison of the normalized a (n words) and b (m words).
    inline int cmp(const W *a, int n, const W *b, int m) {
        if (n != m) { return n < m ? -1 : 1; }
        for (int i = n - 1; i >= 0; --i) { if (a[i] != b[i]) { return a[i] < b[i] ? -1 : 1; } }
        return 0;}
    // T: O(n), M: O(1); r = a + b for n >= m; r may alias a or b; returns the carry.
    inline W add(W *r, const W *a, int n, const W *b, int m) {
        unsigned long long c = 0; int i = 0;
        for (; i + 4 <= m; i += 4) {
            r[i] = __builtin_addcll(a[i], b[i], c, &c); r[i + 1] = __builtin_addcll(a[i + 1], b[i + 1], c, &c);
            r[i + 2] = __builtin_addcll(a[i + 2], b[i + 2], c, &c); r[i + 3] = __builtin_addcll(a[i + 3], b[i + 3], c, &c);}
        for (; i < m; ++i) { r[i] = __builtin_addcll(a[i], b[i], c, &c); }
        for (; i < n; ++i) { r[i] = __builtin_addcll(a[i], 0, c, &c); }
        return c;}
    // T: O(n), M: O(1); a += w; returns the carry out of the n words.
    inline W addWord(W *a, int n, W w) {
        for (int i = 0; i < n && w; ++i) { W s = a[i] + w; w = s < w; a[i] = s; }
        return w;}
    // T: O(n), M: O(1); r = a - b for n >= m; r may alias a or b; returns the borrow.
    inline W sub(W *r, const W *a, int n, const W *b, int m) {
        unsigned long long c = 0; int i = 0;
        for (; i + 4 <= m; i += 4) {
            r[i] = __builtin_subcll(a[i], b[i], c, &c); r[i + 1] = __builtin_subcll(a[i + 1], b[i + 1], c, &c);
            r[i + 2] = __builtin_subcll(a[i + 2], b[i + 2], c, &c); r[i + 3] = __builtin_subcll(a[i + 3], b[i + 3], c, &c);}
        for (; i < m; ++i) { r[i] = __builtin_subcll(a[i], b[i], c, &c); }
        for (; i < n; ++i) { r[i] = __builtin_subcll(a[i], 0, c, &c); }
        return c;}
    // T: O(n), M: O(1); a -= w; returns the borrow out of the n words.
    inline W subWord(W *a, int n, W w) {
        for (int i = 0; i < n && w; ++i) { W s = a[i] - w; w = a[i] < w; a[i] = s; }
        return w;}
    // T: O(n), M: O(1); r = a * w; r may alias a; returns the carry word.
    inline W mulWord(W *r, const W *a, int n, W w) {
        W c = 0;
        for (int i = 0; i < n; ++i) { WW t = WW(a[i]) * w + c; r[i] = W(t); c = W(t >> 64); }
        return c;}
    // T: O(n), M: O(1); r += a * w; returns the carry word.
    [[gnu::noinline]] inline W addMul(W *r, const W *a, int n, W w) {
        W c = 0;
        int i = 0;
        for (; i + 2 <= n; i += 2) {
            { WW t = WW(a[i]) * w + r[i] + c; r[i] = W(t); c = W(t >> 64); }
            { WW t = WW(a[i + 1]) * w + r[i + 1] + c; r[i + 1] = W(t); c = W(t >> 64); }}
        for (; i < n; ++i) { WW t = WW(a[i]) * w + r[i] + c; r[i] = W(t); c = W(t >> 64); }
        return c;}
    // T: O(n), M: O(1); r -= a * w; returns the borrow word. Both rows are pair-unrolled and never inlined: inlined into the Knuth D and Hensel loops, GCC spills
    // the 128-bit product of the carry chain, 1.5-2.0x slower at 64-1024 words in both builds.
    [[gnu::noinline]] inline W subMul(W *r, const W *a, int n, W w) {
        W c = 0;
        int i = 0;
        for (; i + 2 <= n; i += 2) {
            { WW t = WW(a[i]) * w + c; W lo = W(t), x = r[i]; r[i] = x - lo; c = W(t >> 64) + (x < lo); }
            { WW t = WW(a[i + 1]) * w + c; W lo = W(t), x = r[i + 1]; r[i + 1] = x - lo; c = W(t >> 64) + (x < lo); }}
        for (; i < n; ++i) { WW t = WW(a[i]) * w + c; W lo = W(t), x = r[i]; r[i] = x - lo; c = W(t >> 64) + (x < lo); }
        return c;}
    // T: O(n), M: O(1); r = a << s for 0 < s < 64 (r may alias a or start above it); returns the bits shifted out.
    inline W shl(W *r, const W *a, int n, int s) {
        W out = a[n - 1] >> (64 - s);
        for (int i = n - 1; i > 0; --i) { r[i] = (a[i] << s) | (a[i - 1] >> (64 - s)); }
        r[0] = a[0] << s;
        return out;}
    // T: O(n), M: O(1); r = a >> s for 0 < s < 64 (r may alias a or start below it); returns the bits shifted out.
    inline W shr(W *r, const W *a, int n, int s) {
        W out = a[0] << (64 - s);
        for (int i = 0; i + 1 < n; ++i) { r[i] = (a[i] >> s) | (a[i + 1] << (64 - s)); }
        r[n - 1] = a[n - 1] >> s;
        return out;}
    // T: O(n), M: O(1); r += b (m <= n words) with no carry out (asserted).
    inline void addInto(W *r, int n, const W *b, int m) { [[maybe_unused]] W c = add(r, r, n, b, m); assert(c == 0); }
    // T: O(n), M: O(1); clears n words.
    inline void zero(W *r, int n) { if (n > 0) { std::fill(r, r + n, 0); } }
    // T: O(n), M: O(1); r -= b (m <= n words) with no borrow out (asserted).
    inline void subFrom(W *r, int n, const W *b, int m) { [[maybe_unused]] W c = sub(r, r, n, b, m); assert(c == 0); }
    // T: O(M(n + m)) word products (the dispatcher's bound), M: O(n + m); r = a * b for any lengths, r zero-filled to n + m words.
    inline void mulAny(W *r, const W *a, int n, const W *b, int m) {
        int la = norm(a, n), lb = norm(b, m);
        zero(r, n + m);
        if (la == 0 || lb == 0) { return; }
        if (la >= lb) { mul(r, a, la, b, lb); }
        else { mul(r, b, lb, a, la); }}

    // T: O(n * m) word products, M: O(1); n >= m >= 1, r has n + m words.
    inline void mulBasecase(W *r, const W *a, int n, const W *b, int m) {
        r[n] = mulWord(r, a, n, b[0]);
        for (int j = 1; j < m; ++j) { r[n + j] = addMul(r + j, a, n, b[j]); }}
    // T: O(n^2 / 2) word products, M: O(1); r has 2n words.
    inline void squareBasecase(W *r, const W *a, int n) {
        std::fill(r, r + 2 * n, 0);
        for (int i = 0; i + 1 < n; ++i) { r[n + i] = addMul(r + 2 * i + 1, a + i + 1, n - i - 1, a[i]); }
        W c = 0;
        for (int i = 0; i < 2 * n; ++i) { W x = r[i]; r[i] = (x << 1) | c; c = x >> 63; }
        for (int i = 0; i < n; ++i) {
            WW sq = WW(a[i]) * a[i];
            WW t = WW(r[2 * i]) + W(sq) + c; r[2 * i] = W(t); c = W(t >> 64);
            t = WW(r[2 * i + 1]) + W(sq >> 64) + c; r[2 * i + 1] = W(t); c = W(t >> 64);}}
    // T: O(n^1.585) word products, M: O(n); n >= m > n / 2, r has n + m words.
    inline void karatsuba(W *r, const W *a, int n, const W *b, int m) {
        int h = n / 2, t = n - h, u = m - h;
        mulAny(r, a, h, b, h); mulAny(r + 2 * h, a + h, t, b + h, u);
        Scratch s(4 * t + 4);
        W *sa = s, *sb = sa + t + 1, *z = sb + t + 1;
        sa[t] = add(sa, a + h, t, a, h);
        std::fill(sb, sb + t + 1, 0);
        if (u >= h) { sb[u] = add(sb, b + h, u, b, h); }
        else { sb[h] = add(sb, b, h, b + h, u); }
        mulAny(z, sa, t + 1, sb, t + 1);
        subFrom(z, 2 * t + 2, r, 2 * h); subFrom(z, 2 * t + 2, r + 2 * h, t + u);
        addInto(r + h, n + m - h, z, norm(z, 2 * t + 2));}
    // T: O(n^1.585) word products, M: O(n); r (2n words) = a^2.
    inline void karatsubaSquare(W *r, const W *a, int n) {
        int h = n / 2, t = n - h;
        mulAny(r, a, h, a, h); mulAny(r + 2 * h, a + h, t, a + h, t);
        Scratch s(3 * t + 3);
        W *sa = s, *z = sa + t + 1;
        sa[t] = add(sa, a + h, t, a, h);
        mulAny(z, sa, t + 1, sa, t + 1);
        subFrom(z, 2 * t + 2, r, 2 * h); subFrom(z, 2 * t + 2, r + 2 * h, 2 * t);
        addInto(r + h, 2 * n - h, z, norm(z, 2 * t + 2));}
    // T: O(k), M: O(1); evaluations at 1, -1 (sign in sm) and 2 of the k, k, k2-word thirds of x; each k + 1 words.
    inline void toomEval(const W *x, int k, int k2, W *p1, W *pm, W *p2, int &sm) {
        W *s = pm;
        std::copy(x, x + k, s); s[k] = add(s, s, k, x + 2 * k, k2);
        p1[k] = s[k] + add(p1, s, k, x + k, k);
        std::copy(x + 2 * k, x + 2 * k + k2, p2); std::fill(p2 + k2, p2 + k, 0);
        p2[k] = shl(p2, p2, k, 1); p2[k] += add(p2, p2, k, x + k, k);
        shl(p2, p2, k + 1, 1); p2[k] += add(p2, p2, k, x, k);
        if (s[k] || cmp(s, k, x + k, k) >= 0) { s[k] -= sub(s, s, k, x + k, k); sm = 1; }
        else { sub(s, x + k, k, s, k); sm = -1; }}
    // T: O(n), M: O(n); bodrato interpolation of r1 = r(1), rm = |r(-1)| with sign sm, r2 = r(2); r holds r(0) in [0, 2k) and r(inf) in [4k, 2n).
    inline void toomInterpolate(W *r, int n, int k, W *r1, W *rm, W *r2, W *t2, int sm) {
        int len = 2 * k + 2, k2 = n - 2 * k;
        std::copy(r1, r1 + len, t2); subFrom(t2, len, r, 2 * k);
        if (sm > 0) { subFrom(r2, len, rm, len); subFrom(r1, len, rm, len); }
        else { addInto(r2, len, rm, len); addInto(r1, len, rm, len); }
        divWord(r2, r2, len, 3); shr(r1, r1, len, 1);
        subFrom(r2, len, t2, len); shr(r2, r2, len, 1);
        Scratch d(2 * k2 + 1); d[2 * k2] = shl(d, r + 4 * k, 2 * k2, 1);
        subFrom(r2, len, d, 2 * k2 + 1);
        subFrom(t2, len, r1, len); subFrom(t2, len, r + 4 * k, 2 * k2);
        subFrom(r1, len, r2, len);
        std::fill(r + 2 * k, r + 4 * k, 0);
        addInto(r + k, 2 * n - k, r1, norm(r1, len));
        addInto(r + 2 * k, 2 * n - 2 * k, t2, norm(t2, len));
        addInto(r + 3 * k, 2 * n - 3 * k, r2, norm(r2, len));}
    // T: O(n^1.465) word products, M: O(n); equal lengths n >= 6, r has 2n words.
    inline void toom3(W *r, const W *a, const W *b, int n) {
        int k = (n + 2) / 3, k2 = n - 2 * k, len = 2 * k + 2;
        Scratch buf(6 * (k + 1) + 4 * len);
        W *p1 = buf, *pm = p1 + k + 1, *p2 = pm + k + 1, *q1 = p2 + k + 1, *qm = q1 + k + 1, *q2 = qm + k + 1;
        W *r1 = q2 + k + 1, *rm = r1 + len, *r2 = rm + len, *t2 = r2 + len;
        int sa, sb;
        toomEval(a, k, k2, p1, pm, p2, sa); toomEval(b, k, k2, q1, qm, q2, sb);
        mulAny(r1, p1, k + 1, q1, k + 1); mulAny(rm, pm, k + 1, qm, k + 1); mulAny(r2, p2, k + 1, q2, k + 1);
        mulAny(r, a, k, b, k); mulAny(r + 4 * k, a + 2 * k, k2, b + 2 * k, k2);
        toomInterpolate(r, n, k, r1, rm, r2, t2, sa * sb);}
    // T: O(n^1.465) word products, M: O(n); r (2n words) = a^2 for n >= 6.
    inline void toom3Square(W *r, const W *a, int n) {
        int k = (n + 2) / 3, k2 = n - 2 * k, len = 2 * k + 2;
        Scratch buf(3 * (k + 1) + 4 * len);
        W *p1 = buf, *pm = p1 + k + 1, *p2 = pm + k + 1, *r1 = p2 + k + 1, *rm = r1 + len, *r2 = rm + len, *t2 = r2 + len;
        int sa;
        toomEval(a, k, k2, p1, pm, p2, sa);
        mulAny(r1, p1, k + 1, p1, k + 1); mulAny(rm, pm, k + 1, pm, k + 1); mulAny(r2, p2, k + 1, p2, k + 1);
        mulAny(r, a, k, a, k); mulAny(r + 4 * k, a + 2 * k, k2, a + 2 * k, k2);
        toomInterpolate(r, n, k, r1, rm, r2, t2, 1);}
    // T: O((n / c) * M(c + m)), M: O(c + m); c-word chunks of a times b, each through the dispatcher.
    inline void unbalancedMul(W *r, const W *a, int n, const W *b, int m, int c) {
        std::fill(r, r + n + m, 0);
        Scratch t(size_t(c + m));
        for (int i = 0; i < n; i += c) {
            int len = min(c, n - i);
            mulAny(t, b, m, a + i, len);
            addInto(r + i, m + len, t, norm(t, m + len));}}
    // T: O(n^2 / 2) word products below TRUNC, else one full product, M: O(n); r = a * b mod B^n for n-word a, b.
    inline void truncatedMul(W *r, const W *a, const W *b, int n) {
        if (n < TRUNC) {
            zero(r, n);
            for (int j = 0; j < n; ++j) { addMul(r + j, a, n - j, b[j]); }
            return;}
        Scratch t(2 * size_t(n)); mulAny(t, a, n, b, n);
        std::copy(t.p, t.p + n, r);}
    // T: O(1), M: O(1); limbs of a cyclic limb convolution holding len words of residue of an n-word by m-word product: the power of two at or above 2 max(len, n, m).
    inline int cyclicLimbs(int len, int n, int m) { return int(std::bit_ceil(uint(2 * max({len, n, m})))); }
    // T: NA, M: O(1); a transform plan: r 2^lg complex points, k-bit pieces and the output length len in words (lg = 0: no plan).
    struct FftPlan { int lg, r, k, len; };
    inline void fftRun(W *r, const FftPlan &p, const W *a, int n, const W *b, int m, bool cyclic);
    constexpr uint NTT_PRIMES[3] = {2013265921, 1811939329, 469762049}, NTT_ROOTS[3] = {31, 13, 3};
    constexpr int NTT_MAX_LOG = 26, NTT_MAX_WORDS = 1 << 28;
    // T: O(log(e)), M: O(1); a^e mod p on 32-bit words.
    constexpr uint modPow32(uint a, uint e, uint p) {
        ulng r = 1, b = a % p;
        for (; e; e >>= 1, b = b * b % p) { if (e & 1) { r = r * b % p; } }
        return uint(r);}

    // S: O(n) roots per prime, Q: O(n * log(n)), M: O(n); Montgomery NTT modulo P with 2^k | P - 1 on the block roots rt[B + b] = g^((P - 1) bitrev_B(b) / (2B)):
    // forward from natural order to the block order of the roots, depth-first radix-4 above LEAF, inverse back scaled by 1 / n.
    template<uint P, uint G>
    struct Ntt32 {
        static constexpr uint NINV = []() { uint x = P; for (int i = 0; i < 5; ++i) { x *= 2 - P * x; } return -x; }();
        static constexpr uint R2 = uint((ulll(1) << 64) % P);
        static constexpr uint ONE = uint((ulng(1) << 32) % P);
        static constexpr int LEAF = 1 << 12;

        static constexpr uint red(ulng x) { uint r = uint((x + ulng(uint(x) * NINV) * P) >> 32); return r >= P ? r - P : r; }
        static constexpr uint mul(uint a, uint b) { return red(ulng(a) * b); }
        static constexpr uint from(uint a) { return mul(a, R2); }
        static uint add(uint a, uint b) { uint s = a + b; return s >= P ? s - P : s; }
        static uint sub(uint a, uint b) { return a - b + (a < b ? P : 0); }
        static HugeVec<uint> &roots(bool inverse) { static HugeVec<uint> rt, irt; return inverse ? irt : rt; }
        static void ensure(int n) {
            HugeVec<uint> &rt = roots(false), &irt = roots(true);
            int old = rt.n;
            if (old >= n) { return; }
            rt.resize(n); irt.resize(n);
            for (int lvl = max(old, 1); lvl < n; lvl *= 2) {
                uint w = from(modPow32(G, (P - 1) / uint(2 * lvl), P)), iw = from(modPow32(G, P - 1 - (P - 1) / uint(2 * lvl), P)), cur = ONE, icur = ONE;
                for (int k = 0, rev = 0; k < lvl; ++k) {
                    rt[lvl + rev] = cur; irt[lvl + rev] = icur; cur = mul(cur, w); icur = mul(icur, iw);
                    int bit = lvl >> 1; while (rev & bit) { rev ^= bit; bit >>= 1; } rev ^= bit;}}}
#ifdef __AVX2__
        static __m256i vred(__m256i x0, __m256i x1) {
            __m256i p32 = _mm256_set1_epi32(int(P)), p64 = _mm256_set1_epi64x(lng(P)), ninv = _mm256_set1_epi32(int(NINV));
            __m256i q0 = _mm256_mul_epu32(_mm256_mul_epu32(x0, ninv), p64), q1 = _mm256_mul_epu32(_mm256_mul_epu32(x1, ninv), p64);
            __m256i r0 = _mm256_srli_epi64(_mm256_add_epi64(x0, q0), 32), r1 = _mm256_srli_epi64(_mm256_add_epi64(x1, q1), 32);
            __m256i r = _mm256_blend_epi32(r0, _mm256_slli_epi64(r1, 32), 0xAA);
            return _mm256_min_epu32(r, _mm256_sub_epi32(r, p32));}
        static __m256i vadd(__m256i a, __m256i b) {
            __m256i s = _mm256_add_epi32(a, b);
            return _mm256_min_epu32(s, _mm256_sub_epi32(s, _mm256_set1_epi32(int(P))));}
        static __m256i vsub(__m256i a, __m256i b) {
            __m256i d = _mm256_sub_epi32(a, b);
            return _mm256_min_epu32(d, _mm256_add_epi32(d, _mm256_set1_epi32(int(P))));}
        static __m256i vmul(__m256i a, __m256i b) {
            return vred(_mm256_mul_epu32(a, b), _mm256_mul_epu32(_mm256_srli_epi64(a, 32), _mm256_srli_epi64(b, 32)));}
        static __m256i load(const uint *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
        static void store(uint *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
        static __m256i swap128(__m256i x) { return _mm256_permute2x128_si256(x, x, 0x01); }
        static __m256i oddRoots(const uint *w) { return _mm256_or_si256(_mm256_slli_epi64(_mm256_cvtepu32_epi64(_mm_loadu_si128(reinterpret_cast<const __m128i *>(w))), 32), _mm256_set1_epi64x(lng(ONE))); }
#endif
        // One radix-2 level of len pairs with the block root w (forward) or its inverse (inverse).
        static void pass2(uint *a, int len, uint w, bool inverse) {
            int j = 0;
#ifdef __AVX2__
            __m256i vw = _mm256_set1_epi32(int(w));
            for (; j + 8 <= len; j += 8) {
                __m256i u = load(a + j), v = load(a + j + len);
                if (inverse) { store(a + j, vadd(u, v)); store(a + j + len, vmul(vsub(u, v), vw)); }
                else { v = vmul(v, vw); store(a + j, vadd(u, v)); store(a + j + len, vsub(u, v)); }}
#endif
            for (; j < len; ++j) {
                uint u = a[j], v = a[j + len];
                if (inverse) { a[j] = add(u, v); a[j + len] = mul(sub(u, v), w); }
                else { v = mul(v, w); a[j] = add(u, v); a[j + len] = sub(u, v); }}}
        // One radix-4 level over 4 len points: block roots s = rt[i], t = rt[2i], it = rt[2i + 1] forward, their inverses backward.
        static void pass4(uint *a, int len, int i, bool inverse) {
            const uint *rt = roots(inverse).p;
            uint s = rt[i], t = rt[2 * i], it = rt[2 * i + 1];
            uint *a0 = a, *a1 = a + len, *a2 = a + 2 * len, *a3 = a + 3 * len;
            int j = 0;
#ifdef __AVX2__
            __m256i vs = _mm256_set1_epi32(int(s)), vt = _mm256_set1_epi32(int(t)), vit = _mm256_set1_epi32(int(it));
            for (; j + 8 <= len; j += 8) {
                __m256i x0 = load(a0 + j), x1 = load(a1 + j), x2 = load(a2 + j), x3 = load(a3 + j);
                if (inverse) {
                    __m256i p0 = vadd(x0, x1), p1 = vmul(vsub(x0, x1), vt), q0 = vadd(x2, x3), q1 = vmul(vsub(x2, x3), vit);
                    store(a0 + j, vadd(p0, q0)); store(a2 + j, vmul(vsub(p0, q0), vs)); store(a1 + j, vadd(p1, q1)); store(a3 + j, vmul(vsub(p1, q1), vs));}
                else {
                    x2 = vmul(x2, vs); x3 = vmul(x3, vs);
                    __m256i p0 = vadd(x0, x2), q0 = vsub(x0, x2), p1 = vmul(vadd(x1, x3), vt), q1 = vmul(vsub(x1, x3), vit);
                    store(a0 + j, vadd(p0, p1)); store(a1 + j, vsub(p0, p1)); store(a2 + j, vadd(q0, q1)); store(a3 + j, vsub(q0, q1));}}
#endif
            for (; j < len; ++j) {
                uint x0 = a0[j], x1 = a1[j], x2 = a2[j], x3 = a3[j];
                if (inverse) {
                    uint p0 = add(x0, x1), p1 = mul(sub(x0, x1), t), q0 = add(x2, x3), q1 = mul(sub(x2, x3), it);
                    a0[j] = add(p0, q0); a2[j] = mul(sub(p0, q0), s); a1[j] = add(p1, q1); a3[j] = mul(sub(p1, q1), s);}
                else {
                    x2 = mul(x2, s); x3 = mul(x3, s);
                    uint p0 = add(x0, x2), q0 = sub(x0, x2), p1 = mul(add(x1, x3), t), q1 = mul(sub(x1, x3), it);
                    a0[j] = add(p0, p1); a1[j] = sub(p0, p1); a2[j] = add(q0, q1); a3[j] = sub(q0, q1);}}}
        // The last three levels of one block of eight points (block root index i): radix-4 over pairs, then the four pairs with rt[4i + q].
        static void tail8(uint *a, int i, bool inverse) {
            const uint *rt = roots(inverse).p;
#ifdef __AVX2__
            __m256i x = load(a), one = _mm256_set1_epi32(int(ONE));
            __m256i vs = _mm256_blend_epi32(one, _mm256_set1_epi32(int(rt[i])), 0xF0);
            __m256i vt = _mm256_blend_epi32(one, _mm256_blend_epi32(_mm256_set1_epi32(int(rt[2 * i])), _mm256_set1_epi32(int(rt[2 * i + 1])), 0xF0), 0xCC);
            __m256i vq = oddRoots(rt + 4 * i);
            if (inverse) {
                __m256i y = _mm256_shuffle_epi32(x, 0xB1);
                x = vmul(_mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xAA), vq);
                y = _mm256_shuffle_epi32(x, 0x4E);
                x = vmul(_mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xCC), vt);
                y = swap128(x);
                store(a, vmul(_mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xF0), vs));
                return;}
            x = vmul(x, vs);
            __m256i y = swap128(x);
            x = vmul(_mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xF0), vt);
            y = _mm256_shuffle_epi32(x, 0x4E);
            x = vmul(_mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xCC), vq);
            y = _mm256_shuffle_epi32(x, 0xB1);
            store(a, _mm256_blend_epi32(vadd(x, y), vsub(y, x), 0xAA));
            return;
#endif
            if (inverse) {
                for (int q = 0; q < 4; ++q) { pass2(a + 2 * q, 1, rt[4 * i + q], true); }
                pass4(a, 2, i, true);
                return;}
            pass4(a, 2, i, false);
            for (int q = 0; q < 4; ++q) { pass2(a + 2 * q, 1, rt[4 * i + q], false); }}
        // Forward DIF to block order (inverse: back), depth-first radix-4 down to LEAF, then the level loop over the leaf; n >= 8, i = the block root index.
        static void transform(uint *a, int n, bool inverse, int i = 1) {
            if (n < 8) {
                for (int size = inverse ? 2 : n; inverse ? size <= n : size >= 2; size = inverse ? 2 * size : size / 2) {
                    for (int b = 0; b < n / size; ++b) { pass2(a + b * size, size / 2, roots(inverse)[i * (n / size) + b], inverse); }}
                return;}
            if (n > LEAF) {
                bool two = (std::countr_zero(uint(n)) - std::countr_zero(uint(LEAF))) % 2 == 1;
                int parts = two ? 2 : 4, len = n / parts;
                if (!inverse) { if (two) { pass2(a, len, roots(false)[i], false); } else { pass4(a, len, i, false); } }
                for (int q = 0; q < parts; ++q) { transform(a + q * len, len, inverse, parts * i + q); }
                if (inverse) { if (two) { pass2(a, len, roots(true)[i], true); } else { pass4(a, len, i, true); } }
                return;}
            int top = n, base = i;
            if (std::countr_zero(uint(n)) % 2 == 0) { top = n / 2; base = 2 * i; }
            if (inverse) {
                for (int b = 0; b < n / 8; ++b) { tail8(a + 8 * b, base * (top / 8) + b, true); }
                for (int size = 32; size <= top; size *= 4) { for (int b = 0; b < n / size; ++b) { pass4(a + b * size, size / 4, base * (top / size) + b, true); } }
                if (top < n) { pass2(a, n / 2, roots(true)[i], true); }
                return;}
            if (top < n) { pass2(a, n / 2, roots(false)[i], false); }
            for (int size = top; size > 8; size /= 4) { for (int b = 0; b < n / size; ++b) { pass4(a + b * size, size / 4, base * (top / size) + b, false); } }
            for (int b = 0; b < n / 8; ++b) { tail8(a + 8 * b, base * (top / 8) + b, false); }}
        static void run(uint *a, int n, bool inverse) {
            ensure(n);
            transform(a, n, inverse);
            if (!inverse) { return; }
            uint inv = from(P - (P - 1) / uint(n));
            int i = 0;
#ifdef __AVX2__
            __m256i vi = _mm256_set1_epi32(int(inv));
            for (; i + 8 <= n; i += 8) { store(a + i, vmul(load(a + i), vi)); }
#endif
            for (; i < n; ++i) { a[i] = mul(a[i], inv); }}
        // Pointwise c = a * b (b may equal a) on transformed data.
        static void dot(uint *a, const uint *b, int n) {
            int i = 0;
#ifdef __AVX2__
            __m256i r2 = _mm256_set1_epi32(int(R2));
            for (; i + 8 <= n; i += 8) { store(a + i, vmul(load(a + i), vmul(load(b + i), r2))); }
#endif
            for (; i < n; ++i) { a[i] = mul(mul(a[i], R2), b[i]); }}
    };

    // T: O(len * log(len)), M: O(len); three-prime CRT: lo + hi * 2^64 holds each limb coefficient of the length-len cyclic
    // limb convolution (len a power of two in [8, 2^26], coefficients below 2^89).
    inline void ntt3Cyclic(W *lo, W *hi, const uint *a, int n, const uint *b, int m, int len) {
        using E0 = Ntt32<NTT_PRIMES[0], NTT_ROOTS[0]>;
        using E1 = Ntt32<NTT_PRIMES[1], NTT_ROOTS[1]>;
        using E2 = Ntt32<NTT_PRIMES[2], NTT_ROOTS[2]>;
        constexpr uint P0 = NTT_PRIMES[0], P1 = NTT_PRIMES[1], P2 = NTT_PRIMES[2];
        constexpr ulng P01 = ulng(P0) * P1;
        constexpr uint I0_1 = E1::from(modPow32(P0, P1 - 2, P1)), I01_2 = E2::from(modPow32(uint(P01 % P2), P2 - 2, P2)), P0_2 = E2::from(P0 % P2);
        static HugeVec<uint> buf;
        bool same = a == b && n == m;
        buf.resize((same ? 3 : 6) * len);
        uint *x[3] = {buf.p, buf.p + len, buf.p + 2 * len}, *y[3] = {buf.p + 3 * len, buf.p + 4 * len, buf.p + 5 * len};
        auto split = [&](uint **v, const uint *src, int cnt) {
            for (int i = 0; i < cnt; ++i) { uint w = src[i]; v[0][i] = w % P0; v[1][i] = w % P1; v[2][i] = w % P2; }
            for (int k = 0; k < 3; ++k) { std::fill(v[k] + cnt, v[k] + len, 0); }};
        split(x, a, n);
        if (!same) { split(y, b, m); }
        E0::run(x[0], len, false); E1::run(x[1], len, false); E2::run(x[2], len, false);
        if (!same) { E0::run(y[0], len, false); E1::run(y[1], len, false); E2::run(y[2], len, false); }
        E0::dot(x[0], (same ? x : y)[0], len); E1::dot(x[1], (same ? x : y)[1], len); E2::dot(x[2], (same ? x : y)[2], len);
        E0::run(x[0], len, true); E1::run(x[1], len, true); E2::run(x[2], len, true);
        for (int i = 0; i < len; ++i) {
            uint v0 = x[0][i], v1 = E1::mul(E1::sub(x[1][i], v0 >= P1 ? v0 - P1 : v0), I0_1);
            ulng t = v0 + ulng(v1) * P0;
            uint t2 = E2::add(v0 % P2, E2::mul(v1, P0_2)), v2 = E2::mul(E2::sub(x[2][i], t2), I01_2);
            ulll e = t + ulll(v2) * P01;
            lo[i] = W(e); hi[i] = W(e >> 64);}}

    // S: O(len) roots, Q: O(len * log(len)), M: O(len); NTT modulo 2^64 - 2^32 + 1 on 16-bit pieces, length <= 2^32: four-lane AVX2 butterflies from 32 x 32 partial products, or scalar.
    struct Ntt64 {
        static constexpr ulng P = 18446744069414584321ULL, G = 7;
        static ulng red(ulll x) {
            ulng l = ulng(x), hl = uint(x >> 64), hh = ulng(x >> 96);
            ulll c = ulll(l) + (ulll(hl) << 32); ulng s = hl + hh;
            c += (c < s) * ulll(P); c -= s;
            ulng r = ulng(c) + 0xFFFFFFFFULL * ulng(c >> 64);
            return r >= P ? r - P : r;}
        static ulng mul(ulng a, ulng b) { return red(ulll(a) * b); }
        static ulng power(ulng a, ulng e) {
            ulng r = 1;
            for (; e; e >>= 1, a = mul(a, a)) { if (e & 1) { r = mul(r, a); } }
            return r;}
        static HugeVec<ulng> &roots() { static HugeVec<ulng> w; return w; }
        static void ensure(int n) {
            HugeVec<ulng> &w = roots();
            if (w.n == 0) { w.resize(2); w[0] = 0; w[1] = 1; }
            if (w.n >= n) { return; }
            int m = w.n; w.resize(n);
            for (int len = m; len < n; len *= 2) {
                ulng w0 = power(G, (P - 1) / ulng(2 * len));
                for (int i = len / 2; i < len; ++i) { w[2 * i] = w[i]; w[2 * i + 1] = mul(w[i], w0); }}}
#ifdef __AVX2__
        static __m256i load(const ulng *p) { return _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p)); }
        static void store(ulng *p, __m256i x) { _mm256_storeu_si256(reinterpret_cast<__m256i *>(p), x); }
        static __m256i vgt(__m256i a, __m256i b) { __m256i s = _mm256_set1_epi64x(lng(1) << 63); return _mm256_cmpgt_epi64(_mm256_xor_si256(a, s), _mm256_xor_si256(b, s)); }
        // Lanes a + b in [0, P) for a < 2^64 and b < P, and a - b in [0, P) for a, b < P: a wrap of 2^64 is 2^32 - 1 mod P, so a wrapped sum gains it, a borrowed difference loses it, then one subtraction of P.
        static __m256i vadd(__m256i a, __m256i b) {
            __m256i s = _mm256_add_epi64(a, b);
            s = _mm256_add_epi64(s, _mm256_and_si256(vgt(a, s), _mm256_set1_epi64x(lng(uint(-1)))));
            return _mm256_sub_epi64(s, _mm256_and_si256(vgt(s, _mm256_set1_epi64x(lng(P - 1))), _mm256_set1_epi64x(lng(P))));}
        static __m256i vsub(__m256i a, __m256i b) { __m256i d = _mm256_sub_epi64(a, b); return _mm256_sub_epi64(d, _mm256_and_si256(vgt(b, a), _mm256_set1_epi64x(lng(uint(-1))))); }
        // a b mod P from the four 32 x 32 partial products: the 128-bit value l + hl 2^64 + hh 2^96 is l - hh + hl (2^32 - 1) mod P.
        static __m256i vmul(__m256i a, __m256i b) {
            __m256i m = _mm256_set1_epi64x(lng(uint(-1))), ah = _mm256_srli_epi64(a, 32), bh = _mm256_srli_epi64(b, 32);
            __m256i p00 = _mm256_mul_epu32(a, b), p01 = _mm256_mul_epu32(a, bh), p10 = _mm256_mul_epu32(ah, b), p11 = _mm256_mul_epu32(ah, bh);
            __m256i t = _mm256_add_epi64(_mm256_add_epi64(_mm256_srli_epi64(p00, 32), _mm256_and_si256(p01, m)), _mm256_and_si256(p10, m));
            __m256i l = _mm256_or_si256(_mm256_and_si256(p00, m), _mm256_slli_epi64(t, 32));
            __m256i h = _mm256_add_epi64(_mm256_add_epi64(p11, _mm256_srli_epi64(t, 32)), _mm256_add_epi64(_mm256_srli_epi64(p01, 32), _mm256_srli_epi64(p10, 32)));
            __m256i hl = _mm256_and_si256(h, m);
            return vadd(vsub(l, _mm256_srli_epi64(h, 32)), _mm256_sub_epi64(_mm256_slli_epi64(hl, 32), hl));}
        struct Fly { __m256i s, d; };
        static Fly fly(__m256i u, __m256i v, __m256i w, bool dit) {
            if (dit) { v = vmul(v, w); return {vadd(u, v), vsub(u, v)}; }
            return {vadd(u, v), vmul(vsub(u, v), w)};}
#endif
        // One radix-2 level of butterflies at distance len over n points with the roots w[0, len): DIF (the difference times the root) forward, DIT (the root first) in the inverse.
        static void level(ulng *a, int n, int len, const ulng *w, bool dit) {
#ifdef __AVX2__
            if (len >= 4) {
                for (int i = 0; i < n; i += 2 * len) {
                    for (int j = 0; j < len; j += 4) {
                        auto [s, d] = fly(load(a + i + j), load(a + i + j + len), load(w + j), dit);
                        store(a + i + j, s); store(a + i + j + len, d);}}
                return;}
            if (n >= 4) {
                bool two = len == 2;
                __m256i rw = two ? _mm256_setr_epi64x(lng(w[0]), lng(w[1]), lng(w[0]), lng(w[1])) : _mm256_set1_epi64x(lng(w[0]));
                for (int i = 0; i < n; i += 4) {
                    __m256i x = load(a + i), u, v;
                    if (two) { u = _mm256_permute4x64_epi64(x, 0x44); v = _mm256_permute4x64_epi64(x, 0xEE); }
                    else { u = _mm256_permute4x64_epi64(x, 0xA0); v = _mm256_permute4x64_epi64(x, 0xF5); }
                    auto [s, d] = fly(u, v, rw, dit);
                    store(a + i, two ? _mm256_blend_epi32(s, d, 0xF0) : _mm256_blend_epi32(s, d, 0xCC));}
                return;}
#endif
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; ++j) {
                    ulng u = a[i + j], v = dit ? mul(a[i + j + len], w[j]) : a[i + j + len], s = u + v, d = u - v + (u < v) * P;
                    a[i + j] = s - ((s < u) | (s >= P)) * P; a[i + j + len] = dit ? d : mul(d, w[j]);}}}
        // In place: the forward DIF to bit-reversed order; the inverse is the DIT with the same roots, then the index reversal i -> n - i and the 1 / n scale.
        static void transform(ulng *a, int n, bool inverse) {
            ensure(n);
            const ulng *w = roots().p;
            if (!inverse) { for (int len = n / 2; len > 0; len /= 2) { level(a, n, len, w + len, false); } return; }
            for (int len = 1; len < n; len *= 2) { level(a, n, len, w + len, true); }
            ulng inv = P - (P - 1) / ulng(n);
            a[0] = mul(a[0], inv);
            for (int i = 1, j = n - 1; i <= j; ++i, --j) { ulng x = mul(a[i], inv); a[i] = mul(a[j], inv); a[j] = x; }}
        // x[i] = x[i] y[i] mod P (y may be x).
        static void dot(ulng *x, const ulng *y, int n) {
            int i = 0;
#ifdef __AVX2__
            for (; i + 4 <= n; i += 4) { store(x + i, vmul(load(x + i), load(y + i))); }
#endif
            for (; i < n; ++i) { x[i] = mul(x[i], y[i]); }}
    };
    // T: O(len * log(len)), M: O(len); the Goldilocks NTT on 16-bit pieces, the two pieces of each limb coefficient recombined as lo + hi 2^64.
    inline void ntt64Cyclic(W *lo, W *hi, const uint *a, int n, const uint *b, int m, int len) {
        static HugeVec<ulng> buf;
        bool same = a == b && n == m;
        int pieces = 2 * len;
        buf.resize(same ? pieces : 2 * pieces);
        ulng *x = buf.p, *y = buf.p + pieces;
        for (int i = 0; i < n; ++i) { x[2 * i] = a[i] & 0xFFFF; x[2 * i + 1] = a[i] >> 16; }
        std::fill(x + 2 * n, x + pieces, 0);
        Ntt64::transform(x, pieces, false);
        if (same) { Ntt64::dot(x, x, pieces); }
        else {
            for (int i = 0; i < m; ++i) { y[2 * i] = b[i] & 0xFFFF; y[2 * i + 1] = b[i] >> 16; }
            std::fill(y + 2 * m, y + pieces, 0);
            Ntt64::transform(y, pieces, false);
            Ntt64::dot(x, y, pieces);}
        Ntt64::transform(x, pieces, true);
        for (int i = 0; i < len; ++i) {
            ulll e = x[2 * i] + (ulll(x[2 * i + 1]) << 16);
            lo[i] = W(e); hi[i] = W(e >> 64);}}

    // Rounding model of the double FFT: u = 2^-53; a complex product errs by at most RHO |ab| under any FMA contraction, a stored root by MU,
    // a radix-3 or radix-5 column stage by EPS[r] of its output norm.
    constexpr double FFT_U = 0x1p-53, FFT_RHO = (1 + 1.4142135623730951) * FFT_U * (1 + FFT_U), FFT_MU = 1.02 * FFT_U, FFT_TAU = FFT_MU + FFT_RHO + FFT_MU * FFT_RHO;
    constexpr double FFT_EPS[6] = {0, 0, 0, 7 * FFT_U, 0, 16 * FFT_U};
    constexpr int FFT_MIN_LOG = 3, FFT_MAX_LOG = 24, FFT_MIN_K = 8, FFT_MAX_K = 20, FFT_LEAF = 1 << 11;
    // T: O(lg), M: O(1); worst-case error of the packed real convolution through r 2^lg complex points relative to |x| |y|: F^2 (1 + rho) (1 + 2 Sigma) - 1 with the
    // forward factor F = (1+u)^(lg+2) (1+tau)^t (1+eps_r), the inverse factor Sigma = (1+u)^(lg+3) (1+tau)^t (1+eps_r) - 1, t = lg - 1 (r = 1) or lg + 1, and two more u for the inexact 1 / r scale.
    constexpr double fftPhi(int lg, int r) {
        auto mulIn = [](double &acc, double t) { acc += t + acc * t; };
        int t = r == 1 ? lg - 1 : lg + 1;
        double f = 0;
        for (int i = 0; i < lg + 2; ++i) { mulIn(f, FFT_U); }
        for (int i = 0; i < t; ++i) { mulIn(f, FFT_TAU); }
        mulIn(f, FFT_EPS[r]);
        double sigma = f, acc = f;
        mulIn(sigma, FFT_U); mulIn(acc, f); mulIn(acc, FFT_RHO); mulIn(acc, 2 * sigma);
        if (r > 1) { mulIn(acc, FFT_U); mulIn(acc, FFT_U); }
        return acc;}
    constexpr std::array<std::array<double, FFT_MAX_LOG + 1>, 6> FFT_PHI = []() {
        std::array<std::array<double, FFT_MAX_LOG + 1>, 6> t{};
        for (int r : {1, 3, 5}) { for (int i = FFT_MIN_LOG; i <= FFT_MAX_LOG; ++i) { t[size_t(r)][size_t(i)] = fftPhi(i, r); } }
        return t;}();

    // S: O(n) roots, Q: O(n * log(n)), M: O(n); complex FFT of r 2^lg points on split re/im arrays: a radix-r column stage (r = 3, 5), then each of the r chunks with its
    // block roots tw[B + b] = exp(i pi (q + r bitrev_B(b)) / (r B)) forward from natural order to the roots' block order, depth-first radix-4 above the leaf; inverse back, scaled by r 2^lg.
    struct Fft {
        // Table t: 0 the roots of chunk 0 (every r), 1 and 2 the chunks q = 1, 2 of radix 3, 3 to 6 the chunks q = 1 to 4 of radix 5.
        static HugeVec<double> &table(bool imag, int t = 0) { static HugeVec<double> w[2][7]; return w[imag][t]; }
        static int tableId(int r, int q) { return q == 0 ? 0 : r == 3 ? q : 2 + q; }
        // Entries [1, 2n) of the r tables of chunks of n points: the levels B <= n, the last one being the 2n-th roots dot needs; chunk r - q mirrors chunk q.
        static void ensure(int n, int r) {
            for (int q = 0; 2 * q < r; ++q) {
                int t = tableId(r, q), tm = q ? tableId(r, r - q) : 0;
                HugeVec<double> &wr = table(false, t), &wi = table(true, t);
                int old = wr.n;
                if (old >= 2 * n) { continue; }
                wr.resize(2 * n); wi.resize(2 * n);
                if (q) { table(false, tm).resize(2 * n); table(true, tm).resize(2 * n); }
                for (int lvl = max(old, 1); lvl < 2 * n; lvl *= 2) { level(wr.p + lvl, wi.p + lvl, lvl, r, q, q ? table(false, tm).p + lvl : nullptr, q ? table(true, tm).p + lvl : nullptr); }}}
        // The 2^12-th roots exp(i pi bitrev(j) / 2^g) in block order as double-doubles (a prefix serves every g <= 12), grown on demand from long double trig.
        static double *coarse(int g) {
            static std::array<double, 4 << 12> c; static int have = 0;
            if (have >= 1 << g) { return c.data(); }
            have = 1 << g;
            for (int j = 0, k = 0; j < have; ++j) {
                long double a = std::numbers::pi_v<long double> * k / have, x = std::cos(a), y = std::sin(a);
                c[j] = double(x); c[4096 + j] = double(x - c[j]); c[8192 + j] = double(y); c[12288 + j] = double(y - c[8192 + j]);
                int bit = have >> 1; while (k & bit) { k ^= bit; bit >>= 1; } k ^= bit;}
            return c.data();}
        // Level lvl of chunk q of radix r, entry b: exp(i pi (q + r bitrev(b)) / (r lvl)) as a coarse root times a fine one (the 2^h angles below pi / 2^g shifted by the chunk's
        // offset; long double trig split into double-doubles), multiplied in double-double and rounded once; exact at the quarter turns; (mr, mi) mirror chunk r - q as -conj in reverse.
        static void level(double *wr, double *wi, int lvl, int r, int q, double *mr, double *mi) {
            int bits = std::countr_zero(uint(lvl)), h = bits / 2, g = bits - h;
            const double *cr = coarse(g), *cl = cr + 4096, *ci = cr + 8192, *cil = cr + 12288;
            Scratch s(size_t(4) << h);
            double *fr = reinterpret_cast<double *>(s.p), *fl = fr + (1 << h), *fi = fl + (1 << h), *fil = fi + (1 << h);
            for (int j = 0; j < 1 << h; ++j) {
                long double a = std::numbers::pi_v<long double> * ((q + r * j) / static_cast<long double>(r * lvl)), x = std::cos(a), y = std::sin(a);
                fr[j] = double(x); fl[j] = double(x - fr[j]); fi[j] = double(y); fil[j] = double(y - fi[j]);}
            for (int blk = 0, kl = 0; blk < 1 << h; ++blk) {
                int at = blk << g, j = 0;
                double *outr = wr + at, *outi = wi + at, *mirr = mr ? mr + lvl - 1 - at : nullptr, *miri = mi ? mi + lvl - 1 - at : nullptr;
#if defined(__AVX2__) && defined(__FMA__)
                V ar = _mm256_set1_pd(fr[kl]), al = _mm256_set1_pd(fl[kl]), ai = _mm256_set1_pd(fi[kl]), ail = _mm256_set1_pd(fil[kl]), zero = _mm256_setzero_pd();
                auto dd = [](V xh, V xl, V yh, V yl, V &ph, V &pl) { ph = _mm256_mul_pd(xh, yh); pl = _mm256_fmsub_pd(xh, yh, ph); pl = _mm256_fmadd_pd(xh, yl, pl); pl = _mm256_fmadd_pd(xl, yh, pl); };
                auto sum = [](V x, V y, V xl, V yl) {
                    V t = _mm256_add_pd(x, y), bb = _mm256_sub_pd(t, x), e = _mm256_add_pd(_mm256_sub_pd(x, _mm256_sub_pd(t, bb)), _mm256_sub_pd(y, bb));
                    return _mm256_add_pd(t, _mm256_add_pd(e, _mm256_add_pd(xl, yl)));};
                for (; j + 4 <= 1 << g; j += 4) {
                    V br = ld(cr + j), bl = ld(cl + j), bi = ld(ci + j), bil = ld(cil + j), p1, e1, p2, e2;
                    dd(ar, al, br, bl, p1, e1); dd(ai, ail, bi, bil, p2, e2);
                    V re = sum(p1, _mm256_sub_pd(zero, p2), e1, _mm256_sub_pd(zero, e2));
                    dd(ar, al, bi, bil, p1, e1); dd(ai, ail, br, bl, p2, e2);
                    V im = sum(p1, p2, e1, e2);
                    st(outr + j, re); st(outi + j, im);
                    if (mr) { st(mirr - j - 3, rev(_mm256_sub_pd(zero, re))); st(miri - j - 3, rev(im)); }}
#endif
                long double xr = fr[kl] + static_cast<long double>(fl[kl]), xi = fi[kl] + static_cast<long double>(fil[kl]);
                for (; j < 1 << g; ++j) {
                    long double yr = cr[j] + static_cast<long double>(cl[j]), yi = ci[j] + static_cast<long double>(cil[j]);
                    outr[j] = double(xr * yr - xi * yi); outi[j] = double(xr * yi + xi * yr);
                    if (mr) { mirr[-j] = -outr[j]; miri[-j] = outi[j]; }}
                int bit = (1 << h) >> 1; while (kl & bit) { kl ^= bit; bit >>= 1; } kl ^= bit;}
            if (q == 0) { wr[0] = 1; wi[0] = 0; if (lvl > 1) { wr[1] = 0; wi[1] = 1; } }}
#if defined(__AVX2__) && defined(__FMA__)
        using V = __m256d;
        static V ld(const double *p) { return _mm256_loadu_pd(p); }
        static void st(double *p, V x) { _mm256_storeu_pd(p, x); }
        static V rev(V x) { return _mm256_permute4x64_pd(x, 0x1B); }
        static V swap128(V x) { return _mm256_permute2f128_pd(x, x, 0x01); }
        static void cmul(V ar, V ai, V br, V bi, V &rr, V &ri) { rr = _mm256_fmsub_pd(ar, br, _mm256_mul_pd(ai, bi)); ri = _mm256_fmadd_pd(ar, bi, _mm256_mul_pd(ai, br)); }
#endif
        // One radix-2 level of len pairs with the block root (wr, wi): forward (u, v) -> (u + w v, u - w v), inverse (p, q) -> (p + q, (p - q) conj(w)).
        static void pass2(double *re, double *im, int len, double wr, double wi, bool inverse) {
            int j = 0;
            if (inverse) { wi = -wi; }
#if defined(__AVX2__) && defined(__FMA__)
            V bwr = _mm256_set1_pd(wr), bwi = _mm256_set1_pd(wi);
            for (; j + 4 <= len; j += 4) {
                V ur = ld(re + j), ui = ld(im + j), xr = ld(re + j + len), xi = ld(im + j + len), pr, pi;
                if (inverse) {
                    cmul(_mm256_sub_pd(ur, xr), _mm256_sub_pd(ui, xi), bwr, bwi, pr, pi);
                    st(re + j, _mm256_add_pd(ur, xr)); st(im + j, _mm256_add_pd(ui, xi)); st(re + j + len, pr); st(im + j + len, pi);}
                else {
                    cmul(xr, xi, bwr, bwi, pr, pi);
                    st(re + j, _mm256_add_pd(ur, pr)); st(im + j, _mm256_add_pd(ui, pi)); st(re + j + len, _mm256_sub_pd(ur, pr)); st(im + j + len, _mm256_sub_pd(ui, pi));}}
#endif
            for (; j < len; ++j) {
                double ur = re[j], ui = im[j], xr = re[j + len], xi = im[j + len];
                if (inverse) {
                    double dr = ur - xr, di = ui - xi;
                    re[j] = ur + xr; im[j] = ui + xi; re[j + len] = dr * wr - di * wi; im[j + len] = dr * wi + di * wr;}
                else {
                    double vr = xr * wr - xi * wi, vi = xr * wi + xi * wr;
                    re[j] = ur + vr; im[j] = ui + vi; re[j + len] = ur - vr; im[j + len] = ui - vi;}}}
        // One radix-4 level over 4 len points with the block roots s = tw[i], t = tw[2i] (and i t); the inverse uses their conjugates in reverse.
        static void pass4(double *re, double *im, int len, int i, bool inverse, const double *twr, const double *twi) {
            double sr = twr[i], si = inverse ? -twi[i] : twi[i], tr = twr[2 * i], ti = inverse ? -twi[2 * i] : twi[2 * i];
            double *r0 = re, *r1 = re + len, *r2 = re + 2 * len, *r3 = re + 3 * len, *i0 = im, *i1 = im + len, *i2 = im + 2 * len, *i3 = im + 3 * len;
            int j = 0;
#if defined(__AVX2__) && defined(__FMA__)
            V vsr = _mm256_set1_pd(sr), vsi = _mm256_set1_pd(si), vtr = _mm256_set1_pd(tr), vti = _mm256_set1_pd(ti);
            for (; j + 4 <= len; j += 4) {
                V a0r = ld(r0 + j), a0i = ld(i0 + j), a1r = ld(r1 + j), a1i = ld(i1 + j), a2r = ld(r2 + j), a2i = ld(i2 + j), a3r = ld(r3 + j), a3i = ld(i3 + j);
                if (inverse) {
                    V p0r = _mm256_add_pd(a0r, a1r), p0i = _mm256_add_pd(a0i, a1i), q0r = _mm256_add_pd(a2r, a3r), q0i = _mm256_add_pd(a2i, a3i), p1r, p1i, q1r, q1i;
                    cmul(_mm256_sub_pd(a0r, a1r), _mm256_sub_pd(a0i, a1i), vtr, vti, p1r, p1i);
                    cmul(_mm256_sub_pd(a2i, a3i), _mm256_sub_pd(a3r, a2r), vtr, vti, q1r, q1i);
                    V d2r, d2i, d3r, d3i;
                    cmul(_mm256_sub_pd(p0r, q0r), _mm256_sub_pd(p0i, q0i), vsr, vsi, d2r, d2i);
                    cmul(_mm256_sub_pd(p1r, q1r), _mm256_sub_pd(p1i, q1i), vsr, vsi, d3r, d3i);
                    st(r0 + j, _mm256_add_pd(p0r, q0r)); st(i0 + j, _mm256_add_pd(p0i, q0i)); st(r1 + j, _mm256_add_pd(p1r, q1r)); st(i1 + j, _mm256_add_pd(p1i, q1i));
                    st(r2 + j, d2r); st(i2 + j, d2i); st(r3 + j, d3r); st(i3 + j, d3i);}
                else {
                    V s2r, s2i, s3r, s3i;
                    cmul(a2r, a2i, vsr, vsi, s2r, s2i); cmul(a3r, a3i, vsr, vsi, s3r, s3i);
                    V p0r = _mm256_add_pd(a0r, s2r), p0i = _mm256_add_pd(a0i, s2i), q0r = _mm256_sub_pd(a0r, s2r), q0i = _mm256_sub_pd(a0i, s2i);
                    V p1r, p1i, q1r, q1i;
                    cmul(_mm256_add_pd(a1r, s3r), _mm256_add_pd(a1i, s3i), vtr, vti, p1r, p1i);
                    cmul(_mm256_sub_pd(a1r, s3r), _mm256_sub_pd(a1i, s3i), vtr, vti, q1r, q1i);
                    st(r0 + j, _mm256_add_pd(p0r, p1r)); st(i0 + j, _mm256_add_pd(p0i, p1i)); st(r1 + j, _mm256_sub_pd(p0r, p1r)); st(i1 + j, _mm256_sub_pd(p0i, p1i));
                    st(r2 + j, _mm256_sub_pd(q0r, q1i)); st(i2 + j, _mm256_add_pd(q0i, q1r)); st(r3 + j, _mm256_add_pd(q0r, q1i)); st(i3 + j, _mm256_sub_pd(q0i, q1r));}}
#endif
            for (; j < len; ++j) {
                double a0r = r0[j], a0i = i0[j], a1r = r1[j], a1i = i1[j], a2r = r2[j], a2i = i2[j], a3r = r3[j], a3i = i3[j];
                if (inverse) {
                    double p0r = a0r + a1r, p0i = a0i + a1i, dr = a0r - a1r, di = a0i - a1i, p1r = dr * tr - di * ti, p1i = dr * ti + di * tr;
                    double q0r = a2r + a3r, q0i = a2i + a3i, er = a2i - a3i, ei = a3r - a2r, q1r = er * tr - ei * ti, q1i = er * ti + ei * tr;
                    double d2r = p0r - q0r, d2i = p0i - q0i, d3r = p1r - q1r, d3i = p1i - q1i;
                    r0[j] = p0r + q0r; i0[j] = p0i + q0i; r1[j] = p1r + q1r; i1[j] = p1i + q1i;
                    r2[j] = d2r * sr - d2i * si; i2[j] = d2r * si + d2i * sr; r3[j] = d3r * sr - d3i * si; i3[j] = d3r * si + d3i * sr;}
                else {
                    double s2r = a2r * sr - a2i * si, s2i = a2r * si + a2i * sr, s3r = a3r * sr - a3i * si, s3i = a3r * si + a3i * sr;
                    double p0r = a0r + s2r, p0i = a0i + s2i, q0r = a0r - s2r, q0i = a0i - s2i, ur = a1r + s3r, ui = a1i + s3i, vr = a1r - s3r, vi = a1i - s3i;
                    double p1r = ur * tr - ui * ti, p1i = ur * ti + ui * tr, q1r = vr * tr - vi * ti, q1i = vr * ti + vi * tr;
                    r0[j] = p0r + p1r; i0[j] = p0i + p1i; r1[j] = p0r - p1r; i1[j] = p0i - p1i;
                    r2[j] = q0r - q1i; i2[j] = q0i + q1r; r3[j] = q0r + q1i; i3[j] = q0i - q1r;}}}
        // The last three levels of one block of eight points (block root index i): radix-4 over pairs, then the four pairs with tw[4i + q].
        static void tail8(double *re, double *im, int i, bool inverse, const double *twr, const double *twi) {
#if defined(__AVX2__) && defined(__FMA__)
            double sg = inverse ? -1.0 : 1.0;
            V sr = _mm256_set1_pd(twr[i]), si = _mm256_set1_pd(sg * twi[i]), tr = _mm256_setr_pd(1, 1, twr[2 * i], twr[2 * i]), ti = _mm256_setr_pd(0, 0, sg * twi[2 * i], sg * twi[2 * i]);
            V qr = _mm256_permute4x64_pd(ld(twr + 4 * i), 0xD8), qi = _mm256_permute4x64_pd(_mm256_mul_pd(_mm256_set1_pd(sg), ld(twi + 4 * i)), 0xD8);
            V hi = _mm256_setr_pd(1, 1, -1, -1), x0r = ld(re), x0i = ld(im), x1r = ld(re + 4), x1i = ld(im + 4);
            if (inverse) {
                V er = _mm256_unpacklo_pd(x0r, x1r), ei = _mm256_unpacklo_pd(x0i, x1i), fr = _mm256_unpackhi_pd(x0r, x1r), fi = _mm256_unpackhi_pd(x0i, x1i), dr, di;
                cmul(_mm256_sub_pd(er, fr), _mm256_sub_pd(ei, fi), qr, qi, dr, di);
                V gr = _mm256_add_pd(er, fr), gi = _mm256_add_pd(ei, fi);
                x0r = _mm256_unpacklo_pd(gr, dr); x0i = _mm256_unpacklo_pd(gi, di); x1r = _mm256_unpackhi_pd(gr, dr); x1i = _mm256_unpackhi_pd(gi, di);
                V y0r = swap128(x0r), y0i = swap128(x0i), y1r = swap128(x1r), y1i = swap128(x1i), pr, pi, qqr, qqi, ar, ai;
                cmul(_mm256_fmadd_pd(x0r, hi, y0r), _mm256_fmadd_pd(x0i, hi, y0i), tr, ti, pr, pi);
                V cr = _mm256_blend_pd(_mm256_add_pd(x1r, y1r), _mm256_sub_pd(y1i, x1i), 0xC), ci = _mm256_blend_pd(_mm256_add_pd(x1i, y1i), _mm256_sub_pd(x1r, y1r), 0xC);
                cmul(cr, ci, tr, ti, qqr, qqi);
                cmul(_mm256_sub_pd(pr, qqr), _mm256_sub_pd(pi, qqi), sr, si, ar, ai);
                st(re, _mm256_add_pd(pr, qqr)); st(im, _mm256_add_pd(pi, qqi)); st(re + 4, ar); st(im + 4, ai);
                return;}
            V s1r, s1i, tpr, tpi, tqr, tqi;
            cmul(x1r, x1i, sr, si, s1r, s1i);
            cmul(_mm256_add_pd(x0r, s1r), _mm256_add_pd(x0i, s1i), tr, ti, tpr, tpi); cmul(_mm256_sub_pd(x0r, s1r), _mm256_sub_pd(x0i, s1i), tr, ti, tqr, tqi);
            V ypr = swap128(tpr), ypi = swap128(tpi), yqr = swap128(tqr), yqi = swap128(tqi), nhi = _mm256_setr_pd(-1, -1, 1, 1);
            x0r = _mm256_fmadd_pd(tpr, hi, ypr); x0i = _mm256_fmadd_pd(tpi, hi, ypi);
            x1r = _mm256_fmadd_pd(_mm256_blend_pd(yqi, tqi, 0xC), nhi, _mm256_blend_pd(tqr, yqr, 0xC)); x1i = _mm256_fmadd_pd(_mm256_blend_pd(yqr, tqr, 0xC), hi, _mm256_blend_pd(tqi, yqi, 0xC));
            V er = _mm256_unpacklo_pd(x0r, x1r), ei = _mm256_unpacklo_pd(x0i, x1i), fr = _mm256_unpackhi_pd(x0r, x1r), fi = _mm256_unpackhi_pd(x0i, x1i), wr, wi;
            cmul(fr, fi, qr, qi, wr, wi);
            V ar = _mm256_add_pd(er, wr), ai = _mm256_add_pd(ei, wi), br = _mm256_sub_pd(er, wr), bi = _mm256_sub_pd(ei, wi);
            st(re, _mm256_unpacklo_pd(ar, br)); st(im, _mm256_unpacklo_pd(ai, bi)); st(re + 4, _mm256_unpackhi_pd(ar, br)); st(im + 4, _mm256_unpackhi_pd(ai, bi));
            return;
#endif
            if (inverse) {
                for (int q = 0; q < 4; ++q) { pass2(re + 2 * q, im + 2 * q, 1, twr[4 * i + q], twi[4 * i + q], true); }
                pass4(re, im, 2, i, true, twr, twi);
                return;}
            pass4(re, im, 2, i, false, twr, twi);
            for (int q = 0; q < 4; ++q) { pass2(re + 2 * q, im + 2 * q, 1, twr[4 * i + q], twi[4 * i + q], false); }}
        // Radix-3 and radix-5 DFTs of one column (x_q <- sum_p w^(p q) x_p, w = exp(2 i pi / r)) on scalars or vectors; s and sg carry the sign of the sines (-1 inverts).
        template<typename T> static void col3(T &x0r, T &x0i, T &x1r, T &x1i, T &x2r, T &x2i, double s) {
            T ar = x1r + x2r, ai = x1i + x2i, dr = (x1r - x2r) * s, di = (x1i - x2i) * s, mr = x0r - 0.5 * ar, mi = x0i - 0.5 * ai;
            x0r = x0r + ar; x0i = x0i + ai; x1r = mr - di; x1i = mi + dr; x2r = mr + di; x2i = mi - dr;}
        template<typename T> static void col5(T &x0r, T &x0i, T &x1r, T &x1i, T &x2r, T &x2i, T &x3r, T &x3i, T &x4r, T &x4i, double sg) {
            const double c1 = 0.30901699437494742410, c2 = -0.80901699437494742410, s1 = sg * 0.95105651629515357212, s2 = sg * 0.58778525229247312917;
            T a1r = x1r + x4r, a1i = x1i + x4i, a2r = x2r + x3r, a2i = x2i + x3i, d1r = x1r - x4r, d1i = x1i - x4i, d2r = x2r - x3r, d2i = x2i - x3i;
            T m1r = x0r + c1 * a1r + c2 * a2r, m1i = x0i + c1 * a1i + c2 * a2i, m2r = x0r + c2 * a1r + c1 * a2r, m2i = x0i + c2 * a1i + c1 * a2i;
            T t1r = s1 * d1r + s2 * d2r, t1i = s1 * d1i + s2 * d2i, t2r = s2 * d1r - s1 * d2r, t2i = s2 * d1i - s1 * d2i;
            x0r = x0r + a1r + a2r; x0i = x0i + a1i + a2i;
            x1r = m1r - t1i; x1i = m1i + t1r; x4r = m1r + t1i; x4i = m1i - t1r; x2r = m2r - t2i; x2i = m2i + t2r; x3r = m2r + t2i; x3i = m2i - t2r;}
        // The radix-r column stage over the r chunks of len points (r = 3 or 5), conjugate roots when inverse.
        static void column(double *re, double *im, int len, int r, bool inverse) {
            double sg = inverse ? -1.0 : 1.0, s3 = sg * 0.86602540378443864676;
            int j = 0;
#if defined(__AVX2__) && defined(__FMA__)
            for (; j + 4 <= len; j += 4) {
                V x[10];
                for (int q = 0; q < r; ++q) { x[2 * q] = ld(re + q * len + j); x[2 * q + 1] = ld(im + q * len + j); }
                if (r == 3) { col3(x[0], x[1], x[2], x[3], x[4], x[5], s3); } else { col5(x[0], x[1], x[2], x[3], x[4], x[5], x[6], x[7], x[8], x[9], sg); }
                for (int q = 0; q < r; ++q) { st(re + q * len + j, x[2 * q]); st(im + q * len + j, x[2 * q + 1]); }}
#endif
            for (; j < len; ++j) {
                double *r1 = re + len + j, *i1 = im + len + j, *r2 = re + 2 * len + j, *i2 = im + 2 * len + j;
                if (r == 3) { col3(re[j], im[j], *r1, *i1, *r2, *i2, s3); }
                else { col5(re[j], im[j], *r1, *i1, *r2, *i2, re[3 * len + j], im[3 * len + j], re[4 * len + j], im[4 * len + j], sg); }}}
        // Forward to block order (inverse: back) with the roots (twr, twi): depth-first radix-4 down to a leaf of 2^11 or 2^10 points by the parity of the levels
        // above, then the level loop over the leaf; n >= 8, i = the block root index.
        static void transform(double *re, double *im, int n, bool inverse, const double *twr, const double *twi, int i = 1) {
            int lz = std::countr_zero(uint(n)), leaf = (lz - std::countr_zero(uint(FFT_LEAF))) & 1 ? FFT_LEAF / 2 : FFT_LEAF;
            if (n > leaf) {
                int len = n / 4;
                if (!inverse) { pass4(re, im, len, i, false, twr, twi); }
                for (int q = 0; q < 4; ++q) { transform(re + q * len, im + q * len, len, inverse, twr, twi, 4 * i + q); }
                if (inverse) { pass4(re, im, len, i, true, twr, twi); }
                return;}
            int top = n, base = i;
            if (lz % 2 == 0) { top = n / 2; base = 2 * i; }
            if (inverse) {
                for (int b = 0; b < n / 8; ++b) { tail8(re + 8 * b, im + 8 * b, base * (top / 8) + b, true, twr, twi); }
                for (int size = 32; size <= top; size *= 4) { for (int b = 0; b < n / size; ++b) { pass4(re + b * size, im + b * size, size / 4, base * (top / size) + b, true, twr, twi); } }
                if (top < n) { pass2(re, im, n / 2, twr[i], twi[i], true); }
                return;}
            if (top < n) { pass2(re, im, n / 2, twr[i], twi[i], false); }
            for (int size = top; size > 8; size /= 4) { for (int b = 0; b < n / size; ++b) { pass4(re + b * size, im + b * size, size / 4, base * (top / size) + b, false, twr, twi); } }
            for (int b = 0; b < n / 8; ++b) { tail8(re + 8 * b, im + 8 * b, base * (top / 8) + b, false, twr, twi); }}
        // The convolution step of the conjugate positions p and q (frequencies k and M - k) with the 2M-th root w of k: Z'_p and Z'_q of the product of the packed real sequences.
        static void one(double *xr, double *xi, const double *yr, const double *yi, int p, int q, double wr, double wi) {
            double er = xr[p] + xr[q], ei = xi[p] - xi[q], o_r = xi[p] + xi[q], o_i = xr[q] - xr[p];
            double ror = wr * o_r - wi * o_i, roi = wr * o_i + wi * o_r;
            double xpr = er + ror, xpi = ei + roi, xmr = er - ror, xmi = ei - roi, ypr = xpr, ypi = xpi, ymr = xmr, ymi = xmi;
            if (yr != xr) {
                double fr = yr[p] + yr[q], fi = yi[p] - yi[q], g_r = yi[p] + yi[q], g_i = yr[q] - yr[p];
                double rgr = wr * g_r - wi * g_i, rgi = wr * g_i + wi * g_r;
                ypr = fr + rgr; ypi = fi + rgi; ymr = fr - rgr; ymi = fi - rgi;}
            double cpr = xpr * ypr - xpi * ypi, cpi = xpr * ypi + xpi * ypr, cmr = xmr * ymr - xmi * ymi, cmi = xmr * ymi + xmi * ymr;
            double epr = cpr + cmr, epi = cpi + cmi, ddr = cpr - cmr, ddi = cpi - cmi;
            double opr = ddr * wr + ddi * wi, opi = ddi * wr - ddr * wi;
            xr[p] = epr - opi; xi[p] = epi + opr; xr[q] = epr + opi; xi[q] = opr - epi;}
        // The len pairs (p + j, q - j) with the roots (wr[j], wi[j]).
        static void pairs(double *xr, double *xi, const double *yr, const double *yi, int p, int q, int len, const double *wr, const double *wi) {
            int j = 0;
#if defined(__AVX2__) && defined(__FMA__)
            V neg = _mm256_set1_pd(-1.0);
            for (; j + 4 <= len; j += 4, p += 4, q -= 4) {
                V xpr0 = ld(xr + p), xpi0 = ld(xi + p), xqr0 = rev(ld(xr + q - 3)), xqi0 = rev(ld(xi + q - 3)), vwr = ld(wr + j), vwi = ld(wi + j);
                V er = _mm256_add_pd(xpr0, xqr0), ei = _mm256_sub_pd(xpi0, xqi0), o_r = _mm256_add_pd(xpi0, xqi0), o_i = _mm256_sub_pd(xqr0, xpr0), ror, roi;
                cmul(o_r, o_i, vwr, vwi, ror, roi);
                V xpr = _mm256_add_pd(er, ror), xpi = _mm256_add_pd(ei, roi), xmr = _mm256_sub_pd(er, ror), xmi = _mm256_sub_pd(ei, roi), ypr = xpr, ypi = xpi, ymr = xmr, ymi = xmi;
                if (yr != xr) {
                    V ypr0 = ld(yr + p), ypi0 = ld(yi + p), yqr0 = rev(ld(yr + q - 3)), yqi0 = rev(ld(yi + q - 3));
                    V fr = _mm256_add_pd(ypr0, yqr0), fi = _mm256_sub_pd(ypi0, yqi0), g_r = _mm256_add_pd(ypi0, yqi0), g_i = _mm256_sub_pd(yqr0, ypr0), rgr, rgi;
                    cmul(g_r, g_i, vwr, vwi, rgr, rgi);
                    ypr = _mm256_add_pd(fr, rgr); ypi = _mm256_add_pd(fi, rgi); ymr = _mm256_sub_pd(fr, rgr); ymi = _mm256_sub_pd(fi, rgi);}
                V cpr, cpi, cmr, cmi, opr, opi;
                cmul(xpr, xpi, ypr, ypi, cpr, cpi); cmul(xmr, xmi, ymr, ymi, cmr, cmi);
                V epr = _mm256_add_pd(cpr, cmr), epi = _mm256_add_pd(cpi, cmi);
                cmul(_mm256_sub_pd(cpr, cmr), _mm256_sub_pd(cpi, cmi), vwr, _mm256_mul_pd(vwi, neg), opr, opi);
                st(xr + p, _mm256_sub_pd(epr, opi)); st(xi + p, _mm256_add_pd(epi, opr));
                st(xr + q - 3, rev(_mm256_add_pd(epr, opi))); st(xi + q - 3, rev(_mm256_sub_pd(opr, epi)));}
#endif
            for (; j < len; ++j, ++p, --q) { one(xr, xi, yr, yi, p, q, wr[j], wi[j]); }}
        // Packed real convolution step on the block-ordered spectra of r chunks of n points: x <- 8 Z' of the product of the real sequences packed in x and y (y may be x);
        // chunk 0 pairs p in [h, 3h/2) with 3h - 1 - p, chunk q mirrors chunk r - q.
        static void dot(double *xr, double *xi, const double *yr, const double *yi, int n, int r) {
            const double *wr = table(false).p + n, *wi = table(true).p + n;
            one(xr, xi, yr, yi, 0, 0, 1.0, 0.0); one(xr, xi, yr, yi, 1, 1, 0.0, 1.0);
            for (int h = 2; h < n; h *= 2) { pairs(xr, xi, yr, yi, h, 2 * h - 1, h / 2, wr + h, wi + h); }
            for (int q = 1; 2 * q < r; ++q) { pairs(xr, xi, yr, yi, q * n, (r - q + 1) * n - 1, n, table(false, tableId(r, q)).p + n, table(true, tableId(r, q)).p + n); }}
    };

    // T: O(1) (at most 22 * 3 * 13 candidates), M: O(1); plan of an n-word by m-word product: the smallest r 2^lg-point FFT (r in {1, 3, 5}) and the widest k-bit pieces that the bound admits (sqrt(nx ny) 2^(2k - 2) Phi <= 0.49
    // for the piece counts nx, ny), covering the n + m words of the product or, cyclic, want >= max(n, m) words of residue; lg = 0 when none fits.
    inline FftPlan fftPlan(int n, int m, int want, bool cyclic) {
        assert(n >= 1 && m >= 1 && (!cyclic || want >= max(n, m)));
        int lo = cyclic ? 5 : FFT_MIN_LOG;
        for (int lg = lo; lg <= FFT_MAX_LOG; ++lg) {
            for (int r : {1, 5, 3}) {
                int l = lg - (r == 5 ? 2 : r == 3 ? 1 : 0);
                if (l < lo) { continue; }
                lng pts = lng(2 * r) << l;
                double phi = FFT_PHI[size_t(r)][size_t(l)];
                for (int k = FFT_MAX_K; k >= FFT_MIN_K; --k) {
                    lng nx = (64LL * n + k - 1) / k + 1, ny = (64LL * m + k - 1) / k + 1;
                    if (double(nx) * double(ny) * std::ldexp(phi * phi, 4 * k - 4) > 0.2401) { continue; }
                    if (cyclic ? k * pts < 64LL * want : nx + ny - 1 > pts) { break; }
                    return {l, r, k, cyclic ? int(k * pts / 64) : n + m};}}}
        return {0, 0, 0, 0};}
    // T: O(n), M: O(1); balanced k-bit pieces x_j = d_j + b_(j-1) - 2^k b_j of the digits d_j of a, b_j = [d_j >= 2^(k-1)] (so |x_j| <= 2^(k-1) without a carry chain) and the
    // top piece b_last, even pieces into re and odd into im (n2 each, the rest zero; a top piece at 2 n2 folds into x_0); returns the piece count.
    inline int fftSplit(double *re, double *im, int n2, const W *a, int n, int k) {
        W mask = (W(1) << k) - 1, half = W(1) << (k - 1);
        int pieces = int((64LL * n + k - 1) / k), j = 0;
        lng bprev = 0;
#ifdef __AVX2__
        // Eight pieces per step, the even ones in one vector and the odd ones in another; each lane gathers its digit from the two words it spans (all within the four loaded)
        __m256i lane = _mm256_setr_epi64x(0, 2 * k, 4 * k, 6 * k), vk = _mm256_set1_epi64x(k), vmask = _mm256_set1_epi64x(lng(mask)), vhalf = _mm256_set1_epi64x(lng(half) - 1);
        __m256i kbit = _mm256_set1_epi64x(lng(1) << k), one = _mm256_set1_epi64x(1), two = _mm256_set1_epi32(2), sixtyfour = _mm256_set1_epi64x(64), magic = _mm256_set1_epi64x(0x4338000000000000LL);
        __m256d dmagic = _mm256_set1_pd(0x1.8p52);
        auto digits = [&](__m256i sv, __m256i words) {
            __m256i idx = _mm256_srli_epi64(sv, 6), sh = _mm256_and_si256(sv, _mm256_set1_epi64x(63));
            __m256i idx2 = _mm256_add_epi32(_mm256_slli_epi32(_mm256_shuffle_epi32(idx, 0xA0), 1), _mm256_setr_epi32(0, 1, 0, 1, 0, 1, 0, 1));
            __m256i lo = _mm256_permutevar8x32_epi32(words, idx2), hi = _mm256_permutevar8x32_epi32(words, _mm256_add_epi32(idx2, two));
            return _mm256_and_si256(_mm256_or_si256(_mm256_srlv_epi64(lo, sh), _mm256_sllv_epi64(hi, _mm256_sub_epi64(sixtyfour, sh))), vmask);};
        auto toDouble = [&](__m256i v) { return _mm256_sub_pd(_mm256_castsi256_pd(_mm256_add_epi64(v, magic)), dmagic); };
        for (; j + 8 <= pieces && (lng(k) * j >> 6) + 4 <= n; j += 8) {
            lng o = lng(k) * j; int w0 = int(o >> 6);
            __m256i sv = _mm256_add_epi64(_mm256_set1_epi64x(o - 64 * lng(w0)), lane), words = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a + w0));
            __m256i de = digits(sv, words), dodd = digits(_mm256_add_epi64(sv, vk), words), be = _mm256_cmpgt_epi64(de, vhalf), bo = _mm256_cmpgt_epi64(dodd, vhalf);
            __m256i bup = _mm256_blend_epi32(_mm256_permute4x64_epi64(bo, 0x90), _mm256_set1_epi64x(-bprev), 0x03);
            __m256i ve = _mm256_sub_epi64(_mm256_add_epi64(de, _mm256_and_si256(bup, one)), _mm256_and_si256(be, kbit));
            __m256i vo = _mm256_sub_epi64(_mm256_add_epi64(dodd, _mm256_and_si256(be, one)), _mm256_and_si256(bo, kbit));
            _mm256_storeu_pd(re + (j >> 1), toDouble(ve)); _mm256_storeu_pd(im + (j >> 1), toDouble(vo));
            bprev = -lng(_mm256_extract_epi64(bo, 3));}
#endif
        for (; j < pieces; ++j) {
            lng o = lng(k) * j; int w = int(o >> 6), s = int(o & 63);
            W d = (a[w] >> s) | (((w + 1 < n ? a[w + 1] : 0) << 1) << (63 - s)), b = (d & mask) >= half;
            (j & 1 ? im : re)[j >> 1] = double(lng(d & mask) + bprev - lng(b << k));
            bprev = lng(b);}
        if (j == 2 * n2) { re[0] += double(bprev); }
        else { (j & 1 ? im : re)[j >> 1] = double(bprev); ++j; }
        std::fill(re + (j + 1) / 2, re + n2, 0.0); std::fill(im + j / 2, im + n2, 0.0);
        return j;}
    // T: O(len + pieces), M: O(1); words of the sum of the rounded coefficients c_j 2^(k j): the len words of an exact product (the pieces above them cancel), or the fold of all pieces of a cyclic residue.
    inline void fftMerge(W *r, int len, const double *re, const double *im, int pieces, int k, double scale, bool cyclic) {
        int last = cyclic ? pieces : min(pieces, int((64LL * len + k - 1) / k)), w = 0, s = 0;
        // lo and hi: the chunks of the pieces starting in word w that land in words w and w + 1; |c_j| < 2^51, so the bits of c_j + 1.5 2^52 are those of 1.5 2^52 plus the rounded value
        ulll lo = 0; lng hi = 0, up = 0; lll carry = 0;
        auto flush = [&] { lll t = lll(lo) + up + carry; r[w++] = W(t); carry = t >> 64; up = hi; lo = 0; hi = 0; };
        auto step = [&](double v) {
            lng c = std::bit_cast<lng>(v * scale + 0x1.8p52) - 0x4338000000000000LL;
            lo += W(c) << s; hi += (c >> 1) >> (63 - s);
            if ((s += k) >= 64) { flush(); s -= 64; }};
        for (int j = 0; j + 1 < last; j += 2) { step(re[j >> 1]); step(im[j >> 1]); }
        if (last & 1) { step(re[last >> 1]); }
        while (w < len) { flush(); }
        if (!cyclic) { return; }
        lng top = lng(up + carry);
        if (top >= 0) { for (W c = addWord(r, len, W(top)); c; c = addWord(r, len, c)) {} }
        else if (subWord(r, len, W(-top))) { subWord(r, len, 1); }}
    // T: O(1), M: O(N) for the largest plan; the two process-wide piece buffers of the transient operands.
    inline HugeVec<double> &fftBuf(bool second) { static HugeVec<double> x, y; return second ? y : x; }
    // T: O(N * log(N)) for the N pieces of the plan, M: O(1); the forward transform of a (n words) under p into re and im (r 2^lg doubles each, caller-owned); returns the piece count.
    inline int fftForward(double *re, double *im, const FftPlan &p, const W *a, int n) {
        int len = 1 << p.lg;
        Fft::ensure(len, p.r);
        int pieces = fftSplit(re, im, p.r * len, a, n, p.k);
        if (p.r > 1) { Fft::column(re, im, len, p.r, false); }
        for (int q = 0; q < p.r; ++q) { int t = Fft::tableId(p.r, q); Fft::transform(re + q * len, im + q * len, len, false, Fft::table(false, t).p, Fft::table(true, t).p); }
        return pieces;}
    // T: O(N * log(N)), M: O(1); r (len words) from the spectra x (destroyed) and y: the exact product of pieces = nx + ny - 1 coefficients, or, cyclic, a representative in [0, B^len) of it mod (B^len - 1).
    inline void fftProduct(W *r, int len, const FftPlan &p, double *xr, double *xi, const double *yr, const double *yi, int pieces, bool cyclic) {
        int n = 1 << p.lg;
        Fft::dot(xr, xi, yr, yi, n, p.r);
        for (int q = 0; q < p.r; ++q) { int t = Fft::tableId(p.r, q); Fft::transform(xr + q * n, xi + q * n, n, true, Fft::table(false, t).p, Fft::table(true, t).p); }
        if (p.r > 1) { Fft::column(xr, xi, n, p.r, true); }
        fftMerge(r, len, xr, xi, pieces, p.k, std::ldexp(1.0, -(p.lg + 3)) / p.r, cyclic);}
    // T: O(N * log(N)) for the N pieces of the plan, M: O(N); r = a * b (n + m words) or, cyclic, p.len words holding a representative in [0, B^len) of a * b mod (B^len - 1).
    inline void fftRun(W *r, const FftPlan &p, const W *a, int n, const W *b, int m, bool cyclic) {
        int n2 = p.r << p.lg;
        bool same = a == b && n == m;
        HugeVec<double> &bx = fftBuf(false), &by = fftBuf(true);
        bx.resize(2 * n2);
        if (!same) { by.resize(2 * n2); }
        double *xr = bx.p, *xi = xr + n2, *yr = same ? xr : by.p, *yi = yr + n2;
        int nx = fftForward(xr, xi, p, a, n), ny = same ? nx : fftForward(yr, yi, p, b, m);
        fftProduct(r, p.len, p, xr, xi, yr, yi, cyclic ? 2 * n2 : nx + ny - 1, cyclic);}
    // S: O(N * log(N)) for the plan's N pieces (take), U: NA, Q: NA, M: O(N) in caller storage; the forward transform of an n-word operand under plan p, kept (re then im, r 2^lg doubles each) for repeated products with operands of at most cap words.
    struct Spectrum {
        FftPlan p{}; double *re = nullptr, *im = nullptr; int n = 0, cap = 0, pieces = 0;
        static int words(const FftPlan &q) { return q.lg ? 2 * (q.r << q.lg) : 0; }
        void take(W *buf, const FftPlan &q, const W *a, int len, int other) { p = q; re = reinterpret_cast<double *>(buf); im = re + (q.r << q.lg); n = len; cap = other; pieces = fftForward(re, im, p, a, len); }
    };
    // T: O(N * log(N)), M: O(1); r = a (n <= s.cap words) times the operand of s: n + s.n words, or, cyclic, s.p.len words of a representative of the residue as fftRun.
    inline void fftWith(W *r, const Spectrum &s, const W *a, int n, bool cyclic) {
        assert(n <= s.cap);
        int n2 = s.p.r << s.p.lg;
        HugeVec<double> &bx = fftBuf(false);
        bx.resize(2 * n2);
        int nx = fftForward(bx.p, bx.p + n2, s.p, a, n);
        fftProduct(r, cyclic ? s.p.len : n + s.n, s.p, bx.p, bx.p + n2, s.re, s.im, cyclic ? 2 * n2 : nx + s.pieces - 1, cyclic);}
    // T: O(len * log(len)), M: O(len); cyclic limb convolution of cyclicLimbs(len, n, m) limbs of the limb images of a and b (words) through the exact NTTs; lo and hi (that many words each) hold lo + hi * 2^64 per limb.
    inline void nttCyclic(W *lo, W *hi, const W *a, int n, const W *b, int m, int len) {
        static HugeVec<uint> la, lb;
        bool same = a == b && n == m;
        la.resize(2 * n);
        for (int i = 0; i < n; ++i) { la[2 * i] = uint(a[i]); la[2 * i + 1] = uint(a[i] >> 32); }
        if (!same) {
            lb.resize(2 * m);
            for (int i = 0; i < m; ++i) { lb[2 * i] = uint(b[i]); lb[2 * i + 1] = uint(b[i] >> 32); }}
        int limbs = cyclicLimbs(len, n, m);
        const uint *pa = la.p, *pb = same ? la.p : lb.p;
        int na = 2 * n, nb = same ? 2 * n : 2 * m;
        if (limbs <= (1 << NTT_MAX_LOG)) { ntt3Cyclic(lo, hi, pa, na, pb, nb, limbs); return; }
        ntt64Cyclic(lo, hi, pa, na, pb, nb, limbs);}
    // T: O((n + m) * log(n + m)), M: O(n + m); r (n + m words) = a * b: the double FFT while its bound is exact, else the NTTs' limb convolution.
    inline void transformMul(W *r, const W *a, int n, const W *b, int m) {
        FftPlan p = fftPlan(n, m, n + m, false);
        if (p.lg) { fftRun(r, p, a, n, b, m, false); return; }
        int limbs = cyclicLimbs(n + m, n, m);
        Scratch s(2 * size_t(limbs));
        W *lo = s, *hi = lo + limbs;
        nttCyclic(lo, hi, a, n, b, m, n + m);
        WW carry = 0;
        for (int i = 0; i < n + m; ++i) {
            WW c0 = lo[2 * i] + (WW(hi[2 * i]) << 64) + carry;
            W low = uint(c0); carry = c0 >> 32;
            WW c1 = lo[2 * i + 1] + (WW(hi[2 * i + 1]) << 64) + carry;
            r[i] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}}
    // T: NA, M: O(1); a cyclic product plan: the FFT plan (lg > 0) or the NTT limb count, and the residue length len in words.
    struct CyclicPlan { FftPlan p; int limbs, len; };
    // T: O(1), M: O(1); the plan of a * b mod (B^len - 1) for n- and m-word operands with len >= max(want, n, m): the FFT plan while its bound holds, else the NTTs' limb count.
    inline CyclicPlan cyclicPlan(int n, int m, int want) {
        want = max({want, n, m});
        FftPlan p = fftPlan(n, m, want, true);
        if (p.lg) { return {p, 0, p.len}; }
        int limbs = cyclicLimbs(want, n, m);
        return {p, limbs, limbs / 2};}
    // T: O(len), M: O(1); r (len words) becomes the canonical residue in [0, B^len - 1): the all-ones representative of 0 is cleared.
    inline void canonical(W *r, int len) { if (std::all_of(r, r + len, [](W w) { return w == ~W(0); })) { zero(r, len); } }
    // T: O(len * log(len)), M: O(len); r (c.len words) = the canonical residue of a * b mod (B^len - 1): the plan's FFT, else the NTTs' limb convolution carried with its wrap.
    inline void mulCyclic(W *r, const CyclicPlan &c, const W *a, int n, const W *b, int m) {
        if (c.p.lg) { fftRun(r, c.p, a, n, b, m, true); canonical(r, c.len); return; }
        Scratch s(2 * size_t(c.limbs));
        W *lo = s, *hi = lo + c.limbs;
        nttCyclic(lo, hi, a, n, b, m, c.len);
        WW carry = 0;
        for (int i = 0; i < c.len; ++i) {
            WW c0 = lo[2 * i] + (WW(hi[2 * i]) << 64) + carry;
            W low = uint(c0); carry = c0 >> 32;
            WW c1 = lo[2 * i + 1] + (WW(hi[2 * i + 1]) << 64) + carry;
            r[i] = low | (W(uint(c1)) << 32); carry = c1 >> 32;}
        for (W w = W(carry); w; w = addWord(r, c.len, w)) {}
        canonical(r, c.len);}
    // T: O(n), M: O(1); r (len words) = the canonical residue of a (n words) mod (B^len - 1): the sum of its len-word chunks with the wrapped carries.
    inline void foldMod1(W *r, int len, const W *a, int n) {
        int k = min(n, len);
        std::copy(a, a + k, r); zero(r + k, len - k);
        for (int i = len; i < n; i += len) {
            for (W c = add(r, r, len, a + i, min(len, n - i)); c; c = addWord(r, len, c)) {}}
        canonical(r, len);}
    // T: O(len), M: O(1); r = a - b mod (B^len - 1), canonical for canonical a and b (r may alias a).
    inline void subMod1(W *r, const W *a, const W *b, int len) { if (sub(r, a, len, b, len)) { subWord(r, len, 1); } }
    // S: O(len), Q: O(len) per ring operation and O(cnt * len * log(cnt)) per transform, M: O(len); Z/(2^N + 1) for N = 64 len: an element is
    // len + 1 words valued in [0, 2^N]; r may alias a.
    struct Fermat {
        int len; Scratch t;
        explicit Fermat(int l) : len(l), t(size_t(2 * l) + 2) {}
        void add(W *r, const W *a, const W *b) const {
            infint_detail::add(r, a, len + 1, b, len + 1);
            if (r[len] && (r[len] == 2 || norm(r, len))) { W hi = r[len] - 1; r[len] = hi - subWord(r, len, 1); }}
        void sub(W *r, const W *a, const W *b) const { if (infint_detail::sub(r, a, len + 1, b, len + 1)) { r[len] += 1 + addWord(r, len, 1); } }
        void neg(W *r) const {
            if (r[len]) { r[len] = 0; r[0] = 1; return; }
            if (!norm(r, len)) { return; }
            for (int i = 0; i < len; ++i) { r[i] = ~r[i]; }
            r[len] = addWord(r, len, 2);}
        // r = a * 2^s for 0 <= s < 2N: the bits shifted past N come back negated.
        void shift(W *r, const W *a, int s) {
            bool flip = s >= 64 * len; if (flip) { s -= 64 * len; }
            int ws = s / 64, bs = s % 64;
            zero(t, ws);
            if (bs) { t[ws + len + 1] = shl(t + ws, a, len + 1, bs); } else { std::copy(a, a + len + 1, t + ws); t[ws + len + 1] = 0; }
            r[len] = 0;
            if (infint_detail::sub(r, t, len, t + len, min(ws + 2, len))) { r[len] = addWord(r, len, 1); }
            if (flip) { neg(r); }}
        // r = p mod (2^N + 1) for the 2 len + 2 word product p of two elements.
        void reduce(W *r, const W *p) const {
            std::copy(p, p + len, r);
            if (p[2 * len]) { r[len] = addWord(r, len, 1); return; }
            r[len] = 0;
            if (infint_detail::sub(r, r, len, p + len, len)) { r[len] = addWord(r, len, 1); }}
        // Forward (natural to bit-reversed order) or inverse (back, scaled by 1 / cnt) transform of cnt elements with the root 2^(2N / cnt).
        void transform(W *x, int cnt, bool inverse) {
            int e = len + 1, n2 = 128 * len; Scratch u(size_t(len) + 1);
            for (int span = inverse ? 2 : cnt; inverse ? span <= cnt : span >= 2; span = inverse ? 2 * span : span / 2) {
                int half = span / 2, step = n2 / span;
                for (int i = 0; i < cnt; i += span) {
                    for (int j = 0; j < half; ++j) {
                        W *p = x + size_t(i + j) * e, *q = p + size_t(half) * e;
                        if (inverse) { shift(u, q, j ? n2 - j * step : 0); sub(q, p, u); add(p, p, u); }
                        else { sub(u, p, q); add(p, p, q); shift(q, u, j * step); }}}}
            if (inverse) { for (int i = 0; i < cnt; ++i) { shift(x + size_t(i) * e, x + size_t(i) * e, n2 - std::countr_zero(uint(cnt))); } }}
    };
    // T: O(n * log(n)) word operations with the pointwise products through the transforms (O(n * log(n) * log(log(n))) bit operations recursively), M: O(n + m): 2^k pieces of p words,
    // k = (bit_width(n + m) + 1) / 2, convolved over a Fermat ring; n >= m >= 1, n + m <= 2^30, r has n + m words and may alias a or b.
    inline void schonhageStrassen(W *r, const W *a, int n, const W *b, int m) {
        int k = (std::bit_width(uint(n + m)) + 1) / 2, cnt = 1 << k, p = (n + m) / cnt + 1, q = max(1, cnt >> 7), len = (2 * p + q) / q * q, e = len + 1;
        Fermat f(len);
        bool same = a == b && n == m;
        size_t all = size_t(cnt) * e;
        Scratch s((same ? 1 : 2) * all + 2 * size_t(e));
        W *A = s, *B = same ? A : A + all, *t = B + all;
        auto split = [&](W *v, const W *x, int sz) {
            std::fill(v, v + all, 0);
            for (int i = 0; i * p < sz; ++i) { std::copy(x + i * p, x + min(sz, (i + 1) * p), v + size_t(i) * e); }
            f.transform(v, cnt, false);};
        split(A, a, n);
        if (!same) { split(B, b, m); }
        for (int i = 0; i < cnt; ++i) { mulAny(t, A + size_t(i) * e, e, B + size_t(i) * e, e); f.reduce(A + size_t(i) * e, t); }
        f.transform(A, cnt, true);
        zero(r, n + m);
        for (int j = 0; j * p < n + m; ++j) { const W *c = A + size_t(j) * e; addInto(r + j * p, min(len + 1, n + m - j * p), c, norm(c, len)); }}
    // T: O(1), M: O(1); transform work of a product of len words: the smallest r 2^lg >= len (r in {1, 3, 5}) times the log of that length.
    inline ulng transformWork(int len) {
        ulng p = std::bit_ceil(uint(len)), m = 5 * p / 8 >= ulng(len) ? 5 * p / 8 : 3 * p / 4 >= ulng(len) ? 3 * p / 4 : p;
        return m * std::bit_width(p);}
    // T: O(1), M: O(1); true when a transform product of len words beats direct n x m word products: 2 n m (3 n m with a kept spectrum, one transform of three saved) reaches UNBALANCED_FFT transformWork units, UNBALANCED_FFT_BIG from 2^16 words.
    inline bool transformWins(int n, int m, int len, bool kept) { return len <= NTT_MAX_WORDS && (kept ? 3 : 2) * ulng(n) * ulng(m) >= ulng(len < (1 << 16) ? UNBALANCED_FFT : UNBALANCED_FFT_BIG) * transformWork(len); }
    // T: O(M(n)) word products by size: basecase, Karatsuba, Toom-3, transforms, M: O(n + m) workspace; n >= m >= 1, r has n + m words.
    inline void mul(W *r, const W *a, int n, const W *b, int m) {
        if (m == 1) { r[n] = mulWord(r, a, n, b[0]); return; }
        if (a == b && n == m) { square(r, a, n); return; }
        if (m < KARATSUBA) { mulBasecase(r, a, n, b, m); return; }
        if (m >= FFT && n + m <= NTT_MAX_WORDS) {
            int chunk = 0; ulng best = 9 * transformWork(n + m) / 10;
            for (int s = int(std::bit_ceil(uint(n + m))) / 2; s > m; s /= 2) {
                int c = s - m, pieces = (n + c - 1) / c;
                ulng work = ulng(pieces - 1) * transformWork(s) + transformWork(n - (pieces - 1) * c + m);
                if (work < best) { best = work; chunk = c; }}
            if (chunk) { unbalancedMul(r, a, n, b, m, chunk); return; }
            transformMul(r, a, n, b, m); return;}
        if (n >= 2 * m) {
            if (n + m >= 2 * FFT && transformWins(n, m, n + m, false)) { transformMul(r, a, n, b, m); return; }
            unbalancedMul(r, a, n, b, m, m); return;}
        if (m >= TOOM3 && n == m) { toom3(r, a, b, n); return; }
        karatsuba(r, a, n, b, m);}
    // T: O(M(n)) word products by size: basecase, Karatsuba, Toom-3, transforms, M: O(n) workspace; n >= 1, r has 2n words.
    inline void square(W *r, const W *a, int n) {
        if (n < KARATSUBA_SQR) { squareBasecase(r, a, n); return; }
        if (n >= FFT_SQR && 2 * n <= NTT_MAX_WORDS) { transformMul(r, a, n, a, n); return; }
        if (n >= TOOM3_SQR) { toom3Square(r, a, n); return; }
        karatsubaSquare(r, a, n);}
    // T: O(1), M: O(1); moller-Granlund reciprocal of a normalized divisor (top bit set).
    inline W invWord(W d) { return W(((WW(~d) << 64) | ~W(0)) / d); }
    // T: O(1), M: O(1); q = (u1:u0) / d, returns the remainder; d normalized, u1 < d, v = invWord(d).
    inline W div2by1(W &q, W u1, W u0, W d, W v) {
        WW t = WW(v) * u1 + ((WW(u1) << 64) | u0);
        W q1 = W(t >> 64) + 1, q0 = W(t), r = u0 - q1 * d;
        if (r > q0) { --q1; r += d; }
        if (r >= d) { ++q1; r -= d; }
        q = q1;
        return r;}
    // T: O(n), M: O(1); q = a / d (q may alias a), returns a % d; d != 0.
    inline W divWord(W *q, const W *a, int n, W d) {
        if (d == 0) { d = 1; }
        int s = std::countl_zero(d);
        W dn = d << s, v = invWord(dn), r = 0;
        if (s == 0) {
            for (int i = n - 1; i >= 0; --i) { r = div2by1(q[i], r, a[i], dn, v); }
            return r;}
        r = a[n - 1] >> (64 - s);
        for (int i = n - 1; i >= 0; --i) {
            W u = (a[i] << s) | (i ? a[i - 1] >> (64 - s) : 0);
            r = div2by1(q[i], r, u, dn, v);}
        return r >> s;}
    // T: O(n), M: O(1); a % d for d != 0.
    inline W remWord(const W *a, int n, W d) {
        if (d == 0) { d = 1; }
        int s = std::countl_zero(d);
        W dn = d << s, v = invWord(dn), r = 0, q;
        if (s == 0) {
            for (int i = n - 1; i >= 0; --i) { r = div2by1(q, r, a[i], dn, v); }
            return r;}
        r = a[n - 1] >> (64 - s);
        for (int i = n - 1; i >= 0; --i) { r = div2by1(q, r, (a[i] << s) | (i ? a[i - 1] >> (64 - s) : 0), dn, v); }
        return r >> s;}
    // T: O((n - m) * m) word products, M: O(n); Knuth D: q (n - m + 1 words), r (m words) for normalized b of m >= 2 words.
    inline void divBasecase(W *q, W *r, const W *a, int n, const W *b, int m) {
        Scratch uv(size_t(n) + 1);
        W *u = uv, dt = b[m - 1], ds = b[m - 2], v = invWord(dt);
        std::copy(a, a + n, u); u[n] = 0;
        for (int j = n - m; j >= 0; --j) {
            W qh, rh; bool refine = true;
            if (u[j + m] >= dt) { qh = ~W(0); rh = u[j + m - 1] + dt; refine = rh >= dt; }
            else { rh = div2by1(qh, u[j + m], u[j + m - 1], dt, v); }
            while (refine && WW(qh) * ds > ((WW(rh) << 64) | u[j + m - 2])) { --qh; rh += dt; refine = rh >= dt; }
            W borrow = subMul(u + j, b, m, qh), top = u[j + m];
            u[j + m] = top - borrow;
            if (top < borrow) { --qh; u[j + m] += add(u + j, u + j, m, b, m); }
            q[j] = qh;}
        std::copy(u, u + m, r);}
    inline void div2n1n(W *q, W *r, const W *a, const W *b, int n);
    // T: O(M(n) * log(n)), M: O(n); burnikel-Ziegler 3n/2n step: a (3n words) < b * B^n, b (2n words) normalized; q n words, r 2n words.
    inline void div3n2n(W *q, W *r, const W *a, const W *b, int n) {
        Scratch s(5 * size_t(n) + 1);
        W *rr = s, *d = rr + 3 * n + 1;
        std::copy(a, a + n, rr); zero(rr + 2 * n, n + 1);
        if (cmp(a + 2 * n, n, b + n, n) < 0) { div2n1n(q, rr + n, a + n, b + n, n); }
        else {
            std::fill(q, q + n, ~W(0));
            std::copy(a + n, a + 3 * n, rr + n);
            subFrom(rr + 2 * n, n, b + n, n); addInto(rr + n, 2 * n + 1, b + n, n);}
        mulAny(d, q, n, b, n);
        W borrow = sub(rr, rr, 3 * n + 1, d, 2 * n);
        while (borrow) { subWord(q, n, 1); borrow -= add(rr, rr, 3 * n + 1, b, 2 * n); }
        std::copy(rr, rr + 2 * n, r);}
    // T: O(M(n) * log(n)), M: O(n); a (2n words) < b * B^n, b (n words) normalized; q n words, r n words.
    inline void div2n1n(W *q, W *r, const W *a, const W *b, int n) {
        if ((n & 1) || n <= BURNIKEL) {
            Scratch qq(size_t(n) + 1);
            divBasecase(qq, r, a, 2 * n, b, n);
            std::copy(qq.p, qq.p + n, q);
            return;}
        int k = n / 2;
        Scratch t(3 * size_t(k));
        div3n2n(q + k, t + k, a + k, b, k);
        std::copy(a, a + k, t.p);
        div3n2n(q, r, t, b, k);}
    // T: O(M(m) * log(m) * n / m), M: O(n); recursive division for normalized b of m words, n >= m.
    inline void divBurnikelZiegler(W *q, W *r, const W *a, int n, const W *b, int m) {
        int k = 0;
        while ((m + (1 << k) - 1) >> k > BURNIKEL) { ++k; }
        int bs = (m + (1 << k) - 1) >> k, blk = bs << k, pad = blk - m, na = n + pad, chunks = (na + blk - 1) / blk + 1;
        Scratch s(size_t(blk) + na + 2 * size_t(blk) + size_t(chunks) * blk);
        W *bb = s, *aa = bb + blk, *z = aa + na, *qq = z + 2 * blk;
        zero(bb, blk); zero(aa, na); zero(z, 2 * blk);
        std::copy(b, b + m, bb + pad); std::copy(a, a + n, aa + pad);
        if (na != (chunks - 1) * blk || cmp(aa + na - blk, blk, bb, blk) < 0) { --chunks; }
        zero(qq, chunks * blk);
        for (int c = chunks - 1; c >= 0; --c) {
            int lo = c * blk, hi = min(lo + blk, na);
            std::copy(aa + lo, aa + hi, z); std::fill(z + (hi - lo), z + blk, 0);
            if (c == chunks - 1) { std::copy(z, z + blk, z + blk); continue; }
            div2n1n(qq + lo, z + blk, z, bb, blk);}
        std::copy(qq, qq + (n - m + 1), q);
        std::copy(z + blk + pad, z + 2 * blk, r);}
    // T: O(1), M: O(1); the inverse of the odd b modulo 2^64.
    inline W invMod64(W b) {
        W x = b;
        for (int i = 0; i < 6; ++i) { x *= 2 - b * x; }
        return x;}
    // T: O(M(m)), M: O(m); v (m words) with B^m + v <= floor(B^(2m) / d) <= B^m + v + 1 for normalized d of m words: Knuth D on B^(2m) - 1 below BURNIKEL words, else Newton on the
    // top h = m / 2 + 1 words: xh = B^h + vh - 4, e = B^(m + h) - xh d in [0, 6 B^m) from a cyclic product, x = xh B^l + (xh (e >> (h - 2) words)) >> (h + 2) words, both products sharing xh's spectrum.
    inline void invert(W *v, const W *d, int m) {
        if (m < BURNIKEL) {
            Scratch s(4 * size_t(m) + 1);
            W *num = s, *q = num + 2 * m, *rem = q + m + 1;
            std::fill(num, num + 2 * m, ~W(0));
            if (m == 1) { divWord(q, num, 2, d[0]); }
            else { divBasecase(q, rem, num, 2 * m, d, m); }
            std::copy(q, q + m, v);
            return;}
        int h = m / 2 + 1, l = m - h;
        bool fft = m >= FFT;
        CyclicPlan c = fft ? cyclicPlan(m, h + 1, m + 3) : CyclicPlan{{0, 0, 0, 0}, 0, m + h + 1};
        int len = c.len;
        Scratch s(size_t(h) + 1 + 2 * size_t(len) + size_t(Spectrum::words(c.p)));
        W *xh = s, *e = xh + h + 1, *t = e + len;
        invert(xh, d + l, h); xh[h] = 1; subWord(xh, h + 1, 4);
        if (fft) {
            Spectrum sx;
            if (c.p.lg) { sx.take(t + len, c.p, xh, h + 1, m); fftWith(e, sx, d, m, true); canonical(e, len); }
            else { mulCyclic(e, c, xh, h + 1, d, m); }
            zero(t, len); t[(m + h) % len] = 1;
            subMod1(e, t, e, len);
            if (c.p.lg) { fftWith(t, sx, e + (h - 2), l + 3, true); canonical(t, len); }
            else { mulCyclic(t, c, xh, h + 1, e + (h - 2), l + 3); }}
        else {
            mulAny(t, d, m, xh, h + 1);
            zero(e, len); e[m + h] = 1; subFrom(e, len, t, len);
            mulAny(t, xh, h + 1, e + (h - 2), l + 3);}
        assert(norm(e, len) <= m + 1 && e[m] < 6);
        zero(e, l); std::copy(xh, xh + h + 1, e + l);
        addInto(e, m + 1, t + h + 2, l + 1);
        if (e[m] == 1) { std::copy(e, e + m, v); }
        else { std::fill(v, v + m, e[m] ? ~W(0) : W(0)); }}
    // T: O(M(m)), M: O(m); x (m + 1 words) = floor(B^(2m) / d) exactly for normalized d of m words: invert, then the residue B^(2m) - d x settles the unit.
    inline void invertExact(W *x, const W *d, int m) {
        invert(x, d, m); x[m] = 1;
        CyclicPlan c = m >= FFT ? cyclicPlan(m, m + 1, m + 2) : CyclicPlan{{0, 0, 0, 0}, 0, 2 * m + 1};
        Scratch s(2 * size_t(c.len));
        W *e = s, *t = e + c.len;
        if (c.p.lg || c.limbs) { mulCyclic(e, c, d, m, x, m + 1); }
        else { mulAny(e, d, m, x, m + 1); }
        zero(t, c.len); t[(2 * m) % c.len] = 1;
        subMod1(e, t, e, c.len);
        if (cmp(e, norm(e, c.len), d, m) >= 0) { addWord(x, m + 1, 1); }}
    // S: O(M(m)) (prepare), U: NA, Q: O(M(m)) per quotient block (divBlocks), M: O(m + N) words (the inverse and two spectra of N doubles); a divisor b (m words) prepared for quotient blocks of at most blk <= m words: d = b << shift normalized,
    // v (blk words) with B^blk + v within one of floor(B^(m + blk) / d) on either side (one-sided below from blk >= m - 1) from the inverse of the top min(m, blk + 1) words, the spectrum of v (linear, from FFT words) and, with cyclic, of d (cyclic, len > m words, from cyclicGate(m) block words).
    struct Divisor {
        const W *d = nullptr; W *v = nullptr; int m = 0, blk = 0, shift = 0; Spectrum sv, sd; CyclicPlan cr{};
        static FftPlan quotientPlan(int blk) { return blk >= FFT ? fftPlan(blk, blk, 2 * blk, false) : FftPlan{0, 0, 0, 0}; }
        static CyclicPlan remainderPlan(int m, int blk) { return blk >= cyclicGate(m) ? cyclicPlan(m, blk, m + 1) : CyclicPlan{{0, 0, 0, 0}, 0, 0}; }
        static size_t words(int m, int blk) { return size_t(m) + blk + Spectrum::words(quotientPlan(blk)) + Spectrum::words(remainderPlan(m, blk).p); }
        void prepare(W *buf, const W *b, int mw, int bw, bool cyclic = true) {
            assert(bw >= 1 && bw <= mw);
            m = mw; blk = bw; shift = std::countl_zero(b[m - 1]);
            W *dn = buf; d = dn; v = dn + m;
            if (shift) { shl(dn, b, m, shift); } else { std::copy(b, b + m, dn); }
            int k = min(m, blk + 1);
            Scratch x{size_t(k)};
            invert(x, dn + m - k, k);
            std::copy(x.p + (k - blk), x.p + k, v);
            FftPlan pq = quotientPlan(blk);
            W *p = v + blk;
            if (pq.lg) { sv.take(p, pq, v, blk, blk); p += Spectrum::words(pq); }
            cr = cyclic ? remainderPlan(m, blk) : CyclicPlan{{0, 0, 0, 0}, 0, 0};
            if (cr.p.lg) { sd.take(p, cr.p, dn, m, blk); }}
    };
    // T: O((M(blk) + M(m)) * (n - m) / blk + m) word products (M(m) cyclic), M: O(m + blk); q (n - m + 1 words), r (m words, null allowed) of a (n words) by dv's normalized d (m words), n >= m: blocks of at most blk quotient
    // words from the top, each estimated through v (q in [q_hat - 1, q_hat + 4]) and settled by the residue (Knuth D below knuthGate(m, true) words, spectra from cyclicGate words); exact: d odd, d | a, quotient word q0: the last block is settled 2-adically, r is zero.
    inline void divBlocks(W *q, W *r, const W *a, int n, const Divisor &dv, bool exact = false, W q0 = 0) {
        int m = dv.m, blk = dv.blk, qn = n - m;
        const W *d = dv.d;
        bool cyc = dv.cr.len > 0;
        int len = cyc ? dv.cr.len : m + blk, pl = max({2 * blk, len, m + blk});
        Scratch s(size_t(m) + blk + size_t(blk) + 1 + size_t(pl) + (cyc ? size_t(len) : 0));
        W *rem = s, *qe = rem + m + blk, *prod = qe + blk + 1, *res = prod + pl;
        std::copy(a + qn, a + n, rem);
        q[qn] = cmp(rem, m, d, m) >= 0;
        if (q[qn]) { subFrom(rem, m, d, m); }
        for (int pos = qn, t = qn ? (qn - 1) % blk + 1 : 0; pos > 0; pos -= t, t = blk) {
            std::copy_backward(rem, rem + m, rem + m + t); std::copy(a + pos - t, a + pos, rem);
            if (t < knuthGate(m, true)) { divBasecase(qe, rem, rem, m + t, d, m); std::copy(qe, qe + t, q + pos - t); continue; }
            const W *top = rem + m;
            if (dv.sv.p.lg && t >= cyclicGate(blk)) { fftWith(prod, dv.sv, top, t, false); }
            else { mulAny(prod, top, t, dv.v, blk); }
            if (add(qe, prod + blk, t, top, t)) { std::fill(qe, qe + t, ~W(0)); }
            if (exact && pos == t) {
                W delta = q0 - qe[0];
                assert(delta <= 4);
                addWord(qe, t, delta);
                std::copy(qe, qe + t, q + pos - t);
                zero(rem, m);
                break;}
            if (cyc && t >= cyclicGate(m)) {
                foldMod1(res, len, rem, m + t);
                if (dv.cr.p.lg) { fftWith(prod, dv.sd, qe, t, true); canonical(prod, len); }
                else { mulCyclic(prod, dv.cr, qe, t, d, m); }
                subMod1(res, res, prod, len);
                while (res[m] > 4) { addWord(res, len, 1); add(res, res, len, d, m); subWord(qe, t, 1); }
                while (cmp(res, norm(res, m + 1), d, m) >= 0) { subFrom(res, m + 1, d, m); addWord(qe, t, 1); }
                std::copy(res, res + m, rem);}
            else {
                mulAny(prod, d, m, qe, t);
                if (sub(rem, rem, m + t, prod, m + t)) { do { subWord(qe, t, 1); } while (!add(rem, rem, m + t, d, m)); }
                while (cmp(rem, norm(rem, m + t), d, m) >= 0) { subFrom(rem, m + t, d, m); addWord(qe, t, 1); }}
            std::copy(qe, qe + t, q + pos - t);}
        if (r) { std::copy(rem, rem + m, r); }}
    // T: O((n - m) * m) word products by Knuth D (divisors below BURNIKEL or quotients below knuthGate words), O(M(m) log(m) n / m) by Burnikel-Ziegler (quotients of at least m / 2 words, divisors below NEWTON_DIV, or NEWTON_LONG from 4m quotient words),
    // else O(M(n)) by the blocks of a fresh Divisor (cyclic remainders from FFT divisor words or FRESH_CYCLIC cyclicGate(m) quotient words), M: O(n + m); q (n - m + 1 words) and r (m words) may be null; b of m >= 1 words, b[m - 1] != 0, n >= m; dv: b prepared by the caller; exact: b odd, b | a.
    inline void divRem(W *q, W *r, const W *a, int n, const W *b, int m, const Divisor *dv = nullptr, bool exact = false) {
        Scratch s(3 * size_t(n) + 4);
        W *an = s, *bn = an + n + 1, *qt = bn + m, *qq = qt + n - m + 2;
        if (!q) { q = qq; }
        if (!r) { r = qq + (n - m + 1); }
        if (m == 1) { r[0] = divWord(q, a, n, b[0]); return; }
        int sh = dv ? dv->shift : std::countl_zero(b[m - 1]);
        if (sh) { an[n] = shl(an, a, n, sh); shl(bn, b, m, sh); }
        else { std::copy(a, a + n, an); an[n] = 0; std::copy(b, b + m, bn); }
        int na = an[n] ? n + 1 : n, qn = na - m;
        assert(!exact || (b[0] & 1));
        W q0 = exact ? a[0] * invMod64(b[0]) : 0;
        if (m < BURNIKEL || qn < knuthGate(m, dv != nullptr)) { divBasecase(qt, r, an, na, bn, m); }
        else if (dv) { divBlocks(qt, r, an, na, *dv, exact, q0); }
        else if (2 * qn >= m && m < (qn >= 4 * m ? NEWTON_LONG : NEWTON_DIV)) { divBurnikelZiegler(qt, r, an, na, bn, m); }
        else {
            int nb = qn > m ? (qn + m - 1) / m : 3 * qn > m ? 2 : 1, blk = (qn + nb - 1) / nb;
            Divisor d;
            Scratch ctx(Divisor::words(m, blk));
            d.prepare(ctx, b, m, blk, m >= FFT || nb * blk >= FRESH_CYCLIC * cyclicGate(m));
            divBlocks(qt, r, an, na, d, exact, q0);}
        std::copy(qt, qt + (n - m + 1), q);
        if (sh) { shr(r, r, m, sh); }}
    // T: O(k * m) word products, M: O(n); q (k = n - m + 1 words) = a / b for odd b dividing a, word by word through the 2-adic inverse of b[0].
    inline void divExactHensel(W *q, const W *a, int n, const W *b, int m) {
        int k = n - m + 1;
        if (m == 1) { divWord(q, a, n, b[0]); return; }
        W inv0 = invMod64(b[0]);
        Scratch u(size_t(n) + 0);
        std::copy(a, a + n, u.p);
        for (int i = 0; i < k; ++i) {
            W qi = u[i] * inv0; q[i] = qi;
            int len = min(m, n - i);
            W borrow = subMul(u + i, b, len, qi);
            if (i + len < n) { subWord(u + i + len, n - i - len, borrow); }}}
    // T: O(n * b) word operations for b-bit operands (at most b subtract-and-shift steps), M: O(n); binary GCD of a (n words) and b (m words), both nonzero, result in a; returns its length.
    inline int gcdBinary(W *a, int n, W *b, int m) {
        W *out = a;
        auto ctz = [](const W *x, int len) { int c = 0; while (x[c] == 0 && c + 1 < len) { ++c; } return 64 * c + std::countr_zero(x[c]); };
        auto shiftRight = [](W *x, int &len, int bits) {
            int w = bits / 64, s = bits % 64;
            if (w) { std::copy(x + w, x + len, x); len -= w; }
            if (s) { shr(x, x, len, s); }
            len = norm(x, len);};
        int za = ctz(a, n), zb = ctz(b, m), z = min(za, zb);
        shiftRight(a, n, za); shiftRight(b, m, zb);
        while (m) {
            if (cmp(a, n, b, m) < 0) { std::swap(a, b); std::swap(n, m); }
            sub(a, a, n, b, m); n = norm(a, n);
            if (n) { shiftRight(a, n, ctz(a, n)); }
            else { std::swap(a, b); std::swap(n, m); break; }}
        int w = z / 64, s = z % 64;
        Scratch res(size_t(n) + w + 1);
        zero(res, w); std::copy(a, a + n, res + w);
        res[w + n] = s ? shl(res + w, res + w, n, s) : 0;
        n = norm(res, n + w + 1);
        std::copy(res.p, res.p + n, out);
        return n;}

    // S: O(1), U: O(n) per resize or copy, Q: O(1), M: O(n) (four words inline); little-endian words, p = buf while at most four are stored: a vector with int sizes whose resize zero-fills and whose setSize leaves new words unset.
    struct Words {
        static constexpr int INLINE = 4;
        W *p = buf; int len = 0, cap = INLINE; W buf[INLINE] = {};

        Words() {}
        Words(const Words &o) { assign(o.p, o.len); }
        Words(Words &&o) noexcept { take(o); }
        ~Words() { if (p != buf) { delete[] p; } }
        Words &operator=(const Words &o) { if (this != &o) { assign(o.p, o.len); } return *this; }
        Words &operator=(Words &&o) noexcept { if (this != &o) { if (p != buf) { delete[] p; } take(o); } return *this; }
        void take(Words &o) {
            len = o.len; cap = o.cap;
            if (o.p != o.buf) { p = o.p; o.p = o.buf; o.cap = INLINE; } else { p = buf; for (int i = 0; i < len; ++i) { buf[i] = o.buf[i]; } }
            o.len = 0;}
        W *data() { return p; }
        const W *data() const { return p; }
        int size() const { return len; }
        bool empty() const { return len == 0; }
        W &operator[](int i) { return p[i]; }
        W operator[](int i) const { return p[i]; }
        W &back() { return p[len - 1]; }
        W back() const { return p[len - 1]; }
        W *begin() { return p; }
        W *end() { return p + len; }
        const W *begin() const { return p; }
        const W *end() const { return p + len; }
        void reserve(int c) {
            if (c <= cap) { return; }
            W *q = new W[c];
            for (int i = 0; i < len; ++i) { q[i] = p[i]; }
            if (p != buf) { delete[] p; }
            p = q; cap = c;}
        void setSize(int n) { reserve(n); len = n; }
        void resize(int n, W v = 0) { reserve(n); if (n > len) { std::fill(p + len, p + n, v); } len = n; }
        void assign(int n, W v) { setSize(n); std::fill(p, p + n, v); }
        void assign(const W *q, int n) { setSize(n); for (int i = 0; i < n; ++i) { p[i] = q[i]; } }
        void push_back(W w) { if (len == cap) { reserve(2 * cap); } p[len++] = w; }
        void pop_back() { --len; }
        void clear() { len = 0; }
        friend bool operator==(const Words &a, const Words &b) { return a.len == b.len && std::equal(a.p, a.p + a.len, b.p); }
    };
} // namespace infint_detail
template<int N> struct UInt;

// T: O(n) words per linear operation, M: O(n); sign-magnitude base 2^64 words n (no leading zeros, zero is {} or {0}, four words inline) plus a signed infinity.
struct InfInt {
    using W = infint_detail::W;
    using WW = infint_detail::WW;
    using Words = infint_detail::Words;
    static constexpr const char *DIGITS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ", *DIGITS_LOWER = "0123456789abcdefghijklmnopqrstuvwxyz";
    static constexpr array<uint8_t, 256> DIGIT_VALUE = []() {
        array<uint8_t, 256> v{}; v.fill(255);
        for (int i = 0; i < 36; ++i) { v[size_t(DIGITS[i])] = uint8_t(i); if (i >= 10) { v[size_t(DIGITS[i] + 32)] = uint8_t(i); } }
        return v;}();
    static constexpr array<uint, 10000> ITOS4 = []() {
        array<uint, 10000> v{};
        for (int i = 0; i < 10000; ++i) { v[size_t(i)] = uint(0x30303030 + i / 1000 + (i / 100 % 10 << 8) + (i / 10 % 10 << 16) + (i % 10 << 24)); }
        return v;}();
    template<typename T> static constexpr bool isScalar = std::is_integral_v<T> && sizeof(T) <= 8 && !std::same_as<std::remove_cv_t<T>, bool>;
    template<typename T> static constexpr bool isIntegral = std::is_integral_v<T> || std::same_as<T, lll> || std::same_as<T, ulll>;

    int sgn = 1; bool is_inf = false;
    Words n;
    InfInt(const InfInt &) = default;
    InfInt &operator=(const InfInt &) = default;
    InfInt(InfInt &&o) noexcept : sgn(o.sgn), is_inf(o.is_inf), n(std::move(o.n)) { o.sgn = 1; o.is_inf = false; }   // a moved-from value is zero
    InfInt &operator=(InfInt &&o) noexcept {
        if (this != &o) { sgn = o.sgn; is_inf = o.is_inf; n = std::move(o.n); o.sgn = 1; o.is_inf = false; }
        return *this;}

    InfInt() {}
    template<typename T> requires isIntegral<T> InfInt(T x) { *this = x; }
    InfInt(string_view s, uint base = 10) { bool ok = read(s, base); assert(ok); (void)ok; }
    // T: O(M(n) * log(n)), M: O(n); digits most significant first, each below base in [2, 2^32); sign -1 or 1.
    InfInt(const vector<uint> &digits, uint base = 10, int sign = 1) {
        assert(base >= 2 && (sign == 1 || sign == -1) && std::ranges::all_of(digits, [&](uint d) { return d < base; }));
        fromDigits(int(digits.size()), base, [&](int i) { return digits[size_t(i)]; });
        trim(); sgn = isNil() ? 1 : sign;}
    template<int N> InfInt(const UInt<N> &u) { n.assign(u.w.data(), N); trim(); }
    // T: O(n), M: O(n); little-endian magnitude limbs (base 2^32 or 2^64) with a sign.
    static InfInt fromLimbs(const vector<uint> &limbs, int sign = 1) {
        assert(sign == 1 || sign == -1);
        InfInt r; r.n.setSize(int(limbs.size() + 1) / 2);
        for (int i = 0; i < r.n.size(); ++i) { r.n[i] = limbs[size_t(2 * i)] | (size_t(2 * i) + 1 < limbs.size() ? W(limbs[size_t(2 * i) + 1]) << 32 : 0); }
        r.trim(); r.sgn = r.isNil() ? 1 : sign;
        return r;}
    static InfInt fromLimbs(const vector<ulng> &limbs, int sign = 1) {
        assert(sign == 1 || sign == -1);
        InfInt r; r.n.assign(limbs.data(), int(limbs.size())); r.trim(); r.sgn = r.isNil() ? 1 : sign;
        return r;}
    // T: O(b / w), M: O(b / w); the integer part of a non-NaN double, infinities to the sentinel.
    static InfInt fromDouble(double d) {
        assert(!std::isnan(d));
        InfInt r;
        if (std::isinf(d)) { r.is_inf = true; r.sgn = d < 0 ? -1 : 1; return r; }
        d = std::trunc(d);
        if (d == 0) { return r; }
        int e; double f = std::frexp(std::fabs(d), &e);
        r = ulng(std::ldexp(f, 53));
        if (e >= 53) { r <<= e - 53; }
        else { r >>= 53 - e; }
        r.sgn = d < 0 ? -1 : 1;
        return r;}
    template<typename R> static ulng draw64(R &rng) {
        static_assert(R::min() == 0 && (R::max() == 0xFFFFFFFFu || R::max() == ~ulng(0)), "sample needs a 32- or 64-bit full-range generator");
        if constexpr (R::max() == ~ulng(0)) { return ulng(rng()); }
        else { return (ulng(rng()) << 32) | ulng(rng()); }}
    template<typename R> static InfInt randomBits(int bits, R &rng) {
        InfInt r;
        if (bits <= 0) { return r; }
        r.n.setSize((bits + 63) / 64);
        for (W &x : r.n) { x = draw64(rng); }
        r.n.back() &= (W(2) << ((bits - 1) % 64)) - 1;
        return r;}
    // T: O(b / w) expected, M: O(b / w); uniform on [0, bound) for finite bound > 0 (rejection sampling) and a value of exactly bits bits; rng is
    // std::mt19937, std::mt19937_64 or another generator with min() 0 and max() 2^32 - 1 or 2^64 - 1.
    template<typename R> friend InfInt sample(const InfInt &bound, R &rng) {
        assert(!bound.is_inf && bound.sgn == 1 && !bound.isNil());
        if (bound.is_inf || bound.sgn == -1 || bound.isNil()) { return 0; }
        int bits = bitLength(bound);
        while (true) {
            InfInt r = randomBits(bits, rng); r.trim();
            if (r < bound) { return r; }}}
    template<typename R> static InfInt sampleBits(int bits, R &rng) {
        InfInt r = randomBits(bits, rng);
        if (bits > 0) { r.n.back() |= W(1) << ((bits - 1) % 64); }
        return r;}
    template<typename T> requires isIntegral<T> InfInt &operator=(T x) {
        is_inf = false; sgn = 1; ulll u = ulll(x);
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) { if (x < 0) { sgn = -1; u = ulll(0) - u; } }
        n.setSize(1); n[0] = W(u);
        if (W hi = W(u >> 64)) { n.push_back(hi); }
        return *this;}

    // T: O(1), M: O(1)
    bool isNil() const { return !is_inf && (n.empty() || (n.size() == 1 && n[0] == 0)); }
    explicit operator bool() const { return !isNil(); }
    void trim() {
        if (is_inf) { n.clear(); return; }
        n.setSize(infint_detail::norm(n.data(), n.size()));
        if (n.empty()) { n.push_back(0); sgn = 1; }}
    ulng low64() const { return n.empty() ? 0 : n[0]; }
    // Bits [i, i + 64) of the magnitude.
    ulng bitsFrom(int i) const {
        int q = i / 64, r = i % 64;
        if (q >= n.size()) { return 0; }
        return (n[q] >> r) | (r && q + 1 < n.size() ? n[q + 1] << (64 - r) : 0);}
    // The magnitude's low k bits as a value.
    InfInt lowBits(int k) const {
        InfInt r; int len = min(n.size(), (k + 63) / 64);
        r.n.assign(n.data(), len);
        if (k % 64 && len == (k + 63) / 64) { r.n.back() &= (W(1) << (k % 64)) - 1; }
        r.trim();
        return r;}
    bool fitsWord() const { return n.size() <= 1; }
    // T: O(1), M: O(1); finite values convert modulo 2^64 (two's complement), infinity saturates.
    explicit operator lng() const {
        if (is_inf) { return sgn == 1 ? LLONG_MAX : LLONG_MIN; }
        return lng(sgn == 1 ? low64() : ulng(0) - low64());}
    explicit operator ulng() const {
        if (is_inf) { return sgn == 1 ? ULLONG_MAX : 0; }
        return sgn == 1 ? low64() : ulng(0) - low64();}
    // T: O(1), M: O(1); true iff the value is representable in T (integral through 128 bits), then stores it.
    template<typename T> requires isIntegral<T> bool tryCast(T &out) const {
        if (is_inf || n.size() > 2) { return false; }
        ulll mag = n.size() == 2 ? (ulll(n[1]) << 64) | n[0] : low64();
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) {
            ulll lim = ulll(std::numeric_limits<T>::max());
            if (mag > lim + (sgn == -1)) { return false; }
            out = sgn == -1 ? T(-lll(mag - 1) - 1) : T(mag);}
        else {
            if ((sgn == -1 && mag) || mag > ulll(std::numeric_limits<T>::max())) { return false; }
            out = T(mag);}
        return true;}
    // T: O(n), M: O(1); correctly rounded (nearest even) conversion, overflow and the sentinel give +-infinity.
    double toDouble() const {
        if (is_inf) { return sgn * std::numeric_limits<double>::infinity(); }
        if (isNil()) { return 0.0; }
        int shift = max(0, bitLength(*this) - 64);
        ulng m = bitsFrom(shift);
        if (shift) {
            bool sticky = shift % 64 && (n[shift / 64] & ((W(1) << (shift % 64)) - 1));
            for (int i = 0; i < shift / 64 && !sticky; ++i) { sticky = n[i] != 0; }
            m |= sticky;}
        return sgn * std::ldexp(double(m), shift);}
    template<typename T = uint> vector<T> toLimbs() const {
        assert(!is_inf);
        if constexpr (std::same_as<T, uint>) {
            vector<uint> v(2 * size_t(n.size()));
            if (!n.empty()) { std::memcpy(v.data(), n.data(), 8 * size_t(n.size())); }
            while (v.size() > 1 && v.back() == 0) { v.pop_back(); }
            return v.empty() ? vector<uint>{0} : v;}
        else {
            static_assert(std::same_as<T, ulng>);
            return n.empty() ? vector<ulng>{0} : vector<ulng>(n.begin(), n.end());}}

    // S: O(M(n) * log(n)) powers for n words, U: NA, Q: O(1), M: O(n); radix conversion: blocks of DIGITS digits in base BP = base^DIGITS, cached powers BP^(2^j), each from CACHED_DIV words a prepared Divisor (blk = m) and from FFT words a spectrum for the parse;
    // for the scaled remainder tree per level the split's cyclic plan with the kept spectrum of the power below and per top level the inverse of its power with two spectra.
    struct Radix {
        W bp; int digits; vector<vector<W>> pow, ctx, spec; vector<infint_detail::Divisor> div; vector<infint_detail::Spectrum> mulSpec;
        struct Tree { int frac = 0; infint_detail::CyclicPlan plan{}; infint_detail::Spectrum spec; vector<W> buf; };
        struct Root { int frac = 0; infint_detail::FftPlan plan{}; infint_detail::CyclicPlan rem{}; infint_detail::Spectrum inv, mod; vector<W> buf; };
        vector<Tree> split; vector<Root> root;
    };
    static Radix &radix(uint base) {
        static unordered_map<uint, Radix> cache;
        Radix &r = cache[max(base, 2u)];
        if (r.digits == 0) {
            base = max(base, 2u); r.bp = base; r.digits = 1;
            while (r.bp <= ~W(0) / base) { r.bp *= base; ++r.digits; }
            r.pow.push_back({r.bp});}
        return r;}
    static const vector<W> &radixPow(Radix &r, int j) {
        while (int(r.pow.size()) <= j) {
            const vector<W> &p = r.pow.back(); vector<W> sq(2 * p.size());
            infint_detail::square(sq.data(), p.data(), int(p.size()));
            sq.resize(size_t(infint_detail::norm(sq.data(), int(sq.size()))));
            r.pow.push_back(std::move(sq));}
        return r.pow[size_t(j)];}
    // The level-j power as a prepared divisor (null below CACHED_DIV words) and as a spectrum for the parse's products (null below FFT words), both kept for the process.
    static const infint_detail::Divisor *radixDiv(Radix &r, int j) {
        const vector<W> &p = radixPow(r, j); int m = int(p.size());
        if (m < infint_detail::CACHED_DIV) { return nullptr; }
        if (int(r.div.size()) <= j) { r.div.resize(size_t(j) + 1); r.ctx.resize(size_t(j) + 1); }
        if (!r.div[size_t(j)].d) {
            r.ctx[size_t(j)].resize(infint_detail::Divisor::words(m, m));
            r.div[size_t(j)].prepare(r.ctx[size_t(j)].data(), p.data(), m, m);}
        return &r.div[size_t(j)];}
    static const infint_detail::Spectrum *radixSpec(Radix &r, int j) {
        const vector<W> &p = radixPow(r, j); int m = int(p.size());
        infint_detail::FftPlan q = m >= infint_detail::FFT ? infint_detail::fftPlan(m, m, 2 * m, false) : infint_detail::FftPlan{0, 0, 0, 0};
        if (!q.lg) { return nullptr; }
        if (int(r.mulSpec.size()) <= j) { r.mulSpec.resize(size_t(j) + 1); r.spec.resize(size_t(j) + 1); }
        infint_detail::Spectrum &s = r.mulSpec[size_t(j)];
        if (!s.re) { r.spec[size_t(j)].resize(size_t(infint_detail::Spectrum::words(q))); s.take(r.spec[size_t(j)].data(), q, p.data(), m, m); }
        return &s;}
    // T: O(M(n) * log(n)), M: O(n); little-endian blocks to words at out (room for cnt + 1), returns the length.
    static int fromBlocks(W *out, const W *blk, int cnt, Radix &r) {
        if (cnt <= infint_detail::RADIX) {
            int len = 0;
            for (int i = cnt - 1; i >= 0; --i) {
                W c = infint_detail::mulWord(out, out, len, r.bp);
                if (c) { out[len++] = c; }
                c = infint_detail::addWord(out, len, blk[i]);
                if (c) { out[len++] = c; }}
            return len;}
        int j = 31 - std::countl_zero(uint(cnt - 1)), h = 1 << j;
        infint_detail::Scratch s(size_t(cnt) + 2);
        W *lo = s, *hi = lo + h + 1;
        int ll = fromBlocks(lo, blk, h, r), lh = fromBlocks(hi, blk + h, cnt - h, r);
        const vector<W> &p = radixPow(r, j); int m = int(p.size());
        const infint_detail::Spectrum *sp = lh && infint_detail::transformWins(lh, m, 2 * m, true) ? radixSpec(r, j) : nullptr;
        infint_detail::zero(out, cnt + 1);
        if (sp) { infint_detail::fftWith(out, *sp, hi, lh, false); }
        else if (lh) { infint_detail::mulAny(out, hi, lh, p.data(), m); }
        infint_detail::addInto(out, cnt + 1, lo, ll);
        return infint_detail::norm(out, cnt + 1);}
    // T: O(M(n) * log(n)), M: O(n); the len words at v (destroyed) to little-endian blocks appended at out + cnt, padded to 2^(j + 1) blocks when pad.
    static void toBlocks(W *out, int &cnt, W *v, int len, int j, bool pad, Radix &r) {
        len = infint_detail::norm(v, len);
        if (j < 0 || len <= infint_detail::RADIX) {
            int start = cnt;
            while (len) { out[cnt++] = infint_detail::divWord(v, v, len, r.bp); len = infint_detail::norm(v, len); }
            if (pad) { int end = start + (2 << max(j, 0)); infint_detail::zero(out + cnt, end - cnt); cnt = end; }
            return;}
        const vector<W> &p = radixPow(r, j); int m = int(p.size()), lq = max(len - m + 1, 1);
        infint_detail::Scratch s(size_t(lq) + m);
        W *q = s, *rem = q + lq;
        if (len < m) { q[0] = 0; std::copy(v, v + len, rem); infint_detail::zero(rem + len, m - len); }
        else { infint_detail::divRem(q, rem, v, len, p.data(), m, radixDiv(r, j)); }
        toBlocks(out, cnt, rem, m, j - 1, true, r);
        toBlocks(out, cnt, q, lq, j - 1, pad, r);}
    // The fraction words of a level-i node of the scaled remainder tree: the leaf power's words plus SCALED_GUARD, plus the words of every power from level SCALED_LEAF below i, so that a split by p_(i-1) scales the error by less than one.
    static int radixFrac(Radix &r, int i) {
        int f = int(radixPow(r, infint_detail::SCALED_LEAF).size()) + infint_detail::SCALED_GUARD;
        for (int k = infint_detail::SCALED_LEAF; k < i; ++k) { f += int(radixPow(r, k).size()); }
        return f;}
    // Level i > SCALED_LEAF of the tree: the cyclic plan (len > frac(i)) of a fraction times p_(i-1) and, with an FFT plan, the kept spectrum of p_(i-1); kept for the process.
    static Radix::Tree &radixSplit(Radix &r, int i) {
        if (int(r.split.size()) <= i) { r.split.resize(size_t(i) + 1); }
        Radix::Tree &t = r.split[size_t(i)];
        if (!t.frac) {
            const vector<W> &p = radixPow(r, i - 1); int m = int(p.size());
            t.frac = radixFrac(r, i); t.plan = infint_detail::cyclicPlan(t.frac, m, t.frac + 1);
            if (t.plan.p.lg) { t.buf.resize(size_t(infint_detail::Spectrum::words(t.plan.p))); t.spec.take(t.buf.data(), t.plan.p, p.data(), m, t.frac); }}
        return t;}
    // The top level j of a tree: buf starts with inv (frac(j) + 2 words), the reciprocal of p_j (m words) normalized and padded to frac(j) + 1 words, shifted back, so inv is within 2^64 below B^(m + frac(j) + 1) / p_j,
    // then its spectrum for operands of up to m + 1 words (plan, when an FFT plan exists) and the cyclic spectrum of p_j for remainders (rem, len > m); kept for the process.
    static Radix::Root &radixRoot(Radix &r, int j) {
        if (int(r.root.size()) <= j) { r.root.resize(size_t(j) + 1); }
        Radix::Root &t = r.root[size_t(j)];
        if (!t.frac) {
            const vector<W> &p = radixPow(r, j); int m = int(p.size()), f = radixFrac(r, j), k = f + 1, s = std::countl_zero(p.back());
            infint_detail::Scratch d(2 * size_t(k) + 1);
            W *x = d + k;
            infint_detail::zero(d, k - m);
            if (s) { infint_detail::shl(d + (k - m), p.data(), m, s); } else { std::copy(p.begin(), p.end(), d.p + (k - m)); }
            infint_detail::invert(x, d, k); x[k] = 1;
            t.frac = f; t.plan = infint_detail::fftPlan(m + 1, k + 1, m + k + 2, false); t.rem = infint_detail::cyclicPlan(m, m, m + 1);
            t.buf.resize(size_t(k) + 1 + size_t(infint_detail::Spectrum::words(t.plan)) + size_t(infint_detail::Spectrum::words(t.rem.p)));
            if (s) { infint_detail::shl(t.buf.data(), x, k + 1, s); } else { std::copy(x, x + k + 1, t.buf.data()); }
            W *b = t.buf.data() + k + 1;
            if (t.plan.lg) { t.inv.take(b, t.plan, t.buf.data(), k + 1, m + 1); b += infint_detail::Spectrum::words(t.plan); }
            if (t.rem.p.lg) { t.mod.take(b, t.rem.p, p.data(), m, m); }}
        return t;}
    // T: O(M(L) log(L)) for L = 2^i blocks, M: O(1) beyond the caller's buffers; the 2^i blocks at out of the level-i fraction fr (frac(i) words of frac(N / (BP^offset p_i)), within 2 + top - i units either way): leaves
    // extract RADIX blocks as integer parts of x BP, ... and settle a residue within 2^-120 of an integer by the block below; a node splits into the low child's words [m, frac(i)) of F p_(i-1) mod B^len - 1 and the high child's top words of fr.
    static void scaledTree(W *out, const W *fr, int i, W *lo, W *prod, W below, Radix &r) {
        if (i == infint_detail::SCALED_LEAF) {
            int f = radixFrac(r, i);
            std::copy(fr, fr + f, prod);
            for (int k = infint_detail::RADIX - 1; k >= 0; --k) { out[k] = infint_detail::mulWord(prod, prod, f, r.bp); }
            W hi = prod[f - 1], next = prod[f - 2];
            if (hi == ~W(0) && next >= ~W(0) - 255 && below == 0) { for (int k = 0; k < infint_detail::RADIX && ++out[k] == r.bp; ++k) { out[k] = 0; } }
            else if (hi == 0 && next < 256 && below == r.bp - 1) { for (int k = 0; k < infint_detail::RADIX && out[k]-- == 0; ++k) { out[k] = r.bp - 1; } }
            return;}
        Radix::Tree &t = radixSplit(r, i); const vector<W> &p = radixPow(r, i - 1); int m = int(p.size()), half = 1 << (i - 1), fl = t.frac - m;
        if (t.plan.p.lg) { infint_detail::fftWith(prod, t.spec, fr, t.frac, true); } else { infint_detail::mulCyclic(prod, t.plan, fr, t.frac, p.data(), m); }
        std::copy(prod + m, prod + t.frac, lo);
        scaledTree(out, lo, i - 1, lo + fl, prod, below, r);
        scaledTree(out + half, fr + m, i - 1, lo + fl, prod, out[half - 1], r);}
    // T: O(M(n) log(n)), M: O(n); the 2^(j + 1) blocks at out of the len-word magnitude w in [p_j, p_(j + 1)), p_j of m words, by the scaled remainder tree (Bernstein): one Barrett split w = Q p_j + R, Q from the
    // top m + 1 words of w times inv (within two below, settled by the cyclic remainder), then the trees of R and Q from their fractions, the words [m + 1, m + 1 + frac(j)) of R inv and Q inv; a Q below p_(j-1) by the division tree.
    static void scaledBlocks(W *out, const W *w, int len, int j, Radix &r) {
        Radix::Root &rt = radixRoot(r, j); const vector<W> &p = radixPow(r, j);
        int m = int(p.size()), f = rt.frac, il = f + 2, nt = min(len, m + 1), k = len - nt, qn = len + 1 - m, qm = min(qn, m), rl = rt.rem.len, pw = m + il + 1;
        size_t lo = 0, pl = size_t(radixFrac(r, infint_detail::SCALED_LEAF));
        for (int i = infint_detail::SCALED_LEAF + 1; i <= j; ++i) { Radix::Tree &t = radixSplit(r, i); lo += size_t(t.frac - int(radixPow(r, i - 1).size())); pl = max(pl, size_t(t.plan.len)); }
        infint_detail::Scratch s(2 * size_t(pw) + size_t(m) + 1 + 2 * size_t(rl) + lo + pl);
        W *bq = s, *br = bq + pw, *q = br + pw, *res = q + m + 1, *prod = res + rl, *lob = prod + rl, *pb = lob + lo;
        auto times = [&](W *dst, const W *a, int n) {
            if (rt.plan.lg) { infint_detail::fftWith(dst, rt.inv, a, n, false); } else { infint_detail::mulAny(dst, a, n, rt.buf.data(), il); }
            infint_detail::zero(dst + n + il, pw - n - il);};
        times(bq, w + k, nt);
        std::copy(bq + (m + f + 1 - k), bq + (nt + il), q);
        infint_detail::foldMod1(res, rl, w, len);
        if (rt.rem.p.lg) { infint_detail::fftWith(prod, rt.mod, q, qm, true); infint_detail::canonical(prod, rl); } else { infint_detail::mulCyclic(prod, rt.rem, q, qm, p.data(), m); }
        infint_detail::subMod1(res, res, prod, rl);
        while (infint_detail::cmp(res, infint_detail::norm(res, rl), p.data(), m) >= 0) { infint_detail::subFrom(res, rl, p.data(), m); infint_detail::addWord(q, qn, 1); }
        times(br, res, m);
        scaledTree(out, br + m + 1, j, lob, pb, 0, r);
        const vector<W> &h = radixPow(r, j - 1);
        if (infint_detail::cmp(q, infint_detail::norm(q, qn), h.data(), int(h.size())) < 0) { int c = 0; toBlocks(out + (1 << j), c, q, qn, j - 1, true, r); return; }
        times(bq, q, qm);
        scaledTree(out + (1 << j), bq + m + 1, j, lob, pb, 0, r);}
    // Little-endian blocks of the len-word magnitude w into out (room for 4 len + 4); returns the count without leading zero blocks: the scaled remainder tree from SCALED_TREE words, else the division tree.
    static int toBlocks(W *out, const W *w, int len, Radix &r) {
        infint_detail::Scratch v(size_t(max(len, 1)));
        std::copy(w, w + len, v.p);
        len = infint_detail::norm(v, len);
        int j = -1, cnt = 0;
        if (len > infint_detail::RADIX) {
            while (true) {
                const vector<W> &p = radixPow(r, j + 1);
                if (infint_detail::cmp(v, len, p.data(), int(p.size())) < 0) { break; }
                ++j;}}
        if (len >= infint_detail::SCALED_TREE && j >= infint_detail::SCALED_LEAF) { scaledBlocks(out, v, len, j, r); cnt = 2 << j; }
        else { toBlocks(out, cnt, v, len, j, false, r); }
        while (cnt && out[cnt - 1] == 0) { --cnt; }
        return cnt;}
    template<typename G> void fromDigits(int len, uint base, const G &get) {
        Radix &r = radix(base);
        int cnt = (len + r.digits - 1) / r.digits;
        infint_detail::Scratch blk(size_t(max(cnt, 1)));
        for (int b = 0; b < cnt; ++b) {
            int hi = len - b * r.digits, lo = max(hi - r.digits, 0); W v = 0;
            for (int i = lo; i < hi; ++i) { v = v * base + get(i); }
            blk[b] = v;}
        n.setSize(cnt + 1);
        n.setSize(fromBlocks(n.data(), blk, cnt, r));}
    static ulng parse8(const char *p) {
        ulng v; std::memcpy(&v, p, 8);
        v = (v & 0x0F0F0F0F0F0F0F0FULL) * 2561 >> 8;
        v = (v & 0x00FF00FF00FF00FFULL) * 6553601 >> 16;
        return (v & 0x0000FFFF0000FFFFULL) * 42949672960001ULL >> 32;}
    // All eight bytes are hex digits: none above 0x7F and each '0'-'9' or, with bit 5 set, 'a'-'f'.
    static bool hexWord(ulng w) {
        constexpr ulng H = 0x8080808080808080ULL, L = 0x0101010101010101ULL;
        auto ge = [](ulng x, ulng k) { return ((x | H) - k * L) & H; };   // top bit of each byte: byte >= k (bytes below 0x80)
        ulng f = w | 0x2020202020202020ULL;
        return !(w & H) && ((ge(w, '0') & ~ge(w, '9' + 1)) | (ge(f, 'a') & ~ge(f, 'f' + 1))) == H;}
    // T: O(L), M: O(1); the words of a digit string in base 2^bits into out; false when a character is not a digit of the base.
    static bool parsePow2(Words &out, string_view s, int bits) {
        int len = int(s.size());
        out.assign(int(size_t(len) * size_t(bits) / 64) + 1, 0);
        W *w = out.data();
        int i = 0;
        if (bits == 4) {
#ifdef __AVX2__
            __m256i m0f = _mm256_set1_epi8(0x0F), v9 = _mm256_set1_epi8('9'), w16 = _mm256_set1_epi16(0x0110), w256 = _mm256_set1_epi32(0x00010100);
            __m256i rev = _mm256_setr_epi32(6, 4, 2, 0, 0, 0, 0, 0), ok = _mm256_set1_epi8(-1), zero = _mm256_setzero_si256();
            for (; i + 32 <= len; i += 32) {
                __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(s.data() + len - i - 32));
                __m256i dig = _mm256_cmpeq_epi8(_mm256_subs_epu8(_mm256_sub_epi8(x, _mm256_set1_epi8('0')), _mm256_set1_epi8(9)), zero);
                __m256i alp = _mm256_cmpeq_epi8(_mm256_subs_epu8(_mm256_sub_epi8(_mm256_or_si256(x, _mm256_set1_epi8(0x20)), _mm256_set1_epi8('a')), _mm256_set1_epi8(5)), zero);
                ok = _mm256_and_si256(ok, _mm256_or_si256(dig, alp));
                x = _mm256_add_epi8(_mm256_and_si256(x, m0f), _mm256_and_si256(_mm256_cmpgt_epi8(x, v9), _mm256_set1_epi8(9)));
                x = _mm256_madd_epi16(_mm256_maddubs_epi16(x, w16), w256);
                __m256i y = _mm256_permutevar8x32_epi32(_mm256_or_si256(_mm256_slli_epi32(x, 16), _mm256_srli_epi64(x, 32)), rev);
                _mm_storeu_si128(reinterpret_cast<__m128i *>(w + i / 16), _mm256_castsi256_si128(y));}
            if (_mm256_movemask_epi8(ok) != -1) { return false; }
#endif
            for (; i + 8 <= len; i += 8) {
                ulng cur; std::memcpy(&cur, s.data() + len - i - 8, 8);
                if (!hexWord(cur)) { return false; }
                cur = (cur & 0x0F0F0F0F0F0F0F0FULL) + 9 * ((cur & 0x4040404040404040ULL) >> 6);
                cur = ((cur << 4) | (cur >> 8)) & 0x00FF00FF00FF00FFULL;
                cur = ((cur << 8) | (cur >> 16)) & 0x0000FFFF0000FFFFULL;
                w[i / 16] |= W(uint((cur << 16) | (cur >> 32))) << (32 * (i / 8 % 2));}}
        for (; i < len; ++i) {
            uint d = DIGIT_VALUE[size_t(uint8_t(s[size_t(len - i - 1)]))]; int bit = i * bits;
            if (d >> bits) { return false; }
            w[bit / 64] |= W(d) << (bit % 64);
            if (bit % 64 + bits > 64) { w[bit / 64 + 1] |= W(d) >> (64 - bit % 64); }}
        return true;}
    // T: O(L) for power-of-two bases, O(M(L) * log(L)) otherwise, M: O(L / w); optional sign, digits of base 2-36 (either case) or, in bases up
    // to 23, "inf"; false and no change when malformed.
    bool read(string_view s, uint base = 10) {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        int sign = 1;
        if (!s.empty() && (s[0] == '+' || s[0] == '-')) { sign = s[0] == '-' ? -1 : 1; s.remove_prefix(1); }
        if (s.empty()) { return false; }
        if (base <= 23 && s.size() == 3 && (s[0] | 32) == 'i' && (s[1] | 32) == 'n' && (s[2] | 32) == 'f') { sgn = sign; is_inf = true; n.clear(); return true; }
        while (s.size() > 1 && s[0] == '0') { s.remove_prefix(1); }
        int len = int(s.size());
        if (std::has_single_bit(base)) {
            Words v;
            if (!parsePow2(v, s, std::countr_zero(base))) { return false; }
            is_inf = false; n = std::move(v);}
        else {
            if (std::any_of(s.begin(), s.end(), [&](char c) { return DIGIT_VALUE[size_t(uint8_t(c))] >= base; })) { return false; }
            is_inf = false;
            if (len <= 12 || (base == 10 && len <= 19)) {   // one word
                W v = 0;
                for (char c : s) { v = v * base + DIGIT_VALUE[size_t(uint8_t(c))]; }
                n.setSize(1); n[0] = v;}
            else if (base == 10) {
                Radix &r = radix(10); int cnt = (len + 18) / 19;
                infint_detail::Scratch blk(size_t(cnt) + 0);
                for (int b = 0; b + 1 < cnt; ++b) {
                    const char *p = s.data() + len - 19 * (b + 1);
                    blk[b] = (parse8(p) * 100000000 + parse8(p + 8)) * 1000 + ulng(p[16] - '0') * 100 + ulng(p[17] - '0') * 10 + ulng(p[18] - '0');}
                W top = 0;
                for (int i = 0; i < len - 19 * (cnt - 1); ++i) { top = top * 10 + ulng(s[size_t(i)] - '0'); }
                blk[cnt - 1] = top;
                n.setSize(cnt + 1);
                n.setSize(fromBlocks(n.data(), blk, cnt, r));}
            else { fromDigits(len, base, [&](int i) { return uint(DIGIT_VALUE[size_t(uint8_t(s[size_t(i)]))]); }); }}
        trim(); sgn = isNil() ? 1 : sign;
        return true;}
    // The cnt digits of the magnitude w in base 2^bits, most significant first, at out.
    static void writePow2(char *out, int cnt, const W *w, int len, int bits, bool lower) {
        const char *dig = lower ? DIGITS_LOWER : DIGITS;
        int i = 0;
        if (bits == 4) {
            char *o = out + cnt, alpha = lower ? 39 : 7;   // 'a' - '9' - 1 or 'A' - '9' - 1
#ifdef __AVX2__
            __m256i m16 = _mm256_set1_epi64x(0x0000FFFF0000FFFFLL), m8 = _mm256_set1_epi64x(0x00FF00FF00FF00FFLL), m4 = _mm256_set1_epi64x(0x0F0F0F0F0F0F0F0FLL);
            __m256i rev = _mm256_setr_epi8(15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0);
            for (; i + 32 <= cnt; i += 32) {
                __m256i cur = _mm256_cvtepu32_epi64(_mm_loadu_si128(reinterpret_cast<const __m128i *>(w + i / 16)));
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 16)), m16);
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 8)), m8);
                cur = _mm256_and_si256(_mm256_or_si256(cur, _mm256_slli_epi64(cur, 4)), m4);
                __m256i alp = _mm256_cmpgt_epi8(cur, _mm256_set1_epi8(9));
                cur = _mm256_add_epi8(_mm256_add_epi8(cur, _mm256_set1_epi8('0')), _mm256_and_si256(alp, _mm256_set1_epi8(alpha)));
                cur = _mm256_shuffle_epi8(cur, rev); cur = _mm256_permute2x128_si256(cur, cur, 1);
                o -= 32; _mm256_storeu_si256(reinterpret_cast<__m256i *>(o), cur);}
#endif
            for (; i + 8 <= cnt; i += 8) {
                ulng cur = uint(w[i / 16] >> (32 * (i / 8 % 2)));
                cur = (cur | (cur << 16)) & 0x0000FFFF0000FFFFULL;
                cur = (cur | (cur << 8)) & 0x00FF00FF00FF00FFULL;
                cur = (cur | (cur << 4)) & 0x0F0F0F0F0F0F0F0FULL;
                cur = __builtin_bswap64((cur + 0x3030303030303030ULL) + ulng(alpha) * (((cur + 0x3636363636363636ULL) & 0x4040404040404040ULL) >> 6));
                o -= 8; std::memcpy(o, &cur, 8);}}
        for (; i < cnt; ++i) {
            int bit = i * bits; W d = w[bit / 64] >> (bit % 64);
            if (bit % 64 + bits > 64 && bit / 64 + 1 < len) { d |= w[bit / 64 + 1] << (64 - bit % 64); }
            out[cnt - 1 - i] = dig[d & ((W(1) << bits) - 1)];}}
    // T: O(1), M: O(1); an upper bound on the length toChars writes in base 2-36, sign included.
    int charsBound(uint base = 10) const {
        assert(base >= 2 && base <= 36);
        if (is_inf) { return 4; }
        int bits = max(bitLength(*this), 1), k = std::bit_width(std::clamp(base, 2u, 36u)) - 1;
        return (bits + k - 1) / k + 1;}
    // T: O(L) for power-of-two bases and one-word values, O(M(L) * log(L)) otherwise, M: O(L / w); the text of toString into [first, last): the end
    // pointer, or last with errc::value_too_large and nothing written when the room is short.
    std::to_chars_result toChars(char *first, char *last, uint base = 10, bool lower = false) const {
        assert(base >= 2 && base <= 36);
        base = std::clamp(base, 2u, 36u);
        auto put = [&](const char *s, int len) -> std::to_chars_result {
            if (last - first < len) { return {last, std::errc::value_too_large}; }
            std::memcpy(first, s, size_t(len));
            return {first + len, std::errc()};};
        if (is_inf) { return sgn == -1 ? put("-inf", 4) : put("inf", 3); }
        if (isNil()) { return put("0", 1); }
        int neg = sgn == -1;
        const char *dig = lower ? DIGITS_LOWER : DIGITS;
        if (std::has_single_bit(base)) {
            int b = std::countr_zero(base), cnt = (bitLength(*this) + b - 1) / b;
            if (last - first < cnt + neg) { return {last, std::errc::value_too_large}; }
            if (neg) { *first++ = '-'; }
            writePow2(first, cnt, n.data(), n.size(), b, lower);
            return {first + cnt, std::errc()};}
        if (n.size() == 1) {   // one word
            char buf[65]; W v = n[0]; int len = 0;
            buf[0] = '-';
            if (base == 10) { len = int(std::to_chars(buf + 1, buf + 25, v).ptr - buf - 1); }
            else { for (; v; v /= base) { buf[1 + len++] = dig[v % base]; } std::reverse(buf + 1, buf + 1 + len); }
            return put(buf + 1 - neg, len + neg);}
        Radix &r = radix(base);
        infint_detail::Scratch blk(4 * size_t(n.size()) + 4);
        int cnt = toBlocks(blk, n.data(), n.size(), r), top = 0;
        for (W v = blk[cnt - 1]; v; v /= base) { ++top; }
        int len = top + (cnt - 1) * r.digits;
        if (last - first < len + neg) { return {last, std::errc::value_too_large}; }
        if (neg) { *first++ = '-'; }
        char *end = first + len;
        if (base == 10) {
            char *o = std::to_chars(first, end, blk[cnt - 1]).ptr;
            for (int b = cnt - 2; b >= 0; --b, o += 19) {
                W v = blk[b];
                for (int g = 3; g >= 0; --g) { std::memcpy(o + 3 + 4 * g, &ITOS4[size_t(v % 10000)], 4); v /= 10000; }
                o[0] = char('0' + v / 100); o[1] = char('0' + v / 10 % 10); o[2] = char('0' + v % 10);}
            return {end, std::errc()};}
        char *o = end;
        for (int b = 0; b + 1 < cnt; ++b) {
            W v = blk[b];
            for (int d = 0; d < r.digits; ++d) { *--o = dig[v % base]; v /= base; }}
        for (W v = blk[cnt - 1]; v; v /= base) { *--o = dig[v % base]; }
        return {end, std::errc()};}
    // T: as toChars, M: O(L); uppercase digits unless lower, "-" for negatives, "0" for zero, "inf" / "-inf" for the sentinel.
    string toString(uint base = 10, bool lower = false) const {
        string out(size_t(charsBound(base)), '\0');
        out.resize(size_t(toChars(out.data(), out.data() + out.size(), base, lower).ptr - out.data()));
        return out;}
    // T: O(M(L) * log(L)), M: O(n + out); digits most significant first in base [2, 2^32), {0} for zero.
    vector<uint> toDigits(uint base) const {
        assert(!is_inf && base >= 2);
        base = max(base, 2u);
        if (isNil()) { return {0}; }
        Radix &r = radix(base);
        infint_detail::Scratch blk(4 * size_t(n.size()) + 4);
        int cnt = toBlocks(blk, n.data(), n.size(), r);
        vector<uint> out(size_t(cnt) * size_t(r.digits));
        size_t o = out.size();
        for (int b = 0; b < cnt; ++b) { W v = blk[b]; for (int d = 0; d < r.digits; ++d) { out[--o] = uint(v % base); v /= base; } }
        size_t z = 0;
        while (z + 1 < out.size() && out[z] == 0) { ++z; }
        out.erase(out.begin(), out.begin() + ptrdiff_t(z));
        return out;}
    // Magnitude helpers on words: |this| += |o|, |this| -= |o| (|this| >= |o|), |this| = |o| - |this| (|o| > |this|); o may be *this.
    void addMag(const InfInt &o) {
        int k = max(n.size(), o.n.size());
        if (n.cap <= k) { Words r; r.setSize(k + 1); r[k] = addInto(r.data(), *this, o); if (!r[k]) { r.pop_back(); } n = std::move(r); return; }
        if (n.size() < k) { n.resize(k); }
        W c = infint_detail::add(n.data(), n.data(), k, o.n.data(), o.n.size());
        if (c) { n.push_back(c); }}
    // r (k = max words) = |a| + |b|, returns the carry.
    static W addInto(W *r, const InfInt &a, const InfInt &b) {
        const InfInt &x = a.n.size() >= b.n.size() ? a : b, &y = a.n.size() >= b.n.size() ? b : a;
        return infint_detail::add(r, x.n.data(), x.n.size(), y.n.data(), y.n.size());}
    // a + sb * |b| in one pass over the words; the sentinel and zero cases go through the compound forms.
    static InfInt combine(const InfInt &a, const InfInt &b, int sb) {
        if (a.is_inf || b.is_inf || a.isNil() || b.isNil()) { InfInt r = a; if (sb == b.sgn) { r += b; } else { r -= b; } return r; }
        InfInt r;
        if (a.sgn == sb) {
            int k = max(a.n.size(), b.n.size());
            r.n.setSize(k + 1); r.sgn = a.sgn;
            r.n[k] = addInto(r.n.data(), a, b);
            if (!r.n[k]) { r.n.pop_back(); }
            return r;}
        auto c = magCmp(a, b);
        if (c == 0) { return 0; }
        const InfInt &x = c > 0 ? a : b, &y = c > 0 ? b : a;
        r.n.setSize(x.n.size()); r.sgn = c > 0 ? a.sgn : sb;
        infint_detail::sub(r.n.data(), x.n.data(), x.n.size(), y.n.data(), y.n.size());
        r.trim();
        return r;}
    void subMag(const InfInt &o) { infint_detail::sub(n.data(), n.data(), n.size(), o.n.data(), o.n.size()); trim(); }
    void rsubMag(const InfInt &o) {
        int k = n.size(); n.resize(o.n.size());
        infint_detail::sub(n.data(), o.n.data(), o.n.size(), n.data(), k);
        trim();}
    // inf + x = inf, inf + (-inf) = 0.
    InfInt &operator+=(const InfInt &o) {
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn != o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { return *this = o; }
        if (sgn == o.sgn) { addMag(o); return *this; }
        auto c = magCmp(*this, o);
        if (c == 0) { return *this = 0; }
        if (c > 0) { subMag(o); }
        else { rsubMag(o); sgn = o.sgn; }
        return *this;}
    InfInt &operator-=(const InfInt &o) {
        if (this == &o) { return *this = 0; }
        if (is_inf || o.is_inf) {
            if (is_inf && o.is_inf && sgn == o.sgn) { return *this = 0; }
            if (o.is_inf) { sgn = -o.sgn; is_inf = true; }
            n.clear();
            return *this;}
        if (o.isNil()) { return *this; }
        if (isNil()) { *this = o; sgn = -sgn; trim(); return *this; }
        if (sgn != o.sgn) { addMag(o); return *this; }
        auto c = magCmp(*this, o);
        if (c == 0) { return *this = 0; }
        if (c > 0) { subMag(o); }
        else { rsubMag(o); sgn = -sgn; }
        return *this;}
    InfInt &operator++() {
        if (is_inf) { return *this; }
        if (sgn == -1) { sgn = 1; --*this; sgn = isNil() ? 1 : -1; return *this; }
        if (infint_detail::addWord(n.data(), n.size(), 1)) { n.push_back(1); }
        return *this;}
    InfInt &operator--() {
        if (is_inf) { return *this; }
        if (isNil()) { n.assign(1, 1); sgn = -1; return *this; }
        if (sgn == -1) { sgn = 1; ++*this; sgn = -1; return *this; }
        infint_detail::subWord(n.data(), n.size(), 1);
        trim();
        return *this;}
    InfInt operator++(int) { InfInt r = *this; ++*this; return r; }
    InfInt operator--(int) { InfInt r = *this; --*this; return r; }

    template<typename T> requires isScalar<T> static pair<int, ulng> sgnMag(T x) {
        if constexpr (std::is_signed_v<T>) { if (x < 0) { return {-1, ulng(0) - ulng(x)}; } }
        return {1, ulng(x)};}
    // T: O(n), M: O(n); inf * 0 = 0.
    template<typename T> requires isScalar<T> InfInt &operator*=(T o) {
        auto [os, u] = sgnMag(o);
        if (isNil() || u == 0) { return *this = 0; }
        sgn *= os;
        if (is_inf || u == 1) { return *this; }
        if (std::has_single_bit(u)) { return *this <<= std::countr_zero(u); }
        W c = infint_detail::mulWord(n.data(), n.data(), n.size(), u);
        if (c) { n.push_back(c); }
        return *this;}
    // T: O(M(n)), M: O(n); squares when both operands are the same object or equal.
    InfInt &operator*=(const InfInt &o) {
        if (isNil() || o.isNil()) { return *this = 0; }
        if (is_inf || o.is_inf) { sgn *= o.sgn; is_inf = true; n.clear(); return *this; }
        if (o.fitsWord()) { int os = o.sgn; *this *= o.n[0]; sgn *= os; return *this; }
        if (fitsWord()) { W u = n[0]; int s = sgn; *this = o; *this *= u; sgn *= s; return *this; }
        int s = sgn * o.sgn, k = n.size(), m = o.n.size();
        Words r; r.setSize(k + m);
        if (this == &o || n == o.n) { infint_detail::square(r.data(), n.data(), k); }
        else if (k >= m) { infint_detail::mul(r.data(), n.data(), k, o.n.data(), m); }
        else { infint_detail::mul(r.data(), o.n.data(), m, n.data(), k); }
        n = std::move(r); trim(); sgn = s;
        return *this;}
    // T: O(n), M: O(1); truncating; inf / x = inf, x / inf = 0.
    template<typename T> requires isScalar<T> InfInt &operator/=(T o) {
        auto [os, u] = sgnMag(o);
        assert(u != 0);
        if (is_inf) { sgn *= os; return *this; }
        if (isNil()) { return *this; }
        int s = sgn * os;
        if (std::has_single_bit(u)) { sgn = 1; *this >>= std::countr_zero(u); }
        else { infint_detail::divWord(n.data(), n.data(), n.size(), u); trim(); }
        sgn = isNil() ? 1 : s;
        return *this;}
    // T: O(n), M: O(1); the remainder keeps the dividend's sign; inf % x = 0, x % inf = x.
    template<typename T> requires isScalar<T> InfInt &operator%=(T o) {
        ulng u = sgnMag(o).second;
        assert(u != 0);
        if (is_inf || isNil()) { return *this = 0; }
        int s = sgn;
        W r = std::has_single_bit(u) ? n[0] & (u - 1) : infint_detail::remWord(n.data(), n.size(), u);
        *this = r; sgn = isNil() ? 1 : s;
        return *this;}
    // Signs of a truncating quotient and remainder, including the sentinel rules of / and %; false when the operands settle it.
    static bool divSentinel(const InfInt &a, const InfInt &b, pair<InfInt, InfInt> &out) {
        assert(!b.isNil());
        if (b.isNil()) { out = {0, a}; return true; }
        if (a.is_inf || b.is_inf) {
            if (a.is_inf && b.is_inf) { out = {a.sgn * b.sgn, 0}; }
            else if (b.is_inf) { out = {0, a}; }
            else { out = {a, 0}; out.first.sgn *= b.sgn; }
            return true;}
        auto c = magCmp(a, b);
        if (c < 0) { out = {0, a}; return true; }
        if (c == 0) { out = {a.sgn * b.sgn, 0}; return true; }
        return false;}
    // T: O((n - m) * m) word products, M: O(n); Knuth D only.
    friend pair<InfInt, InfInt> divModSlow(const InfInt &a, const InfInt &b) {
        pair<InfInt, InfInt> out;
        if (divSentinel(a, b, out)) { return out; }
        int qs = a.sgn * b.sgn, rs = a.sgn, k = a.n.size(), m = b.n.size();
        InfInt &q = out.first, &r = out.second;
        q.n.setSize(k - m + 1); r.n.setSize(m);
        if (m == 1) { r.n[0] = infint_detail::divWord(q.n.data(), a.n.data(), k, b.n[0]); }
        else {
            infint_detail::Scratch s(2 * size_t(k) + 3);
            W *x = s, *y = x + k + 1, *qt = y + m;
            int sh = std::countl_zero(b.n.back());
            if (sh) { x[k] = infint_detail::shl(x, a.n.data(), k, sh); infint_detail::shl(y, b.n.data(), m, sh); }
            else { std::copy(a.n.begin(), a.n.end(), x); x[k] = 0; std::copy(b.n.begin(), b.n.end(), y); }
            int na = x[k] ? k + 1 : k;
            infint_detail::divBasecase(qt, r.n.data(), x, na, y, m);
            std::copy(qt, qt + (k - m + 1), q.n.data());
            if (sh) { infint_detail::shr(r.n.data(), r.n.data(), m, sh); }}
        q.trim(); r.trim();
        if (!q.isNil()) { q.sgn = qs; }
        if (!r.isNil()) { r.sgn = rs; }
        return out;}
    // T: O(M(n) * log(n)), O(n) for a power-of-two divisor, M: O(n); truncating quotient and remainder (remainder keeps a's sign).
    friend pair<InfInt, InfInt> divMod(const InfInt &a, const InfInt &b) {
        pair<InfInt, InfInt> out;
        if (divSentinel(a, b, out)) { return out; }
        int qs = a.sgn * b.sgn, rs = a.sgn;
        InfInt &q = out.first, &r = out.second;
        if (a.fitsWord()) { q = a.n[0] / b.n[0]; r = a.n[0] % b.n[0]; }   // |b| < |a|, so b fits a word too
        else if (popcount(b) == 1) { int k = ctz(b); r = a.lowBits(k); q = a; q.sgn = 1; q >>= k; }
        else if (b.fitsWord()) { q.n.setSize(a.n.size()); r = infint_detail::divWord(q.n.data(), a.n.data(), a.n.size(), b.n[0]); q.trim(); }
        else {
            int k = a.n.size(), m = b.n.size();
            q.n.setSize(k - m + 1); r.n.setSize(m);
            infint_detail::divRem(q.n.data(), r.n.data(), a.n.data(), k, b.n.data(), m);
            q.trim(); r.trim();}
        if (!q.isNil()) { q.sgn = qs; }
        if (!r.isNil()) { r.sgn = rs; }
        return out;}
    InfInt &operator/=(const InfInt &o) {
        if (!o.is_inf && o.fitsWord()) { int os = o.sgn; *this /= o.low64(); sgn = isNil() ? 1 : sgn * os; return *this; }
        return *this = divMod(*this, o).first;}
    InfInt &operator%=(const InfInt &o) {
        if (!o.is_inf && o.fitsWord()) { return *this %= o.low64(); }
        return *this = divMod(*this, o).second;}

    InfInt operator+() const { return *this; }
    InfInt operator-() const & { InfInt r = *this; if (!r.isNil()) { r.sgn = -r.sgn; } return r; }
    InfInt operator-() && { if (!isNil()) { sgn = -sgn; } return std::move(*this); }
    friend InfInt operator+(const InfInt &a, const InfInt &b) { return combine(a, b, b.sgn); }
    friend InfInt operator-(const InfInt &a, const InfInt &b) { return combine(a, b, -b.sgn); }
    friend InfInt operator+(InfInt &&a, const InfInt &b) { a += b; return std::move(a); }
    friend InfInt operator-(InfInt &&a, const InfInt &b) { a -= b; return std::move(a); }
    friend InfInt operator*(InfInt a, const InfInt &b) { a *= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator*(InfInt a, T b) { a *= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator*(T b, InfInt a) { a *= b; return a; }
    friend InfInt operator/(InfInt a, const InfInt &b) { a /= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator/(InfInt a, T b) { a /= b; return a; }
    friend InfInt operator%(InfInt a, const InfInt &b) { a %= b; return a; }
    template<typename T> requires isScalar<T> friend InfInt operator%(InfInt a, T b) { a %= b; return a; }
    friend InfInt abs(InfInt a) { a.sgn = 1; return a; }
    // Rounded division modes on finite operands, b != 0: floor, ceiling and Euclidean (remainder in [0, |b|)).
    friend pair<InfInt, InfInt> divModFloor(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (!r.isNil() && r.sgn != b.sgn) { --q; r += b; }
        return {std::move(q), std::move(r)};}
    friend pair<InfInt, InfInt> divModCeil(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (!r.isNil() && r.sgn == b.sgn) { ++q; r -= b; }
        return {std::move(q), std::move(r)};}
    friend pair<InfInt, InfInt> divModEuclid(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf);
        auto [q, r] = divMod(a, b);
        if (r.sgn == -1) { if (b.sgn == 1) { --q; r += b; } else { ++q; r -= b; } }
        return {std::move(q), std::move(r)};}
    static ulng residue61(const W *w, int k) {
        constexpr ulng mod61 = (1ULL << 61) - 1; ulng r = 0;
        for (int i = k; i-- > 0;) {
            ulll t = (ulll(r) << 64) | w[i], f = (t & mod61) + (t >> 61);
            f = (f & mod61) + (f >> 61); r = ulng(f >= mod61 ? f - mod61 : f);}
        return r;}
    // T: O(k * m) word products for quotients of k <= HENSEL words, else O(M(n) * log(n)), M: O(n); exact quotient a / b for finite b | a, b != 0:
    // Hensel (2-adic) division, then the measured faster quotient-only Burnikel-Ziegler division; the assertion checks modulo 2^61 - 1.
    friend InfInt divExact(const InfInt &a, const InfInt &b) {
        assert(!a.is_inf && !b.is_inf && !b.isNil());
        if (a.isNil()) { return 0; }
        if (b.fitsWord()) { InfInt q = a; q /= b.low64(); q.sgn = q.isNil() ? 1 : a.sgn * b.sgn; return q; }
        int tz = ctz(b);
        InfInt x = abs(a), y = abs(b); x >>= tz; y >>= tz;
        int k = x.n.size(), m = y.n.size();
        if (k < m) { return 0; }
        InfInt r; r.n.setSize(k - m + 1);
        if (k - m + 1 <= infint_detail::HENSEL) { infint_detail::divExactHensel(r.n.data(), x.n.data(), k, y.n.data(), m); }
        else { infint_detail::divRem(r.n.data(), nullptr, x.n.data(), k, y.n.data(), m, nullptr, true); }
        assert(ulng(ulll(residue61(r.n.data(), r.n.size())) * residue61(y.n.data(), m) % ((1ULL << 61) - 1)) == residue61(x.n.data(), k));
        r.trim(); r.sgn = r.isNil() ? 1 : a.sgn * b.sgn;
        return r;}
    // T: O((k + 1) * M(n) * log(n)) for k removed factors, O(n) for a power-of-two f, M: O(n); divides x by f (|f| >= 2) while possible, returns the count; x finite nonzero.
    friend int removeFactor(InfInt &x, const InfInt &f) {
        assert(!x.is_inf && !f.is_inf && !x.isNil() && magCmp(f, 1) > 0);
        int cnt = 0;
        if (x.is_inf || f.is_inf || x.isNil() || magCmp(f, 1) <= 0) { return 0; }
        if (&x == &f) { x = 1; return 1; }
        if (f.fitsWord() && std::has_single_bit(f.low64())) {
            int k = std::countr_zero(f.low64()), t = ctz(x) / k;
            x >>= t * k; cnt = t;
            if (f.sgn == -1 && (t & 1)) { x.sgn = -x.sgn; }
            return cnt;}
        while (true) {
            auto [q, r] = divMod(x, f);
            if (!r.isNil()) { return cnt; }
            x = std::move(q); ++cnt;}}

    friend std::strong_ordering magCmp(const InfInt &a, const InfInt &b) {
        if (a.is_inf || b.is_inf) { return int(a.is_inf) <=> int(b.is_inf); }
        bool az = a.isNil(), bz = b.isNil();
        if (az || bz) { return int(!az) <=> int(!bz); }
        if (a.n.size() != b.n.size()) { return a.n.size() <=> b.n.size(); }
        for (int i = a.n.size(); i-- > 0;) { if (a.n[i] != b.n[i]) { return a.n[i] <=> b.n[i]; } }
        return std::strong_ordering::equal;}
    friend std::strong_ordering operator<=>(const InfInt &a, const InfInt &b) {
        int as = a.isNil() ? 0 : a.sgn, bs = b.isNil() ? 0 : b.sgn;
        if (as != bs) { return as <=> bs; }
        if (as == 0) { return std::strong_ordering::equal; }
        return as == 1 ? magCmp(a, b) : magCmp(b, a);}
    friend bool operator==(const InfInt &a, const InfInt &b) {
        if (a.is_inf || b.is_inf) { return a.is_inf == b.is_inf && a.sgn == b.sgn; }
        if (a.isNil() || b.isNil()) { return a.isNil() && b.isNil(); }
        return a.sgn == b.sgn && a.n == b.n;}

    // T: O(n), M: O(n); shifts of the magnitude left, arithmetic (floor) right; the sentinel is unchanged.
    InfInt &operator<<=(int s) {
        assert(s >= 0);
        if (is_inf || isNil() || s <= 0) { return *this; }
        int q = s / 64, r = s % 64, len = n.size();
        n.setSize(len + q + 1); W *w = n.data();
        if (r) { w[len + q] = infint_detail::shl(w + q, w, len, r); }
        else { std::copy_backward(w, w + len, w + len + q); w[len + q] = 0; }
        infint_detail::zero(w, q);
        trim();
        return *this;}
    InfInt &operator>>=(int s) {
        assert(s >= 0);
        if (is_inf || isNil() || s <= 0) { return *this; }
        int q = s / 64, r = s % 64, len = n.size();
        if (q >= len) { return *this = sgn == -1 ? -1 : 0; }
        bool lost = false;
        if (sgn == -1) {
            for (int i = 0; i < q && !lost; ++i) { lost = n[i] != 0; }
            lost |= r > 0 && (n[q] & ((W(1) << r) - 1)) != 0;}
        W *w = n.data();
        if (r) { infint_detail::shr(w, w + q, len - q, r); }
        else { std::copy(w + q, w + len, w); }
        n.setSize(len - q);
        trim();
        if (lost) { --*this; }
        return *this;}
    friend InfInt operator<<(InfInt a, int s) { a <<= s; return a; }
    friend InfInt operator>>(InfInt a, int s) { a >>= s; return a; }
    // Two's-complement word image of a finite value, ext = all-ones word for negatives, and the inverse.
    static Words twosComplement(const InfInt &a, int &ext) {
        Words v = a.n; ext = 0;
        if (v.empty()) { v.push_back(0); }
        if (a.sgn == -1 && !a.isNil()) {
            ext = -1;
            for (W &x : v) { x = ~x; }
            infint_detail::addWord(v.data(), v.size(), 1);}
        return v;}
    static InfInt fromTwos(Words v, int ext) {
        InfInt r;
        if (ext) {
            for (W &x : v) { x = ~x; }
            if (infint_detail::addWord(v.data(), v.size(), 1)) { v.push_back(1); }
            r.sgn = -1;}
        r.n = std::move(v); r.trim();
        return r;}
    template<typename F> static InfInt bitOp(const InfInt &a, const InfInt &b, F op) {
        assert(!a.is_inf && !b.is_inf);
        int ea, eb; Words x = twosComplement(a, ea), y = twosComplement(b, eb);
        int len = max(x.size(), y.size());
        x.resize(len, W(ea)); y.resize(len, W(eb));
        for (int i = 0; i < len; ++i) { x[i] = op(x[i], y[i]); }
        return fromTwos(std::move(x), op(W(ea), W(eb)) ? -1 : 0);}
    // T: O(n), M: O(n); two's-complement semantics on finite values.
    friend InfInt operator&(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r; r.n.setSize(min(a.n.size(), b.n.size()));
            for (int i = 0; i < r.n.size(); ++i) { r.n[i] = a.n[i] & b.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_and<W>());}
    friend InfInt operator|(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r = a.n.size() >= b.n.size() ? a : b; const InfInt &o = a.n.size() >= b.n.size() ? b : a;
            for (int i = 0; i < o.n.size(); ++i) { r.n[i] |= o.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_or<W>());}
    friend InfInt operator^(const InfInt &a, const InfInt &b) {
        if (a.sgn == 1 && b.sgn == 1 && !a.is_inf && !b.is_inf) {
            InfInt r = a.n.size() >= b.n.size() ? a : b; const InfInt &o = a.n.size() >= b.n.size() ? b : a;
            for (int i = 0; i < o.n.size(); ++i) { r.n[i] ^= o.n[i]; }
            r.trim(); return r;}
        return bitOp(a, b, std::bit_xor<W>());}
    InfInt operator~() const { assert(!is_inf); InfInt r = -*this; --r; return r; }
    InfInt &operator&=(const InfInt &o) { return *this = *this & o; }
    InfInt &operator|=(const InfInt &o) { return *this = *this | o; }
    InfInt &operator^=(const InfInt &o) { return *this = *this ^ o; }
    // T: O(n), M: O(1), or O(n) for the scans (the two's-complement image) and for setBit, clearBit, flipBit on negatives; bit queries on the magnitude (ctz, popcount,
    // bitLength) and on the two's-complement image (testBit, scans).
    friend int ctz(const InfInt &a) {
        assert(!a.is_inf && a.n.size() < (1 << 25));   // bit counts are int
        if (a.is_inf || a.isNil()) { return 0; }
        int c = 0;
        while (a.n[c] == 0) { ++c; }
        return 64 * c + std::countr_zero(a.n[c]);}
    friend int popcount(const InfInt &a) {
        assert(!a.is_inf && a.n.size() < (1 << 25));   // bit counts are int
        int c = 0, i = 0;
#ifdef __AVX2__
        __m256i lut = _mm256_broadcastsi128_si256(_mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4));
        __m256i msk = _mm256_set1_epi8(0x0F), acc = _mm256_setzero_si256();
        for (; i + 4 <= a.n.size(); i += 4) {
            __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(a.n.data() + i));
            __m256i cur = _mm256_add_epi8(_mm256_shuffle_epi8(lut, _mm256_and_si256(x, msk)), _mm256_shuffle_epi8(lut, _mm256_and_si256(_mm256_srli_epi16(x, 4), msk)));
            acc = _mm256_add_epi64(acc, _mm256_sad_epu8(cur, _mm256_setzero_si256()));}
        ulng res[4]; _mm256_storeu_si256(reinterpret_cast<__m256i *>(res), acc);
        c = int(res[0] + res[1] + res[2] + res[3]);
#endif
        for (; i < a.n.size(); ++i) { c += std::popcount(a.n[i]); }
        return c;}
    friend int bitLength(const InfInt &a) {
        assert(!a.is_inf && a.n.size() < (1 << 25));   // bit counts are int
        return a.is_inf || a.isNil() ? 0 : 64 * a.n.size() - std::countl_zero(a.n.back());}
    friend bool testBit(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf || a.isNil() || i < 0) { return false; }
        if (a.sgn == 1) { return i / 64 < a.n.size() && (a.n[i / 64] >> (i % 64) & 1); }
        int t = ctz(a);
        if (i <= t) { return i == t; }
        return i / 64 >= a.n.size() || !(a.n[i / 64] >> (i % 64) & 1);}
    friend int scan1(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf) { return -1; }
        i = max(i, 0); int ext; Words v = twosComplement(a, ext);
        for (int k = i / 64; k < v.size(); ++k) {
            W x = v[k] & (k == i / 64 ? ~W(0) << (i % 64) : ~W(0));
            if (x) { return 64 * k + std::countr_zero(x); }}
        return ext ? max(i, 64 * v.size()) : -1;}
    friend int scan0(const InfInt &a, int i) {
        assert(!a.is_inf && i >= 0);
        if (a.is_inf) { return -1; }
        i = max(i, 0); int ext; Words v = twosComplement(a, ext);
        for (int k = i / 64; k < v.size(); ++k) {
            W x = ~v[k] & (k == i / 64 ? ~W(0) << (i % 64) : ~W(0));
            if (x) { return 64 * k + std::countr_zero(x); }}
        return ext ? -1 : max(i, 64 * v.size());}
    InfInt &setBit(int i) {
        assert(!is_inf && i >= 0);
        if (is_inf || i < 0) { return *this; }
        if (sgn == -1 && !isNil()) { return *this |= InfInt(1) << i; }
        if (i / 64 >= n.size()) { n.resize(i / 64 + 1); }
        n[i / 64] |= W(1) << (i % 64);
        return *this;}
    InfInt &clearBit(int i) {
        assert(!is_inf && i >= 0);
        if (is_inf || i < 0) { return *this; }
        if (sgn == -1 && !isNil()) { return *this &= ~(InfInt(1) << i); }
        if (i / 64 < n.size()) { n[i / 64] &= ~(W(1) << (i % 64)); trim(); }
        return *this;}
    InfInt &flipBit(int i) { return testBit(*this, i) ? clearBit(i) : setBit(i); }
    // Half-GCD machinery: 2x2 cofactor matrices [[A, B], [C, D]] applied as (a, b) <- (A a + B b, C a + D b).
    static bool isIdentity(const InfInt &ma, const InfInt &mb, const InfInt &mc, const InfInt &md) { return ma == 1 && mb.isNil() && mc.isNil() && md == 1; }
    static void eucStep(InfInt q, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        std::swap(ma, mc); mc -= q * ma;
        std::swap(mb, md); md -= std::move(q) * mb;}
    static void eucBack(InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md, InfInt &a, InfInt &b) {
        InfInt q = mb.sgn == -1 ? abs(mc) / ma : abs(md) / mb;
        mc += q * ma; std::swap(ma, mc);
        md += q * mb; std::swap(mb, md);
        b += q * a; std::swap(a, b);}
    template<typename T> static void matApply(const T &ma, const T &mb, const T &mc, const T &md, InfInt &a, InfInt &b) {
        InfInt a0 = std::move(a), b0 = std::move(b);
        auto mul = [](const T &k, const InfInt &x) -> InfInt {
            if constexpr (std::same_as<T, InfInt>) { return k.isNil() || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : k * x; }
            else { return k == 0 || x.isNil() ? InfInt(0) : k == 1 ? x : k == -1 ? -x : x * k; }};
        a = mul(ma, a0); a += mul(mb, b0);
        b = mul(mc, a0); b += mul(md, b0);}
    // The words of x from index from as a value.
    static InfInt top(const InfInt &x, int from) {
        InfInt t;
        if (from < x.n.size()) { t.n.assign(x.n.data() + from, x.n.size() - from); }
        t.trim();
        return t;}
    static void adjust(const InfInt &ma, const InfInt &mb, const InfInt &mc, const InfInt &md, int shf, InfInt ha, InfInt hb, InfInt &a, InfInt &b) {
        if (shf == 0) { a = std::move(ha); b = std::move(hb); return; }
        InfInt la, lb;
        la.n.assign(a.n.data(), min(shf, a.n.size())); la.trim();
        lb.n.assign(b.n.data(), min(shf, b.n.size())); lb.trim();
        matApply(ma, mb, mc, md, la, lb);
        ha <<= 64 * shf; ha += la;
        hb <<= 64 * shf; hb += lb;
        a = std::move(ha); b = std::move(hb);}
    // T: O(n) per pass, M: O(n); Lehmer step on the leading 64 bits of equally long a >= b >= 0 (at least 2 words); false when no quotient is certain.
    template<bool TRACK> static bool lehmerHalfGcd(InfInt &a, InfInt &b, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        constexpr lll LIM = lll(1) << 60;
        if (a.n.size() < 2 || a.n.size() != b.n.size()) { return false; }
        lng ca = 1, cb = 0, cc = 0, cd = 1;
        int sh = bitLength(a) - 64;
        ulng ah = a.bitsFrom(sh), bh = b.bitsFrom(sh);
        auto bounds = [&](lng x, lng y) {
            lll mid = lll(x) * lll(ah) + lll(y) * lll(bh);
            return pair<lll, lll>{mid + min<lng>(x, 0) + min<lng>(y, 0), mid + max<lng>(x, 0) + max<lng>(y, 0)};};
        bool changed = false;
        while (true) {
            auto [alo, ahi] = bounds(ca, cb);
            auto [blo, bhi] = bounds(cc, cd);
            if (alo <= 0 || blo <= 0) { break; }
            lll q = alo / bhi, qh = (ahi - 1) / blo;
            if (q <= 0 || q != qh || q > LIM) { break; }
            lll nc = lll(ca) - q * cc, nd = lll(cb) - q * cd;
            if (nc < -LIM || nc > LIM || nd < -LIM || nd > LIM) { break; }
            ca = cc; cb = cd; cc = lng(nc); cd = lng(nd);
            changed = true;}
        if (!changed) { return false; }
        matApply(ca, cb, cc, cd, a, b);
        if constexpr (TRACK) { matApply(ca, cb, cc, cd, ma, mc); matApply(ca, cb, cc, cd, mb, md); }
        return true;}
    // T: O(M(n) * log(n)), M: O(n); reduces finite a >= b >= 0 until b has at most half of a's words, returning the matrix when TRACK; false if unchanged.
    template<bool TRACK> static bool halfGcd(InfInt &a, InfInt &b, InfInt &ma, InfInt &mb, InfInt &mc, InfInt &md) {
        assert(!a.is_inf && a.sgn == 1 && !b.is_inf && b.sgn == 1 && a >= b);
        if constexpr (TRACK) { ma = 1; mb = 0; mc = 0; md = 1; }
        if (b.isNil()) { return false; }
        int req = (a.n.size() + 1) / 2;
        if (b.n.size() <= req) { return false; }
        bool changed = false;
        auto reduce = [&]() {
            auto [q, r] = divMod(a, b);
            if constexpr (TRACK) { eucStep(std::move(q), ma, mb, mc, md); }
            a = std::exchange(b, std::move(r));
            changed = true;};
        if (a.n.size() <= infint_detail::HALFGCD) {
            while (!b.isNil() && b.n.size() > req) {
                if (lehmerHalfGcd<TRACK>(a, b, ma, mb, mc, md)) { changed = true; }
                else { reduce(); }}
            return changed;}
        InfInt ha = top(a, req), hb = top(b, req), pa, pb, pc, pd;
        halfGcd<true>(ha, hb, pa, pb, pc, pd);
        if (!isIdentity(pa, pb, pc, pd)) {
            adjust(pa, pb, pc, pd, req, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdentity(pa, pb, pc, pd)) { eucBack(pa, pb, pc, pd, a, b); }
            if (!isIdentity(pa, pb, pc, pd)) {
                changed = true;
                if constexpr (TRACK) { ma = std::move(pa); mb = std::move(pb); mc = std::move(pc); md = std::move(pd); }}}
        if (!b.isNil() && b.n.size() > req) { reduce(); }
        if (b.isNil() || b.n.size() <= req) { return changed; }
        int shf = 2 * req - a.n.size();
        if (shf < 0) {
            while (!b.isNil() && b.n.size() > req) { reduce(); }
            return changed;}
        ha = top(a, shf); hb = top(b, shf);
        halfGcd<true>(ha, hb, pa, pb, pc, pd);
        if (!isIdentity(pa, pb, pc, pd)) {
            adjust(pa, pb, pc, pd, shf, std::move(ha), std::move(hb), a, b);
            while ((a.sgn == -1 || b.sgn == -1 || a < b) && !isIdentity(pa, pb, pc, pd)) { eucBack(pa, pb, pc, pd, a, b); }
            if (!isIdentity(pa, pb, pc, pd)) {
                changed = true;
                if constexpr (TRACK) { matApply(pa, pb, pc, pd, ma, mc); matApply(pa, pb, pc, pd, mb, md); }}}
        while (!b.isNil() && b.n.size() > req) { reduce(); }
        return changed;}
    // T: O(M(n) * log(n)), M: O(n); nonnegative gcd; gcd(inf, x) = |x| for x != 0.
    friend InfInt gcd(InfInt a, InfInt b) {
        if ((a.is_inf && !b.isNil()) || a.isNil()) { b.sgn = 1; return b; }
        if ((b.is_inf && !a.isNil()) || b.isNil()) { a.sgn = 1; return a; }
        a.sgn = 1; b.sgn = 1;
        if (a.fitsWord() && b.fitsWord()) { return std::gcd(a.low64(), b.low64()); }
        int za = ctz(a), zb = ctz(b), z = min(za, zb);
        a >>= za; b >>= zb;
        InfInt ma, mb, mc, md;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); }
            if (b.isNil()) { break; }
            if (b.fitsWord()) { ulng u = b.low64(); a %= u; a = std::gcd(u, a.low64()); break; }
            if (a.n.size() <= infint_detail::BINARY_GCD) {
                infint_detail::Scratch s(size_t(a.n.size()) + b.n.size());
                W *x = s, *y = x + a.n.size();
                std::copy(a.n.begin(), a.n.end(), x); std::copy(b.n.begin(), b.n.end(), y);
                a.n.assign(x, infint_detail::gcdBinary(x, a.n.size(), y, b.n.size()));
                break;}
            if (b.n.size() > infint_detail::HALFGCD && halfGcd<false>(a, b, ma, mb, mc, md)) { continue; }
            if (a.n.size() > b.n.size() || !lehmerHalfGcd<false>(a, b, ma, mb, mc, md)) { a %= b; }}
        return a << z;}
    // T: O(M(n) * log(n)), M: O(n); g = a x + b y with |x| <= |b| / g and |y| <= |a| / g when both are nonzero.
    friend InfInt exGcd(InfInt a, InfInt b, InfInt &x, InfInt &y) {
        if (a.is_inf) {
            if (b.is_inf || b.isNil()) { x = a.sgn; y = 0; a.sgn = 1; return a; }
            x = 0; y = b.sgn; b.sgn = 1;
            return b;}
        if (b.is_inf) {
            if (a.isNil()) { x = 0; y = b.sgn; b.sgn = 1; return b; }
            x = a.sgn; y = 0; a.sgn = 1;
            return a;}
        int as = a.sgn, bs = b.sgn;
        a.sgn = 1; b.sgn = 1;
        x = 1; y = 0; InfInt x1 = 0, y1 = 1, ma, mb, mc, md;
        while (!b.isNil()) {
            if (a < b) { std::swap(a, b); std::swap(x, x1); std::swap(y, y1); }
            if (b.isNil()) { break; }
            if (b.n.size() > 1 && halfGcd<true>(a, b, ma, mb, mc, md)) { matApply(ma, mb, mc, md, x, x1); matApply(ma, mb, mc, md, y, y1); continue; }
            auto [q, r] = divMod(a, b);
            a = std::exchange(b, std::move(r));
            std::swap(x, x1); x1 -= q * x;
            std::swap(y, y1); y1 -= std::move(q) * y;}
        if (as == -1) { x = -std::move(x); }
        if (bs == -1) { y = -std::move(y); }
        return a;}
    friend InfInt lcm(InfInt a, InfInt b) {
        if (a.isNil() || b.isNil()) { return 0; }
        if (a.is_inf || b.is_inf) { InfInt r; r.is_inf = true; return r; }
        InfInt g = gcd(a, b), r = (std::move(a) / std::move(g)) * b; r.sgn = 1;
        return r;}
    // T: O(M(n) * log(n)), M: O(n); inverse of a modulo |mod| in [0, |mod|), -1 when gcd(a, mod) != 1, 0 for |mod| = 1.
    friend InfInt inv(InfInt a, InfInt mod) {
        assert(!a.is_inf && !mod.is_inf && !mod.isNil());
        if (a.is_inf || mod.is_inf || mod.isNil()) { return -1; }
        mod.sgn = 1;
        if (mod == 1) { return 0; }
        a = divModEuclid(a, mod).second;
        if (a.isNil()) { return -1; }
        if (a.fitsWord() && mod.fitsWord()) {
            ulng r0 = a.low64(), r1 = mod.low64(); lll x0 = 1, x1 = 0;
            while (r1) { ulng q = r0 / r1; r0 = std::exchange(r1, r0 - q * r1); x0 = std::exchange(x1, x0 - lll(q) * x1); }
            if (r0 != 1) { return -1; }
            x0 %= lll(mod.low64());
            return x0 < 0 ? ulng(x0 + lll(mod.low64())) : ulng(x0);}
        InfInt x, y, g = exGcd(std::move(a), mod, x, y);
        if (g != 1) { return -1; }
        if (x.sgn == -1) { x += mod; }
        return x;}
    // T: O(M(n * e)), M: O(n * e); nonnegative exponent; inf^0 = 1.
    friend InfInt pow(InfInt a, lng e) {
        assert(e >= 0);
        if (e < 0) { return 0; }
        if (e == 0) { return 1; }
        if (a.is_inf) { if (!(e & 1)) { a.sgn = 1; } return a; }
        if (e == 1 || a.isNil() || a == 1) { return a; }
        if (a == -1) { return e & 1 ? -1 : 1; }
        InfInt r = a;
        for (int i = std::bit_width(ulng(e)) - 2; i >= 0; --i) { r *= r; if ((e >> i) & 1) { r *= a; } }
        return r;}
    // S: O(M(k) * log(k)), U: NA, Q: O(M(k)) per product, M: O(k); modular context for a finite modulus of k words: Montgomery REDC (odd moduli, when asked), else a cached reciprocal from CACHED_MOD words (Knuth D or Burnikel-Ziegler below).
    struct ModCtx {
        int k = 0; bool mont = false; Words m, minv, r2, ctx, t, u, v; infint_detail::Divisor dv;
        ModCtx(const InfInt &modulus, bool montgomery) {
            m = modulus.n; k = m.size(); mont = montgomery && (m[0] & 1) && k >= infint_detail::MONTGOMERY_POW;
            t.setSize(2 * k + 1); u.setSize(k); v.setSize(2 * k);
            if (mont) {
                minv.assign(k, 0); Words y; y.assign(k, 0);
                minv[0] = ulng(0) - infint_detail::invMod64(m[0]);
                for (int p = 1; p < k; p *= 2) {
                    int np = min(2 * p, k);
                    infint_detail::truncatedMul(t.data(), m.data(), minv.data(), np);
                    std::copy(t.data(), t.data() + np, y.data()); infint_detail::addWord(y.data(), np, 2);
                    infint_detail::truncatedMul(t.data(), minv.data(), y.data(), np);
                    std::copy(t.data(), t.data() + np, minv.data());}
                Words big; big.assign(2 * k + 1, 0); big[2 * k] = 1;
                r2.assign(k, 0);
                infint_detail::divRem(nullptr, r2.data(), big.data(), 2 * k + 1, m.data(), k);}
            else if (k >= infint_detail::CACHED_MOD) { ctx.setSize(int(infint_detail::Divisor::words(k, k))); dv.prepare(ctx.data(), m.data(), k, k); }}
        // x = x * y mod m on k-word residues (Montgomery form when mont); y may alias x.
        void mul(Words &x, const Words &y) {
            if (&x == &y) { infint_detail::square(t.data(), x.data(), k); }
            else { infint_detail::mul(t.data(), x.data(), k, y.data(), k); }
            t[2 * k] = 0;
            if (!mont) { infint_detail::divRem(nullptr, x.data(), t.data(), 2 * k, m.data(), k, dv.d ? &dv : nullptr); return; }
            infint_detail::truncatedMul(u.data(), t.data(), minv.data(), k);
            infint_detail::mul(v.data(), u.data(), k, m.data(), k);
            infint_detail::addInto(t.data(), 2 * k + 1, v.data(), 2 * k);
            if (infint_detail::cmp(t.data() + k, infint_detail::norm(t.data() + k, k + 1), m.data(), k) >= 0) { infint_detail::subFrom(t.data() + k, k + 1, m.data(), k); }
            std::copy(t.data() + k, t.data() + 2 * k, x.data());}
        Words residue(const InfInt &a) const {
            InfInt mod; mod.n = m;
            Words x = divModEuclid(a, mod).second.n; x.resize(k);
            return x;}
        Words enter(const InfInt &a) { Words x = residue(a); if (mont) { mul(x, r2); } return x; }
        InfInt leave(Words x) {
            if (mont) { Words one; one.assign(k, 0); one[0] = 1; mul(x, one); }
            InfInt r; r.n = std::move(x); r.trim();
            return r;}
        // Plain modular product of residues in [0, m) without Montgomery form.
        InfInt mul(const InfInt &a, const InfInt &b) {
            Words x = residue(a), y = residue(b);
            infint_detail::mul(t.data(), x.data(), k, y.data(), k);
            infint_detail::divRem(nullptr, x.data(), t.data(), 2 * k, m.data(), k, dv.d ? &dv : nullptr);
            InfInt r; r.n = std::move(x); r.trim();
            return r;}
    };
    // T: O(log(e) * M(n)) with sliding windows, M: O(n); a^e mod |mod| in [0, |mod|), -1 when e < 0 and a is not a unit; |mod| = 1 gives 0; Montgomery REDC for odd moduli below CACHED_DIV words, the cached Divisor from there and for even moduli from CACHED_MOD words.
    friend InfInt powMod(InfInt a, InfInt e, InfInt mod) {
        assert(!a.is_inf && !e.is_inf && !mod.is_inf && !mod.isNil());
        if (a.is_inf || e.is_inf || mod.is_inf || mod.isNil()) { return 0; }
        mod.sgn = 1;
        if (mod == 1) { return 0; }
        if (e.isNil()) { return 1; }
        if (e.sgn == -1) { a = inv(a, mod); if (a == -1) { return -1; } e.sgn = 1; }
        a = divModEuclid(a, mod).second;
        if (a.isNil() || a == 1 || e == 1) { return a; }
        int bits = bitLength(e);
        auto bit = [&](int i) { return int((e.n[i / 64] >> (i % 64)) & 1); };
        int len = 1, best = INT_MAX;
        for (int w = 1; w <= min(bits, 8); ++w) {
            int l = max(bits - w, 0);
            while (!bit(l)) { ++l; }
            int cost = (1 << (w - 1)) - (w == 1) + l;
            for (int j = l - 1; j >= 0;) {
                if (!bit(j)) { --j; continue; }
                int k = max(j - w + 1, 0);
                while (!bit(k)) { ++k; }
                ++cost; j = k - 1;}
            if (cost < best) { len = w; best = cost; }}
        auto window = [&]<typename T>(T base, auto &&mulMod) -> T {
            if (len == 1) {
                T r = base;
                for (int i = bits - 2; i >= 0; --i) { mulMod(r, r); if (bit(i)) { mulMod(r, base); } }
                return r;}
            vector<T> odd(size_t(1) << (len - 1)); odd[0] = std::move(base);
            if (odd.size() > 1) {
                T sq = odd[0]; mulMod(sq, sq);
                for (size_t i = 1; i < odd.size(); ++i) { odd[i] = odd[i - 1]; mulMod(odd[i], sq); }}
            int l = max(bits - len, 0);
            while (!bit(l)) { ++l; }
            int k = 0;
            for (int j = bits - 1; j >= l; --j) { k = (k << 1) | bit(j); }
            T r = odd[size_t(k >> 1)];
            for (int i = l - 1; i >= 0;) {
                if (!bit(i)) { mulMod(r, r); --i; continue; }
                l = max(i - len + 1, 0);
                while (!bit(l)) { ++l; }
                k = 0;
                for (int j = i; j >= l; --j) { k = (k << 1) | bit(j); }
                for (int j = l; j <= i; ++j) { mulMod(r, r); }
                mulMod(r, odd[size_t(k >> 1)]); i = l - 1;}
            return r;};
        if (mod.fitsWord()) {
            ulng um = mod.low64();
            auto mulMod = [&](ulng &x, const ulng &y) { x = ulng(ulll(x) * y % um); };
            return window(a.low64(), mulMod);}
        ModCtx ctx(mod, int(mod.n.size()) < infint_detail::CACHED_DIV);
        auto mulMod = [&](Words &x, const Words &y) { ctx.mul(x, y); };
        return ctx.leave(window(ctx.enter(a), mulMod));}
    // T: O(M(n)), M: O(n); Zimmermann's recursive square root with remainder on words, a has n = 4l words with a[n - 1] >= 2^62; s n / 2 words, r n / 2 + 1.
    static void sqrtRemWords(W *s, W *r, const W *a, int nn) {
        if (nn <= 2) {
            ulll v = nn == 2 ? (ulll(a[1]) << 64) | a[0] : a[0];
            long double f = std::sqrt(static_cast<long double>(v));
            ulng q = f >= 18446744073709551615.0L ? ~ulng(0) : ulng(f);
            while (ulll(q) * q > v) { --q; }
            while (q != ~ulng(0) && ulll(q + 1) * (q + 1) <= v) { ++q; }
            ulll rem = v - ulll(q) * q;
            s[0] = q; r[0] = ulng(rem); r[1] = ulng(rem >> 64);
            return;}
        int l = nn / 4, hn = 2 * l;
        infint_detail::Scratch buf(size_t(12 * hn) + 20);
        W *sh = buf, *rh = sh + l + 1, *pad = rh + hn + 1, *s2 = pad + hn + 2, *r2 = s2 + l + 1, *sq = r2 + hn + 2, *num = sq + 2 * l;
        W *d = num + hn + 1, *q = d + l + 1, *u = q + l + 2, *sx = u + l + 1, *rr = sx + hn + 1, *qq = rr + hn + 2, *t = qq + 2 * l + 2;
        infint_detail::zero(buf, 12 * hn + 20);
        if (hn % 4 == 0 || hn == 2) { sqrtRemWords(sh, rh, a + hn, hn); }
        else {
            std::copy(a + hn, a + nn, pad + 2);
            sqrtRemWords(s2, r2, pad, hn + 2);
            std::copy(s2 + 1, s2 + l + 1, sh);
            infint_detail::square(sq, sh, l);
            std::copy(a + hn, a + nn, rh); infint_detail::subFrom(rh, hn, sq, 2 * l);}
        std::copy(a + l, a + hn, num); std::copy(rh, rh + l + 1, num + l);
        d[l] = infint_detail::shl(d, sh, l, 1);
        int ln = infint_detail::norm(num, hn + 1), ld = infint_detail::norm(d, l + 1);
        if (ln >= ld) { infint_detail::divRem(q, u, num, ln, d, ld); }
        else { std::copy(num, num + ln, u); }
        int lq = infint_detail::norm(q, l + 2);
        std::copy(sh, sh + l, sx + l); sx[hn] = infint_detail::add(sx, sx, hn, q, lq);
        std::copy(a, a + l, rr); std::copy(u, u + l + 1, rr + l);
        if (lq) { infint_detail::square(qq, q, lq); }
        W borrow = infint_detail::sub(rr, rr, hn + 2, qq, 2 * lq);
        while (borrow) {
            infint_detail::zero(t, hn + 2); t[hn + 1] = infint_detail::shl(t, sx, hn + 1, 1);
            infint_detail::subWord(t, hn + 2, 1); infint_detail::subWord(sx, hn + 1, 1);
            borrow -= infint_detail::add(rr, rr, hn + 2, t, hn + 2);}
        std::copy(sx, sx + hn, s); std::copy(rr, rr + hn + 1, r);}
    // T: O(M(n)), M: O(n); floor square root and remainder of a finite a >= 0.
    friend pair<InfInt, InfInt> isqrtRem(const InfInt &a) {
        assert(!a.is_inf && a.sgn == 1);
        if (a.is_inf) { return {a, 0}; }
        if (a.isNil()) { return {0, 0}; }
        int k = a.n.size(), l = (k + 3) / 4, total = 4 * l;
        int t = (std::countl_zero(a.n.back()) / 2) * 2, shift = 64 * (total - k) + t;
        infint_detail::Scratch buf(2 * size_t(total) + 1);
        W *x = buf, *s = x + total, *r = s + total / 2;
        infint_detail::zero(x, total - k); std::copy(a.n.begin(), a.n.end(), x + (total - k));
        if (t) { infint_detail::shl(x, x, total, t); }
        sqrtRemWords(s, r, x, total);
        InfInt root; root.n.assign(s, total / 2); root.trim(); root >>= shift / 2;
        InfInt rem = a - root * root;
        return {std::move(root), std::move(rem)};}
    friend InfInt sqrt(const InfInt &a) {
        assert(a.sgn == 1);
        if (a.is_inf) { return a; }
        return isqrtRem(a).first;}
    // T: O(log(b) * M(n) * log(k)), M: O(n); truncated k-th root (k >= 1) and remainder a - root^k; a < 0 only for odd k. Integer Newton from a
    // floating-point upper estimate (2^-20 relative margin), so the iteration count is logarithmic in b for every k.
    friend pair<InfInt, InfInt> kthRootRem(const InfInt &a, int k) {
        assert(!a.is_inf && k >= 1 && (a.sgn == 1 || (k & 1)));
        k = max(k, 1);
        if (a.is_inf) { return {a, 0}; }
        if (a.sgn == -1) { auto [s, r] = kthRootRem(-a, k); return {-std::move(s), -std::move(r)}; }
        if (k == 1) { return {a, 0}; }
        if (k == 2) { return isqrtRem(a); }
        if (a.isNil()) { return {0, 0}; }
        int bl = bitLength(a);
        if (bl <= k) { return {1, a - 1}; }
        long double lg = (bl > 64 ? std::log2(static_cast<long double>(a.bitsFrom(bl - 64))) + (bl - 64) : std::log2(static_cast<long double>(a.low64()))) / k;
        int ip = int(std::floor(lg)); long double mant = std::exp2(lg - ip) * (1 + std::ldexp(1.0L, -20));
        InfInt x = ulng(std::ceil(std::ldexp(mant, min(ip, 62))));
        if (ip > 62) { x <<= ip - 62; }
        while (true) {
            InfInt y = x * (k - 1); y += a / pow(x, k - 1); y /= k;
            if (y >= x) { break; }
            x = std::move(y);}
        while (pow(x + 1, k) <= a) { ++x; }
        InfInt rem = a - pow(x, k);
        return {std::move(x), std::move(rem)};}
    static uint residueSmall(const InfInt &a, uint m) {
        ulng r = 0;
        for (int i = a.n.size(); i-- > 0;) { r = ((r << 32) | (a.n[i] >> 32)) % m; r = ((r << 32) | uint(a.n[i])) % m; }
        return uint(r);}
    // T: O(M(n)) after O(n) residue filters modulo 64, 63, 65 and 11, M: O(n); false for negatives, true for 0, else the filters and then isqrtRem.
    friend bool isPerfectSquare(const InfInt &a) {
        assert(!a.is_inf);
        if (a.is_inf || a.sgn == -1) { return false; }
        if (a.isNil()) { return true; }
        if (!((0x0202021202030213ULL >> (a.n[0] & 63)) & 1)) { return false; }
        static const auto build = [](uint m) { array<ulng, 2> v{}; for (ulng x = 0; x < m; ++x) { ulng q = x * x % m; v[size_t(q / 64)] |= 1ULL << (q % 64); } return v; };
        static const array<ulng, 2> Q63 = build(63), Q65 = build(65), Q11 = build(11);
        uint r = residueSmall(a, 45045);
        auto test = [&](const array<ulng, 2> &q, uint m) { uint v = r % m; return (q[size_t(v / 64)] >> (v % 64)) & 1; };
        if (!test(Q63, 63) || !test(Q65, 65) || !test(Q11, 11)) { return false; }
        return isqrtRem(a).second.isNil();}
    static bool isPrime32(uint q) {
        if (q < 2) { return false; }
        for (uint p : {2u, 3u, 5u, 7u, 11u, 13u, 17u, 19u, 23u, 29u, 31u, 37u}) { if (q % p == 0) { return q == p; } }
        uint d = q - 1; int s = std::countr_zero(d); d >>= s;
        for (ulng b : {2ULL, 7ULL, 61ULL}) {
            ulng x = powMod64(b % q, d, q);
            if (b % q == 0 || x == 1 || x == q - 1) { continue; }
            bool ok = false;
            for (int i = 1; i < s && !ok; ++i) { x = x * x % q; ok = x == q - 1; }
            if (!ok) { return false; }}
        return true;}
    static ulng powMod64(ulng b, ulng e, ulng m) {
        ulng r = 1; b %= m;
        for (; e; e >>= 1, b = ulng(ulll(b) * b % m)) { if (e & 1) { r = ulng(ulll(r) * b % m); } }
        return r;}
    // T: O(pi(b) * n + M(n) * log(b)) worst case, M: O(n); 0, 1 and every x^k with k >= 2 (odd k for negative values).
    friend bool isPerfectPower(const InfInt &a) {
        assert(!a.is_inf);
        InfInt m = abs(a);
        if (magCmp(m, 1) <= 0) { return true; }
        int bl = bitLength(m), t = ctz(m);
        auto check = [&](int p) { return p >= 2 && (a.sgn == 1 || (p & 1)) && kthRootRem(m, p).second.isNil(); };
        if (t > 0) {
            for (int p = 2; p <= t; ++p) { if (t % p == 0 && check(p)) { return true; } }
            return false;}
        vector<bool> comp(size_t(bl) + 1, false);
        for (int p = 2; p <= bl; ++p) {
            if (comp[size_t(p)]) { continue; }
            for (lng q = lng(p) * p; q <= bl; q += p) { comp[size_t(q)] = true; }
            if (a.sgn == -1 && p == 2) { continue; }
            bool reject = false;
            for (uint i = 1, found = 0; i <= 64 && found < 2 && !reject; ++i) {
                ulng q = 2ULL * i * ulng(p) + 1;
                if (q >= (1ULL << 31) || !isPrime32(uint(q))) { continue; }
                ++found;
                uint r = residueSmall(m, uint(q));
                reject = r != 0 && powMod64(r, (q - 1) / ulng(p), q) != 1;}
            if (!reject && check(p)) { return true; }}
        return false;}
    static int jacobi64(ulng a, ulng m) {
        int res = 1;
        while (a) {
            int t = std::countr_zero(a); a >>= t;
            if ((t & 1) && ((m & 7) == 3 || (m & 7) == 5)) { res = -res; }
            if ((a & 3) == 3 && (m & 3) == 3) { res = -res; }
            std::swap(a, m); a %= m;}
        return m == 1 ? res : 0;}
    // T: O(n^2) word operations, M: O(n); Jacobi symbol (a / m) for odd positive m; kronecker extends it to every finite m.
    friend int jacobi(const InfInt &a, const InfInt &m) {
        assert(!a.is_inf && !m.is_inf && m.sgn == 1 && !m.isNil() && (m.n[0] & 1));
        if (a.is_inf || m.is_inf || m.isNil()) { return 0; }
        InfInt x = divModEuclid(a, m).second, y = m;
        if (y.fitsWord()) { return jacobi64(x.low64(), y.low64()); }
        int res = 1;
        while (!x.isNil()) {
            int t = ctz(x); x >>= t;
            uint y8 = y.n[0] & 7;
            if ((t & 1) && (y8 == 3 || y8 == 5)) { res = -res; }
            if ((x.n[0] & 3) == 3 && (y8 & 3) == 3) { res = -res; }
            std::swap(x, y); x %= y;
            if (y.fitsWord()) { return res * jacobi64(x.low64(), y.low64()); }}
        return y == 1 ? res : 0;}
    friend int kronecker(const InfInt &a, const InfInt &m) {
        assert(!a.is_inf && !m.is_inf);
        if (a.is_inf || m.is_inf) { return 0; }
        if (m.isNil()) { return magCmp(a, 1) == 0 ? 1 : 0; }
        int res = m.sgn == -1 && a.sgn == -1 ? -1 : 1, t = ctz(m);
        InfInt y = abs(m); y >>= t;
        if (t) {
            if (a.isNil() || !(a.n[0] & 1)) { return 0; }
            uint a8 = uint((a & 7).low64());
            if ((t & 1) && (a8 == 3 || a8 == 5)) { res = -res; }}
        return res * jacobi(a, y);}
    // T: O(b * M(n)), M: O(n); Baillie-PSW: trial division, a strong base-2 test and a strong Lucas-Selfridge test; deterministic below 2^64,
    // Monte Carlo above (no counterexample is known); false for values below 2.
    friend bool isProbablePrime(const InfInt &a) {
        assert(!a.is_inf);
        if (a.is_inf || a.sgn == -1 || magCmp(a, 2) < 0) { return false; }
        if (a.fitsWord()) {
            ulng v = a.low64();
            if (v < 4) { return true; }
            if (!(v & 1)) { return false; }
            ulng d = v - 1; int s = std::countr_zero(d); d >>= s;
            for (ulng b : {2ULL, 325ULL, 9375ULL, 28178ULL, 450775ULL, 9780504ULL, 1795265022ULL}) {
                ulng x = powMod64(b % v, d, v);
                if (b % v == 0 || x == 1 || x == v - 1) { continue; }
                bool ok = false;
                for (int i = 1; i < s && !ok; ++i) { x = ulng(ulll(x) * x % v); ok = x == v - 1; }
                if (!ok) { return false; }}
            return true;}
        if (!(a.n[0] & 1)) { return false; }
        for (ulng prod : {614889782588491410ULL, 3749562977351496827ULL, 4343678784233766587ULL}) {
            if (std::gcd(infint_detail::remWord(a.n.data(), a.n.size(), prod), prod) != 1) { return false; }}
        if (isPerfectSquare(a)) { return false; }
        InfInt am1 = a - 1; int s = ctz(am1); InfInt d = am1 >> s;
        InfInt x = powMod(2, d, a);
        if (x != 1 && x != am1) {
            bool ok = false;
            for (int i = 1; i < s && !ok; ++i) { x = x * x % a; ok = x == am1; }
            if (!ok) { return false; }}
        lng disc = 5;
        while (true) {
            int j = jacobi(InfInt(disc), a);
            if (j == -1) { break; }
            if (j == 0) { return false; }
            disc = disc > 0 ? -(disc + 2) : -(disc - 2);}
        InfInt q = divModEuclid(InfInt((1 - disc) / 4), a).second, dm = divModEuclid(InfInt(disc), a).second;
        ModCtx ctx(a, false);
        auto half = [&](InfInt v) { if (v.n[0] & 1) { v += a; } v >>= 1; return divModEuclid(v, a).second; };
        auto dbl = [&](InfInt &u, InfInt &v, InfInt &qk) {
            InfInt uv = ctx.mul(u, v);
            v = ctx.mul(v, v) - ctx.mul(2, qk); if (v.sgn == -1) { v += a; }
            u = std::move(uv); qk = ctx.mul(qk, qk);};
        InfInt ap1 = a + 1; int s2 = ctz(ap1); InfInt dd = ap1 >> s2;
        InfInt u = 1, v = 1, qk = q;
        for (int i = bitLength(dd) - 2; i >= 0; --i) {
            dbl(u, v, qk);
            if (testBit(dd, i)) {
                InfInt nu = half(u + v), nv = half(ctx.mul(dm, u) + v);
                u = std::move(nu); v = std::move(nv); qk = ctx.mul(qk, q);}}
        if (u.isNil() || v.isNil()) { return true; }
        for (int r = 1; r < s2; ++r) {
            v = ctx.mul(v, v) - ctx.mul(2, qk); if (v.sgn == -1) { v += a; }
            qk = ctx.mul(qk, qk);
            if (v.isNil()) { return true; }}
        return false;}
    // T: O(M(n)), M: O(n); floor(2^(64 n) / a) for a finite positive a of n 32-bit limbs with its top limb bit set (n + 1 limbs, exact).
    friend InfInt newtonRaphsonInv(const InfInt &a) {
        assert(!a.is_inf && a.sgn == 1 && !a.isNil() && bitLength(a) % 32 == 0);
        if (a.is_inf || a.isNil() || bitLength(a) % 32) { return 0; }
        bool odd = !(a.n.back() >> 32);
        InfInt x = a, r;
        if (odd) { x <<= 32; }
        r.n.setSize(x.n.size() + 1);
        infint_detail::invertExact(r.n.data(), x.n.data(), x.n.size());
        r.trim();
        if (odd) { r >>= 32; }
        return r;}

    // T: O(1), M: O(1); stream base manipulator: per-stream iword, default 10; std::hex and std::oct flags also apply.
    struct SetBase { uint base; explicit SetBase(uint b) : base(b) {} };
    static int getBaseId() { static const int id = std::ios_base::xalloc(); return id; }
    static uint streamBase(std::ios_base &s) {
        uint base = uint(s.iword(getBaseId()));
        if (s.flags() & std::ios_base::hex) { return 16; }
        if (s.flags() & std::ios_base::oct) { return 8; }
        return base ? base : 10;}
    friend istream &operator>>(istream &is, SetBase sb) { is.iword(getBaseId()) = int(sb.base); return is; }
    friend ostream &operator<<(ostream &os, SetBase sb) { os.iword(getBaseId()) = int(sb.base); return os; }
    // T: O(L), M: O(L); one whitespace-delimited token through the string extractor into a reused thread-local buffer (the library scans the get
    // area directly: 2x faster than a sgetc loop here); a malformed token sets failbit and leaves a unchanged.
    friend istream &operator>>(istream &is, InfInt &a) {
        static thread_local string tok;
        if (is >> tok && !a.read(tok, streamBase(is))) { is.setstate(std::ios_base::failbit); }
        return is;}
    // T: as toChars, M: O(L); the text of toString in the stream's base, written to the stream buffer (through the width field when one is set).
    friend ostream &operator<<(ostream &os, const InfInt &a) {
        uint base = streamBase(os); int bound = a.charsBound(base);
        infint_detail::Scratch s(size_t(bound + 7) / 8);
        char *buf = reinterpret_cast<char *>(s.p);
        int len = int(a.toChars(buf, buf + bound, base).ptr - buf);
        if (os.width()) { return os << string_view(buf, size_t(len)); }
        return os.write(buf, len);}
};
using iint = InfInt;

// T: O(N) per linear operation and O(N^2) per product, M: O(N); fixed N-word unsigned integer, every operation wraps modulo 2^(64 N).
template<int N> struct UInt {
    static_assert(N >= 1);
    using W = ulng;
    array<W, N> w{};

    UInt() {}
    template<typename T> requires InfInt::isIntegral<T> UInt(T x) {
        bool neg = false;
        if constexpr (std::is_signed_v<T> || std::same_as<T, lll>) { neg = x < 0; }
        ulll u = ulll(x);
        if (neg) { w.fill(~W(0)); }
        w[0] = W(u);
        if constexpr (N > 1) { w[1] = W(u >> 64); }}
    // T: O(N), M: O(N); two's-complement image of a finite value modulo 2^(64 N); the string form parses as InfInt does; another width copies its low words.
    explicit UInt(const InfInt &a) {
        assert(!a.is_inf);
        std::copy(a.n.begin(), a.n.begin() + min(N, a.n.size()), w.begin());
        if (a.sgn == -1 && !a.isNil()) { *this = -*this; }}
    explicit UInt(string_view s, uint base = 10) : UInt(InfInt(s, base)) {}
    template<int M> explicit UInt(const UInt<M> &o) { std::copy(o.w.begin(), o.w.begin() + (M < N ? M : N), w.begin()); }
    string toString(uint base = 10, bool lower = false) const { return InfInt(*this).toString(base, lower); }
    explicit operator ulng() const { return w[0]; }
    explicit operator bool() const { return std::ranges::any_of(w, [](W x) { return x != 0; }); }
    int words() const { return infint_detail::norm(w.data(), N); }
    friend int bitLength(const UInt &a) { int k = a.words(); return k ? 64 * k - std::countl_zero(a.w[size_t(k) - 1]) : 0; }
    friend int ctz(const UInt &a) {
        for (int i = 0; i < N; ++i) { if (a.w[size_t(i)]) { return 64 * i + std::countr_zero(a.w[size_t(i)]); } }
        return 0;}
    friend int popcount(const UInt &a) { int c = 0; for (W x : a.w) { c += std::popcount(x); } return c; }

    UInt &operator+=(const UInt &o) { infint_detail::add(w.data(), w.data(), N, o.w.data(), N); return *this; }
    UInt &operator-=(const UInt &o) { infint_detail::sub(w.data(), w.data(), N, o.w.data(), N); return *this; }
    UInt &operator*=(const UInt &o) { array<W, N> r; infint_detail::truncatedMul(r.data(), w.data(), o.w.data(), N); w = r; return *this; }
    // T: O(N^2), M: O(N); quotient and remainder, o != 0.
    friend pair<UInt, UInt> divMod(const UInt &a, const UInt &o) {
        int n = a.words(), m = o.words();
        assert(m > 0);
        UInt q, r;
        if (n < m || m == 0) { r = a; return {q, r}; }
        W qq[N + 1], rr[N];
        infint_detail::divRem(qq, rr, a.w.data(), n, o.w.data(), m);
        std::copy(qq, qq + (n - m + 1), q.w.begin()); std::copy(rr, rr + m, r.w.begin());
        return {q, r};}
    UInt &operator/=(const UInt &o) { return *this = divMod(*this, o).first; }
    UInt &operator%=(const UInt &o) { return *this = divMod(*this, o).second; }
    // T: O(N), M: O(1); in-place quotient by a word d != 0, returns the remainder.
    W divWord(W d) {
        assert(d != 0);
        int n = d ? words() : 0;
        return n ? infint_detail::divWord(w.data(), w.data(), n, d) : 0;}
    UInt &operator&=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] &= o.w[size_t(i)]; } return *this; }
    UInt &operator|=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] |= o.w[size_t(i)]; } return *this; }
    UInt &operator^=(const UInt &o) { for (int i = 0; i < N; ++i) { w[size_t(i)] ^= o.w[size_t(i)]; } return *this; }
    // T: O(N), M: O(1); shift counts in [0, 64 N); bits leave at the ends.
    UInt &operator<<=(int s) {
        assert(s >= 0 && s < 64 * N);
        if (s < 0 || s >= 64 * N) { w.fill(0); return *this; }
        int q = s / 64, r = s % 64;
        for (int i = N - 1; i >= 0; --i) {
            W x = i - q >= 0 ? w[size_t(i - q)] << r : 0;
            if (r && i - q - 1 >= 0) { x |= w[size_t(i - q - 1)] >> (64 - r); }
            w[size_t(i)] = x;}
        return *this;}
    UInt &operator>>=(int s) {
        assert(s >= 0 && s < 64 * N);
        if (s < 0 || s >= 64 * N) { w.fill(0); return *this; }
        int q = s / 64, r = s % 64;
        for (int i = 0; i < N; ++i) {
            W x = i + q < N ? w[size_t(i + q)] >> r : 0;
            if (r && i + q + 1 < N) { x |= w[size_t(i + q + 1)] << (64 - r); }
            w[size_t(i)] = x;}
        return *this;}
    UInt operator~() const { UInt r; for (int i = 0; i < N; ++i) { r.w[size_t(i)] = ~w[size_t(i)]; } return r; }
    UInt operator-() const { UInt r = ~*this; W one[1] = {1}; infint_detail::add(r.w.data(), r.w.data(), N, one, 1); return r; }
    friend UInt operator+(UInt a, const UInt &b) { a += b; return a; }
    friend UInt operator-(UInt a, const UInt &b) { a -= b; return a; }
    friend UInt operator*(UInt a, const UInt &b) { a *= b; return a; }
    friend UInt operator/(const UInt &a, const UInt &b) { return divMod(a, b).first; }
    friend UInt operator%(const UInt &a, const UInt &b) { return divMod(a, b).second; }
    friend UInt operator&(UInt a, const UInt &b) { a &= b; return a; }
    friend UInt operator|(UInt a, const UInt &b) { a |= b; return a; }
    friend UInt operator^(UInt a, const UInt &b) { a ^= b; return a; }
    friend UInt operator<<(UInt a, int s) { a <<= s; return a; }
    friend UInt operator>>(UInt a, int s) { a >>= s; return a; }
    // T: O(N^2), M: O(N); the high N words of the 2N-word product.
    friend UInt mulHigh(const UInt &a, const UInt &b) {
        W r[2 * N]; infint_detail::mulBasecase(r, a.w.data(), N, b.w.data(), N);
        UInt h; std::copy(r + N, r + 2 * N, h.w.begin());
        return h;}
    friend std::strong_ordering operator<=>(const UInt &a, const UInt &b) {
        for (int i = N - 1; i >= 0; --i) { if (a.w[size_t(i)] != b.w[size_t(i)]) { return a.w[size_t(i)] <=> b.w[size_t(i)]; } }
        return std::strong_ordering::equal;}
    friend bool operator==(const UInt &a, const UInt &b) { return a.w == b.w; }
    // One token as InfInt reads it (the two's-complement image of a negative value); the sentinel token sets failbit.
    friend istream &operator>>(istream &is, UInt &a) {
        InfInt t;
        if (is >> t) { if (t.is_inf) { is.setstate(std::ios_base::failbit); } else { a = UInt(t); } }
        return is;}
    friend ostream &operator<<(ostream &os, const UInt &a) { return os << InfInt(a); }
};
using u256 = UInt<4>;

template<> struct std::hash<InfInt> {
    size_t operator()(const InfInt &a) const noexcept {
        ulng h = 0x9E3779B97F4A7C15ULL ^ ulng(a.is_inf ? 2 : 0) ^ ulng(a.sgn == -1 && !a.isNil() ? 1 : 0);
        auto mix = [&](ulng x) { h ^= x; h *= 0xBF58476D1CE4E5B9ULL; h ^= h >> 31; };
        if (a.n.empty()) { mix(0); }
        for (ulng x : a.n) { mix(x); }
        return size_t(h);}
};
template<int N> struct std::hash<UInt<N>> {
    size_t operator()(const UInt<N> &a) const noexcept {
        ulng h = 0x9E3779B97F4A7C15ULL;
        for (ulng x : a.w) { h ^= x; h *= 0xBF58476D1CE4E5B9ULL; h ^= h >> 31; }
        return size_t(h);}
};
// std::format spec [[fill]align][sign][#][0][width][type] with a literal width and type d (default), b, B, o, x or X, as for integers.
template<> struct std::formatter<InfInt> {
    char fill = ' ', align = 0, sign = '-', type = 'd'; bool alt = false, zero = false; int width = 0;
    constexpr auto parse(std::format_parse_context &ctx) {
        auto it = ctx.begin(), end = ctx.end();
        auto isAlign = [](char c) { return c == '<' || c == '>' || c == '^'; };
        if (it != end && it + 1 != end && isAlign(it[1]) && *it != '{' && *it != '}') { fill = *it; align = it[1]; it += 2; }
        else if (it != end && isAlign(*it)) { align = *it++; }
        if (it != end && (*it == '+' || *it == '-' || *it == ' ')) { sign = *it++; }
        if (it != end && *it == '#') { alt = true; ++it; }
        if (it != end && *it == '0') { zero = true; ++it; }
        if (it != end && *it == '0') { throw std::format_error("InfInt: width must be nonzero"); }
        while (it != end && *it >= '0' && *it <= '9') {
            width = 10 * width + (*it++ - '0');
            if (width > 65535) { throw std::format_error("InfInt: width above 65535"); }}
        if (it != end && *it != '}') { type = *it++; }
        if (it != end && *it != '}') { throw std::format_error("InfInt: unsupported format spec"); }
        if (type != 'd' && type != 'b' && type != 'B' && type != 'o' && type != 'x' && type != 'X') { throw std::format_error("InfInt: bad presentation type"); }
        return it;}
    template<typename Ctx> auto format(const InfInt &a, Ctx &ctx) const {
        uint base = type == 'd' ? 10 : type == 'o' ? 8 : (type | 32) == 'x' ? 16 : 2;
        bool lower = type == 'x' || type == 'b';
        int bound = a.charsBound(base);
        infint_detail::Scratch s(size_t(bound + 7) / 8);
        char *buf = reinterpret_cast<char *>(s.p);
        string_view body(buf, size_t(a.toChars(buf, buf + bound, base, lower).ptr - buf)), prefix;
        char sgn = 0;
        if (body[0] == '-') { sgn = '-'; body.remove_prefix(1); }
        else if (sign != '-') { sgn = sign; }
        if (alt) { prefix = base == 16 ? (lower ? "0x" : "0X") : base == 2 ? (lower ? "0b" : "0B") : base == 8 && !a.isNil() ? "0" : ""; }
        int pad = max(0, width - int(body.size() + prefix.size()) - (sgn != 0));
        auto out = ctx.out();
        auto put = [&](char c, int k) { while (k-- > 0) { *out++ = c; } };
        auto puts = [&](string_view v) { out = std::copy(v.begin(), v.end(), out); };
        if (zero && !align) { if (sgn) { *out++ = sgn; } puts(prefix); put('0', pad); puts(body); return out; }
        int left = align == '<' ? 0 : align == '^' ? pad / 2 : pad;
        put(fill, left);
        if (sgn) { *out++ = sgn; }
        puts(prefix); puts(body); put(fill, pad - left);
        return out;}
};
template<int N> struct std::formatter<UInt<N>> : std::formatter<InfInt> {
    template<typename Ctx> auto format(const UInt<N> &a, Ctx &ctx) const { return std::formatter<InfInt>::format(InfInt(a), ctx); }
};
