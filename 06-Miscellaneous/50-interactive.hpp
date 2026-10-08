#pragma once
#include "../01-Core/01-template.hpp"

// T: O(L) per call with L the line length, M: O(1); ask asserts used < budget, every read flushes out first.
// Arguments become space-separated tokens, ranges expanded; a failed read or a signed integer reply equal to error exits(0).
struct Interactive {
    std::istream &in;
    std::ostream &out;
    lng budget, used = 0;
    std::optional<lng> error = -1;

    explicit Interactive(lng budget = LLONG_MAX, std::istream &in = cin, std::ostream &out = cout) : in(in), out(out), budget(budget) {}
    void reset(lng b) { budget = b; used = 0; }

    lng remaining() const { return budget - used; }

    template<class T>
    void put(const T &x, bool &first) {
        if constexpr (std::ranges::range<T> && !std::is_convertible_v<const T &, string_view>) {
            for (const auto &e : x) { put(e, first); }}
        else {
            if (!first) { out << ' '; }
            out << x; first = false;}}
    template<class... A>
    void writeLine(const A &...a) {
        bool first = true;
        (put(a, first), ...);
        out << '\n';}
    void flushNow() { out.flush(); }
    template<class R = lng, class... A>
    R ask(const A &...a) {
        assert(used < budget);
        ++used; writeLine(a...);
        return readReply<R>();}
    template<class... A>
    void answer(const A &...a) {
        writeLine(a...);
        flushNow();}

    template<class T = lng>
    T readReply() {
        if constexpr (std::is_integral_v<T> && sizeof(T) > 1 && std::is_unsigned_v<T>) { assert(!error); }
        flushNow();
        T x{};
        if (!(in >> x)) { judgeErrorExit(); }
        if constexpr (std::is_integral_v<T> && sizeof(T) > 1 && std::is_signed_v<T>) {
            if (error && x == *error) { judgeErrorExit(); }}
        return x;}
    string readLine() {
        flushNow();
        string s;
        if (!std::getline(in >> std::ws, s)) { judgeErrorExit(); }
        if (!s.empty() && s.back() == '\r') { s.pop_back(); }
        return s;}
    [[noreturn]] void judgeErrorExit() {
        flushNow();
        std::exit(0);}
};
