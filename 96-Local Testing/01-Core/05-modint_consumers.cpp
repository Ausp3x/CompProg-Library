#include "../../01-Core/99-All.hpp"
#include "../../05-Mathematics/05-combinatorics.hpp"

// Compatibility smoke for existing mint consumers, not a Matrix/ModFac audit.
int main() {
    ModFac f(20);
    if (f.combiNR(10, 3) != 120 || f.combiWR(3, 4) != 15 || f.permuWR(3, 4) != 81) { return 1; }
    Matrix<mint> a(2, 2, {1, 2, 3, 4}), b = a * a;
    if (b[0][0] != 7 || b[0][1] != 10 || b[1][0] != 15 || b[1][1] != 22 || det(a) != -2) { return 2; }
    return 0;}
