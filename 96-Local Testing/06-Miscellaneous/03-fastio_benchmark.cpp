#include "../../06-Miscellaneous/03-fastio.hpp"
#include <cinttypes>

void need(bool ok, const char *what) { if (!ok) { cerr << "FAIL " << what << '\n'; std::exit(1); } }
struct File {
    std::FILE *f = std::tmpfile();
    File() { need(f, "tmpfile"); }
    ~File() { std::fclose(f); }
};
template<typename T> string decimal(T x) {
    char b[50]; auto r = std::to_chars(b, b + 50, x);
    need(r.ec == std::errc{}, "to_chars"); return string(b, r.ptr);}
template<int N> ulng fast(std::FILE *input, std::FILE *output, int n) {
    FastInput<N> in(input); FastOutput<N> out(output); ulng checksum = 0; lng x;
    for (int i = 0; i < n; ++i) {
        need(in.readInt(x), "fast read"); ulng y = ulng(x) ^ 0x9e3779b97f4a7c15ULL;
        checksum += y; need(out.writeInt(y, '\n'), "fast write");}
    need(!in.readInt(x) && in.status == FastIoStatus::Eof, "fast EOF"); need(out.flush(), "fast flush");
    return checksum;}
ulng stdio(std::FILE *input, std::FILE *output, int n) {
    ulng checksum = 0; lng x;
    for (int i = 0; i < n; ++i) {
        need(std::fscanf(input, "%" SCNd64, &x) == 1, "stdio read"); ulng y = ulng(x) ^ 0x9e3779b97f4a7c15ULL;
        checksum += y; need(std::fprintf(output, "%" PRIu64 "\n", y) > 0, "stdio write");}
    need(std::fscanf(input, "%" SCNd64, &x) == EOF && !std::ferror(input), "stdio EOF");
    need(std::fflush(output) == 0, "stdio flush"); return checksum;}
template<int N> ulng fastReal(std::FILE *input, std::FILE *output, int n) {
    FastInput<N> in(input); FastOutput<N> out(output); ulng checksum = 0; double x;
    for (int i = 0; i < n; ++i) {
        need(in.readDouble(x), "fast real read"); checksum += std::bit_cast<ulng>(x);
        need(out.writeDouble(x, 9, '\n'), "fast real write");}
    need(!in.readDouble(x) && in.status == FastIoStatus::Eof, "fast real EOF"); need(out.flush(), "fast real flush");
    return checksum;}
ulng stdioReal(std::FILE *input, std::FILE *output, int n) {
    ulng checksum = 0; double x;
    for (int i = 0; i < n; ++i) {
        need(std::fscanf(input, "%lf", &x) == 1, "stdio real read"); checksum += std::bit_cast<ulng>(x);
        need(std::fprintf(output, "%.9f\n", x) > 0, "stdio real write");}
    need(std::fscanf(input, "%lf", &x) == EOF && !std::ferror(input), "stdio real EOF");
    need(std::fflush(output) == 0, "stdio real flush"); return checksum;}
int main() {
    std::mt19937_64 rng(20260927);
    for (int n : {32, 20000, 500000}) {
        for (int distribution = 0; distribution < 3; ++distribution) {
            string input, expected; ulng checksum = 0;
            for (int i = 0; distribution == 2 && i < n; ++i) {
                double x = double(lng(rng() % 2000001) - 1000000) / 1000.0 * std::ldexp(1.0, int(rng() % 41) - 20);
                char b[64]; std::snprintf(b, sizeof b, "%.17g ", x); input += b;
                double y = std::strtod(b, nullptr); checksum += std::bit_cast<ulng>(y);
                std::snprintf(b, sizeof b, "%.9f\n", y); expected += b;}
            for (int i = 0; distribution < 2 && i < n; ++i) {
                lng x = distribution ? std::bit_cast<lng>(rng()) : lng(rng() % 2001) - 1000;
                if (distribution && i % 101 == 0) { x = i % 2 ? std::numeric_limits<lng>::min() : std::numeric_limits<lng>::max(); }
                ulng y = ulng(x) ^ 0x9e3779b97f4a7c15ULL; checksum += y;
                input += decimal(x) + (distribution ? " \t\r\n" : " "); expected += decimal(y) + '\n';}
            File source; need(std::fwrite(input.data(), 1, input.size(), source.f) == input.size(), "fixture input");
            need(std::fflush(source.f) == 0, "fixture flush");
            for (int rep = -1; rep < 5; ++rep) {
                for (int j = 0; j < 3; ++j) {
                    int method = (j + rep + 1) % 3;
                    std::rewind(source.f); File dest;
                    auto start = std::chrono::steady_clock::now();
                    ulng actual = distribution == 2 ? (method == 0 ? stdioReal(source.f, dest.f, n)
                            : method == 1 ? fastReal<4096>(source.f, dest.f, n) : fastReal<65536>(source.f, dest.f, n))
                        : method == 0 ? stdio(source.f, dest.f, n)
                        : method == 1 ? fast<4096>(source.f, dest.f, n) : fast<65536>(source.f, dest.f, n);
                    double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                    need(actual == checksum, "checksum"); std::rewind(dest.f);
                    string got(expected.size() + 1, '\0'); size_t bytes = std::fread(got.data(), 1, got.size(), dest.f);
                    got.resize(bytes); need(got == expected && !std::ferror(dest.f), "independent full output equality");
                    if (rep >= 0) {
                        cout << n << ' ' << (distribution == 2 ? "doubles-fixed9" : distribution ? "wide-whitespace" : "small-integers") << ' '
                             << (method == 0 ? "fscanf-fprintf" : method == 1 ? "fast-4096" : "fast-65536") << ' '
                             << rep << ' ' << std::setprecision(12) << ms << ' ' << input.size() << ' ' << expected.size() << ' ' << checksum << '\n';}}}}}}
