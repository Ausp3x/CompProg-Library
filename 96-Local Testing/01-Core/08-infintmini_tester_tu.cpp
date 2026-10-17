// Second translation unit: the mini links without duplicate symbols.
#include "../../01-Core/08-infintmini.hpp"

int otherUnit(const InfIntMini &a) { return int(lng(a + InfIntMini(1))); }
