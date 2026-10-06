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
template<typename K, typename V>
using indexed_map = tree<K, V, std::less<K>, rb_tree_tag, tree_order_statistics_node_update>;

constexpr int INF32 = 0x3f3f3f3f;
constexpr lng INF64 = 0x3f3f3f3f3f3f3f3f;

template<typename T>
constexpr inline bool chmax(T &a, const T &b) { return a < b ? a = b, 1 : 0; }
template<typename T>
constexpr inline bool chmin(T &a, const T &b) { return a > b ? a = b, 1 : 0; }

#ifdef LOCAL
namespace Debug {
    template<typename T> inline constexpr bool IS_OPTIONAL = false;
    template<typename T> inline constexpr bool IS_OPTIONAL<std::optional<T>> = true;
    template<typename T> inline constexpr bool IS_VARIANT = false;
    template<typename ...T> inline constexpr bool IS_VARIANT<std::variant<T...>> = true;
    template<typename T> inline constexpr bool IS_BITSET = false;
    template<size_t N> inline constexpr bool IS_BITSET<bitset<N>> = true;
    template<typename T> inline constexpr bool IS_BIT_PROXY = std::is_class_v<T> && std::is_convertible_v<const T &, bool> && requires(T r) { r.flip(); };
    template<typename T> inline constexpr bool IS_UNSCOPED_ENUM = false;
    template<typename T> requires std::is_enum_v<T> inline constexpr bool IS_UNSCOPED_ENUM<T> = std::is_convertible_v<T, std::underlying_type_t<T>>;

    template<typename T, typename C>
    const C &container(const queue<T, C> &x) {
        struct Accessor : queue<T, C> {
            static const C &get(const queue<T, C> &q) { return q.*&Accessor::c; }
        };
        return Accessor::get(x);}
    template<typename T, typename C>
    const C &container(const stack<T, C> &x) {
        struct Accessor : stack<T, C> {
            static const C &get(const stack<T, C> &q) { return q.*&Accessor::c; }
        };
        return Accessor::get(x);}
    template<typename T, typename C, typename Comp>
    const C &container(const priority_queue<T, C, Comp> &x) {
        struct Accessor : priority_queue<T, C, Comp> {
            static const C &get(const priority_queue<T, C, Comp> &q) { return q.*&Accessor::c; }
        };
        return Accessor::get(x);}

    struct Any { template<typename T> operator T() const; };
    template<typename T, size_t N>
    constexpr bool AGG_SIZE_GEQ = []<size_t ...I>(std::index_sequence<I...>) {
        return requires { T{(void(I), Any{})...}; };}(std::make_index_sequence<N>{});
    template<typename T, size_t N>
    constexpr bool AGG_SIZE_EXACT = AGG_SIZE_GEQ<T, N> && !AGG_SIZE_GEQ<T, N + 1>;

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
        else if constexpr (std::is_same_v<T, bool> || IS_BIT_PROXY<T>) { res += bool(x) ? "true" : "false"; }
        else if constexpr (std::is_same_v<T, char>) { res += '\''; res += x; res += '\''; }
        else if constexpr (std::is_same_v<T, lll> || std::is_same_v<T, ulll>) {
            ulll v = ulll(x);
            if constexpr (std::is_same_v<T, lll>) { if (x < 0) { res += '-'; v = -v; } }
            char buf[40]; char *end = buf + sizeof(buf), *p = end;
            do { *--p = char('0' + v % 10); v /= 10; } while (v);
            res.append(p, end);}
        else if constexpr (std::is_floating_point_v<T>) {
            char buf[64];
            auto [end, ec] = std::to_chars(buf, buf + sizeof(buf), x);
            assert(ec == std::errc());
            res.append(buf, end);}
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
                res += ')';}}
        else if constexpr (std::is_same_v<T, std::monostate>) { res += "monostate"; }
        else if constexpr (requires { Debug::container(x); }) { Debug::write(res, Debug::container(x)); }
        else if constexpr (std::ranges::range<const T>) { Debug::writeRange(res, x); }
        else if constexpr (std::ranges::range<T> && std::copy_constructible<T>) {
            auto copy = x;
            Debug::writeRange(res, copy);}
        else if constexpr (requires { std::tuple_size<T>::value; }) {
            res += '(';
            [&]<size_t ...I>(std::index_sequence<I...>) {
                ((res += (I ? ", " : ""), Debug::write(res, Debug::element<I>(x))), ...);}(std::make_index_sequence<std::tuple_size_v<T>>{});
            res += ')';}
        else if constexpr (IS_UNSCOPED_ENUM<T>) {
            std::ostringstream out;
            out << x;
            if (out.str() == string(1, char(x))) { Debug::write(res, +std::underlying_type_t<T>(x)); } else { res += out.str(); }}
        else if constexpr (requires (ostream &os) { os << x; }) {
            std::ostringstream out;
            out << x;
            res += out.str();}
        else if constexpr (std::is_enum_v<T>) { Debug::write(res, +std::underlying_type_t<T>(x)); }
        else if constexpr (std::is_aggregate_v<T>) {
            if constexpr (AGG_SIZE_EXACT<T, 8>) {
                auto &[a, b, c, d, e, f, g, h] = x; Debug::write(res, std::tie(a, b, c, d, e, f, g, h));}
            else if constexpr (AGG_SIZE_EXACT<T, 7>) {
                auto &[a, b, c, d, e, f, g] = x; Debug::write(res, std::tie(a, b, c, d, e, f, g));}
            else if constexpr (AGG_SIZE_EXACT<T, 6>) {
                auto &[a, b, c, d, e, f] = x; Debug::write(res, std::tie(a, b, c, d, e, f));}
            else if constexpr (AGG_SIZE_EXACT<T, 5>) {
                auto &[a, b, c, d, e] = x; Debug::write(res, std::tie(a, b, c, d, e));}
            else if constexpr (AGG_SIZE_EXACT<T, 4>) {
                auto &[a, b, c, d] = x; Debug::write(res, std::tie(a, b, c, d));}
            else if constexpr (AGG_SIZE_EXACT<T, 3>) {
                auto &[a, b, c] = x; Debug::write(res, std::tie(a, b, c));}
            else if constexpr (AGG_SIZE_EXACT<T, 2>) {
                auto &[a, b] = x; Debug::write(res, std::tie(a, b));}
            else if constexpr (AGG_SIZE_EXACT<T, 1>) {
                auto &[a] = x; Debug::write(res, std::tie(a));}
            else if constexpr (AGG_SIZE_EXACT<T, 0>) { res += "{}"; }
            else { res += "<aggregate: provide debugString>"; }}
        else { static_assert(sizeof(T) == 0, "Debug: provide debugString(const T&) or operator<<"); }}

    template<typename T>
    string to_string(const T &x) { string res; Debug::write(res, x); return res; }

    inline int dep = 0;
    inline string_view indent() {
        static constexpr auto SPACES = []() { array<char, 128> v{}; v.fill(' '); return v; }();
        return string_view(SPACES.data(), size_t(2 * std::clamp(dep, 0, 64)));}

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
            cerr << indent() << "<< " << v << '\n';}
    };
} // namespace Debug

#define debug(...) (cerr << Debug::indent() << "\033[1;31m[L" << __LINE__ << "] [" << #__VA_ARGS__ << "]:\033[0m", Debug::debugO(__VA_ARGS__))
#define TRACE_CNCAT(a, b) a##b
#define TRACE_GUARD(a, b) TRACE_CNCAT(a, b)
#define trace(x) Debug::Tracer TRACE_GUARD(_traceGuard, __COUNTER__)(x)
#else
#define debug(...) void(0)
#define trace(x) void(0)
#endif

void solve(int t) {
    // trace(to_string(t));

    return;
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    int t = 1;
    cin >> t;
    for (int i = 1; i <= t; i++) {
        solve(i);
    }

    return 0;
}
