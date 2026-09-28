# Contest checklist

## Before coding

- Read the statement again after identifying the likely technique.
- Record input bounds, time limit, memory limit, and required output format.
- Estimate the complexity with the largest stated bound.
- Identify whether indices are zero-based or one-based.
- Check whether empty, disconnected, duplicate, or negative cases are possible.

## During implementation

- Write down the invariant before a tricky loop or data structure update.
- Choose integer widths for intermediate products, not only final values.
- Keep modular values normalized; watch subtraction and multiplication overflow.
- Confirm recursion depth and stack use against the largest case.
- Verify all input is consumed and all answers are printed in the required order.

## Before submitting

- Test the sample, smallest case, maximum bound, ties, and boundary transitions.
- Compare against a slow brute-force solution on tiny random instances when possible.
- Recheck off-by-one ranges and inclusive versus exclusive endpoints.
- Remove debug output; confirm the intended language standard and file are selected.

# Quick mathematics

## Modular arithmetic

- A modular inverse of a exists modulo m exactly when gcd(a, m) = 1.
- Fermat's inverse a^(p-2) needs prime p and a not divisible by p.
- Division under a composite modulus needs separate reasoning.

## Graphs and counting

- For weighted shortest paths, Dijkstra needs nonnegative edge weights.
- Distances and path counts are different values; equal-distance relaxations matter.
- For a forest on n vertices and c components, the edge count is n - c.
