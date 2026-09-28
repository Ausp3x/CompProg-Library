// PROBLEM: https://judge.yosupo.jp/problem/division_of_hex_big_integers
// Status: migrated source; online acceptance not verified in this migration.
#include "../../../../01-Core/07-infint.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> iint::SetBase(16) >> a >> b;
        auto [q, r] = divMod(a, b);
        cout << iint::SetBase(16) << q << ' ' << r << '\n';
    }

    return 0;
}