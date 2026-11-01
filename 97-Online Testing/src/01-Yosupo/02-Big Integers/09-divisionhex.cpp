// PROBLEM: https://judge.yosupo.jp/problem/division_of_hex_big_integers
// Status: locally compiled and run against the regenerated judge tests (stage 4, library fast I/O, 2026-11-01); the pre-review stage-4 revision (header 77395952c8b8bd11) was accepted online (#414380, cpp GCC 15.2, 2026-10-31).
// Library driver (01-Core/19-fastio.hpp): FastInput<0> over the whole stdin (mapped on Linux), a 64-bit path when both tokens parse as ulng (at most 16 digits; A >= 0 and B >= 1), else InfInt read in base 16, divMod, print<16>.
#include "../../../../01-Core/07-infint.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int t; in.read(t);
    string_view u, v; ulng x, y; iint a, b;
    while (t--) {
        in.read(u, v);
        if (u.size() <= 16 && v.size() <= 16 && in.parseInt<16>(u, x) && in.parseInt<16>(v, y)) { out.print<16>(x / y, x % y); continue; }
        a.read(u, 16); b.read(v, 16);
        auto [q, r] = divMod(a, b); out.print<16>(q, r);}
    return 0;}
