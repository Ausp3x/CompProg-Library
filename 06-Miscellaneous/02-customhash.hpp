#pragma once
#include "../01-Core/01-template.hpp"

// S: O(1), Q: O(1) scalar, O(L) string, O(k) composite with k recursively visited elements, M: O(1); stack O(nesting depth).
// Seeded SplitMix64, not cryptographic; pointers hash addresses, strings hash bytes; keep the seed fixed while keys exist.
struct CustomHash {
    using ulng = ::ulng;
    static inline const ulng rnd = [] {
        static const char anchor = 0;
        return ulng(std::chrono::steady_clock::now().time_since_epoch().count()) ^ ulng(reinterpret_cast<std::uintptr_t>(&anchor));}();
    ulng seed;
    CustomHash() : seed(rnd) {}
    explicit CustomHash(ulng seed) : seed(seed) {}
    static constexpr ulng splitMix64(ulng x) {
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);}
    static constexpr ulng combine(ulng h, ulng x) { return splitMix64(h ^ x); }

    template<typename T> requires std::is_pointer_v<T>
    size_t operator()(const T &x) const {
        return splitMix64(ulng(reinterpret_cast<std::uintptr_t>(x)) + seed);}
    size_t operator()(std::nullptr_t) const { return (*this)(static_cast<void *>(nullptr)); }
    size_t operator()(ulll x) const { return combine(splitMix64(ulng(x) + seed), ulng(x >> 64)); }
    size_t operator()(lll x) const { return (*this)(ulll(x)); }
    template<typename T> requires (std::is_convertible_v<T, ulng> || std::is_enum_v<T>)
    size_t operator()(const T &x) const { return splitMix64(ulng(x) + seed); }
    size_t operator()(std::string_view x) const {
        ulng h = seed;
        size_t i = 0;
        for (; x.size() - i >= 8; i += 8) {
            ulng word;
            std::memcpy(&word, x.data() + i, 8);
            h = combine(h, word);}
        ulng tail = 0;
        if (i < x.size()) { std::memcpy(&tail, x.data() + i, x.size() - i); }
        return combine(combine(h, tail), x.size());}
    template<typename T1, typename T2>
    size_t operator()(const pair<T1, T2> &x) const {
        return combine(combine(seed, (*this)(x.first)), (*this)(x.second));}
    template<typename ...Ts>
    size_t operator()(const tuple<Ts...> &x) const {
        ulng h = seed;
        std::apply([&](const auto &...args) { ((h = combine(h, (*this)(args))), ...); }, x);
        return combine(h, sizeof...(Ts));}
    template<typename T>
    requires (std::ranges::input_range<const T> && !std::is_convertible_v<T, std::string_view>)
    size_t operator()(const T &x) const {
        ulng h = seed, n = 0;
        for (const auto &y : x) { h = combine(h, (*this)(y)); ++n; }
        return combine(h, n);}
};
template<typename K, typename V>
using safe_unordered_map = unordered_map<K, V, CustomHash>;
template<typename T>
using safe_unordered_set = unordered_set<T, CustomHash>;
template<typename K, typename V>
using safe_gp_hash_table = __gnu_pbds::gp_hash_table<K, V, CustomHash>;
