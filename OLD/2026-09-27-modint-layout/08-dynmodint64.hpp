#pragma once
#include "05-modint.hpp"

// Full nonzero ulng modulus; independent per ID and from DynModInt's IDs.
// Default 998244353; setMod invalidates all prior values/caches on every call.
// Fields of the shared context are read-only; use setMod. Single-threaded.
// S: O(log(mod)) auto-primality, Q: O(1), M: O(1) per value and per ID.
// pow/inv/root/batch/stream bounds and failure contracts in 05-modint.hpp.
template<int ID = 0> using DynModInt64 = modint_detail::Value<modint_detail::Context<ulng, 0, ID>>;
