"""Linear algebra against Leibniz expansion, brute force over GF(p), own Gauss-Jordan, block powers and residuals.

C++ (optimized -O2 -DNDEBUG, checked -O0 _GLIBCXX_DEBUG and, outside quick, ASan/UBSan; every mode):
oracle validation (Leibniz fixtures, own Gauss determinant and rank against Leibniz and GF(3) span
enumeration, no-pivot elimination against leading minors); identityMatrix and matrixProduct over mint,
Z/12, ulng mod 2^64 and double with empty and n x 0 shapes; rref (RREF shape, pivots, row space),
matrixRank, solveLinear, nullspace, solveLinearMultiRhs (x untouched on failure), determinant (Leibniz,
n <= 7), inverse (A X = X A = I, failure iff det = 0) and adjugate (cofactors on random, rank n - 1 and
rank <= n - 2 matrices) over GF(2), GF(3), GF(5), GF(7), DynModInt mod 11 with whole solution sets and
row spaces enumerated, and over 998244353 and 1e9+7 with own-Gauss ranks and exact residuals;
determinantMod and inverseMod over 27 moduli in [1, 2^63) (1, 2^k, 6, 12, 720720, 2^32 + 1, 10^18,
2^62 + 135, 2^63 - 1, products of 30-bit primes, the primes 2^61 - 1 and 29 * 2^57 + 1) against
Leibniz with __int128 arithmetic (n <= 6, any lng entries including INT64_MIN/MAX, unit-free columns)
and P L U products with known determinant (n <= 24, non-unit pivots), failure iff gcd(det, m) != 1,
divisor reductions and prime-field determinants; determinantInteger (int, lng, lll) against Leibniz in
lll with zero rows/columns and forced swaps plus P L U products (n <= 10); matrixPower and
matrixPowerSum over mint, GF(2), Z/12 and ulng for e <= 20 by repeated products and e up to 2^64 - 1
by [[A, I], [0, I]]^e; tridiagonalSolve, cyclicTridiagonalSolve (n = 1, 2 folds) and bandedSolve
(p, q <= 3, junk outside the band) for n <= 30 over GF(2..7) and mint (success iff every leading minor
is nonzero, exact A x = d) and over diagonally dominant doubles (residuals); determinantReal (Leibniz,
tolerance 1e-11 * product of row sums), matrixRankReal (rank-r products, eps fixtures),
solveLinearReal (residuals, kernel size and independence, inconsistent systems), luDecompose and
luSolve (reconstruction of row perm[i], residuals, singular inputs and eps); lowRankUpdateInverse
(k <= 4, k = 0, singular updates leave ainv untouched) against own inverses of A + U V over mint and
GF(2..5); FieldBasis insert/represent/rollback/size random interleavings against a model list (brute
spans over GF(2..5), own ranks over mint); freivaldsProductCheck (true on products and empty or inner-0
shapes, perturbations caught with 2 rounds over mint and 40 over GF(2), seed reproducibility);
schwartzZippelIdentityCheck (determinant identities, polynomial expansions, nonzero polynomials,
rounds 0, vars 0, recorded evaluation points per seed). Cases: quick 1/10, full 1x, stress 4x.
Builds (once per run, after every variant and probe passed): header alone; two translation units instantiating every
operation, with and without -DNDEBUG, -Wall -Wextra -Wconversion -Werror. 35 assertion probes.
"""
import os
from pathlib import Path
import shlex
import subprocess
import sys
import tempfile
from _00_runner import HERE, main

ROOT = HERE.parents[1]
INCLUDES = (f'#include "{ROOT / "05-Mathematics" / "16-linear_algebra.hpp"}"\n'
            f'#include "{ROOT / "01-Core" / "05-modint.hpp"}"\n')
USE_ALL = r'''
static int useAll() {
    Mat<mint> a{{1, 2}, {3, 4}}, e = a, x, ainv{{-2, 1}, {mint(3) / 2, mint(-1) / 2}};
    Mat<lng> l{{1, 2}, {3, 4}};
    Mat<double> d{{4, 1}, {2, 3}}, lu = d;
    vector<int> perm;
    vector<mint> coef, b{2}, one{4};
    FieldBasis<mint> fb(2);
    int bad = 0;
    bad += matrixProduct(a, identityMatrix<mint>(2)) != a;
    bad += rref(e) != vector<int>{0, 1} || matrixRank(a) != 2 || determinant(a) != mint(-2);
    bad += determinantMod(l, 6) != 4 || determinantInteger(l) != -2 || !inverseMod(l, 7);
    e = a;
    bad += !inverse(e) || e != ainv || adjugate(a) != Mat<mint>{{4, -2}, {-3, 1}};
    bad += solveLinear(a, vector<mint>{5, 11}) != Mat<mint>{{1, 2}} || !nullspace(a).empty();
    bad += !solveLinearMultiRhs(a, Mat<mint>{{5}, {11}}, x) || x != Mat<mint>{{1}, {2}};
    bad += matrixPower(a, 2) != Mat<mint>{{7, 10}, {15, 22}} || matrixPowerSum(a, 2) != Mat<mint>{{2, 2}, {3, 5}};
    bad += !tridiagonalSolve(b, b, b, one) || one != vector<mint>{2};
    one = {8};
    bad += !cyclicTridiagonalSolve(b, b, b, one) || one != vector<mint>{mint(8) / 6};
    one = {4};
    bad += !bandedSolve(Mat<mint>{{2}}, 0, 0, one) || one != vector<mint>{2};
    bad += !luDecompose(lu, perm) || abs(luSolve(lu, perm, vector<double>{5, 5})[0] - 1) > 1e-12;
    bad += abs(solveLinearReal(d, vector<double>{5, 5})[0][1] - 1) > 1e-12 || matrixRankReal(d) != 2 || abs(determinantReal(d) - 10) > 1e-12;
    e = ainv;
    bad += !lowRankUpdateInverse(e, Mat<mint>{{1}, {0}}, Mat<mint>{{0, 0}});
    bad += !fb.insert(vector<mint>{1, 1}) || !fb.represent(vector<mint>{2, 2}, coef) || coef != vector<mint>{2};
    fb.rollback(0);
    bad += fb.size() != 0 || !freivaldsProductCheck(a, identityMatrix<mint>(2), a);
    bad += !schwartzZippelIdentityCheck<mint>([](const vector<mint> &v) { return v[0] * v[1] - v[1] * v[0]; }, 2);
    return bad;}
'''


def builds():
    """Header alone and two translation units, after every variant and probe passed."""
    compiler = os.environ.get('CXX', 'g++')
    warn = ['-std=gnu++20', '-Wall', '-Wextra', '-Wconversion', '-Werror']
    color = sys.stdout.isatty() and 'NO_COLOR' not in os.environ
    try:
        with tempfile.TemporaryDirectory(prefix='cp-linalg-') as name:
            tmp = Path(name)
            (tmp / 'alone.cpp').write_text(INCLUDES.splitlines()[0] + '\nint main() {}\n')
            (tmp / 'tu1.cpp').write_text(INCLUDES + USE_ALL + 'int other();\nint main() { return useAll() * 10 + other(); }\n')
            (tmp / 'tu2.cpp').write_text(INCLUDES + USE_ALL + 'int other() { return useAll(); }\n')
            commands = [[compiler, *warn, '-fsyntax-only', str(tmp / 'alone.cpp')]]
            for label, flags in (('asserts', ['-O2']), ('ndebug', ['-O2', '-DNDEBUG'])):
                out = str(tmp / label)
                commands += [[compiler, *warn, *flags, str(tmp / 'tu1.cpp'), str(tmp / 'tu2.cpp'), '-o', out], [out]]
            for command in commands:
                result = subprocess.run(command, capture_output=True, text=True, timeout=180)
                if result.returncode:
                    raise RuntimeError(f'returncode={result.returncode} command={shlex.join(command)}\n{result.stdout}\n{result.stderr}')
    except (OSError, RuntimeError, subprocess.TimeoutExpired) as error:
        print(('\033[31m' if color else '') + f'FAIL 16-linear_algebra builds: {error}' + ('\033[0m' if color else ''),
              file=sys.stderr, flush=True)
        return 1
    print(('\033[32m' if color else '') + 'PASS 16-linear_algebra header alone; two translation units (asserts and -DNDEBUG,'
          ' -Wall -Wextra -Wconversion -Werror)' + ('\033[0m' if color else ''), flush=True)
    return 0


if __name__ == '__main__':
    raise SystemExit(main('16-linear_algebra', [
        'det-nonsquare', 'product-shape', 'power-nonsquare', 'detmod-zero', 'detmod-negative', 'detmod-nonsquare',
        'inversemod-zero', 'inversemod-nonsquare', 'integer-nonsquare', 'inverse-nonsquare', 'adjugate-nonsquare',
        'solve-size', 'solve-ragged', 'multirhs-size', 'multirhs-ragged', 'lu-nonsquare', 'lusolve-size',
        'tridiagonal-size', 'cyclic-size', 'banded-width', 'banded-negative', 'banded-rows', 'solvereal-size',
        'detreal-nonsquare', 'lowrank-shape', 'basis-length', 'basis-represent-length', 'basis-rollback',
        'freivalds-shape', 'freivalds-inner', 'rank-ragged', 'rref-ragged', 'nullspace-ragged', 'product-ragged',
        'basis-rollback-negative']) or builds())
