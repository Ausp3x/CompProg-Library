#include "../../06-Miscellaneous/03-fastio.hpp"
#include <unistd.h>

ulng seed = 20260927;
string context;
void check(bool ok, const string &s) {
    if (!ok) { cerr << "FAIL seed=" << seed << " case=" << context << " operation=" << s << '\n'; std::exit(1); }}
struct File {
    std::FILE *f = std::tmpfile();
    File() { check(f, "tmpfile"); }
    ~File() { std::fclose(f); }
    void load(const string &s) {
        check(std::fwrite(s.data(), 1, s.size(), f) == s.size(), "fixture write"); std::rewind(f);}
    string text() {
        check(std::fflush(f) == 0, "fixture flush"); std::rewind(f); string s; char b[4096];
        for (size_t n; (n = std::fread(b, 1, sizeof(b), f));) { s.append(b, n); }
        check(!std::ferror(f), "fixture read"); return s;}
};
template<typename T> string decimal(T v) {
    // libstdc++'s independent integer-to-decimal implementation is the oracle.
    char b[50]; auto r = std::to_chars(b, b + sizeof(b), v);
    check(r.ec == std::errc{}, "to_chars oracle"); return string(b, r.ptr);}
string incrementMagnitude(string s) {
    int i = int(s.size()) - 1;
    while (i >= 0 && s[i] == '9') { s[i--] = '0'; }
    if (i < 0 || s[i] == '-') { s.insert(i + 1, 1, '1'); }
    else { ++s[i]; }
    return s;}
template<typename T, int N> void values(const vector<T> &a) {
    File input, output; string s, expected;
    for (int i = 0; i < int(a.size()); ++i) {
        string d = decimal(a[i]);
        s += string(" \t\r\n\v\f").substr(0, i % 6 + 1);
        if (d[0] == '-') { s += "-00" + d.substr(1); }
        else { s += (i % 2 ? "+00" : "") + d; }
        expected += d + '\n';}
    input.load(s);
    {
        FastInput<N> in(input.f); FastOutput<N> out(output.f);
        check(std::ftell(input.f) == 0 && std::ftell(output.f) == 0, "construction does no IO");
        for (int i = 0; i < int(a.size()); ++i) {
            context = "width=" + std::to_string(sizeof(T) * 8) + " N=" + std::to_string(N) + " i=" + std::to_string(i);
            T x = T(42);
            check(in.readInt(x), "readInt expected=" + decimal(a[i]) + " status=" + std::to_string(int(in.status)));
            check(x == a[i], "readInt expected=" + decimal(a[i]) + " actual=" + decimal(x));
            check(out.writeInt(x, '\n'), "writeInt");}
        T x = 42; check(!in.readInt(x) && in.status == FastIoStatus::Eof && x == 42, "EOF preserves destination");
        check(in.get() == EOF && in.peek() == EOF, "repeated EOF");
        check(out.flush() && out.flush(), "explicit repeated flush"); }
    check(output.text() == expected, "independent full output comparison");
    std::rewind(input.f); FastInput<N> reused(input.f); T first{};
    check(reused.readInt(first) && first == a[0], "FILE survives reader destruction and rewind");}
template<typename T> void domain(std::mt19937_64 &rng, int rounds, bool full) {
    vector<T> a{0, 1, T(std::numeric_limits<T>::max()), T(std::numeric_limits<T>::lowest())};
    if constexpr (std::is_signed_v<T>) { a.push_back(-1); }
    if constexpr (sizeof(T) <= 2) {
        if (sizeof(T) == 1 || full) {
            for (int i = int(std::numeric_limits<T>::lowest()); i <= int(std::numeric_limits<T>::max()); ++i) { a.push_back(T(i)); }}}
    for (int i = 0; i < rounds; ++i) {
        using U = std::make_unsigned_t<T>;
        U x = U(rng()); if constexpr (sizeof(T) == 16) { x |= U(rng()) << 64; }
        a.push_back(std::bit_cast<T>(x));}
    values<T, 1>(a); values<T, 7>(a); values<T, 65536>(a);}
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
        check(in.readInt(x) && x == 17, "whole token recovery");}
    File file; file.load("-0 +0 000"); FastInput<1> in(file.f); T x = 42;
    bool ok = in.readInt(x);
    check(ok == std::is_signed_v<T> && (ok ? x == 0 : x == 42), "signed versus unsigned -0");
    check(in.readInt(x) && x == 0 && in.readInt(x) && x == 0, "positive and zero forms");}
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
          check(in.peek() == static_cast<unsigned char>(s[i]) && in.get() == static_cast<unsigned char>(s[i]), "raw peek/get byte=" + std::to_string(i));}
      check(in.get() == EOF && in.status == FastIoStatus::Eof, "binary EOF"); }
    File tokens; string tok = string("a\0", 2) + char(255); tokens.load(" \r\n" + tok + " \tZ");
    FastInput<1> in(tokens.f); string t = "unchanged"; char c = '!';
    check(in.readToken(t) && t == tok && in.peek() == ' ', "binary token and preserved whitespace");
    check(in.readChar(c) && c == 'Z' && !in.readChar(c) && c == 'Z', "readChar skip and EOF");
    check(!in.readToken(t) && t == tok, "readToken EOF preserves destination");
    for (int i = 0; i <= 255; ++i) {
        check(FastInput<1>::space(i) == (i == 32 || (i >= 9 && i <= 13)), "ASCII space");}
    File empty; FastInput<1> e(empty.f); check(!e.skipSpace() && e.status == FastIoStatus::Eof, "empty skipSpace");
    File spaces; spaces.load(" \t\n\r\f\v"); FastInput<7> w(spaces.f);
    check(!w.skipSpace() && w.status == FastIoStatus::Eof, "whitespace-only EOF");}
vector<string> splitLines(const string &s) {
    vector<string> res; string cur; bool open = false;
    for (char c : s) {
        if (c == '\n') { res.push_back(cur); cur.clear(); open = false; }
        else { cur += c; open = true; }}
    if (open) { res.push_back(cur); }
    for (auto &x : res) {
        if (!x.empty() && x.back() == '\r') { x.pop_back(); }}
    return res;}
template<int N> void linesWith(const string &s) {
    File file; file.load(s); FastInput<N> in(file.f); vector<string> got; string x = "unchanged";
    while (in.readLine(x)) { got.push_back(x); }
    context = "readLine N=" + std::to_string(N) + " bytes=" + std::to_string(s.size());
    check(got == splitLines(s), "readLine equals independent split");
    check(in.status == FastIoStatus::Eof && (got.empty() ? x == "unchanged" : x == got.back()), "readLine EOF preserves destination");}
void linesAndDoubles(std::mt19937_64 &rng, int rounds) {
    vector<string> fixed{"", "\n", "\n\n", "a", "a\n", "a\r\n", "\r\n", "\r", "a\rb\n", "x\r\r\n", string("\0\n\0", 3),
        "  lead and trail  \n\tz", string(70000, 'q') + "\nend", string(65535, 'w') + "\n" + string(65536, 'v')};
    for (int i = 0; i < rounds; ++i) {
        string t; int n = int(rng() % 40);
        for (int j = 0; j < n; ++j) { t += "ab\r\n \t"[rng() % 6]; }
        fixed.push_back(t);}
    for (const string &t : fixed) { linesWith<1>(t); linesWith<7>(t); linesWith<65536>(t); }
    { File mix; mix.load("12 word\nnext line\r\n  7"); FastInput<3> in(mix.f); int a = 0; string w, l;
      context = "readInt/readToken then readLine";
      check(in.readInt(a) && in.readToken(w) && in.readLine(l) && l.empty(), "rest of the current line is empty");
      check(in.readLine(l) && l == "next line" && in.readInt(a) && a == 7 && !in.readLine(l) && l == "next line", "line after tokens"); }
    context = "readDouble";
    vector<pair<string, FastIoStatus>> cases{{"1.5e", FastIoStatus::Invalid}, {"0x10", FastIoStatus::Invalid},
        {"+", FastIoStatus::Invalid}, {"+-1", FastIoStatus::Invalid}, {"--1", FastIoStatus::Invalid}, {".", FastIoStatus::Invalid},
        {"1,5", FastIoStatus::Invalid}, {"e5", FastIoStatus::Invalid}, {"1e400", FastIoStatus::Overflow},
        {"-1e400", FastIoStatus::Overflow}, {"1e-400", FastIoStatus::Overflow}, {"1e400x", FastIoStatus::Invalid}};
    for (auto &[t, status] : cases) {
        File file; file.load(t + " 2.5"); FastInput<3> in(file.f); double x = 42;
        check(!in.readDouble(x) && in.status == status && x == 42, "readDouble status token=" + t);
        check(in.readDouble(x) && x == 2.5, "readDouble whole token recovery token=" + t);
        File eof; eof.load(t); FastInput<1> e(eof.f);
        check(!e.readDouble(x) && e.status == status && x == 2.5 && !e.readDouble(x) && e.status == FastIoStatus::Eof, "readDouble at EOF token=" + t);}
    vector<string> good{"0", "-0", "+0", "+1.5", "-.5e3", "5.", ".5", "1E5", "1e-320", "4.9406564584124654e-324",
        "1.7976931348623157e308", "123456789012345678901234567890", "0.1", "inf", "-INF", "nan", "Infinity"};
    for (int i = 0; i < rounds; ++i) {
        double v = std::bit_cast<double>(rng());
        if (!std::isfinite(v)) { continue; }
        char b[64]; std::snprintf(b, sizeof b, i % 3 == 0 ? "%.17g" : i % 3 == 1 ? "%.3e" : "%.25f", v); good.push_back(b);}
    string all; for (auto &t : good) { all += t + "\n\t "; }
    File file; file.load(all); FastInput<7> in(file.f);
    for (auto &t : good) {
        double x = 0, want = std::strtod(t.c_str(), nullptr);
        check(in.readDouble(x), "readDouble valid token=" + t);
        check(std::isnan(want) ? std::isnan(x) : std::bit_cast<ulng>(x) == std::bit_cast<ulng>(want), "readDouble equals strtod token=" + t);}
    double x = 1; check(!in.readDouble(x) && in.status == FastIoStatus::Eof && x == 1, "readDouble EOF");
    context = "writeDouble";
    File out; string expected;
    { FastOutput<7> o(out.f);
      vector<double> vals{0.0, -0.0, 0.5, 1.5, 2.5, -2.5, 0.125, 1e-7, 123456.789, DBL_MAX, -DBL_MAX, DBL_MIN, 5e-324,
                          INFINITY, -INFINITY, 0.1, 1.0 / 3, 999.9995};
      for (int i = 0; i < rounds; ++i) {
          double v = std::bit_cast<double>(rng());
          if (std::isfinite(v)) { vals.push_back(v); }}
      int k = 0;
      for (double v : vals) {
          for (int prec : {0, 1, 3, 6, 9, 15, 17, 20, 200}) {
              if (++k % 7 && prec == 200) { continue; }
              char b[1024]; std::snprintf(b, sizeof b, "%.*f", prec, v); expected += string(b) + (k % 2 ? "\n" : "");
              check(o.writeDouble(v, prec, k % 2 ? '\n' : '\0'), "writeDouble");}}
      check(o.flush(), "writeDouble flush"); }
    check(out.text() == expected, "writeDouble equals printf %.*f");
    File nan; { FastOutput<1> o(nan.f); check(o.writeDouble(NAN, 2, ' ') && o.writeDouble(-NAN, 0), "NaN"); }
    check(nan.text() == "nan -nan", "NaN spelling");}
template<typename T> concept WritesDouble = requires(FastOutput<> &o, T x) { o.writeDouble(x, 1); };
static_assert(!WritesDouble<long double> && WritesDouble<double> && WritesDouble<float> && WritesDouble<int>);
void variadic() {
    context = "read/print";
    File file; file.load("  -5 word Z 2.25 3\n10 20 30\n");
    FastInput<5> in(file.f); int a = 0; string w; char c = 0; double d = 0; vector<lng> v(4); uint8_t small = 0;
    check(in.read(a, w, c, d, small) && a == -5 && w == "word" && c == 'Z' && d == 2.25 && small == 3, "variadic read");
    check(!in.read(v) && v[0] == 10 && v[2] == 30 && v[3] == 0 && in.status == FastIoStatus::Eof, "range read stops at EOF");
    File out;
    { FastOutput<3> o(out.f); o.precision = 3;
      vector<int> empty; vector<vector<int>> nested{{1, 2}, {3}};
      check(o.print(1, "two", string("three"), 'c', 2.5, vector<int>{4, 5}, empty, nested, lll(-7), int8_t(-8)), "print");
      check(o.print() && o.print(string_view("sv")) && o.write("raw", '!') && o.writeInt(9) && o.put('\n'), "print/write end");
      check(o.writeValue(1.0f) && o.writeValue(vector<double>{0.5}), "writeValue float"); }
    check(out.text() == "1 two three c 2.500 4 5  1 2 3 -7 -8\n\nsv\nraw!9\n1.0000.500", "print text");}
struct Cookie { string bytes; size_t pos = 0; int calls = 0, limit = 0; };
ssize_t brokenRead(void *p, char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p); ++c.calls;
    n = min({n, size_t(3), c.bytes.size() - c.pos});
    if (!n) { errno = EIO; return -1; }
    std::memcpy(s, c.bytes.data() + c.pos, n); c.pos += n; return ssize_t(n);}
ssize_t brokenWrite(void *p, const char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p); ++c.calls;
    n = min(n, size_t(c.limit)); c.bytes.append(s, n); errno = ENOSPC; return ssize_t(n);}
void faults() {
    context = "injected FILE errors";
    for (string text : {string(""), string("123"), string("x123"), string(100, '9')}) {
        Cookie c{text}; cookie_io_functions_t ops{}; ops.read = brokenRead;
        std::FILE *f = fopencookie(&c, "r", ops); check(f, "read cookie");
        { FastInput<7> in(f); lng v = 42;
          check(!in.readInt(v) && v == 42 && in.status == FastIoStatus::Error, "input Error precedence");
          check(in.get() == EOF && in.status == FastIoStatus::Error, "sticky FILE error"); }
        std::fclose(f);}
    { Cookie c{"word"}; cookie_io_functions_t ops{}; ops.read = brokenRead;
      std::FILE *f = fopencookie(&c, "r", ops); check(f, "token cookie");
      { FastInput<7> in(f); string s = "old"; check(!in.readToken(s) && s == "old" && in.status == FastIoStatus::Error, "token error preserves destination"); }
      std::fclose(f); }
    for (int kind = 0; kind < 2; ++kind) {
        Cookie c{kind ? "1.25" : "line"}; cookie_io_functions_t ops{}; ops.read = brokenRead;
        std::FILE *f = fopencookie(&c, "r", ops); check(f, "line/double cookie");
        { FastInput<7> in(f); string s = "old"; double d = 7;
          check(kind ? !in.readDouble(d) && d == 7 : !in.readLine(s) && s == "old", "line/double error preserves destination");
          check(in.status == FastIoStatus::Error, "line/double Error precedence"); }
        std::fclose(f);}
    for (int limit : {0, 2}) {
        Cookie c; c.limit = limit; cookie_io_functions_t ops{}; ops.write = brokenWrite;
        std::FILE *f = fopencookie(&c, "w", ops); check(f, "write cookie");
        check(std::setvbuf(f, nullptr, _IONBF, 0) == 0, "unbuffered fault fixture");
        { FastOutput<7> out(f); check(out.write("abcde"), "buffer pending");
          check(!out.drain() && out.error, "short/failed fwrite"); int calls = c.calls;
          check(!out.flush() && !out.put('x') && !out.write("") && !out.writeInt(42), "sticky failed writer");
          check(c.calls == calls, "failed writer does not retry"); }
        std::fclose(f);}
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
    std::fclose(f);}
int main(int argc, char **argv) {
    if (argc == 2 && string(argv[1]) == "--lines") {
        FastInput<7> in; FastOutput<7> out; string l;
        while (in.readLine(l)) { out.writeInt(l.size(), ':'); out.write(l, '\n'); }
        check(in.status == FastIoStatus::Eof && out.flush(), "Python line fixture"); return 0;}
    if (argc == 2 && string(argv[1]) == "--doubles") {
        FastInput<7> in; FastOutput<7> out; double x;
        while (in.readDouble(x) || in.status != FastIoStatus::Eof) {
            char b[40]; std::snprintf(b, sizeof b, "%a", x);
            out.write(in.status == FastIoStatus::Ok ? b : in.status == FastIoStatus::Invalid ? "I" : "O", '\n');}
        check(out.flush(), "Python double fixture"); return 0;}
    if (argc == 2 && string(argv[1]) == "--write-doubles") {
        FastInput<7> in; FastOutput<7> out; ulng bits; int prec;
        while (in.readInt(bits)) {
            check(in.readInt(prec), "Python precision fixture");
            check(out.writeDouble(std::bit_cast<double>(bits), prec, '\n'), "Python writeDouble");}
        check(in.status == FastIoStatus::Eof && out.flush(), "Python write fixture"); return 0;}
    if (argc == 2 && string(argv[1]) == "--roundtrip-128") {
        FastInput<7> in; FastOutput<7> out; lll a; ulll b;
        while (in.readInt(a)) {
            check(in.readInt(b), "Python unsigned fixture");
            check(out.writeInt(a, ' ') && out.writeInt(b, '\n'), "Python output");}
        check(in.status == FastIoStatus::Eof && out.flush(), "Python fixture EOF and flush"); return 0;}
    string mode = "full";
    for (int i = 1; i < argc; ++i) {
        string s = argv[i];
        if (s == "--mode") { mode = argv[++i]; }
        else if (s == "--seed") { seed = std::stoull(argv[++i]); }
        else if (s == "--invalid") {
            string p = argv[++i];
            if (p == "null-input") { FastInput<> in(nullptr); }
            else if (p == "null-output") { FastOutput<> out(nullptr); }
            else if (p == "precision-negative") { FastOutput<> out(stdout); out.writeDouble(1, -1); }
            else if (p == "precision-large") { FastOutput<> out(stdout); out.writeDouble(1, 201); }
            return 0;}}
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
    linesAndDoubles(rng, rounds); variadic();
    cout << "PASS readLine/readDouble/writeDouble against split/strtod/printf, read/print/write end\n";
    faults(); cout << "PASS ownership, explicit/destructor flush, FILE faults and interactive pipe\n";}
