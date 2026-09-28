#include "../1-Core/02-debug.hpp"

constexpr const char *GREEN = "\033[1;32m";
constexpr const char *RED = "\033[1;31m";
constexpr const char *RESET = "\033[0m";

int sideEffects = 0;

int bump() {
    sideEffects++;
    return sideEffects;
}

void fail(const string &expr, int line) {
    clog << RED << "FAIL: 1-Core_02-debug_no-local_tester.cpp:" << line << ": " << expr << RESET << '\n';
    std::abort();
}

void require(bool ok, const string &expr, int line) {
    if (!ok) {
        fail(expr, line);
    }
}

void pass() {
    clog << GREEN << "PASS: 1-Core_02-debug_no-local_tester" << RESET << '\n';
}

#define REQUIRE(expr) require((expr), #expr, __LINE__)

int main() {
    std::ostringstream oss;
    auto *old = cerr.rdbuf(oss.rdbuf());

    int x = 0;
    debug();
    debug(++x, bump());
    trace((++x, "scope"));
    REQUIRE(x == 0);
    REQUIRE(sideEffects == 0);

    if (true) {
        debug(++x);
    }
    else {
        x = 100;
    }
    REQUIRE(x == 0);

    for (int i = 0; i < 5; i++) {
        debug(++x, bump());
        trace((++x, "loop"));
    }
    REQUIRE(x == 0);
    REQUIRE(sideEffects == 0);

    int y = (debug(++x, bump()), 42);
    REQUIRE(y == 42);
    REQUIRE(x == 0);
    REQUIRE(sideEffects == 0);

    cerr.rdbuf(old);
    REQUIRE(oss.str().empty());
    pass();
}
