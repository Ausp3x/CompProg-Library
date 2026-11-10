#include "../../01-Core/19-fastio.hpp"
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/08-infintmini.hpp"
#include "../../01-Core/09-rational.hpp"
#include <sys/mman.h>
#include <unistd.h>
#ifndef FASTIO_PART   // one unit, as 02-integration.py builds every Core tester: a sanitizer smoke subset that fits its budget (Docs/19-fastio.md)
#define FASTIO_PART -1
#endif

using St = FastIoStatus;
using IntTypes = std::tuple<char, int8_t, uint8_t, int16_t, uint16_t, int, uint, lng, ulng, long long, unsigned long long, lll, ulll, wchar_t, char8_t, char16_t, char32_t>;
inline constexpr array<const char *, 17> TYPE_NAMES{"char", "int8_t", "uint8_t", "int16_t", "uint16_t", "int", "uint", "lng", "ulng", "long long", "unsigned long long", "lll", "ulll", "wchar_t", "char8_t", "char16_t", "char32_t"};
enum Kind { REGULAR, OFFSET, CONSUMED, PIPE, MEMORY };
inline constexpr array<const char *, 5> KIND_NAMES{"regular", "offset", "consumed", "pipe", "memory"};
inline constexpr array<const char *, 5> API_NAMES{"readInt", "readToken+parseInt", "read", "read(tuple<T&>)", "read(x,y)"};
inline constexpr char DIG[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
inline constexpr lll LLL_MAX = lll(~ulll(0) >> 1), LLL_MIN = -LLL_MAX - 1;
inline constexpr ulll ULLL_MAX = ~ulll(0);
inline constexpr ulng ULNG_MAX = ~0ULL;
inline constexpr int CANARY = 0x5EED, PARTS = 10;
inline const string WS = " \t\n\v\f\r", BAD = string("\0\x01\x08\x0E\x10\x19\x1F", 7) + "!+-/.:@GgZz[`{\x7F\x80\xC0\xFF";
inline string DATA, mode = "full", section;
inline ulng seed = 1, digest = 0;
inline int failures = 0, section_failures = 0;
inline std::mt19937_64 rng;

inline int scale(int q, int f, int s) { return mode == "quick" ? q : mode == "full" ? f : s; }
inline ulng rnd(ulng n) { return rng() % n; }
inline int rint(int lo, int hi) { return lo + int(rng() % ulng(hi - lo + 1)); }
inline bool isWs(int c) { return c == ' ' || (c >= '\t' && c <= '\r'); }
constexpr bool isKey(int B) { return B == 2 || B == 10 || B == 16 || B == 36; }
#ifdef FASTIO_QUICK
inline constexpr bool QUICK = true;   // the quick build: bases 2, 7, 10, 16, 36 and buffer sizes 0, 7, 64, 160 in the type matrices
#else
inline constexpr bool QUICK = false;
#endif
constexpr bool built(int B) { return !QUICK || isKey(B) || B == 7; }
constexpr int partOf(int B) { return 1 + (B == 2 ? 0 : B == 10 ? 1 : B == 16 ? 2 : B == 36 ? 3 : B + 1 - (B > 10) - (B > 16)) % PARTS; }
inline const char *stName(St s) { return array<const char *, 5>{"Ok", "Eof", "Invalid", "Overflow", "Error"}[size_t(s)]; }
inline string esc(string_view s) {
    string r = "\"";
    for (size_t i = 0; i < s.size(); ++i) {
        if (s.size() > 200 && i == 80) {
            r += "...<" + std::to_string(s.size() - 160) + " more bytes>...";
            i = s.size() - 81; continue;}
        auto c = uint8_t(s[i]);
        if (c > ' ' && c < 0x7F && c != '"' && c != '\\') { r += char(c); }
        else {
            char b[8]; std::snprintf(b, sizeof b, "\\x%02X", c);
            r += b;}}
    return r + "\"";}
inline string lowered(string s) {
    for (char &c : s) { c = char(std::tolower(uint8_t(c))); }
    return s;}
inline string wsRun(int n) {
    string r;
    for (int i = 0; i < n; ++i) { r += WS[rnd(6)]; }
    return r;}
inline string randomDigits(int len, int B, bool lead) {
    string r;
    for (int i = 0; i < len; ++i) {
        char c = DIG[(i == 0 && lead) ? 1 + rnd(ulng(B - 1)) : rnd(ulng(B))];
        r += rnd(2) ? char(std::tolower(c)) : c;}
    return r;}
inline string diffAt(const string &got, const string &want) {
    size_t i = 0;
    while (i < got.size() && i < want.size() && got[i] == want[i]) { ++i; }
    size_t from = i - min(i, size_t(40));
    return "first difference at byte " + std::to_string(i) + " (sizes got=" + std::to_string(got.size()) + " want=" + std::to_string(want.size()) + ") want=" + esc(string_view(want).substr(min(from, want.size()), 100)) + " got=" + esc(string_view(got).substr(min(from, got.size()), 100));}
template<typename F> void check(bool ok, F &&what) {
    if (ok) { return; }
    ++failures;
    if (++section_failures > 6) { return; }
    cerr << "FAIL seed=" << seed << " section=" << section << ' ';
    if constexpr (std::is_invocable_v<F &>) { cerr << what(); }
    else { cerr << what; }
    cerr << std::endl;}
inline void enter(const string &s) {
    section = s; section_failures = 0; digest = 0xCBF29CE484222325ULL;
    ulng h = seed * 0x9E3779B97F4A7C15ULL + 1;
    for (char c : s) { h = (h ^ uint8_t(c)) * 0x100000001B3ULL; }
    rng.seed(h);}
inline void leave() {
    cout << (section_failures ? "FAIL " : "PASS ") << section;
    if (section_failures) { cout << " failures=" << section_failures; }
    cout << "\nDIGEST " << section << ' ' << std::hex << digest << std::dec << std::endl;}
inline void mix(ulng v) { digest = (digest ^ v) * 0x100000001B3ULL; digest ^= digest >> 29; }
inline void mixText(string_view s) {
    for (char c : s) { mix(uint8_t(c)); }
    mix(0x1FF);}

// Integer oracles: std::to_chars / std::from_chars for at most 64 bits, a digit loop for 128 bits; each validates the other.
template<typename T> constexpr bool SIGNED_T = T(-1) < T(0);
template<typename T> auto stdOf() {
    if constexpr (sizeof(T) > 8) { return T(); }
    else if constexpr (SIGNED_T<T>) { return std::make_signed_t<T>(); }
    else { return std::make_unsigned_t<T>(); }}
template<typename T> using Std = decltype(stdOf<T>());
template<typename T> constexpr size_t typeIndex() {
    size_t r = 0;
    [&]<size_t... I>(std::index_sequence<I...>) { ((std::is_same_v<T, std::tuple_element_t<I, IntTypes>> ? void(r = I) : void()), ...); }(std::make_index_sequence<17>{});
    return r;}
template<typename T> string tname() { return TYPE_NAMES[typeIndex<T>()]; }
template<typename T> bool isNeg([[maybe_unused]] T v) {
    if constexpr (SIGNED_T<T>) { return v < 0; }
    else { return false; }}
template<typename T> struct Got { St st; T v; };
inline uint digitOf(char c) {
    auto u = uint8_t(c);
    return u >= '0' && u <= '9' ? u - '0' : u >= 'A' && u <= 'Z' ? u - 'A' + 10 : u >= 'a' && u <= 'z' ? u - 'a' + 10 : 99;}
template<typename T> ulll magMax(bool neg) {
    if constexpr (SIGNED_T<T>) { return neg ? ulll(0) - ulll(lll(std::numeric_limits<T>::min())) : ulll(std::numeric_limits<T>::max()); }
    else { return ulll(std::numeric_limits<T>::max()); }}
template<typename T> string loopText(T v, int B) {
    ulll m = ulll(v);
    bool neg = isNeg(v);
    if (neg) { m = ulll(0) - m; }
    string r;
    do { r += DIG[int(m % uint(B))]; m /= uint(B); } while (m);
    if (neg) { r += '-'; }
    return string(r.rbegin(), r.rend());}
template<typename T> Got<T> loopParse(string_view s, int B) {
    bool neg = !s.empty() && s[0] == '-';
    if (!s.empty() && (s[0] == '+' || neg)) { s.remove_prefix(1); }
    if (s.empty() || (neg && !SIGNED_T<T>) || std::ranges::any_of(s, [&](char c) { return digitOf(c) >= uint(B); })) { return {St::Invalid, T()}; }
    ulll lim = magMax<T>(neg), m = 0;
    bool over = false;
    for (char c : s) {
        uint d = digitOf(c);
        if (over || m > (lim - d) / uint(B)) { over = true; }
        else { m = m * uint(B) + d; }}
    if (over) { return {St::Overflow, T()}; }
    return {St::Ok, T(neg ? ulll(0) - m : m)};}
template<typename T> string charsText(T v, int B) {
    char b[160];
    auto r = std::to_chars(b, b + 160, Std<T>(v), B);
    for (char *c = b; c < r.ptr; ++c) { *c = char(std::toupper(uint8_t(*c))); }
    return string(b, r.ptr);}
template<typename T> Got<T> charsParse(string_view s, int B) {
    bool neg = !s.empty() && s[0] == '-';
    string_view b = s.substr(!s.empty() && (s[0] == '+' || neg));
    if (b.empty() || b[0] == '+' || b[0] == '-' || (neg && !SIGNED_T<T>)) { return {St::Invalid, T()}; }
    string_view f = neg ? s : b;
    Std<T> v{};
    auto [p, ec] = std::from_chars(f.data(), f.data() + f.size(), v, B);
    if (p != f.data() + f.size() || ec == std::errc::invalid_argument) { return {St::Invalid, T()}; }
    if (ec == std::errc::result_out_of_range) { return {St::Overflow, T()}; }
    return {St::Ok, T(v)};}
template<typename T> string refText(T v, int B) {
    if constexpr (sizeof(T) > 8) { return loopText(v, B); }
    else { return charsText(v, B); }}
template<typename T> Got<T> refParse(string_view s, int B) {
    if constexpr (sizeof(T) > 8) { return loopParse<T>(s, B); }
    else { return charsParse<T>(s, B); }}
inline string incDigits(string d, int B) {
    int i = int(d.size()) - 1;
    for (; i >= 0 && digitOf(d[size_t(i)]) == uint(B - 1); --i) { d[size_t(i)] = '0'; }
    if (i < 0) { return "1" + d; }
    d[size_t(i)] = DIG[digitOf(d[size_t(i)]) + 1];
    return d;}
inline ulll rnd128() { return ulll(rng()) << 64 | rng(); }
template<typename T> T randomValue() {
    int bits = 1 + int(rnd(8 * sizeof(T)));
    ulll m = rnd128() & (bits >= 128 ? ULLL_MAX : (ulll(1) << bits) - 1);
    return SIGNED_T<T> && rnd(2) ? T(ulll(0) - m) : T(m);}

// Fixtures: a FILE over bytes (regular file, regular file entered at an offset by fseek or by consuming bytes, pipe, memory) and an output sink.
struct Source {
    std::FILE *f = nullptr;
    string data;
    size_t skip = 0;
    std::thread writer;

    Source(string s, Kind kind) : data(std::move(s)) {
        if (kind == PIPE) {
            int fd[2];
            check(pipe(fd) == 0, "pipe fixture");
            f = fdopen(fd[0], "r");
            writer = std::thread([w = fd[1], d = data] {
                for (size_t i = 0; i < d.size();) {
                    ssize_t k = ::write(w, d.data() + i, d.size() - i);
                    if (k <= 0) { break; }
                    i += size_t(k);}
                close(w);});}
        else if (kind == MEMORY && !data.empty()) { f = fmemopen(data.data(), data.size(), "r"); }
        else {
            f = std::tmpfile();
            string junk = kind == OFFSET || kind == CONSUMED ? string(1 + data.size() % 97, '7') : "";
            skip = junk.size();
            check(f && std::fwrite(junk.data(), 1, skip, f) == skip && std::fwrite(data.data(), 1, data.size(), f) == data.size() && std::fflush(f) == 0, "file fixture");
            std::rewind(f);
            if (kind == OFFSET) { check(fseeko(f, off_t(skip), SEEK_SET) == 0, "fixture seek"); }
            for (size_t i = 0; kind == CONSUMED && i < skip; ++i) { std::fgetc(f); }}
        check(f != nullptr, "fixture FILE");}
    ~Source() {
        if (f) { std::fclose(f); }
        if (writer.joinable()) { writer.join(); }}
};
struct Sink {
    char *p = nullptr;
    size_t n = 0;
    std::FILE *f = open_memstream(&p, &n);

    Sink() { check(f != nullptr, "open_memstream"); }
    ~Sink() { std::fclose(f); std::free(p); }
    string text() { std::fflush(f); return string(p, n); }
};
struct Stream {
    string s;
    vector<size_t> at;
};
// Tokens separated by runs of the six whitespace bytes; with align, token i starts i % 34 bytes after a multiple of align.
inline Stream join(const vector<string> &toks, bool lead, bool trail, size_t align) {
    Stream st;
    for (size_t i = 0; i < toks.size(); ++i) {
        if (i || lead) { st.s += wsRun(rnd(16) ? rint(1, 3) : rint(31, 33)); }
        if (align) {
            size_t want = (st.s.size() + align - 1) / align * align + i % 34;
            while (st.s.size() < want) { st.s += WS[rnd(6)]; }}
        st.at.push_back(st.s.size());
        st.s += toks[i];}
    if (trail) { st.s += wsRun(rint(1, 3)); }
    return st;}
// The reference cursor: whitespace is exactly the six bytes.
struct Model {
    const string &s;
    size_t p = 0;

    bool skip() {
        while (p < s.size() && isWs(uint8_t(s[p]))) { ++p; }
        return p < s.size();}
    bool token(size_t &a, size_t &b) {
        if (!skip()) { return false; }
        a = p;
        while (p < s.size() && !isWs(uint8_t(s[p]))) { ++p; }
        b = p; return true;}
    int peek() const { return p < s.size() ? uint8_t(s[p]) : EOF; }
    int get() { return p < s.size() ? uint8_t(s[p++]) : EOF; }
    bool line(string &x) {
        if (p == s.size()) { return false; }
        size_t q = s.find('\n', p);
        x = s.substr(p, q == string::npos ? string::npos : q - p);
        p = q == string::npos ? s.size() : q + 1;
        if (!x.empty() && x.back() == '\r') { x.pop_back(); }
        return true;}
};
// Where a readToken view must point: N = 0 into the stream; N > 0 into tok when the token reaches the end of its block, else into buf.
template<int N> bool viewAt(const FastInput<N> &in, string_view v, size_t a, size_t b, size_t total, size_t skip) {
    if constexpr (N == 0) { return v.data() == reinterpret_cast<const char *>(in.b0) + (in.mapped ? skip : 0) + a; }
    else {
        if (b >= min((a / size_t(N) + 1) * size_t(N), total)) { return v.data() == in.tok.data() && v.size() == in.tok.size(); }
        return v.data() == reinterpret_cast<const char *>(in.buf.data()) + a % size_t(N);}}

// One call site per API and (N, B, T): the matrix, the protocol and the boundary tests share these expansions.
template<int N, int B, typename T> [[gnu::noinline]] bool readVia(FastInput<N> &in, T &x, int api, string_view &sv) {
    if (api == 1) { return in.readToken(sv) && in.template parseInt<B>(sv, x); }
    if constexpr (isKey(B) && !std::is_same_v<T, char>) {
        if (api == 2) { return in.template read<B>(x); }
        if (api == 3) {
            std::tuple<T &> r{x};
            return in.template read<B>(r);}}
    return in.template readInt<B>(x);}
template<int N, int B, typename T> [[gnu::noinline]] bool readPair(FastInput<N> &in, T &x, T &y) { return in.template read<B>(x, y); }
template<int N, int B, typename T> [[gnu::noinline]] bool writeVia(FastOutput<N> &out, T v, T w, T u, int op, char end) {
    if constexpr (isKey(B)) {
        if (op == 1) { return out.template print<B>(v); }
        if (op == 3) { return out.template print<B>(v, w, u); }
        if (op == 4) { return out.template writeValue<B>(v); }
        if (op == 5) { return out.template writeValue<B>(vector<T>{v, w}); }
        if (op == 6) { return out.template writeValue<B>(pair<T, T>{v, w}); }
        if (op == 7) { return out.template print<B>(v, LLL_MIN, uint8_t(255), w); }
        if (op == 8) { return out.template print<B>(std::tuple<T, int16_t>{v, int16_t(-5)}, u); }}
    if (op == 2) { return out.template print<B>(v, w); }
    return out.template writeInt<B>(v, end);}
template<typename T> string asValue(T v, int B) {
    if constexpr (std::is_same_v<T, char>) { return string(1, v); }
    else { return refText(v, B); }}
template<typename T> string writeWant(T v, T w, T u, int op, char end, int B) {
    string a = asValue(v, B), b = asValue(w, B), c = asValue(u, B);
    if (op == 0) { return refText(v, B) + (end ? string(1, end) : ""); }
    if (op == 1) { return a + '\n'; }
    if (op == 2) { return a + ' ' + b + '\n'; }
    if (op == 3) { return a + ' ' + b + ' ' + c + '\n'; }
    if (op == 4) { return a; }
    if (op == 5 || op == 6) { return a + ' ' + b; }
    if (op == 7) { return a + ' ' + refText(LLL_MIN, B) + ' ' + refText(uint8_t(255), B) + ' ' + b + '\n'; }
    return a + ' ' + refText(int16_t(-5), B) + ' ' + c + '\n';}
// One read as observed: the result, the status, the destinations as 128-bit patterns, and whether a readToken view matched its token.
struct Obs { bool ok; St st; ulll x, y; bool view; };
template<typename T> int apiOf(int B, size_t i) { return int((i + seed) % (isKey(B) && !std::is_same_v<T, char> ? 5 : 2)); }
template<typename T> int opOf(int B, size_t i) { return std::is_same_v<T, char> ? (isKey(B) && i % 2 ? 4 : 0) : isKey(B) ? int(i % 9) : (i % 2 ? 2 : 0); }
inline char endOf(size_t i) { return " \n,\x01\0"[i / 9 % 5]; }
inline string intWhere(const string &tn, int B, int N, Kind kind, int api, const Stream &st, const vector<string> &toks, size_t i) {
    size_t a = i < st.at.size() ? st.at[i] : st.s.size(), k = min(a, size_t(40));
    return "T=" + tn + " B=" + std::to_string(B) + " N=" + std::to_string(N) + " kind=" + KIND_NAMES[kind] + " api=" + API_NAMES[size_t(api)] + " token#" + std::to_string(i) + "="
        + (i < toks.size() ? esc(toks[i]) : string("<EOF>")) + " offset=" + std::to_string(a) + " preceded by " + esc(string_view(st.s).substr(a - k, k));}
// The reader's side of the matrix: every API in turn over the stream, then one read past the end; checked by readCheck.
template<int N, int B, typename T> [[gnu::noinline]] vector<Obs> readRun(const Stream &st, const vector<string> &toks, const vector<Got<T>> &exp, Kind kind) {
    Source src(st.s, kind);
    FastInput<N> in(src.f);
    vector<Obs> r;
    T x = T(42);
    for (size_t i = 0; i < toks.size();) {
        int api = apiOf<T>(B, i);
        if (api == 4 && i + 1 < toks.size()) {
            T y = T(43);
            bool ok = readPair<N, B, T>(in, x, y);
            r.push_back({ok, in.status, ulll(x), ulll(y), true});
            i += exp[i].st == St::Ok ? 2 : 1; continue;}
        string_view sv;
        bool ok = readVia<N, B, T>(in, x, api, sv);
        r.push_back({ok, in.status, ulll(x), 0, api != 1 || sv == toks[i]});
        ++i;}
    string_view sv;
    bool ok = readVia<N, B, T>(in, x, 0, sv);
    r.push_back({ok, in.status, ulll(x), 0, true});
    return r;}
template<typename T> void readCheck(int B, int N, Kind kind, const Stream &st, const vector<string> &toks, const vector<Got<T>> &exp, const vector<Obs> &obs) {
    T x = T(42);
    size_t k = 0;
    for (size_t i = 0; i < toks.size(); ++k) {
        int api = apiOf<T>(B, i);
        const Obs &o = obs[k];
        if (api == 4 && i + 1 < toks.size()) {
            const Got<T> &e = exp[i], &f = exp[i + 1];
            St want = e.st != St::Ok ? e.st : f.st;
            T y = want == St::Ok ? f.v : T(43);
            if (e.st == St::Ok) { x = e.v; }
            check(o.ok == (want == St::Ok) && o.st == want && o.x == ulll(x) && o.y == ulll(y), [&] { return intWhere(tname<T>(), B, N, kind, 4, st, toks, i) + " next=" + esc(toks[i + 1]) + " expected=" + stName(e.st) + "," + stName(f.st) + " got=" + stName(o.st) + " x=" + refText(T(o.x), 10) + " y=" + refText(T(o.y), 10); });
            mix(ulng(o.st)); mix(ulng(o.x)); mix(ulng(o.y >> 64));
            i += e.st == St::Ok ? 2 : 1; continue;}
        const Got<T> &e = exp[i];
        T before = x;
        if (e.st == St::Ok) { x = e.v; }
        if (api == 1) { check(o.view, [&] { return intWhere(tname<T>(), B, N, kind, 1, st, toks, i) + " readToken view differs from the token"; }); }
        check(o.ok == (e.st == St::Ok) && o.st == e.st && o.x == ulll(x), [&] { return intWhere(tname<T>(), B, N, kind, api, st, toks, i) + " expected=" + stName(e.st) + (e.st == St::Ok ? "(" + refText(e.v, 10) + ")" : "") + " got=" + (o.ok ? "true/" : "false/") + stName(o.st) + " x=" + refText(T(o.x), 10) + " (x before=" + refText(before, 10) + ")"; });
        mix(ulng(o.st)); mix(ulng(o.x)); mix(ulng(o.x >> 64));
        ++i;}
    const Obs &o = obs[k];
    check(k + 1 == obs.size() && !o.ok && o.st == St::Eof && o.x == ulll(x), [&] { return intWhere(tname<T>(), B, N, kind, 0, st, toks, toks.size()) + " read after the last token: expected Eof, got " + stName(o.st); });}
template<int N, int B, typename T> void readInts(const Stream &st, const vector<string> &toks, const vector<Got<T>> &exp, Kind kind) { readCheck<T>(B, N, kind, st, toks, exp, readRun<N, B, T>(st, toks, exp, kind)); }
// The writer's side: every form in turn into one sink; the text, and the first call that failed (or left pos or precision wrong), else npos.
template<int N, int B, typename T> [[gnu::noinline]] pair<string, size_t> writeRun(const vector<T> &vals) {
    Sink sink;
    size_t bad = string::npos;
    {
        FastOutput<N> out(sink.f);
        out.precision = CANARY;
        for (size_t i = 0; i < vals.size(); ++i) {
            T v = vals[i], w = vals[(7 * i + 3) % vals.size()], u = vals[(13 * i + 5) % vals.size()];
            bool ok = writeVia<N, B, T>(out, v, w, u, opOf<T>(B, i), endOf(i));
            if (!(ok && out.precision == CANARY && out.pos >= 0 && out.pos <= N) && bad == string::npos) { bad = i; }}}
    return {sink.text(), bad};}
template<typename T> void writeCheck(int B, int N, const vector<T> &vals, const pair<string, size_t> &run) {
    string want;
    vector<size_t> marks;
    for (size_t i = 0; i < vals.size(); ++i) {
        want += writeWant<T>(vals[i], vals[(7 * i + 3) % vals.size()], vals[(13 * i + 5) % vals.size()], opOf<T>(B, i), endOf(i), B);
        marks.push_back(want.size());}
    const string &got = run.first;
    auto where = [&](size_t op) { return "T=" + tname<T>() + " B=" + std::to_string(B) + " N=" + std::to_string(N) + " write#" + std::to_string(op) + " form=" + std::to_string(opOf<T>(B, op)) + " v=" + (op < vals.size() ? refText(vals[op], 10) : "-"); };
    check(run.second == string::npos, [&] { return where(run.second) + " returned false or left pos/precision wrong"; });
    if (got != want) {
        size_t at = 0;
        while (at < got.size() && at < want.size() && got[at] == want[at]) { ++at; }
        check(false, [&] { return where(size_t(std::upper_bound(marks.begin(), marks.end(), at) - marks.begin())) + " " + diffAt(got, want); });}
    mixText(got);}
template<int N, int B, typename T> void writeInts(const vector<T> &vals) { writeCheck<T>(B, N, vals, writeRun<N, B, T>(vals)); }

#if FASTIO_PART != 0
template<typename T> vector<T> sampleValues(int B) {
    using L = std::numeric_limits<T>;
    vector<T> a{T(0), T(1), T(2), L::max(), T(L::max() - 1), L::min(), T(L::min() + 1)};
    if constexpr (SIGNED_T<T>) { a.push_back(T(-1)); a.push_back(T(-2)); }
    ulll lim = ulll(L::max()), mag = magMax<T>(true);
    for (ulll p = 1;; p *= uint(B)) {
        for (ulll q : {p - 1, p, p + 1}) {
            if (q <= lim) { a.push_back(T(q)); }
            if (SIGNED_T<T> && q && q <= mag) { a.push_back(T(ulll(0) - q)); }}
        if (p > lim / uint(B)) { break; }}
    for (int k = 1; k < 8 * int(sizeof(T)); ++k) { a.push_back(T((ulll(1) << k) - 1)); }
    for (int i = 0; i < scale(16, 80, 400); ++i) { a.push_back(randomValue<T>()); }
    if constexpr (sizeof(T) == 1) {
        for (int v = 0; v < 256; ++v) { a.push_back(T(v)); }}
    return a;}
template<typename T> vector<string> intTokens(int B) {
    vector<string> t;
    for (T v : sampleValues<T>(B)) {
        string c = refText(v, B), z(size_t(rint(1, 24)), '0');
        t.push_back(c); t.push_back(lowered(c));
        t.push_back(isNeg(v) ? "-" + z + c.substr(1) : (rnd(2) ? "+" : "") + z + c);
        t.push_back(isNeg(v) ? "-0" + c.substr(1) : "+" + c);}
    for (int len = 1; len <= 40; ++len) {
        for (int pat = 0; pat < 4; ++pat) {
            string d = pat == 0 ? string(size_t(len), DIG[B - 1]) : pat == 1 ? "1" + string(size_t(len - 1), '0') : pat == 2 ? string(size_t(len - 1), '0') + DIG[1 + rnd(ulng(B - 1))] : randomDigits(len, B, false);
            for (const char *sg : {"", "+", "-"}) { t.push_back(sg + d); }}}
    // invalid bytes at the first, middle and last digit of the window lengths and their neighbours, and every nonspace byte mid-token
    for (int len : {1, 2, 8, 9, 15, 16, 17, 19, 20, 31, 32, 33, 38, 39, 63, 64, 65}) {
        string d = randomDigits(len, B, false);
        for (size_t pos : {size_t(0), size_t(len / 2), size_t(len - 1)}) {
            for (char c : BAD) { string x = d; x[pos] = c; t.push_back(x); t.push_back("-" + x); }}}
    string mid = randomDigits(19, B, true);
    for (char c : DATA) { string x = mid; x[9] = c; t.push_back(x); }
    string top = refText(std::numeric_limits<T>::max(), B), over = incDigits(top, B);
    for (int len : {16, 19, 20, 31, 32, 33, 38, 39, 63, 64, 65}) {   // the limit and limit + 1 padded by leading zeros to the window lengths
        if (int(top.size()) < len) { t.push_back(string(size_t(len) - top.size(), '0') + top); t.push_back(string(size_t(len) - top.size(), '0') + over); }}
    for (const char *pre : {"", "+", "000", "+00"}) { t.push_back(pre + over); t.push_back(pre + over + "!"); t.push_back(pre + over + "\x10"); t.push_back(string("\x19") + pre + over); }
    if constexpr (SIGNED_T<T>) {
        string under = incDigits(refText(std::numeric_limits<T>::min(), B).substr(1), B);
        t.push_back("-" + under); t.push_back("-00" + under);}
    for (int n : {64, 65, 300, 1000}) { t.push_back(string(size_t(n), DIG[B - 1])); }
    t.push_back(string(1000, DIG[B - 1]) + "#"); t.push_back(string(1000, '0') + "1"); t.push_back("-" + string(999, '0') + "1"); t.push_back(string(70, '0'));
    for (const char *s : {"+", "-", "+-1", "-+1", "++1", "--1", "+-0", "-0", "+0", "-00", "1-", "1+", "!", "#1", "1#", "\x80" "1", "1\x7F", "\xFF", "0x1F"}) { t.push_back(s); }
    t.push_back(string("2\0x", 3)); t.push_back(string(1, '\0'));
    if (B < 36) { t.push_back(string(1, DIG[B])); t.push_back("1" + string(1, char(std::tolower(DIG[B])))); }
    size_t n = t.size();
    for (int i = 0; i < scale(30, 150, 600); ++i) {
        string s = t[rnd(n)];
        s[rnd(s.size())] = DATA[rnd(DATA.size())];
        t.push_back(s);}
    std::shuffle(t.begin(), t.end(), rng);
    return t;}
// Each token alone at the start of the input (after lead spaces): the padded window near the buffer start.
template<int N, int B, typename T> [[gnu::noinline]] Obs aloneRun(const string &s, int api) {
    Source src(s, MEMORY);
    FastInput<N> in(src.f);
    T x = T(42);
    string_view sv;
    bool ok = readVia<N, B, T>(in, x, api, sv);
    return {ok, in.status, ulll(x), 0, true};}
template<int B, typename T> void aloneTokens(const vector<string> &toks, const vector<Got<T>> &exp) {
    vector<int> leads = mode == "quick" ? vector<int>{0, 32} : vector<int>{0, 1, 31, 32, 33};
    for (size_t i = 0; i < toks.size(); ++i) {
        if (toks[i].size() > 70) { continue; }
        for (int lead : leads) {
            string s = string(size_t(lead), ' ') + toks[i] + (i % 2 ? " " : "");
            for (int n = 0; n < 2; ++n) {
                Obs o = n ? aloneRun<64, B, T>(s, int(i % 2)) : aloneRun<0, B, T>(s, int(i % 2));
                const Got<T> &e = exp[i];
                check(o.ok == (e.st == St::Ok) && o.st == e.st && o.x == ulll(e.st == St::Ok ? e.v : T(42)), [&] { return "first bytes T=" + tname<T>() + " B=" + std::to_string(B) + " N=" + (n ? "64" : "0") + " api=" + API_NAMES[i % 2] + " input=" + esc(s) + " expected=" + stName(e.st) + " got=" + stName(o.st) + " x=" + refText(T(o.x), 10); });
                mix(ulng(o.st)); mix(ulng(o.x));}}}}
template<typename T, int B> void exhaustive16() {
    vector<string> toks;
    vector<T> vals;
    vector<Got<T>> exp;
    for (int v = 0; v < 65536; ++v) {
        T x = T(v);
        string c = refText(x, B);
        vals.push_back(x);
        toks.push_back(v % 4 == 1 ? lowered(c) : v % 4 == 2 ? (isNeg(x) ? "-0" + c.substr(1) : "+0" + c) : c);
        exp.push_back(refParse<T>(toks.back(), B));
        check(exp.back().st == St::Ok && exp.back().v == x, [&] { return "oracle round trip T=" + tname<T>() + " token=" + esc(toks.back()); });}
    Stream st = join(toks, false, false, 0);
    readInts<0, B, T>(st, toks, exp, REGULAR);
    readInts<65536, B, T>(st, toks, exp, PIPE);
    writeInts<65536, B, T>(vals);}
template<typename T, int B> void intCase() {
    vector<string> toks = intTokens<T>(B);
    vector<Got<T>> exp;
    for (const string &t : toks) { exp.push_back(refParse<T>(t, B)); }
    Stream plain = join(toks, false, B % 2 == 1, 0);
    readInts<64, B, T>(plain, toks, exp, MEMORY);
    if constexpr (isKey(B)) {
        readInts<64, B, T>(join(toks, true, true, 64), toks, exp, MEMORY);
        readInts<0, B, T>(plain, toks, exp, REGULAR);
        readInts<0, B, T>(plain, toks, exp, OFFSET);
        readInts<7, B, T>(plain, toks, exp, MEMORY);
        if constexpr (!QUICK) { readInts<1, B, T>(plain, toks, exp, PIPE); readInts<65536, B, T>(plain, toks, exp, CONSUMED); }}
    if constexpr (B == 10 || B == 16) { aloneTokens<B, T>(toks, exp); }
    vector<T> vals = sampleValues<T>(B);
    writeInts<64, B, T>(vals);
    if constexpr (!QUICK) { writeInts<65536, B, T>(vals); }
    if constexpr (isKey(B)) { writeInts<160, B, T>(vals); }
    if constexpr (isKey(B) && !QUICK) { writeInts<1, B, T>(vals); writeInts<320, B, T>(vals); }}
template<int B> void intBase() {
    enter("int-base-" + std::to_string(B));
    [&]<size_t... I>(std::index_sequence<I...>) { (intCase<std::tuple_element_t<I, IntTypes>, B>(), ...); }(std::make_index_sequence<17>{});
    if constexpr (isKey(B)) {
        if constexpr (!QUICK) {
            if (mode != "quick") { exhaustive16<int16_t, B>(); exhaustive16<uint16_t, B>(); exhaustive16<char16_t, B>(); }}}
    leave();}
template<int P, int B> void intIf() {
    if constexpr (partOf(B) == P && built(B)) { intBase<B>(); }}
template<int P> void intPart() { []<int... I>(std::integer_sequence<int, I...>) { (intIf<P, I + 2>(), ...); }(std::make_integer_sequence<int, 35>{}); }
// One protocol record: a token read as T in base B, echoed by writeInt, or its status letter and whether x changed.
template<int N, int B, typename T> void protoRecord(FastInput<N> &in, FastOutput<1 << 16> &out, lng k) {
    T x = T(42);
    string_view sv;
    if (readVia<N, B, T>(in, x, int(k % 2), sv)) {
        out.write("O ");
        writeVia<1 << 16, B, T>(out, x, x, x, 0, '\n');
        return;}
    out.put("OFIVE"[size_t(in.status)]);
    out.write(x == T(42) ? "\n" : "X\n");}
template<int P, int N, int B> bool protoBase(int code, int base, FastInput<N> &in, FastOutput<1 << 16> &out, lng k) {
    if constexpr (partOf(B) == P && built(B) && (N == 64 || isKey(B))) {
        if (base == B) {
            return [&]<size_t... J>(std::index_sequence<J...>) {
                return ((code == int(J) && (protoRecord<N, B, std::tuple_element_t<J, IntTypes>>(in, out, k), true)) || ...);}(std::make_index_sequence<17>{});}}
    return false;}
template<int P, int N> bool protoIntPart(int code, int base, FastInput<N> &in, FastOutput<1 << 16> &out, lng k) {
    return [&]<int... I>(std::integer_sequence<int, I...>) { return (protoBase<P, N, I + 2>(code, base, in, out, k) || ...); }(std::make_integer_sequence<int, 35>{});}
#else
template<int P> void intPart();
template<int P, int N> bool protoIntPart(int code, int base, FastInput<N> &in, FastOutput<1 << 16> &out, lng k);
#endif
#if FASTIO_PART > 0
template void intPart<FASTIO_PART>();
template bool protoIntPart<FASTIO_PART, 0>(int, int, FastInput<0> &, FastOutput<1 << 16> &, lng);
template bool protoIntPart<FASTIO_PART, 64>(int, int, FastInput<64> &, FastOutput<1 << 16> &, lng);
#endif

#if FASTIO_PART <= 0
template<typename T> void oracleType() {
    for (int i = 0; i < scale(2000, 10000, 50000); ++i) {
        int B = rint(2, 36);
        string t = rnd(4) == 0 ? "-" : rnd(4) == 0 ? "+" : "";
        for (int k = rint(1, 45); k > 0; --k) { t += rnd(30) == 0 ? "!+-/:@[`{\x7F"[rnd(10)] : DIG[rnd(ulng(B))]; }
        Got<T> a = charsParse<T>(t, B), b = loopParse<T>(t, B);
        check(a.st == b.st && (a.st != St::Ok || a.v == b.v), [&] { return "parse oracles disagree T=" + tname<T>() + " B=" + std::to_string(B) + " token=" + esc(t) + " from_chars=" + stName(a.st) + " loop=" + stName(b.st); });
        T v = randomValue<T>();
        string c = charsText(v, B), l = loopText(v, B);
        Got<T> r = loopParse<T>(l, B);
        check(c == l && r.st == St::Ok && r.v == v, [&] { return "text oracles disagree T=" + tname<T>() + " B=" + std::to_string(B) + " to_chars=" + c + " loop=" + l; });
        if (!isNeg(v) && v != std::numeric_limits<T>::max()) { check(incDigits(l, B) == loopText(T(v + 1), B), [&] { return "incDigits " + l; }); }}}
template<typename F> F strtoReal(const char *s, char **e) {
    if constexpr (std::is_same_v<F, float>) { return std::strtof(s, e); }
    else { return std::strtod(s, e); }}
// The status rule: from_chars Invalid for hex, '+-' and trailing bytes, Overflow when the correctly rounded value is infinite or zero from nonzero digits.
template<typename F> Got<F> refReal(const string &t) {
    size_t k = !t.empty() && (t[0] == '+' || t[0] == '-');
    string body = t.substr(k), low = lowered(body);
    if (t.empty() || (t[0] == '+' && !body.empty() && body[0] == '-') || low.starts_with("0x")) { return {St::Invalid, F()}; }
    char *e = nullptr;
    F v = strtoReal<F>(t.c_str(), &e);
    if (e != t.c_str() + t.size()) { return {St::Invalid, F()}; }
    bool special = low.starts_with("inf") || low.starts_with("nan");
    string mant = body.substr(0, body.find_first_of("eE"));
    if (!special && (std::isinf(v) || (v == 0 && mant.find_first_of("123456789") != string::npos))) { return {St::Overflow, F()}; }
    return {St::Ok, v};}
template<typename F> bool sameReal(F a, F b) {
    using U = std::conditional_t<sizeof(F) == 4, uint, ulng>;
    if (std::isnan(b)) { return std::isnan(a) && std::signbit(a) == std::signbit(b); }
    return std::bit_cast<U>(a) == std::bit_cast<U>(b);}
template<typename F> string realHex(F v) {
    char b[64]; std::snprintf(b, sizeof b, "%a", double(v));
    return b;}
// Exact decimal of the midpoint of f and the next F up, and decimals just above and below it (round-to-nearest-even and double-rounding hazards).
template<typename F> vector<string> midpoints(F f) {
    using W = std::conditional_t<sizeof(F) == 4, double, long double>;
    W mid = (W(f) + (f == std::numeric_limits<F>::max() ? std::ldexp(W(1), std::numeric_limits<F>::max_exponent) : W(std::nextafter(f, std::numeric_limits<F>::infinity())))) / 2;
    vector<char> b(1400);
    std::snprintf(b.data(), b.size(), "%.*Le", 1100, (long double)(mid));
    string s = b.data();
    size_t e = s.find('e');
    string m = s.substr(0, e), x = s.substr(e);
    while (m.back() == '0') { m.pop_back(); }
    string below = m;
    size_t d = below.back() == '.' ? below.size() - 2 : below.size() - 1;
    below[d] = char(below[d] - 1);
    below += below.back() == '.' ? "9999999" : "9999999";
    return {m + x, m + (m.back() == '.' ? "" : "") + "0000001" + x, below + x};}
template<typename F> vector<string> realTokens() {
    vector<string> t{"0", "-0", "+0", "+1.5", "-.5e3", "5.", ".5", "1E5", "1e-320", "4.9406564584124654e-324", "2.4703282292062328e-324", "2.4703282292062327e-324", "1.7976931348623157e308", "1.7976931348623158e308", "1.797693134862316e308",
        "123456789012345678901234567890", "0.1", "inf", "-INF", "+inf", "nan", "-nan", "+nan", "Infinity", "-infinity", "NaN(123)", "nan()", "1e400", "-1e400", "1e-400", "3.4028234e38", "3.4028235e38", "3.40282357e38", "3.4028236e38",
        "1e39", "-1e39", "1e-45", "1.4e-45", "7e-46", "7.1e-46", "1e-46", "1e-50", "1.17549435e-38", "1.1754942e-38", "0x1p3", "-0x1P3", "+-1", "1.5e", ".", "e5", "1,5", "--1", "+", "-", "1e+", "1e-5x", "1..2", "nan(", "infin",
        "00000000000000000000000000001", "1" + string(60, '0') + "e-60", "0e99999999999", "1e99999999999", "1e-99999999999", "0." + string(400, '0') + "1", string(400, '9'), string("1\0", 2)};
    using L = std::numeric_limits<F>;
    using U = std::conditional_t<sizeof(F) == 4, uint, ulng>;
    char b[1200];
    for (int i = 0; i < scale(300, 3000, 20000); ++i) {
        F v = std::bit_cast<F>(U(rng()));
        if (!std::isfinite(v)) { continue; }
        std::snprintf(b, sizeof b, i % 3 == 0 ? "%.*g" : i % 3 == 1 ? "%.*e" : "%.*f", i % 3 == 2 ? 50 : L::max_digits10 + int(rnd(3)) - 1, double(v));
        t.push_back(b);}
    for (F f : {F(0), L::denorm_min(), L::min(), F(1), L::max()}) {
        for (const string &s : midpoints(f)) { t.push_back(s); t.push_back("-" + s); }}
    for (int i = 0; i < scale(100, 1000, 6000); ++i) {
        F f = std::abs(std::bit_cast<F>(U(rng())));
        if (!std::isfinite(f)) { continue; }
        for (const string &s : midpoints(f)) { t.push_back(rnd(2) ? "-" + s : s); }}
    for (int i = 0; i < scale(300, 3000, 20000); ++i) {
        string m = randomDigits(rint(1, 25), 10, false);
        size_t dot = rnd(m.size() + 1);
        string s = (rnd(10) < 7 ? m.substr(0, dot) + "." + m.substr(dot) : m) + (rnd(10) < 6 ? string(1, "eE"[rnd(2)]) + array<const char *, 3>{"", "+", "-"}[rnd(3)] + std::to_string(rnd(sizeof(F) == 4 ? 60 : 330)) : "");
        t.push_back(array<const char *, 3>{"", "+", "-"}[rnd(3)] + s);}
    return t;}
template<int N, typename F> void readReals(const Stream &st, const vector<string> &toks, const vector<Got<F>> &exp, Kind kind) {
    Source src(st.s, kind);
    FastInput<N> in(src.f);
    F x = F(42);
    for (size_t i = 0; i < toks.size(); ++i) {
        F before = x;
        bool ok;
        if (i % 3 == 0) { ok = in.readDouble(x); }
        else if (i % 3 == 1) { ok = in.read(x); }
        else {
            std::tuple<F &> r{x};
            ok = in.read(r);}
        const Got<F> &e = exp[i];
        check(ok == (e.st == St::Ok) && in.status == e.st && sameReal(x, e.st == St::Ok ? e.v : before), [&] { return string(sizeof(F) == 4 ? "float" : "double") + " N=" + std::to_string(N) + " kind=" + KIND_NAMES[kind] + " token=" + esc(toks[i]) + " expected=" + stName(e.st) + " " + realHex(e.v) + " got=" + stName(in.status) + " " + realHex(x); });
        mix(ulng(in.status)); mix(ulng(std::bit_cast<std::conditional_t<sizeof(F) == 4, uint, ulng>>(x)));}
    F z = x;
    check(!in.readDouble(x) && in.status == St::Eof && sameReal(x, z), "real read after the last token");}
template<typename F> void reals() {
    enter(sizeof(F) == 4 ? "read-float" : "read-double");
    for (F f : {F(1), std::numeric_limits<F>::min(), F(1e10)}) {
        vector<string> m = midpoints(f);
        F up = std::nextafter(f, std::numeric_limits<F>::infinity());
        check(refReal<F>(m[1]).v == up && refReal<F>(m[2]).v == f, [&] { return "oracle rounding at the midpoint above " + realHex(f) + ": " + m[1]; });}
    vector<string> toks = realTokens<F>();
    vector<Got<F>> exp;
    for (const string &t : toks) { exp.push_back(refReal<F>(t)); }
    Stream st = join(toks, false, false, 0);
    readReals<0, F>(st, toks, exp, REGULAR);
    readReals<0, F>(st, toks, exp, PIPE);
    readReals<7, F>(st, toks, exp, MEMORY);
    readReals<65536, F>(st, toks, exp, CONSUMED);
    leave();}

// The byte classes over all 256 values: space, the AVX2 mask (each byte at each lane among random bytes, and random blocks), and tokenEnd's
// first whitespace (each byte at each offset of a run of random nonspace bytes, through the 64-, 32- and 8-byte steps and the byte tail).
inline void spaces() {
    enter("space-bytes");
    for (int c = 0; c < 256; ++c) {
        bool want = c == ' ' || (c >= '\t' && c <= '\r');
        check(FastInput<1>::space(c) == want, [&] { return "space(" + std::to_string(c) + ")"; });
#ifdef __AVX2__
        for (int at = 0; at < 32; ++at) {
            uint8_t b[32];
            for (uint8_t &x : b) { x = uint8_t(DATA[rnd(DATA.size())]); }
            b[at] = uint8_t(c);
            check(FastInput<1>::spaceMask(b) == (want ? 1u << at : 0u), [&] { return "spaceMask byte=" + std::to_string(c) + " at=" + std::to_string(at); });}
#endif
        for (int len : {1, 7, 8, 9, 31, 32, 33, 63, 64, 65, 100, 130}) {
            auto b = vector<uint8_t>(size_t(len));
            for (uint8_t &x : b) { x = uint8_t(DATA[rnd(DATA.size())]); }
            for (int at = 0; at < len; at += at < 70 ? 1 : 13) {
                b[size_t(at)] = uint8_t(c);
                const uint8_t *e = FastInput<1>::tokenEnd(b.data(), b.data() + len);
                check(e - b.data() == (want ? at : len), [&] { return "tokenEnd byte=" + std::to_string(c) + " at=" + std::to_string(at) + " len=" + std::to_string(len) + " got=" + std::to_string(e - b.data()); });
                b[size_t(at)] = uint8_t(DATA[rnd(DATA.size())]);}}
        mix(want);}
#ifdef __AVX2__
    for (int i = 0; i < scale(2000, 20000, 200000); ++i) {
        uint8_t b[32];
        uint want = 0;
        for (int k = 0; k < 32; ++k) {
            b[k] = uint8_t(rnd(4) ? rng() : ulng(WS[rnd(6)]));
            want |= uint(isWs(b[k])) << k;}
        check(FastInput<1>::spaceMask(b) == want, [&] { return "spaceMask random block " + esc(string_view(reinterpret_cast<const char *>(b), 32)); });}
#endif
    leave();}
inline bool decDigit(int c) { return c >= '0' && c <= '9'; }
inline bool hexDigit(int c) { return decDigit(c) || ((c | 0x20) >= 'a' && (c | 0x20) <= 'f'); }
// Kernel digit validity over all 256 bytes: dec19, hex16 and hex32 with byte c at each position of every token length (the window's
// bytes before the token random), decWord and hexWord at each byte, and the values of valid windows against a digit loop.
inline void kernels() {
    enter("kernels");
    alignas(64) char b[64];
    auto fill = [&](int len, int B) {
        for (int i = 0; i < 64 - len; ++i) { b[i] = char(rng()); }
        string d = randomDigits(len, B, false);
        std::memcpy(b + 64 - len, d.data(), size_t(len));};
    auto value = [&](int len, int B) {
        ulll v = 0;
        for (int i = 64 - len; i < 64; ++i) { v = v * uint(B) + digitOf(b[i]); }
        return v;};
    for (int kind = 0; kind < 3; ++kind) {
        int B = kind ? 16 : 10, top = kind == 0 ? 19 : kind == 1 ? 16 : 32;
        for (int len = 1; len <= top; ++len) {
            for (int pos = 0; pos < len; ++pos) {
                for (int c = 0; c < 256; ++c) {
                    fill(len, B);
                    b[64 - len + pos] = char(c);
                    bool want = true;
                    for (int i = 64 - len; i < 64; ++i) { want &= B == 10 ? decDigit(uint8_t(b[i])) : hexDigit(uint8_t(b[i])); }
                    ulng v = 0, hi = 0;
                    bool ok = kind == 0 ? fastio_detail::dec19(b + 32, len, v) : kind == 1 ? fastio_detail::hex16(b + 48, len, v) : fastio_detail::hex32(b + 32, len, v, hi);
                    check(ok == want && (!ok || (ulll(hi) << 64 | v) == value(len, B)), [&] { return array<const char *, 3>{"dec19", "hex16", "hex32"}[size_t(kind)] + string(" len=") + std::to_string(len) + " pos=" + std::to_string(pos) + " byte=" + std::to_string(c) + " window=" + esc(string_view(b, 64)) + " ok=" + std::to_string(ok); });
                    mix(ok);
                    if (ok) { mix(v); mix(hi); }}}}}
    for (int pos = 0; pos < 8; ++pos) {
        for (int c = 0; c < 256; ++c) {
            for (int B : {10, 16}) {
                string d = randomDigits(8, B, false);
                d[size_t(pos)] = char(c);
                ulng w;
                std::memcpy(&w, d.data(), 8);
                bool want = std::ranges::all_of(d, [&](char x) { return B == 10 ? decDigit(uint8_t(x)) : hexDigit(uint8_t(x)); });
                bool ok = B == 10 ? fastio_detail::decWord(w) : fastio_detail::hexWord(w);
                check(ok == want, [&] { return (B == 10 ? "decWord " : "hexWord ") + esc(d); });
                if (ok) {
                    ulll v = 0;
                    for (char x : d) { v = v * uint(B) + digitOf(x); }
                    check((B == 10 ? fastio_detail::dec8(w) : fastio_detail::hex8(w)) == v, [&] { return (B == 10 ? "dec8 " : "hex8 ") + esc(d); });}}}}
    leave();}
template<int N> void gridRead(const string &s, Kind kind) {
    Source src(s, kind);
    FastInput<N> in(src.f);
    Model m{s};
    string_view sv;
    size_t a = 0, b = 0;
    auto where = [&] { return "N=" + std::to_string(N) + " kind=" + KIND_NAMES[kind] + " input=" + esc(s); };
    while (m.token(a, b)) {
        bool ok = in.readToken(sv);
        check(ok && in.status == St::Ok && sv == string_view(s).substr(a, b - a), [&] { return where() + " token at " + std::to_string(a) + " expected " + esc(s.substr(a, b - a)) + " got " + (ok ? esc(sv) : stName(in.status)); });
        check(!ok || viewAt(in, sv, a, b, s.size(), src.skip), [&] { return where() + " token at " + std::to_string(a) + " view points to the wrong storage"; });
        check(in.peek() == m.peek(), [&] { return where() + " delimiter after token at " + std::to_string(a) + " not preserved"; });
        mix(a); mix(b);}
    string_view old = sv;
    bool ok = in.readToken(sv);
    check(!ok && in.status == St::Eof && sv.data() == old.data() && sv.size() == old.size(), [&] { return where() + " EOF readToken"; });
    check(!in.readToken(sv) && in.status == St::Eof, [&] { return where() + " repeated EOF"; });}
string gridBytes(int len, ulng key) {
    std::mt19937_64 g(key);
    string r;
    for (int i = 0; i < len; ++i) { r += DATA[g() % DATA.size()]; }
    return r;}
inline void tokenGrid() {
    enter("token-grid");
    vector<int> ws, lens;
    if (mode == "quick") { ws = {0, 1, 2, 3, 15, 16, 17, 30, 31, 32, 33, 34, 62, 63, 64, 65, 66}; lens = {0, 1, 2, 3, 7, 8, 9, 15, 16, 17, 30, 31, 32, 33, 34, 62, 63, 64, 65, 66, 94, 95, 96, 97, 98, 127, 128, 129, 200}; }
    else {
        for (int w = 0; w <= 70; ++w) { ws.push_back(w); }
        for (int l = 0; l <= 140; ++l) { lens.push_back(l); }
        for (int l : {150, 191, 192, 193, 199, 200}) { lens.push_back(l); }}
    for (int w : ws) {
        for (int len : lens) {
            for (int suffix = 0; suffix < 3; ++suffix) {
                ulng key = ulng(w) * 1000003 + ulng(len) * 7 + ulng(suffix);
                string s;
                for (int i = 0; i < w; ++i) { s += WS[size_t(i + w + len) % 6]; }
                s += gridBytes(len, key);
                if (suffix == 1) { s += WS[size_t(len) % 6]; }
                if (suffix == 2) { s += WS[size_t(len + 1) % 6] + gridBytes(1 + len % 40, ~key) + (len % 2 ? " " : ""); }
                gridRead<0>(s, REGULAR); gridRead<0>(s, MEMORY); gridRead<1>(s, MEMORY); gridRead<7>(s, MEMORY); gridRead<31>(s, MEMORY);
                gridRead<32>(s, MEMORY); gridRead<33>(s, MEMORY); gridRead<64>(s, MEMORY); gridRead<65536>(s, MEMORY);}}}
    leave();}
string randomStream(size_t bytes) {
    string s;
    while (s.size() < bytes) {
        int r = rint(0, 99), w = r < 80 ? rint(1, 3) : r < 90 ? rint(31, 33) : r < 96 ? rint(63, 65) : rint(1, 200);
        s += wsRun(s.empty() ? int(rnd(2)) * w : w);
        int q = rint(0, 99), len = q < 50 ? rint(1, 8) : q < 80 ? rint(29, 36) : q < 90 ? rint(61, 68) : q < 97 ? rint(93, 100) : rint(1, 200);
        s += gridBytes(len, rng());}
    if (rnd(2)) { s += wsRun(rint(1, 3)); }
    return s;}
// Random interleavings of every byte and token API against the model; for N = 0 every view stays checked until the reader dies.
template<int N> void mixedOps(const string &s, Kind kind, ulng key) {
    Source src(s, kind);
    FastInput<N> in(src.f);
    Model m{s};
    std::mt19937_64 r(key);
    vector<pair<string_view, size_t>> kept;
    string_view sv;
    string str = "init", line;
    char c = '?';
    auto where = [&](const char *op) { return "N=" + std::to_string(N) + " kind=" + KIND_NAMES[kind] + " stream bytes=" + std::to_string(s.size()) + " key=" + std::to_string(key) + " op=" + op + " at model offset " + std::to_string(m.p); };
    for (int idle = 0; idle < 3;) {
        int op = int(r() % 20);
        size_t a = 0, b = 0;
        if (m.p == s.size()) { ++idle; }
        if (op < 10) {
            string_view old = sv;
            bool want = m.token(a, b), ok = op < 8 ? in.readToken(sv) : in.read(sv);
            if (!want) { check(!ok && in.status == St::Eof && sv.data() == old.data(), [&] { return where("readToken at EOF"); }); continue; }
            check(ok && in.status == St::Ok && sv == string_view(s).substr(a, b - a) && viewAt(in, sv, a, b, s.size(), src.skip), [&] { return where("readToken") + " expected " + esc(s.substr(a, b - a)) + " got " + esc(sv); });
            if constexpr (N == 0) { kept.push_back({sv, a}); }}
        else if (op < 12) {
            string old = str;
            bool want = m.token(a, b), ok = op == 10 ? in.readToken(str) : in.read(str);
            check(ok == want && in.status == (want ? St::Ok : St::Eof) && str == (want ? s.substr(a, b - a) : old), [&] { return where("readToken(string)"); });}
        else if (op == 12) {
            char old = c;
            bool want = m.skip(), ok = in.readChar(c);
            check(ok == want && c == (want ? s[m.p++] : old) && in.status == (want ? St::Ok : St::Eof), [&] { return where("readChar"); });}
        else if (op == 13) {
            int want = m.get(), got = in.get();
            check(got == want && in.status == (want == EOF ? St::Eof : St::Ok), [&] { return where("get") + " expected " + std::to_string(want) + " got " + std::to_string(got); });}
        else if (op == 14) { check(in.peek() == m.peek(), [&] { return where("peek"); }); }
        else if (op == 15) { check(in.skipSpace() == m.skip(), [&] { return where("skipSpace"); }); }
        else if (op == 16 && r() % 4 == 0) {
            string want, old = line;
            bool w = m.line(want), ok = in.readLine(line);
            check(ok == w && line == (w ? want : old) && in.status == (w ? St::Ok : St::Eof), [&] { return where("readLine") + " expected " + esc(want) + " got " + esc(line); });}
        else if (op >= 17) {
            string old = str;
            string_view oldv = sv;
            size_t a2 = 0, b2 = 0;
            bool w1 = m.token(a, b), w2 = w1 && m.token(a2, b2), ok = in.read(str, sv);
            check(ok == w2 && str == (w1 ? s.substr(a, b - a) : old) && (w2 ? sv == string_view(s).substr(a2, b2 - a2) : sv.data() == oldv.data()), [&] { return where("read(string, string_view)"); });
            if constexpr (N == 0) {
                if (w2) { kept.push_back({sv, a2}); }}}
        mix(m.p); mix(ulng(in.status));}
    if constexpr (N == 0) {
        for (auto &[v, a] : kept) { check(v == string_view(s).substr(a, v.size()), [&] { return where("view lifetime") + " view of offset " + std::to_string(a) + " changed"; }); }}}
inline void tokenStreams() {
    enter("token-streams");
    vector<size_t> sizes = mode == "quick" ? vector<size_t>{31, 200, 20000} : mode == "full" ? vector<size_t>{31, 200, 5000, 70000, 300000} : vector<size_t>{31, 200, 5000, 70000, 300000, 2000000};
    for (size_t n : sizes) {
        for (int rep = 0; rep < (n < 1000 ? scale(20, 100, 400) : 1); ++rep) {
            string s = randomStream(n);
            ulng key = rng();
            mixedOps<0>(s, REGULAR, key); mixedOps<0>(s, OFFSET, key); mixedOps<0>(s, CONSUMED, key); mixedOps<0>(s, PIPE, key); mixedOps<0>(s, MEMORY, key);
            mixedOps<1>(s, PIPE, key); mixedOps<7>(s, MEMORY, key); mixedOps<64>(s, REGULAR, key); mixedOps<65536>(s, PIPE, key); mixedOps<65536>(s, CONSUMED, key);}}
    leave();}

inline void oracles() {
    enter("oracles");
    [&]<size_t... I>(std::index_sequence<I...>) { (oracleType<std::tuple_element_t<I, IntTypes>>(), ...); }(std::make_index_sequence<17>{});
    leave();}

// Text types: x.read is the oracle of readText, InfInt's toString of writeText (validated by a word dump in base 16 and by the Python protocol).
struct Probe {
    string last;
    uint base = 0;
    int calls = 0, result = 0;

    Probe() {}
    bool read(string_view s, uint b) { last = s; base = b; ++calls; return s.size() % 3 != 0; }
};
static_assert(fastio_detail::TextIn<Probe> && !fastio_detail::Exact<Probe, lng> && !fastio_detail::TextOut<Probe>);
static_assert(fastio_detail::Exact<InfInt, lll> && fastio_detail::Exact<InfIntMini, lll> && fastio_detail::Exact<Rational<lll>, lll> && fastio_detail::Exact<Rational<InfInt>, lll>);
static_assert(fastio_detail::Exact<Rational<lng>, lng> && !fastio_detail::Exact<Rational<lng>, lll> && fastio_detail::TextOut<InfInt> && !fastio_detail::TextOut<InfIntMini>);
inline bool same(const InfInt &a, const InfInt &b) { return a == b && a.sgn == b.sgn && a.is_inf == b.is_inf && a.toString(16) == b.toString(16); }
inline bool same(const InfIntMini &a, const InfIntMini &b) { return a == b && a.sgn == b.sgn && a.is_inf == b.is_inf && a.toString(10) == b.toString(10); }
template<typename T> bool same(const Rational<T> &a, const Rational<T> &b) {
    if constexpr (std::is_same_v<T, InfInt>) { return same(a.num, b.num) && same(a.den, b.den); }
    else { return a.num == b.num && a.den == b.den; }}
inline string show(const InfInt &a) { string s = a.toString(10); return s.size() > 90 ? s.substr(0, 90) + "..." : s; }
inline string show(const InfIntMini &a) { string s = a.toString(10); return s.size() > 90 ? s.substr(0, 90) + "..." : s; }
template<typename T> string show(const Rational<T> &a) { string s = a.toString(10); return s.size() > 90 ? s.substr(0, 90) + "..." : s; }
template<typename X> string xname() {
    if constexpr (std::is_same_v<X, InfInt>) { return "InfInt"; }
    else if constexpr (std::is_same_v<X, InfIntMini>) { return "InfIntMini"; }
    else if constexpr (std::is_same_v<X, Rational<lng>>) { return "Rational<lng>"; }
    else if constexpr (std::is_same_v<X, Rational<lll>>) { return "Rational<lll>"; }
    else { return "Rational<InfInt>"; }}
inline vector<string> textTokens(int B) {
    vector<string> t;
    for (int len = 1; len <= 40; ++len) {
        for (int pat = 0; pat < 4; ++pat) {
            string d = pat == 0 ? string(size_t(len), DIG[B - 1]) : pat == 1 ? "1" + string(size_t(len - 1), '0') : pat == 2 ? string(size_t(len - 1), '0') + DIG[1 + rnd(ulng(B - 1))] : randomDigits(len, B, false);
            for (const char *sg : {"", "+", "-"}) { t.push_back(sg + d); }}}
    ulll e19 = 10000000000000000000ULL;
    for (ulll v : {ulll(LLONG_MAX), ulll(LLONG_MAX) + 1, ulll(ULNG_MAX), ulll(ULNG_MAX) + 1, (ulll(1) << 124) - 1, ulll(1) << 124, ulll(LLL_MAX), ulll(LLL_MAX) + 1, ULLL_MAX, e19 / 10 - 1, e19 / 10, e19 - 1, e19, e19 * e19 - 1, e19 * e19}) {
        string c = loopText(v, B);
        for (const char *sg : {"", "+", "-"}) { t.push_back(sg + c); t.push_back(sg + string("00") + c); }}
    t.push_back(incDigits(loopText(ULLL_MAX, B), B)); t.push_back("-" + incDigits(loopText(ULLL_MAX, B), B));
    for (const char *s : {"+", "-", "+-1", "-+1", "--1", "++1", "inf", "-inf", "+inf", "INF", "iNf", "infinity", "in", "-in", "1/2", "-3/6", "+4/-2", "1/0", "0/5", "1.5", "-.25", "1e3", "2.5e-3", "1/", "/2", "1//2", "1.2.3", "A/B", "-0", "+0", "00", "-000"}) { t.push_back(s); }
    for (int n : {50, 64, 65, 100}) { t.push_back((rnd(2) ? "-" : "") + randomDigits(n, B, true)); }
    if (mode != "quick") { t.push_back(randomDigits(1000, B, true)); t.push_back("-" + randomDigits(1000, B, false)); }
    if (B == 10 || B == 16) {
        for (int len = 1; len <= 40; ++len) {
            if (mode == "quick" && len % 7 != 1 && len != 18 && len != 19 && len != 31 && len != 32) { continue; }
            string d = randomDigits(len, B, false);
            for (size_t pos : {size_t(0), size_t(len / 2), size_t(len - 1)}) {
                for (char c : BAD) {
                    string s = d;
                    s[pos] = c;
                    t.push_back(s);}}}}
    size_t n = t.size();
    for (int i = 0; i < scale(30, 100, 400); ++i) {
        string s = t[rnd(n)];
        s[rnd(s.size())] = BAD[rnd(BAD.size())];
        t.push_back(s);}
    std::shuffle(t.begin(), t.end(), rng);
    return t;}
// One text read as observed: the result, the status and the destination.
template<typename X> struct TextObs { bool ok; St st; X x; };
template<int N, int B, typename X> [[gnu::noinline]] vector<TextObs<X>> textRun(const string &s, size_t count, Kind kind, const X &init) {
    Source src(s, kind);
    FastInput<N> in(src.f);
    X x = init;
    vector<TextObs<X>> r;
    for (size_t i = 0; i <= count; ++i) {
        bool ok = i % 3 ? in.template readText<B>(x) : in.template read<B>(x);
        r.push_back({ok, in.status, x});}
    return r;}
template<typename X> void textCheck(int B, int N, Kind kind, const Stream &st, const vector<string> &toks, const X &init, const vector<TextObs<X>> &obs) {
    X x = init;
    for (size_t i = 0; i < toks.size(); ++i) {
        X y = x;
        bool want = y.read(toks[i], uint(B));
        const TextObs<X> &o = obs[i];
        if (want) { x = y; }
        check(o.ok == want && o.st == (want ? St::Ok : St::Invalid) && same(o.x, x), [&] { return xname<X>() + " B=" + std::to_string(B) + " N=" + std::to_string(N) + " kind=" + KIND_NAMES[kind] + " token=" + esc(toks[i]) + " offset=" + std::to_string(st.at[i]) + " x.read=" + std::to_string(want) + " (" + show(x) + ") readText=" + std::to_string(o.ok) + "/" + stName(o.st) + " (" + show(o.x) + ")"; });
        mix(o.ok); mix(ulng(o.st));}
    const TextObs<X> &o = obs.back();
    check(!o.ok && o.st == St::Eof && same(o.x, x), [&] { return xname<X>() + " B=" + std::to_string(B) + " N=" + std::to_string(N) + " readText after the last token: " + stName(o.st); });}
template<int N, int B, typename X> void readTexts(const Stream &st, const vector<string> &toks, Kind kind, const X &init) { textCheck<X>(B, N, kind, st, toks, init, textRun<N, B, X>(st.s, toks.size(), kind, init)); }
template<typename X> void aloneCheck(int B, int N, const string &tok, const X &init, const string &s, const TextObs<X> &o) {
    X y = init;
    bool want = y.read(tok, uint(B));
    check(o.ok == want && o.st == (want ? St::Ok : St::Invalid) && same(o.x, want ? y : init), [&] { return "first bytes " + xname<X>() + " B=" + std::to_string(B) + " N=" + std::to_string(N) + " input=" + esc(s) + " x.read=" + std::to_string(want) + " readText=" + std::to_string(o.ok) + " (" + show(o.x) + " vs " + show(y) + ")"; });
    mix(o.ok);}
template<int N, int B, typename X> void aloneText(const string &tok, const X &init, int lead) {
    string s = string(size_t(lead), ' ') + tok;
    aloneCheck<X>(B, N, tok, init, s, textRun<N, B, X>(s, 0, MEMORY, init)[0]);}
template<typename X, int B> void textBase(const X &init) {
    vector<string> toks = textTokens(B);
    Stream plain = join(toks, false, true, 0);
    readTexts<0, B, X>(plain, toks, REGULAR, init);
    readTexts<64, B, X>(plain, toks, MEMORY, init);
    if constexpr (B == 10 || B == 16) {
        readTexts<64, B, X>(join(toks, true, true, 64), toks, MEMORY, init);
        readTexts<7, B, X>(plain, toks, MEMORY, init);
        readTexts<0, B, X>(plain, toks, OFFSET, init);
        if constexpr (!QUICK) { readTexts<1, B, X>(plain, toks, PIPE, init); readTexts<65536, B, X>(plain, toks, CONSUMED, init); }
        for (const string &t : toks) {
            if (t.size() > 70) { continue; }
            for (int lead : {0, 32}) { aloneText<0, B, X>(t, init, lead); aloneText<64, B, X>(t, init, lead); }}}}
template<typename X, int B> void textCase(const X &init) {
    if constexpr (built(B)) { textBase<X, B>(init); }}
// Probe reads: every token reaches x.read whole with the base, and its verdict is the status.
template<int N, int B> [[gnu::noinline]] vector<Probe> probeRun(const string &s, size_t count, Kind kind) {
    Source src(s, kind);
    FastInput<N> in(src.f);
    Probe x;
    vector<Probe> r;
    for (size_t i = 0; i <= count; ++i) {
        bool ok = i % 2 ? in.template readText<B>(x) : in.template read<B>(x);
        r.push_back(x);
        r.back().result = ok ? int(in.status) : -1 - int(in.status);}
    return r;}
inline void probeRead(int B, int N, const vector<string> &toks, const vector<Probe> &obs) {
    for (size_t i = 0; i < toks.size(); ++i) {
        bool want = toks[i].size() % 3 != 0;
        const Probe &o = obs[i];
        check(o.result == (want ? 0 : -1 - int(St::Invalid)) && o.calls == int(i) + 1 && o.last == toks[i] && o.base == uint(B), [&] { return "Probe B=" + std::to_string(B) + " N=" + std::to_string(N) + " token=" + esc(toks[i]) + " passed=" + esc(o.last) + " base=" + std::to_string(o.base) + " result=" + std::to_string(o.result) + " calls=" + std::to_string(o.calls); });
        mix(want);}
    check(obs.back().result == -1 - int(St::Eof) && obs.back().calls == int(toks.size()), [&] { return "Probe B=" + std::to_string(B) + " N=" + std::to_string(N) + " after the last token"; });}
template<int N, int B> void probeReads(const vector<string> &toks, Kind kind) { probeRead(B, N, toks, probeRun<N, B>(join(toks, true, false, 0).s, toks.size(), kind)); }
template<typename X, int... Bs> void textSuite(const X &init) {
    enter("text-read-" + xname<X>());
    (textCase<X, Bs>(init), ...);
    leave();}
template<int... I> void textAllBases(std::integer_sequence<int, I...>) {
    textSuite<InfInt, (I + 2)...>(InfInt(-777));
    enter("text-read-Probe");
    ([&] {
        if constexpr (built(I + 2)) {
            vector<string> toks = textTokens(I + 2);
            probeReads<0, I + 2>(toks, REGULAR); probeReads<64, I + 2>(toks, MEMORY);}}(), ...);
    leave();}
inline string hexDump(const InfInt &a) {
    if (a.is_inf) { return a.sgn < 0 ? "-inf" : "inf"; }
    string s;
    for (int i = a.n.size() - 1; i >= 0; --i) {
        for (int k = 60; k >= 0; k -= 4) { s += DIG[(a.n[i] >> k) & 15]; }}
    size_t z = s.find_first_not_of('0');
    s = z == string::npos ? "0" : s.substr(z);
    return (a.sgn < 0 && s != "0" ? "-" : "") + s;}
inline vector<InfInt> textValues() {
    vector<InfInt> v{InfInt(0), InfInt(1), InfInt(-1), InfInt(ULNG_MAX), -InfInt(ULNG_MAX), InfInt(ULLL_MAX), -InfInt(ULLL_MAX), InfInt(ULLL_MAX) + InfInt(1), InfInt(LLL_MIN), InfInt("inf"), InfInt("-inf")};
    for (int i = 0; i < scale(30, 120, 500); ++i) {
        InfInt x(randomDigits(rint(1, i % 10 == 0 ? 3000 : 80), 16, true), 16);
        v.push_back(rnd(2) ? -x : x);}
    return v;}
template<int N, int B> [[gnu::noinline]] pair<string, size_t> writeTextRun(const vector<InfInt> &vals) {
    Sink sink;
    size_t bad = string::npos;
    {
        FastOutput<N> out(sink.f);
        out.precision = CANARY;
        for (size_t i = 0; i < vals.size(); ++i) {
            const InfInt &v = vals[i];
            int op = int(i % 7);
            bool ok = op == 0 ? out.template writeText<B>(v) : op == 1 ? out.template writeValue<B>(v) : op == 2 ? out.template print<B>(v) : op == 3 ? out.template print<B>(v, int(i))
                : op == 4 ? out.template print<B>(vector<InfInt>{v, v}) : op == 5 ? out.template print<B>(pair<InfInt, int>{v, -1}) : out.template print<B>(std::tuple<int, InfInt, string>{-1, v, "s"});
            if (!(ok && out.precision == CANARY) && bad == string::npos) { bad = i; }}}
    return {sink.text(), bad};}
inline void writeTextCheck(int B, int N, const vector<InfInt> &vals, const pair<string, size_t> &run) {
    string want;
    for (size_t i = 0; i < vals.size(); ++i) {
        string t = vals[i].toString(uint(B));
        int op = int(i % 7);
        want += op < 2 ? t : op == 2 ? t + '\n' : op == 3 ? t + ' ' + refText(int(i), B) + '\n' : op == 4 ? t + ' ' + t + '\n' : op == 5 ? t + " -1\n" : "-1 " + t + " s\n";}
    check(run.second == string::npos, [&] { return "writeText B=" + std::to_string(B) + " N=" + std::to_string(N) + " op#" + std::to_string(run.second % 7) + " value=" + show(vals[run.second]) + " returned false"; });
    check(run.first == want, [&] { return "writeText B=" + std::to_string(B) + " N=" + std::to_string(N) + " " + diffAt(run.first, want); });
    mixText(run.first);}
template<int N, int B> void writeTexts(const vector<InfInt> &vals) {
    if constexpr (built(B)) { writeTextCheck(B, N, vals, writeTextRun<N, B>(vals)); }}
template<int... I> void writeTextBases(const vector<InfInt> &vals, std::integer_sequence<int, I...>) { ((writeTexts<64, I + 2>(vals), writeTexts<65536, I + 2>(vals)), ...); }
// Text length against the room left: the buffer filled to N - k, then values of d digits for d around k and N.
template<int N, int B> void textRoom() {
    for (int k = 0; k <= min(N, 64); ++k) {
        for (int d : {1, 2, k - 1, k, k + 1, k + 2, N - 1, N, N + 1, 2 * N}) {
            if (d < 1) { continue; }
            string digits = randomDigits(d, B, true), up;
            for (char c : digits) { up += char(std::toupper(c)); }
            bool neg = (k + d) % 2;
            InfInt x(digits, uint(B));
            if (neg) { x = -x; }
            string t = (neg ? "-" : "") + up;
            Sink sink;
            string want(size_t(N - k), 'f');
            {
                FastOutput<N> out(sink.f);
                out.precision = CANARY;
                bool ok = out.write(want) && out.template writeText<B>(x) && out.template writeText<B>(x) && out.put('|');
                check(ok && out.precision == CANARY, [&] { return "text room N=" + std::to_string(N) + " B=" + std::to_string(B) + " k=" + std::to_string(k) + " digits=" + std::to_string(d); });}
            want += t + t + '|';
            string got = sink.text();
            check(got == want, [&] { return "text room N=" + std::to_string(N) + " B=" + std::to_string(B) + " k=" + std::to_string(k) + " digits=" + std::to_string(d) + " " + diffAt(got, want); });
            mixText(got);}}}
inline void textWrites() {
    enter("text-write");
    vector<InfInt> vals = textValues();
    for (const InfInt &v : vals) { check(hexDump(v) == v.toString(16), [&] { return "InfInt toString(16) against the word dump: " + show(v); }); }
    writeTextBases(vals, std::make_integer_sequence<int, 35>{});
    writeTexts<160, 10>(vals); writeTexts<4096, 16>(vals); writeTexts<1, 2>(vals);
    textRoom<64, 16>(); textRoom<64, 10>(); textRoom<160, 16>(); textRoom<160, 10>();
    if (mode != "quick") { textRoom<4096, 16>(); textRoom<4096, 10>(); }
    leave();
    enter("text-write-big");
    vector<pair<int, int>> big{{16, 524286}, {16, 524287}, {16, 524288}, {16, 3 << 20}, {10, mode == "quick" ? 100000 : 600000}};
    for (auto [B, d] : big) {
        string digits = randomDigits(d, B, true), up;
        for (char c : digits) { up += char(std::toupper(c)); }
        InfInt x(digits, uint(B));
        check(x.charsBound(uint(B)) >= d, "charsBound");
        for (int small = 0; small < 2; ++small) {
            Sink sink;
            {
                FastOutput<1 << 16> out(sink.f);
                FastOutput<64> tiny(sink.f);
                bool ok = small ? tiny.write("ab") && (B == 16 ? tiny.writeText<16>(x) : tiny.writeText<10>(x)) && tiny.put('\n') && tiny.flush() : out.write("ab") && (B == 16 ? out.writeText<16>(x) : out.writeText<10>(x)) && out.put('\n') && out.flush();
                check(ok, [&] { return "big writeText digits=" + std::to_string(d) + " B=" + std::to_string(B); });}
            string got = sink.text(), want = "ab" + up + "\n";
            check(got == want, [&] { return "big writeText digits=" + std::to_string(d) + " B=" + std::to_string(B) + " N=" + (small ? "64 " : "65536 ") + diffAt(got, want); });
            mix(got.size());}}
    leave();}

// The buffer filled to N - k, then 128-bit extremes by writeInt and print: the room check and the drain at every fill level.
template<int N, int B> [[gnu::noinline]] pair<string, bool> writeRoomRun(int k) {
    Sink sink;
    bool ok;
    {
        FastOutput<N> out(sink.f);
        out.precision = CANARY;
        ok = out.write(string(size_t(N - k), 'f')) && out.template writeInt<B>(LLL_MIN, '\n') && out.template writeInt<B>(ULLL_MAX) && out.template writeInt<B>(int8_t(-128), ' ') && out.template print<B>(LLL_MIN, ULLL_MAX)
            && out.template print<B>(ulng(0), lll(-1), ULLL_MAX) && out.template writeInt<B>(0);
        ok = ok && out.precision == CANARY && out.pos <= N;}
    return {sink.text(), ok};}
inline void writeRoomCheck(int B, int N, int k, const pair<string, bool> &run) {
    string want = string(size_t(N - k), 'f') + refText(LLL_MIN, B) + '\n' + refText(ULLL_MAX, B) + refText(int8_t(-128), B) + ' ' + refText(LLL_MIN, B) + ' ' + refText(ULLL_MAX, B) + "\n0 -1 " + refText(ULLL_MAX, B) + "\n0";
    check(run.second && run.first == want, [&] { return "write room N=" + std::to_string(N) + " B=" + std::to_string(B) + " k=" + std::to_string(k) + " returned " + std::to_string(run.second) + " " + diffAt(run.first, want); });
    mixText(run.first);}
template<int N, int B> void writeRoomOne() {
    for (int k = 0; k <= min(N, 170); ++k) { writeRoomCheck(B, N, k, writeRoomRun<N, B>(k)); }}
template<int B> void writeRoomBase() {
    writeRoomOne<64, B>(); writeRoomOne<159, B>(); writeRoomOne<160, B>(); writeRoomOne<161, B>(); writeRoomOne<200, B>();
    writeRoomOne<319, B>(); writeRoomOne<320, B>(); writeRoomOne<321, B>(); writeRoomOne<480, B>(); writeRoomOne<4096, B>();}
inline void writeRoom() {
    enter("write-room");
    writeRoomBase<2>(); writeRoomBase<7>(); writeRoomBase<10>(); writeRoomBase<16>(); writeRoomBase<36>();
    leave();}

// Print forms: the contract writes char through put, bool as 0/1, print() as "\n", tuples element-wise and an empty tuple as nothing.
template<int N> void printFormsOne() {
    Sink sink;
    {
        FastOutput<N> out(sink.f);
        out.precision = 3;
        bool ok = out.print() && out.print('c') && out.print(1, 'c') && out.print('a', 'b') && out.print(1, std::tuple<>{}, 2) && out.print(std::tuple<>{}) && out.writeValue(std::tuple<>{}) && out.put('|');
        ok = ok && out.print(int8_t(-8), uint8_t(200), char8_t(7), char16_t(65535), wchar_t(-3)) && out.print(true, false) && out.print(vector<bool>{true, false, true}) && out.print(1, true) && out.template print<16>(true, 255);
        ok = ok && out.print(std::tuple<bool, int, string>{false, 2, "s"}) && out.print(pair<bool, bool>{true, true}) && out.writeValue(vector<bool>{}) && out.print(array<bool, 2>{false, true}) && out.writeValue(true) && out.writeValue(false) && out.put('|');
        ok = ok && out.print(pair<int, string>{3, "x"}) && out.print(std::tuple<int, vector<int>, pair<char, double>>{1, {2, 3}, {'c', 0.5}}) && out.template print<16>(std::tuple<int, ulll, InfInt>{255, ULLL_MAX, InfInt(-255)});
        ok = ok && out.print(vector<pair<int, int>>{{1, 2}, {3, 4}}) && out.print(array<int, 3>{1, 2, 3}) && out.writeValue(std::tuple<std::tuple<>, int>{std::tuple<>{}, 5}) && out.print(std::tuple<float, double>{0.25f, -1.5}) && out.template print<2>(std::tuple<string_view, char, uint8_t>{"sv", 'z', 5});
        check(ok, [&] { return "print forms N=" + std::to_string(N) + " returned false"; });}
    string want = "\nc\n1 c\na b\n1  2\n\n|-8 200 7 65535 -3\n1 0\n1 0 1\n1 1\n1 FF\n0 2 s\n1 1\n0 1\n10|3 x\n1 2 3 c 0.500\nFF FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF -FF\n1 2 3 4\n1 2 3\n 50.250 -1.500\nsv z 101\n", got = sink.text();
    check(got == want, [&] { return "print forms N=" + std::to_string(N) + " " + diffAt(got, want); });
    mixText(got);}
template<typename T> concept ReadsInt = requires(FastInput<> &in, T &x) { in.readInt(x); };
template<typename T> concept WritesInt = requires(FastOutput<> &out, T x) { out.writeInt(x); };
template<typename T> concept WritesDouble = requires(FastOutput<> &o, T x) { o.writeDouble(x, 1); };
template<typename T> concept ReadsDouble = requires(FastInput<> &in, T &x) { in.readDouble(x); };
static_assert(!ReadsInt<bool> && !WritesInt<bool> && ReadsInt<char> && ReadsInt<lll> && WritesInt<char32_t> && !ReadsInt<double>);
static_assert(!WritesDouble<long double> && WritesDouble<double> && WritesDouble<float> && WritesDouble<int>);
static_assert(ReadsDouble<float> && ReadsDouble<double> && !ReadsDouble<long double> && !ReadsDouble<int>);
static_assert(!std::is_copy_constructible_v<FastInput<>> && !std::is_move_constructible_v<FastInput<0>> && !std::is_copy_constructible_v<FastOutput<>> && !std::is_move_constructible_v<FastOutput<1>>);
inline void printForms() {
    enter("print-forms");
    printFormsOne<1>(); printFormsOne<64>(); printFormsOne<159>(); printFormsOne<160>(); printFormsOne<320>(); printFormsOne<1 << 16>();
    leave();}
// Tuple-like reads: elements in order through read<B>; at a failure the earlier ones stay assigned and the rest untouched.
template<int N> void tupleReadsOne() {
    string text = " 12 word 2.5 -7 x 0.25 3 ff 1A z 9 8 7 bad 4 5 6 bad2 8 1 2 3 4 tok 123456789012345678901234567890 0.5 -0.125 q 77";
    Source src(text, MEMORY);
    FastInput<N> in(src.f);
    auto at = [&](const char *what) { return string("tuple read N=") + std::to_string(N) + " " + what; };
    pair<int, string> p{0, "old"};
    check(in.read(p) && p.first == 12 && p.second == "word", at("pair<int, string>"));
    std::tuple<double, lng, char> t{};
    check(in.read(t) && std::get<0>(t) == 2.5 && std::get<1>(t) == -7 && std::get<2>(t) == 'x', at("tuple<double, lng, char>"));
    std::tuple<> e;
    check(in.read(e) && in.read(e), at("empty tuple"));
    std::tuple<float, int> ft{};
    check(in.read(ft) && std::get<0>(ft) == 0.25f && std::get<1>(ft) == 3, at("tuple<float, int>"));
    std::tuple<int, ulll> hx{};
    check(in.template read<16>(hx) && std::get<0>(hx) == 255 && std::get<1>(hx) == 26, at("read<16>(tuple<int, ulll>)"));
    array<char, 1> ac{};
    check(in.read(ac) && ac[0] == 'z', at("array<char, 1>"));
    std::tuple<int, int, int> f3{1, 2, 3};
    check(in.read(f3) && f3 == std::tuple<int, int, int>{9, 8, 7}, at("tuple<int, int, int>"));
    f3 = {1, 2, 3};
    check(!in.read(f3) && in.status == St::Invalid && f3 == std::tuple<int, int, int>{1, 2, 3}, at("failure at the first element"));
    int x = 0;
    check(in.read(x) && x == 4 && in.read(x) && x == 5, at("recovery after the first element"));
    f3 = {1, 2, 3};
    check(!in.read(f3) && in.status == St::Invalid && f3 == std::tuple<int, int, int>{6, 2, 3}, at("failure at the second element"));
    check(in.read(x) && x == 8, at("recovery after the second element"));
    std::tuple<vector<int>, pair<int, int>> nested{vector<int>(2), {}};
    check(in.read(nested) && std::get<0>(nested) == vector<int>{1, 2} && std::get<1>(nested) == pair<int, int>{3, 4}, at("nested tuple"));
    std::tuple<string_view, InfInt> ti{};
    // the view element is checked only where the next read cannot reuse its bytes: for N > 0 a view lives until the next read
    check(in.read(ti) && ((N != 0 && N != 1 << 16) || std::get<0>(ti) == "tok") && std::get<1>(ti) == InfInt("123456789012345678901234567890"), at("tuple<string_view, InfInt>"));
    std::tuple<double, float, string> df{};
    check(in.read(df) && std::get<0>(df) == 0.5 && std::get<1>(df) == -0.125f && std::get<2>(df) == "q", at("tuple<double, float, string>"));
    pair<int, int> last{-1, -1};
    check(!in.read(last) && in.status == St::Eof && last == pair<int, int>{77, -1}, at("EOF inside a pair"));
    mix(ulng(in.status));}
// Random int8_t / ulll / lng triples in base 16 against the token oracle.
template<int N> void tupleRandomOne() {
    vector<string> toks;
    for (int i = 0; i < scale(300, 2000, 10000); ++i) {
        int k = rint(0, 9);
        toks.push_back(k == 0 ? "-80" : k == 1 ? "80" : k == 2 ? "g1" : k == 3 ? "-1" : k == 4 ? string(33, 'F') : k == 5 ? "+" : randomDigits(rint(1, 33), 16, false));}
    Stream st = join(toks, true, false, 0);
    Source src(st.s, MEMORY);
    FastInput<N> in(src.f);
    size_t i = 0;
    while (i < toks.size()) {
        std::tuple<int8_t, ulll, lng> t{1, 2, 3}, want = t;
        Got<int8_t> a = refParse<int8_t>(toks[i], 16);
        Got<ulll> b = i + 1 < toks.size() ? refParse<ulll>(toks[i + 1], 16) : Got<ulll>{St::Eof, 0};
        Got<lng> c = i + 2 < toks.size() ? refParse<lng>(toks[i + 2], 16) : Got<lng>{St::Eof, 0};
        St ws = a.st != St::Ok ? a.st : b.st != St::Ok ? b.st : c.st;
        if (a.st == St::Ok) { std::get<0>(want) = a.v; }
        if (a.st == St::Ok && b.st == St::Ok) { std::get<1>(want) = b.v; }
        if (ws == St::Ok) { std::get<2>(want) = c.v; }
        bool ok = in.template read<16>(t);
        check(ok == (ws == St::Ok) && in.status == ws && t == want, [&] { return "random tuple N=" + std::to_string(N) + " tokens " + esc(toks[i]) + " status " + stName(in.status) + " expected " + stName(ws); });
        mix(ulng(in.status));
        i += a.st != St::Ok ? 1 : b.st != St::Ok ? 2 : 3;}}
inline void tuples() {
    enter("tuple-read");
    tupleReadsOne<0>(); tupleReadsOne<1>(); tupleReadsOne<7>(); tupleReadsOne<1 << 16>();
    tupleRandomOne<0>(); tupleRandomOne<7>(); tupleRandomOne<1 << 16>();
    leave();}

// Legacy contract ports (06-Miscellaneous/03-fastio tester): statuses at delimiters and at EOF, recovery, bytes, lines, doubles, variadic forms, faults, lifetime.
template<typename T> void invalidsLegacy() {
    string hi = incDigits(refText(std::numeric_limits<T>::max(), 10), 10);
    string lo = SIGNED_T<T> ? "-" + incDigits(refText(std::numeric_limits<T>::min(), 10).substr(1), 10) : "-1";
    vector<pair<string, St>> cases{{"+", St::Invalid}, {"-", St::Invalid}, {"--1", St::Invalid}, {"1x", St::Invalid}, {"x1", St::Invalid}, {"1.0", St::Invalid}, {"0x10", St::Invalid}, {string("2\0x", 3), St::Invalid},
        {string(1, char(255)), St::Invalid}, {hi, St::Overflow}, {lo, SIGNED_T<T> ? St::Overflow : St::Invalid}, {string(1000, '9'), St::Overflow}, {string(1000, '9') + 'x', St::Invalid}};
    if constexpr (!SIGNED_T<T>) { cases.push_back({"-0", St::Invalid}); }
    for (auto &[s, status] : cases) {
        auto where = [&](const char *what) { return "legacy invalid T=" + tname<T>() + " token=" + esc(s) + " " + what; };
        {
            Source eof(s, REGULAR);
            FastInput<3> in(eof.f);
            T x = T(42);
            check(!in.readInt(x) && in.status == status && x == T(42), where("at EOF"));
            check(!in.readInt(x) && in.status == St::Eof && x == T(42), where("next read reports Eof"));}
        Source file(s + " 17", REGULAR);
        FastInput<3> in(file.f);
        T x = T(42);
        check(!in.readInt(x) && in.status == status && x == T(42), where("before a delimiter"));
        check(in.peek() == ' ', where("delimiter preserved"));
        check(in.readInt(x) && x == T(17), where("whole token recovery"));
        mix(ulng(status));}
    Source file("-0 +0 000", REGULAR);
    FastInput<1> in(file.f);
    T x = T(42);
    bool ok = in.readInt(x);
    check(ok == SIGNED_T<T> && (ok ? x == T(0) : x == T(42)), "legacy signed versus unsigned -0 T=" + tname<T>());
    check(in.readInt(x) && x == T(0) && in.readInt(x) && x == T(0), "legacy +0 and 000 T=" + tname<T>());}
// readDouble statuses before a delimiter (then whole-token recovery) and at EOF (then Eof), for N = 3 and 1 as the old tester and N = 0.
template<int N> void doubleStatuses() {
    vector<pair<string, St>> cases{{"1.5e", St::Invalid}, {"0x10", St::Invalid}, {"+", St::Invalid}, {"+-1", St::Invalid}, {"--1", St::Invalid}, {".", St::Invalid}, {"1,5", St::Invalid},
        {"e5", St::Invalid}, {"1e400", St::Overflow}, {"-1e400", St::Overflow}, {"1e-400", St::Overflow}, {"1e400x", St::Invalid}, {"2.5", St::Ok}};
    for (auto &[t, status] : cases) {
        auto where = [&](const char *what) { return "readDouble N=" + std::to_string(N) + " token=" + esc(t) + " " + what; };
        Source file(t + " 2.5", REGULAR);
        FastInput<N> in(file.f);
        double x = 42;
        check(in.readDouble(x) == (status == St::Ok) && in.status == status && x == (status == St::Ok ? 2.5 : 42), where("before a delimiter"));
        check(in.readDouble(x) && x == 2.5, where("whole token recovery"));
        Source eof(t, REGULAR);
        FastInput<N> e(eof.f);
        x = 7;
        check(e.readDouble(x) == (status == St::Ok) && e.status == status && x == (status == St::Ok ? 2.5 : 7) && !e.readDouble(x) && e.status == St::Eof, where("at EOF"));
        mix(ulng(status));}}
// parseInt sets the status on success too: a failure on a view (Overflow, or Invalid for a sign of an unsigned type), then Ok from a wider type on the same view,
// near the buffer start (the out-of-line path) and 40 bytes in (the inline one), in bases 10 and 16.
template<int N> void parseAfterFailure() {
    for (int lead : {0, 40}) {
        Source src(string(size_t(lead), ' ') + "300 -7 FFF", MEMORY);
        FastInput<N> in(src.f);
        string_view s; uint8_t a = 1; int b = 0;
        string at = " N=" + std::to_string(N) + " lead=" + std::to_string(lead);
        check(in.readToken(s) && !in.parseInt(s, a) && in.status == St::Overflow && a == 1 && in.parseInt(s, b) && b == 300 && in.status == St::Ok, "parseInt Ok after Overflow" + at);
        check(in.readToken(s) && !in.parseInt(s, a) && in.status == St::Invalid && a == 1 && in.parseInt(s, b) && b == -7 && in.status == St::Ok, "parseInt Ok after Invalid" + at);
        check(in.readToken(s) && !in.template parseInt<16>(s, a) && in.status == St::Overflow && in.template parseInt<16>(s, b) && b == 0xFFF && in.status == St::Ok, "parseInt<16> Ok after Overflow" + at);}}
inline void legacyStatuses() {
    enter("legacy-statuses");
    [&]<size_t... I>(std::index_sequence<I...>) { (invalidsLegacy<std::tuple_element_t<I, IntTypes>>(), ...); }(std::make_index_sequence<17>{});
    doubleStatuses<3>(); doubleStatuses<1>(); doubleStatuses<0>();
    parseAfterFailure<1 << 16>(); parseAfterFailure<64>(); parseAfterFailure<1>(); parseAfterFailure<0>();
    leave();}
template<int N> void bytesOne(int n) {
    string s;
    for (int i = 0; i < n; ++i) { s += char(i % 256); }
    string file;
    {
        Sink sink;
        {
            FastOutput<7> out(sink.f);
            check(out.write(s) && out.drain() && out.put('\0'), "large binary write/drain and NUL put");}
        file = sink.text();}
    check(file == s + '\0', "destructor flush and binary output");
    Source src(file, REGULAR);
    FastInput<N> in(src.f);
    bool good = true;
    for (size_t i = 0; i < file.size() && good; ++i) { good = in.peek() == uint8_t(file[i]) && in.get() == uint8_t(file[i]); }
    check(good && in.get() == EOF && in.status == St::Eof, "raw peek/get over all bytes N=" + std::to_string(N));
    Source tokens(" \r\n" + string("a\0", 2) + char(255) + " \tZ", MEMORY);
    FastInput<N> tin(tokens.f);
    string t = "unchanged";
    char c = '!';
    check(tin.readToken(t) && t == string("a\0\xFF", 3) && tin.peek() == ' ', "binary token and preserved whitespace N=" + std::to_string(N));
    check(tin.readChar(c) && c == 'Z' && !tin.readChar(c) && c == 'Z', "readChar skip and EOF");
    check(!tin.readToken(t) && t == string("a\0\xFF", 3), "readToken EOF preserves destination");
    Source empty("", REGULAR);
    FastInput<N> e(empty.f);
    check(!e.skipSpace() && e.status == St::Eof, "empty skipSpace N=" + std::to_string(N));
    Source spaces(" \t\n\r\f\v", MEMORY);
    FastInput<N> w(spaces.f);
    check(!w.skipSpace() && w.status == St::Eof, "whitespace-only EOF N=" + std::to_string(N));
    mix(file.size());}
inline void bytes() {
    enter("bytes");
    int n = scale(10000, 1 << 20, 8 << 20);
    bytesOne<0>(n); bytesOne<1>(scale(10000, 100000, 1000000)); bytesOne<7>(n); bytesOne<1 << 16>(n);
    leave();}
vector<string> splitLines(const string &s) {
    vector<string> res;
    string cur;
    bool open = false;
    for (char c : s) {
        if (c == '\n') { res.push_back(cur); cur.clear(); open = false; }
        else { cur += c; open = true; }}
    if (open) { res.push_back(cur); }
    for (auto &x : res) {
        if (!x.empty() && x.back() == '\r') { x.pop_back(); }}
    return res;}
template<int N> void linesWith(const string &s) {
    Source src(s, s.empty() ? REGULAR : MEMORY);
    FastInput<N> in(src.f);
    vector<string> got;
    string x = "unchanged";
    while (in.readLine(x)) { got.push_back(x); }
    check(got == splitLines(s), [&] { return "readLine N=" + std::to_string(N) + " input=" + esc(s); });
    check(in.status == St::Eof && (got.empty() ? x == "unchanged" : x == got.back()), "readLine EOF preserves destination N=" + std::to_string(N));
    mix(got.size());}
inline void lines() {
    enter("read-line");
    vector<string> fixed{"", "\n", "\n\n", "a", "a\n", "a\r\n", "\r\n", "\r", "a\rb\n", "x\r\r\n", string("\0\n\0", 3), "\xFF\r\n\xFF\r", string("\r\0\xFF\n", 4), "  lead and trail  \n\tz", string(70000, 'q') + "\nend", string(65535, 'w') + "\n" + string(65536, 'v')};
    for (int i = 0; i < scale(100, 3000, 30000); ++i) {
        string t;
        for (int j = int(rnd(40)); j > 0; --j) { t += "ab\r\n \t"[rnd(6)]; }
        fixed.push_back(t);}
    for (const string &t : fixed) { linesWith<0>(t); linesWith<1>(t); linesWith<7>(t); linesWith<65536>(t); }
    Source mixed("12 word\nnext line\r\n  7", MEMORY);
    FastInput<3> in(mixed.f);
    int a = 0;
    string w, l;
    check(in.readInt(a) && in.readToken(w) && in.readLine(l) && l.empty(), "rest of the current line is empty");
    check(in.readLine(l) && l == "next line" && in.readInt(a) && a == 7 && !in.readLine(l) && l == "next line", "line after tokens");
    leave();}
inline void writeDoubles() {
    enter("write-double");
    Sink sink;
    string expected;
    {
        FastOutput<7> o(sink.f);
        vector<double> vals{0.0, -0.0, 0.5, 1.5, 2.5, 3.5, -2.5, 0.125, 0.375, -0.0005, -1e-7, -0.4, 0.5000000000000001, 1e-7, 123456.789, 1e22, 1e23, 9007199254740993.0, DBL_MAX, -DBL_MAX, DBL_MIN, -DBL_MIN, 5e-324, -5e-324,
            INFINITY, -INFINITY, 0.1, 1.0 / 3, 999.9995, double(1.1f), double(FLT_MAX)};
        size_t every = vals.size();   // these at every precision 0-200, the random ones at a sample
        for (int i = 0; i < scale(100, 3000, 30000); ++i) {
            double v = std::bit_cast<double>(rng());
            if (std::isfinite(v)) { vals.push_back(v); }
            if (i % 4 == 0) { vals.push_back(std::ldexp(double(rng() >> 11), rint(-1100, 1000))); }}
        int k = 0;
        vector<int> all(201), some{0, 1, 2, 3, 6, 9, 15, 16, 17, 20, 50, 200};
        std::iota(all.begin(), all.end(), 0);
        for (size_t i = 0; i < vals.size(); ++i) {
            double v = vals[i];
            for (int prec : i < every ? all : some) {
                if (++k % 7 && prec == 200 && i >= every) { continue; }
                char b[1024];
                std::snprintf(b, sizeof b, "%.*f", prec, v);
                expected += string(b) + (k % 2 ? "\n" : "");
                check(o.writeDouble(v, prec, k % 2 ? '\n' : '\0'), "writeDouble");}}
        check(o.flush(), "writeDouble flush");}
    string got = sink.text();
    check(got == expected, [&] { return "writeDouble equals printf %.*f: " + diffAt(got, expected); });
    Sink nan;
    {
        FastOutput<1> o(nan.f);
        check(o.writeDouble(NAN, 2, ' ') && o.writeDouble(-NAN, 0) && o.writeValue(1.0f) && o.writeValue(vector<double>{0.5}) && o.writeDouble(1.1f, 9, ' ') && o.writeDouble(-3, 2) && o.put(' ')
            && o.print(-0.0f, 0.1f), "NaN, float and integer writes");}
    check(nan.text() == "nan -nan1.0000000000000000.5000000000000001.100000024 -3.00 -0.000000000000000 0.100000001490116\n", [&] { return "NaN spelling, float conversion and default precision: " + esc(nan.text()); });
    mixText(got);
    leave();}
inline void variadic() {
    enter("variadic");
    Source file("  -5 word Z 2.25 3\n10 20 30\n", MEMORY);
    FastInput<5> in(file.f);
    int a = 0;
    string w;
    char c = 0;
    double d = 0;
    vector<lng> v(4);
    uint8_t small = 0;
    check(in.read(a, w, c, d, small) && a == -5 && w == "word" && c == 'Z' && d == 2.25 && small == 3, "variadic read");
    check(!in.read(v) && v[0] == 10 && v[2] == 30 && v[3] == 0 && in.status == St::Eof, "range read stops at EOF");
    Sink out;
    {
        FastOutput<3> o(out.f);
        o.precision = 3;
        vector<int> none;
        vector<vector<int>> nested{{1, 2}, {3}};
        check(o.print(1, "two", string("three"), 'c', 2.5, vector<int>{4, 5}, none, nested, lll(-7), int8_t(-8)), "print");
        check(o.print() && o.print(string_view("sv")) && o.write("raw", '!') && o.writeInt(9) && o.put('\n'), "print/write end");
        check(o.writeValue(1.0f) && o.writeValue(vector<double>{0.5}), "writeValue float");}
    string got = out.text(), want = "1 two three c 2.500 4 5  1 2 3 -7 -8\n\nsv\nraw!9\n1.0000.500";
    check(got == want, [&] { return "legacy print text: " + diffAt(got, want); });
    Source hex("ff -7F 1z Zz 101 9 x", MEMORY);
    FastInput<7> h(hex.f);
    int p = 0, q = 0, r = 0, s = 0;
    check(h.read<16>(p, q) && p == 255 && q == -127 && h.read<36>(r, s) && r == 71 && s == 1295 && h.read<2>(p) && p == 5, "read<B>(xs...) bases");
    check(!h.read<8>(p, q) && h.status == St::Invalid && p == 5 && h.read(c) && c == 'x', "read<8> of 9 fails, then read(char) is readChar");
    mixText(got);
    leave();}
struct Cookie { string bytes; size_t pos = 0; int calls = 0, limit = 0; };
inline ssize_t brokenRead(void *p, char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p);
    ++c.calls;
    n = min({n, size_t(3), c.bytes.size() - c.pos});
    if (!n) { errno = EIO; return -1; }
    std::memcpy(s, c.bytes.data() + c.pos, n);
    c.pos += n; return ssize_t(n);}
inline ssize_t brokenWrite(void *p, const char *s, size_t n) {
    auto &c = *static_cast<Cookie *>(p);
    ++c.calls;
    n = min(n, size_t(c.limit));
    c.bytes.append(s, n);
    errno = ENOSPC; return ssize_t(n);}
inline std::FILE *readCookie(Cookie &c) {
    cookie_io_functions_t ops{};
    ops.read = brokenRead;
    std::FILE *f = fopencookie(&c, "r", ops);
    check(f != nullptr, "read cookie");
    return f;}
inline std::FILE *writeCookie(Cookie &c) {
    cookie_io_functions_t ops{};
    ops.write = brokenWrite;
    std::FILE *f = fopencookie(&c, "w", ops);
    check(f != nullptr && std::setvbuf(f, nullptr, _IONBF, 0) == 0, "write cookie");
    return f;}
// A read error before the token's boundary is Error, above Invalid and Overflow; bytes before a delimiter stay readable.
template<int N> void inputFaults() {
    auto where = [&](const string &what) { return "input fault N=" + std::to_string(N) + " " + what; };
    for (const string &text : {string(""), string("123"), string("x123"), string(100, '9'), string("word")}) {
        Cookie c{text};
        std::FILE *f = readCookie(c);
        {
            FastInput<N> in(f);
            lng v = 42;
            check(!in.readInt(v) && v == 42 && in.status == St::Error, [&] { return where("readInt of " + esc(text) + " then a read error: expected Error, got " + stName(in.status) + " v=" + std::to_string(v)); });
            check(in.get() == EOF && in.status == St::Error, where("sticky FILE error after " + esc(text)));}
        std::fclose(f);}
    for (int kind = 0; kind < 4; ++kind) {
        Cookie c{kind == 2 ? "1.25" : kind == 3 ? "123456789" : "word"};
        std::FILE *f = readCookie(c);
        {
            FastInput<N> in(f);
            string s = "old";
            string_view sv = "view";
            double d = 7;
            InfInt x(5);
            bool ok = kind == 0 ? in.readToken(s) : kind == 1 ? in.readToken(sv) : kind == 2 ? in.readDouble(d) : in.readText(x);
            check(!ok && in.status == St::Error && s == "old" && sv == "view" && d == 7 && x == InfInt(5), [&] { return where(array<const char *, 4>{"readToken(string)", "readToken(string_view)", "readDouble", "readText(InfInt)"}[size_t(kind)] + string(" before a read error: got ") + stName(in.status)); });}
        std::fclose(f);}
    {
        Cookie c{"line"};
        std::FILE *f = readCookie(c);
        {
            FastInput<N> in(f);
            string s = "old";
            check(!in.readLine(s) && s == "old" && in.status == St::Error, where("readLine before a read error"));}
        std::fclose(f);}
    {
        Cookie c{"123 4"};
        std::FILE *f = readCookie(c);
        {
            FastInput<N> in(f);
            lng v = 0;
            check(in.readInt(v) && v == 123 && !in.readInt(v) && v == 123 && in.status == St::Error, where("token before a delimiter survives the later error"));}
        std::fclose(f);}}
inline void outputFaults() {
    for (int limit : {0, 2}) {
        Cookie c;
        c.limit = limit;
        std::FILE *f = writeCookie(c);
        {
            FastOutput<7> out(f);
            check(out.write("abcde"), "buffer pending");
            check(!out.drain() && out.error, "short/failed fwrite");
            int calls = c.calls;
            check(!out.flush() && !out.put('x') && !out.write("") && !out.writeInt(42) && !out.writeText(InfInt(5)) && !out.print(1, 2) && !out.print() && !out.writeValue(true) && !out.writeDouble(1.5, 2), "sticky failed writer");
            check(c.calls == calls, "failed writer does not retry");}
        std::fclose(f);}
    for (int form = 0; form < 3; ++form) {
        Cookie c;
        std::FILE *f = writeCookie(c);
        {
            FastOutput<4096> out(f);
            string fill(form == 2 ? 4090 : 4000, 'f');   // form 2: a short text that does not fit the rest, so it needs the failing drain
            InfInt big(randomDigits(9000, 16, true), 16);
            bool ok = out.write(fill) && (form == 0 ? out.print(1, 2, 3) : form == 1 ? out.writeText<16>(big) : out.writeText<16>(InfInt(1) << 64));
            check(!ok && out.error, "a full buffer that fails to drain: form " + std::to_string(form));
            int calls = c.calls;
            check(!out.print(4) && !out.flush() && c.calls == calls, "sticky after form " + std::to_string(form));}
        std::fclose(f);}
    {
        Cookie c;
        cookie_io_functions_t ops{};
        ops.write = brokenWrite;
        std::FILE *f = fopencookie(&c, "w", ops);
        {
            FastOutput<7> out(f);
            check(out.write("abc") && out.drain() && !out.error, "C buffer defers error");
            check(!out.flush() && out.error, "fflush detects delayed error");}
        std::fclose(f);}
    {
        Cookie c;
        std::FILE *f = writeCookie(c);
        { FastOutput<7> out(f); }
        check(c.calls == 0, "empty lifetime no IO");
        std::fclose(f);}}
inline void faults() {
    enter("faults");
    inputFaults<7>(); inputFaults<1>(); inputFaults<1 << 16>();
    outputFaults();
    leave();
    enter("faults-whole-stream");
    inputFaults<0>();
    leave();}
// Flush on destruction (pending and drained bytes reach the descriptor only through fflush), no I/O at construction, the mapping released.
inline void lifetime() {
    enter("lifetime");
    for (int drained = 0; drained < 2; ++drained) {
        std::FILE *f = std::tmpfile();
        check(f && std::setvbuf(f, nullptr, _IOFBF, 1 << 16) == 0, "fully buffered fixture");
        char b[16];
        {
            FastOutput<64> out(f);
            check(out.write("pending") && (!drained || out.drain()), "buffer bytes");
            check(pread(fileno(f), b, 16, 0) == 0, "nothing reaches the file before the destructor");}
        check(pread(fileno(f), b, 16, 0) == 7 && string(b, 7) == "pending", drained ? "destructor flushes drained bytes" : "destructor flushes pending bytes");
        std::fclose(f);}
    {
        Source src("12 345 6", REGULAR);
        {
            FastInput<64> in(src.f);
            check(std::ftell(src.f) == 0, "construction does no IO for N > 0");
            int x = 0;
            check(in.readInt(x) && x == 12, "first read");}
        std::rewind(src.f);
        FastInput<1> again(src.f);
        int x = 0;
        check(again.readInt(x) && x == 12, "FILE survives reader destruction and rewind");}
    {
        Source src(string(10000, 'x') + " y", REGULAR);
        const uint8_t *at = nullptr;
        size_t len = 0;
        {
            FastInput<0> in(src.f);
            at = in.b0; len = in.mapped;
            string_view sv;
            check(len == 10002 && in.readToken(sv) && sv.size() == 10000 && in.readToken(sv) && sv == "y", "N = 0 maps a regular file");}
        auto page = reinterpret_cast<void *>(reinterpret_cast<std::uintptr_t>(at) & ~std::uintptr_t(sysconf(_SC_PAGESIZE) - 1));
        check(msync(page, len, MS_ASYNC) == -1 && errno == ENOMEM, "the destructor releases the mapping");
        check(std::fgetc(src.f) == 'x', "N = 0 leaves the FILE position (mapped from its offset)");}
    {
        Source pipe("a b", PIPE);
        FastInput<0> in(pipe.f);
        string_view a, b;
        check(in.mapped == 0 && in.readToken(a) && in.readToken(b) && a == "a" && b == "b" && !in.readToken(b) && in.status == St::Eof, "N = 0 over a pipe reads everything");}
    for (Kind kind : {OFFSET, CONSUMED}) {   // the mapping starts at the FILE's offset, whether set by fseek or by consumed bytes
        Source src("ab cd", kind);
        FastInput<0> in(src.f);
        string_view a;
        check(in.mapped == src.skip + 5 && in.readToken(a) && a == "ab" && a.data() == reinterpret_cast<const char *>(in.b0) + src.skip, string("N = 0 maps from the offset: ") + KIND_NAMES[kind]);}
    for (Kind kind : {REGULAR, PIPE}) {
        Source src("", kind);
        FastInput<0> in(src.f);
        char c = '?';
        check(in.mapped == 0 && !in.readChar(c) && c == '?' && in.status == St::Eof && in.get() == EOF && in.peek() == EOF, string("N = 0 over empty input: ") + KIND_NAMES[kind]);}
    {
        Source src("xyz", REGULAR);
        check(fseeko(src.f, 3, SEEK_SET) == 0, "seek to the end");
        FastInput<0> in(src.f);
        check(in.mapped == 0 && in.peek() == EOF && in.status == St::Eof, "N = 0 at the end of a regular file falls back to fread and reads nothing");}
    {
        Sink sink;
        FastOutput<16> out(sink.f);
        check(std::ftell(sink.f) == 0 && out.write("ab") && std::ftell(sink.f) == 0 && out.flush() && out.flush() && sink.text() == "ab", "FastOutput construction and buffering do no IO; repeated flush");}
    mix(1);
    leave();}
// write with and without an end byte for views around N (drained past the buffer from N bytes), a real NUL through put.
template<int N> void writeFormsOne() {
    Sink sink;
    string want;
    {
        FastOutput<N> out(sink.f);
        bool ok = true;
        int k = 0;
        for (int fill : {0, 1, N - 1}) {
            for (size_t len : {size_t(0), size_t(1), size_t(N - 1), size_t(N), size_t(N + 1), size_t(3 * N + 2)}) {
                for (char end : {'\0', '!'}) {
                    string f(size_t(fill), '.'), v(len, char('a' + k++ % 26));
                    ok = ok && out.write(f) && out.write(v, end) && out.put('\0');
                    want += f + v + (end ? string(1, end) : "") + '\0';}}}
        check(ok && out.pos <= N, "write forms N=" + std::to_string(N));}
    string got = sink.text();
    check(got == want, [&] { return "write forms N=" + std::to_string(N) + " " + diffAt(got, want); });
    mixText(got);}
inline void writeForms() {
    enter("write-forms");
    writeFormsOne<1>(); writeFormsOne<2>(); writeFormsOne<7>(); writeFormsOne<64>(); writeFormsOne<1 << 16>();
    leave();}
inline void interactive() {
    enter("interactive");
    int p[2];
    check(pipe(p) == 0, "pipe");
    check(::write(p[1], "-17 42\n", 7) == 7, "pipe data");
    std::FILE *f = fdopen(p[0], "r");
    {
        FastInput<1> in(f);
        int a = 0, b = 0;
        check(in.readInt(a) && a == -17 && in.readInt(b) && b == 42, "interactive N=1 before the writer closes");
        check(::write(p[1], "ff 7\n", 5) == 5, "more pipe data");
        check(in.readInt<16>(a) && a == 255 && in.readInt(b) && b == 7, "interactive N=1 second batch");
        close(p[1]);
        check(!in.readInt(a) && in.status == St::Eof, "pipe EOF");}
    std::fclose(f);
    mix(1);
    leave();}

template<int B, typename F> void baseIf(int base, F &f) {
    if constexpr (built(B)) {
        if (base == B) { f.template operator()<B>(); }}}
template<typename F> void withBase(int base, F &&f) { [&]<int... I>(std::integer_sequence<int, I...>) { (baseIf<I + 2>(base, f), ...); }(std::make_integer_sequence<int, 35>{}); }
template<int N> int protoInt() {
    FastInput<N> in;
    FastOutput<1 << 16> out;
    int code = 0, base = 0;
    for (lng k = 0; in.readInt(code) && in.readInt(base); ++k) {
        bool done = [&]<int... P>(std::integer_sequence<int, P...>) { return (protoIntPart<P + 1, N>(code, base, in, out, k) || ...); }(std::make_integer_sequence<int, PARTS>{});
        if (!done) { cerr << "unsupported record " << code << ' ' << base << '\n'; return 2; }}
    out.write("END mapped=");
    out.writeInt(in.mapped, ' ');
    out.write(stName(in.status), '\n');
    return out.flush() ? 0 : 1;}
template<int N> int protoText() {
    FastInput<N> in;
    FastOutput<4096> out;
    int base = 0;
    while (in.readInt(base)) {
        withBase(base, [&]<int B>() {
            InfInt x(-777);
            if (in.template readText<B>(x)) {
                out.write("O ");
                out.template writeText<B>(x);
                out.put(' ');
                out.template writeText<10>(x);
                out.put('\n');}
            else {
                out.put("OFIVE"[size_t(in.status)]);
                out.write(x == InfInt(-777) ? "\n" : "X\n");}});}
    out.write("END mapped=");
    out.writeInt(in.mapped, ' ');
    out.write(stName(in.status), '\n');
    return out.flush() ? 0 : 1;}
template<int N> int protoLines() {
    FastInput<N> in;
    FastOutput<7> out;
    string l;
    while (in.readLine(l)) { out.writeInt(l.size(), ':'); out.write(l, '\n'); }
    return in.status == St::Eof && out.flush() ? 0 : 1;}
template<int N> int protoDoubles() {
    FastInput<N> in;
    FastOutput<7> out;
    double x = 0;
    while (in.readDouble(x) || in.status != St::Eof) {
        char b[40]; std::snprintf(b, sizeof b, "%a", x);
        out.write(in.status == St::Ok ? b : in.status == St::Invalid ? "I" : "O", '\n');}
    return out.flush() ? 0 : 1;}
inline int protoWriteDoubles() {
    FastInput<0> in;
    FastOutput<7> out;
    ulng bits = 0;
    int prec = 0;
    while (in.readInt(bits) && in.readInt(prec)) { out.writeDouble(std::bit_cast<double>(bits), prec, '\n'); }
    return in.status == St::Eof && out.flush() ? 0 : 1;}
template<int... Ns, typename F> int withN(int n, F &&f) {
    int res = 3;
    ((n == Ns ? void(res = f.template operator()<Ns>()) : void()), ...);
    return res;}

int main(int argc, char **argv) {
    std::signal(SIGPIPE, SIG_IGN);
    for (int c = 0; c < 256; ++c) {
        if (!isWs(c)) { DATA += char(c); }}
    vector<string> args(argv + 1, argv + argc);
    string cmd = args.empty() ? "" : args[0];
    if (cmd == "--isa") {
#ifdef __AVX2__
        cout << "avx2\n";
#else
        cout << "scalar\n";
#endif
        return 0;}
    if (cmd == "--types") {
        [&]<size_t... I>(std::index_sequence<I...>) { ((cout << 8 * sizeof(std::tuple_element_t<I, IntTypes>) << ' ' << SIGNED_T<std::tuple_element_t<I, IntTypes>> << '\n'), ...); }(std::make_index_sequence<17>{});
        return 0;}
#if FASTIO_PART >= 0   // the protocols run on the parts build only
    int n = args.size() > 1 && cmd.starts_with("--proto") ? std::stoi(args[1]) : -1;
    if (cmd == "--proto-int") { return withN<0, 64>(n, []<int N>() { return protoInt<N>(); }); }
    if (cmd == "--proto-text") { return withN<0, 64>(n, []<int N>() { return protoText<N>(); }); }
    if (cmd == "--proto-lines") { return withN<0, 7, 65536>(n, []<int N>() { return protoLines<N>(); }); }
    if (cmd == "--proto-doubles") { return withN<0, 7>(n, []<int N>() { return protoDoubles<N>(); }); }
    if (cmd == "--proto-write-doubles") { return protoWriteDoubles(); }
#endif
    if (cmd == "--invalid" && args.size() > 1) {
        if (args[1] == "null-input") { FastInput<> in(nullptr); }
        else if (args[1] == "null-input-0") { FastInput<0> in(nullptr); }
        else if (args[1] == "null-output") { FastOutput<> out(nullptr); }
        else if (args[1] == "precision-negative") { FastOutput<> out(stdout); out.writeDouble(1, -1); }
        else if (args[1] == "precision-large") { FastOutput<> out(stdout); out.writeDouble(1, 201); }
        else if (args[1] == "precision-field") { FastOutput<> out(stdout); out.precision = 201; out.print(1.5); }
        return 0;}
    for (size_t i = 0; i + 1 < args.size(); ++i) {
        if (args[i] == "--mode") { mode = args[i + 1]; }
        if (args[i] == "--seed") { seed = std::stoull(args[i + 1]); }}
#ifdef FASTIO_QUICK
    if (mode != "quick") { cerr << "a FASTIO_QUICK build (bases 2, 7, 10, 16, 36) runs only --mode quick\n"; return 2; }
#endif
#if FASTIO_PART < 0
    spaces(); kernels(); tokenGrid(); tokenStreams(); printForms(); bytes(); lines(); variadic(); faults(); lifetime(); writeForms();
#else
    oracles(); spaces(); kernels(); tokenGrid(); tokenStreams();
    []<int... P>(std::integer_sequence<int, P...>) { (intPart<P + 1>(), ...); }(std::make_integer_sequence<int, PARTS>{});
    textAllBases(std::make_integer_sequence<int, 35>{});
    textSuite<InfIntMini, 2, 7, 10, 16, 23, 24, 36>(InfIntMini(-777));
    textSuite<Rational<lng>, 2, 7, 10, 16, 23, 24, 36>(Rational<lng>(-7, 3));
    textSuite<Rational<lll>, 2, 7, 10, 16, 23, 24, 36>(Rational<lll>(5, 9));
    textSuite<Rational<InfInt>, 2, 7, 10, 16, 23, 24, 36>(Rational<InfInt>(InfInt(-7), InfInt(11)));
    textWrites(); writeRoom(); printForms(); tuples(); reals<float>(); reals<double>();
    legacyStatuses(); bytes(); lines(); writeDoubles(); writeForms(); variadic(); faults(); lifetime(); interactive();
#endif
    cout << (failures ? "FAIL" : "PASS") << " fastio mode=" << mode << " seed=" << seed << " failures=" << failures << std::endl;
    return failures ? 1 : 0;}
#endif
