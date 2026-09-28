#pragma once
#include "../../01-Core/01-template.hpp"

inline string mode = "quick", context;
inline ulng seed = 20260928, cases = 0;
inline std::mt19937_64 rng;

template<class T> string show(const vector<T> &a) {
    std::ostringstream out; out << '[';
    for (int i = 0; i < int(a.size()); ++i) { out << (i ? "," : "") << +a[i]; }
    out << ']'; return out.str();
}
template<class T> string show(const T &a) { std::ostringstream out; out << a; return out.str(); }
template<class A, class B> void check(const A &actual, const B &expected, const string &operation) {
    ++cases;
    if (actual != expected) {
        cerr << "FAIL seed=" << seed << " mode=" << mode << " operation=" << operation
             << " smallest-known-reproducer=" << context << " expected=" << show(expected)
             << " actual=" << show(actual) << '\n'; std::exit(1); }
}
inline void configure(int argc, char **argv) {
    for (int i = 1; i + 1 < argc; i += 2) {
        if (string(argv[i]) == "--mode") { mode = argv[i + 1]; }
        if (string(argv[i]) == "--seed") { seed = std::stoull(argv[i + 1]); } }
    rng.seed(seed);
}
inline int limit(int quick, int full, int stress) { return mode == "quick" ? quick : mode == "full" ? full : stress; }
template<class F> void words(int max_n, int sigma, F f) {
    vector<int> s;
    auto go = [&](auto &&self, int left) -> void {
        f(s); if (!left) { return; }
        for (int c = 0; c < sigma; ++c) { s.push_back(c); self(self, left - 1); s.pop_back(); }
    };
    go(go, max_n);
}
inline vector<int> naivePi(const vector<int> &s) {
    int n = int(s.size()); vector<int> pi(n);
    for (int i = 0; i < n; ++i) {
        for (int k = 1; k <= i; ++k) {
            if (std::equal(s.begin(), s.begin() + k, s.begin() + i + 1 - k)) { pi[i] = k; } } }
    return pi;
}
inline vector<int> naiveLcp(const vector<int> &p, const vector<int> &s) {
    vector<int> a(s.size());
    for (int i = 0; i < int(s.size()); ++i) {
        while (a[i] < int(p.size()) && i + a[i] < int(s.size()) && p[a[i]] == s[i + a[i]]) { ++a[i]; } }
    return a;
}
inline vector<int> naiveMatches(const vector<int> &p, const vector<int> &s) {
    vector<int> a;
    for (int i = 0; i + int(p.size()) <= int(s.size()); ++i) {
        if (std::equal(p.begin(), p.end(), s.begin() + i)) { a.push_back(i); } }
    return a;
}
inline vector<int> naiveBorders(const vector<int> &s, bool full = false) {
    vector<int> a; int n = int(s.size());
    for (int k = 1; k < n + int(full); ++k) {
        if (std::equal(s.begin(), s.begin() + k, s.end() - k)) { a.push_back(k); } }
    return a;
}
inline vector<int> naivePeriods(const vector<int> &s) {
    vector<int> a;
    for (int p = 1; p <= int(s.size()); ++p) {
        bool ok = true;
        for (int i = p; i < int(s.size()); ++i) { if (s[i] != s[i - p]) { ok = false; break; } }
        if (ok) { a.push_back(p); } }
    return a;
}
inline vector<lng> naiveCounts(const vector<int> &p, const vector<int> &s) {
    vector<lng> a(p.size() + 1);
    for (int k = 0; k <= int(p.size()); ++k) { a[k] = lng(naiveMatches(vector<int>(p.begin(), p.begin() + k), s).size()); }
    return a;
}
inline int naiveState(const vector<int> &p, const vector<int> &s) {
    for (int k = min(int(p.size()), int(s.size())); k >= 0; --k) {
        if (std::equal(p.begin(), p.begin() + k, s.end() - k)) { return k; } }
    return -1;
}
template<class F> void partitions(int n, F f) {
    vector<int> s;
    auto go = [&](auto &&self, int largest) -> void {
        if (int(s.size()) == n) { f(s); return; }
        for (int c = 0; c <= largest + 1; ++c) { s.push_back(c); self(self, max(largest, c)); s.pop_back(); }
    };
    go(go, -1);
}
