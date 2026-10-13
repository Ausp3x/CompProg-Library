"""SlopeTrick, WeightedSlopeTrick and DualSlopeTrick against an exact dense-window oracle and independent DPs.

The oracle stores exact values on the integers of [-128, 128] (+INF outside the domain) and extends
them linearly beyond the window; a conservative breakpoint bound keeps every breakpoint and finite
domain end inside [-124, 124], so each brute operation (window minimum for slide, prefix/suffix minima,
direct infimal convolution over all breakpoint candidates, clamp by direct minimization) is exact.
Random sequences replay through position/weight maps p -> s*p + t, w -> u*w, which reach positions
near 2^62, lll intermediate sums and int/double instantiations; queries after every operation compare
size, min, argminRange (with -INF/INF ends), domain and evaluate (dense on the last step of each
sequence and for small extents, 20 sampled points otherwise, plus four far points). DualSlopeTrick is
probed through atZero, domain and f(x) by shift-and-atZero on a copy.
Independent end-to-end oracles: monotone fit in both directions, bounded-step and linear-move-cost
tracking against O(n*V) / O(n*V^2) tables, APIO 2016 Fireworks tree DP, choose-exactly-K against a
sorting closed form with costs up to 5*10^14, and a weighted-median oracle on the heap-of-pairs
ping-pong pattern. Balance regressions check treap depth after duplicate-key merges, an equal-edge
Fireworks star, self-merges and shifted-copy merges, and reuse of moved-from objects. A unit/weighted
cross-check (supporting only) also checks treap node reuse and copy independence. Hand fixtures
validate the oracle and the INF/2^62 extremes.

quick: optimized and checked builds; per map 40 unit, 40 weighted and 30 dual sequences (20 for the
int/double instantiations) of up to 16 operations; classics 10 rounds; balance 4000 merges and 8
doublings; ping-pong 2000; cross-check 20000. full: adds ASan/UBSan; 8x the sequences, up to 48
operations; classics 80 rounds; balance 16000 merges and 12 doublings; ping-pong 50000; cross-check
200000. stress: full-mode counts with up to 96 operations, ping-pong 200000, cross-check 10^6 (the
-O0 _GLIBCXX_DEBUG build verifies the whole heap on every push/pop, so more rounds exceed the runner
timeout; stress volume comes from several seeds).
The checked build runs 20 assertion probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('34-slope_trick', [
        'slide-reversed', 'linear-range',
        'weighted-negative-weight', 'weighted-slide-reversed', 'weighted-empty-domain',
        'weighted-disjoint-merge', 'weighted-min-unbounded', 'weighted-argmin-unbounded',
        'weighted-prefix-unbounded', 'weighted-suffix-unbounded', 'weighted-clamp-positive',
        'weighted-clamp-unbounded', 'weighted-convolution-unbounded',
        'weighted-amx-negative-weight', 'weighted-abs-negative-weight', 'weighted-clamp-negative-right',
        'weighted-convolution-this-unbounded',
        'dual-slide-reversed', 'dual-negative-weight', 'dual-amx-negative-weight']))
