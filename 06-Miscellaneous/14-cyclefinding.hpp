#pragma once
#include "../01-Core/01-template.hpp"

// found=false means the call budget was exhausted, NOT that no cycle exists.
// On failure entry=start and tail=length=0; evaluations is the consumed budget.
// On success entry=f^tail(start), length>0 is the least cycle period.
template<class T>
struct CycleResult {
    bool found;
    T entry;
    ulng tail, length, evaluations;
};

namespace cycle_detail {
    // Successor calls, including all recovery phases, share one strict budget.
    template<class T, class F>
    bool advance(T &x, F &next, ulng budget, ulng &used) {
        if (used == budget) { return false; }
        x = std::invoke(next, std::as_const(x)); ++used; return true;}
} // namespace cycle_detail

// T: O(mu + lambda) successor/equality calls on success, O(budget + 1) on
// exhaustion; M: O(1) states. State copies/assignments and callback costs extra.
// T supports copy/move construction and assignment; next(const T&) returns a
// state without mutating its input, and is a total deterministic successor.
// Equal is an equivalence relation
// preserved by next; tail/period refer to its equivalence classes. Exceptions
// propagate. No default constructor, ordering, hashing or operator== is required
// when a custom Equal is supplied. budget may be any ulng, including 0/ULLONG_MAX.
// Floyd: 1x/2x runners meet, then equal-speed recovery finds the entry.
template<class T, class F, class Equal = std::equal_to<T>>
CycleResult<T> floydCycle(T start, F &&next, ulng budget, Equal equal = {}) {
    T slow = start, fast = start;
    ulng used = 0, tail = 0, length = 1;
    auto step = [&](T &x) -> bool { return cycle_detail::advance(x, next, budget, used); };
    auto failed = [&]() -> CycleResult<T> { return {false, start, 0, 0, used}; };
    do {
        if (!step(slow) || !step(fast) || !step(fast)) { return failed(); }
    } while (!std::invoke(equal, std::as_const(slow), std::as_const(fast)));
    slow = start;
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(slow) || !step(fast)) { return failed(); }
        ++tail;}
    if (!step(fast)) { return failed(); }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(fast)) { return failed(); }
        ++length;}
    return {true, std::move(slow), tail, length, used};
}

// T: O(mu + lambda) successor/equality calls on success, O(budget + 1) on
// exhaustion; M: O(1) states. Same state/callback/budget contract as floydCycle.
// Brent: compare against the start of blocks of doubling size to find the period;
// offset two runners by that period to recover the entry. Power saturates safely.
template<class T, class F, class Equal = std::equal_to<T>>
CycleResult<T> brentCycle(T start, F &&next, ulng budget, Equal equal = {}) {
    T slow = start, fast = start;
    ulng used = 0, power = 1, length = 1, tail = 0;
    auto step = [&](T &x) -> bool { return cycle_detail::advance(x, next, budget, used); };
    auto failed = [&]() -> CycleResult<T> { return {false, start, 0, 0, used}; };
    if (!step(fast)) { return failed(); }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (power == length) {
            slow = fast; length = 0;
            power = power > ULLONG_MAX / 2 ? ULLONG_MAX : 2 * power; }
        if (!step(fast)) { return failed(); }
        ++length;}
    slow = fast = start;
    for (ulng i = 0; i < length; ++i) { if (!step(fast)) { return failed(); } }
    while (!std::invoke(equal, std::as_const(slow), std::as_const(fast))) {
        if (!step(slow) || !step(fast)) { return failed(); }
        ++tail;}
    return {true, std::move(slow), tail, length, used};
}
