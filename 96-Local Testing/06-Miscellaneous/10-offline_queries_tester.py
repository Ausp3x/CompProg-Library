"""Offline sweep order and half-open range counts against independent scans."""
from _00_runner import main


if __name__ == '__main__':
    raise SystemExit(main('10-offline_queries', [
        'negative-left', 'reversed-range', 'right-past-end',
        'empty-past-end', 'left-past-end']))
