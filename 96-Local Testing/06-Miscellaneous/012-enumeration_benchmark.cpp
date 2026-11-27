// Revolving-door dispatch: the public forEachGrayCombination against copies of its two branches, forced on every row.
#include "../../06-Miscellaneous/012-enumeration.hpp"

// Copy of the Algorithm R branch, run on every (n, k).
template<class F>
bool algorithmR(int n, int k, F &&visit) {
    vector<int> a(k); iota(a.begin(), a.end(), 0);
    auto above = [&](int j) { return j < k ? a[j] : n; };
    int removed = -1, added = -1;
    while (true) {
        if (!visit(std::as_const(a), removed, added)) { return false; }
        if (!k) { return true; }
        if (k & 1 ? a[0] + 1 < above(1) : a[0] > 0) {
            removed = a[0]; added = a[0] += k & 1 ? 1 : -1;
            continue;}
        if (k == 1) { return true; }
        for (int j = 2; ; ++j) {
            if ((j ^ k) & 1) {
                if (a[j - 1] >= j) {
                    removed = a[j - 1]; a[j - 1] = a[j - 2]; added = a[j - 2] = j - 2;
                    break;}}
            else if (a[j - 1] + 1 < above(j)) {
                removed = a[j - 2]; a[j - 2] = a[j - 1]; added = ++a[j - 1];
                break;}
            if (j == k) { return true; }}}}

// Copy of the loopless Bitner-Ehrlich-Reingold branch, run on every (n, k) with k >= 1.
template<class F>
bool loopless(int n, int k, F &&visit) {
    vector<int> a(k); iota(a.begin(), a.end(), 0);
    vector<char> g(n);
    vector<int> tau(n);
    for (int j = 0; j < n; ++j) { g[j] = j < k; tau[j] = j + 1; }
    int t = k; tau[0] = k;
    if (!visit(std::as_const(a), -1, -1)) { return false; }
    while (true) {
        int i = tau[0], removed, added;
        if (i >= n) { return true; }
        tau[0] = tau[i]; tau[i] = i + 1;
        if (g[i]) {
            if (t) { g[t - 1] ^= 1; a[t - 1] = added = t - 1; a[t] = i - 1; }
            else { g[i - 1] ^= 1; added = a[0] = i - 1; }
            removed = i; ++t;}
        else {
            if (t != 1) { g[t - 2] ^= 1; a[t - 2] = i - 1; a[t - 1] = i; removed = t - 2; }
            else { g[i - 1] ^= 1; a[0] = i; removed = i - 1; }
            added = i; --t;}
        g[i] ^= 1;
        if (t == i || t == 0) { ++t; }
        else {
            t -= g[i - 1]; tau[i - 1] = tau[0];
            tau[0] = t ? t : i - 1;}
        if (!visit(std::as_const(a), removed, added)) { return false; }}}

int main() {
    const vector<pair<int, int>> rows = {
        {2000, 2}, {200, 3}, {34, 6}, {26, 13}, {25, 15}, {25, 17}, {25, 18}, {25, 19}, {25, 20}, {25, 22},
        {20, 15}, {20, 16}, {30, 20}, {30, 21}, {30, 22}, {30, 23}, {30, 24}, {35, 25}, {35, 26}, {40, 33}, {40, 36}, {50, 44}, {50, 47}, {100, 96}, {100, 98},
        {200, 197}, {200, 199}};
    const char *names[3] = {"public", "R", "loopless"};
    for (auto [n, k] : rows) {
        for (int rep = -1; rep < 7; ++rep) {
            for (int s = 0; s < 3; ++s) {
                // Rotate the order per repetition; rep -1 is the warmup and prints nothing.
                int impl = (s + max(rep, 0)) % 3;
                ulng sum = 0, count = 0;
                auto visit = [&](const vector<int> &a, int removed, int added) {
                    sum = sum * 1000003 + ulng(a[0]) + ulng(a[k - 1]) * 7 + ulng(removed + 1) * 31 + ulng(added + 1) * 131;
                    ++count; return true;};
                auto start = std::chrono::steady_clock::now();
                if (impl == 0) { forEachGrayCombination(n, k, visit); }
                else if (impl == 1) { algorithmR(n, k, visit); }
                else { loopless(n, k, visit); }
                double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - start).count();
                if (rep >= 0) {
                    cout << n << ' ' << k << ' ' << names[impl] << ' ' << rep << ' ' << count << ' ' << sum << ' '
                         << std::fixed << std::setprecision(3) << ns / double(count) << '\n';}}}}}
