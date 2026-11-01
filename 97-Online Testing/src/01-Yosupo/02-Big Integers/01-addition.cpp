// PROBLEM: https://judge.yosupo.jp/problem/addition_of_big_integers
// Status: locally compiled and run against the regenerated judge tests (stage 4, library fast I/O, 2026-11-01); the pre-review stage-4 revision (header 77395952c8b8bd11) was accepted online (#414370, cpp GCC 15.2, 2026-10-31).
// Library driver (01-Core/19-fastio.hpp): FastInput<0> over the whole stdin (mapped on Linux), a 64-bit path when both tokens parse as lng (at most 19 digits) and the sum fits, else InfInt read, +=, print.
#include "../../../../01-Core/07-infint.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int t; in.read(t);
    string_view u, v; lng x, y, z; iint a, b;
    while (t--) {
        in.read(u, v);
        if (u.size() <= 19 && v.size() <= 19 && in.parseInt(u, x) && in.parseInt(v, y)) {
            if (!__builtin_add_overflow(x, y, &z)) { out.print(z); continue; }
            a = x; b = y;}
        else { a.read(u); b.read(v); }
        a += b; out.print(a);}
    return 0;}
