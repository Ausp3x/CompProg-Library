"""Stable integer sorts, partition and selection against independent references."""
import os
from pathlib import Path
import subprocess
import tempfile
from _00_runner import main


def compile_rejections(binary, args, env):
    if binary.name != 'optimized' or args.mode == 'quick':
        return
    root = Path(__file__).resolve().parents[2]
    header = root / '06-Miscellaneous/11-sorting_selection.hpp'
    cases = {
        'counting-bool': 'vector<bool> a; countingSort(a, false, true);',
        'counting-real': 'vector<double> a; countingSort(a, 0.0, 1.0);',
        'radix-bool': 'vector<bool> a; radixSort(a);',
        'radix-real': 'vector<double> a; radixSort(a);',
        'stable-counting-bool-key': 'vector<int> a; stableCountingSort(a, 2, [](int x) { return bool(x); });',
        'stable-counting-real-key': 'vector<int> a; stableCountingSort(a, 2, [](int x) { return double(x); });',
    }
    with tempfile.TemporaryDirectory(prefix='cp-sorting-rejections-') as directory:
        for label, body in cases.items():
            source = Path(directory) / (label + '.cpp')
            source.write_text(f'#include "{header}"\nint main() {{ {body} }}\n')
            command = [os.environ.get('CXX', 'g++'), '-std=gnu++20', '-fsyntax-only', str(source)]
            result = subprocess.run(command, capture_output=True, text=True, env=env, timeout=180)
            if result.returncode == 0:
                raise RuntimeError(f'compile rejection={label} expected=nonzero actual=0 command={command}')
            if not any(marker in result.stderr for marker in ('static assertion', 'constraints', 'make_unsigned')):
                raise RuntimeError(f'compile rejection={label} failed for unexpected reason command={command}\n{result.stderr}')
    print(f'PASS integer-domain compile rejections={len(cases)}', flush=True)


if __name__ == '__main__':
    raise SystemExit(main('11-sorting_selection', [
        'count-reverse', 'count-below', 'count-above', 'count-width', 'count-wide-width',
        'alphabet-negative', 'alphabet-zero', 'key-negative', 'key-past-end',
        'key-wide', 'select-negative', 'select-past-end', 'select-empty',
        'median-negative', 'median-past-end', 'median-empty'], compile_rejections))
