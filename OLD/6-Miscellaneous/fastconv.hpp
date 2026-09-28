#pragma once
#include "../1-Core/01-template.hpp"

// S: NA, U: NA, Q: O(n * log(n)), M: O(1)
template<typename T>
struct FastConv {
    static void fctOr(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (is_inv) { a[i + len + j] -= a[i + j]; } 
                    else { a[i + len + j] += a[i + j]; }}}}}
    static void fctAnd(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    if (is_inv) { a[i + j] -= a[i + len + j]; } 
                    else { a[i + j] += a[i + len + j]; }}}}}
    static void fctXor(vector<T> &a, bool is_inv) {
        int n = a.size();
        for (int len = 1; len < n; len <<= 1) {
            for (int i = 0; i < n; i += 2 * len) {
                for (int j = 0; j < len; j++) {
                    T u = a[i + j], v = a[i + len + j];
                    a[i + j]       = u + v;
                    a[i + len + j] = u - v;}}}
        if (is_inv) {
            T f_inv_n = T(1) / T(n);
            for (int i = 0; i < n; i++) { a[i] *= f_inv_n; }}}
    static void fctGcdSlow(vector<T> &a, bool is_inv) {
        int n = a.size() - 1;
        if (is_inv) {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) { a[i] -= a[j]; }}} 
        else {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) { a[i] += a[j]; }}}}
    // Q: O(n * loglog(n))
    //*
    static void fctGcd(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        int n = a.size() - 1;
        for (int p : sv.prms) {
            if (p > n) { break; }
            if (is_inv) {
                for (int i = 1; i * p <= n; i++) { a[i] -= a[i * p]; }} 
            else {
                for (int i = n / p; i >= 1; i--) { a[i] += a[i * p]; }}}}
    //*/
    static void fctLcmSlow(vector<T> &a, bool is_inv) {
        int n = a.size() - 1;
        if (is_inv) {
            for (int i = 1; i <= n; i++) {
                for (int j = 2 * i; j <= n; j += i) { a[j] -= a[i]; }}} 
        else {
            for (int i = n; i >= 1; i--) {
                for (int j = 2 * i; j <= n; j += i) { a[j] += a[i]; }}}}
    // Q: O(n * loglog(n))
    //*
    static void fctLcm(vector<T> &a, bool is_inv, const LinearSieve &sv) {
        int n = a.size() - 1;
        for (int p : sv.prms) {
            if (p > n) { break; }
            if (is_inv) {
                for (int i = n / p; i >= 1; i--) { a[i * p] -= a[i]; }} 
            else {
                for (int i = 1; i * p <= n; i++) { a[i * p] += a[i]; }}}}
    //*/
};