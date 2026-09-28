#pragma once
#include "../1-Core/01-template.hpp"

// T: O(n), M: O(1)
template<typename F>
double fixedIntegS(double a, double b, F f, int n = 1000000) {
    n += n & 1; double res = f(a) + f(b), d = (b - a) / n;
    for (int i = 1; i < n; i++) { res += f(a + i * d) * (2.0 + (i & 1) * 2.0); }
    return res * d / 3.0;}

// T: O(n), M: O(log(n))
template<typename F>
double adaptiveIntegS(double a, double b, F f, double eps = 1e-9) {
    auto calc = [&](auto &&calc, double l, double r, 
                    double fl, double fm, double fr, double res, double eps) -> double {
        double m = l + (r - l) / 2.0;
        double flm = f(l + (m - l) / 2.0), lres = (fl + 4.0 * flm + fm) * (m - l) / 6.0; 
        double frm = f(m + (r - m) / 2.0), rres = (fm + 4.0 * frm + fr) * (r - m) / 6.0;
        if (abs(lres + rres - res) <= 15.0 * eps) { 
            return lres + rres + (lres + rres - res) / 15.0;}
        return calc(calc, l, m, fl, flm, fm, lres, eps / 2.0) + 
               calc(calc, m, r, fm, frm, fr, rres, eps / 2.0);};
    double m = a + (b - a) / 2.0, fa = f(a), fm = f(m), fb = f(b);
    return calc(calc, a, b, fa, fm, fb, (fa + 4.0 * fm + fb) * (b - a) / 6.0, eps);}