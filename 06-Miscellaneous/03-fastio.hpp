#pragma once
#include "../01-Core/01-template.hpp"

enum class FastIoStatus { Ok, Eof, Invalid, Overflow, Error };

// T: O(bytes read), M: O(N); readToken additionally stores O(token length).
// Borrowed byte-oriented FILE*, kept open and exclusively read until destruction.
// ASCII whitespace; integers are whole [+/-]decimal tokens, excluding bool.
// Unsigned negative tokens (including -0) are invalid. On failure destination is
// unchanged; bad tokens are consumed, with Error > Invalid > Overflow precedence.
// EOF after a complete token succeeds. get/peek return unsigned bytes or EOF.
// Bulk fread may wait for more input on pipes: use N=1 for interactive input.
template<int N = 1 << 16>
struct FastInput {
    static_assert(N > 0);
    std::FILE *file;
    array<unsigned char, N> buf;
    int pos = 0, len = 0;
    FastIoStatus status = FastIoStatus::Ok;

    explicit FastInput(std::FILE *file = stdin) : file(file) { assert(file); }
    FastInput(const FastInput &) = delete;
    FastInput &operator=(const FastInput &) = delete;
    FastInput(FastInput &&) = delete;
    FastInput &operator=(FastInput &&) = delete;

    static bool space(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
    int peek() {
        if (pos == len) {
            if (std::ferror(file) || std::feof(file)) {
                status = std::ferror(file) ? FastIoStatus::Error : FastIoStatus::Eof;
                return EOF; }
            pos = 0; len = int(std::fread(buf.data(), 1, N, file));
            if (!len) {
                status = std::ferror(file) ? FastIoStatus::Error : FastIoStatus::Eof;
                return EOF; }}
        status = FastIoStatus::Ok; return buf[pos]; }
    int get() { int c = peek(); if (c != EOF) { ++pos; } return c; }
    bool skipSpace() {
        while (space(peek())) { ++pos; }
        return status == FastIoStatus::Ok; }
    bool readChar(char &x) {
        if (!skipSpace()) { return false; }
        x = char(get()); return true; }
    bool readToken(string &x) {
        if (!skipSpace()) { return false; }
        string s;
        for (int c = peek(); c != EOF && !space(c); c = peek()) {
            s.push_back(char(c)); ++pos; }
        if (status == FastIoStatus::Error) { return false; }
        x = std::move(s); status = FastIoStatus::Ok; return true; }

    template<typename T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    bool readInt(T &x) {
        if (!skipSpace()) { return false; }
        using U = std::make_unsigned_t<T>;
        int c = peek(); bool neg = c == '-';
        if (neg || c == '+') { ++pos; c = peek(); }
        U lim = U(std::numeric_limits<T>::max()) + U(neg && std::is_signed_v<T>);
        U q = lim / 10, r = lim % 10, v = 0;
        bool digit = false, bad = neg && !std::is_signed_v<T>, overflow = false;
        for (; c != EOF && !space(c); c = peek()) {
            if (c < '0' || c > '9') { bad = true; }
            else {
                digit = true; U d = U(c - '0');
                if (v > q || (v == q && d > r)) { overflow = true; }
                else if (!overflow) { v = U(10 * v + d); }}
            ++pos; }
        if (status == FastIoStatus::Error) { return false; }
        status = bad || !digit ? FastIoStatus::Invalid : overflow ? FastIoStatus::Overflow : FastIoStatus::Ok;
        if (status != FastIoStatus::Ok) { return false; }
        if constexpr (std::is_signed_v<T>) {
            x = neg ? T(-T(v - (v != 0)) - (v != 0)) : T(v); }
        else { x = T(v); }
        return true; }
};

// T: O(bytes written), M: O(N). Borrowed byte-oriented FILE* must outlive this
// object; no copy/move, close, global stream setup or automatic input tie.
// flush drains both buffers (required before an interactive read). Destructor
// best-effort flushes pending output; call flush explicitly to observe errors.
// Errors are sticky: later writes/flush return false; already-written bytes
// cannot be rolled back. No retry/reset; create a new writer after recovery.
// write's source must not overlap buf. drain transfers only to the FILE buffer.
template<int N = 1 << 16>
struct FastOutput {
    static_assert(N > 0);
    std::FILE *file;
    array<char, N> buf;
    int pos = 0;
    bool error = false, pending = false;

    explicit FastOutput(std::FILE *file = stdout) : file(file) { assert(file); }
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
            error = true; }
        return !error; }
    bool flush() {
        if (!drain()) { return false; }
        if (std::fflush(file) == EOF) { error = true; }
        else { pending = false; }
        return !error; }
    bool put(char c) {
        if (error || (pos == N && !drain())) { return false; }
        buf[pos++] = c; return true; }
    bool write(string_view s) {
        if (error) { return false; }
        while (!s.empty()) {
            if (pos == N && !drain()) { return false; }
            int n = int(min(s.size(), size_t(N - pos)));
            std::memcpy(buf.data() + pos, s.data(), n); pos += n;
            s.remove_prefix(n); }
        return true; }
    template<typename T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    bool writeInt(T x, char end = '\0') {
        using U = std::make_unsigned_t<T>;
        bool neg = std::is_signed_v<T> && x < 0;
        U v = neg ? U(0) - U(x) : U(x);
        char s[std::numeric_limits<T>::digits10 + 3]; int n = 0;
        do { s[n++] = char('0' + v % 10); v /= 10; } while (v);
        if (neg) { s[n++] = '-'; }
        reverse(s, s + n);
        return write(string_view(s, n)) && (!end || put(end)); }
};
