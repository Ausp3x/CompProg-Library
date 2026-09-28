#pragma once
#include "../1-Core/01-template.hpp"

// T: O(log(n)), M: O(1)
template<typename F>
lng binSearch(lng ok, lng ng, F f) {
    while (abs(ok - ng) > 1) {
        lng md = ok + (ng - ok) / 2;
        if (f(md)) { ok = md; }
        else { ng = md; }}
    return ok;}

// T: O(itr), M: O(1)
template<typename F>
double binSearchReal(double ok, double ng, F f, int itr = 100) {
    for (int i = 0; i < itr; i++) {
        double md = ok + (ng - ok) / 2.0;
        if (f(md)) { ok = md; }
        else { ng = md; }}
    return ok;}

// T: O(log(n)), M: O(1)
template<typename F>
lng ternSearch(lng l, lng r, F f) { // default min, negate f for max
    while (r - l > 2) {
        lng m1 = l + (r - l) / 3;
        lng m2 = r - (r - l) / 3;
        if (f(m1) > f(m2)) { l = m1; }
        else { r = m2; }}
    lng mn = f(l), res = l;
    for (lng i = l + 1; i <= r; i++) {
        lng cur = f(i);
        if (cur < mn) { mn = cur; res = i; }}
    return res;}

// T: O(itr), M: O(1).
template<typename F>
double ternSearchReal(double l, double r, F f, int itr = 200) { // default min, negate f for max
    for (int i = 0; i < itr; i++) {
        double m1 = l + (r - l) / 3.0;
        double m2 = r - (r - l) / 3.0;
        if (f(m1) > f(m2)) { l = m1; }
        else { r = m2; }}
    return l + (r - l) / 2.0;}