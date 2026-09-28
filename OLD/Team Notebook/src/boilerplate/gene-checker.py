import itertools, subprocess
for i in itertools.count(start=1):
    subprocess.run('./gen.py > input.txt', shell=True, check=True)
    subprocess.run('./solution < input.txt > output.txt', shell=True, check=True)
    subprocess.run('./brute.py < input.txt > expected.txt', shell=True, check=True)
    subprocess.run('diff output.txt expected.txt', shell=True, check=True)
    print(f'{i} test cases passed')