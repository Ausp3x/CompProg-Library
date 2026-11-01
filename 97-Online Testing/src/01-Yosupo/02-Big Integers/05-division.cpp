// PROBLEM: https://judge.yosupo.jp/problem/division_of_big_integers
// Status: locally compiled and run against the regenerated judge tests (stage 4, library fast I/O, 2026-11-01); the pre-review stage-4 revision (header 77395952c8b8bd11) was accepted online (#414372, cpp GCC 15.2, 2026-10-31).
// Library driver (01-Core/19-fastio.hpp): FastInput<0> over the whole stdin (mapped on Linux), a 64-bit path when both tokens parse as ulng (at most 19 digits; A >= 0 and B >= 1), else InfInt read, divMod, print.
#include "../../../../01-Core/07-infint.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int t; in.read(t);
    string_view u, v; ulng x, y; iint a, b;
    while (t--) {
        in.read(u, v);
        if (u.size() <= 19 && v.size() <= 19 && in.parseInt(u, x) && in.parseInt(v, y)) { out.print(x / y, x % y); continue; }
        a.read(u); b.read(v);
        auto [q, r] = divMod(a, b); out.print(q, r);}
    return 0;}
