#!/usr/bin/env python3
"""Start Claude Code without cached experiment ids (user decision 2026-11-11): remove `atis` and `experimentKey` from
every experiment cache slot of the global config (.claude.json, the one harness.py reads), then run `claude` with the
given arguments. It writes under the client's own lock (`.claude.json.lock`, a directory). It writes nothing, warns
and still launches when another Claude Code process holds that lock for over two seconds, the file is unreadable or a
symbolic link, or its format is no longer the audited one (harness.slot_format, or an id also stored outside those two fields); a
format warning or an internal error asks for a re-audit of the harness. The server can assign the ids again on its
next fetch, and the SUP28 strip test could not show any effect (00-Guidelines/13-Plan/Docs/00-harness-audit.md,
Strip test). It prints only how many ids it removed or why it removed none.

Usage: python3 .claude/launch.py [claude arguments]   e.g. python3 .claude/launch.py --agent contest-session --effort high"""
import json
import os
import sys
import time
from pathlib import Path

KEYS = ('atis', 'experimentKey')
WAIT = 2.0


def strip(path, harness):
    """(ids removed, None), or (None, why) when nothing was written."""
    if not path.is_file():
        return 0, None
    if path.is_symlink():  # replacing it would leave the ids in its target
        return None, 'the global config is a symbolic link'
    redo = f'; the launcher is inactive until the harness is re-audited ({harness.REAUDIT})'
    lock = path.with_name(path.name + '.lock')
    end = time.monotonic() + WAIT
    while True:
        try:
            os.mkdir(lock)
            break
        except FileExistsError:
            if time.monotonic() > end:
                return None, 'another Claude Code process held the global config lock'
            time.sleep(0.05)
        except OSError:
            return None, 'the global config lock could not be created'
    try:
        try:
            mode = path.stat().st_mode & 0o777
            data = json.loads(path.read_text(encoding='utf-8'))
        except OSError:
            return None, 'the global config is unreadable'
        except ValueError:
            return None, 'the global config is not JSON' + redo
        drift = harness.slot_format(data) if isinstance(data, dict) else ['the global config is not a JSON object']
        if drift:
            return None, '; '.join(drift) + redo
        gone = [v for slot in (data.get('clientDataCacheSlots') or {}).values() for k in KEYS if (v := slot['data'].pop(k, None)) is not None]
        if any(v and v in s for s in harness.strings(data) for v in gone):
            return None, 'an experiment id is also stored outside atis and experimentKey' + redo
        removed = sum(1 for v in gone if v)  # an empty atis is no id
        if removed:
            try:  # raw UTF-8 as the client writes it, escaped where a lone surrogate cannot be encoded
                out = json.dumps(data, indent=2, ensure_ascii=False).encode('utf-8')
            except UnicodeEncodeError:
                out = json.dumps(data, indent=2).encode('ascii')
            tmp = path.with_name(f'{path.name}.strip.{os.getpid()}')
            try:
                fd = os.open(tmp, os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
                with os.fdopen(fd, 'wb') as f:
                    f.write(out)
                    f.flush()
                    os.fsync(f.fileno())
                os.chmod(tmp, mode)
                os.replace(tmp, path)
            except OSError:
                tmp.unlink(missing_ok=True)
                return None, 'the global config could not be written'
        return removed, None
    finally:
        try:
            os.rmdir(lock)
        except OSError:
            pass


def main(argv):
    try:
        sys.path.insert(0, str(Path(__file__).resolve().parent / 'hooks'))
        import harness
        n, why = strip(harness.global_config(), harness)
        msg = why and f'ids not stripped: {why}' or f'{n} cached experiment ids removed'
    except Exception as exc:  # the launcher never blocks the launch
        msg = f'ids not stripped after an internal error ({type(exc).__name__}); re-audit the harness (package P287)'
    print('launch: ' + msg, file=sys.stderr)
    try:
        os.execvp('claude', ['claude', *argv])
    except OSError as exc:
        print(f'launch: cannot run claude: {exc.strerror}', file=sys.stderr)
        return 127


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
