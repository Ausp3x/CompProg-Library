// PROBLEM: https://judge.yosupo.jp/problem/inv_of_polynomials
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: tryInvMod (half-GCD extended Euclid, -1 when gcd(f, g) is not constant; a constant g answered as 0 in the driver); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n, m; in.read(n, m);
    auto f = get(n), g = get(m);
    Poly<mint> h;
    if (g.deg() == 0) { out.print(0); out.put('\n'); }   // modulo a unit every h is 0 (tryInvMod needs deg g >= 1)
    else if (!tryInvMod(f, g, h)) { out.print(-1); }
    else { h.trim(); out.print(h.size()); put(h.v); }
    return 0;}
