from _00_foundation_runner import run_suite

if __name__ == "__main__":
    run_suite("02-debug", ["02-debug_tester.cpp", "02-debug_no-local_tester.cpp"], "02-debug_compile_tester.py")
