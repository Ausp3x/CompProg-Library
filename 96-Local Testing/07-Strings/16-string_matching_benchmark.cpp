#include "../../07-Strings/16-string_matching.hpp"

using Clock = std::chrono::steady_clock;
double ms(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double, std::milli>(b - a).count(); }
template<class F> double median(F f, lng expected, const string &label) {
    vector<double> times;
    for (int rep = 0; rep < 6; ++rep) {
        auto start = Clock::now();
        lng got = f();
        auto stop = Clock::now();
        if (got != expected) {
            cerr << "FAIL " << label << " expected=" << expected << " actual=" << got << '\n';
            std::exit(1);}
        if (rep) { times.push_back(ms(start, stop)); }}
    sort(times.begin(), times.end());
    return times[times.size() / 2];}
int main(int argc, char **argv) {
    ulng seed = argc > 1 ? std::stoull(argv[1]) : 20261010;
    std::mt19937_64 rng(seed);
    int n = 4000000;
    struct Work { string name; int sigma, m; };
    vector<Work> works;
    for (int sigma : {2, 4, 26, 256}) {
        for (int m : {4, 32, 256}) { works.push_back({"random", sigma, m}); }}
    for (int m : {4, 32, 256}) { works.push_back({"unary-hit", 1, m}); works.push_back({"unary-miss", 1, m}); works.push_back({"period2", 2, m}); }
    for (auto [name, sigma, m] : works) {
        string text(n, 'a'), pattern;
        if (name == "random") {
            for (char &c : text) { c = char('a' + rng() % sigma); }
            pattern = text.substr(rng() % (n - m), m);}
        else if (name == "unary-hit") { pattern.assign(m, 'a'); }
        else if (name == "unary-miss") { pattern = string(m - 1, 'a') + 'b'; }
        else {
            for (int i = 0; i < n; ++i) { text[i] = "ab"[i % 2]; }
            pattern = text.substr(0, m - 1) + 'a';}
        lng expected = lng(kmpOccurrences(pattern, text).size());
        vector<int> codes;
        for (char c : pattern) { codes.push_back(uint8_t(c)); }
        auto aut = name == "random" && sigma == 256 && m == 256 ? vector<vector<int>>{} : prefixAutomaton(codes, 256);
        vector<pair<string, std::function<lng()>>> algorithms{
            {"boyerMoore", [&] { return lng(boyerMoore(pattern, text).size()); }},
            {"horspool", [&] { return lng(horspool(pattern, text).size()); }},
            {"sunday", [&] { return lng(sunday(pattern, text).size()); }},
            {"twoWay", [&] { return lng(twoWay(pattern, text).size()); }},
            {"kmpOccurrences", [&] { return lng(kmpOccurrences(pattern, text).size()); }},
            {"zOccurrences", [&] { return lng(zOccurrences(pattern, text).size()); }},
            {"rabinKarp", [&] { return lng(rabinKarp(pattern, text).size()); }},
            {"string::find", [&] {
                lng res = 0;
                for (size_t at = text.find(pattern); at != string::npos; at = text.find(pattern, at + 1)) { ++res; }
                return res;}}};
        if (!aut.empty()) { algorithms.push_back({"dfaSearch", [&] { return lng(dfaSearch(aut, text).size()); }}); }
        if (name != "unary-hit" && name != "unary-miss" && name != "period2") { algorithms.push_back({"naiveSearch", [&] { return lng(naiveSearch(pattern, text).size()); }}); }
        for (auto &[label, f] : algorithms) {
            double t = median(f, expected, name + " " + label);
            cout << "{\"type\": \"bytes\", \"workload\": \"" << name << "\", \"sigma\": " << sigma << ", \"m\": " << m << ", \"n\": " << n
                 << ", \"occurrences\": " << expected << ", \"algorithm\": \"" << label << "\", \"median_ms\": " << t << "}" << endl;}}
    for (int sigma : {4, 1000000000}) {
        for (int m : {4, 64}) {
            vector<int> text(n);
            for (int &c : text) { c = int(rng() % sigma); }
            vector<int> pattern(text.begin() + 1000, text.begin() + 1000 + m);
            lng expected = lng(kmpOccurrences(pattern, text).size());
            vector<pair<string, std::function<lng()>>> algorithms{
                {"twoWay", [&] { return lng(twoWay(pattern, text).size()); }},
                {"kmpOccurrences", [&] { return lng(kmpOccurrences(pattern, text).size()); }},
                {"rabinKarp", [&] { return lng(rabinKarp(pattern, text).size()); }},
                {"naiveSearch", [&] { return lng(naiveSearch(pattern, text).size()); }}};
            for (auto &[label, f] : algorithms) {
                double t = median(f, expected, "ints " + label);
                cout << "{\"type\": \"ints\", \"workload\": \"random\", \"sigma\": " << sigma << ", \"m\": " << m << ", \"n\": " << n
                     << ", \"occurrences\": " << expected << ", \"algorithm\": \"" << label << "\", \"median_ms\": " << t << "}" << endl;}}}}
