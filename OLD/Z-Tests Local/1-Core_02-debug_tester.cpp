#define LOCAL
#include "../1-Core/02-debug.hpp"

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

void fail(const string &label, const string &actual = "", const string &expected = "") {
    clog << RED << "FAIL: " << label << RESET << '\n';
    if (!expected.empty() || !actual.empty()) {
        clog << "expected: [" << expected << "]\n";
        clog << "actual:   [" << actual << "]\n";
    }
    std::abort();
}

void pass() {
    clog << GREEN << "PASS: 1-Core_02-debug_tester" << RESET << '\n';
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
    string zeroChar = Debug::to_string('\0');
    require(zeroChar.size() == 3 && zeroChar[0] == '\'' && zeroChar[1] == '\0' && zeroChar[2] == '\'', "nul char");

    expectEq(Debug::to_string(0), "0", "int through ostream/std overload");
    expectEq(Debug::to_string((lll)0), "0", "lll zero");
    expectEq(Debug::to_string((lll(1) << 100)), "1267650600228229401496703205376", "lll positive large");
    expectEq(Debug::to_string(-(lll(1) << 100)), "-1267650600228229401496703205376", "lll negative large");
    expectEq(Debug::to_string((ulll)0), "0", "ulll zero");
    expectEq(Debug::to_string((ulll(1) << 127)), "170141183460469231731687303715884105728", "ulll high bit");

    expectEq(Debug::to_string(string("")), "\"\"", "empty string");
    expectEq(Debug::to_string(string("abc")), "\"abc\"", "string");
    expectEq(Debug::to_string(std::string_view("sv")), "\"sv\"", "string_view");
    expectEq(Debug::to_string("lit"), "\"lit\"", "const char pointer");
    char mutableStr[] = "mut";
    expectEq(Debug::to_string(mutableStr), "\"mut\"", "char pointer");
    const char *nil = nullptr;
    expectEq(Debug::to_string(nil), "", "null const char pointer");
    char *mutableNil = nullptr;
    expectEq(Debug::to_string(mutableNil), "", "null char pointer");
    string withNul("a\0b", 3);
    string expectedWithNul("\"a\0b\"", 5);
    expectEq(Debug::to_string(withNul), expectedWithNul, "string containing nul");

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
    expectEq(Debug::to_string(list<int>{4, 5}), "{4, 5}", "list");
    expectEq(Debug::to_string(forward_list<int>{4, 5}), "{4, 5}", "forward_list");
    expectEq(Debug::to_string(set<int>{3, 1, 2}), "{1, 2, 3}", "set");
    expectEq(Debug::to_string(map<string, int>{{"a", 1}, {"b", 2}}), "{(\"a\", 1), (\"b\", 2)}", "map");
    expectEq(Debug::to_string(vector<vector<int>>{{1, 2}, {}, {3}}), "{{1, 2}, {}, {3}}", "nested vector");
    int cArray[] = {8, 9};
    expectEq(Debug::to_string(cArray), "{8, 9}", "C array range");
    span<int> sp(cArray);
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
    expectEq(Debug::to_string(Agg9{1, 2, 3, 4, 5, 6, 7, 8, 9}), "{}", "aggregate over supported size");
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
    vector<int> pqValues = parseIntList(Debug::to_string(pq));
    sort(pqValues.begin(), pqValues.end());
    require((pqValues == vector<int>{1, 3, 4, 5}), "priority_queue contents");
    require(pq.size() == 4 && pq.top() == 5, "priority_queue unchanged");

    priority_queue<int, vector<int>, greater<int>> minpq;
    for (int x : vector<int>{5, 1, 4}) {
        minpq.push(x);
    }
    vector<int> minValues = parseIntList(Debug::to_string(minpq));
    sort(minValues.begin(), minValues.end());
    require((minValues == vector<int>{1, 4, 5}), "min priority_queue contents");
    require(minpq.size() == 3 && minpq.top() == 1, "min priority_queue unchanged");
}

void testIndentSliceAndOutput() {
    Debug::dep = 0;
    expectEq(string(Debug::indent()), "", "indent zero");
    Debug::dep = 3;
    expectEq(string(Debug::indent()), string(6, ' '), "indent normal");
    Debug::dep = 100;
    expectEq(string(Debug::indent()), string(128, ' '), "indent clamp");
    Debug::dep = 0;

    vector<int> a{0, 1, 2, 3, 4};
    auto oneDim = Debug::slice(a, 1, 3);
    expectEq(Debug::to_string(oneDim), "{1, 2, 3}", "slice 1D");
    a[2] = 20;
    expectEq(Debug::to_string(oneDim), "{1, 20, 3}", "slice keeps lvalue reference");
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
    string macroOut = captureCerr([&]() {
        debug(x, string("bee"), vector<int>{1, 2});
    });
    require(macroOut.find("\033[1;31m[L") == 0, "debug macro color and line prefix");
    require(macroOut.find("[x, string(\"bee\"), vector<int>{1, 2}]:\033[0m") != string::npos, "debug macro argument names");
    require(macroOut.ends_with(" 7 \"bee\" {1, 2}\n"), "debug macro values");
    string emptyMacroOut = captureCerr([]() {
        debug();
    });
    require(emptyMacroOut.find("[]:\033[0m") != string::npos, "debug macro no args names");
    require(emptyMacroOut.ends_with("\n"), "debug macro no args newline");
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

int main() {
    testScalarsAndStrings();
    testTuplesRangesAndAggregates();
    testContainerAdaptors();
    testIndentSliceAndOutput();
    testTracer();
    testTracerOwnership();
    pass();
}
