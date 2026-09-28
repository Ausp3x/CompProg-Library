#include "../1-Core/05-infint.hpp"

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    int t;
    cin >> t;
    while (t--) {
        iint a, b;
        cin >> a >> b;
        a += b;
        cout << a << '\n';
    }

    return 0;
}