// Second translation unit: Rational over every supported T links without duplicate symbols.
#include "../../01-Core/07-infint.hpp"
#include "../../01-Core/09-rational.hpp"

int otherUnit(const Frac &x) { return int(floor(x * 12)) + int(lng(Rational<InfInt>(InfInt(-1)).num)) + int(Rational<lll>(1).num); }
