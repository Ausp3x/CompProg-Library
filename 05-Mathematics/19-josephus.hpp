#pragma once
#include "../01-Core/01-template.hpp"

// T: O(min(t + 1, k * log(2 + n / (n - t)))), t = n - 1 for josephus and josephusTime, M: O(1); n >= 1, k >= 1, 0 <= t, p < n; people 0..n-1 in a circle, counting from 0, every k-th counted removed: the person removed at step t, the survivor, the step removing p.
inline lng josephusKth(lng n, lng k, lng t) {
    assert(n >= 1 && k >= 1 && 0 <= t && t < n);
    lng s = n - t, r = lng(ulng(k - 1) % ulng(s));
    while (s < n) {
        lng j = k <= 1 ? n - s : s - 1 - r < k - 1 ? 0 : min(n - s, (s - 1 - r) / (k - 1));
        r += j * k; s += j;
        if (s < n) { r = lng((ulng(r) + ulng(k)) % ulng(s + 1)); ++s; }}
    return r;}
inline lng josephus(lng n, lng k) { return josephusKth(n, k, n - 1); }
inline lng josephusTime(lng n, lng k, lng p) {
    assert(n >= 1 && k >= 1 && 0 <= p && p < n);
    ulll c = ulll(p) + 1, uk = ulng(k), e = c / uk;
    for (; c != e * uk && e < ulng(n); e = c / uk) {
        ulll alive = ulng(n) - e;
        if (alive < uk) { c += ((e + 1) * uk - 1 - c) / alive * alive; }
        c += alive;}
    return lng(e) - 1;}

// T: O(1), M: O(1); n >= 1; the survivor for k = 2.
inline lng josephusK2(lng n) {
    assert(n >= 1);
    return lng(2 * (ulng(n) - std::bit_floor(ulng(n))));}

// T: O(n * log(n)), M: O(n); n >= 0, k >= 1; every person in removal order, by Fenwick descent.
inline vector<int> josephusOrder(int n, lng k) {
    assert(n >= 0 && k >= 1);
    vector<int> fw(n + 1), res;
    for (int i = 1; i <= n; ++i) { fw[i] = i & -i; }
    int top = n ? int(std::bit_floor(uint(n))) : 0;
    ulng cur = 0;
    for (int alive = n; alive > 0; --alive) {
        cur = (cur + ulng(k - 1)) % ulng(alive);
        int pos = 0, rem = int(cur) + 1;
        for (int w = top; w; w >>= 1) {
            if (pos + w <= n && fw[pos + w] < rem) { pos += w; rem -= fw[pos]; }}
        res.pb(pos);
        for (int i = pos + 1; i <= n; i += i & -i) { --fw[i]; }}
    return res;}
