// Second translation unit: the same inline PolyMini instantiations must link without duplicate symbols.
#include "../../01-Core/06-modintmini.hpp"
#include "../../01-Core/17-polymini.hpp"

ulng otherTranslationUnit() {
    using T = ModIntMini<998244353>;
    PolyMini<T> a = {T(1), T(2), T(3), T(4)}, b = a * PolyMini<T>{T(1), T(-1)};
    vector<T> v(a.begin(), a.end());
    PolyMini<T>::ntt(v); PolyMini<T>::ntt(v, true);
    return ulng(eval(b, T(1)).val()) + ulng(v[3].val() * 5 % T::mod()) + 20 * ulng(inv(PolyMini<T>{T(1), T(1)}, 2)[1].val()) % T::mod();}
