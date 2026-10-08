"""Interactive checked by instrumented streams, exit scenarios and a live pipe interactor.

In-process (all modes): an output streambuf that publishes bytes only on sync
and an input streambuf that serves one byte at a time and records any byte read
while output is pending, so a missing flush before any read fails. Fixed cases
cover argument formatting (chars, strings, string_view, integers, doubles,
nested/empty ranges, sets, arrays, empty lines), ask/ask<T> return types,
used/remaining/budget/reset, answer not counted, LLONG_MIN/MAX, token "-1",
disabled sentinel, unsigned replies (all-ones, text -1) with the sentinel
disabled, readLine whitespace/CR handling. Seeded random transcripts (200/5000/50000 for
quick/full/stress) are compared with an independently rebuilt transcript.

Subprocess (all modes, every configuration): exit scenarios on real stdio check
status 0 and the exact flushed output for an integer -1 reply, a custom
sentinel, a verdict -1 after answer, EOF on int/token/line, a malformed token
and an int overflow. A Python interactor plays seeded multi-test binary search
over pipes (cin untied, sync off) with an echo line, enforces ceil(log2 n)
queries per case, answers verdicts and finally replies -1 mid-case, expecting a
silent exit 0; 3/20/60 games of 1/5/12 cases, with a 30 s watchdog that turns a
missing flush into a failure. Budget overruns and unsigned reply types with an
active sentinel are assertion probes.
"""
import random
import subprocess
import threading

from _00_runner import main

SCENARIOS = {
    'error-int': ('5\n-1\n', '? 1\n? 2\n'),
    'error-custom': ('3 0\n', '? 1\n? 2\n'),
    'error-after-answer': ('-1\n', '! 3\n'),
    'eof-int': ('', '? 1\n'),
    'eof-token': ('  \n', '? 1\n'),
    'eof-line': ('\n\n', 'hi\n'),
    'bad-token': ('abc\n', '? 1\n'),
    'overflow': ('99999999999\n', '? 1\n'),
}


def run_scenarios(binary, env):
    for name, (stdin, expected) in SCENARIOS.items():
        result = subprocess.run([str(binary), '--scenario', name], input=stdin, capture_output=True,
                                text=True, env=env, timeout=30)
        if result.returncode != 0 or result.stdout != expected:
            raise RuntimeError(f'scenario={name} input={stdin!r} expected status 0 and {expected!r}, '
                               f'got status {result.returncode} and {result.stdout!r} stderr={result.stderr!r}')


def play(binary, env, rng, cases):
    proc = subprocess.Popen([str(binary), '--scenario', 'play'], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, text=True, env=env, bufsize=1)
    timer = threading.Timer(30, proc.kill)
    timer.start()
    log = []

    def send(line):
        log.append('> ' + line)
        proc.stdin.write(line + '\n')
        proc.stdin.flush()

    def recv():
        line = proc.stdout.readline()
        log.append('< ' + line.rstrip('\n'))
        if not line.endswith('\n'):
            raise RuntimeError('program output ended or stalled (missing flush or watchdog kill)')
        return line[:-1]

    try:
        greeting = ' '.join(rng.choice(['a', 'bc', 'def', '1', '-1']) for _ in range(rng.randint(1, 5)))
        send(str(cases))
        send(' ' + greeting + ' \r')
        if recv() != 'echo ' + greeting + ' ':
            raise RuntimeError('readLine echo mismatch')
        for case in range(cases):
            n = rng.choice([1, 2, 3, rng.randint(1, 100), rng.randint(1, 10**18)])
            sabotage = case == cases - 1
            n = max(n, 2) if sabotage else n
            x = rng.randint(1, n)
            send(str(n))
            limit, used = (n - 1).bit_length(), 0
            while True:
                words = recv().split()
                if words[0] == '!':
                    if sabotage or len(words) != 2 or int(words[1]) != x:
                        raise RuntimeError(f'wrong answer {words}, hidden {x}')
                    send('1')
                    break
                if words[0] != '?' or len(words) != 2:
                    raise RuntimeError(f'malformed query {words}')
                used += 1
                if used > limit:
                    raise RuntimeError(f'query budget {limit} exceeded')
                if sabotage:
                    send('-1')
                    rest = proc.stdout.read()
                    proc.wait(timeout=30)
                    if rest or proc.returncode != 0:
                        raise RuntimeError(f'after -1 expected silent exit 0, got {rest!r} status {proc.returncode}')
                    return
                send('1' if x >= int(words[1]) else '0')
        if recv() != 'done':
            raise RuntimeError('missing final line')
        proc.stdin.close()
        proc.wait(timeout=30)
        if proc.returncode != 0:
            raise RuntimeError(f'status {proc.returncode}')
    except (RuntimeError, ValueError, IndexError, OSError) as error:
        proc.kill()
        raise RuntimeError(f'interactor: {error}\ntranscript tail:\n' + '\n'.join(log[-12:])) from error
    finally:
        timer.cancel()
        proc.wait()


def oracle(binary, args, env):
    run_scenarios(binary, env)
    rng = random.Random(args.seed)
    games, cases = {'quick': (3, 1), 'full': (20, 5), 'stress': (60, 12)}[args.mode]
    for _ in range(games):
        play(binary, env, rng, rng.randint(1, cases))
    print(f'PASS 50-interactive {len(SCENARIOS)} exit scenarios, {games} piped games', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('50-interactive', ['budget-zero', 'budget-exceeded', 'unsigned-sentinel', 'unsigned-sentinel-custom'], oracle))
