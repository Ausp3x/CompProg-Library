// PROBLEM: https://judge.yosupo.jp/problem/bell_number
// Status: accepted online (#417093, cpp, 2026-11-14 UTC, 120 ms; the expanded bundle of commit 5765edd, SHA-256 c2a269a499c1056f); locally run against the regenerated judge tests (2026-11-15).
// Exercises: ogfToEgf, exp and egfToOgf: B_n = n! [x^n] exp(e^x - 1); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int n; in.read(n);
    Poly<mint> s(n + 1, 1); s[0] = 0;
    put(egfToOgf(exp(ogfToEgf(s), n + 1).resize(n + 1)).v);
    return 0;}
