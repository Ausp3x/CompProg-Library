#pragma once
#include "../01-Core/01-template.hpp"

// T: O(1) per call, M: O(1); steady_clock since construction or reset, limits in milliseconds, remaining clamps at 0.
struct Timer {
    using Clock = std::chrono::steady_clock;
    Clock::time_point start = Clock::now();

    void reset() { start = Clock::now(); }

    Clock::duration elapsed() const { return Clock::now() - start; }
    lng elapsedUs() const { return std::chrono::duration_cast<std::chrono::microseconds>(elapsed()).count(); }
    double elapsedMs() const { return std::chrono::duration<double, std::milli>(elapsed()).count(); }
    double elapsedSec() const { return std::chrono::duration<double>(elapsed()).count(); }

    bool expired(double limit) const { return elapsedMs() >= limit; }
    double remaining(double limit) const { return max(0.0, limit - elapsedMs()); }
    double progress(double limit) const {
        assert(limit > 0);
        return min(1.0, elapsedMs() / limit);}
};

// T: O(1), M: O(L); on destruction writes "label: <ms> ms" with three decimals to os.
struct ScopedTimer {
    string label;
    std::ostream &os;
    Timer timer;

    explicit ScopedTimer(string label, std::ostream &os = cerr) : label(std::move(label)), os(os) {}
    ScopedTimer(const ScopedTimer &) = delete;
    ScopedTimer &operator=(const ScopedTimer &) = delete;
    ~ScopedTimer() {
        std::ostringstream line;
        line << label << ": " << std::fixed << std::setprecision(3) << timer.elapsedMs() << " ms\n";
        os << line.str();}
};
