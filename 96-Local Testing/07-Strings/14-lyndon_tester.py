"""Lyndon words: brute-force Lyndon tests (strictly below every proper suffix).

Every case checks duval against the greedy longest-Lyndon-prefix factorization,
longestLyndonPrefix, isLyndon, standardFactorization (longest proper Lyndon
suffix), lyndonArray per suffix, suffixFactorization from every start,
lyndonTree (node count, roots = factors, every node Lyndon, right child = the
longest proper Lyndon suffix) and IncrementalLyndon factorize/minSuffix of every
prefix after every add. Quick/full/stress enumerate ternary strings through
lengths 6/8/9, compare lyndonWords (up to n and exact n) and nextLyndonWord with
a sorted brute enumeration for k <= 4 and n <= 6/9/11, run 200/1500/10000
random byte strings (some periodic) also as extreme-lng vectors plus the 256
bytes in both orders, every string over bytes {00, 7f, 80, ff} through lengths
5/7/7 as string, 5/6/6 as vector<char> and vector<signed char>, 5 as
vector<unsigned char> and u8string, 200/1000/2000 random vector<string>,
deque<int16_t> and vector<uint16_t> cases of length <= 20, cross-check Duval, IncrementalLyndon
and the SA-IS Lyndon array on unary, random, periodic and Thue-Morse strings of
length 20000/300000/1000000, and check lyndonArray on random 256-byte, random
{00, ff} and (7f^4 80)^* strings of length 20000/1000000/1000000 against the
independent prefix-doubling suffix array (next smaller suffix by sparse table). Builds use -Wall -Wextra -Wshadow -Wconversion
-Werror. Full/stress add ASan/UBSan (the checked _GLIBCXX_DEBUG build runs stress at full-mode sizes); checked builds run twelve precondition
probes.
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('14-lyndon', [
        'standard-single', 'standard-not-lyndon', 'tree-zero-length', 'tree-overflow',
        'suffix-negative', 'suffix-high', 'suffix-zero-length', 'min-suffix-high', 'factorize-negative',
        'next-empty', 'next-too-long', 'words-negative'], strict=True))
