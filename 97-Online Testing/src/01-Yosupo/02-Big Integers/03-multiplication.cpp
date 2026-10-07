// PROBLEM: https://judge.yosupo.jp/problem/multiplication_of_big_integers
// Status: migrated source; online acceptance not verified in this migration.
#include "../../../../01-Core/07-infint.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> a >> b;
        a *= b;
        cout << a << '\n';
    }

    return 0;
}