#pragma once
#include "../1-Core/01-template.hpp"

// S: NA, U: NA, Q: O(1), M: O(1)
struct CustomHash {
    using ulng = unsigned long long;
    static inline const ulng rnd = std::chrono::steady_clock::now().time_since_epoch().count();
    static ulng splitMix64(ulng x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);}
    
    template<typename T>
    requires (std::is_convertible_v<T, ulng> || std::is_pointer_v<T>)
    size_t operator()(const T &x) const { return splitMix64(ulng(x) + rnd); }
    size_t operator()(std::string_view x) const {
        static const ulng mul = splitMix64(rnd) | 1;
        ulng hash = 5381 + rnd;
        for (char y : x) { hash = hash * mul + y; }
        return splitMix64(hash);}
    template<typename T1, typename T2>
    size_t operator()(const pair<T1, T2> &x) const {
        return ((*this)(x.first) << 3) ^ ((*this)(x.second) >> 1);}
    // Q: O(n)
    template<typename ...Ts>
    size_t operator()(const tuple<Ts...> &x) const {
        size_t seed = 0;
        std::apply(
            [&](const auto &...args) {
                ((seed = ((seed << 3) ^ ((*this)(args) >> 1)) + 0x9e3779b97f4a7c15ULL), ...);}, x);
        return seed;}
    // Q: O(n)
    template<std::ranges::range T>
    requires (!std::is_convertible_v<T, std::string_view>)
    size_t operator()(const T &x) const {
        size_t seed = 0;
        for (const auto &y : x) { seed ^= (*this)(y) + (seed << 6) + (seed >> 2) + 0x9e3779b97f4a7c15ULL; }
        return seed;}
};
template<typename K, typename V> 
using safe_unordered_map = unordered_map<K, V, CustomHash>;
template<typename T> 
using safe_unordered_set = unordered_set<T, CustomHash>;