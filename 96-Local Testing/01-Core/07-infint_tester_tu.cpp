// Second translation unit: the header's inline functions, statics and templates link once.
#include "../../01-Core/07-infint.hpp"
ulng otherUnitSum(const InfInt &a) { InfInt s = 0; for (uint x : a.n) { s += x; } return ulng(s % 1000000007ULL) + ulng(newtonRaphsonInv(InfInt(0x80000000u)).n.size()); }
