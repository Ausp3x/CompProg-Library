#!/usr/bin/env python3
from _00_runner import main

if __name__ == "__main__":
    raise SystemExit(main("32-static_range_queries", ["distinct-reversed", "distinct-right", "distinct-negative", "distinct-empty-right",
                                                      "frequency-reversed", "frequency-right", "frequency-negative", "frequency-default-right",
                                                      "mex-reversed", "mex-right", "mex-negative",
                                                      "persistent-reversed", "persistent-right", "persistent-negative", "persistent-default-right",
                                                      "majority-reversed", "majority-right", "majority-negative",
                                                      "frequent-reversed", "frequent-right", "frequent-negative", "frequent-zero-threshold", "frequent-zero-threshold-empty", "frequent-negative-threshold"]))
