#pragma once
#include "../01-Core/01-template.hpp"

// T: NA, M: O(1); found=false means the budget ran out (entry=start, tail=length=0), not that no cycle exists.
template<class T>
struct CycleResult {
    bool found;
    T entry;
    ulng tail, length, evaluations;
};

// T: O(min(mu + lambda, budget + 1)) calls, M: O(1) states; mu tail, lambda least period, Equal an equivalence kept by next.
template<class T, class F, class Equal = std::equal_to<T>>
CycleResult<T> floydCycle(T start, F &&next, ulng budget, Equal equal = {}) {
    T slow = start, fast = start;
    ulng used = 0, tail = 0, length = 1;
    auto step = [&](T &x) -> bool {
        if (used == budget) { return false; }
        x = std::invoke(next, std::as_const(x)); ++used; return true;};
    auto failed = [&]() -> CycleResult<T> { return {false, start, 0, 0, used}; };
    do { if (!step(slow) || !step(fast) || !step(fast)) { return failed(); } }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast)));
    slow = start;
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(slow) || !step(fast)) { return failed(); }
        ++tail;}
    if (!step(fast)) { return failed(); }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(fast)) { return failed(); }
        ++length;}
    return {true, std::move(slow), tail, length, used};}

// T: O(min(mu + lambda, budget + 1)) calls, M: O(1) states; mu tail, lambda least period, contract as floydCycle.
template<class T, class F, class Equal = std::equal_to<T>>
CycleResult<T> brentCycle(T start, F &&next, ulng budget, Equal equal = {}) {
    T slow = start, fast = start;
    ulng used = 0, power = 1, length = 1, tail = 0;
    auto step = [&](T &x) -> bool {
        if (used == budget) { return false; }
        x = std::invoke(next, std::as_const(x)); ++used; return true;};
    auto failed = [&]() -> CycleResult<T> { return {false, start, 0, 0, used}; };
    if (!step(fast)) { return failed(); }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (power == length) {
            slow = fast; length = 0;
            power = power > ULLONG_MAX / 2 ? ULLONG_MAX : 2 * power;}
        if (!step(fast)) { return failed(); }
        ++length;}
    slow = fast = start;
    for (ulng i = 0; i < length; ++i) { if (!step(fast)) { return failed(); } }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(slow) || !step(fast)) { return failed(); }
        ++tail;}
    return {true, std::move(slow), tail, length, used};}

// T: O(min(k, mu + lambda)) calls, M: O(1) states; mu tail, lambda least period; f^k(start) for any ulng k.
template<class T, class F, class Equal = std::equal_to<T>>
T orbitTerm(T start, F &&next, ulng k, Equal equal = {}) {
    auto res = brentCycle(start, next, k, equal);
    if (res.found && k > res.tail) { k = res.tail + (k - res.tail) % res.length; }
    for (; k; --k) { start = std::invoke(next, std::as_const(start)); }
    return start;}
