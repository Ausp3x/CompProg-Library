// Hardware counters and the row protocol of 06-compare.py (Docs/00-compare.md): one line `type|row|cycles|instructions|misses|checksum` per row.
#pragma once
#include <linux/perf_event.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

// User-space cycles, instructions and mispredicted branches of this thread, read as one group.
struct Perf {
    int fd[3];

    static int open(uint64_t config, int group) {
        perf_event_attr a{}; a.type = PERF_TYPE_HARDWARE; a.size = sizeof a; a.config = config;
        a.exclude_kernel = 1; a.exclude_hv = 1; a.disabled = group == -1; a.read_format = PERF_FORMAT_GROUP;
        int f = int(syscall(SYS_perf_event_open, &a, 0, -1, group, 0));
        if (f < 0) { std::perror("perf_event_open (kernel.perf_event_paranoid must be at most 2)"); std::exit(2); }
        return f;}
    Perf() {
        fd[0] = open(PERF_COUNT_HW_CPU_CYCLES, -1); fd[1] = open(PERF_COUNT_HW_INSTRUCTIONS, fd[0]);
        fd[2] = open(PERF_COUNT_HW_BRANCH_MISSES, fd[0]);}
    void start() { ioctl(fd[0], PERF_EVENT_IOC_RESET, PERF_IOC_FLAG_GROUP); ioctl(fd[0], PERF_EVENT_IOC_ENABLE, PERF_IOC_FLAG_GROUP); }
    std::array<uint64_t, 3> stop() {
        ioctl(fd[0], PERF_EVENT_IOC_DISABLE, PERF_IOC_FLAG_GROUP);
        uint64_t b[4];
        if (read(fd[0], b, sizeof b) != sizeof b) { std::perror("perf read"); std::exit(2); }
        return {b[1], b[2], b[3]};}
};

// Rows of one rig run. `filter` holds comma-separated alternatives; a row runs when "type name" contains one of them.
struct Rows {
    Perf perf;
    std::mt19937_64 rng;
    std::string filter;
    int warm = 3, reps = 15;

    explicit Rows(int argc, char **argv) : filter(argc > 1 ? argv[1] : "") {}
    bool wanted(const std::string &full) const {
        if (filter.empty()) { return true; }
        for (size_t i = 0; i <= filter.size();) {
            size_t c = std::min(filter.find(',', i), filter.size());
            if (full.find(filter.substr(i, c - i)) != std::string::npos) { return true; }
            i = c + 1;}
        return false;}
    // gen() prepares the input (untimed), f() is the timed kernel, chk() digests its output; `per` is the units of work in one f().
    template<class G, class F, class H> void run(const std::string &type, const std::string &name, double per, G gen, F f, H chk) {
        net(type, name, per, gen, f, nullptr, chk);}
    // As run, less the counters of base(), timed before f() in every repetition: the cost of f beyond a prefix both share,
    // such as touching 64 KB so that a lookup table has left the first-level cache.
    template<class G, class F, class B, class H> void net(const std::string &type, const std::string &name, double per, G gen, F f, B base, H chk) {
        if (!wanted(type + " " + name)) { return; }
        rng.seed(std::hash<std::string>{}(name) ^ 0x9e3779b97f4a7c15ULL);
        std::array<uint64_t, 3> best{~uint64_t(0), 0, 0}, low{}; uint64_t sum = 0;
        for (int r = 0; r < warm + reps; ++r) {
            gen();
            if constexpr (!std::is_null_pointer_v<B>) {
                perf.start(); base(); auto u = perf.stop();
                if (r >= warm && (r == warm || u[0] < low[0])) { low = u; }}
            perf.start(); f(); auto t = perf.stop();
            sum = sum * 1000003 + uint64_t(chk());
            if (r >= warm && t[0] < best[0]) { best = t; }}
        auto over = [&](int k, double least) { return std::max(least, (double(best[k]) - double(low[k])) / per); };
        std::printf("%s|%s|%.3f|%.3f|%.4f|%llu\n", type.c_str(), name.c_str(), over(0, 0.001), over(1, 0.001), over(2, 0.0),
                    static_cast<unsigned long long>(sum));
        std::fflush(stdout);}
};
