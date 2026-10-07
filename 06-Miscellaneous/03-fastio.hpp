#pragma once
#include "../01-Core/01-template.hpp"

enum class FastIoStatus { Ok, Eof, Invalid, Overflow, Error };

// T: O(bytes read), M: O(N) plus O(L) per token or line; borrowed byte FILE*, use N = 1 for interactive pipes.
// Whole [+/-] decimal tokens, readDouble is from_chars-exact; failure leaves x unchanged with Error > Invalid > Overflow.
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
                return EOF;}
            pos = 0; len = int(std::fread(buf.data(), 1, N, file));
            if (!len) {
                status = std::ferror(file) ? FastIoStatus::Error : FastIoStatus::Eof;
                return EOF;}}
        status = FastIoStatus::Ok; return buf[pos];}
    int get() { int c = peek(); if (c != EOF) { ++pos; } return c; }
    bool skipSpace() {
        while (space(peek())) { ++pos; }
        return status == FastIoStatus::Ok;}

    bool readChar(char &x) {
        if (!skipSpace()) { return false; }
        x = char(get()); return true;}
    bool readToken(string &x) {
        if (!skipSpace()) { return false; }
        string s;
        for (int c = peek(); c != EOF && !space(c); c = peek()) {
            s.push_back(char(c)); ++pos;}
        if (status == FastIoStatus::Error) { return false; }
        x = std::move(s); status = FastIoStatus::Ok; return true;}
    bool readLine(string &x) {
        if (peek() == EOF) { return false; }
        string s;
        while (peek() != EOF) {
            auto *b = buf.data() + pos, *e = buf.data() + len;
            auto *nl = static_cast<unsigned char *>(std::memchr(b, '\n', e - b));
            s.append(reinterpret_cast<const char *>(b), (nl ? nl : e) - b);
            pos = int((nl ? nl + 1 : e) - buf.data());
            if (nl) { break; }}
        if (status == FastIoStatus::Error) { return false; }
        if (!s.empty() && s.back() == '\r') { s.pop_back(); }
        x = std::move(s); status = FastIoStatus::Ok; return true;}

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
            ++pos;}
        if (status == FastIoStatus::Error) { return false; }
        status = bad || !digit ? FastIoStatus::Invalid : overflow ? FastIoStatus::Overflow : FastIoStatus::Ok;
        if (status != FastIoStatus::Ok) { return false; }
        if constexpr (std::is_signed_v<T>) {
            x = neg ? T(-T(v - (v != 0)) - (v != 0)) : T(v);}
        else { x = T(v); }
        return true;}
    bool readDouble(double &x) {
        string s;
        if (!readToken(s)) { return false; }
        const char *b = s.data(), *e = b + s.size();
        double v = 0;
        if (*b == '+' && (++b == e || *b == '-')) { status = FastIoStatus::Invalid; return false; }
        auto [p, ec] = std::from_chars(b, e, v);
        status = ec == std::errc::invalid_argument || p != e ? FastIoStatus::Invalid
               : ec == std::errc::result_out_of_range ? FastIoStatus::Overflow : FastIoStatus::Ok;
        if (status != FastIoStatus::Ok) { return false; }
        x = v; return true;}
    template<typename T>
    bool read(T &x) {
        if constexpr (std::is_same_v<T, char>) { return readChar(x); }
        else if constexpr (std::is_same_v<T, string>) { return readToken(x); }
        else if constexpr (std::is_same_v<T, double>) { return readDouble(x); }
        else if constexpr (std::is_integral_v<T>) { return readInt(x); }
        else {
            for (auto &y : x) {
                if (!read(y)) { return false; }}
            return true;}}
    template<typename ...Ts> requires (sizeof...(Ts) > 1)
    bool read(Ts &...xs) { return (read(xs) && ...); }
};

// T: O(bytes written), M: O(N); borrowed byte FILE*, sticky errors, flush before an interactive read.
// writeDouble/print use to_chars fixed with 0 <= precision <= 200; print separates by spaces and ends with '\n'.
template<int N = 1 << 16>
struct FastOutput {
    static_assert(N > 0);
    std::FILE *file;
    array<char, N> buf;
    int pos = 0, precision = 15;
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
        if (error) { return false; }
        while (!s.empty()) {
            if (pos == N && !drain()) { return false; }
            int n = int(min(s.size(), size_t(N - pos)));
            std::memcpy(buf.data() + pos, s.data(), n); pos += n;
            s.remove_prefix(n);}
        return !end || put(end);}
    template<typename T> requires (std::is_integral_v<T> && !std::is_same_v<T, bool>)
    bool writeInt(T x, char end = '\0') {
        using U = std::make_unsigned_t<T>;
        bool neg = std::is_signed_v<T> && x < 0;
        U v = neg ? U(0) - U(x) : U(x);
        char s[std::numeric_limits<T>::digits10 + 3]; int n = 0;
        do { s[n++] = char('0' + v % 10); v /= 10; } while (v);
        if (neg) { s[n++] = '-'; }
        reverse(s, s + n);
        return write(string_view(s, n), end);}
    bool writeDouble(double x, int p, char end = '\0') {
        assert(0 <= p && p <= 200);
        char s[512];
        auto r = std::to_chars(s, s + 512, x, std::chars_format::fixed, p);
        return write(string_view(s, r.ptr - s), end);}
    template<typename T> requires std::is_same_v<T, long double>
    bool writeDouble(T, int, char = '\0') = delete;
    template<typename T>
    bool writeValue(const T &x) {
        if constexpr (std::is_same_v<T, char>) { return put(x); }
        else if constexpr (std::is_convertible_v<const T &, string_view>) { return write(x); }
        else if constexpr (std::is_floating_point_v<T>) { return writeDouble(x, precision); }
        else if constexpr (std::is_integral_v<T>) { return writeInt(x); }
        else {
            bool first = true;
            for (const auto &y : x) {
                if (!(first || put(' ')) || !writeValue(y)) { return false; }
                first = false;}
            return true;}}
    template<typename ...Ts>
    bool print(const Ts &...xs) {
        [[maybe_unused]] bool first = true;
        return (((std::exchange(first, false) || put(' ')) && writeValue(xs)) && ...) && put('\n');}
};
