# Contest testing sources

Protocol references only; no code was imported. Decisions are in [00-notes.md](00-notes.md).

| Source | Used for |
|---|---|
| [Python subprocess documentation](https://docs.python.org/3/library/subprocess.html) | process lifecycle, timeouts, pipe cleanup (P001, 2026-09-27) |
| [testlib.h](https://github.com/MikeMirzayanov/testlib) | generator/validator/interactor/checker families, `registerValidation` input validator (P001, 2026-09-27 and 2026-10-06) |
| [Kattis problem package format](https://www.kattis.com/problem-package-format/spec/legacy.html) | input validators, `data/sample/*.in/.ans`, float tolerance and limits (2026-10-06 sweep) |
| [online-judge-tools `oj test`](https://raw.githubusercontent.com/online-judge-tools/oj/master/onlinejudge_command/subcommand/test.py), [getting started](https://github.com/online-judge-tools/oj/blob/master/docs/getting-started.md), [`generate-input`](https://raw.githubusercontent.com/online-judge-tools/oj/master/onlinejudge_command/subcommand/generate_input.py) | expected-answer sample tests, `-e`, `--mle`, generator commands (2026-10-06 sweep) |
| [cf-tool](https://github.com/xalanq/cf-tool), [CPH](https://github.com/agrawal-d/cph) | stored input/answer sample testing (2026-10-06 sweep) |
| [PyRival stress tester](https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/tools/stress_tester.py), [interactive runner](https://raw.githubusercontent.com/cheran-senthil/PyRival/master/pyrival/tools/interactive_runner.py) | comparison of stress/interactive protocols (2026-10-06 sweep) |
