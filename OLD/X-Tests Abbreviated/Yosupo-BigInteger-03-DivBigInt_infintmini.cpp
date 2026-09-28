#include "../1-Core/06-infintmini.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> a >> b;
        auto [q, r] = divMod(a, b);
        cout << q << ' ' << r << '\n';
    }

    return 0;
}