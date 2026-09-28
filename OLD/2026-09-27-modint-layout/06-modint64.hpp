#pragma once
#include "01-template.hpp"
#include "05-modint.hpp"

// Full nonzero ulng modulus domain; same operations/contracts as ModInt.
// S: O(1), Q: O(1), M: O(1); pow/inv/root/batch/stream bounds in 05-modint.hpp.
template<ulng MOD> requires (MOD > 0)
using ModInt64 = modint_detail::Value<modint_detail::Context<ulng, MOD, 0>>;
