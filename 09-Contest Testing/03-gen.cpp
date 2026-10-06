// Alternative generator with a decimal unsigned 64-bit seed; pass to 01-stress.py.
#include <bits/stdc++.h>
using namespace std;
using ulng = uint64_t;

// T: O(s + n), M: O(1); s = seed text length, n = generated array length.
int main(int argc, char **argv) {
    if (argc != 2) { return 2; }
    ulng seed;
    string_view s(argv[1]);
    auto [end, error] = std::from_chars(s.data(), s.data() + s.size(), seed);
    if (error != std::errc() || end != s.data() + s.size()) { return 2; }

    std::mt19937_64 rng(seed);
    int n = 1 + int(rng() % 20);
    cout << n << '\n';
    for (int i = 0; i < n; ++i) {
        cout << int(rng() % 201) - 100 << " \n"[i == n - 1];}}
