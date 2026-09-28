#include "../1-Core/05-infint.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> iint::SetBase(16) >> a >> b;
        a *= b;
        cout << iint::SetBase(16) << a << '\n';
    }

    return 0;
}