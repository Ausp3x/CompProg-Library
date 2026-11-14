// PROBLEM: https://judge.yosupo.jp/problem/kth_term_of_linearly_recurrent_sequence
// Status: accepted online (#417080, cpp, 2026-11-14 UTC, 332 ms; the expanded bundle of commit 5765edd, SHA-256 fb5729d82dd08026); locally run against the regenerated judge tests (2026-11-15).
// Exercises: Poly::linearRecurrenceKth (Bostan–Mori through coefOfRationalFps); FastInput<0>, FastOutput.
#include "../../../../01-Core/16-poly.hpp"
#include "../../../../01-Core/19-fastio.hpp"

FastInput<0> in; FastOutput<> out;
// get(n): n residues as a polynomial; put(v): the values space-separated on one line.
template<typename M = mint> Poly<M> get(int n) { Poly<M> a(n); for (auto &x : a.v) { uint t; in.read(t); x = M::raw(t); } return a; }
template<typename M> void put(const vector<M> &v) {
    for (size_t i = 0; i < v.size(); ++i) { out.writeInt(v[i].val(), i + 1 < v.size() ? ' ' : '\n'); }
    if (v.empty()) { out.put('\n'); }}

int main() {
    int d; lng k; in.read(d, k);
    auto a = get(d).v, c = get(d).v;
    out.print(Poly<mint>::linearRecurrenceKth(a, c, k).val());
    return 0;}
