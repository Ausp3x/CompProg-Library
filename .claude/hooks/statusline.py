#!/usr/bin/env python3
"""Status line: model, effort, context against the auto-compact window, prompt cache, the experiment id, plan usage,
session cost (a list-price estimate), `audit N new` while the weekly audit has unreviewed findings, and the harness
warnings of harness.py, which also keeps the plan usage for the audit's canary. Reads Claude Code's status JSON on
stdin; a missing field is left out."""
import datetime
import json
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))


def field(d, keys):
    for k in keys:
        d = d.get(k) if isinstance(d, dict) else None
    return d


def clock(value):
    """Local HH:MM of an epoch (s or ms) or ISO timestamp, or ''."""
    try:
        if isinstance(value, (int, float)):
            return f'{datetime.datetime.fromtimestamp(value / 1000 if value > 1e11 else value):%H:%M}'
        return f'{datetime.datetime.fromisoformat(str(value).replace("Z", "+00:00")).astimezone():%H:%M}'
    except (TypeError, ValueError, OverflowError, OSError):
        return ''


def main():
    d = json.load(sys.stdin)
    parts = [field(d, ('model', 'display_name')) or field(d, ('model', 'id')) or '?']
    if field(d, ('effort', 'level')):
        parts.append(field(d, ('effort', 'level')))
    use = field(d, ('context_window', 'current_usage')) or {}
    ctx = sum(use.get(k) or 0 for k in ('input_tokens', 'cache_creation_input_tokens', 'cache_read_input_tokens'))
    try:
        root = Path(field(d, ('workspace', 'project_dir')) or '.')
        window = json.loads((root / '.claude/settings.json').read_text()).get('autoCompactWindow')
    except (OSError, ValueError):
        window = None
    window = window or field(d, ('context_window', 'context_window_size'))
    parts.append(f'ctx {ctx // 1000}k' + (f'/{window // 1000}k' if window else ''))
    cache = d.get('prompt_cache') or {}
    if cache:
        until = clock(cache.get('expires_at'))
        parts.append(f'cache {cache.get("ttl") or ""} ' + (('warm to ' + until if until else 'warm') if cache.get('warm') else 'cold'))
    try:
        import harness
        exp, warnings = harness.status(d)
    except Exception:  # The warnings must not take the status line down with them.
        exp, warnings = '', ['harness error']
    parts.append(exp)
    for label, key in (('5h', 'five_hour'), ('7d', 'seven_day')):
        pct = field(d, ('rate_limits', key, 'used_percentage'))
        if isinstance(pct, (int, float)):
            parts.append(f'{label} {pct:.0f}%')
    cost = field(d, ('cost', 'total_cost_usd'))
    if isinstance(cost, (int, float)):
        parts.append(f'${cost:.2f}')
    try:
        harness.record_usage(d.get('rate_limits'))
    except Exception:  # A full disk must not hide the audit label or take the status line down.
        pass
    try:
        parts.append(harness.audit_label())
    except Exception:
        pass
    parts += [f'⚠ {w}' for w in warnings]
    print(' · '.join(p.replace('  ', ' ').strip() for p in parts if p))


if __name__ == '__main__':
    try:
        main()
    except Exception:  # A broken status line must not break the session.
        print('status line error')
