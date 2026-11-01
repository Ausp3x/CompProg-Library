// PROBLEM: https://judge.yosupo.jp/problem/multiplication_of_hex_big_integers
// Status: locally compiled and run against the regenerated judge tests (stage 4, library fast I/O, 2026-11-01); the pre-review stage-4 revision (header 77395952c8b8bd11) was accepted online (#414385, cpp GCC 15.2, 2026-10-31).
// Library driver (01-Core/19-fastio.hpp): FastInput<0> over the whole stdin (mapped on Linux), a 128-bit path when both tokens parse as lll (at most 32 digits) and the product fits, else InfInt read in base 16, *=, print<16>.
#include "../../../../01-Core/07-infint.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int t; in.read(t);
    string_view u, v; lll x, y, z; iint a, b;
    while (t--) {
        in.read(u, v);
        if (u.size() <= 32 && v.size() <= 32 && in.parseInt<16>(u, x) && in.parseInt<16>(v, y)) {
            if (!__builtin_mul_overflow(x, y, &z)) { out.print<16>(z); continue; }
            a = x; b = y;}
        else { a.read(u, 16); b.read(v, 16); }
        a *= b; out.print<16>(a);}
    return 0;}
