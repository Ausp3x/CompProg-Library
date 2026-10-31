// PROBLEM: https://judge.yosupo.jp/problem/addition_of_hex_big_integers
// Status: migrated source; online acceptance not verified in this migration.
// Whole-stdin driver: one read, tokens as string_view through read, toChars into one buffer, one write (the per-case iostream cost dominated the many-case files).
#include "../../../../01-Core/07-infint.hpp"
#include <sys/stat.h>
#include <unistd.h>

int main() {
    string in;
    { struct stat st; if (fstat(0, &st) == 0 && st.st_size > 0) { in.reserve(size_t(st.st_size) + 1); } }
    { char buf[1 << 16]; ssize_t k; while ((k = read(0, buf, sizeof buf)) > 0) { in.append(buf, size_t(k)); } }
    const char *p = in.data(), *e = p + in.size();
    auto token = [&] { while (p < e && *p <= ' ') { ++p; } const char *s = p; while (p < e && *p > ' ') { ++p; } return string_view(s, size_t(p - s)); };
    int t = 0;
    for (char c : token()) { t = t * 10 + (c - '0'); }
    string out(in.size() + size_t(t) * 2 + 64, '\0');
    char *o = out.data(), *oe = o + out.size();
    auto reserve = [&](size_t need) { if (size_t(oe - o) < need) { size_t off = size_t(o - out.data()); out.resize(2 * out.size() + need); o = out.data() + off; oe = out.data() + out.size(); } };
    iint a, b;
    while (t--) {
        a.read(token(), 16); b.read(token(), 16); a += b;
        reserve(size_t(a.charsBound(16)) + 1);
        o = a.toChars(o, oe, 16).ptr; *o++ = '\n';
    }
    for (const char *w = out.data(); w < o;) { ssize_t k = write(1, w, size_t(o - w)); if (k <= 0) { return 1; } w += k; }
    return 0;
}
