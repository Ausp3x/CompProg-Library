// PROBLEM: https://judge.yosupo.jp/problem/stirling_number_of_the_second_kind
// Status: accepted online (#417090, cpp, 2026-11-14 UTC, 89 ms; the expanded bundle of commit 5765edd, SHA-256 91302e22f26e5a57); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Poly::stirling2Row (one convolution); FastInput<0>, FastOutput.
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
    put(Poly<mint>::stirling2Row(n));
    return 0;}
