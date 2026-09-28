#!/usr/bin/env python3
"""Standalone headers, aggregates, multiple-TU linkage, and optional sanitizer self-tests."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import tempfile

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitizers', action='store_true')
    args = parser.parse_args()
    workspace = ROOT/'99-Workspace/template.cpp'
    if not workspace.is_file():
        raise RuntimeError('Required workspace snapshot missing: '+str(workspace)+'; run 97-Online Testing/03-workspace.py explicitly')
    compiler = os.environ.get('CXX', 'g++')
    headers = sorted(p for d in ROOT.iterdir() if d.is_dir() and d.name[:2] in ('01','02','03','04','05','06','07') for p in d.rglob('*.hpp'))
    flags = ['-std=gnu++20']
    def run(command, timeout=180):
        result = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
        if result.returncode:
            raise RuntimeError('Command failed: '+repr(command)+'\n'+result.stdout+'\n'+result.stderr)
    with tempfile.TemporaryDirectory(prefix='cp-integration-') as name:
        build = Path(name)
        unit = build/'unit.cpp'
        for header in headers:
            unit.write_text(f'#include "{header}"\nint main() {{}}\n')
            run([compiler, *flags, '-fsyntax-only', str(unit)])
        includes = ''.join(f'#include "{p}"\n' for p in headers if p.name == '99-All.hpp')
        a,b = build/'a.cpp',build/'b.cpp'
        a.write_text(includes+'int other(); int main() { return other(); }\n')
        b.write_text(includes+'int other() { return 0; }\n')
        variants = [('scalar', ['-mno-avx2'])]
        cpu = Path('/proc/cpuinfo')
        if cpu.exists() and 'avx2' in cpu.read_text().split(): variants.append(('avx2', ['-mavx2']))
        else: print('SKIP AVX2 integration: hardware support not detected')
        for label,target in variants:
            output = build/f'all-{label}'
            run([compiler,*flags,*target,str(a),str(b),'-o',str(output)])
            run([str(output)])
        for local in ([],['-DLOCAL']):run([compiler,*flags,*local,'-fsyntax-only',str(workspace)])
        if args.sanitizers:
            core = HERE/'01-Core'
            sources = sorted([*core.glob('*_tester.cpp'), *core.glob('05-*_cases.cpp')])
            for source in sources:
                output=build/source.stem
                run([compiler,*flags,'-O1','-g','-fno-omit-frame-pointer','-fno-pie','-no-pie',
                     '-fsanitize=address,undefined','-fno-sanitize-recover=all',str(source),'-o',str(output)])
                run([str(output)],300)
        print(f'PASS: {len(headers)} standalone/aggregate headers, scalar/available AVX2 multi-TU, workspace'+(', sanitizer self-tests' if args.sanitizers else ''))
    return 0

if __name__=='__main__':
    try: raise SystemExit(main())
    except (OSError,subprocess.TimeoutExpired,RuntimeError) as error:
        print('FAIL:',error,file=sys.stderr);raise SystemExit(1)
