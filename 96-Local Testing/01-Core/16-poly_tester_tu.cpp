// Second translation unit: the header's inline functions, caches and templates must link and share state with the main tester.
#include "../../01-Core/16-poly.hpp"

ulng otherUnitConvolution(int n) {
    Poly<mint> a(n), b(n);
    for (int i = 0; i < n; ++i) { a.v[i] = mint(i + 1); b.v[i] = mint(2 * i + 3); }
    Poly<mint> c = a * b;
    ulng h = 0;
    for (int i = 0; i < c.size(); ++i) { h = h * 1000003 + c.v[i].val(); }
    return h;}
int otherUnitMaxLength() { return Poly<mint>::nttMaxLength(); }
