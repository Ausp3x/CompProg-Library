// PROBLEM: https://judge.yosupo.jp/problem/convolution_F_2_64
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: PolyCompanion::convolutionGF2k with k = 64, x^64 = x^4 + x^3 + x + 1 (Karatsuba over PCLMUL carry-less products); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

int main() {
    FastInput<0> in; FastOutput<> out;
    int n, m; in.read(n, m);
    vector<ulng> a(n), b(m); in.read(a, b);
    out.print(PolyCompanion::convolutionGF2k(a, b, 64, 0x1B));
    return 0;}
