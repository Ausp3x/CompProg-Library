"""Longest common subsequence family: independent full-table DP and enumeration.

Every pair checks lcs, bitsetLcs and huntSzymanski against an O(nm) table,
lcsWitness and hirschbergLcs as strictly increasing matched pairs of the right
size, myersDiff by replaying the script (cursors, keeps, deletions, insertions,
edit count n + m - 2 LCS) and shortestCommonSupersequence (length and both
inputs as subsequences), over bytes, sparse lng alphabets (rare symbols use
scratch masks), equality-only symbols (scalar rows) and string symbols.
Subsequence enumeration checks countDistinctSubsequences, both
countPalindromicSubsequences variants (exact and mod 998244353),
longestPalindromicSubsequence and lcs3. Quick/full/stress enumerate all binary
pairs through length 5/6/7, run 300/2000/6000 random cases (lengths up to 139,
second lengths at 63..65/127..129/191..193), medium strings of length
300/1200/2500 over 2/4/26/256 letters with interval-DP and per-letter
recurrence oracles, and large cases n = 5000/40000/60000 (bitsetLcs versus
huntSzymanski, witness, myersDiff on 40 edits, unary closed forms). Builds use
-Wall -Wextra -Wshadow -Wconversion -Werror. Full/stress add ASan/UBSan. The
checked _GLIBCXX_DEBUG build divides the random count and the medium and large
sizes by 4 and runs stress at full-mode sizes (the debug library checks every
binary-search range linearly); it runs one precondition probe (the other
preconditions are sizes near INT_MAX).
"""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('19-lcs', ['lcs3-table-overflow'], strict=True))
