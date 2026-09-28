// Generated explicitly by 97-Online Testing/03-workspace.py.
// GNU C++20 / GCC 14+, Linux x86-64. No global ISA/FP flags or I/O setup.
// Width aliases are exact; INF32/INF64 are finite sentinels, not overflow guards.
// indexed_set stores unique keys; use (key, id) pairs for repeated values.
// chmin/chmax compare once, assign only on improvement, return whether assigned;
// T supplies ordinary comparison/copy-assignment semantics (NaN never improves).
// Each helper is O(1) comparisons/assignments, plus the costs of T.
// 知彼知己，百战不殆
#include <bits/stdc++.h>
#include <ext/pb_ds/assoc_container.hpp>
#include <ext/pb_ds/tree_policy.hpp>
using namespace std;
using namespace __gnu_pbds;

#define fi    first
#define se    second
#define pb    push_back
using uint = uint32_t;
using lng = int64_t;    using ulng = uint64_t;
using lll = __int128_t; using ulll = __uint128_t;
template<typename T>
using indexed_set = tree<T, null_type, std::less<T>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T>
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T>
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }

// LOCAL-only formatting; debug/trace otherwise discard even ill-formed arguments.
// debugString(const T&) -> string, found by ADL, overrides all built-in formatting.
// Finite ranges preserve iteration order; adaptors expose their backing-container
// order (stack bottom first, priority_queue heap order), without copying elements.
// Strings/chars preserve raw bytes between quotes; char arrays/pointers must be
// NUL-terminated (or null pointers). Recursive structures must be acyclic. See 21-c01-verification.md.
#ifdef LOCAL
namespace Debug {
    template<typename T> inline constexpr bool IS_OPTIONAL = false;
    template<typename T> inline constexpr bool IS_OPTIONAL<std::optional<T>> = true;
    template<typename T> inline constexpr bool IS_VARIANT = false;
    template<typename ...T> inline constexpr bool IS_VARIANT<std::variant<T...>> = true;
    template<typename T> inline constexpr bool IS_BITSET = false;
    template<size_t N> inline constexpr bool IS_BITSET<bitset<N>> = true;

    template<typename T, typename C>
    const C &container(const queue<T, C> &x) {
        struct Accessor : queue<T, C> {
            static const C &get(const queue<T, C> &q) { return q.*&Accessor::c; }};
        return Accessor::get(x);}
    template<typename T, typename C>
    const C &container(const stack<T, C> &x) {
        struct Accessor : stack<T, C> {
            static const C &get(const stack<T, C> &q) { return q.*&Accessor::c; }};
        return Accessor::get(x);}
    template<typename T, typename C, typename Comp>
    const C &container(const priority_queue<T, C, Comp> &x) {
        struct Accessor : priority_queue<T, C, Comp> {
            static const C &get(const priority_queue<T, C, Comp> &q) { return q.*&Accessor::c; }};
        return Accessor::get(x);}

    // Legacy convenience: flat aggregates with <= 8 non-array, non-reference,
    // non-bit-field data members, no base classes or anonymous unions. Other
    // aggregates need debugString/streaming; >8 members get an explicit marker.
    struct Any { template<typename T> operator T() const; };
    template<typename T, size_t N>
    constexpr bool aggSizGeq = []<size_t ...I>(std::index_sequence<I...>) {
        return requires { T{(void(I), Any{})...}; };}(std::make_index_sequence<N>{});
    template<typename T, size_t N>
    constexpr bool aggSizExact = aggSizGeq<T, N> && !aggSizGeq<T, N + 1>;

    template<typename T> void write(string &res, const T &x);
    template<size_t I, typename T>
    decltype(auto) element(const T &x) { using std::get; return get<I>(x); }

    template<typename T>
    void writeRange(string &res, T &&x) {
        res += '{';
        bool first = true;
        for (auto &&y : x) {
            if (!std::exchange(first, false)) { res += ", "; }
            Debug::write(res, y);}
        res += '}';}

    template<typename T>
    void write(string &res, const T &x) {
        if constexpr (requires { { debugString(x) } -> std::convertible_to<string>; }) {
            res += debugString(x);}
        else if constexpr (std::is_same_v<T, bool>) { res += x ? "true" : "false"; }
        else if constexpr (std::is_same_v<T, char>) { res += '\''; res += x; res += '\''; }
        else if constexpr (std::is_same_v<T, lll> || std::is_same_v<T, ulll>) {
            ulll v = ulll(x);
            if constexpr (std::is_same_v<T, lll>) { if (x < 0) { res += '-'; v = -v; } }
            char buf[40]; char *end = buf + sizeof(buf), *p = end;
            do { *--p = char('0' + v % 10); v /= 10; } while (v);
            res.append(p, end);}
        else if constexpr (std::is_arithmetic_v<T>) { res += std::to_string(x); }
        else if constexpr (std::is_same_v<T, std::nullptr_t>) { res += "nullptr"; }
        else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, const char*>) {
            if (x) { Debug::write(res, string_view(x)); } else { res += "nullptr"; }}
        else if constexpr (std::is_convertible_v<const T&, string_view>) {
            res += '"'; res += string_view(x); res += '"';}
        else if constexpr (IS_BITSET<T>) { res += x.to_string(); }
        else if constexpr (IS_OPTIONAL<T>) {
            if (x) { res += "optional("; Debug::write(res, *x); res += ')'; }
            else { res += "nullopt"; }}
        else if constexpr (std::is_same_v<T, std::nullopt_t>) { res += "nullopt"; }
        else if constexpr (IS_VARIANT<T>) {
            if (x.valueless_by_exception()) { res += "variant(valueless)"; }
            else {
                res += "variant[" + std::to_string(x.index()) + "](";
                std::visit([&](const auto &v) { Debug::write(res, v); }, x);
                res += ')'; }}
        else if constexpr (std::is_same_v<T, std::monostate>) { res += "monostate"; }
        else if constexpr (requires { Debug::container(x); }) { Debug::write(res, Debug::container(x)); }
        else if constexpr (std::ranges::range<const T>) { Debug::writeRange(res, x); }
        else if constexpr (std::ranges::range<T> && std::copy_constructible<T>) {
            auto copy = x;
            Debug::writeRange(res, copy);}
        else if constexpr (requires { std::tuple_size<T>::value; }) {
            res += '(';
            [&]<size_t ...I>(std::index_sequence<I...>) {
                ((res += (I ? ", " : ""), Debug::write(res, Debug::element<I>(x))), ...);
            }(std::make_index_sequence<std::tuple_size_v<T>>{});
            res += ')';}
        else if constexpr (requires (ostream &os) { os << x; }) {
            std::ostringstream out;
            out << x;
            res += out.str();}
        else if constexpr (std::is_aggregate_v<T>) {
            if constexpr (aggSizExact<T, 8>) {
                auto &[a, b, c, d, e, f, g, h] = x; Debug::write(res, std::tie(a, b, c, d, e, f, g, h));}
            else if constexpr (aggSizExact<T, 7>) {
                auto &[a, b, c, d, e, f, g] = x; Debug::write(res, std::tie(a, b, c, d, e, f, g));}
            else if constexpr (aggSizExact<T, 6>) {
                auto &[a, b, c, d, e, f] = x; Debug::write(res, std::tie(a, b, c, d, e, f));}
            else if constexpr (aggSizExact<T, 5>) {
                auto &[a, b, c, d, e] = x; Debug::write(res, std::tie(a, b, c, d, e));}
            else if constexpr (aggSizExact<T, 4>) {
                auto &[a, b, c, d] = x; Debug::write(res, std::tie(a, b, c, d));}
            else if constexpr (aggSizExact<T, 3>) {
                auto &[a, b, c] = x; Debug::write(res, std::tie(a, b, c));}
            else if constexpr (aggSizExact<T, 2>) {
                auto &[a, b] = x; Debug::write(res, std::tie(a, b));}
            else if constexpr (aggSizExact<T, 1>) {
                auto &[a] = x; Debug::write(res, std::tie(a));}
            else if constexpr (aggSizExact<T, 0>) { res += "{}"; }
            else { res += "<aggregate: provide debugString>"; }}
        else { static_assert(sizeof(T) == 0, "Debug: provide debugString(const T&) or operator<<"); }}

    template<typename T>
    string to_string(const T &x) { string res; Debug::write(res, x); return res; }

    inline int dep = 0;
    inline string_view indent() {
        static constexpr auto SPACES = []() { array<char, 128> v{}; v.fill(' '); return v; }();
        return string_view(SPACES.data(), size_t(2 * std::clamp(dep, 0, 64)));}

    // Legacy inclusive [l, r], unlike ordinary library ranges. l >= 0, r >= l-1;
    // endpoints beyond the range clip naturally. Lvalue views borrow the source,
    // rvalue views own it when permitted by views::all. Nested indices are paired.
    template<std::ranges::viewable_range R, typename ...Args>
    auto slice(R &&ran, int l, int r, Args ...args) {
        static_assert(sizeof...(args) % 2 == 0, "slice needs (l, r) pairs");
        assert(l >= 0 && r >= l - 1);
        auto v = std::forward<R>(ran) | std::views::drop(ptrdiff_t(l))
                 | std::views::take(ptrdiff_t(r) - l + 1);
        if constexpr (sizeof...(args) == 0) { return v; }
        else { return std::move(v) | std::views::transform([=](auto &&cur) { return slice(std::forward<decltype(cur)>(cur), args...); }); }}

    template<typename ...Args>
    void debugO(const Args &...args) { ((cerr << ' ' << Debug::to_string(args)), ...); cerr << '\n'; }

    struct Tracer {
        Tracer(const Tracer&) = delete;
        Tracer(Tracer&&) = delete;

        Tracer &operator=(const Tracer&) = delete;
        Tracer &operator=(Tracer&&) = delete;

        string v;

        Tracer(string x) : v(std::move(x)) {
            assert(dep >= 0 && dep < std::numeric_limits<int>::max());
            cerr << indent() << ">> " << v << '\n';
            dep++;}
        ~Tracer() {
            dep--;
            cerr << indent() << "<< " << v << '\n'; }};
} // namespace Debug

#define debug(...) (cerr << Debug::indent() << "\033[1;31m[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:\033[0m", Debug::debugO(__VA_ARGS__))
#define TRACE_CNCAT(a, b) a##b
#define TRACE_GUARD(a, b) TRACE_CNCAT(a, b)
#define trace(x) Debug::Tracer TRACE_GUARD(_traceGuard, __COUNTER__)(x)
#else
#define debug(...) void(0)
#define trace(x) void(0)
#endif

void solve() {
}

int main() {
    std::ios::sync_with_stdio(false);
    cin.tie(nullptr);

    // Multi-case: replace solve() below with these two lines.
    // int t; cin >> t;
    // while (t--) { solve(); }
    solve();
    return 0;
}
