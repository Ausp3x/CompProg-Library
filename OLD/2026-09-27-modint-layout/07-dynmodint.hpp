#pragma once
#include "05-modint.hpp"

// Full nonzero uint modulus; independent per ID, default 998244353.
// setMod invalidates all prior values/caches, even if the modulus is unchanged.
// Fields of the shared context are read-only; use setMod. Single-threaded.
// S: O(log(mod)) auto-primality, Q: O(1), M: O(1) per value and per ID.
// pow/inv/root/batch/stream bounds and failure contracts in 05-modint.hpp.
template<int ID = 0> using DynModInt = modint_detail::Value<modint_detail::Context<uint, 0, ID>>;
