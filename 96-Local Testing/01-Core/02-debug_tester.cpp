#define LOCAL
#include "../../01-Core/02-debug.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";

struct Agg0 {};
struct Agg1 { int a; };
struct Agg2 { int a; string b; };
struct Agg3 { int a; bool b; char c; };
struct Agg4 { int a; int b; int c; int d; };
struct Agg5 { int a; int b; int c; int d; int e; };
struct Agg6 { int a; int b; int c; int d; int e; int f; };
struct Agg7 { int a; int b; int c; int d; int e; int f; int g; };
struct Agg8 { int a; lng b; string c; vector<int> d; pair<int, int> e; tuple<int, string, bool> f; array<int, 2> g; bool h; };
struct Agg9 { int a; int b; int c; int d; int e; int f; int g; int h; int i; };
struct Streamable { int x; };

ostream &operator<<(ostream &os, const Streamable &s) {
    return os << "S(" << s.x << ")";
}

namespace Custom {
    struct Hook { int x; };
    string debugString(const Hook &x) { return "Hook(" + std::to_string(x.x) + ")"; }
    ostream &operator<<(ostream &out, const Hook&) { return out << "wrong precedence"; }

    struct ComplexAggregate { int a[2]; int &ref; };
    string debugString(const ComplexAggregate &x) { return Debug::to_string(std::tie(x.a, x.ref)); }

    struct TupleLike { int x; string y; };
    template<size_t I> decltype(auto) get(const TupleLike &x) {
        if constexpr (I == 0) { return (x.x); } else { return (x.y); }}

    struct RangeHook {
        vector<int> data;

        auto begin() const { return data.begin(); }
        auto end() const { return data.end(); }
    };
    string debugString(const RangeHook&) { return "range hook"; }

    struct ThrowMove {
        ThrowMove() = default;
        ThrowMove(ThrowMove&&) { throw 7; }

        ThrowMove &operator=(ThrowMove&&) = default;
    };
    string debugString(const ThrowMove&) { return "throwing alternative"; }

    struct MoveOnly {
        int x;

        explicit MoveOnly(int y) : x(y) {}
        MoveOnly(MoveOnly&&) = default;
        MoveOnly(const MoveOnly&) = delete;
    };
    string debugString(const MoveOnly &x) { return std::to_string(x.x); }
} // namespace Custom

namespace std {
    template<> struct tuple_size<Custom::TupleLike> : std::integral_constant<size_t, 2> {};
    template<size_t I> struct tuple_element<I, Custom::TupleLike> { using type = std::conditional_t<I == 0, int, string>; };
} // namespace std

void fail(const string &label, const string &actual = "", const string &expected = "") {
    std::clog << RED << "FAIL: " << label << RESET << '\n';
    if (!expected.empty() || !actual.empty()) {
        std::clog << "expected: [" << expected << "]\n";
        std::clog << "actual:   [" << actual << "]\n";
    }
    std::abort();
}

void pass() {
    std::clog << GREEN << "PASS: 1-Core_02-debug_tester" << RESET << '\n';
}

void require(bool ok, const string &label) {
    if (!ok) {
        fail(label);
    }
}

void expectEq(const string &actual, const string &expected, const string &label) {
    if (actual != expected) {
        fail(label, actual, expected);
    }
}

string captureCerr(const std::function<void()> &fn) {
    std::ostringstream oss;
    auto *old = cerr.rdbuf(oss.rdbuf());
    fn();
    cerr.rdbuf(old);
    return oss.str();
}

vector<int> parseIntList(const string &s) {
    require(s.size() >= 2 && s.front() == '{' && s.back() == '}', "braced integer list");
    vector<int> res;
    for (int i = 1; i + 1 < int(s.size());) {
        while (i + 1 < int(s.size()) && (s[i] == ' ' || s[i] == ',')) {
            i++;
        }
        if (i + 1 >= int(s.size())) {
            break;
        }
        int sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        }
        require(i + 1 < int(s.size()) && std::isdigit(static_cast<unsigned char>(s[i])), "integer token");
        int x = 0;
        while (i + 1 < int(s.size()) && std::isdigit(static_cast<unsigned char>(s[i]))) {
            x = 10 * x + (s[i] - '0');
            i++;
        }
        res.pb(sign * x);
    }
    return res;
}

void testScalarsAndStrings() {
    expectEq(Debug::to_string(true), "true", "bool true");
    expectEq(Debug::to_string(false), "false", "bool false");
    expectEq(Debug::to_string('x'), "'x'", "char");
    string nul = Debug::to_string('\0');
    require(nul.size() == 3 && nul[0] == '\'' && nul[1] == '\0' && nul[2] == '\'', "nul char");

    expectEq(Debug::to_string(0), "0", "int through ostream/std overload");
    expectEq(Debug::to_string(lll(0)), "0", "lll zero");
    expectEq(Debug::to_string(lll(1) << 100), "1267650600228229401496703205376", "lll positive large");
    expectEq(Debug::to_string(-(lll(1) << 100)), "-1267650600228229401496703205376", "lll negative large");
    expectEq(Debug::to_string(ulll(0)), "0", "ulll zero");
    expectEq(Debug::to_string(ulll(1) << 127), "170141183460469231731687303715884105728", "ulll high bit");

    expectEq(Debug::to_string(string("")), "\"\"", "empty string");
    expectEq(Debug::to_string(string("abc")), "\"abc\"", "string");
    expectEq(Debug::to_string(std::string_view("sv")), "\"sv\"", "string_view");
    expectEq(Debug::to_string("lit"), "\"lit\"", "const char pointer");
    char text[] = "mut";
    expectEq(Debug::to_string(text), "\"mut\"", "char pointer");
    const char *nil = nullptr;
    expectEq(Debug::to_string(nil), "nullptr", "null const char pointer");
    char *mut_nil = nullptr;
    expectEq(Debug::to_string(mut_nil), "nullptr", "null char pointer");
    string s("a\0b", 3);
    string want("\"a\0b\"", 5);
    expectEq(Debug::to_string(s), want, "string containing nul");

    expectEq(Debug::to_string(bitset<0>()), "", "empty bitset");
    expectEq(Debug::to_string(bitset<5>(string("10101"))), "10101", "bitset");
    expectEq(Debug::to_string(Streamable{7}), "S(7)", "ostreamable object");
    expectEq(Debug::to_string(nullptr), "nullptr", "nullptr");
    expectEq(Debug::to_string(1.25), "1.250000", "double");
}

void testTuplesRangesAndAggregates() {
    expectEq(Debug::to_string(pair<int, string>{2, "x"}), "(2, \"x\")", "pair");
    expectEq(Debug::to_string(tuple<>{}), "()", "empty tuple");
    expectEq(Debug::to_string(tuple<int, string, bool>{1, "a", true}), "(1, \"a\", true)", "tuple");
    expectEq(Debug::to_string(array<int, 0>{}), "{}", "empty array as range");
    expectEq(Debug::to_string(array<int, 3>{1, 2, 3}), "{1, 2, 3}", "array as range");

    expectEq(Debug::to_string(vector<int>{}), "{}", "empty vector");
    expectEq(Debug::to_string(vector<int>{1, 2, 3}), "{1, 2, 3}", "vector");
    expectEq(Debug::to_string(vector<string>{"a", ""}), "{\"a\", \"\"}", "vector string");
    expectEq(Debug::to_string(deque<int>{4, 5}), "{4, 5}", "deque");
    expectEq(Debug::to_string(std::list<int>{4, 5}), "{4, 5}", "list");
    expectEq(Debug::to_string(std::forward_list<int>{4, 5}), "{4, 5}", "forward_list");
    expectEq(Debug::to_string(set<int>{3, 1, 2}), "{1, 2, 3}", "set");
    expectEq(Debug::to_string(map<string, int>{{"a", 1}, {"b", 2}}), "{(\"a\", 1), (\"b\", 2)}", "map");
    expectEq(Debug::to_string(vector<vector<int>>{{1, 2}, {}, {3}}), "{{1, 2}, {}, {3}}", "nested vector");
    int a[] = {8, 9};
    expectEq(Debug::to_string(a), "{8, 9}", "C array range");
    std::span<int> sp(a);
    expectEq(Debug::to_string(sp), "{8, 9}", "span");

    static_assert(Debug::aggSizExact<Agg0, 0>);
    static_assert(Debug::aggSizExact<Agg1, 1>);
    static_assert(Debug::aggSizExact<Agg8, 8>);
    static_assert(!Debug::aggSizExact<Agg9, 8>);
    static_assert(Debug::aggSizGeq<Agg9, 9>);
    expectEq(Debug::to_string(Agg0{}), "{}", "empty aggregate");
    expectEq(Debug::to_string(Agg1{4}), "(4)", "aggregate size 1");
    expectEq(Debug::to_string(Agg2{4, "q"}), "(4, \"q\")", "aggregate size 2");
    expectEq(Debug::to_string(Agg3{1, false, 'z'}), "(1, false, 'z')", "aggregate size 3");
    expectEq(Debug::to_string(Agg4{1, 2, 3, 4}), "(1, 2, 3, 4)", "aggregate size 4");
    expectEq(Debug::to_string(Agg5{1, 2, 3, 4, 5}), "(1, 2, 3, 4, 5)", "aggregate size 5");
    expectEq(Debug::to_string(Agg6{1, 2, 3, 4, 5, 6}), "(1, 2, 3, 4, 5, 6)", "aggregate size 6");
    expectEq(Debug::to_string(Agg7{1, 2, 3, 4, 5, 6, 7}), "(1, 2, 3, 4, 5, 6, 7)", "aggregate size 7");
    expectEq(Debug::to_string(Agg8{1, 2, "c", {3, 4}, {5, 6}, {7, "d", true}, {8, 9}, false}), "(1, 2, \"c\", {3, 4}, (5, 6), (7, \"d\", true), {8, 9}, false)", "aggregate size 8");
    expectEq(Debug::to_string(Agg9{1, 2, 3, 4, 5, 6, 7, 8, 9}), "<aggregate: provide debugString>", "aggregate over supported size");
}

void testContainerAdaptors() {
    queue<int> q;
    expectEq(Debug::to_string(q), "{}", "empty queue");
    q.push(1);
    q.push(2);
    expectEq(Debug::to_string(q), "{1, 2}", "queue");
    require(q.size() == 2 && q.front() == 1 && q.back() == 2, "queue unchanged");

    stack<int> st;
    expectEq(Debug::to_string(st), "{}", "empty stack");
    st.push(1);
    st.push(2);
    expectEq(Debug::to_string(st), "{1, 2}", "stack underlying order");
    require(st.size() == 2 && st.top() == 2, "stack unchanged");

    priority_queue<int> pq;
    expectEq(Debug::to_string(pq), "{}", "empty priority_queue");
    for (int x : vector<int>{5, 1, 4, 3}) {
        pq.push(x);
    }
    vector<int> vals = parseIntList(Debug::to_string(pq));
    sort(vals.begin(), vals.end());
    require((vals == vector<int>{1, 3, 4, 5}), "priority_queue contents");
    require(pq.size() == 4 && pq.top() == 5, "priority_queue unchanged");

    priority_queue<int, vector<int>, std::greater<int>> minpq;
    for (int x : vector<int>{5, 1, 4}) {
        minpq.push(x);
    }
    vector<int> min_vals = parseIntList(Debug::to_string(minpq));
    sort(min_vals.begin(), min_vals.end());
    require((min_vals == vector<int>{1, 4, 5}), "min priority_queue contents");
    require(minpq.size() == 3 && minpq.top() == 1, "min priority_queue unchanged");
}

void testIndentSliceAndOutput() {
    Debug::dep = 0;
    expectEq(string(Debug::indent()), "", "indent zero");
    Debug::dep = 3;
    expectEq(string(Debug::indent()), string(6, ' '), "indent normal");
    Debug::dep = 100;
    expectEq(string(Debug::indent()), string(128, ' '), "indent clamp");
    Debug::dep = std::numeric_limits<int>::max();
    expectEq(string(Debug::indent()), string(128, ' '), "indent large without overflow");
    Debug::dep = -1;
    expectEq(string(Debug::indent()), "", "indent negative clamp");
    Debug::dep = 0;

    vector<int> a{0, 1, 2, 3, 4};
    auto part = Debug::slice(a, 1, 3);
    expectEq(Debug::to_string(part), "{1, 2, 3}", "slice 1D");
    a[2] = 20;
    expectEq(Debug::to_string(part), "{1, 20, 3}", "slice keeps lvalue reference");
    expectEq(Debug::to_string(Debug::slice(a, 2, 1)), "{}", "slice empty");
    expectEq(Debug::to_string(Debug::slice(a, 9, 12)), "{}", "slice beyond source");
    expectEq(Debug::to_string(Debug::slice(a, 3, std::numeric_limits<int>::max())), "{3, 4}", "slice wide count");
    expectEq(Debug::to_string(Debug::slice(vector<int>{}, 0, -1)), "{}", "slice empty owned source");
    expectEq(Debug::to_string(Debug::slice(vector<vector<int>>{{1, 2}, {3, 4}}, 0, 1, 0, 0)), "{{1}, {3}}", "slice owns nested rvalue");
    expectEq(Debug::to_string(Debug::slice(a, 0, 0)), "{0}", "slice singleton");
    expectEq(Debug::to_string(Debug::slice(vector<int>{5, 6, 7}, 1, 2)), "{6, 7}", "slice temporary range");

    const vector<int> ca{0, 1, 2, 3};
    expectEq(Debug::to_string(Debug::slice(ca, 1, 2)), "{1, 2}", "slice const range");

    vector<vector<int>> mat{{1, 2, 3}, {4, 5, 6}, {7, 8, 9}};
    expectEq(Debug::to_string(Debug::slice(mat, 0, 1, 1, 2)), "{{2, 3}, {5, 6}}", "slice 2D");

    vector<vector<vector<int>>> cube{{{1, 2}, {3, 4}}, {{5, 6}, {7, 8}}};
    expectEq(Debug::to_string(Debug::slice(cube, 0, 1, 0, 0, 1, 1)), "{{{2}}, {{6}}}", "slice 3D");

    expectEq(captureCerr([]() {
        Debug::debugO(1, string("x"), vector<int>{2, 3});
    }), " 1 \"x\" {2, 3}\n", "debugO output");
    expectEq(captureCerr([]() {
        Debug::debugO();
    }), "\n", "debugO empty output");

    int x = 7;
    string out = captureCerr([&]() {
        debug(x, string("bee"), vector<int>{1, 2});
    });
    require(out.find("\033[1;31m[L") == 0, "debug macro color and line prefix");
    require(out.find("[x, string(\"bee\"), vector<int>{1, 2}]:\033[0m") != string::npos, "debug macro argument names");
    require(out.ends_with(" 7 \"bee\" {1, 2}\n"), "debug macro values");
    string empty_out = captureCerr([]() {
        debug();
    });
    require(empty_out.find("[]:\033[0m") != string::npos, "debug macro no args names");
    require(empty_out.ends_with("\n"), "debug macro no args newline");
}

void testTracer() {
    Debug::dep = 0;
    string direct = captureCerr([]() {
        {
            Debug::Tracer t("scope");
            require(Debug::dep == 1, "tracer increments depth");
            expectEq(string(Debug::indent()), "  ", "tracer indent inside");
        }
        require(Debug::dep == 0, "tracer decrements depth");
    });
    expectEq(direct, ">> scope\n<< scope\n", "direct tracer output");

    string nested = captureCerr([]() {
        Debug::Tracer outer("outer");
        {
            Debug::Tracer inner("inner");
        }
    });
    expectEq(nested, ">> outer\n  >> inner\n  << inner\n<< outer\n", "nested tracer output");

    string macro = captureCerr([]() {
        trace("macro");
    });
    expectEq(macro, ">> macro\n<< macro\n", "trace macro output");
}

void testTracerOwnership() {
    require(!std::is_copy_constructible_v<Debug::Tracer>, "Tracer must not be copy constructible");
    require(!std::is_copy_assignable_v<Debug::Tracer>, "Tracer must not be copy assignable");
    require(!std::is_move_constructible_v<Debug::Tracer>, "Tracer must not be move constructible");
    require(!std::is_move_assignable_v<Debug::Tracer>, "Tracer must not be move assignable");
}

void testOptionalVariantHooksAndWidths() {
    expectEq(Debug::to_string(std::optional<int>{}), "nullopt", "optional absent");
    expectEq(Debug::to_string(std::nullopt), "nullopt", "nullopt tag");
    expectEq(Debug::to_string(std::optional<vector<int>>{{1, 2}}), "optional({1, 2})", "optional vector");
    expectEq(Debug::to_string(std::optional<std::optional<int>>{std::in_place}), "optional(nullopt)", "nested optional absence");
    expectEq(Debug::to_string(std::variant<int, string>{3}), "variant[0](3)", "variant first");
    expectEq(Debug::to_string(std::variant<int, string>{"x"}), "variant[1](\"x\")", "variant string");
    expectEq(Debug::to_string(std::variant<int, int>{std::in_place_index<1>, 5}), "variant[1](5)", "variant repeated type");
    expectEq(Debug::to_string(std::variant<std::monostate, int>{}), "variant[0](monostate)", "variant monostate");
    std::variant<int, Custom::ThrowMove> bad, throwing(std::in_place_index<1>);
    try { bad = std::move(throwing); } catch (int) {}
    require(bad.valueless_by_exception(), "test produces valueless variant");
    expectEq(Debug::to_string(bad), "variant(valueless)", "valueless variant formatting");
    expectEq(Debug::to_string(Custom::Hook{3}), "Hook(3)", "ADL hook before stream/aggregate");
    expectEq(Debug::to_string(Custom::RangeHook{{1, 2}}), "range hook", "ADL hook before range");
    expectEq(Debug::to_string(vector<std::optional<Custom::Hook>>{Custom::Hook{4}, std::nullopt}), "{optional(Hook(4)), nullopt}", "nested ADL hooks");
    expectEq(Debug::to_string(Custom::TupleLike{2, "x"}), "(2, \"x\")", "custom tuple protocol");
    int ref = 4;
    expectEq(Debug::to_string(Custom::ComplexAggregate{{1, 2}, ref}), "({1, 2}, 4)", "hook for aggregates outside automatic domain");
    queue<Custom::MoveOnly> q;
    q.emplace(3); q.emplace(5);
    expectEq(Debug::to_string(q), "{3, 5}", "adaptor move-only elements without copying");
    require(q.size() == 2 && q.front().x == 3, "move-only queue unchanged");
    expectEq(Debug::to_string(std::numeric_limits<lll>::min()), "-170141183460469231731687303715884105728", "signed 128 min");
    expectEq(Debug::to_string(std::numeric_limits<lll>::max()), "170141183460469231731687303715884105727", "signed 128 max");
    expectEq(Debug::to_string(std::numeric_limits<ulll>::max()), "340282366920938463463374607431768211455", "unsigned 128 max");
    for (int i = -128; i <= 255; i++) {
        expectEq(Debug::to_string(lll(i)), std::to_string(i), "bounded exhaustive signed 128");
        if (i >= 0) { expectEq(Debug::to_string(ulll(i)), std::to_string(i), "bounded exhaustive unsigned 128"); }
    }
    expectEq(Debug::to_string(static_cast<signed char>(-128)), "-128", "signed byte numeric");
    expectEq(Debug::to_string(static_cast<unsigned char>(255)), "255", "unsigned byte numeric");
    for (int i = 0; i < 256; i++) {
        expectEq(Debug::to_string(char(i)), string({'\'', char(i), '\''}), "all raw character bytes");
    }
}

void testViewsAndRandomNesting() {
    vector<int> v{1, 2, 3, 4};
    auto odd = v | std::views::filter([](int x) { return x % 2; });
    expectEq(Debug::to_string(odd), "{1, 3}", "non-const iterable view copy");
    auto rows = std::views::iota(1, 4) | std::views::transform([](int x) { return vector<int>{x, x + 1}; });
    expectEq(Debug::to_string(Debug::slice(rows, 0, 2, 1, 1)), "{{2}, {3}, {4}}", "nested slice owns prvalue subranges");
    expectEq(Debug::to_string(std::views::iota(2, 5)), "{2, 3, 4}", "iota view");
    expectEq(Debug::to_string(vector<bool>{true, false, true}), "{true, false, true}", "packed bool range");
    expectEq(Debug::to_string(std::unordered_set<int>{}), "{}", "empty unordered range");
    auto values = parseIntList(Debug::to_string(std::unordered_set<int>{3, 1, 2}));
    sort(values.begin(), values.end());
    require(values == vector<int>({1, 2, 3}), "unordered range values");
    uint seed = std::getenv("CP_TEST_SEED") ? uint(std::stoul(std::getenv("CP_TEST_SEED"))) : 335597;
    int rounds = std::getenv("CP_TEST_ITERATIONS") ? std::stoi(std::getenv("CP_TEST_ITERATIONS")) : 5000;
    std::mt19937 rng(seed);
    for (int rep = 0; rep < rounds; rep++) {
        int n = int(rng() % 8);
        vector<vector<int>> a(n);
        std::ostringstream expected;
        expected << '{';
        for (int i = 0; i < n; i++) {
            if (i) { expected << ", "; }
            expected << '{';
            int m = int(rng() % 8);
            a[i].resize(m);
            for (int j = 0; j < m; j++) {
                a[i][j] = int(rng() % 2001) - 1000;
                if (j) { expected << ", "; }
                expected << a[i][j];
            }
            expected << '}';
        }
        expected << '}';
        expectEq(Debug::to_string(a), expected.str(), "nested range independent stream oracle seed=" + std::to_string(seed) + " case=" + std::to_string(rep));
    }
    int evaluations = 0;
    captureCerr([&]() {
        int answer = true ? (debug(++evaluations), 7) : 0;
        require(answer == 7 && evaluations == 1, "debug expression once");
        bool skipped = false && (debug(++evaluations), true);
        require(!skipped && evaluations == 1, "debug short-circuit context");
        if (true) debug(++evaluations); else evaluations = 99;
        require(evaluations == 2, "debug dangling else context");
    });
}

int main(int argc, char **argv) {
    if (argc > 1) {
        string which = argv[1];
        if (which == "negative-start") { Debug::slice(vector<int>{}, -1, 0); }
        if (which == "inverted-range") { Debug::slice(vector<int>{}, 2, 0); }
        if (which == "tracer-depth") { Debug::dep = std::numeric_limits<int>::max(); Debug::Tracer t("invalid"); }
        return 0;
    }
    testOptionalVariantHooksAndWidths();
    testViewsAndRandomNesting();
    testScalarsAndStrings();
    testTuplesRangesAndAggregates();
    testContainerAdaptors();
    testIndentSliceAndOutput();
    testTracer();
    testTracerOwnership();
    pass();
}
