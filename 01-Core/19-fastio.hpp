#pragma once
#include "01-template.hpp"
#ifdef __AVX2__
#include <immintrin.h>
#endif
#ifdef __linux__
#include <sys/mman.h>
#include <sys/stat.h>
#endif

enum class FastIoStatus { Ok, Eof, Invalid, Overflow, Error };

namespace fastio_detail {
    // TAIL + 32 - k + len: the k-byte mask (k = 16 or 32) of a right-aligned token of len <= k bytes, 0 before it and 0xFF on it.
    alignas(64) inline constexpr char TAIL[64] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
    // T: O(1), M: O(1); whether the eight bytes of w (memory order) are all decimal digits.
    constexpr bool decWord(ulng w) { return ((w & 0xF0F0F0F0F0F0F0F0ULL) | (((w + 0x0606060606060606ULL) & 0xF0F0F0F0F0F0F0F0ULL) >> 4)) == 0x3333333333333333ULL; }
    // T: O(1), M: O(1); the value of the eight decimal digits of w (memory order, most significant first).
    constexpr ulng dec8(ulng w) {
        w = ((w & 0x0F0F0F0F0F0F0F0FULL) * 2561) >> 8;
        w = ((w & 0x00FF00FF00FF00FFULL) * 6553601) >> 16;
        return ((w & 0x0000FFFF0000FFFFULL) * 42949672960001ULL) >> 32;}
    // T: O(1), M: O(1); the value of the len (1-19) decimal digits ending at w + 32 (bytes before them ignored) and true, or false when one is not a digit.
    [[gnu::always_inline]] inline bool dec19(const char *w, int len, ulng &v) {
        bool ok = true;
#ifdef __AVX2__
        __m256i x = _mm256_blendv_epi8(_mm256_set1_epi8('0'), _mm256_loadu_si256(reinterpret_cast<const __m256i *>(w)), _mm256_loadu_si256(reinterpret_cast<const __m256i *>(TAIL + len)));
        x = _mm256_sub_epi8(x, _mm256_set1_epi8('0'));
        if (_mm256_movemask_epi8(_mm256_or_si256(x, _mm256_cmpgt_epi8(x, _mm256_set1_epi8(9))))) { return false; }
        // bytes to 2-, 4-, then 8-digit lanes; 64-bit lanes 0 and 2 hold the digits 17-24 (high half) and 9-16, 1-8 from the right
        x = _mm256_madd_epi16(_mm256_maddubs_epi16(x, _mm256_set1_epi16(0x010A)), _mm256_set1_epi32(0x00010064));
        x = _mm256_madd_epi16(_mm256_packus_epi32(x, x), _mm256_set1_epi32(0x00012710));
        ulng top = ulng(_mm256_extract_epi64(x, 0)) >> 32, low = ulng(_mm256_extract_epi64(x, 2));
        v = (top * 100000000 + (low & 0xFFFFFFFF)) * 100000000 + (low >> 32);
#else
        ulng g[3], valid = 1; v = 0;   // a word accumulator: a bool one changes GCC's scalar code in every driver
        std::memcpy(g, w + 8, 24);
        for (int k = 0; k < 3; ++k) {   // the first max(0, 24 - len - 8k) bytes of group k become '0' padding
            int pad = std::clamp(24 - len - 8 * k, 0, 8);
            ulng m = pad ? ~0ULL >> (64 - 8 * pad) : 0;
            g[k] = (g[k] & ~m) | (0x3030303030303030ULL & m);
            valid &= decWord(g[k]); v = 100000000 * v + dec8(g[k]);}
        ok = valid;
#endif
        return ok;}
    // T: O(1), M: O(1); whether the eight bytes of w (memory order) are all hex digits of either case.
    constexpr bool hexWord(ulng w) {
        constexpr ulng H = 0x8080808080808080ULL, L = 0x0101010101010101ULL;
        auto ge = [](ulng x, ulng k) { return ((x | H) - k * L) & H; };   // top bit of each byte: byte >= k (bytes below 0x80)
        ulng f = w | 0x2020202020202020ULL;
        return !(w & H) && ((ge(w, '0') & ~ge(w, '9' + 1)) | (ge(f, 'a') & ~ge(f, 'f' + 1))) == H;}
    // T: O(1), M: O(1); the value of the eight hex digits of w (memory order, most significant first), given hexWord(w).
    constexpr ulng hex8(ulng w) {
        w = (w & 0x0F0F0F0F0F0F0F0FULL) + 9 * ((w & 0x4040404040404040ULL) >> 6);
        w = ((w << 4) | (w >> 8)) & 0x00FF00FF00FF00FFULL;
        w = ((w << 8) | (w >> 16)) & 0x0000FFFF0000FFFFULL;
        return ((w << 16) | (w >> 32)) & 0xFFFFFFFF;}
    // T: O(1), M: O(1); the value of the len (1-16) hex digits (either case) ending at w + 16 (bytes before them ignored) and true, or false when one is not a hex digit.
    [[gnu::always_inline]] inline bool hex16(const char *w, int len, ulng &v) {
        bool ok = true;
#ifdef __AVX2__
        __m128i x = _mm_blendv_epi8(_mm_set1_epi8('0'), _mm_loadu_si128(reinterpret_cast<const __m128i *>(w)), _mm_loadu_si128(reinterpret_cast<const __m128i *>(TAIL + 16 + len)));
        __m128i alpha = _mm_cmpgt_epi8(x, _mm_set1_epi8('9')), d = _mm_add_epi8(_mm_and_si128(x, _mm_set1_epi8(0x0F)), _mm_and_si128(alpha, _mm_set1_epi8(9)));
        __m128i back = _mm_shuffle_epi8(_mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'), d);
        if (_mm_movemask_epi8(_mm_cmpeq_epi8(back, _mm_or_si128(x, _mm_and_si128(alpha, _mm_set1_epi8(0x20))))) != 0xFFFF) { return false; }
        d = _mm_maddubs_epi16(d, _mm_set1_epi16(0x0110));   // digit pairs to bytes, packed, then the bytes reversed
        v = __builtin_bswap64(ulng(_mm_cvtsi128_si64(_mm_packus_epi16(d, d))));
#else
        ulng g[2];
        std::memcpy(g, w, 16);
        for (int k = 0; k < 2; ++k) {   // the first max(0, 16 - len - 8k) bytes of group k become '0' padding
            int pad = std::clamp(16 - len - 8 * k, 0, 8);
            ulng m = pad ? ~0ULL >> (64 - 8 * pad) : 0;
            g[k] = (g[k] & ~m) | (0x3030303030303030ULL & m);}
        v = hex8(g[0]) << 32 | hex8(g[1]);
        ok = hexWord(g[0]) && hexWord(g[1]);
#endif
        return ok;}
    // T: O(1), M: O(1); the value lo + 2^64 hi of the len (1-32) hex digits (either case) ending at w + 32 (bytes before them ignored) and true, or
    // false when one is not a hex digit.
    [[gnu::always_inline]] inline bool hex32(const char *w, int len, ulng &lo, ulng &hi) {
        bool ok = true;
#ifdef __AVX2__
        __m256i x = _mm256_blendv_epi8(_mm256_set1_epi8('0'), _mm256_loadu_si256(reinterpret_cast<const __m256i *>(w)), _mm256_loadu_si256(reinterpret_cast<const __m256i *>(TAIL + len)));
        __m256i alpha = _mm256_cmpgt_epi8(x, _mm256_set1_epi8('9')), d = _mm256_add_epi8(_mm256_and_si256(x, _mm256_set1_epi8(0x0F)), _mm256_and_si256(alpha, _mm256_set1_epi8(9)));
        // a byte is a hex digit exactly when its value d prints back as the byte, with bit 5 set above '9' (checked over all 256 bytes)
        __m256i back = _mm256_shuffle_epi8(_mm256_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'), d);
        if (_mm256_movemask_epi8(_mm256_cmpeq_epi8(back, _mm256_or_si256(x, _mm256_and_si256(alpha, _mm256_set1_epi8(0x20))))) != -1) { return false; }
        // digit pairs to bytes, then 4-digit 16-bit lanes, then the words gathered most significant group last
        x = _mm256_madd_epi16(_mm256_maddubs_epi16(d, _mm256_set1_epi16(0x0110)), _mm256_set1_epi32(0x00010100));
        x = _mm256_permutevar8x32_epi32(_mm256_or_si256(_mm256_slli_epi32(x, 16), _mm256_srli_epi64(x, 32)), _mm256_setr_epi32(6, 4, 2, 0, 0, 0, 0, 0));
        lo = ulng(_mm256_extract_epi64(x, 0)); hi = ulng(_mm256_extract_epi64(x, 1));
#else
        ulng g[4], v[2] = {0, 0};
        std::memcpy(g, w, 32);
        for (int k = 0; k < 4; ++k) {   // the first max(0, 32 - len - 8k) bytes of group k become '0' padding
            int pad = std::clamp(32 - len - 8 * k, 0, 8);
            ulng m = pad ? ~0ULL >> (64 - 8 * pad) : 0;
            g[k] = (g[k] & ~m) | (0x3030303030303030ULL & m);
            ok &= hexWord(g[k]); v[k / 2] = v[k / 2] << 32 | hex8(g[k]);}
        hi = v[0]; lo = v[1];
#endif
        return ok;}
    // Integer tokens: the standard integer types but bool, and the 128-bit ones by name (strict modes do not count them as integral).
    template<typename T> concept Int = (std::is_integral_v<T> && !std::is_same_v<T, bool>) || std::is_same_v<T, lll> || std::is_same_v<T, ulll>;
    inline constexpr array<ulng, 20> POW10 = []() { array<ulng, 20> p{1}; for (int i = 1; i < 20; ++i) { p[i] = 10 * p[i - 1]; } return p; }();
    inline constexpr char DIGITS2[201] = "00010203040506070809101112131415161718192021222324252627282930313233343536373839404142434445464748495051525354555657585960616263646566676869707172737475767778798081828384858687888990919293949596979899";
    inline constexpr ulng E19 = 10000000000000000000ULL, E16 = 10000000000000000ULL;
    // T: O(1), M: O(1); the decimal digits of v (one for zero): log10(2) > 1233 / 2^12, so one power settles it.
    constexpr int decLen(ulng v) { int t = std::bit_width(v) * 1233 >> 12; return t + (v >= POW10[size_t(t)]) + !v; }
    // T: O(1), M: O(1); the eight decimal digits of v < 10^8 as bytes in memory order: 4 + 4, 2 + 2 and 1 + 1 digits split in lanes by exact reciprocal products.
    [[gnu::always_inline]] inline ulng dec8Chars(uint v) {
        ulng x = v / 10000 | ulng(v % 10000) << 32;
        ulng q = (x * 10486 >> 20) & 0x0000007F0000007FULL; x = q | (x - 100 * q) << 16;
        q = (x * 103 >> 10) & 0x000F000F000F000FULL; x = q | (x - 10 * q) << 8;
        return x + 0x3030303030303030ULL;}
#ifdef __AVX2__
    // T: O(1), M: O(1); the 16 decimal digits of hi * 10^8 + lo (both below 10^8) as bytes: dec8Chars in two 64-bit lanes.
    [[gnu::always_inline]] inline __m128i dec16Chars(uint hi, uint lo) {
        __m128i x = _mm_set_epi64x(lng(lo), lng(hi)), q4 = _mm_srli_epi64(_mm_mul_epu32(x, _mm_set1_epi64x(0xD1B71759)), 45);
        __m128i y = _mm_or_si128(q4, _mm_slli_epi64(_mm_sub_epi64(x, _mm_mul_epu32(q4, _mm_set1_epi64x(10000))), 32));
        __m128i h = _mm_srli_epi16(_mm_mulhi_epu16(y, _mm_set1_epi32(5243)), 3);
        __m128i z = _mm_or_si128(h, _mm_slli_epi32(_mm_sub_epi16(y, _mm_mullo_epi16(h, _mm_set1_epi32(100))), 16));
        __m128i d = _mm_srli_epi16(_mm_mullo_epi16(z, _mm_set1_epi16(103)), 10);
        return _mm_add_epi8(_mm_or_si128(d, _mm_slli_epi16(_mm_sub_epi16(z, _mm_mullo_epi16(d, _mm_set1_epi16(10))), 8)), _mm_set1_epi8('0'));}
#endif
    // T: O(1), M: O(1); the decimal digits of v at o (one for zero) by stores into [o, o + 24) and no branch on the length; returns the end.
    [[gnu::always_inline]] inline char *writeDec(char *o, ulng v) {
        if (v < 100) { std::memcpy(o, DIGITS2 + 2 * v + (v < 10), 2); return o + 1 + (v >= 10); }
        if (v < 100000000) { ulng g = dec8Chars(uint(v)); int z = std::countr_zero((g ^ 0x3030303030303030ULL) | 1ULL << 56) >> 3; g >>= 8 * z; std::memcpy(o, &g, 8); return o + 8 - z; }
        ulng r = v % E16;
        int len = decLen(v), z = 20 - len;   // leading zeros of the 20 digits
#ifdef __AVX2__
        auto pos = [](int a) { return a & ~(a >> 31); };   // max(a, 0) by a mask: GCC compiles a selection to branches
        alignas(32) static constexpr char SHIFT[32] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
        uint top = uint(dec8Chars(uint(v / E16)) >> 32) >> (8 * (z - pos(z - 3)));   // the top four digits from the first nonzero one
        std::memcpy(o, &top, 4);
        _mm_storeu_si128(reinterpret_cast<__m128i *>(o + pos(4 - z)), _mm_shuffle_epi8(dec16Chars(uint(r / 100000000), uint(r % 100000000)), _mm_loadu_si128(reinterpret_cast<const __m128i *>(SHIFT + pos(z - 4)))));
#else
        ulng g[5] = {dec8Chars(uint(v / E16)), dec8Chars(uint(r / 100000000)), dec8Chars(uint(r % 100000000)), 0, 0};
        int s = 8 * ((z + 4) & 7);
        const ulng *a = g + (z + 4) / 8;   // the words from the first digit's, indexed rather than selected
        ulng w0 = ulng((ulll(a[1]) << 64 | a[0]) >> s), w1 = ulng((ulll(a[2]) << 64 | a[1]) >> s), w2 = a[2] >> s;
        std::memcpy(o, &w0, 8); std::memcpy(o + 8, &w1, 8); std::memcpy(o + 16, &w2, 8);   // three stores: one built through memory would block forwarding
#endif
        return o + len;}
    // T: O(1), M: O(1); the 19 decimal digits of v < 10^19 at o, leading zeros included, by stores into [o, o + 19).
    inline char *dec19Chars(char *o, ulng v) {
        ulng q = v / E16, r = v - q * E16;
        uint top = uint(dec8Chars(uint(q)) >> 40);
        std::memcpy(o, &top, 4);
#ifdef __AVX2__
        _mm_storeu_si128(reinterpret_cast<__m128i *>(o + 3), dec16Chars(uint(r / 100000000), uint(r % 100000000)));
#else
        ulng a = dec8Chars(uint(r / 100000000)), b = dec8Chars(uint(r % 100000000));
        std::memcpy(o + 3, &a, 8); std::memcpy(o + 11, &b, 8);
#endif
        return o + 19;}
    // T: O(1), M: O(1); the decimal digits of v at o (one for zero) by stores into [o, o + 40); v >= 2^64 splits at 10^19 by a Moller-Granlund reciprocal.
    inline char *writeDec(char *o, ulll v) {
        constexpr ulng INV = ulng(((ulll(~E19) << 64) | ~0ULL) / E19);
        constexpr ulll E38 = ulll(E19) * E19;
        if (!(v >> 64)) { return writeDec(o, ulng(v)); }
        bool full = v >= E38;   // 39 digits: the top one apart, then 38 in two padded blocks
        if (full) { int top = 1 + (v >= 2 * E38) + (v >= 3 * E38); *o++ = char('0' + top); v -= ulll(top) * E38; }
        ulng hi = ulng(v >> 64), lo = ulng(v);   // hi < 10^19: one two-by-one step
        ulll t = ulll(INV) * hi + v;
        ulng q = ulng(t >> 64) + 1, r = lo - q * E19;
        ulng m = ulng(0) - ulng(r > ulng(t)); q += m; r += m & E19;   // the likely correction by a mask: GCC branches on it, mispredicted about half the time
        if (r >= E19) { ++q; r -= E19; }
        if (full) { o = dec19Chars(o, q); }
        else { o = writeDec(o, q); }
        return dec19Chars(o, r);}
    // T: O(1), M: O(1); the 16 hex digits of v at o (uppercase).
    [[gnu::always_inline]] inline char *hex16Chars(char *o, ulng v) {
#ifdef __AVX2__
        __m128i x = _mm_cvtsi64_si128(lng(__builtin_bswap64(v))), m = _mm_set1_epi8(0x0F);
        x = _mm_unpacklo_epi8(_mm_and_si128(_mm_srli_epi16(x, 4), m), _mm_and_si128(x, m));
        _mm_storeu_si128(reinterpret_cast<__m128i *>(o), _mm_shuffle_epi8(_mm_setr_epi8('0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'), x));
#else
        auto half = [](uint h) {   // nibbles spread to bytes, then '0' + d plus 7 above 9
            ulng w = h;
            w = (w | (w << 16)) & 0x0000FFFF0000FFFFULL;
            w = (w | (w << 8)) & 0x00FF00FF00FF00FFULL;
            w = (w | (w << 4)) & 0x0F0F0F0F0F0F0F0FULL;
            return __builtin_bswap64(w + 0x3030303030303030ULL + 7 * (((w + 0x3636363636363636ULL) & 0x4040404040404040ULL) >> 6));};
        ulng a = half(uint(v >> 32)), b = half(uint(v));
        std::memcpy(o, &a, 8); std::memcpy(o + 8, &b, 8);
#endif
        return o + 16;}
    // T: O(1), M: O(1); the hex digits of v at o (one for zero) by stores into [o, o + 16); returns the end.
    [[gnu::always_inline]] inline char *writeHex(char *o, ulng v) {
        int top = 16 - (std::countl_zero(v | 1) >> 2);   // a shift: GCC compiles this division to idiv
        hex16Chars(o, v << (64 - 4 * top)); return o + top;}
    // T: O(1), M: O(1); the same for v < 2^128 by stores into [o, o + 32): the top word shifted to its first digit, then the low word (past the end for one word).
    [[gnu::always_inline]] inline char *writeHex(char *o, ulll v) {
        ulng lo = ulng(v), hi = ulng(v >> 64), two = ulng(0) - (hi != 0), w = (hi & two) | (lo & ~two);   // the top word by a mask: a selection compiles to a branch
        int top = 16 - (std::countl_zero(w | 1) >> 2);
        hex16Chars(o, w << (64 - 4 * top)); hex16Chars(o + top, lo);
        return o + top + (two & 16);}
    // T: O(b), M: O(1); the digits of v in base B (2-36) at o (uppercase, one for zero), by stores into [o, o + 128); returns the end.
    template<uint B, typename U>
    [[gnu::noinline]] char *writeBase(char *o, U v) {
        char s[128]; int n = 128;
        do { s[--n] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ"[int(v % B)]; v /= B; } while (v);
        std::memcpy(o, s + n, size_t(128 - n)); return o + 128 - n;}
    inline constexpr size_t HUGE_PAGE = size_t(1) << 21;
    // T: O(1), M: O(1); frees ::operator new storage of alignment a.
    struct AlignedFree { size_t a; void operator()(char *p) const { ::operator delete(p, std::align_val_t(a)); } };
    // Text-number types: read from one token in a base (InfInt, InfIntMini, Rational) and written into a caller buffer (InfInt); Exact: T{v} without
    // narrowing, so assigning v keeps its value.
    template<typename T, typename I> concept Exact = requires(T &x, I v) { T{v}; x = v; };
    template<typename T> concept TextIn = requires(T &x, string_view s, uint base) { { x.read(s, base) } -> std::same_as<bool>; };
    template<typename T> concept TextOut = requires(const T &x, char *o, uint base) { { x.charsBound(base) } -> std::convertible_to<int>; { x.toChars(o, o, base) } -> std::same_as<std::to_chars_result>; };
} // namespace fastio_detail

// T: O(n) for n bytes read, M: O(N) plus O(L) per token crossing a refill, or for N = 0 the whole stream (a private mapping of a regular file on Linux); borrowed byte FILE*.
// Whole [+/-] tokens in base B (a template argument, 10 by default), readDouble from_chars-exact; failure leaves x unchanged with Error > Invalid > Overflow; N = 1 for interactive pipes.
template<int N = 1 << 16>
struct FastInput {
    static_assert(N >= 0);
    std::FILE *file;
    array<uint8_t, N> buf;
    const uint8_t *b0 = buf.data(), *p = b0, *e = b0;
    string tok;
    char pad[64];
    vector<uint8_t> all;
    size_t mapped = 0;
    size_t held = 0;   // N = 0 after a read error: the trailing nonspace bytes, past e until fill() reaches them, so a token they end reports the error
    bool cut = false;   // N = 0: the load stopped at a read error
    FastIoStatus status = FastIoStatus::Ok;

    explicit FastInput(std::FILE *f = stdin) : file(f) {
        assert(file);
        if constexpr (N == 0) { load(); }}
    FastInput(const FastInput &) = delete;
    FastInput &operator=(const FastInput &) = delete;
    FastInput(FastInput &&) = delete;
    FastInput &operator=(FastInput &&) = delete;
#ifdef __linux__
    ~FastInput() { if (mapped) { munmap(const_cast<uint8_t *>(b0), mapped); } }
#endif
    // The whole stream for N = 0: a private mapping of a regular file from its current offset (Linux), else everything fread returns.
    void load() {
#ifdef __linux__
        struct stat st;
        if (fstat(fileno(file), &st) == 0 && S_ISREG(st.st_mode)) {
            lng at = std::ftell(file), len = st.st_size;
            void *m = at >= 0 && at < len ? mmap(nullptr, size_t(len), PROT_READ, MAP_PRIVATE | MAP_POPULATE, fileno(file), 0) : MAP_FAILED;
            if (m != MAP_FAILED) { mapped = size_t(len); b0 = static_cast<const uint8_t *>(m); p = b0 + at; e = b0 + len; return; }}
#endif
        size_t len = 0;
        while (!std::feof(file) && !std::ferror(file)) {
            all.resize(max(2 * all.size(), size_t(1) << 16));
            len += std::fread(all.data() + len, 1, all.size() - len, file);}
        if ((cut = std::ferror(file))) { while (held < len && !space(all[len - 1 - held])) { ++held; }}
        b0 = p = all.data(); e = p + len - held;}

    static bool space(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
#ifdef __AVX2__
    // Bit i: p[i] (i < 32) is whitespace: the byte equals its low nibble's entry in a table of the six (a byte from 0x80 looks up zero).
    static uint spaceMask(const uint8_t *p) {
        __m256i x = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(p));
        __m256i ws = _mm256_setr_epi8(' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, '\t', '\n', '\v', '\f', '\r', 0, 0);
        return uint(_mm256_movemask_epi8(_mm256_cmpeq_epi8(_mm256_shuffle_epi8(ws, x), x)));}
#endif
    // The bytes 1 KiB ahead of p into the cache, by an integer address: it may lie past the buffer, and a prefetch never faults.
    static void prefetch(const uint8_t *p) { __builtin_prefetch(reinterpret_cast<const void *>(reinterpret_cast<std::uintptr_t>(p) + 1024)); }
    // The first whitespace byte in [p, e), else e.
    static const uint8_t *tokenEnd(const uint8_t *p, const uint8_t *e) {
#ifdef __AVX2__
        for (; e - p >= 64; p += 64) {   // two loads per test
            prefetch(p);
            if (ulng m = spaceMask(p) | ulng(spaceMask(p + 32)) << 32) { return p + std::countr_zero(m); }}
        for (; e - p >= 32; p += 32) {
            if (uint m = spaceMask(p)) { return p + std::countr_zero(m); }}
#else
        for (ulng x; e - p >= 8; p += 8) {   // bytes below '!' flagged from the lowest one up (control bytes that are data resume byte by byte)
            prefetch(p); std::memcpy(&x, p, 8);
            if (ulng m = (x - 0x2121212121212121ULL) & ~x & 0x8080808080808080ULL) { p += std::countr_zero(m) / 8; break; }}
#endif
        while (p < e && !space(*p)) { ++p; }
        return p;}
    // Whether unread bytes remain, reading the next block (N > 0) when none is buffered; sets status.
    bool fill() {
        if (p == e) {
            if constexpr (N > 0) {
                if (!std::ferror(file) && !std::feof(file)) { p = buf.data(); e = p + std::fread(buf.data(), 1, N, file); }}
            else { size_t k = min(held, size_t(31)); e += k; held -= k; }   // at most 31 at a time: readToken's 32-byte path never sees them
            if (p == e) { status = std::ferror(file) ? FastIoStatus::Error : FastIoStatus::Eof; return false; }}
        status = FastIoStatus::Ok; return true;}
    int peek() { return fill() ? *p : EOF; }
    int get() { return fill() ? *p++ : EOF; }
    bool skipSpace() {
        while (fill()) {
            const uint8_t *q = p;   // a local cursor: stores through the member would alias the bytes read
            while (q < e && space(*q)) { ++q; }
            p = q;
            if (q < e) { return true; }}
        return false;}

    bool readChar(char &x) {
        if (!skipSpace()) { return false; }
        x = char(*p++); return true;}
    // The view stays valid until the next read; it points into the input unless the token crossed a refill.
    [[gnu::always_inline]] bool readToken(string_view &x) {
#ifdef __AVX2__
        if (e - p >= 32) {   // the leading whitespace within one 32-byte load: the token ends in it or is scanned on from its end
            uint ws = spaceMask(p), end = ws & (ws + 1);   // ws without its trailing ones: the whitespace after the token
            if (~ws) {
                const uint8_t *s = p + std::countr_one(ws), *t = end ? p + std::countr_zero(end) : tokenEnd(p + 32, e);
                if (N == 0 || t < e) { p = t; x = {reinterpret_cast<const char *>(s), size_t(t - s)}; status = FastIoStatus::Ok; return true; }}}
#endif
        string_view s = tokenRest();   // empty on failure: a token has a byte
        if (s.empty()) { return false; }
        x = s; return true;}
    // The rest of readToken, out of line: a load of whitespace only, the last 31 bytes, a token reaching the end of a block, refills.
    [[gnu::noinline]] string_view tokenRest() {
        if (!skipSpace()) { return {}; }
        const uint8_t *s = p; p = tokenEnd(p, e);
        if (N == 0 && p == e && cut) { p = e += std::exchange(held, 0); status = FastIoStatus::Error; return {}; }   // a token a read error cut, consumed whole
        if (N == 0 || p < e) { return {reinterpret_cast<const char *>(s), size_t(p - s)}; }
        tok.assign(s, p);
        while (fill()) {
            s = p; p = tokenEnd(p, e); tok.append(s, p);
            if (p < e) { break; }}
        if (status == FastIoStatus::Error) { return {}; }
        status = FastIoStatus::Ok; return tok;}
    bool readToken(string &x) {
        string_view s;
        if (!readToken(s)) { return false; }
        x.assign(s); return true;}
    // The 64 bytes ending at the end of the token s (at most 64 bytes) from the buffer holding it, or s right-aligned in pad when it starts closer.
    const char *window(string_view s) {
        const char *lo = N > 0 && s.data() == tok.data() ? tok.data() : reinterpret_cast<const char *>(b0), *t = s.data() + s.size();
        return t - lo >= 64 ? t - 64 : padded(s);}
    [[gnu::noinline]] const char *padded(string_view s) {
        std::memset(pad, '0', 64); std::memcpy(pad + 64 - s.size(), s.data(), s.size());
        return pad;}
    bool readLine(string &x) {
        if (!fill()) { return false; }
        string s;
        while (fill()) {
            auto *nl = static_cast<const uint8_t *>(std::memchr(p, '\n', size_t(e - p)));
            s.append(p, nl ? nl : e); p = nl ? nl + 1 : e;
            if (nl) { break; }}
        if (status == FastIoStatus::Error) { return false; }
        if (!s.empty() && s.back() == '\r') { s.pop_back(); }
        x = std::move(s); status = FastIoStatus::Ok; return true;}

    // [+/-] digits in base B (2-36, either case); 1 to 19 decimal or 1 to 32 hex digits in one window, the rest out of line.
    template<int B = 10, fastio_detail::Int T>
    [[gnu::always_inline]] bool readInt(T &x) {
        string_view s;
        return readToken(s) && parseInt<B>(s, x);}
    // The value of a token s that readToken returned, while s is valid (the window before its end is then in the buffer or padded).
    template<int B = 10, fastio_detail::Int T>
    [[gnu::always_inline]] bool parseInt(string_view s, T &x) {
        static_assert(B >= 2 && B <= 36);
        status = FastIoStatus::Ok;
        using U = std::conditional_t<(sizeof(T) > 8), ulll, ulng>;
        constexpr bool SIGNED = T(-1) < T(0);
        constexpr U MAX = (~U(0) >> (8 * (sizeof(U) - sizeof(T)))) >> SIGNED;
        const char *t = s.data() + s.size(), *lo = N > 0 && s.data() == tok.data() ? tok.data() : reinterpret_cast<const char *>(b0);
        int neg = 0, len = int(s.size());
        if constexpr (SIGNED) { neg = s[0] == '-'; len -= neg || s[0] == '+'; }   // a sign of unsigned T fails the kernels: out of line
        ulng v, hi = 0;
        if (t - lo >= 32 && (!SIGNED || len >= 1) && ((B == 10 && len <= 19 && fastio_detail::dec19(t - 32, len, v))
                || (B == 16 && len <= int(2 * sizeof(U)) && (sizeof(U) > 8 ? fastio_detail::hex32(t - 32, len, v, hi) : fastio_detail::hex16(t - 16, len, v))))) {
            U u = v, m = U(0) - U(neg);   // the sign applied as a mask
            if constexpr (sizeof(U) > 8) { u |= U(hi) << 64; }
            if (u > MAX + U(neg)) { status = FastIoStatus::Overflow; return false; }
            x = T((u ^ m) - m); return true;}
        return intRest<B>(x, s);}
    // The rest of readInt, out of line: tokens near the buffer start, 20 to 38 decimal digits for 128 bits, other bases and longer tokens.
    template<int B, fastio_detail::Int T>
    [[gnu::noinline]] bool intRest(T &x, string_view s) {
        using U = std::conditional_t<(sizeof(T) > 8), ulll, ulng>;
        constexpr bool SIGNED = T(-1) < T(0);
        constexpr U MAX = (~U(0) >> (8 * (sizeof(U) - sizeof(T)))) >> SIGNED;
        int neg = s[0] == '-', len = int(s.size()) - (neg || s[0] == '+');
        U lim = MAX + U(neg), v = 0;
        bool bad = !len || (neg && !SIGNED), over = false;
        ulng a, b = 0;
        const char *w = s.size() <= 64 ? window(s) + 32 : nullptr;
        if (bad) {}
        else if (w && B == 10 && len <= 19 && fastio_detail::dec19(w, len, a)) { v = a; over = v > lim; }
        else if (w && B == 10 && sizeof(U) > 8 && len <= 38 && fastio_detail::dec19(w, 19, a) && fastio_detail::dec19(w - 19, len - 19, b)) {
            v = U(b) * fastio_detail::E19 + a; over = v > lim;}   // below 10^38 < 2^127
        else if (w && B == 16 && len <= int(2 * sizeof(U)) && fastio_detail::hex32(w, len, a, b)) {
            v = a;
            if constexpr (sizeof(U) > 8) { v |= U(b) << 64; }
            over = v > lim;}
        else {   // digit by digit up to an overflow, then only validity (eight bytes at a time in base 10 and 16)
            U q = lim / uint(B), r = lim % uint(B);
            for (const char *c = s.data() + s.size() - len, *t = s.data() + s.size(); c < t && !bad; ++c) {
                if (ulng g; over && t - c >= 8 && (B == 10 || B == 16)) {
                    std::memcpy(&g, c, 8); c += 7;
                    bad = !(B == 10 ? fastio_detail::decWord(g) : fastio_detail::hexWord(g)); continue;}
                uint l = uint((uint8_t(*c) | 0x20) - 'a'), d = uint8_t(*c) <= '9' ? uint(uint8_t(*c) - '0') : l < 26 ? l + 10 : 36;   // bytes below '0' wrap past 36
                if (d >= uint(B)) { bad = true; }
                else if (v > q || (v == q && d > r)) { over = true; }
                else if (!over) { v = v * uint(B) + d; }}}
        status = bad ? FastIoStatus::Invalid : over ? FastIoStatus::Overflow : FastIoStatus::Ok;
        if (status != FastIoStatus::Ok) { return false; }
        U m = U(0) - U(neg);
        x = T((v ^ m) - m); return true;}
    template<typename T> requires std::is_same_v<T, float> || std::is_same_v<T, double>
    bool readDouble(T &x) {
        string_view s;
        if (!readToken(s)) { return false; }
        const char *b = s.data(), *t = b + s.size();
        T v = 0;
        if (*b == '+' && (++b == t || *b == '-')) { status = FastIoStatus::Invalid; return false; }
        auto [q, ec] = std::from_chars(b, t, v);
        status = ec == std::errc::invalid_argument || q != t ? FastIoStatus::Invalid
               : ec == std::errc::result_out_of_range ? FastIoStatus::Overflow : FastIoStatus::Ok;
        if (status != FastIoStatus::Ok) { return false; }
        x = v; return true;}
    template<int B = 10, fastio_detail::TextIn T>
    [[gnu::always_inline]] bool readText(T &x) {
        static_assert(B >= 2 && B <= 36);
        string_view s;
        if (!readToken(s)) { return false; }
        if constexpr (fastio_detail::Exact<T, lng>) {   // decimal or hex integers below 2^63 (2^127 when T takes lll exactly) by assignment, which read must agree with
            constexpr bool wide = fastio_detail::Exact<T, lll>;
            const char *t = s.data() + s.size(), *lo = N > 0 && s.data() == tok.data() ? tok.data() : reinterpret_cast<const char *>(b0);
            int neg = s[0] == '-', len = int(s.size()) - (neg || s[0] == '+');
            lng m = -lng(neg);   // the sign applied as a mask: a branch on it would be mispredicted
            ulng v, hi;
            if (t - lo >= 64 && len >= 1) {   // 32 bytes before the token's end in the buffer holding it (64 for the two decimal windows)
                if (B == 16 && len <= (wide ? 31 : 15) && fastio_detail::hex32(t - 32, len, v, hi)) {
                    if constexpr (wide) { x = ((lll(hi) << 64 | v) ^ m) - m; }
                    else { x = (lng(v) ^ m) - m; }
                    return true;}
                if (B == 10 && len <= 18 && fastio_detail::dec19(t - 32, len, v)) { x = (lng(v) ^ m) - m; return true; }}}
        return textRest<B>(x, s);}
    // The rest of readText, out of line: tokens near the buffer start, 19 to 38 decimal digits, other bases, and x.read.
    template<int B, fastio_detail::TextIn T>
    [[gnu::noinline]] bool textRest(T &x, string_view s) {
        if constexpr (fastio_detail::Exact<T, lng>) {
            constexpr bool wide = fastio_detail::Exact<T, lll>;
            int neg = s[0] == '-', len = int(s.size()) - (neg || s[0] == '+');
            lng m = -lng(neg);
            ulng v, hi;
            const char *w = window(s) + 32;
            if (B == 16 && len >= 1 && len <= (wide ? 31 : 15) && fastio_detail::hex32(w, len, v, hi)) {
                if constexpr (wide) { x = ((lll(hi) << 64 | v) ^ m) - m; }
                else { x = (lng(v) ^ m) - m; }
                return true;}
            if (B == 10 && len >= 1 && len <= 18 && fastio_detail::dec19(w, len, v)) { x = (lng(v) ^ m) - m; return true; }
            if constexpr (wide) {   // 19 to 38 decimal digits: two windows
                if (B == 10 && len > 18 && len <= 38 && fastio_detail::dec19(w, 19, v) && fastio_detail::dec19(w - 19, len - 19, hi)) {
                    x = (lll(ulll(hi) * 10000000000000000000ULL + v) ^ m) - m; return true;}}}
        if (!x.read(s, uint(B))) { status = FastIoStatus::Invalid; return false; }
        return true;}
    template<int B = 10, typename T>
    [[gnu::always_inline]] bool read(T &x) {
        if constexpr (std::is_same_v<T, char>) { return readChar(x); }
        else if constexpr (std::is_same_v<T, string> || std::is_same_v<T, string_view>) { return readToken(x); }
        else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) { return readDouble(x); }
        else if constexpr (fastio_detail::Int<T>) { return readInt<B>(x); }
        else if constexpr (fastio_detail::TextIn<T>) { return readText<B>(x); }
        else if constexpr (std::ranges::range<T>) {
            for (auto &y : x) {
                if (!read<B>(y)) { return false; }}
            return true;}
        else { return std::apply([&](auto &...ys) { return (read<B>(ys) && ...); }, x); }}
    template<int B = 10, typename ...Ts> requires (sizeof...(Ts) > 1)
    [[gnu::always_inline]] bool read(Ts &...xs) { return (read<B>(xs) && ...); }
};

// T: O(n) for n bytes written, M: O(N); borrowed byte FILE*, sticky errors, flush before an interactive read.
// Integers and text in base B (a template argument, 10 by default); writeDouble/print use to_chars fixed with 0 <= precision <= 200; print separates by spaces, ends with '\n'.
template<int N = 1 << 16>
struct FastOutput {
    static_assert(N > 0);
    std::FILE *file;
    array<char, N> buf;
    int pos = 0, precision = 15;
    bool error = false, pending = false;

    explicit FastOutput(std::FILE *f = stdout) : file(f) { assert(file); }
    FastOutput(const FastOutput &) = delete;
    FastOutput &operator=(const FastOutput &) = delete;
    FastOutput(FastOutput &&) = delete;
    FastOutput &operator=(FastOutput &&) = delete;
    ~FastOutput() { if (pos || pending) { flush(); } }

    bool drain() {
        if (error) { return false; }
        int n = pos; pos = 0;
        pending |= n != 0;
        if (n && (std::fwrite(buf.data(), 1, n, file) != size_t(n) || std::ferror(file))) {
            error = true;}
        return !error;}
    bool flush() {
        if (!drain()) { return false; }
        if (std::fflush(file) == EOF) { error = true; }
        else { pending = false; }
        return !error;}

    bool put(char c) {
        if (error || (pos == N && !drain())) { return false; }
        buf[pos++] = c; return true;}
    bool write(string_view s, char end = '\0') {
        if (error || (s.size() > size_t(N - pos) && !drain())) { return false; }
        if (s.size() >= size_t(N)) {   // a block of at least N bytes goes to the stream directly
            pending = true;
            if (std::fwrite(s.data(), 1, s.size(), file) != s.size() || std::ferror(file)) { error = true; return false; }}
        else if (!s.empty()) { std::memcpy(buf.data() + pos, s.data(), s.size()); pos += int(s.size()); }
        return !end || put(end);}
    // x in base B (2-36, uppercase): decimal and hex by the wide kernels, other bases digit by digit.
    template<int B = 10, fastio_detail::Int T>
    [[gnu::always_inline]] bool writeInt(T x, char end = '\0') {
        constexpr int W = 160;   // a sign, 128 binary digits and the kernels' stores past the end
        if constexpr (N < W) {
            char s[W];
            return write(string_view(s, size_t(intText<B>(s, x, end) - s)));}
        else {
            if (error || (N - pos < W && !drain())) { return false; }
            pos = int(intText<B>(buf.data() + pos, x, end) - buf.data()); return true;}}
    // The text of x and end (none for '\0') at o; returns the end.
    template<int B, fastio_detail::Int T>
    [[gnu::always_inline]] char *intText(char *o, T x, char end) const {
        static_assert(B >= 2 && B <= 36);
        using U = std::conditional_t<(sizeof(T) > 8), ulll, ulng>;
        U v = U(x);
        if constexpr (T(-1) < T(0)) {
            U m = U(0) - U(x < 0);
            *o = '-'; o += x < 0; v = (v ^ m) - m;}
        if constexpr (B == 10) { o = fastio_detail::writeDec(o, v); }
        else if constexpr (B == 16) { o = fastio_detail::writeHex(o, v); }
        else { o = fastio_detail::writeBase<uint(B)>(o, v); }
        *o = end; return o + (end != '\0');}
    bool writeDouble(double x, int p, char end = '\0') {
        assert(0 <= p && p <= 200);
        char s[512];
        auto r = std::to_chars(s, s + 512, x, std::chars_format::fixed, p);
        return write(string_view(s, r.ptr - s), end);}
    template<typename T> requires std::is_same_v<T, long double>
    bool writeDouble(T, int, char = '\0') = delete;
    template<int B = 10, fastio_detail::TextOut T>
    [[gnu::always_inline]] bool writeText(const T &x) {
        static_assert(B >= 2 && B <= 36);
        if (!error) {   // the text straight into the rest of the buffer (toChars refuses longer text before converting it)
            auto [q, ec] = x.toChars(buf.data() + pos, buf.data() + N, uint(B));
            if (ec == std::errc()) { pos = int(q - buf.data()); return true; }}
        return textRest<B>(x);}
    // The rest of writeText, out of line: a drain when the bound is short, and text longer than the buffer through a temporary.
    template<int B, fastio_detail::TextOut T>
    [[gnu::noinline]] bool textRest(const T &x) {
        int k = x.charsBound(uint(B));
        if (error || (k > N - pos && !drain())) { return false; }
        if (k <= N) { pos = int(x.toChars(buf.data() + pos, buf.data() + N, uint(B)).ptr - buf.data()); return true; }
        size_t a = size_t(k) >= fastio_detail::HUGE_PAGE / 4 ? fastio_detail::HUGE_PAGE : 64, n = (size_t(k) + a - 1) & ~(a - 1);   // from 512 KiB whole 2 MiB pages, advised huge: one fault per 2 MiB
        std::unique_ptr<char[], fastio_detail::AlignedFree> s(static_cast<char *>(::operator new(n, std::align_val_t(a))), {a});
#ifdef __linux__
        if (a == fastio_detail::HUGE_PAGE) { madvise(s.get(), n, MADV_HUGEPAGE); }
#endif
        return write(string_view(s.get(), size_t(x.toChars(s.get(), s.get() + k, uint(B)).ptr - s.get())));}
    template<int B = 10, typename T>
    bool writeValue(const T &x) {
        if constexpr (std::is_same_v<T, char>) { return put(x); }
        else if constexpr (std::is_same_v<T, bool>) { return put(char('0' + x)); }
        else if constexpr (std::is_convertible_v<const T &, string_view>) { return write(x); }
        else if constexpr (std::is_floating_point_v<T>) { return writeDouble(x, precision); }
        else if constexpr (fastio_detail::Int<T>) { return writeInt<B>(x); }
        else if constexpr (fastio_detail::TextOut<T>) { return writeText<B>(x); }
        else if constexpr (std::ranges::range<T>) {
            bool first = true;
            for (const auto &y : x) {
                if (!(first || put(' ')) || !writeValue<B>(y)) { return false; }
                first = false;}
            return true;}
        else {
            bool first = true;
            return std::apply([&](const auto &...ys) { return (((std::exchange(first, false) || put(' ')) && writeValue<B>(ys)) && ...); }, x);}}
    template<int B = 10, typename ...Ts>
    bool print(const Ts &...xs) {
        if constexpr (sizeof...(Ts) > 0 && ((fastio_detail::Int<Ts> && !std::is_same_v<Ts, char>) && ...) && N >= 160 * int(sizeof...(Ts))) {   // integers alone (char goes to put): one room check, then the texts and separators through a local cursor
            if (error || (N - pos < 160 * int(sizeof...(Ts)) && !drain())) { return false; }
            char *o = buf.data() + pos; int k = 0;
            ((o = intText<B>(o, xs, ++k < int(sizeof...(Ts)) ? ' ' : '\n')), ...);
            pos = int(o - buf.data()); return true;}
        [[maybe_unused]] bool first = true;
        return (((std::exchange(first, false) || put(' ')) && writeValue<B>(xs)) && ...) && put('\n');}
};
