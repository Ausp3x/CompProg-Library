#pragma once
#include "../1-Core/01-template.hpp"

// S: O(1), U: NA, Q: O(1), M: O(1)
struct Montgomery {
    ulng mod, inv, rsq;
    Montgomery(ulng n = 1) : mod(n), inv(n) {
        assert(n & 1);
        for (int i = 0; i < 5; i++) { inv *= 2 - mod * inv; }
        inv = -inv; rsq = -ulll(mod) % mod;}

    ulng red(ulll x) const {
        ulng q = ulng(x) * inv, res = (x + ulll(q) * mod) >> 64;
        return res - (res >= mod) * mod;}
    ulng init(ulng x) const { return red(ulll(x) * rsq); }
    ulng mul(ulng a, ulng b) const { return red(ulll(a) * b); }
    // Q: O(log(n))
    ulng pow(ulng a, ulng b) const {
        a = init(a); ulng res = init(1);
        while (b > 0) {
            if (b & 1) { res = mul(res, a); }
            b >>= 1;
            if (b > 0) { a = mul(a, a); }}
        return red(res);}
};