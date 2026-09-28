#pragma once
#include "../1-Core/01-template.hpp"

// S: O(n * loglog(n)), U: NA, Q: O(1), M: O(n)
struct SieveOfErath {
    int n; vector<char> is_prime;
    SieveOfErath(int N) : n(N) {
        is_prime.assign(n + 1, true);
        if (n >= 0) { is_prime[0] = false; }
        if (n >= 1) { is_prime[1] = false; }
        for (int i = 4; i <= n; i += 2) { is_prime[i] = false; }
        for (int i = 3; 1LL * i * i <= n; i += 2) {
            if (!is_prime[i]) { continue; }
            for (lng j = 1LL * i * i; j <= n; j += 2 * i) { is_prime[j] = false; }}}
};