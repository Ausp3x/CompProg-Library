#pragma once
#include "../1-Core/01-template.hpp"

// S: O(1), U: NA, Q: O(1), M: O(1)
struct Random {
    std::mt19937_64 rng; 
    Random() : rng(std::chrono::steady_clock::now().time_since_epoch().count()) {}

    int randInt(int l, int r) { 
        assert(l <= r); 
        return std::uniform_int_distribution<int>(l, r)(rng);}
    lng randLng(lng l, lng r) { 
        assert(l <= r); 
        return std::uniform_int_distribution<lng>(l, r)(rng);}
};