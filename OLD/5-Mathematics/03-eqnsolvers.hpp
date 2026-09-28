#pragma once
#include "../1-Core/01-template.hpp"
#include "../5-Mathematics/01-exgcdandmodpow.hpp"

// T: O(log(n)), M: O(1)
bool solveDioEq(lng a, lng b, lng c, lng &x, lng &y, lng &g) { // ax + by = c
    if (a == 0 && b == 0) {
        if (c != 0) { return false; } 
        x = y = g = 0; 
        return true;}
    if (a == 0) {
        if (c % b != 0) { return false; }
        x = 0; y = c / b; g = abs(b); 
        return true;}
    if (b == 0) {
        if (c % a != 0) { return false; }
        x = c / a; y = 0; g = abs(a); 
        return true;}
    g = exGcd(abs(a), abs(b), x, y);
    if (c % g != 0) { return false; }
    if (a < 0) { x = -x; }
    if (b < 0) { y = -y; }
    x = lll(x) * (c / g) % abs(b / g); 
    if (x < 0) { x += abs(b / g); }
    y = (c - lll(a) * x) / b;
    return true;}

// T: O(log(n)), M: O(1)
pair<lng, lng> solveModEq(lng a, lng b, lng m0) { // ax ≡ b (mod m0)
    assert(m0 > 0);
    a = (a % m0 + m0) % m0; b = (b % m0 + m0) % m0;
    lng x, y, g = exGcd(a, m0, x, y);
    if (b % g != 0) { return {-1, -1}; }
    lng m = m0 / g;
    x = lll(x) * (b / g) % m;
    if (x < 0) { x += m; }
    return {x, m};}