#include "../1-Core/01-template.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";

void fail(const string &expr, int line) {
    clog << RED << "FAIL: 1-Core_01-template_tester.cpp:" << line << ": " << expr << RESET << '\n';
    std::abort();
}

void require(bool ok, const string &expr, int line) {
    if (!ok) {
        fail(expr, line);
    }
}

void pass() {
    clog << GREEN << "PASS: 1-Core_01-template_tester" << RESET << '\n';
}

#define REQUIRE(expr) require((expr), #expr, __LINE__)

void testAliases() {
    static_assert(std::is_same_v<uint, std::uint32_t>);
    static_assert(std::is_same_v<lng, std::int64_t>);
    static_assert(std::is_same_v<ulng, std::uint64_t>);
    static_assert(std::is_same_v<lll, __int128_t>);
    static_assert(std::is_same_v<ulll, __uint128_t>);
    static_assert(sizeof(lll) == 16);
    static_assert(sizeof(ulll) == 16);

    lll big = (lll(1) << 100) + 123;
    ulll ubig = (ulll(1) << 127) + 456;
    REQUIRE(big > (lll(1) << 100));
    REQUIRE(ubig > (ulll(1) << 127));
}

void testMacrosAndConstants() {
    static_assert(INF32 == 1061109567);
    static_assert(INF64 == 4557430888798830399LL);
    static_assert(2 * INF32 < numeric_limits<int>::max());
    static_assert(2 * INF64 < numeric_limits<lng>::max());

    pair<int, string> p = {3, "a"};
    REQUIRE(p.fi == 3);
    p.se = "b";
    REQUIRE(p.second == "b");

    vector<int> v;
    v.pb(4);
    v.pb(7);
    REQUIRE((v == vector<int>{4, 7}));
}

void testIndexedSet() {
    indexed_set<int> s;
    for (int x : vector<int>{5, 1, 3, 3, -2, 10}) {
        s.insert(x);
    }
    REQUIRE(s.size() == 5);
    REQUIRE(*s.find_by_order(0) == -2);
    REQUIRE(*s.find_by_order(1) == 1);
    REQUIRE(*s.find_by_order(2) == 3);
    REQUIRE(*s.find_by_order(3) == 5);
    REQUIRE(*s.find_by_order(4) == 10);
    REQUIRE(s.find_by_order(5) == s.end());
    REQUIRE(s.order_of_key(-3) == 0);
    REQUIRE(s.order_of_key(-2) == 0);
    REQUIRE(s.order_of_key(0) == 1);
    REQUIRE(s.order_of_key(4) == 3);
    REQUIRE(s.order_of_key(11) == 5);

    s.erase(3);
    REQUIRE(s.size() == 4);
    REQUIRE(*s.find_by_order(2) == 5);
    REQUIRE(s.order_of_key(10) == 3);

    indexed_set<pair<int, int>> ms;
    ms.insert({4, 0});
    ms.insert({4, 1});
    ms.insert({2, 0});
    REQUIRE(ms.order_of_key({4, 0}) == 1);
    REQUIRE(ms.order_of_key({4, 2}) == 3);
    REQUIRE((*ms.find_by_order(1) == pair<int, int>{4, 0}));

    mt19937 rng(0x51cedu);
    indexed_set<int> got;
    set<int> want;
    for (int step = 0; step < 5000; step++) {
        int x = int(rng() % 401) - 200;
        if (rng() & 1) { got.insert(x); want.insert(x); }
        else { got.erase(x); want.erase(x); }
        REQUIRE(got.size() == want.size());
        int k = 0;
        for (int y : want) { REQUIRE(*got.find_by_order(k++) == y); }
        REQUIRE(got.find_by_order(k) == got.end());
        int q = int(rng() % 501) - 250;
        REQUIRE(got.order_of_key(q) == size_t(distance(want.begin(), want.lower_bound(q))));
    }
}

void testChminChmax() {
    static_assert([]() {
        int x = 5;
        bool a = chmax(x, 7);
        bool b = chmax(x, 7);
        bool c = chmin(x, 2);
        bool d = chmin(x, 2);
        return a && !b && c && !d && x == 2;
    }());
    static_assert(std::is_same_v<decltype(chmax(std::declval<int&>(), std::declval<const int&>())), bool>);
    static_assert(std::is_same_v<decltype(chmin(std::declval<int&>(), std::declval<const int&>())), bool>);

    int x = 0;
    REQUIRE(chmax(x, 1) && x == 1);
    REQUIRE(!chmax(x, 1) && x == 1);
    REQUIRE(!chmax(x, -5) && x == 1);
    REQUIRE(chmin(x, -5) && x == -5);
    REQUIRE(!chmin(x, -5) && x == -5);
    REQUIRE(!chmin(x, 100) && x == -5);

    string s = "abc";
    REQUIRE(chmax(s, string("abd")) && s == "abd");
    REQUIRE(!chmax(s, string("abb")) && s == "abd");
    REQUIRE(chmin(s, string("aaa")) && s == "aaa");
}

int main() {
    testAliases();
    testMacrosAndConstants();
    testIndexedSet();
    testChminChmax();
    pass();
}
