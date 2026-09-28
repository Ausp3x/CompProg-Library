#include "../../06-Miscellaneous/03-fastio.hpp"
#include <unistd.h>

ulng seed = 20260927;
string context;
void check(bool ok, const string &s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << s << '\n'; std::exit(1); } }
struct File {
    std::FILE *f = std::tmpfile();
    File() { check(f, "tmpfile"); }
    ~File() { std::fclose(f); }
    void load(const string &s) {
        check(std::fwrite(s.data(), 1, s.size(), f) == s.size(), "fixture write"); std::rewind(f); }
    string text() {
        check(std::fflush(f) == 0, "fixture flush"); std::rewind(f); string s; char b[4096];
        for (size_t n; (n = std::fread(b, 1, sizeof(b), f));) { s.append(b, n); }
        check(!std::ferror(f), "fixture read"); return s; }
};
template<typename T> string decimal(T v) {
    // libstdc++'s independent integer-to-decimal implementation is the oracle.
    char b[50]; auto r = std::to_chars(b, b + sizeof(b), v);
    check(r.ec == std::errc{}, "to_chars oracle"); return string(b, r.ptr); }
string incrementMagnitude(string s) {
    int i = int(s.size()) - 1;
    while (i >= 0 && s[i] == '9') { s[i--] = '0'; }
    if (i < 0 || s[i] == '-') { s.insert(i + 1, 1, '1'); }
    else { ++s[i]; }
    return s; }
template<typename T, int N> void values(const vector<T> &a) {
    File input, output; string s, expected;
    for (int i = 0; i < int(a.size()); ++i) {
        string d = decimal(a[i]);
        s += string(" \t\r\n\v\f").substr(0, i % 6 + 1);
        if (d[0] == '-') { s += "-00" + d.substr(1); }
        else { s += (i % 2 ? "+00" : "") + d; }
        expected += d + '\n'; }
    input.load(s);
    {
        FastInput<N> in(input.f); FastOutput<N> out(output.f);
        check(std::ftell(input.f) == 0 && std::ftell(output.f) == 0, "construction does no IO");
        for (int i = 0; i < int(a.size()); ++i) {
            context = "width=" + std::to_string(sizeof(T) * 8) + " N=" + std::to_string(N) + " i=" + std::to_string(i);
            T x = T(42);
            check(in.readInt(x), "readInt expected=" + decimal(a[i]) + " status=" + std::to_string(int(in.status)));
            check(x == a[i], "readInt expected=" + decimal(a[i]) + " actual=" + decimal(x));
            check(out.writeInt(x, '\n'), "writeInt"); }
        T x = 42; check(!in.readInt(x) && in.status == FastIoStatus::Eof && x == 42, "EOF preserves destination");
        check(in.get() == EOF && in.peek() == EOF, "repeated EOF");
        check(out.flush() && out.flush(), "explicit repeated flush"); }
    check(output.text() == expected, "independent full output comparison");
    std::rewind(input.f); FastInput<N> reused(input.f); T first{};
    check(reused.readInt(first) && first == a[0], "FILE survives reader destruction and rewind"); }
template<typename T> void domain(std::mt19937_64 &rng, int rounds, bool full) {
    vector<T> a{0, 1, T(std::numeric_limits<T>::max()), T(std::numeric_limits<T>::lowest())};
    if constexpr (std::is_signed_v<T>) { a.push_back(-1); }
    if constexpr (sizeof(T) <= 2) {
        if (sizeof(T) == 1 || full) {
            for (int i = int(std::numeric_limits<T>::lowest()); i <= int(std::numeric_limits<T>::max()); ++i) { a.push_back(T(i)); } } }
    for (int i = 0; i < rounds; ++i) {
        using U = std::make_unsigned_t<T>;
        U x = U(rng()); if constexpr (sizeof(T) == 16) { x |= U(rng()) << 64; }
        a.push_back(std::bit_cast<T>(x)); }
    values<T, 1>(a); values<T, 7>(a); values<T, 65536>(a); }
template<typename T> void invalids() {
    string hi = incrementMagnitude(decimal(std::numeric_limits<T>::max()));
    string lo = std::is_signed_v<T> ? incrementMagnitude(decimal(std::numeric_limits<T>::lowest())) : "-1";
    vector<pair<string, FastIoStatus>> cases{{"+", FastIoStatus::Invalid}, {"-", FastIoStatus::Invalid},
        {"--1", FastIoStatus::Invalid}, {"1x", FastIoStatus::Invalid}, {"x1", FastIoStatus::Invalid},
        {"1.0", FastIoStatus::Invalid}, {"0x10", FastIoStatus::Invalid}, {string("2\0x", 3), FastIoStatus::Invalid},
        {string(1, char(255)), FastIoStatus::Invalid}, {hi, FastIoStatus::Overflow},
        {lo, std::is_signed_v<T> ? FastIoStatus::Overflow : FastIoStatus::Invalid},
        {string(1000, '9'), FastIoStatus::Overflow}, {string(1000, '9') + 'x', FastIoStatus::Invalid}};
    if constexpr (!std::is_signed_v<T>) { cases.push_back({"-0", FastIoStatus::Invalid}); }
    for (auto &[s, status] : cases) {
        { File eof; eof.load(s); FastInput<3> in(eof.f); T x = 42;
          context = "invalid-at-EOF width=" + std::to_string(sizeof(T) * 8) + " token=" + s;
          check(!in.readInt(x) && in.status == status && x == 42, "malformed/overflow beats EOF");
          check(!in.readInt(x) && in.status == FastIoStatus::Eof && x == 42, "next operation reports EOF"); }
        File file; file.load(s + " 17"); FastInput<3> in(file.f); T x = 42;
        context = "invalid width=" + std::to_string(sizeof(T) * 8) + " token=" + s;
        check(!in.readInt(x) && in.status == status && x == 42, "status expected=" + std::to_string(int(status)) + " actual=" + std::to_string(int(in.status)));
        check(in.peek() == ' ', "delimiter preserved");
        check(in.readInt(x) && x == 17, "whole token recovery"); }
    File file; file.load("-0 +0 000"); FastInput<1> in(file.f); T x = 42;
    bool ok = in.readInt(x);
    check(ok == std::is_signed_v<T> && (ok ? x == 0 : x == 42), "signed versus unsigned -0");
    check(in.readInt(x) && x == 0 && in.readInt(x) && x == 0, "positive and zero forms"); }
void bytes(int n) {
    context = "binary and string APIs"; string s;
    { File drained;
      { FastOutput<7> out(drained.f); check(out.write("hello") && out.drain(), "drain before destructor"); }
      char b[5]; check(pread(fileno(drained.f), b, 5, 0) == 5 && string(b, 5) == "hello", "destructor flushes earlier drain to underlying file"); }
    for (int i = 0; i < n; ++i) { s += char(i % 256); }
    File file;
    { FastOutput<7> out(file.f); check(out.write(s) && out.drain(), "large binary write/drain"); check(out.put('\0'), "NUL put"); }
    s += '\0'; check(file.text() == s, "destructor flush and binary output");
    std::rewind(file.f);
    { FastInput<7> in(file.f);
      for (int i = 0; i < int(s.size()); ++i) {
          check(in.peek() == static_cast<unsigned char>(s[i]) && in.get() == static_cast<unsigned char>(s[i]), "raw peek/get byte=" + std::to_string(i)); }
      check(in.get() == EOF && in.status == FastIoStatus::Eof, "binary EOF"); }
    File tokens; string tok = string("a\0", 2) + char(255); tokens.load(" \r\n" + tok + " \tZ");
    FastInput<1> in(tokens.f); string t = "unchanged"; char c = '!';
    check(in.readToken(t) && t == tok && in.peek() == ' ', "binary token and preserved whitespace");
    check(in.readChar(c) && c == 'Z' && !in.readChar(c) && c == 'Z', "readChar skip and EOF");
    check(!in.readToken(t) && t == tok, "readToken EOF preserves destination");
    for (int i = 0; i <= 255; ++i) {
        check(FastInput<1>::space(i) == (i == 32 || (i >= 9 && i <= 13)), "ASCII space"); }
    File empty; FastInput<1> e(empty.f); check(!e.skipSpace() && e.status == FastIoStatus::Eof, "empty skipSpace");
    File spaces; spaces.load(" \t\n\r\f\v"); FastInput<7> w(spaces.f);
    check(!w.skipSpace() && w.status == FastIoStatus::Eof, "whitespace-only EOF"); }
struct Cookie { string bytes; size_t pos = 0; int calls = 0, limit = 0; };
ssize_t brokenRead(void *p, char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p); ++c.calls;
    n = min({n, size_t(3), c.bytes.size() - c.pos});
    if (!n) { errno = EIO; return -1; }
    std::memcpy(s, c.bytes.data() + c.pos, n); c.pos += n; return ssize_t(n); }
ssize_t brokenWrite(void *p, const char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p); ++c.calls;
    n = min(n, size_t(c.limit)); c.bytes.append(s, n); errno = ENOSPC; return ssize_t(n); }
void faults() {
    context = "injected FILE errors";
    for (string text : {string(""), string("123"), string("x123"), string(100, '9')}) {
        Cookie c{text}; cookie_io_functions_t ops{}; ops.read = brokenRead;
        std::FILE *f = fopencookie(&c, "r", ops); check(f, "read cookie");
        { FastInput<7> in(f); lng v = 42;
          check(!in.readInt(v) && v == 42 && in.status == FastIoStatus::Error, "input Error precedence");
          check(in.get() == EOF && in.status == FastIoStatus::Error, "sticky FILE error"); }
        std::fclose(f); }
    { Cookie c{"word"}; cookie_io_functions_t ops{}; ops.read = brokenRead;
      std::FILE *f = fopencookie(&c, "r", ops); check(f, "token cookie");
      { FastInput<7> in(f); string s = "old"; check(!in.readToken(s) && s == "old" && in.status == FastIoStatus::Error, "token error preserves destination"); }
      std::fclose(f); }
    for (int limit : {0, 2}) {
        Cookie c; c.limit = limit; cookie_io_functions_t ops{}; ops.write = brokenWrite;
        std::FILE *f = fopencookie(&c, "w", ops); check(f, "write cookie");
        check(std::setvbuf(f, nullptr, _IONBF, 0) == 0, "unbuffered fault fixture");
        { FastOutput<7> out(f); check(out.write("abcde"), "buffer pending");
          check(!out.drain() && out.error, "short/failed fwrite"); int calls = c.calls;
          check(!out.flush() && !out.put('x') && !out.write("") && !out.writeInt(42), "sticky failed writer");
          check(c.calls == calls, "failed writer does not retry"); }
        std::fclose(f); }
    { Cookie c; cookie_io_functions_t ops{}; ops.write = brokenWrite;
      std::FILE *f = fopencookie(&c, "w", ops); check(f, "flush cookie");
      { FastOutput<7> out(f); check(out.write("abc") && out.drain() && !out.error, "C buffer defers error");
        check(!out.flush() && out.error, "fflush detects delayed error"); }
      std::fclose(f); }
    { Cookie c; cookie_io_functions_t ops{}; ops.write = brokenWrite;
      std::FILE *f = fopencookie(&c, "w", ops); check(f, "empty cookie");
      { FastOutput<7> out(f); } check(c.calls == 0, "empty lifetime no IO"); std::fclose(f); }
    int p[2]; check(pipe(p) == 0, "pipe");
    check(::write(p[1], "-17 42\n", 7) == 7, "pipe data");
    std::FILE *f = fdopen(p[0], "r"); check(f, "fdopen");
    { FastInput<1> in(f); int a, b;
      check(in.readInt(a) && a == -17 && in.readInt(b) && b == 42, "interactive N=1 before writer closes");
      close(p[1]); check(!in.readInt(a) && in.status == FastIoStatus::Eof, "pipe EOF"); }
    std::fclose(f); }
int main(int argc, char **argv) {
    if (argc == 2 && string(argv[1]) == "--roundtrip-128") {
        FastInput<7> in; FastOutput<7> out; lll a; ulll b;
        while (in.readInt(a)) {
            check(in.readInt(b), "Python unsigned fixture");
            check(out.writeInt(a, ' ') && out.writeInt(b, '\n'), "Python output"); }
        check(in.status == FastIoStatus::Eof && out.flush(), "Python fixture EOF and flush"); return 0; }
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i];
            if (p == "null-input") { FastInput<> in(nullptr); }
            else if (p == "null-output") { FastOutput<> out(nullptr); }
            return 0; } }
    static_assert(!std::is_copy_constructible_v<FastInput<>> && !std::is_move_constructible_v<FastInput<>>);
    static_assert(!std::is_copy_constructible_v<FastOutput<>> && !std::is_move_constructible_v<FastOutput<>>);
    int rounds = mode == "quick" ? 100 : mode == "full" ? 3000 : 30000;
    std::mt19937_64 rng(seed); bool full = mode != "quick";
    auto suite = [&]<typename T>() -> void { domain<T>(rng, rounds, full); invalids<T>(); };
    suite.operator()<int8_t>(); suite.operator()<uint8_t>(); suite.operator()<int16_t>(); suite.operator()<uint16_t>();
    suite.operator()<int>(); suite.operator()<uint>(); suite.operator()<lng>(); suite.operator()<ulng>();
    suite.operator()<lll>(); suite.operator()<ulll>();
    suite.operator()<char>(); suite.operator()<wchar_t>(); suite.operator()<char8_t>();
    suite.operator()<char16_t>(); suite.operator()<char32_t>();
    suite.operator()<long long>(); suite.operator()<unsigned long long>();
    cout << "PASS integer boundaries, exhaustive domains, independent decimals and invalid recovery\n";
    bytes(mode == "quick" ? 10000 : mode == "full" ? 1 << 20 : 8 << 20);
    cout << "PASS byte/token/character APIs and buffer boundaries\n";
    faults(); cout << "PASS ownership, explicit/destructor flush, FILE faults and interactive pipe\n"; }
