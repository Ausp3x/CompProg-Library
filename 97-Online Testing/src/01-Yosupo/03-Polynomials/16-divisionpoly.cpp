// PROBLEM: https://judge.yosupo.jp/problem/division_of_polynomials
// Status: locally compiled and run against the regenerated judge tests (2026-11-15); online acceptance not verified.
// Exercises: divMod (Newton division over a field); FastInput<0>, FastOutput.
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
    auto [q, r] = divMod(f, g);
    q.trim(); r.trim();
    out.print(q.size(), r.size()); put(q.v); put(r.v);
    return 0;}
