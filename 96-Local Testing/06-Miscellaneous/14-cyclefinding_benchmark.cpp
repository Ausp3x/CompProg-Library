#include "../../06-Miscellaneous/14-cyclefinding.hpp"

constexpr ulng SEED = 20260928;
string context;

[[gnu::noinline]] ulng heavyDigest(int x) {
    ulng hash = ulng(x) ^ SEED;
    for (int i = 0; i < 16; ++i) {
        hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
        hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
        hash ^= hash >> 31; }
    return hash;
}

template<bool Heavy, bool Cached>
struct Successor {
    const vector<int> &arc;
    const vector<ulng> &digests;
    // Every actual invocation is observable, preventing batch/loop elimination.
    // This instrumentation costs a volatile load/store in both measured methods.
    volatile ulng calls = 0;
    ulng checksum = 0;

    int operator()(const int &x) {
        calls = calls + 1;
        if constexpr (Heavy) {
            ulng digest = Cached ? digests[x] : heavyDigest(x);
            checksum = std::rotl(checksum, 7) + digest; }
        return arc[x];}
};

struct Output { CycleResult<int> result; ulng calls, checksum; };

template<bool Heavy, bool Cached = false>
Output run(const vector<int> &arc, const vector<ulng> &digests, int method, ulng budget) {
    Successor<Heavy, Cached> next{arc, digests};
    auto result = method ? brentCycle(0, next, budget) : floydCycle(0, next, budget);
    return {result, next.calls, next.checksum};
}

void require(bool ok, const string &details) {
    if (!ok) { cerr << "FAIL benchmark seed=" << SEED << ' ' << context << ' ' << details << '\n'; std::exit(1); }
}

// Independent closed forms for this header's complete detection+entry+period
// pipelines on a lollipop orbit. Floyd first meets at the least positive lambda
// multiple >=mu. Brent's first usable anchor is p-1, p=nextPow2(max(mu+1,lambda)).
ulng expectedCalls(int mu, int lambda, int method) {
    ulng tail = ulng(mu), length = ulng(lambda);
    if (!method) {
        ulng meeting = max(length, (tail + length - 1) / length * length);
        return 3 * meeting + 2 * tail + length; }
    ulng power = 1;
    while (power < max(tail + 1, length)) { power *= 2; }
    return power - 1 + 2 * length + 2 * tail;
}

void verify(const Output &out, int mu, int lambda, ulng calls, ulng checksum) {
    require(out.result.found, "expected found=true");
    require(out.result.entry == mu && out.result.tail == ulng(mu) && out.result.length == ulng(lambda),
            "expected entry/tail/length=" + std::to_string(mu) + "/" + std::to_string(mu) + "/" + std::to_string(lambda)
            + " actual=" + std::to_string(out.result.entry) + "/" + std::to_string(out.result.tail) + "/" + std::to_string(out.result.length));
    require(out.calls == calls && out.result.evaluations == calls,
            "expected calls=" + std::to_string(calls) + " actual callback/result=" + std::to_string(out.calls)
            + "/" + std::to_string(out.result.evaluations));
    require(out.checksum == checksum, "callback checksum expected=" + std::to_string(checksum)
            + " actual=" + std::to_string(out.checksum));
}

template<bool Heavy>
void benchmark(int mu, int lambda) {
    int n = mu + lambda, batch = n <= 128 ? 128 : n <= 8193 ? 8 : 1;
    vector<int> arc(n); vector<ulng> digests(Heavy ? n : 0);
    for (int i = 0; i < n; ++i) {
        arc[i] = i + 1 < n ? i + 1 : mu;
        if constexpr (Heavy) { digests[i] = heavyDigest(i); }}
    array<Output, 2> references;
    for (int method = 0; method < 2; ++method) {
        context = "mu=" + std::to_string(mu) + " lambda=" + std::to_string(lambda)
                  + " successor=" + (Heavy ? "mix16" : "table") + " method=" + std::to_string(method);
        ulng calls = expectedCalls(mu, lambda, method);
        references[method] = run<Heavy, true>(arc, digests, method, ULLONG_MAX);
        verify(references[method], mu, lambda, calls, references[method].checksum);
        auto exact = run<Heavy, true>(arc, digests, method, calls);
        verify(exact, mu, lambda, calls, references[method].checksum);
        for (ulng budget : {ulng(0), calls - 1}) {
            auto stopped = run<Heavy, true>(arc, digests, method, budget);
            require(!stopped.result.found && stopped.result.entry == 0
                    && stopped.result.tail == 0 && stopped.result.length == 0,
                    "budget=" + std::to_string(budget) + " expected failure with reset result");
            require(stopped.calls == budget && stopped.result.evaluations == budget,
                    "budget=" + std::to_string(budget) + " expected exact invocation cap, actual="
                    + std::to_string(stopped.calls) + "/" + std::to_string(stopped.result.evaluations)); }}
    for (int rep = -1; rep < 5; ++rep) {
        for (int j = 0; j < 2; ++j) {
            int method = (j + rep + 1) % 2;
            vector<Output> outputs(batch);
            context = "mu=" + std::to_string(mu) + " lambda=" + std::to_string(lambda)
                      + " successor=" + (Heavy ? "mix16" : "table") + " method=" + std::to_string(method)
                      + " rep=" + std::to_string(rep);
            auto start = std::chrono::steady_clock::now();
            for (auto &out : outputs) { out = run<Heavy>(arc, digests, method, ULLONG_MAX); }
            double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() / batch;
            for (const auto &out : outputs) {
                verify(out, mu, lambda, references[method].calls, references[method].checksum); }
            if (rep >= 0) {
                cout << mu << ' ' << lambda << ' ' << (Heavy ? "mix16" : "table") << ' '
                     << (method ? "brent" : "floyd") << ' ' << rep << ' ' << batch << ' '
                     << references[method].calls << ' ' << references[method].checksum << ' '
                     << std::setprecision(12) << ms << '\n'; }}}
}

int main() {
    for (auto [mu, lambda] : vector<pair<int, int>>{
            {0, 1}, {1, 1}, {0, 2}, {3, 2}, {8, 1}, {1, 8},
            {31, 1}, {32, 1}, {33, 1}, {0, 31}, {0, 32}, {0, 33}, {63, 65},
            {4096, 1}, {1, 4096}, {4095, 4096}, {4096, 4097},
            {65536, 3}, {3, 65536}, {32768, 32769}}) {
        benchmark<false>(mu, lambda); benchmark<true>(mu, lambda); }
}
