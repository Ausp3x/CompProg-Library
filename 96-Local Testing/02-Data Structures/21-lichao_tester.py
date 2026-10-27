#!/usr/bin/env python3
from _00_runner import main

if __name__ == "__main__":
    raise SystemExit(main("21-lichao", [
        "tree-domain-empty", "tree-domain-inverted", "per-domain-empty", "tree-query-below", "tree-query-above",
        "per-query-below", "per-query-above", "tree-merge-domain", "tree-merge-self", "tree-undo-empty",
        "tree-undo-exhausted", "compressed-undo-empty", "compressed-undo-exhausted", "per-addline-version", "per-addline-negative",
        "per-addsegment-version", "per-query-version", "compressed-query-nonpoint", "compressed-query-above",
        "compressed-query-empty"]))
