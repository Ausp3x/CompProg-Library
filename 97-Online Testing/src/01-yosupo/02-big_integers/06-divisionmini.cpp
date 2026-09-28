// PROBLEM: https://judge.yosupo.jp/problem/division_of_big_integers
// Status: migrated source; online acceptance not verified in this migration.
#include "../../../../01-Core/08-infintmini.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iintmini a, b;
        cin >> a >> b;
        auto [q, r] = divMod(a, b);
        cout << q << ' ' << r << '\n';
    }

    return 0;
}