#!/usr/bin/env python3
from _00_runner import main

if __name__ == "__main__":
    raise SystemExit(main("31-radixheap", ["heap-push-below-last-after-pop", "heap-push-below-last-after-top", "heap-push-below-last-drained", "heap-push-below-last-negative", "heap-push-below-last-unsigned",
                                           "heap-top-empty", "heap-pop-empty", "heap-pop-drained",
                                           "array-push-below-last-after-pop", "array-push-below-last-after-top", "array-top-empty", "array-pop-empty", "array-contains-range", "array-contains-negative",
                                           "array-push-range", "array-key-absent", "array-key-popped", "array-push-present", "array-decrease-absent", "array-decrease-above", "array-decrease-below-last", "array-size-negative"]))
